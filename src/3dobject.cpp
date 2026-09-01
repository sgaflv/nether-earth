#include "glport.h"
#include "assets.h"


#include "string.h"
#include "stdio.h"

#include "cmc.h"
#include "3dobject.h"

#include "vector.h"
#include "myglutaux.h"


#define ST_INIT		0
#define ST_DATA		1

extern void Normal (double vector1[3],double vector2[3],double resultado[3]);



C3DObject::C3DObject()
{
	puntos=0;
	ncaras=0;
	puntos=0;
	normales=0;
	caras=0;
	r=0;
	g=0;
	b=0;
	display_list=-1;
	tx=0;
	ty=0;
	textures=0;
	drw_vtx=0;
	drw_nor=0;
	drw_txc=0;
	drw_built=false;
} /* C3DObject::C3DObject */ 



C3DObject::C3DObject(const char *file,const char *texturedir)
{
	int l;

	npuntos=0;
	ncaras=0;
	puntos=0;
	normales=0;
	caras=0;
	r=g=b=0;
	display_list=-1;
	tx=0;
	ty=0;
	textures=0;
	drw_vtx=0;
	drw_nor=0;
	drw_txc=0;
	drw_built=false;

	l=strlen(file);

	if (file[l-1]=='c' || file[l-1]=='C') loadASC(file);
									 else loadASE(file,texturedir);
} /* C3DObject::C3DObject */ 


bool C3DObject::loadASC(const char *file)
{
	FILE *fp;
	char buffer[256],buffer2[256],buffer3[256],buffer4[256],buffer5[256];
	bool end;
	int state;
	int i;

	int act_face,tmp;
	float x,y,z;
	int p1,p2,p3;

	int *smooth;

	fp=asset_open(file,"r");
	if (fp==NULL) return false;

	/* Import a 3DStudio .ASC file */ 
	end=false;
	state=ST_INIT; 
	do{
		if (1!=fscanf(fp,"%s",buffer)) {
			end=true;
		} else {
			switch(state) {
			case ST_INIT:
				if (strcmp(buffer,"Vertices:")==0) {
					if (1==fscanf(fp,"%i",&npuntos)) {
						puntos=new float[npuntos*3];
						for(i=0;i<npuntos*3;i++) puntos[i]=0.0F;
						if (ncaras!=0) state=ST_DATA;
					} /* if */ 
				} /* if */ 
				if (strcmp(buffer,"Faces:")==0) {
					if (1==fscanf(fp,"%i",&ncaras)) {
						caras=new int[ncaras*3];
						smooth=new int[ncaras];
						r=new float[ncaras];
						g=new float[ncaras];
						b=new float[ncaras];
						for(i=0;i<ncaras*3;i++) {
							caras[i]=0;
						} /* for */ 
						for(i=0;i<ncaras;i++) {
							r[i]=0.5;
							g[i]=0.5;
							b[i]=0.5;
						} /* for */ 
						if (npuntos!=0) state=ST_DATA;
					} /* if */ 
				} /* if */ 
				break;
			case ST_DATA:
				if (strcmp(buffer,"Mapped")==0) {
					/* ... */ 
				} /* if */ 
				if (strcmp(buffer,"Vertex")==0) {
					if (1==fscanf(fp,"%s",buffer2) &&
						0!=strcmp(buffer2,"list:")) {
						if (3==fscanf(fp,"%s %s %s",buffer3,buffer4,buffer5)) {
							buffer2[strlen(buffer2)]=0;
							if (1==sscanf(buffer2,"%i",&tmp) &&
								1==sscanf(buffer3+2,"%f",&x) &&
								1==sscanf(buffer4+2,"%f",&y) &&
								1==sscanf(buffer5+2,"%f",&z)) {
								puntos[tmp*3]=x;
								puntos[tmp*3+1]=y;
								puntos[tmp*3+2]=z;
								/* Test for textures: */ 
								if (textures!=0) {
									if (2==fscanf(fp,"%s %s",buffer3,buffer4)) {
										if (1==sscanf(buffer3+2,"%f",&x) &&
											1==sscanf(buffer4+2,"%f",&y)) {
											/* ... */ 
										} /* if */ 
									} /* if */ 
								}/* if */ 
							} /* if */ 
						} /* if */ 
					} /* if */ 
				} /* if */ 
				if (strcmp(buffer,"Face")==0) {
					if (1==fscanf(fp,"%s",buffer2) &&
						0!=strcmp(buffer2,"list:")) {
						if (3==fscanf(fp,"%s %s %s",buffer3,buffer4,buffer5)) {
							buffer2[strlen(buffer2)]=0;
							if (1==sscanf(buffer2,"%i",&act_face) &&
								1==sscanf(buffer3+2,"%i",&p1) &&
								1==sscanf(buffer4+2,"%i",&p2) &&
								1==sscanf(buffer5+2,"%i",&p3)) {
								caras[act_face*3]=p1;
								caras[act_face*3+1]=p2;
								caras[act_face*3+2]=p3;
								smooth[act_face]=0;
							} /* if */ 
						} /* if */ 
					} /* if */ 
				} /* if */ 

				if (strcmp(buffer,"Material:")==0) {
					int c,i;

					buffer2[0]=0;
					while(fgetc(fp)!='\"');
					i=0;
					do{
						c=fgetc(fp);
						buffer2[i++]=c;
					}while(c!='\"');
					i--;
					buffer2[i++]='.';
					buffer2[i++]='b';
					buffer2[i++]='m';
					buffer2[i++]='p';
					buffer2[i]=0;
					/* ... */ 
				} /* if */ 

				if (strcmp(buffer,"Smoothing:")==0) {
					if (1==fscanf(fp,"%i",&p1)) {
						smooth[act_face]=p1;
					} /* if */ 					
				} /* if */ 
				break;
			} /* switch */ 
		} /* if */ 
	}while(!end);

	if (state==ST_INIT) {
		if (puntos!=NULL) delete []puntos;
		if (caras!=NULL) delete []caras;
		npuntos=0;
		ncaras=0;
		puntos=NULL;
		caras=NULL;
		return false;
	} /* if */ 

	CalculaNormales(smooth);
	cmc.set(puntos,npuntos);

	delete []smooth;

	fclose(fp); 
	return true;
} /* C3DObject::loadASC */ 


