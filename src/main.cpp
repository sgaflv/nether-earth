#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "audio.h"
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

/*                      GLOBAL VARIABLES INITIALIZATION:                            */

int SCREEN_X=640;
int SCREEN_Y=480;
int COLOUR_DEPTH=32;

int shadows=1;
int detaillevel=4;

bool sound=true;

int up_key=KEY_Q;
int down_key=KEY_A;
int left_key=KEY_O;
int right_key=KEY_P;
int fire_key=KEY_SPACE;
int pause_key=KEY_F1;

int level=1;

int mainmenu_status=0;
int mainmenu_substatus=0;
int mainmenu_selection=0;

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

/*                      AUXILIAR FUNCTION DEFINITION:                           */


void pause(unsigned int time)
{
    unsigned int initt=platform_ticks();

    while((platform_ticks()-initt)<time);
}

/* pause */


bool initialization(void)
{
    if(!platform_init())
        return false;

    if(!platform_create_window(SCREEN_X,SCREEN_Y,fullscreen))
        return false;

    platform_log_gl_info();

    pause(400);

    if(!audio_init())
        return false;

    platform_show_cursor(0);

    return true;
}

/* initialization */


void finalization()
{
    audio_shutdown();

    platform_destroy_window();

    platform_shutdown();
}

/* finalization */


bool apply_video_settings(bool fs)
{
    if(game!=0)
        game->refresh_display_lists();

    if(nethertittle!=0)
        nethertittle->refresh_display_lists();

    if(game!=0)
        game->deleteobjects();

    platform_destroy_window();

    if(!platform_create_window(SCREEN_X,SCREEN_Y,fs))
        return false;

    platform_log_gl_info();

    if(game!=0)
        game->loadobjects();

    return true;
}

/* apply_video_settings */


void swap_buffers(void)
{
    platform_swap_buffers();
}

/* swap_buffers */


void get_render_size(int &w,int &h)
{
    platform_get_drawable_size(&w,&h);
}

/* get_render_size */


/* ------------------------------------------------------------------ */
/* Stroke font helpers for debug / test modes.                        */
/* ------------------------------------------------------------------ */


/* Small helper to draw a text string at a pixel position, with a     */
/* given scale factor. The pen is repositioned for each character.   */

static void draw_stroke_text(
    const char *s,
    float x,
    float y,
    float scale)
{
    glMatrixMode(GL_MODELVIEW);

    glPushMatrix();

    glTranslatef(x,y,0.0f);
    glScalef(scale,scale,1.0f);

    while(*s) {

        stroke_char(*s);

        glTranslatef(STROKE_ADVANCE,0.0f,0.0f);

        s++;
    }

    glPopMatrix();
}


/* ------------------------------------------------------------------ */
/* Font test mode --fonttest                                          */
/*                                                                    */
/* Renders every printable ASCII character in the stroke font, each   */
/* in its own grid cell with its ASCII code printed below.  Exits     */
/* only on Escape.  While running, the most recently pressed key      */
/* name is displayed at the bottom.                                   */
/* ------------------------------------------------------------------ */


