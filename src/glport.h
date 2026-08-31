#ifndef GLPORT_H
#define GLPORT_H

/*
 * Portable OpenGL header and math helpers.
 *
 * This header centralises GL header selection so the code can compile against
 * desktop OpenGL (GL 1.x/2.x) or the fixed-function pipeline of OpenGL ES 1.x
 * (the only GL version available inside an APK that still supports the legacy
 * immediate-mode / glBegin-glEnd API used by this code base).
 *
 * It also provides tiny self-contained replacements for the two GLU routines
 * the game used (gluPerspective / gluLookAt), since GLU is not part of
 * OpenGL ES and is therefore not available inside an APK.
 */

#if defined(__APPLE__)
  #include <OpenGL/gl.h>
#elif defined(__ANDROID__) || defined(ANDROID)
  #include <GLES/gl.h>
#elif defined(_WIN32)
  #include <windows.h>
  #include <GL/gl.h>
#else
  #include <GL/gl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Replacement for gluPerspective (OpenGL ES lacks GLU). */
/* Builds a perspective projection and multiplies the current matrix. */
void gluPerspective(float fovy, float aspect, float zNear, float zFar);

/* Replacement for gluLookAt (OpenGL ES lacks GLU). */
/* Builds a camera (model view) matrix and multiplies the current matrix. */
void gluLookAt(float eyeX,float eyeY,float eyeZ,
               float centerX,float centerY,float centerZ,
               float upX,float upY,float upZ);

#if defined(__ANDROID__) || defined(ANDROID)
/* OpenGL ES 1.x lacks the GLdouble glOrtho and glColor3f used by this code
 * base; provide GLES-compatible replacements here. */
void glOrtho(double l, double r, double b, double t, double n, double f);
void glColor3f(GLfloat red, GLfloat green, GLfloat blue);
#endif

#ifdef __cplusplus
}
#endif

#endif
