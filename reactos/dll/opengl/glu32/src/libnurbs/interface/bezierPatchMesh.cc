/*
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.1 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
** 
** http://oss.sgi.com/projects/FreeB
** 
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
** 
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
** 
** Additional Notice Provisions: The application programming interfaces
** established by SGI in conjunction with the Original Code are The
** OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
** April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
** 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
** Window System(R) (Version 1.3), released October 19, 1998. This software
** was created using the OpenGL(R) version 1.2.1 Sample Implementation
** published by SGI, but has not been independently verified as being
** compliant with the OpenGL(R) version 1.2.1 Specification.
**
*/
/*
*/

#include "gluos.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <GL/gl.h>
#include "bezierEval.h"
#include "bezierPatchMesh.h"

static int isDegenerate(float A[2], float B[2], float C[2]);

void drawStrips(float *vertex_array, float *normal_array, int *length_array, GLenum *type_array, int num_strips)
{
  int i,j,k;
  k=0;
  /*k is the index of the first component of the current vertex*/
  for(i=0; i<num_strips; i++)
    {
      glBegin(type_array[i]);
      for(j=0; j<length_array[i]; j++)
	{
	  glNormal3fv(normal_array+k);
	  glVertex3fv(vertex_array+k);
	  k += 3;
	}
      glEnd();
    }
}

void bezierPatchMeshListDelDeg(bezierPatchMesh* list)
{
  bezierPatchMesh* temp;
  for(temp=list; temp != NULL; temp = temp->next)
    {
      bezierPatchMeshDelDeg(temp);
    }
}

void bezierPatchMeshListDelete(bezierPatchMesh *list)
{
  if(list == NULL) return;
  bezierPatchMeshListDelete(list->next);
  bezierPatchMeshDelete(list);  
}




bezierPatchMesh* bezierPatchMeshListReverse(bezierPatchMesh* list)
{
 bezierPatchMesh* ret=NULL;
 bezierPatchMesh* temp;
 bezierPatchMesh* nextone;
  for(temp = list; temp != NULL; temp = nextone)
    {
      nextone = temp->next;
      ret=bezierPatchMeshListInsert(ret, temp);
    }
 return ret;
}

/*maptype is either GL_MAP2_VERTEX_3 or GL_MAP2_VERTEX_4
 */
bezierPatchMesh *bezierPatchMeshMake(int maptype, float umin, float umax, int ustride, int uorder, float vmin, float vmax, int vstride, int vorder, float *ctlpoints,  int size_UVarray, int size_length_array)
{
  int i,j,k;
  int dimension;
  int the_ustride;
  int the_vstride;

  if(maptype == GL_MAP2_VERTEX_3) dimension = 3;
  else if (maptype==GL_MAP2_VERTEX_4) dimension = 4;
  else {
    fprintf(stderr, "error in inMap2f, maptype=%i is wrong, maptype,map is invalid\n", maptype);
    return NULL;
  }

  bezierPatchMesh *ret = (bezierPatchMesh*) malloc(sizeof(bezierPatchMesh));
  assert(ret);

  ret->bpatch_normal = NULL;
  ret->bpatch_color  = NULL;
  ret->bpatch_texcoord = NULL;
  ret->bpatch = bezierPatchMake(umin, vmin, umax, vmax, uorder, vorder, dimension);

  /*copy the control points there*/
  the_ustride = vorder * dimension;
  the_vstride = dimension;
  for(i=0; i<uorder; i++)
    for(j=0; j<vorder; j++)
      for(k=0; k<dimension; k++)
	ret->bpatch->ctlpoints[i * the_ustride + j*the_vstride+k] = ctlpoints[i*ustride+j*vstride+k];
  

  ret->size_UVarray = size_UVarray;
  ret->size_length_array = size_length_array;
  ret->UVarray = (float*) malloc(sizeof(float) * size_UVarray);
  assert(ret->UVarray);
  ret->length_array = (int *)malloc(sizeof(int) * size_length_array);
  assert(ret->length_array);
  ret->type_array = (GLenum *)malloc(sizeof(GLenum) * size_length_array);
  assert(ret->type_array);

  ret->index_UVarray = 0;
  ret->index_length_array = 0;

  ret->vertex_array = NULL;
  ret->normal_array = NULL;
  ret->color_array  = NULL;
  ret->texcoord_array = NULL;

  ret->next = NULL;
  return ret;
}