static void font_test_draw(int w,int h,const char *lastkey)
{
    int i;

    int first=32;
    int last=126;

    int total=last-first+1;

    int margin=16;
    int cols=16;

    int rows=(total+cols-1)/cols;

    int label_h=64;

    int cellw=(w-2*margin)/cols;
    int cellh=(h-2*margin-label_h)/rows;

    if(cellw<1)
        cellw=1;

    if(cellh<1)
        cellh=1;

    /* Glyph scale: fit cap height (~119 native) and advance (~104) inside cell. */

    float sx=(cellw*0.8f)/STROKE_ADVANCE;
    float sy=(cellh*0.6f)/119.0f;

    float scale=(sx<sy?sx:sy);

    if(scale<=0.0f)
        scale=1.0f;

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);

    glClearColor(0.05f,0.05f,0.08f,1.0f);

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);

    glLoadIdentity();

    glOrtho(
        0,
        (GLdouble)w,
        0,
        (GLdouble)h,
        -1,
        1
    );

    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();


    /* Title + last-pressed info. */

    glColor3f(1.0f,1.0f,1.0f);

    if(lastkey && lastkey[0]) {
        draw_stroke_text(
            "FONT TEST - ESC to exit  Last key:",
            margin,
            h-margin-label_h+32,
            0.45f
        );

        glColor3f(1.0f,0.8f,0.0f);

        draw_stroke_text(
            lastkey,
            margin+520.0f,
            h-margin-label_h+32,
            0.55f
        );
    }
    else {
        draw_stroke_text(
            "FONT TEST - ESC to exit  Last key: (none)",
            margin,
            h-margin-label_h+32,
            0.45f
        );
    }


    for(i=0;i<total;i++) {

        int col=i%cols;
        int row=i/cols;

        int code=first+i;

        float cx=margin+(float)col*cellw;
        float cy=h-margin-label_h-(float)(row+1)*cellh;


        /* Cell border (dim). */

        glColor3f(0.25f,0.25f,0.30f);

        {
            float v[4*2] = {
                cx+1,          cy+1,
                cx+cellw-1,    cy+1,
                cx+cellw-1,    cy+cellh-1,
                cx+1,          cy+cellh-1
            };

            glEnableClientState(GL_VERTEX_ARRAY);

            glVertexPointer(
                2,
                GL_FLOAT,
                0,
                v
            );

            glDrawArrays(
                GL_LINE_LOOP,
                0,
                4
            );

            glDisableClientState(GL_VERTEX_ARRAY);
        }


        if(code==32) {

            /* Space glyph has no strokes; just paint a faint block so it is visible. */

            glColor3f(0.15f,0.15f,0.20f);

            {
                float v[4*2] = {
                    cx+4,          cy+4,
                    cx+cellw-4,    cy+4,
                    cx+cellw-4,    cy+cellh-4,
                    cx+4,          cy+cellh-4
                };

                glEnableClientState(GL_VERTEX_ARRAY);

                glVertexPointer(
                    2,
                    GL_FLOAT,
                    0,
                    v
                );

                glDrawArrays(
                    GL_TRIANGLE_FAN,
                    0,
                    4
                );

                glDisableClientState(GL_VERTEX_ARRAY);
            }
        }


        /* The glyph: centered horizontally on the baseline. */

        glColor3f(1.0f,0.9f,0.3f);

        glMatrixMode(GL_MODELVIEW);

        glPushMatrix();

        glTranslatef(
            cx+cellw*0.5f,
            cy+cellh*0.2f,
            0.0f
        );

        glScalef(scale,scale,1.0f);

        glTranslatef(
            -STROKE_ADVANCE*0.5f,
            0.0f,
            0.0f
        );

        stroke_char(code);

        glPopMatrix();


        /* ASCII code label under the glyph. */

        char lbl[16];

        sprintf(lbl,"%d",code);

        glColor3f(0.5f,0.8f,1.0f);

        draw_stroke_text(
            lbl,
            cx+cellw*0.5f-14.0f,
            cy+4.0f,
            0.25f
        );
    }
}


/* Run the font test mode until Escape is pressed (or the window closes). */

void font_test(void)
{
    int w,h;
    bool quit=false;
    char lastkey[64]="";

    while(!quit) {

        platform_pump_input();

        if(platform_quit_requested())
            quit=true;

        /* Exit only on the physical Escape; track the last edge-      */
        /* detected key name.  Gamepad buttons that alias ESC must not  */
        /* quit this mode.                                              */
        {
            unsigned int nk=platform_num_keys();
            const unsigned char *kb=platform_get_keyboard_state();
            const unsigned char *okb=platform_get_old_keyboard_state();
            unsigned int k;

            if(platform_key_pressed_raw(KEY_ESCAPE))
                quit=true;

            for(k=0;k<nk;k++) {
                if(kb[k] && !okb[k] && k!=(unsigned)KEY_ESCAPE)
                    snprintf(lastkey,sizeof(lastkey),"%s",platform_key_name(k));
            }
        }

        get_render_size(w,h);

        font_test_draw(w,h,lastkey);

        swap_buffers();

        platform_sleep(16);
    }
}


/* font_test */


/* ------------------------------------------------------------------ */
/* Key test mode --keytest                                            */
/*                                                                    */
/* Displays the real-time keyboard state so every held key can be     */
/* visually confirmed.  Used to validate input handling, including    */
/* gamepad mapping in the future.  Exits only on Escape.              */
/* ------------------------------------------------------------------ */


/* Describes a single entry in the key test grid. The label is copied   */
/* into the entry because platform_key_name() returns a pointer to a    */
/* shared static buffer.                                                */

struct KeyTestEntry {
    int  key_id;
    char label[16];
};


/* Fill one table entry: key id plus a copy of its short display name. */

static void key_test_set_entry(KeyTestEntry *e,int key_id)
{
    e->key_id=key_id;
    snprintf(e->label,sizeof(e->label),"%s",platform_key_name(key_id));
}


