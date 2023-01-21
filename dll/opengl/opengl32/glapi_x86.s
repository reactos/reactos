/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 dll/opengl/opengl32/glapi_x86.s
 * PURPOSE:              OpenGL32 DLL
 */

/* X86 opengl API entry points, fast forward to the current thread's dispatch table */
#include <asm.inc>
#include <ks386.inc>

.data
ASSUME nothing

.align 16

.code

MACRO(USE_GL_FUNC, name, offset, stack)
PUBLIC _gl&name&@&stack
.PROC _gl&name&@&stack

    FPO 0, 0, 0, 0, 0, FRAME_FPO

    /* Get the TEB */
    mov eax, fs:[TEB_SELF]
    /* Get the GL table */
    mov eax, [eax + TEB_GL_TABLE]

    /* If we don't have a dispatch table, this is a nop */
    test eax, eax
    jz name&_fast_ret
    /* Jump into the ICD */
    jmp dword ptr [eax+4*VAL(offset)]
name&_fast_ret:
    ret VAL(stack)
.ENDP
ENDM

USE_GL_FUNC Accum, 213, 8
USE_GL_FUNC AlphaFunc, 240, 8
USE_GL_FUNC AreTexturesResident, 322, 12
USE_GL_FUNC ArrayElement, 306, 4
USE_GL_FUNC Begin, 7, 4
USE_GL_FUNC BindTexture, 307, 8
USE_GL_FUNC Bitmap, 8, 28
USE_GL_FUNC BlendFunc, 241, 8
USE_GL_FUNC CallList, 2, 4
USE_GL_FUNC CallLists, 3, 12
USE_GL_FUNC Clear, 203, 4
USE_GL_FUNC ClearAccum, 204, 16
USE_GL_FUNC ClearColor, 206, 16
USE_GL_FUNC ClearDepth, 208, 8
USE_GL_FUNC ClearIndex, 205, 4
USE_GL_FUNC ClearStencil, 207, 4
USE_GL_FUNC ClipPlane, 150, 8
USE_GL_FUNC Color3b, 9, 12
USE_GL_FUNC Color3bv, 10, 4
USE_GL_FUNC Color3d, 11, 24
USE_GL_FUNC Color3dv, 12, 4
USE_GL_FUNC Color3f, 13, 12
USE_GL_FUNC Color3fv, 14, 4
USE_GL_FUNC Color3i, 15, 12
USE_GL_FUNC Color3iv, 16, 4
USE_GL_FUNC Color3s, 17, 12
USE_GL_FUNC Color3sv, 18, 4
USE_GL_FUNC Color3ub, 19, 12
USE_GL_FUNC Color3ubv, 20, 4
USE_GL_FUNC Color3ui, 21, 12
USE_GL_FUNC Color3uiv, 22, 4
USE_GL_FUNC Color3us, 23, 12
USE_GL_FUNC Color3usv, 24, 4
USE_GL_FUNC Color4b, 25, 16
USE_GL_FUNC Color4bv, 26, 4
USE_GL_FUNC Color4d, 27, 32
USE_GL_FUNC Color4dv, 28, 4
USE_GL_FUNC Color4f, 29, 16
USE_GL_FUNC Color4fv, 30, 4
USE_GL_FUNC Color4i, 31, 16
USE_GL_FUNC Color4iv, 32, 4
USE_GL_FUNC Color4s, 33, 16
USE_GL_FUNC Color4sv, 34, 4
USE_GL_FUNC Color4ub, 35, 16
USE_GL_FUNC Color4ubv, 36, 4
USE_GL_FUNC Color4ui, 37, 16
USE_GL_FUNC Color4uiv, 38, 4
USE_GL_FUNC Color4us, 39, 16
USE_GL_FUNC Color4usv, 40, 4
USE_GL_FUNC ColorMask, 210, 16
USE_GL_FUNC ColorMaterial, 151, 8
USE_GL_FUNC ColorPointer, 308, 16
USE_GL_FUNC CopyPixels, 255, 20
USE_GL_FUNC CopyTexImage1D, 323, 28
USE_GL_FUNC CopyTexImage2D, 324, 32
USE_GL_FUNC CopyTexSubImage1D, 325, 24
USE_GL_FUNC CopyTexSubImage2D, 326, 32
USE_GL_FUNC CullFace, 152, 4
USE_GL_FUNC DeleteLists, 4, 8
USE_GL_FUNC DeleteTextures, 327, 8
USE_GL_FUNC DepthFunc, 245, 4
USE_GL_FUNC DepthMask, 211, 4
USE_GL_FUNC DepthRange, 288, 16
USE_GL_FUNC Disable, 214, 4
USE_GL_FUNC DisableClientState, 309, 4
USE_GL_FUNC DrawArrays, 310, 12
USE_GL_FUNC DrawBuffer, 202, 4
USE_GL_FUNC DrawElements, 311, 16
USE_GL_FUNC DrawPixels, 257, 20
USE_GL_FUNC EdgeFlag, 41, 4
USE_GL_FUNC EdgeFlagPointer, 312, 8
USE_GL_FUNC EdgeFlagv, 42, 4
USE_GL_FUNC Enable, 215, 4
USE_GL_FUNC EnableClientState, 313, 4
USE_GL_FUNC End, 43, 0
USE_GL_FUNC EndList, 1, 0
USE_GL_FUNC EvalCoord1d, 228, 8
USE_GL_FUNC EvalCoord1dv, 229, 4
USE_GL_FUNC EvalCoord1f, 230, 4
USE_GL_FUNC EvalCoord1fv, 231, 4
USE_GL_FUNC EvalCoord2d, 232, 16
USE_GL_FUNC EvalCoord2dv, 233, 4
USE_GL_FUNC EvalCoord2f, 234, 8
USE_GL_FUNC EvalCoord2fv, 235, 4
USE_GL_FUNC EvalMesh1, 236, 12
USE_GL_FUNC EvalMesh2, 238, 20
USE_GL_FUNC EvalPoint1, 237, 4
USE_GL_FUNC EvalPoint2, 239, 8
USE_GL_FUNC FeedbackBuffer, 194, 12
USE_GL_FUNC Finish, 216, 0
USE_GL_FUNC Flush, 217, 0
USE_GL_FUNC Fogf, 153, 8
USE_GL_FUNC Fogfv, 154, 8
USE_GL_FUNC Fogi, 155, 8
USE_GL_FUNC Fogiv, 156, 8
USE_GL_FUNC FrontFace, 157, 4
USE_GL_FUNC Frustum, 289, 48
USE_GL_FUNC GenLists, 5, 4
USE_GL_FUNC GenTextures, 328, 8
USE_GL_FUNC GetBooleanv, 258, 8
USE_GL_FUNC GetClipPlane, 259, 8
USE_GL_FUNC GetDoublev, 260, 8
USE_GL_FUNC GetError, 261, 0
USE_GL_FUNC GetFloatv, 262, 8
USE_GL_FUNC GetIntegerv, 263, 8
USE_GL_FUNC GetLightfv, 264, 12
USE_GL_FUNC GetLightiv, 265, 12
USE_GL_FUNC GetMapdv, 266, 12
USE_GL_FUNC GetMapfv, 267, 12
USE_GL_FUNC GetMapiv, 268, 12
USE_GL_FUNC GetMaterialfv, 269, 12
USE_GL_FUNC GetMaterialiv, 270, 12
USE_GL_FUNC GetPixelMapfv, 271, 8
USE_GL_FUNC GetPixelMapuiv, 272, 8
USE_GL_FUNC GetPixelMapusv, 273, 8
USE_GL_FUNC GetPointerv, 329, 8
USE_GL_FUNC GetPolygonStipple, 274, 4
USE_GL_FUNC GetString, 275, 4
USE_GL_FUNC GetTexEnvfv, 276, 12
USE_GL_FUNC GetTexEnviv, 277, 12
USE_GL_FUNC GetTexGendv, 278, 12
USE_GL_FUNC GetTexGenfv, 279, 12
USE_GL_FUNC GetTexGeniv, 280, 12
USE_GL_FUNC GetTexImage, 281, 20
USE_GL_FUNC GetTexLevelParameterfv, 284, 16
USE_GL_FUNC GetTexLevelParameteriv, 285, 16
USE_GL_FUNC GetTexParameterfv, 282, 12
USE_GL_FUNC GetTexParameteriv, 283, 12
USE_GL_FUNC Hint, 158, 8
USE_GL_FUNC IndexMask, 212, 4
USE_GL_FUNC IndexPointer, 314, 12
USE_GL_FUNC Indexd, 44, 8
USE_GL_FUNC Indexdv, 45, 4
USE_GL_FUNC Indexf, 46, 4
USE_GL_FUNC Indexfv, 47, 4
USE_GL_FUNC Indexi, 48, 4
USE_GL_FUNC Indexiv, 49, 4
USE_GL_FUNC Indexs, 50, 4
USE_GL_FUNC Indexsv, 51, 4
USE_GL_FUNC Indexub, 315, 4
USE_GL_FUNC Indexubv, 316, 4
USE_GL_FUNC InitNames, 197, 0
USE_GL_FUNC InterleavedArrays, 317, 12
USE_GL_FUNC IsEnabled, 286, 4
USE_GL_FUNC IsList, 287, 4
USE_GL_FUNC IsTexture, 330, 4
USE_GL_FUNC LightModelf, 163, 8
USE_GL_FUNC LightModelfv, 164, 8
USE_GL_FUNC LightModeli, 165, 8
USE_GL_FUNC LightModeliv, 166, 8
USE_GL_FUNC Lightf, 159, 12
USE_GL_FUNC Lightfv, 160, 12
USE_GL_FUNC Lighti, 161, 12
USE_GL_FUNC Lightiv, 162, 12
USE_GL_FUNC LineStipple, 167, 8
USE_GL_FUNC LineWidth, 168, 4
USE_GL_FUNC ListBase, 6, 4
USE_GL_FUNC LoadIdentity, 290, 0
USE_GL_FUNC LoadMatrixd, 292, 4
USE_GL_FUNC LoadMatrixf, 291, 4
USE_GL_FUNC LoadName, 198, 4
USE_GL_FUNC LogicOp, 242, 4
USE_GL_FUNC Map1d, 220, 32
USE_GL_FUNC Map1f, 221, 24
USE_GL_FUNC Map2d, 222, 56
USE_GL_FUNC Map2f, 223, 40
USE_GL_FUNC MapGrid1d, 224, 20
USE_GL_FUNC MapGrid1f, 225, 12
USE_GL_FUNC MapGrid2d, 226, 40
USE_GL_FUNC MapGrid2f, 227, 24
USE_GL_FUNC Materialf, 169, 12
USE_GL_FUNC Materialfv, 170, 12
USE_GL_FUNC Materiali, 171, 12
USE_GL_FUNC Materialiv, 172, 12
USE_GL_FUNC MatrixMode, 293, 4
USE_GL_FUNC MultMatrixd, 295, 4
USE_GL_FUNC MultMatrixf, 294, 4
USE_GL_FUNC NewList, 0, 8
USE_GL_FUNC Normal3b, 52, 12
USE_GL_FUNC Normal3bv, 53, 4
USE_GL_FUNC Normal3d, 54, 24
USE_GL_FUNC Normal3dv, 55, 4
USE_GL_FUNC Normal3f, 56, 12
USE_GL_FUNC Normal3fv, 57, 4
USE_GL_FUNC Normal3i, 58, 12
USE_GL_FUNC Normal3iv, 59, 4
USE_GL_FUNC Normal3s, 60, 12
USE_GL_FUNC Normal3sv, 61, 4
USE_GL_FUNC NormalPointer, 318, 12
USE_GL_FUNC Ortho, 296, 48
USE_GL_FUNC PassThrough, 199, 4
USE_GL_FUNC PixelMapfv, 251, 12
USE_GL_FUNC PixelMapuiv, 252, 12
USE_GL_FUNC PixelMapusv, 253, 12
USE_GL_FUNC PixelStoref, 249, 8
USE_GL_FUNC PixelStorei, 250, 8
USE_GL_FUNC PixelTransferf, 247, 8
USE_GL_FUNC PixelTransferi, 248, 8
USE_GL_FUNC PixelZoom, 246, 8
USE_GL_FUNC PointSize, 173, 4
USE_GL_FUNC PolygonMode, 174, 8
USE_GL_FUNC PolygonOffset, 319, 8
USE_GL_FUNC PolygonStipple, 175, 4
USE_GL_FUNC PopAttrib, 218, 0
USE_GL_FUNC PopClientAttrib, 334, 0
USE_GL_FUNC PopMatrix, 297, 0
USE_GL_FUNC PopName, 200, 0
USE_GL_FUNC PrioritizeTextures, 331, 12
USE_GL_FUNC PushAttrib, 219, 4
USE_GL_FUNC PushClientAttrib, 335, 4
USE_GL_FUNC PushMatrix, 298, 0
USE_GL_FUNC PushName, 201, 4
USE_GL_FUNC RasterPos2d, 62, 16
USE_GL_FUNC RasterPos2dv, 63, 4
USE_GL_FUNC RasterPos2f, 64, 8
USE_GL_FUNC RasterPos2fv, 65, 4
USE_GL_FUNC RasterPos2i, 66, 8
USE_GL_FUNC RasterPos2iv, 67, 4
USE_GL_FUNC RasterPos2s, 68, 8
USE_GL_FUNC RasterPos2sv, 69, 4
USE_GL_FUNC RasterPos3d, 70, 24
USE_GL_FUNC RasterPos3dv, 71, 4
USE_GL_FUNC RasterPos3f, 72, 12
USE_GL_FUNC RasterPos3fv, 73, 4
USE_GL_FUNC RasterPos3i, 74, 12
USE_GL_FUNC RasterPos3iv, 75, 4
USE_GL_FUNC RasterPos3s, 76, 12
USE_GL_FUNC RasterPos3sv, 77, 4
USE_GL_FUNC RasterPos4d, 78, 32
USE_GL_FUNC RasterPos4dv, 79, 4
USE_GL_FUNC RasterPos4f, 80, 16
USE_GL_FUNC RasterPos4fv, 81, 4
USE_GL_FUNC RasterPos4i, 82, 16
USE_GL_FUNC RasterPos4iv, 83, 4
USE_GL_FUNC RasterPos4s, 84, 16
USE_GL_FUNC RasterPos4sv, 85, 4
USE_GL_FUNC ReadBuffer, 254, 4
USE_GL_FUNC ReadPixels, 256, 28
USE_GL_FUNC Rectd, 86, 32
USE_GL_FUNC Rectdv, 87, 8
USE_GL_FUNC Rectf, 88, 16
USE_GL_FUNC Rectfv, 89, 8
USE_GL_FUNC Recti, 90, 16
USE_GL_FUNC Rectiv, 91, 8
USE_GL_FUNC Rects, 92, 16
USE_GL_FUNC Rectsv, 93, 8
USE_GL_FUNC RenderMode, 196, 4
USE_GL_FUNC Rotated, 299, 32
USE_GL_FUNC Rotatef, 300, 16
USE_GL_FUNC Scaled, 301, 24
USE_GL_FUNC Scalef, 302, 12
USE_GL_FUNC Scissor, 176, 16
USE_GL_FUNC SelectBuffer, 195, 8
USE_GL_FUNC ShadeModel, 177, 4
USE_GL_FUNC StencilFunc, 243, 12
USE_GL_FUNC StencilMask, 209, 4
USE_GL_FUNC StencilOp, 244, 12
USE_GL_FUNC TexCoord1d, 94, 8
USE_GL_FUNC TexCoord1dv, 95, 4
USE_GL_FUNC TexCoord1f, 96, 4
USE_GL_FUNC TexCoord1fv, 97, 4
USE_GL_FUNC TexCoord1i, 98, 4
USE_GL_FUNC TexCoord1iv, 99, 4
USE_GL_FUNC TexCoord1s, 100, 4
USE_GL_FUNC TexCoord1sv, 101, 4
USE_GL_FUNC TexCoord2d, 102, 16
USE_GL_FUNC TexCoord2dv, 103, 4
USE_GL_FUNC TexCoord2f, 104, 8
USE_GL_FUNC TexCoord2fv, 105, 4
USE_GL_FUNC TexCoord2i, 106, 8
USE_GL_FUNC TexCoord2iv, 107, 4
USE_GL_FUNC TexCoord2s, 108, 8
USE_GL_FUNC TexCoord2sv, 109, 4
USE_GL_FUNC TexCoord3d, 110, 24
USE_GL_FUNC TexCoord3dv, 111, 4
USE_GL_FUNC TexCoord3f, 112, 12
USE_GL_FUNC TexCoord3fv, 113, 4
USE_GL_FUNC TexCoord3i, 114, 12
USE_GL_FUNC TexCoord3iv, 115, 4
USE_GL_FUNC TexCoord3s, 116, 12
USE_GL_FUNC TexCoord3sv, 117, 4
USE_GL_FUNC TexCoord4d, 118, 32
USE_GL_FUNC TexCoord4dv, 119, 4
USE_GL_FUNC TexCoord4f, 120, 16
USE_GL_FUNC TexCoord4fv, 121, 4
USE_GL_FUNC TexCoord4i, 122, 16
USE_GL_FUNC TexCoord4iv, 123, 4
USE_GL_FUNC TexCoord4s, 124, 16
USE_GL_FUNC TexCoord4sv, 125, 4
USE_GL_FUNC TexCoordPointer, 320, 16
USE_GL_FUNC TexEnvf, 184, 12
USE_GL_FUNC TexEnvfv, 185, 12
USE_GL_FUNC TexEnvi, 186, 12
USE_GL_FUNC TexEnviv, 187, 12
USE_GL_FUNC TexGend, 188, 16
USE_GL_FUNC TexGendv, 189, 12
USE_GL_FUNC TexGenf, 190, 12
USE_GL_FUNC TexGenfv, 191, 12
USE_GL_FUNC TexGeni, 192, 12
USE_GL_FUNC TexGeniv, 193, 12
USE_GL_FUNC TexImage1D, 182, 32
USE_GL_FUNC TexImage2D, 183, 36
USE_GL_FUNC TexParameterf, 178, 12
USE_GL_FUNC TexParameterfv, 179, 12
USE_GL_FUNC TexParameteri, 180, 12
USE_GL_FUNC TexParameteriv, 181, 12
USE_GL_FUNC TexSubImage1D, 332, 28
USE_GL_FUNC TexSubImage2D, 333, 36
USE_GL_FUNC Translated, 303, 24
USE_GL_FUNC Translatef, 304, 12
USE_GL_FUNC Vertex2d, 126, 16
USE_GL_FUNC Vertex2dv, 127, 4
USE_GL_FUNC Vertex2f, 128, 8
USE_GL_FUNC Vertex2fv, 129, 4
USE_GL_FUNC Vertex2i, 130, 8
USE_GL_FUNC Vertex2iv, 131, 4
USE_GL_FUNC Vertex2s, 132, 8
USE_GL_FUNC Vertex2sv, 133, 4
USE_GL_FUNC Vertex3d, 134, 24
USE_GL_FUNC Vertex3dv, 135, 4
USE_GL_FUNC Vertex3f, 136, 12
USE_GL_FUNC Vertex3fv, 137, 4
USE_GL_FUNC Vertex3i, 138, 12
USE_GL_FUNC Vertex3iv, 139, 4
USE_GL_FUNC Vertex3s, 140, 12
USE_GL_FUNC Vertex3sv, 141, 4
USE_GL_FUNC Vertex4d, 142, 32
USE_GL_FUNC Vertex4dv, 143, 4
USE_GL_FUNC Vertex4f, 144, 16
USE_GL_FUNC Vertex4fv, 145, 4
USE_GL_FUNC Vertex4i, 146, 16
USE_GL_FUNC Vertex4iv, 147, 4
USE_GL_FUNC Vertex4s, 148, 16
USE_GL_FUNC Vertex4sv, 149, 4
USE_GL_FUNC VertexPointer, 321, 16
USE_GL_FUNC Viewport, 305, 16

END
