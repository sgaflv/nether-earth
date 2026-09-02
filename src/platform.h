#ifndef PLATFORM_H
#define PLATFORM_H

/*
 * Platform abstraction layer.
 *
 * This header (and platform.cpp) is the single place where the game talks to
 * the outside world: window/GL context creation, buffer swap, drawing size,
 * cursor, timing, input and (via audio.h) sound.
 *
 * Game/application code must NOT include SDL or any other desktop-only header.
 * Everything it needs is exposed here. The desktop backend (platform.cpp)
 * currently implements this with SDL; a future Android backend will provide
 * the same interface (e.g. via a Java Activity / EGL / GLES + AAssetManager).
 */

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Timing                                                              */
/*                                                                     */
/* Monotonic milliseconds since an arbitrary start point. Never moves  */
/* backwards, unaffected by wall-clock changes.                        */
unsigned int platform_ticks(void);

/* Diagnostic log helper: on Android goes to logcat, on desktop to    */
/* stderr. Format/args like printf.                                   */
void platform_log_diag(const char *fmt, ...);

/* Busy-sleep / yield for at least 'ms' milliseconds. */
void platform_sleep(unsigned int ms);

/* ------------------------------------------------------------------ */
/* Window / GL context                                                */
/* ------------------------------------------------------------------ */

/* Initialise the platform (video subsystem, etc.). Returns non-zero on  */
/* success. Must be called before any other platform_* function.         */
int platform_init(void);

/* Shut the platform back down (opposite of platform_init).              */
void platform_shutdown(void);

/* Create a window and an OpenGL context of the given pixel size.       */
/* On success fills *out_w / *out_h with the actual drawable size and   */
/* returns true. 'fullscreen' requests a desktop fullscreen window.     */
int  platform_create_window(int w, int h, int fullscreen);
void platform_destroy_window(void);
void platform_swap_buffers(void);
void platform_get_drawable_size(int *w, int *h);
void platform_show_cursor(int show);

/* Raw GL error / capability diagnostics. Called once after context     */
/* creation to record GL version, renderer and vendor.                  */
void platform_log_gl_info(void);

/*
 * Number of stencil bits in the current GL context (0 if there is no
 * stencil buffer). The game's drop shadows need a stencil buffer, so it
 * turns them off when this reports 0 rather than drawing them wrong.
 * Only meaningful after platform_create_window() has succeeded.
 */
int platform_stencil_bits(void);

/* ------------------------------------------------------------------ */
/* Input                                                               */
/* ------------------------------------------------------------------ */

/* Number of entries in the keyboard state array returned by           */
/* platform_get_keyboard_state(). Index with the KEY_ constants below.  */
unsigned int platform_num_keys(void);

/*
 * Pump the platform event queue and refresh the keyboard state.
 * Must be called once per game/menu cycle frame.
 * Also updates an internal "old" state used for edge detection, and
 * latches a window-close request that platform_quit_requested() reports.
 */
void platform_pump_input(void);

/* Pointer to the current keyboard state: bytes indexed by KEY_*.       */
/* A non-zero value means the key is currently held down.               */
const unsigned char *platform_get_keyboard_state(void);

/*
 * Pointer to the previous frame's keyboard state (for edge detection).
 * Same layout as platform_get_keyboard_state().
 */
const unsigned char *platform_get_old_keyboard_state(void);

/* Returns non-zero if the user closed the window / requested quit.     */
/* Cleared automatically by the next platform_pump_input().             */
int platform_quit_requested(void);

/* Human-readable name of a KEY_ value (e.g. "Q", "SPACE").             */
const char *platform_key_name(int key);

/*
 * Whether the physical keyboard alone currently holds 'key', ignoring
 * any gamepad-synthesized keys (the merged state returned by the other
 * accessors includes both).  Debug tools that must only react to a real
 * Escape key (keytest/testpad/fonttest) use this.
 */
int platform_key_pressed_raw(int key);