/* Build the table of keys to display. The table is static (size fixed  */
/* at compile time) and covers the full KEY_* range without the special */
/* control keys (LSHIFT/RSHIFT/etc.) which are less interesting to     */
/* visualise.  With 'gamepad_only' set the table lists just the virtual */
/* gamepad controls, for the --testpad mode.                            */

static int key_test_build_table(KeyTestEntry *out,int max,int gamepad_only)
{
    int n=0;
    int i;

    if(!gamepad_only) {

    /* Letters. */
    for(i=KEY_A;i<=KEY_Z && n<max;i++) {
        key_test_set_entry(&out[n],i);
        n++;
    }
    /* Digits. */
    for(i=KEY_0;i<=KEY_9 && n<max;i++) {
        key_test_set_entry(&out[n],i);
        n++;
    }
    /* Space. */
    if(n<max) {
        key_test_set_entry(&out[n],KEY_SPACE);
        n++;
    }
    /* Enter / Return. */
    if(n<max) {
        key_test_set_entry(&out[n],KEY_RETURN);
        n++;
    }
    /* Arrow keys. */
    if(n<max) {
        key_test_set_entry(&out[n],KEY_UP);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_DOWN);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_LEFT);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_RIGHT);
        n++;
    }
    /* F keys. */
    for(i=KEY_F1;i<=KEY_F12 && n<max;i++) {
        key_test_set_entry(&out[n],i);
        n++;
    }
    /* Page Up / Page Down. */
    if(n<max) {
        key_test_set_entry(&out[n],KEY_PAGEUP);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_PAGEDOWN);
        n++;
    }
    /* Tab / Backspace / Delete / Insert / Home / End. */
    if(n<max) {
        key_test_set_entry(&out[n],KEY_TAB);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_BACKSPACE);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_DELETE);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_INSERT);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_HOME);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_END);
        n++;
    }
    /* Modifier keys. */
    if(n<max) {
        key_test_set_entry(&out[n],KEY_LSHIFT);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_RSHIFT);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_LCTRL);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_RCTRL);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_LALT);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_RALT);
        n++;
    }
    /* KP keys. */
    if(n<max) {
        key_test_set_entry(&out[n],KEY_KP_0);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_KP_1);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_KP_PLUS);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_KP_MINUS);
        n++;
    }
    }

    /* Gamepad buttons. */
    if(n<max) {
        key_test_set_entry(&out[n],KEY_GAMEPAD_A);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_GAMEPAD_B);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_GAMEPAD_X);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_GAMEPAD_Y);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_GAMEPAD_START);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_GAMEPAD_BACK);
        n++;
    }
    /* Gamepad shoulders / triggers. */
    if(n<max) {
        key_test_set_entry(&out[n],KEY_GAMEPAD_LB);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_GAMEPAD_RB);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_GAMEPAD_LT);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_GAMEPAD_RT);
        n++;
    }
    /* Gamepad D-pad. */
    if(n<max) {
        key_test_set_entry(&out[n],KEY_GAMEPAD_DPAD_UP);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_GAMEPAD_DPAD_DOWN);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_GAMEPAD_DPAD_LEFT);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_GAMEPAD_DPAD_RIGHT);
        n++;
    }
    /* Gamepad sticks. */
    if(n<max) {
        key_test_set_entry(&out[n],KEY_GAMEPAD_STICK_UP);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_GAMEPAD_STICK_DOWN);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_GAMEPAD_STICK_LEFT);
        n++;
    }
    if(n<max) {
        key_test_set_entry(&out[n],KEY_GAMEPAD_STICK_RIGHT);
        n++;
    }

    return n;
}


