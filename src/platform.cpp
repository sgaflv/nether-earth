/*
 * Platform abstraction - desktop SDL backend.
 *
 * All SDL usage in the project is confined to this file (plus audio.cpp,
 * which wraps SDL_mixer). Game code includes only platform.h / audio.h.
 *
 * A future Android backend will replace this file with one built on a Java
 * Activity + EGL/GLES + AAssetManager, exposing the same platform.h API.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "platform.h"

/* SDL is only used by this backend, never by game code. */
#include <SDL.h>
#include "glport.h"

/* ------------------------------------------------------------------ */
/* Gamepad                                                             */
/* ------------------------------------------------------------------ */

static SDL_GameController *s_controller = 0;
static SDL_JoystickID      s_controller_id = -1;

/* Stick / trigger dead zone: values beyond this count as "pressed".   */
#define GAMEPAD_AXIS_THRESHOLD  10000

/* Open controller 'device_index' if no controller is open yet.        */
static void gamepad_open(int device_index)
{
	SDL_Joystick *joy;

	if (s_controller != 0)
		return;

	s_controller = SDL_GameControllerOpen(device_index);
	if (s_controller == 0)
		return;

	joy = SDL_GameControllerGetJoystick(s_controller);
	s_controller_id = SDL_JoystickInstanceID(joy);
}

/* Close the controller that reported the given instance id.           */
static void gamepad_close_id(SDL_JoystickID id)
{
	SDL_Joystick *joy;

	if (s_controller == 0)
		return;

	joy = SDL_GameControllerGetJoystick(s_controller);
	if (joy != 0 && SDL_JoystickInstanceID(joy) == id) {
		SDL_GameControllerClose(s_controller);
		s_controller = 0;
		s_controller_id = -1;
	}
}

static void gamepad_close(void)
{
	if (s_controller != 0) {
		SDL_GameControllerClose(s_controller);
		s_controller = 0;
		s_controller_id = -1;
	}
}

/* ------------------------------------------------------------------ */
/* Initialisation / shutdown                                           */
/* ------------------------------------------------------------------ */

int platform_init(void)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
		fprintf(stderr, "Video initialization failed: %s\n", SDL_GetError());
		return 0;
	}
	gamepad_open(0);
	return 1;
}

void platform_shutdown(void)
{
	gamepad_close();
	SDL_Quit();
}

/* ------------------------------------------------------------------ */
/* Timing (monotonic)                                                  */
/* ------------------------------------------------------------------ */

unsigned int platform_ticks(void)
{
	return (unsigned int)SDL_GetTicks();
}

void platform_sleep(unsigned int ms)
{
	SDL_Delay(ms);
}

/* ------------------------------------------------------------------ */
/* Window / GL context                                                 */
/* ------------------------------------------------------------------ */

static SDL_Window *s_window = 0;
static SDL_GLContext s_glcontext = 0;

static int s_screen_w = 640;
static int s_screen_h = 480;

int platform_create_window(int w, int h, int fullscreen)
{
	s_screen_w = w;
	s_screen_h = h;

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);

	s_window = SDL_CreateWindow(
		"Nether Earth",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		w,
		h,
		SDL_WINDOW_OPENGL |
		SDL_WINDOW_RESIZABLE |
		(fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0)
	);

	if (s_window == 0)
		return 0;

	s_glcontext = SDL_GL_CreateContext(s_window);

	if (s_glcontext == 0)
		return 0;

	return 1;
}

void platform_destroy_window(void)
{
	if (s_glcontext != 0) {
		SDL_GL_MakeCurrent(s_window, NULL);
		SDL_GL_DeleteContext(s_glcontext);
		s_glcontext = 0;
	}
	if (s_window != 0) {
		SDL_DestroyWindow(s_window);
		s_window = 0;
	}
}

void platform_swap_buffers(void)
{
	if (s_window != 0)
		SDL_GL_SwapWindow(s_window);
}

void platform_get_drawable_size(int *w, int *h)
{
	if (s_window != 0) {
		SDL_GL_GetDrawableSize(s_window, w, h);
	} else {
		*w = s_screen_w;
		*h = s_screen_h;
	}
}

void platform_show_cursor(int show)
{
	SDL_ShowCursor(show ? SDL_ENABLE : SDL_DISABLE);
}