bezierPatchMesh *bezierPatchMeshMake2(int size_UVarray, int size_length_array)
{
  bezierPatchMesh *ret = (bezierPatchMesh*) malloc(sizeof(bezierPatchMesh));
  assert(ret);

  ret->bpatch = NULL;
  ret->bpatch_normal = NULL;
  ret->bpatch_color  = NULL;
  ret->bpatch_texcoord = NULL;

  ret->size_UVarray = size_UVarray;
  ret->size_length_array = size_length_array;
  ret->UVarray = (float*) malloc(sizeof(float) * size_UVarray);
  assert(ret->UVarray);
  ret->length_array = (int *)malloc(sizeof(int) * size_length_array);
  assert(ret->length_array);
  ret->type_array = (GLenum *)malloc(sizeof(GLenum) * size_length_array);
  assert(ret->type_array);

  ret->index_UVarray = 0;
  ret->index_length_array = 0;

  ret->vertex_array = NULL;
  ret->normal_array = NULL;
  ret->color_array  = NULL;
  ret->texcoord_array = NULL;

  ret->next = NULL;
  return ret;
}

void bezierPatchMeshPutPatch(bezierPatchMesh *bpm, int maptype, float umin, float umax, int ustride, int uorder, float vmin, float vmax, int vstride, int vorder, float *ctlpoints)
{
  switch(maptype){
  case GL_MAP2_VERTEX_3:
    bpm->bpatch = bezierPatchMake2(umin, vmin, umax, vmax, uorder, vorder, 3, ustride, vstride, ctlpoints);
    break;
  case GL_MAP2_VERTEX_4:
    bpm->bpatch = bezierPatchMake2(umin, vmin, umax, vmax, uorder, vorder, 4,ustride, vstride, ctlpoints );
    break;
  case GL_MAP2_NORMAL:
    bpm->bpatch_normal = bezierPatchMake2(umin, vmin, umax, vmax, uorder, vorder, 3, ustride, vstride, ctlpoints);
    break;
  case GL_MAP2_INDEX:
    bpm->bpatch_color = bezierPatchMake2(umin, vmin, umax, vmax, uorder, vorder, 1, ustride, vstride, ctlpoints);
    break;
  case GL_MAP2_COLOR_4:
    bpm->bpatch_color = bezierPatchMake2(umin, vmin, umax, vmax, uorder, vorder, 4, ustride, vstride, ctlpoints);
    break;
  case GL_MAP2_TEXTURE_COORD_1:
    bpm->bpatch_texcoord = bezierPatchMake2(umin, vmin, umax, vmax, uorder, vorder, 1, ustride, vstride, ctlpoints);
    break;
  case GL_MAP2_TEXTURE_COORD_2:
    bpm->bpatch_texcoord = bezierPatchMake2(umin, vmin, umax, vmax, uorder, vorder, 2, ustride, vstride, ctlpoints);
    break;    
  case GL_MAP2_TEXTURE_COORD_3:
    bpm->bpatch_texcoord = bezierPatchMake2(umin, vmin, umax, vmax, uorder, vorder, 3, ustride, vstride, ctlpoints);
    break;    
  case GL_MAP2_TEXTURE_COORD_4:
    bpm->bpatch_texcoord = bezierPatchMake2(umin, vmin, umax, vmax, uorder, vorder, 4, ustride, vstride, ctlpoints);
    break;    
  default:
    fprintf(stderr, "error in bezierPatchMeshPutPatch, maptype=%i is wrong, maptype,map is invalid\n", maptype);
  }
}
  

/*delete everything including the arrays. So if you want to output the
 *pointers of the arrays, you should not use this function to deallocate space.
 *you should dealocate manually
 */
