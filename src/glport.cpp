#include <math.h>
#include "glport.h"

/* ------------------------------------------------------------------ */
/* gluPerspective replacement                                          */
/*                                                                     */
/* Mirrors the semantics of GLU gluPerspective: it builds the standard */
/* symmetric perspective projection matrix and multiplies it into the  */
/* current matrix. Column-major ordering as expected by OpenGL.        */
/* ------------------------------------------------------------------ */
void gluPerspective(float fovy, float aspect, float zNear, float zFar)
{
	const float pi = 3.14159265358979323846f;
	float f, m[16];
	int i;

	f = 1.0f / tanf(fovy * pi / 360.0f);

	for(i=0;i<16;i++) m[i]=0.0f;
	m[0]  = f / aspect;
	m[5]  = f;
	m[10] = (zFar + zNear) / (zNear - zFar);
	m[11] = -1.0f;
	m[14] = (2.0f * zFar * zNear) / (zNear - zFar);

	glMultMatrixf(m);
}

/* ------------------------------------------------------------------ */
/* gluLookAt replacement                                               */
/*                                                                     */
/* Mirrors the semantics of GLU gluLookAt: builds a camera framing     */
/* matrix from eye/center/up and multiplies it (with the -eye          */
/* translation) into the current matrix.                               */
/* ------------------------------------------------------------------ */
void gluLookAt(float eyeX,float eyeY,float eyeZ,
               float centerX,float centerY,float centerZ,
               float upX,float upY,float upZ)
{
	float forward[3], side[3], up[3];
	float m[16];
	float len, invlen;

	/* Forward vector (looking direction): */ 
	forward[0] = centerX - eyeX;
	forward[1] = centerY - eyeY;
	forward[2] = centerZ - eyeZ;
	len = sqrtf(forward[0]*forward[0] + forward[1]*forward[1] + forward[2]*forward[2]);
	if (len != 0.0f) { invlen = 1.0f/len; forward[0]*=invlen; forward[1]*=invlen; forward[2]*=invlen; }

	/* Side vector = forward x up: */ 
	side[0] = forward[1]*upZ - forward[2]*upY;
	side[1] = forward[2]*upX - forward[0]*upZ;
	side[2] = forward[0]*upY - forward[1]*upX;
	len = sqrtf(side[0]*side[0] + side[1]*side[1] + side[2]*side[2]);
	if (len != 0.0f) { invlen = 1.0f/len; side[0]*=invlen; side[1]*=invlen; side[2]*=invlen; }

	/* Corrected up vector = side x forward: */ 
	up[0] = side[1]*forward[2] - side[2]*forward[1];
	up[1] = side[2]*forward[0] - side[0]*forward[2];
	up[2] = side[0]*forward[1] - side[1]*forward[0];

	m[0] = side[0];   m[4] = side[1];   m[8]  = side[2];   m[12] = 0.0f;
	m[1] = up[0];     m[5] = up[1];     m[9]  = up[2];     m[13] = 0.0f;
	m[2] = -forward[0]; m[6] = -forward[1]; m[10] = -forward[2]; m[14] = 0.0f;
	m[3] = 0.0f;      m[7] = 0.0f;      m[11] = 0.0f;      m[15] = 1.0f;

	glMultMatrixf(m);
	glTranslatef(-eyeX, -eyeY, -eyeZ);
}

/* ------------------------------------------------------------------ */
/* glOrtho replacement (OpenGL ES only)                               */
/*                                                                     */
/* OpenGL ES 1.x provides glOrthof (GLfloat) but not the GLdouble      */
/* glOrtho used elsewhere; build the orthographic projection matrix    */
/* and multiply it into the current matrix (matches desktop glOrtho).  */
/* ------------------------------------------------------------------ */
#if defined(__ANDROID__) || defined(ANDROID)
void glOrtho(double l, double r, double b, double t, double n, double f)
{
	GLfloat m[16];
	int i;

	for(i=0;i<16;i++) m[i]=0.0f;
	m[0]  = (GLfloat)(2.0 / (r - l));
	m[5]  = (GLfloat)(2.0 / (t - b));
	m[10] = (GLfloat)(-2.0 / (f - n));
	m[12] = (GLfloat)(-(r + l) / (r - l));
	m[13] = (GLfloat)(-(t + b) / (t - b));
	m[14] = (GLfloat)(-(f + n) / (f - n));
	m[15] = 1.0f;

	glMultMatrixf(m);
}

void glColor3f(GLfloat red, GLfloat green, GLfloat blue)
{
	glColor4f(red, green, blue, 1.0f);
}
#endif
