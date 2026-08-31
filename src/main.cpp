#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>
#else
#include <sys/time.h>
#include <time.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SDL.h"
#include "SDL_mixer.h"
#include "glport.h"

#include "list.h"
#include "vector.h"

#include "cmc.h"
#include "3dobject.h"
#include "shadow3dobject.h"
#include "piece3dobject.h"
#include "nether.h"

#include "glprintf.h"
#include "stroke_font.h"

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


/* ------------------------------------------------------------------ */
/* Font debug/test mode.                                               */
/*                                                                     */
/* Renders every printable ASCII character using the stroke font,     */
/* each in its own grid cell with its ASCII code printed below, so     */
/* broken glyphs are easy to spot. Exits on any key press.             */
/* ------------------------------------------------------------------ */

/* Small helper to draw a text string at a pixel position, with a       */
/* given scale factor. The pen is repositioned for each character.      */
static void draw_stroke_text(const char *s, float x, float y, float scale)
{
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(x,y,0.0f);
	glScalef(scale,scale,1.0f);
	while (*s) {
		stroke_char(*s);
		glTranslatef(STROKE_ADVANCE,0.0f,0.0f);
		s++;
	}
	glPopMatrix();
}

/* Draw one frame of the font test screen. Returns nothing. */
static void font_test_draw(int w,int h)
{
	int i;
	int first=32, last=126;            /* printable ASCII range, inclusive */
	int total=last-first+1;
	int margin=16;
	int cols=16;
	int rows=(total+cols-1)/cols;
	int label_h=64;                     /* space reserved on top for the title */

	int cellw=(w-2*margin)/cols;
	int cellh=(h-2*margin-label_h)/rows;
	if (cellw<1) cellw=1;
	if (cellh<1) cellh=1;

	/* Glyph scale: fit cap height (~119 native) and advance (~104) inside cell. */
	float sx=(cellw*0.8f)/STROKE_ADVANCE;
	float sy=(cellh*0.6f)/119.0f;
	float scale=(sx<sy?sx:sy);
	if (scale<=0.0f) scale=1.0f;

	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_SCISSOR_TEST);

	glClearColor(0.05f,0.05f,0.08f,1.0f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,(GLdouble)w,0,(GLdouble)h,-1,1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	/* Title. */
	glColor3f(1.0f,1.0f,1.0f);
	draw_stroke_text("FONT TEST - press any key to exit", margin, h-margin-label_h, 0.5f);

	for (i=0;i<total;i++) {
		int col=i%cols;
		int row=i/cols;
		int code=first+i;
		float cx=margin+(float)col*cellw;
		float cy=h-margin-label_h-(float)(row+1)*cellh;

		/* Cell border (dim). */
		glColor3f(0.25f,0.25f,0.30f);
		glBegin(GL_LINE_LOOP);
		glVertex2f(cx+1,cy+1);
		glVertex2f(cx+cellw-1,cy+1);
		glVertex2f(cx+cellw-1,cy+cellh-1);
		glVertex2f(cx+1,cy+cellh-1);
		glEnd();

		if (code==32) {
			/* Space glyph has no strokes; just paint a faint block so it is visible. */
			glColor3f(0.15f,0.15f,0.20f);
			glBegin(GL_QUADS);
			glVertex2f(cx+4,cy+4);
			glVertex2f(cx+cellw-4,cy+4);
			glVertex2f(cx+cellw-4,cy+cellh-4);
			glVertex2f(cx+4,cy+cellh-4);
			glEnd();
		}

		/* The glyph: centered horizontally on the baseline. */
		glColor3f(1.0f,0.9f,0.3f);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glTranslatef(cx+cellw*0.5f, cy+cellh*0.2f, 0.0f);
		glScalef(scale,scale,1.0f);
		glTranslatef(-STROKE_ADVANCE*0.5f,0.0f,0.0f);
		stroke_char(code);
		glPopMatrix();

		/* ASCII code label under the glyph. */
		char lbl[16];
		sprintf(lbl,"%d",code);
		glColor3f(0.5f,0.8f,1.0f);
		draw_stroke_text(lbl, cx+cellw*0.5f-14.0f, cy+4.0f, 0.25f);
	}
}

/* Run the font test mode until a key is pressed (or the window closes). */
void font_test(void)
{
	int w,h;
	SDL_Event event;
	bool quit=false;

	while (!quit) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_KEYDOWN:
					quit=true;
					break;
				case SDL_QUIT:
					quit=true;
					break;
				default:
					break;
			}
		}
		get_render_size(w,h);
		font_test_draw(w,h);
		swap_buffers();
		SDL_Delay(16);
	}
} /* font_test */


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


	load_configuration();

	if (!initialization((fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0))) return 0;

	/* Debug mode: dump the whole ASCII font for visual inspection, then  */
	/* exit on any key press. Triggered with --fonttest on the command    */
	/* line (or the first argument), so it never affects normal play.     */
	{
		bool do_fonttest=false;
		for (int i=1;i<argc;i++) {
			if (strcmp(argv[i],"--fonttest")==0 || strcmp(argv[i],"-t")==0)
				do_fonttest=true;
		}
		if (do_fonttest) {
			font_test();
			finalization();
			return 0;
		}
	}

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