static void key_test_draw(
    int w,
    int h,
    const KeyTestEntry *table,
    int count,
    const char *title)
{
    int i;

    int margin=16;
    int cols=6;
    int rows=(count+cols-1)/cols;
    int label_h=48;

    int cellw=(w-2*margin)/cols;
    int cellh=(h-2*margin-label_h)/rows;

    if(cellw<1) cellw=1;
    if(cellh<1) cellh=1;

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);

    glClearColor(0.06f,0.06f,0.09f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,(GLdouble)w,0,(GLdouble)h,-1,1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* Title. */

    glColor3f(0.6f,0.6f,0.9f);
    draw_stroke_text(title,margin,h-margin-label_h+28,0.50f);

    /* Count held. */

    {
        const unsigned char *kb=platform_get_keyboard_state();
        int held=0;

        for(i=0;i<count;i++) {
            if(kb[table[i].key_id])
                held++;
        }

        glColor3f(0.5f,0.75f,0.5f);
        draw_stroke_text(
            (held>0 ? "Held:" : "Held: (none)"),
            margin,
            h-margin-label_h+4,
            0.35f
        );

        if(held>0) {
            char buf[256]="";
            int pos=0;

            for(i=0;i<count && pos<240;i++) {
                if(kb[table[i].key_id]) {
                    if(pos>0) { buf[pos++]=' '; buf[pos++]=' '; }
                    int len=strlen(table[i].label);
                    memcpy(buf+pos,table[i].label,len);
                    pos+=len;
                }
            }
            buf[pos]=0;

            glColor3f(1.0f,0.85f,0.3f);
            draw_stroke_text(buf,margin+160.0f,h-margin-label_h+4,0.35f);
        }
    }

    /* Controller status. */

    {
        char padbuf[64];

        platform_gamepad_name(padbuf,sizeof(padbuf));

        glColor3f(
            platform_gamepad_connected() ? 0.5f : 0.4f,
            platform_gamepad_connected() ? 0.8f : 0.4f,
            platform_gamepad_connected() ? 0.5f : 0.4f
        );

        if(platform_gamepad_connected()) {
            draw_stroke_text("Gamepad:",margin,h-margin-label_h-16,0.35f);
            glColor3f(0.9f,0.9f,0.6f);
            draw_stroke_text(padbuf,margin+150.0f,h-margin-label_h-16,0.35f);
        }
        else {
            draw_stroke_text("No gamepad connected",margin,h-margin-label_h-16,0.35f);
        }
    }

    /* Key grid. */

    for(i=0;i<count;i++) {

        int col=i%cols;
        int row=i/cols;

        float cx=margin+(float)col*cellw;
        float cy=h-margin-label_h-(float)(row+1)*cellh;

        const unsigned char *kb=platform_get_keyboard_state();
        int pressed=kb[table[i].key_id];

        /* Cell background / border. */

        if(pressed) {
            glColor3f(0.30f,0.45f,0.70f);
        }
        else {
            glColor3f(0.15f,0.15f,0.20f);
        }

        {
            float v[4*2] = {
                cx+1,   cy+1,
                cx+cellw-1, cy+1,
                cx+cellw-1, cy+cellh-1,
                cx+1,   cy+cellh-1
            };

            glEnableClientState(GL_VERTEX_ARRAY);
            glVertexPointer(2,GL_FLOAT,0,v);
            glDrawArrays(GL_TRIANGLE_FAN,0,4);
            glDisableClientState(GL_VERTEX_ARRAY);
        }

        /* Cell border. */

        glColor3f(pressed ? 0.7f : 0.30f,
                  pressed ? 0.8f : 0.30f,
                  pressed ? 1.0f : 0.35f);

        {
            float v[4*2] = {
                cx+1,   cy+1,
                cx+cellw-1, cy+1,
                cx+cellw-1, cy+cellh-1,
                cx+1,   cy+cellh-1
            };

            glEnableClientState(GL_VERTEX_ARRAY);
            glVertexPointer(2,GL_FLOAT,0,v);
            glDrawArrays(GL_LINE_LOOP,0,4);
            glDisableClientState(GL_VERTEX_ARRAY);
        }

        /* Key label. */

        {
            const char *label=table[i].label;
            int len=strlen(label);
            float charw=STROKE_ADVANCE*0.30f;
            float textw=len*charw;
            float label_scale=0.30f;

            if(textw>cellw-8.0f)
                label_scale=(cellw-8.0f)/textw*0.30f;

            float sx2=cx+cellw*0.5f-textw*0.5f;
            float sy2=cy+cellh*0.5f;

            glColor3f(pressed ? 1.0f : 0.75f,
                      pressed ? 1.0f : 0.75f,
                      pressed ? 1.0f : 0.85f);

            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glTranslatef(sx2,sy2,0.0f);
            glScalef(label_scale,label_scale,1.0f);
            {
                const char *p=label;
                while(*p) {
                    stroke_char(*p);
                    glTranslatef(STROKE_ADVANCE,0.0f,0.0f);
                    p++;
                }
            }
            glPopMatrix();
        }
    }
}


