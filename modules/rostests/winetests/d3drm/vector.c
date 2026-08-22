/*
 * Copyright 2007 Vijay Kiran Kamuju
 * Copyright 2007 David Adam
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <math.h>

#include "d3drmdef.h"

#include "wine/test.h"

#define PI (4.0f*atanf(1.0f))
#define admit_error 0.000001f

#define expect_mat( expectedmat, gotmat)\
{ \
    int i,j; \
    BOOL equal = TRUE; \
    for (i=0; i<4; i++)\
        {\
         for (j=0; j<4; j++)\
             {\
              if (fabs(expectedmat[i][j]-gotmat[i][j])>admit_error)\
                 {\
                  equal = FALSE;\
                 }\
             }\
        }\
    ok(equal, "Expected matrix=\n(%f,%f,%f,%f\n %f,%f,%f,%f\n %f,%f,%f,%f\n %f,%f,%f,%f\n)\n\n" \
       "Got matrix=\n(%f,%f,%f,%f\n %f,%f,%f,%f\n %f,%f,%f,%f\n %f,%f,%f,%f)\n", \
       expectedmat[0][0],expectedmat[0][1],expectedmat[0][2],expectedmat[0][3], \
       expectedmat[1][0],expectedmat[1][1],expectedmat[1][2],expectedmat[1][3], \
       expectedmat[2][0],expectedmat[2][1],expectedmat[2][2],expectedmat[2][3], \
       expectedmat[3][0],expectedmat[3][1],expectedmat[3][2],expectedmat[3][3], \
       gotmat[0][0],gotmat[0][1],gotmat[0][2],gotmat[0][3], \
       gotmat[1][0],gotmat[1][1],gotmat[1][2],gotmat[1][3], \
       gotmat[2][0],gotmat[2][1],gotmat[2][2],gotmat[2][3], \
       gotmat[3][0],gotmat[3][1],gotmat[3][2],gotmat[3][3] ); \
}

#define expect_quat(expectedquat,gotquat) \
  ok( (fabs(expectedquat.v.x-gotquat.v.x)<admit_error) &&     \
      (fabs(expectedquat.v.y-gotquat.v.y)<admit_error) &&     \
      (fabs(expectedquat.v.z-gotquat.v.z)<admit_error) &&     \
      (fabs(expectedquat.s-gotquat.s)<admit_error), \
  "Expected Quaternion %f %f %f %f , Got Quaternion %f %f %f %f\n", \
      expectedquat.s,expectedquat.v.x,expectedquat.v.y,expectedquat.v.z, \
      gotquat.s,gotquat.v.x,gotquat.v.y,gotquat.v.z);

#define expect_vec(expectedvec,gotvec) \
  ok( ((fabs(expectedvec.x-gotvec.x)<admit_error)&&(fabs(expectedvec.y-gotvec.y)<admit_error)&&(fabs(expectedvec.z-gotvec.z)<admit_error)), \
  "Expected Vector= (%f, %f, %f)\n , Got Vector= (%f, %f, %f)\n", \
  expectedvec.x,expectedvec.y,expectedvec.z, gotvec.x, gotvec.y, gotvec.z);

static void VectorTest(void)
{
    D3DVALUE mod,par,theta;
    D3DVECTOR e,r,u,v,w,axis,casnul,norm,ray,self;

    u.x=2.0f; u.y=2.0f; u.z=1.0f;
    v.x=4.0f; v.y=4.0f; v.z=0.0f;


/*______________________VectorAdd_________________________________*/
    D3DRMVectorAdd(&r,&u,&v);
    e.x=6.0f; e.y=6.0f; e.z=1.0f;
    expect_vec(e,r);

    self.x=9.0f; self.y=18.0f; self.z=27.0f;
    D3DRMVectorAdd(&self,&self,&u);
    e.x=11.0f; e.y=20.0f; e.z=28.0f;
    expect_vec(e,self);

/*_______________________VectorSubtract__________________________*/
    D3DRMVectorSubtract(&r,&u,&v);
    e.x=-2.0f; e.y=-2.0f; e.z=1.0f;
    expect_vec(e,r);

    self.x=9.0f; self.y=18.0f; self.z=27.0f;
    D3DRMVectorSubtract(&self,&self,&u);
    e.x=7.0f; e.y=16.0f; e.z=26.0f;
    expect_vec(e,self);

/*_______________________VectorCrossProduct_______________________*/
    D3DRMVectorCrossProduct(&r,&u,&v);
    e.x=-4.0f; e.y=4.0f; e.z=0.0f;
    expect_vec(e,r);

    self.x=9.0f; self.y=18.0f; self.z=27.0f;
    D3DRMVectorCrossProduct(&self,&self,&u);
    e.x=-36.0f; e.y=45.0f; e.z=-18.0f;
    expect_vec(e,self);