float C3DObject::normalize(void)
{
	return normalize(1.0);
} /* normalize */ 


float C3DObject::normalize(float c)
{
	int i;
	float cx,fx,cy,fy,cz,fz,factor;

	cmc.set(puntos,npuntos);
	fx=(cmc.x[1]-cmc.x[0])/2;
	fy=(cmc.y[1]-cmc.y[0])/2;
	fz=(cmc.z[1]-cmc.z[0])/2;
	cx=(cmc.x[1]+cmc.x[0])/2;
	cy=(cmc.y[1]+cmc.y[0])/2;
	cz=(cmc.z[1]+cmc.z[0])/2;

	factor=fx;
	if (fy>factor) factor=fy;
	if (fz>factor) factor=fz;

	factor/=c;

	for(i=0;i<npuntos;i++) {
		puntos[i*3]=(puntos[i*3]-cx)/factor;
		puntos[i*3+1]=(puntos[i*3+1]-cy)/factor;
		puntos[i*3+2]=(puntos[i*3+2]-cz)/factor;
	} /* for */ 
	cmc.set(puntos,npuntos);

	return factor;
} /* normalize */ 


float C3DObject::normalizexy(float c)
{
	int i;
	float cx,fx,cy,fy,cz,factor;

	cmc.set(puntos,npuntos);
	fx=(cmc.x[1]-cmc.x[0])/2;
	fy=(cmc.y[1]-cmc.y[0])/2;
	cx=(cmc.x[1]+cmc.x[0])/2;
	cy=(cmc.y[1]+cmc.y[0])/2;
	cz=(cmc.z[1]+cmc.z[0])/2;

	factor=fx;
	if (fy>factor) factor=fy;

	factor/=c;

	for(i=0;i<npuntos;i++) {
		puntos[i*3]=(puntos[i*3]-cx)/factor;
		puntos[i*3+1]=(puntos[i*3+1]-cy)/factor;
		puntos[i*3+2]=(puntos[i*3+2]-cz)/factor;
	} /* for */ 
	cmc.set(puntos,npuntos);

	return factor;
} /* normalizexy */ 


void C3DObject::makepositive(void)
{
	int i;

	cmc.set(puntos,npuntos);

	for(i=0;i<npuntos;i++) {
		puntos[i*3]-=cmc.x[0];
		puntos[i*3+1]-=cmc.y[0];
		puntos[i*3+2]-=cmc.z[0];
	} /* for */ 
	cmc.set(puntos,npuntos);
} /* makepositive */ 


void C3DObject::makepositivex(void)
{
	int i;

	cmc.set(puntos,npuntos);

	for(i=0;i<npuntos;i++) {
		puntos[i*3]-=cmc.x[0];
	} /* for */ 
	cmc.set(puntos,npuntos);
} /* makepositive */ 


void C3DObject::makepositivey(void)
{
	int i;

	cmc.set(puntos,npuntos);

	for(i=0;i<npuntos;i++) {
		puntos[i*3+1]-=cmc.y[0];
	} /* for */ 
	cmc.set(puntos,npuntos);
} /* makepositive */ 