void bezierPatchMeshDelete(bezierPatchMesh *bpm)
{
  if(bpm->bpatch != NULL)
    bezierPatchDelete(bpm->bpatch);
  if(bpm->bpatch_normal != NULL)
    bezierPatchDelete(bpm->bpatch_normal);
  if(bpm->bpatch_color != NULL)
    bezierPatchDelete(bpm->bpatch_color);
  if(bpm->bpatch_texcoord != NULL)
    bezierPatchDelete(bpm->bpatch_texcoord);
  
  free(bpm->UVarray);
  free(bpm->length_array);
  free(bpm->vertex_array);
  free(bpm->normal_array);
  free(bpm->type_array);
  free(bpm);
}
 
/*begin a strip
 *type is the primitive type:
 */
void bezierPatchMeshBeginStrip(bezierPatchMesh *bpm, GLenum type)
{
  bpm->counter = 0;
  bpm->type = type;
}

/*signal the end of the current strip*/
void bezierPatchMeshEndStrip(bezierPatchMesh *bpm)
{
  int i;
  
  /*if there are no vertices in this strip, then nothing needs to be done*/
  if(bpm->counter == 0) return;
  
  /*if the length_array is full, it should be expanded*/
  if(bpm->index_length_array >= bpm->size_length_array)
    {
      int *temp = (int*) malloc(sizeof(int) * (bpm->size_length_array*2 + 1));
      assert(temp);
      GLenum *temp_type = (GLenum*) malloc(sizeof(GLenum) * (bpm->size_length_array*2 + 1));
      assert(temp_type);
      /*update the size*/
      bpm->size_length_array = bpm->size_length_array*2 + 1;
      
      /*copy*/
      for(i=0; i<bpm->index_length_array; i++)
	{
	  temp[i] = bpm->length_array[i];
	  temp_type[i] = bpm->type_array[i];
	}
      
      /*deallocate old array*/
      free(bpm->length_array);
      free(bpm->type_array);
      
      /*point to the new array which is twice as bigger*/
      bpm->length_array = temp;
      bpm->type_array = temp_type;
    }
  bpm->type_array[bpm->index_length_array] = bpm->type;
  bpm->length_array[bpm->index_length_array++] = bpm->counter;

}

/*insert (u,v) */
void bezierPatchMeshInsertUV(bezierPatchMesh *bpm, float u, float v)
{
  int i;
  /*if the UVarray is full, it should be expanded*/
  if(bpm->index_UVarray+1 >= bpm->size_UVarray)
    {
      float *temp = (float*) malloc(sizeof(float) * (bpm->size_UVarray * 2 + 2));
      assert(temp);
      
      /*update the size*/
      bpm->size_UVarray = bpm->size_UVarray*2 + 2;
      
      /*copy*/
      for(i=0; i<bpm->index_UVarray; i++)
	{
	  temp[i] = bpm->UVarray[i];
	}
      
      /*deallocate old array*/
      free(bpm->UVarray);
      
      /*pointing to the new arrays*/
      bpm->UVarray = temp;
    }
  /*insert the new UV*/
  bpm->UVarray[bpm->index_UVarray] = u;
  bpm->index_UVarray++;
  bpm->UVarray[bpm->index_UVarray] = v;
  bpm->index_UVarray++;

  /*update counter: one more vertex*/
  bpm->counter++;


}

void bezierPatchMeshPrint(bezierPatchMesh *bpm)
{
  int i;
  printf("the bezier patch is\n");
  bezierPatchPrint(bpm->bpatch);
  printf("index_length_array= %i\n", bpm->index_length_array);
  printf("size_length_array =%i\n", bpm->size_length_array);
  printf("index_UVarray =%i\n", bpm->index_UVarray);
  printf("size_UVarray =%i\n", bpm->size_UVarray);
  printf("UVarray is\n");
  for(i=0; i<bpm->index_UVarray; i++)
    printf("%f ", bpm->UVarray[i]);

  printf("length_array is\n");
  for(i=0; i<bpm->index_length_array; i++)
    printf("%i ", bpm->length_array[i]);
  printf("\n");

}

/*insert a new patch in front of the current linked list and return the new list*/
bezierPatchMesh* bezierPatchMeshListInsert(bezierPatchMesh* list, bezierPatchMesh* bpm)
{
  bpm->next=list;
  return bpm;
}

/*print all the patches*/
void bezierPatchMeshListPrint(bezierPatchMesh* list)
{
  bezierPatchMesh *temp;
  for(temp = list; temp != NULL; temp = temp->next)
    {
      bezierPatchMeshPrint(temp);
    }
}