/*_______________________VectorDotProduct__________________________*/
    mod=D3DRMVectorDotProduct(&u,&v);
    ok((mod == 16.0f), "Expected 16.0f, Got %f\n", mod);

/*_______________________VectorModulus_____________________________*/
    mod=D3DRMVectorModulus(&u);
    ok((mod == 3.0f), "Expected 3.0f, Got %f\n", mod);

/*_______________________VectorNormalize___________________________*/
    D3DRMVectorNormalize(&u);
    e.x=2.0f/3.0f; e.y=2.0f/3.0f; e.z=1.0f/3.0f;
    expect_vec(e,u);

/* If u is the NULL vector, MSDN says that the return vector is NULL. In fact, the returned vector is (1,0,0). The following test case prove it. */

    casnul.x=0.0f; casnul.y=0.0f; casnul.z=0.0f;
    D3DRMVectorNormalize(&casnul);
    e.x=1.0f; e.y=0.0f; e.z=0.0f;
    expect_vec(e,casnul);

/*____________________VectorReflect_________________________________*/
    ray.x=3.0f; ray.y=-4.0f; ray.z=5.0f;
    norm.x=1.0f; norm.y=-2.0f; norm.z=6.0f;
    e.x=79.0f; e.y=-160.0f; e.z=487.0f;
    D3DRMVectorReflect(&r,&ray,&norm);
    expect_vec(e,r);

/*_______________________VectorRotate_______________________________*/
    w.x=3.0f; w.y=4.0f; w.z=0.0f;
    axis.x=0.0f; axis.y=0.0f; axis.z=1.0f;
    theta=2.0f*PI/3.0f;
    D3DRMVectorRotate(&r,&w,&axis,theta);
    e.x=-0.3f-0.4f*sqrtf(3.0f); e.y=0.3f*sqrtf(3.0f)-0.4f; e.z=0.0f;
    expect_vec(e,r);

/* The same formula gives D3DRMVectorRotate, for theta in [-PI/2;+PI/2] or not. The following test proves this fact.*/
    theta=-PI/4.0f;
    D3DRMVectorRotate(&r,&w,&axis,theta);
    e.x=1.4f/sqrtf(2.0f); e.y=0.2f/sqrtf(2.0f); e.z=0.0f;
    expect_vec(e,r);

    theta=PI/8.0f;
    D3DRMVectorRotate(&self,&self,&axis,theta);
    e.x=0.989950; e.y=0.141421f; e.z=0.0f;
    expect_vec(e,r);

/*_______________________VectorScale__________________________*/
    par=2.5f;
    D3DRMVectorScale(&r,&v,par);
    e.x=10.0f; e.y=10.0f; e.z=0.0f;
    expect_vec(e,r);

    self.x=9.0f; self.y=18.0f; self.z=27.0f;
    D3DRMVectorScale(&self,&self,2);
    e.x=18.0f; e.y=36.0f; e.z=54.0f;
    expect_vec(e,self);
}

static void MatrixTest(void)
{
    D3DRMQUATERNION q;
    D3DRMMATRIX4D exp,mat;

    exp[0][0]=-49.0f; exp[0][1]=4.0f;   exp[0][2]=22.0f;  exp[0][3]=0.0f;
    exp[1][0]=20.0f;  exp[1][1]=-39.0f; exp[1][2]=20.0f;  exp[1][3]=0.0f;
    exp[2][0]=10.0f;  exp[2][1]=28.0f;  exp[2][2]=-25.0f; exp[2][3]=0.0f;
    exp[3][0]=0.0f;   exp[3][1]=0.0f;   exp[3][2]=0.0f;   exp[3][3]=1.0f;
    q.s=1.0f; q.v.x=2.0f; q.v.y=3.0f; q.v.z=4.0f;

   D3DRMMatrixFromQuaternion(mat,&q);
   expect_mat(exp,mat);
}

