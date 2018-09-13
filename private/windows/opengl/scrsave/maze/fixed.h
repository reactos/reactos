#ifndef __FIXED_H__
#define __FIXED_H__

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif
    
#ifndef PI
#define PI (3.1415926535897932384626433832795028841971693993751)
#endif

/* Default table size for precomputed sincos table */
#define FA_TABLE_SIZE 360

/*
   Flags for initialization
   FA_CARTESIAN_Y       - Y axis is positive up
 */
#define FA_DEFAULT      0
#define FA_CARTESIAN_Y  1
    
#if defined(FX_DOUBLE) || defined(FX_SINGLE)

#ifdef FX_DOUBLE
typedef double FxValue;

#define FX_MAX_VALUE (1e100)
#define FX_MIN_VALUE (1e-10)
#else
typedef float FxValue;

#define FX_MAX_VALUE (1e38f)
#define FX_MIN_VALUE (1e-7f)
#endif

#define FxVal(i) ((FxValue)(i))
#define FxInt(v) ((int)(v))
#define FxFltVal(f) ((FxValue)(f))
#define FxFlt(v) ((double)(v))
#define FxPromote(v) (v)
#define FxDemote(v) (v)

#define FxMul(a, b) ((a)*(b))
#define FxDemotedMul(a, b) FxMul(a, b)
#define FxDiv(a, b) ((a)/(b))
#define FxDemotedDiv(a, b) FxDiv(a, b)
#define FxMulToInt(a, b) FxInt((a)*(b))
#define FxDivToInt(a, b) ((int)((a)/(b)))
#define FxMulDiv(a, m, d) (((a)*(m))/(d))
#define FxSqrt(v) ((FxValue)sqrt((double)(v)))
#define FxDemotedSqrt(v) FxSqrt(v)

typedef FxValue FaAngle;

#define FaAng(a) (a)

#define FaSin(v) ((FxValue)-sin((double)(v)))
#define FaCos(v) ((FxValue)cos((double)(v)))

#define FaAdd(a, d) ((a)+(d))
FaAngle FaNorm(FaAngle a);
#define FaDeg(da) ((da)*(FxValue)(PI/180.0))
#define FaRad(ra) (ra)
#define FaAngVal(aa) (aa)
#define FaFltDegVal(a) ((a)*180.0/PI)
#define FaFltRadVal(a) (a)

#define FxInitialize(table_size, flags) ((flags) == FA_DEFAULT)
#define FxEnd()

#else

/* If integer sqrt isn't interesting, define FX_PRECISE_SQRT
   and the floating point sqrt will be used */

#ifndef FX_SHIFT
#define FX_SHIFT 10
#endif
#define FX_MULT (1L << FX_SHIFT)

typedef long FxValue;

#define FX_MAX_VALUE (0x7fffffff)
#define FX_MIN_VALUE (1)

#define FxVal(i) FxPromote((FxValue)(i))
#define FxInt(v) ((int)FxDemote(v))
#define FxFltVal(f) ((FxValue)((f)*(double)FX_MULT))
#define FxFlt(v) (((double)(v))/(double)FX_MULT)
#define FxPromote(v) ((v) << FX_SHIFT)
#define FxDemote(v) ((v) >> FX_SHIFT)

#if FX_SHIFT != 16
/* These can overflow if the shift and numbers are too large */
#define FxMul(a, b) FxDemote((a)*(b))
#define FxDiv(a, b) (FxPromote(a)/(b))
#define FxMulToInt(a, b) FxInt(FxDemote((a)*(b)))
#define FxMulDiv(a, m, d) (((a)*(m))/(d))
#else
/* For FX_SHIFT == 16 and certain platforms, assembly routines are
   provided which do 64-bit intermediate math, preserving accuracy
   There is still a danger of overflow if the results don't fit in
   32 bits, though */
FxValue FxMul(FxValue a, FxValue b);
FxValue FxDiv(FxValue a, FxValue b);
int FxMulToInt(FxValue a, FxValue b);
FxValue FxMulDiv(FxValue a, FxValue m, FxValue d);
#endif

#ifndef FX_PRECISE_SQRT
FxValue FxSqrt(FxValue v);
/* Computing the square root of a demoted value leaves it out of
   adjustment by sqrt(FX_MULT) so shift by FX_SHIFT/2 to
   restore fixed point
   FX_SHIFT should be even for this to work */
#define FxDemotedSqrt(v) (FxSqrt(FxDemote(v)) << (FX_SHIFT/2))
#else
#define FxSqrt(v) FxFltVal(sqrt(FxFlt(v)))
#define FxDemotedSqrt(v) FxSqrt(v)
#endif

#define FxDemotedMul(a, b) (FxDemote(a)*(b))
#define FxDemotedDiv(a, b) FxPromote((a)/(b))
#define FxDivToInt(a, b) ((int)((a)/(b)))

/* One unit of angle is the table quantum
   One unit of angle equals 360/_fa_table_size degrees */ 
typedef FxValue FaAngle;

extern int _fa_table_size;

extern FxValue *_fa_sines;
extern FxValue *_fa_cosines;

#define FaAng(a) FxDemote(a)
#define FaSin(a) _fa_sines[(int)FaAng(a)]
#define FaCos(a) _fa_cosines[(int)FaAng(a)]

FaAngle FaAdd(FaAngle a, FaAngle d);
FaAngle FaBisectingAngle(FaAngle f, FaAngle t);
#define FaNorm(a) FaAdd(FxVal(0), a)
#define FaDeg(da) \
    FaNorm(FxMulDiv(FxVal(da), _fa_table_size, 360))