void C3DObject::makepositivez(void)
{
	int i;

	cmc.set(puntos,npuntos);

	for(i=0;i<npuntos;i++) {
		puntos[i*3+2]-=cmc.z[0];
	} /* for */ 
	cmc.set(puntos,npuntos);
} /* makepositive */ 


void C3DObject::moveobject(float x,float y,float z)
{
	int i;

	for(i=0;i<npuntos;i++) {
		puntos[i*3]+=x;
		puntos[i*3+1]+=y;
		puntos[i*3+2]+=z;
	} /* for */ 
	cmc.set(puntos,npuntos);
} /* moveobject */ 



void C3DObject::CalculaNormales(int *smooth)
{

	/* Calculate the face normals: */ 
	int i,j,k,act_vertex;
	float *normales_tmp;
	double vector1[3],vector2[3],normal[3];
	int num;

	normales=new float[ncaras*3*3];
	normales_tmp=new float[ncaras*3];

	for(i=0;i<ncaras;i++) {
	    vector1[0]=puntos[caras[i*3+1]*3]-puntos[caras[i*3]*3];
		vector1[1]=puntos[caras[i*3+1]*3+1]-puntos[caras[i*3]*3+1];
	    vector1[2]=puntos[caras[i*3+1]*3+2]-puntos[caras[i*3]*3+2];
	    vector2[0]=puntos[caras[i*3+2]*3]-puntos[caras[i*3]*3];
	    vector2[1]=puntos[caras[i*3+2]*3+1]-puntos[caras[i*3]*3+1];
	    vector2[2]=puntos[caras[i*3+2]*3+2]-puntos[caras[i*3]*3+2];
	    Normal(vector1,vector2,normal);		
		normales_tmp[i*3]=float(normal[0]);
		normales_tmp[i*3+1]=float(normal[1]);
		normales_tmp[i*3+2]=float(normal[2]);
	} /* for */ 

	/* Calculate the normal of each vertex according to its "smooth" groups: */ 
	for(i=0;i<ncaras;i++) {
		for(j=0;j<3;j++) {
			act_vertex=caras[i*3+j];
			if (smooth[i]==0) {
				normales[i*9+j*3]=normales_tmp[i*3];
				normales[i*9+j*3+1]=normales_tmp[i*3+1];
				normales[i*9+j*3+2]=normales_tmp[i*3+2];
			} else {
				num=0;
				normales[i*9+j*3]=0.0F;
				normales[i*9+j*3+1]=0.0F;
				normales[i*9+j*3+2]=0.0F;
				for(k=0;k<ncaras;k++) {
					if (smooth[k]==smooth[i] &&
						(caras[k*3]==act_vertex || caras[k*3+1]==act_vertex || caras[k*3+2]==act_vertex)) {
						num++;
						normales[i*9+j*3]+=normales_tmp[k*3];
						normales[i*9+j*3+1]+=normales_tmp[k*3+1];
						normales[i*9+j*3+2]+=normales_tmp[k*3+2];
					} /* if */ 
				} /* for */ 
				if (num!=0) {
					normales[i*9+j*3]/=num;
					normales[i*9+j*3+1]/=num;
					normales[i*9+j*3+2]/=num;
				} /* if */ 
			} /* if */ 
		} /* for */ 
	} /* for */ 

	delete []normales_tmp;

} /* C3DObject::CalculaNormales */ 


C3DObject::~C3DObject()
{
	discard_draw_buffers();
	if (puntos!=NULL) delete []puntos;
	if (normales!=NULL) delete []normales;
	if (caras!=NULL) delete []caras;
	if (r!=NULL) delete []r;
	if (g!=NULL) delete []g;
	if (b!=NULL) delete []b;
	if (tx!=0) delete tx;
	if (ty!=0) delete ty;
	if (textures!=0) delete textures;
	tx=0;
	ty=0;
	textures=0;
} /* C3DObject::~CObject */ 



/* Build index-expanded vertex/normal/texcoord buffers for GLES drawing.
 * Each face is expanded into 3 explicit vertices so glDrawArrays works. */
bool C3DObject::build_draw_buffers(void)
{
	int i,off1;

	discard_draw_buffers();

	if (!valid() || ncaras<=0) return false;

	drw_vtx=new float[ncaras*3*3];
	drw_nor=new float[ncaras*3*3];
	if (drw_vtx==0 || drw_nor==0) return false;
	if (textures!=0 && tx!=0 && ty!=0) {
		drw_txc=new float[ncaras*3*2];
		if (drw_txc==0) return false;
	}

	for(i=0,off1=0;i<ncaras;i++) {
		int k;
		for(k=0;k<3;k++) {
			int idx=caras[i*3+k];
			drw_vtx[off1*3+0]=puntos[idx*3+0];
			drw_vtx[off1*3+1]=puntos[idx*3+1];
			drw_vtx[off1*3+2]=puntos[idx*3+2];
			drw_nor[off1*3+0]=normales[i*9+k*3+0];
			drw_nor[off1*3+1]=normales[i*9+k*3+1];
			drw_nor[off1*3+2]=normales[i*9+k*3+2];
			if (drw_txc!=0) {
				drw_txc[off1*2+0]=tx[i*3+k];
				drw_txc[off1*2+1]=ty[i*3+k];
			}
			off1++;
		}
	}

	drw_built=true;
	return true;
} /* build_draw_buffers */