static void key_test_mode(int gamepad_only)
{
    int w,h;
    bool quit=false;

    /* Pre-build the display table once. */
    KeyTestEntry table[KEY_COUNT];
    int count=key_test_build_table(
        table,KEY_COUNT,gamepad_only);

    while(!quit) {

        platform_pump_input();

        if(platform_quit_requested())
            quit=true;

        /* Exit on the physical Escape key only (gamepad buttons that   */
        /* alias ESC must not quit the test).                           */
        {
            static bool prev_esc=false;
            bool esc=platform_key_pressed_raw(KEY_ESCAPE)!=0;

            if(esc && !prev_esc)
                quit=true;
            prev_esc=esc;
        }

        get_render_size(w,h);

        key_test_draw(
            w,h,table,count,
            gamepad_only ? "TESTPAD - ESC to exit"
                         : "KEY TEST - ESC to exit");

        swap_buffers();

        platform_sleep(16);
    }
}


void key_test(void)
{
    key_test_mode(0);
}


void pad_test(void)
{
    key_test_mode(1);
}


/* key_test */


int main(int argc,char** argv)
{

    int time,act_time;

    int w,h;

    bool quit=false;


    load_configuration();

    /* The controller synthesizes whatever keys the user bound to the   */
    /* in-game actions, so remapping keys changes the gamepad too.      */
    platform_set_gamepad_mapping(
        up_key,down_key,left_key,right_key,fire_key,pause_key);

    if(!initialization())
        return 0;


    /* Debug modes triggered from the command line:                     */
    /*   --fonttest / -t  : render the whole stroke font grid; ESC only.*/
    /*   --keytest / -k   : real-time keyboard state; ESC only.         */
    /*   --testpad / -g   : real-time gamepad state; ESC only.          */
    /* All are mutually exclusive and never affect normal play.         */

    {
        bool do_fonttest=false;
        bool do_keytest=false;
        bool do_testpad=false;

        for(int i=1;i<argc;i++) {

            if(
                strcmp(argv[i],"--fonttest")==0 ||
                strcmp(argv[i],"-t")==0
            )
                do_fonttest=true;

            if(
                strcmp(argv[i],"--keytest")==0 ||
                strcmp(argv[i],"-k")==0
            )
                do_keytest=true;

            if(
                strcmp(argv[i],"--testpad")==0 ||
                strcmp(argv[i],"-g")==0
            )
                do_testpad=true;
        }

        if(do_fonttest) {

            font_test();

            finalization();

            return 0;
        }

        if(do_keytest) {

            key_test();

            finalization();

            return 0;
        }

        if(do_testpad) {

            pad_test();

            finalization();

            return 0;
        }
    }


    time=init_time=platform_ticks();


    while(!quit) {

        platform_pump_input();

        {
            const unsigned char *kb=platform_get_keyboard_state();
            const unsigned char *okb=platform_get_old_keyboard_state();

            if(platform_quit_requested())
                quit=true;

            /* F12 to quit: */
            if(kb[KEY_F12] && !okb[KEY_F12])
                quit=true;

            /* Alt+Enter toggles fullscreen: */
            if(kb[KEY_RETURN] && !okb[KEY_RETURN] &&
               (kb[KEY_LALT] || kb[KEY_RALT])) {

                /* Toggle FULLSCREEN mode: */

                if(fullscreen)
                    fullscreen=false;
                else
                    fullscreen=true;

                if(!apply_video_settings(fullscreen))
                    quit=true;
            }
        }


        get_render_size(w,h);

        act_time=platform_ticks();


        if(act_time-time>=REDRAWING_PERIOD) {

            frames_per_sec_tmp+=1;

            if((act_time-init_time)>=1000) {

                frames_per_sec=frames_per_sec_tmp;

                frames_per_sec_tmp=0;

                init_time=act_time;
            }


            do {

                time+=REDRAWING_PERIOD;

                if((act_time-time)>50*REDRAWING_PERIOD)
                    time=act_time;


                if(game!=0) {

                    if(!game->gamecycle(w,h)) {

                        delete game;

                        game=0;

                        mainmenu_status=0;
                        mainmenu_substatus=0;
                        mainmenu_selection=0;
                    }
                }
                else {

                    int val=mainmenu_cycle(w,h);

                    if(val==1) {

                        char tmp[256];

                        sprintf(tmp,"assets/maps/%s",mapname);

                        game=new NETHER(tmp);
                    }


                    if(val==2)
                        quit=true;


                    if(val==3) {

                        if(!apply_video_settings(fullscreen))
                            quit=true;
                    }
                }


                act_time=platform_ticks();

            } while(act_time-time>=REDRAWING_PERIOD);


            if(game!=0) {

                game->gameredraw(w,h);
            }
            else {

                mainmenu_draw(w,h);
            }
        }
    }


    delete game;

    finalization();

    return 0;
}