static void QuaternionTest(void)
{
    D3DVECTOR axis;
    D3DVALUE par,theta;
    D3DRMQUATERNION q,q1,q1final,q2,q2final,r;

/*_________________QuaternionFromRotation___________________*/
    axis.x=1.0f; axis.y=1.0f; axis.z=1.0f;
    theta=2.0f*PI/3.0f;
    D3DRMQuaternionFromRotation(&r,&axis,theta);
    q.s=0.5f; q.v.x=0.5f; q.v.y=0.5f; q.v.z=0.5f;
    expect_quat(q,r);

/*_________________QuaternionSlerp_________________________*/
/* If the angle of the two quaternions is in ]PI/2;3PI/2[, QuaternionSlerp
 * interpolates between the first quaternion and the opposite of the second one.
 * The test proves this fact. */
    par=0.31f;
    q1.s=1.0f; q1.v.x=2.0f; q1.v.y=3.0f; q1.v.z=50.0f;
    q2.s=-4.0f; q2.v.x=6.0f; q2.v.y=7.0f; q2.v.z=8.0f;
/* The angle between q1 and q2 is in [-PI/2,PI/2]. So, one interpolates between q1 and q2. */
    q.s = -0.55f; q.v.x=3.24f; q.v.y=4.24f; q.v.z=36.98f;
    D3DRMQuaternionSlerp(&r,&q1,&q2,par);
    expect_quat(q,r);

    q1.s=1.0f; q1.v.x=2.0f; q1.v.y=3.0f; q1.v.z=50.0f;
    q2.s=-94.0f; q2.v.x=6.0f; q2.v.y=7.0f; q2.v.z=-8.0f;
/* The angle between q1 and q2 is not in [-PI/2,PI/2]. So, one interpolates between q1 and -q2. */
    q.s=29.83f; q.v.x=-0.48f; q.v.y=-0.10f; q.v.z=36.98f;
    D3DRMQuaternionSlerp(&r,&q1,&q2,par);
    expect_quat(q,r);

/* Test the spherical interpolation part */
    q1.s=0.1f; q1.v.x=0.2f; q1.v.y=0.3f; q1.v.z=0.4f;
    q2.s=0.5f; q2.v.x=0.6f; q2.v.y=0.7f; q2.v.z=0.8f;
    q.s = 0.243943f; q.v.x = 0.351172f; q.v.y = 0.458401f; q.v.z = 0.565629f;

    q1final=q1;
    q2final=q2;
    D3DRMQuaternionSlerp(&r,&q1,&q2,par);
    expect_quat(q,r);

/* Test to show that the input quaternions are not changed */
    expect_quat(q1,q1final);
    expect_quat(q2,q2final);
}

static void ColorTest(void)
{
    D3DCOLOR color, expected_color, got_color;
    D3DVALUE expected, got, red, green, blue, alpha;

/*___________D3DRMCreateColorRGB_________________________*/
    red=0.8f;
    green=0.3f;
    blue=0.55f;
    expected_color=0xffcc4c8c;
    got_color=D3DRMCreateColorRGB(red,green,blue);
    ok((expected_color==got_color),"Expected color=%lx, Got color=%lx\n",expected_color,got_color);

/*___________D3DRMCreateColorRGBA________________________*/
    red=0.1f;
    green=0.4f;
    blue=0.7f;
    alpha=0.58f;
    expected_color=0x931966b2;
    got_color=D3DRMCreateColorRGBA(red,green,blue,alpha);
    ok((expected_color==got_color),"Expected color=%lx, Got color=%lx\n",expected_color,got_color);

/* if a component is <0 then, then one considers this component as 0. The following test proves this fact (test only with the red component). */
    red=-0.88f;
    green=0.4f;
    blue=0.6f;
    alpha=0.41f;
    expected_color=0x68006699;
    got_color=D3DRMCreateColorRGBA(red,green,blue,alpha);
    ok((expected_color==got_color),"Expected color=%lx, Got color=%lx\n",expected_color,got_color);

/* if a component is >1 then, then one considers this component as 1. The following test proves this fact (test only with the red component). */
    red=2.37f;
    green=0.4f;
    blue=0.6f;
    alpha=0.41f;
    expected_color=0x68ff6699;
    got_color=D3DRMCreateColorRGBA(red,green,blue,alpha);
    ok((expected_color==got_color),"Expected color=%lx, Got color=%lx\n",expected_color,got_color);

/*___________D3DRMColorGetAlpha_________________________*/
    color=0x0e4921bf;
    expected=14.0f/255.0f;
    got=D3DRMColorGetAlpha(color);
    ok((fabs(expected-got)<admit_error),"Expected=%f, Got=%f\n",expected,got);

/*___________D3DRMColorGetBlue__________________________*/
    color=0xc82a1455;
    expected=1.0f/3.0f;
    got=D3DRMColorGetBlue(color);
    ok((fabs(expected-got)<admit_error),"Expected=%f, Got=%f\n",expected,got);

/*___________D3DRMColorGetGreen_________________________*/
    color=0xad971203;
    expected=6.0f/85.0f;
    got=D3DRMColorGetGreen(color);
    ok((fabs(expected-got)<admit_error),"Expected=%f, Got=%f\n",expected,got);

/*___________D3DRMColorGetRed__________________________*/
    color=0xb62d7a1c;
    expected=3.0f/17.0f;
    got=D3DRMColorGetRed(color);
    ok((fabs(expected-got)<admit_error),"Expected=%f, Got=%f\n",expected,got);
}

START_TEST(vector)
{
    VectorTest();
    MatrixTest();
    QuaternionTest();
    ColorTest();
}
