/* $Id: opengl32.h,v 1.4 2004/02/02 05:36:37 royce Exp $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/opengl32/opengl32.h
 * PURPOSE:              OpenGL32 lib
 * PROGRAMMER:           Royce Mitchell III, Anich Gregor (blight)
 * UPDATE HISTORY:
 *                       Feb 1, 2004: Created
 */

#ifndef OPENGL32_PRIVATE_H
#define OPENGL32_PRIVATE_H

/* debug flags */
#define DEBUG_OPENGL32
#define DEBUG_OPENGL32_ICD_EXPORTS	/* dumps the list of supported glXXX
                                       functions when an ICD is loaded. */

/* gl function list */
#define GLFUNCS_MACRO \
	X(glAccum) \
	X(glAddSwapHintRectWIN) \
	X(glAlphaFunc) \
	X(glAreTexturesResident) \
	X(glArrayElement) \
	X(glBegin) \
	X(glBindTexture) \
	X(glBitmap) \
	X(glBlendFunc) \
	X(glCallList) \
	X(glCallLists) \
	X(glClear) \
	X(glClearAccum) \
	X(glClearColor) \
	X(glClearDepth) \
	X(glClearIndex) \
	X(glClearStencil) \
	X(glClipPlane) \
	X(glColor3b) \
	X(glColor3bv) \
	X(glColor3d) \
	X(glColor3dv) \
	X(glColor3f) \
	X(glColor3fv) \
	X(glColor3i) \
	X(glColor3iv) \
	X(glColor3s) \
	X(glColor3sv) \
	X(glColor3ub) \
	X(glColor3ubv) \
	X(glColor3ui) \
	X(glColor3uiv) \
	X(glColor3us) \
	X(glColor3usv) \
	X(glColor4b) \
	X(glColor4bv) \
	X(glColor4d) \
	X(glColor4dv) \
	X(glColor4f) \
	X(glColor4fv) \
	X(glColor4i) \
	X(glColor4iv) \
	X(glColor4s) \
	X(glColor4sv) \
	X(glColor4ub) \
	X(glColor4ubv) \
	X(glColor4ui) \
	X(glColor4uiv) \
	X(glColor4us) \
	X(glColor4usv) \
	X(glColorMask) \
	X(glColorMaterial) \
	X(glColorPointer) \
	X(glCopyPixels) \
	X(glCopyTexImage1D) \
	X(glCopyTexImage2D) \
	X(glCopyTexSubImage1D) \
	X(glCopyTexSubImage2D) \
	X(glCullFace) \
	X(glDebugEntry) \
	X(glDeleteLists) \
	X(glDeleteTextures) \
	X(glDepthFunc) \
	X(glDepthMask) \
	X(glDepthRange) \
	X(glDisable) \
	X(glDisableClientState) \
	X(glDrawArrays) \
	X(glDrawBuffer) \
	X(glDrawElements) \
	X(glDrawPixels) \
	X(glEdgeFlag) \
	X(glEdgeFlagPointer) \
	X(glEdgeFlagv) \
	X(glEnable) \
	X(glEnableClientState) \
	X(glEnd) \
	X(glEndList) \
	X(glEvalCoord1d) \
	X(glEvalCoord1dv) \
	X(glEvalCoord1f) \
	X(glEvalCoord1fv) \
	X(glEvalCoord2d) \
	X(glEvalCoord2dv) \
	X(glEvalCoord2f) \
	X(glEvalCoord2fv) \
	X(glEvalMesh1) \
	X(glEvalMesh2) \
	X(glEvalPoint1) \
	X(glEvalPoint2) \
	X(glFeedbackBuffer) \
	X(glFinish) \
	X(glFlush) \
	X(glFogf) \
	X(glFogfv) \
	X(glFogi) \
	X(glFogiv) \
	X(glFrontFace) \
	X(glFrustum) \
	X(glGenLists) \
	X(glGenTextures) \
	X(glGetBooleanv) \
	X(glGetClipPlane) \
	X(glGetDoublev) \
	X(glGetError) \
	X(glGetFloatv) \
	X(glGetIntegerv) \
	X(glGetLightfv) \
	X(glGetLightiv) \
	X(glGetMapdv) \
	X(glGetMapfv) \
	X(glGetMapiv) \
	X(glGetMaterialfv) \
	X(glGetMaterialiv) \
	X(glGetPixelMapfv) \
	X(glGetPixelMapuiv) \
	X(glGetPixelMapusv) \
	X(glGetPointerv) \
	X(glGetPolygonStipple) \
	X(glGetString) \
	X(glGetTexEnvfv) \
	X(glGetTexEnviv) \
	X(glGetTexGendv) \
	X(glGetTexGenfv) \
	X(glGetTexGeniv) \
	X(glGetTexImage) \
	X(glGetTexLevelParameterfv) \
	X(glGetTexLevelParameteriv) \
	X(glGetTexParameterfv) \
	X(glGetTexParameteriv) \
	X(glHint) \
	X(glIndexd) \
	X(glIndexdv) \
	X(glIndexf) \
	X(glIndexfv) \
	X(glIndexi) \
	X(glIndexiv) \
	X(glIndexMask) \
	X(glIndexPointer) \
	X(glIndexs) \
	X(glIndexsv) \
	X(glIndexub) \
	X(glIndexubv) \
	X(glInitNames) \
	X(glInterleavedArrays) \
	X(glIsEnabled) \
	X(glIsList) \
	X(glIsTexture) \
	X(glLightf) \
	X(glLightfv) \
	X(glLighti) \
	X(glLightiv) \
	X(glLightModelf) \
	X(glLightModelfv) \
	X(glLightModeli) \
	X(glLightModeliv) \
	X(glLineStipple) \
	X(glLineWidth) \
	X(glListBase) \
	X(glLoadIdentity) \
	X(glLoadMatrixd) \
	X(glLoadMatrixf) \
	X(glLoadName) \
	X(glLogicOp) \
	X(glMap1d) \
	X(glMap1f) \
	X(glMap2d) \
	X(glMap2f) \
	X(glMapGrid1d) \
	X(glMapGrid1f) \
	X(glMapGrid2d) \
	X(glMapGrid2f) \
	X(glMaterialf) \
	X(glMaterialfv) \
	X(glMateriali) \
	X(glMaterialiv) \
	X(glMatrixMode) \
	X(glMultMatrixd) \
	X(glMultMatrixf) \
	X(glNewList) \
	X(glNormal3b) \
	X(glNormal3bv) \
	X(glNormal3d) \
	X(glNormal3dv) \
	X(glNormal3f) \
	X(glNormal3fv) \
	X(glNormal3i) \
	X(glNormal3iv) \
	X(glNormal3s) \
	X(glNormal3sv) \
	X(glNormalPointer) \
	X(glOrtho) \
	X(glPassThrough) \
	X(glPixelMapfv) \
	X(glPixelMapuiv) \
	X(glPixelMapusv) \
	X(glPixelStoref) \
	X(glPixelStorei) \
	X(glPixelTransferf) \
	X(glPixelTransferi) \
	X(glPixelZoom) \
	X(glPointSize) \
	X(glPolygonMode) \
	X(glPolygonOffset) \
	X(glPolygonStipple) \
	X(glPopAttrib) \
	X(glPopClientAttrib) \
	X(glPopMatrix) \
	X(glPopName) \
	X(glPrioritizeTextures) \
	X(glPushAttrib) \
	X(glPushClientAttrib) \
	X(glPushMatrix) \
	X(glPushName) \
	X(glRasterPos2d) \
	X(glRasterPos2dv) \
	X(glRasterPos2f) \
	X(glRasterPos2fv) \
	X(glRasterPos2i) \
	X(glRasterPos2iv) \
	X(glRasterPos2s) \
	X(glRasterPos2sv) \
	X(glRasterPos3d) \
	X(glRasterPos3dv) \
	X(glRasterPos3f) \
	X(glRasterPos3fv) \
	X(glRasterPos3i) \
	X(glRasterPos3iv) \
	X(glRasterPos3s) \
	X(glRasterPos3sv) \
	X(glRasterPos4d) \
	X(glRasterPos4dv) \
	X(glRasterPos4f) \
	X(glRasterPos4fv) \
	X(glRasterPos4i) \
	X(glRasterPos4iv) \
	X(glRasterPos4s) \
	X(glRasterPos4sv) \
	X(glReadBuffer) \
	X(glReadPixels) \
	X(glRectd) \
	X(glRectdv) \
	X(glRectf) \
	X(glRectfv) \
	X(glRecti) \
	X(glRectiv) \
	X(glRects) \
	X(glRectsv) \
	X(glRenderMode) \
	X(glRotated) \
	X(glRotatef) \
	X(glScaled) \
	X(glScalef) \
	X(glScissor) \
	X(glSelectBuffer) \
	X(glShadeModel) \
	X(glStencilFunc) \
	X(glStencilMask) \
	X(glStencilOp) \
	X(glTexCoord1d) \
	X(glTexCoord1dv) \
	X(glTexCoord1f) \
	X(glTexCoord1fv) \
	X(glTexCoord1i) \
	X(glTexCoord1iv) \
	X(glTexCoord1s) \
	X(glTexCoord1sv) \
	X(glTexCoord2d) \
	X(glTexCoord2dv) \
	X(glTexCoord2f) \
	X(glTexCoord2fv) \
	X(glTexCoord2i) \
	X(glTexCoord2iv) \
	X(glTexCoord2s) \
	X(glTexCoord2sv) \
	X(glTexCoord3d) \
	X(glTexCoord3dv) \
	X(glTexCoord3f) \
	X(glTexCoord3fv) \
	X(glTexCoord3i) \
	X(glTexCoord3iv) \
	X(glTexCoord3s) \
	X(glTexCoord3sv) \
	X(glTexCoord4d) \
	X(glTexCoord4dv) \
	X(glTexCoord4f) \
	X(glTexCoord4fv) \
	X(glTexCoord4i) \
	X(glTexCoord4iv) \
	X(glTexCoord4s) \
	X(glTexCoord4sv) \
	X(glTexCoordPointer) \
	X(glTexEnvf) \
	X(glTexEnvfv) \
	X(glTexEnvi) \
	X(glTexEnviv) \
	X(glTexGend) \
	X(glTexGendv) \
	X(glTexGenf) \
	X(glTexGenfv) \
	X(glTexGeni) \
	X(glTexGeniv) \
	X(glTexImage1D) \
	X(glTexImage2D) \
	X(glTexParameterf) \
	X(glTexParameterfv) \
	X(glTexParameteri) \
	X(glTexParameteriv) \
	X(glTexSubImage1D) \
	X(glTexSubImage2D) \
	X(glTranslated) \
	X(glTranslatef) \
	X(glVertex2d) \
	X(glVertex2dv) \
	X(glVertex2f) \
	X(glVertex2fv) \
	X(glVertex2i) \
	X(glVertex2iv) \
	X(glVertex2s) \
	X(glVertex2sv) \
	X(glVertex3d) \
	X(glVertex3dv) \
	X(glVertex3f) \
	X(glVertex3fv) \
	X(glVertex3i) \
	X(glVertex3iv) \
	X(glVertex3s) \
	X(glVertex3sv) \
	X(glVertex4d) \
	X(glVertex4dv) \
	X(glVertex4f) \
	X(glVertex4fv) \
	X(glVertex4i) \
	X(glVertex4iv) \
	X(glVertex4s) \
	X(glVertex4sv) \
	X(glVertexPointer) \
	X(glViewport)