/* ------------------------------------------------------------------ */
/* Gamepad                                                             */
/*                                                                     */
/* Gamepad buttons and axes are surfaced as ordinary KEY_ values (see  */
/* the KEY_GAMEPAD_* ids below), merged into the same keyboard state   */
/* that game code already polls, so no game-side input changes are     */
/* needed.  In addition, the primary movement/action controls are      */
/* mapped onto the standard keyboard arrows, SPACE and F1 so the       */
/* default key bindings work with a gamepad out of the box.            */
/* ------------------------------------------------------------------ */

/* Non-zero while a controller is connected (and usable).              */
int platform_gamepad_connected(void);

/* Copy the controller name into buf (or a placeholder if none).       */
void platform_gamepad_name(char *buf,int buflen);

/*
 * Tell the gamepad backend which keyboard keys currently drive the
 * five in-game actions, so the controller's movement/face buttons
 * synthesis the *configured* keys rather than hardcoded ones (the user
 * can remap them in the "REDEFINE KEYBOARD" menu).  Pass the normal
 * keyboard ids (up/down/left/right/fire/pause).  Values outside the
 * valid KEY_* range keep the previous setting.
 */
void platform_set_gamepad_mapping(
	int up,
	int down,
	int left,
	int right,
	int fire,
	int pause_key);

/* ------------------------------------------------------------------ */
/* Key constants                                                      */
/*                                                                     */
/* Project-owned virtual key ids, translated by the platform layer     */
/* from physical/keyboard/gamepad events. Game code must never see     */
/* SDL scancodes.                                                      */
/* ------------------------------------------------------------------ */

enum {
	KEY_UNKNOWN = -1,

	/* Special / editing keys first so the printable range below stays */
	/* stable and simple to extend with new letters. */
	KEY_BACKSPACE = 0,
	KEY_TAB,
	KEY_RETURN,
	KEY_ESCAPE,
	KEY_SPACE,

	KEY_F1, KEY_F2, KEY_F3, KEY_F4,
	KEY_F5, KEY_F6, KEY_F7, KEY_F8,
	KEY_F9, KEY_F10, KEY_F11, KEY_F12,

	KEY_PAGEUP, KEY_PAGEDOWN,
	KEY_HOME, KEY_END,
	KEY_INSERT, KEY_DELETE,
	KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,

	KEY_KP_PLUS, KEY_KP_MINUS,
	KEY_KP_0, KEY_KP_1,

	KEY_LSHIFT, KEY_RSHIFT,
	KEY_LCTRL, KEY_RCTRL,
	KEY_LALT, KEY_RALT,

	/* Gamepad controls, reported as virtual keys so the same edge-    */
	/* detection and "redefine keys" code paths work for them. The ids */
	/* live in the unused gap below the printable letter range.        */
	KEY_GAMEPAD_A,          /* bottom face button                       */
	KEY_GAMEPAD_B,          /* right face button                        */
	KEY_GAMEPAD_X,          /* left face button                         */
	KEY_GAMEPAD_Y,          /* top face button                          */
	KEY_GAMEPAD_START,
	KEY_GAMEPAD_BACK,
	KEY_GAMEPAD_LB,         /* left shoulder / bumper                    */
	KEY_GAMEPAD_RB,         /* right shoulder / bumper                   */
	KEY_GAMEPAD_LT,         /* left trigger (digital)                    */
	KEY_GAMEPAD_RT,         /* right trigger (digital)                   */
	KEY_GAMEPAD_DPAD_UP,
	KEY_GAMEPAD_DPAD_DOWN,
	KEY_GAMEPAD_DPAD_LEFT,
	KEY_GAMEPAD_DPAD_RIGHT,
	KEY_GAMEPAD_STICK_UP,   /* left stick pushed up                      */
	KEY_GAMEPAD_STICK_DOWN,
	KEY_GAMEPAD_STICK_LEFT,
	KEY_GAMEPAD_STICK_RIGHT,

	/* Printable single characters: '0'-'9','A'-'Z' map directly from  */
	/* their ASCII codes. The id is computed as KEY_A + (c-'A').       */
	KEY_A = 64,
	KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J,
	KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S,
	KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,

	KEY_0 = 96,
	KEY_1, KEY_2, KEY_3, KEY_4, KEY_5,
	KEY_6, KEY_7, KEY_8, KEY_9,

	KEY_COUNT
};

#ifdef __cplusplus
}
#endif

#endif
