#ifndef GLPRINTF_HEADER
#define GLPRINTF_HEADER

void glprintf(const char *fmt, ...);
void scaledglprintf(float sx,float sy,const char *fmt, ...);	/* center alignment */ 
void scaledglprintf2(float sx,float sy,const char *fmt, ...);	/* left alignment */ 
void fittedglprintf(float sx,float sy,const char *fmt, ...);

/* Draw a single stroke-font character at the current modelview origin. */
/* The caller is responsible for advancing the pen position afterwards.  */
void stroke_char(int c);

#endif