/* table indices for funcnames and function pointers */
enum glfunc_indices
{
	GLIDX_INVALID = -1,
#define X(X) GLIDX_##X,
	GLFUNCS_MACRO
#undef X
	GLIDX_COUNT
};

/* function name table */
extern const char* OPENGL32_funcnames[GLIDX_COUNT];

/* FIXME: what type of argument does this take? */
typedef DWORD (CALLBACK * SetContextCallBack) (void *);

/* OpenGL ICD data */
typedef struct tagGLDRIVERDATA
{
	HMODULE handle;                 /* DLL handle */
	UINT    refcount;               /* number of references to this ICD */
	WCHAR   driver_name[256];       /* name of display driver */

	WCHAR   dll[256];               /* Dll value from registry */
	DWORD   version;                /* Version value from registry */
	DWORD   driver_version;         /* DriverVersion value from registry */
	DWORD   flags;                  /* Flags value from registry */

	BOOL    (*DrvCopyContext)( HGLRC, HGLRC, UINT );
	HGLRC   (*DrvCreateContext)( HDC );
	HGLRC   (*DrvCreateLayerContext)( HDC, int );
	BOOL    (*DrvDeleteContext)( HGLRC );
	BOOL    (*DrvDescribeLayerPlane)( HDC, int, int, UINT, LPLAYERPLANEDESCRIPTOR );
	int     (*DrvDescribePixelFormat)( IN HDC, IN int, IN UINT, OUT LPPIXELFORMATDESCRIPTOR );
	int     (*DrvGetLayerPaletteEntries)( HDC, int, int, int, COLORREF * )
	FARPROC (*DrvGetProcAddress)( LPCSTR lpProcName );
	void    (*DrvReleaseContext)();
	BOOL    (*DrvRealizeLayerPalette)( HDC, int, BOOL )
	int     (*DrvSetContext)( HDC hdc, HGLRC hglrc, SetContextCallBack callback )
	int     (*DrvSetLayerPaletteEntries)( HDC, int, int, int, CONST COLORREF * )
	BOOL    (*DrvSetPixelFormat)( IN HDC, IN int, IN CONST PIXELFORMATDESCRIPTOR * )
	BOOL    (*DrvShareLists)( HGLRC, HGLRC )
	BOOL    (*DrvSwapBuffers)( HDC )
	BOOL    (*DrvSwapLayerBuffers)( HDC, UINT )
	void    (*DrvValidateVersion)();

	PVOID   func_list[GLIDX_COUNT]; /* glXXX functions supported by ICD */

	struct tagGLDRIVERDATA *next;   /* next ICD -- linked list */
} GLDRIVERDATA;

/* OpenGL context */
typedef struct tagGLRC
{
	GLDRIVERDATA *icd;  /* driver used for this context */
	INT     iFormat;    /* current pixel format index - ? */
	HDC     hdc;        /* DC handle */
	DWORD   threadid;   /* thread holding this context */

	HGLRC   hglrc;      /* GLRC from DrvCreateContext */
	PVOID   func_list[GLIDX_COUNT];  /* glXXX function pointers */

	struct tagGLRC *next; /* linked list */
} GLRC;

/* Process data */
typedef struct tagGLPROCESSDATA
{
	GLDRIVERDATA *driver_list;  /* list of loaded drivers */
	GLRC *glrc_list;            /* list of GL rendering contexts */
} GLPROCESSDATA;

/* TLS data */
typedef struct tagGLTHREADDATA
{
	GLRC   *hglrc;      /* current GL rendering context */
} GLTHREADDATA;

extern DWORD OPENGL32_tls;
extern GLPROCESSDATA OPENGL32_processdata;

/* function prototypes */
GLDRIVERDATA *OPENGL32_LoadICDW( LPCWSTR driver );
BOOL OPENGL32_UnloadICD( GLDRIVERDATA *icd );

#endif//OPENGL32_PRIVATE_H