int platform_stencil_bits(void)
{
	int bits = 0;

	if (SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &bits) != 0)
		return 0;

	return bits;
}


void platform_log_gl_info(void)
{
	const unsigned char *glversion = glGetString(GL_VERSION);
	const unsigned char *glrenderer = glGetString(GL_RENDERER);
	const unsigned char *glvendor = glGetString(GL_VENDOR);

	fprintf(stderr, "[glport] GL version : %s\n",
	        glversion ? (const char *)glversion : "(null)");
	fprintf(stderr, "[glport] GL renderer: %s\n",
	        glrenderer ? (const char *)glrenderer : "(null)");
	fprintf(stderr, "[glport] GL vendor  : %s\n",
	        glvendor ? (const char *)glvendor : "(null)");
}

/* ------------------------------------------------------------------ */
/* Input                                                               */
/* ------------------------------------------------------------------ */

/*
 * Keyboard state is kept in three layers:
 *
 *   s_keyboard - physical keyboard, persistent: a held key stays set
 *                until its KEYUP arrives.
 *   s_gamepad  - gamepad state, rebuilt from the controller every frame
 *                (buttons, D-pad, sticks and triggers).
 *   s_merged   - OR of the two, exposed to game code.  s_old_merged is
 *                the previous frame for edge detection.
 *
 * Merging at pump time (not at read time) keeps the returned pointers
 * stable within a frame.
 */

static unsigned char s_keyboard[KEY_COUNT];
static unsigned char s_gamepad[KEY_COUNT];
static unsigned char s_merged[KEY_COUNT];
static unsigned char s_old_merged[KEY_COUNT];
static int s_quit_requested = 0;

/*
 * The configured action keys.  Gamepad movement (D-pad / left stick) and
 * the face buttons synthesize these, so the controller drives whatever
 * keys the user bound in the "REDEFINE KEYBOARD" menu.  Updated via
 * platform_set_gamepad_mapping(); the defaults match the out-of-the-box
 * nether.cfg bindings.
 */

static int s_gpad_up_key       = KEY_UP;
static int s_gpad_down_key     = KEY_DOWN;
static int s_gpad_left_key     = KEY_LEFT;
static int s_gpad_right_key    = KEY_RIGHT;
static int s_gpad_fire_key     = KEY_SPACE;
static int s_gpad_pause_key    = KEY_F1;

/* Gamepad virtual ids must never be used as the aliased config keys,  */
/* or the synthesis would cascade (a button pressing itself).          */
static int is_gamepad_key_id(int key)
{
	return (key >= KEY_GAMEPAD_A && key <= KEY_GAMEPAD_STICK_RIGHT);
}

void platform_set_gamepad_mapping(
	int up,
	int down,
	int left,
	int right,
	int fire,
	int pause_key)
{
	if (!is_gamepad_key_id(up)    && up    >= 0 && up    < KEY_COUNT) s_gpad_up_key    = up;
	if (!is_gamepad_key_id(down)  && down  >= 0 && down  < KEY_COUNT) s_gpad_down_key  = down;
	if (!is_gamepad_key_id(left)  && left  >= 0 && left  < KEY_COUNT) s_gpad_left_key  = left;
	if (!is_gamepad_key_id(right) && right >= 0 && right < KEY_COUNT) s_gpad_right_key = right;
	if (!is_gamepad_key_id(fire)  && fire  >= 0 && fire  < KEY_COUNT) s_gpad_fire_key  = fire;
	if (!is_gamepad_key_id(pause_key)&&pause_key>=0 && pause_key<KEY_COUNT) s_gpad_pause_key = pause_key;
}

int platform_gamepad_connected(void)
{
	return (s_controller != 0);
}

void platform_gamepad_name(char *buf,int buflen)
{
	const char *name;

	if (buf == 0 || buflen <= 0)
		return;

	if (s_controller == 0) {
		snprintf(buf,buflen,"(none)");
		return;
	}

	name = SDL_GameControllerName(s_controller);
	if (name == 0)
		name = "Gamepad";

	snprintf(buf,buflen,"%s",name);
}