#define FaRad(ra) FaNorm(FxFltVal((ra)*_fa_table_size/PI2))
#define FaAngVal(aa) FaNorm(FxVal(aa))
#define FaFltDegVal(ang) FxFltVal(FxMulDiv(ang, 360, _fa_table_size))
#define FaFltRadVal(ang) (FxFltVal(ang)*PI2/_fa_table_size)

BOOL FxInitialize(int table_size, ULONG flags);
void FxEnd(void);

#endif

typedef struct _FxPt2
{
    FxValue x, y;
} FxPt2;
typedef FxPt2 FxVec2;

typedef struct _FxBox2
{
    FxPt2 min, max;
} FxBox2;

typedef struct _FxPt3
{
    FxValue x, y, z;
} FxPt3;
typedef FxPt3 FxVec3;

typedef struct _FxBox3
{
    FxPt3 min, max;
} FxBox3;

typedef struct _FxPt4
{
    FxValue x, y, z, w;
} FxPt4;
typedef FxPt4 FxVec4;

void FxBBox2Empty(FxBox2 *bb);
void FxBBox2AddPt(FxBox2 *bb, FxPt2 *pt);

void FxBBox3Empty(FxBox3 *bb);
void FxBBox3AddPt(FxBox3 *bb, FxPt3 *pt);

#define FxV2Set(v, xv, yv) \
    ((v)->x = (xv), (v)->y = (yv))
#define FxV2Add(a, b, r) \
    ((r)->x = (a)->x+(b)->x, (r)->y = (a)->y+(b)->y)
#define FxV2Sub(a, b, r) \
    ((r)->x = (a)->x-(b)->x, (r)->y = (a)->y-(b)->y)
#define FxV2Dot(a, b) \
    (FxMul((a)->x, (b)->x)+FxMul((a)->y, (b)->y))
#define FxV2Neg(v, r) \
    ((r)->x = -(v)->x, (r)->y = -(v)->y)
#define FxV2NegV(v) FxV2Neg(v, v)
#define FxV2NormV(v) FxV2Norm(v, v)

FxValue FxV2Len(FxVec2 *v);
FxValue FxV2Norm(FxVec2 *v, FxVec2 *r);

#define FxvV2Set(v, xv, yv) FxV2Set(&(v), xv, yv)
#define FxvV2Add(a, b, r) FxV2Add(&(a), &(b), &(r))
#define FxvV2Sub(a, b, r) FxV2Sub(&(a), &(b), &(r))
#define FxvV2Dot(a, b) FxV2Dot(&(a), &(b))
#define FxvV2Neg(v, r) FxV2Neg(&(v), &(r))
#define FxvV2NegV(v) FxV2NegV(&(v))
#define FxvV2Len(v) FxV2Len(&(v))
#define FxvV2Norm(v, r) FxV2Norm(&(v), &(r))
#define FxvV2NormV(v) FxV2NormV(&(v))

#define FxV3Set(v, xv, yv, zv) \
    ((v)->x = (xv), (v)->y = (yv), (v)->z = (zv))
#define FxV3Add(a, b, r) \
    ((r)->x = (a)->x+(b)->x, (r)->y = (a)->y+(b)->y, (r)->z = (a)->z+(b)->z)
#define FxV3Sub(a, b, r) \
    ((r)->x = (a)->x-(b)->x, (r)->y = (a)->y-(b)->y, (r)->z = (a)->z-(b)->z)
#define FxV3Dot(a, b) \
    (FxMul((a)->x, (b)->x)+FxMul((a)->y, (b)->y)+FxMul((a)->z, (b)->z))
#define FxV3Neg(v, r) \
    ((r)->x = -(v)->x, (r)->y = -(v)->y, (r)->z = -(v)->z)
#define FxV3NegV(v) FxV3Neg(v, v)
#define FxV3Cross(a, b, r) \
    ((r)->x = (a)->y*(b)->z-(b)->y*(a)->z,\
     (r)->y = (a)->z*(b)->x-(b)->z*(a)->x,\
     (r)->z = (a)->x*(b)->y-(b)->x*(a)->y)
#define FxV3NormV(v) FxV3Norm(v, v)

FxValue FxV3Len(FxVec3 *v);
FxValue FxV3Norm(FxVec3 *v, FxVec3 *r);

#define FxvV3Set(v, xv, yv, zv) FxV3Set(&(v), xv, yv, zv)
#define FxvV3Add(a, b, r) FxV3Add(&(a), &(b), &(r))
#define FxvV3Sub(a, b, r) FxV3Sub(&(a), &(b), &(r))
#define FxvV3Dot(a, b) FxV3Dot(&(a), &(b))
#define FxvV3Neg(v, r) FxV3Neg(&(v), &(r))
#define FxvV3NegV(v) FxV3NegV(&(v))
#define FxvV3Cross(a, b, r) FxV3Cross(&(a), &(b), &(r))
#define FxvV3Len(v) FxV3Len(&(v))
#define FxvV3Norm(v, r) FxV3Norm(&(v), &(r))
#define FxvV3NormV(v) FxV3NormV(&(v))

typedef FxValue FxMatrix2[2][2];
typedef FxValue FxMatrix3[3][3];
typedef FxValue FxMatrix4[4][4];

typedef FxValue FxTMatrix2[2][3];
typedef FxValue FxTMatrix3[3][4];

void FxT2Ident(FxTMatrix2 m);
void FxT2Mul(FxTMatrix2 a, FxTMatrix2 b, FxTMatrix2 r);
void FxT2Vec2(FxTMatrix2 m, int n, FxVec2 *f, FxVec2 *t);

void FxT3Ident(FxTMatrix3 m);
void FxT3Mul(FxTMatrix3 a, FxTMatrix3 b, FxTMatrix3 r);
void FxT3Vec3(FxTMatrix3 m, int n, FxVec3 *f, FxVec3 *t);

#ifdef __cplusplus
}
#endif

#endif
