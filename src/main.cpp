#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>
#else
#include <sys/time.h>
#include <time.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include "SDL.h"
#include "SDL_mixer.h"
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "list.h"
#include "vector.h"

#include "cmc.h"
#include "3dobject.h"
#include "shadow3dobject.h"
#include "piece3dobject.h"
#include "nether.h"

static SDL_Window *window=0;
static SDL_GLContext glcontext=0;

/*						GLOBAL VARIABLES INITIALIZATION:							*/ 

int SCREEN_X=640;
int SCREEN_Y=480;

int COLOUR_DEPTH=32;
int shadows=1;
int detaillevel=4;
bool sound=true;
int up_key=SDL_SCANCODE_Q,down_key=SDL_SCANCODE_A,left_key=SDL_SCANCODE_O,right_key=SDL_SCANCODE_P,fire_key=SDL_SCANCODE_SPACE,pause_key=SDL_SCANCODE_F1;
int level=1;
int mainmenu_status=0;
int mainmenu_substatus=0;
bool fullscreen=false;
bool show_radar=true;
char mapname[128]="original.map";
C3DObject *nethertittle=0;

/* DRAWING REGION AROUND THE SHIP: */ 
float MINY=-8,MAXY=8,MINX=-8,MAXX=8;

/* Redrawing constant: */ 
const int REDRAWING_PERIOD=20;

/* Frames per second counter: */ 
int frames_per_sec=0;
int frames_per_sec_tmp=0;
int init_time=0;


/* Surfaces: */ 

NETHER *game=0;

void save_configuration(void);
void load_configuration(void);
int mainmenu_cycle(int width,int height);
void mainmenu_draw(int width,int height);

/*						AUXILIAR FUNCTION DEFINITION:							*/ 


void pause(unsigned int time)
{
	unsigned int initt=SDL_GetTicks();

	while((SDL_GetTicks()-initt)<time);
} /* pause */ 


bool initialization(Uint32 windowflags) 
{
    Uint32 flags=windowflags;

    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)<0) {
        fprintf(stderr,"Video initialization failed: %s\n",SDL_GetError());
		return false;
    } /* if */ 

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE,8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,1);

	window=SDL_CreateWindow("Nether Earth REMAKE v0.52",
                            SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
                            SCREEN_X,SCREEN_Y,
                            SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|flags);
    if(window==0) {
        fprintf(stderr,"Video mode set failed: %s\n",SDL_GetError());
		return false;
    } /* if */ 

	glcontext=SDL_GL_CreateContext(window);
	if (glcontext==0) {
		fprintf(stderr,"GL context creation failed: %s\n",SDL_GetError());
		return false;
	} /* if */ 

	pause(400);
	if (Mix_OpenAudio(22050, AUDIO_S16, 2, 1024)) {
		return false;
	} /* if */ 

	SDL_ShowCursor(SDL_DISABLE);

	return true;
} /* initialization */ 


void finalization()
{
	Mix_CloseAudio();
	if (glcontext) SDL_GL_DeleteContext(glcontext);
	if (window) SDL_DestroyWindow(window);
	SDL_Quit();
} /* finalization */


bool apply_video_settings(bool fs)
{
	if (game!=0) game->refresh_display_lists();
	if (nethertittle!=0) nethertittle->refresh_display_lists();
	if (game!=0) game->deleteobjects();

	SDL_GL_MakeCurrent(window,NULL);
	SDL_GL_DeleteContext(glcontext);
	glcontext=0;
	SDL_DestroyWindow(window);
	window=0;

	window=SDL_CreateWindow("Nether Earth REMAKE v0.52",
	                        SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
	                        SCREEN_X,SCREEN_Y,
	                        SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|(fs ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0));
	if (window==0) {
		fprintf(stderr,"Video mode set failed: %s\n",SDL_GetError());
		return false;
	} /* if */

	glcontext=SDL_GL_CreateContext(window);
	if (glcontext==0) {
		fprintf(stderr,"GL context creation failed: %s\n",SDL_GetError());
		return false;
	} /* if */

	if (game!=0) game->loadobjects();
	return true;
} /* apply_video_settings */


void swap_buffers(void)
{
	SDL_GL_SwapWindow(window);
} /* swap_buffers */


void get_render_size(int &w,int &h)
{
	if (window) SDL_GL_GetDrawableSize(window,&w,&h);
	else { w=SCREEN_X; h=SCREEN_Y; }
} /* get_render_size */ 



#ifdef _WIN32
int PASCAL WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPSTR lpCmdLine, int nCmdShow)
{
#else
int main(int argc, char** argv)
{
#endif

	int time,act_time;
	int w,h;
	SDL_Event event;
    bool quit = false;

	glutInit(&argc, argv);

	load_configuration();

	if (!initialization((fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0))) return 0;

	time=init_time=SDL_GetTicks();

	while (!quit) {
		while( SDL_PollEvent( &event ) ) {
            switch( event.type ) {
                /* Keyboard event */
                case SDL_KEYDOWN:
					if (event.key.keysym.sym==SDLK_F12) quit = true;

					if (event.key.keysym.sym==SDLK_RETURN) {
						if ((event.key.keysym.mod & KMOD_ALT)!=0) {
							/* Toogle FULLSCREEN mode: */ 
							if (fullscreen) fullscreen=false;
									   else fullscreen=true;
							if (!apply_video_settings(fullscreen)) quit = true;
						} /* if */ 
					} /* if */ 
                    break;

                /* SDL_QUIT event (window close) */
                case SDL_QUIT:
                    quit = true;
                    break;
            } /* switch */ 
        } /* while */ 

		get_render_size(w,h);
		act_time=SDL_GetTicks();
		if (act_time-time>=REDRAWING_PERIOD)
		{
			frames_per_sec_tmp+=1;
			if ((act_time-init_time)>=1000) {
				frames_per_sec=frames_per_sec_tmp;
				frames_per_sec_tmp=0;
				init_time=act_time;
			} /* if */ 

			do {
				time+=REDRAWING_PERIOD;
				if ((act_time-time)>50*REDRAWING_PERIOD) time=act_time;
			
				if (game!=0) {
					if (!game->gamecycle(w,h)) {
						delete game;
						game=0;
						mainmenu_status=0;
						mainmenu_substatus=0;
					} /* if */  
				} else {
					int val=mainmenu_cycle(w,h);
					if (val==1) {
						char tmp[256];
						sprintf(tmp,"maps/%s",mapname);
						game=new NETHER(tmp);
					} /* if */ 
					if (val==2) quit=true;
					if (val==3) {
						if (!apply_video_settings(fullscreen)) quit=true;
					} /* if */ 
				} /* if */ 
				act_time=SDL_GetTicks();
			} while(act_time-time>=REDRAWING_PERIOD);

			if (game!=0) {
				game->gameredraw(w,h);
			} else {
				mainmenu_draw(w,h);
			} /* if */ 
		} /* if */ 
	} /* while */ 

	delete game;

	finalization();

	return 0;
}