/* Set one gamepad key and zero-or-one aliased keyboard key.           */
static void gpad_key(int gamepad_key, int alias_key)
{
	s_gamepad[gamepad_key] = 1;
	if (alias_key >= 0)
		s_gamepad[alias_key] = 1;
}

/* Rebuild the per-frame gamepad state from the currently open       */
/* controller.  Called at the end of every platform_pump_input().     */
static void platform_update_gamepad_state(void)
{
	SDL_GameController *c = s_controller;
	Sint16 lx, ly, rx, ry;

	memset(s_gamepad, 0, sizeof(s_gamepad));

	if (c == 0)
		return;

	/* Face buttons.  A is the primary action (fire/confirm); Y an       */
	/* alternative confirm; START and LB open the in-game menu/options.  */
	/* B and BACK cancel (ESC).  All synthesize the *configured* keys.   */
	if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_A))
		gpad_key(KEY_GAMEPAD_A, s_gpad_fire_key);
	if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_B))
		gpad_key(KEY_GAMEPAD_B, KEY_ESCAPE);
	if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_X))
		gpad_key(KEY_GAMEPAD_X, -1);
	if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_Y))
		gpad_key(KEY_GAMEPAD_Y, s_gpad_fire_key);
	if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_START))
		gpad_key(KEY_GAMEPAD_START, s_gpad_pause_key);
	if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_BACK))
		gpad_key(KEY_GAMEPAD_BACK, KEY_ESCAPE);

	/*
	 * Zoom.  The game treats "zoom in" (magnify) as PAGEDOWN and
	 * "zoom out" (see more map) as PAGEUP (nethercycle.cpp).  For a
	 * controller the right side zooms IN and the left side zooms OUT,
	 * consistent across triggers and the right stick.
	 */
	if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_LEFTSHOULDER))
		gpad_key(KEY_GAMEPAD_LB, s_gpad_pause_key);
	if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))
		gpad_key(KEY_GAMEPAD_RB, KEY_PAGEDOWN);   /* zoom in  */
	if (SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > GAMEPAD_AXIS_THRESHOLD)
		gpad_key(KEY_GAMEPAD_LT, KEY_PAGEUP);     /* zoom out */
	if (SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > GAMEPAD_AXIS_THRESHOLD)
		gpad_key(KEY_GAMEPAD_RT, KEY_PAGEDOWN);   /* zoom in  */

	/* D-pad: movement, mapped onto the configured direction keys. */
	if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_DPAD_UP))
		gpad_key(KEY_GAMEPAD_DPAD_UP, s_gpad_up_key);
	if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_DPAD_DOWN))
		gpad_key(KEY_GAMEPAD_DPAD_DOWN, s_gpad_down_key);
	if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_DPAD_LEFT))
		gpad_key(KEY_GAMEPAD_DPAD_LEFT, s_gpad_left_key);
	if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
		gpad_key(KEY_GAMEPAD_DPAD_RIGHT, s_gpad_right_key);

	/* Left stick: movement, same mapping as the D-pad. */
	lx = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTX);
	ly = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTY);

	if (ly < -GAMEPAD_AXIS_THRESHOLD)
		gpad_key(KEY_GAMEPAD_STICK_UP, s_gpad_up_key);
	if (ly > GAMEPAD_AXIS_THRESHOLD)
		gpad_key(KEY_GAMEPAD_STICK_DOWN, s_gpad_down_key);
	if (lx < -GAMEPAD_AXIS_THRESHOLD)
		gpad_key(KEY_GAMEPAD_STICK_LEFT, s_gpad_left_key);
	if (lx > GAMEPAD_AXIS_THRESHOLD)
		gpad_key(KEY_GAMEPAD_STICK_RIGHT, s_gpad_right_key);

	/* Right stick: camera zoom (up = magnify). */
	rx = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_RIGHTX);
	ry = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_RIGHTY);

	if (ry < -GAMEPAD_AXIS_THRESHOLD)
		gpad_key(KEY_GAMEPAD_STICK_UP, KEY_PAGEDOWN);  /* zoom in  */
	if (ry > GAMEPAD_AXIS_THRESHOLD)
		gpad_key(KEY_GAMEPAD_STICK_DOWN, KEY_PAGEUP);  /* zoom out */
	if (rx < -GAMEPAD_AXIS_THRESHOLD)
		gpad_key(KEY_GAMEPAD_STICK_LEFT, -1);
	if (rx > GAMEPAD_AXIS_THRESHOLD)
		gpad_key(KEY_GAMEPAD_STICK_RIGHT, -1);
}

