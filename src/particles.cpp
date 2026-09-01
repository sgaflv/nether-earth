
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

PARTICLE::PARTICLE(void)
{
	size1=size2=0;
	r=g=b=0;
	a1=a2=0;
	lifetime=acttime=0;
} /* PARTICLE::PARTICLE */ 


PARTICLE::PARTICLE(Vector p,Vector spd1,Vector spd2,float sz1,float sz2,float rp,float gp,float bp,float a1p,float a2p,int lt)
{
	pos=p;
	speed1=spd1;
	speed2=spd2;
	size1=sz1;
	size2=sz2;
	r=rp;
	g=gp;
	b=gp;
	a1=a1p;
	a2=a2p;
	lifetime=lt;
	acttime=0;

} /* PARTICLE::PARTICLE */ 



void NETHER::DrawParticle(PARTICLE *p)
{
	float val,val2;
	float sz;

	val2=float(p->acttime)/float(p->lifetime);
	val=1-val2;

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	glTranslatef(p->pos.x,p->pos.y,p->pos.z);
	glDepthMask(GL_FALSE);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	sz=val*p->size1+val2*p->size2;
	glNormal3f(0,0,1);

	{
		/* 12 vertices (4 triangles), each with position and per-vertex alpha color */ 
		float vx[12*3];
		float vc[12*4];
		float a_core=val*p->a1+val2*p->a2;
		int k=0;
#define PART_V(pvx,pvy,pvz, alpha) do { \
			vx[k*3+0]=(pvx); vx[k*3+1]=(pvy); vx[k*3+2]=(pvz); \
			vc[k*4+0]=p->r; vc[k*4+1]=p->g; vc[k*4+2]=p->b; vc[k*4+3]=(alpha); k++; } while(0)
		PART_V(0,0,0, a_core); PART_V(sz,0,0, 0); PART_V(0,sz,0, 0);
		PART_V(0,0,0, a_core); PART_V(0,sz,0, 0); PART_V(-sz,0,0, 0);
		PART_V(0,0,0, a_core); PART_V(-sz,0,0, 0); PART_V(0,-sz,0, 0);
		PART_V(0,0,0, a_core); PART_V(0,-sz,0, 0); PART_V(sz,0,0, 0);
#undef PART_V

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glVertexPointer(3,GL_FLOAT,0,vx);
		glColorPointer(4,GL_FLOAT,0,vc);
		glDrawArrays(GL_TRIANGLES,0,k);
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
	}

	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);


	glPopMatrix();

} /* NETHER::DrawParticle */ 



bool NETHER::CycleParticle(PARTICLE *p)
{
	float val,val2;

	val2=float(p->acttime)/float(p->lifetime);
	val=1-val2;

	p->pos=p->pos+(p->speed1*val+p->speed2*val2);

	p->acttime++;
	if (p->acttime>=p->lifetime) return false;
	return true;
} /* NETHER::CyclePArticle */ 
