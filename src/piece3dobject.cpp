
#include "stdio.h"

#include "glport.h"

#include "vector.h"
#include "quaternion.h"
#include "cmc.h"
#include "3dobject.h"
#include "piece3dobject.h"

#include "myglutaux.h"

Piece3DObject::Piece3DObject() : C3DObject()
{
	shdw_npuntos_0=0;
	shdw_ncaras_0=0;
	shdw_puntos_0=0;
	shdw_caras_0=0;
	shdw_npuntos_90=0;
	shdw_ncaras_90=0;
	shdw_puntos_90=0;
	shdw_caras_90=0;
	shdw_npuntos_180=0;
	shdw_ncaras_180=0;
	shdw_puntos_180=0;
	shdw_caras_180=0;
	shdw_npuntos_270=0;
	shdw_ncaras_270=0;
	shdw_puntos_270=0;
	shdw_caras_270=0;
	shdw_npuntos_dynamic=0;
	shdw_ncaras_dynamic=0;
	shdw_puntos_dynamic=0;
	shdw_caras_dynamic=0;
} /* Piece3DObject::Piece3DObject */ 


Piece3DObject::Piece3DObject(const char *file,const char *texturedir) : C3DObject(file,texturedir)
{
	shdw_npuntos_0=0;
	shdw_ncaras_0=0;
	shdw_puntos_0=0;
	shdw_caras_0=0;
	shdw_npuntos_90=0;
	shdw_ncaras_90=0;
	shdw_puntos_90=0;
	shdw_caras_90=0;
	shdw_npuntos_180=0;
	shdw_ncaras_180=0;
	shdw_puntos_180=0;
	shdw_caras_180=0;
	shdw_npuntos_270=0;
	shdw_ncaras_270=0;
	shdw_puntos_270=0;
	shdw_caras_270=0;
	shdw_npuntos_dynamic=0;
	shdw_ncaras_dynamic=0;
	shdw_puntos_dynamic=0;
	shdw_caras_dynamic=0;
} /* Piece3DObject::Piece3DObject */ 


Piece3DObject::~Piece3DObject()
{
	delete shdw_puntos_0;
	delete shdw_caras_0;
	shdw_puntos_0=0;
	shdw_caras_0=0;
	delete shdw_puntos_90;
	delete shdw_caras_90;
	shdw_puntos_90=0;
	shdw_caras_90=0;
	delete shdw_puntos_180;
	delete shdw_caras_180;
	shdw_puntos_180=0;
	shdw_caras_180=0;
	delete shdw_puntos_270;
	delete shdw_caras_270;
	shdw_puntos_270=0;
	shdw_caras_270=0;
	delete shdw_puntos_dynamic;
	delete shdw_caras_dynamic;
	shdw_puntos_dynamic=0;
	shdw_caras_dynamic=0;
} /* Piece3DObject::~Piece3DObject */ 


void Piece3DObject::DrawShadow(int angle,Vector light,float r,float g,float b,float a)
{
	/* Draw the object: */ 
	glEnableClientState(GL_VERTEX_ARRAY);
	glColor4f(r,g,b,a);
	glNormal3f(0,1,0);

	angle=angle%360;

	if (a!=1){
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
	} /* if */ 

	switch(angle) {
	case 0:
		ExpandAndDrawShadow(shdw_puntos_0,shdw_caras_0,shdw_ncaras_0);
		break;
	case 90:
		ExpandAndDrawShadow(shdw_puntos_90,shdw_caras_90,shdw_ncaras_90);
		break;
	case 180:
		ExpandAndDrawShadow(shdw_puntos_180,shdw_caras_180,shdw_ncaras_180);
		break;
	case 270:
		ExpandAndDrawShadow(shdw_puntos_270,shdw_caras_270,shdw_ncaras_270);
		break;
	default:
		ComputeDynamicShadow(angle,light);

		ExpandAndDrawShadow(shdw_puntos_dynamic,shdw_caras_dynamic,shdw_ncaras_dynamic);
		break;
	} /* switch */ 

	if (a!=1) glDisable(GL_BLEND);
} /* Piece3DObject::DraqwShadow */ 


void Piece3DObject::ExpandAndDrawShadow(float *puntos,int *caras,int ncaras)
{
	int i;
	float *vtx;

	if (puntos==0 || caras==0 || ncaras<=0) return;

	vtx=new float[ncaras*3*3];
	for(i=0;i<ncaras*3;i++) {
		int idx=caras[i];
		vtx[i*3+0]=puntos[idx*3+0];
		vtx[i*3+1]=puntos[idx*3+1];
		vtx[i*3+2]=puntos[idx*3+2];
	} /* for */ 

	glVertexPointer(3,GL_FLOAT,0,vtx);
	glDrawArrays(GL_TRIANGLES,0,ncaras*3);

	delete []vtx;
} /* Piece3DObject::ExpandAndDrawShadow */ 


