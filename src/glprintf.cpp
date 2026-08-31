#ifdef _WIN32
#include "windows.h"
#endif

#include "stdio.h"
#include "string.h"
#include "stdarg.h"
#include "glport.h"

#include "stroke_font.h"

static int stroke_width(char c)
{
	(void)c;
	return STROKE_ADVANCE;
}

static void draw_char(float x, float y, unsigned char c)
{
	const short *g;
	int first_x, first_y;
	bool have_point=false;
	int i=0;
	float lc[256*2*2];

	if (c>=128) return;
	g=glyph_table[c];
	if (g==0) return;

	/* Iterate over strokes: accumulate segments into a line vertex array. */ 
	while (*g!=END_GLYPH_VAL) {
		int cx,cy;
		cx=g[0]; cy=g[1];
		if (cx==PEN_UP_VAL && cy==PEN_UP_VAL) {
			/* Start a new stroke (pen up): move to the next point. */
			g+=2;
			first_x=g[0]; first_y=g[1];
			have_point=true;
			g+=2;
			continue;
		}
		if (have_point && i<256) {
			lc[i*4+0]=x+first_x; lc[i*4+1]=y+first_y;
			lc[i*4+2]=x+cx;      lc[i*4+3]=y+cy;
			i++;
		}
		first_x=cx; first_y=cy;
		have_point=true;
		g+=2;
	}

	if (i>0) {
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2,GL_FLOAT,0,lc);
		glDrawArrays(GL_LINES,0,i*2);
		glDisableClientState(GL_VERTEX_ARRAY);
	}
}

/* ------------------------------------------------------------------ */
/* Replacement for glutStrokeWidth.                                    */
/* ------------------------------------------------------------------ */
static int glutStrokeWidth(int font, char c)
{
	(void)font;
	return stroke_width(c);
}

/* ------------------------------------------------------------------ */
/* Replacement for glutStrokeCharacter: draws one character at the     */
/* current raster/modelview position in the XY plane.                  */
/* ------------------------------------------------------------------ */
static void glutStrokeCharacter(int font, char c)
{
	(void)font;
	glNormal3f(0.0f,0.0f,1.0f);
	draw_char(0.0f,0.0f,c);
}

/* Public helper (used by the font debug/test mode): draws a single     */
/* stroke-font character at the current modelview origin.               */
void stroke_char(int c)
{
	glNormal3f(0.0f,0.0f,1.0f);
	draw_char(0.0f,0.0f,(unsigned char)c);
}

void glprintf(const char *fmt, ...)						
{
	float		length=0;								// Used To Find The Length Of The Text
	char		text[256];								// Holds Our String
	va_list		ap;										// Pointer To List Of Arguments
	int i,tl;

	if (fmt==0) return;

	va_start(ap, fmt);									// Parses The String For Variables
	vsprintf(text, fmt, ap);							// And Converts Symbols To Actual Numbers
	va_end(ap);											// Results Are Stored In Text

	tl=strlen(text);
	for(i=0;i<tl;i++) {
		length+=glutStrokeWidth(0,text[i]);	
	} /* for /*/ 

	glTranslatef(-length/2,0.0f,0.0f);					// Center Our Text On The Screen

	glNormal3f(0.0,0.0,1.0);
	for(i=0;i<tl;i++) {
		glutStrokeCharacter(0,text[i]);
		glTranslatef(STROKE_ADVANCE,0.0f,0.0f);
	} /* for */ 
} /* glprintf */ 



void scaledglprintf(float sx,float sy,const char *fmt, ...)						
{
	float		length=0;								// Used To Find The Length Of The Text
	char		text[256];								// Holds Our String
	va_list		ap;										// Pointer To List Of Arguments
	int i,tl;

	if (fmt==0) return;

	va_start(ap, fmt);									// Parses The String For Variables
	vsprintf(text, fmt, ap);							// And Converts Symbols To Actual Numbers
	va_end(ap);											// Results Are Stored In Text

	tl=strlen(text);
	for(i=0;i<tl;i++) {
		length+=glutStrokeWidth(0,text[i]);	
	} /* for /*/ 

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glScalef(sx,sy,1.0f);

	glTranslatef(-length/2,0.0f,0.0f);					// Center Our Text On The Screen

	glNormal3f(0.0,0.0,1.0);
	for(i=0;i<tl;i++) {
		glutStrokeCharacter(0,text[i]);
		glTranslatef(STROKE_ADVANCE,0.0f,0.0f);
	} /* for */ 

	glPopMatrix();
} /* scaledglprintf */ 



void scaledglprintf2(float sx,float sy,const char *fmt, ...)						
{
	float		length=0;								// Used To Find The Length Of The Text
	char		text[256];								// Holds Our String
	va_list		ap;										// Pointer To List Of Arguments
	int i,tl;

	if (fmt==0) return;

	va_start(ap, fmt);									// Parses The String For Variables
	vsprintf(text, fmt, ap);							// And Converts Symbols To Actual Numbers
	va_end(ap);											// Results Are Stored In Text

	tl=strlen(text);
	for(i=0;i<tl;i++) {
		length+=glutStrokeWidth(0,text[i]);	
	} /* for /*/ 

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glScalef(sx,sy,1.0f);

	glTranslatef(-length/2,0.0f,0.0f);					// Center Our Text On The Screen

	glNormal3f(0.0,0.0,1.0);
	for(i=0;i<tl;i++) {
		glutStrokeCharacter(0,text[i]);
		glTranslatef(STROKE_ADVANCE,0.0f,0.0f);
	} /* for */ 

	glPopMatrix();
} /* scaledglprintf2 */ 



void fittedglprintf(float sx,float sy,const char *fmt, ...)						
{
	float		length=0;								// Used To Find The Length Of The Text
	char		text[256];								// Holds Our String
	va_list		ap;										// Pointer To List Of Arguments
	int i,tl;

	if (fmt==0) return;

	va_start(ap, fmt);									// Parses The String For Variables
	vsprintf(text, fmt, ap);							// And Converts Symbols To Actual Numbers
	va_end(ap);											// Results Are Stored In Text

	tl=strlen(text);
	for(i=0;i<tl;i++) {
		length+=glutStrokeWidth(0,text[i]);	
	} /* for /*/ 

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glScalef(sx/length,sy/(119.05f+33.33f),1.0f);

	glTranslatef(-length/2,0.0f,0.0f);					// Center Our Text On The Screen

	glNormal3f(0.0,0.0,1.0);
	for(i=0;i<tl;i++) {
		glutStrokeCharacter(0,text[i]);
		glTranslatef(STROKE_ADVANCE,0.0f,0.0f);
	} /* for */ 

	glPopMatrix();
} /* fittedglprintf */ 
