/* DO NOT EDIT - This file generated automatically with glx86asm.py script */
#include "assyntax.h"
#include "glapioffsets.h"

#ifndef __WIN32__

#if defined(STDCALL_API)
# if defined(USE_MGL_NAMESPACE)
#  define GL_PREFIX(n,n2) GLNAME(CONCAT(mgl,n2))
# else
#  define GL_PREFIX(n,n2) GLNAME(CONCAT(gl,n2))
# endif
#elif defined(USE_MGL_NAMESPACE)
# define GL_PREFIX(n,n2) GLNAME(CONCAT(mgl,n))
#else
# define GL_PREFIX(n,n2) GLNAME(CONCAT(gl,n))
#endif

#define GL_OFFSET(x) CODEPTR(REGOFF(4 * x, EAX))

#if defined(GNU_ASSEMBLER) && !defined(__DJGPP__) && !defined(__MINGW32__)
#define GLOBL_FN(x) GLOBL x ; .type x,@function
#else
#define GLOBL_FN(x) GLOBL x
#endif

SEG_TEXT

EXTERN GLNAME(_glapi_Dispatch)

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(NewList,NewList@8))
GL_PREFIX(NewList,NewList@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_NewList))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EndList,EndList@0))
GL_PREFIX(EndList,EndList@0):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EndList))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CallList,CallList@4))
GL_PREFIX(CallList,CallList@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CallList))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CallLists,CallLists@12))
GL_PREFIX(CallLists,CallLists@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CallLists))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DeleteLists,DeleteLists@8))
GL_PREFIX(DeleteLists,DeleteLists@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DeleteLists))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GenLists,GenLists@4))
GL_PREFIX(GenLists,GenLists@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GenLists))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ListBase,ListBase@4))
GL_PREFIX(ListBase,ListBase@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ListBase))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Begin,Begin@4))
GL_PREFIX(Begin,Begin@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Begin))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Bitmap,Bitmap@28))
GL_PREFIX(Bitmap,Bitmap@28):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Bitmap))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3b,Color3b@12))
GL_PREFIX(Color3b,Color3b@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3b))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3bv,Color3bv@4))
GL_PREFIX(Color3bv,Color3bv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3bv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3d,Color3d@24))
GL_PREFIX(Color3d,Color3d@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3dv,Color3dv@4))
GL_PREFIX(Color3dv,Color3dv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3f,Color3f@12))
GL_PREFIX(Color3f,Color3f@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3fv,Color3fv@4))
GL_PREFIX(Color3fv,Color3fv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3i,Color3i@12))
GL_PREFIX(Color3i,Color3i@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3iv,Color3iv@4))
GL_PREFIX(Color3iv,Color3iv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3s,Color3s@12))
GL_PREFIX(Color3s,Color3s@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3sv,Color3sv@4))
GL_PREFIX(Color3sv,Color3sv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3ub,Color3ub@12))
GL_PREFIX(Color3ub,Color3ub@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3ub))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3ubv,Color3ubv@4))
GL_PREFIX(Color3ubv,Color3ubv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3ubv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3ui,Color3ui@12))
GL_PREFIX(Color3ui,Color3ui@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3ui))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3uiv,Color3uiv@4))
GL_PREFIX(Color3uiv,Color3uiv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3uiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3us,Color3us@12))
GL_PREFIX(Color3us,Color3us@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3us))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color3usv,Color3usv@4))
GL_PREFIX(Color3usv,Color3usv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color3usv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4b,Color4b@16))
GL_PREFIX(Color4b,Color4b@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4b))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4bv,Color4bv@4))
GL_PREFIX(Color4bv,Color4bv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4bv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4d,Color4d@32))
GL_PREFIX(Color4d,Color4d@32):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4dv,Color4dv@4))
GL_PREFIX(Color4dv,Color4dv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4f,Color4f@16))
GL_PREFIX(Color4f,Color4f@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4fv,Color4fv@4))
GL_PREFIX(Color4fv,Color4fv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4i,Color4i@16))
GL_PREFIX(Color4i,Color4i@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4iv,Color4iv@4))
GL_PREFIX(Color4iv,Color4iv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4s,Color4s@16))
GL_PREFIX(Color4s,Color4s@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4sv,Color4sv@4))
GL_PREFIX(Color4sv,Color4sv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4ub,Color4ub@16))
GL_PREFIX(Color4ub,Color4ub@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4ub))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4ubv,Color4ubv@4))
GL_PREFIX(Color4ubv,Color4ubv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4ubv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4ui,Color4ui@16))
GL_PREFIX(Color4ui,Color4ui@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4ui))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4uiv,Color4uiv@4))
GL_PREFIX(Color4uiv,Color4uiv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4uiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4us,Color4us@16))
GL_PREFIX(Color4us,Color4us@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4us))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Color4usv,Color4usv@4))
GL_PREFIX(Color4usv,Color4usv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Color4usv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EdgeFlag,EdgeFlag@4))
GL_PREFIX(EdgeFlag,EdgeFlag@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EdgeFlag))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EdgeFlagv,EdgeFlagv@4))
GL_PREFIX(EdgeFlagv,EdgeFlagv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EdgeFlagv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(End,End@0))
GL_PREFIX(End,End@0):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_End))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexd,Indexd@8))
GL_PREFIX(Indexd,Indexd@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Indexd))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexdv,Indexdv@4))
GL_PREFIX(Indexdv,Indexdv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Indexdv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexf,Indexf@4))
GL_PREFIX(Indexf,Indexf@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Indexf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexfv,Indexfv@4))
GL_PREFIX(Indexfv,Indexfv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Indexfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexi,Indexi@4))
GL_PREFIX(Indexi,Indexi@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Indexi))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexiv,Indexiv@4))
GL_PREFIX(Indexiv,Indexiv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Indexiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexs,Indexs@4))
GL_PREFIX(Indexs,Indexs@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Indexs))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexsv,Indexsv@4))
GL_PREFIX(Indexsv,Indexsv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Indexsv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3b,Normal3b@12))
GL_PREFIX(Normal3b,Normal3b@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Normal3b))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3bv,Normal3bv@4))
GL_PREFIX(Normal3bv,Normal3bv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Normal3bv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3d,Normal3d@24))
GL_PREFIX(Normal3d,Normal3d@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Normal3d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3dv,Normal3dv@4))
GL_PREFIX(Normal3dv,Normal3dv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Normal3dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3f,Normal3f@12))
GL_PREFIX(Normal3f,Normal3f@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Normal3f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3fv,Normal3fv@4))
GL_PREFIX(Normal3fv,Normal3fv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Normal3fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3i,Normal3i@12))
GL_PREFIX(Normal3i,Normal3i@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Normal3i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3iv,Normal3iv@4))
GL_PREFIX(Normal3iv,Normal3iv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Normal3iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3s,Normal3s@12))
GL_PREFIX(Normal3s,Normal3s@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Normal3s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Normal3sv,Normal3sv@4))
GL_PREFIX(Normal3sv,Normal3sv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Normal3sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos2d,RasterPos2d@16))
GL_PREFIX(RasterPos2d,RasterPos2d@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos2d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos2dv,RasterPos2dv@4))
GL_PREFIX(RasterPos2dv,RasterPos2dv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos2dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos2f,RasterPos2f@8))
GL_PREFIX(RasterPos2f,RasterPos2f@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos2f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos2fv,RasterPos2fv@4))
GL_PREFIX(RasterPos2fv,RasterPos2fv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos2fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos2i,RasterPos2i@8))
GL_PREFIX(RasterPos2i,RasterPos2i@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos2i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos2iv,RasterPos2iv@4))
GL_PREFIX(RasterPos2iv,RasterPos2iv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos2iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos2s,RasterPos2s@8))
GL_PREFIX(RasterPos2s,RasterPos2s@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos2s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos2sv,RasterPos2sv@4))
GL_PREFIX(RasterPos2sv,RasterPos2sv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos2sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos3d,RasterPos3d@24))
GL_PREFIX(RasterPos3d,RasterPos3d@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos3d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos3dv,RasterPos3dv@4))
GL_PREFIX(RasterPos3dv,RasterPos3dv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos3dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos3f,RasterPos3f@12))
GL_PREFIX(RasterPos3f,RasterPos3f@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos3f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos3fv,RasterPos3fv@4))
GL_PREFIX(RasterPos3fv,RasterPos3fv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos3fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos3i,RasterPos3i@12))
GL_PREFIX(RasterPos3i,RasterPos3i@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos3i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos3iv,RasterPos3iv@4))
GL_PREFIX(RasterPos3iv,RasterPos3iv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos3iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos3s,RasterPos3s@12))
GL_PREFIX(RasterPos3s,RasterPos3s@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos3s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos3sv,RasterPos3sv@4))
GL_PREFIX(RasterPos3sv,RasterPos3sv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos3sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos4d,RasterPos4d@32))
GL_PREFIX(RasterPos4d,RasterPos4d@32):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos4d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos4dv,RasterPos4dv@4))
GL_PREFIX(RasterPos4dv,RasterPos4dv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos4dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos4f,RasterPos4f@16))
GL_PREFIX(RasterPos4f,RasterPos4f@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos4f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos4fv,RasterPos4fv@4))
GL_PREFIX(RasterPos4fv,RasterPos4fv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos4fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos4i,RasterPos4i@16))
GL_PREFIX(RasterPos4i,RasterPos4i@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos4i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos4iv,RasterPos4iv@4))
GL_PREFIX(RasterPos4iv,RasterPos4iv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos4iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos4s,RasterPos4s@16))
GL_PREFIX(RasterPos4s,RasterPos4s@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos4s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RasterPos4sv,RasterPos4sv@4))
GL_PREFIX(RasterPos4sv,RasterPos4sv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RasterPos4sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Rectd,Rectd@32))
GL_PREFIX(Rectd,Rectd@32):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Rectd))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Rectdv,Rectdv@8))
GL_PREFIX(Rectdv,Rectdv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Rectdv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Rectf,Rectf@16))
GL_PREFIX(Rectf,Rectf@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Rectf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Rectfv,Rectfv@8))
GL_PREFIX(Rectfv,Rectfv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Rectfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Recti,Recti@16))
GL_PREFIX(Recti,Recti@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Recti))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Rectiv,Rectiv@8))
GL_PREFIX(Rectiv,Rectiv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Rectiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Rects,Rects@16))
GL_PREFIX(Rects,Rects@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Rects))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Rectsv,Rectsv@8))
GL_PREFIX(Rectsv,Rectsv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Rectsv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord1d,TexCoord1d@8))
GL_PREFIX(TexCoord1d,TexCoord1d@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord1d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord1dv,TexCoord1dv@4))
GL_PREFIX(TexCoord1dv,TexCoord1dv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord1dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord1f,TexCoord1f@4))
GL_PREFIX(TexCoord1f,TexCoord1f@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord1f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord1fv,TexCoord1fv@4))
GL_PREFIX(TexCoord1fv,TexCoord1fv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord1fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord1i,TexCoord1i@4))
GL_PREFIX(TexCoord1i,TexCoord1i@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord1i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord1iv,TexCoord1iv@4))
GL_PREFIX(TexCoord1iv,TexCoord1iv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord1iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord1s,TexCoord1s@4))
GL_PREFIX(TexCoord1s,TexCoord1s@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord1s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord1sv,TexCoord1sv@4))
GL_PREFIX(TexCoord1sv,TexCoord1sv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord1sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord2d,TexCoord2d@16))
GL_PREFIX(TexCoord2d,TexCoord2d@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord2d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord2dv,TexCoord2dv@4))
GL_PREFIX(TexCoord2dv,TexCoord2dv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord2dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord2f,TexCoord2f@8))
GL_PREFIX(TexCoord2f,TexCoord2f@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord2f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord2fv,TexCoord2fv@4))
GL_PREFIX(TexCoord2fv,TexCoord2fv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord2fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord2i,TexCoord2i@8))
GL_PREFIX(TexCoord2i,TexCoord2i@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord2i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord2iv,TexCoord2iv@4))
GL_PREFIX(TexCoord2iv,TexCoord2iv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord2iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord2s,TexCoord2s@8))
GL_PREFIX(TexCoord2s,TexCoord2s@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord2s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord2sv,TexCoord2sv@4))
GL_PREFIX(TexCoord2sv,TexCoord2sv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord2sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord3d,TexCoord3d@24))
GL_PREFIX(TexCoord3d,TexCoord3d@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord3d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord3dv,TexCoord3dv@4))
GL_PREFIX(TexCoord3dv,TexCoord3dv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord3dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord3f,TexCoord3f@12))
GL_PREFIX(TexCoord3f,TexCoord3f@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord3f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord3fv,TexCoord3fv@4))
GL_PREFIX(TexCoord3fv,TexCoord3fv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord3fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord3i,TexCoord3i@12))
GL_PREFIX(TexCoord3i,TexCoord3i@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord3i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord3iv,TexCoord3iv@4))
GL_PREFIX(TexCoord3iv,TexCoord3iv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord3iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord3s,TexCoord3s@12))
GL_PREFIX(TexCoord3s,TexCoord3s@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord3s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord3sv,TexCoord3sv@4))
GL_PREFIX(TexCoord3sv,TexCoord3sv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord3sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord4d,TexCoord4d@32))
GL_PREFIX(TexCoord4d,TexCoord4d@32):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord4d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord4dv,TexCoord4dv@4))
GL_PREFIX(TexCoord4dv,TexCoord4dv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord4dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord4f,TexCoord4f@16))
GL_PREFIX(TexCoord4f,TexCoord4f@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord4f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord4fv,TexCoord4fv@4))
GL_PREFIX(TexCoord4fv,TexCoord4fv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord4fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord4i,TexCoord4i@16))
GL_PREFIX(TexCoord4i,TexCoord4i@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord4i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord4iv,TexCoord4iv@4))
GL_PREFIX(TexCoord4iv,TexCoord4iv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord4iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord4s,TexCoord4s@16))
GL_PREFIX(TexCoord4s,TexCoord4s@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord4s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoord4sv,TexCoord4sv@4))
GL_PREFIX(TexCoord4sv,TexCoord4sv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoord4sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex2d,Vertex2d@16))
GL_PREFIX(Vertex2d,Vertex2d@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex2d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex2dv,Vertex2dv@4))
GL_PREFIX(Vertex2dv,Vertex2dv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex2dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex2f,Vertex2f@8))
GL_PREFIX(Vertex2f,Vertex2f@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex2f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex2fv,Vertex2fv@4))
GL_PREFIX(Vertex2fv,Vertex2fv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex2fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex2i,Vertex2i@8))
GL_PREFIX(Vertex2i,Vertex2i@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex2i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex2iv,Vertex2iv@4))
GL_PREFIX(Vertex2iv,Vertex2iv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex2iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex2s,Vertex2s@8))
GL_PREFIX(Vertex2s,Vertex2s@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex2s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex2sv,Vertex2sv@4))
GL_PREFIX(Vertex2sv,Vertex2sv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex2sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex3d,Vertex3d@24))
GL_PREFIX(Vertex3d,Vertex3d@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex3d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex3dv,Vertex3dv@4))
GL_PREFIX(Vertex3dv,Vertex3dv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex3dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex3f,Vertex3f@12))
GL_PREFIX(Vertex3f,Vertex3f@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex3f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex3fv,Vertex3fv@4))
GL_PREFIX(Vertex3fv,Vertex3fv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex3fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex3i,Vertex3i@12))
GL_PREFIX(Vertex3i,Vertex3i@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex3i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex3iv,Vertex3iv@4))
GL_PREFIX(Vertex3iv,Vertex3iv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex3iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex3s,Vertex3s@12))
GL_PREFIX(Vertex3s,Vertex3s@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex3s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex3sv,Vertex3sv@4))
GL_PREFIX(Vertex3sv,Vertex3sv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex3sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex4d,Vertex4d@32))
GL_PREFIX(Vertex4d,Vertex4d@32):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex4d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex4dv,Vertex4dv@4))
GL_PREFIX(Vertex4dv,Vertex4dv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex4dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex4f,Vertex4f@16))
GL_PREFIX(Vertex4f,Vertex4f@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex4f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex4fv,Vertex4fv@4))
GL_PREFIX(Vertex4fv,Vertex4fv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex4fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex4i,Vertex4i@16))
GL_PREFIX(Vertex4i,Vertex4i@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex4i))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex4iv,Vertex4iv@4))
GL_PREFIX(Vertex4iv,Vertex4iv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex4iv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex4s,Vertex4s@16))
GL_PREFIX(Vertex4s,Vertex4s@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex4s))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Vertex4sv,Vertex4sv@4))
GL_PREFIX(Vertex4sv,Vertex4sv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Vertex4sv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ClipPlane,ClipPlane@8))
GL_PREFIX(ClipPlane,ClipPlane@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ClipPlane))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorMaterial,ColorMaterial@8))
GL_PREFIX(ColorMaterial,ColorMaterial@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ColorMaterial))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CullFace,CullFace@4))
GL_PREFIX(CullFace,CullFace@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CullFace))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Fogf,Fogf@8))
GL_PREFIX(Fogf,Fogf@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Fogf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Fogfv,Fogfv@8))
GL_PREFIX(Fogfv,Fogfv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Fogfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Fogi,Fogi@8))
GL_PREFIX(Fogi,Fogi@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Fogi))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Fogiv,Fogiv@8))
GL_PREFIX(Fogiv,Fogiv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Fogiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FrontFace,FrontFace@4))
GL_PREFIX(FrontFace,FrontFace@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FrontFace))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Hint,Hint@8))
GL_PREFIX(Hint,Hint@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Hint))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Lightf,Lightf@12))
GL_PREFIX(Lightf,Lightf@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Lightf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Lightfv,Lightfv@12))
GL_PREFIX(Lightfv,Lightfv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Lightfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Lighti,Lighti@12))
GL_PREFIX(Lighti,Lighti@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Lighti))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Lightiv,Lightiv@12))
GL_PREFIX(Lightiv,Lightiv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Lightiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LightModelf,LightModelf@8))
GL_PREFIX(LightModelf,LightModelf@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LightModelf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LightModelfv,LightModelfv@8))
GL_PREFIX(LightModelfv,LightModelfv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LightModelfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LightModeli,LightModeli@8))
GL_PREFIX(LightModeli,LightModeli@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LightModeli))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LightModeliv,LightModeliv@8))
GL_PREFIX(LightModeliv,LightModeliv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LightModeliv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LineStipple,LineStipple@8))
GL_PREFIX(LineStipple,LineStipple@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LineStipple))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LineWidth,LineWidth@4))
GL_PREFIX(LineWidth,LineWidth@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LineWidth))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Materialf,Materialf@12))
GL_PREFIX(Materialf,Materialf@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Materialf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Materialfv,Materialfv@12))
GL_PREFIX(Materialfv,Materialfv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Materialfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Materiali,Materiali@12))
GL_PREFIX(Materiali,Materiali@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Materiali))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Materialiv,Materialiv@12))
GL_PREFIX(Materialiv,Materialiv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Materialiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointSize,PointSize@4))
GL_PREFIX(PointSize,PointSize@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PointSize))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PolygonMode,PolygonMode@8))
GL_PREFIX(PolygonMode,PolygonMode@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PolygonMode))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PolygonStipple,PolygonStipple@4))
GL_PREFIX(PolygonStipple,PolygonStipple@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PolygonStipple))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Scissor,Scissor@16))
GL_PREFIX(Scissor,Scissor@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Scissor))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ShadeModel,ShadeModel@4))
GL_PREFIX(ShadeModel,ShadeModel@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ShadeModel))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexParameterf,TexParameterf@12))
GL_PREFIX(TexParameterf,TexParameterf@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexParameterf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexParameterfv,TexParameterfv@12))
GL_PREFIX(TexParameterfv,TexParameterfv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexParameteri,TexParameteri@12))
GL_PREFIX(TexParameteri,TexParameteri@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexParameteri))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexParameteriv,TexParameteriv@12))
GL_PREFIX(TexParameteriv,TexParameteriv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexImage1D,TexImage1D@32))
GL_PREFIX(TexImage1D,TexImage1D@32):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexImage1D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexImage2D,TexImage2D@36))
GL_PREFIX(TexImage2D,TexImage2D@36):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexImage2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexEnvf,TexEnvf@12))
GL_PREFIX(TexEnvf,TexEnvf@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexEnvf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexEnvfv,TexEnvfv@12))
GL_PREFIX(TexEnvfv,TexEnvfv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexEnvfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexEnvi,TexEnvi@12))
GL_PREFIX(TexEnvi,TexEnvi@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexEnvi))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexEnviv,TexEnviv@12))
GL_PREFIX(TexEnviv,TexEnviv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexEnviv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexGend,TexGend@16))
GL_PREFIX(TexGend,TexGend@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexGend))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexGendv,TexGendv@12))
GL_PREFIX(TexGendv,TexGendv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexGendv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexGenf,TexGenf@12))
GL_PREFIX(TexGenf,TexGenf@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexGenf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexGenfv,TexGenfv@12))
GL_PREFIX(TexGenfv,TexGenfv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexGenfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexGeni,TexGeni@12))
GL_PREFIX(TexGeni,TexGeni@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexGeni))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexGeniv,TexGeniv@12))
GL_PREFIX(TexGeniv,TexGeniv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexGeniv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FeedbackBuffer,FeedbackBuffer@12))
GL_PREFIX(FeedbackBuffer,FeedbackBuffer@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FeedbackBuffer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SelectBuffer,SelectBuffer@8))
GL_PREFIX(SelectBuffer,SelectBuffer@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SelectBuffer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RenderMode,RenderMode@4))
GL_PREFIX(RenderMode,RenderMode@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RenderMode))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(InitNames,InitNames@0))
GL_PREFIX(InitNames,InitNames@0):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_InitNames))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LoadName,LoadName@4))
GL_PREFIX(LoadName,LoadName@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LoadName))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PassThrough,PassThrough@4))
GL_PREFIX(PassThrough,PassThrough@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PassThrough))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PopName,PopName@0))
GL_PREFIX(PopName,PopName@0):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PopName))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PushName,PushName@4))
GL_PREFIX(PushName,PushName@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PushName))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DrawBuffer,DrawBuffer@4))
GL_PREFIX(DrawBuffer,DrawBuffer@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DrawBuffer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Clear,Clear@4))
GL_PREFIX(Clear,Clear@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Clear))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ClearAccum,ClearAccum@16))
GL_PREFIX(ClearAccum,ClearAccum@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ClearAccum))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ClearIndex,ClearIndex@4))
GL_PREFIX(ClearIndex,ClearIndex@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ClearIndex))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ClearColor,ClearColor@16))
GL_PREFIX(ClearColor,ClearColor@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ClearColor))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ClearStencil,ClearStencil@4))
GL_PREFIX(ClearStencil,ClearStencil@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ClearStencil))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ClearDepth,ClearDepth@8))
GL_PREFIX(ClearDepth,ClearDepth@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ClearDepth))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(StencilMask,StencilMask@4))
GL_PREFIX(StencilMask,StencilMask@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_StencilMask))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorMask,ColorMask@16))
GL_PREFIX(ColorMask,ColorMask@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ColorMask))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DepthMask,DepthMask@4))
GL_PREFIX(DepthMask,DepthMask@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DepthMask))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IndexMask,IndexMask@4))
GL_PREFIX(IndexMask,IndexMask@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_IndexMask))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Accum,Accum@8))
GL_PREFIX(Accum,Accum@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Accum))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Disable,Disable@4))
GL_PREFIX(Disable,Disable@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Disable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Enable,Enable@4))
GL_PREFIX(Enable,Enable@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Enable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Finish,Finish@0))
GL_PREFIX(Finish,Finish@0):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Finish))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Flush,Flush@0))
GL_PREFIX(Flush,Flush@0):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Flush))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PopAttrib,PopAttrib@0))
GL_PREFIX(PopAttrib,PopAttrib@0):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PopAttrib))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PushAttrib,PushAttrib@4))
GL_PREFIX(PushAttrib,PushAttrib@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PushAttrib))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Map1d,Map1d@32))
GL_PREFIX(Map1d,Map1d@32):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Map1d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Map1f,Map1f@24))
GL_PREFIX(Map1f,Map1f@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Map1f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Map2d,Map2d@56))
GL_PREFIX(Map2d,Map2d@56):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Map2d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Map2f,Map2f@40))
GL_PREFIX(Map2f,Map2f@40):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Map2f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MapGrid1d,MapGrid1d@20))
GL_PREFIX(MapGrid1d,MapGrid1d@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MapGrid1d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MapGrid1f,MapGrid1f@12))
GL_PREFIX(MapGrid1f,MapGrid1f@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MapGrid1f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MapGrid2d,MapGrid2d@40))
GL_PREFIX(MapGrid2d,MapGrid2d@40):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MapGrid2d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MapGrid2f,MapGrid2f@24))
GL_PREFIX(MapGrid2f,MapGrid2f@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MapGrid2f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalCoord1d,EvalCoord1d@8))
GL_PREFIX(EvalCoord1d,EvalCoord1d@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EvalCoord1d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalCoord1dv,EvalCoord1dv@4))
GL_PREFIX(EvalCoord1dv,EvalCoord1dv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EvalCoord1dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalCoord1f,EvalCoord1f@4))
GL_PREFIX(EvalCoord1f,EvalCoord1f@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EvalCoord1f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalCoord1fv,EvalCoord1fv@4))
GL_PREFIX(EvalCoord1fv,EvalCoord1fv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EvalCoord1fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalCoord2d,EvalCoord2d@16))
GL_PREFIX(EvalCoord2d,EvalCoord2d@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EvalCoord2d))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalCoord2dv,EvalCoord2dv@4))
GL_PREFIX(EvalCoord2dv,EvalCoord2dv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EvalCoord2dv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalCoord2f,EvalCoord2f@8))
GL_PREFIX(EvalCoord2f,EvalCoord2f@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EvalCoord2f))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalCoord2fv,EvalCoord2fv@4))
GL_PREFIX(EvalCoord2fv,EvalCoord2fv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EvalCoord2fv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalMesh1,EvalMesh1@12))
GL_PREFIX(EvalMesh1,EvalMesh1@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EvalMesh1))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalPoint1,EvalPoint1@4))
GL_PREFIX(EvalPoint1,EvalPoint1@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EvalPoint1))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalMesh2,EvalMesh2@20))
GL_PREFIX(EvalMesh2,EvalMesh2@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EvalMesh2))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EvalPoint2,EvalPoint2@8))
GL_PREFIX(EvalPoint2,EvalPoint2@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EvalPoint2))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(AlphaFunc,AlphaFunc@8))
GL_PREFIX(AlphaFunc,AlphaFunc@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_AlphaFunc))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BlendFunc,BlendFunc@8))
GL_PREFIX(BlendFunc,BlendFunc@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BlendFunc))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LogicOp,LogicOp@4))
GL_PREFIX(LogicOp,LogicOp@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LogicOp))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(StencilFunc,StencilFunc@12))
GL_PREFIX(StencilFunc,StencilFunc@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_StencilFunc))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(StencilOp,StencilOp@12))
GL_PREFIX(StencilOp,StencilOp@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_StencilOp))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DepthFunc,DepthFunc@4))
GL_PREFIX(DepthFunc,DepthFunc@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DepthFunc))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelZoom,PixelZoom@8))
GL_PREFIX(PixelZoom,PixelZoom@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PixelZoom))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelTransferf,PixelTransferf@8))
GL_PREFIX(PixelTransferf,PixelTransferf@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PixelTransferf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelTransferi,PixelTransferi@8))
GL_PREFIX(PixelTransferi,PixelTransferi@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PixelTransferi))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelStoref,PixelStoref@8))
GL_PREFIX(PixelStoref,PixelStoref@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PixelStoref))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelStorei,PixelStorei@8))
GL_PREFIX(PixelStorei,PixelStorei@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PixelStorei))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelMapfv,PixelMapfv@12))
GL_PREFIX(PixelMapfv,PixelMapfv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PixelMapfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelMapuiv,PixelMapuiv@12))
GL_PREFIX(PixelMapuiv,PixelMapuiv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PixelMapuiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelMapusv,PixelMapusv@12))
GL_PREFIX(PixelMapusv,PixelMapusv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PixelMapusv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ReadBuffer,ReadBuffer@4))
GL_PREFIX(ReadBuffer,ReadBuffer@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ReadBuffer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyPixels,CopyPixels@20))
GL_PREFIX(CopyPixels,CopyPixels@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyPixels))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ReadPixels,ReadPixels@28))
GL_PREFIX(ReadPixels,ReadPixels@28):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ReadPixels))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DrawPixels,DrawPixels@20))
GL_PREFIX(DrawPixels,DrawPixels@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DrawPixels))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetBooleanv,GetBooleanv@8))
GL_PREFIX(GetBooleanv,GetBooleanv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetBooleanv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetClipPlane,GetClipPlane@8))
GL_PREFIX(GetClipPlane,GetClipPlane@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetClipPlane))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetDoublev,GetDoublev@8))
GL_PREFIX(GetDoublev,GetDoublev@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetDoublev))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetError,GetError@0))
GL_PREFIX(GetError,GetError@0):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetError))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetFloatv,GetFloatv@8))
GL_PREFIX(GetFloatv,GetFloatv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetFloatv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetIntegerv,GetIntegerv@8))
GL_PREFIX(GetIntegerv,GetIntegerv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetIntegerv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetLightfv,GetLightfv@12))
GL_PREFIX(GetLightfv,GetLightfv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetLightfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetLightiv,GetLightiv@12))
GL_PREFIX(GetLightiv,GetLightiv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetLightiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMapdv,GetMapdv@12))
GL_PREFIX(GetMapdv,GetMapdv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetMapdv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMapfv,GetMapfv@12))
GL_PREFIX(GetMapfv,GetMapfv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetMapfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMapiv,GetMapiv@12))
GL_PREFIX(GetMapiv,GetMapiv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetMapiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMaterialfv,GetMaterialfv@12))
GL_PREFIX(GetMaterialfv,GetMaterialfv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetMaterialfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMaterialiv,GetMaterialiv@12))
GL_PREFIX(GetMaterialiv,GetMaterialiv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetMaterialiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetPixelMapfv,GetPixelMapfv@8))
GL_PREFIX(GetPixelMapfv,GetPixelMapfv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetPixelMapfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetPixelMapuiv,GetPixelMapuiv@8))
GL_PREFIX(GetPixelMapuiv,GetPixelMapuiv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetPixelMapuiv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetPixelMapusv,GetPixelMapusv@8))
GL_PREFIX(GetPixelMapusv,GetPixelMapusv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetPixelMapusv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetPolygonStipple,GetPolygonStipple@4))
GL_PREFIX(GetPolygonStipple,GetPolygonStipple@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetPolygonStipple))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetString,GetString@4))
GL_PREFIX(GetString,GetString@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetString))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexEnvfv,GetTexEnvfv@12))
GL_PREFIX(GetTexEnvfv,GetTexEnvfv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetTexEnvfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexEnviv,GetTexEnviv@12))
GL_PREFIX(GetTexEnviv,GetTexEnviv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetTexEnviv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexGendv,GetTexGendv@12))
GL_PREFIX(GetTexGendv,GetTexGendv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetTexGendv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexGenfv,GetTexGenfv@12))
GL_PREFIX(GetTexGenfv,GetTexGenfv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetTexGenfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexGeniv,GetTexGeniv@12))
GL_PREFIX(GetTexGeniv,GetTexGeniv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetTexGeniv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexImage,GetTexImage@20))
GL_PREFIX(GetTexImage,GetTexImage@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetTexImage))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexParameterfv,GetTexParameterfv@12))
GL_PREFIX(GetTexParameterfv,GetTexParameterfv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetTexParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexParameteriv,GetTexParameteriv@12))
GL_PREFIX(GetTexParameteriv,GetTexParameteriv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetTexParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexLevelParameterfv,GetTexLevelParameterfv@16))
GL_PREFIX(GetTexLevelParameterfv,GetTexLevelParameterfv@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetTexLevelParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexLevelParameteriv,GetTexLevelParameteriv@16))
GL_PREFIX(GetTexLevelParameteriv,GetTexLevelParameteriv@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetTexLevelParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IsEnabled,IsEnabled@4))
GL_PREFIX(IsEnabled,IsEnabled@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_IsEnabled))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IsList,IsList@4))
GL_PREFIX(IsList,IsList@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_IsList))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DepthRange,DepthRange@16))
GL_PREFIX(DepthRange,DepthRange@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DepthRange))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Frustum,Frustum@48))
GL_PREFIX(Frustum,Frustum@48):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Frustum))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LoadIdentity,LoadIdentity@0))
GL_PREFIX(LoadIdentity,LoadIdentity@0):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LoadIdentity))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LoadMatrixf,LoadMatrixf@4))
GL_PREFIX(LoadMatrixf,LoadMatrixf@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LoadMatrixf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LoadMatrixd,LoadMatrixd@4))
GL_PREFIX(LoadMatrixd,LoadMatrixd@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LoadMatrixd))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MatrixMode,MatrixMode@4))
GL_PREFIX(MatrixMode,MatrixMode@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MatrixMode))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultMatrixf,MultMatrixf@4))
GL_PREFIX(MultMatrixf,MultMatrixf@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultMatrixf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultMatrixd,MultMatrixd@4))
GL_PREFIX(MultMatrixd,MultMatrixd@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultMatrixd))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Ortho,Ortho@48))
GL_PREFIX(Ortho,Ortho@48):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Ortho))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PopMatrix,PopMatrix@0))
GL_PREFIX(PopMatrix,PopMatrix@0):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PopMatrix))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PushMatrix,PushMatrix@0))
GL_PREFIX(PushMatrix,PushMatrix@0):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PushMatrix))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Rotated,Rotated@32))
GL_PREFIX(Rotated,Rotated@32):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Rotated))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Rotatef,Rotatef@16))
GL_PREFIX(Rotatef,Rotatef@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Rotatef))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Scaled,Scaled@24))
GL_PREFIX(Scaled,Scaled@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Scaled))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Scalef,Scalef@12))
GL_PREFIX(Scalef,Scalef@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Scalef))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Translated,Translated@24))
GL_PREFIX(Translated,Translated@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Translated))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Translatef,Translatef@12))
GL_PREFIX(Translatef,Translatef@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Translatef))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Viewport,Viewport@16))
GL_PREFIX(Viewport,Viewport@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Viewport))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ArrayElement,ArrayElement@4))
GL_PREFIX(ArrayElement,ArrayElement@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ArrayElement))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorPointer,ColorPointer@16))
GL_PREFIX(ColorPointer,ColorPointer@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ColorPointer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DisableClientState,DisableClientState@4))
GL_PREFIX(DisableClientState,DisableClientState@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DisableClientState))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DrawArrays,DrawArrays@12))
GL_PREFIX(DrawArrays,DrawArrays@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DrawArrays))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DrawElements,DrawElements@16))
GL_PREFIX(DrawElements,DrawElements@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DrawElements))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EdgeFlagPointer,EdgeFlagPointer@8))
GL_PREFIX(EdgeFlagPointer,EdgeFlagPointer@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EdgeFlagPointer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EnableClientState,EnableClientState@4))
GL_PREFIX(EnableClientState,EnableClientState@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EnableClientState))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetPointerv,GetPointerv@8))
GL_PREFIX(GetPointerv,GetPointerv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetPointerv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IndexPointer,IndexPointer@12))
GL_PREFIX(IndexPointer,IndexPointer@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_IndexPointer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(InterleavedArrays,InterleavedArrays@12))
GL_PREFIX(InterleavedArrays,InterleavedArrays@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_InterleavedArrays))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(NormalPointer,NormalPointer@12))
GL_PREFIX(NormalPointer,NormalPointer@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_NormalPointer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoordPointer,TexCoordPointer@16))
GL_PREFIX(TexCoordPointer,TexCoordPointer@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoordPointer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexPointer,VertexPointer@16))
GL_PREFIX(VertexPointer,VertexPointer@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexPointer))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PolygonOffset,PolygonOffset@8))
GL_PREFIX(PolygonOffset,PolygonOffset@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PolygonOffset))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexImage1D,CopyTexImage1D@28))
GL_PREFIX(CopyTexImage1D,CopyTexImage1D@28):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyTexImage1D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexImage2D,CopyTexImage2D@32))
GL_PREFIX(CopyTexImage2D,CopyTexImage2D@32):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyTexImage2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexSubImage1D,CopyTexSubImage1D@24))
GL_PREFIX(CopyTexSubImage1D,CopyTexSubImage1D@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyTexSubImage1D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexSubImage2D,CopyTexSubImage2D@32))
GL_PREFIX(CopyTexSubImage2D,CopyTexSubImage2D@32):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyTexSubImage2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexSubImage1D,TexSubImage1D@28))
GL_PREFIX(TexSubImage1D,TexSubImage1D@28):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexSubImage1D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexSubImage2D,TexSubImage2D@36))
GL_PREFIX(TexSubImage2D,TexSubImage2D@36):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexSubImage2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(AreTexturesResident,AreTexturesResident@12))
GL_PREFIX(AreTexturesResident,AreTexturesResident@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_AreTexturesResident))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BindTexture,BindTexture@8))
GL_PREFIX(BindTexture,BindTexture@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BindTexture))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DeleteTextures,DeleteTextures@8))
GL_PREFIX(DeleteTextures,DeleteTextures@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DeleteTextures))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GenTextures,GenTextures@8))
GL_PREFIX(GenTextures,GenTextures@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GenTextures))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IsTexture,IsTexture@4))
GL_PREFIX(IsTexture,IsTexture@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_IsTexture))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PrioritizeTextures,PrioritizeTextures@12))
GL_PREFIX(PrioritizeTextures,PrioritizeTextures@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PrioritizeTextures))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexub,Indexub@4))
GL_PREFIX(Indexub,Indexub@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Indexub))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Indexubv,Indexubv@4))
GL_PREFIX(Indexubv,Indexubv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Indexubv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PopClientAttrib,PopClientAttrib@0))
GL_PREFIX(PopClientAttrib,PopClientAttrib@0):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PopClientAttrib))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PushClientAttrib,PushClientAttrib@4))
GL_PREFIX(PushClientAttrib,PushClientAttrib@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PushClientAttrib))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BlendColor,BlendColor@16))
GL_PREFIX(BlendColor,BlendColor@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BlendColor))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BlendEquation,BlendEquation@4))
GL_PREFIX(BlendEquation,BlendEquation@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BlendEquation))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DrawRangeElements,DrawRangeElements@24))
GL_PREFIX(DrawRangeElements,DrawRangeElements@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DrawRangeElements))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorTable,ColorTable@24))
GL_PREFIX(ColorTable,ColorTable@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ColorTable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorTableParameterfv,ColorTableParameterfv@12))
GL_PREFIX(ColorTableParameterfv,ColorTableParameterfv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ColorTableParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorTableParameteriv,ColorTableParameteriv@12))
GL_PREFIX(ColorTableParameteriv,ColorTableParameteriv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ColorTableParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyColorTable,CopyColorTable@20))
GL_PREFIX(CopyColorTable,CopyColorTable@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyColorTable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetColorTable,GetColorTable@16))
GL_PREFIX(GetColorTable,GetColorTable@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetColorTable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetColorTableParameterfv,GetColorTableParameterfv@12))
GL_PREFIX(GetColorTableParameterfv,GetColorTableParameterfv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetColorTableParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetColorTableParameteriv,GetColorTableParameteriv@12))
GL_PREFIX(GetColorTableParameteriv,GetColorTableParameteriv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetColorTableParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorSubTable,ColorSubTable@24))
GL_PREFIX(ColorSubTable,ColorSubTable@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ColorSubTable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyColorSubTable,CopyColorSubTable@20))
GL_PREFIX(CopyColorSubTable,CopyColorSubTable@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyColorSubTable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionFilter1D,ConvolutionFilter1D@24))
GL_PREFIX(ConvolutionFilter1D,ConvolutionFilter1D@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ConvolutionFilter1D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionFilter2D,ConvolutionFilter2D@28))
GL_PREFIX(ConvolutionFilter2D,ConvolutionFilter2D@28):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ConvolutionFilter2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionParameterf,ConvolutionParameterf@12))
GL_PREFIX(ConvolutionParameterf,ConvolutionParameterf@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ConvolutionParameterf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionParameterfv,ConvolutionParameterfv@12))
GL_PREFIX(ConvolutionParameterfv,ConvolutionParameterfv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ConvolutionParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionParameteri,ConvolutionParameteri@12))
GL_PREFIX(ConvolutionParameteri,ConvolutionParameteri@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ConvolutionParameteri))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionParameteriv,ConvolutionParameteriv@12))
GL_PREFIX(ConvolutionParameteriv,ConvolutionParameteriv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ConvolutionParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyConvolutionFilter1D,CopyConvolutionFilter1D@20))
GL_PREFIX(CopyConvolutionFilter1D,CopyConvolutionFilter1D@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyConvolutionFilter1D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyConvolutionFilter2D,CopyConvolutionFilter2D@24))
GL_PREFIX(CopyConvolutionFilter2D,CopyConvolutionFilter2D@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyConvolutionFilter2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetConvolutionFilter,GetConvolutionFilter@16))
GL_PREFIX(GetConvolutionFilter,GetConvolutionFilter@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetConvolutionFilter))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetConvolutionParameterfv,GetConvolutionParameterfv@12))
GL_PREFIX(GetConvolutionParameterfv,GetConvolutionParameterfv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetConvolutionParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetConvolutionParameteriv,GetConvolutionParameteriv@12))
GL_PREFIX(GetConvolutionParameteriv,GetConvolutionParameteriv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetConvolutionParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetSeparableFilter,GetSeparableFilter@24))
GL_PREFIX(GetSeparableFilter,GetSeparableFilter@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetSeparableFilter))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SeparableFilter2D,SeparableFilter2D@32))
GL_PREFIX(SeparableFilter2D,SeparableFilter2D@32):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SeparableFilter2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetHistogram,GetHistogram@20))
GL_PREFIX(GetHistogram,GetHistogram@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetHistogram))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetHistogramParameterfv,GetHistogramParameterfv@12))
GL_PREFIX(GetHistogramParameterfv,GetHistogramParameterfv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetHistogramParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetHistogramParameteriv,GetHistogramParameteriv@12))
GL_PREFIX(GetHistogramParameteriv,GetHistogramParameteriv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetHistogramParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMinmax,GetMinmax@20))
GL_PREFIX(GetMinmax,GetMinmax@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetMinmax))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMinmaxParameterfv,GetMinmaxParameterfv@12))
GL_PREFIX(GetMinmaxParameterfv,GetMinmaxParameterfv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetMinmaxParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMinmaxParameteriv,GetMinmaxParameteriv@12))
GL_PREFIX(GetMinmaxParameteriv,GetMinmaxParameteriv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetMinmaxParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Histogram,Histogram@16))
GL_PREFIX(Histogram,Histogram@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Histogram))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(Minmax,Minmax@12))
GL_PREFIX(Minmax,Minmax@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Minmax))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ResetHistogram,ResetHistogram@4))
GL_PREFIX(ResetHistogram,ResetHistogram@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ResetHistogram))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ResetMinmax,ResetMinmax@4))
GL_PREFIX(ResetMinmax,ResetMinmax@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ResetMinmax))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexImage3D,TexImage3D@40))
GL_PREFIX(TexImage3D,TexImage3D@40):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexImage3D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexSubImage3D,TexSubImage3D@44))
GL_PREFIX(TexSubImage3D,TexSubImage3D@44):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexSubImage3D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexSubImage3D,CopyTexSubImage3D@36))
GL_PREFIX(CopyTexSubImage3D,CopyTexSubImage3D@36):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyTexSubImage3D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ActiveTextureARB,ActiveTextureARB@4))
GL_PREFIX(ActiveTextureARB,ActiveTextureARB@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ActiveTextureARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ClientActiveTextureARB,ClientActiveTextureARB@4))
GL_PREFIX(ClientActiveTextureARB,ClientActiveTextureARB@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ClientActiveTextureARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1dARB,MultiTexCoord1dARB@12))
GL_PREFIX(MultiTexCoord1dARB,MultiTexCoord1dARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1dARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1dvARB,MultiTexCoord1dvARB@8))
GL_PREFIX(MultiTexCoord1dvARB,MultiTexCoord1dvARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1dvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1fARB,MultiTexCoord1fARB@8))
GL_PREFIX(MultiTexCoord1fARB,MultiTexCoord1fARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1fARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1fvARB,MultiTexCoord1fvARB@8))
GL_PREFIX(MultiTexCoord1fvARB,MultiTexCoord1fvARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1fvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1iARB,MultiTexCoord1iARB@8))
GL_PREFIX(MultiTexCoord1iARB,MultiTexCoord1iARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1iARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1ivARB,MultiTexCoord1ivARB@8))
GL_PREFIX(MultiTexCoord1ivARB,MultiTexCoord1ivARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1ivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1sARB,MultiTexCoord1sARB@8))
GL_PREFIX(MultiTexCoord1sARB,MultiTexCoord1sARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1sARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1svARB,MultiTexCoord1svARB@8))
GL_PREFIX(MultiTexCoord1svARB,MultiTexCoord1svARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1svARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2dARB,MultiTexCoord2dARB@20))
GL_PREFIX(MultiTexCoord2dARB,MultiTexCoord2dARB@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2dARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2dvARB,MultiTexCoord2dvARB@8))
GL_PREFIX(MultiTexCoord2dvARB,MultiTexCoord2dvARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2dvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2fARB,MultiTexCoord2fARB@12))
GL_PREFIX(MultiTexCoord2fARB,MultiTexCoord2fARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2fARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2fvARB,MultiTexCoord2fvARB@8))
GL_PREFIX(MultiTexCoord2fvARB,MultiTexCoord2fvARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2fvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2iARB,MultiTexCoord2iARB@12))
GL_PREFIX(MultiTexCoord2iARB,MultiTexCoord2iARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2iARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2ivARB,MultiTexCoord2ivARB@8))
GL_PREFIX(MultiTexCoord2ivARB,MultiTexCoord2ivARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2ivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2sARB,MultiTexCoord2sARB@12))
GL_PREFIX(MultiTexCoord2sARB,MultiTexCoord2sARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2sARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2svARB,MultiTexCoord2svARB@8))
GL_PREFIX(MultiTexCoord2svARB,MultiTexCoord2svARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2svARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3dARB,MultiTexCoord3dARB@28))
GL_PREFIX(MultiTexCoord3dARB,MultiTexCoord3dARB@28):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3dARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3dvARB,MultiTexCoord3dvARB@8))
GL_PREFIX(MultiTexCoord3dvARB,MultiTexCoord3dvARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3dvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3fARB,MultiTexCoord3fARB@16))
GL_PREFIX(MultiTexCoord3fARB,MultiTexCoord3fARB@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3fARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3fvARB,MultiTexCoord3fvARB@8))
GL_PREFIX(MultiTexCoord3fvARB,MultiTexCoord3fvARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3fvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3iARB,MultiTexCoord3iARB@16))
GL_PREFIX(MultiTexCoord3iARB,MultiTexCoord3iARB@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3iARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3ivARB,MultiTexCoord3ivARB@8))
GL_PREFIX(MultiTexCoord3ivARB,MultiTexCoord3ivARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3ivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3sARB,MultiTexCoord3sARB@16))
GL_PREFIX(MultiTexCoord3sARB,MultiTexCoord3sARB@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3sARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3svARB,MultiTexCoord3svARB@8))
GL_PREFIX(MultiTexCoord3svARB,MultiTexCoord3svARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3svARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4dARB,MultiTexCoord4dARB@36))
GL_PREFIX(MultiTexCoord4dARB,MultiTexCoord4dARB@36):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4dARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4dvARB,MultiTexCoord4dvARB@8))
GL_PREFIX(MultiTexCoord4dvARB,MultiTexCoord4dvARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4dvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4fARB,MultiTexCoord4fARB@20))
GL_PREFIX(MultiTexCoord4fARB,MultiTexCoord4fARB@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4fARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4fvARB,MultiTexCoord4fvARB@8))
GL_PREFIX(MultiTexCoord4fvARB,MultiTexCoord4fvARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4fvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4iARB,MultiTexCoord4iARB@20))
GL_PREFIX(MultiTexCoord4iARB,MultiTexCoord4iARB@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4iARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4ivARB,MultiTexCoord4ivARB@8))
GL_PREFIX(MultiTexCoord4ivARB,MultiTexCoord4ivARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4ivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4sARB,MultiTexCoord4sARB@20))
GL_PREFIX(MultiTexCoord4sARB,MultiTexCoord4sARB@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4sARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4svARB,MultiTexCoord4svARB@8))
GL_PREFIX(MultiTexCoord4svARB,MultiTexCoord4svARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4svARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LoadTransposeMatrixfARB,LoadTransposeMatrixfARB@4))
GL_PREFIX(LoadTransposeMatrixfARB,LoadTransposeMatrixfARB@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LoadTransposeMatrixfARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LoadTransposeMatrixdARB,LoadTransposeMatrixdARB@4))
GL_PREFIX(LoadTransposeMatrixdARB,LoadTransposeMatrixdARB@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LoadTransposeMatrixdARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultTransposeMatrixfARB,MultTransposeMatrixfARB@4))
GL_PREFIX(MultTransposeMatrixfARB,MultTransposeMatrixfARB@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultTransposeMatrixfARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultTransposeMatrixdARB,MultTransposeMatrixdARB@4))
GL_PREFIX(MultTransposeMatrixdARB,MultTransposeMatrixdARB@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultTransposeMatrixdARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SampleCoverageARB,SampleCoverageARB@8))
GL_PREFIX(SampleCoverageARB,SampleCoverageARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SampleCoverageARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(__unused413,__unused413@0))
GL_PREFIX(__unused413,__unused413@0):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset___unused413))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CompressedTexImage3DARB,CompressedTexImage3DARB@36))
GL_PREFIX(CompressedTexImage3DARB,CompressedTexImage3DARB@36):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CompressedTexImage3DARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CompressedTexImage2DARB,CompressedTexImage2DARB@32))
GL_PREFIX(CompressedTexImage2DARB,CompressedTexImage2DARB@32):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CompressedTexImage2DARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CompressedTexImage1DARB,CompressedTexImage1DARB@28))
GL_PREFIX(CompressedTexImage1DARB,CompressedTexImage1DARB@28):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CompressedTexImage1DARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CompressedTexSubImage3DARB,CompressedTexSubImage3DARB@44))
GL_PREFIX(CompressedTexSubImage3DARB,CompressedTexSubImage3DARB@44):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CompressedTexSubImage3DARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CompressedTexSubImage2DARB,CompressedTexSubImage2DARB@36))
GL_PREFIX(CompressedTexSubImage2DARB,CompressedTexSubImage2DARB@36):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CompressedTexSubImage2DARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CompressedTexSubImage1DARB,CompressedTexSubImage1DARB@28))
GL_PREFIX(CompressedTexSubImage1DARB,CompressedTexSubImage1DARB@28):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CompressedTexSubImage1DARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetCompressedTexImageARB,GetCompressedTexImageARB@12))
GL_PREFIX(GetCompressedTexImageARB,GetCompressedTexImageARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetCompressedTexImageARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ActiveTexture,ActiveTexture@4))
GL_PREFIX(ActiveTexture,ActiveTexture@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ActiveTextureARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ClientActiveTexture,ClientActiveTexture@4))
GL_PREFIX(ClientActiveTexture,ClientActiveTexture@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ClientActiveTextureARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1d,MultiTexCoord1d@12))
GL_PREFIX(MultiTexCoord1d,MultiTexCoord1d@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1dARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1dv,MultiTexCoord1dv@8))
GL_PREFIX(MultiTexCoord1dv,MultiTexCoord1dv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1dvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1f,MultiTexCoord1f@8))
GL_PREFIX(MultiTexCoord1f,MultiTexCoord1f@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1fARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1fv,MultiTexCoord1fv@8))
GL_PREFIX(MultiTexCoord1fv,MultiTexCoord1fv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1fvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1i,MultiTexCoord1i@8))
GL_PREFIX(MultiTexCoord1i,MultiTexCoord1i@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1iARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1iv,MultiTexCoord1iv@8))
GL_PREFIX(MultiTexCoord1iv,MultiTexCoord1iv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1ivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1s,MultiTexCoord1s@8))
GL_PREFIX(MultiTexCoord1s,MultiTexCoord1s@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1sARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord1sv,MultiTexCoord1sv@8))
GL_PREFIX(MultiTexCoord1sv,MultiTexCoord1sv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord1svARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2d,MultiTexCoord2d@20))
GL_PREFIX(MultiTexCoord2d,MultiTexCoord2d@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2dARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2dv,MultiTexCoord2dv@8))
GL_PREFIX(MultiTexCoord2dv,MultiTexCoord2dv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2dvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2f,MultiTexCoord2f@12))
GL_PREFIX(MultiTexCoord2f,MultiTexCoord2f@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2fARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2fv,MultiTexCoord2fv@8))
GL_PREFIX(MultiTexCoord2fv,MultiTexCoord2fv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2fvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2i,MultiTexCoord2i@12))
GL_PREFIX(MultiTexCoord2i,MultiTexCoord2i@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2iARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2iv,MultiTexCoord2iv@8))
GL_PREFIX(MultiTexCoord2iv,MultiTexCoord2iv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2ivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2s,MultiTexCoord2s@12))
GL_PREFIX(MultiTexCoord2s,MultiTexCoord2s@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2sARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord2sv,MultiTexCoord2sv@8))
GL_PREFIX(MultiTexCoord2sv,MultiTexCoord2sv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord2svARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3d,MultiTexCoord3d@28))
GL_PREFIX(MultiTexCoord3d,MultiTexCoord3d@28):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3dARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3dv,MultiTexCoord3dv@8))
GL_PREFIX(MultiTexCoord3dv,MultiTexCoord3dv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3dvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3f,MultiTexCoord3f@16))
GL_PREFIX(MultiTexCoord3f,MultiTexCoord3f@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3fARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3fv,MultiTexCoord3fv@8))
GL_PREFIX(MultiTexCoord3fv,MultiTexCoord3fv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3fvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3i,MultiTexCoord3i@16))
GL_PREFIX(MultiTexCoord3i,MultiTexCoord3i@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3iARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3iv,MultiTexCoord3iv@8))
GL_PREFIX(MultiTexCoord3iv,MultiTexCoord3iv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3ivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3s,MultiTexCoord3s@16))
GL_PREFIX(MultiTexCoord3s,MultiTexCoord3s@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3sARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord3sv,MultiTexCoord3sv@8))
GL_PREFIX(MultiTexCoord3sv,MultiTexCoord3sv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord3svARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4d,MultiTexCoord4d@36))
GL_PREFIX(MultiTexCoord4d,MultiTexCoord4d@36):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4dARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4dv,MultiTexCoord4dv@8))
GL_PREFIX(MultiTexCoord4dv,MultiTexCoord4dv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4dvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4f,MultiTexCoord4f@20))
GL_PREFIX(MultiTexCoord4f,MultiTexCoord4f@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4fARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4fv,MultiTexCoord4fv@8))
GL_PREFIX(MultiTexCoord4fv,MultiTexCoord4fv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4fvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4i,MultiTexCoord4i@20))
GL_PREFIX(MultiTexCoord4i,MultiTexCoord4i@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4iARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4iv,MultiTexCoord4iv@8))
GL_PREFIX(MultiTexCoord4iv,MultiTexCoord4iv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4ivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4s,MultiTexCoord4s@20))
GL_PREFIX(MultiTexCoord4s,MultiTexCoord4s@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4sARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiTexCoord4sv,MultiTexCoord4sv@8))
GL_PREFIX(MultiTexCoord4sv,MultiTexCoord4sv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiTexCoord4svARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LoadTransposeMatrixf,LoadTransposeMatrixf@4))
GL_PREFIX(LoadTransposeMatrixf,LoadTransposeMatrixf@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LoadTransposeMatrixfARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LoadTransposeMatrixd,LoadTransposeMatrixd@4))
GL_PREFIX(LoadTransposeMatrixd,LoadTransposeMatrixd@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LoadTransposeMatrixdARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultTransposeMatrixf,MultTransposeMatrixf@4))
GL_PREFIX(MultTransposeMatrixf,MultTransposeMatrixf@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultTransposeMatrixfARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultTransposeMatrixd,MultTransposeMatrixd@4))
GL_PREFIX(MultTransposeMatrixd,MultTransposeMatrixd@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultTransposeMatrixdARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SampleCoverage,SampleCoverage@8))
GL_PREFIX(SampleCoverage,SampleCoverage@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SampleCoverageARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CompressedTexImage3D,CompressedTexImage3D@36))
GL_PREFIX(CompressedTexImage3D,CompressedTexImage3D@36):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CompressedTexImage3DARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CompressedTexImage2D,CompressedTexImage2D@32))
GL_PREFIX(CompressedTexImage2D,CompressedTexImage2D@32):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CompressedTexImage2DARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CompressedTexImage1D,CompressedTexImage1D@28))
GL_PREFIX(CompressedTexImage1D,CompressedTexImage1D@28):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CompressedTexImage1DARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CompressedTexSubImage3D,CompressedTexSubImage3D@44))
GL_PREFIX(CompressedTexSubImage3D,CompressedTexSubImage3D@44):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CompressedTexSubImage3DARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CompressedTexSubImage2D,CompressedTexSubImage2D@36))
GL_PREFIX(CompressedTexSubImage2D,CompressedTexSubImage2D@36):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CompressedTexSubImage2DARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CompressedTexSubImage1D,CompressedTexSubImage1D@28))
GL_PREFIX(CompressedTexSubImage1D,CompressedTexSubImage1D@28):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CompressedTexSubImage1DARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetCompressedTexImage,GetCompressedTexImage@12))
GL_PREFIX(GetCompressedTexImage,GetCompressedTexImage@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetCompressedTexImageARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BlendColorEXT,BlendColorEXT@16))
GL_PREFIX(BlendColorEXT,BlendColorEXT@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BlendColor))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PolygonOffsetEXT,PolygonOffsetEXT@8))
GL_PREFIX(PolygonOffsetEXT,PolygonOffsetEXT@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PolygonOffsetEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexImage3DEXT,TexImage3DEXT@40))
GL_PREFIX(TexImage3DEXT,TexImage3DEXT@40):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexImage3D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexSubImage3DEXT,TexSubImage3DEXT@44))
GL_PREFIX(TexSubImage3DEXT,TexSubImage3DEXT@44):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexSubImage3D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTexFilterFuncSGIS,GetTexFilterFuncSGIS@12))
GL_PREFIX(GetTexFilterFuncSGIS,GetTexFilterFuncSGIS@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetTexFilterFuncSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexFilterFuncSGIS,TexFilterFuncSGIS@16))
GL_PREFIX(TexFilterFuncSGIS,TexFilterFuncSGIS@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexFilterFuncSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexSubImage1DEXT,TexSubImage1DEXT@28))
GL_PREFIX(TexSubImage1DEXT,TexSubImage1DEXT@28):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexSubImage1D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexSubImage2DEXT,TexSubImage2DEXT@36))
GL_PREFIX(TexSubImage2DEXT,TexSubImage2DEXT@36):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexSubImage2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexImage1DEXT,CopyTexImage1DEXT@28))
GL_PREFIX(CopyTexImage1DEXT,CopyTexImage1DEXT@28):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyTexImage1D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexImage2DEXT,CopyTexImage2DEXT@32))
GL_PREFIX(CopyTexImage2DEXT,CopyTexImage2DEXT@32):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyTexImage2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexSubImage1DEXT,CopyTexSubImage1DEXT@24))
GL_PREFIX(CopyTexSubImage1DEXT,CopyTexSubImage1DEXT@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyTexSubImage1D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexSubImage2DEXT,CopyTexSubImage2DEXT@32))
GL_PREFIX(CopyTexSubImage2DEXT,CopyTexSubImage2DEXT@32):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyTexSubImage2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyTexSubImage3DEXT,CopyTexSubImage3DEXT@36))
GL_PREFIX(CopyTexSubImage3DEXT,CopyTexSubImage3DEXT@36):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyTexSubImage3D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetHistogramEXT,GetHistogramEXT@20))
GL_PREFIX(GetHistogramEXT,GetHistogramEXT@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetHistogramEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetHistogramParameterfvEXT,GetHistogramParameterfvEXT@12))
GL_PREFIX(GetHistogramParameterfvEXT,GetHistogramParameterfvEXT@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetHistogramParameterfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetHistogramParameterivEXT,GetHistogramParameterivEXT@12))
GL_PREFIX(GetHistogramParameterivEXT,GetHistogramParameterivEXT@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetHistogramParameterivEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMinmaxEXT,GetMinmaxEXT@20))
GL_PREFIX(GetMinmaxEXT,GetMinmaxEXT@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetMinmaxEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMinmaxParameterfvEXT,GetMinmaxParameterfvEXT@12))
GL_PREFIX(GetMinmaxParameterfvEXT,GetMinmaxParameterfvEXT@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetMinmaxParameterfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetMinmaxParameterivEXT,GetMinmaxParameterivEXT@12))
GL_PREFIX(GetMinmaxParameterivEXT,GetMinmaxParameterivEXT@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetMinmaxParameterivEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(HistogramEXT,HistogramEXT@16))
GL_PREFIX(HistogramEXT,HistogramEXT@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Histogram))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MinmaxEXT,MinmaxEXT@12))
GL_PREFIX(MinmaxEXT,MinmaxEXT@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_Minmax))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ResetHistogramEXT,ResetHistogramEXT@4))
GL_PREFIX(ResetHistogramEXT,ResetHistogramEXT@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ResetHistogram))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ResetMinmaxEXT,ResetMinmaxEXT@4))
GL_PREFIX(ResetMinmaxEXT,ResetMinmaxEXT@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ResetMinmax))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionFilter1DEXT,ConvolutionFilter1DEXT@24))
GL_PREFIX(ConvolutionFilter1DEXT,ConvolutionFilter1DEXT@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ConvolutionFilter1D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionFilter2DEXT,ConvolutionFilter2DEXT@28))
GL_PREFIX(ConvolutionFilter2DEXT,ConvolutionFilter2DEXT@28):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ConvolutionFilter2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionParameterfEXT,ConvolutionParameterfEXT@12))
GL_PREFIX(ConvolutionParameterfEXT,ConvolutionParameterfEXT@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ConvolutionParameterf))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionParameterfvEXT,ConvolutionParameterfvEXT@12))
GL_PREFIX(ConvolutionParameterfvEXT,ConvolutionParameterfvEXT@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ConvolutionParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionParameteriEXT,ConvolutionParameteriEXT@12))
GL_PREFIX(ConvolutionParameteriEXT,ConvolutionParameteriEXT@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ConvolutionParameteri))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ConvolutionParameterivEXT,ConvolutionParameterivEXT@12))
GL_PREFIX(ConvolutionParameterivEXT,ConvolutionParameterivEXT@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ConvolutionParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyConvolutionFilter1DEXT,CopyConvolutionFilter1DEXT@20))
GL_PREFIX(CopyConvolutionFilter1DEXT,CopyConvolutionFilter1DEXT@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyConvolutionFilter1D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyConvolutionFilter2DEXT,CopyConvolutionFilter2DEXT@24))
GL_PREFIX(CopyConvolutionFilter2DEXT,CopyConvolutionFilter2DEXT@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyConvolutionFilter2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetConvolutionFilterEXT,GetConvolutionFilterEXT@16))
GL_PREFIX(GetConvolutionFilterEXT,GetConvolutionFilterEXT@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetConvolutionFilterEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetConvolutionParameterfvEXT,GetConvolutionParameterfvEXT@12))
GL_PREFIX(GetConvolutionParameterfvEXT,GetConvolutionParameterfvEXT@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetConvolutionParameterfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetConvolutionParameterivEXT,GetConvolutionParameterivEXT@12))
GL_PREFIX(GetConvolutionParameterivEXT,GetConvolutionParameterivEXT@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetConvolutionParameterivEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetSeparableFilterEXT,GetSeparableFilterEXT@24))
GL_PREFIX(GetSeparableFilterEXT,GetSeparableFilterEXT@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetSeparableFilterEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SeparableFilter2DEXT,SeparableFilter2DEXT@32))
GL_PREFIX(SeparableFilter2DEXT,SeparableFilter2DEXT@32):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SeparableFilter2D))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorTableSGI,ColorTableSGI@24))
GL_PREFIX(ColorTableSGI,ColorTableSGI@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ColorTable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorTableParameterfvSGI,ColorTableParameterfvSGI@12))
GL_PREFIX(ColorTableParameterfvSGI,ColorTableParameterfvSGI@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ColorTableParameterfv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorTableParameterivSGI,ColorTableParameterivSGI@12))
GL_PREFIX(ColorTableParameterivSGI,ColorTableParameterivSGI@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ColorTableParameteriv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyColorTableSGI,CopyColorTableSGI@20))
GL_PREFIX(CopyColorTableSGI,CopyColorTableSGI@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyColorTable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetColorTableSGI,GetColorTableSGI@16))
GL_PREFIX(GetColorTableSGI,GetColorTableSGI@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetColorTableSGI))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetColorTableParameterfvSGI,GetColorTableParameterfvSGI@12))
GL_PREFIX(GetColorTableParameterfvSGI,GetColorTableParameterfvSGI@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetColorTableParameterfvSGI))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetColorTableParameterivSGI,GetColorTableParameterivSGI@12))
GL_PREFIX(GetColorTableParameterivSGI,GetColorTableParameterivSGI@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetColorTableParameterivSGI))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelTexGenSGIX,PixelTexGenSGIX@4))
GL_PREFIX(PixelTexGenSGIX,PixelTexGenSGIX@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PixelTexGenSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelTexGenParameteriSGIS,PixelTexGenParameteriSGIS@8))
GL_PREFIX(PixelTexGenParameteriSGIS,PixelTexGenParameteriSGIS@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PixelTexGenParameteriSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelTexGenParameterivSGIS,PixelTexGenParameterivSGIS@8))
GL_PREFIX(PixelTexGenParameterivSGIS,PixelTexGenParameterivSGIS@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PixelTexGenParameterivSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelTexGenParameterfSGIS,PixelTexGenParameterfSGIS@8))
GL_PREFIX(PixelTexGenParameterfSGIS,PixelTexGenParameterfSGIS@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PixelTexGenParameterfSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PixelTexGenParameterfvSGIS,PixelTexGenParameterfvSGIS@8))
GL_PREFIX(PixelTexGenParameterfvSGIS,PixelTexGenParameterfvSGIS@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PixelTexGenParameterfvSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetPixelTexGenParameterivSGIS,GetPixelTexGenParameterivSGIS@8))
GL_PREFIX(GetPixelTexGenParameterivSGIS,GetPixelTexGenParameterivSGIS@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetPixelTexGenParameterivSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetPixelTexGenParameterfvSGIS,GetPixelTexGenParameterfvSGIS@8))
GL_PREFIX(GetPixelTexGenParameterfvSGIS,GetPixelTexGenParameterfvSGIS@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetPixelTexGenParameterfvSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexImage4DSGIS,TexImage4DSGIS@44))
GL_PREFIX(TexImage4DSGIS,TexImage4DSGIS@44):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexImage4DSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexSubImage4DSGIS,TexSubImage4DSGIS@52))
GL_PREFIX(TexSubImage4DSGIS,TexSubImage4DSGIS@52):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexSubImage4DSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(AreTexturesResidentEXT,AreTexturesResidentEXT@12))
GL_PREFIX(AreTexturesResidentEXT,AreTexturesResidentEXT@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_AreTexturesResidentEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BindTextureEXT,BindTextureEXT@8))
GL_PREFIX(BindTextureEXT,BindTextureEXT@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BindTexture))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DeleteTexturesEXT,DeleteTexturesEXT@8))
GL_PREFIX(DeleteTexturesEXT,DeleteTexturesEXT@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DeleteTextures))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GenTexturesEXT,GenTexturesEXT@8))
GL_PREFIX(GenTexturesEXT,GenTexturesEXT@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GenTexturesEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IsTextureEXT,IsTextureEXT@4))
GL_PREFIX(IsTextureEXT,IsTextureEXT@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_IsTextureEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PrioritizeTexturesEXT,PrioritizeTexturesEXT@12))
GL_PREFIX(PrioritizeTexturesEXT,PrioritizeTexturesEXT@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PrioritizeTextures))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DetailTexFuncSGIS,DetailTexFuncSGIS@12))
GL_PREFIX(DetailTexFuncSGIS,DetailTexFuncSGIS@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DetailTexFuncSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetDetailTexFuncSGIS,GetDetailTexFuncSGIS@8))
GL_PREFIX(GetDetailTexFuncSGIS,GetDetailTexFuncSGIS@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetDetailTexFuncSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SharpenTexFuncSGIS,SharpenTexFuncSGIS@12))
GL_PREFIX(SharpenTexFuncSGIS,SharpenTexFuncSGIS@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SharpenTexFuncSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetSharpenTexFuncSGIS,GetSharpenTexFuncSGIS@8))
GL_PREFIX(GetSharpenTexFuncSGIS,GetSharpenTexFuncSGIS@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetSharpenTexFuncSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SampleMaskSGIS,SampleMaskSGIS@8))
GL_PREFIX(SampleMaskSGIS,SampleMaskSGIS@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SampleMaskSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SamplePatternSGIS,SamplePatternSGIS@4))
GL_PREFIX(SamplePatternSGIS,SamplePatternSGIS@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SamplePatternSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ArrayElementEXT,ArrayElementEXT@4))
GL_PREFIX(ArrayElementEXT,ArrayElementEXT@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ArrayElement))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorPointerEXT,ColorPointerEXT@20))
GL_PREFIX(ColorPointerEXT,ColorPointerEXT@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ColorPointerEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DrawArraysEXT,DrawArraysEXT@12))
GL_PREFIX(DrawArraysEXT,DrawArraysEXT@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DrawArrays))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EdgeFlagPointerEXT,EdgeFlagPointerEXT@12))
GL_PREFIX(EdgeFlagPointerEXT,EdgeFlagPointerEXT@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EdgeFlagPointerEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetPointervEXT,GetPointervEXT@8))
GL_PREFIX(GetPointervEXT,GetPointervEXT@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetPointerv))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IndexPointerEXT,IndexPointerEXT@16))
GL_PREFIX(IndexPointerEXT,IndexPointerEXT@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_IndexPointerEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(NormalPointerEXT,NormalPointerEXT@16))
GL_PREFIX(NormalPointerEXT,NormalPointerEXT@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_NormalPointerEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TexCoordPointerEXT,TexCoordPointerEXT@20))
GL_PREFIX(TexCoordPointerEXT,TexCoordPointerEXT@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TexCoordPointerEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexPointerEXT,VertexPointerEXT@20))
GL_PREFIX(VertexPointerEXT,VertexPointerEXT@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexPointerEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BlendEquationEXT,BlendEquationEXT@4))
GL_PREFIX(BlendEquationEXT,BlendEquationEXT@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BlendEquation))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SpriteParameterfSGIX,SpriteParameterfSGIX@8))
GL_PREFIX(SpriteParameterfSGIX,SpriteParameterfSGIX@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SpriteParameterfSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SpriteParameterfvSGIX,SpriteParameterfvSGIX@8))
GL_PREFIX(SpriteParameterfvSGIX,SpriteParameterfvSGIX@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SpriteParameterfvSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SpriteParameteriSGIX,SpriteParameteriSGIX@8))
GL_PREFIX(SpriteParameteriSGIX,SpriteParameteriSGIX@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SpriteParameteriSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SpriteParameterivSGIX,SpriteParameterivSGIX@8))
GL_PREFIX(SpriteParameterivSGIX,SpriteParameterivSGIX@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SpriteParameterivSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointParameterfEXT,PointParameterfEXT@8))
GL_PREFIX(PointParameterfEXT,PointParameterfEXT@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PointParameterfEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointParameterfvEXT,PointParameterfvEXT@8))
GL_PREFIX(PointParameterfvEXT,PointParameterfvEXT@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PointParameterfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointParameterfARB,PointParameterfARB@8))
GL_PREFIX(PointParameterfARB,PointParameterfARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PointParameterfEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointParameterfvARB,PointParameterfvARB@8))
GL_PREFIX(PointParameterfvARB,PointParameterfvARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PointParameterfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointParameterfSGIS,PointParameterfSGIS@8))
GL_PREFIX(PointParameterfSGIS,PointParameterfSGIS@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PointParameterfEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointParameterfvSGIS,PointParameterfvSGIS@8))
GL_PREFIX(PointParameterfvSGIS,PointParameterfvSGIS@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PointParameterfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetInstrumentsSGIX,GetInstrumentsSGIX@0))
GL_PREFIX(GetInstrumentsSGIX,GetInstrumentsSGIX@0):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetInstrumentsSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(InstrumentsBufferSGIX,InstrumentsBufferSGIX@8))
GL_PREFIX(InstrumentsBufferSGIX,InstrumentsBufferSGIX@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_InstrumentsBufferSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PollInstrumentsSGIX,PollInstrumentsSGIX@4))
GL_PREFIX(PollInstrumentsSGIX,PollInstrumentsSGIX@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PollInstrumentsSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ReadInstrumentsSGIX,ReadInstrumentsSGIX@4))
GL_PREFIX(ReadInstrumentsSGIX,ReadInstrumentsSGIX@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ReadInstrumentsSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(StartInstrumentsSGIX,StartInstrumentsSGIX@0))
GL_PREFIX(StartInstrumentsSGIX,StartInstrumentsSGIX@0):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_StartInstrumentsSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(StopInstrumentsSGIX,StopInstrumentsSGIX@4))
GL_PREFIX(StopInstrumentsSGIX,StopInstrumentsSGIX@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_StopInstrumentsSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FrameZoomSGIX,FrameZoomSGIX@4))
GL_PREFIX(FrameZoomSGIX,FrameZoomSGIX@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FrameZoomSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TagSampleBufferSGIX,TagSampleBufferSGIX@0))
GL_PREFIX(TagSampleBufferSGIX,TagSampleBufferSGIX@0):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TagSampleBufferSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ReferencePlaneSGIX,ReferencePlaneSGIX@4))
GL_PREFIX(ReferencePlaneSGIX,ReferencePlaneSGIX@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ReferencePlaneSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FlushRasterSGIX,FlushRasterSGIX@0))
GL_PREFIX(FlushRasterSGIX,FlushRasterSGIX@0):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FlushRasterSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorSubTableEXT,ColorSubTableEXT@24))
GL_PREFIX(ColorSubTableEXT,ColorSubTableEXT@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ColorSubTable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CopyColorSubTableEXT,CopyColorSubTableEXT@20))
GL_PREFIX(CopyColorSubTableEXT,CopyColorSubTableEXT@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CopyColorSubTable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(HintPGI,HintPGI@8))
GL_PREFIX(HintPGI,HintPGI@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_HintPGI))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ColorTableEXT,ColorTableEXT@24))
GL_PREFIX(ColorTableEXT,ColorTableEXT@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ColorTable))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetColorTableEXT,GetColorTableEXT@16))
GL_PREFIX(GetColorTableEXT,GetColorTableEXT@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetColorTableEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetColorTableParameterivEXT,GetColorTableParameterivEXT@12))
GL_PREFIX(GetColorTableParameterivEXT,GetColorTableParameterivEXT@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetColorTableParameterivEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetColorTableParameterfvEXT,GetColorTableParameterfvEXT@12))
GL_PREFIX(GetColorTableParameterfvEXT,GetColorTableParameterfvEXT@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetColorTableParameterfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetListParameterfvSGIX,GetListParameterfvSGIX@12))
GL_PREFIX(GetListParameterfvSGIX,GetListParameterfvSGIX@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetListParameterfvSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetListParameterivSGIX,GetListParameterivSGIX@12))
GL_PREFIX(GetListParameterivSGIX,GetListParameterivSGIX@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetListParameterivSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ListParameterfSGIX,ListParameterfSGIX@12))
GL_PREFIX(ListParameterfSGIX,ListParameterfSGIX@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ListParameterfSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ListParameterfvSGIX,ListParameterfvSGIX@12))
GL_PREFIX(ListParameterfvSGIX,ListParameterfvSGIX@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ListParameterfvSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ListParameteriSGIX,ListParameteriSGIX@12))
GL_PREFIX(ListParameteriSGIX,ListParameteriSGIX@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ListParameteriSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ListParameterivSGIX,ListParameterivSGIX@12))
GL_PREFIX(ListParameterivSGIX,ListParameterivSGIX@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ListParameterivSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IndexMaterialEXT,IndexMaterialEXT@8))
GL_PREFIX(IndexMaterialEXT,IndexMaterialEXT@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_IndexMaterialEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IndexFuncEXT,IndexFuncEXT@8))
GL_PREFIX(IndexFuncEXT,IndexFuncEXT@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_IndexFuncEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LockArraysEXT,LockArraysEXT@8))
GL_PREFIX(LockArraysEXT,LockArraysEXT@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LockArraysEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(UnlockArraysEXT,UnlockArraysEXT@0))
GL_PREFIX(UnlockArraysEXT,UnlockArraysEXT@0):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_UnlockArraysEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CullParameterdvEXT,CullParameterdvEXT@8))
GL_PREFIX(CullParameterdvEXT,CullParameterdvEXT@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CullParameterdvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CullParameterfvEXT,CullParameterfvEXT@8))
GL_PREFIX(CullParameterfvEXT,CullParameterfvEXT@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CullParameterfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FragmentColorMaterialSGIX,FragmentColorMaterialSGIX@8))
GL_PREFIX(FragmentColorMaterialSGIX,FragmentColorMaterialSGIX@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FragmentColorMaterialSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FragmentLightfSGIX,FragmentLightfSGIX@12))
GL_PREFIX(FragmentLightfSGIX,FragmentLightfSGIX@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FragmentLightfSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FragmentLightfvSGIX,FragmentLightfvSGIX@12))
GL_PREFIX(FragmentLightfvSGIX,FragmentLightfvSGIX@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FragmentLightfvSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FragmentLightiSGIX,FragmentLightiSGIX@12))
GL_PREFIX(FragmentLightiSGIX,FragmentLightiSGIX@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FragmentLightiSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FragmentLightivSGIX,FragmentLightivSGIX@12))
GL_PREFIX(FragmentLightivSGIX,FragmentLightivSGIX@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FragmentLightivSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FragmentLightModelfSGIX,FragmentLightModelfSGIX@8))
GL_PREFIX(FragmentLightModelfSGIX,FragmentLightModelfSGIX@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FragmentLightModelfSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FragmentLightModelfvSGIX,FragmentLightModelfvSGIX@8))
GL_PREFIX(FragmentLightModelfvSGIX,FragmentLightModelfvSGIX@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FragmentLightModelfvSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FragmentLightModeliSGIX,FragmentLightModeliSGIX@8))
GL_PREFIX(FragmentLightModeliSGIX,FragmentLightModeliSGIX@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FragmentLightModeliSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FragmentLightModelivSGIX,FragmentLightModelivSGIX@8))
GL_PREFIX(FragmentLightModelivSGIX,FragmentLightModelivSGIX@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FragmentLightModelivSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FragmentMaterialfSGIX,FragmentMaterialfSGIX@12))
GL_PREFIX(FragmentMaterialfSGIX,FragmentMaterialfSGIX@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FragmentMaterialfSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FragmentMaterialfvSGIX,FragmentMaterialfvSGIX@12))
GL_PREFIX(FragmentMaterialfvSGIX,FragmentMaterialfvSGIX@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FragmentMaterialfvSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FragmentMaterialiSGIX,FragmentMaterialiSGIX@12))
GL_PREFIX(FragmentMaterialiSGIX,FragmentMaterialiSGIX@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FragmentMaterialiSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FragmentMaterialivSGIX,FragmentMaterialivSGIX@12))
GL_PREFIX(FragmentMaterialivSGIX,FragmentMaterialivSGIX@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FragmentMaterialivSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetFragmentLightfvSGIX,GetFragmentLightfvSGIX@12))
GL_PREFIX(GetFragmentLightfvSGIX,GetFragmentLightfvSGIX@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetFragmentLightfvSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetFragmentLightivSGIX,GetFragmentLightivSGIX@12))
GL_PREFIX(GetFragmentLightivSGIX,GetFragmentLightivSGIX@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetFragmentLightivSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetFragmentMaterialfvSGIX,GetFragmentMaterialfvSGIX@12))
GL_PREFIX(GetFragmentMaterialfvSGIX,GetFragmentMaterialfvSGIX@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetFragmentMaterialfvSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetFragmentMaterialivSGIX,GetFragmentMaterialivSGIX@12))
GL_PREFIX(GetFragmentMaterialivSGIX,GetFragmentMaterialivSGIX@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetFragmentMaterialivSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LightEnviSGIX,LightEnviSGIX@8))
GL_PREFIX(LightEnviSGIX,LightEnviSGIX@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LightEnviSGIX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DrawRangeElementsEXT,DrawRangeElementsEXT@24))
GL_PREFIX(DrawRangeElementsEXT,DrawRangeElementsEXT@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DrawRangeElements))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3bEXT,SecondaryColor3bEXT@12))
GL_PREFIX(SecondaryColor3bEXT,SecondaryColor3bEXT@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3bEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3bvEXT,SecondaryColor3bvEXT@4))
GL_PREFIX(SecondaryColor3bvEXT,SecondaryColor3bvEXT@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3bvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3dEXT,SecondaryColor3dEXT@24))
GL_PREFIX(SecondaryColor3dEXT,SecondaryColor3dEXT@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3dEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3dvEXT,SecondaryColor3dvEXT@4))
GL_PREFIX(SecondaryColor3dvEXT,SecondaryColor3dvEXT@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3dvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3fEXT,SecondaryColor3fEXT@12))
GL_PREFIX(SecondaryColor3fEXT,SecondaryColor3fEXT@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3fEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3fvEXT,SecondaryColor3fvEXT@4))
GL_PREFIX(SecondaryColor3fvEXT,SecondaryColor3fvEXT@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3fvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3iEXT,SecondaryColor3iEXT@12))
GL_PREFIX(SecondaryColor3iEXT,SecondaryColor3iEXT@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3iEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3ivEXT,SecondaryColor3ivEXT@4))
GL_PREFIX(SecondaryColor3ivEXT,SecondaryColor3ivEXT@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3ivEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3sEXT,SecondaryColor3sEXT@12))
GL_PREFIX(SecondaryColor3sEXT,SecondaryColor3sEXT@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3sEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3svEXT,SecondaryColor3svEXT@4))
GL_PREFIX(SecondaryColor3svEXT,SecondaryColor3svEXT@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3svEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3ubEXT,SecondaryColor3ubEXT@12))
GL_PREFIX(SecondaryColor3ubEXT,SecondaryColor3ubEXT@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3ubEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3ubvEXT,SecondaryColor3ubvEXT@4))
GL_PREFIX(SecondaryColor3ubvEXT,SecondaryColor3ubvEXT@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3ubvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3uiEXT,SecondaryColor3uiEXT@12))
GL_PREFIX(SecondaryColor3uiEXT,SecondaryColor3uiEXT@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3uiEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3uivEXT,SecondaryColor3uivEXT@4))
GL_PREFIX(SecondaryColor3uivEXT,SecondaryColor3uivEXT@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3uivEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3usEXT,SecondaryColor3usEXT@12))
GL_PREFIX(SecondaryColor3usEXT,SecondaryColor3usEXT@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3usEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3usvEXT,SecondaryColor3usvEXT@4))
GL_PREFIX(SecondaryColor3usvEXT,SecondaryColor3usvEXT@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3usvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColorPointerEXT,SecondaryColorPointerEXT@16))
GL_PREFIX(SecondaryColorPointerEXT,SecondaryColorPointerEXT@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColorPointerEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiDrawArraysEXT,MultiDrawArraysEXT@16))
GL_PREFIX(MultiDrawArraysEXT,MultiDrawArraysEXT@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiDrawArraysEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiDrawElementsEXT,MultiDrawElementsEXT@20))
GL_PREFIX(MultiDrawElementsEXT,MultiDrawElementsEXT@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiDrawElementsEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FogCoordfEXT,FogCoordfEXT@4))
GL_PREFIX(FogCoordfEXT,FogCoordfEXT@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FogCoordfEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FogCoordfvEXT,FogCoordfvEXT@4))
GL_PREFIX(FogCoordfvEXT,FogCoordfvEXT@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FogCoordfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FogCoorddEXT,FogCoorddEXT@8))
GL_PREFIX(FogCoorddEXT,FogCoorddEXT@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FogCoorddEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FogCoorddvEXT,FogCoorddvEXT@4))
GL_PREFIX(FogCoorddvEXT,FogCoorddvEXT@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FogCoorddvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FogCoordPointerEXT,FogCoordPointerEXT@12))
GL_PREFIX(FogCoordPointerEXT,FogCoordPointerEXT@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FogCoordPointerEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BlendFuncSeparateEXT,BlendFuncSeparateEXT@16))
GL_PREFIX(BlendFuncSeparateEXT,BlendFuncSeparateEXT@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BlendFuncSeparateEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BlendFuncSeparateINGR,BlendFuncSeparateINGR@16))
GL_PREFIX(BlendFuncSeparateINGR,BlendFuncSeparateINGR@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BlendFuncSeparateEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexWeightfEXT,VertexWeightfEXT@4))
GL_PREFIX(VertexWeightfEXT,VertexWeightfEXT@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexWeightfEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexWeightfvEXT,VertexWeightfvEXT@4))
GL_PREFIX(VertexWeightfvEXT,VertexWeightfvEXT@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexWeightfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexWeightPointerEXT,VertexWeightPointerEXT@16))
GL_PREFIX(VertexWeightPointerEXT,VertexWeightPointerEXT@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexWeightPointerEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FlushVertexArrayRangeNV,FlushVertexArrayRangeNV@0))
GL_PREFIX(FlushVertexArrayRangeNV,FlushVertexArrayRangeNV@0):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FlushVertexArrayRangeNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexArrayRangeNV,VertexArrayRangeNV@8))
GL_PREFIX(VertexArrayRangeNV,VertexArrayRangeNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexArrayRangeNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CombinerParameterfvNV,CombinerParameterfvNV@8))
GL_PREFIX(CombinerParameterfvNV,CombinerParameterfvNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CombinerParameterfvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CombinerParameterfNV,CombinerParameterfNV@8))
GL_PREFIX(CombinerParameterfNV,CombinerParameterfNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CombinerParameterfNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CombinerParameterivNV,CombinerParameterivNV@8))
GL_PREFIX(CombinerParameterivNV,CombinerParameterivNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CombinerParameterivNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CombinerParameteriNV,CombinerParameteriNV@8))
GL_PREFIX(CombinerParameteriNV,CombinerParameteriNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CombinerParameteriNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CombinerInputNV,CombinerInputNV@24))
GL_PREFIX(CombinerInputNV,CombinerInputNV@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CombinerInputNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(CombinerOutputNV,CombinerOutputNV@40))
GL_PREFIX(CombinerOutputNV,CombinerOutputNV@40):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_CombinerOutputNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FinalCombinerInputNV,FinalCombinerInputNV@16))
GL_PREFIX(FinalCombinerInputNV,FinalCombinerInputNV@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FinalCombinerInputNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetCombinerInputParameterfvNV,GetCombinerInputParameterfvNV@20))
GL_PREFIX(GetCombinerInputParameterfvNV,GetCombinerInputParameterfvNV@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetCombinerInputParameterfvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetCombinerInputParameterivNV,GetCombinerInputParameterivNV@20))
GL_PREFIX(GetCombinerInputParameterivNV,GetCombinerInputParameterivNV@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetCombinerInputParameterivNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetCombinerOutputParameterfvNV,GetCombinerOutputParameterfvNV@16))
GL_PREFIX(GetCombinerOutputParameterfvNV,GetCombinerOutputParameterfvNV@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetCombinerOutputParameterfvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetCombinerOutputParameterivNV,GetCombinerOutputParameterivNV@16))
GL_PREFIX(GetCombinerOutputParameterivNV,GetCombinerOutputParameterivNV@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetCombinerOutputParameterivNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetFinalCombinerInputParameterfvNV,GetFinalCombinerInputParameterfvNV@12))
GL_PREFIX(GetFinalCombinerInputParameterfvNV,GetFinalCombinerInputParameterfvNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetFinalCombinerInputParameterfvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetFinalCombinerInputParameterivNV,GetFinalCombinerInputParameterivNV@12))
GL_PREFIX(GetFinalCombinerInputParameterivNV,GetFinalCombinerInputParameterivNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetFinalCombinerInputParameterivNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ResizeBuffersMESA,ResizeBuffersMESA@0))
GL_PREFIX(ResizeBuffersMESA,ResizeBuffersMESA@0):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ResizeBuffersMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2dMESA,WindowPos2dMESA@16))
GL_PREFIX(WindowPos2dMESA,WindowPos2dMESA@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2dMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2dvMESA,WindowPos2dvMESA@4))
GL_PREFIX(WindowPos2dvMESA,WindowPos2dvMESA@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2dvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2fMESA,WindowPos2fMESA@8))
GL_PREFIX(WindowPos2fMESA,WindowPos2fMESA@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2fMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2fvMESA,WindowPos2fvMESA@4))
GL_PREFIX(WindowPos2fvMESA,WindowPos2fvMESA@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2fvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2iMESA,WindowPos2iMESA@8))
GL_PREFIX(WindowPos2iMESA,WindowPos2iMESA@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2iMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2ivMESA,WindowPos2ivMESA@4))
GL_PREFIX(WindowPos2ivMESA,WindowPos2ivMESA@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2ivMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2sMESA,WindowPos2sMESA@8))
GL_PREFIX(WindowPos2sMESA,WindowPos2sMESA@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2sMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2svMESA,WindowPos2svMESA@4))
GL_PREFIX(WindowPos2svMESA,WindowPos2svMESA@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2svMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3dMESA,WindowPos3dMESA@24))
GL_PREFIX(WindowPos3dMESA,WindowPos3dMESA@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3dMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3dvMESA,WindowPos3dvMESA@4))
GL_PREFIX(WindowPos3dvMESA,WindowPos3dvMESA@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3dvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3fMESA,WindowPos3fMESA@12))
GL_PREFIX(WindowPos3fMESA,WindowPos3fMESA@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3fMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3fvMESA,WindowPos3fvMESA@4))
GL_PREFIX(WindowPos3fvMESA,WindowPos3fvMESA@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3fvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3iMESA,WindowPos3iMESA@12))
GL_PREFIX(WindowPos3iMESA,WindowPos3iMESA@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3iMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3ivMESA,WindowPos3ivMESA@4))
GL_PREFIX(WindowPos3ivMESA,WindowPos3ivMESA@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3ivMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3sMESA,WindowPos3sMESA@12))
GL_PREFIX(WindowPos3sMESA,WindowPos3sMESA@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3sMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3svMESA,WindowPos3svMESA@4))
GL_PREFIX(WindowPos3svMESA,WindowPos3svMESA@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3svMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos4dMESA,WindowPos4dMESA@32))
GL_PREFIX(WindowPos4dMESA,WindowPos4dMESA@32):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos4dMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos4dvMESA,WindowPos4dvMESA@4))
GL_PREFIX(WindowPos4dvMESA,WindowPos4dvMESA@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos4dvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos4fMESA,WindowPos4fMESA@16))
GL_PREFIX(WindowPos4fMESA,WindowPos4fMESA@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos4fMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos4fvMESA,WindowPos4fvMESA@4))
GL_PREFIX(WindowPos4fvMESA,WindowPos4fvMESA@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos4fvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos4iMESA,WindowPos4iMESA@16))
GL_PREFIX(WindowPos4iMESA,WindowPos4iMESA@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos4iMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos4ivMESA,WindowPos4ivMESA@4))
GL_PREFIX(WindowPos4ivMESA,WindowPos4ivMESA@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos4ivMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos4sMESA,WindowPos4sMESA@16))
GL_PREFIX(WindowPos4sMESA,WindowPos4sMESA@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos4sMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos4svMESA,WindowPos4svMESA@4))
GL_PREFIX(WindowPos4svMESA,WindowPos4svMESA@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos4svMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiModeDrawArraysIBM,MultiModeDrawArraysIBM@20))
GL_PREFIX(MultiModeDrawArraysIBM,MultiModeDrawArraysIBM@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiModeDrawArraysIBM))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiModeDrawElementsIBM,MultiModeDrawElementsIBM@24))
GL_PREFIX(MultiModeDrawElementsIBM,MultiModeDrawElementsIBM@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiModeDrawElementsIBM))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TbufferMask3DFX,TbufferMask3DFX@4))
GL_PREFIX(TbufferMask3DFX,TbufferMask3DFX@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TbufferMask3DFX))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SampleMaskEXT,SampleMaskEXT@8))
GL_PREFIX(SampleMaskEXT,SampleMaskEXT@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SampleMaskSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SamplePatternEXT,SamplePatternEXT@4))
GL_PREFIX(SamplePatternEXT,SamplePatternEXT@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SamplePatternSGIS))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2dARB,WindowPos2dARB@16))
GL_PREFIX(WindowPos2dARB,WindowPos2dARB@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2dMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2fARB,WindowPos2fARB@8))
GL_PREFIX(WindowPos2fARB,WindowPos2fARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2fMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2iARB,WindowPos2iARB@8))
GL_PREFIX(WindowPos2iARB,WindowPos2iARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2iMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2sARB,WindowPos2sARB@8))
GL_PREFIX(WindowPos2sARB,WindowPos2sARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2sMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2dvARB,WindowPos2dvARB@4))
GL_PREFIX(WindowPos2dvARB,WindowPos2dvARB@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2dvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2fvARB,WindowPos2fvARB@4))
GL_PREFIX(WindowPos2fvARB,WindowPos2fvARB@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2fvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2ivARB,WindowPos2ivARB@4))
GL_PREFIX(WindowPos2ivARB,WindowPos2ivARB@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2ivMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2svARB,WindowPos2svARB@4))
GL_PREFIX(WindowPos2svARB,WindowPos2svARB@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2svMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3dARB,WindowPos3dARB@24))
GL_PREFIX(WindowPos3dARB,WindowPos3dARB@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3dMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3fARB,WindowPos3fARB@12))
GL_PREFIX(WindowPos3fARB,WindowPos3fARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3fMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3iARB,WindowPos3iARB@12))
GL_PREFIX(WindowPos3iARB,WindowPos3iARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3iMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3sARB,WindowPos3sARB@12))
GL_PREFIX(WindowPos3sARB,WindowPos3sARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3sMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3dvARB,WindowPos3dvARB@4))
GL_PREFIX(WindowPos3dvARB,WindowPos3dvARB@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3dvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3fvARB,WindowPos3fvARB@4))
GL_PREFIX(WindowPos3fvARB,WindowPos3fvARB@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3fvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3ivARB,WindowPos3ivARB@4))
GL_PREFIX(WindowPos3ivARB,WindowPos3ivARB@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3ivMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3svARB,WindowPos3svARB@4))
GL_PREFIX(WindowPos3svARB,WindowPos3svARB@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3svMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(AreProgramsResidentNV,AreProgramsResidentNV@12))
GL_PREFIX(AreProgramsResidentNV,AreProgramsResidentNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_AreProgramsResidentNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BindProgramNV,BindProgramNV@8))
GL_PREFIX(BindProgramNV,BindProgramNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BindProgramNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DeleteProgramsNV,DeleteProgramsNV@8))
GL_PREFIX(DeleteProgramsNV,DeleteProgramsNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DeleteProgramsNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ExecuteProgramNV,ExecuteProgramNV@12))
GL_PREFIX(ExecuteProgramNV,ExecuteProgramNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ExecuteProgramNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GenProgramsNV,GenProgramsNV@8))
GL_PREFIX(GenProgramsNV,GenProgramsNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GenProgramsNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetProgramParameterdvNV,GetProgramParameterdvNV@16))
GL_PREFIX(GetProgramParameterdvNV,GetProgramParameterdvNV@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetProgramParameterdvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetProgramParameterfvNV,GetProgramParameterfvNV@16))
GL_PREFIX(GetProgramParameterfvNV,GetProgramParameterfvNV@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetProgramParameterfvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetProgramivNV,GetProgramivNV@12))
GL_PREFIX(GetProgramivNV,GetProgramivNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetProgramivNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetProgramStringNV,GetProgramStringNV@12))
GL_PREFIX(GetProgramStringNV,GetProgramStringNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetProgramStringNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetTrackMatrixivNV,GetTrackMatrixivNV@16))
GL_PREFIX(GetTrackMatrixivNV,GetTrackMatrixivNV@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetTrackMatrixivNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetVertexAttribdvNV,GetVertexAttribdvNV@12))
GL_PREFIX(GetVertexAttribdvNV,GetVertexAttribdvNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetVertexAttribdvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetVertexAttribfvNV,GetVertexAttribfvNV@12))
GL_PREFIX(GetVertexAttribfvNV,GetVertexAttribfvNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetVertexAttribfvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetVertexAttribivNV,GetVertexAttribivNV@12))
GL_PREFIX(GetVertexAttribivNV,GetVertexAttribivNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetVertexAttribivNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetVertexAttribPointervNV,GetVertexAttribPointervNV@12))
GL_PREFIX(GetVertexAttribPointervNV,GetVertexAttribPointervNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetVertexAttribPointervNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IsProgramNV,IsProgramNV@4))
GL_PREFIX(IsProgramNV,IsProgramNV@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_IsProgramNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(LoadProgramNV,LoadProgramNV@16))
GL_PREFIX(LoadProgramNV,LoadProgramNV@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_LoadProgramNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ProgramParameter4dNV,ProgramParameter4dNV@40))
GL_PREFIX(ProgramParameter4dNV,ProgramParameter4dNV@40):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ProgramParameter4dNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ProgramParameter4dvNV,ProgramParameter4dvNV@12))
GL_PREFIX(ProgramParameter4dvNV,ProgramParameter4dvNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ProgramParameter4dvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ProgramParameter4fNV,ProgramParameter4fNV@24))
GL_PREFIX(ProgramParameter4fNV,ProgramParameter4fNV@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ProgramParameter4fNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ProgramParameter4fvNV,ProgramParameter4fvNV@12))
GL_PREFIX(ProgramParameter4fvNV,ProgramParameter4fvNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ProgramParameter4fvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ProgramParameters4dvNV,ProgramParameters4dvNV@16))
GL_PREFIX(ProgramParameters4dvNV,ProgramParameters4dvNV@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ProgramParameters4dvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ProgramParameters4fvNV,ProgramParameters4fvNV@16))
GL_PREFIX(ProgramParameters4fvNV,ProgramParameters4fvNV@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ProgramParameters4fvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(RequestResidentProgramsNV,RequestResidentProgramsNV@8))
GL_PREFIX(RequestResidentProgramsNV,RequestResidentProgramsNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_RequestResidentProgramsNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TrackMatrixNV,TrackMatrixNV@16))
GL_PREFIX(TrackMatrixNV,TrackMatrixNV@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TrackMatrixNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribPointerNV,VertexAttribPointerNV@20))
GL_PREFIX(VertexAttribPointerNV,VertexAttribPointerNV@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribPointerNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib1dNV,VertexAttrib1dNV@12))
GL_PREFIX(VertexAttrib1dNV,VertexAttrib1dNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib1dNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib1dvNV,VertexAttrib1dvNV@8))
GL_PREFIX(VertexAttrib1dvNV,VertexAttrib1dvNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib1dvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib1fNV,VertexAttrib1fNV@8))
GL_PREFIX(VertexAttrib1fNV,VertexAttrib1fNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib1fNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib1fvNV,VertexAttrib1fvNV@8))
GL_PREFIX(VertexAttrib1fvNV,VertexAttrib1fvNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib1fvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib1sNV,VertexAttrib1sNV@8))
GL_PREFIX(VertexAttrib1sNV,VertexAttrib1sNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib1sNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib1svNV,VertexAttrib1svNV@8))
GL_PREFIX(VertexAttrib1svNV,VertexAttrib1svNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib1svNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib2dNV,VertexAttrib2dNV@20))
GL_PREFIX(VertexAttrib2dNV,VertexAttrib2dNV@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib2dNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib2dvNV,VertexAttrib2dvNV@8))
GL_PREFIX(VertexAttrib2dvNV,VertexAttrib2dvNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib2dvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib2fNV,VertexAttrib2fNV@12))
GL_PREFIX(VertexAttrib2fNV,VertexAttrib2fNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib2fNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib2fvNV,VertexAttrib2fvNV@8))
GL_PREFIX(VertexAttrib2fvNV,VertexAttrib2fvNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib2fvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib2sNV,VertexAttrib2sNV@12))
GL_PREFIX(VertexAttrib2sNV,VertexAttrib2sNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib2sNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib2svNV,VertexAttrib2svNV@8))
GL_PREFIX(VertexAttrib2svNV,VertexAttrib2svNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib2svNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib3dNV,VertexAttrib3dNV@28))
GL_PREFIX(VertexAttrib3dNV,VertexAttrib3dNV@28):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib3dNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib3dvNV,VertexAttrib3dvNV@8))
GL_PREFIX(VertexAttrib3dvNV,VertexAttrib3dvNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib3dvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib3fNV,VertexAttrib3fNV@16))
GL_PREFIX(VertexAttrib3fNV,VertexAttrib3fNV@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib3fNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib3fvNV,VertexAttrib3fvNV@8))
GL_PREFIX(VertexAttrib3fvNV,VertexAttrib3fvNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib3fvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib3sNV,VertexAttrib3sNV@16))
GL_PREFIX(VertexAttrib3sNV,VertexAttrib3sNV@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib3sNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib3svNV,VertexAttrib3svNV@8))
GL_PREFIX(VertexAttrib3svNV,VertexAttrib3svNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib3svNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4dNV,VertexAttrib4dNV@36))
GL_PREFIX(VertexAttrib4dNV,VertexAttrib4dNV@36):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4dNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4dvNV,VertexAttrib4dvNV@8))
GL_PREFIX(VertexAttrib4dvNV,VertexAttrib4dvNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4dvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4fNV,VertexAttrib4fNV@20))
GL_PREFIX(VertexAttrib4fNV,VertexAttrib4fNV@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4fNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4fvNV,VertexAttrib4fvNV@8))
GL_PREFIX(VertexAttrib4fvNV,VertexAttrib4fvNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4fvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4sNV,VertexAttrib4sNV@20))
GL_PREFIX(VertexAttrib4sNV,VertexAttrib4sNV@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4sNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4svNV,VertexAttrib4svNV@8))
GL_PREFIX(VertexAttrib4svNV,VertexAttrib4svNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4svNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4ubNV,VertexAttrib4ubNV@20))
GL_PREFIX(VertexAttrib4ubNV,VertexAttrib4ubNV@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4ubNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4ubvNV,VertexAttrib4ubvNV@8))
GL_PREFIX(VertexAttrib4ubvNV,VertexAttrib4ubvNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4ubvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribs1dvNV,VertexAttribs1dvNV@12))
GL_PREFIX(VertexAttribs1dvNV,VertexAttribs1dvNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribs1dvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribs1fvNV,VertexAttribs1fvNV@12))
GL_PREFIX(VertexAttribs1fvNV,VertexAttribs1fvNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribs1fvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribs1svNV,VertexAttribs1svNV@12))
GL_PREFIX(VertexAttribs1svNV,VertexAttribs1svNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribs1svNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribs2dvNV,VertexAttribs2dvNV@12))
GL_PREFIX(VertexAttribs2dvNV,VertexAttribs2dvNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribs2dvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribs2fvNV,VertexAttribs2fvNV@12))
GL_PREFIX(VertexAttribs2fvNV,VertexAttribs2fvNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribs2fvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribs2svNV,VertexAttribs2svNV@12))
GL_PREFIX(VertexAttribs2svNV,VertexAttribs2svNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribs2svNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribs3dvNV,VertexAttribs3dvNV@12))
GL_PREFIX(VertexAttribs3dvNV,VertexAttribs3dvNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribs3dvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribs3fvNV,VertexAttribs3fvNV@12))
GL_PREFIX(VertexAttribs3fvNV,VertexAttribs3fvNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribs3fvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribs3svNV,VertexAttribs3svNV@12))
GL_PREFIX(VertexAttribs3svNV,VertexAttribs3svNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribs3svNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribs4dvNV,VertexAttribs4dvNV@12))
GL_PREFIX(VertexAttribs4dvNV,VertexAttribs4dvNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribs4dvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribs4fvNV,VertexAttribs4fvNV@12))
GL_PREFIX(VertexAttribs4fvNV,VertexAttribs4fvNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribs4fvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribs4svNV,VertexAttribs4svNV@12))
GL_PREFIX(VertexAttribs4svNV,VertexAttribs4svNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribs4svNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribs4ubvNV,VertexAttribs4ubvNV@12))
GL_PREFIX(VertexAttribs4ubvNV,VertexAttribs4ubvNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribs4ubvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointParameteriNV,PointParameteriNV@8))
GL_PREFIX(PointParameteriNV,PointParameteriNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PointParameteriNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointParameterivNV,PointParameterivNV@8))
GL_PREFIX(PointParameterivNV,PointParameterivNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PointParameterivNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BlendFuncSeparate,BlendFuncSeparate@16))
GL_PREFIX(BlendFuncSeparate,BlendFuncSeparate@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BlendFuncSeparateEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FogCoordf,FogCoordf@4))
GL_PREFIX(FogCoordf,FogCoordf@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FogCoordfEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FogCoordfv,FogCoordfv@4))
GL_PREFIX(FogCoordfv,FogCoordfv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FogCoordfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FogCoordd,FogCoordd@8))
GL_PREFIX(FogCoordd,FogCoordd@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FogCoorddEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FogCoorddv,FogCoorddv@4))
GL_PREFIX(FogCoorddv,FogCoorddv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FogCoorddvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FogCoordPointer,FogCoordPointer@12))
GL_PREFIX(FogCoordPointer,FogCoordPointer@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FogCoordPointerEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiDrawArrays,MultiDrawArrays@16))
GL_PREFIX(MultiDrawArrays,MultiDrawArrays@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiDrawArraysEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MultiDrawElements,MultiDrawElements@20))
GL_PREFIX(MultiDrawElements,MultiDrawElements@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MultiDrawElementsEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointParameterf,PointParameterf@8))
GL_PREFIX(PointParameterf,PointParameterf@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PointParameterfEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointParameterfv,PointParameterfv@8))
GL_PREFIX(PointParameterfv,PointParameterfv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PointParameterfvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointParameteri,PointParameteri@8))
GL_PREFIX(PointParameteri,PointParameteri@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PointParameteriNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(PointParameteriv,PointParameteriv@8))
GL_PREFIX(PointParameteriv,PointParameteriv@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_PointParameterivNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3b,SecondaryColor3b@12))
GL_PREFIX(SecondaryColor3b,SecondaryColor3b@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3bEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3bv,SecondaryColor3bv@4))
GL_PREFIX(SecondaryColor3bv,SecondaryColor3bv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3bvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3d,SecondaryColor3d@24))
GL_PREFIX(SecondaryColor3d,SecondaryColor3d@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3dEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3dv,SecondaryColor3dv@4))
GL_PREFIX(SecondaryColor3dv,SecondaryColor3dv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3dvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3f,SecondaryColor3f@12))
GL_PREFIX(SecondaryColor3f,SecondaryColor3f@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3fEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3fv,SecondaryColor3fv@4))
GL_PREFIX(SecondaryColor3fv,SecondaryColor3fv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3fvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3i,SecondaryColor3i@12))
GL_PREFIX(SecondaryColor3i,SecondaryColor3i@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3iEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3iv,SecondaryColor3iv@4))
GL_PREFIX(SecondaryColor3iv,SecondaryColor3iv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3ivEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3s,SecondaryColor3s@12))
GL_PREFIX(SecondaryColor3s,SecondaryColor3s@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3sEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3sv,SecondaryColor3sv@4))
GL_PREFIX(SecondaryColor3sv,SecondaryColor3sv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3svEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3ub,SecondaryColor3ub@12))
GL_PREFIX(SecondaryColor3ub,SecondaryColor3ub@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3ubEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3ubv,SecondaryColor3ubv@4))
GL_PREFIX(SecondaryColor3ubv,SecondaryColor3ubv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3ubvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3ui,SecondaryColor3ui@12))
GL_PREFIX(SecondaryColor3ui,SecondaryColor3ui@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3uiEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3uiv,SecondaryColor3uiv@4))
GL_PREFIX(SecondaryColor3uiv,SecondaryColor3uiv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3uivEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3us,SecondaryColor3us@12))
GL_PREFIX(SecondaryColor3us,SecondaryColor3us@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3usEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColor3usv,SecondaryColor3usv@4))
GL_PREFIX(SecondaryColor3usv,SecondaryColor3usv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColor3usvEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SecondaryColorPointer,SecondaryColorPointer@16))
GL_PREFIX(SecondaryColorPointer,SecondaryColorPointer@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SecondaryColorPointerEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2d,WindowPos2d@16))
GL_PREFIX(WindowPos2d,WindowPos2d@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2dMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2dv,WindowPos2dv@4))
GL_PREFIX(WindowPos2dv,WindowPos2dv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2dvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2f,WindowPos2f@8))
GL_PREFIX(WindowPos2f,WindowPos2f@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2fMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2fv,WindowPos2fv@4))
GL_PREFIX(WindowPos2fv,WindowPos2fv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2fvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2i,WindowPos2i@8))
GL_PREFIX(WindowPos2i,WindowPos2i@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2iMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2iv,WindowPos2iv@4))
GL_PREFIX(WindowPos2iv,WindowPos2iv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2ivMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2s,WindowPos2s@8))
GL_PREFIX(WindowPos2s,WindowPos2s@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2sMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos2sv,WindowPos2sv@4))
GL_PREFIX(WindowPos2sv,WindowPos2sv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos2svMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3d,WindowPos3d@24))
GL_PREFIX(WindowPos3d,WindowPos3d@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3dMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3dv,WindowPos3dv@4))
GL_PREFIX(WindowPos3dv,WindowPos3dv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3dvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3f,WindowPos3f@12))
GL_PREFIX(WindowPos3f,WindowPos3f@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3fMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3fv,WindowPos3fv@4))
GL_PREFIX(WindowPos3fv,WindowPos3fv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3fvMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3i,WindowPos3i@12))
GL_PREFIX(WindowPos3i,WindowPos3i@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3iMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3iv,WindowPos3iv@4))
GL_PREFIX(WindowPos3iv,WindowPos3iv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3ivMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3s,WindowPos3s@12))
GL_PREFIX(WindowPos3s,WindowPos3s@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3sMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(WindowPos3sv,WindowPos3sv@4))
GL_PREFIX(WindowPos3sv,WindowPos3sv@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_WindowPos3svMESA))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ActiveStencilFaceEXT,ActiveStencilFaceEXT@4))
GL_PREFIX(ActiveStencilFaceEXT,ActiveStencilFaceEXT@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ActiveStencilFaceEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DeleteFencesNV,DeleteFencesNV@8))
GL_PREFIX(DeleteFencesNV,DeleteFencesNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DeleteFencesNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GenFencesNV,GenFencesNV@8))
GL_PREFIX(GenFencesNV,GenFencesNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GenFencesNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IsFenceNV,IsFenceNV@4))
GL_PREFIX(IsFenceNV,IsFenceNV@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_IsFenceNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(TestFenceNV,TestFenceNV@4))
GL_PREFIX(TestFenceNV,TestFenceNV@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_TestFenceNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetFenceivNV,GetFenceivNV@12))
GL_PREFIX(GetFenceivNV,GetFenceivNV@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetFenceivNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(FinishFenceNV,FinishFenceNV@4))
GL_PREFIX(FinishFenceNV,FinishFenceNV@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_FinishFenceNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(SetFenceNV,SetFenceNV@8))
GL_PREFIX(SetFenceNV,SetFenceNV@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_SetFenceNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib1sARB,VertexAttrib1sARB@8))
GL_PREFIX(VertexAttrib1sARB,VertexAttrib1sARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib1sNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib1fARB,VertexAttrib1fARB@8))
GL_PREFIX(VertexAttrib1fARB,VertexAttrib1fARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib1fNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib1dARB,VertexAttrib1dARB@12))
GL_PREFIX(VertexAttrib1dARB,VertexAttrib1dARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib1dNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib2sARB,VertexAttrib2sARB@12))
GL_PREFIX(VertexAttrib2sARB,VertexAttrib2sARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib2sNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib2fARB,VertexAttrib2fARB@12))
GL_PREFIX(VertexAttrib2fARB,VertexAttrib2fARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib2fNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib2dARB,VertexAttrib2dARB@20))
GL_PREFIX(VertexAttrib2dARB,VertexAttrib2dARB@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib2dNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib3sARB,VertexAttrib3sARB@16))
GL_PREFIX(VertexAttrib3sARB,VertexAttrib3sARB@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib3sNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib3fARB,VertexAttrib3fARB@16))
GL_PREFIX(VertexAttrib3fARB,VertexAttrib3fARB@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib3fNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib3dARB,VertexAttrib3dARB@28))
GL_PREFIX(VertexAttrib3dARB,VertexAttrib3dARB@28):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib3dNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4sARB,VertexAttrib4sARB@20))
GL_PREFIX(VertexAttrib4sARB,VertexAttrib4sARB@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4sNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4fARB,VertexAttrib4fARB@20))
GL_PREFIX(VertexAttrib4fARB,VertexAttrib4fARB@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4fNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4dARB,VertexAttrib4dARB@36))
GL_PREFIX(VertexAttrib4dARB,VertexAttrib4dARB@36):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4dNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4NubARB,VertexAttrib4NubARB@20))
GL_PREFIX(VertexAttrib4NubARB,VertexAttrib4NubARB@20):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4ubNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib1svARB,VertexAttrib1svARB@8))
GL_PREFIX(VertexAttrib1svARB,VertexAttrib1svARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib1svNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib1fvARB,VertexAttrib1fvARB@8))
GL_PREFIX(VertexAttrib1fvARB,VertexAttrib1fvARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib1fvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib1dvARB,VertexAttrib1dvARB@8))
GL_PREFIX(VertexAttrib1dvARB,VertexAttrib1dvARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib1dvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib2svARB,VertexAttrib2svARB@8))
GL_PREFIX(VertexAttrib2svARB,VertexAttrib2svARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib2svNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib2fvARB,VertexAttrib2fvARB@8))
GL_PREFIX(VertexAttrib2fvARB,VertexAttrib2fvARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib2fvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib2dvARB,VertexAttrib2dvARB@8))
GL_PREFIX(VertexAttrib2dvARB,VertexAttrib2dvARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib2dvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib3svARB,VertexAttrib3svARB@8))
GL_PREFIX(VertexAttrib3svARB,VertexAttrib3svARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib3svNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib3fvARB,VertexAttrib3fvARB@8))
GL_PREFIX(VertexAttrib3fvARB,VertexAttrib3fvARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib3fvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib3dvARB,VertexAttrib3dvARB@8))
GL_PREFIX(VertexAttrib3dvARB,VertexAttrib3dvARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib3dvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4bvARB,VertexAttrib4bvARB@8))
GL_PREFIX(VertexAttrib4bvARB,VertexAttrib4bvARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4bvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4svARB,VertexAttrib4svARB@8))
GL_PREFIX(VertexAttrib4svARB,VertexAttrib4svARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4svNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4ivARB,VertexAttrib4ivARB@8))
GL_PREFIX(VertexAttrib4ivARB,VertexAttrib4ivARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4ivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4ubvARB,VertexAttrib4ubvARB@8))
GL_PREFIX(VertexAttrib4ubvARB,VertexAttrib4ubvARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4ubvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4usvARB,VertexAttrib4usvARB@8))
GL_PREFIX(VertexAttrib4usvARB,VertexAttrib4usvARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4usvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4uivARB,VertexAttrib4uivARB@8))
GL_PREFIX(VertexAttrib4uivARB,VertexAttrib4uivARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4uivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4fvARB,VertexAttrib4fvARB@8))
GL_PREFIX(VertexAttrib4fvARB,VertexAttrib4fvARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4fvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4dvARB,VertexAttrib4dvARB@8))
GL_PREFIX(VertexAttrib4dvARB,VertexAttrib4dvARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4dvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4NbvARB,VertexAttrib4NbvARB@8))
GL_PREFIX(VertexAttrib4NbvARB,VertexAttrib4NbvARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4NbvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4NsvARB,VertexAttrib4NsvARB@8))
GL_PREFIX(VertexAttrib4NsvARB,VertexAttrib4NsvARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4NsvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4NivARB,VertexAttrib4NivARB@8))
GL_PREFIX(VertexAttrib4NivARB,VertexAttrib4NivARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4NivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4NubvARB,VertexAttrib4NubvARB@8))
GL_PREFIX(VertexAttrib4NubvARB,VertexAttrib4NubvARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4ubvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4NusvARB,VertexAttrib4NusvARB@8))
GL_PREFIX(VertexAttrib4NusvARB,VertexAttrib4NusvARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4NusvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttrib4NuivARB,VertexAttrib4NuivARB@8))
GL_PREFIX(VertexAttrib4NuivARB,VertexAttrib4NuivARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttrib4NuivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(VertexAttribPointerARB,VertexAttribPointerARB@24))
GL_PREFIX(VertexAttribPointerARB,VertexAttribPointerARB@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_VertexAttribPointerARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EnableVertexAttribArrayARB,EnableVertexAttribArrayARB@4))
GL_PREFIX(EnableVertexAttribArrayARB,EnableVertexAttribArrayARB@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EnableVertexAttribArrayARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DisableVertexAttribArrayARB,DisableVertexAttribArrayARB@4))
GL_PREFIX(DisableVertexAttribArrayARB,DisableVertexAttribArrayARB@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DisableVertexAttribArrayARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ProgramStringARB,ProgramStringARB@16))
GL_PREFIX(ProgramStringARB,ProgramStringARB@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ProgramStringARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BindProgramARB,BindProgramARB@8))
GL_PREFIX(BindProgramARB,BindProgramARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BindProgramNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DeleteProgramsARB,DeleteProgramsARB@8))
GL_PREFIX(DeleteProgramsARB,DeleteProgramsARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DeleteProgramsNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GenProgramsARB,GenProgramsARB@8))
GL_PREFIX(GenProgramsARB,GenProgramsARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GenProgramsNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IsProgramARB,IsProgramARB@4))
GL_PREFIX(IsProgramARB,IsProgramARB@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_IsProgramNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ProgramEnvParameter4dARB,ProgramEnvParameter4dARB@40))
GL_PREFIX(ProgramEnvParameter4dARB,ProgramEnvParameter4dARB@40):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ProgramEnvParameter4dARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ProgramEnvParameter4dvARB,ProgramEnvParameter4dvARB@12))
GL_PREFIX(ProgramEnvParameter4dvARB,ProgramEnvParameter4dvARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ProgramEnvParameter4dvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ProgramEnvParameter4fARB,ProgramEnvParameter4fARB@24))
GL_PREFIX(ProgramEnvParameter4fARB,ProgramEnvParameter4fARB@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ProgramEnvParameter4fARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ProgramEnvParameter4fvARB,ProgramEnvParameter4fvARB@12))
GL_PREFIX(ProgramEnvParameter4fvARB,ProgramEnvParameter4fvARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ProgramEnvParameter4fvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ProgramLocalParameter4dARB,ProgramLocalParameter4dARB@40))
GL_PREFIX(ProgramLocalParameter4dARB,ProgramLocalParameter4dARB@40):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ProgramLocalParameter4dARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ProgramLocalParameter4dvARB,ProgramLocalParameter4dvARB@12))
GL_PREFIX(ProgramLocalParameter4dvARB,ProgramLocalParameter4dvARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ProgramLocalParameter4dvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ProgramLocalParameter4fARB,ProgramLocalParameter4fARB@24))
GL_PREFIX(ProgramLocalParameter4fARB,ProgramLocalParameter4fARB@24):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ProgramLocalParameter4fARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ProgramLocalParameter4fvARB,ProgramLocalParameter4fvARB@12))
GL_PREFIX(ProgramLocalParameter4fvARB,ProgramLocalParameter4fvARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ProgramLocalParameter4fvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetProgramEnvParameterdvARB,GetProgramEnvParameterdvARB@12))
GL_PREFIX(GetProgramEnvParameterdvARB,GetProgramEnvParameterdvARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetProgramEnvParameterdvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetProgramEnvParameterfvARB,GetProgramEnvParameterfvARB@12))
GL_PREFIX(GetProgramEnvParameterfvARB,GetProgramEnvParameterfvARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetProgramEnvParameterfvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetProgramLocalParameterdvARB,GetProgramLocalParameterdvARB@12))
GL_PREFIX(GetProgramLocalParameterdvARB,GetProgramLocalParameterdvARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetProgramLocalParameterdvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetProgramLocalParameterfvARB,GetProgramLocalParameterfvARB@12))
GL_PREFIX(GetProgramLocalParameterfvARB,GetProgramLocalParameterfvARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetProgramLocalParameterfvARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetProgramivARB,GetProgramivARB@12))
GL_PREFIX(GetProgramivARB,GetProgramivARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetProgramivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetProgramStringARB,GetProgramStringARB@12))
GL_PREFIX(GetProgramStringARB,GetProgramStringARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetProgramStringARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetVertexAttribdvARB,GetVertexAttribdvARB@12))
GL_PREFIX(GetVertexAttribdvARB,GetVertexAttribdvARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetVertexAttribdvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetVertexAttribfvARB,GetVertexAttribfvARB@12))
GL_PREFIX(GetVertexAttribfvARB,GetVertexAttribfvARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetVertexAttribfvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetVertexAttribivARB,GetVertexAttribivARB@12))
GL_PREFIX(GetVertexAttribivARB,GetVertexAttribivARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetVertexAttribivNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetVertexAttribPointervARB,GetVertexAttribPointervARB@12))
GL_PREFIX(GetVertexAttribPointervARB,GetVertexAttribPointervARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetVertexAttribPointervNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ProgramNamedParameter4fNV,ProgramNamedParameter4fNV@28))
GL_PREFIX(ProgramNamedParameter4fNV,ProgramNamedParameter4fNV@28):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ProgramNamedParameter4fNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ProgramNamedParameter4dNV,ProgramNamedParameter4dNV@44))
GL_PREFIX(ProgramNamedParameter4dNV,ProgramNamedParameter4dNV@44):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ProgramNamedParameter4dNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ProgramNamedParameter4fvNV,ProgramNamedParameter4fvNV@16))
GL_PREFIX(ProgramNamedParameter4fvNV,ProgramNamedParameter4fvNV@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ProgramNamedParameter4fvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(ProgramNamedParameter4dvNV,ProgramNamedParameter4dvNV@16))
GL_PREFIX(ProgramNamedParameter4dvNV,ProgramNamedParameter4dvNV@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_ProgramNamedParameter4dvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetProgramNamedParameterfvNV,GetProgramNamedParameterfvNV@16))
GL_PREFIX(GetProgramNamedParameterfvNV,GetProgramNamedParameterfvNV@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetProgramNamedParameterfvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetProgramNamedParameterdvNV,GetProgramNamedParameterdvNV@16))
GL_PREFIX(GetProgramNamedParameterdvNV,GetProgramNamedParameterdvNV@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetProgramNamedParameterdvNV))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BindBufferARB,BindBufferARB@8))
GL_PREFIX(BindBufferARB,BindBufferARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BindBufferARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BufferDataARB,BufferDataARB@16))
GL_PREFIX(BufferDataARB,BufferDataARB@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BufferDataARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BufferSubDataARB,BufferSubDataARB@16))
GL_PREFIX(BufferSubDataARB,BufferSubDataARB@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BufferSubDataARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DeleteBuffersARB,DeleteBuffersARB@8))
GL_PREFIX(DeleteBuffersARB,DeleteBuffersARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DeleteBuffersARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GenBuffersARB,GenBuffersARB@8))
GL_PREFIX(GenBuffersARB,GenBuffersARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GenBuffersARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetBufferParameterivARB,GetBufferParameterivARB@12))
GL_PREFIX(GetBufferParameterivARB,GetBufferParameterivARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetBufferParameterivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetBufferPointervARB,GetBufferPointervARB@12))
GL_PREFIX(GetBufferPointervARB,GetBufferPointervARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetBufferPointervARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetBufferSubDataARB,GetBufferSubDataARB@16))
GL_PREFIX(GetBufferSubDataARB,GetBufferSubDataARB@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetBufferSubDataARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IsBufferARB,IsBufferARB@4))
GL_PREFIX(IsBufferARB,IsBufferARB@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_IsBufferARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MapBufferARB,MapBufferARB@8))
GL_PREFIX(MapBufferARB,MapBufferARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MapBufferARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(UnmapBufferARB,UnmapBufferARB@4))
GL_PREFIX(UnmapBufferARB,UnmapBufferARB@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_UnmapBufferARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DepthBoundsEXT,DepthBoundsEXT@16))
GL_PREFIX(DepthBoundsEXT,DepthBoundsEXT@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DepthBoundsEXT))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GenQueriesARB,GenQueriesARB@8))
GL_PREFIX(GenQueriesARB,GenQueriesARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GenQueriesARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DeleteQueriesARB,DeleteQueriesARB@8))
GL_PREFIX(DeleteQueriesARB,DeleteQueriesARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DeleteQueriesARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IsQueryARB,IsQueryARB@4))
GL_PREFIX(IsQueryARB,IsQueryARB@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_IsQueryARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BeginQueryARB,BeginQueryARB@8))
GL_PREFIX(BeginQueryARB,BeginQueryARB@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BeginQueryARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EndQueryARB,EndQueryARB@4))
GL_PREFIX(EndQueryARB,EndQueryARB@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EndQueryARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetQueryivARB,GetQueryivARB@12))
GL_PREFIX(GetQueryivARB,GetQueryivARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetQueryivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetQueryObjectivARB,GetQueryObjectivARB@12))
GL_PREFIX(GetQueryObjectivARB,GetQueryObjectivARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetQueryObjectivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetQueryObjectuivARB,GetQueryObjectuivARB@12))
GL_PREFIX(GetQueryObjectuivARB,GetQueryObjectuivARB@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetQueryObjectuivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BindBuffer,BindBuffer@8))
GL_PREFIX(BindBuffer,BindBuffer@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BindBufferARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BufferData,BufferData@16))
GL_PREFIX(BufferData,BufferData@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BufferDataARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BufferSubData,BufferSubData@16))
GL_PREFIX(BufferSubData,BufferSubData@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BufferSubDataARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DeleteBuffers,DeleteBuffers@8))
GL_PREFIX(DeleteBuffers,DeleteBuffers@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DeleteBuffersARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GenBuffers,GenBuffers@8))
GL_PREFIX(GenBuffers,GenBuffers@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GenBuffersARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetBufferParameteriv,GetBufferParameteriv@12))
GL_PREFIX(GetBufferParameteriv,GetBufferParameteriv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetBufferParameterivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetBufferPointerv,GetBufferPointerv@12))
GL_PREFIX(GetBufferPointerv,GetBufferPointerv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetBufferPointervARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetBufferSubData,GetBufferSubData@16))
GL_PREFIX(GetBufferSubData,GetBufferSubData@16):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetBufferSubDataARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IsBuffer,IsBuffer@4))
GL_PREFIX(IsBuffer,IsBuffer@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_IsBufferARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(MapBuffer,MapBuffer@8))
GL_PREFIX(MapBuffer,MapBuffer@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_MapBufferARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(UnmapBuffer,UnmapBuffer@4))
GL_PREFIX(UnmapBuffer,UnmapBuffer@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_UnmapBufferARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GenQueries,GenQueries@8))
GL_PREFIX(GenQueries,GenQueries@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GenQueriesARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(DeleteQueries,DeleteQueries@8))
GL_PREFIX(DeleteQueries,DeleteQueries@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_DeleteQueriesARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(IsQuery,IsQuery@4))
GL_PREFIX(IsQuery,IsQuery@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_IsQueryARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(BeginQuery,BeginQuery@8))
GL_PREFIX(BeginQuery,BeginQuery@8):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_BeginQueryARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(EndQuery,EndQuery@4))
GL_PREFIX(EndQuery,EndQuery@4):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_EndQueryARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetQueryiv,GetQueryiv@12))
GL_PREFIX(GetQueryiv,GetQueryiv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetQueryivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetQueryObjectiv,GetQueryObjectiv@12))
GL_PREFIX(GetQueryObjectiv,GetQueryObjectiv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetQueryObjectivARB))

ALIGNTEXT16
GLOBL_FN(GL_PREFIX(GetQueryObjectuiv,GetQueryObjectuiv@12))
GL_PREFIX(GetQueryObjectuiv,GetQueryObjectuiv@12):
	MOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)
	JMP(GL_OFFSET(_gloffset_GetQueryObjectuivARB))


#endif  /* __WIN32__ */