void Piece3DObject::ComputeShadow(int angle,Vector light,int *np,int *nc,float **p,int **c,CMC *cmc)
{
	int i;
	int pry_npuntos;
	float *pry_puntos;
	int *pry_caras,pry_ncaras;
	float plane[4];
	float l[4];
	float v[3],w[3],n[3],value;
	int shdw_ncaras;

	angle=angle%360;

	plane[0]=0;
	plane[1]=0;
	plane[2]=1;
	plane[3]=0;


	/* Rotate the light: */ 
	{
		float lv[4]={float(light.x),float(light.y),float(light.z),1};
		float axis[4]={0,0,1,1};
		float m[16]={1,0,0,0,
					 0,1,0,0,
					 0,0,1,0,
					 0,0,0,1};
		Quaternion q2;

		q2.from_axis_angle(axis,float((-angle*3.14592f)/180));
		q2.to_matrix(m);

		ApplyMatrix(lv,m,l);
	}

	pry_npuntos=npuntos;
	pry_puntos=new float[npuntos*3];
	/* Project ALL the triangles onto the Z plane: */ 
	{
		float p[3],tmp[3];

		for(i=0;i<npuntos;i++) {
			p[0]=puntos[i*3];
			p[1]=puntos[i*3+1];
			p[2]=puntos[i*3+2];
			PlaneLineCollision(plane,p,l,tmp);
			
			pry_puntos[i*3]=tmp[0];
			pry_puntos[i*3+1]=tmp[1];
			pry_puntos[i*3+2]=0;
		} /* for */ 
	}

	/* Create the projected triangles: */ 
	shdw_ncaras=0;
	pry_ncaras=ncaras;
	pry_caras=new int[ncaras*3];
	for(i=0;i<ncaras;i++) {
		/* Check that the triangle is visible: */ 

		v[0]=puntos[caras[i*3+1]*3]-puntos[caras[i*3]*3];;
		v[1]=puntos[caras[i*3+1]*3+1]-puntos[caras[i*3]*3+1];
		v[2]=puntos[caras[i*3+1]*3+2]-puntos[caras[i*3]*3+2];
		w[0]=puntos[caras[i*3+2]*3]-puntos[caras[i*3+1]*3];
		w[1]=puntos[caras[i*3+2]*3+1]-puntos[caras[i*3+1]*3+1];
		w[2]=puntos[caras[i*3+2]*3+2]-puntos[caras[i*3+1]*3+2];
		Normalf(v,w,n);

		value=n[0]*l[0]+n[1]*l[1]+n[2]*l[2];
		if (value>0) {
			pry_caras[shdw_ncaras*3]=caras[i*3];
			pry_caras[shdw_ncaras*3+1]=caras[i*3+1];
			pry_caras[shdw_ncaras*3+2]=caras[i*3+2];
			shdw_ncaras++;
		} /* if */ 
	} /* for */ 


	/* Join the projected triangles: */ 
	/* ... */ 

	/* Copy them into the object's variables: */ 
	delete *p;
	delete *c;
	*p=0;
	*c=0;
//	shdw_puntos_dynamic=0;
//	shdw_caras_dynamic=0;

	*c=new int[shdw_ncaras*3];
	for(i=0;i<shdw_ncaras*3;i++) (*c)[i]=pry_caras[i];
	delete pry_caras;
	pry_caras=0;
	*nc=shdw_ncaras;
	*np=pry_npuntos;
	*p=pry_puntos;
	cmc->set(*p,*np);

} /* Piece3DObject::ComputeShadow */ 


void Piece3DObject::ComputeDynamicShadow(int angle,Vector light)
{
	ComputeShadow(angle,light,&shdw_npuntos_dynamic,&shdw_ncaras_dynamic,
							  &shdw_puntos_dynamic,&shdw_caras_dynamic,&shdw_cmc_dynamic);
} /* Piece3DObject::ComputeDynamicShadow */ 


void Piece3DObject::ComputeFixedShadows(Vector light)
{
	ComputeShadow(0,light,&shdw_npuntos_0,&shdw_ncaras_0,
						  &shdw_puntos_0,&shdw_caras_0,&shdw_cmc_0);
	ComputeShadow(90,light,&shdw_npuntos_90,&shdw_ncaras_90,
						   &shdw_puntos_90,&shdw_caras_90,&shdw_cmc_90);
	ComputeShadow(180,light,&shdw_npuntos_180,&shdw_ncaras_180,
						    &shdw_puntos_180,&shdw_caras_180,&shdw_cmc_180);
	ComputeShadow(270,light,&shdw_npuntos_270,&shdw_ncaras_270,
						    &shdw_puntos_270,&shdw_caras_270,&shdw_cmc_270);
} /* Piece3DObject::ComputeShadow */ 