int bezierPatchMeshListTotalStrips(bezierPatchMesh* list)
{
  int sum=0;
  bezierPatchMesh *temp;
  for(temp=list; temp != NULL; temp = temp->next)
    {
      sum += temp->index_length_array;
    }
  return sum;
}

int bezierPatchMeshListTotalVert(bezierPatchMesh* list)
{
  int sum=0;
  bezierPatchMesh *temp;
  for(temp=list; temp != NULL; temp = temp->next)
    {
      sum += temp->index_UVarray;
    }
  return sum/2;
}

int bezierPatchMeshListNumTriangles(bezierPatchMesh* list)
{
  int sum=0;
  bezierPatchMesh* temp;
  for(temp=list; temp != NULL; temp = temp->next)
    {
      sum +=  bezierPatchMeshNumTriangles(temp);
    }
  return sum;
}

int bezierPatchMeshNumTriangles(bezierPatchMesh* bpm)
{
  int i;
  int sum=0;
  for(i=0; i<bpm->index_length_array; i++)
    {
      switch(bpm->type_array[i])
	{
	case GL_TRIANGLES:
	  sum += bpm->length_array[i]/3;
	  break;
	case GL_TRIANGLE_FAN:
	  if(bpm->length_array[i] > 2)
	    sum += bpm->length_array[i]-2;
	  break;
	case GL_TRIANGLE_STRIP:
	  if(bpm->length_array[i] > 2)
	    sum += bpm->length_array[i]-2;
	  break;
	case GL_QUAD_STRIP:
	  if(bpm->length_array[i]>2)
	    sum += (bpm->length_array[i]-2);
	  break;
	default:
	  fprintf(stderr,"error in bezierPatchMeshListNumTriangles, type invalid\n");
	}
    }
  return sum;
}

/*delete degenerate triangles*/
void bezierPatchMeshDelDeg(bezierPatchMesh* bpm)
{
  if(bpm == NULL) return;
  int i,j,k;
  int *new_length_array;
  GLenum *new_type_array;
  int index_new_length_array;
  float *new_UVarray;
  int index_new_UVarray;

  new_length_array = (int*)malloc(sizeof(int) * bpm->index_length_array);
  assert(new_length_array);
  new_type_array = (GLenum*)malloc(sizeof(GLenum) * bpm->index_length_array);
  assert(new_length_array);
  new_UVarray = (float*) malloc(sizeof(float) * bpm->index_UVarray);
  assert(new_UVarray);

  index_new_length_array = 0;
  index_new_UVarray=0;
  k=0;
  for(i=0; i<bpm->index_length_array; i++){
    
    /*(if not degenerate, we have to copy*/
    if( (bpm->length_array[i] != 3) || (!isDegenerate(bpm->UVarray+k, bpm->UVarray+k+2, bpm->UVarray+k+4)))
	  {
	    for(j=0; j<2* bpm->length_array[i]; j++)
	      new_UVarray[index_new_UVarray++] = bpm->UVarray[k++];

	    new_length_array[index_new_length_array] = bpm->length_array[i];
	    new_type_array[index_new_length_array] = bpm->type_array[i];
	    index_new_length_array++;
	  }
    else
      {
	k += 6;
      }
  }  
  free(bpm->UVarray);
  free(bpm->length_array);
  free(bpm->type_array);
  bpm->UVarray=new_UVarray;
  bpm->length_array=new_length_array;
  bpm->type_array=new_type_array;
  bpm->index_UVarray = index_new_UVarray;
  bpm->index_length_array = index_new_length_array;
  
}

/*(u,v) to XYZ
 *the xyz and normals are stored in vertex_array, 
 *and normal_array. the spaces of both are allocated here
 */