unsigned int platform_num_keys(void)
{
	return KEY_COUNT;
}

const unsigned char *platform_get_keyboard_state(void)
{
	return s_merged;
}

const unsigned char *platform_get_old_keyboard_state(void)
{
	return s_old_merged;
}

int platform_quit_requested(void)
{
	return s_quit_requested;
}

int platform_key_pressed_raw(int key)
{
	if (key < 0 || key >= KEY_COUNT)
		return 0;
	return (s_keyboard[key] != 0);
}

/* Map an SDL keycode to a project KEY_ id, or KEY_UNKNOWN. */
static int sdl_to_key(int sym)
{
	switch (sym) {
	case SDLK_BACKSPACE: return KEY_BACKSPACE;
	case SDLK_TAB:       return KEY_TAB;
	case SDLK_RETURN:    return KEY_RETURN;
	case SDLK_ESCAPE:    return KEY_ESCAPE;
	case SDLK_SPACE:     return KEY_SPACE;
	case SDLK_F1:  return KEY_F1;
	case SDLK_F2:  return KEY_F2;
	case SDLK_F3:  return KEY_F3;
	case SDLK_F4:  return KEY_F4;
	case SDLK_F5:  return KEY_F5;
	case SDLK_F6:  return KEY_F6;
	case SDLK_F7:  return KEY_F7;
	case SDLK_F8:  return KEY_F8;
	case SDLK_F9:  return KEY_F9;
	case SDLK_F10: return KEY_F10;
	case SDLK_F11: return KEY_F11;
	case SDLK_F12: return KEY_F12;
	case SDLK_PAGEUP:   return KEY_PAGEUP;
	case SDLK_PAGEDOWN: return KEY_PAGEDOWN;
	case SDLK_HOME:     return KEY_HOME;
	case SDLK_END:      return KEY_END;
	case SDLK_INSERT:   return KEY_INSERT;
	case SDLK_DELETE:   return KEY_DELETE;
	case SDLK_UP:    return KEY_UP;
	case SDLK_DOWN:  return KEY_DOWN;
	case SDLK_LEFT:  return KEY_LEFT;
	case SDLK_RIGHT: return KEY_RIGHT;
	case SDLK_KP_PLUS:  return KEY_KP_PLUS;
	case SDLK_KP_MINUS: return KEY_KP_MINUS;
	case SDLK_KP_0:     return KEY_KP_0;
	case SDLK_KP_1:     return KEY_KP_1;
	case SDLK_LSHIFT: return KEY_LSHIFT;
	case SDLK_RSHIFT: return KEY_RSHIFT;
	case SDLK_LCTRL:  return KEY_LCTRL;
	case SDLK_RCTRL:  return KEY_RCTRL;
	case SDLK_LALT:   return KEY_LALT;
	case SDLK_RALT:   return KEY_RALT;
	default: break;
	}

	if (sym >= SDLK_a && sym <= SDLK_z)
		return KEY_A + (sym - SDLK_a);
	if (sym >= SDLK_0 && sym <= SDLK_9)
		return KEY_0 + (sym - SDLK_0);

	return KEY_UNKNOWN;
}

void platform_pump_input(void)
{
	SDL_Event event;
	int i;

	/* Snapshot the merged state from last frame for edge detection. */
	for (i = 0; i < KEY_COUNT; i++)
		s_old_merged[i] = (s_keyboard[i] || s_gamepad[i]) ? 1 : 0;

	s_quit_requested = 0;

	SDL_PumpEvents();

	while (SDL_PollEvent(&event)) {
		int key;
		switch (event.type) {
		case SDL_KEYDOWN:
			key = sdl_to_key((int)event.key.keysym.sym);
			if (key >= 0 && key < KEY_COUNT)
				s_keyboard[key] = 1;
			break;
		case SDL_KEYUP:
			key = sdl_to_key((int)event.key.keysym.sym);
			if (key >= 0 && key < KEY_COUNT)
				s_keyboard[key] = 0;
			break;
		case SDL_CONTROLLERDEVICEADDED:
			gamepad_open((int)event.cdevice.which);
			break;
		case SDL_CONTROLLERDEVICEREMOVED:
			gamepad_close_id(event.cdevice.which);
			break;
		case SDL_QUIT:
			s_quit_requested = 1;
			break;
		default:
			break;
		}
	}

	platform_update_gamepad_state();

	/* Rebuild the merged view exposed to game code. */
	for (i = 0; i < KEY_COUNT; i++)
		s_merged[i] = (s_keyboard[i] || s_gamepad[i]) ? 1 : 0;
}

