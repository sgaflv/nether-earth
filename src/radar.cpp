
#include "string.h"
#include "stdio.h"
#include "math.h"

#include "glport.h"



#include "list.h"
#include "vector.h"
#include "cmc.h"
#include "3dobject.h"
#include "shadow3dobject.h"
#include "piece3dobject.h"
#include "myglutaux.h"
#include "nether.h"


#include "glprintf.h"

void NETHER::draw_radar(void)
{
    /* Clear the color and depth buffers. */ 
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

	glTranslatef(64.0,8.0,0.0);
	glColor3f(0.5f,0.5f,1.0f);
	scaledglprintf(0.18f,0.4f,"RADAR:");

	glLoadIdentity();
	glScalef(4.0,4.0,4.0);

	/* It will draw the RADAR map, directly taken from the IA variables: */ 
	{
		int x,y;
		int starty,startx;
		const int maxy=94,maxx=16;

		/*
		 * The map cells used to be one glDrawArrays each: 94*16 = 1504 draw
		 * calls per radar frame, which dwarfed everything else the game
		 * draws. They are gathered into a single vertex/colour array and
		 * issued as one call instead. Sized for the fixed maxy*maxx grid,
		 * six vertices (two triangles) per cell.
		 */
		static float   cell_vtx[maxy*maxx*6*3];
		static GLfloat cell_col[maxy*maxx*6*4];
		int nvtx=0;

		/*
		 * Colour carried between cells: an unrecognised terrain value sets
		 * no colour and inherits the previous cell's, exactly as the
		 * per-cell glColor3f version did. On entry that is the colour the
		 * "RADAR:" caption above was drawn with.
		 */
		float cr=0.5f,cg=0.5f,cb=1.0f;

		startx=(int)((shipp.x-4)*2);
		starty=(int)((shipp.y-23)*2);
		if ((starty+maxy)>(map_h*2)) starty=(map_h*2)-maxy;
		if (starty<0) starty=0;
		if ((startx+maxx)>(map_w*2)) startx=(map_w*2)-maxx;
		if (startx<0) startx=0;

		for(y=0;y<maxy;y++) {
			for(x=0;x<maxx;x++) {
				if (x+startx<(map_w*2) &&
					y+starty<(map_h*2) &&
					discreetmap!=0) {
					switch(discreetmap[x+startx+(y+starty)*(map_w*2)]) {
					case T_GRASS: 
							cr=0.0f; cg=1.0f; cb=0.0f;
							break;
					case T_SAND:
							cr=0.2f; cg=0.9f; cb=0.0f;
							break;
					case T_MOUNTAINS:
							cr=0.4f; cg=0.8f; cb=0.0f;
							break;
					case T_HOLE:
							cr=0.0f; cg=0.8f; cb=0.0f;
							break;
					case T_LOWBUILDING:
							cr=0.3f; cg=0.3f; cb=0.3f;
							break;
					case T_BUILDING:
							cr=0.0f; cg=0.0f; cb=0.0f;
							break;
					case T_SHIP:
							cr=1.0f; cg=1.0f; cb=1.0f;
							break;
					case T_ROBOT:
							cr=0.0f; cg=0.0f; cb=1.0f;
							break;
					case T_EROBOT:
							cr=1.0f; cg=0.0f; cb=0.0f;
							break;	
					} /* switch */ 
					{
						/* The quad's corners, wound as two triangles. */
						const float x0=(float)(30+y),   x1=(float)(30+y+1);
						const float y0=(float)(maxx-(x+1)), y1=(float)(maxx-x);
						const float corner[6][2] = {
							{x0,y0},{x1,y0},{x1,y1},
							{x0,y0},{x1,y1},{x0,y1}
						};
						int c;

						for(c=0;c<6;c++) {
							cell_vtx[nvtx*3+0]=corner[c][0];
							cell_vtx[nvtx*3+1]=corner[c][1];
							cell_vtx[nvtx*3+2]=0.0f;
							cell_col[nvtx*4+0]=cr;
							cell_col[nvtx*4+1]=cg;
							cell_col[nvtx*4+2]=cb;
							cell_col[nvtx*4+3]=1.0f;
							nvtx++;
						} /* for */ 
					}
				} /* if */ 
			} /* for */ 
		} /* for */ 

		if (nvtx>0) {
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_COLOR_ARRAY);
			glVertexPointer(3,GL_FLOAT,0,cell_vtx);
			glColorPointer(4,GL_FLOAT,0,cell_col);
			glDrawArrays(GL_TRIANGLES,0,nvtx);
			glDisableClientState(GL_COLOR_ARRAY);
			glDisableClientState(GL_VERTEX_ARRAY);
		} /* if */ 

		/* Draw the SHIP: */ 
		x=(int)(shipp.x*2-startx);
		y=(int)(shipp.y*2-starty);
		glColor3f(1.0f,1.0f,1.0f);
		{
			float q[4*3] = {
				(float)(30+y),     (float)(maxx-(x+2)), 2.0f,
				(float)(30+y+2),   (float)(maxx-(x+2)), 2.0f,
				(float)(30+y+2),   (float)(maxx-x),     2.0f,
				(float)(30+y),     (float)(maxx-x),     2.0f
			};
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3,GL_FLOAT,0,q);
			glDrawArrays(GL_TRIANGLE_FAN,0,4);
			glDisableClientState(GL_VERTEX_ARRAY);
		}

	}


} /* NETHER::draw_radar */ 