void bezierPatchMeshEval(bezierPatchMesh* bpm)
{
  int i,j,k,l;
  float u,v;
  float u0 = bpm->bpatch->umin;
  float u1 = bpm->bpatch->umax;
  int uorder = bpm->bpatch->uorder;
  float v0 = bpm->bpatch->vmin;
  float v1 = bpm->bpatch->vmax;
  int vorder = bpm->bpatch->vorder;
  int dimension = bpm->bpatch->dimension;
  int ustride = dimension * vorder;
  int vstride = dimension;
  float *ctlpoints = bpm->bpatch->ctlpoints;
  
  bpm->vertex_array = (float*) malloc(sizeof(float)* (bpm->index_UVarray/2) * 3);
  assert(bpm->vertex_array);
  bpm->normal_array = (float*) malloc(sizeof(float)* (bpm->index_UVarray/2) * 3);
  assert(bpm->normal_array);

  k=0;
  l=0;
  for(i=0; i<bpm->index_length_array; i++)
    {
      for(j=0; j<bpm->length_array[i]; j++)
	{
	  u = bpm->UVarray[k];
	  v = bpm->UVarray[k+1];
	  bezierSurfEval(u0,u1,uorder, v0, v1, vorder, dimension, ctlpoints, ustride, vstride, u,v, bpm->vertex_array+l);
	  bezierSurfEvalNormal(u0,u1,uorder, v0, v1, vorder, dimension, ctlpoints, ustride, vstride, u,v, bpm->normal_array+l);
	  k += 2;
	  l += 3;
	}
    }
}
    
void bezierPatchMeshListEval(bezierPatchMesh* list)
{
  bezierPatchMesh* temp;
  for(temp = list; temp != NULL; temp = temp->next)
    {
      bezierPatchMeshEval(temp);
    }
}

void bezierPatchMeshDraw(bezierPatchMesh* bpm)
{
  int i,j,k;
  k=0;
  /*k is the index of the first component of the current vertex*/
  for(i=0; i<bpm->index_length_array; i++)
    {
      glBegin(bpm->type_array[i]);
      for(j=0; j<bpm->length_array[i]; j++)
	{
	  glNormal3fv(bpm->normal_array+k);
	  glVertex3fv(bpm->vertex_array+k);
	  k+= 3;
	}
      glEnd();
    }
}

void bezierPatchMeshListDraw(bezierPatchMesh* list)
{
  bezierPatchMesh* temp;
  for(temp = list; temp != NULL; temp = temp->next)
    {
      bezierPatchMeshDraw(temp);
    }
}

void bezierPatchMeshListCollect(bezierPatchMesh* list, float **vertex_array, float **normal_array, int **length_array, GLenum **type_array, int *num_strips)
{
  int i,j,k,l;
  bezierPatchMesh *temp;
  int total_num_vertices = bezierPatchMeshListTotalVert(list);
  (*vertex_array) = (float *) malloc(sizeof(float) * total_num_vertices*3);
  assert(*vertex_array);
  (*normal_array) = (float *) malloc(sizeof(float) * total_num_vertices*3);
  assert(*normal_array);

  *num_strips = bezierPatchMeshListTotalStrips(list);
   
  *length_array = (int*) malloc(sizeof(int) * (*num_strips));
  assert(*length_array);

  *type_array = (GLenum*) malloc(sizeof(GLenum) * (*num_strips));
  assert(*type_array);
  
  k=0;
  l=0;
  for(temp = list; temp != NULL; temp = temp->next)
    {
      int x=0;
      for(i=0; i<temp->index_length_array; i++)
	{
	  for(j=0; j<temp->length_array[i]; j++)
	    {
	      (*vertex_array)[k] = temp->vertex_array[x];
	      (*vertex_array)[k+1] = temp->vertex_array[x+1];
	      (*vertex_array)[k+2] = temp->vertex_array[x+2];

	      (*normal_array)[k] = temp->normal_array[x];
	      (*normal_array)[k+1] = temp->normal_array[x+1];
	      (*normal_array)[k+2] = temp->normal_array[x+2];

	      x += 3;
	      k += 3;
	    }
	  (*type_array)[l]  = temp->type_array[i];
	  (*length_array)[l++] = temp->length_array[i];
	}
    }
}



static int isDegenerate(float A[2], float B[2], float C[2])
{
  if( (A[0] == B[0] && A[1]==B[1]) ||
      (A[0] == C[0] && A[1]==C[1]) ||
      (B[0] == C[0] && B[1]==C[1])
     )
    return 1;
  else
    return 0;
}