const char *platform_key_name(int key)
{
	if (key < 0 || key >= KEY_COUNT)
		return "?";

	if (key >= KEY_A && key <= KEY_Z) {
		static char buf[2];
		buf[0] = (char)('A' + (key - KEY_A));
		buf[1] = 0;
		return buf;
	}
	if (key >= KEY_0 && key <= KEY_9) {
		static char buf[2];
		buf[0] = (char)('0' + (key - KEY_0));
		buf[1] = 0;
		return buf;
	}

	switch (key) {
	case KEY_BACKSPACE: return "BACKSPACE";
	case KEY_TAB:       return "TAB";
	case KEY_RETURN:    return "RETURN";
	case KEY_ESCAPE:    return "ESCAPE";
	case KEY_SPACE:     return "SPACE";
	case KEY_F1:  return "F1";
	case KEY_F2:  return "F2";
	case KEY_F3:  return "F3";
	case KEY_F4:  return "F4";
	case KEY_F5:  return "F5";
	case KEY_F6:  return "F6";
	case KEY_F7:  return "F7";
	case KEY_F8:  return "F8";
	case KEY_F9:  return "F9";
	case KEY_F10: return "F10";
	case KEY_F11: return "F11";
	case KEY_F12: return "F12";
	case KEY_PAGEUP:   return "PAGEUP";
	case KEY_PAGEDOWN: return "PAGEDOWN";
	case KEY_HOME:     return "HOME";
	case KEY_END:      return "END";
	case KEY_INSERT:   return "INSERT";
	case KEY_DELETE:   return "DELETE";
	case KEY_UP:    return "UP";
	case KEY_DOWN:  return "DOWN";
	case KEY_LEFT:  return "LEFT";
	case KEY_RIGHT: return "RIGHT";
	case KEY_KP_PLUS:  return "KP+";
	case KEY_KP_MINUS: return "KP-";
	case KEY_KP_0:     return "KP0";
	case KEY_KP_1:     return "KP1";
	case KEY_LSHIFT: return "LSHIFT";
	case KEY_RSHIFT: return "RSHIFT";
	case KEY_LCTRL:  return "LCTRL";
	case KEY_RCTRL:  return "RCTRL";
	case KEY_LALT:   return "LALT";
	case KEY_RALT:   return "RALT";
	case KEY_GAMEPAD_A:          return "G-A";
	case KEY_GAMEPAD_B:          return "G-B";
	case KEY_GAMEPAD_X:          return "G-X";
	case KEY_GAMEPAD_Y:          return "G-Y";
	case KEY_GAMEPAD_START:      return "START";
	case KEY_GAMEPAD_BACK:       return "BACK";
	case KEY_GAMEPAD_LB:         return "LB";
	case KEY_GAMEPAD_RB:         return "RB";
	case KEY_GAMEPAD_LT:         return "LT";
	case KEY_GAMEPAD_RT:         return "RT";
	case KEY_GAMEPAD_DPAD_UP:    return "PAD-UP";
	case KEY_GAMEPAD_DPAD_DOWN:  return "PAD-DOWN";
	case KEY_GAMEPAD_DPAD_LEFT:  return "PAD-LEFT";
	case KEY_GAMEPAD_DPAD_RIGHT: return "PAD-RIGHT";
	case KEY_GAMEPAD_STICK_UP:   return "STK-UP";
	case KEY_GAMEPAD_STICK_DOWN: return "STK-DOWN";
	case KEY_GAMEPAD_STICK_LEFT: return "STK-LEFT";
	case KEY_GAMEPAD_STICK_RIGHT:return "STK-RIGHT";
	default: break;
	}

	return "?";
}