void C3DObject::discard_draw_buffers(void)
{
	if (drw_vtx!=0) delete []drw_vtx;
	if (drw_nor!=0) delete []drw_nor;
	if (drw_txc!=0) delete []drw_txc;
	drw_vtx=0;
	drw_nor=0;
	drw_txc=0;
	drw_built=false;
} /* discard_draw_buffers */



bool C3DObject::valid(void)
{
	if (npuntos!=0 && ncaras!=0 && puntos!=NULL && normales!=NULL && caras!=NULL) return true;

	return false;
} /* C3DObject::valid */ 



void C3DObject::draw(void)
{
	int i;

	if (!drw_built) build_draw_buffers();

	if (drw_vtx==0) return;

	if (textures!=0 && drw_txc!=0) {
		/* Textured: each face binds its own texture. */
		glEnable(GL_TEXTURE_2D);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glVertexPointer(3,GL_FLOAT,0,drw_vtx);
		glNormalPointer(GL_FLOAT,0,drw_nor);
		glTexCoordPointer(2,GL_FLOAT,0,drw_txc);

		for(i=0;i<ncaras;i++) {
			glBindTexture(GL_TEXTURE_2D,textures[i]);
			glColor3f(1,1,1);
			glDrawArrays(GL_TRIANGLES,i*3,3);
		} /* for */ 		
		glDisable(GL_TEXTURE_2D);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
	} else {
		/* Untextured: draw all faces at once. */
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);
		glVertexPointer(3,GL_FLOAT,0,drw_vtx);
		glNormalPointer(GL_FLOAT,0,drw_nor);
		glDrawArrays(GL_TRIANGLES,0,ncaras*3);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
	} /* if */ 
	
} /* C3DObject::draw */ 


void C3DObject::draw(float r,float g,float b)
{
	int i;

	if (!drw_built) build_draw_buffers();

	if (drw_vtx==0) return;

	if (textures!=0 && drw_txc!=0) {
		/* Textured: per-face textures. */
		glEnable(GL_TEXTURE_2D);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glVertexPointer(3,GL_FLOAT,0,drw_vtx);
		glNormalPointer(GL_FLOAT,0,drw_nor);
		glTexCoordPointer(2,GL_FLOAT,0,drw_txc);

		for(i=0;i<ncaras;i++) {
			glBindTexture(GL_TEXTURE_2D,textures[i]);
			glColor3f(1,1,1);
			glDrawArrays(GL_TRIANGLES,i*3,3);
		} /* for */ 		
		glDisable(GL_TEXTURE_2D);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
	} else {
		/* Untextured flat color: all faces at once. */
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);
		glVertexPointer(3,GL_FLOAT,0,drw_vtx);
		glNormalPointer(GL_FLOAT,0,drw_nor);
		glColor3f(r,g,b);
		glDrawArrays(GL_TRIANGLES,0,ncaras*3);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
	} /* if */ 
} /* C3DObject::draw */ 


void C3DObject::draw_notexture(float r,float g,float b)
{
	if (!drw_built) build_draw_buffers();
	if (drw_vtx==0) return;

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glVertexPointer(3,GL_FLOAT,0,drw_vtx);
	glNormalPointer(GL_FLOAT,0,drw_nor);
	glColor3f(r,g,b);
	glDrawArrays(GL_TRIANGLES,0,ncaras*3);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
} /* draw_notexture */ 


void C3DObject::draw_notexture(float r,float g,float b,float a)
{
	if (!drw_built) build_draw_buffers();
	if (drw_vtx==0) return;

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glVertexPointer(3,GL_FLOAT,0,drw_vtx);
	glNormalPointer(GL_FLOAT,0,drw_nor);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glColor4f(r,g,b,a);
	glDrawArrays(GL_TRIANGLES,0,ncaras*3);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisable(GL_BLEND);
} /* draw_notexture */ 


void C3DObject::drawcmc(float r,float g,float b)
{
	cmc.draw(r,g,b);
} /* C3DObject::drawcmc */ 





void C3DObject::refresh_display_lists(void)
{
	discard_draw_buffers();
	display_list=-1;
} /* C3DObject::refresh_display_lists */ 



