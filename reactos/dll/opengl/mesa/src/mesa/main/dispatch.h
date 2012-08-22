/* DO NOT EDIT - This file generated automatically by gl_table.py (from Mesa) script */

/*
 * (C) Copyright IBM Corporation 2005
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM,
 * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#if !defined( _DISPATCH_H_ )
#  define _DISPATCH_H_


/**
 * \file main/dispatch.h
 * Macros for handling GL dispatch tables.
 *
 * For each known GL function, there are 3 macros in this file.  The first
 * macro is named CALL_FuncName and is used to call that GL function using
 * the specified dispatch table.  The other 2 macros, called GET_FuncName
 * can SET_FuncName, are used to get and set the dispatch pointer for the
 * named function in the specified dispatch table.
 */

/* GLXEXT is defined when building the GLX extension in the xserver.
 */
#if !defined(GLXEXT)
#include "main/mfeatures.h"
#endif

#define CALL_by_offset(disp, cast, offset, parameters) \
    (*(cast (GET_by_offset(disp, offset)))) parameters
#define GET_by_offset(disp, offset) \
    (offset >= 0) ? (((_glapi_proc *)(disp))[offset]) : NULL
#define SET_by_offset(disp, offset, fn) \
    do { \
        if ( (offset) < 0 ) { \
            /* fprintf( stderr, "[%s:%u] SET_by_offset(%p, %d, %s)!\n", */ \
            /*         __func__, __LINE__, disp, offset, # fn); */ \
            /* abort(); */ \
        } \
        else { \
            ( (_glapi_proc *) (disp) )[offset] = (_glapi_proc) fn; \
        } \
    } while(0)

/* total number of offsets below */
#define _gloffset_COUNT 973

#define _gloffset_NewList 0
#define _gloffset_EndList 1
#define _gloffset_CallList 2
#define _gloffset_CallLists 3
#define _gloffset_DeleteLists 4
#define _gloffset_GenLists 5
#define _gloffset_ListBase 6
#define _gloffset_Begin 7
#define _gloffset_Bitmap 8
#define _gloffset_Color3b 9
#define _gloffset_Color3bv 10
#define _gloffset_Color3d 11
#define _gloffset_Color3dv 12
#define _gloffset_Color3f 13
#define _gloffset_Color3fv 14
#define _gloffset_Color3i 15
#define _gloffset_Color3iv 16
#define _gloffset_Color3s 17
#define _gloffset_Color3sv 18
#define _gloffset_Color3ub 19
#define _gloffset_Color3ubv 20
#define _gloffset_Color3ui 21
#define _gloffset_Color3uiv 22
#define _gloffset_Color3us 23
#define _gloffset_Color3usv 24
#define _gloffset_Color4b 25
#define _gloffset_Color4bv 26
#define _gloffset_Color4d 27
#define _gloffset_Color4dv 28
#define _gloffset_Color4f 29
#define _gloffset_Color4fv 30
#define _gloffset_Color4i 31
#define _gloffset_Color4iv 32
#define _gloffset_Color4s 33
#define _gloffset_Color4sv 34
#define _gloffset_Color4ub 35
#define _gloffset_Color4ubv 36
#define _gloffset_Color4ui 37
#define _gloffset_Color4uiv 38
#define _gloffset_Color4us 39
#define _gloffset_Color4usv 40
#define _gloffset_EdgeFlag 41
#define _gloffset_EdgeFlagv 42
#define _gloffset_End 43
#define _gloffset_Indexd 44
#define _gloffset_Indexdv 45
#define _gloffset_Indexf 46
#define _gloffset_Indexfv 47
#define _gloffset_Indexi 48
#define _gloffset_Indexiv 49
#define _gloffset_Indexs 50
#define _gloffset_Indexsv 51
#define _gloffset_Normal3b 52
#define _gloffset_Normal3bv 53
#define _gloffset_Normal3d 54
#define _gloffset_Normal3dv 55
#define _gloffset_Normal3f 56
#define _gloffset_Normal3fv 57
#define _gloffset_Normal3i 58
#define _gloffset_Normal3iv 59
#define _gloffset_Normal3s 60
#define _gloffset_Normal3sv 61
#define _gloffset_RasterPos2d 62
#define _gloffset_RasterPos2dv 63
#define _gloffset_RasterPos2f 64
#define _gloffset_RasterPos2fv 65
#define _gloffset_RasterPos2i 66
#define _gloffset_RasterPos2iv 67
#define _gloffset_RasterPos2s 68
#define _gloffset_RasterPos2sv 69
#define _gloffset_RasterPos3d 70
#define _gloffset_RasterPos3dv 71
#define _gloffset_RasterPos3f 72
#define _gloffset_RasterPos3fv 73
#define _gloffset_RasterPos3i 74
#define _gloffset_RasterPos3iv 75
#define _gloffset_RasterPos3s 76
#define _gloffset_RasterPos3sv 77
#define _gloffset_RasterPos4d 78
#define _gloffset_RasterPos4dv 79
#define _gloffset_RasterPos4f 80
#define _gloffset_RasterPos4fv 81
#define _gloffset_RasterPos4i 82
#define _gloffset_RasterPos4iv 83
#define _gloffset_RasterPos4s 84
#define _gloffset_RasterPos4sv 85
#define _gloffset_Rectd 86
#define _gloffset_Rectdv 87
#define _gloffset_Rectf 88
#define _gloffset_Rectfv 89
#define _gloffset_Recti 90
#define _gloffset_Rectiv 91
#define _gloffset_Rects 92
#define _gloffset_Rectsv 93
#define _gloffset_TexCoord1d 94
#define _gloffset_TexCoord1dv 95
#define _gloffset_TexCoord1f 96
#define _gloffset_TexCoord1fv 97
#define _gloffset_TexCoord1i 98
#define _gloffset_TexCoord1iv 99
#define _gloffset_TexCoord1s 100
#define _gloffset_TexCoord1sv 101
#define _gloffset_TexCoord2d 102
#define _gloffset_TexCoord2dv 103
#define _gloffset_TexCoord2f 104
#define _gloffset_TexCoord2fv 105
#define _gloffset_TexCoord2i 106
#define _gloffset_TexCoord2iv 107
#define _gloffset_TexCoord2s 108
#define _gloffset_TexCoord2sv 109
#define _gloffset_TexCoord3d 110
#define _gloffset_TexCoord3dv 111
#define _gloffset_TexCoord3f 112
#define _gloffset_TexCoord3fv 113
#define _gloffset_TexCoord3i 114
#define _gloffset_TexCoord3iv 115
#define _gloffset_TexCoord3s 116
#define _gloffset_TexCoord3sv 117
#define _gloffset_TexCoord4d 118
#define _gloffset_TexCoord4dv 119
#define _gloffset_TexCoord4f 120
#define _gloffset_TexCoord4fv 121
#define _gloffset_TexCoord4i 122
#define _gloffset_TexCoord4iv 123
#define _gloffset_TexCoord4s 124
#define _gloffset_TexCoord4sv 125
#define _gloffset_Vertex2d 126
#define _gloffset_Vertex2dv 127
#define _gloffset_Vertex2f 128
#define _gloffset_Vertex2fv 129
#define _gloffset_Vertex2i 130
#define _gloffset_Vertex2iv 131
#define _gloffset_Vertex2s 132
#define _gloffset_Vertex2sv 133
#define _gloffset_Vertex3d 134
#define _gloffset_Vertex3dv 135
#define _gloffset_Vertex3f 136
#define _gloffset_Vertex3fv 137
#define _gloffset_Vertex3i 138
#define _gloffset_Vertex3iv 139
#define _gloffset_Vertex3s 140
#define _gloffset_Vertex3sv 141
#define _gloffset_Vertex4d 142
#define _gloffset_Vertex4dv 143
#define _gloffset_Vertex4f 144
#define _gloffset_Vertex4fv 145
#define _gloffset_Vertex4i 146
#define _gloffset_Vertex4iv 147
#define _gloffset_Vertex4s 148
#define _gloffset_Vertex4sv 149
#define _gloffset_ClipPlane 150
#define _gloffset_ColorMaterial 151
#define _gloffset_CullFace 152
#define _gloffset_Fogf 153
#define _gloffset_Fogfv 154
#define _gloffset_Fogi 155
#define _gloffset_Fogiv 156
#define _gloffset_FrontFace 157
#define _gloffset_Hint 158
#define _gloffset_Lightf 159
#define _gloffset_Lightfv 160
#define _gloffset_Lighti 161
#define _gloffset_Lightiv 162
#define _gloffset_LightModelf 163
#define _gloffset_LightModelfv 164
#define _gloffset_LightModeli 165
#define _gloffset_LightModeliv 166
#define _gloffset_LineStipple 167
#define _gloffset_LineWidth 168
#define _gloffset_Materialf 169
#define _gloffset_Materialfv 170
#define _gloffset_Materiali 171
#define _gloffset_Materialiv 172
#define _gloffset_PointSize 173
#define _gloffset_PolygonMode 174
#define _gloffset_PolygonStipple 175
#define _gloffset_Scissor 176
#define _gloffset_ShadeModel 177
#define _gloffset_TexParameterf 178
#define _gloffset_TexParameterfv 179
#define _gloffset_TexParameteri 180
#define _gloffset_TexParameteriv 181
#define _gloffset_TexImage1D 182
#define _gloffset_TexImage2D 183
#define _gloffset_TexEnvf 184
#define _gloffset_TexEnvfv 185
#define _gloffset_TexEnvi 186
#define _gloffset_TexEnviv 187
#define _gloffset_TexGend 188
#define _gloffset_TexGendv 189
#define _gloffset_TexGenf 190
#define _gloffset_TexGenfv 191
#define _gloffset_TexGeni 192
#define _gloffset_TexGeniv 193
#define _gloffset_FeedbackBuffer 194
#define _gloffset_SelectBuffer 195
#define _gloffset_RenderMode 196
#define _gloffset_InitNames 197
#define _gloffset_LoadName 198
#define _gloffset_PassThrough 199
#define _gloffset_PopName 200
#define _gloffset_PushName 201
#define _gloffset_DrawBuffer 202
#define _gloffset_Clear 203
#define _gloffset_ClearAccum 204
#define _gloffset_ClearIndex 205
#define _gloffset_ClearColor 206
#define _gloffset_ClearStencil 207
#define _gloffset_ClearDepth 208
#define _gloffset_StencilMask 209
#define _gloffset_ColorMask 210
#define _gloffset_DepthMask 211
#define _gloffset_IndexMask 212
#define _gloffset_Accum 213
#define _gloffset_Disable 214
#define _gloffset_Enable 215
#define _gloffset_Finish 216
#define _gloffset_Flush 217
#define _gloffset_PopAttrib 218
#define _gloffset_PushAttrib 219
#define _gloffset_Map1d 220
#define _gloffset_Map1f 221
#define _gloffset_Map2d 222
#define _gloffset_Map2f 223
#define _gloffset_MapGrid1d 224
#define _gloffset_MapGrid1f 225
#define _gloffset_MapGrid2d 226
#define _gloffset_MapGrid2f 227
#define _gloffset_EvalCoord1d 228
#define _gloffset_EvalCoord1dv 229
#define _gloffset_EvalCoord1f 230
#define _gloffset_EvalCoord1fv 231
#define _gloffset_EvalCoord2d 232
#define _gloffset_EvalCoord2dv 233
#define _gloffset_EvalCoord2f 234
#define _gloffset_EvalCoord2fv 235
#define _gloffset_EvalMesh1 236
#define _gloffset_EvalPoint1 237
#define _gloffset_EvalMesh2 238
#define _gloffset_EvalPoint2 239
#define _gloffset_AlphaFunc 240
#define _gloffset_BlendFunc 241
#define _gloffset_LogicOp 242
#define _gloffset_StencilFunc 243
#define _gloffset_StencilOp 244
#define _gloffset_DepthFunc 245
#define _gloffset_PixelZoom 246
#define _gloffset_PixelTransferf 247
#define _gloffset_PixelTransferi 248
#define _gloffset_PixelStoref 249
#define _gloffset_PixelStorei 250
#define _gloffset_PixelMapfv 251
#define _gloffset_PixelMapuiv 252
#define _gloffset_PixelMapusv 253
#define _gloffset_ReadBuffer 254
#define _gloffset_CopyPixels 255
#define _gloffset_ReadPixels 256
#define _gloffset_DrawPixels 257
#define _gloffset_GetBooleanv 258
#define _gloffset_GetClipPlane 259
#define _gloffset_GetDoublev 260
#define _gloffset_GetError 261
#define _gloffset_GetFloatv 262
#define _gloffset_GetIntegerv 263
#define _gloffset_GetLightfv 264
#define _gloffset_GetLightiv 265
#define _gloffset_GetMapdv 266
#define _gloffset_GetMapfv 267
#define _gloffset_GetMapiv 268
#define _gloffset_GetMaterialfv 269
#define _gloffset_GetMaterialiv 270
#define _gloffset_GetPixelMapfv 271
#define _gloffset_GetPixelMapuiv 272
#define _gloffset_GetPixelMapusv 273
#define _gloffset_GetPolygonStipple 274
#define _gloffset_GetString 275
#define _gloffset_GetTexEnvfv 276
#define _gloffset_GetTexEnviv 277
#define _gloffset_GetTexGendv 278
#define _gloffset_GetTexGenfv 279
#define _gloffset_GetTexGeniv 280
#define _gloffset_GetTexImage 281
#define _gloffset_GetTexParameterfv 282
#define _gloffset_GetTexParameteriv 283
#define _gloffset_GetTexLevelParameterfv 284
#define _gloffset_GetTexLevelParameteriv 285
#define _gloffset_IsEnabled 286
#define _gloffset_IsList 287
#define _gloffset_DepthRange 288
#define _gloffset_Frustum 289
#define _gloffset_LoadIdentity 290
#define _gloffset_LoadMatrixf 291
#define _gloffset_LoadMatrixd 292
#define _gloffset_MatrixMode 293
#define _gloffset_MultMatrixf 294
#define _gloffset_MultMatrixd 295
#define _gloffset_Ortho 296
#define _gloffset_PopMatrix 297
#define _gloffset_PushMatrix 298
#define _gloffset_Rotated 299
#define _gloffset_Rotatef 300
#define _gloffset_Scaled 301
#define _gloffset_Scalef 302
#define _gloffset_Translated 303
#define _gloffset_Translatef 304
#define _gloffset_Viewport 305
#define _gloffset_ArrayElement 306
#define _gloffset_BindTexture 307
#define _gloffset_ColorPointer 308
#define _gloffset_DisableClientState 309
#define _gloffset_DrawArrays 310
#define _gloffset_DrawElements 311
#define _gloffset_EdgeFlagPointer 312
#define _gloffset_EnableClientState 313
#define _gloffset_IndexPointer 314
#define _gloffset_Indexub 315
#define _gloffset_Indexubv 316
#define _gloffset_InterleavedArrays 317
#define _gloffset_NormalPointer 318
#define _gloffset_PolygonOffset 319
#define _gloffset_TexCoordPointer 320
#define _gloffset_VertexPointer 321
#define _gloffset_AreTexturesResident 322
#define _gloffset_CopyTexImage1D 323
#define _gloffset_CopyTexImage2D 324
#define _gloffset_CopyTexSubImage1D 325
#define _gloffset_CopyTexSubImage2D 326
#define _gloffset_DeleteTextures 327
#define _gloffset_GenTextures 328
#define _gloffset_GetPointerv 329
#define _gloffset_IsTexture 330
#define _gloffset_PrioritizeTextures 331
#define _gloffset_TexSubImage1D 332
#define _gloffset_TexSubImage2D 333
#define _gloffset_PopClientAttrib 334
#define _gloffset_PushClientAttrib 335
#define _gloffset_BlendColor 336
#define _gloffset_BlendEquation 337
#define _gloffset_DrawRangeElements 338
#define _gloffset_ColorTable 339
#define _gloffset_ColorTableParameterfv 340
#define _gloffset_ColorTableParameteriv 341
#define _gloffset_CopyColorTable 342
#define _gloffset_GetColorTable 343
#define _gloffset_GetColorTableParameterfv 344
#define _gloffset_GetColorTableParameteriv 345
#define _gloffset_ColorSubTable 346
#define _gloffset_CopyColorSubTable 347
#define _gloffset_ConvolutionFilter1D 348
#define _gloffset_ConvolutionFilter2D 349
#define _gloffset_ConvolutionParameterf 350
#define _gloffset_ConvolutionParameterfv 351
#define _gloffset_ConvolutionParameteri 352
#define _gloffset_ConvolutionParameteriv 353
#define _gloffset_CopyConvolutionFilter1D 354
#define _gloffset_CopyConvolutionFilter2D 355
#define _gloffset_GetConvolutionFilter 356
#define _gloffset_GetConvolutionParameterfv 357
#define _gloffset_GetConvolutionParameteriv 358
#define _gloffset_GetSeparableFilter 359
#define _gloffset_SeparableFilter2D 360
#define _gloffset_GetHistogram 361
#define _gloffset_GetHistogramParameterfv 362
#define _gloffset_GetHistogramParameteriv 363
#define _gloffset_GetMinmax 364
#define _gloffset_GetMinmaxParameterfv 365
#define _gloffset_GetMinmaxParameteriv 366
#define _gloffset_Histogram 367
#define _gloffset_Minmax 368
#define _gloffset_ResetHistogram 369
#define _gloffset_ResetMinmax 370
#define _gloffset_TexImage3D 371
#define _gloffset_TexSubImage3D 372
#define _gloffset_CopyTexSubImage3D 373
#define _gloffset_ActiveTextureARB 374
#define _gloffset_ClientActiveTextureARB 375
#define _gloffset_MultiTexCoord1dARB 376
#define _gloffset_MultiTexCoord1dvARB 377
#define _gloffset_MultiTexCoord1fARB 378
#define _gloffset_MultiTexCoord1fvARB 379
#define _gloffset_MultiTexCoord1iARB 380
#define _gloffset_MultiTexCoord1ivARB 381
#define _gloffset_MultiTexCoord1sARB 382
#define _gloffset_MultiTexCoord1svARB 383
#define _gloffset_MultiTexCoord2dARB 384
#define _gloffset_MultiTexCoord2dvARB 385
#define _gloffset_MultiTexCoord2fARB 386
#define _gloffset_MultiTexCoord2fvARB 387
#define _gloffset_MultiTexCoord2iARB 388
#define _gloffset_MultiTexCoord2ivARB 389
#define _gloffset_MultiTexCoord2sARB 390
#define _gloffset_MultiTexCoord2svARB 391
#define _gloffset_MultiTexCoord3dARB 392
#define _gloffset_MultiTexCoord3dvARB 393
#define _gloffset_MultiTexCoord3fARB 394
#define _gloffset_MultiTexCoord3fvARB 395
#define _gloffset_MultiTexCoord3iARB 396
#define _gloffset_MultiTexCoord3ivARB 397
#define _gloffset_MultiTexCoord3sARB 398
#define _gloffset_MultiTexCoord3svARB 399
#define _gloffset_MultiTexCoord4dARB 400
#define _gloffset_MultiTexCoord4dvARB 401
#define _gloffset_MultiTexCoord4fARB 402
#define _gloffset_MultiTexCoord4fvARB 403
#define _gloffset_MultiTexCoord4iARB 404
#define _gloffset_MultiTexCoord4ivARB 405
#define _gloffset_MultiTexCoord4sARB 406
#define _gloffset_MultiTexCoord4svARB 407

#if !FEATURE_remap_table

#define _gloffset_AttachShader 408
#define _gloffset_CreateProgram 409
#define _gloffset_CreateShader 410
#define _gloffset_DeleteProgram 411
#define _gloffset_DeleteShader 412
#define _gloffset_DetachShader 413
#define _gloffset_GetAttachedShaders 414
#define _gloffset_GetProgramInfoLog 415
#define _gloffset_GetProgramiv 416
#define _gloffset_GetShaderInfoLog 417
#define _gloffset_GetShaderiv 418
#define _gloffset_IsProgram 419
#define _gloffset_IsShader 420
#define _gloffset_StencilFuncSeparate 421
#define _gloffset_StencilMaskSeparate 422
#define _gloffset_StencilOpSeparate 423
#define _gloffset_UniformMatrix2x3fv 424
#define _gloffset_UniformMatrix2x4fv 425
#define _gloffset_UniformMatrix3x2fv 426
#define _gloffset_UniformMatrix3x4fv 427
#define _gloffset_UniformMatrix4x2fv 428
#define _gloffset_UniformMatrix4x3fv 429
#define _gloffset_ClampColor 430
#define _gloffset_ClearBufferfi 431
#define _gloffset_ClearBufferfv 432
#define _gloffset_ClearBufferiv 433
#define _gloffset_ClearBufferuiv 434
#define _gloffset_GetStringi 435
#define _gloffset_TexBuffer 436
#define _gloffset_FramebufferTexture 437
#define _gloffset_GetBufferParameteri64v 438
#define _gloffset_GetInteger64i_v 439
#define _gloffset_VertexAttribDivisor 440
#define _gloffset_LoadTransposeMatrixdARB 441
#define _gloffset_LoadTransposeMatrixfARB 442
#define _gloffset_MultTransposeMatrixdARB 443
#define _gloffset_MultTransposeMatrixfARB 444
#define _gloffset_SampleCoverageARB 445
#define _gloffset_CompressedTexImage1DARB 446
#define _gloffset_CompressedTexImage2DARB 447
#define _gloffset_CompressedTexImage3DARB 448
#define _gloffset_CompressedTexSubImage1DARB 449
#define _gloffset_CompressedTexSubImage2DARB 450
#define _gloffset_CompressedTexSubImage3DARB 451
#define _gloffset_GetCompressedTexImageARB 452
#define _gloffset_DisableVertexAttribArrayARB 453
#define _gloffset_EnableVertexAttribArrayARB 454
#define _gloffset_GetProgramEnvParameterdvARB 455
#define _gloffset_GetProgramEnvParameterfvARB 456
#define _gloffset_GetProgramLocalParameterdvARB 457
#define _gloffset_GetProgramLocalParameterfvARB 458
#define _gloffset_GetProgramStringARB 459
#define _gloffset_GetProgramivARB 460
#define _gloffset_GetVertexAttribdvARB 461
#define _gloffset_GetVertexAttribfvARB 462
#define _gloffset_GetVertexAttribivARB 463
#define _gloffset_ProgramEnvParameter4dARB 464
#define _gloffset_ProgramEnvParameter4dvARB 465
#define _gloffset_ProgramEnvParameter4fARB 466
#define _gloffset_ProgramEnvParameter4fvARB 467
#define _gloffset_ProgramLocalParameter4dARB 468
#define _gloffset_ProgramLocalParameter4dvARB 469
#define _gloffset_ProgramLocalParameter4fARB 470
#define _gloffset_ProgramLocalParameter4fvARB 471
#define _gloffset_ProgramStringARB 472
#define _gloffset_VertexAttrib1dARB 473
#define _gloffset_VertexAttrib1dvARB 474
#define _gloffset_VertexAttrib1fARB 475
#define _gloffset_VertexAttrib1fvARB 476
#define _gloffset_VertexAttrib1sARB 477
#define _gloffset_VertexAttrib1svARB 478
#define _gloffset_VertexAttrib2dARB 479
#define _gloffset_VertexAttrib2dvARB 480
#define _gloffset_VertexAttrib2fARB 481
#define _gloffset_VertexAttrib2fvARB 482
#define _gloffset_VertexAttrib2sARB 483
#define _gloffset_VertexAttrib2svARB 484
#define _gloffset_VertexAttrib3dARB 485
#define _gloffset_VertexAttrib3dvARB 486
#define _gloffset_VertexAttrib3fARB 487
#define _gloffset_VertexAttrib3fvARB 488
#define _gloffset_VertexAttrib3sARB 489
#define _gloffset_VertexAttrib3svARB 490
#define _gloffset_VertexAttrib4NbvARB 491
#define _gloffset_VertexAttrib4NivARB 492
#define _gloffset_VertexAttrib4NsvARB 493
#define _gloffset_VertexAttrib4NubARB 494
#define _gloffset_VertexAttrib4NubvARB 495
#define _gloffset_VertexAttrib4NuivARB 496
#define _gloffset_VertexAttrib4NusvARB 497
#define _gloffset_VertexAttrib4bvARB 498
#define _gloffset_VertexAttrib4dARB 499
#define _gloffset_VertexAttrib4dvARB 500
#define _gloffset_VertexAttrib4fARB 501
#define _gloffset_VertexAttrib4fvARB 502
#define _gloffset_VertexAttrib4ivARB 503
#define _gloffset_VertexAttrib4sARB 504
#define _gloffset_VertexAttrib4svARB 505
#define _gloffset_VertexAttrib4ubvARB 506
#define _gloffset_VertexAttrib4uivARB 507
#define _gloffset_VertexAttrib4usvARB 508
#define _gloffset_VertexAttribPointerARB 509
#define _gloffset_BindBufferARB 510
#define _gloffset_BufferDataARB 511
#define _gloffset_BufferSubDataARB 512
#define _gloffset_DeleteBuffersARB 513
#define _gloffset_GenBuffersARB 514
#define _gloffset_GetBufferParameterivARB 515
#define _gloffset_GetBufferPointervARB 516
#define _gloffset_GetBufferSubDataARB 517
#define _gloffset_IsBufferARB 518
#define _gloffset_MapBufferARB 519
#define _gloffset_UnmapBufferARB 520
#define _gloffset_BeginQueryARB 521
#define _gloffset_DeleteQueriesARB 522
#define _gloffset_EndQueryARB 523
#define _gloffset_GenQueriesARB 524
#define _gloffset_GetQueryObjectivARB 525
#define _gloffset_GetQueryObjectuivARB 526
#define _gloffset_GetQueryivARB 527
#define _gloffset_IsQueryARB 528
#define _gloffset_AttachObjectARB 529
#define _gloffset_CompileShaderARB 530
#define _gloffset_CreateProgramObjectARB 531
#define _gloffset_CreateShaderObjectARB 532
#define _gloffset_DeleteObjectARB 533
#define _gloffset_DetachObjectARB 534
#define _gloffset_GetActiveUniformARB 535
#define _gloffset_GetAttachedObjectsARB 536
#define _gloffset_GetHandleARB 537
#define _gloffset_GetInfoLogARB 538
#define _gloffset_GetObjectParameterfvARB 539
#define _gloffset_GetObjectParameterivARB 540
#define _gloffset_GetShaderSourceARB 541
#define _gloffset_GetUniformLocationARB 542
#define _gloffset_GetUniformfvARB 543
#define _gloffset_GetUniformivARB 544
#define _gloffset_LinkProgramARB 545
#define _gloffset_ShaderSourceARB 546
#define _gloffset_Uniform1fARB 547
#define _gloffset_Uniform1fvARB 548
#define _gloffset_Uniform1iARB 549
#define _gloffset_Uniform1ivARB 550
#define _gloffset_Uniform2fARB 551
#define _gloffset_Uniform2fvARB 552
#define _gloffset_Uniform2iARB 553
#define _gloffset_Uniform2ivARB 554
#define _gloffset_Uniform3fARB 555
#define _gloffset_Uniform3fvARB 556
#define _gloffset_Uniform3iARB 557
#define _gloffset_Uniform3ivARB 558
#define _gloffset_Uniform4fARB 559
#define _gloffset_Uniform4fvARB 560
#define _gloffset_Uniform4iARB 561
#define _gloffset_Uniform4ivARB 562
#define _gloffset_UniformMatrix2fvARB 563
#define _gloffset_UniformMatrix3fvARB 564
#define _gloffset_UniformMatrix4fvARB 565
#define _gloffset_UseProgramObjectARB 566
#define _gloffset_ValidateProgramARB 567
#define _gloffset_BindAttribLocationARB 568
#define _gloffset_GetActiveAttribARB 569
#define _gloffset_GetAttribLocationARB 570
#define _gloffset_DrawBuffersARB 571
#define _gloffset_ClampColorARB 572
#define _gloffset_DrawArraysInstancedARB 573
#define _gloffset_DrawElementsInstancedARB 574
#define _gloffset_RenderbufferStorageMultisample 575
#define _gloffset_FramebufferTextureARB 576
#define _gloffset_FramebufferTextureFaceARB 577
#define _gloffset_ProgramParameteriARB 578
#define _gloffset_VertexAttribDivisorARB 579
#define _gloffset_FlushMappedBufferRange 580
#define _gloffset_MapBufferRange 581
#define _gloffset_TexBufferARB 582
#define _gloffset_BindVertexArray 583
#define _gloffset_GenVertexArrays 584
#define _gloffset_CopyBufferSubData 585
#define _gloffset_ClientWaitSync 586
#define _gloffset_DeleteSync 587
#define _gloffset_FenceSync 588
#define _gloffset_GetInteger64v 589
#define _gloffset_GetSynciv 590
#define _gloffset_IsSync 591
#define _gloffset_WaitSync 592
#define _gloffset_DrawElementsBaseVertex 593
#define _gloffset_DrawElementsInstancedBaseVertex 594
#define _gloffset_DrawRangeElementsBaseVertex 595
#define _gloffset_MultiDrawElementsBaseVertex 596
#define _gloffset_BlendEquationSeparateiARB 597
#define _gloffset_BlendEquationiARB 598
#define _gloffset_BlendFuncSeparateiARB 599
#define _gloffset_BlendFunciARB 600
#define _gloffset_BindSampler 601
#define _gloffset_DeleteSamplers 602
#define _gloffset_GenSamplers 603
#define _gloffset_GetSamplerParameterIiv 604
#define _gloffset_GetSamplerParameterIuiv 605
#define _gloffset_GetSamplerParameterfv 606
#define _gloffset_GetSamplerParameteriv 607
#define _gloffset_IsSampler 608
#define _gloffset_SamplerParameterIiv 609
#define _gloffset_SamplerParameterIuiv 610
#define _gloffset_SamplerParameterf 611
#define _gloffset_SamplerParameterfv 612
#define _gloffset_SamplerParameteri 613
#define _gloffset_SamplerParameteriv 614
#define _gloffset_ColorP3ui 615
#define _gloffset_ColorP3uiv 616
#define _gloffset_ColorP4ui 617
#define _gloffset_ColorP4uiv 618
#define _gloffset_MultiTexCoordP1ui 619
#define _gloffset_MultiTexCoordP1uiv 620
#define _gloffset_MultiTexCoordP2ui 621
#define _gloffset_MultiTexCoordP2uiv 622
#define _gloffset_MultiTexCoordP3ui 623
#define _gloffset_MultiTexCoordP3uiv 624
#define _gloffset_MultiTexCoordP4ui 625
#define _gloffset_MultiTexCoordP4uiv 626
#define _gloffset_NormalP3ui 627
#define _gloffset_NormalP3uiv 628
#define _gloffset_SecondaryColorP3ui 629
#define _gloffset_SecondaryColorP3uiv 630
#define _gloffset_TexCoordP1ui 631
#define _gloffset_TexCoordP1uiv 632
#define _gloffset_TexCoordP2ui 633
#define _gloffset_TexCoordP2uiv 634
#define _gloffset_TexCoordP3ui 635
#define _gloffset_TexCoordP3uiv 636
#define _gloffset_TexCoordP4ui 637
#define _gloffset_TexCoordP4uiv 638
#define _gloffset_VertexAttribP1ui 639
#define _gloffset_VertexAttribP1uiv 640
#define _gloffset_VertexAttribP2ui 641
#define _gloffset_VertexAttribP2uiv 642
#define _gloffset_VertexAttribP3ui 643
#define _gloffset_VertexAttribP3uiv 644
#define _gloffset_VertexAttribP4ui 645
#define _gloffset_VertexAttribP4uiv 646
#define _gloffset_VertexP2ui 647
#define _gloffset_VertexP2uiv 648
#define _gloffset_VertexP3ui 649
#define _gloffset_VertexP3uiv 650
#define _gloffset_VertexP4ui 651
#define _gloffset_VertexP4uiv 652
#define _gloffset_BindTransformFeedback 653
#define _gloffset_DeleteTransformFeedbacks 654
#define _gloffset_DrawTransformFeedback 655
#define _gloffset_GenTransformFeedbacks 656
#define _gloffset_IsTransformFeedback 657
#define _gloffset_PauseTransformFeedback 658
#define _gloffset_ResumeTransformFeedback 659
#define _gloffset_ClearDepthf 660
#define _gloffset_DepthRangef 661
#define _gloffset_GetShaderPrecisionFormat 662
#define _gloffset_ReleaseShaderCompiler 663
#define _gloffset_ShaderBinary 664
#define _gloffset_GetGraphicsResetStatusARB 665
#define _gloffset_GetnColorTableARB 666
#define _gloffset_GetnCompressedTexImageARB 667
#define _gloffset_GetnConvolutionFilterARB 668
#define _gloffset_GetnHistogramARB 669
#define _gloffset_GetnMapdvARB 670
#define _gloffset_GetnMapfvARB 671
#define _gloffset_GetnMapivARB 672
#define _gloffset_GetnMinmaxARB 673
#define _gloffset_GetnPixelMapfvARB 674
#define _gloffset_GetnPixelMapuivARB 675
#define _gloffset_GetnPixelMapusvARB 676
#define _gloffset_GetnPolygonStippleARB 677
#define _gloffset_GetnSeparableFilterARB 678
#define _gloffset_GetnTexImageARB 679
#define _gloffset_GetnUniformdvARB 680
#define _gloffset_GetnUniformfvARB 681
#define _gloffset_GetnUniformivARB 682
#define _gloffset_GetnUniformuivARB 683
#define _gloffset_ReadnPixelsARB 684
#define _gloffset_TexStorage1D 685
#define _gloffset_TexStorage2D 686
#define _gloffset_TexStorage3D 687
#define _gloffset_TextureStorage1DEXT 688
#define _gloffset_TextureStorage2DEXT 689
#define _gloffset_TextureStorage3DEXT 690
#define _gloffset_PolygonOffsetEXT 691
#define _gloffset_GetPixelTexGenParameterfvSGIS 692
#define _gloffset_GetPixelTexGenParameterivSGIS 693
#define _gloffset_PixelTexGenParameterfSGIS 694
#define _gloffset_PixelTexGenParameterfvSGIS 695
#define _gloffset_PixelTexGenParameteriSGIS 696
#define _gloffset_PixelTexGenParameterivSGIS 697
#define _gloffset_SampleMaskSGIS 698
#define _gloffset_SamplePatternSGIS 699
#define _gloffset_ColorPointerEXT 700
#define _gloffset_EdgeFlagPointerEXT 701
#define _gloffset_IndexPointerEXT 702
#define _gloffset_NormalPointerEXT 703
#define _gloffset_TexCoordPointerEXT 704
#define _gloffset_VertexPointerEXT 705
#define _gloffset_PointParameterfEXT 706
#define _gloffset_PointParameterfvEXT 707
#define _gloffset_LockArraysEXT 708
#define _gloffset_UnlockArraysEXT 709
#define _gloffset_SecondaryColor3bEXT 710
#define _gloffset_SecondaryColor3bvEXT 711
#define _gloffset_SecondaryColor3dEXT 712
#define _gloffset_SecondaryColor3dvEXT 713
#define _gloffset_SecondaryColor3fEXT 714
#define _gloffset_SecondaryColor3fvEXT 715
#define _gloffset_SecondaryColor3iEXT 716
#define _gloffset_SecondaryColor3ivEXT 717
#define _gloffset_SecondaryColor3sEXT 718
#define _gloffset_SecondaryColor3svEXT 719
#define _gloffset_SecondaryColor3ubEXT 720
#define _gloffset_SecondaryColor3ubvEXT 721
#define _gloffset_SecondaryColor3uiEXT 722
#define _gloffset_SecondaryColor3uivEXT 723
#define _gloffset_SecondaryColor3usEXT 724
#define _gloffset_SecondaryColor3usvEXT 725
#define _gloffset_SecondaryColorPointerEXT 726
#define _gloffset_MultiDrawArraysEXT 727
#define _gloffset_MultiDrawElementsEXT 728
#define _gloffset_FogCoordPointerEXT 729
#define _gloffset_FogCoorddEXT 730
#define _gloffset_FogCoorddvEXT 731
#define _gloffset_FogCoordfEXT 732
#define _gloffset_FogCoordfvEXT 733
#define _gloffset_PixelTexGenSGIX 734
#define _gloffset_BlendFuncSeparateEXT 735
#define _gloffset_FlushVertexArrayRangeNV 736
#define _gloffset_VertexArrayRangeNV 737
#define _gloffset_CombinerInputNV 738
#define _gloffset_CombinerOutputNV 739
#define _gloffset_CombinerParameterfNV 740
#define _gloffset_CombinerParameterfvNV 741
#define _gloffset_CombinerParameteriNV 742
#define _gloffset_CombinerParameterivNV 743
#define _gloffset_FinalCombinerInputNV 744
#define _gloffset_GetCombinerInputParameterfvNV 745
#define _gloffset_GetCombinerInputParameterivNV 746
#define _gloffset_GetCombinerOutputParameterfvNV 747
#define _gloffset_GetCombinerOutputParameterivNV 748
#define _gloffset_GetFinalCombinerInputParameterfvNV 749
#define _gloffset_GetFinalCombinerInputParameterivNV 750
#define _gloffset_ResizeBuffersMESA 751
#define _gloffset_WindowPos2dMESA 752
#define _gloffset_WindowPos2dvMESA 753
#define _gloffset_WindowPos2fMESA 754
#define _gloffset_WindowPos2fvMESA 755
#define _gloffset_WindowPos2iMESA 756
#define _gloffset_WindowPos2ivMESA 757
#define _gloffset_WindowPos2sMESA 758
#define _gloffset_WindowPos2svMESA 759
#define _gloffset_WindowPos3dMESA 760
#define _gloffset_WindowPos3dvMESA 761
#define _gloffset_WindowPos3fMESA 762
#define _gloffset_WindowPos3fvMESA 763
#define _gloffset_WindowPos3iMESA 764
#define _gloffset_WindowPos3ivMESA 765
#define _gloffset_WindowPos3sMESA 766
#define _gloffset_WindowPos3svMESA 767
#define _gloffset_WindowPos4dMESA 768
#define _gloffset_WindowPos4dvMESA 769
#define _gloffset_WindowPos4fMESA 770
#define _gloffset_WindowPos4fvMESA 771
#define _gloffset_WindowPos4iMESA 772
#define _gloffset_WindowPos4ivMESA 773
#define _gloffset_WindowPos4sMESA 774
#define _gloffset_WindowPos4svMESA 775
#define _gloffset_MultiModeDrawArraysIBM 776
#define _gloffset_MultiModeDrawElementsIBM 777
#define _gloffset_DeleteFencesNV 778
#define _gloffset_FinishFenceNV 779
#define _gloffset_GenFencesNV 780
#define _gloffset_GetFenceivNV 781
#define _gloffset_IsFenceNV 782
#define _gloffset_SetFenceNV 783
#define _gloffset_TestFenceNV 784
#define _gloffset_AreProgramsResidentNV 785
#define _gloffset_BindProgramNV 786
#define _gloffset_DeleteProgramsNV 787
#define _gloffset_ExecuteProgramNV 788
#define _gloffset_GenProgramsNV 789
#define _gloffset_GetProgramParameterdvNV 790
#define _gloffset_GetProgramParameterfvNV 791
#define _gloffset_GetProgramStringNV 792
#define _gloffset_GetProgramivNV 793
#define _gloffset_GetTrackMatrixivNV 794
#define _gloffset_GetVertexAttribPointervNV 795
#define _gloffset_GetVertexAttribdvNV 796
#define _gloffset_GetVertexAttribfvNV 797
#define _gloffset_GetVertexAttribivNV 798
#define _gloffset_IsProgramNV 799
#define _gloffset_LoadProgramNV 800
#define _gloffset_ProgramParameters4dvNV 801
#define _gloffset_ProgramParameters4fvNV 802
#define _gloffset_RequestResidentProgramsNV 803
#define _gloffset_TrackMatrixNV 804
#define _gloffset_VertexAttrib1dNV 805
#define _gloffset_VertexAttrib1dvNV 806
#define _gloffset_VertexAttrib1fNV 807
#define _gloffset_VertexAttrib1fvNV 808
#define _gloffset_VertexAttrib1sNV 809
#define _gloffset_VertexAttrib1svNV 810
#define _gloffset_VertexAttrib2dNV 811
#define _gloffset_VertexAttrib2dvNV 812
#define _gloffset_VertexAttrib2fNV 813
#define _gloffset_VertexAttrib2fvNV 814
#define _gloffset_VertexAttrib2sNV 815
#define _gloffset_VertexAttrib2svNV 816
#define _gloffset_VertexAttrib3dNV 817
#define _gloffset_VertexAttrib3dvNV 818
#define _gloffset_VertexAttrib3fNV 819
#define _gloffset_VertexAttrib3fvNV 820
#define _gloffset_VertexAttrib3sNV 821
#define _gloffset_VertexAttrib3svNV 822
#define _gloffset_VertexAttrib4dNV 823
#define _gloffset_VertexAttrib4dvNV 824
#define _gloffset_VertexAttrib4fNV 825
#define _gloffset_VertexAttrib4fvNV 826
#define _gloffset_VertexAttrib4sNV 827
#define _gloffset_VertexAttrib4svNV 828
#define _gloffset_VertexAttrib4ubNV 829
#define _gloffset_VertexAttrib4ubvNV 830
#define _gloffset_VertexAttribPointerNV 831
#define _gloffset_VertexAttribs1dvNV 832
#define _gloffset_VertexAttribs1fvNV 833
#define _gloffset_VertexAttribs1svNV 834
#define _gloffset_VertexAttribs2dvNV 835
#define _gloffset_VertexAttribs2fvNV 836
#define _gloffset_VertexAttribs2svNV 837
#define _gloffset_VertexAttribs3dvNV 838
#define _gloffset_VertexAttribs3fvNV 839
#define _gloffset_VertexAttribs3svNV 840
#define _gloffset_VertexAttribs4dvNV 841
#define _gloffset_VertexAttribs4fvNV 842
#define _gloffset_VertexAttribs4svNV 843
#define _gloffset_VertexAttribs4ubvNV 844
#define _gloffset_GetTexBumpParameterfvATI 845
#define _gloffset_GetTexBumpParameterivATI 846
#define _gloffset_TexBumpParameterfvATI 847
#define _gloffset_TexBumpParameterivATI 848
#define _gloffset_AlphaFragmentOp1ATI 849
#define _gloffset_AlphaFragmentOp2ATI 850
#define _gloffset_AlphaFragmentOp3ATI 851
#define _gloffset_BeginFragmentShaderATI 852
#define _gloffset_BindFragmentShaderATI 853
#define _gloffset_ColorFragmentOp1ATI 854
#define _gloffset_ColorFragmentOp2ATI 855
#define _gloffset_ColorFragmentOp3ATI 856
#define _gloffset_DeleteFragmentShaderATI 857
#define _gloffset_EndFragmentShaderATI 858
#define _gloffset_GenFragmentShadersATI 859
#define _gloffset_PassTexCoordATI 860
#define _gloffset_SampleMapATI 861
#define _gloffset_SetFragmentShaderConstantATI 862
#define _gloffset_PointParameteriNV 863
#define _gloffset_PointParameterivNV 864
#define _gloffset_ActiveStencilFaceEXT 865
#define _gloffset_BindVertexArrayAPPLE 866
#define _gloffset_DeleteVertexArraysAPPLE 867
#define _gloffset_GenVertexArraysAPPLE 868
#define _gloffset_IsVertexArrayAPPLE 869
#define _gloffset_GetProgramNamedParameterdvNV 870
#define _gloffset_GetProgramNamedParameterfvNV 871
#define _gloffset_ProgramNamedParameter4dNV 872
#define _gloffset_ProgramNamedParameter4dvNV 873
#define _gloffset_ProgramNamedParameter4fNV 874
#define _gloffset_ProgramNamedParameter4fvNV 875
#define _gloffset_PrimitiveRestartIndexNV 876
#define _gloffset_PrimitiveRestartNV 877
#define _gloffset_DepthBoundsEXT 878
#define _gloffset_BlendEquationSeparateEXT 879
#define _gloffset_BindFramebufferEXT 880
#define _gloffset_BindRenderbufferEXT 881
#define _gloffset_CheckFramebufferStatusEXT 882
#define _gloffset_DeleteFramebuffersEXT 883
#define _gloffset_DeleteRenderbuffersEXT 884
#define _gloffset_FramebufferRenderbufferEXT 885
#define _gloffset_FramebufferTexture1DEXT 886
#define _gloffset_FramebufferTexture2DEXT 887
#define _gloffset_FramebufferTexture3DEXT 888
#define _gloffset_GenFramebuffersEXT 889
#define _gloffset_GenRenderbuffersEXT 890
#define _gloffset_GenerateMipmapEXT 891
#define _gloffset_GetFramebufferAttachmentParameterivEXT 892
#define _gloffset_GetRenderbufferParameterivEXT 893
#define _gloffset_IsFramebufferEXT 894
#define _gloffset_IsRenderbufferEXT 895
#define _gloffset_RenderbufferStorageEXT 896
#define _gloffset_BlitFramebufferEXT 897
#define _gloffset_BufferParameteriAPPLE 898
#define _gloffset_FlushMappedBufferRangeAPPLE 899
#define _gloffset_BindFragDataLocationEXT 900
#define _gloffset_GetFragDataLocationEXT 901
#define _gloffset_GetUniformuivEXT 902
#define _gloffset_GetVertexAttribIivEXT 903
#define _gloffset_GetVertexAttribIuivEXT 904
#define _gloffset_Uniform1uiEXT 905
#define _gloffset_Uniform1uivEXT 906
#define _gloffset_Uniform2uiEXT 907
#define _gloffset_Uniform2uivEXT 908
#define _gloffset_Uniform3uiEXT 909
#define _gloffset_Uniform3uivEXT 910
#define _gloffset_Uniform4uiEXT 911
#define _gloffset_Uniform4uivEXT 912
#define _gloffset_VertexAttribI1iEXT 913
#define _gloffset_VertexAttribI1ivEXT 914
#define _gloffset_VertexAttribI1uiEXT 915
#define _gloffset_VertexAttribI1uivEXT 916
#define _gloffset_VertexAttribI2iEXT 917
#define _gloffset_VertexAttribI2ivEXT 918
#define _gloffset_VertexAttribI2uiEXT 919
#define _gloffset_VertexAttribI2uivEXT 920
#define _gloffset_VertexAttribI3iEXT 921
#define _gloffset_VertexAttribI3ivEXT 922
#define _gloffset_VertexAttribI3uiEXT 923
#define _gloffset_VertexAttribI3uivEXT 924
#define _gloffset_VertexAttribI4bvEXT 925
#define _gloffset_VertexAttribI4iEXT 926
#define _gloffset_VertexAttribI4ivEXT 927
#define _gloffset_VertexAttribI4svEXT 928
#define _gloffset_VertexAttribI4ubvEXT 929
#define _gloffset_VertexAttribI4uiEXT 930
#define _gloffset_VertexAttribI4uivEXT 931
#define _gloffset_VertexAttribI4usvEXT 932
#define _gloffset_VertexAttribIPointerEXT 933
#define _gloffset_FramebufferTextureLayerEXT 934
#define _gloffset_ColorMaskIndexedEXT 935
#define _gloffset_DisableIndexedEXT 936
#define _gloffset_EnableIndexedEXT 937
#define _gloffset_GetBooleanIndexedvEXT 938
#define _gloffset_GetIntegerIndexedvEXT 939
#define _gloffset_IsEnabledIndexedEXT 940
#define _gloffset_ClearColorIiEXT 941
#define _gloffset_ClearColorIuiEXT 942
#define _gloffset_GetTexParameterIivEXT 943
#define _gloffset_GetTexParameterIuivEXT 944
#define _gloffset_TexParameterIivEXT 945
#define _gloffset_TexParameterIuivEXT 946
#define _gloffset_BeginConditionalRenderNV 947
#define _gloffset_EndConditionalRenderNV 948
#define _gloffset_BeginTransformFeedbackEXT 949
#define _gloffset_BindBufferBaseEXT 950
#define _gloffset_BindBufferOffsetEXT 951
#define _gloffset_BindBufferRangeEXT 952
#define _gloffset_EndTransformFeedbackEXT 953
#define _gloffset_GetTransformFeedbackVaryingEXT 954
#define _gloffset_TransformFeedbackVaryingsEXT 955
#define _gloffset_ProvokingVertexEXT 956
#define _gloffset_GetTexParameterPointervAPPLE 957
#define _gloffset_TextureRangeAPPLE 958
#define _gloffset_GetObjectParameterivAPPLE 959
#define _gloffset_ObjectPurgeableAPPLE 960
#define _gloffset_ObjectUnpurgeableAPPLE 961
#define _gloffset_ActiveProgramEXT 962
#define _gloffset_CreateShaderProgramEXT 963
#define _gloffset_UseShaderProgramEXT 964
#define _gloffset_TextureBarrierNV 965
#define _gloffset_StencilFuncSeparateATI 966
#define _gloffset_ProgramEnvParameters4fvEXT 967
#define _gloffset_ProgramLocalParameters4fvEXT 968
#define _gloffset_GetQueryObjecti64vEXT 969
#define _gloffset_GetQueryObjectui64vEXT 970
#define _gloffset_EGLImageTargetRenderbufferStorageOES 971
#define _gloffset_EGLImageTargetTexture2DOES 972

#else /* !FEATURE_remap_table */

#define driDispatchRemapTable_size 565
extern int driDispatchRemapTable[ driDispatchRemapTable_size ];

#define AttachShader_remap_index 0
#define CreateProgram_remap_index 1
#define CreateShader_remap_index 2
#define DeleteProgram_remap_index 3
#define DeleteShader_remap_index 4
#define DetachShader_remap_index 5
#define GetAttachedShaders_remap_index 6
#define GetProgramInfoLog_remap_index 7
#define GetProgramiv_remap_index 8
#define GetShaderInfoLog_remap_index 9
#define GetShaderiv_remap_index 10
#define IsProgram_remap_index 11
#define IsShader_remap_index 12
#define StencilFuncSeparate_remap_index 13
#define StencilMaskSeparate_remap_index 14
#define StencilOpSeparate_remap_index 15
#define UniformMatrix2x3fv_remap_index 16
#define UniformMatrix2x4fv_remap_index 17
#define UniformMatrix3x2fv_remap_index 18
#define UniformMatrix3x4fv_remap_index 19
#define UniformMatrix4x2fv_remap_index 20
#define UniformMatrix4x3fv_remap_index 21
#define ClampColor_remap_index 22
#define ClearBufferfi_remap_index 23
#define ClearBufferfv_remap_index 24
#define ClearBufferiv_remap_index 25
#define ClearBufferuiv_remap_index 26
#define GetStringi_remap_index 27
#define TexBuffer_remap_index 28
#define FramebufferTexture_remap_index 29
#define GetBufferParameteri64v_remap_index 30
#define GetInteger64i_v_remap_index 31
#define VertexAttribDivisor_remap_index 32
#define LoadTransposeMatrixdARB_remap_index 33
#define LoadTransposeMatrixfARB_remap_index 34
#define MultTransposeMatrixdARB_remap_index 35
#define MultTransposeMatrixfARB_remap_index 36
#define SampleCoverageARB_remap_index 37
#define CompressedTexImage1DARB_remap_index 38
#define CompressedTexImage2DARB_remap_index 39
#define CompressedTexImage3DARB_remap_index 40
#define CompressedTexSubImage1DARB_remap_index 41
#define CompressedTexSubImage2DARB_remap_index 42
#define CompressedTexSubImage3DARB_remap_index 43
#define GetCompressedTexImageARB_remap_index 44
#define DisableVertexAttribArrayARB_remap_index 45
#define EnableVertexAttribArrayARB_remap_index 46
#define GetProgramEnvParameterdvARB_remap_index 47
#define GetProgramEnvParameterfvARB_remap_index 48
#define GetProgramLocalParameterdvARB_remap_index 49
#define GetProgramLocalParameterfvARB_remap_index 50
#define GetProgramStringARB_remap_index 51
#define GetProgramivARB_remap_index 52
#define GetVertexAttribdvARB_remap_index 53
#define GetVertexAttribfvARB_remap_index 54
#define GetVertexAttribivARB_remap_index 55
#define ProgramEnvParameter4dARB_remap_index 56
#define ProgramEnvParameter4dvARB_remap_index 57
#define ProgramEnvParameter4fARB_remap_index 58
#define ProgramEnvParameter4fvARB_remap_index 59
#define ProgramLocalParameter4dARB_remap_index 60
#define ProgramLocalParameter4dvARB_remap_index 61
#define ProgramLocalParameter4fARB_remap_index 62
#define ProgramLocalParameter4fvARB_remap_index 63
#define ProgramStringARB_remap_index 64
#define VertexAttrib1dARB_remap_index 65
#define VertexAttrib1dvARB_remap_index 66
#define VertexAttrib1fARB_remap_index 67
#define VertexAttrib1fvARB_remap_index 68
#define VertexAttrib1sARB_remap_index 69
#define VertexAttrib1svARB_remap_index 70
#define VertexAttrib2dARB_remap_index 71
#define VertexAttrib2dvARB_remap_index 72
#define VertexAttrib2fARB_remap_index 73
#define VertexAttrib2fvARB_remap_index 74
#define VertexAttrib2sARB_remap_index 75
#define VertexAttrib2svARB_remap_index 76
#define VertexAttrib3dARB_remap_index 77
#define VertexAttrib3dvARB_remap_index 78
#define VertexAttrib3fARB_remap_index 79
#define VertexAttrib3fvARB_remap_index 80
#define VertexAttrib3sARB_remap_index 81
#define VertexAttrib3svARB_remap_index 82
#define VertexAttrib4NbvARB_remap_index 83
#define VertexAttrib4NivARB_remap_index 84
#define VertexAttrib4NsvARB_remap_index 85
#define VertexAttrib4NubARB_remap_index 86
#define VertexAttrib4NubvARB_remap_index 87
#define VertexAttrib4NuivARB_remap_index 88
#define VertexAttrib4NusvARB_remap_index 89
#define VertexAttrib4bvARB_remap_index 90
#define VertexAttrib4dARB_remap_index 91
#define VertexAttrib4dvARB_remap_index 92
#define VertexAttrib4fARB_remap_index 93
#define VertexAttrib4fvARB_remap_index 94
#define VertexAttrib4ivARB_remap_index 95
#define VertexAttrib4sARB_remap_index 96
#define VertexAttrib4svARB_remap_index 97
#define VertexAttrib4ubvARB_remap_index 98
#define VertexAttrib4uivARB_remap_index 99
#define VertexAttrib4usvARB_remap_index 100
#define VertexAttribPointerARB_remap_index 101
#define BindBufferARB_remap_index 102
#define BufferDataARB_remap_index 103
#define BufferSubDataARB_remap_index 104
#define DeleteBuffersARB_remap_index 105
#define GenBuffersARB_remap_index 106
#define GetBufferParameterivARB_remap_index 107
#define GetBufferPointervARB_remap_index 108
#define GetBufferSubDataARB_remap_index 109
#define IsBufferARB_remap_index 110
#define MapBufferARB_remap_index 111
#define UnmapBufferARB_remap_index 112
#define BeginQueryARB_remap_index 113
#define DeleteQueriesARB_remap_index 114
#define EndQueryARB_remap_index 115
#define GenQueriesARB_remap_index 116
#define GetQueryObjectivARB_remap_index 117
#define GetQueryObjectuivARB_remap_index 118
#define GetQueryivARB_remap_index 119
#define IsQueryARB_remap_index 120
#define AttachObjectARB_remap_index 121
#define CompileShaderARB_remap_index 122
#define CreateProgramObjectARB_remap_index 123
#define CreateShaderObjectARB_remap_index 124
#define DeleteObjectARB_remap_index 125
#define DetachObjectARB_remap_index 126
#define GetActiveUniformARB_remap_index 127
#define GetAttachedObjectsARB_remap_index 128
#define GetHandleARB_remap_index 129
#define GetInfoLogARB_remap_index 130
#define GetObjectParameterfvARB_remap_index 131
#define GetObjectParameterivARB_remap_index 132
#define GetShaderSourceARB_remap_index 133
#define GetUniformLocationARB_remap_index 134
#define GetUniformfvARB_remap_index 135
#define GetUniformivARB_remap_index 136
#define LinkProgramARB_remap_index 137
#define ShaderSourceARB_remap_index 138
#define Uniform1fARB_remap_index 139
#define Uniform1fvARB_remap_index 140
#define Uniform1iARB_remap_index 141
#define Uniform1ivARB_remap_index 142
#define Uniform2fARB_remap_index 143
#define Uniform2fvARB_remap_index 144
#define Uniform2iARB_remap_index 145
#define Uniform2ivARB_remap_index 146
#define Uniform3fARB_remap_index 147
#define Uniform3fvARB_remap_index 148
#define Uniform3iARB_remap_index 149
#define Uniform3ivARB_remap_index 150
#define Uniform4fARB_remap_index 151
#define Uniform4fvARB_remap_index 152
#define Uniform4iARB_remap_index 153
#define Uniform4ivARB_remap_index 154
#define UniformMatrix2fvARB_remap_index 155
#define UniformMatrix3fvARB_remap_index 156
#define UniformMatrix4fvARB_remap_index 157
#define UseProgramObjectARB_remap_index 158
#define ValidateProgramARB_remap_index 159
#define BindAttribLocationARB_remap_index 160
#define GetActiveAttribARB_remap_index 161
#define GetAttribLocationARB_remap_index 162
#define DrawBuffersARB_remap_index 163
#define ClampColorARB_remap_index 164
#define DrawArraysInstancedARB_remap_index 165
#define DrawElementsInstancedARB_remap_index 166
#define RenderbufferStorageMultisample_remap_index 167
#define FramebufferTextureARB_remap_index 168
#define FramebufferTextureFaceARB_remap_index 169
#define ProgramParameteriARB_remap_index 170
#define VertexAttribDivisorARB_remap_index 171
#define FlushMappedBufferRange_remap_index 172
#define MapBufferRange_remap_index 173
#define TexBufferARB_remap_index 174
#define BindVertexArray_remap_index 175
#define GenVertexArrays_remap_index 176
#define CopyBufferSubData_remap_index 177
#define ClientWaitSync_remap_index 178
#define DeleteSync_remap_index 179
#define FenceSync_remap_index 180
#define GetInteger64v_remap_index 181
#define GetSynciv_remap_index 182
#define IsSync_remap_index 183
#define WaitSync_remap_index 184
#define DrawElementsBaseVertex_remap_index 185
#define DrawElementsInstancedBaseVertex_remap_index 186
#define DrawRangeElementsBaseVertex_remap_index 187
#define MultiDrawElementsBaseVertex_remap_index 188
#define BlendEquationSeparateiARB_remap_index 189
#define BlendEquationiARB_remap_index 190
#define BlendFuncSeparateiARB_remap_index 191
#define BlendFunciARB_remap_index 192
#define BindSampler_remap_index 193
#define DeleteSamplers_remap_index 194
#define GenSamplers_remap_index 195
#define GetSamplerParameterIiv_remap_index 196
#define GetSamplerParameterIuiv_remap_index 197
#define GetSamplerParameterfv_remap_index 198
#define GetSamplerParameteriv_remap_index 199
#define IsSampler_remap_index 200
#define SamplerParameterIiv_remap_index 201
#define SamplerParameterIuiv_remap_index 202
#define SamplerParameterf_remap_index 203
#define SamplerParameterfv_remap_index 204
#define SamplerParameteri_remap_index 205
#define SamplerParameteriv_remap_index 206
#define ColorP3ui_remap_index 207
#define ColorP3uiv_remap_index 208
#define ColorP4ui_remap_index 209
#define ColorP4uiv_remap_index 210
#define MultiTexCoordP1ui_remap_index 211
#define MultiTexCoordP1uiv_remap_index 212
#define MultiTexCoordP2ui_remap_index 213
#define MultiTexCoordP2uiv_remap_index 214
#define MultiTexCoordP3ui_remap_index 215
#define MultiTexCoordP3uiv_remap_index 216
#define MultiTexCoordP4ui_remap_index 217
#define MultiTexCoordP4uiv_remap_index 218
#define NormalP3ui_remap_index 219
#define NormalP3uiv_remap_index 220
#define SecondaryColorP3ui_remap_index 221
#define SecondaryColorP3uiv_remap_index 222
#define TexCoordP1ui_remap_index 223
#define TexCoordP1uiv_remap_index 224
#define TexCoordP2ui_remap_index 225
#define TexCoordP2uiv_remap_index 226
#define TexCoordP3ui_remap_index 227
#define TexCoordP3uiv_remap_index 228
#define TexCoordP4ui_remap_index 229
#define TexCoordP4uiv_remap_index 230
#define VertexAttribP1ui_remap_index 231
#define VertexAttribP1uiv_remap_index 232
#define VertexAttribP2ui_remap_index 233
#define VertexAttribP2uiv_remap_index 234
#define VertexAttribP3ui_remap_index 235
#define VertexAttribP3uiv_remap_index 236
#define VertexAttribP4ui_remap_index 237
#define VertexAttribP4uiv_remap_index 238
#define VertexP2ui_remap_index 239
#define VertexP2uiv_remap_index 240
#define VertexP3ui_remap_index 241
#define VertexP3uiv_remap_index 242
#define VertexP4ui_remap_index 243
#define VertexP4uiv_remap_index 244
#define BindTransformFeedback_remap_index 245
#define DeleteTransformFeedbacks_remap_index 246
#define DrawTransformFeedback_remap_index 247
#define GenTransformFeedbacks_remap_index 248
#define IsTransformFeedback_remap_index 249
#define PauseTransformFeedback_remap_index 250
#define ResumeTransformFeedback_remap_index 251
#define ClearDepthf_remap_index 252
#define DepthRangef_remap_index 253
#define GetShaderPrecisionFormat_remap_index 254
#define ReleaseShaderCompiler_remap_index 255
#define ShaderBinary_remap_index 256
#define GetGraphicsResetStatusARB_remap_index 257
#define GetnColorTableARB_remap_index 258
#define GetnCompressedTexImageARB_remap_index 259
#define GetnConvolutionFilterARB_remap_index 260
#define GetnHistogramARB_remap_index 261
#define GetnMapdvARB_remap_index 262
#define GetnMapfvARB_remap_index 263
#define GetnMapivARB_remap_index 264
#define GetnMinmaxARB_remap_index 265
#define GetnPixelMapfvARB_remap_index 266
#define GetnPixelMapuivARB_remap_index 267
#define GetnPixelMapusvARB_remap_index 268
#define GetnPolygonStippleARB_remap_index 269
#define GetnSeparableFilterARB_remap_index 270
#define GetnTexImageARB_remap_index 271
#define GetnUniformdvARB_remap_index 272
#define GetnUniformfvARB_remap_index 273
#define GetnUniformivARB_remap_index 274
#define GetnUniformuivARB_remap_index 275
#define ReadnPixelsARB_remap_index 276
#define TexStorage1D_remap_index 277
#define TexStorage2D_remap_index 278
#define TexStorage3D_remap_index 279
#define TextureStorage1DEXT_remap_index 280
#define TextureStorage2DEXT_remap_index 281
#define TextureStorage3DEXT_remap_index 282
#define PolygonOffsetEXT_remap_index 283
#define GetPixelTexGenParameterfvSGIS_remap_index 284
#define GetPixelTexGenParameterivSGIS_remap_index 285
#define PixelTexGenParameterfSGIS_remap_index 286
#define PixelTexGenParameterfvSGIS_remap_index 287
#define PixelTexGenParameteriSGIS_remap_index 288
#define PixelTexGenParameterivSGIS_remap_index 289
#define SampleMaskSGIS_remap_index 290
#define SamplePatternSGIS_remap_index 291
#define ColorPointerEXT_remap_index 292
#define EdgeFlagPointerEXT_remap_index 293
#define IndexPointerEXT_remap_index 294
#define NormalPointerEXT_remap_index 295
#define TexCoordPointerEXT_remap_index 296
#define VertexPointerEXT_remap_index 297
#define PointParameterfEXT_remap_index 298
#define PointParameterfvEXT_remap_index 299
#define LockArraysEXT_remap_index 300
#define UnlockArraysEXT_remap_index 301
#define SecondaryColor3bEXT_remap_index 302
#define SecondaryColor3bvEXT_remap_index 303
#define SecondaryColor3dEXT_remap_index 304
#define SecondaryColor3dvEXT_remap_index 305
#define SecondaryColor3fEXT_remap_index 306
#define SecondaryColor3fvEXT_remap_index 307
#define SecondaryColor3iEXT_remap_index 308
#define SecondaryColor3ivEXT_remap_index 309
#define SecondaryColor3sEXT_remap_index 310
#define SecondaryColor3svEXT_remap_index 311
#define SecondaryColor3ubEXT_remap_index 312
#define SecondaryColor3ubvEXT_remap_index 313
#define SecondaryColor3uiEXT_remap_index 314
#define SecondaryColor3uivEXT_remap_index 315
#define SecondaryColor3usEXT_remap_index 316
#define SecondaryColor3usvEXT_remap_index 317
#define SecondaryColorPointerEXT_remap_index 318
#define MultiDrawArraysEXT_remap_index 319
#define MultiDrawElementsEXT_remap_index 320
#define FogCoordPointerEXT_remap_index 321
#define FogCoorddEXT_remap_index 322
#define FogCoorddvEXT_remap_index 323
#define FogCoordfEXT_remap_index 324
#define FogCoordfvEXT_remap_index 325
#define PixelTexGenSGIX_remap_index 326
#define BlendFuncSeparateEXT_remap_index 327
#define FlushVertexArrayRangeNV_remap_index 328
#define VertexArrayRangeNV_remap_index 329
#define CombinerInputNV_remap_index 330
#define CombinerOutputNV_remap_index 331
#define CombinerParameterfNV_remap_index 332
#define CombinerParameterfvNV_remap_index 333
#define CombinerParameteriNV_remap_index 334
#define CombinerParameterivNV_remap_index 335
#define FinalCombinerInputNV_remap_index 336
#define GetCombinerInputParameterfvNV_remap_index 337
#define GetCombinerInputParameterivNV_remap_index 338
#define GetCombinerOutputParameterfvNV_remap_index 339
#define GetCombinerOutputParameterivNV_remap_index 340
#define GetFinalCombinerInputParameterfvNV_remap_index 341
#define GetFinalCombinerInputParameterivNV_remap_index 342
#define ResizeBuffersMESA_remap_index 343
#define WindowPos2dMESA_remap_index 344
#define WindowPos2dvMESA_remap_index 345
#define WindowPos2fMESA_remap_index 346
#define WindowPos2fvMESA_remap_index 347
#define WindowPos2iMESA_remap_index 348
#define WindowPos2ivMESA_remap_index 349
#define WindowPos2sMESA_remap_index 350
#define WindowPos2svMESA_remap_index 351
#define WindowPos3dMESA_remap_index 352
#define WindowPos3dvMESA_remap_index 353
#define WindowPos3fMESA_remap_index 354
#define WindowPos3fvMESA_remap_index 355
#define WindowPos3iMESA_remap_index 356
#define WindowPos3ivMESA_remap_index 357
#define WindowPos3sMESA_remap_index 358
#define WindowPos3svMESA_remap_index 359
#define WindowPos4dMESA_remap_index 360
#define WindowPos4dvMESA_remap_index 361
#define WindowPos4fMESA_remap_index 362
#define WindowPos4fvMESA_remap_index 363
#define WindowPos4iMESA_remap_index 364
#define WindowPos4ivMESA_remap_index 365
#define WindowPos4sMESA_remap_index 366
#define WindowPos4svMESA_remap_index 367
#define MultiModeDrawArraysIBM_remap_index 368
#define MultiModeDrawElementsIBM_remap_index 369
#define DeleteFencesNV_remap_index 370
#define FinishFenceNV_remap_index 371
#define GenFencesNV_remap_index 372
#define GetFenceivNV_remap_index 373
#define IsFenceNV_remap_index 374
#define SetFenceNV_remap_index 375
#define TestFenceNV_remap_index 376
#define AreProgramsResidentNV_remap_index 377
#define BindProgramNV_remap_index 378
#define DeleteProgramsNV_remap_index 379
#define ExecuteProgramNV_remap_index 380
#define GenProgramsNV_remap_index 381
#define GetProgramParameterdvNV_remap_index 382
#define GetProgramParameterfvNV_remap_index 383
#define GetProgramStringNV_remap_index 384
#define GetProgramivNV_remap_index 385
#define GetTrackMatrixivNV_remap_index 386
#define GetVertexAttribPointervNV_remap_index 387
#define GetVertexAttribdvNV_remap_index 388
#define GetVertexAttribfvNV_remap_index 389
#define GetVertexAttribivNV_remap_index 390
#define IsProgramNV_remap_index 391
#define LoadProgramNV_remap_index 392
#define ProgramParameters4dvNV_remap_index 393
#define ProgramParameters4fvNV_remap_index 394
#define RequestResidentProgramsNV_remap_index 395
#define TrackMatrixNV_remap_index 396
#define VertexAttrib1dNV_remap_index 397
#define VertexAttrib1dvNV_remap_index 398
#define VertexAttrib1fNV_remap_index 399
#define VertexAttrib1fvNV_remap_index 400
#define VertexAttrib1sNV_remap_index 401
#define VertexAttrib1svNV_remap_index 402
#define VertexAttrib2dNV_remap_index 403
#define VertexAttrib2dvNV_remap_index 404
#define VertexAttrib2fNV_remap_index 405
#define VertexAttrib2fvNV_remap_index 406
#define VertexAttrib2sNV_remap_index 407
#define VertexAttrib2svNV_remap_index 408
#define VertexAttrib3dNV_remap_index 409
#define VertexAttrib3dvNV_remap_index 410
#define VertexAttrib3fNV_remap_index 411
#define VertexAttrib3fvNV_remap_index 412
#define VertexAttrib3sNV_remap_index 413
#define VertexAttrib3svNV_remap_index 414
#define VertexAttrib4dNV_remap_index 415
#define VertexAttrib4dvNV_remap_index 416
#define VertexAttrib4fNV_remap_index 417
#define VertexAttrib4fvNV_remap_index 418
#define VertexAttrib4sNV_remap_index 419
#define VertexAttrib4svNV_remap_index 420
#define VertexAttrib4ubNV_remap_index 421
#define VertexAttrib4ubvNV_remap_index 422
#define VertexAttribPointerNV_remap_index 423
#define VertexAttribs1dvNV_remap_index 424
#define VertexAttribs1fvNV_remap_index 425
#define VertexAttribs1svNV_remap_index 426
#define VertexAttribs2dvNV_remap_index 427
#define VertexAttribs2fvNV_remap_index 428
#define VertexAttribs2svNV_remap_index 429
#define VertexAttribs3dvNV_remap_index 430
#define VertexAttribs3fvNV_remap_index 431
#define VertexAttribs3svNV_remap_index 432
#define VertexAttribs4dvNV_remap_index 433
#define VertexAttribs4fvNV_remap_index 434
#define VertexAttribs4svNV_remap_index 435
#define VertexAttribs4ubvNV_remap_index 436
#define GetTexBumpParameterfvATI_remap_index 437
#define GetTexBumpParameterivATI_remap_index 438
#define TexBumpParameterfvATI_remap_index 439
#define TexBumpParameterivATI_remap_index 440
#define AlphaFragmentOp1ATI_remap_index 441
#define AlphaFragmentOp2ATI_remap_index 442
#define AlphaFragmentOp3ATI_remap_index 443
#define BeginFragmentShaderATI_remap_index 444
#define BindFragmentShaderATI_remap_index 445
#define ColorFragmentOp1ATI_remap_index 446
#define ColorFragmentOp2ATI_remap_index 447
#define ColorFragmentOp3ATI_remap_index 448
#define DeleteFragmentShaderATI_remap_index 449
#define EndFragmentShaderATI_remap_index 450
#define GenFragmentShadersATI_remap_index 451
#define PassTexCoordATI_remap_index 452
#define SampleMapATI_remap_index 453
#define SetFragmentShaderConstantATI_remap_index 454
#define PointParameteriNV_remap_index 455
#define PointParameterivNV_remap_index 456
#define ActiveStencilFaceEXT_remap_index 457
#define BindVertexArrayAPPLE_remap_index 458
#define DeleteVertexArraysAPPLE_remap_index 459
#define GenVertexArraysAPPLE_remap_index 460
#define IsVertexArrayAPPLE_remap_index 461
#define GetProgramNamedParameterdvNV_remap_index 462
#define GetProgramNamedParameterfvNV_remap_index 463
#define ProgramNamedParameter4dNV_remap_index 464
#define ProgramNamedParameter4dvNV_remap_index 465
#define ProgramNamedParameter4fNV_remap_index 466
#define ProgramNamedParameter4fvNV_remap_index 467
#define PrimitiveRestartIndexNV_remap_index 468
#define PrimitiveRestartNV_remap_index 469
#define DepthBoundsEXT_remap_index 470
#define BlendEquationSeparateEXT_remap_index 471
#define BindFramebufferEXT_remap_index 472
#define BindRenderbufferEXT_remap_index 473
#define CheckFramebufferStatusEXT_remap_index 474
#define DeleteFramebuffersEXT_remap_index 475
#define DeleteRenderbuffersEXT_remap_index 476
#define FramebufferRenderbufferEXT_remap_index 477
#define FramebufferTexture1DEXT_remap_index 478
#define FramebufferTexture2DEXT_remap_index 479
#define FramebufferTexture3DEXT_remap_index 480
#define GenFramebuffersEXT_remap_index 481
#define GenRenderbuffersEXT_remap_index 482
#define GenerateMipmapEXT_remap_index 483
#define GetFramebufferAttachmentParameterivEXT_remap_index 484
#define GetRenderbufferParameterivEXT_remap_index 485
#define IsFramebufferEXT_remap_index 486
#define IsRenderbufferEXT_remap_index 487
#define RenderbufferStorageEXT_remap_index 488
#define BlitFramebufferEXT_remap_index 489
#define BufferParameteriAPPLE_remap_index 490
#define FlushMappedBufferRangeAPPLE_remap_index 491
#define BindFragDataLocationEXT_remap_index 492
#define GetFragDataLocationEXT_remap_index 493
#define GetUniformuivEXT_remap_index 494
#define GetVertexAttribIivEXT_remap_index 495
#define GetVertexAttribIuivEXT_remap_index 496
#define Uniform1uiEXT_remap_index 497
#define Uniform1uivEXT_remap_index 498
#define Uniform2uiEXT_remap_index 499
#define Uniform2uivEXT_remap_index 500
#define Uniform3uiEXT_remap_index 501
#define Uniform3uivEXT_remap_index 502
#define Uniform4uiEXT_remap_index 503
#define Uniform4uivEXT_remap_index 504
#define VertexAttribI1iEXT_remap_index 505
#define VertexAttribI1ivEXT_remap_index 506
#define VertexAttribI1uiEXT_remap_index 507
#define VertexAttribI1uivEXT_remap_index 508
#define VertexAttribI2iEXT_remap_index 509
#define VertexAttribI2ivEXT_remap_index 510
#define VertexAttribI2uiEXT_remap_index 511
#define VertexAttribI2uivEXT_remap_index 512
#define VertexAttribI3iEXT_remap_index 513
#define VertexAttribI3ivEXT_remap_index 514
#define VertexAttribI3uiEXT_remap_index 515
#define VertexAttribI3uivEXT_remap_index 516
#define VertexAttribI4bvEXT_remap_index 517
#define VertexAttribI4iEXT_remap_index 518
#define VertexAttribI4ivEXT_remap_index 519
#define VertexAttribI4svEXT_remap_index 520
#define VertexAttribI4ubvEXT_remap_index 521
#define VertexAttribI4uiEXT_remap_index 522
#define VertexAttribI4uivEXT_remap_index 523
#define VertexAttribI4usvEXT_remap_index 524
#define VertexAttribIPointerEXT_remap_index 525
#define FramebufferTextureLayerEXT_remap_index 526
#define ColorMaskIndexedEXT_remap_index 527
#define DisableIndexedEXT_remap_index 528
#define EnableIndexedEXT_remap_index 529
#define GetBooleanIndexedvEXT_remap_index 530
#define GetIntegerIndexedvEXT_remap_index 531
#define IsEnabledIndexedEXT_remap_index 532
#define ClearColorIiEXT_remap_index 533
#define ClearColorIuiEXT_remap_index 534
#define GetTexParameterIivEXT_remap_index 535
#define GetTexParameterIuivEXT_remap_index 536
#define TexParameterIivEXT_remap_index 537
#define TexParameterIuivEXT_remap_index 538
#define BeginConditionalRenderNV_remap_index 539
#define EndConditionalRenderNV_remap_index 540
#define BeginTransformFeedbackEXT_remap_index 541
#define BindBufferBaseEXT_remap_index 542
#define BindBufferOffsetEXT_remap_index 543
#define BindBufferRangeEXT_remap_index 544
#define EndTransformFeedbackEXT_remap_index 545
#define GetTransformFeedbackVaryingEXT_remap_index 546
#define TransformFeedbackVaryingsEXT_remap_index 547
#define ProvokingVertexEXT_remap_index 548
#define GetTexParameterPointervAPPLE_remap_index 549
#define TextureRangeAPPLE_remap_index 550
#define GetObjectParameterivAPPLE_remap_index 551
#define ObjectPurgeableAPPLE_remap_index 552
#define ObjectUnpurgeableAPPLE_remap_index 553
#define ActiveProgramEXT_remap_index 554
#define CreateShaderProgramEXT_remap_index 555
#define UseShaderProgramEXT_remap_index 556
#define TextureBarrierNV_remap_index 557
#define StencilFuncSeparateATI_remap_index 558
#define ProgramEnvParameters4fvEXT_remap_index 559
#define ProgramLocalParameters4fvEXT_remap_index 560
#define GetQueryObjecti64vEXT_remap_index 561
#define GetQueryObjectui64vEXT_remap_index 562
#define EGLImageTargetRenderbufferStorageOES_remap_index 563
#define EGLImageTargetTexture2DOES_remap_index 564

#define _gloffset_AttachShader driDispatchRemapTable[AttachShader_remap_index]
#define _gloffset_CreateProgram driDispatchRemapTable[CreateProgram_remap_index]
#define _gloffset_CreateShader driDispatchRemapTable[CreateShader_remap_index]
#define _gloffset_DeleteProgram driDispatchRemapTable[DeleteProgram_remap_index]
#define _gloffset_DeleteShader driDispatchRemapTable[DeleteShader_remap_index]
#define _gloffset_DetachShader driDispatchRemapTable[DetachShader_remap_index]
#define _gloffset_GetAttachedShaders driDispatchRemapTable[GetAttachedShaders_remap_index]
#define _gloffset_GetProgramInfoLog driDispatchRemapTable[GetProgramInfoLog_remap_index]
#define _gloffset_GetProgramiv driDispatchRemapTable[GetProgramiv_remap_index]
#define _gloffset_GetShaderInfoLog driDispatchRemapTable[GetShaderInfoLog_remap_index]
#define _gloffset_GetShaderiv driDispatchRemapTable[GetShaderiv_remap_index]
#define _gloffset_IsProgram driDispatchRemapTable[IsProgram_remap_index]
#define _gloffset_IsShader driDispatchRemapTable[IsShader_remap_index]
#define _gloffset_StencilFuncSeparate driDispatchRemapTable[StencilFuncSeparate_remap_index]
#define _gloffset_StencilMaskSeparate driDispatchRemapTable[StencilMaskSeparate_remap_index]
#define _gloffset_StencilOpSeparate driDispatchRemapTable[StencilOpSeparate_remap_index]
#define _gloffset_UniformMatrix2x3fv driDispatchRemapTable[UniformMatrix2x3fv_remap_index]
#define _gloffset_UniformMatrix2x4fv driDispatchRemapTable[UniformMatrix2x4fv_remap_index]
#define _gloffset_UniformMatrix3x2fv driDispatchRemapTable[UniformMatrix3x2fv_remap_index]
#define _gloffset_UniformMatrix3x4fv driDispatchRemapTable[UniformMatrix3x4fv_remap_index]
#define _gloffset_UniformMatrix4x2fv driDispatchRemapTable[UniformMatrix4x2fv_remap_index]
#define _gloffset_UniformMatrix4x3fv driDispatchRemapTable[UniformMatrix4x3fv_remap_index]
#define _gloffset_ClampColor driDispatchRemapTable[ClampColor_remap_index]
#define _gloffset_ClearBufferfi driDispatchRemapTable[ClearBufferfi_remap_index]
#define _gloffset_ClearBufferfv driDispatchRemapTable[ClearBufferfv_remap_index]
#define _gloffset_ClearBufferiv driDispatchRemapTable[ClearBufferiv_remap_index]
#define _gloffset_ClearBufferuiv driDispatchRemapTable[ClearBufferuiv_remap_index]
#define _gloffset_GetStringi driDispatchRemapTable[GetStringi_remap_index]
#define _gloffset_TexBuffer driDispatchRemapTable[TexBuffer_remap_index]
#define _gloffset_FramebufferTexture driDispatchRemapTable[FramebufferTexture_remap_index]
#define _gloffset_GetBufferParameteri64v driDispatchRemapTable[GetBufferParameteri64v_remap_index]
#define _gloffset_GetInteger64i_v driDispatchRemapTable[GetInteger64i_v_remap_index]
#define _gloffset_VertexAttribDivisor driDispatchRemapTable[VertexAttribDivisor_remap_index]
#define _gloffset_LoadTransposeMatrixdARB driDispatchRemapTable[LoadTransposeMatrixdARB_remap_index]
#define _gloffset_LoadTransposeMatrixfARB driDispatchRemapTable[LoadTransposeMatrixfARB_remap_index]
#define _gloffset_MultTransposeMatrixdARB driDispatchRemapTable[MultTransposeMatrixdARB_remap_index]
#define _gloffset_MultTransposeMatrixfARB driDispatchRemapTable[MultTransposeMatrixfARB_remap_index]
#define _gloffset_SampleCoverageARB driDispatchRemapTable[SampleCoverageARB_remap_index]
#define _gloffset_CompressedTexImage1DARB driDispatchRemapTable[CompressedTexImage1DARB_remap_index]
#define _gloffset_CompressedTexImage2DARB driDispatchRemapTable[CompressedTexImage2DARB_remap_index]
#define _gloffset_CompressedTexImage3DARB driDispatchRemapTable[CompressedTexImage3DARB_remap_index]
#define _gloffset_CompressedTexSubImage1DARB driDispatchRemapTable[CompressedTexSubImage1DARB_remap_index]
#define _gloffset_CompressedTexSubImage2DARB driDispatchRemapTable[CompressedTexSubImage2DARB_remap_index]
#define _gloffset_CompressedTexSubImage3DARB driDispatchRemapTable[CompressedTexSubImage3DARB_remap_index]
#define _gloffset_GetCompressedTexImageARB driDispatchRemapTable[GetCompressedTexImageARB_remap_index]
#define _gloffset_DisableVertexAttribArrayARB driDispatchRemapTable[DisableVertexAttribArrayARB_remap_index]
#define _gloffset_EnableVertexAttribArrayARB driDispatchRemapTable[EnableVertexAttribArrayARB_remap_index]
#define _gloffset_GetProgramEnvParameterdvARB driDispatchRemapTable[GetProgramEnvParameterdvARB_remap_index]
#define _gloffset_GetProgramEnvParameterfvARB driDispatchRemapTable[GetProgramEnvParameterfvARB_remap_index]
#define _gloffset_GetProgramLocalParameterdvARB driDispatchRemapTable[GetProgramLocalParameterdvARB_remap_index]
#define _gloffset_GetProgramLocalParameterfvARB driDispatchRemapTable[GetProgramLocalParameterfvARB_remap_index]
#define _gloffset_GetProgramStringARB driDispatchRemapTable[GetProgramStringARB_remap_index]
#define _gloffset_GetProgramivARB driDispatchRemapTable[GetProgramivARB_remap_index]
#define _gloffset_GetVertexAttribdvARB driDispatchRemapTable[GetVertexAttribdvARB_remap_index]
#define _gloffset_GetVertexAttribfvARB driDispatchRemapTable[GetVertexAttribfvARB_remap_index]
#define _gloffset_GetVertexAttribivARB driDispatchRemapTable[GetVertexAttribivARB_remap_index]
#define _gloffset_ProgramEnvParameter4dARB driDispatchRemapTable[ProgramEnvParameter4dARB_remap_index]
#define _gloffset_ProgramEnvParameter4dvARB driDispatchRemapTable[ProgramEnvParameter4dvARB_remap_index]
#define _gloffset_ProgramEnvParameter4fARB driDispatchRemapTable[ProgramEnvParameter4fARB_remap_index]
#define _gloffset_ProgramEnvParameter4fvARB driDispatchRemapTable[ProgramEnvParameter4fvARB_remap_index]
#define _gloffset_ProgramLocalParameter4dARB driDispatchRemapTable[ProgramLocalParameter4dARB_remap_index]
#define _gloffset_ProgramLocalParameter4dvARB driDispatchRemapTable[ProgramLocalParameter4dvARB_remap_index]
#define _gloffset_ProgramLocalParameter4fARB driDispatchRemapTable[ProgramLocalParameter4fARB_remap_index]
#define _gloffset_ProgramLocalParameter4fvARB driDispatchRemapTable[ProgramLocalParameter4fvARB_remap_index]
#define _gloffset_ProgramStringARB driDispatchRemapTable[ProgramStringARB_remap_index]
#define _gloffset_VertexAttrib1dARB driDispatchRemapTable[VertexAttrib1dARB_remap_index]
#define _gloffset_VertexAttrib1dvARB driDispatchRemapTable[VertexAttrib1dvARB_remap_index]
#define _gloffset_VertexAttrib1fARB driDispatchRemapTable[VertexAttrib1fARB_remap_index]
#define _gloffset_VertexAttrib1fvARB driDispatchRemapTable[VertexAttrib1fvARB_remap_index]
#define _gloffset_VertexAttrib1sARB driDispatchRemapTable[VertexAttrib1sARB_remap_index]
#define _gloffset_VertexAttrib1svARB driDispatchRemapTable[VertexAttrib1svARB_remap_index]
#define _gloffset_VertexAttrib2dARB driDispatchRemapTable[VertexAttrib2dARB_remap_index]
#define _gloffset_VertexAttrib2dvARB driDispatchRemapTable[VertexAttrib2dvARB_remap_index]
#define _gloffset_VertexAttrib2fARB driDispatchRemapTable[VertexAttrib2fARB_remap_index]
#define _gloffset_VertexAttrib2fvARB driDispatchRemapTable[VertexAttrib2fvARB_remap_index]
#define _gloffset_VertexAttrib2sARB driDispatchRemapTable[VertexAttrib2sARB_remap_index]
#define _gloffset_VertexAttrib2svARB driDispatchRemapTable[VertexAttrib2svARB_remap_index]
#define _gloffset_VertexAttrib3dARB driDispatchRemapTable[VertexAttrib3dARB_remap_index]
#define _gloffset_VertexAttrib3dvARB driDispatchRemapTable[VertexAttrib3dvARB_remap_index]
#define _gloffset_VertexAttrib3fARB driDispatchRemapTable[VertexAttrib3fARB_remap_index]
#define _gloffset_VertexAttrib3fvARB driDispatchRemapTable[VertexAttrib3fvARB_remap_index]
#define _gloffset_VertexAttrib3sARB driDispatchRemapTable[VertexAttrib3sARB_remap_index]
#define _gloffset_VertexAttrib3svARB driDispatchRemapTable[VertexAttrib3svARB_remap_index]
#define _gloffset_VertexAttrib4NbvARB driDispatchRemapTable[VertexAttrib4NbvARB_remap_index]
#define _gloffset_VertexAttrib4NivARB driDispatchRemapTable[VertexAttrib4NivARB_remap_index]
#define _gloffset_VertexAttrib4NsvARB driDispatchRemapTable[VertexAttrib4NsvARB_remap_index]
#define _gloffset_VertexAttrib4NubARB driDispatchRemapTable[VertexAttrib4NubARB_remap_index]
#define _gloffset_VertexAttrib4NubvARB driDispatchRemapTable[VertexAttrib4NubvARB_remap_index]
#define _gloffset_VertexAttrib4NuivARB driDispatchRemapTable[VertexAttrib4NuivARB_remap_index]
#define _gloffset_VertexAttrib4NusvARB driDispatchRemapTable[VertexAttrib4NusvARB_remap_index]
#define _gloffset_VertexAttrib4bvARB driDispatchRemapTable[VertexAttrib4bvARB_remap_index]
#define _gloffset_VertexAttrib4dARB driDispatchRemapTable[VertexAttrib4dARB_remap_index]
#define _gloffset_VertexAttrib4dvARB driDispatchRemapTable[VertexAttrib4dvARB_remap_index]
#define _gloffset_VertexAttrib4fARB driDispatchRemapTable[VertexAttrib4fARB_remap_index]
#define _gloffset_VertexAttrib4fvARB driDispatchRemapTable[VertexAttrib4fvARB_remap_index]
#define _gloffset_VertexAttrib4ivARB driDispatchRemapTable[VertexAttrib4ivARB_remap_index]
#define _gloffset_VertexAttrib4sARB driDispatchRemapTable[VertexAttrib4sARB_remap_index]
#define _gloffset_VertexAttrib4svARB driDispatchRemapTable[VertexAttrib4svARB_remap_index]
#define _gloffset_VertexAttrib4ubvARB driDispatchRemapTable[VertexAttrib4ubvARB_remap_index]
#define _gloffset_VertexAttrib4uivARB driDispatchRemapTable[VertexAttrib4uivARB_remap_index]
#define _gloffset_VertexAttrib4usvARB driDispatchRemapTable[VertexAttrib4usvARB_remap_index]
#define _gloffset_VertexAttribPointerARB driDispatchRemapTable[VertexAttribPointerARB_remap_index]
#define _gloffset_BindBufferARB driDispatchRemapTable[BindBufferARB_remap_index]
#define _gloffset_BufferDataARB driDispatchRemapTable[BufferDataARB_remap_index]
#define _gloffset_BufferSubDataARB driDispatchRemapTable[BufferSubDataARB_remap_index]
#define _gloffset_DeleteBuffersARB driDispatchRemapTable[DeleteBuffersARB_remap_index]
#define _gloffset_GenBuffersARB driDispatchRemapTable[GenBuffersARB_remap_index]
#define _gloffset_GetBufferParameterivARB driDispatchRemapTable[GetBufferParameterivARB_remap_index]
#define _gloffset_GetBufferPointervARB driDispatchRemapTable[GetBufferPointervARB_remap_index]
#define _gloffset_GetBufferSubDataARB driDispatchRemapTable[GetBufferSubDataARB_remap_index]
#define _gloffset_IsBufferARB driDispatchRemapTable[IsBufferARB_remap_index]
#define _gloffset_MapBufferARB driDispatchRemapTable[MapBufferARB_remap_index]
#define _gloffset_UnmapBufferARB driDispatchRemapTable[UnmapBufferARB_remap_index]
#define _gloffset_BeginQueryARB driDispatchRemapTable[BeginQueryARB_remap_index]
#define _gloffset_DeleteQueriesARB driDispatchRemapTable[DeleteQueriesARB_remap_index]
#define _gloffset_EndQueryARB driDispatchRemapTable[EndQueryARB_remap_index]
#define _gloffset_GenQueriesARB driDispatchRemapTable[GenQueriesARB_remap_index]
#define _gloffset_GetQueryObjectivARB driDispatchRemapTable[GetQueryObjectivARB_remap_index]
#define _gloffset_GetQueryObjectuivARB driDispatchRemapTable[GetQueryObjectuivARB_remap_index]
#define _gloffset_GetQueryivARB driDispatchRemapTable[GetQueryivARB_remap_index]
#define _gloffset_IsQueryARB driDispatchRemapTable[IsQueryARB_remap_index]
#define _gloffset_AttachObjectARB driDispatchRemapTable[AttachObjectARB_remap_index]
#define _gloffset_CompileShaderARB driDispatchRemapTable[CompileShaderARB_remap_index]
#define _gloffset_CreateProgramObjectARB driDispatchRemapTable[CreateProgramObjectARB_remap_index]
#define _gloffset_CreateShaderObjectARB driDispatchRemapTable[CreateShaderObjectARB_remap_index]
#define _gloffset_DeleteObjectARB driDispatchRemapTable[DeleteObjectARB_remap_index]
#define _gloffset_DetachObjectARB driDispatchRemapTable[DetachObjectARB_remap_index]
#define _gloffset_GetActiveUniformARB driDispatchRemapTable[GetActiveUniformARB_remap_index]
#define _gloffset_GetAttachedObjectsARB driDispatchRemapTable[GetAttachedObjectsARB_remap_index]
#define _gloffset_GetHandleARB driDispatchRemapTable[GetHandleARB_remap_index]
#define _gloffset_GetInfoLogARB driDispatchRemapTable[GetInfoLogARB_remap_index]
#define _gloffset_GetObjectParameterfvARB driDispatchRemapTable[GetObjectParameterfvARB_remap_index]
#define _gloffset_GetObjectParameterivARB driDispatchRemapTable[GetObjectParameterivARB_remap_index]
#define _gloffset_GetShaderSourceARB driDispatchRemapTable[GetShaderSourceARB_remap_index]
#define _gloffset_GetUniformLocationARB driDispatchRemapTable[GetUniformLocationARB_remap_index]
#define _gloffset_GetUniformfvARB driDispatchRemapTable[GetUniformfvARB_remap_index]
#define _gloffset_GetUniformivARB driDispatchRemapTable[GetUniformivARB_remap_index]
#define _gloffset_LinkProgramARB driDispatchRemapTable[LinkProgramARB_remap_index]
#define _gloffset_ShaderSourceARB driDispatchRemapTable[ShaderSourceARB_remap_index]
#define _gloffset_Uniform1fARB driDispatchRemapTable[Uniform1fARB_remap_index]
#define _gloffset_Uniform1fvARB driDispatchRemapTable[Uniform1fvARB_remap_index]
#define _gloffset_Uniform1iARB driDispatchRemapTable[Uniform1iARB_remap_index]
#define _gloffset_Uniform1ivARB driDispatchRemapTable[Uniform1ivARB_remap_index]
#define _gloffset_Uniform2fARB driDispatchRemapTable[Uniform2fARB_remap_index]
#define _gloffset_Uniform2fvARB driDispatchRemapTable[Uniform2fvARB_remap_index]
#define _gloffset_Uniform2iARB driDispatchRemapTable[Uniform2iARB_remap_index]
#define _gloffset_Uniform2ivARB driDispatchRemapTable[Uniform2ivARB_remap_index]
#define _gloffset_Uniform3fARB driDispatchRemapTable[Uniform3fARB_remap_index]
#define _gloffset_Uniform3fvARB driDispatchRemapTable[Uniform3fvARB_remap_index]
#define _gloffset_Uniform3iARB driDispatchRemapTable[Uniform3iARB_remap_index]
#define _gloffset_Uniform3ivARB driDispatchRemapTable[Uniform3ivARB_remap_index]
#define _gloffset_Uniform4fARB driDispatchRemapTable[Uniform4fARB_remap_index]
#define _gloffset_Uniform4fvARB driDispatchRemapTable[Uniform4fvARB_remap_index]
#define _gloffset_Uniform4iARB driDispatchRemapTable[Uniform4iARB_remap_index]
#define _gloffset_Uniform4ivARB driDispatchRemapTable[Uniform4ivARB_remap_index]
#define _gloffset_UniformMatrix2fvARB driDispatchRemapTable[UniformMatrix2fvARB_remap_index]
#define _gloffset_UniformMatrix3fvARB driDispatchRemapTable[UniformMatrix3fvARB_remap_index]
#define _gloffset_UniformMatrix4fvARB driDispatchRemapTable[UniformMatrix4fvARB_remap_index]
#define _gloffset_UseProgramObjectARB driDispatchRemapTable[UseProgramObjectARB_remap_index]
#define _gloffset_ValidateProgramARB driDispatchRemapTable[ValidateProgramARB_remap_index]
#define _gloffset_BindAttribLocationARB driDispatchRemapTable[BindAttribLocationARB_remap_index]
#define _gloffset_GetActiveAttribARB driDispatchRemapTable[GetActiveAttribARB_remap_index]
#define _gloffset_GetAttribLocationARB driDispatchRemapTable[GetAttribLocationARB_remap_index]
#define _gloffset_DrawBuffersARB driDispatchRemapTable[DrawBuffersARB_remap_index]
#define _gloffset_ClampColorARB driDispatchRemapTable[ClampColorARB_remap_index]
#define _gloffset_DrawArraysInstancedARB driDispatchRemapTable[DrawArraysInstancedARB_remap_index]
#define _gloffset_DrawElementsInstancedARB driDispatchRemapTable[DrawElementsInstancedARB_remap_index]
#define _gloffset_RenderbufferStorageMultisample driDispatchRemapTable[RenderbufferStorageMultisample_remap_index]
#define _gloffset_FramebufferTextureARB driDispatchRemapTable[FramebufferTextureARB_remap_index]
#define _gloffset_FramebufferTextureFaceARB driDispatchRemapTable[FramebufferTextureFaceARB_remap_index]
#define _gloffset_ProgramParameteriARB driDispatchRemapTable[ProgramParameteriARB_remap_index]
#define _gloffset_VertexAttribDivisorARB driDispatchRemapTable[VertexAttribDivisorARB_remap_index]
#define _gloffset_FlushMappedBufferRange driDispatchRemapTable[FlushMappedBufferRange_remap_index]
#define _gloffset_MapBufferRange driDispatchRemapTable[MapBufferRange_remap_index]
#define _gloffset_TexBufferARB driDispatchRemapTable[TexBufferARB_remap_index]
#define _gloffset_BindVertexArray driDispatchRemapTable[BindVertexArray_remap_index]
#define _gloffset_GenVertexArrays driDispatchRemapTable[GenVertexArrays_remap_index]
#define _gloffset_CopyBufferSubData driDispatchRemapTable[CopyBufferSubData_remap_index]
#define _gloffset_ClientWaitSync driDispatchRemapTable[ClientWaitSync_remap_index]
#define _gloffset_DeleteSync driDispatchRemapTable[DeleteSync_remap_index]
#define _gloffset_FenceSync driDispatchRemapTable[FenceSync_remap_index]
#define _gloffset_GetInteger64v driDispatchRemapTable[GetInteger64v_remap_index]
#define _gloffset_GetSynciv driDispatchRemapTable[GetSynciv_remap_index]
#define _gloffset_IsSync driDispatchRemapTable[IsSync_remap_index]
#define _gloffset_WaitSync driDispatchRemapTable[WaitSync_remap_index]
#define _gloffset_DrawElementsBaseVertex driDispatchRemapTable[DrawElementsBaseVertex_remap_index]
#define _gloffset_DrawElementsInstancedBaseVertex driDispatchRemapTable[DrawElementsInstancedBaseVertex_remap_index]
#define _gloffset_DrawRangeElementsBaseVertex driDispatchRemapTable[DrawRangeElementsBaseVertex_remap_index]
#define _gloffset_MultiDrawElementsBaseVertex driDispatchRemapTable[MultiDrawElementsBaseVertex_remap_index]
#define _gloffset_BlendEquationSeparateiARB driDispatchRemapTable[BlendEquationSeparateiARB_remap_index]
#define _gloffset_BlendEquationiARB driDispatchRemapTable[BlendEquationiARB_remap_index]
#define _gloffset_BlendFuncSeparateiARB driDispatchRemapTable[BlendFuncSeparateiARB_remap_index]
#define _gloffset_BlendFunciARB driDispatchRemapTable[BlendFunciARB_remap_index]
#define _gloffset_BindSampler driDispatchRemapTable[BindSampler_remap_index]
#define _gloffset_DeleteSamplers driDispatchRemapTable[DeleteSamplers_remap_index]
#define _gloffset_GenSamplers driDispatchRemapTable[GenSamplers_remap_index]
#define _gloffset_GetSamplerParameterIiv driDispatchRemapTable[GetSamplerParameterIiv_remap_index]
#define _gloffset_GetSamplerParameterIuiv driDispatchRemapTable[GetSamplerParameterIuiv_remap_index]
#define _gloffset_GetSamplerParameterfv driDispatchRemapTable[GetSamplerParameterfv_remap_index]
#define _gloffset_GetSamplerParameteriv driDispatchRemapTable[GetSamplerParameteriv_remap_index]
#define _gloffset_IsSampler driDispatchRemapTable[IsSampler_remap_index]
#define _gloffset_SamplerParameterIiv driDispatchRemapTable[SamplerParameterIiv_remap_index]
#define _gloffset_SamplerParameterIuiv driDispatchRemapTable[SamplerParameterIuiv_remap_index]
#define _gloffset_SamplerParameterf driDispatchRemapTable[SamplerParameterf_remap_index]
#define _gloffset_SamplerParameterfv driDispatchRemapTable[SamplerParameterfv_remap_index]
#define _gloffset_SamplerParameteri driDispatchRemapTable[SamplerParameteri_remap_index]
#define _gloffset_SamplerParameteriv driDispatchRemapTable[SamplerParameteriv_remap_index]
#define _gloffset_ColorP3ui driDispatchRemapTable[ColorP3ui_remap_index]
#define _gloffset_ColorP3uiv driDispatchRemapTable[ColorP3uiv_remap_index]
#define _gloffset_ColorP4ui driDispatchRemapTable[ColorP4ui_remap_index]
#define _gloffset_ColorP4uiv driDispatchRemapTable[ColorP4uiv_remap_index]
#define _gloffset_MultiTexCoordP1ui driDispatchRemapTable[MultiTexCoordP1ui_remap_index]
#define _gloffset_MultiTexCoordP1uiv driDispatchRemapTable[MultiTexCoordP1uiv_remap_index]
#define _gloffset_MultiTexCoordP2ui driDispatchRemapTable[MultiTexCoordP2ui_remap_index]
#define _gloffset_MultiTexCoordP2uiv driDispatchRemapTable[MultiTexCoordP2uiv_remap_index]
#define _gloffset_MultiTexCoordP3ui driDispatchRemapTable[MultiTexCoordP3ui_remap_index]
#define _gloffset_MultiTexCoordP3uiv driDispatchRemapTable[MultiTexCoordP3uiv_remap_index]
#define _gloffset_MultiTexCoordP4ui driDispatchRemapTable[MultiTexCoordP4ui_remap_index]
#define _gloffset_MultiTexCoordP4uiv driDispatchRemapTable[MultiTexCoordP4uiv_remap_index]
#define _gloffset_NormalP3ui driDispatchRemapTable[NormalP3ui_remap_index]
#define _gloffset_NormalP3uiv driDispatchRemapTable[NormalP3uiv_remap_index]
#define _gloffset_SecondaryColorP3ui driDispatchRemapTable[SecondaryColorP3ui_remap_index]
#define _gloffset_SecondaryColorP3uiv driDispatchRemapTable[SecondaryColorP3uiv_remap_index]
#define _gloffset_TexCoordP1ui driDispatchRemapTable[TexCoordP1ui_remap_index]
#define _gloffset_TexCoordP1uiv driDispatchRemapTable[TexCoordP1uiv_remap_index]
#define _gloffset_TexCoordP2ui driDispatchRemapTable[TexCoordP2ui_remap_index]
#define _gloffset_TexCoordP2uiv driDispatchRemapTable[TexCoordP2uiv_remap_index]
#define _gloffset_TexCoordP3ui driDispatchRemapTable[TexCoordP3ui_remap_index]
#define _gloffset_TexCoordP3uiv driDispatchRemapTable[TexCoordP3uiv_remap_index]
#define _gloffset_TexCoordP4ui driDispatchRemapTable[TexCoordP4ui_remap_index]
#define _gloffset_TexCoordP4uiv driDispatchRemapTable[TexCoordP4uiv_remap_index]
#define _gloffset_VertexAttribP1ui driDispatchRemapTable[VertexAttribP1ui_remap_index]
#define _gloffset_VertexAttribP1uiv driDispatchRemapTable[VertexAttribP1uiv_remap_index]
#define _gloffset_VertexAttribP2ui driDispatchRemapTable[VertexAttribP2ui_remap_index]
#define _gloffset_VertexAttribP2uiv driDispatchRemapTable[VertexAttribP2uiv_remap_index]
#define _gloffset_VertexAttribP3ui driDispatchRemapTable[VertexAttribP3ui_remap_index]
#define _gloffset_VertexAttribP3uiv driDispatchRemapTable[VertexAttribP3uiv_remap_index]
#define _gloffset_VertexAttribP4ui driDispatchRemapTable[VertexAttribP4ui_remap_index]
#define _gloffset_VertexAttribP4uiv driDispatchRemapTable[VertexAttribP4uiv_remap_index]
#define _gloffset_VertexP2ui driDispatchRemapTable[VertexP2ui_remap_index]
#define _gloffset_VertexP2uiv driDispatchRemapTable[VertexP2uiv_remap_index]
#define _gloffset_VertexP3ui driDispatchRemapTable[VertexP3ui_remap_index]
#define _gloffset_VertexP3uiv driDispatchRemapTable[VertexP3uiv_remap_index]
#define _gloffset_VertexP4ui driDispatchRemapTable[VertexP4ui_remap_index]
#define _gloffset_VertexP4uiv driDispatchRemapTable[VertexP4uiv_remap_index]
#define _gloffset_BindTransformFeedback driDispatchRemapTable[BindTransformFeedback_remap_index]
#define _gloffset_DeleteTransformFeedbacks driDispatchRemapTable[DeleteTransformFeedbacks_remap_index]
#define _gloffset_DrawTransformFeedback driDispatchRemapTable[DrawTransformFeedback_remap_index]
#define _gloffset_GenTransformFeedbacks driDispatchRemapTable[GenTransformFeedbacks_remap_index]
#define _gloffset_IsTransformFeedback driDispatchRemapTable[IsTransformFeedback_remap_index]
#define _gloffset_PauseTransformFeedback driDispatchRemapTable[PauseTransformFeedback_remap_index]
#define _gloffset_ResumeTransformFeedback driDispatchRemapTable[ResumeTransformFeedback_remap_index]
#define _gloffset_ClearDepthf driDispatchRemapTable[ClearDepthf_remap_index]
#define _gloffset_DepthRangef driDispatchRemapTable[DepthRangef_remap_index]
#define _gloffset_GetShaderPrecisionFormat driDispatchRemapTable[GetShaderPrecisionFormat_remap_index]
#define _gloffset_ReleaseShaderCompiler driDispatchRemapTable[ReleaseShaderCompiler_remap_index]
#define _gloffset_ShaderBinary driDispatchRemapTable[ShaderBinary_remap_index]
#define _gloffset_GetGraphicsResetStatusARB driDispatchRemapTable[GetGraphicsResetStatusARB_remap_index]
#define _gloffset_GetnColorTableARB driDispatchRemapTable[GetnColorTableARB_remap_index]
#define _gloffset_GetnCompressedTexImageARB driDispatchRemapTable[GetnCompressedTexImageARB_remap_index]
#define _gloffset_GetnConvolutionFilterARB driDispatchRemapTable[GetnConvolutionFilterARB_remap_index]
#define _gloffset_GetnHistogramARB driDispatchRemapTable[GetnHistogramARB_remap_index]
#define _gloffset_GetnMapdvARB driDispatchRemapTable[GetnMapdvARB_remap_index]
#define _gloffset_GetnMapfvARB driDispatchRemapTable[GetnMapfvARB_remap_index]
#define _gloffset_GetnMapivARB driDispatchRemapTable[GetnMapivARB_remap_index]
#define _gloffset_GetnMinmaxARB driDispatchRemapTable[GetnMinmaxARB_remap_index]
#define _gloffset_GetnPixelMapfvARB driDispatchRemapTable[GetnPixelMapfvARB_remap_index]
#define _gloffset_GetnPixelMapuivARB driDispatchRemapTable[GetnPixelMapuivARB_remap_index]
#define _gloffset_GetnPixelMapusvARB driDispatchRemapTable[GetnPixelMapusvARB_remap_index]
#define _gloffset_GetnPolygonStippleARB driDispatchRemapTable[GetnPolygonStippleARB_remap_index]
#define _gloffset_GetnSeparableFilterARB driDispatchRemapTable[GetnSeparableFilterARB_remap_index]
#define _gloffset_GetnTexImageARB driDispatchRemapTable[GetnTexImageARB_remap_index]
#define _gloffset_GetnUniformdvARB driDispatchRemapTable[GetnUniformdvARB_remap_index]
#define _gloffset_GetnUniformfvARB driDispatchRemapTable[GetnUniformfvARB_remap_index]
#define _gloffset_GetnUniformivARB driDispatchRemapTable[GetnUniformivARB_remap_index]
#define _gloffset_GetnUniformuivARB driDispatchRemapTable[GetnUniformuivARB_remap_index]
#define _gloffset_ReadnPixelsARB driDispatchRemapTable[ReadnPixelsARB_remap_index]
#define _gloffset_TexStorage1D driDispatchRemapTable[TexStorage1D_remap_index]
#define _gloffset_TexStorage2D driDispatchRemapTable[TexStorage2D_remap_index]
#define _gloffset_TexStorage3D driDispatchRemapTable[TexStorage3D_remap_index]
#define _gloffset_TextureStorage1DEXT driDispatchRemapTable[TextureStorage1DEXT_remap_index]
#define _gloffset_TextureStorage2DEXT driDispatchRemapTable[TextureStorage2DEXT_remap_index]
#define _gloffset_TextureStorage3DEXT driDispatchRemapTable[TextureStorage3DEXT_remap_index]
#define _gloffset_PolygonOffsetEXT driDispatchRemapTable[PolygonOffsetEXT_remap_index]
#define _gloffset_GetPixelTexGenParameterfvSGIS driDispatchRemapTable[GetPixelTexGenParameterfvSGIS_remap_index]
#define _gloffset_GetPixelTexGenParameterivSGIS driDispatchRemapTable[GetPixelTexGenParameterivSGIS_remap_index]
#define _gloffset_PixelTexGenParameterfSGIS driDispatchRemapTable[PixelTexGenParameterfSGIS_remap_index]
#define _gloffset_PixelTexGenParameterfvSGIS driDispatchRemapTable[PixelTexGenParameterfvSGIS_remap_index]
#define _gloffset_PixelTexGenParameteriSGIS driDispatchRemapTable[PixelTexGenParameteriSGIS_remap_index]
#define _gloffset_PixelTexGenParameterivSGIS driDispatchRemapTable[PixelTexGenParameterivSGIS_remap_index]
#define _gloffset_SampleMaskSGIS driDispatchRemapTable[SampleMaskSGIS_remap_index]
#define _gloffset_SamplePatternSGIS driDispatchRemapTable[SamplePatternSGIS_remap_index]
#define _gloffset_ColorPointerEXT driDispatchRemapTable[ColorPointerEXT_remap_index]
#define _gloffset_EdgeFlagPointerEXT driDispatchRemapTable[EdgeFlagPointerEXT_remap_index]
#define _gloffset_IndexPointerEXT driDispatchRemapTable[IndexPointerEXT_remap_index]
#define _gloffset_NormalPointerEXT driDispatchRemapTable[NormalPointerEXT_remap_index]
#define _gloffset_TexCoordPointerEXT driDispatchRemapTable[TexCoordPointerEXT_remap_index]
#define _gloffset_VertexPointerEXT driDispatchRemapTable[VertexPointerEXT_remap_index]
#define _gloffset_PointParameterfEXT driDispatchRemapTable[PointParameterfEXT_remap_index]
#define _gloffset_PointParameterfvEXT driDispatchRemapTable[PointParameterfvEXT_remap_index]
#define _gloffset_LockArraysEXT driDispatchRemapTable[LockArraysEXT_remap_index]
#define _gloffset_UnlockArraysEXT driDispatchRemapTable[UnlockArraysEXT_remap_index]
#define _gloffset_SecondaryColor3bEXT driDispatchRemapTable[SecondaryColor3bEXT_remap_index]
#define _gloffset_SecondaryColor3bvEXT driDispatchRemapTable[SecondaryColor3bvEXT_remap_index]
#define _gloffset_SecondaryColor3dEXT driDispatchRemapTable[SecondaryColor3dEXT_remap_index]
#define _gloffset_SecondaryColor3dvEXT driDispatchRemapTable[SecondaryColor3dvEXT_remap_index]
#define _gloffset_SecondaryColor3fEXT driDispatchRemapTable[SecondaryColor3fEXT_remap_index]
#define _gloffset_SecondaryColor3fvEXT driDispatchRemapTable[SecondaryColor3fvEXT_remap_index]
#define _gloffset_SecondaryColor3iEXT driDispatchRemapTable[SecondaryColor3iEXT_remap_index]
#define _gloffset_SecondaryColor3ivEXT driDispatchRemapTable[SecondaryColor3ivEXT_remap_index]
#define _gloffset_SecondaryColor3sEXT driDispatchRemapTable[SecondaryColor3sEXT_remap_index]
#define _gloffset_SecondaryColor3svEXT driDispatchRemapTable[SecondaryColor3svEXT_remap_index]
#define _gloffset_SecondaryColor3ubEXT driDispatchRemapTable[SecondaryColor3ubEXT_remap_index]
#define _gloffset_SecondaryColor3ubvEXT driDispatchRemapTable[SecondaryColor3ubvEXT_remap_index]
#define _gloffset_SecondaryColor3uiEXT driDispatchRemapTable[SecondaryColor3uiEXT_remap_index]
#define _gloffset_SecondaryColor3uivEXT driDispatchRemapTable[SecondaryColor3uivEXT_remap_index]
#define _gloffset_SecondaryColor3usEXT driDispatchRemapTable[SecondaryColor3usEXT_remap_index]
#define _gloffset_SecondaryColor3usvEXT driDispatchRemapTable[SecondaryColor3usvEXT_remap_index]
#define _gloffset_SecondaryColorPointerEXT driDispatchRemapTable[SecondaryColorPointerEXT_remap_index]
#define _gloffset_MultiDrawArraysEXT driDispatchRemapTable[MultiDrawArraysEXT_remap_index]
#define _gloffset_MultiDrawElementsEXT driDispatchRemapTable[MultiDrawElementsEXT_remap_index]
#define _gloffset_FogCoordPointerEXT driDispatchRemapTable[FogCoordPointerEXT_remap_index]
#define _gloffset_FogCoorddEXT driDispatchRemapTable[FogCoorddEXT_remap_index]
#define _gloffset_FogCoorddvEXT driDispatchRemapTable[FogCoorddvEXT_remap_index]
#define _gloffset_FogCoordfEXT driDispatchRemapTable[FogCoordfEXT_remap_index]
#define _gloffset_FogCoordfvEXT driDispatchRemapTable[FogCoordfvEXT_remap_index]
#define _gloffset_PixelTexGenSGIX driDispatchRemapTable[PixelTexGenSGIX_remap_index]
#define _gloffset_BlendFuncSeparateEXT driDispatchRemapTable[BlendFuncSeparateEXT_remap_index]
#define _gloffset_FlushVertexArrayRangeNV driDispatchRemapTable[FlushVertexArrayRangeNV_remap_index]
#define _gloffset_VertexArrayRangeNV driDispatchRemapTable[VertexArrayRangeNV_remap_index]
#define _gloffset_CombinerInputNV driDispatchRemapTable[CombinerInputNV_remap_index]
#define _gloffset_CombinerOutputNV driDispatchRemapTable[CombinerOutputNV_remap_index]
#define _gloffset_CombinerParameterfNV driDispatchRemapTable[CombinerParameterfNV_remap_index]
#define _gloffset_CombinerParameterfvNV driDispatchRemapTable[CombinerParameterfvNV_remap_index]
#define _gloffset_CombinerParameteriNV driDispatchRemapTable[CombinerParameteriNV_remap_index]
#define _gloffset_CombinerParameterivNV driDispatchRemapTable[CombinerParameterivNV_remap_index]
#define _gloffset_FinalCombinerInputNV driDispatchRemapTable[FinalCombinerInputNV_remap_index]
#define _gloffset_GetCombinerInputParameterfvNV driDispatchRemapTable[GetCombinerInputParameterfvNV_remap_index]
#define _gloffset_GetCombinerInputParameterivNV driDispatchRemapTable[GetCombinerInputParameterivNV_remap_index]
#define _gloffset_GetCombinerOutputParameterfvNV driDispatchRemapTable[GetCombinerOutputParameterfvNV_remap_index]
#define _gloffset_GetCombinerOutputParameterivNV driDispatchRemapTable[GetCombinerOutputParameterivNV_remap_index]
#define _gloffset_GetFinalCombinerInputParameterfvNV driDispatchRemapTable[GetFinalCombinerInputParameterfvNV_remap_index]
#define _gloffset_GetFinalCombinerInputParameterivNV driDispatchRemapTable[GetFinalCombinerInputParameterivNV_remap_index]
#define _gloffset_ResizeBuffersMESA driDispatchRemapTable[ResizeBuffersMESA_remap_index]
#define _gloffset_WindowPos2dMESA driDispatchRemapTable[WindowPos2dMESA_remap_index]
#define _gloffset_WindowPos2dvMESA driDispatchRemapTable[WindowPos2dvMESA_remap_index]
#define _gloffset_WindowPos2fMESA driDispatchRemapTable[WindowPos2fMESA_remap_index]
#define _gloffset_WindowPos2fvMESA driDispatchRemapTable[WindowPos2fvMESA_remap_index]
#define _gloffset_WindowPos2iMESA driDispatchRemapTable[WindowPos2iMESA_remap_index]
#define _gloffset_WindowPos2ivMESA driDispatchRemapTable[WindowPos2ivMESA_remap_index]
#define _gloffset_WindowPos2sMESA driDispatchRemapTable[WindowPos2sMESA_remap_index]
#define _gloffset_WindowPos2svMESA driDispatchRemapTable[WindowPos2svMESA_remap_index]
#define _gloffset_WindowPos3dMESA driDispatchRemapTable[WindowPos3dMESA_remap_index]
#define _gloffset_WindowPos3dvMESA driDispatchRemapTable[WindowPos3dvMESA_remap_index]
#define _gloffset_WindowPos3fMESA driDispatchRemapTable[WindowPos3fMESA_remap_index]
#define _gloffset_WindowPos3fvMESA driDispatchRemapTable[WindowPos3fvMESA_remap_index]
#define _gloffset_WindowPos3iMESA driDispatchRemapTable[WindowPos3iMESA_remap_index]
#define _gloffset_WindowPos3ivMESA driDispatchRemapTable[WindowPos3ivMESA_remap_index]
#define _gloffset_WindowPos3sMESA driDispatchRemapTable[WindowPos3sMESA_remap_index]
#define _gloffset_WindowPos3svMESA driDispatchRemapTable[WindowPos3svMESA_remap_index]
#define _gloffset_WindowPos4dMESA driDispatchRemapTable[WindowPos4dMESA_remap_index]
#define _gloffset_WindowPos4dvMESA driDispatchRemapTable[WindowPos4dvMESA_remap_index]
#define _gloffset_WindowPos4fMESA driDispatchRemapTable[WindowPos4fMESA_remap_index]
#define _gloffset_WindowPos4fvMESA driDispatchRemapTable[WindowPos4fvMESA_remap_index]
#define _gloffset_WindowPos4iMESA driDispatchRemapTable[WindowPos4iMESA_remap_index]
#define _gloffset_WindowPos4ivMESA driDispatchRemapTable[WindowPos4ivMESA_remap_index]
#define _gloffset_WindowPos4sMESA driDispatchRemapTable[WindowPos4sMESA_remap_index]
#define _gloffset_WindowPos4svMESA driDispatchRemapTable[WindowPos4svMESA_remap_index]
#define _gloffset_MultiModeDrawArraysIBM driDispatchRemapTable[MultiModeDrawArraysIBM_remap_index]
#define _gloffset_MultiModeDrawElementsIBM driDispatchRemapTable[MultiModeDrawElementsIBM_remap_index]
#define _gloffset_DeleteFencesNV driDispatchRemapTable[DeleteFencesNV_remap_index]
#define _gloffset_FinishFenceNV driDispatchRemapTable[FinishFenceNV_remap_index]
#define _gloffset_GenFencesNV driDispatchRemapTable[GenFencesNV_remap_index]
#define _gloffset_GetFenceivNV driDispatchRemapTable[GetFenceivNV_remap_index]
#define _gloffset_IsFenceNV driDispatchRemapTable[IsFenceNV_remap_index]
#define _gloffset_SetFenceNV driDispatchRemapTable[SetFenceNV_remap_index]
#define _gloffset_TestFenceNV driDispatchRemapTable[TestFenceNV_remap_index]
#define _gloffset_AreProgramsResidentNV driDispatchRemapTable[AreProgramsResidentNV_remap_index]
#define _gloffset_BindProgramNV driDispatchRemapTable[BindProgramNV_remap_index]
#define _gloffset_DeleteProgramsNV driDispatchRemapTable[DeleteProgramsNV_remap_index]
#define _gloffset_ExecuteProgramNV driDispatchRemapTable[ExecuteProgramNV_remap_index]
#define _gloffset_GenProgramsNV driDispatchRemapTable[GenProgramsNV_remap_index]
#define _gloffset_GetProgramParameterdvNV driDispatchRemapTable[GetProgramParameterdvNV_remap_index]
#define _gloffset_GetProgramParameterfvNV driDispatchRemapTable[GetProgramParameterfvNV_remap_index]
#define _gloffset_GetProgramStringNV driDispatchRemapTable[GetProgramStringNV_remap_index]
#define _gloffset_GetProgramivNV driDispatchRemapTable[GetProgramivNV_remap_index]
#define _gloffset_GetTrackMatrixivNV driDispatchRemapTable[GetTrackMatrixivNV_remap_index]
#define _gloffset_GetVertexAttribPointervNV driDispatchRemapTable[GetVertexAttribPointervNV_remap_index]
#define _gloffset_GetVertexAttribdvNV driDispatchRemapTable[GetVertexAttribdvNV_remap_index]
#define _gloffset_GetVertexAttribfvNV driDispatchRemapTable[GetVertexAttribfvNV_remap_index]
#define _gloffset_GetVertexAttribivNV driDispatchRemapTable[GetVertexAttribivNV_remap_index]
#define _gloffset_IsProgramNV driDispatchRemapTable[IsProgramNV_remap_index]
#define _gloffset_LoadProgramNV driDispatchRemapTable[LoadProgramNV_remap_index]
#define _gloffset_ProgramParameters4dvNV driDispatchRemapTable[ProgramParameters4dvNV_remap_index]
#define _gloffset_ProgramParameters4fvNV driDispatchRemapTable[ProgramParameters4fvNV_remap_index]
#define _gloffset_RequestResidentProgramsNV driDispatchRemapTable[RequestResidentProgramsNV_remap_index]
#define _gloffset_TrackMatrixNV driDispatchRemapTable[TrackMatrixNV_remap_index]
#define _gloffset_VertexAttrib1dNV driDispatchRemapTable[VertexAttrib1dNV_remap_index]
#define _gloffset_VertexAttrib1dvNV driDispatchRemapTable[VertexAttrib1dvNV_remap_index]
#define _gloffset_VertexAttrib1fNV driDispatchRemapTable[VertexAttrib1fNV_remap_index]
#define _gloffset_VertexAttrib1fvNV driDispatchRemapTable[VertexAttrib1fvNV_remap_index]
#define _gloffset_VertexAttrib1sNV driDispatchRemapTable[VertexAttrib1sNV_remap_index]
#define _gloffset_VertexAttrib1svNV driDispatchRemapTable[VertexAttrib1svNV_remap_index]
#define _gloffset_VertexAttrib2dNV driDispatchRemapTable[VertexAttrib2dNV_remap_index]
#define _gloffset_VertexAttrib2dvNV driDispatchRemapTable[VertexAttrib2dvNV_remap_index]
#define _gloffset_VertexAttrib2fNV driDispatchRemapTable[VertexAttrib2fNV_remap_index]
#define _gloffset_VertexAttrib2fvNV driDispatchRemapTable[VertexAttrib2fvNV_remap_index]
#define _gloffset_VertexAttrib2sNV driDispatchRemapTable[VertexAttrib2sNV_remap_index]
#define _gloffset_VertexAttrib2svNV driDispatchRemapTable[VertexAttrib2svNV_remap_index]
#define _gloffset_VertexAttrib3dNV driDispatchRemapTable[VertexAttrib3dNV_remap_index]
#define _gloffset_VertexAttrib3dvNV driDispatchRemapTable[VertexAttrib3dvNV_remap_index]
#define _gloffset_VertexAttrib3fNV driDispatchRemapTable[VertexAttrib3fNV_remap_index]
#define _gloffset_VertexAttrib3fvNV driDispatchRemapTable[VertexAttrib3fvNV_remap_index]
#define _gloffset_VertexAttrib3sNV driDispatchRemapTable[VertexAttrib3sNV_remap_index]
#define _gloffset_VertexAttrib3svNV driDispatchRemapTable[VertexAttrib3svNV_remap_index]
#define _gloffset_VertexAttrib4dNV driDispatchRemapTable[VertexAttrib4dNV_remap_index]
#define _gloffset_VertexAttrib4dvNV driDispatchRemapTable[VertexAttrib4dvNV_remap_index]
#define _gloffset_VertexAttrib4fNV driDispatchRemapTable[VertexAttrib4fNV_remap_index]
#define _gloffset_VertexAttrib4fvNV driDispatchRemapTable[VertexAttrib4fvNV_remap_index]
#define _gloffset_VertexAttrib4sNV driDispatchRemapTable[VertexAttrib4sNV_remap_index]
#define _gloffset_VertexAttrib4svNV driDispatchRemapTable[VertexAttrib4svNV_remap_index]
#define _gloffset_VertexAttrib4ubNV driDispatchRemapTable[VertexAttrib4ubNV_remap_index]
#define _gloffset_VertexAttrib4ubvNV driDispatchRemapTable[VertexAttrib4ubvNV_remap_index]
#define _gloffset_VertexAttribPointerNV driDispatchRemapTable[VertexAttribPointerNV_remap_index]
#define _gloffset_VertexAttribs1dvNV driDispatchRemapTable[VertexAttribs1dvNV_remap_index]
#define _gloffset_VertexAttribs1fvNV driDispatchRemapTable[VertexAttribs1fvNV_remap_index]
#define _gloffset_VertexAttribs1svNV driDispatchRemapTable[VertexAttribs1svNV_remap_index]
#define _gloffset_VertexAttribs2dvNV driDispatchRemapTable[VertexAttribs2dvNV_remap_index]
#define _gloffset_VertexAttribs2fvNV driDispatchRemapTable[VertexAttribs2fvNV_remap_index]
#define _gloffset_VertexAttribs2svNV driDispatchRemapTable[VertexAttribs2svNV_remap_index]
#define _gloffset_VertexAttribs3dvNV driDispatchRemapTable[VertexAttribs3dvNV_remap_index]
#define _gloffset_VertexAttribs3fvNV driDispatchRemapTable[VertexAttribs3fvNV_remap_index]
#define _gloffset_VertexAttribs3svNV driDispatchRemapTable[VertexAttribs3svNV_remap_index]
#define _gloffset_VertexAttribs4dvNV driDispatchRemapTable[VertexAttribs4dvNV_remap_index]
#define _gloffset_VertexAttribs4fvNV driDispatchRemapTable[VertexAttribs4fvNV_remap_index]
#define _gloffset_VertexAttribs4svNV driDispatchRemapTable[VertexAttribs4svNV_remap_index]
#define _gloffset_VertexAttribs4ubvNV driDispatchRemapTable[VertexAttribs4ubvNV_remap_index]
#define _gloffset_GetTexBumpParameterfvATI driDispatchRemapTable[GetTexBumpParameterfvATI_remap_index]
#define _gloffset_GetTexBumpParameterivATI driDispatchRemapTable[GetTexBumpParameterivATI_remap_index]
#define _gloffset_TexBumpParameterfvATI driDispatchRemapTable[TexBumpParameterfvATI_remap_index]
#define _gloffset_TexBumpParameterivATI driDispatchRemapTable[TexBumpParameterivATI_remap_index]
#define _gloffset_AlphaFragmentOp1ATI driDispatchRemapTable[AlphaFragmentOp1ATI_remap_index]
#define _gloffset_AlphaFragmentOp2ATI driDispatchRemapTable[AlphaFragmentOp2ATI_remap_index]
#define _gloffset_AlphaFragmentOp3ATI driDispatchRemapTable[AlphaFragmentOp3ATI_remap_index]
#define _gloffset_BeginFragmentShaderATI driDispatchRemapTable[BeginFragmentShaderATI_remap_index]
#define _gloffset_BindFragmentShaderATI driDispatchRemapTable[BindFragmentShaderATI_remap_index]
#define _gloffset_ColorFragmentOp1ATI driDispatchRemapTable[ColorFragmentOp1ATI_remap_index]
#define _gloffset_ColorFragmentOp2ATI driDispatchRemapTable[ColorFragmentOp2ATI_remap_index]
#define _gloffset_ColorFragmentOp3ATI driDispatchRemapTable[ColorFragmentOp3ATI_remap_index]
#define _gloffset_DeleteFragmentShaderATI driDispatchRemapTable[DeleteFragmentShaderATI_remap_index]
#define _gloffset_EndFragmentShaderATI driDispatchRemapTable[EndFragmentShaderATI_remap_index]
#define _gloffset_GenFragmentShadersATI driDispatchRemapTable[GenFragmentShadersATI_remap_index]
#define _gloffset_PassTexCoordATI driDispatchRemapTable[PassTexCoordATI_remap_index]
#define _gloffset_SampleMapATI driDispatchRemapTable[SampleMapATI_remap_index]
#define _gloffset_SetFragmentShaderConstantATI driDispatchRemapTable[SetFragmentShaderConstantATI_remap_index]
#define _gloffset_PointParameteriNV driDispatchRemapTable[PointParameteriNV_remap_index]
#define _gloffset_PointParameterivNV driDispatchRemapTable[PointParameterivNV_remap_index]
#define _gloffset_ActiveStencilFaceEXT driDispatchRemapTable[ActiveStencilFaceEXT_remap_index]
#define _gloffset_BindVertexArrayAPPLE driDispatchRemapTable[BindVertexArrayAPPLE_remap_index]
#define _gloffset_DeleteVertexArraysAPPLE driDispatchRemapTable[DeleteVertexArraysAPPLE_remap_index]
#define _gloffset_GenVertexArraysAPPLE driDispatchRemapTable[GenVertexArraysAPPLE_remap_index]
#define _gloffset_IsVertexArrayAPPLE driDispatchRemapTable[IsVertexArrayAPPLE_remap_index]
#define _gloffset_GetProgramNamedParameterdvNV driDispatchRemapTable[GetProgramNamedParameterdvNV_remap_index]
#define _gloffset_GetProgramNamedParameterfvNV driDispatchRemapTable[GetProgramNamedParameterfvNV_remap_index]
#define _gloffset_ProgramNamedParameter4dNV driDispatchRemapTable[ProgramNamedParameter4dNV_remap_index]
#define _gloffset_ProgramNamedParameter4dvNV driDispatchRemapTable[ProgramNamedParameter4dvNV_remap_index]
#define _gloffset_ProgramNamedParameter4fNV driDispatchRemapTable[ProgramNamedParameter4fNV_remap_index]
#define _gloffset_ProgramNamedParameter4fvNV driDispatchRemapTable[ProgramNamedParameter4fvNV_remap_index]
#define _gloffset_PrimitiveRestartIndexNV driDispatchRemapTable[PrimitiveRestartIndexNV_remap_index]
#define _gloffset_PrimitiveRestartNV driDispatchRemapTable[PrimitiveRestartNV_remap_index]
#define _gloffset_DepthBoundsEXT driDispatchRemapTable[DepthBoundsEXT_remap_index]
#define _gloffset_BlendEquationSeparateEXT driDispatchRemapTable[BlendEquationSeparateEXT_remap_index]
#define _gloffset_BindFramebufferEXT driDispatchRemapTable[BindFramebufferEXT_remap_index]
#define _gloffset_BindRenderbufferEXT driDispatchRemapTable[BindRenderbufferEXT_remap_index]
#define _gloffset_CheckFramebufferStatusEXT driDispatchRemapTable[CheckFramebufferStatusEXT_remap_index]
#define _gloffset_DeleteFramebuffersEXT driDispatchRemapTable[DeleteFramebuffersEXT_remap_index]
#define _gloffset_DeleteRenderbuffersEXT driDispatchRemapTable[DeleteRenderbuffersEXT_remap_index]
#define _gloffset_FramebufferRenderbufferEXT driDispatchRemapTable[FramebufferRenderbufferEXT_remap_index]
#define _gloffset_FramebufferTexture1DEXT driDispatchRemapTable[FramebufferTexture1DEXT_remap_index]
#define _gloffset_FramebufferTexture2DEXT driDispatchRemapTable[FramebufferTexture2DEXT_remap_index]
#define _gloffset_FramebufferTexture3DEXT driDispatchRemapTable[FramebufferTexture3DEXT_remap_index]
#define _gloffset_GenFramebuffersEXT driDispatchRemapTable[GenFramebuffersEXT_remap_index]
#define _gloffset_GenRenderbuffersEXT driDispatchRemapTable[GenRenderbuffersEXT_remap_index]
#define _gloffset_GenerateMipmapEXT driDispatchRemapTable[GenerateMipmapEXT_remap_index]
#define _gloffset_GetFramebufferAttachmentParameterivEXT driDispatchRemapTable[GetFramebufferAttachmentParameterivEXT_remap_index]
#define _gloffset_GetRenderbufferParameterivEXT driDispatchRemapTable[GetRenderbufferParameterivEXT_remap_index]
#define _gloffset_IsFramebufferEXT driDispatchRemapTable[IsFramebufferEXT_remap_index]
#define _gloffset_IsRenderbufferEXT driDispatchRemapTable[IsRenderbufferEXT_remap_index]
#define _gloffset_RenderbufferStorageEXT driDispatchRemapTable[RenderbufferStorageEXT_remap_index]
#define _gloffset_BlitFramebufferEXT driDispatchRemapTable[BlitFramebufferEXT_remap_index]
#define _gloffset_BufferParameteriAPPLE driDispatchRemapTable[BufferParameteriAPPLE_remap_index]
#define _gloffset_FlushMappedBufferRangeAPPLE driDispatchRemapTable[FlushMappedBufferRangeAPPLE_remap_index]
#define _gloffset_BindFragDataLocationEXT driDispatchRemapTable[BindFragDataLocationEXT_remap_index]
#define _gloffset_GetFragDataLocationEXT driDispatchRemapTable[GetFragDataLocationEXT_remap_index]
#define _gloffset_GetUniformuivEXT driDispatchRemapTable[GetUniformuivEXT_remap_index]
#define _gloffset_GetVertexAttribIivEXT driDispatchRemapTable[GetVertexAttribIivEXT_remap_index]
#define _gloffset_GetVertexAttribIuivEXT driDispatchRemapTable[GetVertexAttribIuivEXT_remap_index]
#define _gloffset_Uniform1uiEXT driDispatchRemapTable[Uniform1uiEXT_remap_index]
#define _gloffset_Uniform1uivEXT driDispatchRemapTable[Uniform1uivEXT_remap_index]
#define _gloffset_Uniform2uiEXT driDispatchRemapTable[Uniform2uiEXT_remap_index]
#define _gloffset_Uniform2uivEXT driDispatchRemapTable[Uniform2uivEXT_remap_index]
#define _gloffset_Uniform3uiEXT driDispatchRemapTable[Uniform3uiEXT_remap_index]
#define _gloffset_Uniform3uivEXT driDispatchRemapTable[Uniform3uivEXT_remap_index]
#define _gloffset_Uniform4uiEXT driDispatchRemapTable[Uniform4uiEXT_remap_index]
#define _gloffset_Uniform4uivEXT driDispatchRemapTable[Uniform4uivEXT_remap_index]
#define _gloffset_VertexAttribI1iEXT driDispatchRemapTable[VertexAttribI1iEXT_remap_index]
#define _gloffset_VertexAttribI1ivEXT driDispatchRemapTable[VertexAttribI1ivEXT_remap_index]
#define _gloffset_VertexAttribI1uiEXT driDispatchRemapTable[VertexAttribI1uiEXT_remap_index]
#define _gloffset_VertexAttribI1uivEXT driDispatchRemapTable[VertexAttribI1uivEXT_remap_index]
#define _gloffset_VertexAttribI2iEXT driDispatchRemapTable[VertexAttribI2iEXT_remap_index]
#define _gloffset_VertexAttribI2ivEXT driDispatchRemapTable[VertexAttribI2ivEXT_remap_index]
#define _gloffset_VertexAttribI2uiEXT driDispatchRemapTable[VertexAttribI2uiEXT_remap_index]
#define _gloffset_VertexAttribI2uivEXT driDispatchRemapTable[VertexAttribI2uivEXT_remap_index]
#define _gloffset_VertexAttribI3iEXT driDispatchRemapTable[VertexAttribI3iEXT_remap_index]
#define _gloffset_VertexAttribI3ivEXT driDispatchRemapTable[VertexAttribI3ivEXT_remap_index]
#define _gloffset_VertexAttribI3uiEXT driDispatchRemapTable[VertexAttribI3uiEXT_remap_index]
#define _gloffset_VertexAttribI3uivEXT driDispatchRemapTable[VertexAttribI3uivEXT_remap_index]
#define _gloffset_VertexAttribI4bvEXT driDispatchRemapTable[VertexAttribI4bvEXT_remap_index]
#define _gloffset_VertexAttribI4iEXT driDispatchRemapTable[VertexAttribI4iEXT_remap_index]
#define _gloffset_VertexAttribI4ivEXT driDispatchRemapTable[VertexAttribI4ivEXT_remap_index]
#define _gloffset_VertexAttribI4svEXT driDispatchRemapTable[VertexAttribI4svEXT_remap_index]
#define _gloffset_VertexAttribI4ubvEXT driDispatchRemapTable[VertexAttribI4ubvEXT_remap_index]
#define _gloffset_VertexAttribI4uiEXT driDispatchRemapTable[VertexAttribI4uiEXT_remap_index]
#define _gloffset_VertexAttribI4uivEXT driDispatchRemapTable[VertexAttribI4uivEXT_remap_index]
#define _gloffset_VertexAttribI4usvEXT driDispatchRemapTable[VertexAttribI4usvEXT_remap_index]
#define _gloffset_VertexAttribIPointerEXT driDispatchRemapTable[VertexAttribIPointerEXT_remap_index]
#define _gloffset_FramebufferTextureLayerEXT driDispatchRemapTable[FramebufferTextureLayerEXT_remap_index]
#define _gloffset_ColorMaskIndexedEXT driDispatchRemapTable[ColorMaskIndexedEXT_remap_index]
#define _gloffset_DisableIndexedEXT driDispatchRemapTable[DisableIndexedEXT_remap_index]
#define _gloffset_EnableIndexedEXT driDispatchRemapTable[EnableIndexedEXT_remap_index]
#define _gloffset_GetBooleanIndexedvEXT driDispatchRemapTable[GetBooleanIndexedvEXT_remap_index]
#define _gloffset_GetIntegerIndexedvEXT driDispatchRemapTable[GetIntegerIndexedvEXT_remap_index]
#define _gloffset_IsEnabledIndexedEXT driDispatchRemapTable[IsEnabledIndexedEXT_remap_index]
#define _gloffset_ClearColorIiEXT driDispatchRemapTable[ClearColorIiEXT_remap_index]
#define _gloffset_ClearColorIuiEXT driDispatchRemapTable[ClearColorIuiEXT_remap_index]
#define _gloffset_GetTexParameterIivEXT driDispatchRemapTable[GetTexParameterIivEXT_remap_index]
#define _gloffset_GetTexParameterIuivEXT driDispatchRemapTable[GetTexParameterIuivEXT_remap_index]
#define _gloffset_TexParameterIivEXT driDispatchRemapTable[TexParameterIivEXT_remap_index]
#define _gloffset_TexParameterIuivEXT driDispatchRemapTable[TexParameterIuivEXT_remap_index]
#define _gloffset_BeginConditionalRenderNV driDispatchRemapTable[BeginConditionalRenderNV_remap_index]
#define _gloffset_EndConditionalRenderNV driDispatchRemapTable[EndConditionalRenderNV_remap_index]
#define _gloffset_BeginTransformFeedbackEXT driDispatchRemapTable[BeginTransformFeedbackEXT_remap_index]
#define _gloffset_BindBufferBaseEXT driDispatchRemapTable[BindBufferBaseEXT_remap_index]
#define _gloffset_BindBufferOffsetEXT driDispatchRemapTable[BindBufferOffsetEXT_remap_index]
#define _gloffset_BindBufferRangeEXT driDispatchRemapTable[BindBufferRangeEXT_remap_index]
#define _gloffset_EndTransformFeedbackEXT driDispatchRemapTable[EndTransformFeedbackEXT_remap_index]
#define _gloffset_GetTransformFeedbackVaryingEXT driDispatchRemapTable[GetTransformFeedbackVaryingEXT_remap_index]
#define _gloffset_TransformFeedbackVaryingsEXT driDispatchRemapTable[TransformFeedbackVaryingsEXT_remap_index]
#define _gloffset_ProvokingVertexEXT driDispatchRemapTable[ProvokingVertexEXT_remap_index]
#define _gloffset_GetTexParameterPointervAPPLE driDispatchRemapTable[GetTexParameterPointervAPPLE_remap_index]
#define _gloffset_TextureRangeAPPLE driDispatchRemapTable[TextureRangeAPPLE_remap_index]
#define _gloffset_GetObjectParameterivAPPLE driDispatchRemapTable[GetObjectParameterivAPPLE_remap_index]
#define _gloffset_ObjectPurgeableAPPLE driDispatchRemapTable[ObjectPurgeableAPPLE_remap_index]
#define _gloffset_ObjectUnpurgeableAPPLE driDispatchRemapTable[ObjectUnpurgeableAPPLE_remap_index]
#define _gloffset_ActiveProgramEXT driDispatchRemapTable[ActiveProgramEXT_remap_index]
#define _gloffset_CreateShaderProgramEXT driDispatchRemapTable[CreateShaderProgramEXT_remap_index]
#define _gloffset_UseShaderProgramEXT driDispatchRemapTable[UseShaderProgramEXT_remap_index]
#define _gloffset_TextureBarrierNV driDispatchRemapTable[TextureBarrierNV_remap_index]
#define _gloffset_StencilFuncSeparateATI driDispatchRemapTable[StencilFuncSeparateATI_remap_index]
#define _gloffset_ProgramEnvParameters4fvEXT driDispatchRemapTable[ProgramEnvParameters4fvEXT_remap_index]
#define _gloffset_ProgramLocalParameters4fvEXT driDispatchRemapTable[ProgramLocalParameters4fvEXT_remap_index]
#define _gloffset_GetQueryObjecti64vEXT driDispatchRemapTable[GetQueryObjecti64vEXT_remap_index]
#define _gloffset_GetQueryObjectui64vEXT driDispatchRemapTable[GetQueryObjectui64vEXT_remap_index]
#define _gloffset_EGLImageTargetRenderbufferStorageOES driDispatchRemapTable[EGLImageTargetRenderbufferStorageOES_remap_index]
#define _gloffset_EGLImageTargetTexture2DOES driDispatchRemapTable[EGLImageTargetTexture2DOES_remap_index]

#endif /* !FEATURE_remap_table */

typedef void (GLAPIENTRYP _glptr_NewList)(GLuint, GLenum);
#define CALL_NewList(disp, parameters) \
    (* GET_NewList(disp)) parameters
static inline _glptr_NewList GET_NewList(struct _glapi_table *disp) {
   return (_glptr_NewList) (GET_by_offset(disp, _gloffset_NewList));
}

static inline void SET_NewList(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum)) {
   SET_by_offset(disp, _gloffset_NewList, fn);
}

typedef void (GLAPIENTRYP _glptr_EndList)(void);
#define CALL_EndList(disp, parameters) \
    (* GET_EndList(disp)) parameters
static inline _glptr_EndList GET_EndList(struct _glapi_table *disp) {
   return (_glptr_EndList) (GET_by_offset(disp, _gloffset_EndList));
}

static inline void SET_EndList(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_EndList, fn);
}

typedef void (GLAPIENTRYP _glptr_CallList)(GLuint);
#define CALL_CallList(disp, parameters) \
    (* GET_CallList(disp)) parameters
static inline _glptr_CallList GET_CallList(struct _glapi_table *disp) {
   return (_glptr_CallList) (GET_by_offset(disp, _gloffset_CallList));
}

static inline void SET_CallList(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_CallList, fn);
}

typedef void (GLAPIENTRYP _glptr_CallLists)(GLsizei, GLenum, const GLvoid *);
#define CALL_CallLists(disp, parameters) \
    (* GET_CallLists(disp)) parameters
static inline _glptr_CallLists GET_CallLists(struct _glapi_table *disp) {
   return (_glptr_CallLists) (GET_by_offset(disp, _gloffset_CallLists));
}

static inline void SET_CallLists(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLenum, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_CallLists, fn);
}

typedef void (GLAPIENTRYP _glptr_DeleteLists)(GLuint, GLsizei);
#define CALL_DeleteLists(disp, parameters) \
    (* GET_DeleteLists(disp)) parameters
static inline _glptr_DeleteLists GET_DeleteLists(struct _glapi_table *disp) {
   return (_glptr_DeleteLists) (GET_by_offset(disp, _gloffset_DeleteLists));
}

static inline void SET_DeleteLists(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei)) {
   SET_by_offset(disp, _gloffset_DeleteLists, fn);
}

typedef GLuint (GLAPIENTRYP _glptr_GenLists)(GLsizei);
#define CALL_GenLists(disp, parameters) \
    (* GET_GenLists(disp)) parameters
static inline _glptr_GenLists GET_GenLists(struct _glapi_table *disp) {
   return (_glptr_GenLists) (GET_by_offset(disp, _gloffset_GenLists));
}

static inline void SET_GenLists(struct _glapi_table *disp, GLuint (GLAPIENTRYP fn)(GLsizei)) {
   SET_by_offset(disp, _gloffset_GenLists, fn);
}

typedef void (GLAPIENTRYP _glptr_ListBase)(GLuint);
#define CALL_ListBase(disp, parameters) \
    (* GET_ListBase(disp)) parameters
static inline _glptr_ListBase GET_ListBase(struct _glapi_table *disp) {
   return (_glptr_ListBase) (GET_by_offset(disp, _gloffset_ListBase));
}

static inline void SET_ListBase(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_ListBase, fn);
}

typedef void (GLAPIENTRYP _glptr_Begin)(GLenum);
#define CALL_Begin(disp, parameters) \
    (* GET_Begin(disp)) parameters
static inline _glptr_Begin GET_Begin(struct _glapi_table *disp) {
   return (_glptr_Begin) (GET_by_offset(disp, _gloffset_Begin));
}

static inline void SET_Begin(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_Begin, fn);
}

typedef void (GLAPIENTRYP _glptr_Bitmap)(GLsizei, GLsizei, GLfloat, GLfloat, GLfloat, GLfloat, const GLubyte *);
#define CALL_Bitmap(disp, parameters) \
    (* GET_Bitmap(disp)) parameters
static inline _glptr_Bitmap GET_Bitmap(struct _glapi_table *disp) {
   return (_glptr_Bitmap) (GET_by_offset(disp, _gloffset_Bitmap));
}

static inline void SET_Bitmap(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLsizei, GLfloat, GLfloat, GLfloat, GLfloat, const GLubyte *)) {
   SET_by_offset(disp, _gloffset_Bitmap, fn);
}

typedef void (GLAPIENTRYP _glptr_Color3b)(GLbyte, GLbyte, GLbyte);
#define CALL_Color3b(disp, parameters) \
    (* GET_Color3b(disp)) parameters
static inline _glptr_Color3b GET_Color3b(struct _glapi_table *disp) {
   return (_glptr_Color3b) (GET_by_offset(disp, _gloffset_Color3b));
}

static inline void SET_Color3b(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLbyte, GLbyte, GLbyte)) {
   SET_by_offset(disp, _gloffset_Color3b, fn);
}

typedef void (GLAPIENTRYP _glptr_Color3bv)(const GLbyte *);
#define CALL_Color3bv(disp, parameters) \
    (* GET_Color3bv(disp)) parameters
static inline _glptr_Color3bv GET_Color3bv(struct _glapi_table *disp) {
   return (_glptr_Color3bv) (GET_by_offset(disp, _gloffset_Color3bv));
}

static inline void SET_Color3bv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLbyte *)) {
   SET_by_offset(disp, _gloffset_Color3bv, fn);
}

typedef void (GLAPIENTRYP _glptr_Color3d)(GLdouble, GLdouble, GLdouble);
#define CALL_Color3d(disp, parameters) \
    (* GET_Color3d(disp)) parameters
static inline _glptr_Color3d GET_Color3d(struct _glapi_table *disp) {
   return (_glptr_Color3d) (GET_by_offset(disp, _gloffset_Color3d));
}

static inline void SET_Color3d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_Color3d, fn);
}

typedef void (GLAPIENTRYP _glptr_Color3dv)(const GLdouble *);
#define CALL_Color3dv(disp, parameters) \
    (* GET_Color3dv(disp)) parameters
static inline _glptr_Color3dv GET_Color3dv(struct _glapi_table *disp) {
   return (_glptr_Color3dv) (GET_by_offset(disp, _gloffset_Color3dv));
}

static inline void SET_Color3dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_Color3dv, fn);
}

typedef void (GLAPIENTRYP _glptr_Color3f)(GLfloat, GLfloat, GLfloat);
#define CALL_Color3f(disp, parameters) \
    (* GET_Color3f(disp)) parameters
static inline _glptr_Color3f GET_Color3f(struct _glapi_table *disp) {
   return (_glptr_Color3f) (GET_by_offset(disp, _gloffset_Color3f));
}

static inline void SET_Color3f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_Color3f, fn);
}

typedef void (GLAPIENTRYP _glptr_Color3fv)(const GLfloat *);
#define CALL_Color3fv(disp, parameters) \
    (* GET_Color3fv(disp)) parameters
static inline _glptr_Color3fv GET_Color3fv(struct _glapi_table *disp) {
   return (_glptr_Color3fv) (GET_by_offset(disp, _gloffset_Color3fv));
}

static inline void SET_Color3fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Color3fv, fn);
}

typedef void (GLAPIENTRYP _glptr_Color3i)(GLint, GLint, GLint);
#define CALL_Color3i(disp, parameters) \
    (* GET_Color3i(disp)) parameters
static inline _glptr_Color3i GET_Color3i(struct _glapi_table *disp) {
   return (_glptr_Color3i) (GET_by_offset(disp, _gloffset_Color3i));
}

static inline void SET_Color3i(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_Color3i, fn);
}

typedef void (GLAPIENTRYP _glptr_Color3iv)(const GLint *);
#define CALL_Color3iv(disp, parameters) \
    (* GET_Color3iv(disp)) parameters
static inline _glptr_Color3iv GET_Color3iv(struct _glapi_table *disp) {
   return (_glptr_Color3iv) (GET_by_offset(disp, _gloffset_Color3iv));
}

static inline void SET_Color3iv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_Color3iv, fn);
}

typedef void (GLAPIENTRYP _glptr_Color3s)(GLshort, GLshort, GLshort);
#define CALL_Color3s(disp, parameters) \
    (* GET_Color3s(disp)) parameters
static inline _glptr_Color3s GET_Color3s(struct _glapi_table *disp) {
   return (_glptr_Color3s) (GET_by_offset(disp, _gloffset_Color3s));
}

static inline void SET_Color3s(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_Color3s, fn);
}

typedef void (GLAPIENTRYP _glptr_Color3sv)(const GLshort *);
#define CALL_Color3sv(disp, parameters) \
    (* GET_Color3sv(disp)) parameters
static inline _glptr_Color3sv GET_Color3sv(struct _glapi_table *disp) {
   return (_glptr_Color3sv) (GET_by_offset(disp, _gloffset_Color3sv));
}

static inline void SET_Color3sv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_Color3sv, fn);
}

typedef void (GLAPIENTRYP _glptr_Color3ub)(GLubyte, GLubyte, GLubyte);
#define CALL_Color3ub(disp, parameters) \
    (* GET_Color3ub(disp)) parameters
static inline _glptr_Color3ub GET_Color3ub(struct _glapi_table *disp) {
   return (_glptr_Color3ub) (GET_by_offset(disp, _gloffset_Color3ub));
}

static inline void SET_Color3ub(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLubyte, GLubyte, GLubyte)) {
   SET_by_offset(disp, _gloffset_Color3ub, fn);
}

typedef void (GLAPIENTRYP _glptr_Color3ubv)(const GLubyte *);
#define CALL_Color3ubv(disp, parameters) \
    (* GET_Color3ubv(disp)) parameters
static inline _glptr_Color3ubv GET_Color3ubv(struct _glapi_table *disp) {
   return (_glptr_Color3ubv) (GET_by_offset(disp, _gloffset_Color3ubv));
}

static inline void SET_Color3ubv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLubyte *)) {
   SET_by_offset(disp, _gloffset_Color3ubv, fn);
}

typedef void (GLAPIENTRYP _glptr_Color3ui)(GLuint, GLuint, GLuint);
#define CALL_Color3ui(disp, parameters) \
    (* GET_Color3ui(disp)) parameters
static inline _glptr_Color3ui GET_Color3ui(struct _glapi_table *disp) {
   return (_glptr_Color3ui) (GET_by_offset(disp, _gloffset_Color3ui));
}

static inline void SET_Color3ui(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_Color3ui, fn);
}

typedef void (GLAPIENTRYP _glptr_Color3uiv)(const GLuint *);
#define CALL_Color3uiv(disp, parameters) \
    (* GET_Color3uiv(disp)) parameters
static inline _glptr_Color3uiv GET_Color3uiv(struct _glapi_table *disp) {
   return (_glptr_Color3uiv) (GET_by_offset(disp, _gloffset_Color3uiv));
}

static inline void SET_Color3uiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLuint *)) {
   SET_by_offset(disp, _gloffset_Color3uiv, fn);
}

typedef void (GLAPIENTRYP _glptr_Color3us)(GLushort, GLushort, GLushort);
#define CALL_Color3us(disp, parameters) \
    (* GET_Color3us(disp)) parameters
static inline _glptr_Color3us GET_Color3us(struct _glapi_table *disp) {
   return (_glptr_Color3us) (GET_by_offset(disp, _gloffset_Color3us));
}

static inline void SET_Color3us(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLushort, GLushort, GLushort)) {
   SET_by_offset(disp, _gloffset_Color3us, fn);
}

typedef void (GLAPIENTRYP _glptr_Color3usv)(const GLushort *);
#define CALL_Color3usv(disp, parameters) \
    (* GET_Color3usv(disp)) parameters
static inline _glptr_Color3usv GET_Color3usv(struct _glapi_table *disp) {
   return (_glptr_Color3usv) (GET_by_offset(disp, _gloffset_Color3usv));
}

static inline void SET_Color3usv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLushort *)) {
   SET_by_offset(disp, _gloffset_Color3usv, fn);
}

typedef void (GLAPIENTRYP _glptr_Color4b)(GLbyte, GLbyte, GLbyte, GLbyte);
#define CALL_Color4b(disp, parameters) \
    (* GET_Color4b(disp)) parameters
static inline _glptr_Color4b GET_Color4b(struct _glapi_table *disp) {
   return (_glptr_Color4b) (GET_by_offset(disp, _gloffset_Color4b));
}

static inline void SET_Color4b(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLbyte, GLbyte, GLbyte, GLbyte)) {
   SET_by_offset(disp, _gloffset_Color4b, fn);
}

typedef void (GLAPIENTRYP _glptr_Color4bv)(const GLbyte *);
#define CALL_Color4bv(disp, parameters) \
    (* GET_Color4bv(disp)) parameters
static inline _glptr_Color4bv GET_Color4bv(struct _glapi_table *disp) {
   return (_glptr_Color4bv) (GET_by_offset(disp, _gloffset_Color4bv));
}

static inline void SET_Color4bv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLbyte *)) {
   SET_by_offset(disp, _gloffset_Color4bv, fn);
}

typedef void (GLAPIENTRYP _glptr_Color4d)(GLdouble, GLdouble, GLdouble, GLdouble);
#define CALL_Color4d(disp, parameters) \
    (* GET_Color4d(disp)) parameters
static inline _glptr_Color4d GET_Color4d(struct _glapi_table *disp) {
   return (_glptr_Color4d) (GET_by_offset(disp, _gloffset_Color4d));
}

static inline void SET_Color4d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_Color4d, fn);
}

typedef void (GLAPIENTRYP _glptr_Color4dv)(const GLdouble *);
#define CALL_Color4dv(disp, parameters) \
    (* GET_Color4dv(disp)) parameters
static inline _glptr_Color4dv GET_Color4dv(struct _glapi_table *disp) {
   return (_glptr_Color4dv) (GET_by_offset(disp, _gloffset_Color4dv));
}

static inline void SET_Color4dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_Color4dv, fn);
}

typedef void (GLAPIENTRYP _glptr_Color4f)(GLfloat, GLfloat, GLfloat, GLfloat);
#define CALL_Color4f(disp, parameters) \
    (* GET_Color4f(disp)) parameters
static inline _glptr_Color4f GET_Color4f(struct _glapi_table *disp) {
   return (_glptr_Color4f) (GET_by_offset(disp, _gloffset_Color4f));
}

static inline void SET_Color4f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_Color4f, fn);
}

typedef void (GLAPIENTRYP _glptr_Color4fv)(const GLfloat *);
#define CALL_Color4fv(disp, parameters) \
    (* GET_Color4fv(disp)) parameters
static inline _glptr_Color4fv GET_Color4fv(struct _glapi_table *disp) {
   return (_glptr_Color4fv) (GET_by_offset(disp, _gloffset_Color4fv));
}

static inline void SET_Color4fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Color4fv, fn);
}

typedef void (GLAPIENTRYP _glptr_Color4i)(GLint, GLint, GLint, GLint);
#define CALL_Color4i(disp, parameters) \
    (* GET_Color4i(disp)) parameters
static inline _glptr_Color4i GET_Color4i(struct _glapi_table *disp) {
   return (_glptr_Color4i) (GET_by_offset(disp, _gloffset_Color4i));
}

static inline void SET_Color4i(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_Color4i, fn);
}

typedef void (GLAPIENTRYP _glptr_Color4iv)(const GLint *);
#define CALL_Color4iv(disp, parameters) \
    (* GET_Color4iv(disp)) parameters
static inline _glptr_Color4iv GET_Color4iv(struct _glapi_table *disp) {
   return (_glptr_Color4iv) (GET_by_offset(disp, _gloffset_Color4iv));
}

static inline void SET_Color4iv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_Color4iv, fn);
}

typedef void (GLAPIENTRYP _glptr_Color4s)(GLshort, GLshort, GLshort, GLshort);
#define CALL_Color4s(disp, parameters) \
    (* GET_Color4s(disp)) parameters
static inline _glptr_Color4s GET_Color4s(struct _glapi_table *disp) {
   return (_glptr_Color4s) (GET_by_offset(disp, _gloffset_Color4s));
}

static inline void SET_Color4s(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_Color4s, fn);
}

typedef void (GLAPIENTRYP _glptr_Color4sv)(const GLshort *);
#define CALL_Color4sv(disp, parameters) \
    (* GET_Color4sv(disp)) parameters
static inline _glptr_Color4sv GET_Color4sv(struct _glapi_table *disp) {
   return (_glptr_Color4sv) (GET_by_offset(disp, _gloffset_Color4sv));
}

static inline void SET_Color4sv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_Color4sv, fn);
}

typedef void (GLAPIENTRYP _glptr_Color4ub)(GLubyte, GLubyte, GLubyte, GLubyte);
#define CALL_Color4ub(disp, parameters) \
    (* GET_Color4ub(disp)) parameters
static inline _glptr_Color4ub GET_Color4ub(struct _glapi_table *disp) {
   return (_glptr_Color4ub) (GET_by_offset(disp, _gloffset_Color4ub));
}

static inline void SET_Color4ub(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLubyte, GLubyte, GLubyte, GLubyte)) {
   SET_by_offset(disp, _gloffset_Color4ub, fn);
}

typedef void (GLAPIENTRYP _glptr_Color4ubv)(const GLubyte *);
#define CALL_Color4ubv(disp, parameters) \
    (* GET_Color4ubv(disp)) parameters
static inline _glptr_Color4ubv GET_Color4ubv(struct _glapi_table *disp) {
   return (_glptr_Color4ubv) (GET_by_offset(disp, _gloffset_Color4ubv));
}

static inline void SET_Color4ubv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLubyte *)) {
   SET_by_offset(disp, _gloffset_Color4ubv, fn);
}

typedef void (GLAPIENTRYP _glptr_Color4ui)(GLuint, GLuint, GLuint, GLuint);
#define CALL_Color4ui(disp, parameters) \
    (* GET_Color4ui(disp)) parameters
static inline _glptr_Color4ui GET_Color4ui(struct _glapi_table *disp) {
   return (_glptr_Color4ui) (GET_by_offset(disp, _gloffset_Color4ui));
}

static inline void SET_Color4ui(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_Color4ui, fn);
}

typedef void (GLAPIENTRYP _glptr_Color4uiv)(const GLuint *);
#define CALL_Color4uiv(disp, parameters) \
    (* GET_Color4uiv(disp)) parameters
static inline _glptr_Color4uiv GET_Color4uiv(struct _glapi_table *disp) {
   return (_glptr_Color4uiv) (GET_by_offset(disp, _gloffset_Color4uiv));
}

static inline void SET_Color4uiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLuint *)) {
   SET_by_offset(disp, _gloffset_Color4uiv, fn);
}

typedef void (GLAPIENTRYP _glptr_Color4us)(GLushort, GLushort, GLushort, GLushort);
#define CALL_Color4us(disp, parameters) \
    (* GET_Color4us(disp)) parameters
static inline _glptr_Color4us GET_Color4us(struct _glapi_table *disp) {
   return (_glptr_Color4us) (GET_by_offset(disp, _gloffset_Color4us));
}

static inline void SET_Color4us(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLushort, GLushort, GLushort, GLushort)) {
   SET_by_offset(disp, _gloffset_Color4us, fn);
}

typedef void (GLAPIENTRYP _glptr_Color4usv)(const GLushort *);
#define CALL_Color4usv(disp, parameters) \
    (* GET_Color4usv(disp)) parameters
static inline _glptr_Color4usv GET_Color4usv(struct _glapi_table *disp) {
   return (_glptr_Color4usv) (GET_by_offset(disp, _gloffset_Color4usv));
}

static inline void SET_Color4usv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLushort *)) {
   SET_by_offset(disp, _gloffset_Color4usv, fn);
}

typedef void (GLAPIENTRYP _glptr_EdgeFlag)(GLboolean);
#define CALL_EdgeFlag(disp, parameters) \
    (* GET_EdgeFlag(disp)) parameters
static inline _glptr_EdgeFlag GET_EdgeFlag(struct _glapi_table *disp) {
   return (_glptr_EdgeFlag) (GET_by_offset(disp, _gloffset_EdgeFlag));
}

static inline void SET_EdgeFlag(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLboolean)) {
   SET_by_offset(disp, _gloffset_EdgeFlag, fn);
}

typedef void (GLAPIENTRYP _glptr_EdgeFlagv)(const GLboolean *);
#define CALL_EdgeFlagv(disp, parameters) \
    (* GET_EdgeFlagv(disp)) parameters
static inline _glptr_EdgeFlagv GET_EdgeFlagv(struct _glapi_table *disp) {
   return (_glptr_EdgeFlagv) (GET_by_offset(disp, _gloffset_EdgeFlagv));
}

static inline void SET_EdgeFlagv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLboolean *)) {
   SET_by_offset(disp, _gloffset_EdgeFlagv, fn);
}

typedef void (GLAPIENTRYP _glptr_End)(void);
#define CALL_End(disp, parameters) \
    (* GET_End(disp)) parameters
static inline _glptr_End GET_End(struct _glapi_table *disp) {
   return (_glptr_End) (GET_by_offset(disp, _gloffset_End));
}

static inline void SET_End(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_End, fn);
}

typedef void (GLAPIENTRYP _glptr_Indexd)(GLdouble);
#define CALL_Indexd(disp, parameters) \
    (* GET_Indexd(disp)) parameters
static inline _glptr_Indexd GET_Indexd(struct _glapi_table *disp) {
   return (_glptr_Indexd) (GET_by_offset(disp, _gloffset_Indexd));
}

static inline void SET_Indexd(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble)) {
   SET_by_offset(disp, _gloffset_Indexd, fn);
}

typedef void (GLAPIENTRYP _glptr_Indexdv)(const GLdouble *);
#define CALL_Indexdv(disp, parameters) \
    (* GET_Indexdv(disp)) parameters
static inline _glptr_Indexdv GET_Indexdv(struct _glapi_table *disp) {
   return (_glptr_Indexdv) (GET_by_offset(disp, _gloffset_Indexdv));
}

static inline void SET_Indexdv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_Indexdv, fn);
}

typedef void (GLAPIENTRYP _glptr_Indexf)(GLfloat);
#define CALL_Indexf(disp, parameters) \
    (* GET_Indexf(disp)) parameters
static inline _glptr_Indexf GET_Indexf(struct _glapi_table *disp) {
   return (_glptr_Indexf) (GET_by_offset(disp, _gloffset_Indexf));
}

static inline void SET_Indexf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat)) {
   SET_by_offset(disp, _gloffset_Indexf, fn);
}

typedef void (GLAPIENTRYP _glptr_Indexfv)(const GLfloat *);
#define CALL_Indexfv(disp, parameters) \
    (* GET_Indexfv(disp)) parameters
static inline _glptr_Indexfv GET_Indexfv(struct _glapi_table *disp) {
   return (_glptr_Indexfv) (GET_by_offset(disp, _gloffset_Indexfv));
}

static inline void SET_Indexfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Indexfv, fn);
}

typedef void (GLAPIENTRYP _glptr_Indexi)(GLint);
#define CALL_Indexi(disp, parameters) \
    (* GET_Indexi(disp)) parameters
static inline _glptr_Indexi GET_Indexi(struct _glapi_table *disp) {
   return (_glptr_Indexi) (GET_by_offset(disp, _gloffset_Indexi));
}

static inline void SET_Indexi(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint)) {
   SET_by_offset(disp, _gloffset_Indexi, fn);
}

typedef void (GLAPIENTRYP _glptr_Indexiv)(const GLint *);
#define CALL_Indexiv(disp, parameters) \
    (* GET_Indexiv(disp)) parameters
static inline _glptr_Indexiv GET_Indexiv(struct _glapi_table *disp) {
   return (_glptr_Indexiv) (GET_by_offset(disp, _gloffset_Indexiv));
}

static inline void SET_Indexiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_Indexiv, fn);
}

typedef void (GLAPIENTRYP _glptr_Indexs)(GLshort);
#define CALL_Indexs(disp, parameters) \
    (* GET_Indexs(disp)) parameters
static inline _glptr_Indexs GET_Indexs(struct _glapi_table *disp) {
   return (_glptr_Indexs) (GET_by_offset(disp, _gloffset_Indexs));
}

static inline void SET_Indexs(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort)) {
   SET_by_offset(disp, _gloffset_Indexs, fn);
}

typedef void (GLAPIENTRYP _glptr_Indexsv)(const GLshort *);
#define CALL_Indexsv(disp, parameters) \
    (* GET_Indexsv(disp)) parameters
static inline _glptr_Indexsv GET_Indexsv(struct _glapi_table *disp) {
   return (_glptr_Indexsv) (GET_by_offset(disp, _gloffset_Indexsv));
}

static inline void SET_Indexsv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_Indexsv, fn);
}

typedef void (GLAPIENTRYP _glptr_Normal3b)(GLbyte, GLbyte, GLbyte);
#define CALL_Normal3b(disp, parameters) \
    (* GET_Normal3b(disp)) parameters
static inline _glptr_Normal3b GET_Normal3b(struct _glapi_table *disp) {
   return (_glptr_Normal3b) (GET_by_offset(disp, _gloffset_Normal3b));
}

static inline void SET_Normal3b(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLbyte, GLbyte, GLbyte)) {
   SET_by_offset(disp, _gloffset_Normal3b, fn);
}

typedef void (GLAPIENTRYP _glptr_Normal3bv)(const GLbyte *);
#define CALL_Normal3bv(disp, parameters) \
    (* GET_Normal3bv(disp)) parameters
static inline _glptr_Normal3bv GET_Normal3bv(struct _glapi_table *disp) {
   return (_glptr_Normal3bv) (GET_by_offset(disp, _gloffset_Normal3bv));
}

static inline void SET_Normal3bv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLbyte *)) {
   SET_by_offset(disp, _gloffset_Normal3bv, fn);
}

typedef void (GLAPIENTRYP _glptr_Normal3d)(GLdouble, GLdouble, GLdouble);
#define CALL_Normal3d(disp, parameters) \
    (* GET_Normal3d(disp)) parameters
static inline _glptr_Normal3d GET_Normal3d(struct _glapi_table *disp) {
   return (_glptr_Normal3d) (GET_by_offset(disp, _gloffset_Normal3d));
}

static inline void SET_Normal3d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_Normal3d, fn);
}

typedef void (GLAPIENTRYP _glptr_Normal3dv)(const GLdouble *);
#define CALL_Normal3dv(disp, parameters) \
    (* GET_Normal3dv(disp)) parameters
static inline _glptr_Normal3dv GET_Normal3dv(struct _glapi_table *disp) {
   return (_glptr_Normal3dv) (GET_by_offset(disp, _gloffset_Normal3dv));
}

static inline void SET_Normal3dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_Normal3dv, fn);
}

typedef void (GLAPIENTRYP _glptr_Normal3f)(GLfloat, GLfloat, GLfloat);
#define CALL_Normal3f(disp, parameters) \
    (* GET_Normal3f(disp)) parameters
static inline _glptr_Normal3f GET_Normal3f(struct _glapi_table *disp) {
   return (_glptr_Normal3f) (GET_by_offset(disp, _gloffset_Normal3f));
}

static inline void SET_Normal3f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_Normal3f, fn);
}

typedef void (GLAPIENTRYP _glptr_Normal3fv)(const GLfloat *);
#define CALL_Normal3fv(disp, parameters) \
    (* GET_Normal3fv(disp)) parameters
static inline _glptr_Normal3fv GET_Normal3fv(struct _glapi_table *disp) {
   return (_glptr_Normal3fv) (GET_by_offset(disp, _gloffset_Normal3fv));
}

static inline void SET_Normal3fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Normal3fv, fn);
}

typedef void (GLAPIENTRYP _glptr_Normal3i)(GLint, GLint, GLint);
#define CALL_Normal3i(disp, parameters) \
    (* GET_Normal3i(disp)) parameters
static inline _glptr_Normal3i GET_Normal3i(struct _glapi_table *disp) {
   return (_glptr_Normal3i) (GET_by_offset(disp, _gloffset_Normal3i));
}

static inline void SET_Normal3i(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_Normal3i, fn);
}

typedef void (GLAPIENTRYP _glptr_Normal3iv)(const GLint *);
#define CALL_Normal3iv(disp, parameters) \
    (* GET_Normal3iv(disp)) parameters
static inline _glptr_Normal3iv GET_Normal3iv(struct _glapi_table *disp) {
   return (_glptr_Normal3iv) (GET_by_offset(disp, _gloffset_Normal3iv));
}

static inline void SET_Normal3iv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_Normal3iv, fn);
}

typedef void (GLAPIENTRYP _glptr_Normal3s)(GLshort, GLshort, GLshort);
#define CALL_Normal3s(disp, parameters) \
    (* GET_Normal3s(disp)) parameters
static inline _glptr_Normal3s GET_Normal3s(struct _glapi_table *disp) {
   return (_glptr_Normal3s) (GET_by_offset(disp, _gloffset_Normal3s));
}

static inline void SET_Normal3s(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_Normal3s, fn);
}

typedef void (GLAPIENTRYP _glptr_Normal3sv)(const GLshort *);
#define CALL_Normal3sv(disp, parameters) \
    (* GET_Normal3sv(disp)) parameters
static inline _glptr_Normal3sv GET_Normal3sv(struct _glapi_table *disp) {
   return (_glptr_Normal3sv) (GET_by_offset(disp, _gloffset_Normal3sv));
}

static inline void SET_Normal3sv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_Normal3sv, fn);
}

typedef void (GLAPIENTRYP _glptr_RasterPos2d)(GLdouble, GLdouble);
#define CALL_RasterPos2d(disp, parameters) \
    (* GET_RasterPos2d(disp)) parameters
static inline _glptr_RasterPos2d GET_RasterPos2d(struct _glapi_table *disp) {
   return (_glptr_RasterPos2d) (GET_by_offset(disp, _gloffset_RasterPos2d));
}

static inline void SET_RasterPos2d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_RasterPos2d, fn);
}

typedef void (GLAPIENTRYP _glptr_RasterPos2dv)(const GLdouble *);
#define CALL_RasterPos2dv(disp, parameters) \
    (* GET_RasterPos2dv(disp)) parameters
static inline _glptr_RasterPos2dv GET_RasterPos2dv(struct _glapi_table *disp) {
   return (_glptr_RasterPos2dv) (GET_by_offset(disp, _gloffset_RasterPos2dv));
}

static inline void SET_RasterPos2dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_RasterPos2dv, fn);
}

typedef void (GLAPIENTRYP _glptr_RasterPos2f)(GLfloat, GLfloat);
#define CALL_RasterPos2f(disp, parameters) \
    (* GET_RasterPos2f(disp)) parameters
static inline _glptr_RasterPos2f GET_RasterPos2f(struct _glapi_table *disp) {
   return (_glptr_RasterPos2f) (GET_by_offset(disp, _gloffset_RasterPos2f));
}

static inline void SET_RasterPos2f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_RasterPos2f, fn);
}

typedef void (GLAPIENTRYP _glptr_RasterPos2fv)(const GLfloat *);
#define CALL_RasterPos2fv(disp, parameters) \
    (* GET_RasterPos2fv(disp)) parameters
static inline _glptr_RasterPos2fv GET_RasterPos2fv(struct _glapi_table *disp) {
   return (_glptr_RasterPos2fv) (GET_by_offset(disp, _gloffset_RasterPos2fv));
}

static inline void SET_RasterPos2fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_RasterPos2fv, fn);
}

typedef void (GLAPIENTRYP _glptr_RasterPos2i)(GLint, GLint);
#define CALL_RasterPos2i(disp, parameters) \
    (* GET_RasterPos2i(disp)) parameters
static inline _glptr_RasterPos2i GET_RasterPos2i(struct _glapi_table *disp) {
   return (_glptr_RasterPos2i) (GET_by_offset(disp, _gloffset_RasterPos2i));
}

static inline void SET_RasterPos2i(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint)) {
   SET_by_offset(disp, _gloffset_RasterPos2i, fn);
}

typedef void (GLAPIENTRYP _glptr_RasterPos2iv)(const GLint *);
#define CALL_RasterPos2iv(disp, parameters) \
    (* GET_RasterPos2iv(disp)) parameters
static inline _glptr_RasterPos2iv GET_RasterPos2iv(struct _glapi_table *disp) {
   return (_glptr_RasterPos2iv) (GET_by_offset(disp, _gloffset_RasterPos2iv));
}

static inline void SET_RasterPos2iv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_RasterPos2iv, fn);
}

typedef void (GLAPIENTRYP _glptr_RasterPos2s)(GLshort, GLshort);
#define CALL_RasterPos2s(disp, parameters) \
    (* GET_RasterPos2s(disp)) parameters
static inline _glptr_RasterPos2s GET_RasterPos2s(struct _glapi_table *disp) {
   return (_glptr_RasterPos2s) (GET_by_offset(disp, _gloffset_RasterPos2s));
}

static inline void SET_RasterPos2s(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_RasterPos2s, fn);
}

typedef void (GLAPIENTRYP _glptr_RasterPos2sv)(const GLshort *);
#define CALL_RasterPos2sv(disp, parameters) \
    (* GET_RasterPos2sv(disp)) parameters
static inline _glptr_RasterPos2sv GET_RasterPos2sv(struct _glapi_table *disp) {
   return (_glptr_RasterPos2sv) (GET_by_offset(disp, _gloffset_RasterPos2sv));
}

static inline void SET_RasterPos2sv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_RasterPos2sv, fn);
}

typedef void (GLAPIENTRYP _glptr_RasterPos3d)(GLdouble, GLdouble, GLdouble);
#define CALL_RasterPos3d(disp, parameters) \
    (* GET_RasterPos3d(disp)) parameters
static inline _glptr_RasterPos3d GET_RasterPos3d(struct _glapi_table *disp) {
   return (_glptr_RasterPos3d) (GET_by_offset(disp, _gloffset_RasterPos3d));
}

static inline void SET_RasterPos3d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_RasterPos3d, fn);
}

typedef void (GLAPIENTRYP _glptr_RasterPos3dv)(const GLdouble *);
#define CALL_RasterPos3dv(disp, parameters) \
    (* GET_RasterPos3dv(disp)) parameters
static inline _glptr_RasterPos3dv GET_RasterPos3dv(struct _glapi_table *disp) {
   return (_glptr_RasterPos3dv) (GET_by_offset(disp, _gloffset_RasterPos3dv));
}

static inline void SET_RasterPos3dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_RasterPos3dv, fn);
}

typedef void (GLAPIENTRYP _glptr_RasterPos3f)(GLfloat, GLfloat, GLfloat);
#define CALL_RasterPos3f(disp, parameters) \
    (* GET_RasterPos3f(disp)) parameters
static inline _glptr_RasterPos3f GET_RasterPos3f(struct _glapi_table *disp) {
   return (_glptr_RasterPos3f) (GET_by_offset(disp, _gloffset_RasterPos3f));
}

static inline void SET_RasterPos3f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_RasterPos3f, fn);
}

typedef void (GLAPIENTRYP _glptr_RasterPos3fv)(const GLfloat *);
#define CALL_RasterPos3fv(disp, parameters) \
    (* GET_RasterPos3fv(disp)) parameters
static inline _glptr_RasterPos3fv GET_RasterPos3fv(struct _glapi_table *disp) {
   return (_glptr_RasterPos3fv) (GET_by_offset(disp, _gloffset_RasterPos3fv));
}

static inline void SET_RasterPos3fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_RasterPos3fv, fn);
}

typedef void (GLAPIENTRYP _glptr_RasterPos3i)(GLint, GLint, GLint);
#define CALL_RasterPos3i(disp, parameters) \
    (* GET_RasterPos3i(disp)) parameters
static inline _glptr_RasterPos3i GET_RasterPos3i(struct _glapi_table *disp) {
   return (_glptr_RasterPos3i) (GET_by_offset(disp, _gloffset_RasterPos3i));
}

static inline void SET_RasterPos3i(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_RasterPos3i, fn);
}

typedef void (GLAPIENTRYP _glptr_RasterPos3iv)(const GLint *);
#define CALL_RasterPos3iv(disp, parameters) \
    (* GET_RasterPos3iv(disp)) parameters
static inline _glptr_RasterPos3iv GET_RasterPos3iv(struct _glapi_table *disp) {
   return (_glptr_RasterPos3iv) (GET_by_offset(disp, _gloffset_RasterPos3iv));
}

static inline void SET_RasterPos3iv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_RasterPos3iv, fn);
}

typedef void (GLAPIENTRYP _glptr_RasterPos3s)(GLshort, GLshort, GLshort);
#define CALL_RasterPos3s(disp, parameters) \
    (* GET_RasterPos3s(disp)) parameters
static inline _glptr_RasterPos3s GET_RasterPos3s(struct _glapi_table *disp) {
   return (_glptr_RasterPos3s) (GET_by_offset(disp, _gloffset_RasterPos3s));
}

static inline void SET_RasterPos3s(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_RasterPos3s, fn);
}

typedef void (GLAPIENTRYP _glptr_RasterPos3sv)(const GLshort *);
#define CALL_RasterPos3sv(disp, parameters) \
    (* GET_RasterPos3sv(disp)) parameters
static inline _glptr_RasterPos3sv GET_RasterPos3sv(struct _glapi_table *disp) {
   return (_glptr_RasterPos3sv) (GET_by_offset(disp, _gloffset_RasterPos3sv));
}

static inline void SET_RasterPos3sv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_RasterPos3sv, fn);
}

typedef void (GLAPIENTRYP _glptr_RasterPos4d)(GLdouble, GLdouble, GLdouble, GLdouble);
#define CALL_RasterPos4d(disp, parameters) \
    (* GET_RasterPos4d(disp)) parameters
static inline _glptr_RasterPos4d GET_RasterPos4d(struct _glapi_table *disp) {
   return (_glptr_RasterPos4d) (GET_by_offset(disp, _gloffset_RasterPos4d));
}

static inline void SET_RasterPos4d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_RasterPos4d, fn);
}

typedef void (GLAPIENTRYP _glptr_RasterPos4dv)(const GLdouble *);
#define CALL_RasterPos4dv(disp, parameters) \
    (* GET_RasterPos4dv(disp)) parameters
static inline _glptr_RasterPos4dv GET_RasterPos4dv(struct _glapi_table *disp) {
   return (_glptr_RasterPos4dv) (GET_by_offset(disp, _gloffset_RasterPos4dv));
}

static inline void SET_RasterPos4dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_RasterPos4dv, fn);
}

typedef void (GLAPIENTRYP _glptr_RasterPos4f)(GLfloat, GLfloat, GLfloat, GLfloat);
#define CALL_RasterPos4f(disp, parameters) \
    (* GET_RasterPos4f(disp)) parameters
static inline _glptr_RasterPos4f GET_RasterPos4f(struct _glapi_table *disp) {
   return (_glptr_RasterPos4f) (GET_by_offset(disp, _gloffset_RasterPos4f));
}

static inline void SET_RasterPos4f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_RasterPos4f, fn);
}

typedef void (GLAPIENTRYP _glptr_RasterPos4fv)(const GLfloat *);
#define CALL_RasterPos4fv(disp, parameters) \
    (* GET_RasterPos4fv(disp)) parameters
static inline _glptr_RasterPos4fv GET_RasterPos4fv(struct _glapi_table *disp) {
   return (_glptr_RasterPos4fv) (GET_by_offset(disp, _gloffset_RasterPos4fv));
}

static inline void SET_RasterPos4fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_RasterPos4fv, fn);
}

typedef void (GLAPIENTRYP _glptr_RasterPos4i)(GLint, GLint, GLint, GLint);
#define CALL_RasterPos4i(disp, parameters) \
    (* GET_RasterPos4i(disp)) parameters
static inline _glptr_RasterPos4i GET_RasterPos4i(struct _glapi_table *disp) {
   return (_glptr_RasterPos4i) (GET_by_offset(disp, _gloffset_RasterPos4i));
}

static inline void SET_RasterPos4i(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_RasterPos4i, fn);
}

typedef void (GLAPIENTRYP _glptr_RasterPos4iv)(const GLint *);
#define CALL_RasterPos4iv(disp, parameters) \
    (* GET_RasterPos4iv(disp)) parameters
static inline _glptr_RasterPos4iv GET_RasterPos4iv(struct _glapi_table *disp) {
   return (_glptr_RasterPos4iv) (GET_by_offset(disp, _gloffset_RasterPos4iv));
}

static inline void SET_RasterPos4iv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_RasterPos4iv, fn);
}

typedef void (GLAPIENTRYP _glptr_RasterPos4s)(GLshort, GLshort, GLshort, GLshort);
#define CALL_RasterPos4s(disp, parameters) \
    (* GET_RasterPos4s(disp)) parameters
static inline _glptr_RasterPos4s GET_RasterPos4s(struct _glapi_table *disp) {
   return (_glptr_RasterPos4s) (GET_by_offset(disp, _gloffset_RasterPos4s));
}

static inline void SET_RasterPos4s(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_RasterPos4s, fn);
}

typedef void (GLAPIENTRYP _glptr_RasterPos4sv)(const GLshort *);
#define CALL_RasterPos4sv(disp, parameters) \
    (* GET_RasterPos4sv(disp)) parameters
static inline _glptr_RasterPos4sv GET_RasterPos4sv(struct _glapi_table *disp) {
   return (_glptr_RasterPos4sv) (GET_by_offset(disp, _gloffset_RasterPos4sv));
}

static inline void SET_RasterPos4sv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_RasterPos4sv, fn);
}

typedef void (GLAPIENTRYP _glptr_Rectd)(GLdouble, GLdouble, GLdouble, GLdouble);
#define CALL_Rectd(disp, parameters) \
    (* GET_Rectd(disp)) parameters
static inline _glptr_Rectd GET_Rectd(struct _glapi_table *disp) {
   return (_glptr_Rectd) (GET_by_offset(disp, _gloffset_Rectd));
}

static inline void SET_Rectd(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_Rectd, fn);
}

typedef void (GLAPIENTRYP _glptr_Rectdv)(const GLdouble *, const GLdouble *);
#define CALL_Rectdv(disp, parameters) \
    (* GET_Rectdv(disp)) parameters
static inline _glptr_Rectdv GET_Rectdv(struct _glapi_table *disp) {
   return (_glptr_Rectdv) (GET_by_offset(disp, _gloffset_Rectdv));
}

static inline void SET_Rectdv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_Rectdv, fn);
}

typedef void (GLAPIENTRYP _glptr_Rectf)(GLfloat, GLfloat, GLfloat, GLfloat);
#define CALL_Rectf(disp, parameters) \
    (* GET_Rectf(disp)) parameters
static inline _glptr_Rectf GET_Rectf(struct _glapi_table *disp) {
   return (_glptr_Rectf) (GET_by_offset(disp, _gloffset_Rectf));
}

static inline void SET_Rectf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_Rectf, fn);
}

typedef void (GLAPIENTRYP _glptr_Rectfv)(const GLfloat *, const GLfloat *);
#define CALL_Rectfv(disp, parameters) \
    (* GET_Rectfv(disp)) parameters
static inline _glptr_Rectfv GET_Rectfv(struct _glapi_table *disp) {
   return (_glptr_Rectfv) (GET_by_offset(disp, _gloffset_Rectfv));
}

static inline void SET_Rectfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Rectfv, fn);
}

typedef void (GLAPIENTRYP _glptr_Recti)(GLint, GLint, GLint, GLint);
#define CALL_Recti(disp, parameters) \
    (* GET_Recti(disp)) parameters
static inline _glptr_Recti GET_Recti(struct _glapi_table *disp) {
   return (_glptr_Recti) (GET_by_offset(disp, _gloffset_Recti));
}

static inline void SET_Recti(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_Recti, fn);
}

typedef void (GLAPIENTRYP _glptr_Rectiv)(const GLint *, const GLint *);
#define CALL_Rectiv(disp, parameters) \
    (* GET_Rectiv(disp)) parameters
static inline _glptr_Rectiv GET_Rectiv(struct _glapi_table *disp) {
   return (_glptr_Rectiv) (GET_by_offset(disp, _gloffset_Rectiv));
}

static inline void SET_Rectiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *, const GLint *)) {
   SET_by_offset(disp, _gloffset_Rectiv, fn);
}

typedef void (GLAPIENTRYP _glptr_Rects)(GLshort, GLshort, GLshort, GLshort);
#define CALL_Rects(disp, parameters) \
    (* GET_Rects(disp)) parameters
static inline _glptr_Rects GET_Rects(struct _glapi_table *disp) {
   return (_glptr_Rects) (GET_by_offset(disp, _gloffset_Rects));
}

static inline void SET_Rects(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_Rects, fn);
}

typedef void (GLAPIENTRYP _glptr_Rectsv)(const GLshort *, const GLshort *);
#define CALL_Rectsv(disp, parameters) \
    (* GET_Rectsv(disp)) parameters
static inline _glptr_Rectsv GET_Rectsv(struct _glapi_table *disp) {
   return (_glptr_Rectsv) (GET_by_offset(disp, _gloffset_Rectsv));
}

static inline void SET_Rectsv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *, const GLshort *)) {
   SET_by_offset(disp, _gloffset_Rectsv, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord1d)(GLdouble);
#define CALL_TexCoord1d(disp, parameters) \
    (* GET_TexCoord1d(disp)) parameters
static inline _glptr_TexCoord1d GET_TexCoord1d(struct _glapi_table *disp) {
   return (_glptr_TexCoord1d) (GET_by_offset(disp, _gloffset_TexCoord1d));
}

static inline void SET_TexCoord1d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble)) {
   SET_by_offset(disp, _gloffset_TexCoord1d, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord1dv)(const GLdouble *);
#define CALL_TexCoord1dv(disp, parameters) \
    (* GET_TexCoord1dv(disp)) parameters
static inline _glptr_TexCoord1dv GET_TexCoord1dv(struct _glapi_table *disp) {
   return (_glptr_TexCoord1dv) (GET_by_offset(disp, _gloffset_TexCoord1dv));
}

static inline void SET_TexCoord1dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_TexCoord1dv, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord1f)(GLfloat);
#define CALL_TexCoord1f(disp, parameters) \
    (* GET_TexCoord1f(disp)) parameters
static inline _glptr_TexCoord1f GET_TexCoord1f(struct _glapi_table *disp) {
   return (_glptr_TexCoord1f) (GET_by_offset(disp, _gloffset_TexCoord1f));
}

static inline void SET_TexCoord1f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat)) {
   SET_by_offset(disp, _gloffset_TexCoord1f, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord1fv)(const GLfloat *);
#define CALL_TexCoord1fv(disp, parameters) \
    (* GET_TexCoord1fv(disp)) parameters
static inline _glptr_TexCoord1fv GET_TexCoord1fv(struct _glapi_table *disp) {
   return (_glptr_TexCoord1fv) (GET_by_offset(disp, _gloffset_TexCoord1fv));
}

static inline void SET_TexCoord1fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_TexCoord1fv, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord1i)(GLint);
#define CALL_TexCoord1i(disp, parameters) \
    (* GET_TexCoord1i(disp)) parameters
static inline _glptr_TexCoord1i GET_TexCoord1i(struct _glapi_table *disp) {
   return (_glptr_TexCoord1i) (GET_by_offset(disp, _gloffset_TexCoord1i));
}

static inline void SET_TexCoord1i(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint)) {
   SET_by_offset(disp, _gloffset_TexCoord1i, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord1iv)(const GLint *);
#define CALL_TexCoord1iv(disp, parameters) \
    (* GET_TexCoord1iv(disp)) parameters
static inline _glptr_TexCoord1iv GET_TexCoord1iv(struct _glapi_table *disp) {
   return (_glptr_TexCoord1iv) (GET_by_offset(disp, _gloffset_TexCoord1iv));
}

static inline void SET_TexCoord1iv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_TexCoord1iv, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord1s)(GLshort);
#define CALL_TexCoord1s(disp, parameters) \
    (* GET_TexCoord1s(disp)) parameters
static inline _glptr_TexCoord1s GET_TexCoord1s(struct _glapi_table *disp) {
   return (_glptr_TexCoord1s) (GET_by_offset(disp, _gloffset_TexCoord1s));
}

static inline void SET_TexCoord1s(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort)) {
   SET_by_offset(disp, _gloffset_TexCoord1s, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord1sv)(const GLshort *);
#define CALL_TexCoord1sv(disp, parameters) \
    (* GET_TexCoord1sv(disp)) parameters
static inline _glptr_TexCoord1sv GET_TexCoord1sv(struct _glapi_table *disp) {
   return (_glptr_TexCoord1sv) (GET_by_offset(disp, _gloffset_TexCoord1sv));
}

static inline void SET_TexCoord1sv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_TexCoord1sv, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord2d)(GLdouble, GLdouble);
#define CALL_TexCoord2d(disp, parameters) \
    (* GET_TexCoord2d(disp)) parameters
static inline _glptr_TexCoord2d GET_TexCoord2d(struct _glapi_table *disp) {
   return (_glptr_TexCoord2d) (GET_by_offset(disp, _gloffset_TexCoord2d));
}

static inline void SET_TexCoord2d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_TexCoord2d, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord2dv)(const GLdouble *);
#define CALL_TexCoord2dv(disp, parameters) \
    (* GET_TexCoord2dv(disp)) parameters
static inline _glptr_TexCoord2dv GET_TexCoord2dv(struct _glapi_table *disp) {
   return (_glptr_TexCoord2dv) (GET_by_offset(disp, _gloffset_TexCoord2dv));
}

static inline void SET_TexCoord2dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_TexCoord2dv, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord2f)(GLfloat, GLfloat);
#define CALL_TexCoord2f(disp, parameters) \
    (* GET_TexCoord2f(disp)) parameters
static inline _glptr_TexCoord2f GET_TexCoord2f(struct _glapi_table *disp) {
   return (_glptr_TexCoord2f) (GET_by_offset(disp, _gloffset_TexCoord2f));
}

static inline void SET_TexCoord2f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_TexCoord2f, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord2fv)(const GLfloat *);
#define CALL_TexCoord2fv(disp, parameters) \
    (* GET_TexCoord2fv(disp)) parameters
static inline _glptr_TexCoord2fv GET_TexCoord2fv(struct _glapi_table *disp) {
   return (_glptr_TexCoord2fv) (GET_by_offset(disp, _gloffset_TexCoord2fv));
}

static inline void SET_TexCoord2fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_TexCoord2fv, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord2i)(GLint, GLint);
#define CALL_TexCoord2i(disp, parameters) \
    (* GET_TexCoord2i(disp)) parameters
static inline _glptr_TexCoord2i GET_TexCoord2i(struct _glapi_table *disp) {
   return (_glptr_TexCoord2i) (GET_by_offset(disp, _gloffset_TexCoord2i));
}

static inline void SET_TexCoord2i(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint)) {
   SET_by_offset(disp, _gloffset_TexCoord2i, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord2iv)(const GLint *);
#define CALL_TexCoord2iv(disp, parameters) \
    (* GET_TexCoord2iv(disp)) parameters
static inline _glptr_TexCoord2iv GET_TexCoord2iv(struct _glapi_table *disp) {
   return (_glptr_TexCoord2iv) (GET_by_offset(disp, _gloffset_TexCoord2iv));
}

static inline void SET_TexCoord2iv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_TexCoord2iv, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord2s)(GLshort, GLshort);
#define CALL_TexCoord2s(disp, parameters) \
    (* GET_TexCoord2s(disp)) parameters
static inline _glptr_TexCoord2s GET_TexCoord2s(struct _glapi_table *disp) {
   return (_glptr_TexCoord2s) (GET_by_offset(disp, _gloffset_TexCoord2s));
}

static inline void SET_TexCoord2s(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_TexCoord2s, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord2sv)(const GLshort *);
#define CALL_TexCoord2sv(disp, parameters) \
    (* GET_TexCoord2sv(disp)) parameters
static inline _glptr_TexCoord2sv GET_TexCoord2sv(struct _glapi_table *disp) {
   return (_glptr_TexCoord2sv) (GET_by_offset(disp, _gloffset_TexCoord2sv));
}

static inline void SET_TexCoord2sv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_TexCoord2sv, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord3d)(GLdouble, GLdouble, GLdouble);
#define CALL_TexCoord3d(disp, parameters) \
    (* GET_TexCoord3d(disp)) parameters
static inline _glptr_TexCoord3d GET_TexCoord3d(struct _glapi_table *disp) {
   return (_glptr_TexCoord3d) (GET_by_offset(disp, _gloffset_TexCoord3d));
}

static inline void SET_TexCoord3d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_TexCoord3d, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord3dv)(const GLdouble *);
#define CALL_TexCoord3dv(disp, parameters) \
    (* GET_TexCoord3dv(disp)) parameters
static inline _glptr_TexCoord3dv GET_TexCoord3dv(struct _glapi_table *disp) {
   return (_glptr_TexCoord3dv) (GET_by_offset(disp, _gloffset_TexCoord3dv));
}

static inline void SET_TexCoord3dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_TexCoord3dv, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord3f)(GLfloat, GLfloat, GLfloat);
#define CALL_TexCoord3f(disp, parameters) \
    (* GET_TexCoord3f(disp)) parameters
static inline _glptr_TexCoord3f GET_TexCoord3f(struct _glapi_table *disp) {
   return (_glptr_TexCoord3f) (GET_by_offset(disp, _gloffset_TexCoord3f));
}

static inline void SET_TexCoord3f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_TexCoord3f, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord3fv)(const GLfloat *);
#define CALL_TexCoord3fv(disp, parameters) \
    (* GET_TexCoord3fv(disp)) parameters
static inline _glptr_TexCoord3fv GET_TexCoord3fv(struct _glapi_table *disp) {
   return (_glptr_TexCoord3fv) (GET_by_offset(disp, _gloffset_TexCoord3fv));
}

static inline void SET_TexCoord3fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_TexCoord3fv, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord3i)(GLint, GLint, GLint);
#define CALL_TexCoord3i(disp, parameters) \
    (* GET_TexCoord3i(disp)) parameters
static inline _glptr_TexCoord3i GET_TexCoord3i(struct _glapi_table *disp) {
   return (_glptr_TexCoord3i) (GET_by_offset(disp, _gloffset_TexCoord3i));
}

static inline void SET_TexCoord3i(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_TexCoord3i, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord3iv)(const GLint *);
#define CALL_TexCoord3iv(disp, parameters) \
    (* GET_TexCoord3iv(disp)) parameters
static inline _glptr_TexCoord3iv GET_TexCoord3iv(struct _glapi_table *disp) {
   return (_glptr_TexCoord3iv) (GET_by_offset(disp, _gloffset_TexCoord3iv));
}

static inline void SET_TexCoord3iv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_TexCoord3iv, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord3s)(GLshort, GLshort, GLshort);
#define CALL_TexCoord3s(disp, parameters) \
    (* GET_TexCoord3s(disp)) parameters
static inline _glptr_TexCoord3s GET_TexCoord3s(struct _glapi_table *disp) {
   return (_glptr_TexCoord3s) (GET_by_offset(disp, _gloffset_TexCoord3s));
}

static inline void SET_TexCoord3s(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_TexCoord3s, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord3sv)(const GLshort *);
#define CALL_TexCoord3sv(disp, parameters) \
    (* GET_TexCoord3sv(disp)) parameters
static inline _glptr_TexCoord3sv GET_TexCoord3sv(struct _glapi_table *disp) {
   return (_glptr_TexCoord3sv) (GET_by_offset(disp, _gloffset_TexCoord3sv));
}

static inline void SET_TexCoord3sv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_TexCoord3sv, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord4d)(GLdouble, GLdouble, GLdouble, GLdouble);
#define CALL_TexCoord4d(disp, parameters) \
    (* GET_TexCoord4d(disp)) parameters
static inline _glptr_TexCoord4d GET_TexCoord4d(struct _glapi_table *disp) {
   return (_glptr_TexCoord4d) (GET_by_offset(disp, _gloffset_TexCoord4d));
}

static inline void SET_TexCoord4d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_TexCoord4d, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord4dv)(const GLdouble *);
#define CALL_TexCoord4dv(disp, parameters) \
    (* GET_TexCoord4dv(disp)) parameters
static inline _glptr_TexCoord4dv GET_TexCoord4dv(struct _glapi_table *disp) {
   return (_glptr_TexCoord4dv) (GET_by_offset(disp, _gloffset_TexCoord4dv));
}

static inline void SET_TexCoord4dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_TexCoord4dv, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord4f)(GLfloat, GLfloat, GLfloat, GLfloat);
#define CALL_TexCoord4f(disp, parameters) \
    (* GET_TexCoord4f(disp)) parameters
static inline _glptr_TexCoord4f GET_TexCoord4f(struct _glapi_table *disp) {
   return (_glptr_TexCoord4f) (GET_by_offset(disp, _gloffset_TexCoord4f));
}

static inline void SET_TexCoord4f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_TexCoord4f, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord4fv)(const GLfloat *);
#define CALL_TexCoord4fv(disp, parameters) \
    (* GET_TexCoord4fv(disp)) parameters
static inline _glptr_TexCoord4fv GET_TexCoord4fv(struct _glapi_table *disp) {
   return (_glptr_TexCoord4fv) (GET_by_offset(disp, _gloffset_TexCoord4fv));
}

static inline void SET_TexCoord4fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_TexCoord4fv, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord4i)(GLint, GLint, GLint, GLint);
#define CALL_TexCoord4i(disp, parameters) \
    (* GET_TexCoord4i(disp)) parameters
static inline _glptr_TexCoord4i GET_TexCoord4i(struct _glapi_table *disp) {
   return (_glptr_TexCoord4i) (GET_by_offset(disp, _gloffset_TexCoord4i));
}

static inline void SET_TexCoord4i(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_TexCoord4i, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord4iv)(const GLint *);
#define CALL_TexCoord4iv(disp, parameters) \
    (* GET_TexCoord4iv(disp)) parameters
static inline _glptr_TexCoord4iv GET_TexCoord4iv(struct _glapi_table *disp) {
   return (_glptr_TexCoord4iv) (GET_by_offset(disp, _gloffset_TexCoord4iv));
}

static inline void SET_TexCoord4iv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_TexCoord4iv, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord4s)(GLshort, GLshort, GLshort, GLshort);
#define CALL_TexCoord4s(disp, parameters) \
    (* GET_TexCoord4s(disp)) parameters
static inline _glptr_TexCoord4s GET_TexCoord4s(struct _glapi_table *disp) {
   return (_glptr_TexCoord4s) (GET_by_offset(disp, _gloffset_TexCoord4s));
}

static inline void SET_TexCoord4s(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_TexCoord4s, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoord4sv)(const GLshort *);
#define CALL_TexCoord4sv(disp, parameters) \
    (* GET_TexCoord4sv(disp)) parameters
static inline _glptr_TexCoord4sv GET_TexCoord4sv(struct _glapi_table *disp) {
   return (_glptr_TexCoord4sv) (GET_by_offset(disp, _gloffset_TexCoord4sv));
}

static inline void SET_TexCoord4sv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_TexCoord4sv, fn);
}

typedef void (GLAPIENTRYP _glptr_Vertex2d)(GLdouble, GLdouble);
#define CALL_Vertex2d(disp, parameters) \
    (* GET_Vertex2d(disp)) parameters
static inline _glptr_Vertex2d GET_Vertex2d(struct _glapi_table *disp) {
   return (_glptr_Vertex2d) (GET_by_offset(disp, _gloffset_Vertex2d));
}

static inline void SET_Vertex2d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_Vertex2d, fn);
}

typedef void (GLAPIENTRYP _glptr_Vertex2dv)(const GLdouble *);
#define CALL_Vertex2dv(disp, parameters) \
    (* GET_Vertex2dv(disp)) parameters
static inline _glptr_Vertex2dv GET_Vertex2dv(struct _glapi_table *disp) {
   return (_glptr_Vertex2dv) (GET_by_offset(disp, _gloffset_Vertex2dv));
}

static inline void SET_Vertex2dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_Vertex2dv, fn);
}

typedef void (GLAPIENTRYP _glptr_Vertex2f)(GLfloat, GLfloat);
#define CALL_Vertex2f(disp, parameters) \
    (* GET_Vertex2f(disp)) parameters
static inline _glptr_Vertex2f GET_Vertex2f(struct _glapi_table *disp) {
   return (_glptr_Vertex2f) (GET_by_offset(disp, _gloffset_Vertex2f));
}

static inline void SET_Vertex2f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_Vertex2f, fn);
}

typedef void (GLAPIENTRYP _glptr_Vertex2fv)(const GLfloat *);
#define CALL_Vertex2fv(disp, parameters) \
    (* GET_Vertex2fv(disp)) parameters
static inline _glptr_Vertex2fv GET_Vertex2fv(struct _glapi_table *disp) {
   return (_glptr_Vertex2fv) (GET_by_offset(disp, _gloffset_Vertex2fv));
}

static inline void SET_Vertex2fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Vertex2fv, fn);
}

typedef void (GLAPIENTRYP _glptr_Vertex2i)(GLint, GLint);
#define CALL_Vertex2i(disp, parameters) \
    (* GET_Vertex2i(disp)) parameters
static inline _glptr_Vertex2i GET_Vertex2i(struct _glapi_table *disp) {
   return (_glptr_Vertex2i) (GET_by_offset(disp, _gloffset_Vertex2i));
}

static inline void SET_Vertex2i(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint)) {
   SET_by_offset(disp, _gloffset_Vertex2i, fn);
}

typedef void (GLAPIENTRYP _glptr_Vertex2iv)(const GLint *);
#define CALL_Vertex2iv(disp, parameters) \
    (* GET_Vertex2iv(disp)) parameters
static inline _glptr_Vertex2iv GET_Vertex2iv(struct _glapi_table *disp) {
   return (_glptr_Vertex2iv) (GET_by_offset(disp, _gloffset_Vertex2iv));
}

static inline void SET_Vertex2iv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_Vertex2iv, fn);
}

typedef void (GLAPIENTRYP _glptr_Vertex2s)(GLshort, GLshort);
#define CALL_Vertex2s(disp, parameters) \
    (* GET_Vertex2s(disp)) parameters
static inline _glptr_Vertex2s GET_Vertex2s(struct _glapi_table *disp) {
   return (_glptr_Vertex2s) (GET_by_offset(disp, _gloffset_Vertex2s));
}

static inline void SET_Vertex2s(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_Vertex2s, fn);
}

typedef void (GLAPIENTRYP _glptr_Vertex2sv)(const GLshort *);
#define CALL_Vertex2sv(disp, parameters) \
    (* GET_Vertex2sv(disp)) parameters
static inline _glptr_Vertex2sv GET_Vertex2sv(struct _glapi_table *disp) {
   return (_glptr_Vertex2sv) (GET_by_offset(disp, _gloffset_Vertex2sv));
}

static inline void SET_Vertex2sv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_Vertex2sv, fn);
}

typedef void (GLAPIENTRYP _glptr_Vertex3d)(GLdouble, GLdouble, GLdouble);
#define CALL_Vertex3d(disp, parameters) \
    (* GET_Vertex3d(disp)) parameters
static inline _glptr_Vertex3d GET_Vertex3d(struct _glapi_table *disp) {
   return (_glptr_Vertex3d) (GET_by_offset(disp, _gloffset_Vertex3d));
}

static inline void SET_Vertex3d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_Vertex3d, fn);
}

typedef void (GLAPIENTRYP _glptr_Vertex3dv)(const GLdouble *);
#define CALL_Vertex3dv(disp, parameters) \
    (* GET_Vertex3dv(disp)) parameters
static inline _glptr_Vertex3dv GET_Vertex3dv(struct _glapi_table *disp) {
   return (_glptr_Vertex3dv) (GET_by_offset(disp, _gloffset_Vertex3dv));
}

static inline void SET_Vertex3dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_Vertex3dv, fn);
}

typedef void (GLAPIENTRYP _glptr_Vertex3f)(GLfloat, GLfloat, GLfloat);
#define CALL_Vertex3f(disp, parameters) \
    (* GET_Vertex3f(disp)) parameters
static inline _glptr_Vertex3f GET_Vertex3f(struct _glapi_table *disp) {
   return (_glptr_Vertex3f) (GET_by_offset(disp, _gloffset_Vertex3f));
}

static inline void SET_Vertex3f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_Vertex3f, fn);
}

typedef void (GLAPIENTRYP _glptr_Vertex3fv)(const GLfloat *);
#define CALL_Vertex3fv(disp, parameters) \
    (* GET_Vertex3fv(disp)) parameters
static inline _glptr_Vertex3fv GET_Vertex3fv(struct _glapi_table *disp) {
   return (_glptr_Vertex3fv) (GET_by_offset(disp, _gloffset_Vertex3fv));
}

static inline void SET_Vertex3fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Vertex3fv, fn);
}

typedef void (GLAPIENTRYP _glptr_Vertex3i)(GLint, GLint, GLint);
#define CALL_Vertex3i(disp, parameters) \
    (* GET_Vertex3i(disp)) parameters
static inline _glptr_Vertex3i GET_Vertex3i(struct _glapi_table *disp) {
   return (_glptr_Vertex3i) (GET_by_offset(disp, _gloffset_Vertex3i));
}

static inline void SET_Vertex3i(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_Vertex3i, fn);
}

typedef void (GLAPIENTRYP _glptr_Vertex3iv)(const GLint *);
#define CALL_Vertex3iv(disp, parameters) \
    (* GET_Vertex3iv(disp)) parameters
static inline _glptr_Vertex3iv GET_Vertex3iv(struct _glapi_table *disp) {
   return (_glptr_Vertex3iv) (GET_by_offset(disp, _gloffset_Vertex3iv));
}

static inline void SET_Vertex3iv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_Vertex3iv, fn);
}

typedef void (GLAPIENTRYP _glptr_Vertex3s)(GLshort, GLshort, GLshort);
#define CALL_Vertex3s(disp, parameters) \
    (* GET_Vertex3s(disp)) parameters
static inline _glptr_Vertex3s GET_Vertex3s(struct _glapi_table *disp) {
   return (_glptr_Vertex3s) (GET_by_offset(disp, _gloffset_Vertex3s));
}

static inline void SET_Vertex3s(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_Vertex3s, fn);
}

typedef void (GLAPIENTRYP _glptr_Vertex3sv)(const GLshort *);
#define CALL_Vertex3sv(disp, parameters) \
    (* GET_Vertex3sv(disp)) parameters
static inline _glptr_Vertex3sv GET_Vertex3sv(struct _glapi_table *disp) {
   return (_glptr_Vertex3sv) (GET_by_offset(disp, _gloffset_Vertex3sv));
}

static inline void SET_Vertex3sv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_Vertex3sv, fn);
}

typedef void (GLAPIENTRYP _glptr_Vertex4d)(GLdouble, GLdouble, GLdouble, GLdouble);
#define CALL_Vertex4d(disp, parameters) \
    (* GET_Vertex4d(disp)) parameters
static inline _glptr_Vertex4d GET_Vertex4d(struct _glapi_table *disp) {
   return (_glptr_Vertex4d) (GET_by_offset(disp, _gloffset_Vertex4d));
}

static inline void SET_Vertex4d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_Vertex4d, fn);
}

typedef void (GLAPIENTRYP _glptr_Vertex4dv)(const GLdouble *);
#define CALL_Vertex4dv(disp, parameters) \
    (* GET_Vertex4dv(disp)) parameters
static inline _glptr_Vertex4dv GET_Vertex4dv(struct _glapi_table *disp) {
   return (_glptr_Vertex4dv) (GET_by_offset(disp, _gloffset_Vertex4dv));
}

static inline void SET_Vertex4dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_Vertex4dv, fn);
}

typedef void (GLAPIENTRYP _glptr_Vertex4f)(GLfloat, GLfloat, GLfloat, GLfloat);
#define CALL_Vertex4f(disp, parameters) \
    (* GET_Vertex4f(disp)) parameters
static inline _glptr_Vertex4f GET_Vertex4f(struct _glapi_table *disp) {
   return (_glptr_Vertex4f) (GET_by_offset(disp, _gloffset_Vertex4f));
}

static inline void SET_Vertex4f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_Vertex4f, fn);
}

typedef void (GLAPIENTRYP _glptr_Vertex4fv)(const GLfloat *);
#define CALL_Vertex4fv(disp, parameters) \
    (* GET_Vertex4fv(disp)) parameters
static inline _glptr_Vertex4fv GET_Vertex4fv(struct _glapi_table *disp) {
   return (_glptr_Vertex4fv) (GET_by_offset(disp, _gloffset_Vertex4fv));
}

static inline void SET_Vertex4fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Vertex4fv, fn);
}

typedef void (GLAPIENTRYP _glptr_Vertex4i)(GLint, GLint, GLint, GLint);
#define CALL_Vertex4i(disp, parameters) \
    (* GET_Vertex4i(disp)) parameters
static inline _glptr_Vertex4i GET_Vertex4i(struct _glapi_table *disp) {
   return (_glptr_Vertex4i) (GET_by_offset(disp, _gloffset_Vertex4i));
}

static inline void SET_Vertex4i(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_Vertex4i, fn);
}

typedef void (GLAPIENTRYP _glptr_Vertex4iv)(const GLint *);
#define CALL_Vertex4iv(disp, parameters) \
    (* GET_Vertex4iv(disp)) parameters
static inline _glptr_Vertex4iv GET_Vertex4iv(struct _glapi_table *disp) {
   return (_glptr_Vertex4iv) (GET_by_offset(disp, _gloffset_Vertex4iv));
}

static inline void SET_Vertex4iv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_Vertex4iv, fn);
}

typedef void (GLAPIENTRYP _glptr_Vertex4s)(GLshort, GLshort, GLshort, GLshort);
#define CALL_Vertex4s(disp, parameters) \
    (* GET_Vertex4s(disp)) parameters
static inline _glptr_Vertex4s GET_Vertex4s(struct _glapi_table *disp) {
   return (_glptr_Vertex4s) (GET_by_offset(disp, _gloffset_Vertex4s));
}

static inline void SET_Vertex4s(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_Vertex4s, fn);
}

typedef void (GLAPIENTRYP _glptr_Vertex4sv)(const GLshort *);
#define CALL_Vertex4sv(disp, parameters) \
    (* GET_Vertex4sv(disp)) parameters
static inline _glptr_Vertex4sv GET_Vertex4sv(struct _glapi_table *disp) {
   return (_glptr_Vertex4sv) (GET_by_offset(disp, _gloffset_Vertex4sv));
}

static inline void SET_Vertex4sv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_Vertex4sv, fn);
}

typedef void (GLAPIENTRYP _glptr_ClipPlane)(GLenum, const GLdouble *);
#define CALL_ClipPlane(disp, parameters) \
    (* GET_ClipPlane(disp)) parameters
static inline _glptr_ClipPlane GET_ClipPlane(struct _glapi_table *disp) {
   return (_glptr_ClipPlane) (GET_by_offset(disp, _gloffset_ClipPlane));
}

static inline void SET_ClipPlane(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_ClipPlane, fn);
}

typedef void (GLAPIENTRYP _glptr_ColorMaterial)(GLenum, GLenum);
#define CALL_ColorMaterial(disp, parameters) \
    (* GET_ColorMaterial(disp)) parameters
static inline _glptr_ColorMaterial GET_ColorMaterial(struct _glapi_table *disp) {
   return (_glptr_ColorMaterial) (GET_by_offset(disp, _gloffset_ColorMaterial));
}

static inline void SET_ColorMaterial(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_ColorMaterial, fn);
}

typedef void (GLAPIENTRYP _glptr_CullFace)(GLenum);
#define CALL_CullFace(disp, parameters) \
    (* GET_CullFace(disp)) parameters
static inline _glptr_CullFace GET_CullFace(struct _glapi_table *disp) {
   return (_glptr_CullFace) (GET_by_offset(disp, _gloffset_CullFace));
}

static inline void SET_CullFace(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_CullFace, fn);
}

typedef void (GLAPIENTRYP _glptr_Fogf)(GLenum, GLfloat);
#define CALL_Fogf(disp, parameters) \
    (* GET_Fogf(disp)) parameters
static inline _glptr_Fogf GET_Fogf(struct _glapi_table *disp) {
   return (_glptr_Fogf) (GET_by_offset(disp, _gloffset_Fogf));
}

static inline void SET_Fogf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_Fogf, fn);
}

typedef void (GLAPIENTRYP _glptr_Fogfv)(GLenum, const GLfloat *);
#define CALL_Fogfv(disp, parameters) \
    (* GET_Fogfv(disp)) parameters
static inline _glptr_Fogfv GET_Fogfv(struct _glapi_table *disp) {
   return (_glptr_Fogfv) (GET_by_offset(disp, _gloffset_Fogfv));
}

static inline void SET_Fogfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Fogfv, fn);
}

typedef void (GLAPIENTRYP _glptr_Fogi)(GLenum, GLint);
#define CALL_Fogi(disp, parameters) \
    (* GET_Fogi(disp)) parameters
static inline _glptr_Fogi GET_Fogi(struct _glapi_table *disp) {
   return (_glptr_Fogi) (GET_by_offset(disp, _gloffset_Fogi));
}

static inline void SET_Fogi(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_Fogi, fn);
}

typedef void (GLAPIENTRYP _glptr_Fogiv)(GLenum, const GLint *);
#define CALL_Fogiv(disp, parameters) \
    (* GET_Fogiv(disp)) parameters
static inline _glptr_Fogiv GET_Fogiv(struct _glapi_table *disp) {
   return (_glptr_Fogiv) (GET_by_offset(disp, _gloffset_Fogiv));
}

static inline void SET_Fogiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_Fogiv, fn);
}

typedef void (GLAPIENTRYP _glptr_FrontFace)(GLenum);
#define CALL_FrontFace(disp, parameters) \
    (* GET_FrontFace(disp)) parameters
static inline _glptr_FrontFace GET_FrontFace(struct _glapi_table *disp) {
   return (_glptr_FrontFace) (GET_by_offset(disp, _gloffset_FrontFace));
}

static inline void SET_FrontFace(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_FrontFace, fn);
}

typedef void (GLAPIENTRYP _glptr_Hint)(GLenum, GLenum);
#define CALL_Hint(disp, parameters) \
    (* GET_Hint(disp)) parameters
static inline _glptr_Hint GET_Hint(struct _glapi_table *disp) {
   return (_glptr_Hint) (GET_by_offset(disp, _gloffset_Hint));
}

static inline void SET_Hint(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_Hint, fn);
}

typedef void (GLAPIENTRYP _glptr_Lightf)(GLenum, GLenum, GLfloat);
#define CALL_Lightf(disp, parameters) \
    (* GET_Lightf(disp)) parameters
static inline _glptr_Lightf GET_Lightf(struct _glapi_table *disp) {
   return (_glptr_Lightf) (GET_by_offset(disp, _gloffset_Lightf));
}

static inline void SET_Lightf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_Lightf, fn);
}

typedef void (GLAPIENTRYP _glptr_Lightfv)(GLenum, GLenum, const GLfloat *);
#define CALL_Lightfv(disp, parameters) \
    (* GET_Lightfv(disp)) parameters
static inline _glptr_Lightfv GET_Lightfv(struct _glapi_table *disp) {
   return (_glptr_Lightfv) (GET_by_offset(disp, _gloffset_Lightfv));
}

static inline void SET_Lightfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Lightfv, fn);
}

typedef void (GLAPIENTRYP _glptr_Lighti)(GLenum, GLenum, GLint);
#define CALL_Lighti(disp, parameters) \
    (* GET_Lighti(disp)) parameters
static inline _glptr_Lighti GET_Lighti(struct _glapi_table *disp) {
   return (_glptr_Lighti) (GET_by_offset(disp, _gloffset_Lighti));
}

static inline void SET_Lighti(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_Lighti, fn);
}

typedef void (GLAPIENTRYP _glptr_Lightiv)(GLenum, GLenum, const GLint *);
#define CALL_Lightiv(disp, parameters) \
    (* GET_Lightiv(disp)) parameters
static inline _glptr_Lightiv GET_Lightiv(struct _glapi_table *disp) {
   return (_glptr_Lightiv) (GET_by_offset(disp, _gloffset_Lightiv));
}

static inline void SET_Lightiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_Lightiv, fn);
}

typedef void (GLAPIENTRYP _glptr_LightModelf)(GLenum, GLfloat);
#define CALL_LightModelf(disp, parameters) \
    (* GET_LightModelf(disp)) parameters
static inline _glptr_LightModelf GET_LightModelf(struct _glapi_table *disp) {
   return (_glptr_LightModelf) (GET_by_offset(disp, _gloffset_LightModelf));
}

static inline void SET_LightModelf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_LightModelf, fn);
}

typedef void (GLAPIENTRYP _glptr_LightModelfv)(GLenum, const GLfloat *);
#define CALL_LightModelfv(disp, parameters) \
    (* GET_LightModelfv(disp)) parameters
static inline _glptr_LightModelfv GET_LightModelfv(struct _glapi_table *disp) {
   return (_glptr_LightModelfv) (GET_by_offset(disp, _gloffset_LightModelfv));
}

static inline void SET_LightModelfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_LightModelfv, fn);
}

typedef void (GLAPIENTRYP _glptr_LightModeli)(GLenum, GLint);
#define CALL_LightModeli(disp, parameters) \
    (* GET_LightModeli(disp)) parameters
static inline _glptr_LightModeli GET_LightModeli(struct _glapi_table *disp) {
   return (_glptr_LightModeli) (GET_by_offset(disp, _gloffset_LightModeli));
}

static inline void SET_LightModeli(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_LightModeli, fn);
}

typedef void (GLAPIENTRYP _glptr_LightModeliv)(GLenum, const GLint *);
#define CALL_LightModeliv(disp, parameters) \
    (* GET_LightModeliv(disp)) parameters
static inline _glptr_LightModeliv GET_LightModeliv(struct _glapi_table *disp) {
   return (_glptr_LightModeliv) (GET_by_offset(disp, _gloffset_LightModeliv));
}

static inline void SET_LightModeliv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_LightModeliv, fn);
}

typedef void (GLAPIENTRYP _glptr_LineStipple)(GLint, GLushort);
#define CALL_LineStipple(disp, parameters) \
    (* GET_LineStipple(disp)) parameters
static inline _glptr_LineStipple GET_LineStipple(struct _glapi_table *disp) {
   return (_glptr_LineStipple) (GET_by_offset(disp, _gloffset_LineStipple));
}

static inline void SET_LineStipple(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLushort)) {
   SET_by_offset(disp, _gloffset_LineStipple, fn);
}

typedef void (GLAPIENTRYP _glptr_LineWidth)(GLfloat);
#define CALL_LineWidth(disp, parameters) \
    (* GET_LineWidth(disp)) parameters
static inline _glptr_LineWidth GET_LineWidth(struct _glapi_table *disp) {
   return (_glptr_LineWidth) (GET_by_offset(disp, _gloffset_LineWidth));
}

static inline void SET_LineWidth(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat)) {
   SET_by_offset(disp, _gloffset_LineWidth, fn);
}

typedef void (GLAPIENTRYP _glptr_Materialf)(GLenum, GLenum, GLfloat);
#define CALL_Materialf(disp, parameters) \
    (* GET_Materialf(disp)) parameters
static inline _glptr_Materialf GET_Materialf(struct _glapi_table *disp) {
   return (_glptr_Materialf) (GET_by_offset(disp, _gloffset_Materialf));
}

static inline void SET_Materialf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_Materialf, fn);
}

typedef void (GLAPIENTRYP _glptr_Materialfv)(GLenum, GLenum, const GLfloat *);
#define CALL_Materialfv(disp, parameters) \
    (* GET_Materialfv(disp)) parameters
static inline _glptr_Materialfv GET_Materialfv(struct _glapi_table *disp) {
   return (_glptr_Materialfv) (GET_by_offset(disp, _gloffset_Materialfv));
}

static inline void SET_Materialfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Materialfv, fn);
}

typedef void (GLAPIENTRYP _glptr_Materiali)(GLenum, GLenum, GLint);
#define CALL_Materiali(disp, parameters) \
    (* GET_Materiali(disp)) parameters
static inline _glptr_Materiali GET_Materiali(struct _glapi_table *disp) {
   return (_glptr_Materiali) (GET_by_offset(disp, _gloffset_Materiali));
}

static inline void SET_Materiali(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_Materiali, fn);
}

typedef void (GLAPIENTRYP _glptr_Materialiv)(GLenum, GLenum, const GLint *);
#define CALL_Materialiv(disp, parameters) \
    (* GET_Materialiv(disp)) parameters
static inline _glptr_Materialiv GET_Materialiv(struct _glapi_table *disp) {
   return (_glptr_Materialiv) (GET_by_offset(disp, _gloffset_Materialiv));
}

static inline void SET_Materialiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_Materialiv, fn);
}

typedef void (GLAPIENTRYP _glptr_PointSize)(GLfloat);
#define CALL_PointSize(disp, parameters) \
    (* GET_PointSize(disp)) parameters
static inline _glptr_PointSize GET_PointSize(struct _glapi_table *disp) {
   return (_glptr_PointSize) (GET_by_offset(disp, _gloffset_PointSize));
}

static inline void SET_PointSize(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat)) {
   SET_by_offset(disp, _gloffset_PointSize, fn);
}

typedef void (GLAPIENTRYP _glptr_PolygonMode)(GLenum, GLenum);
#define CALL_PolygonMode(disp, parameters) \
    (* GET_PolygonMode(disp)) parameters
static inline _glptr_PolygonMode GET_PolygonMode(struct _glapi_table *disp) {
   return (_glptr_PolygonMode) (GET_by_offset(disp, _gloffset_PolygonMode));
}

static inline void SET_PolygonMode(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_PolygonMode, fn);
}

typedef void (GLAPIENTRYP _glptr_PolygonStipple)(const GLubyte *);
#define CALL_PolygonStipple(disp, parameters) \
    (* GET_PolygonStipple(disp)) parameters
static inline _glptr_PolygonStipple GET_PolygonStipple(struct _glapi_table *disp) {
   return (_glptr_PolygonStipple) (GET_by_offset(disp, _gloffset_PolygonStipple));
}

static inline void SET_PolygonStipple(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLubyte *)) {
   SET_by_offset(disp, _gloffset_PolygonStipple, fn);
}

typedef void (GLAPIENTRYP _glptr_Scissor)(GLint, GLint, GLsizei, GLsizei);
#define CALL_Scissor(disp, parameters) \
    (* GET_Scissor(disp)) parameters
static inline _glptr_Scissor GET_Scissor(struct _glapi_table *disp) {
   return (_glptr_Scissor) (GET_by_offset(disp, _gloffset_Scissor));
}

static inline void SET_Scissor(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLsizei, GLsizei)) {
   SET_by_offset(disp, _gloffset_Scissor, fn);
}

typedef void (GLAPIENTRYP _glptr_ShadeModel)(GLenum);
#define CALL_ShadeModel(disp, parameters) \
    (* GET_ShadeModel(disp)) parameters
static inline _glptr_ShadeModel GET_ShadeModel(struct _glapi_table *disp) {
   return (_glptr_ShadeModel) (GET_by_offset(disp, _gloffset_ShadeModel));
}

static inline void SET_ShadeModel(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_ShadeModel, fn);
}

typedef void (GLAPIENTRYP _glptr_TexParameterf)(GLenum, GLenum, GLfloat);
#define CALL_TexParameterf(disp, parameters) \
    (* GET_TexParameterf(disp)) parameters
static inline _glptr_TexParameterf GET_TexParameterf(struct _glapi_table *disp) {
   return (_glptr_TexParameterf) (GET_by_offset(disp, _gloffset_TexParameterf));
}

static inline void SET_TexParameterf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_TexParameterf, fn);
}

typedef void (GLAPIENTRYP _glptr_TexParameterfv)(GLenum, GLenum, const GLfloat *);
#define CALL_TexParameterfv(disp, parameters) \
    (* GET_TexParameterfv(disp)) parameters
static inline _glptr_TexParameterfv GET_TexParameterfv(struct _glapi_table *disp) {
   return (_glptr_TexParameterfv) (GET_by_offset(disp, _gloffset_TexParameterfv));
}

static inline void SET_TexParameterfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_TexParameterfv, fn);
}

typedef void (GLAPIENTRYP _glptr_TexParameteri)(GLenum, GLenum, GLint);
#define CALL_TexParameteri(disp, parameters) \
    (* GET_TexParameteri(disp)) parameters
static inline _glptr_TexParameteri GET_TexParameteri(struct _glapi_table *disp) {
   return (_glptr_TexParameteri) (GET_by_offset(disp, _gloffset_TexParameteri));
}

static inline void SET_TexParameteri(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_TexParameteri, fn);
}

typedef void (GLAPIENTRYP _glptr_TexParameteriv)(GLenum, GLenum, const GLint *);
#define CALL_TexParameteriv(disp, parameters) \
    (* GET_TexParameteriv(disp)) parameters
static inline _glptr_TexParameteriv GET_TexParameteriv(struct _glapi_table *disp) {
   return (_glptr_TexParameteriv) (GET_by_offset(disp, _gloffset_TexParameteriv));
}

static inline void SET_TexParameteriv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_TexParameteriv, fn);
}

typedef void (GLAPIENTRYP _glptr_TexImage1D)(GLenum, GLint, GLint, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
#define CALL_TexImage1D(disp, parameters) \
    (* GET_TexImage1D(disp)) parameters
static inline _glptr_TexImage1D GET_TexImage1D(struct _glapi_table *disp) {
   return (_glptr_TexImage1D) (GET_by_offset(disp, _gloffset_TexImage1D));
}

static inline void SET_TexImage1D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLsizei, GLint, GLenum, GLenum, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_TexImage1D, fn);
}

typedef void (GLAPIENTRYP _glptr_TexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
#define CALL_TexImage2D(disp, parameters) \
    (* GET_TexImage2D(disp)) parameters
static inline _glptr_TexImage2D GET_TexImage2D(struct _glapi_table *disp) {
   return (_glptr_TexImage2D) (GET_by_offset(disp, _gloffset_TexImage2D));
}

static inline void SET_TexImage2D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_TexImage2D, fn);
}

typedef void (GLAPIENTRYP _glptr_TexEnvf)(GLenum, GLenum, GLfloat);
#define CALL_TexEnvf(disp, parameters) \
    (* GET_TexEnvf(disp)) parameters
static inline _glptr_TexEnvf GET_TexEnvf(struct _glapi_table *disp) {
   return (_glptr_TexEnvf) (GET_by_offset(disp, _gloffset_TexEnvf));
}

static inline void SET_TexEnvf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_TexEnvf, fn);
}

typedef void (GLAPIENTRYP _glptr_TexEnvfv)(GLenum, GLenum, const GLfloat *);
#define CALL_TexEnvfv(disp, parameters) \
    (* GET_TexEnvfv(disp)) parameters
static inline _glptr_TexEnvfv GET_TexEnvfv(struct _glapi_table *disp) {
   return (_glptr_TexEnvfv) (GET_by_offset(disp, _gloffset_TexEnvfv));
}

static inline void SET_TexEnvfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_TexEnvfv, fn);
}

typedef void (GLAPIENTRYP _glptr_TexEnvi)(GLenum, GLenum, GLint);
#define CALL_TexEnvi(disp, parameters) \
    (* GET_TexEnvi(disp)) parameters
static inline _glptr_TexEnvi GET_TexEnvi(struct _glapi_table *disp) {
   return (_glptr_TexEnvi) (GET_by_offset(disp, _gloffset_TexEnvi));
}

static inline void SET_TexEnvi(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_TexEnvi, fn);
}

typedef void (GLAPIENTRYP _glptr_TexEnviv)(GLenum, GLenum, const GLint *);
#define CALL_TexEnviv(disp, parameters) \
    (* GET_TexEnviv(disp)) parameters
static inline _glptr_TexEnviv GET_TexEnviv(struct _glapi_table *disp) {
   return (_glptr_TexEnviv) (GET_by_offset(disp, _gloffset_TexEnviv));
}

static inline void SET_TexEnviv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_TexEnviv, fn);
}

typedef void (GLAPIENTRYP _glptr_TexGend)(GLenum, GLenum, GLdouble);
#define CALL_TexGend(disp, parameters) \
    (* GET_TexGend(disp)) parameters
static inline _glptr_TexGend GET_TexGend(struct _glapi_table *disp) {
   return (_glptr_TexGend) (GET_by_offset(disp, _gloffset_TexGend));
}

static inline void SET_TexGend(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLdouble)) {
   SET_by_offset(disp, _gloffset_TexGend, fn);
}

typedef void (GLAPIENTRYP _glptr_TexGendv)(GLenum, GLenum, const GLdouble *);
#define CALL_TexGendv(disp, parameters) \
    (* GET_TexGendv(disp)) parameters
static inline _glptr_TexGendv GET_TexGendv(struct _glapi_table *disp) {
   return (_glptr_TexGendv) (GET_by_offset(disp, _gloffset_TexGendv));
}

static inline void SET_TexGendv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_TexGendv, fn);
}

typedef void (GLAPIENTRYP _glptr_TexGenf)(GLenum, GLenum, GLfloat);
#define CALL_TexGenf(disp, parameters) \
    (* GET_TexGenf(disp)) parameters
static inline _glptr_TexGenf GET_TexGenf(struct _glapi_table *disp) {
   return (_glptr_TexGenf) (GET_by_offset(disp, _gloffset_TexGenf));
}

static inline void SET_TexGenf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_TexGenf, fn);
}

typedef void (GLAPIENTRYP _glptr_TexGenfv)(GLenum, GLenum, const GLfloat *);
#define CALL_TexGenfv(disp, parameters) \
    (* GET_TexGenfv(disp)) parameters
static inline _glptr_TexGenfv GET_TexGenfv(struct _glapi_table *disp) {
   return (_glptr_TexGenfv) (GET_by_offset(disp, _gloffset_TexGenfv));
}

static inline void SET_TexGenfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_TexGenfv, fn);
}

typedef void (GLAPIENTRYP _glptr_TexGeni)(GLenum, GLenum, GLint);
#define CALL_TexGeni(disp, parameters) \
    (* GET_TexGeni(disp)) parameters
static inline _glptr_TexGeni GET_TexGeni(struct _glapi_table *disp) {
   return (_glptr_TexGeni) (GET_by_offset(disp, _gloffset_TexGeni));
}

static inline void SET_TexGeni(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_TexGeni, fn);
}

typedef void (GLAPIENTRYP _glptr_TexGeniv)(GLenum, GLenum, const GLint *);
#define CALL_TexGeniv(disp, parameters) \
    (* GET_TexGeniv(disp)) parameters
static inline _glptr_TexGeniv GET_TexGeniv(struct _glapi_table *disp) {
   return (_glptr_TexGeniv) (GET_by_offset(disp, _gloffset_TexGeniv));
}

static inline void SET_TexGeniv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_TexGeniv, fn);
}

typedef void (GLAPIENTRYP _glptr_FeedbackBuffer)(GLsizei, GLenum, GLfloat *);
#define CALL_FeedbackBuffer(disp, parameters) \
    (* GET_FeedbackBuffer(disp)) parameters
static inline _glptr_FeedbackBuffer GET_FeedbackBuffer(struct _glapi_table *disp) {
   return (_glptr_FeedbackBuffer) (GET_by_offset(disp, _gloffset_FeedbackBuffer));
}

static inline void SET_FeedbackBuffer(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_FeedbackBuffer, fn);
}

typedef void (GLAPIENTRYP _glptr_SelectBuffer)(GLsizei, GLuint *);
#define CALL_SelectBuffer(disp, parameters) \
    (* GET_SelectBuffer(disp)) parameters
static inline _glptr_SelectBuffer GET_SelectBuffer(struct _glapi_table *disp) {
   return (_glptr_SelectBuffer) (GET_by_offset(disp, _gloffset_SelectBuffer));
}

static inline void SET_SelectBuffer(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLuint *)) {
   SET_by_offset(disp, _gloffset_SelectBuffer, fn);
}

typedef GLint (GLAPIENTRYP _glptr_RenderMode)(GLenum);
#define CALL_RenderMode(disp, parameters) \
    (* GET_RenderMode(disp)) parameters
static inline _glptr_RenderMode GET_RenderMode(struct _glapi_table *disp) {
   return (_glptr_RenderMode) (GET_by_offset(disp, _gloffset_RenderMode));
}

static inline void SET_RenderMode(struct _glapi_table *disp, GLint (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_RenderMode, fn);
}

typedef void (GLAPIENTRYP _glptr_InitNames)(void);
#define CALL_InitNames(disp, parameters) \
    (* GET_InitNames(disp)) parameters
static inline _glptr_InitNames GET_InitNames(struct _glapi_table *disp) {
   return (_glptr_InitNames) (GET_by_offset(disp, _gloffset_InitNames));
}

static inline void SET_InitNames(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_InitNames, fn);
}

typedef void (GLAPIENTRYP _glptr_LoadName)(GLuint);
#define CALL_LoadName(disp, parameters) \
    (* GET_LoadName(disp)) parameters
static inline _glptr_LoadName GET_LoadName(struct _glapi_table *disp) {
   return (_glptr_LoadName) (GET_by_offset(disp, _gloffset_LoadName));
}

static inline void SET_LoadName(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_LoadName, fn);
}

typedef void (GLAPIENTRYP _glptr_PassThrough)(GLfloat);
#define CALL_PassThrough(disp, parameters) \
    (* GET_PassThrough(disp)) parameters
static inline _glptr_PassThrough GET_PassThrough(struct _glapi_table *disp) {
   return (_glptr_PassThrough) (GET_by_offset(disp, _gloffset_PassThrough));
}

static inline void SET_PassThrough(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat)) {
   SET_by_offset(disp, _gloffset_PassThrough, fn);
}

typedef void (GLAPIENTRYP _glptr_PopName)(void);
#define CALL_PopName(disp, parameters) \
    (* GET_PopName(disp)) parameters
static inline _glptr_PopName GET_PopName(struct _glapi_table *disp) {
   return (_glptr_PopName) (GET_by_offset(disp, _gloffset_PopName));
}

static inline void SET_PopName(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_PopName, fn);
}

typedef void (GLAPIENTRYP _glptr_PushName)(GLuint);
#define CALL_PushName(disp, parameters) \
    (* GET_PushName(disp)) parameters
static inline _glptr_PushName GET_PushName(struct _glapi_table *disp) {
   return (_glptr_PushName) (GET_by_offset(disp, _gloffset_PushName));
}

static inline void SET_PushName(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_PushName, fn);
}

typedef void (GLAPIENTRYP _glptr_DrawBuffer)(GLenum);
#define CALL_DrawBuffer(disp, parameters) \
    (* GET_DrawBuffer(disp)) parameters
static inline _glptr_DrawBuffer GET_DrawBuffer(struct _glapi_table *disp) {
   return (_glptr_DrawBuffer) (GET_by_offset(disp, _gloffset_DrawBuffer));
}

static inline void SET_DrawBuffer(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_DrawBuffer, fn);
}

typedef void (GLAPIENTRYP _glptr_Clear)(GLbitfield);
#define CALL_Clear(disp, parameters) \
    (* GET_Clear(disp)) parameters
static inline _glptr_Clear GET_Clear(struct _glapi_table *disp) {
   return (_glptr_Clear) (GET_by_offset(disp, _gloffset_Clear));
}

static inline void SET_Clear(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLbitfield)) {
   SET_by_offset(disp, _gloffset_Clear, fn);
}

typedef void (GLAPIENTRYP _glptr_ClearAccum)(GLfloat, GLfloat, GLfloat, GLfloat);
#define CALL_ClearAccum(disp, parameters) \
    (* GET_ClearAccum(disp)) parameters
static inline _glptr_ClearAccum GET_ClearAccum(struct _glapi_table *disp) {
   return (_glptr_ClearAccum) (GET_by_offset(disp, _gloffset_ClearAccum));
}

static inline void SET_ClearAccum(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_ClearAccum, fn);
}

typedef void (GLAPIENTRYP _glptr_ClearIndex)(GLfloat);
#define CALL_ClearIndex(disp, parameters) \
    (* GET_ClearIndex(disp)) parameters
static inline _glptr_ClearIndex GET_ClearIndex(struct _glapi_table *disp) {
   return (_glptr_ClearIndex) (GET_by_offset(disp, _gloffset_ClearIndex));
}

static inline void SET_ClearIndex(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat)) {
   SET_by_offset(disp, _gloffset_ClearIndex, fn);
}

typedef void (GLAPIENTRYP _glptr_ClearColor)(GLclampf, GLclampf, GLclampf, GLclampf);
#define CALL_ClearColor(disp, parameters) \
    (* GET_ClearColor(disp)) parameters
static inline _glptr_ClearColor GET_ClearColor(struct _glapi_table *disp) {
   return (_glptr_ClearColor) (GET_by_offset(disp, _gloffset_ClearColor));
}

static inline void SET_ClearColor(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLclampf, GLclampf, GLclampf, GLclampf)) {
   SET_by_offset(disp, _gloffset_ClearColor, fn);
}

typedef void (GLAPIENTRYP _glptr_ClearStencil)(GLint);
#define CALL_ClearStencil(disp, parameters) \
    (* GET_ClearStencil(disp)) parameters
static inline _glptr_ClearStencil GET_ClearStencil(struct _glapi_table *disp) {
   return (_glptr_ClearStencil) (GET_by_offset(disp, _gloffset_ClearStencil));
}

static inline void SET_ClearStencil(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint)) {
   SET_by_offset(disp, _gloffset_ClearStencil, fn);
}

typedef void (GLAPIENTRYP _glptr_ClearDepth)(GLclampd);
#define CALL_ClearDepth(disp, parameters) \
    (* GET_ClearDepth(disp)) parameters
static inline _glptr_ClearDepth GET_ClearDepth(struct _glapi_table *disp) {
   return (_glptr_ClearDepth) (GET_by_offset(disp, _gloffset_ClearDepth));
}

static inline void SET_ClearDepth(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLclampd)) {
   SET_by_offset(disp, _gloffset_ClearDepth, fn);
}

typedef void (GLAPIENTRYP _glptr_StencilMask)(GLuint);
#define CALL_StencilMask(disp, parameters) \
    (* GET_StencilMask(disp)) parameters
static inline _glptr_StencilMask GET_StencilMask(struct _glapi_table *disp) {
   return (_glptr_StencilMask) (GET_by_offset(disp, _gloffset_StencilMask));
}

static inline void SET_StencilMask(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_StencilMask, fn);
}

typedef void (GLAPIENTRYP _glptr_ColorMask)(GLboolean, GLboolean, GLboolean, GLboolean);
#define CALL_ColorMask(disp, parameters) \
    (* GET_ColorMask(disp)) parameters
static inline _glptr_ColorMask GET_ColorMask(struct _glapi_table *disp) {
   return (_glptr_ColorMask) (GET_by_offset(disp, _gloffset_ColorMask));
}

static inline void SET_ColorMask(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLboolean, GLboolean, GLboolean, GLboolean)) {
   SET_by_offset(disp, _gloffset_ColorMask, fn);
}

typedef void (GLAPIENTRYP _glptr_DepthMask)(GLboolean);
#define CALL_DepthMask(disp, parameters) \
    (* GET_DepthMask(disp)) parameters
static inline _glptr_DepthMask GET_DepthMask(struct _glapi_table *disp) {
   return (_glptr_DepthMask) (GET_by_offset(disp, _gloffset_DepthMask));
}

static inline void SET_DepthMask(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLboolean)) {
   SET_by_offset(disp, _gloffset_DepthMask, fn);
}

typedef void (GLAPIENTRYP _glptr_IndexMask)(GLuint);
#define CALL_IndexMask(disp, parameters) \
    (* GET_IndexMask(disp)) parameters
static inline _glptr_IndexMask GET_IndexMask(struct _glapi_table *disp) {
   return (_glptr_IndexMask) (GET_by_offset(disp, _gloffset_IndexMask));
}

static inline void SET_IndexMask(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_IndexMask, fn);
}

typedef void (GLAPIENTRYP _glptr_Accum)(GLenum, GLfloat);
#define CALL_Accum(disp, parameters) \
    (* GET_Accum(disp)) parameters
static inline _glptr_Accum GET_Accum(struct _glapi_table *disp) {
   return (_glptr_Accum) (GET_by_offset(disp, _gloffset_Accum));
}

static inline void SET_Accum(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_Accum, fn);
}

typedef void (GLAPIENTRYP _glptr_Disable)(GLenum);
#define CALL_Disable(disp, parameters) \
    (* GET_Disable(disp)) parameters
static inline _glptr_Disable GET_Disable(struct _glapi_table *disp) {
   return (_glptr_Disable) (GET_by_offset(disp, _gloffset_Disable));
}

static inline void SET_Disable(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_Disable, fn);
}

typedef void (GLAPIENTRYP _glptr_Enable)(GLenum);
#define CALL_Enable(disp, parameters) \
    (* GET_Enable(disp)) parameters
static inline _glptr_Enable GET_Enable(struct _glapi_table *disp) {
   return (_glptr_Enable) (GET_by_offset(disp, _gloffset_Enable));
}

static inline void SET_Enable(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_Enable, fn);
}

typedef void (GLAPIENTRYP _glptr_Finish)(void);
#define CALL_Finish(disp, parameters) \
    (* GET_Finish(disp)) parameters
static inline _glptr_Finish GET_Finish(struct _glapi_table *disp) {
   return (_glptr_Finish) (GET_by_offset(disp, _gloffset_Finish));
}

static inline void SET_Finish(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_Finish, fn);
}

typedef void (GLAPIENTRYP _glptr_Flush)(void);
#define CALL_Flush(disp, parameters) \
    (* GET_Flush(disp)) parameters
static inline _glptr_Flush GET_Flush(struct _glapi_table *disp) {
   return (_glptr_Flush) (GET_by_offset(disp, _gloffset_Flush));
}

static inline void SET_Flush(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_Flush, fn);
}

typedef void (GLAPIENTRYP _glptr_PopAttrib)(void);
#define CALL_PopAttrib(disp, parameters) \
    (* GET_PopAttrib(disp)) parameters
static inline _glptr_PopAttrib GET_PopAttrib(struct _glapi_table *disp) {
   return (_glptr_PopAttrib) (GET_by_offset(disp, _gloffset_PopAttrib));
}

static inline void SET_PopAttrib(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_PopAttrib, fn);
}

typedef void (GLAPIENTRYP _glptr_PushAttrib)(GLbitfield);
#define CALL_PushAttrib(disp, parameters) \
    (* GET_PushAttrib(disp)) parameters
static inline _glptr_PushAttrib GET_PushAttrib(struct _glapi_table *disp) {
   return (_glptr_PushAttrib) (GET_by_offset(disp, _gloffset_PushAttrib));
}

static inline void SET_PushAttrib(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLbitfield)) {
   SET_by_offset(disp, _gloffset_PushAttrib, fn);
}

typedef void (GLAPIENTRYP _glptr_Map1d)(GLenum, GLdouble, GLdouble, GLint, GLint, const GLdouble *);
#define CALL_Map1d(disp, parameters) \
    (* GET_Map1d(disp)) parameters
static inline _glptr_Map1d GET_Map1d(struct _glapi_table *disp) {
   return (_glptr_Map1d) (GET_by_offset(disp, _gloffset_Map1d));
}

static inline void SET_Map1d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLdouble, GLdouble, GLint, GLint, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_Map1d, fn);
}

typedef void (GLAPIENTRYP _glptr_Map1f)(GLenum, GLfloat, GLfloat, GLint, GLint, const GLfloat *);
#define CALL_Map1f(disp, parameters) \
    (* GET_Map1f(disp)) parameters
static inline _glptr_Map1f GET_Map1f(struct _glapi_table *disp) {
   return (_glptr_Map1f) (GET_by_offset(disp, _gloffset_Map1f));
}

static inline void SET_Map1f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat, GLfloat, GLint, GLint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Map1f, fn);
}

typedef void (GLAPIENTRYP _glptr_Map2d)(GLenum, GLdouble, GLdouble, GLint, GLint, GLdouble, GLdouble, GLint, GLint, const GLdouble *);
#define CALL_Map2d(disp, parameters) \
    (* GET_Map2d(disp)) parameters
static inline _glptr_Map2d GET_Map2d(struct _glapi_table *disp) {
   return (_glptr_Map2d) (GET_by_offset(disp, _gloffset_Map2d));
}

static inline void SET_Map2d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLdouble, GLdouble, GLint, GLint, GLdouble, GLdouble, GLint, GLint, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_Map2d, fn);
}

typedef void (GLAPIENTRYP _glptr_Map2f)(GLenum, GLfloat, GLfloat, GLint, GLint, GLfloat, GLfloat, GLint, GLint, const GLfloat *);
#define CALL_Map2f(disp, parameters) \
    (* GET_Map2f(disp)) parameters
static inline _glptr_Map2f GET_Map2f(struct _glapi_table *disp) {
   return (_glptr_Map2f) (GET_by_offset(disp, _gloffset_Map2f));
}

static inline void SET_Map2f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat, GLfloat, GLint, GLint, GLfloat, GLfloat, GLint, GLint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Map2f, fn);
}

typedef void (GLAPIENTRYP _glptr_MapGrid1d)(GLint, GLdouble, GLdouble);
#define CALL_MapGrid1d(disp, parameters) \
    (* GET_MapGrid1d(disp)) parameters
static inline _glptr_MapGrid1d GET_MapGrid1d(struct _glapi_table *disp) {
   return (_glptr_MapGrid1d) (GET_by_offset(disp, _gloffset_MapGrid1d));
}

static inline void SET_MapGrid1d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_MapGrid1d, fn);
}

typedef void (GLAPIENTRYP _glptr_MapGrid1f)(GLint, GLfloat, GLfloat);
#define CALL_MapGrid1f(disp, parameters) \
    (* GET_MapGrid1f(disp)) parameters
static inline _glptr_MapGrid1f GET_MapGrid1f(struct _glapi_table *disp) {
   return (_glptr_MapGrid1f) (GET_by_offset(disp, _gloffset_MapGrid1f));
}

static inline void SET_MapGrid1f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_MapGrid1f, fn);
}

typedef void (GLAPIENTRYP _glptr_MapGrid2d)(GLint, GLdouble, GLdouble, GLint, GLdouble, GLdouble);
#define CALL_MapGrid2d(disp, parameters) \
    (* GET_MapGrid2d(disp)) parameters
static inline _glptr_MapGrid2d GET_MapGrid2d(struct _glapi_table *disp) {
   return (_glptr_MapGrid2d) (GET_by_offset(disp, _gloffset_MapGrid2d));
}

static inline void SET_MapGrid2d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLdouble, GLdouble, GLint, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_MapGrid2d, fn);
}

typedef void (GLAPIENTRYP _glptr_MapGrid2f)(GLint, GLfloat, GLfloat, GLint, GLfloat, GLfloat);
#define CALL_MapGrid2f(disp, parameters) \
    (* GET_MapGrid2f(disp)) parameters
static inline _glptr_MapGrid2f GET_MapGrid2f(struct _glapi_table *disp) {
   return (_glptr_MapGrid2f) (GET_by_offset(disp, _gloffset_MapGrid2f));
}

static inline void SET_MapGrid2f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLfloat, GLfloat, GLint, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_MapGrid2f, fn);
}

typedef void (GLAPIENTRYP _glptr_EvalCoord1d)(GLdouble);
#define CALL_EvalCoord1d(disp, parameters) \
    (* GET_EvalCoord1d(disp)) parameters
static inline _glptr_EvalCoord1d GET_EvalCoord1d(struct _glapi_table *disp) {
   return (_glptr_EvalCoord1d) (GET_by_offset(disp, _gloffset_EvalCoord1d));
}

static inline void SET_EvalCoord1d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble)) {
   SET_by_offset(disp, _gloffset_EvalCoord1d, fn);
}

typedef void (GLAPIENTRYP _glptr_EvalCoord1dv)(const GLdouble *);
#define CALL_EvalCoord1dv(disp, parameters) \
    (* GET_EvalCoord1dv(disp)) parameters
static inline _glptr_EvalCoord1dv GET_EvalCoord1dv(struct _glapi_table *disp) {
   return (_glptr_EvalCoord1dv) (GET_by_offset(disp, _gloffset_EvalCoord1dv));
}

static inline void SET_EvalCoord1dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_EvalCoord1dv, fn);
}

typedef void (GLAPIENTRYP _glptr_EvalCoord1f)(GLfloat);
#define CALL_EvalCoord1f(disp, parameters) \
    (* GET_EvalCoord1f(disp)) parameters
static inline _glptr_EvalCoord1f GET_EvalCoord1f(struct _glapi_table *disp) {
   return (_glptr_EvalCoord1f) (GET_by_offset(disp, _gloffset_EvalCoord1f));
}

static inline void SET_EvalCoord1f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat)) {
   SET_by_offset(disp, _gloffset_EvalCoord1f, fn);
}

typedef void (GLAPIENTRYP _glptr_EvalCoord1fv)(const GLfloat *);
#define CALL_EvalCoord1fv(disp, parameters) \
    (* GET_EvalCoord1fv(disp)) parameters
static inline _glptr_EvalCoord1fv GET_EvalCoord1fv(struct _glapi_table *disp) {
   return (_glptr_EvalCoord1fv) (GET_by_offset(disp, _gloffset_EvalCoord1fv));
}

static inline void SET_EvalCoord1fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_EvalCoord1fv, fn);
}

typedef void (GLAPIENTRYP _glptr_EvalCoord2d)(GLdouble, GLdouble);
#define CALL_EvalCoord2d(disp, parameters) \
    (* GET_EvalCoord2d(disp)) parameters
static inline _glptr_EvalCoord2d GET_EvalCoord2d(struct _glapi_table *disp) {
   return (_glptr_EvalCoord2d) (GET_by_offset(disp, _gloffset_EvalCoord2d));
}

static inline void SET_EvalCoord2d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_EvalCoord2d, fn);
}

typedef void (GLAPIENTRYP _glptr_EvalCoord2dv)(const GLdouble *);
#define CALL_EvalCoord2dv(disp, parameters) \
    (* GET_EvalCoord2dv(disp)) parameters
static inline _glptr_EvalCoord2dv GET_EvalCoord2dv(struct _glapi_table *disp) {
   return (_glptr_EvalCoord2dv) (GET_by_offset(disp, _gloffset_EvalCoord2dv));
}

static inline void SET_EvalCoord2dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_EvalCoord2dv, fn);
}

typedef void (GLAPIENTRYP _glptr_EvalCoord2f)(GLfloat, GLfloat);
#define CALL_EvalCoord2f(disp, parameters) \
    (* GET_EvalCoord2f(disp)) parameters
static inline _glptr_EvalCoord2f GET_EvalCoord2f(struct _glapi_table *disp) {
   return (_glptr_EvalCoord2f) (GET_by_offset(disp, _gloffset_EvalCoord2f));
}

static inline void SET_EvalCoord2f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_EvalCoord2f, fn);
}

typedef void (GLAPIENTRYP _glptr_EvalCoord2fv)(const GLfloat *);
#define CALL_EvalCoord2fv(disp, parameters) \
    (* GET_EvalCoord2fv(disp)) parameters
static inline _glptr_EvalCoord2fv GET_EvalCoord2fv(struct _glapi_table *disp) {
   return (_glptr_EvalCoord2fv) (GET_by_offset(disp, _gloffset_EvalCoord2fv));
}

static inline void SET_EvalCoord2fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_EvalCoord2fv, fn);
}

typedef void (GLAPIENTRYP _glptr_EvalMesh1)(GLenum, GLint, GLint);
#define CALL_EvalMesh1(disp, parameters) \
    (* GET_EvalMesh1(disp)) parameters
static inline _glptr_EvalMesh1 GET_EvalMesh1(struct _glapi_table *disp) {
   return (_glptr_EvalMesh1) (GET_by_offset(disp, _gloffset_EvalMesh1));
}

static inline void SET_EvalMesh1(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_EvalMesh1, fn);
}

typedef void (GLAPIENTRYP _glptr_EvalPoint1)(GLint);
#define CALL_EvalPoint1(disp, parameters) \
    (* GET_EvalPoint1(disp)) parameters
static inline _glptr_EvalPoint1 GET_EvalPoint1(struct _glapi_table *disp) {
   return (_glptr_EvalPoint1) (GET_by_offset(disp, _gloffset_EvalPoint1));
}

static inline void SET_EvalPoint1(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint)) {
   SET_by_offset(disp, _gloffset_EvalPoint1, fn);
}

typedef void (GLAPIENTRYP _glptr_EvalMesh2)(GLenum, GLint, GLint, GLint, GLint);
#define CALL_EvalMesh2(disp, parameters) \
    (* GET_EvalMesh2(disp)) parameters
static inline _glptr_EvalMesh2 GET_EvalMesh2(struct _glapi_table *disp) {
   return (_glptr_EvalMesh2) (GET_by_offset(disp, _gloffset_EvalMesh2));
}

static inline void SET_EvalMesh2(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_EvalMesh2, fn);
}

typedef void (GLAPIENTRYP _glptr_EvalPoint2)(GLint, GLint);
#define CALL_EvalPoint2(disp, parameters) \
    (* GET_EvalPoint2(disp)) parameters
static inline _glptr_EvalPoint2 GET_EvalPoint2(struct _glapi_table *disp) {
   return (_glptr_EvalPoint2) (GET_by_offset(disp, _gloffset_EvalPoint2));
}

static inline void SET_EvalPoint2(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint)) {
   SET_by_offset(disp, _gloffset_EvalPoint2, fn);
}

typedef void (GLAPIENTRYP _glptr_AlphaFunc)(GLenum, GLclampf);
#define CALL_AlphaFunc(disp, parameters) \
    (* GET_AlphaFunc(disp)) parameters
static inline _glptr_AlphaFunc GET_AlphaFunc(struct _glapi_table *disp) {
   return (_glptr_AlphaFunc) (GET_by_offset(disp, _gloffset_AlphaFunc));
}

static inline void SET_AlphaFunc(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLclampf)) {
   SET_by_offset(disp, _gloffset_AlphaFunc, fn);
}

typedef void (GLAPIENTRYP _glptr_BlendFunc)(GLenum, GLenum);
#define CALL_BlendFunc(disp, parameters) \
    (* GET_BlendFunc(disp)) parameters
static inline _glptr_BlendFunc GET_BlendFunc(struct _glapi_table *disp) {
   return (_glptr_BlendFunc) (GET_by_offset(disp, _gloffset_BlendFunc));
}

static inline void SET_BlendFunc(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_BlendFunc, fn);
}

typedef void (GLAPIENTRYP _glptr_LogicOp)(GLenum);
#define CALL_LogicOp(disp, parameters) \
    (* GET_LogicOp(disp)) parameters
static inline _glptr_LogicOp GET_LogicOp(struct _glapi_table *disp) {
   return (_glptr_LogicOp) (GET_by_offset(disp, _gloffset_LogicOp));
}

static inline void SET_LogicOp(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_LogicOp, fn);
}

typedef void (GLAPIENTRYP _glptr_StencilFunc)(GLenum, GLint, GLuint);
#define CALL_StencilFunc(disp, parameters) \
    (* GET_StencilFunc(disp)) parameters
static inline _glptr_StencilFunc GET_StencilFunc(struct _glapi_table *disp) {
   return (_glptr_StencilFunc) (GET_by_offset(disp, _gloffset_StencilFunc));
}

static inline void SET_StencilFunc(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLuint)) {
   SET_by_offset(disp, _gloffset_StencilFunc, fn);
}

typedef void (GLAPIENTRYP _glptr_StencilOp)(GLenum, GLenum, GLenum);
#define CALL_StencilOp(disp, parameters) \
    (* GET_StencilOp(disp)) parameters
static inline _glptr_StencilOp GET_StencilOp(struct _glapi_table *disp) {
   return (_glptr_StencilOp) (GET_by_offset(disp, _gloffset_StencilOp));
}

static inline void SET_StencilOp(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_StencilOp, fn);
}

typedef void (GLAPIENTRYP _glptr_DepthFunc)(GLenum);
#define CALL_DepthFunc(disp, parameters) \
    (* GET_DepthFunc(disp)) parameters
static inline _glptr_DepthFunc GET_DepthFunc(struct _glapi_table *disp) {
   return (_glptr_DepthFunc) (GET_by_offset(disp, _gloffset_DepthFunc));
}

static inline void SET_DepthFunc(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_DepthFunc, fn);
}

typedef void (GLAPIENTRYP _glptr_PixelZoom)(GLfloat, GLfloat);
#define CALL_PixelZoom(disp, parameters) \
    (* GET_PixelZoom(disp)) parameters
static inline _glptr_PixelZoom GET_PixelZoom(struct _glapi_table *disp) {
   return (_glptr_PixelZoom) (GET_by_offset(disp, _gloffset_PixelZoom));
}

static inline void SET_PixelZoom(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_PixelZoom, fn);
}

typedef void (GLAPIENTRYP _glptr_PixelTransferf)(GLenum, GLfloat);
#define CALL_PixelTransferf(disp, parameters) \
    (* GET_PixelTransferf(disp)) parameters
static inline _glptr_PixelTransferf GET_PixelTransferf(struct _glapi_table *disp) {
   return (_glptr_PixelTransferf) (GET_by_offset(disp, _gloffset_PixelTransferf));
}

static inline void SET_PixelTransferf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_PixelTransferf, fn);
}

typedef void (GLAPIENTRYP _glptr_PixelTransferi)(GLenum, GLint);
#define CALL_PixelTransferi(disp, parameters) \
    (* GET_PixelTransferi(disp)) parameters
static inline _glptr_PixelTransferi GET_PixelTransferi(struct _glapi_table *disp) {
   return (_glptr_PixelTransferi) (GET_by_offset(disp, _gloffset_PixelTransferi));
}

static inline void SET_PixelTransferi(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_PixelTransferi, fn);
}

typedef void (GLAPIENTRYP _glptr_PixelStoref)(GLenum, GLfloat);
#define CALL_PixelStoref(disp, parameters) \
    (* GET_PixelStoref(disp)) parameters
static inline _glptr_PixelStoref GET_PixelStoref(struct _glapi_table *disp) {
   return (_glptr_PixelStoref) (GET_by_offset(disp, _gloffset_PixelStoref));
}

static inline void SET_PixelStoref(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_PixelStoref, fn);
}

typedef void (GLAPIENTRYP _glptr_PixelStorei)(GLenum, GLint);
#define CALL_PixelStorei(disp, parameters) \
    (* GET_PixelStorei(disp)) parameters
static inline _glptr_PixelStorei GET_PixelStorei(struct _glapi_table *disp) {
   return (_glptr_PixelStorei) (GET_by_offset(disp, _gloffset_PixelStorei));
}

static inline void SET_PixelStorei(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_PixelStorei, fn);
}

typedef void (GLAPIENTRYP _glptr_PixelMapfv)(GLenum, GLsizei, const GLfloat *);
#define CALL_PixelMapfv(disp, parameters) \
    (* GET_PixelMapfv(disp)) parameters
static inline _glptr_PixelMapfv GET_PixelMapfv(struct _glapi_table *disp) {
   return (_glptr_PixelMapfv) (GET_by_offset(disp, _gloffset_PixelMapfv));
}

static inline void SET_PixelMapfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_PixelMapfv, fn);
}

typedef void (GLAPIENTRYP _glptr_PixelMapuiv)(GLenum, GLsizei, const GLuint *);
#define CALL_PixelMapuiv(disp, parameters) \
    (* GET_PixelMapuiv(disp)) parameters
static inline _glptr_PixelMapuiv GET_PixelMapuiv(struct _glapi_table *disp) {
   return (_glptr_PixelMapuiv) (GET_by_offset(disp, _gloffset_PixelMapuiv));
}

static inline void SET_PixelMapuiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_PixelMapuiv, fn);
}

typedef void (GLAPIENTRYP _glptr_PixelMapusv)(GLenum, GLsizei, const GLushort *);
#define CALL_PixelMapusv(disp, parameters) \
    (* GET_PixelMapusv(disp)) parameters
static inline _glptr_PixelMapusv GET_PixelMapusv(struct _glapi_table *disp) {
   return (_glptr_PixelMapusv) (GET_by_offset(disp, _gloffset_PixelMapusv));
}

static inline void SET_PixelMapusv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, const GLushort *)) {
   SET_by_offset(disp, _gloffset_PixelMapusv, fn);
}

typedef void (GLAPIENTRYP _glptr_ReadBuffer)(GLenum);
#define CALL_ReadBuffer(disp, parameters) \
    (* GET_ReadBuffer(disp)) parameters
static inline _glptr_ReadBuffer GET_ReadBuffer(struct _glapi_table *disp) {
   return (_glptr_ReadBuffer) (GET_by_offset(disp, _gloffset_ReadBuffer));
}

static inline void SET_ReadBuffer(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_ReadBuffer, fn);
}

typedef void (GLAPIENTRYP _glptr_CopyPixels)(GLint, GLint, GLsizei, GLsizei, GLenum);
#define CALL_CopyPixels(disp, parameters) \
    (* GET_CopyPixels(disp)) parameters
static inline _glptr_CopyPixels GET_CopyPixels(struct _glapi_table *disp) {
   return (_glptr_CopyPixels) (GET_by_offset(disp, _gloffset_CopyPixels));
}

static inline void SET_CopyPixels(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLsizei, GLsizei, GLenum)) {
   SET_by_offset(disp, _gloffset_CopyPixels, fn);
}

typedef void (GLAPIENTRYP _glptr_ReadPixels)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid *);
#define CALL_ReadPixels(disp, parameters) \
    (* GET_ReadPixels(disp)) parameters
static inline _glptr_ReadPixels GET_ReadPixels(struct _glapi_table *disp) {
   return (_glptr_ReadPixels) (GET_by_offset(disp, _gloffset_ReadPixels));
}

static inline void SET_ReadPixels(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid *)) {
   SET_by_offset(disp, _gloffset_ReadPixels, fn);
}

typedef void (GLAPIENTRYP _glptr_DrawPixels)(GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
#define CALL_DrawPixels(disp, parameters) \
    (* GET_DrawPixels(disp)) parameters
static inline _glptr_DrawPixels GET_DrawPixels(struct _glapi_table *disp) {
   return (_glptr_DrawPixels) (GET_by_offset(disp, _gloffset_DrawPixels));
}

static inline void SET_DrawPixels(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLsizei, GLenum, GLenum, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_DrawPixels, fn);
}

typedef void (GLAPIENTRYP _glptr_GetBooleanv)(GLenum, GLboolean *);
#define CALL_GetBooleanv(disp, parameters) \
    (* GET_GetBooleanv(disp)) parameters
static inline _glptr_GetBooleanv GET_GetBooleanv(struct _glapi_table *disp) {
   return (_glptr_GetBooleanv) (GET_by_offset(disp, _gloffset_GetBooleanv));
}

static inline void SET_GetBooleanv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLboolean *)) {
   SET_by_offset(disp, _gloffset_GetBooleanv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetClipPlane)(GLenum, GLdouble *);
#define CALL_GetClipPlane(disp, parameters) \
    (* GET_GetClipPlane(disp)) parameters
static inline _glptr_GetClipPlane GET_GetClipPlane(struct _glapi_table *disp) {
   return (_glptr_GetClipPlane) (GET_by_offset(disp, _gloffset_GetClipPlane));
}

static inline void SET_GetClipPlane(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLdouble *)) {
   SET_by_offset(disp, _gloffset_GetClipPlane, fn);
}

typedef void (GLAPIENTRYP _glptr_GetDoublev)(GLenum, GLdouble *);
#define CALL_GetDoublev(disp, parameters) \
    (* GET_GetDoublev(disp)) parameters
static inline _glptr_GetDoublev GET_GetDoublev(struct _glapi_table *disp) {
   return (_glptr_GetDoublev) (GET_by_offset(disp, _gloffset_GetDoublev));
}

static inline void SET_GetDoublev(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLdouble *)) {
   SET_by_offset(disp, _gloffset_GetDoublev, fn);
}

typedef GLenum (GLAPIENTRYP _glptr_GetError)(void);
#define CALL_GetError(disp, parameters) \
    (* GET_GetError(disp)) parameters
static inline _glptr_GetError GET_GetError(struct _glapi_table *disp) {
   return (_glptr_GetError) (GET_by_offset(disp, _gloffset_GetError));
}

static inline void SET_GetError(struct _glapi_table *disp, GLenum (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_GetError, fn);
}

typedef void (GLAPIENTRYP _glptr_GetFloatv)(GLenum, GLfloat *);
#define CALL_GetFloatv(disp, parameters) \
    (* GET_GetFloatv(disp)) parameters
static inline _glptr_GetFloatv GET_GetFloatv(struct _glapi_table *disp) {
   return (_glptr_GetFloatv) (GET_by_offset(disp, _gloffset_GetFloatv));
}

static inline void SET_GetFloatv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetFloatv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetIntegerv)(GLenum, GLint *);
#define CALL_GetIntegerv(disp, parameters) \
    (* GET_GetIntegerv(disp)) parameters
static inline _glptr_GetIntegerv GET_GetIntegerv(struct _glapi_table *disp) {
   return (_glptr_GetIntegerv) (GET_by_offset(disp, _gloffset_GetIntegerv));
}

static inline void SET_GetIntegerv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetIntegerv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetLightfv)(GLenum, GLenum, GLfloat *);
#define CALL_GetLightfv(disp, parameters) \
    (* GET_GetLightfv(disp)) parameters
static inline _glptr_GetLightfv GET_GetLightfv(struct _glapi_table *disp) {
   return (_glptr_GetLightfv) (GET_by_offset(disp, _gloffset_GetLightfv));
}

static inline void SET_GetLightfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetLightfv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetLightiv)(GLenum, GLenum, GLint *);
#define CALL_GetLightiv(disp, parameters) \
    (* GET_GetLightiv(disp)) parameters
static inline _glptr_GetLightiv GET_GetLightiv(struct _glapi_table *disp) {
   return (_glptr_GetLightiv) (GET_by_offset(disp, _gloffset_GetLightiv));
}

static inline void SET_GetLightiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetLightiv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetMapdv)(GLenum, GLenum, GLdouble *);
#define CALL_GetMapdv(disp, parameters) \
    (* GET_GetMapdv(disp)) parameters
static inline _glptr_GetMapdv GET_GetMapdv(struct _glapi_table *disp) {
   return (_glptr_GetMapdv) (GET_by_offset(disp, _gloffset_GetMapdv));
}

static inline void SET_GetMapdv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLdouble *)) {
   SET_by_offset(disp, _gloffset_GetMapdv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetMapfv)(GLenum, GLenum, GLfloat *);
#define CALL_GetMapfv(disp, parameters) \
    (* GET_GetMapfv(disp)) parameters
static inline _glptr_GetMapfv GET_GetMapfv(struct _glapi_table *disp) {
   return (_glptr_GetMapfv) (GET_by_offset(disp, _gloffset_GetMapfv));
}

static inline void SET_GetMapfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetMapfv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetMapiv)(GLenum, GLenum, GLint *);
#define CALL_GetMapiv(disp, parameters) \
    (* GET_GetMapiv(disp)) parameters
static inline _glptr_GetMapiv GET_GetMapiv(struct _glapi_table *disp) {
   return (_glptr_GetMapiv) (GET_by_offset(disp, _gloffset_GetMapiv));
}

static inline void SET_GetMapiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetMapiv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetMaterialfv)(GLenum, GLenum, GLfloat *);
#define CALL_GetMaterialfv(disp, parameters) \
    (* GET_GetMaterialfv(disp)) parameters
static inline _glptr_GetMaterialfv GET_GetMaterialfv(struct _glapi_table *disp) {
   return (_glptr_GetMaterialfv) (GET_by_offset(disp, _gloffset_GetMaterialfv));
}

static inline void SET_GetMaterialfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetMaterialfv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetMaterialiv)(GLenum, GLenum, GLint *);
#define CALL_GetMaterialiv(disp, parameters) \
    (* GET_GetMaterialiv(disp)) parameters
static inline _glptr_GetMaterialiv GET_GetMaterialiv(struct _glapi_table *disp) {
   return (_glptr_GetMaterialiv) (GET_by_offset(disp, _gloffset_GetMaterialiv));
}

static inline void SET_GetMaterialiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetMaterialiv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetPixelMapfv)(GLenum, GLfloat *);
#define CALL_GetPixelMapfv(disp, parameters) \
    (* GET_GetPixelMapfv(disp)) parameters
static inline _glptr_GetPixelMapfv GET_GetPixelMapfv(struct _glapi_table *disp) {
   return (_glptr_GetPixelMapfv) (GET_by_offset(disp, _gloffset_GetPixelMapfv));
}

static inline void SET_GetPixelMapfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetPixelMapfv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetPixelMapuiv)(GLenum, GLuint *);
#define CALL_GetPixelMapuiv(disp, parameters) \
    (* GET_GetPixelMapuiv(disp)) parameters
static inline _glptr_GetPixelMapuiv GET_GetPixelMapuiv(struct _glapi_table *disp) {
   return (_glptr_GetPixelMapuiv) (GET_by_offset(disp, _gloffset_GetPixelMapuiv));
}

static inline void SET_GetPixelMapuiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint *)) {
   SET_by_offset(disp, _gloffset_GetPixelMapuiv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetPixelMapusv)(GLenum, GLushort *);
#define CALL_GetPixelMapusv(disp, parameters) \
    (* GET_GetPixelMapusv(disp)) parameters
static inline _glptr_GetPixelMapusv GET_GetPixelMapusv(struct _glapi_table *disp) {
   return (_glptr_GetPixelMapusv) (GET_by_offset(disp, _gloffset_GetPixelMapusv));
}

static inline void SET_GetPixelMapusv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLushort *)) {
   SET_by_offset(disp, _gloffset_GetPixelMapusv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetPolygonStipple)(GLubyte *);
#define CALL_GetPolygonStipple(disp, parameters) \
    (* GET_GetPolygonStipple(disp)) parameters
static inline _glptr_GetPolygonStipple GET_GetPolygonStipple(struct _glapi_table *disp) {
   return (_glptr_GetPolygonStipple) (GET_by_offset(disp, _gloffset_GetPolygonStipple));
}

static inline void SET_GetPolygonStipple(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLubyte *)) {
   SET_by_offset(disp, _gloffset_GetPolygonStipple, fn);
}

typedef const GLubyte * (GLAPIENTRYP _glptr_GetString)(GLenum);
#define CALL_GetString(disp, parameters) \
    (* GET_GetString(disp)) parameters
static inline _glptr_GetString GET_GetString(struct _glapi_table *disp) {
   return (_glptr_GetString) (GET_by_offset(disp, _gloffset_GetString));
}

static inline void SET_GetString(struct _glapi_table *disp, const GLubyte * (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_GetString, fn);
}

typedef void (GLAPIENTRYP _glptr_GetTexEnvfv)(GLenum, GLenum, GLfloat *);
#define CALL_GetTexEnvfv(disp, parameters) \
    (* GET_GetTexEnvfv(disp)) parameters
static inline _glptr_GetTexEnvfv GET_GetTexEnvfv(struct _glapi_table *disp) {
   return (_glptr_GetTexEnvfv) (GET_by_offset(disp, _gloffset_GetTexEnvfv));
}

static inline void SET_GetTexEnvfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetTexEnvfv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetTexEnviv)(GLenum, GLenum, GLint *);
#define CALL_GetTexEnviv(disp, parameters) \
    (* GET_GetTexEnviv(disp)) parameters
static inline _glptr_GetTexEnviv GET_GetTexEnviv(struct _glapi_table *disp) {
   return (_glptr_GetTexEnviv) (GET_by_offset(disp, _gloffset_GetTexEnviv));
}

static inline void SET_GetTexEnviv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetTexEnviv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetTexGendv)(GLenum, GLenum, GLdouble *);
#define CALL_GetTexGendv(disp, parameters) \
    (* GET_GetTexGendv(disp)) parameters
static inline _glptr_GetTexGendv GET_GetTexGendv(struct _glapi_table *disp) {
   return (_glptr_GetTexGendv) (GET_by_offset(disp, _gloffset_GetTexGendv));
}

static inline void SET_GetTexGendv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLdouble *)) {
   SET_by_offset(disp, _gloffset_GetTexGendv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetTexGenfv)(GLenum, GLenum, GLfloat *);
#define CALL_GetTexGenfv(disp, parameters) \
    (* GET_GetTexGenfv(disp)) parameters
static inline _glptr_GetTexGenfv GET_GetTexGenfv(struct _glapi_table *disp) {
   return (_glptr_GetTexGenfv) (GET_by_offset(disp, _gloffset_GetTexGenfv));
}

static inline void SET_GetTexGenfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetTexGenfv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetTexGeniv)(GLenum, GLenum, GLint *);
#define CALL_GetTexGeniv(disp, parameters) \
    (* GET_GetTexGeniv(disp)) parameters
static inline _glptr_GetTexGeniv GET_GetTexGeniv(struct _glapi_table *disp) {
   return (_glptr_GetTexGeniv) (GET_by_offset(disp, _gloffset_GetTexGeniv));
}

static inline void SET_GetTexGeniv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetTexGeniv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetTexImage)(GLenum, GLint, GLenum, GLenum, GLvoid *);
#define CALL_GetTexImage(disp, parameters) \
    (* GET_GetTexImage(disp)) parameters
static inline _glptr_GetTexImage GET_GetTexImage(struct _glapi_table *disp) {
   return (_glptr_GetTexImage) (GET_by_offset(disp, _gloffset_GetTexImage));
}

static inline void SET_GetTexImage(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLenum, GLenum, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetTexImage, fn);
}

typedef void (GLAPIENTRYP _glptr_GetTexParameterfv)(GLenum, GLenum, GLfloat *);
#define CALL_GetTexParameterfv(disp, parameters) \
    (* GET_GetTexParameterfv(disp)) parameters
static inline _glptr_GetTexParameterfv GET_GetTexParameterfv(struct _glapi_table *disp) {
   return (_glptr_GetTexParameterfv) (GET_by_offset(disp, _gloffset_GetTexParameterfv));
}

static inline void SET_GetTexParameterfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetTexParameterfv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetTexParameteriv)(GLenum, GLenum, GLint *);
#define CALL_GetTexParameteriv(disp, parameters) \
    (* GET_GetTexParameteriv(disp)) parameters
static inline _glptr_GetTexParameteriv GET_GetTexParameteriv(struct _glapi_table *disp) {
   return (_glptr_GetTexParameteriv) (GET_by_offset(disp, _gloffset_GetTexParameteriv));
}

static inline void SET_GetTexParameteriv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetTexParameteriv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetTexLevelParameterfv)(GLenum, GLint, GLenum, GLfloat *);
#define CALL_GetTexLevelParameterfv(disp, parameters) \
    (* GET_GetTexLevelParameterfv(disp)) parameters
static inline _glptr_GetTexLevelParameterfv GET_GetTexLevelParameterfv(struct _glapi_table *disp) {
   return (_glptr_GetTexLevelParameterfv) (GET_by_offset(disp, _gloffset_GetTexLevelParameterfv));
}

static inline void SET_GetTexLevelParameterfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetTexLevelParameterfv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetTexLevelParameteriv)(GLenum, GLint, GLenum, GLint *);
#define CALL_GetTexLevelParameteriv(disp, parameters) \
    (* GET_GetTexLevelParameteriv(disp)) parameters
static inline _glptr_GetTexLevelParameteriv GET_GetTexLevelParameteriv(struct _glapi_table *disp) {
   return (_glptr_GetTexLevelParameteriv) (GET_by_offset(disp, _gloffset_GetTexLevelParameteriv));
}

static inline void SET_GetTexLevelParameteriv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetTexLevelParameteriv, fn);
}

typedef GLboolean (GLAPIENTRYP _glptr_IsEnabled)(GLenum);
#define CALL_IsEnabled(disp, parameters) \
    (* GET_IsEnabled(disp)) parameters
static inline _glptr_IsEnabled GET_IsEnabled(struct _glapi_table *disp) {
   return (_glptr_IsEnabled) (GET_by_offset(disp, _gloffset_IsEnabled));
}

static inline void SET_IsEnabled(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_IsEnabled, fn);
}

typedef GLboolean (GLAPIENTRYP _glptr_IsList)(GLuint);
#define CALL_IsList(disp, parameters) \
    (* GET_IsList(disp)) parameters
static inline _glptr_IsList GET_IsList(struct _glapi_table *disp) {
   return (_glptr_IsList) (GET_by_offset(disp, _gloffset_IsList));
}

static inline void SET_IsList(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_IsList, fn);
}

typedef void (GLAPIENTRYP _glptr_DepthRange)(GLclampd, GLclampd);
#define CALL_DepthRange(disp, parameters) \
    (* GET_DepthRange(disp)) parameters
static inline _glptr_DepthRange GET_DepthRange(struct _glapi_table *disp) {
   return (_glptr_DepthRange) (GET_by_offset(disp, _gloffset_DepthRange));
}

static inline void SET_DepthRange(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLclampd, GLclampd)) {
   SET_by_offset(disp, _gloffset_DepthRange, fn);
}

typedef void (GLAPIENTRYP _glptr_Frustum)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
#define CALL_Frustum(disp, parameters) \
    (* GET_Frustum(disp)) parameters
static inline _glptr_Frustum GET_Frustum(struct _glapi_table *disp) {
   return (_glptr_Frustum) (GET_by_offset(disp, _gloffset_Frustum));
}

static inline void SET_Frustum(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_Frustum, fn);
}

typedef void (GLAPIENTRYP _glptr_LoadIdentity)(void);
#define CALL_LoadIdentity(disp, parameters) \
    (* GET_LoadIdentity(disp)) parameters
static inline _glptr_LoadIdentity GET_LoadIdentity(struct _glapi_table *disp) {
   return (_glptr_LoadIdentity) (GET_by_offset(disp, _gloffset_LoadIdentity));
}

static inline void SET_LoadIdentity(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_LoadIdentity, fn);
}

typedef void (GLAPIENTRYP _glptr_LoadMatrixf)(const GLfloat *);
#define CALL_LoadMatrixf(disp, parameters) \
    (* GET_LoadMatrixf(disp)) parameters
static inline _glptr_LoadMatrixf GET_LoadMatrixf(struct _glapi_table *disp) {
   return (_glptr_LoadMatrixf) (GET_by_offset(disp, _gloffset_LoadMatrixf));
}

static inline void SET_LoadMatrixf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_LoadMatrixf, fn);
}

typedef void (GLAPIENTRYP _glptr_LoadMatrixd)(const GLdouble *);
#define CALL_LoadMatrixd(disp, parameters) \
    (* GET_LoadMatrixd(disp)) parameters
static inline _glptr_LoadMatrixd GET_LoadMatrixd(struct _glapi_table *disp) {
   return (_glptr_LoadMatrixd) (GET_by_offset(disp, _gloffset_LoadMatrixd));
}

static inline void SET_LoadMatrixd(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_LoadMatrixd, fn);
}

typedef void (GLAPIENTRYP _glptr_MatrixMode)(GLenum);
#define CALL_MatrixMode(disp, parameters) \
    (* GET_MatrixMode(disp)) parameters
static inline _glptr_MatrixMode GET_MatrixMode(struct _glapi_table *disp) {
   return (_glptr_MatrixMode) (GET_by_offset(disp, _gloffset_MatrixMode));
}

static inline void SET_MatrixMode(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_MatrixMode, fn);
}

typedef void (GLAPIENTRYP _glptr_MultMatrixf)(const GLfloat *);
#define CALL_MultMatrixf(disp, parameters) \
    (* GET_MultMatrixf(disp)) parameters
static inline _glptr_MultMatrixf GET_MultMatrixf(struct _glapi_table *disp) {
   return (_glptr_MultMatrixf) (GET_by_offset(disp, _gloffset_MultMatrixf));
}

static inline void SET_MultMatrixf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_MultMatrixf, fn);
}

typedef void (GLAPIENTRYP _glptr_MultMatrixd)(const GLdouble *);
#define CALL_MultMatrixd(disp, parameters) \
    (* GET_MultMatrixd(disp)) parameters
static inline _glptr_MultMatrixd GET_MultMatrixd(struct _glapi_table *disp) {
   return (_glptr_MultMatrixd) (GET_by_offset(disp, _gloffset_MultMatrixd));
}

static inline void SET_MultMatrixd(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_MultMatrixd, fn);
}

typedef void (GLAPIENTRYP _glptr_Ortho)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
#define CALL_Ortho(disp, parameters) \
    (* GET_Ortho(disp)) parameters
static inline _glptr_Ortho GET_Ortho(struct _glapi_table *disp) {
   return (_glptr_Ortho) (GET_by_offset(disp, _gloffset_Ortho));
}

static inline void SET_Ortho(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_Ortho, fn);
}

typedef void (GLAPIENTRYP _glptr_PopMatrix)(void);
#define CALL_PopMatrix(disp, parameters) \
    (* GET_PopMatrix(disp)) parameters
static inline _glptr_PopMatrix GET_PopMatrix(struct _glapi_table *disp) {
   return (_glptr_PopMatrix) (GET_by_offset(disp, _gloffset_PopMatrix));
}

static inline void SET_PopMatrix(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_PopMatrix, fn);
}

typedef void (GLAPIENTRYP _glptr_PushMatrix)(void);
#define CALL_PushMatrix(disp, parameters) \
    (* GET_PushMatrix(disp)) parameters
static inline _glptr_PushMatrix GET_PushMatrix(struct _glapi_table *disp) {
   return (_glptr_PushMatrix) (GET_by_offset(disp, _gloffset_PushMatrix));
}

static inline void SET_PushMatrix(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_PushMatrix, fn);
}

typedef void (GLAPIENTRYP _glptr_Rotated)(GLdouble, GLdouble, GLdouble, GLdouble);
#define CALL_Rotated(disp, parameters) \
    (* GET_Rotated(disp)) parameters
static inline _glptr_Rotated GET_Rotated(struct _glapi_table *disp) {
   return (_glptr_Rotated) (GET_by_offset(disp, _gloffset_Rotated));
}

static inline void SET_Rotated(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_Rotated, fn);
}

typedef void (GLAPIENTRYP _glptr_Rotatef)(GLfloat, GLfloat, GLfloat, GLfloat);
#define CALL_Rotatef(disp, parameters) \
    (* GET_Rotatef(disp)) parameters
static inline _glptr_Rotatef GET_Rotatef(struct _glapi_table *disp) {
   return (_glptr_Rotatef) (GET_by_offset(disp, _gloffset_Rotatef));
}

static inline void SET_Rotatef(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_Rotatef, fn);
}

typedef void (GLAPIENTRYP _glptr_Scaled)(GLdouble, GLdouble, GLdouble);
#define CALL_Scaled(disp, parameters) \
    (* GET_Scaled(disp)) parameters
static inline _glptr_Scaled GET_Scaled(struct _glapi_table *disp) {
   return (_glptr_Scaled) (GET_by_offset(disp, _gloffset_Scaled));
}

static inline void SET_Scaled(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_Scaled, fn);
}

typedef void (GLAPIENTRYP _glptr_Scalef)(GLfloat, GLfloat, GLfloat);
#define CALL_Scalef(disp, parameters) \
    (* GET_Scalef(disp)) parameters
static inline _glptr_Scalef GET_Scalef(struct _glapi_table *disp) {
   return (_glptr_Scalef) (GET_by_offset(disp, _gloffset_Scalef));
}

static inline void SET_Scalef(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_Scalef, fn);
}

typedef void (GLAPIENTRYP _glptr_Translated)(GLdouble, GLdouble, GLdouble);
#define CALL_Translated(disp, parameters) \
    (* GET_Translated(disp)) parameters
static inline _glptr_Translated GET_Translated(struct _glapi_table *disp) {
   return (_glptr_Translated) (GET_by_offset(disp, _gloffset_Translated));
}

static inline void SET_Translated(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_Translated, fn);
}

typedef void (GLAPIENTRYP _glptr_Translatef)(GLfloat, GLfloat, GLfloat);
#define CALL_Translatef(disp, parameters) \
    (* GET_Translatef(disp)) parameters
static inline _glptr_Translatef GET_Translatef(struct _glapi_table *disp) {
   return (_glptr_Translatef) (GET_by_offset(disp, _gloffset_Translatef));
}

static inline void SET_Translatef(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_Translatef, fn);
}

typedef void (GLAPIENTRYP _glptr_Viewport)(GLint, GLint, GLsizei, GLsizei);
#define CALL_Viewport(disp, parameters) \
    (* GET_Viewport(disp)) parameters
static inline _glptr_Viewport GET_Viewport(struct _glapi_table *disp) {
   return (_glptr_Viewport) (GET_by_offset(disp, _gloffset_Viewport));
}

static inline void SET_Viewport(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLsizei, GLsizei)) {
   SET_by_offset(disp, _gloffset_Viewport, fn);
}

typedef void (GLAPIENTRYP _glptr_ArrayElement)(GLint);
#define CALL_ArrayElement(disp, parameters) \
    (* GET_ArrayElement(disp)) parameters
static inline _glptr_ArrayElement GET_ArrayElement(struct _glapi_table *disp) {
   return (_glptr_ArrayElement) (GET_by_offset(disp, _gloffset_ArrayElement));
}

static inline void SET_ArrayElement(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint)) {
   SET_by_offset(disp, _gloffset_ArrayElement, fn);
}

typedef void (GLAPIENTRYP _glptr_BindTexture)(GLenum, GLuint);
#define CALL_BindTexture(disp, parameters) \
    (* GET_BindTexture(disp)) parameters
static inline _glptr_BindTexture GET_BindTexture(struct _glapi_table *disp) {
   return (_glptr_BindTexture) (GET_by_offset(disp, _gloffset_BindTexture));
}

static inline void SET_BindTexture(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_BindTexture, fn);
}

typedef void (GLAPIENTRYP _glptr_ColorPointer)(GLint, GLenum, GLsizei, const GLvoid *);
#define CALL_ColorPointer(disp, parameters) \
    (* GET_ColorPointer(disp)) parameters
static inline _glptr_ColorPointer GET_ColorPointer(struct _glapi_table *disp) {
   return (_glptr_ColorPointer) (GET_by_offset(disp, _gloffset_ColorPointer));
}

static inline void SET_ColorPointer(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLenum, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_ColorPointer, fn);
}

typedef void (GLAPIENTRYP _glptr_DisableClientState)(GLenum);
#define CALL_DisableClientState(disp, parameters) \
    (* GET_DisableClientState(disp)) parameters
static inline _glptr_DisableClientState GET_DisableClientState(struct _glapi_table *disp) {
   return (_glptr_DisableClientState) (GET_by_offset(disp, _gloffset_DisableClientState));
}

static inline void SET_DisableClientState(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_DisableClientState, fn);
}

typedef void (GLAPIENTRYP _glptr_DrawArrays)(GLenum, GLint, GLsizei);
#define CALL_DrawArrays(disp, parameters) \
    (* GET_DrawArrays(disp)) parameters
static inline _glptr_DrawArrays GET_DrawArrays(struct _glapi_table *disp) {
   return (_glptr_DrawArrays) (GET_by_offset(disp, _gloffset_DrawArrays));
}

static inline void SET_DrawArrays(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLsizei)) {
   SET_by_offset(disp, _gloffset_DrawArrays, fn);
}

typedef void (GLAPIENTRYP _glptr_DrawElements)(GLenum, GLsizei, GLenum, const GLvoid *);
#define CALL_DrawElements(disp, parameters) \
    (* GET_DrawElements(disp)) parameters
static inline _glptr_DrawElements GET_DrawElements(struct _glapi_table *disp) {
   return (_glptr_DrawElements) (GET_by_offset(disp, _gloffset_DrawElements));
}

static inline void SET_DrawElements(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLenum, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_DrawElements, fn);
}

typedef void (GLAPIENTRYP _glptr_EdgeFlagPointer)(GLsizei, const GLvoid *);
#define CALL_EdgeFlagPointer(disp, parameters) \
    (* GET_EdgeFlagPointer(disp)) parameters
static inline _glptr_EdgeFlagPointer GET_EdgeFlagPointer(struct _glapi_table *disp) {
   return (_glptr_EdgeFlagPointer) (GET_by_offset(disp, _gloffset_EdgeFlagPointer));
}

static inline void SET_EdgeFlagPointer(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_EdgeFlagPointer, fn);
}

typedef void (GLAPIENTRYP _glptr_EnableClientState)(GLenum);
#define CALL_EnableClientState(disp, parameters) \
    (* GET_EnableClientState(disp)) parameters
static inline _glptr_EnableClientState GET_EnableClientState(struct _glapi_table *disp) {
   return (_glptr_EnableClientState) (GET_by_offset(disp, _gloffset_EnableClientState));
}

static inline void SET_EnableClientState(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_EnableClientState, fn);
}

typedef void (GLAPIENTRYP _glptr_IndexPointer)(GLenum, GLsizei, const GLvoid *);
#define CALL_IndexPointer(disp, parameters) \
    (* GET_IndexPointer(disp)) parameters
static inline _glptr_IndexPointer GET_IndexPointer(struct _glapi_table *disp) {
   return (_glptr_IndexPointer) (GET_by_offset(disp, _gloffset_IndexPointer));
}

static inline void SET_IndexPointer(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_IndexPointer, fn);
}

typedef void (GLAPIENTRYP _glptr_Indexub)(GLubyte);
#define CALL_Indexub(disp, parameters) \
    (* GET_Indexub(disp)) parameters
static inline _glptr_Indexub GET_Indexub(struct _glapi_table *disp) {
   return (_glptr_Indexub) (GET_by_offset(disp, _gloffset_Indexub));
}

static inline void SET_Indexub(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLubyte)) {
   SET_by_offset(disp, _gloffset_Indexub, fn);
}

typedef void (GLAPIENTRYP _glptr_Indexubv)(const GLubyte *);
#define CALL_Indexubv(disp, parameters) \
    (* GET_Indexubv(disp)) parameters
static inline _glptr_Indexubv GET_Indexubv(struct _glapi_table *disp) {
   return (_glptr_Indexubv) (GET_by_offset(disp, _gloffset_Indexubv));
}

static inline void SET_Indexubv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLubyte *)) {
   SET_by_offset(disp, _gloffset_Indexubv, fn);
}

typedef void (GLAPIENTRYP _glptr_InterleavedArrays)(GLenum, GLsizei, const GLvoid *);
#define CALL_InterleavedArrays(disp, parameters) \
    (* GET_InterleavedArrays(disp)) parameters
static inline _glptr_InterleavedArrays GET_InterleavedArrays(struct _glapi_table *disp) {
   return (_glptr_InterleavedArrays) (GET_by_offset(disp, _gloffset_InterleavedArrays));
}

static inline void SET_InterleavedArrays(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_InterleavedArrays, fn);
}

typedef void (GLAPIENTRYP _glptr_NormalPointer)(GLenum, GLsizei, const GLvoid *);
#define CALL_NormalPointer(disp, parameters) \
    (* GET_NormalPointer(disp)) parameters
static inline _glptr_NormalPointer GET_NormalPointer(struct _glapi_table *disp) {
   return (_glptr_NormalPointer) (GET_by_offset(disp, _gloffset_NormalPointer));
}

static inline void SET_NormalPointer(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_NormalPointer, fn);
}

typedef void (GLAPIENTRYP _glptr_PolygonOffset)(GLfloat, GLfloat);
#define CALL_PolygonOffset(disp, parameters) \
    (* GET_PolygonOffset(disp)) parameters
static inline _glptr_PolygonOffset GET_PolygonOffset(struct _glapi_table *disp) {
   return (_glptr_PolygonOffset) (GET_by_offset(disp, _gloffset_PolygonOffset));
}

static inline void SET_PolygonOffset(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_PolygonOffset, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoordPointer)(GLint, GLenum, GLsizei, const GLvoid *);
#define CALL_TexCoordPointer(disp, parameters) \
    (* GET_TexCoordPointer(disp)) parameters
static inline _glptr_TexCoordPointer GET_TexCoordPointer(struct _glapi_table *disp) {
   return (_glptr_TexCoordPointer) (GET_by_offset(disp, _gloffset_TexCoordPointer));
}

static inline void SET_TexCoordPointer(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLenum, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_TexCoordPointer, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexPointer)(GLint, GLenum, GLsizei, const GLvoid *);
#define CALL_VertexPointer(disp, parameters) \
    (* GET_VertexPointer(disp)) parameters
static inline _glptr_VertexPointer GET_VertexPointer(struct _glapi_table *disp) {
   return (_glptr_VertexPointer) (GET_by_offset(disp, _gloffset_VertexPointer));
}

static inline void SET_VertexPointer(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLenum, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_VertexPointer, fn);
}

typedef GLboolean (GLAPIENTRYP _glptr_AreTexturesResident)(GLsizei, const GLuint *, GLboolean *);
#define CALL_AreTexturesResident(disp, parameters) \
    (* GET_AreTexturesResident(disp)) parameters
static inline _glptr_AreTexturesResident GET_AreTexturesResident(struct _glapi_table *disp) {
   return (_glptr_AreTexturesResident) (GET_by_offset(disp, _gloffset_AreTexturesResident));
}

static inline void SET_AreTexturesResident(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLsizei, const GLuint *, GLboolean *)) {
   SET_by_offset(disp, _gloffset_AreTexturesResident, fn);
}

typedef void (GLAPIENTRYP _glptr_CopyTexImage1D)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLint);
#define CALL_CopyTexImage1D(disp, parameters) \
    (* GET_CopyTexImage1D(disp)) parameters
static inline _glptr_CopyTexImage1D GET_CopyTexImage1D(struct _glapi_table *disp) {
   return (_glptr_CopyTexImage1D) (GET_by_offset(disp, _gloffset_CopyTexImage1D));
}

static inline void SET_CopyTexImage1D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLint)) {
   SET_by_offset(disp, _gloffset_CopyTexImage1D, fn);
}

typedef void (GLAPIENTRYP _glptr_CopyTexImage2D)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint);
#define CALL_CopyTexImage2D(disp, parameters) \
    (* GET_CopyTexImage2D(disp)) parameters
static inline _glptr_CopyTexImage2D GET_CopyTexImage2D(struct _glapi_table *disp) {
   return (_glptr_CopyTexImage2D) (GET_by_offset(disp, _gloffset_CopyTexImage2D));
}

static inline void SET_CopyTexImage2D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint)) {
   SET_by_offset(disp, _gloffset_CopyTexImage2D, fn);
}

typedef void (GLAPIENTRYP _glptr_CopyTexSubImage1D)(GLenum, GLint, GLint, GLint, GLint, GLsizei);
#define CALL_CopyTexSubImage1D(disp, parameters) \
    (* GET_CopyTexSubImage1D(disp)) parameters
static inline _glptr_CopyTexSubImage1D GET_CopyTexSubImage1D(struct _glapi_table *disp) {
   return (_glptr_CopyTexSubImage1D) (GET_by_offset(disp, _gloffset_CopyTexSubImage1D));
}

static inline void SET_CopyTexSubImage1D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLint, GLint, GLsizei)) {
   SET_by_offset(disp, _gloffset_CopyTexSubImage1D, fn);
}

typedef void (GLAPIENTRYP _glptr_CopyTexSubImage2D)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
#define CALL_CopyTexSubImage2D(disp, parameters) \
    (* GET_CopyTexSubImage2D(disp)) parameters
static inline _glptr_CopyTexSubImage2D GET_CopyTexSubImage2D(struct _glapi_table *disp) {
   return (_glptr_CopyTexSubImage2D) (GET_by_offset(disp, _gloffset_CopyTexSubImage2D));
}

static inline void SET_CopyTexSubImage2D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei)) {
   SET_by_offset(disp, _gloffset_CopyTexSubImage2D, fn);
}

typedef void (GLAPIENTRYP _glptr_DeleteTextures)(GLsizei, const GLuint *);
#define CALL_DeleteTextures(disp, parameters) \
    (* GET_DeleteTextures(disp)) parameters
static inline _glptr_DeleteTextures GET_DeleteTextures(struct _glapi_table *disp) {
   return (_glptr_DeleteTextures) (GET_by_offset(disp, _gloffset_DeleteTextures));
}

static inline void SET_DeleteTextures(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_DeleteTextures, fn);
}

typedef void (GLAPIENTRYP _glptr_GenTextures)(GLsizei, GLuint *);
#define CALL_GenTextures(disp, parameters) \
    (* GET_GenTextures(disp)) parameters
static inline _glptr_GenTextures GET_GenTextures(struct _glapi_table *disp) {
   return (_glptr_GenTextures) (GET_by_offset(disp, _gloffset_GenTextures));
}

static inline void SET_GenTextures(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLuint *)) {
   SET_by_offset(disp, _gloffset_GenTextures, fn);
}

typedef void (GLAPIENTRYP _glptr_GetPointerv)(GLenum, GLvoid **);
#define CALL_GetPointerv(disp, parameters) \
    (* GET_GetPointerv(disp)) parameters
static inline _glptr_GetPointerv GET_GetPointerv(struct _glapi_table *disp) {
   return (_glptr_GetPointerv) (GET_by_offset(disp, _gloffset_GetPointerv));
}

static inline void SET_GetPointerv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLvoid **)) {
   SET_by_offset(disp, _gloffset_GetPointerv, fn);
}

typedef GLboolean (GLAPIENTRYP _glptr_IsTexture)(GLuint);
#define CALL_IsTexture(disp, parameters) \
    (* GET_IsTexture(disp)) parameters
static inline _glptr_IsTexture GET_IsTexture(struct _glapi_table *disp) {
   return (_glptr_IsTexture) (GET_by_offset(disp, _gloffset_IsTexture));
}

static inline void SET_IsTexture(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_IsTexture, fn);
}

typedef void (GLAPIENTRYP _glptr_PrioritizeTextures)(GLsizei, const GLuint *, const GLclampf *);
#define CALL_PrioritizeTextures(disp, parameters) \
    (* GET_PrioritizeTextures(disp)) parameters
static inline _glptr_PrioritizeTextures GET_PrioritizeTextures(struct _glapi_table *disp) {
   return (_glptr_PrioritizeTextures) (GET_by_offset(disp, _gloffset_PrioritizeTextures));
}

static inline void SET_PrioritizeTextures(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLuint *, const GLclampf *)) {
   SET_by_offset(disp, _gloffset_PrioritizeTextures, fn);
}

typedef void (GLAPIENTRYP _glptr_TexSubImage1D)(GLenum, GLint, GLint, GLsizei, GLenum, GLenum, const GLvoid *);
#define CALL_TexSubImage1D(disp, parameters) \
    (* GET_TexSubImage1D(disp)) parameters
static inline _glptr_TexSubImage1D GET_TexSubImage1D(struct _glapi_table *disp) {
   return (_glptr_TexSubImage1D) (GET_by_offset(disp, _gloffset_TexSubImage1D));
}

static inline void SET_TexSubImage1D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLsizei, GLenum, GLenum, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_TexSubImage1D, fn);
}

typedef void (GLAPIENTRYP _glptr_TexSubImage2D)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
#define CALL_TexSubImage2D(disp, parameters) \
    (* GET_TexSubImage2D(disp)) parameters
static inline _glptr_TexSubImage2D GET_TexSubImage2D(struct _glapi_table *disp) {
   return (_glptr_TexSubImage2D) (GET_by_offset(disp, _gloffset_TexSubImage2D));
}

static inline void SET_TexSubImage2D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_TexSubImage2D, fn);
}

typedef void (GLAPIENTRYP _glptr_PopClientAttrib)(void);
#define CALL_PopClientAttrib(disp, parameters) \
    (* GET_PopClientAttrib(disp)) parameters
static inline _glptr_PopClientAttrib GET_PopClientAttrib(struct _glapi_table *disp) {
   return (_glptr_PopClientAttrib) (GET_by_offset(disp, _gloffset_PopClientAttrib));
}

static inline void SET_PopClientAttrib(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_PopClientAttrib, fn);
}

typedef void (GLAPIENTRYP _glptr_PushClientAttrib)(GLbitfield);
#define CALL_PushClientAttrib(disp, parameters) \
    (* GET_PushClientAttrib(disp)) parameters
static inline _glptr_PushClientAttrib GET_PushClientAttrib(struct _glapi_table *disp) {
   return (_glptr_PushClientAttrib) (GET_by_offset(disp, _gloffset_PushClientAttrib));
}

static inline void SET_PushClientAttrib(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLbitfield)) {
   SET_by_offset(disp, _gloffset_PushClientAttrib, fn);
}

typedef void (GLAPIENTRYP _glptr_BlendColor)(GLclampf, GLclampf, GLclampf, GLclampf);
#define CALL_BlendColor(disp, parameters) \
    (* GET_BlendColor(disp)) parameters
static inline _glptr_BlendColor GET_BlendColor(struct _glapi_table *disp) {
   return (_glptr_BlendColor) (GET_by_offset(disp, _gloffset_BlendColor));
}

static inline void SET_BlendColor(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLclampf, GLclampf, GLclampf, GLclampf)) {
   SET_by_offset(disp, _gloffset_BlendColor, fn);
}

typedef void (GLAPIENTRYP _glptr_BlendEquation)(GLenum);
#define CALL_BlendEquation(disp, parameters) \
    (* GET_BlendEquation(disp)) parameters
static inline _glptr_BlendEquation GET_BlendEquation(struct _glapi_table *disp) {
   return (_glptr_BlendEquation) (GET_by_offset(disp, _gloffset_BlendEquation));
}

static inline void SET_BlendEquation(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_BlendEquation, fn);
}

typedef void (GLAPIENTRYP _glptr_DrawRangeElements)(GLenum, GLuint, GLuint, GLsizei, GLenum, const GLvoid *);
#define CALL_DrawRangeElements(disp, parameters) \
    (* GET_DrawRangeElements(disp)) parameters
static inline _glptr_DrawRangeElements GET_DrawRangeElements(struct _glapi_table *disp) {
   return (_glptr_DrawRangeElements) (GET_by_offset(disp, _gloffset_DrawRangeElements));
}

static inline void SET_DrawRangeElements(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLuint, GLsizei, GLenum, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_DrawRangeElements, fn);
}

typedef void (GLAPIENTRYP _glptr_ColorTable)(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *);
#define CALL_ColorTable(disp, parameters) \
    (* GET_ColorTable(disp)) parameters
static inline _glptr_ColorTable GET_ColorTable(struct _glapi_table *disp) {
   return (_glptr_ColorTable) (GET_by_offset(disp, _gloffset_ColorTable));
}

static inline void SET_ColorTable(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_ColorTable, fn);
}

typedef void (GLAPIENTRYP _glptr_ColorTableParameterfv)(GLenum, GLenum, const GLfloat *);
#define CALL_ColorTableParameterfv(disp, parameters) \
    (* GET_ColorTableParameterfv(disp)) parameters
static inline _glptr_ColorTableParameterfv GET_ColorTableParameterfv(struct _glapi_table *disp) {
   return (_glptr_ColorTableParameterfv) (GET_by_offset(disp, _gloffset_ColorTableParameterfv));
}

static inline void SET_ColorTableParameterfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_ColorTableParameterfv, fn);
}

typedef void (GLAPIENTRYP _glptr_ColorTableParameteriv)(GLenum, GLenum, const GLint *);
#define CALL_ColorTableParameteriv(disp, parameters) \
    (* GET_ColorTableParameteriv(disp)) parameters
static inline _glptr_ColorTableParameteriv GET_ColorTableParameteriv(struct _glapi_table *disp) {
   return (_glptr_ColorTableParameteriv) (GET_by_offset(disp, _gloffset_ColorTableParameteriv));
}

static inline void SET_ColorTableParameteriv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_ColorTableParameteriv, fn);
}

typedef void (GLAPIENTRYP _glptr_CopyColorTable)(GLenum, GLenum, GLint, GLint, GLsizei);
#define CALL_CopyColorTable(disp, parameters) \
    (* GET_CopyColorTable(disp)) parameters
static inline _glptr_CopyColorTable GET_CopyColorTable(struct _glapi_table *disp) {
   return (_glptr_CopyColorTable) (GET_by_offset(disp, _gloffset_CopyColorTable));
}

static inline void SET_CopyColorTable(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint, GLint, GLsizei)) {
   SET_by_offset(disp, _gloffset_CopyColorTable, fn);
}

typedef void (GLAPIENTRYP _glptr_GetColorTable)(GLenum, GLenum, GLenum, GLvoid *);
#define CALL_GetColorTable(disp, parameters) \
    (* GET_GetColorTable(disp)) parameters
static inline _glptr_GetColorTable GET_GetColorTable(struct _glapi_table *disp) {
   return (_glptr_GetColorTable) (GET_by_offset(disp, _gloffset_GetColorTable));
}

static inline void SET_GetColorTable(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetColorTable, fn);
}

typedef void (GLAPIENTRYP _glptr_GetColorTableParameterfv)(GLenum, GLenum, GLfloat *);
#define CALL_GetColorTableParameterfv(disp, parameters) \
    (* GET_GetColorTableParameterfv(disp)) parameters
static inline _glptr_GetColorTableParameterfv GET_GetColorTableParameterfv(struct _glapi_table *disp) {
   return (_glptr_GetColorTableParameterfv) (GET_by_offset(disp, _gloffset_GetColorTableParameterfv));
}

static inline void SET_GetColorTableParameterfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetColorTableParameterfv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetColorTableParameteriv)(GLenum, GLenum, GLint *);
#define CALL_GetColorTableParameteriv(disp, parameters) \
    (* GET_GetColorTableParameteriv(disp)) parameters
static inline _glptr_GetColorTableParameteriv GET_GetColorTableParameteriv(struct _glapi_table *disp) {
   return (_glptr_GetColorTableParameteriv) (GET_by_offset(disp, _gloffset_GetColorTableParameteriv));
}

static inline void SET_GetColorTableParameteriv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetColorTableParameteriv, fn);
}

typedef void (GLAPIENTRYP _glptr_ColorSubTable)(GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
#define CALL_ColorSubTable(disp, parameters) \
    (* GET_ColorSubTable(disp)) parameters
static inline _glptr_ColorSubTable GET_ColorSubTable(struct _glapi_table *disp) {
   return (_glptr_ColorSubTable) (GET_by_offset(disp, _gloffset_ColorSubTable));
}

static inline void SET_ColorSubTable(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_ColorSubTable, fn);
}

typedef void (GLAPIENTRYP _glptr_CopyColorSubTable)(GLenum, GLsizei, GLint, GLint, GLsizei);
#define CALL_CopyColorSubTable(disp, parameters) \
    (* GET_CopyColorSubTable(disp)) parameters
static inline _glptr_CopyColorSubTable GET_CopyColorSubTable(struct _glapi_table *disp) {
   return (_glptr_CopyColorSubTable) (GET_by_offset(disp, _gloffset_CopyColorSubTable));
}

static inline void SET_CopyColorSubTable(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLint, GLint, GLsizei)) {
   SET_by_offset(disp, _gloffset_CopyColorSubTable, fn);
}

typedef void (GLAPIENTRYP _glptr_ConvolutionFilter1D)(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *);
#define CALL_ConvolutionFilter1D(disp, parameters) \
    (* GET_ConvolutionFilter1D(disp)) parameters
static inline _glptr_ConvolutionFilter1D GET_ConvolutionFilter1D(struct _glapi_table *disp) {
   return (_glptr_ConvolutionFilter1D) (GET_by_offset(disp, _gloffset_ConvolutionFilter1D));
}

static inline void SET_ConvolutionFilter1D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_ConvolutionFilter1D, fn);
}

typedef void (GLAPIENTRYP _glptr_ConvolutionFilter2D)(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
#define CALL_ConvolutionFilter2D(disp, parameters) \
    (* GET_ConvolutionFilter2D(disp)) parameters
static inline _glptr_ConvolutionFilter2D GET_ConvolutionFilter2D(struct _glapi_table *disp) {
   return (_glptr_ConvolutionFilter2D) (GET_by_offset(disp, _gloffset_ConvolutionFilter2D));
}

static inline void SET_ConvolutionFilter2D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_ConvolutionFilter2D, fn);
}

typedef void (GLAPIENTRYP _glptr_ConvolutionParameterf)(GLenum, GLenum, GLfloat);
#define CALL_ConvolutionParameterf(disp, parameters) \
    (* GET_ConvolutionParameterf(disp)) parameters
static inline _glptr_ConvolutionParameterf GET_ConvolutionParameterf(struct _glapi_table *disp) {
   return (_glptr_ConvolutionParameterf) (GET_by_offset(disp, _gloffset_ConvolutionParameterf));
}

static inline void SET_ConvolutionParameterf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_ConvolutionParameterf, fn);
}

typedef void (GLAPIENTRYP _glptr_ConvolutionParameterfv)(GLenum, GLenum, const GLfloat *);
#define CALL_ConvolutionParameterfv(disp, parameters) \
    (* GET_ConvolutionParameterfv(disp)) parameters
static inline _glptr_ConvolutionParameterfv GET_ConvolutionParameterfv(struct _glapi_table *disp) {
   return (_glptr_ConvolutionParameterfv) (GET_by_offset(disp, _gloffset_ConvolutionParameterfv));
}

static inline void SET_ConvolutionParameterfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_ConvolutionParameterfv, fn);
}

typedef void (GLAPIENTRYP _glptr_ConvolutionParameteri)(GLenum, GLenum, GLint);
#define CALL_ConvolutionParameteri(disp, parameters) \
    (* GET_ConvolutionParameteri(disp)) parameters
static inline _glptr_ConvolutionParameteri GET_ConvolutionParameteri(struct _glapi_table *disp) {
   return (_glptr_ConvolutionParameteri) (GET_by_offset(disp, _gloffset_ConvolutionParameteri));
}

static inline void SET_ConvolutionParameteri(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_ConvolutionParameteri, fn);
}

typedef void (GLAPIENTRYP _glptr_ConvolutionParameteriv)(GLenum, GLenum, const GLint *);
#define CALL_ConvolutionParameteriv(disp, parameters) \
    (* GET_ConvolutionParameteriv(disp)) parameters
static inline _glptr_ConvolutionParameteriv GET_ConvolutionParameteriv(struct _glapi_table *disp) {
   return (_glptr_ConvolutionParameteriv) (GET_by_offset(disp, _gloffset_ConvolutionParameteriv));
}

static inline void SET_ConvolutionParameteriv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_ConvolutionParameteriv, fn);
}

typedef void (GLAPIENTRYP _glptr_CopyConvolutionFilter1D)(GLenum, GLenum, GLint, GLint, GLsizei);
#define CALL_CopyConvolutionFilter1D(disp, parameters) \
    (* GET_CopyConvolutionFilter1D(disp)) parameters
static inline _glptr_CopyConvolutionFilter1D GET_CopyConvolutionFilter1D(struct _glapi_table *disp) {
   return (_glptr_CopyConvolutionFilter1D) (GET_by_offset(disp, _gloffset_CopyConvolutionFilter1D));
}

static inline void SET_CopyConvolutionFilter1D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint, GLint, GLsizei)) {
   SET_by_offset(disp, _gloffset_CopyConvolutionFilter1D, fn);
}

typedef void (GLAPIENTRYP _glptr_CopyConvolutionFilter2D)(GLenum, GLenum, GLint, GLint, GLsizei, GLsizei);
#define CALL_CopyConvolutionFilter2D(disp, parameters) \
    (* GET_CopyConvolutionFilter2D(disp)) parameters
static inline _glptr_CopyConvolutionFilter2D GET_CopyConvolutionFilter2D(struct _glapi_table *disp) {
   return (_glptr_CopyConvolutionFilter2D) (GET_by_offset(disp, _gloffset_CopyConvolutionFilter2D));
}

static inline void SET_CopyConvolutionFilter2D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint, GLint, GLsizei, GLsizei)) {
   SET_by_offset(disp, _gloffset_CopyConvolutionFilter2D, fn);
}

typedef void (GLAPIENTRYP _glptr_GetConvolutionFilter)(GLenum, GLenum, GLenum, GLvoid *);
#define CALL_GetConvolutionFilter(disp, parameters) \
    (* GET_GetConvolutionFilter(disp)) parameters
static inline _glptr_GetConvolutionFilter GET_GetConvolutionFilter(struct _glapi_table *disp) {
   return (_glptr_GetConvolutionFilter) (GET_by_offset(disp, _gloffset_GetConvolutionFilter));
}

static inline void SET_GetConvolutionFilter(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetConvolutionFilter, fn);
}

typedef void (GLAPIENTRYP _glptr_GetConvolutionParameterfv)(GLenum, GLenum, GLfloat *);
#define CALL_GetConvolutionParameterfv(disp, parameters) \
    (* GET_GetConvolutionParameterfv(disp)) parameters
static inline _glptr_GetConvolutionParameterfv GET_GetConvolutionParameterfv(struct _glapi_table *disp) {
   return (_glptr_GetConvolutionParameterfv) (GET_by_offset(disp, _gloffset_GetConvolutionParameterfv));
}

static inline void SET_GetConvolutionParameterfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetConvolutionParameterfv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetConvolutionParameteriv)(GLenum, GLenum, GLint *);
#define CALL_GetConvolutionParameteriv(disp, parameters) \
    (* GET_GetConvolutionParameteriv(disp)) parameters
static inline _glptr_GetConvolutionParameteriv GET_GetConvolutionParameteriv(struct _glapi_table *disp) {
   return (_glptr_GetConvolutionParameteriv) (GET_by_offset(disp, _gloffset_GetConvolutionParameteriv));
}

static inline void SET_GetConvolutionParameteriv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetConvolutionParameteriv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetSeparableFilter)(GLenum, GLenum, GLenum, GLvoid *, GLvoid *, GLvoid *);
#define CALL_GetSeparableFilter(disp, parameters) \
    (* GET_GetSeparableFilter(disp)) parameters
static inline _glptr_GetSeparableFilter GET_GetSeparableFilter(struct _glapi_table *disp) {
   return (_glptr_GetSeparableFilter) (GET_by_offset(disp, _gloffset_GetSeparableFilter));
}

static inline void SET_GetSeparableFilter(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLvoid *, GLvoid *, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetSeparableFilter, fn);
}

typedef void (GLAPIENTRYP _glptr_SeparableFilter2D)(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *, const GLvoid *);
#define CALL_SeparableFilter2D(disp, parameters) \
    (* GET_SeparableFilter2D(disp)) parameters
static inline _glptr_SeparableFilter2D GET_SeparableFilter2D(struct _glapi_table *disp) {
   return (_glptr_SeparableFilter2D) (GET_by_offset(disp, _gloffset_SeparableFilter2D));
}

static inline void SET_SeparableFilter2D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_SeparableFilter2D, fn);
}

typedef void (GLAPIENTRYP _glptr_GetHistogram)(GLenum, GLboolean, GLenum, GLenum, GLvoid *);
#define CALL_GetHistogram(disp, parameters) \
    (* GET_GetHistogram(disp)) parameters
static inline _glptr_GetHistogram GET_GetHistogram(struct _glapi_table *disp) {
   return (_glptr_GetHistogram) (GET_by_offset(disp, _gloffset_GetHistogram));
}

static inline void SET_GetHistogram(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLboolean, GLenum, GLenum, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetHistogram, fn);
}

typedef void (GLAPIENTRYP _glptr_GetHistogramParameterfv)(GLenum, GLenum, GLfloat *);
#define CALL_GetHistogramParameterfv(disp, parameters) \
    (* GET_GetHistogramParameterfv(disp)) parameters
static inline _glptr_GetHistogramParameterfv GET_GetHistogramParameterfv(struct _glapi_table *disp) {
   return (_glptr_GetHistogramParameterfv) (GET_by_offset(disp, _gloffset_GetHistogramParameterfv));
}

static inline void SET_GetHistogramParameterfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetHistogramParameterfv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetHistogramParameteriv)(GLenum, GLenum, GLint *);
#define CALL_GetHistogramParameteriv(disp, parameters) \
    (* GET_GetHistogramParameteriv(disp)) parameters
static inline _glptr_GetHistogramParameteriv GET_GetHistogramParameteriv(struct _glapi_table *disp) {
   return (_glptr_GetHistogramParameteriv) (GET_by_offset(disp, _gloffset_GetHistogramParameteriv));
}

static inline void SET_GetHistogramParameteriv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetHistogramParameteriv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetMinmax)(GLenum, GLboolean, GLenum, GLenum, GLvoid *);
#define CALL_GetMinmax(disp, parameters) \
    (* GET_GetMinmax(disp)) parameters
static inline _glptr_GetMinmax GET_GetMinmax(struct _glapi_table *disp) {
   return (_glptr_GetMinmax) (GET_by_offset(disp, _gloffset_GetMinmax));
}

static inline void SET_GetMinmax(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLboolean, GLenum, GLenum, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetMinmax, fn);
}

typedef void (GLAPIENTRYP _glptr_GetMinmaxParameterfv)(GLenum, GLenum, GLfloat *);
#define CALL_GetMinmaxParameterfv(disp, parameters) \
    (* GET_GetMinmaxParameterfv(disp)) parameters
static inline _glptr_GetMinmaxParameterfv GET_GetMinmaxParameterfv(struct _glapi_table *disp) {
   return (_glptr_GetMinmaxParameterfv) (GET_by_offset(disp, _gloffset_GetMinmaxParameterfv));
}

static inline void SET_GetMinmaxParameterfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetMinmaxParameterfv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetMinmaxParameteriv)(GLenum, GLenum, GLint *);
#define CALL_GetMinmaxParameteriv(disp, parameters) \
    (* GET_GetMinmaxParameteriv(disp)) parameters
static inline _glptr_GetMinmaxParameteriv GET_GetMinmaxParameteriv(struct _glapi_table *disp) {
   return (_glptr_GetMinmaxParameteriv) (GET_by_offset(disp, _gloffset_GetMinmaxParameteriv));
}

static inline void SET_GetMinmaxParameteriv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetMinmaxParameteriv, fn);
}

typedef void (GLAPIENTRYP _glptr_Histogram)(GLenum, GLsizei, GLenum, GLboolean);
#define CALL_Histogram(disp, parameters) \
    (* GET_Histogram(disp)) parameters
static inline _glptr_Histogram GET_Histogram(struct _glapi_table *disp) {
   return (_glptr_Histogram) (GET_by_offset(disp, _gloffset_Histogram));
}

static inline void SET_Histogram(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLenum, GLboolean)) {
   SET_by_offset(disp, _gloffset_Histogram, fn);
}

typedef void (GLAPIENTRYP _glptr_Minmax)(GLenum, GLenum, GLboolean);
#define CALL_Minmax(disp, parameters) \
    (* GET_Minmax(disp)) parameters
static inline _glptr_Minmax GET_Minmax(struct _glapi_table *disp) {
   return (_glptr_Minmax) (GET_by_offset(disp, _gloffset_Minmax));
}

static inline void SET_Minmax(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLboolean)) {
   SET_by_offset(disp, _gloffset_Minmax, fn);
}

typedef void (GLAPIENTRYP _glptr_ResetHistogram)(GLenum);
#define CALL_ResetHistogram(disp, parameters) \
    (* GET_ResetHistogram(disp)) parameters
static inline _glptr_ResetHistogram GET_ResetHistogram(struct _glapi_table *disp) {
   return (_glptr_ResetHistogram) (GET_by_offset(disp, _gloffset_ResetHistogram));
}

static inline void SET_ResetHistogram(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_ResetHistogram, fn);
}

typedef void (GLAPIENTRYP _glptr_ResetMinmax)(GLenum);
#define CALL_ResetMinmax(disp, parameters) \
    (* GET_ResetMinmax(disp)) parameters
static inline _glptr_ResetMinmax GET_ResetMinmax(struct _glapi_table *disp) {
   return (_glptr_ResetMinmax) (GET_by_offset(disp, _gloffset_ResetMinmax));
}

static inline void SET_ResetMinmax(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_ResetMinmax, fn);
}

typedef void (GLAPIENTRYP _glptr_TexImage3D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
#define CALL_TexImage3D(disp, parameters) \
    (* GET_TexImage3D(disp)) parameters
static inline _glptr_TexImage3D GET_TexImage3D(struct _glapi_table *disp) {
   return (_glptr_TexImage3D) (GET_by_offset(disp, _gloffset_TexImage3D));
}

static inline void SET_TexImage3D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_TexImage3D, fn);
}

typedef void (GLAPIENTRYP _glptr_TexSubImage3D)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
#define CALL_TexSubImage3D(disp, parameters) \
    (* GET_TexSubImage3D(disp)) parameters
static inline _glptr_TexSubImage3D GET_TexSubImage3D(struct _glapi_table *disp) {
   return (_glptr_TexSubImage3D) (GET_by_offset(disp, _gloffset_TexSubImage3D));
}

static inline void SET_TexSubImage3D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_TexSubImage3D, fn);
}

typedef void (GLAPIENTRYP _glptr_CopyTexSubImage3D)(GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
#define CALL_CopyTexSubImage3D(disp, parameters) \
    (* GET_CopyTexSubImage3D(disp)) parameters
static inline _glptr_CopyTexSubImage3D GET_CopyTexSubImage3D(struct _glapi_table *disp) {
   return (_glptr_CopyTexSubImage3D) (GET_by_offset(disp, _gloffset_CopyTexSubImage3D));
}

static inline void SET_CopyTexSubImage3D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei)) {
   SET_by_offset(disp, _gloffset_CopyTexSubImage3D, fn);
}

typedef void (GLAPIENTRYP _glptr_ActiveTextureARB)(GLenum);
#define CALL_ActiveTextureARB(disp, parameters) \
    (* GET_ActiveTextureARB(disp)) parameters
static inline _glptr_ActiveTextureARB GET_ActiveTextureARB(struct _glapi_table *disp) {
   return (_glptr_ActiveTextureARB) (GET_by_offset(disp, _gloffset_ActiveTextureARB));
}

static inline void SET_ActiveTextureARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_ActiveTextureARB, fn);
}

typedef void (GLAPIENTRYP _glptr_ClientActiveTextureARB)(GLenum);
#define CALL_ClientActiveTextureARB(disp, parameters) \
    (* GET_ClientActiveTextureARB(disp)) parameters
static inline _glptr_ClientActiveTextureARB GET_ClientActiveTextureARB(struct _glapi_table *disp) {
   return (_glptr_ClientActiveTextureARB) (GET_by_offset(disp, _gloffset_ClientActiveTextureARB));
}

static inline void SET_ClientActiveTextureARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_ClientActiveTextureARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord1dARB)(GLenum, GLdouble);
#define CALL_MultiTexCoord1dARB(disp, parameters) \
    (* GET_MultiTexCoord1dARB(disp)) parameters
static inline _glptr_MultiTexCoord1dARB GET_MultiTexCoord1dARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord1dARB) (GET_by_offset(disp, _gloffset_MultiTexCoord1dARB));
}

static inline void SET_MultiTexCoord1dARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLdouble)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord1dARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord1dvARB)(GLenum, const GLdouble *);
#define CALL_MultiTexCoord1dvARB(disp, parameters) \
    (* GET_MultiTexCoord1dvARB(disp)) parameters
static inline _glptr_MultiTexCoord1dvARB GET_MultiTexCoord1dvARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord1dvARB) (GET_by_offset(disp, _gloffset_MultiTexCoord1dvARB));
}

static inline void SET_MultiTexCoord1dvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord1dvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord1fARB)(GLenum, GLfloat);
#define CALL_MultiTexCoord1fARB(disp, parameters) \
    (* GET_MultiTexCoord1fARB(disp)) parameters
static inline _glptr_MultiTexCoord1fARB GET_MultiTexCoord1fARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord1fARB) (GET_by_offset(disp, _gloffset_MultiTexCoord1fARB));
}

static inline void SET_MultiTexCoord1fARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord1fARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord1fvARB)(GLenum, const GLfloat *);
#define CALL_MultiTexCoord1fvARB(disp, parameters) \
    (* GET_MultiTexCoord1fvARB(disp)) parameters
static inline _glptr_MultiTexCoord1fvARB GET_MultiTexCoord1fvARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord1fvARB) (GET_by_offset(disp, _gloffset_MultiTexCoord1fvARB));
}

static inline void SET_MultiTexCoord1fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord1fvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord1iARB)(GLenum, GLint);
#define CALL_MultiTexCoord1iARB(disp, parameters) \
    (* GET_MultiTexCoord1iARB(disp)) parameters
static inline _glptr_MultiTexCoord1iARB GET_MultiTexCoord1iARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord1iARB) (GET_by_offset(disp, _gloffset_MultiTexCoord1iARB));
}

static inline void SET_MultiTexCoord1iARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord1iARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord1ivARB)(GLenum, const GLint *);
#define CALL_MultiTexCoord1ivARB(disp, parameters) \
    (* GET_MultiTexCoord1ivARB(disp)) parameters
static inline _glptr_MultiTexCoord1ivARB GET_MultiTexCoord1ivARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord1ivARB) (GET_by_offset(disp, _gloffset_MultiTexCoord1ivARB));
}

static inline void SET_MultiTexCoord1ivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord1ivARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord1sARB)(GLenum, GLshort);
#define CALL_MultiTexCoord1sARB(disp, parameters) \
    (* GET_MultiTexCoord1sARB(disp)) parameters
static inline _glptr_MultiTexCoord1sARB GET_MultiTexCoord1sARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord1sARB) (GET_by_offset(disp, _gloffset_MultiTexCoord1sARB));
}

static inline void SET_MultiTexCoord1sARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLshort)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord1sARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord1svARB)(GLenum, const GLshort *);
#define CALL_MultiTexCoord1svARB(disp, parameters) \
    (* GET_MultiTexCoord1svARB(disp)) parameters
static inline _glptr_MultiTexCoord1svARB GET_MultiTexCoord1svARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord1svARB) (GET_by_offset(disp, _gloffset_MultiTexCoord1svARB));
}

static inline void SET_MultiTexCoord1svARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLshort *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord1svARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord2dARB)(GLenum, GLdouble, GLdouble);
#define CALL_MultiTexCoord2dARB(disp, parameters) \
    (* GET_MultiTexCoord2dARB(disp)) parameters
static inline _glptr_MultiTexCoord2dARB GET_MultiTexCoord2dARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord2dARB) (GET_by_offset(disp, _gloffset_MultiTexCoord2dARB));
}

static inline void SET_MultiTexCoord2dARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord2dARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord2dvARB)(GLenum, const GLdouble *);
#define CALL_MultiTexCoord2dvARB(disp, parameters) \
    (* GET_MultiTexCoord2dvARB(disp)) parameters
static inline _glptr_MultiTexCoord2dvARB GET_MultiTexCoord2dvARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord2dvARB) (GET_by_offset(disp, _gloffset_MultiTexCoord2dvARB));
}

static inline void SET_MultiTexCoord2dvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord2dvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord2fARB)(GLenum, GLfloat, GLfloat);
#define CALL_MultiTexCoord2fARB(disp, parameters) \
    (* GET_MultiTexCoord2fARB(disp)) parameters
static inline _glptr_MultiTexCoord2fARB GET_MultiTexCoord2fARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord2fARB) (GET_by_offset(disp, _gloffset_MultiTexCoord2fARB));
}

static inline void SET_MultiTexCoord2fARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord2fARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord2fvARB)(GLenum, const GLfloat *);
#define CALL_MultiTexCoord2fvARB(disp, parameters) \
    (* GET_MultiTexCoord2fvARB(disp)) parameters
static inline _glptr_MultiTexCoord2fvARB GET_MultiTexCoord2fvARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord2fvARB) (GET_by_offset(disp, _gloffset_MultiTexCoord2fvARB));
}

static inline void SET_MultiTexCoord2fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord2fvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord2iARB)(GLenum, GLint, GLint);
#define CALL_MultiTexCoord2iARB(disp, parameters) \
    (* GET_MultiTexCoord2iARB(disp)) parameters
static inline _glptr_MultiTexCoord2iARB GET_MultiTexCoord2iARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord2iARB) (GET_by_offset(disp, _gloffset_MultiTexCoord2iARB));
}

static inline void SET_MultiTexCoord2iARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord2iARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord2ivARB)(GLenum, const GLint *);
#define CALL_MultiTexCoord2ivARB(disp, parameters) \
    (* GET_MultiTexCoord2ivARB(disp)) parameters
static inline _glptr_MultiTexCoord2ivARB GET_MultiTexCoord2ivARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord2ivARB) (GET_by_offset(disp, _gloffset_MultiTexCoord2ivARB));
}

static inline void SET_MultiTexCoord2ivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord2ivARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord2sARB)(GLenum, GLshort, GLshort);
#define CALL_MultiTexCoord2sARB(disp, parameters) \
    (* GET_MultiTexCoord2sARB(disp)) parameters
static inline _glptr_MultiTexCoord2sARB GET_MultiTexCoord2sARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord2sARB) (GET_by_offset(disp, _gloffset_MultiTexCoord2sARB));
}

static inline void SET_MultiTexCoord2sARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord2sARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord2svARB)(GLenum, const GLshort *);
#define CALL_MultiTexCoord2svARB(disp, parameters) \
    (* GET_MultiTexCoord2svARB(disp)) parameters
static inline _glptr_MultiTexCoord2svARB GET_MultiTexCoord2svARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord2svARB) (GET_by_offset(disp, _gloffset_MultiTexCoord2svARB));
}

static inline void SET_MultiTexCoord2svARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLshort *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord2svARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord3dARB)(GLenum, GLdouble, GLdouble, GLdouble);
#define CALL_MultiTexCoord3dARB(disp, parameters) \
    (* GET_MultiTexCoord3dARB(disp)) parameters
static inline _glptr_MultiTexCoord3dARB GET_MultiTexCoord3dARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord3dARB) (GET_by_offset(disp, _gloffset_MultiTexCoord3dARB));
}

static inline void SET_MultiTexCoord3dARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord3dARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord3dvARB)(GLenum, const GLdouble *);
#define CALL_MultiTexCoord3dvARB(disp, parameters) \
    (* GET_MultiTexCoord3dvARB(disp)) parameters
static inline _glptr_MultiTexCoord3dvARB GET_MultiTexCoord3dvARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord3dvARB) (GET_by_offset(disp, _gloffset_MultiTexCoord3dvARB));
}

static inline void SET_MultiTexCoord3dvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord3dvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord3fARB)(GLenum, GLfloat, GLfloat, GLfloat);
#define CALL_MultiTexCoord3fARB(disp, parameters) \
    (* GET_MultiTexCoord3fARB(disp)) parameters
static inline _glptr_MultiTexCoord3fARB GET_MultiTexCoord3fARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord3fARB) (GET_by_offset(disp, _gloffset_MultiTexCoord3fARB));
}

static inline void SET_MultiTexCoord3fARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord3fARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord3fvARB)(GLenum, const GLfloat *);
#define CALL_MultiTexCoord3fvARB(disp, parameters) \
    (* GET_MultiTexCoord3fvARB(disp)) parameters
static inline _glptr_MultiTexCoord3fvARB GET_MultiTexCoord3fvARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord3fvARB) (GET_by_offset(disp, _gloffset_MultiTexCoord3fvARB));
}

static inline void SET_MultiTexCoord3fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord3fvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord3iARB)(GLenum, GLint, GLint, GLint);
#define CALL_MultiTexCoord3iARB(disp, parameters) \
    (* GET_MultiTexCoord3iARB(disp)) parameters
static inline _glptr_MultiTexCoord3iARB GET_MultiTexCoord3iARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord3iARB) (GET_by_offset(disp, _gloffset_MultiTexCoord3iARB));
}

static inline void SET_MultiTexCoord3iARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord3iARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord3ivARB)(GLenum, const GLint *);
#define CALL_MultiTexCoord3ivARB(disp, parameters) \
    (* GET_MultiTexCoord3ivARB(disp)) parameters
static inline _glptr_MultiTexCoord3ivARB GET_MultiTexCoord3ivARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord3ivARB) (GET_by_offset(disp, _gloffset_MultiTexCoord3ivARB));
}

static inline void SET_MultiTexCoord3ivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord3ivARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord3sARB)(GLenum, GLshort, GLshort, GLshort);
#define CALL_MultiTexCoord3sARB(disp, parameters) \
    (* GET_MultiTexCoord3sARB(disp)) parameters
static inline _glptr_MultiTexCoord3sARB GET_MultiTexCoord3sARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord3sARB) (GET_by_offset(disp, _gloffset_MultiTexCoord3sARB));
}

static inline void SET_MultiTexCoord3sARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord3sARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord3svARB)(GLenum, const GLshort *);
#define CALL_MultiTexCoord3svARB(disp, parameters) \
    (* GET_MultiTexCoord3svARB(disp)) parameters
static inline _glptr_MultiTexCoord3svARB GET_MultiTexCoord3svARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord3svARB) (GET_by_offset(disp, _gloffset_MultiTexCoord3svARB));
}

static inline void SET_MultiTexCoord3svARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLshort *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord3svARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord4dARB)(GLenum, GLdouble, GLdouble, GLdouble, GLdouble);
#define CALL_MultiTexCoord4dARB(disp, parameters) \
    (* GET_MultiTexCoord4dARB(disp)) parameters
static inline _glptr_MultiTexCoord4dARB GET_MultiTexCoord4dARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord4dARB) (GET_by_offset(disp, _gloffset_MultiTexCoord4dARB));
}

static inline void SET_MultiTexCoord4dARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord4dARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord4dvARB)(GLenum, const GLdouble *);
#define CALL_MultiTexCoord4dvARB(disp, parameters) \
    (* GET_MultiTexCoord4dvARB(disp)) parameters
static inline _glptr_MultiTexCoord4dvARB GET_MultiTexCoord4dvARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord4dvARB) (GET_by_offset(disp, _gloffset_MultiTexCoord4dvARB));
}

static inline void SET_MultiTexCoord4dvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord4dvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord4fARB)(GLenum, GLfloat, GLfloat, GLfloat, GLfloat);
#define CALL_MultiTexCoord4fARB(disp, parameters) \
    (* GET_MultiTexCoord4fARB(disp)) parameters
static inline _glptr_MultiTexCoord4fARB GET_MultiTexCoord4fARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord4fARB) (GET_by_offset(disp, _gloffset_MultiTexCoord4fARB));
}

static inline void SET_MultiTexCoord4fARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord4fARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord4fvARB)(GLenum, const GLfloat *);
#define CALL_MultiTexCoord4fvARB(disp, parameters) \
    (* GET_MultiTexCoord4fvARB(disp)) parameters
static inline _glptr_MultiTexCoord4fvARB GET_MultiTexCoord4fvARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord4fvARB) (GET_by_offset(disp, _gloffset_MultiTexCoord4fvARB));
}

static inline void SET_MultiTexCoord4fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord4fvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord4iARB)(GLenum, GLint, GLint, GLint, GLint);
#define CALL_MultiTexCoord4iARB(disp, parameters) \
    (* GET_MultiTexCoord4iARB(disp)) parameters
static inline _glptr_MultiTexCoord4iARB GET_MultiTexCoord4iARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord4iARB) (GET_by_offset(disp, _gloffset_MultiTexCoord4iARB));
}

static inline void SET_MultiTexCoord4iARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord4iARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord4ivARB)(GLenum, const GLint *);
#define CALL_MultiTexCoord4ivARB(disp, parameters) \
    (* GET_MultiTexCoord4ivARB(disp)) parameters
static inline _glptr_MultiTexCoord4ivARB GET_MultiTexCoord4ivARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord4ivARB) (GET_by_offset(disp, _gloffset_MultiTexCoord4ivARB));
}

static inline void SET_MultiTexCoord4ivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord4ivARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord4sARB)(GLenum, GLshort, GLshort, GLshort, GLshort);
#define CALL_MultiTexCoord4sARB(disp, parameters) \
    (* GET_MultiTexCoord4sARB(disp)) parameters
static inline _glptr_MultiTexCoord4sARB GET_MultiTexCoord4sARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord4sARB) (GET_by_offset(disp, _gloffset_MultiTexCoord4sARB));
}

static inline void SET_MultiTexCoord4sARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLshort, GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord4sARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoord4svARB)(GLenum, const GLshort *);
#define CALL_MultiTexCoord4svARB(disp, parameters) \
    (* GET_MultiTexCoord4svARB(disp)) parameters
static inline _glptr_MultiTexCoord4svARB GET_MultiTexCoord4svARB(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoord4svARB) (GET_by_offset(disp, _gloffset_MultiTexCoord4svARB));
}

static inline void SET_MultiTexCoord4svARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLshort *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord4svARB, fn);
}

typedef void (GLAPIENTRYP _glptr_AttachShader)(GLuint, GLuint);
#define CALL_AttachShader(disp, parameters) \
    (* GET_AttachShader(disp)) parameters
static inline _glptr_AttachShader GET_AttachShader(struct _glapi_table *disp) {
   return (_glptr_AttachShader) (GET_by_offset(disp, _gloffset_AttachShader));
}

static inline void SET_AttachShader(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_AttachShader, fn);
}

typedef GLuint (GLAPIENTRYP _glptr_CreateProgram)(void);
#define CALL_CreateProgram(disp, parameters) \
    (* GET_CreateProgram(disp)) parameters
static inline _glptr_CreateProgram GET_CreateProgram(struct _glapi_table *disp) {
   return (_glptr_CreateProgram) (GET_by_offset(disp, _gloffset_CreateProgram));
}

static inline void SET_CreateProgram(struct _glapi_table *disp, GLuint (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_CreateProgram, fn);
}

typedef GLuint (GLAPIENTRYP _glptr_CreateShader)(GLenum);
#define CALL_CreateShader(disp, parameters) \
    (* GET_CreateShader(disp)) parameters
static inline _glptr_CreateShader GET_CreateShader(struct _glapi_table *disp) {
   return (_glptr_CreateShader) (GET_by_offset(disp, _gloffset_CreateShader));
}

static inline void SET_CreateShader(struct _glapi_table *disp, GLuint (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_CreateShader, fn);
}

typedef void (GLAPIENTRYP _glptr_DeleteProgram)(GLuint);
#define CALL_DeleteProgram(disp, parameters) \
    (* GET_DeleteProgram(disp)) parameters
static inline _glptr_DeleteProgram GET_DeleteProgram(struct _glapi_table *disp) {
   return (_glptr_DeleteProgram) (GET_by_offset(disp, _gloffset_DeleteProgram));
}

static inline void SET_DeleteProgram(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_DeleteProgram, fn);
}

typedef void (GLAPIENTRYP _glptr_DeleteShader)(GLuint);
#define CALL_DeleteShader(disp, parameters) \
    (* GET_DeleteShader(disp)) parameters
static inline _glptr_DeleteShader GET_DeleteShader(struct _glapi_table *disp) {
   return (_glptr_DeleteShader) (GET_by_offset(disp, _gloffset_DeleteShader));
}

static inline void SET_DeleteShader(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_DeleteShader, fn);
}

typedef void (GLAPIENTRYP _glptr_DetachShader)(GLuint, GLuint);
#define CALL_DetachShader(disp, parameters) \
    (* GET_DetachShader(disp)) parameters
static inline _glptr_DetachShader GET_DetachShader(struct _glapi_table *disp) {
   return (_glptr_DetachShader) (GET_by_offset(disp, _gloffset_DetachShader));
}

static inline void SET_DetachShader(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_DetachShader, fn);
}

typedef void (GLAPIENTRYP _glptr_GetAttachedShaders)(GLuint, GLsizei, GLsizei *, GLuint *);
#define CALL_GetAttachedShaders(disp, parameters) \
    (* GET_GetAttachedShaders(disp)) parameters
static inline _glptr_GetAttachedShaders GET_GetAttachedShaders(struct _glapi_table *disp) {
   return (_glptr_GetAttachedShaders) (GET_by_offset(disp, _gloffset_GetAttachedShaders));
}

static inline void SET_GetAttachedShaders(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, GLsizei *, GLuint *)) {
   SET_by_offset(disp, _gloffset_GetAttachedShaders, fn);
}

typedef void (GLAPIENTRYP _glptr_GetProgramInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *);
#define CALL_GetProgramInfoLog(disp, parameters) \
    (* GET_GetProgramInfoLog(disp)) parameters
static inline _glptr_GetProgramInfoLog GET_GetProgramInfoLog(struct _glapi_table *disp) {
   return (_glptr_GetProgramInfoLog) (GET_by_offset(disp, _gloffset_GetProgramInfoLog));
}

static inline void SET_GetProgramInfoLog(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, GLsizei *, GLchar *)) {
   SET_by_offset(disp, _gloffset_GetProgramInfoLog, fn);
}

typedef void (GLAPIENTRYP _glptr_GetProgramiv)(GLuint, GLenum, GLint *);
#define CALL_GetProgramiv(disp, parameters) \
    (* GET_GetProgramiv(disp)) parameters
static inline _glptr_GetProgramiv GET_GetProgramiv(struct _glapi_table *disp) {
   return (_glptr_GetProgramiv) (GET_by_offset(disp, _gloffset_GetProgramiv));
}

static inline void SET_GetProgramiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetProgramiv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetShaderInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *);
#define CALL_GetShaderInfoLog(disp, parameters) \
    (* GET_GetShaderInfoLog(disp)) parameters
static inline _glptr_GetShaderInfoLog GET_GetShaderInfoLog(struct _glapi_table *disp) {
   return (_glptr_GetShaderInfoLog) (GET_by_offset(disp, _gloffset_GetShaderInfoLog));
}

static inline void SET_GetShaderInfoLog(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, GLsizei *, GLchar *)) {
   SET_by_offset(disp, _gloffset_GetShaderInfoLog, fn);
}

typedef void (GLAPIENTRYP _glptr_GetShaderiv)(GLuint, GLenum, GLint *);
#define CALL_GetShaderiv(disp, parameters) \
    (* GET_GetShaderiv(disp)) parameters
static inline _glptr_GetShaderiv GET_GetShaderiv(struct _glapi_table *disp) {
   return (_glptr_GetShaderiv) (GET_by_offset(disp, _gloffset_GetShaderiv));
}

static inline void SET_GetShaderiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetShaderiv, fn);
}

typedef GLboolean (GLAPIENTRYP _glptr_IsProgram)(GLuint);
#define CALL_IsProgram(disp, parameters) \
    (* GET_IsProgram(disp)) parameters
static inline _glptr_IsProgram GET_IsProgram(struct _glapi_table *disp) {
   return (_glptr_IsProgram) (GET_by_offset(disp, _gloffset_IsProgram));
}

static inline void SET_IsProgram(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_IsProgram, fn);
}

typedef GLboolean (GLAPIENTRYP _glptr_IsShader)(GLuint);
#define CALL_IsShader(disp, parameters) \
    (* GET_IsShader(disp)) parameters
static inline _glptr_IsShader GET_IsShader(struct _glapi_table *disp) {
   return (_glptr_IsShader) (GET_by_offset(disp, _gloffset_IsShader));
}

static inline void SET_IsShader(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_IsShader, fn);
}

typedef void (GLAPIENTRYP _glptr_StencilFuncSeparate)(GLenum, GLenum, GLint, GLuint);
#define CALL_StencilFuncSeparate(disp, parameters) \
    (* GET_StencilFuncSeparate(disp)) parameters
static inline _glptr_StencilFuncSeparate GET_StencilFuncSeparate(struct _glapi_table *disp) {
   return (_glptr_StencilFuncSeparate) (GET_by_offset(disp, _gloffset_StencilFuncSeparate));
}

static inline void SET_StencilFuncSeparate(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint, GLuint)) {
   SET_by_offset(disp, _gloffset_StencilFuncSeparate, fn);
}

typedef void (GLAPIENTRYP _glptr_StencilMaskSeparate)(GLenum, GLuint);
#define CALL_StencilMaskSeparate(disp, parameters) \
    (* GET_StencilMaskSeparate(disp)) parameters
static inline _glptr_StencilMaskSeparate GET_StencilMaskSeparate(struct _glapi_table *disp) {
   return (_glptr_StencilMaskSeparate) (GET_by_offset(disp, _gloffset_StencilMaskSeparate));
}

static inline void SET_StencilMaskSeparate(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_StencilMaskSeparate, fn);
}

typedef void (GLAPIENTRYP _glptr_StencilOpSeparate)(GLenum, GLenum, GLenum, GLenum);
#define CALL_StencilOpSeparate(disp, parameters) \
    (* GET_StencilOpSeparate(disp)) parameters
static inline _glptr_StencilOpSeparate GET_StencilOpSeparate(struct _glapi_table *disp) {
   return (_glptr_StencilOpSeparate) (GET_by_offset(disp, _gloffset_StencilOpSeparate));
}

static inline void SET_StencilOpSeparate(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_StencilOpSeparate, fn);
}

typedef void (GLAPIENTRYP _glptr_UniformMatrix2x3fv)(GLint, GLsizei, GLboolean, const GLfloat *);
#define CALL_UniformMatrix2x3fv(disp, parameters) \
    (* GET_UniformMatrix2x3fv(disp)) parameters
static inline _glptr_UniformMatrix2x3fv GET_UniformMatrix2x3fv(struct _glapi_table *disp) {
   return (_glptr_UniformMatrix2x3fv) (GET_by_offset(disp, _gloffset_UniformMatrix2x3fv));
}

static inline void SET_UniformMatrix2x3fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, GLboolean, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_UniformMatrix2x3fv, fn);
}

typedef void (GLAPIENTRYP _glptr_UniformMatrix2x4fv)(GLint, GLsizei, GLboolean, const GLfloat *);
#define CALL_UniformMatrix2x4fv(disp, parameters) \
    (* GET_UniformMatrix2x4fv(disp)) parameters
static inline _glptr_UniformMatrix2x4fv GET_UniformMatrix2x4fv(struct _glapi_table *disp) {
   return (_glptr_UniformMatrix2x4fv) (GET_by_offset(disp, _gloffset_UniformMatrix2x4fv));
}

static inline void SET_UniformMatrix2x4fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, GLboolean, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_UniformMatrix2x4fv, fn);
}

typedef void (GLAPIENTRYP _glptr_UniformMatrix3x2fv)(GLint, GLsizei, GLboolean, const GLfloat *);
#define CALL_UniformMatrix3x2fv(disp, parameters) \
    (* GET_UniformMatrix3x2fv(disp)) parameters
static inline _glptr_UniformMatrix3x2fv GET_UniformMatrix3x2fv(struct _glapi_table *disp) {
   return (_glptr_UniformMatrix3x2fv) (GET_by_offset(disp, _gloffset_UniformMatrix3x2fv));
}

static inline void SET_UniformMatrix3x2fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, GLboolean, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_UniformMatrix3x2fv, fn);
}

typedef void (GLAPIENTRYP _glptr_UniformMatrix3x4fv)(GLint, GLsizei, GLboolean, const GLfloat *);
#define CALL_UniformMatrix3x4fv(disp, parameters) \
    (* GET_UniformMatrix3x4fv(disp)) parameters
static inline _glptr_UniformMatrix3x4fv GET_UniformMatrix3x4fv(struct _glapi_table *disp) {
   return (_glptr_UniformMatrix3x4fv) (GET_by_offset(disp, _gloffset_UniformMatrix3x4fv));
}

static inline void SET_UniformMatrix3x4fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, GLboolean, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_UniformMatrix3x4fv, fn);
}

typedef void (GLAPIENTRYP _glptr_UniformMatrix4x2fv)(GLint, GLsizei, GLboolean, const GLfloat *);
#define CALL_UniformMatrix4x2fv(disp, parameters) \
    (* GET_UniformMatrix4x2fv(disp)) parameters
static inline _glptr_UniformMatrix4x2fv GET_UniformMatrix4x2fv(struct _glapi_table *disp) {
   return (_glptr_UniformMatrix4x2fv) (GET_by_offset(disp, _gloffset_UniformMatrix4x2fv));
}

static inline void SET_UniformMatrix4x2fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, GLboolean, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_UniformMatrix4x2fv, fn);
}

typedef void (GLAPIENTRYP _glptr_UniformMatrix4x3fv)(GLint, GLsizei, GLboolean, const GLfloat *);
#define CALL_UniformMatrix4x3fv(disp, parameters) \
    (* GET_UniformMatrix4x3fv(disp)) parameters
static inline _glptr_UniformMatrix4x3fv GET_UniformMatrix4x3fv(struct _glapi_table *disp) {
   return (_glptr_UniformMatrix4x3fv) (GET_by_offset(disp, _gloffset_UniformMatrix4x3fv));
}

static inline void SET_UniformMatrix4x3fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, GLboolean, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_UniformMatrix4x3fv, fn);
}

typedef void (GLAPIENTRYP _glptr_ClampColor)(GLenum, GLenum);
#define CALL_ClampColor(disp, parameters) \
    (* GET_ClampColor(disp)) parameters
static inline _glptr_ClampColor GET_ClampColor(struct _glapi_table *disp) {
   return (_glptr_ClampColor) (GET_by_offset(disp, _gloffset_ClampColor));
}

static inline void SET_ClampColor(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_ClampColor, fn);
}

typedef void (GLAPIENTRYP _glptr_ClearBufferfi)(GLenum, GLint, GLfloat, GLint);
#define CALL_ClearBufferfi(disp, parameters) \
    (* GET_ClearBufferfi(disp)) parameters
static inline _glptr_ClearBufferfi GET_ClearBufferfi(struct _glapi_table *disp) {
   return (_glptr_ClearBufferfi) (GET_by_offset(disp, _gloffset_ClearBufferfi));
}

static inline void SET_ClearBufferfi(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLfloat, GLint)) {
   SET_by_offset(disp, _gloffset_ClearBufferfi, fn);
}

typedef void (GLAPIENTRYP _glptr_ClearBufferfv)(GLenum, GLint, const GLfloat *);
#define CALL_ClearBufferfv(disp, parameters) \
    (* GET_ClearBufferfv(disp)) parameters
static inline _glptr_ClearBufferfv GET_ClearBufferfv(struct _glapi_table *disp) {
   return (_glptr_ClearBufferfv) (GET_by_offset(disp, _gloffset_ClearBufferfv));
}

static inline void SET_ClearBufferfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_ClearBufferfv, fn);
}

typedef void (GLAPIENTRYP _glptr_ClearBufferiv)(GLenum, GLint, const GLint *);
#define CALL_ClearBufferiv(disp, parameters) \
    (* GET_ClearBufferiv(disp)) parameters
static inline _glptr_ClearBufferiv GET_ClearBufferiv(struct _glapi_table *disp) {
   return (_glptr_ClearBufferiv) (GET_by_offset(disp, _gloffset_ClearBufferiv));
}

static inline void SET_ClearBufferiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, const GLint *)) {
   SET_by_offset(disp, _gloffset_ClearBufferiv, fn);
}

typedef void (GLAPIENTRYP _glptr_ClearBufferuiv)(GLenum, GLint, const GLuint *);
#define CALL_ClearBufferuiv(disp, parameters) \
    (* GET_ClearBufferuiv(disp)) parameters
static inline _glptr_ClearBufferuiv GET_ClearBufferuiv(struct _glapi_table *disp) {
   return (_glptr_ClearBufferuiv) (GET_by_offset(disp, _gloffset_ClearBufferuiv));
}

static inline void SET_ClearBufferuiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, const GLuint *)) {
   SET_by_offset(disp, _gloffset_ClearBufferuiv, fn);
}

typedef const GLubyte * (GLAPIENTRYP _glptr_GetStringi)(GLenum, GLuint);
#define CALL_GetStringi(disp, parameters) \
    (* GET_GetStringi(disp)) parameters
static inline _glptr_GetStringi GET_GetStringi(struct _glapi_table *disp) {
   return (_glptr_GetStringi) (GET_by_offset(disp, _gloffset_GetStringi));
}

static inline void SET_GetStringi(struct _glapi_table *disp, const GLubyte * (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_GetStringi, fn);
}

typedef void (GLAPIENTRYP _glptr_TexBuffer)(GLenum, GLenum, GLuint);
#define CALL_TexBuffer(disp, parameters) \
    (* GET_TexBuffer(disp)) parameters
static inline _glptr_TexBuffer GET_TexBuffer(struct _glapi_table *disp) {
   return (_glptr_TexBuffer) (GET_by_offset(disp, _gloffset_TexBuffer));
}

static inline void SET_TexBuffer(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_TexBuffer, fn);
}

typedef void (GLAPIENTRYP _glptr_FramebufferTexture)(GLenum, GLenum, GLuint, GLint);
#define CALL_FramebufferTexture(disp, parameters) \
    (* GET_FramebufferTexture(disp)) parameters
static inline _glptr_FramebufferTexture GET_FramebufferTexture(struct _glapi_table *disp) {
   return (_glptr_FramebufferTexture) (GET_by_offset(disp, _gloffset_FramebufferTexture));
}

static inline void SET_FramebufferTexture(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLuint, GLint)) {
   SET_by_offset(disp, _gloffset_FramebufferTexture, fn);
}

typedef void (GLAPIENTRYP _glptr_GetBufferParameteri64v)(GLenum, GLenum, GLint64 *);
#define CALL_GetBufferParameteri64v(disp, parameters) \
    (* GET_GetBufferParameteri64v(disp)) parameters
static inline _glptr_GetBufferParameteri64v GET_GetBufferParameteri64v(struct _glapi_table *disp) {
   return (_glptr_GetBufferParameteri64v) (GET_by_offset(disp, _gloffset_GetBufferParameteri64v));
}

static inline void SET_GetBufferParameteri64v(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint64 *)) {
   SET_by_offset(disp, _gloffset_GetBufferParameteri64v, fn);
}

typedef void (GLAPIENTRYP _glptr_GetInteger64i_v)(GLenum, GLuint, GLint64 *);
#define CALL_GetInteger64i_v(disp, parameters) \
    (* GET_GetInteger64i_v(disp)) parameters
static inline _glptr_GetInteger64i_v GET_GetInteger64i_v(struct _glapi_table *disp) {
   return (_glptr_GetInteger64i_v) (GET_by_offset(disp, _gloffset_GetInteger64i_v));
}

static inline void SET_GetInteger64i_v(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLint64 *)) {
   SET_by_offset(disp, _gloffset_GetInteger64i_v, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribDivisor)(GLuint, GLuint);
#define CALL_VertexAttribDivisor(disp, parameters) \
    (* GET_VertexAttribDivisor(disp)) parameters
static inline _glptr_VertexAttribDivisor GET_VertexAttribDivisor(struct _glapi_table *disp) {
   return (_glptr_VertexAttribDivisor) (GET_by_offset(disp, _gloffset_VertexAttribDivisor));
}

static inline void SET_VertexAttribDivisor(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_VertexAttribDivisor, fn);
}

typedef void (GLAPIENTRYP _glptr_LoadTransposeMatrixdARB)(const GLdouble *);
#define CALL_LoadTransposeMatrixdARB(disp, parameters) \
    (* GET_LoadTransposeMatrixdARB(disp)) parameters
static inline _glptr_LoadTransposeMatrixdARB GET_LoadTransposeMatrixdARB(struct _glapi_table *disp) {
   return (_glptr_LoadTransposeMatrixdARB) (GET_by_offset(disp, _gloffset_LoadTransposeMatrixdARB));
}

static inline void SET_LoadTransposeMatrixdARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_LoadTransposeMatrixdARB, fn);
}

typedef void (GLAPIENTRYP _glptr_LoadTransposeMatrixfARB)(const GLfloat *);
#define CALL_LoadTransposeMatrixfARB(disp, parameters) \
    (* GET_LoadTransposeMatrixfARB(disp)) parameters
static inline _glptr_LoadTransposeMatrixfARB GET_LoadTransposeMatrixfARB(struct _glapi_table *disp) {
   return (_glptr_LoadTransposeMatrixfARB) (GET_by_offset(disp, _gloffset_LoadTransposeMatrixfARB));
}

static inline void SET_LoadTransposeMatrixfARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_LoadTransposeMatrixfARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultTransposeMatrixdARB)(const GLdouble *);
#define CALL_MultTransposeMatrixdARB(disp, parameters) \
    (* GET_MultTransposeMatrixdARB(disp)) parameters
static inline _glptr_MultTransposeMatrixdARB GET_MultTransposeMatrixdARB(struct _glapi_table *disp) {
   return (_glptr_MultTransposeMatrixdARB) (GET_by_offset(disp, _gloffset_MultTransposeMatrixdARB));
}

static inline void SET_MultTransposeMatrixdARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_MultTransposeMatrixdARB, fn);
}

typedef void (GLAPIENTRYP _glptr_MultTransposeMatrixfARB)(const GLfloat *);
#define CALL_MultTransposeMatrixfARB(disp, parameters) \
    (* GET_MultTransposeMatrixfARB(disp)) parameters
static inline _glptr_MultTransposeMatrixfARB GET_MultTransposeMatrixfARB(struct _glapi_table *disp) {
   return (_glptr_MultTransposeMatrixfARB) (GET_by_offset(disp, _gloffset_MultTransposeMatrixfARB));
}

static inline void SET_MultTransposeMatrixfARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_MultTransposeMatrixfARB, fn);
}

typedef void (GLAPIENTRYP _glptr_SampleCoverageARB)(GLclampf, GLboolean);
#define CALL_SampleCoverageARB(disp, parameters) \
    (* GET_SampleCoverageARB(disp)) parameters
static inline _glptr_SampleCoverageARB GET_SampleCoverageARB(struct _glapi_table *disp) {
   return (_glptr_SampleCoverageARB) (GET_by_offset(disp, _gloffset_SampleCoverageARB));
}

static inline void SET_SampleCoverageARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLclampf, GLboolean)) {
   SET_by_offset(disp, _gloffset_SampleCoverageARB, fn);
}

typedef void (GLAPIENTRYP _glptr_CompressedTexImage1DARB)(GLenum, GLint, GLenum, GLsizei, GLint, GLsizei, const GLvoid *);
#define CALL_CompressedTexImage1DARB(disp, parameters) \
    (* GET_CompressedTexImage1DARB(disp)) parameters
static inline _glptr_CompressedTexImage1DARB GET_CompressedTexImage1DARB(struct _glapi_table *disp) {
   return (_glptr_CompressedTexImage1DARB) (GET_by_offset(disp, _gloffset_CompressedTexImage1DARB));
}

static inline void SET_CompressedTexImage1DARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLenum, GLsizei, GLint, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_CompressedTexImage1DARB, fn);
}

typedef void (GLAPIENTRYP _glptr_CompressedTexImage2DARB)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *);
#define CALL_CompressedTexImage2DARB(disp, parameters) \
    (* GET_CompressedTexImage2DARB(disp)) parameters
static inline _glptr_CompressedTexImage2DARB GET_CompressedTexImage2DARB(struct _glapi_table *disp) {
   return (_glptr_CompressedTexImage2DARB) (GET_by_offset(disp, _gloffset_CompressedTexImage2DARB));
}

static inline void SET_CompressedTexImage2DARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_CompressedTexImage2DARB, fn);
}

typedef void (GLAPIENTRYP _glptr_CompressedTexImage3DARB)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *);
#define CALL_CompressedTexImage3DARB(disp, parameters) \
    (* GET_CompressedTexImage3DARB(disp)) parameters
static inline _glptr_CompressedTexImage3DARB GET_CompressedTexImage3DARB(struct _glapi_table *disp) {
   return (_glptr_CompressedTexImage3DARB) (GET_by_offset(disp, _gloffset_CompressedTexImage3DARB));
}

static inline void SET_CompressedTexImage3DARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_CompressedTexImage3DARB, fn);
}

typedef void (GLAPIENTRYP _glptr_CompressedTexSubImage1DARB)(GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, const GLvoid *);
#define CALL_CompressedTexSubImage1DARB(disp, parameters) \
    (* GET_CompressedTexSubImage1DARB(disp)) parameters
static inline _glptr_CompressedTexSubImage1DARB GET_CompressedTexSubImage1DARB(struct _glapi_table *disp) {
   return (_glptr_CompressedTexSubImage1DARB) (GET_by_offset(disp, _gloffset_CompressedTexSubImage1DARB));
}

static inline void SET_CompressedTexSubImage1DARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_CompressedTexSubImage1DARB, fn);
}

typedef void (GLAPIENTRYP _glptr_CompressedTexSubImage2DARB)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid *);
#define CALL_CompressedTexSubImage2DARB(disp, parameters) \
    (* GET_CompressedTexSubImage2DARB(disp)) parameters
static inline _glptr_CompressedTexSubImage2DARB GET_CompressedTexSubImage2DARB(struct _glapi_table *disp) {
   return (_glptr_CompressedTexSubImage2DARB) (GET_by_offset(disp, _gloffset_CompressedTexSubImage2DARB));
}

static inline void SET_CompressedTexSubImage2DARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_CompressedTexSubImage2DARB, fn);
}

typedef void (GLAPIENTRYP _glptr_CompressedTexSubImage3DARB)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid *);
#define CALL_CompressedTexSubImage3DARB(disp, parameters) \
    (* GET_CompressedTexSubImage3DARB(disp)) parameters
static inline _glptr_CompressedTexSubImage3DARB GET_CompressedTexSubImage3DARB(struct _glapi_table *disp) {
   return (_glptr_CompressedTexSubImage3DARB) (GET_by_offset(disp, _gloffset_CompressedTexSubImage3DARB));
}

static inline void SET_CompressedTexSubImage3DARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_CompressedTexSubImage3DARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetCompressedTexImageARB)(GLenum, GLint, GLvoid *);
#define CALL_GetCompressedTexImageARB(disp, parameters) \
    (* GET_GetCompressedTexImageARB(disp)) parameters
static inline _glptr_GetCompressedTexImageARB GET_GetCompressedTexImageARB(struct _glapi_table *disp) {
   return (_glptr_GetCompressedTexImageARB) (GET_by_offset(disp, _gloffset_GetCompressedTexImageARB));
}

static inline void SET_GetCompressedTexImageARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetCompressedTexImageARB, fn);
}

typedef void (GLAPIENTRYP _glptr_DisableVertexAttribArrayARB)(GLuint);
#define CALL_DisableVertexAttribArrayARB(disp, parameters) \
    (* GET_DisableVertexAttribArrayARB(disp)) parameters
static inline _glptr_DisableVertexAttribArrayARB GET_DisableVertexAttribArrayARB(struct _glapi_table *disp) {
   return (_glptr_DisableVertexAttribArrayARB) (GET_by_offset(disp, _gloffset_DisableVertexAttribArrayARB));
}

static inline void SET_DisableVertexAttribArrayARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_DisableVertexAttribArrayARB, fn);
}

typedef void (GLAPIENTRYP _glptr_EnableVertexAttribArrayARB)(GLuint);
#define CALL_EnableVertexAttribArrayARB(disp, parameters) \
    (* GET_EnableVertexAttribArrayARB(disp)) parameters
static inline _glptr_EnableVertexAttribArrayARB GET_EnableVertexAttribArrayARB(struct _glapi_table *disp) {
   return (_glptr_EnableVertexAttribArrayARB) (GET_by_offset(disp, _gloffset_EnableVertexAttribArrayARB));
}

static inline void SET_EnableVertexAttribArrayARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_EnableVertexAttribArrayARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetProgramEnvParameterdvARB)(GLenum, GLuint, GLdouble *);
#define CALL_GetProgramEnvParameterdvARB(disp, parameters) \
    (* GET_GetProgramEnvParameterdvARB(disp)) parameters
static inline _glptr_GetProgramEnvParameterdvARB GET_GetProgramEnvParameterdvARB(struct _glapi_table *disp) {
   return (_glptr_GetProgramEnvParameterdvARB) (GET_by_offset(disp, _gloffset_GetProgramEnvParameterdvARB));
}

static inline void SET_GetProgramEnvParameterdvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLdouble *)) {
   SET_by_offset(disp, _gloffset_GetProgramEnvParameterdvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetProgramEnvParameterfvARB)(GLenum, GLuint, GLfloat *);
#define CALL_GetProgramEnvParameterfvARB(disp, parameters) \
    (* GET_GetProgramEnvParameterfvARB(disp)) parameters
static inline _glptr_GetProgramEnvParameterfvARB GET_GetProgramEnvParameterfvARB(struct _glapi_table *disp) {
   return (_glptr_GetProgramEnvParameterfvARB) (GET_by_offset(disp, _gloffset_GetProgramEnvParameterfvARB));
}

static inline void SET_GetProgramEnvParameterfvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetProgramEnvParameterfvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetProgramLocalParameterdvARB)(GLenum, GLuint, GLdouble *);
#define CALL_GetProgramLocalParameterdvARB(disp, parameters) \
    (* GET_GetProgramLocalParameterdvARB(disp)) parameters
static inline _glptr_GetProgramLocalParameterdvARB GET_GetProgramLocalParameterdvARB(struct _glapi_table *disp) {
   return (_glptr_GetProgramLocalParameterdvARB) (GET_by_offset(disp, _gloffset_GetProgramLocalParameterdvARB));
}

static inline void SET_GetProgramLocalParameterdvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLdouble *)) {
   SET_by_offset(disp, _gloffset_GetProgramLocalParameterdvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetProgramLocalParameterfvARB)(GLenum, GLuint, GLfloat *);
#define CALL_GetProgramLocalParameterfvARB(disp, parameters) \
    (* GET_GetProgramLocalParameterfvARB(disp)) parameters
static inline _glptr_GetProgramLocalParameterfvARB GET_GetProgramLocalParameterfvARB(struct _glapi_table *disp) {
   return (_glptr_GetProgramLocalParameterfvARB) (GET_by_offset(disp, _gloffset_GetProgramLocalParameterfvARB));
}

static inline void SET_GetProgramLocalParameterfvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetProgramLocalParameterfvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetProgramStringARB)(GLenum, GLenum, GLvoid *);
#define CALL_GetProgramStringARB(disp, parameters) \
    (* GET_GetProgramStringARB(disp)) parameters
static inline _glptr_GetProgramStringARB GET_GetProgramStringARB(struct _glapi_table *disp) {
   return (_glptr_GetProgramStringARB) (GET_by_offset(disp, _gloffset_GetProgramStringARB));
}

static inline void SET_GetProgramStringARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetProgramStringARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetProgramivARB)(GLenum, GLenum, GLint *);
#define CALL_GetProgramivARB(disp, parameters) \
    (* GET_GetProgramivARB(disp)) parameters
static inline _glptr_GetProgramivARB GET_GetProgramivARB(struct _glapi_table *disp) {
   return (_glptr_GetProgramivARB) (GET_by_offset(disp, _gloffset_GetProgramivARB));
}

static inline void SET_GetProgramivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetProgramivARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetVertexAttribdvARB)(GLuint, GLenum, GLdouble *);
#define CALL_GetVertexAttribdvARB(disp, parameters) \
    (* GET_GetVertexAttribdvARB(disp)) parameters
static inline _glptr_GetVertexAttribdvARB GET_GetVertexAttribdvARB(struct _glapi_table *disp) {
   return (_glptr_GetVertexAttribdvARB) (GET_by_offset(disp, _gloffset_GetVertexAttribdvARB));
}

static inline void SET_GetVertexAttribdvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLdouble *)) {
   SET_by_offset(disp, _gloffset_GetVertexAttribdvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetVertexAttribfvARB)(GLuint, GLenum, GLfloat *);
#define CALL_GetVertexAttribfvARB(disp, parameters) \
    (* GET_GetVertexAttribfvARB(disp)) parameters
static inline _glptr_GetVertexAttribfvARB GET_GetVertexAttribfvARB(struct _glapi_table *disp) {
   return (_glptr_GetVertexAttribfvARB) (GET_by_offset(disp, _gloffset_GetVertexAttribfvARB));
}

static inline void SET_GetVertexAttribfvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetVertexAttribfvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetVertexAttribivARB)(GLuint, GLenum, GLint *);
#define CALL_GetVertexAttribivARB(disp, parameters) \
    (* GET_GetVertexAttribivARB(disp)) parameters
static inline _glptr_GetVertexAttribivARB GET_GetVertexAttribivARB(struct _glapi_table *disp) {
   return (_glptr_GetVertexAttribivARB) (GET_by_offset(disp, _gloffset_GetVertexAttribivARB));
}

static inline void SET_GetVertexAttribivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetVertexAttribivARB, fn);
}

typedef void (GLAPIENTRYP _glptr_ProgramEnvParameter4dARB)(GLenum, GLuint, GLdouble, GLdouble, GLdouble, GLdouble);
#define CALL_ProgramEnvParameter4dARB(disp, parameters) \
    (* GET_ProgramEnvParameter4dARB(disp)) parameters
static inline _glptr_ProgramEnvParameter4dARB GET_ProgramEnvParameter4dARB(struct _glapi_table *disp) {
   return (_glptr_ProgramEnvParameter4dARB) (GET_by_offset(disp, _gloffset_ProgramEnvParameter4dARB));
}

static inline void SET_ProgramEnvParameter4dARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_ProgramEnvParameter4dARB, fn);
}

typedef void (GLAPIENTRYP _glptr_ProgramEnvParameter4dvARB)(GLenum, GLuint, const GLdouble *);
#define CALL_ProgramEnvParameter4dvARB(disp, parameters) \
    (* GET_ProgramEnvParameter4dvARB(disp)) parameters
static inline _glptr_ProgramEnvParameter4dvARB GET_ProgramEnvParameter4dvARB(struct _glapi_table *disp) {
   return (_glptr_ProgramEnvParameter4dvARB) (GET_by_offset(disp, _gloffset_ProgramEnvParameter4dvARB));
}

static inline void SET_ProgramEnvParameter4dvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_ProgramEnvParameter4dvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_ProgramEnvParameter4fARB)(GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat);
#define CALL_ProgramEnvParameter4fARB(disp, parameters) \
    (* GET_ProgramEnvParameter4fARB(disp)) parameters
static inline _glptr_ProgramEnvParameter4fARB GET_ProgramEnvParameter4fARB(struct _glapi_table *disp) {
   return (_glptr_ProgramEnvParameter4fARB) (GET_by_offset(disp, _gloffset_ProgramEnvParameter4fARB));
}

static inline void SET_ProgramEnvParameter4fARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_ProgramEnvParameter4fARB, fn);
}

typedef void (GLAPIENTRYP _glptr_ProgramEnvParameter4fvARB)(GLenum, GLuint, const GLfloat *);
#define CALL_ProgramEnvParameter4fvARB(disp, parameters) \
    (* GET_ProgramEnvParameter4fvARB(disp)) parameters
static inline _glptr_ProgramEnvParameter4fvARB GET_ProgramEnvParameter4fvARB(struct _glapi_table *disp) {
   return (_glptr_ProgramEnvParameter4fvARB) (GET_by_offset(disp, _gloffset_ProgramEnvParameter4fvARB));
}

static inline void SET_ProgramEnvParameter4fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_ProgramEnvParameter4fvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_ProgramLocalParameter4dARB)(GLenum, GLuint, GLdouble, GLdouble, GLdouble, GLdouble);
#define CALL_ProgramLocalParameter4dARB(disp, parameters) \
    (* GET_ProgramLocalParameter4dARB(disp)) parameters
static inline _glptr_ProgramLocalParameter4dARB GET_ProgramLocalParameter4dARB(struct _glapi_table *disp) {
   return (_glptr_ProgramLocalParameter4dARB) (GET_by_offset(disp, _gloffset_ProgramLocalParameter4dARB));
}

static inline void SET_ProgramLocalParameter4dARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_ProgramLocalParameter4dARB, fn);
}

typedef void (GLAPIENTRYP _glptr_ProgramLocalParameter4dvARB)(GLenum, GLuint, const GLdouble *);
#define CALL_ProgramLocalParameter4dvARB(disp, parameters) \
    (* GET_ProgramLocalParameter4dvARB(disp)) parameters
static inline _glptr_ProgramLocalParameter4dvARB GET_ProgramLocalParameter4dvARB(struct _glapi_table *disp) {
   return (_glptr_ProgramLocalParameter4dvARB) (GET_by_offset(disp, _gloffset_ProgramLocalParameter4dvARB));
}

static inline void SET_ProgramLocalParameter4dvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_ProgramLocalParameter4dvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_ProgramLocalParameter4fARB)(GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat);
#define CALL_ProgramLocalParameter4fARB(disp, parameters) \
    (* GET_ProgramLocalParameter4fARB(disp)) parameters
static inline _glptr_ProgramLocalParameter4fARB GET_ProgramLocalParameter4fARB(struct _glapi_table *disp) {
   return (_glptr_ProgramLocalParameter4fARB) (GET_by_offset(disp, _gloffset_ProgramLocalParameter4fARB));
}

static inline void SET_ProgramLocalParameter4fARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_ProgramLocalParameter4fARB, fn);
}

typedef void (GLAPIENTRYP _glptr_ProgramLocalParameter4fvARB)(GLenum, GLuint, const GLfloat *);
#define CALL_ProgramLocalParameter4fvARB(disp, parameters) \
    (* GET_ProgramLocalParameter4fvARB(disp)) parameters
static inline _glptr_ProgramLocalParameter4fvARB GET_ProgramLocalParameter4fvARB(struct _glapi_table *disp) {
   return (_glptr_ProgramLocalParameter4fvARB) (GET_by_offset(disp, _gloffset_ProgramLocalParameter4fvARB));
}

static inline void SET_ProgramLocalParameter4fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_ProgramLocalParameter4fvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_ProgramStringARB)(GLenum, GLenum, GLsizei, const GLvoid *);
#define CALL_ProgramStringARB(disp, parameters) \
    (* GET_ProgramStringARB(disp)) parameters
static inline _glptr_ProgramStringARB GET_ProgramStringARB(struct _glapi_table *disp) {
   return (_glptr_ProgramStringARB) (GET_by_offset(disp, _gloffset_ProgramStringARB));
}

static inline void SET_ProgramStringARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_ProgramStringARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib1dARB)(GLuint, GLdouble);
#define CALL_VertexAttrib1dARB(disp, parameters) \
    (* GET_VertexAttrib1dARB(disp)) parameters
static inline _glptr_VertexAttrib1dARB GET_VertexAttrib1dARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib1dARB) (GET_by_offset(disp, _gloffset_VertexAttrib1dARB));
}

static inline void SET_VertexAttrib1dARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLdouble)) {
   SET_by_offset(disp, _gloffset_VertexAttrib1dARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib1dvARB)(GLuint, const GLdouble *);
#define CALL_VertexAttrib1dvARB(disp, parameters) \
    (* GET_VertexAttrib1dvARB(disp)) parameters
static inline _glptr_VertexAttrib1dvARB GET_VertexAttrib1dvARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib1dvARB) (GET_by_offset(disp, _gloffset_VertexAttrib1dvARB));
}

static inline void SET_VertexAttrib1dvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib1dvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib1fARB)(GLuint, GLfloat);
#define CALL_VertexAttrib1fARB(disp, parameters) \
    (* GET_VertexAttrib1fARB(disp)) parameters
static inline _glptr_VertexAttrib1fARB GET_VertexAttrib1fARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib1fARB) (GET_by_offset(disp, _gloffset_VertexAttrib1fARB));
}

static inline void SET_VertexAttrib1fARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLfloat)) {
   SET_by_offset(disp, _gloffset_VertexAttrib1fARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib1fvARB)(GLuint, const GLfloat *);
#define CALL_VertexAttrib1fvARB(disp, parameters) \
    (* GET_VertexAttrib1fvARB(disp)) parameters
static inline _glptr_VertexAttrib1fvARB GET_VertexAttrib1fvARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib1fvARB) (GET_by_offset(disp, _gloffset_VertexAttrib1fvARB));
}

static inline void SET_VertexAttrib1fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib1fvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib1sARB)(GLuint, GLshort);
#define CALL_VertexAttrib1sARB(disp, parameters) \
    (* GET_VertexAttrib1sARB(disp)) parameters
static inline _glptr_VertexAttrib1sARB GET_VertexAttrib1sARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib1sARB) (GET_by_offset(disp, _gloffset_VertexAttrib1sARB));
}

static inline void SET_VertexAttrib1sARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLshort)) {
   SET_by_offset(disp, _gloffset_VertexAttrib1sARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib1svARB)(GLuint, const GLshort *);
#define CALL_VertexAttrib1svARB(disp, parameters) \
    (* GET_VertexAttrib1svARB(disp)) parameters
static inline _glptr_VertexAttrib1svARB GET_VertexAttrib1svARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib1svARB) (GET_by_offset(disp, _gloffset_VertexAttrib1svARB));
}

static inline void SET_VertexAttrib1svARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLshort *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib1svARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib2dARB)(GLuint, GLdouble, GLdouble);
#define CALL_VertexAttrib2dARB(disp, parameters) \
    (* GET_VertexAttrib2dARB(disp)) parameters
static inline _glptr_VertexAttrib2dARB GET_VertexAttrib2dARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib2dARB) (GET_by_offset(disp, _gloffset_VertexAttrib2dARB));
}

static inline void SET_VertexAttrib2dARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_VertexAttrib2dARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib2dvARB)(GLuint, const GLdouble *);
#define CALL_VertexAttrib2dvARB(disp, parameters) \
    (* GET_VertexAttrib2dvARB(disp)) parameters
static inline _glptr_VertexAttrib2dvARB GET_VertexAttrib2dvARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib2dvARB) (GET_by_offset(disp, _gloffset_VertexAttrib2dvARB));
}

static inline void SET_VertexAttrib2dvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib2dvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib2fARB)(GLuint, GLfloat, GLfloat);
#define CALL_VertexAttrib2fARB(disp, parameters) \
    (* GET_VertexAttrib2fARB(disp)) parameters
static inline _glptr_VertexAttrib2fARB GET_VertexAttrib2fARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib2fARB) (GET_by_offset(disp, _gloffset_VertexAttrib2fARB));
}

static inline void SET_VertexAttrib2fARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_VertexAttrib2fARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib2fvARB)(GLuint, const GLfloat *);
#define CALL_VertexAttrib2fvARB(disp, parameters) \
    (* GET_VertexAttrib2fvARB(disp)) parameters
static inline _glptr_VertexAttrib2fvARB GET_VertexAttrib2fvARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib2fvARB) (GET_by_offset(disp, _gloffset_VertexAttrib2fvARB));
}

static inline void SET_VertexAttrib2fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib2fvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib2sARB)(GLuint, GLshort, GLshort);
#define CALL_VertexAttrib2sARB(disp, parameters) \
    (* GET_VertexAttrib2sARB(disp)) parameters
static inline _glptr_VertexAttrib2sARB GET_VertexAttrib2sARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib2sARB) (GET_by_offset(disp, _gloffset_VertexAttrib2sARB));
}

static inline void SET_VertexAttrib2sARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_VertexAttrib2sARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib2svARB)(GLuint, const GLshort *);
#define CALL_VertexAttrib2svARB(disp, parameters) \
    (* GET_VertexAttrib2svARB(disp)) parameters
static inline _glptr_VertexAttrib2svARB GET_VertexAttrib2svARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib2svARB) (GET_by_offset(disp, _gloffset_VertexAttrib2svARB));
}

static inline void SET_VertexAttrib2svARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLshort *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib2svARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib3dARB)(GLuint, GLdouble, GLdouble, GLdouble);
#define CALL_VertexAttrib3dARB(disp, parameters) \
    (* GET_VertexAttrib3dARB(disp)) parameters
static inline _glptr_VertexAttrib3dARB GET_VertexAttrib3dARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib3dARB) (GET_by_offset(disp, _gloffset_VertexAttrib3dARB));
}

static inline void SET_VertexAttrib3dARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_VertexAttrib3dARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib3dvARB)(GLuint, const GLdouble *);
#define CALL_VertexAttrib3dvARB(disp, parameters) \
    (* GET_VertexAttrib3dvARB(disp)) parameters
static inline _glptr_VertexAttrib3dvARB GET_VertexAttrib3dvARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib3dvARB) (GET_by_offset(disp, _gloffset_VertexAttrib3dvARB));
}

static inline void SET_VertexAttrib3dvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib3dvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib3fARB)(GLuint, GLfloat, GLfloat, GLfloat);
#define CALL_VertexAttrib3fARB(disp, parameters) \
    (* GET_VertexAttrib3fARB(disp)) parameters
static inline _glptr_VertexAttrib3fARB GET_VertexAttrib3fARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib3fARB) (GET_by_offset(disp, _gloffset_VertexAttrib3fARB));
}

static inline void SET_VertexAttrib3fARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_VertexAttrib3fARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib3fvARB)(GLuint, const GLfloat *);
#define CALL_VertexAttrib3fvARB(disp, parameters) \
    (* GET_VertexAttrib3fvARB(disp)) parameters
static inline _glptr_VertexAttrib3fvARB GET_VertexAttrib3fvARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib3fvARB) (GET_by_offset(disp, _gloffset_VertexAttrib3fvARB));
}

static inline void SET_VertexAttrib3fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib3fvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib3sARB)(GLuint, GLshort, GLshort, GLshort);
#define CALL_VertexAttrib3sARB(disp, parameters) \
    (* GET_VertexAttrib3sARB(disp)) parameters
static inline _glptr_VertexAttrib3sARB GET_VertexAttrib3sARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib3sARB) (GET_by_offset(disp, _gloffset_VertexAttrib3sARB));
}

static inline void SET_VertexAttrib3sARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_VertexAttrib3sARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib3svARB)(GLuint, const GLshort *);
#define CALL_VertexAttrib3svARB(disp, parameters) \
    (* GET_VertexAttrib3svARB(disp)) parameters
static inline _glptr_VertexAttrib3svARB GET_VertexAttrib3svARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib3svARB) (GET_by_offset(disp, _gloffset_VertexAttrib3svARB));
}

static inline void SET_VertexAttrib3svARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLshort *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib3svARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib4NbvARB)(GLuint, const GLbyte *);
#define CALL_VertexAttrib4NbvARB(disp, parameters) \
    (* GET_VertexAttrib4NbvARB(disp)) parameters
static inline _glptr_VertexAttrib4NbvARB GET_VertexAttrib4NbvARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib4NbvARB) (GET_by_offset(disp, _gloffset_VertexAttrib4NbvARB));
}

static inline void SET_VertexAttrib4NbvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLbyte *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4NbvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib4NivARB)(GLuint, const GLint *);
#define CALL_VertexAttrib4NivARB(disp, parameters) \
    (* GET_VertexAttrib4NivARB(disp)) parameters
static inline _glptr_VertexAttrib4NivARB GET_VertexAttrib4NivARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib4NivARB) (GET_by_offset(disp, _gloffset_VertexAttrib4NivARB));
}

static inline void SET_VertexAttrib4NivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLint *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4NivARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib4NsvARB)(GLuint, const GLshort *);
#define CALL_VertexAttrib4NsvARB(disp, parameters) \
    (* GET_VertexAttrib4NsvARB(disp)) parameters
static inline _glptr_VertexAttrib4NsvARB GET_VertexAttrib4NsvARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib4NsvARB) (GET_by_offset(disp, _gloffset_VertexAttrib4NsvARB));
}

static inline void SET_VertexAttrib4NsvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLshort *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4NsvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib4NubARB)(GLuint, GLubyte, GLubyte, GLubyte, GLubyte);
#define CALL_VertexAttrib4NubARB(disp, parameters) \
    (* GET_VertexAttrib4NubARB(disp)) parameters
static inline _glptr_VertexAttrib4NubARB GET_VertexAttrib4NubARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib4NubARB) (GET_by_offset(disp, _gloffset_VertexAttrib4NubARB));
}

static inline void SET_VertexAttrib4NubARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLubyte, GLubyte, GLubyte, GLubyte)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4NubARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib4NubvARB)(GLuint, const GLubyte *);
#define CALL_VertexAttrib4NubvARB(disp, parameters) \
    (* GET_VertexAttrib4NubvARB(disp)) parameters
static inline _glptr_VertexAttrib4NubvARB GET_VertexAttrib4NubvARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib4NubvARB) (GET_by_offset(disp, _gloffset_VertexAttrib4NubvARB));
}

static inline void SET_VertexAttrib4NubvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLubyte *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4NubvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib4NuivARB)(GLuint, const GLuint *);
#define CALL_VertexAttrib4NuivARB(disp, parameters) \
    (* GET_VertexAttrib4NuivARB(disp)) parameters
static inline _glptr_VertexAttrib4NuivARB GET_VertexAttrib4NuivARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib4NuivARB) (GET_by_offset(disp, _gloffset_VertexAttrib4NuivARB));
}

static inline void SET_VertexAttrib4NuivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLuint *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4NuivARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib4NusvARB)(GLuint, const GLushort *);
#define CALL_VertexAttrib4NusvARB(disp, parameters) \
    (* GET_VertexAttrib4NusvARB(disp)) parameters
static inline _glptr_VertexAttrib4NusvARB GET_VertexAttrib4NusvARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib4NusvARB) (GET_by_offset(disp, _gloffset_VertexAttrib4NusvARB));
}

static inline void SET_VertexAttrib4NusvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLushort *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4NusvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib4bvARB)(GLuint, const GLbyte *);
#define CALL_VertexAttrib4bvARB(disp, parameters) \
    (* GET_VertexAttrib4bvARB(disp)) parameters
static inline _glptr_VertexAttrib4bvARB GET_VertexAttrib4bvARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib4bvARB) (GET_by_offset(disp, _gloffset_VertexAttrib4bvARB));
}

static inline void SET_VertexAttrib4bvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLbyte *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4bvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib4dARB)(GLuint, GLdouble, GLdouble, GLdouble, GLdouble);
#define CALL_VertexAttrib4dARB(disp, parameters) \
    (* GET_VertexAttrib4dARB(disp)) parameters
static inline _glptr_VertexAttrib4dARB GET_VertexAttrib4dARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib4dARB) (GET_by_offset(disp, _gloffset_VertexAttrib4dARB));
}

static inline void SET_VertexAttrib4dARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4dARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib4dvARB)(GLuint, const GLdouble *);
#define CALL_VertexAttrib4dvARB(disp, parameters) \
    (* GET_VertexAttrib4dvARB(disp)) parameters
static inline _glptr_VertexAttrib4dvARB GET_VertexAttrib4dvARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib4dvARB) (GET_by_offset(disp, _gloffset_VertexAttrib4dvARB));
}

static inline void SET_VertexAttrib4dvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4dvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib4fARB)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat);
#define CALL_VertexAttrib4fARB(disp, parameters) \
    (* GET_VertexAttrib4fARB(disp)) parameters
static inline _glptr_VertexAttrib4fARB GET_VertexAttrib4fARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib4fARB) (GET_by_offset(disp, _gloffset_VertexAttrib4fARB));
}

static inline void SET_VertexAttrib4fARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4fARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib4fvARB)(GLuint, const GLfloat *);
#define CALL_VertexAttrib4fvARB(disp, parameters) \
    (* GET_VertexAttrib4fvARB(disp)) parameters
static inline _glptr_VertexAttrib4fvARB GET_VertexAttrib4fvARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib4fvARB) (GET_by_offset(disp, _gloffset_VertexAttrib4fvARB));
}

static inline void SET_VertexAttrib4fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4fvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib4ivARB)(GLuint, const GLint *);
#define CALL_VertexAttrib4ivARB(disp, parameters) \
    (* GET_VertexAttrib4ivARB(disp)) parameters
static inline _glptr_VertexAttrib4ivARB GET_VertexAttrib4ivARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib4ivARB) (GET_by_offset(disp, _gloffset_VertexAttrib4ivARB));
}

static inline void SET_VertexAttrib4ivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLint *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4ivARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib4sARB)(GLuint, GLshort, GLshort, GLshort, GLshort);
#define CALL_VertexAttrib4sARB(disp, parameters) \
    (* GET_VertexAttrib4sARB(disp)) parameters
static inline _glptr_VertexAttrib4sARB GET_VertexAttrib4sARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib4sARB) (GET_by_offset(disp, _gloffset_VertexAttrib4sARB));
}

static inline void SET_VertexAttrib4sARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLshort, GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4sARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib4svARB)(GLuint, const GLshort *);
#define CALL_VertexAttrib4svARB(disp, parameters) \
    (* GET_VertexAttrib4svARB(disp)) parameters
static inline _glptr_VertexAttrib4svARB GET_VertexAttrib4svARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib4svARB) (GET_by_offset(disp, _gloffset_VertexAttrib4svARB));
}

static inline void SET_VertexAttrib4svARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLshort *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4svARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib4ubvARB)(GLuint, const GLubyte *);
#define CALL_VertexAttrib4ubvARB(disp, parameters) \
    (* GET_VertexAttrib4ubvARB(disp)) parameters
static inline _glptr_VertexAttrib4ubvARB GET_VertexAttrib4ubvARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib4ubvARB) (GET_by_offset(disp, _gloffset_VertexAttrib4ubvARB));
}

static inline void SET_VertexAttrib4ubvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLubyte *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4ubvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib4uivARB)(GLuint, const GLuint *);
#define CALL_VertexAttrib4uivARB(disp, parameters) \
    (* GET_VertexAttrib4uivARB(disp)) parameters
static inline _glptr_VertexAttrib4uivARB GET_VertexAttrib4uivARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib4uivARB) (GET_by_offset(disp, _gloffset_VertexAttrib4uivARB));
}

static inline void SET_VertexAttrib4uivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLuint *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4uivARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib4usvARB)(GLuint, const GLushort *);
#define CALL_VertexAttrib4usvARB(disp, parameters) \
    (* GET_VertexAttrib4usvARB(disp)) parameters
static inline _glptr_VertexAttrib4usvARB GET_VertexAttrib4usvARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib4usvARB) (GET_by_offset(disp, _gloffset_VertexAttrib4usvARB));
}

static inline void SET_VertexAttrib4usvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLushort *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4usvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribPointerARB)(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid *);
#define CALL_VertexAttribPointerARB(disp, parameters) \
    (* GET_VertexAttribPointerARB(disp)) parameters
static inline _glptr_VertexAttribPointerARB GET_VertexAttribPointerARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttribPointerARB) (GET_by_offset(disp, _gloffset_VertexAttribPointerARB));
}

static inline void SET_VertexAttribPointerARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_VertexAttribPointerARB, fn);
}

typedef void (GLAPIENTRYP _glptr_BindBufferARB)(GLenum, GLuint);
#define CALL_BindBufferARB(disp, parameters) \
    (* GET_BindBufferARB(disp)) parameters
static inline _glptr_BindBufferARB GET_BindBufferARB(struct _glapi_table *disp) {
   return (_glptr_BindBufferARB) (GET_by_offset(disp, _gloffset_BindBufferARB));
}

static inline void SET_BindBufferARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_BindBufferARB, fn);
}

typedef void (GLAPIENTRYP _glptr_BufferDataARB)(GLenum, GLsizeiptrARB, const GLvoid *, GLenum);
#define CALL_BufferDataARB(disp, parameters) \
    (* GET_BufferDataARB(disp)) parameters
static inline _glptr_BufferDataARB GET_BufferDataARB(struct _glapi_table *disp) {
   return (_glptr_BufferDataARB) (GET_by_offset(disp, _gloffset_BufferDataARB));
}

static inline void SET_BufferDataARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizeiptrARB, const GLvoid *, GLenum)) {
   SET_by_offset(disp, _gloffset_BufferDataARB, fn);
}

typedef void (GLAPIENTRYP _glptr_BufferSubDataARB)(GLenum, GLintptrARB, GLsizeiptrARB, const GLvoid *);
#define CALL_BufferSubDataARB(disp, parameters) \
    (* GET_BufferSubDataARB(disp)) parameters
static inline _glptr_BufferSubDataARB GET_BufferSubDataARB(struct _glapi_table *disp) {
   return (_glptr_BufferSubDataARB) (GET_by_offset(disp, _gloffset_BufferSubDataARB));
}

static inline void SET_BufferSubDataARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLintptrARB, GLsizeiptrARB, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_BufferSubDataARB, fn);
}

typedef void (GLAPIENTRYP _glptr_DeleteBuffersARB)(GLsizei, const GLuint *);
#define CALL_DeleteBuffersARB(disp, parameters) \
    (* GET_DeleteBuffersARB(disp)) parameters
static inline _glptr_DeleteBuffersARB GET_DeleteBuffersARB(struct _glapi_table *disp) {
   return (_glptr_DeleteBuffersARB) (GET_by_offset(disp, _gloffset_DeleteBuffersARB));
}

static inline void SET_DeleteBuffersARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_DeleteBuffersARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GenBuffersARB)(GLsizei, GLuint *);
#define CALL_GenBuffersARB(disp, parameters) \
    (* GET_GenBuffersARB(disp)) parameters
static inline _glptr_GenBuffersARB GET_GenBuffersARB(struct _glapi_table *disp) {
   return (_glptr_GenBuffersARB) (GET_by_offset(disp, _gloffset_GenBuffersARB));
}

static inline void SET_GenBuffersARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLuint *)) {
   SET_by_offset(disp, _gloffset_GenBuffersARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetBufferParameterivARB)(GLenum, GLenum, GLint *);
#define CALL_GetBufferParameterivARB(disp, parameters) \
    (* GET_GetBufferParameterivARB(disp)) parameters
static inline _glptr_GetBufferParameterivARB GET_GetBufferParameterivARB(struct _glapi_table *disp) {
   return (_glptr_GetBufferParameterivARB) (GET_by_offset(disp, _gloffset_GetBufferParameterivARB));
}

static inline void SET_GetBufferParameterivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetBufferParameterivARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetBufferPointervARB)(GLenum, GLenum, GLvoid **);
#define CALL_GetBufferPointervARB(disp, parameters) \
    (* GET_GetBufferPointervARB(disp)) parameters
static inline _glptr_GetBufferPointervARB GET_GetBufferPointervARB(struct _glapi_table *disp) {
   return (_glptr_GetBufferPointervARB) (GET_by_offset(disp, _gloffset_GetBufferPointervARB));
}

static inline void SET_GetBufferPointervARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLvoid **)) {
   SET_by_offset(disp, _gloffset_GetBufferPointervARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetBufferSubDataARB)(GLenum, GLintptrARB, GLsizeiptrARB, GLvoid *);
#define CALL_GetBufferSubDataARB(disp, parameters) \
    (* GET_GetBufferSubDataARB(disp)) parameters
static inline _glptr_GetBufferSubDataARB GET_GetBufferSubDataARB(struct _glapi_table *disp) {
   return (_glptr_GetBufferSubDataARB) (GET_by_offset(disp, _gloffset_GetBufferSubDataARB));
}

static inline void SET_GetBufferSubDataARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLintptrARB, GLsizeiptrARB, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetBufferSubDataARB, fn);
}

typedef GLboolean (GLAPIENTRYP _glptr_IsBufferARB)(GLuint);
#define CALL_IsBufferARB(disp, parameters) \
    (* GET_IsBufferARB(disp)) parameters
static inline _glptr_IsBufferARB GET_IsBufferARB(struct _glapi_table *disp) {
   return (_glptr_IsBufferARB) (GET_by_offset(disp, _gloffset_IsBufferARB));
}

static inline void SET_IsBufferARB(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_IsBufferARB, fn);
}

typedef GLvoid * (GLAPIENTRYP _glptr_MapBufferARB)(GLenum, GLenum);
#define CALL_MapBufferARB(disp, parameters) \
    (* GET_MapBufferARB(disp)) parameters
static inline _glptr_MapBufferARB GET_MapBufferARB(struct _glapi_table *disp) {
   return (_glptr_MapBufferARB) (GET_by_offset(disp, _gloffset_MapBufferARB));
}

static inline void SET_MapBufferARB(struct _glapi_table *disp, GLvoid * (GLAPIENTRYP fn)(GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_MapBufferARB, fn);
}

typedef GLboolean (GLAPIENTRYP _glptr_UnmapBufferARB)(GLenum);
#define CALL_UnmapBufferARB(disp, parameters) \
    (* GET_UnmapBufferARB(disp)) parameters
static inline _glptr_UnmapBufferARB GET_UnmapBufferARB(struct _glapi_table *disp) {
   return (_glptr_UnmapBufferARB) (GET_by_offset(disp, _gloffset_UnmapBufferARB));
}

static inline void SET_UnmapBufferARB(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_UnmapBufferARB, fn);
}

typedef void (GLAPIENTRYP _glptr_BeginQueryARB)(GLenum, GLuint);
#define CALL_BeginQueryARB(disp, parameters) \
    (* GET_BeginQueryARB(disp)) parameters
static inline _glptr_BeginQueryARB GET_BeginQueryARB(struct _glapi_table *disp) {
   return (_glptr_BeginQueryARB) (GET_by_offset(disp, _gloffset_BeginQueryARB));
}

static inline void SET_BeginQueryARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_BeginQueryARB, fn);
}

typedef void (GLAPIENTRYP _glptr_DeleteQueriesARB)(GLsizei, const GLuint *);
#define CALL_DeleteQueriesARB(disp, parameters) \
    (* GET_DeleteQueriesARB(disp)) parameters
static inline _glptr_DeleteQueriesARB GET_DeleteQueriesARB(struct _glapi_table *disp) {
   return (_glptr_DeleteQueriesARB) (GET_by_offset(disp, _gloffset_DeleteQueriesARB));
}

static inline void SET_DeleteQueriesARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_DeleteQueriesARB, fn);
}

typedef void (GLAPIENTRYP _glptr_EndQueryARB)(GLenum);
#define CALL_EndQueryARB(disp, parameters) \
    (* GET_EndQueryARB(disp)) parameters
static inline _glptr_EndQueryARB GET_EndQueryARB(struct _glapi_table *disp) {
   return (_glptr_EndQueryARB) (GET_by_offset(disp, _gloffset_EndQueryARB));
}

static inline void SET_EndQueryARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_EndQueryARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GenQueriesARB)(GLsizei, GLuint *);
#define CALL_GenQueriesARB(disp, parameters) \
    (* GET_GenQueriesARB(disp)) parameters
static inline _glptr_GenQueriesARB GET_GenQueriesARB(struct _glapi_table *disp) {
   return (_glptr_GenQueriesARB) (GET_by_offset(disp, _gloffset_GenQueriesARB));
}

static inline void SET_GenQueriesARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLuint *)) {
   SET_by_offset(disp, _gloffset_GenQueriesARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetQueryObjectivARB)(GLuint, GLenum, GLint *);
#define CALL_GetQueryObjectivARB(disp, parameters) \
    (* GET_GetQueryObjectivARB(disp)) parameters
static inline _glptr_GetQueryObjectivARB GET_GetQueryObjectivARB(struct _glapi_table *disp) {
   return (_glptr_GetQueryObjectivARB) (GET_by_offset(disp, _gloffset_GetQueryObjectivARB));
}

static inline void SET_GetQueryObjectivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetQueryObjectivARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetQueryObjectuivARB)(GLuint, GLenum, GLuint *);
#define CALL_GetQueryObjectuivARB(disp, parameters) \
    (* GET_GetQueryObjectuivARB(disp)) parameters
static inline _glptr_GetQueryObjectuivARB GET_GetQueryObjectuivARB(struct _glapi_table *disp) {
   return (_glptr_GetQueryObjectuivARB) (GET_by_offset(disp, _gloffset_GetQueryObjectuivARB));
}

static inline void SET_GetQueryObjectuivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLuint *)) {
   SET_by_offset(disp, _gloffset_GetQueryObjectuivARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetQueryivARB)(GLenum, GLenum, GLint *);
#define CALL_GetQueryivARB(disp, parameters) \
    (* GET_GetQueryivARB(disp)) parameters
static inline _glptr_GetQueryivARB GET_GetQueryivARB(struct _glapi_table *disp) {
   return (_glptr_GetQueryivARB) (GET_by_offset(disp, _gloffset_GetQueryivARB));
}

static inline void SET_GetQueryivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetQueryivARB, fn);
}

typedef GLboolean (GLAPIENTRYP _glptr_IsQueryARB)(GLuint);
#define CALL_IsQueryARB(disp, parameters) \
    (* GET_IsQueryARB(disp)) parameters
static inline _glptr_IsQueryARB GET_IsQueryARB(struct _glapi_table *disp) {
   return (_glptr_IsQueryARB) (GET_by_offset(disp, _gloffset_IsQueryARB));
}

static inline void SET_IsQueryARB(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_IsQueryARB, fn);
}

typedef void (GLAPIENTRYP _glptr_AttachObjectARB)(GLhandleARB, GLhandleARB);
#define CALL_AttachObjectARB(disp, parameters) \
    (* GET_AttachObjectARB(disp)) parameters
static inline _glptr_AttachObjectARB GET_AttachObjectARB(struct _glapi_table *disp) {
   return (_glptr_AttachObjectARB) (GET_by_offset(disp, _gloffset_AttachObjectARB));
}

static inline void SET_AttachObjectARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLhandleARB)) {
   SET_by_offset(disp, _gloffset_AttachObjectARB, fn);
}

typedef void (GLAPIENTRYP _glptr_CompileShaderARB)(GLhandleARB);
#define CALL_CompileShaderARB(disp, parameters) \
    (* GET_CompileShaderARB(disp)) parameters
static inline _glptr_CompileShaderARB GET_CompileShaderARB(struct _glapi_table *disp) {
   return (_glptr_CompileShaderARB) (GET_by_offset(disp, _gloffset_CompileShaderARB));
}

static inline void SET_CompileShaderARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB)) {
   SET_by_offset(disp, _gloffset_CompileShaderARB, fn);
}

typedef GLhandleARB (GLAPIENTRYP _glptr_CreateProgramObjectARB)(void);
#define CALL_CreateProgramObjectARB(disp, parameters) \
    (* GET_CreateProgramObjectARB(disp)) parameters
static inline _glptr_CreateProgramObjectARB GET_CreateProgramObjectARB(struct _glapi_table *disp) {
   return (_glptr_CreateProgramObjectARB) (GET_by_offset(disp, _gloffset_CreateProgramObjectARB));
}

static inline void SET_CreateProgramObjectARB(struct _glapi_table *disp, GLhandleARB (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_CreateProgramObjectARB, fn);
}

typedef GLhandleARB (GLAPIENTRYP _glptr_CreateShaderObjectARB)(GLenum);
#define CALL_CreateShaderObjectARB(disp, parameters) \
    (* GET_CreateShaderObjectARB(disp)) parameters
static inline _glptr_CreateShaderObjectARB GET_CreateShaderObjectARB(struct _glapi_table *disp) {
   return (_glptr_CreateShaderObjectARB) (GET_by_offset(disp, _gloffset_CreateShaderObjectARB));
}

static inline void SET_CreateShaderObjectARB(struct _glapi_table *disp, GLhandleARB (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_CreateShaderObjectARB, fn);
}

typedef void (GLAPIENTRYP _glptr_DeleteObjectARB)(GLhandleARB);
#define CALL_DeleteObjectARB(disp, parameters) \
    (* GET_DeleteObjectARB(disp)) parameters
static inline _glptr_DeleteObjectARB GET_DeleteObjectARB(struct _glapi_table *disp) {
   return (_glptr_DeleteObjectARB) (GET_by_offset(disp, _gloffset_DeleteObjectARB));
}

static inline void SET_DeleteObjectARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB)) {
   SET_by_offset(disp, _gloffset_DeleteObjectARB, fn);
}

typedef void (GLAPIENTRYP _glptr_DetachObjectARB)(GLhandleARB, GLhandleARB);
#define CALL_DetachObjectARB(disp, parameters) \
    (* GET_DetachObjectARB(disp)) parameters
static inline _glptr_DetachObjectARB GET_DetachObjectARB(struct _glapi_table *disp) {
   return (_glptr_DetachObjectARB) (GET_by_offset(disp, _gloffset_DetachObjectARB));
}

static inline void SET_DetachObjectARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLhandleARB)) {
   SET_by_offset(disp, _gloffset_DetachObjectARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetActiveUniformARB)(GLhandleARB, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLcharARB *);
#define CALL_GetActiveUniformARB(disp, parameters) \
    (* GET_GetActiveUniformARB(disp)) parameters
static inline _glptr_GetActiveUniformARB GET_GetActiveUniformARB(struct _glapi_table *disp) {
   return (_glptr_GetActiveUniformARB) (GET_by_offset(disp, _gloffset_GetActiveUniformARB));
}

static inline void SET_GetActiveUniformARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLcharARB *)) {
   SET_by_offset(disp, _gloffset_GetActiveUniformARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetAttachedObjectsARB)(GLhandleARB, GLsizei, GLsizei *, GLhandleARB *);
#define CALL_GetAttachedObjectsARB(disp, parameters) \
    (* GET_GetAttachedObjectsARB(disp)) parameters
static inline _glptr_GetAttachedObjectsARB GET_GetAttachedObjectsARB(struct _glapi_table *disp) {
   return (_glptr_GetAttachedObjectsARB) (GET_by_offset(disp, _gloffset_GetAttachedObjectsARB));
}

static inline void SET_GetAttachedObjectsARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLsizei, GLsizei *, GLhandleARB *)) {
   SET_by_offset(disp, _gloffset_GetAttachedObjectsARB, fn);
}

typedef GLhandleARB (GLAPIENTRYP _glptr_GetHandleARB)(GLenum);
#define CALL_GetHandleARB(disp, parameters) \
    (* GET_GetHandleARB(disp)) parameters
static inline _glptr_GetHandleARB GET_GetHandleARB(struct _glapi_table *disp) {
   return (_glptr_GetHandleARB) (GET_by_offset(disp, _gloffset_GetHandleARB));
}

static inline void SET_GetHandleARB(struct _glapi_table *disp, GLhandleARB (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_GetHandleARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetInfoLogARB)(GLhandleARB, GLsizei, GLsizei *, GLcharARB *);
#define CALL_GetInfoLogARB(disp, parameters) \
    (* GET_GetInfoLogARB(disp)) parameters
static inline _glptr_GetInfoLogARB GET_GetInfoLogARB(struct _glapi_table *disp) {
   return (_glptr_GetInfoLogARB) (GET_by_offset(disp, _gloffset_GetInfoLogARB));
}

static inline void SET_GetInfoLogARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLsizei, GLsizei *, GLcharARB *)) {
   SET_by_offset(disp, _gloffset_GetInfoLogARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetObjectParameterfvARB)(GLhandleARB, GLenum, GLfloat *);
#define CALL_GetObjectParameterfvARB(disp, parameters) \
    (* GET_GetObjectParameterfvARB(disp)) parameters
static inline _glptr_GetObjectParameterfvARB GET_GetObjectParameterfvARB(struct _glapi_table *disp) {
   return (_glptr_GetObjectParameterfvARB) (GET_by_offset(disp, _gloffset_GetObjectParameterfvARB));
}

static inline void SET_GetObjectParameterfvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetObjectParameterfvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetObjectParameterivARB)(GLhandleARB, GLenum, GLint *);
#define CALL_GetObjectParameterivARB(disp, parameters) \
    (* GET_GetObjectParameterivARB(disp)) parameters
static inline _glptr_GetObjectParameterivARB GET_GetObjectParameterivARB(struct _glapi_table *disp) {
   return (_glptr_GetObjectParameterivARB) (GET_by_offset(disp, _gloffset_GetObjectParameterivARB));
}

static inline void SET_GetObjectParameterivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetObjectParameterivARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetShaderSourceARB)(GLhandleARB, GLsizei, GLsizei *, GLcharARB *);
#define CALL_GetShaderSourceARB(disp, parameters) \
    (* GET_GetShaderSourceARB(disp)) parameters
static inline _glptr_GetShaderSourceARB GET_GetShaderSourceARB(struct _glapi_table *disp) {
   return (_glptr_GetShaderSourceARB) (GET_by_offset(disp, _gloffset_GetShaderSourceARB));
}

static inline void SET_GetShaderSourceARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLsizei, GLsizei *, GLcharARB *)) {
   SET_by_offset(disp, _gloffset_GetShaderSourceARB, fn);
}

typedef GLint (GLAPIENTRYP _glptr_GetUniformLocationARB)(GLhandleARB, const GLcharARB *);
#define CALL_GetUniformLocationARB(disp, parameters) \
    (* GET_GetUniformLocationARB(disp)) parameters
static inline _glptr_GetUniformLocationARB GET_GetUniformLocationARB(struct _glapi_table *disp) {
   return (_glptr_GetUniformLocationARB) (GET_by_offset(disp, _gloffset_GetUniformLocationARB));
}

static inline void SET_GetUniformLocationARB(struct _glapi_table *disp, GLint (GLAPIENTRYP fn)(GLhandleARB, const GLcharARB *)) {
   SET_by_offset(disp, _gloffset_GetUniformLocationARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetUniformfvARB)(GLhandleARB, GLint, GLfloat *);
#define CALL_GetUniformfvARB(disp, parameters) \
    (* GET_GetUniformfvARB(disp)) parameters
static inline _glptr_GetUniformfvARB GET_GetUniformfvARB(struct _glapi_table *disp) {
   return (_glptr_GetUniformfvARB) (GET_by_offset(disp, _gloffset_GetUniformfvARB));
}

static inline void SET_GetUniformfvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLint, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetUniformfvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetUniformivARB)(GLhandleARB, GLint, GLint *);
#define CALL_GetUniformivARB(disp, parameters) \
    (* GET_GetUniformivARB(disp)) parameters
static inline _glptr_GetUniformivARB GET_GetUniformivARB(struct _glapi_table *disp) {
   return (_glptr_GetUniformivARB) (GET_by_offset(disp, _gloffset_GetUniformivARB));
}

static inline void SET_GetUniformivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLint, GLint *)) {
   SET_by_offset(disp, _gloffset_GetUniformivARB, fn);
}

typedef void (GLAPIENTRYP _glptr_LinkProgramARB)(GLhandleARB);
#define CALL_LinkProgramARB(disp, parameters) \
    (* GET_LinkProgramARB(disp)) parameters
static inline _glptr_LinkProgramARB GET_LinkProgramARB(struct _glapi_table *disp) {
   return (_glptr_LinkProgramARB) (GET_by_offset(disp, _gloffset_LinkProgramARB));
}

static inline void SET_LinkProgramARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB)) {
   SET_by_offset(disp, _gloffset_LinkProgramARB, fn);
}

typedef void (GLAPIENTRYP _glptr_ShaderSourceARB)(GLhandleARB, GLsizei, const GLcharARB **, const GLint *);
#define CALL_ShaderSourceARB(disp, parameters) \
    (* GET_ShaderSourceARB(disp)) parameters
static inline _glptr_ShaderSourceARB GET_ShaderSourceARB(struct _glapi_table *disp) {
   return (_glptr_ShaderSourceARB) (GET_by_offset(disp, _gloffset_ShaderSourceARB));
}

static inline void SET_ShaderSourceARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLsizei, const GLcharARB **, const GLint *)) {
   SET_by_offset(disp, _gloffset_ShaderSourceARB, fn);
}

typedef void (GLAPIENTRYP _glptr_Uniform1fARB)(GLint, GLfloat);
#define CALL_Uniform1fARB(disp, parameters) \
    (* GET_Uniform1fARB(disp)) parameters
static inline _glptr_Uniform1fARB GET_Uniform1fARB(struct _glapi_table *disp) {
   return (_glptr_Uniform1fARB) (GET_by_offset(disp, _gloffset_Uniform1fARB));
}

static inline void SET_Uniform1fARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLfloat)) {
   SET_by_offset(disp, _gloffset_Uniform1fARB, fn);
}

typedef void (GLAPIENTRYP _glptr_Uniform1fvARB)(GLint, GLsizei, const GLfloat *);
#define CALL_Uniform1fvARB(disp, parameters) \
    (* GET_Uniform1fvARB(disp)) parameters
static inline _glptr_Uniform1fvARB GET_Uniform1fvARB(struct _glapi_table *disp) {
   return (_glptr_Uniform1fvARB) (GET_by_offset(disp, _gloffset_Uniform1fvARB));
}

static inline void SET_Uniform1fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Uniform1fvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_Uniform1iARB)(GLint, GLint);
#define CALL_Uniform1iARB(disp, parameters) \
    (* GET_Uniform1iARB(disp)) parameters
static inline _glptr_Uniform1iARB GET_Uniform1iARB(struct _glapi_table *disp) {
   return (_glptr_Uniform1iARB) (GET_by_offset(disp, _gloffset_Uniform1iARB));
}

static inline void SET_Uniform1iARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint)) {
   SET_by_offset(disp, _gloffset_Uniform1iARB, fn);
}

typedef void (GLAPIENTRYP _glptr_Uniform1ivARB)(GLint, GLsizei, const GLint *);
#define CALL_Uniform1ivARB(disp, parameters) \
    (* GET_Uniform1ivARB(disp)) parameters
static inline _glptr_Uniform1ivARB GET_Uniform1ivARB(struct _glapi_table *disp) {
   return (_glptr_Uniform1ivARB) (GET_by_offset(disp, _gloffset_Uniform1ivARB));
}

static inline void SET_Uniform1ivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, const GLint *)) {
   SET_by_offset(disp, _gloffset_Uniform1ivARB, fn);
}

typedef void (GLAPIENTRYP _glptr_Uniform2fARB)(GLint, GLfloat, GLfloat);
#define CALL_Uniform2fARB(disp, parameters) \
    (* GET_Uniform2fARB(disp)) parameters
static inline _glptr_Uniform2fARB GET_Uniform2fARB(struct _glapi_table *disp) {
   return (_glptr_Uniform2fARB) (GET_by_offset(disp, _gloffset_Uniform2fARB));
}

static inline void SET_Uniform2fARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_Uniform2fARB, fn);
}

typedef void (GLAPIENTRYP _glptr_Uniform2fvARB)(GLint, GLsizei, const GLfloat *);
#define CALL_Uniform2fvARB(disp, parameters) \
    (* GET_Uniform2fvARB(disp)) parameters
static inline _glptr_Uniform2fvARB GET_Uniform2fvARB(struct _glapi_table *disp) {
   return (_glptr_Uniform2fvARB) (GET_by_offset(disp, _gloffset_Uniform2fvARB));
}

static inline void SET_Uniform2fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Uniform2fvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_Uniform2iARB)(GLint, GLint, GLint);
#define CALL_Uniform2iARB(disp, parameters) \
    (* GET_Uniform2iARB(disp)) parameters
static inline _glptr_Uniform2iARB GET_Uniform2iARB(struct _glapi_table *disp) {
   return (_glptr_Uniform2iARB) (GET_by_offset(disp, _gloffset_Uniform2iARB));
}

static inline void SET_Uniform2iARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_Uniform2iARB, fn);
}

typedef void (GLAPIENTRYP _glptr_Uniform2ivARB)(GLint, GLsizei, const GLint *);
#define CALL_Uniform2ivARB(disp, parameters) \
    (* GET_Uniform2ivARB(disp)) parameters
static inline _glptr_Uniform2ivARB GET_Uniform2ivARB(struct _glapi_table *disp) {
   return (_glptr_Uniform2ivARB) (GET_by_offset(disp, _gloffset_Uniform2ivARB));
}

static inline void SET_Uniform2ivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, const GLint *)) {
   SET_by_offset(disp, _gloffset_Uniform2ivARB, fn);
}

typedef void (GLAPIENTRYP _glptr_Uniform3fARB)(GLint, GLfloat, GLfloat, GLfloat);
#define CALL_Uniform3fARB(disp, parameters) \
    (* GET_Uniform3fARB(disp)) parameters
static inline _glptr_Uniform3fARB GET_Uniform3fARB(struct _glapi_table *disp) {
   return (_glptr_Uniform3fARB) (GET_by_offset(disp, _gloffset_Uniform3fARB));
}

static inline void SET_Uniform3fARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_Uniform3fARB, fn);
}

typedef void (GLAPIENTRYP _glptr_Uniform3fvARB)(GLint, GLsizei, const GLfloat *);
#define CALL_Uniform3fvARB(disp, parameters) \
    (* GET_Uniform3fvARB(disp)) parameters
static inline _glptr_Uniform3fvARB GET_Uniform3fvARB(struct _glapi_table *disp) {
   return (_glptr_Uniform3fvARB) (GET_by_offset(disp, _gloffset_Uniform3fvARB));
}

static inline void SET_Uniform3fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Uniform3fvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_Uniform3iARB)(GLint, GLint, GLint, GLint);
#define CALL_Uniform3iARB(disp, parameters) \
    (* GET_Uniform3iARB(disp)) parameters
static inline _glptr_Uniform3iARB GET_Uniform3iARB(struct _glapi_table *disp) {
   return (_glptr_Uniform3iARB) (GET_by_offset(disp, _gloffset_Uniform3iARB));
}

static inline void SET_Uniform3iARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_Uniform3iARB, fn);
}

typedef void (GLAPIENTRYP _glptr_Uniform3ivARB)(GLint, GLsizei, const GLint *);
#define CALL_Uniform3ivARB(disp, parameters) \
    (* GET_Uniform3ivARB(disp)) parameters
static inline _glptr_Uniform3ivARB GET_Uniform3ivARB(struct _glapi_table *disp) {
   return (_glptr_Uniform3ivARB) (GET_by_offset(disp, _gloffset_Uniform3ivARB));
}

static inline void SET_Uniform3ivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, const GLint *)) {
   SET_by_offset(disp, _gloffset_Uniform3ivARB, fn);
}

typedef void (GLAPIENTRYP _glptr_Uniform4fARB)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
#define CALL_Uniform4fARB(disp, parameters) \
    (* GET_Uniform4fARB(disp)) parameters
static inline _glptr_Uniform4fARB GET_Uniform4fARB(struct _glapi_table *disp) {
   return (_glptr_Uniform4fARB) (GET_by_offset(disp, _gloffset_Uniform4fARB));
}

static inline void SET_Uniform4fARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_Uniform4fARB, fn);
}

typedef void (GLAPIENTRYP _glptr_Uniform4fvARB)(GLint, GLsizei, const GLfloat *);
#define CALL_Uniform4fvARB(disp, parameters) \
    (* GET_Uniform4fvARB(disp)) parameters
static inline _glptr_Uniform4fvARB GET_Uniform4fvARB(struct _glapi_table *disp) {
   return (_glptr_Uniform4fvARB) (GET_by_offset(disp, _gloffset_Uniform4fvARB));
}

static inline void SET_Uniform4fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Uniform4fvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_Uniform4iARB)(GLint, GLint, GLint, GLint, GLint);
#define CALL_Uniform4iARB(disp, parameters) \
    (* GET_Uniform4iARB(disp)) parameters
static inline _glptr_Uniform4iARB GET_Uniform4iARB(struct _glapi_table *disp) {
   return (_glptr_Uniform4iARB) (GET_by_offset(disp, _gloffset_Uniform4iARB));
}

static inline void SET_Uniform4iARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_Uniform4iARB, fn);
}

typedef void (GLAPIENTRYP _glptr_Uniform4ivARB)(GLint, GLsizei, const GLint *);
#define CALL_Uniform4ivARB(disp, parameters) \
    (* GET_Uniform4ivARB(disp)) parameters
static inline _glptr_Uniform4ivARB GET_Uniform4ivARB(struct _glapi_table *disp) {
   return (_glptr_Uniform4ivARB) (GET_by_offset(disp, _gloffset_Uniform4ivARB));
}

static inline void SET_Uniform4ivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, const GLint *)) {
   SET_by_offset(disp, _gloffset_Uniform4ivARB, fn);
}

typedef void (GLAPIENTRYP _glptr_UniformMatrix2fvARB)(GLint, GLsizei, GLboolean, const GLfloat *);
#define CALL_UniformMatrix2fvARB(disp, parameters) \
    (* GET_UniformMatrix2fvARB(disp)) parameters
static inline _glptr_UniformMatrix2fvARB GET_UniformMatrix2fvARB(struct _glapi_table *disp) {
   return (_glptr_UniformMatrix2fvARB) (GET_by_offset(disp, _gloffset_UniformMatrix2fvARB));
}

static inline void SET_UniformMatrix2fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, GLboolean, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_UniformMatrix2fvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_UniformMatrix3fvARB)(GLint, GLsizei, GLboolean, const GLfloat *);
#define CALL_UniformMatrix3fvARB(disp, parameters) \
    (* GET_UniformMatrix3fvARB(disp)) parameters
static inline _glptr_UniformMatrix3fvARB GET_UniformMatrix3fvARB(struct _glapi_table *disp) {
   return (_glptr_UniformMatrix3fvARB) (GET_by_offset(disp, _gloffset_UniformMatrix3fvARB));
}

static inline void SET_UniformMatrix3fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, GLboolean, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_UniformMatrix3fvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_UniformMatrix4fvARB)(GLint, GLsizei, GLboolean, const GLfloat *);
#define CALL_UniformMatrix4fvARB(disp, parameters) \
    (* GET_UniformMatrix4fvARB(disp)) parameters
static inline _glptr_UniformMatrix4fvARB GET_UniformMatrix4fvARB(struct _glapi_table *disp) {
   return (_glptr_UniformMatrix4fvARB) (GET_by_offset(disp, _gloffset_UniformMatrix4fvARB));
}

static inline void SET_UniformMatrix4fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, GLboolean, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_UniformMatrix4fvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_UseProgramObjectARB)(GLhandleARB);
#define CALL_UseProgramObjectARB(disp, parameters) \
    (* GET_UseProgramObjectARB(disp)) parameters
static inline _glptr_UseProgramObjectARB GET_UseProgramObjectARB(struct _glapi_table *disp) {
   return (_glptr_UseProgramObjectARB) (GET_by_offset(disp, _gloffset_UseProgramObjectARB));
}

static inline void SET_UseProgramObjectARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB)) {
   SET_by_offset(disp, _gloffset_UseProgramObjectARB, fn);
}

typedef void (GLAPIENTRYP _glptr_ValidateProgramARB)(GLhandleARB);
#define CALL_ValidateProgramARB(disp, parameters) \
    (* GET_ValidateProgramARB(disp)) parameters
static inline _glptr_ValidateProgramARB GET_ValidateProgramARB(struct _glapi_table *disp) {
   return (_glptr_ValidateProgramARB) (GET_by_offset(disp, _gloffset_ValidateProgramARB));
}

static inline void SET_ValidateProgramARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB)) {
   SET_by_offset(disp, _gloffset_ValidateProgramARB, fn);
}

typedef void (GLAPIENTRYP _glptr_BindAttribLocationARB)(GLhandleARB, GLuint, const GLcharARB *);
#define CALL_BindAttribLocationARB(disp, parameters) \
    (* GET_BindAttribLocationARB(disp)) parameters
static inline _glptr_BindAttribLocationARB GET_BindAttribLocationARB(struct _glapi_table *disp) {
   return (_glptr_BindAttribLocationARB) (GET_by_offset(disp, _gloffset_BindAttribLocationARB));
}

static inline void SET_BindAttribLocationARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLuint, const GLcharARB *)) {
   SET_by_offset(disp, _gloffset_BindAttribLocationARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetActiveAttribARB)(GLhandleARB, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLcharARB *);
#define CALL_GetActiveAttribARB(disp, parameters) \
    (* GET_GetActiveAttribARB(disp)) parameters
static inline _glptr_GetActiveAttribARB GET_GetActiveAttribARB(struct _glapi_table *disp) {
   return (_glptr_GetActiveAttribARB) (GET_by_offset(disp, _gloffset_GetActiveAttribARB));
}

static inline void SET_GetActiveAttribARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLcharARB *)) {
   SET_by_offset(disp, _gloffset_GetActiveAttribARB, fn);
}

typedef GLint (GLAPIENTRYP _glptr_GetAttribLocationARB)(GLhandleARB, const GLcharARB *);
#define CALL_GetAttribLocationARB(disp, parameters) \
    (* GET_GetAttribLocationARB(disp)) parameters
static inline _glptr_GetAttribLocationARB GET_GetAttribLocationARB(struct _glapi_table *disp) {
   return (_glptr_GetAttribLocationARB) (GET_by_offset(disp, _gloffset_GetAttribLocationARB));
}

static inline void SET_GetAttribLocationARB(struct _glapi_table *disp, GLint (GLAPIENTRYP fn)(GLhandleARB, const GLcharARB *)) {
   SET_by_offset(disp, _gloffset_GetAttribLocationARB, fn);
}

typedef void (GLAPIENTRYP _glptr_DrawBuffersARB)(GLsizei, const GLenum *);
#define CALL_DrawBuffersARB(disp, parameters) \
    (* GET_DrawBuffersARB(disp)) parameters
static inline _glptr_DrawBuffersARB GET_DrawBuffersARB(struct _glapi_table *disp) {
   return (_glptr_DrawBuffersARB) (GET_by_offset(disp, _gloffset_DrawBuffersARB));
}

static inline void SET_DrawBuffersARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLenum *)) {
   SET_by_offset(disp, _gloffset_DrawBuffersARB, fn);
}

typedef void (GLAPIENTRYP _glptr_ClampColorARB)(GLenum, GLenum);
#define CALL_ClampColorARB(disp, parameters) \
    (* GET_ClampColorARB(disp)) parameters
static inline _glptr_ClampColorARB GET_ClampColorARB(struct _glapi_table *disp) {
   return (_glptr_ClampColorARB) (GET_by_offset(disp, _gloffset_ClampColorARB));
}

static inline void SET_ClampColorARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_ClampColorARB, fn);
}

typedef void (GLAPIENTRYP _glptr_DrawArraysInstancedARB)(GLenum, GLint, GLsizei, GLsizei);
#define CALL_DrawArraysInstancedARB(disp, parameters) \
    (* GET_DrawArraysInstancedARB(disp)) parameters
static inline _glptr_DrawArraysInstancedARB GET_DrawArraysInstancedARB(struct _glapi_table *disp) {
   return (_glptr_DrawArraysInstancedARB) (GET_by_offset(disp, _gloffset_DrawArraysInstancedARB));
}

static inline void SET_DrawArraysInstancedARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLsizei, GLsizei)) {
   SET_by_offset(disp, _gloffset_DrawArraysInstancedARB, fn);
}

typedef void (GLAPIENTRYP _glptr_DrawElementsInstancedARB)(GLenum, GLsizei, GLenum, const GLvoid *, GLsizei);
#define CALL_DrawElementsInstancedARB(disp, parameters) \
    (* GET_DrawElementsInstancedARB(disp)) parameters
static inline _glptr_DrawElementsInstancedARB GET_DrawElementsInstancedARB(struct _glapi_table *disp) {
   return (_glptr_DrawElementsInstancedARB) (GET_by_offset(disp, _gloffset_DrawElementsInstancedARB));
}

static inline void SET_DrawElementsInstancedARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLenum, const GLvoid *, GLsizei)) {
   SET_by_offset(disp, _gloffset_DrawElementsInstancedARB, fn);
}

typedef void (GLAPIENTRYP _glptr_RenderbufferStorageMultisample)(GLenum, GLsizei, GLenum, GLsizei, GLsizei);
#define CALL_RenderbufferStorageMultisample(disp, parameters) \
    (* GET_RenderbufferStorageMultisample(disp)) parameters
static inline _glptr_RenderbufferStorageMultisample GET_RenderbufferStorageMultisample(struct _glapi_table *disp) {
   return (_glptr_RenderbufferStorageMultisample) (GET_by_offset(disp, _gloffset_RenderbufferStorageMultisample));
}

static inline void SET_RenderbufferStorageMultisample(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLenum, GLsizei, GLsizei)) {
   SET_by_offset(disp, _gloffset_RenderbufferStorageMultisample, fn);
}

typedef void (GLAPIENTRYP _glptr_FramebufferTextureARB)(GLenum, GLenum, GLuint, GLint);
#define CALL_FramebufferTextureARB(disp, parameters) \
    (* GET_FramebufferTextureARB(disp)) parameters
static inline _glptr_FramebufferTextureARB GET_FramebufferTextureARB(struct _glapi_table *disp) {
   return (_glptr_FramebufferTextureARB) (GET_by_offset(disp, _gloffset_FramebufferTextureARB));
}

static inline void SET_FramebufferTextureARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLuint, GLint)) {
   SET_by_offset(disp, _gloffset_FramebufferTextureARB, fn);
}

typedef void (GLAPIENTRYP _glptr_FramebufferTextureFaceARB)(GLenum, GLenum, GLuint, GLint, GLenum);
#define CALL_FramebufferTextureFaceARB(disp, parameters) \
    (* GET_FramebufferTextureFaceARB(disp)) parameters
static inline _glptr_FramebufferTextureFaceARB GET_FramebufferTextureFaceARB(struct _glapi_table *disp) {
   return (_glptr_FramebufferTextureFaceARB) (GET_by_offset(disp, _gloffset_FramebufferTextureFaceARB));
}

static inline void SET_FramebufferTextureFaceARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLuint, GLint, GLenum)) {
   SET_by_offset(disp, _gloffset_FramebufferTextureFaceARB, fn);
}

typedef void (GLAPIENTRYP _glptr_ProgramParameteriARB)(GLuint, GLenum, GLint);
#define CALL_ProgramParameteriARB(disp, parameters) \
    (* GET_ProgramParameteriARB(disp)) parameters
static inline _glptr_ProgramParameteriARB GET_ProgramParameteriARB(struct _glapi_table *disp) {
   return (_glptr_ProgramParameteriARB) (GET_by_offset(disp, _gloffset_ProgramParameteriARB));
}

static inline void SET_ProgramParameteriARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_ProgramParameteriARB, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribDivisorARB)(GLuint, GLuint);
#define CALL_VertexAttribDivisorARB(disp, parameters) \
    (* GET_VertexAttribDivisorARB(disp)) parameters
static inline _glptr_VertexAttribDivisorARB GET_VertexAttribDivisorARB(struct _glapi_table *disp) {
   return (_glptr_VertexAttribDivisorARB) (GET_by_offset(disp, _gloffset_VertexAttribDivisorARB));
}

static inline void SET_VertexAttribDivisorARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_VertexAttribDivisorARB, fn);
}

typedef void (GLAPIENTRYP _glptr_FlushMappedBufferRange)(GLenum, GLintptr, GLsizeiptr);
#define CALL_FlushMappedBufferRange(disp, parameters) \
    (* GET_FlushMappedBufferRange(disp)) parameters
static inline _glptr_FlushMappedBufferRange GET_FlushMappedBufferRange(struct _glapi_table *disp) {
   return (_glptr_FlushMappedBufferRange) (GET_by_offset(disp, _gloffset_FlushMappedBufferRange));
}

static inline void SET_FlushMappedBufferRange(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLintptr, GLsizeiptr)) {
   SET_by_offset(disp, _gloffset_FlushMappedBufferRange, fn);
}

typedef GLvoid * (GLAPIENTRYP _glptr_MapBufferRange)(GLenum, GLintptr, GLsizeiptr, GLbitfield);
#define CALL_MapBufferRange(disp, parameters) \
    (* GET_MapBufferRange(disp)) parameters
static inline _glptr_MapBufferRange GET_MapBufferRange(struct _glapi_table *disp) {
   return (_glptr_MapBufferRange) (GET_by_offset(disp, _gloffset_MapBufferRange));
}

static inline void SET_MapBufferRange(struct _glapi_table *disp, GLvoid * (GLAPIENTRYP fn)(GLenum, GLintptr, GLsizeiptr, GLbitfield)) {
   SET_by_offset(disp, _gloffset_MapBufferRange, fn);
}

typedef void (GLAPIENTRYP _glptr_TexBufferARB)(GLenum, GLenum, GLuint);
#define CALL_TexBufferARB(disp, parameters) \
    (* GET_TexBufferARB(disp)) parameters
static inline _glptr_TexBufferARB GET_TexBufferARB(struct _glapi_table *disp) {
   return (_glptr_TexBufferARB) (GET_by_offset(disp, _gloffset_TexBufferARB));
}

static inline void SET_TexBufferARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_TexBufferARB, fn);
}

typedef void (GLAPIENTRYP _glptr_BindVertexArray)(GLuint);
#define CALL_BindVertexArray(disp, parameters) \
    (* GET_BindVertexArray(disp)) parameters
static inline _glptr_BindVertexArray GET_BindVertexArray(struct _glapi_table *disp) {
   return (_glptr_BindVertexArray) (GET_by_offset(disp, _gloffset_BindVertexArray));
}

static inline void SET_BindVertexArray(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_BindVertexArray, fn);
}

typedef void (GLAPIENTRYP _glptr_GenVertexArrays)(GLsizei, GLuint *);
#define CALL_GenVertexArrays(disp, parameters) \
    (* GET_GenVertexArrays(disp)) parameters
static inline _glptr_GenVertexArrays GET_GenVertexArrays(struct _glapi_table *disp) {
   return (_glptr_GenVertexArrays) (GET_by_offset(disp, _gloffset_GenVertexArrays));
}

static inline void SET_GenVertexArrays(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLuint *)) {
   SET_by_offset(disp, _gloffset_GenVertexArrays, fn);
}

typedef void (GLAPIENTRYP _glptr_CopyBufferSubData)(GLenum, GLenum, GLintptr, GLintptr, GLsizeiptr);
#define CALL_CopyBufferSubData(disp, parameters) \
    (* GET_CopyBufferSubData(disp)) parameters
static inline _glptr_CopyBufferSubData GET_CopyBufferSubData(struct _glapi_table *disp) {
   return (_glptr_CopyBufferSubData) (GET_by_offset(disp, _gloffset_CopyBufferSubData));
}

static inline void SET_CopyBufferSubData(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLintptr, GLintptr, GLsizeiptr)) {
   SET_by_offset(disp, _gloffset_CopyBufferSubData, fn);
}

typedef GLenum (GLAPIENTRYP _glptr_ClientWaitSync)(GLsync, GLbitfield, GLuint64);
#define CALL_ClientWaitSync(disp, parameters) \
    (* GET_ClientWaitSync(disp)) parameters
static inline _glptr_ClientWaitSync GET_ClientWaitSync(struct _glapi_table *disp) {
   return (_glptr_ClientWaitSync) (GET_by_offset(disp, _gloffset_ClientWaitSync));
}

static inline void SET_ClientWaitSync(struct _glapi_table *disp, GLenum (GLAPIENTRYP fn)(GLsync, GLbitfield, GLuint64)) {
   SET_by_offset(disp, _gloffset_ClientWaitSync, fn);
}

typedef void (GLAPIENTRYP _glptr_DeleteSync)(GLsync);
#define CALL_DeleteSync(disp, parameters) \
    (* GET_DeleteSync(disp)) parameters
static inline _glptr_DeleteSync GET_DeleteSync(struct _glapi_table *disp) {
   return (_glptr_DeleteSync) (GET_by_offset(disp, _gloffset_DeleteSync));
}

static inline void SET_DeleteSync(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsync)) {
   SET_by_offset(disp, _gloffset_DeleteSync, fn);
}

typedef GLsync (GLAPIENTRYP _glptr_FenceSync)(GLenum, GLbitfield);
#define CALL_FenceSync(disp, parameters) \
    (* GET_FenceSync(disp)) parameters
static inline _glptr_FenceSync GET_FenceSync(struct _glapi_table *disp) {
   return (_glptr_FenceSync) (GET_by_offset(disp, _gloffset_FenceSync));
}

static inline void SET_FenceSync(struct _glapi_table *disp, GLsync (GLAPIENTRYP fn)(GLenum, GLbitfield)) {
   SET_by_offset(disp, _gloffset_FenceSync, fn);
}

typedef void (GLAPIENTRYP _glptr_GetInteger64v)(GLenum, GLint64 *);
#define CALL_GetInteger64v(disp, parameters) \
    (* GET_GetInteger64v(disp)) parameters
static inline _glptr_GetInteger64v GET_GetInteger64v(struct _glapi_table *disp) {
   return (_glptr_GetInteger64v) (GET_by_offset(disp, _gloffset_GetInteger64v));
}

static inline void SET_GetInteger64v(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint64 *)) {
   SET_by_offset(disp, _gloffset_GetInteger64v, fn);
}

typedef void (GLAPIENTRYP _glptr_GetSynciv)(GLsync, GLenum, GLsizei, GLsizei *, GLint *);
#define CALL_GetSynciv(disp, parameters) \
    (* GET_GetSynciv(disp)) parameters
static inline _glptr_GetSynciv GET_GetSynciv(struct _glapi_table *disp) {
   return (_glptr_GetSynciv) (GET_by_offset(disp, _gloffset_GetSynciv));
}

static inline void SET_GetSynciv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsync, GLenum, GLsizei, GLsizei *, GLint *)) {
   SET_by_offset(disp, _gloffset_GetSynciv, fn);
}

typedef GLboolean (GLAPIENTRYP _glptr_IsSync)(GLsync);
#define CALL_IsSync(disp, parameters) \
    (* GET_IsSync(disp)) parameters
static inline _glptr_IsSync GET_IsSync(struct _glapi_table *disp) {
   return (_glptr_IsSync) (GET_by_offset(disp, _gloffset_IsSync));
}

static inline void SET_IsSync(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLsync)) {
   SET_by_offset(disp, _gloffset_IsSync, fn);
}

typedef void (GLAPIENTRYP _glptr_WaitSync)(GLsync, GLbitfield, GLuint64);
#define CALL_WaitSync(disp, parameters) \
    (* GET_WaitSync(disp)) parameters
static inline _glptr_WaitSync GET_WaitSync(struct _glapi_table *disp) {
   return (_glptr_WaitSync) (GET_by_offset(disp, _gloffset_WaitSync));
}

static inline void SET_WaitSync(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsync, GLbitfield, GLuint64)) {
   SET_by_offset(disp, _gloffset_WaitSync, fn);
}

typedef void (GLAPIENTRYP _glptr_DrawElementsBaseVertex)(GLenum, GLsizei, GLenum, const GLvoid *, GLint);
#define CALL_DrawElementsBaseVertex(disp, parameters) \
    (* GET_DrawElementsBaseVertex(disp)) parameters
static inline _glptr_DrawElementsBaseVertex GET_DrawElementsBaseVertex(struct _glapi_table *disp) {
   return (_glptr_DrawElementsBaseVertex) (GET_by_offset(disp, _gloffset_DrawElementsBaseVertex));
}

static inline void SET_DrawElementsBaseVertex(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLenum, const GLvoid *, GLint)) {
   SET_by_offset(disp, _gloffset_DrawElementsBaseVertex, fn);
}

typedef void (GLAPIENTRYP _glptr_DrawElementsInstancedBaseVertex)(GLenum, GLsizei, GLenum, const GLvoid *, GLsizei, GLint);
#define CALL_DrawElementsInstancedBaseVertex(disp, parameters) \
    (* GET_DrawElementsInstancedBaseVertex(disp)) parameters
static inline _glptr_DrawElementsInstancedBaseVertex GET_DrawElementsInstancedBaseVertex(struct _glapi_table *disp) {
   return (_glptr_DrawElementsInstancedBaseVertex) (GET_by_offset(disp, _gloffset_DrawElementsInstancedBaseVertex));
}

static inline void SET_DrawElementsInstancedBaseVertex(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLenum, const GLvoid *, GLsizei, GLint)) {
   SET_by_offset(disp, _gloffset_DrawElementsInstancedBaseVertex, fn);
}

typedef void (GLAPIENTRYP _glptr_DrawRangeElementsBaseVertex)(GLenum, GLuint, GLuint, GLsizei, GLenum, const GLvoid *, GLint);
#define CALL_DrawRangeElementsBaseVertex(disp, parameters) \
    (* GET_DrawRangeElementsBaseVertex(disp)) parameters
static inline _glptr_DrawRangeElementsBaseVertex GET_DrawRangeElementsBaseVertex(struct _glapi_table *disp) {
   return (_glptr_DrawRangeElementsBaseVertex) (GET_by_offset(disp, _gloffset_DrawRangeElementsBaseVertex));
}

static inline void SET_DrawRangeElementsBaseVertex(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLuint, GLsizei, GLenum, const GLvoid *, GLint)) {
   SET_by_offset(disp, _gloffset_DrawRangeElementsBaseVertex, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiDrawElementsBaseVertex)(GLenum, const GLsizei *, GLenum, const GLvoid **, GLsizei, const GLint *);
#define CALL_MultiDrawElementsBaseVertex(disp, parameters) \
    (* GET_MultiDrawElementsBaseVertex(disp)) parameters
static inline _glptr_MultiDrawElementsBaseVertex GET_MultiDrawElementsBaseVertex(struct _glapi_table *disp) {
   return (_glptr_MultiDrawElementsBaseVertex) (GET_by_offset(disp, _gloffset_MultiDrawElementsBaseVertex));
}

static inline void SET_MultiDrawElementsBaseVertex(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLsizei *, GLenum, const GLvoid **, GLsizei, const GLint *)) {
   SET_by_offset(disp, _gloffset_MultiDrawElementsBaseVertex, fn);
}

typedef void (GLAPIENTRYP _glptr_BlendEquationSeparateiARB)(GLuint, GLenum, GLenum);
#define CALL_BlendEquationSeparateiARB(disp, parameters) \
    (* GET_BlendEquationSeparateiARB(disp)) parameters
static inline _glptr_BlendEquationSeparateiARB GET_BlendEquationSeparateiARB(struct _glapi_table *disp) {
   return (_glptr_BlendEquationSeparateiARB) (GET_by_offset(disp, _gloffset_BlendEquationSeparateiARB));
}

static inline void SET_BlendEquationSeparateiARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_BlendEquationSeparateiARB, fn);
}

typedef void (GLAPIENTRYP _glptr_BlendEquationiARB)(GLuint, GLenum);
#define CALL_BlendEquationiARB(disp, parameters) \
    (* GET_BlendEquationiARB(disp)) parameters
static inline _glptr_BlendEquationiARB GET_BlendEquationiARB(struct _glapi_table *disp) {
   return (_glptr_BlendEquationiARB) (GET_by_offset(disp, _gloffset_BlendEquationiARB));
}

static inline void SET_BlendEquationiARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum)) {
   SET_by_offset(disp, _gloffset_BlendEquationiARB, fn);
}

typedef void (GLAPIENTRYP _glptr_BlendFuncSeparateiARB)(GLuint, GLenum, GLenum, GLenum, GLenum);
#define CALL_BlendFuncSeparateiARB(disp, parameters) \
    (* GET_BlendFuncSeparateiARB(disp)) parameters
static inline _glptr_BlendFuncSeparateiARB GET_BlendFuncSeparateiARB(struct _glapi_table *disp) {
   return (_glptr_BlendFuncSeparateiARB) (GET_by_offset(disp, _gloffset_BlendFuncSeparateiARB));
}

static inline void SET_BlendFuncSeparateiARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLenum, GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_BlendFuncSeparateiARB, fn);
}

typedef void (GLAPIENTRYP _glptr_BlendFunciARB)(GLuint, GLenum, GLenum);
#define CALL_BlendFunciARB(disp, parameters) \
    (* GET_BlendFunciARB(disp)) parameters
static inline _glptr_BlendFunciARB GET_BlendFunciARB(struct _glapi_table *disp) {
   return (_glptr_BlendFunciARB) (GET_by_offset(disp, _gloffset_BlendFunciARB));
}

static inline void SET_BlendFunciARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_BlendFunciARB, fn);
}

typedef void (GLAPIENTRYP _glptr_BindSampler)(GLuint, GLuint);
#define CALL_BindSampler(disp, parameters) \
    (* GET_BindSampler(disp)) parameters
static inline _glptr_BindSampler GET_BindSampler(struct _glapi_table *disp) {
   return (_glptr_BindSampler) (GET_by_offset(disp, _gloffset_BindSampler));
}

static inline void SET_BindSampler(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_BindSampler, fn);
}

typedef void (GLAPIENTRYP _glptr_DeleteSamplers)(GLsizei, const GLuint *);
#define CALL_DeleteSamplers(disp, parameters) \
    (* GET_DeleteSamplers(disp)) parameters
static inline _glptr_DeleteSamplers GET_DeleteSamplers(struct _glapi_table *disp) {
   return (_glptr_DeleteSamplers) (GET_by_offset(disp, _gloffset_DeleteSamplers));
}

static inline void SET_DeleteSamplers(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_DeleteSamplers, fn);
}

typedef void (GLAPIENTRYP _glptr_GenSamplers)(GLsizei, GLuint *);
#define CALL_GenSamplers(disp, parameters) \
    (* GET_GenSamplers(disp)) parameters
static inline _glptr_GenSamplers GET_GenSamplers(struct _glapi_table *disp) {
   return (_glptr_GenSamplers) (GET_by_offset(disp, _gloffset_GenSamplers));
}

static inline void SET_GenSamplers(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLuint *)) {
   SET_by_offset(disp, _gloffset_GenSamplers, fn);
}

typedef void (GLAPIENTRYP _glptr_GetSamplerParameterIiv)(GLuint, GLenum, GLint *);
#define CALL_GetSamplerParameterIiv(disp, parameters) \
    (* GET_GetSamplerParameterIiv(disp)) parameters
static inline _glptr_GetSamplerParameterIiv GET_GetSamplerParameterIiv(struct _glapi_table *disp) {
   return (_glptr_GetSamplerParameterIiv) (GET_by_offset(disp, _gloffset_GetSamplerParameterIiv));
}

static inline void SET_GetSamplerParameterIiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetSamplerParameterIiv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetSamplerParameterIuiv)(GLuint, GLenum, GLuint *);
#define CALL_GetSamplerParameterIuiv(disp, parameters) \
    (* GET_GetSamplerParameterIuiv(disp)) parameters
static inline _glptr_GetSamplerParameterIuiv GET_GetSamplerParameterIuiv(struct _glapi_table *disp) {
   return (_glptr_GetSamplerParameterIuiv) (GET_by_offset(disp, _gloffset_GetSamplerParameterIuiv));
}

static inline void SET_GetSamplerParameterIuiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLuint *)) {
   SET_by_offset(disp, _gloffset_GetSamplerParameterIuiv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetSamplerParameterfv)(GLuint, GLenum, GLfloat *);
#define CALL_GetSamplerParameterfv(disp, parameters) \
    (* GET_GetSamplerParameterfv(disp)) parameters
static inline _glptr_GetSamplerParameterfv GET_GetSamplerParameterfv(struct _glapi_table *disp) {
   return (_glptr_GetSamplerParameterfv) (GET_by_offset(disp, _gloffset_GetSamplerParameterfv));
}

static inline void SET_GetSamplerParameterfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetSamplerParameterfv, fn);
}

typedef void (GLAPIENTRYP _glptr_GetSamplerParameteriv)(GLuint, GLenum, GLint *);
#define CALL_GetSamplerParameteriv(disp, parameters) \
    (* GET_GetSamplerParameteriv(disp)) parameters
static inline _glptr_GetSamplerParameteriv GET_GetSamplerParameteriv(struct _glapi_table *disp) {
   return (_glptr_GetSamplerParameteriv) (GET_by_offset(disp, _gloffset_GetSamplerParameteriv));
}

static inline void SET_GetSamplerParameteriv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetSamplerParameteriv, fn);
}

typedef GLboolean (GLAPIENTRYP _glptr_IsSampler)(GLuint);
#define CALL_IsSampler(disp, parameters) \
    (* GET_IsSampler(disp)) parameters
static inline _glptr_IsSampler GET_IsSampler(struct _glapi_table *disp) {
   return (_glptr_IsSampler) (GET_by_offset(disp, _gloffset_IsSampler));
}

static inline void SET_IsSampler(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_IsSampler, fn);
}

typedef void (GLAPIENTRYP _glptr_SamplerParameterIiv)(GLuint, GLenum, const GLint *);
#define CALL_SamplerParameterIiv(disp, parameters) \
    (* GET_SamplerParameterIiv(disp)) parameters
static inline _glptr_SamplerParameterIiv GET_SamplerParameterIiv(struct _glapi_table *disp) {
   return (_glptr_SamplerParameterIiv) (GET_by_offset(disp, _gloffset_SamplerParameterIiv));
}

static inline void SET_SamplerParameterIiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_SamplerParameterIiv, fn);
}

typedef void (GLAPIENTRYP _glptr_SamplerParameterIuiv)(GLuint, GLenum, const GLuint *);
#define CALL_SamplerParameterIuiv(disp, parameters) \
    (* GET_SamplerParameterIuiv(disp)) parameters
static inline _glptr_SamplerParameterIuiv GET_SamplerParameterIuiv(struct _glapi_table *disp) {
   return (_glptr_SamplerParameterIuiv) (GET_by_offset(disp, _gloffset_SamplerParameterIuiv));
}

static inline void SET_SamplerParameterIuiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, const GLuint *)) {
   SET_by_offset(disp, _gloffset_SamplerParameterIuiv, fn);
}

typedef void (GLAPIENTRYP _glptr_SamplerParameterf)(GLuint, GLenum, GLfloat);
#define CALL_SamplerParameterf(disp, parameters) \
    (* GET_SamplerParameterf(disp)) parameters
static inline _glptr_SamplerParameterf GET_SamplerParameterf(struct _glapi_table *disp) {
   return (_glptr_SamplerParameterf) (GET_by_offset(disp, _gloffset_SamplerParameterf));
}

static inline void SET_SamplerParameterf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_SamplerParameterf, fn);
}

typedef void (GLAPIENTRYP _glptr_SamplerParameterfv)(GLuint, GLenum, const GLfloat *);
#define CALL_SamplerParameterfv(disp, parameters) \
    (* GET_SamplerParameterfv(disp)) parameters
static inline _glptr_SamplerParameterfv GET_SamplerParameterfv(struct _glapi_table *disp) {
   return (_glptr_SamplerParameterfv) (GET_by_offset(disp, _gloffset_SamplerParameterfv));
}

static inline void SET_SamplerParameterfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_SamplerParameterfv, fn);
}

typedef void (GLAPIENTRYP _glptr_SamplerParameteri)(GLuint, GLenum, GLint);
#define CALL_SamplerParameteri(disp, parameters) \
    (* GET_SamplerParameteri(disp)) parameters
static inline _glptr_SamplerParameteri GET_SamplerParameteri(struct _glapi_table *disp) {
   return (_glptr_SamplerParameteri) (GET_by_offset(disp, _gloffset_SamplerParameteri));
}

static inline void SET_SamplerParameteri(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_SamplerParameteri, fn);
}

typedef void (GLAPIENTRYP _glptr_SamplerParameteriv)(GLuint, GLenum, const GLint *);
#define CALL_SamplerParameteriv(disp, parameters) \
    (* GET_SamplerParameteriv(disp)) parameters
static inline _glptr_SamplerParameteriv GET_SamplerParameteriv(struct _glapi_table *disp) {
   return (_glptr_SamplerParameteriv) (GET_by_offset(disp, _gloffset_SamplerParameteriv));
}

static inline void SET_SamplerParameteriv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_SamplerParameteriv, fn);
}

typedef void (GLAPIENTRYP _glptr_ColorP3ui)(GLenum, GLuint);
#define CALL_ColorP3ui(disp, parameters) \
    (* GET_ColorP3ui(disp)) parameters
static inline _glptr_ColorP3ui GET_ColorP3ui(struct _glapi_table *disp) {
   return (_glptr_ColorP3ui) (GET_by_offset(disp, _gloffset_ColorP3ui));
}

static inline void SET_ColorP3ui(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_ColorP3ui, fn);
}

typedef void (GLAPIENTRYP _glptr_ColorP3uiv)(GLenum, const GLuint *);
#define CALL_ColorP3uiv(disp, parameters) \
    (* GET_ColorP3uiv(disp)) parameters
static inline _glptr_ColorP3uiv GET_ColorP3uiv(struct _glapi_table *disp) {
   return (_glptr_ColorP3uiv) (GET_by_offset(disp, _gloffset_ColorP3uiv));
}

static inline void SET_ColorP3uiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLuint *)) {
   SET_by_offset(disp, _gloffset_ColorP3uiv, fn);
}

typedef void (GLAPIENTRYP _glptr_ColorP4ui)(GLenum, GLuint);
#define CALL_ColorP4ui(disp, parameters) \
    (* GET_ColorP4ui(disp)) parameters
static inline _glptr_ColorP4ui GET_ColorP4ui(struct _glapi_table *disp) {
   return (_glptr_ColorP4ui) (GET_by_offset(disp, _gloffset_ColorP4ui));
}

static inline void SET_ColorP4ui(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_ColorP4ui, fn);
}

typedef void (GLAPIENTRYP _glptr_ColorP4uiv)(GLenum, const GLuint *);
#define CALL_ColorP4uiv(disp, parameters) \
    (* GET_ColorP4uiv(disp)) parameters
static inline _glptr_ColorP4uiv GET_ColorP4uiv(struct _glapi_table *disp) {
   return (_glptr_ColorP4uiv) (GET_by_offset(disp, _gloffset_ColorP4uiv));
}

static inline void SET_ColorP4uiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLuint *)) {
   SET_by_offset(disp, _gloffset_ColorP4uiv, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoordP1ui)(GLenum, GLenum, GLuint);
#define CALL_MultiTexCoordP1ui(disp, parameters) \
    (* GET_MultiTexCoordP1ui(disp)) parameters
static inline _glptr_MultiTexCoordP1ui GET_MultiTexCoordP1ui(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoordP1ui) (GET_by_offset(disp, _gloffset_MultiTexCoordP1ui));
}

static inline void SET_MultiTexCoordP1ui(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_MultiTexCoordP1ui, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoordP1uiv)(GLenum, GLenum, const GLuint *);
#define CALL_MultiTexCoordP1uiv(disp, parameters) \
    (* GET_MultiTexCoordP1uiv(disp)) parameters
static inline _glptr_MultiTexCoordP1uiv GET_MultiTexCoordP1uiv(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoordP1uiv) (GET_by_offset(disp, _gloffset_MultiTexCoordP1uiv));
}

static inline void SET_MultiTexCoordP1uiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLuint *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoordP1uiv, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoordP2ui)(GLenum, GLenum, GLuint);
#define CALL_MultiTexCoordP2ui(disp, parameters) \
    (* GET_MultiTexCoordP2ui(disp)) parameters
static inline _glptr_MultiTexCoordP2ui GET_MultiTexCoordP2ui(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoordP2ui) (GET_by_offset(disp, _gloffset_MultiTexCoordP2ui));
}

static inline void SET_MultiTexCoordP2ui(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_MultiTexCoordP2ui, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoordP2uiv)(GLenum, GLenum, const GLuint *);
#define CALL_MultiTexCoordP2uiv(disp, parameters) \
    (* GET_MultiTexCoordP2uiv(disp)) parameters
static inline _glptr_MultiTexCoordP2uiv GET_MultiTexCoordP2uiv(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoordP2uiv) (GET_by_offset(disp, _gloffset_MultiTexCoordP2uiv));
}

static inline void SET_MultiTexCoordP2uiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLuint *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoordP2uiv, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoordP3ui)(GLenum, GLenum, GLuint);
#define CALL_MultiTexCoordP3ui(disp, parameters) \
    (* GET_MultiTexCoordP3ui(disp)) parameters
static inline _glptr_MultiTexCoordP3ui GET_MultiTexCoordP3ui(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoordP3ui) (GET_by_offset(disp, _gloffset_MultiTexCoordP3ui));
}

static inline void SET_MultiTexCoordP3ui(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_MultiTexCoordP3ui, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoordP3uiv)(GLenum, GLenum, const GLuint *);
#define CALL_MultiTexCoordP3uiv(disp, parameters) \
    (* GET_MultiTexCoordP3uiv(disp)) parameters
static inline _glptr_MultiTexCoordP3uiv GET_MultiTexCoordP3uiv(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoordP3uiv) (GET_by_offset(disp, _gloffset_MultiTexCoordP3uiv));
}

static inline void SET_MultiTexCoordP3uiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLuint *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoordP3uiv, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoordP4ui)(GLenum, GLenum, GLuint);
#define CALL_MultiTexCoordP4ui(disp, parameters) \
    (* GET_MultiTexCoordP4ui(disp)) parameters
static inline _glptr_MultiTexCoordP4ui GET_MultiTexCoordP4ui(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoordP4ui) (GET_by_offset(disp, _gloffset_MultiTexCoordP4ui));
}

static inline void SET_MultiTexCoordP4ui(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_MultiTexCoordP4ui, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiTexCoordP4uiv)(GLenum, GLenum, const GLuint *);
#define CALL_MultiTexCoordP4uiv(disp, parameters) \
    (* GET_MultiTexCoordP4uiv(disp)) parameters
static inline _glptr_MultiTexCoordP4uiv GET_MultiTexCoordP4uiv(struct _glapi_table *disp) {
   return (_glptr_MultiTexCoordP4uiv) (GET_by_offset(disp, _gloffset_MultiTexCoordP4uiv));
}

static inline void SET_MultiTexCoordP4uiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLuint *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoordP4uiv, fn);
}

typedef void (GLAPIENTRYP _glptr_NormalP3ui)(GLenum, GLuint);
#define CALL_NormalP3ui(disp, parameters) \
    (* GET_NormalP3ui(disp)) parameters
static inline _glptr_NormalP3ui GET_NormalP3ui(struct _glapi_table *disp) {
   return (_glptr_NormalP3ui) (GET_by_offset(disp, _gloffset_NormalP3ui));
}

static inline void SET_NormalP3ui(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_NormalP3ui, fn);
}

typedef void (GLAPIENTRYP _glptr_NormalP3uiv)(GLenum, const GLuint *);
#define CALL_NormalP3uiv(disp, parameters) \
    (* GET_NormalP3uiv(disp)) parameters
static inline _glptr_NormalP3uiv GET_NormalP3uiv(struct _glapi_table *disp) {
   return (_glptr_NormalP3uiv) (GET_by_offset(disp, _gloffset_NormalP3uiv));
}

static inline void SET_NormalP3uiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLuint *)) {
   SET_by_offset(disp, _gloffset_NormalP3uiv, fn);
}

typedef void (GLAPIENTRYP _glptr_SecondaryColorP3ui)(GLenum, GLuint);
#define CALL_SecondaryColorP3ui(disp, parameters) \
    (* GET_SecondaryColorP3ui(disp)) parameters
static inline _glptr_SecondaryColorP3ui GET_SecondaryColorP3ui(struct _glapi_table *disp) {
   return (_glptr_SecondaryColorP3ui) (GET_by_offset(disp, _gloffset_SecondaryColorP3ui));
}

static inline void SET_SecondaryColorP3ui(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_SecondaryColorP3ui, fn);
}

typedef void (GLAPIENTRYP _glptr_SecondaryColorP3uiv)(GLenum, const GLuint *);
#define CALL_SecondaryColorP3uiv(disp, parameters) \
    (* GET_SecondaryColorP3uiv(disp)) parameters
static inline _glptr_SecondaryColorP3uiv GET_SecondaryColorP3uiv(struct _glapi_table *disp) {
   return (_glptr_SecondaryColorP3uiv) (GET_by_offset(disp, _gloffset_SecondaryColorP3uiv));
}

static inline void SET_SecondaryColorP3uiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLuint *)) {
   SET_by_offset(disp, _gloffset_SecondaryColorP3uiv, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoordP1ui)(GLenum, GLuint);
#define CALL_TexCoordP1ui(disp, parameters) \
    (* GET_TexCoordP1ui(disp)) parameters
static inline _glptr_TexCoordP1ui GET_TexCoordP1ui(struct _glapi_table *disp) {
   return (_glptr_TexCoordP1ui) (GET_by_offset(disp, _gloffset_TexCoordP1ui));
}

static inline void SET_TexCoordP1ui(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_TexCoordP1ui, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoordP1uiv)(GLenum, const GLuint *);
#define CALL_TexCoordP1uiv(disp, parameters) \
    (* GET_TexCoordP1uiv(disp)) parameters
static inline _glptr_TexCoordP1uiv GET_TexCoordP1uiv(struct _glapi_table *disp) {
   return (_glptr_TexCoordP1uiv) (GET_by_offset(disp, _gloffset_TexCoordP1uiv));
}

static inline void SET_TexCoordP1uiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLuint *)) {
   SET_by_offset(disp, _gloffset_TexCoordP1uiv, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoordP2ui)(GLenum, GLuint);
#define CALL_TexCoordP2ui(disp, parameters) \
    (* GET_TexCoordP2ui(disp)) parameters
static inline _glptr_TexCoordP2ui GET_TexCoordP2ui(struct _glapi_table *disp) {
   return (_glptr_TexCoordP2ui) (GET_by_offset(disp, _gloffset_TexCoordP2ui));
}

static inline void SET_TexCoordP2ui(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_TexCoordP2ui, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoordP2uiv)(GLenum, const GLuint *);
#define CALL_TexCoordP2uiv(disp, parameters) \
    (* GET_TexCoordP2uiv(disp)) parameters
static inline _glptr_TexCoordP2uiv GET_TexCoordP2uiv(struct _glapi_table *disp) {
   return (_glptr_TexCoordP2uiv) (GET_by_offset(disp, _gloffset_TexCoordP2uiv));
}

static inline void SET_TexCoordP2uiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLuint *)) {
   SET_by_offset(disp, _gloffset_TexCoordP2uiv, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoordP3ui)(GLenum, GLuint);
#define CALL_TexCoordP3ui(disp, parameters) \
    (* GET_TexCoordP3ui(disp)) parameters
static inline _glptr_TexCoordP3ui GET_TexCoordP3ui(struct _glapi_table *disp) {
   return (_glptr_TexCoordP3ui) (GET_by_offset(disp, _gloffset_TexCoordP3ui));
}

static inline void SET_TexCoordP3ui(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_TexCoordP3ui, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoordP3uiv)(GLenum, const GLuint *);
#define CALL_TexCoordP3uiv(disp, parameters) \
    (* GET_TexCoordP3uiv(disp)) parameters
static inline _glptr_TexCoordP3uiv GET_TexCoordP3uiv(struct _glapi_table *disp) {
   return (_glptr_TexCoordP3uiv) (GET_by_offset(disp, _gloffset_TexCoordP3uiv));
}

static inline void SET_TexCoordP3uiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLuint *)) {
   SET_by_offset(disp, _gloffset_TexCoordP3uiv, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoordP4ui)(GLenum, GLuint);
#define CALL_TexCoordP4ui(disp, parameters) \
    (* GET_TexCoordP4ui(disp)) parameters
static inline _glptr_TexCoordP4ui GET_TexCoordP4ui(struct _glapi_table *disp) {
   return (_glptr_TexCoordP4ui) (GET_by_offset(disp, _gloffset_TexCoordP4ui));
}

static inline void SET_TexCoordP4ui(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_TexCoordP4ui, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoordP4uiv)(GLenum, const GLuint *);
#define CALL_TexCoordP4uiv(disp, parameters) \
    (* GET_TexCoordP4uiv(disp)) parameters
static inline _glptr_TexCoordP4uiv GET_TexCoordP4uiv(struct _glapi_table *disp) {
   return (_glptr_TexCoordP4uiv) (GET_by_offset(disp, _gloffset_TexCoordP4uiv));
}

static inline void SET_TexCoordP4uiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLuint *)) {
   SET_by_offset(disp, _gloffset_TexCoordP4uiv, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribP1ui)(GLuint, GLenum, GLboolean, GLuint);
#define CALL_VertexAttribP1ui(disp, parameters) \
    (* GET_VertexAttribP1ui(disp)) parameters
static inline _glptr_VertexAttribP1ui GET_VertexAttribP1ui(struct _glapi_table *disp) {
   return (_glptr_VertexAttribP1ui) (GET_by_offset(disp, _gloffset_VertexAttribP1ui));
}

static inline void SET_VertexAttribP1ui(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLboolean, GLuint)) {
   SET_by_offset(disp, _gloffset_VertexAttribP1ui, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribP1uiv)(GLuint, GLenum, GLboolean, const GLuint *);
#define CALL_VertexAttribP1uiv(disp, parameters) \
    (* GET_VertexAttribP1uiv(disp)) parameters
static inline _glptr_VertexAttribP1uiv GET_VertexAttribP1uiv(struct _glapi_table *disp) {
   return (_glptr_VertexAttribP1uiv) (GET_by_offset(disp, _gloffset_VertexAttribP1uiv));
}

static inline void SET_VertexAttribP1uiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLboolean, const GLuint *)) {
   SET_by_offset(disp, _gloffset_VertexAttribP1uiv, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribP2ui)(GLuint, GLenum, GLboolean, GLuint);
#define CALL_VertexAttribP2ui(disp, parameters) \
    (* GET_VertexAttribP2ui(disp)) parameters
static inline _glptr_VertexAttribP2ui GET_VertexAttribP2ui(struct _glapi_table *disp) {
   return (_glptr_VertexAttribP2ui) (GET_by_offset(disp, _gloffset_VertexAttribP2ui));
}

static inline void SET_VertexAttribP2ui(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLboolean, GLuint)) {
   SET_by_offset(disp, _gloffset_VertexAttribP2ui, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribP2uiv)(GLuint, GLenum, GLboolean, const GLuint *);
#define CALL_VertexAttribP2uiv(disp, parameters) \
    (* GET_VertexAttribP2uiv(disp)) parameters
static inline _glptr_VertexAttribP2uiv GET_VertexAttribP2uiv(struct _glapi_table *disp) {
   return (_glptr_VertexAttribP2uiv) (GET_by_offset(disp, _gloffset_VertexAttribP2uiv));
}

static inline void SET_VertexAttribP2uiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLboolean, const GLuint *)) {
   SET_by_offset(disp, _gloffset_VertexAttribP2uiv, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribP3ui)(GLuint, GLenum, GLboolean, GLuint);
#define CALL_VertexAttribP3ui(disp, parameters) \
    (* GET_VertexAttribP3ui(disp)) parameters
static inline _glptr_VertexAttribP3ui GET_VertexAttribP3ui(struct _glapi_table *disp) {
   return (_glptr_VertexAttribP3ui) (GET_by_offset(disp, _gloffset_VertexAttribP3ui));
}

static inline void SET_VertexAttribP3ui(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLboolean, GLuint)) {
   SET_by_offset(disp, _gloffset_VertexAttribP3ui, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribP3uiv)(GLuint, GLenum, GLboolean, const GLuint *);
#define CALL_VertexAttribP3uiv(disp, parameters) \
    (* GET_VertexAttribP3uiv(disp)) parameters
static inline _glptr_VertexAttribP3uiv GET_VertexAttribP3uiv(struct _glapi_table *disp) {
   return (_glptr_VertexAttribP3uiv) (GET_by_offset(disp, _gloffset_VertexAttribP3uiv));
}

static inline void SET_VertexAttribP3uiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLboolean, const GLuint *)) {
   SET_by_offset(disp, _gloffset_VertexAttribP3uiv, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribP4ui)(GLuint, GLenum, GLboolean, GLuint);
#define CALL_VertexAttribP4ui(disp, parameters) \
    (* GET_VertexAttribP4ui(disp)) parameters
static inline _glptr_VertexAttribP4ui GET_VertexAttribP4ui(struct _glapi_table *disp) {
   return (_glptr_VertexAttribP4ui) (GET_by_offset(disp, _gloffset_VertexAttribP4ui));
}

static inline void SET_VertexAttribP4ui(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLboolean, GLuint)) {
   SET_by_offset(disp, _gloffset_VertexAttribP4ui, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribP4uiv)(GLuint, GLenum, GLboolean, const GLuint *);
#define CALL_VertexAttribP4uiv(disp, parameters) \
    (* GET_VertexAttribP4uiv(disp)) parameters
static inline _glptr_VertexAttribP4uiv GET_VertexAttribP4uiv(struct _glapi_table *disp) {
   return (_glptr_VertexAttribP4uiv) (GET_by_offset(disp, _gloffset_VertexAttribP4uiv));
}

static inline void SET_VertexAttribP4uiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLboolean, const GLuint *)) {
   SET_by_offset(disp, _gloffset_VertexAttribP4uiv, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexP2ui)(GLenum, GLuint);
#define CALL_VertexP2ui(disp, parameters) \
    (* GET_VertexP2ui(disp)) parameters
static inline _glptr_VertexP2ui GET_VertexP2ui(struct _glapi_table *disp) {
   return (_glptr_VertexP2ui) (GET_by_offset(disp, _gloffset_VertexP2ui));
}

static inline void SET_VertexP2ui(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_VertexP2ui, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexP2uiv)(GLenum, const GLuint *);
#define CALL_VertexP2uiv(disp, parameters) \
    (* GET_VertexP2uiv(disp)) parameters
static inline _glptr_VertexP2uiv GET_VertexP2uiv(struct _glapi_table *disp) {
   return (_glptr_VertexP2uiv) (GET_by_offset(disp, _gloffset_VertexP2uiv));
}

static inline void SET_VertexP2uiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLuint *)) {
   SET_by_offset(disp, _gloffset_VertexP2uiv, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexP3ui)(GLenum, GLuint);
#define CALL_VertexP3ui(disp, parameters) \
    (* GET_VertexP3ui(disp)) parameters
static inline _glptr_VertexP3ui GET_VertexP3ui(struct _glapi_table *disp) {
   return (_glptr_VertexP3ui) (GET_by_offset(disp, _gloffset_VertexP3ui));
}

static inline void SET_VertexP3ui(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_VertexP3ui, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexP3uiv)(GLenum, const GLuint *);
#define CALL_VertexP3uiv(disp, parameters) \
    (* GET_VertexP3uiv(disp)) parameters
static inline _glptr_VertexP3uiv GET_VertexP3uiv(struct _glapi_table *disp) {
   return (_glptr_VertexP3uiv) (GET_by_offset(disp, _gloffset_VertexP3uiv));
}

static inline void SET_VertexP3uiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLuint *)) {
   SET_by_offset(disp, _gloffset_VertexP3uiv, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexP4ui)(GLenum, GLuint);
#define CALL_VertexP4ui(disp, parameters) \
    (* GET_VertexP4ui(disp)) parameters
static inline _glptr_VertexP4ui GET_VertexP4ui(struct _glapi_table *disp) {
   return (_glptr_VertexP4ui) (GET_by_offset(disp, _gloffset_VertexP4ui));
}

static inline void SET_VertexP4ui(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_VertexP4ui, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexP4uiv)(GLenum, const GLuint *);
#define CALL_VertexP4uiv(disp, parameters) \
    (* GET_VertexP4uiv(disp)) parameters
static inline _glptr_VertexP4uiv GET_VertexP4uiv(struct _glapi_table *disp) {
   return (_glptr_VertexP4uiv) (GET_by_offset(disp, _gloffset_VertexP4uiv));
}

static inline void SET_VertexP4uiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLuint *)) {
   SET_by_offset(disp, _gloffset_VertexP4uiv, fn);
}

typedef void (GLAPIENTRYP _glptr_BindTransformFeedback)(GLenum, GLuint);
#define CALL_BindTransformFeedback(disp, parameters) \
    (* GET_BindTransformFeedback(disp)) parameters
static inline _glptr_BindTransformFeedback GET_BindTransformFeedback(struct _glapi_table *disp) {
   return (_glptr_BindTransformFeedback) (GET_by_offset(disp, _gloffset_BindTransformFeedback));
}

static inline void SET_BindTransformFeedback(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_BindTransformFeedback, fn);
}

typedef void (GLAPIENTRYP _glptr_DeleteTransformFeedbacks)(GLsizei, const GLuint *);
#define CALL_DeleteTransformFeedbacks(disp, parameters) \
    (* GET_DeleteTransformFeedbacks(disp)) parameters
static inline _glptr_DeleteTransformFeedbacks GET_DeleteTransformFeedbacks(struct _glapi_table *disp) {
   return (_glptr_DeleteTransformFeedbacks) (GET_by_offset(disp, _gloffset_DeleteTransformFeedbacks));
}

static inline void SET_DeleteTransformFeedbacks(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_DeleteTransformFeedbacks, fn);
}

typedef void (GLAPIENTRYP _glptr_DrawTransformFeedback)(GLenum, GLuint);
#define CALL_DrawTransformFeedback(disp, parameters) \
    (* GET_DrawTransformFeedback(disp)) parameters
static inline _glptr_DrawTransformFeedback GET_DrawTransformFeedback(struct _glapi_table *disp) {
   return (_glptr_DrawTransformFeedback) (GET_by_offset(disp, _gloffset_DrawTransformFeedback));
}

static inline void SET_DrawTransformFeedback(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_DrawTransformFeedback, fn);
}

typedef void (GLAPIENTRYP _glptr_GenTransformFeedbacks)(GLsizei, GLuint *);
#define CALL_GenTransformFeedbacks(disp, parameters) \
    (* GET_GenTransformFeedbacks(disp)) parameters
static inline _glptr_GenTransformFeedbacks GET_GenTransformFeedbacks(struct _glapi_table *disp) {
   return (_glptr_GenTransformFeedbacks) (GET_by_offset(disp, _gloffset_GenTransformFeedbacks));
}

static inline void SET_GenTransformFeedbacks(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLuint *)) {
   SET_by_offset(disp, _gloffset_GenTransformFeedbacks, fn);
}

typedef GLboolean (GLAPIENTRYP _glptr_IsTransformFeedback)(GLuint);
#define CALL_IsTransformFeedback(disp, parameters) \
    (* GET_IsTransformFeedback(disp)) parameters
static inline _glptr_IsTransformFeedback GET_IsTransformFeedback(struct _glapi_table *disp) {
   return (_glptr_IsTransformFeedback) (GET_by_offset(disp, _gloffset_IsTransformFeedback));
}

static inline void SET_IsTransformFeedback(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_IsTransformFeedback, fn);
}

typedef void (GLAPIENTRYP _glptr_PauseTransformFeedback)(void);
#define CALL_PauseTransformFeedback(disp, parameters) \
    (* GET_PauseTransformFeedback(disp)) parameters
static inline _glptr_PauseTransformFeedback GET_PauseTransformFeedback(struct _glapi_table *disp) {
   return (_glptr_PauseTransformFeedback) (GET_by_offset(disp, _gloffset_PauseTransformFeedback));
}

static inline void SET_PauseTransformFeedback(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_PauseTransformFeedback, fn);
}

typedef void (GLAPIENTRYP _glptr_ResumeTransformFeedback)(void);
#define CALL_ResumeTransformFeedback(disp, parameters) \
    (* GET_ResumeTransformFeedback(disp)) parameters
static inline _glptr_ResumeTransformFeedback GET_ResumeTransformFeedback(struct _glapi_table *disp) {
   return (_glptr_ResumeTransformFeedback) (GET_by_offset(disp, _gloffset_ResumeTransformFeedback));
}

static inline void SET_ResumeTransformFeedback(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_ResumeTransformFeedback, fn);
}

typedef void (GLAPIENTRYP _glptr_ClearDepthf)(GLclampf);
#define CALL_ClearDepthf(disp, parameters) \
    (* GET_ClearDepthf(disp)) parameters
static inline _glptr_ClearDepthf GET_ClearDepthf(struct _glapi_table *disp) {
   return (_glptr_ClearDepthf) (GET_by_offset(disp, _gloffset_ClearDepthf));
}

static inline void SET_ClearDepthf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLclampf)) {
   SET_by_offset(disp, _gloffset_ClearDepthf, fn);
}

typedef void (GLAPIENTRYP _glptr_DepthRangef)(GLclampf, GLclampf);
#define CALL_DepthRangef(disp, parameters) \
    (* GET_DepthRangef(disp)) parameters
static inline _glptr_DepthRangef GET_DepthRangef(struct _glapi_table *disp) {
   return (_glptr_DepthRangef) (GET_by_offset(disp, _gloffset_DepthRangef));
}

static inline void SET_DepthRangef(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLclampf, GLclampf)) {
   SET_by_offset(disp, _gloffset_DepthRangef, fn);
}

typedef void (GLAPIENTRYP _glptr_GetShaderPrecisionFormat)(GLenum, GLenum, GLint *, GLint *);
#define CALL_GetShaderPrecisionFormat(disp, parameters) \
    (* GET_GetShaderPrecisionFormat(disp)) parameters
static inline _glptr_GetShaderPrecisionFormat GET_GetShaderPrecisionFormat(struct _glapi_table *disp) {
   return (_glptr_GetShaderPrecisionFormat) (GET_by_offset(disp, _gloffset_GetShaderPrecisionFormat));
}

static inline void SET_GetShaderPrecisionFormat(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *, GLint *)) {
   SET_by_offset(disp, _gloffset_GetShaderPrecisionFormat, fn);
}

typedef void (GLAPIENTRYP _glptr_ReleaseShaderCompiler)(void);
#define CALL_ReleaseShaderCompiler(disp, parameters) \
    (* GET_ReleaseShaderCompiler(disp)) parameters
static inline _glptr_ReleaseShaderCompiler GET_ReleaseShaderCompiler(struct _glapi_table *disp) {
   return (_glptr_ReleaseShaderCompiler) (GET_by_offset(disp, _gloffset_ReleaseShaderCompiler));
}

static inline void SET_ReleaseShaderCompiler(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_ReleaseShaderCompiler, fn);
}

typedef void (GLAPIENTRYP _glptr_ShaderBinary)(GLsizei, const GLuint *, GLenum, const GLvoid *, GLsizei);
#define CALL_ShaderBinary(disp, parameters) \
    (* GET_ShaderBinary(disp)) parameters
static inline _glptr_ShaderBinary GET_ShaderBinary(struct _glapi_table *disp) {
   return (_glptr_ShaderBinary) (GET_by_offset(disp, _gloffset_ShaderBinary));
}

static inline void SET_ShaderBinary(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLuint *, GLenum, const GLvoid *, GLsizei)) {
   SET_by_offset(disp, _gloffset_ShaderBinary, fn);
}

typedef GLenum (GLAPIENTRYP _glptr_GetGraphicsResetStatusARB)(void);
#define CALL_GetGraphicsResetStatusARB(disp, parameters) \
    (* GET_GetGraphicsResetStatusARB(disp)) parameters
static inline _glptr_GetGraphicsResetStatusARB GET_GetGraphicsResetStatusARB(struct _glapi_table *disp) {
   return (_glptr_GetGraphicsResetStatusARB) (GET_by_offset(disp, _gloffset_GetGraphicsResetStatusARB));
}

static inline void SET_GetGraphicsResetStatusARB(struct _glapi_table *disp, GLenum (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_GetGraphicsResetStatusARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetnColorTableARB)(GLenum, GLenum, GLenum, GLsizei, GLvoid *);
#define CALL_GetnColorTableARB(disp, parameters) \
    (* GET_GetnColorTableARB(disp)) parameters
static inline _glptr_GetnColorTableARB GET_GetnColorTableARB(struct _glapi_table *disp) {
   return (_glptr_GetnColorTableARB) (GET_by_offset(disp, _gloffset_GetnColorTableARB));
}

static inline void SET_GetnColorTableARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLsizei, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetnColorTableARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetnCompressedTexImageARB)(GLenum, GLint, GLsizei, GLvoid *);
#define CALL_GetnCompressedTexImageARB(disp, parameters) \
    (* GET_GetnCompressedTexImageARB(disp)) parameters
static inline _glptr_GetnCompressedTexImageARB GET_GetnCompressedTexImageARB(struct _glapi_table *disp) {
   return (_glptr_GetnCompressedTexImageARB) (GET_by_offset(disp, _gloffset_GetnCompressedTexImageARB));
}

static inline void SET_GetnCompressedTexImageARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLsizei, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetnCompressedTexImageARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetnConvolutionFilterARB)(GLenum, GLenum, GLenum, GLsizei, GLvoid *);
#define CALL_GetnConvolutionFilterARB(disp, parameters) \
    (* GET_GetnConvolutionFilterARB(disp)) parameters
static inline _glptr_GetnConvolutionFilterARB GET_GetnConvolutionFilterARB(struct _glapi_table *disp) {
   return (_glptr_GetnConvolutionFilterARB) (GET_by_offset(disp, _gloffset_GetnConvolutionFilterARB));
}

static inline void SET_GetnConvolutionFilterARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLsizei, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetnConvolutionFilterARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetnHistogramARB)(GLenum, GLboolean, GLenum, GLenum, GLsizei, GLvoid *);
#define CALL_GetnHistogramARB(disp, parameters) \
    (* GET_GetnHistogramARB(disp)) parameters
static inline _glptr_GetnHistogramARB GET_GetnHistogramARB(struct _glapi_table *disp) {
   return (_glptr_GetnHistogramARB) (GET_by_offset(disp, _gloffset_GetnHistogramARB));
}

static inline void SET_GetnHistogramARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLboolean, GLenum, GLenum, GLsizei, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetnHistogramARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetnMapdvARB)(GLenum, GLenum, GLsizei, GLdouble *);
#define CALL_GetnMapdvARB(disp, parameters) \
    (* GET_GetnMapdvARB(disp)) parameters
static inline _glptr_GetnMapdvARB GET_GetnMapdvARB(struct _glapi_table *disp) {
   return (_glptr_GetnMapdvARB) (GET_by_offset(disp, _gloffset_GetnMapdvARB));
}

static inline void SET_GetnMapdvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLsizei, GLdouble *)) {
   SET_by_offset(disp, _gloffset_GetnMapdvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetnMapfvARB)(GLenum, GLenum, GLsizei, GLfloat *);
#define CALL_GetnMapfvARB(disp, parameters) \
    (* GET_GetnMapfvARB(disp)) parameters
static inline _glptr_GetnMapfvARB GET_GetnMapfvARB(struct _glapi_table *disp) {
   return (_glptr_GetnMapfvARB) (GET_by_offset(disp, _gloffset_GetnMapfvARB));
}

static inline void SET_GetnMapfvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLsizei, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetnMapfvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetnMapivARB)(GLenum, GLenum, GLsizei, GLint *);
#define CALL_GetnMapivARB(disp, parameters) \
    (* GET_GetnMapivARB(disp)) parameters
static inline _glptr_GetnMapivARB GET_GetnMapivARB(struct _glapi_table *disp) {
   return (_glptr_GetnMapivARB) (GET_by_offset(disp, _gloffset_GetnMapivARB));
}

static inline void SET_GetnMapivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLsizei, GLint *)) {
   SET_by_offset(disp, _gloffset_GetnMapivARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetnMinmaxARB)(GLenum, GLboolean, GLenum, GLenum, GLsizei, GLvoid *);
#define CALL_GetnMinmaxARB(disp, parameters) \
    (* GET_GetnMinmaxARB(disp)) parameters
static inline _glptr_GetnMinmaxARB GET_GetnMinmaxARB(struct _glapi_table *disp) {
   return (_glptr_GetnMinmaxARB) (GET_by_offset(disp, _gloffset_GetnMinmaxARB));
}

static inline void SET_GetnMinmaxARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLboolean, GLenum, GLenum, GLsizei, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetnMinmaxARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetnPixelMapfvARB)(GLenum, GLsizei, GLfloat *);
#define CALL_GetnPixelMapfvARB(disp, parameters) \
    (* GET_GetnPixelMapfvARB(disp)) parameters
static inline _glptr_GetnPixelMapfvARB GET_GetnPixelMapfvARB(struct _glapi_table *disp) {
   return (_glptr_GetnPixelMapfvARB) (GET_by_offset(disp, _gloffset_GetnPixelMapfvARB));
}

static inline void SET_GetnPixelMapfvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetnPixelMapfvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetnPixelMapuivARB)(GLenum, GLsizei, GLuint *);
#define CALL_GetnPixelMapuivARB(disp, parameters) \
    (* GET_GetnPixelMapuivARB(disp)) parameters
static inline _glptr_GetnPixelMapuivARB GET_GetnPixelMapuivARB(struct _glapi_table *disp) {
   return (_glptr_GetnPixelMapuivARB) (GET_by_offset(disp, _gloffset_GetnPixelMapuivARB));
}

static inline void SET_GetnPixelMapuivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLuint *)) {
   SET_by_offset(disp, _gloffset_GetnPixelMapuivARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetnPixelMapusvARB)(GLenum, GLsizei, GLushort *);
#define CALL_GetnPixelMapusvARB(disp, parameters) \
    (* GET_GetnPixelMapusvARB(disp)) parameters
static inline _glptr_GetnPixelMapusvARB GET_GetnPixelMapusvARB(struct _glapi_table *disp) {
   return (_glptr_GetnPixelMapusvARB) (GET_by_offset(disp, _gloffset_GetnPixelMapusvARB));
}

static inline void SET_GetnPixelMapusvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLushort *)) {
   SET_by_offset(disp, _gloffset_GetnPixelMapusvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetnPolygonStippleARB)(GLsizei, GLubyte *);
#define CALL_GetnPolygonStippleARB(disp, parameters) \
    (* GET_GetnPolygonStippleARB(disp)) parameters
static inline _glptr_GetnPolygonStippleARB GET_GetnPolygonStippleARB(struct _glapi_table *disp) {
   return (_glptr_GetnPolygonStippleARB) (GET_by_offset(disp, _gloffset_GetnPolygonStippleARB));
}

static inline void SET_GetnPolygonStippleARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLubyte *)) {
   SET_by_offset(disp, _gloffset_GetnPolygonStippleARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetnSeparableFilterARB)(GLenum, GLenum, GLenum, GLsizei, GLvoid *, GLsizei, GLvoid *, GLvoid *);
#define CALL_GetnSeparableFilterARB(disp, parameters) \
    (* GET_GetnSeparableFilterARB(disp)) parameters
static inline _glptr_GetnSeparableFilterARB GET_GetnSeparableFilterARB(struct _glapi_table *disp) {
   return (_glptr_GetnSeparableFilterARB) (GET_by_offset(disp, _gloffset_GetnSeparableFilterARB));
}

static inline void SET_GetnSeparableFilterARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLsizei, GLvoid *, GLsizei, GLvoid *, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetnSeparableFilterARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetnTexImageARB)(GLenum, GLint, GLenum, GLenum, GLsizei, GLvoid *);
#define CALL_GetnTexImageARB(disp, parameters) \
    (* GET_GetnTexImageARB(disp)) parameters
static inline _glptr_GetnTexImageARB GET_GetnTexImageARB(struct _glapi_table *disp) {
   return (_glptr_GetnTexImageARB) (GET_by_offset(disp, _gloffset_GetnTexImageARB));
}

static inline void SET_GetnTexImageARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLenum, GLenum, GLsizei, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetnTexImageARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetnUniformdvARB)(GLhandleARB, GLint, GLsizei, GLdouble *);
#define CALL_GetnUniformdvARB(disp, parameters) \
    (* GET_GetnUniformdvARB(disp)) parameters
static inline _glptr_GetnUniformdvARB GET_GetnUniformdvARB(struct _glapi_table *disp) {
   return (_glptr_GetnUniformdvARB) (GET_by_offset(disp, _gloffset_GetnUniformdvARB));
}

static inline void SET_GetnUniformdvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLint, GLsizei, GLdouble *)) {
   SET_by_offset(disp, _gloffset_GetnUniformdvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetnUniformfvARB)(GLhandleARB, GLint, GLsizei, GLfloat *);
#define CALL_GetnUniformfvARB(disp, parameters) \
    (* GET_GetnUniformfvARB(disp)) parameters
static inline _glptr_GetnUniformfvARB GET_GetnUniformfvARB(struct _glapi_table *disp) {
   return (_glptr_GetnUniformfvARB) (GET_by_offset(disp, _gloffset_GetnUniformfvARB));
}

static inline void SET_GetnUniformfvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLint, GLsizei, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetnUniformfvARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetnUniformivARB)(GLhandleARB, GLint, GLsizei, GLint *);
#define CALL_GetnUniformivARB(disp, parameters) \
    (* GET_GetnUniformivARB(disp)) parameters
static inline _glptr_GetnUniformivARB GET_GetnUniformivARB(struct _glapi_table *disp) {
   return (_glptr_GetnUniformivARB) (GET_by_offset(disp, _gloffset_GetnUniformivARB));
}

static inline void SET_GetnUniformivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLint, GLsizei, GLint *)) {
   SET_by_offset(disp, _gloffset_GetnUniformivARB, fn);
}

typedef void (GLAPIENTRYP _glptr_GetnUniformuivARB)(GLhandleARB, GLint, GLsizei, GLuint *);
#define CALL_GetnUniformuivARB(disp, parameters) \
    (* GET_GetnUniformuivARB(disp)) parameters
static inline _glptr_GetnUniformuivARB GET_GetnUniformuivARB(struct _glapi_table *disp) {
   return (_glptr_GetnUniformuivARB) (GET_by_offset(disp, _gloffset_GetnUniformuivARB));
}

static inline void SET_GetnUniformuivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLint, GLsizei, GLuint *)) {
   SET_by_offset(disp, _gloffset_GetnUniformuivARB, fn);
}

typedef void (GLAPIENTRYP _glptr_ReadnPixelsARB)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLsizei, GLvoid *);
#define CALL_ReadnPixelsARB(disp, parameters) \
    (* GET_ReadnPixelsARB(disp)) parameters
static inline _glptr_ReadnPixelsARB GET_ReadnPixelsARB(struct _glapi_table *disp) {
   return (_glptr_ReadnPixelsARB) (GET_by_offset(disp, _gloffset_ReadnPixelsARB));
}

static inline void SET_ReadnPixelsARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLsizei, GLvoid *)) {
   SET_by_offset(disp, _gloffset_ReadnPixelsARB, fn);
}

typedef void (GLAPIENTRYP _glptr_TexStorage1D)(GLenum, GLsizei, GLenum, GLsizei);
#define CALL_TexStorage1D(disp, parameters) \
    (* GET_TexStorage1D(disp)) parameters
static inline _glptr_TexStorage1D GET_TexStorage1D(struct _glapi_table *disp) {
   return (_glptr_TexStorage1D) (GET_by_offset(disp, _gloffset_TexStorage1D));
}

static inline void SET_TexStorage1D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLenum, GLsizei)) {
   SET_by_offset(disp, _gloffset_TexStorage1D, fn);
}

typedef void (GLAPIENTRYP _glptr_TexStorage2D)(GLenum, GLsizei, GLenum, GLsizei, GLsizei);
#define CALL_TexStorage2D(disp, parameters) \
    (* GET_TexStorage2D(disp)) parameters
static inline _glptr_TexStorage2D GET_TexStorage2D(struct _glapi_table *disp) {
   return (_glptr_TexStorage2D) (GET_by_offset(disp, _gloffset_TexStorage2D));
}

static inline void SET_TexStorage2D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLenum, GLsizei, GLsizei)) {
   SET_by_offset(disp, _gloffset_TexStorage2D, fn);
}

typedef void (GLAPIENTRYP _glptr_TexStorage3D)(GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLsizei);
#define CALL_TexStorage3D(disp, parameters) \
    (* GET_TexStorage3D(disp)) parameters
static inline _glptr_TexStorage3D GET_TexStorage3D(struct _glapi_table *disp) {
   return (_glptr_TexStorage3D) (GET_by_offset(disp, _gloffset_TexStorage3D));
}

static inline void SET_TexStorage3D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLsizei)) {
   SET_by_offset(disp, _gloffset_TexStorage3D, fn);
}

typedef void (GLAPIENTRYP _glptr_TextureStorage1DEXT)(GLuint, GLenum, GLsizei, GLenum, GLsizei);
#define CALL_TextureStorage1DEXT(disp, parameters) \
    (* GET_TextureStorage1DEXT(disp)) parameters
static inline _glptr_TextureStorage1DEXT GET_TextureStorage1DEXT(struct _glapi_table *disp) {
   return (_glptr_TextureStorage1DEXT) (GET_by_offset(disp, _gloffset_TextureStorage1DEXT));
}

static inline void SET_TextureStorage1DEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLsizei, GLenum, GLsizei)) {
   SET_by_offset(disp, _gloffset_TextureStorage1DEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_TextureStorage2DEXT)(GLuint, GLenum, GLsizei, GLenum, GLsizei, GLsizei);
#define CALL_TextureStorage2DEXT(disp, parameters) \
    (* GET_TextureStorage2DEXT(disp)) parameters
static inline _glptr_TextureStorage2DEXT GET_TextureStorage2DEXT(struct _glapi_table *disp) {
   return (_glptr_TextureStorage2DEXT) (GET_by_offset(disp, _gloffset_TextureStorage2DEXT));
}

static inline void SET_TextureStorage2DEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLsizei, GLenum, GLsizei, GLsizei)) {
   SET_by_offset(disp, _gloffset_TextureStorage2DEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_TextureStorage3DEXT)(GLuint, GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLsizei);
#define CALL_TextureStorage3DEXT(disp, parameters) \
    (* GET_TextureStorage3DEXT(disp)) parameters
static inline _glptr_TextureStorage3DEXT GET_TextureStorage3DEXT(struct _glapi_table *disp) {
   return (_glptr_TextureStorage3DEXT) (GET_by_offset(disp, _gloffset_TextureStorage3DEXT));
}

static inline void SET_TextureStorage3DEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLsizei)) {
   SET_by_offset(disp, _gloffset_TextureStorage3DEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_PolygonOffsetEXT)(GLfloat, GLfloat);
#define CALL_PolygonOffsetEXT(disp, parameters) \
    (* GET_PolygonOffsetEXT(disp)) parameters
static inline _glptr_PolygonOffsetEXT GET_PolygonOffsetEXT(struct _glapi_table *disp) {
   return (_glptr_PolygonOffsetEXT) (GET_by_offset(disp, _gloffset_PolygonOffsetEXT));
}

static inline void SET_PolygonOffsetEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_PolygonOffsetEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_GetPixelTexGenParameterfvSGIS)(GLenum, GLfloat *);
#define CALL_GetPixelTexGenParameterfvSGIS(disp, parameters) \
    (* GET_GetPixelTexGenParameterfvSGIS(disp)) parameters
static inline _glptr_GetPixelTexGenParameterfvSGIS GET_GetPixelTexGenParameterfvSGIS(struct _glapi_table *disp) {
   return (_glptr_GetPixelTexGenParameterfvSGIS) (GET_by_offset(disp, _gloffset_GetPixelTexGenParameterfvSGIS));
}

static inline void SET_GetPixelTexGenParameterfvSGIS(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetPixelTexGenParameterfvSGIS, fn);
}

typedef void (GLAPIENTRYP _glptr_GetPixelTexGenParameterivSGIS)(GLenum, GLint *);
#define CALL_GetPixelTexGenParameterivSGIS(disp, parameters) \
    (* GET_GetPixelTexGenParameterivSGIS(disp)) parameters
static inline _glptr_GetPixelTexGenParameterivSGIS GET_GetPixelTexGenParameterivSGIS(struct _glapi_table *disp) {
   return (_glptr_GetPixelTexGenParameterivSGIS) (GET_by_offset(disp, _gloffset_GetPixelTexGenParameterivSGIS));
}

static inline void SET_GetPixelTexGenParameterivSGIS(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetPixelTexGenParameterivSGIS, fn);
}

typedef void (GLAPIENTRYP _glptr_PixelTexGenParameterfSGIS)(GLenum, GLfloat);
#define CALL_PixelTexGenParameterfSGIS(disp, parameters) \
    (* GET_PixelTexGenParameterfSGIS(disp)) parameters
static inline _glptr_PixelTexGenParameterfSGIS GET_PixelTexGenParameterfSGIS(struct _glapi_table *disp) {
   return (_glptr_PixelTexGenParameterfSGIS) (GET_by_offset(disp, _gloffset_PixelTexGenParameterfSGIS));
}

static inline void SET_PixelTexGenParameterfSGIS(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_PixelTexGenParameterfSGIS, fn);
}

typedef void (GLAPIENTRYP _glptr_PixelTexGenParameterfvSGIS)(GLenum, const GLfloat *);
#define CALL_PixelTexGenParameterfvSGIS(disp, parameters) \
    (* GET_PixelTexGenParameterfvSGIS(disp)) parameters
static inline _glptr_PixelTexGenParameterfvSGIS GET_PixelTexGenParameterfvSGIS(struct _glapi_table *disp) {
   return (_glptr_PixelTexGenParameterfvSGIS) (GET_by_offset(disp, _gloffset_PixelTexGenParameterfvSGIS));
}

static inline void SET_PixelTexGenParameterfvSGIS(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_PixelTexGenParameterfvSGIS, fn);
}

typedef void (GLAPIENTRYP _glptr_PixelTexGenParameteriSGIS)(GLenum, GLint);
#define CALL_PixelTexGenParameteriSGIS(disp, parameters) \
    (* GET_PixelTexGenParameteriSGIS(disp)) parameters
static inline _glptr_PixelTexGenParameteriSGIS GET_PixelTexGenParameteriSGIS(struct _glapi_table *disp) {
   return (_glptr_PixelTexGenParameteriSGIS) (GET_by_offset(disp, _gloffset_PixelTexGenParameteriSGIS));
}

static inline void SET_PixelTexGenParameteriSGIS(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_PixelTexGenParameteriSGIS, fn);
}

typedef void (GLAPIENTRYP _glptr_PixelTexGenParameterivSGIS)(GLenum, const GLint *);
#define CALL_PixelTexGenParameterivSGIS(disp, parameters) \
    (* GET_PixelTexGenParameterivSGIS(disp)) parameters
static inline _glptr_PixelTexGenParameterivSGIS GET_PixelTexGenParameterivSGIS(struct _glapi_table *disp) {
   return (_glptr_PixelTexGenParameterivSGIS) (GET_by_offset(disp, _gloffset_PixelTexGenParameterivSGIS));
}

static inline void SET_PixelTexGenParameterivSGIS(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_PixelTexGenParameterivSGIS, fn);
}

typedef void (GLAPIENTRYP _glptr_SampleMaskSGIS)(GLclampf, GLboolean);
#define CALL_SampleMaskSGIS(disp, parameters) \
    (* GET_SampleMaskSGIS(disp)) parameters
static inline _glptr_SampleMaskSGIS GET_SampleMaskSGIS(struct _glapi_table *disp) {
   return (_glptr_SampleMaskSGIS) (GET_by_offset(disp, _gloffset_SampleMaskSGIS));
}

static inline void SET_SampleMaskSGIS(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLclampf, GLboolean)) {
   SET_by_offset(disp, _gloffset_SampleMaskSGIS, fn);
}

typedef void (GLAPIENTRYP _glptr_SamplePatternSGIS)(GLenum);
#define CALL_SamplePatternSGIS(disp, parameters) \
    (* GET_SamplePatternSGIS(disp)) parameters
static inline _glptr_SamplePatternSGIS GET_SamplePatternSGIS(struct _glapi_table *disp) {
   return (_glptr_SamplePatternSGIS) (GET_by_offset(disp, _gloffset_SamplePatternSGIS));
}

static inline void SET_SamplePatternSGIS(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_SamplePatternSGIS, fn);
}

typedef void (GLAPIENTRYP _glptr_ColorPointerEXT)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *);
#define CALL_ColorPointerEXT(disp, parameters) \
    (* GET_ColorPointerEXT(disp)) parameters
static inline _glptr_ColorPointerEXT GET_ColorPointerEXT(struct _glapi_table *disp) {
   return (_glptr_ColorPointerEXT) (GET_by_offset(disp, _gloffset_ColorPointerEXT));
}

static inline void SET_ColorPointerEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_ColorPointerEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_EdgeFlagPointerEXT)(GLsizei, GLsizei, const GLboolean *);
#define CALL_EdgeFlagPointerEXT(disp, parameters) \
    (* GET_EdgeFlagPointerEXT(disp)) parameters
static inline _glptr_EdgeFlagPointerEXT GET_EdgeFlagPointerEXT(struct _glapi_table *disp) {
   return (_glptr_EdgeFlagPointerEXT) (GET_by_offset(disp, _gloffset_EdgeFlagPointerEXT));
}

static inline void SET_EdgeFlagPointerEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLsizei, const GLboolean *)) {
   SET_by_offset(disp, _gloffset_EdgeFlagPointerEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_IndexPointerEXT)(GLenum, GLsizei, GLsizei, const GLvoid *);
#define CALL_IndexPointerEXT(disp, parameters) \
    (* GET_IndexPointerEXT(disp)) parameters
static inline _glptr_IndexPointerEXT GET_IndexPointerEXT(struct _glapi_table *disp) {
   return (_glptr_IndexPointerEXT) (GET_by_offset(disp, _gloffset_IndexPointerEXT));
}

static inline void SET_IndexPointerEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_IndexPointerEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_NormalPointerEXT)(GLenum, GLsizei, GLsizei, const GLvoid *);
#define CALL_NormalPointerEXT(disp, parameters) \
    (* GET_NormalPointerEXT(disp)) parameters
static inline _glptr_NormalPointerEXT GET_NormalPointerEXT(struct _glapi_table *disp) {
   return (_glptr_NormalPointerEXT) (GET_by_offset(disp, _gloffset_NormalPointerEXT));
}

static inline void SET_NormalPointerEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_NormalPointerEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_TexCoordPointerEXT)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *);
#define CALL_TexCoordPointerEXT(disp, parameters) \
    (* GET_TexCoordPointerEXT(disp)) parameters
static inline _glptr_TexCoordPointerEXT GET_TexCoordPointerEXT(struct _glapi_table *disp) {
   return (_glptr_TexCoordPointerEXT) (GET_by_offset(disp, _gloffset_TexCoordPointerEXT));
}

static inline void SET_TexCoordPointerEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_TexCoordPointerEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexPointerEXT)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *);
#define CALL_VertexPointerEXT(disp, parameters) \
    (* GET_VertexPointerEXT(disp)) parameters
static inline _glptr_VertexPointerEXT GET_VertexPointerEXT(struct _glapi_table *disp) {
   return (_glptr_VertexPointerEXT) (GET_by_offset(disp, _gloffset_VertexPointerEXT));
}

static inline void SET_VertexPointerEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_VertexPointerEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_PointParameterfEXT)(GLenum, GLfloat);
#define CALL_PointParameterfEXT(disp, parameters) \
    (* GET_PointParameterfEXT(disp)) parameters
static inline _glptr_PointParameterfEXT GET_PointParameterfEXT(struct _glapi_table *disp) {
   return (_glptr_PointParameterfEXT) (GET_by_offset(disp, _gloffset_PointParameterfEXT));
}

static inline void SET_PointParameterfEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_PointParameterfEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_PointParameterfvEXT)(GLenum, const GLfloat *);
#define CALL_PointParameterfvEXT(disp, parameters) \
    (* GET_PointParameterfvEXT(disp)) parameters
static inline _glptr_PointParameterfvEXT GET_PointParameterfvEXT(struct _glapi_table *disp) {
   return (_glptr_PointParameterfvEXT) (GET_by_offset(disp, _gloffset_PointParameterfvEXT));
}

static inline void SET_PointParameterfvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_PointParameterfvEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_LockArraysEXT)(GLint, GLsizei);
#define CALL_LockArraysEXT(disp, parameters) \
    (* GET_LockArraysEXT(disp)) parameters
static inline _glptr_LockArraysEXT GET_LockArraysEXT(struct _glapi_table *disp) {
   return (_glptr_LockArraysEXT) (GET_by_offset(disp, _gloffset_LockArraysEXT));
}

static inline void SET_LockArraysEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei)) {
   SET_by_offset(disp, _gloffset_LockArraysEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_UnlockArraysEXT)(void);
#define CALL_UnlockArraysEXT(disp, parameters) \
    (* GET_UnlockArraysEXT(disp)) parameters
static inline _glptr_UnlockArraysEXT GET_UnlockArraysEXT(struct _glapi_table *disp) {
   return (_glptr_UnlockArraysEXT) (GET_by_offset(disp, _gloffset_UnlockArraysEXT));
}

static inline void SET_UnlockArraysEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_UnlockArraysEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_SecondaryColor3bEXT)(GLbyte, GLbyte, GLbyte);
#define CALL_SecondaryColor3bEXT(disp, parameters) \
    (* GET_SecondaryColor3bEXT(disp)) parameters
static inline _glptr_SecondaryColor3bEXT GET_SecondaryColor3bEXT(struct _glapi_table *disp) {
   return (_glptr_SecondaryColor3bEXT) (GET_by_offset(disp, _gloffset_SecondaryColor3bEXT));
}

static inline void SET_SecondaryColor3bEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLbyte, GLbyte, GLbyte)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3bEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_SecondaryColor3bvEXT)(const GLbyte *);
#define CALL_SecondaryColor3bvEXT(disp, parameters) \
    (* GET_SecondaryColor3bvEXT(disp)) parameters
static inline _glptr_SecondaryColor3bvEXT GET_SecondaryColor3bvEXT(struct _glapi_table *disp) {
   return (_glptr_SecondaryColor3bvEXT) (GET_by_offset(disp, _gloffset_SecondaryColor3bvEXT));
}

static inline void SET_SecondaryColor3bvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLbyte *)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3bvEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_SecondaryColor3dEXT)(GLdouble, GLdouble, GLdouble);
#define CALL_SecondaryColor3dEXT(disp, parameters) \
    (* GET_SecondaryColor3dEXT(disp)) parameters
static inline _glptr_SecondaryColor3dEXT GET_SecondaryColor3dEXT(struct _glapi_table *disp) {
   return (_glptr_SecondaryColor3dEXT) (GET_by_offset(disp, _gloffset_SecondaryColor3dEXT));
}

static inline void SET_SecondaryColor3dEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3dEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_SecondaryColor3dvEXT)(const GLdouble *);
#define CALL_SecondaryColor3dvEXT(disp, parameters) \
    (* GET_SecondaryColor3dvEXT(disp)) parameters
static inline _glptr_SecondaryColor3dvEXT GET_SecondaryColor3dvEXT(struct _glapi_table *disp) {
   return (_glptr_SecondaryColor3dvEXT) (GET_by_offset(disp, _gloffset_SecondaryColor3dvEXT));
}

static inline void SET_SecondaryColor3dvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3dvEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_SecondaryColor3fEXT)(GLfloat, GLfloat, GLfloat);
#define CALL_SecondaryColor3fEXT(disp, parameters) \
    (* GET_SecondaryColor3fEXT(disp)) parameters
static inline _glptr_SecondaryColor3fEXT GET_SecondaryColor3fEXT(struct _glapi_table *disp) {
   return (_glptr_SecondaryColor3fEXT) (GET_by_offset(disp, _gloffset_SecondaryColor3fEXT));
}

static inline void SET_SecondaryColor3fEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3fEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_SecondaryColor3fvEXT)(const GLfloat *);
#define CALL_SecondaryColor3fvEXT(disp, parameters) \
    (* GET_SecondaryColor3fvEXT(disp)) parameters
static inline _glptr_SecondaryColor3fvEXT GET_SecondaryColor3fvEXT(struct _glapi_table *disp) {
   return (_glptr_SecondaryColor3fvEXT) (GET_by_offset(disp, _gloffset_SecondaryColor3fvEXT));
}

static inline void SET_SecondaryColor3fvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3fvEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_SecondaryColor3iEXT)(GLint, GLint, GLint);
#define CALL_SecondaryColor3iEXT(disp, parameters) \
    (* GET_SecondaryColor3iEXT(disp)) parameters
static inline _glptr_SecondaryColor3iEXT GET_SecondaryColor3iEXT(struct _glapi_table *disp) {
   return (_glptr_SecondaryColor3iEXT) (GET_by_offset(disp, _gloffset_SecondaryColor3iEXT));
}

static inline void SET_SecondaryColor3iEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3iEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_SecondaryColor3ivEXT)(const GLint *);
#define CALL_SecondaryColor3ivEXT(disp, parameters) \
    (* GET_SecondaryColor3ivEXT(disp)) parameters
static inline _glptr_SecondaryColor3ivEXT GET_SecondaryColor3ivEXT(struct _glapi_table *disp) {
   return (_glptr_SecondaryColor3ivEXT) (GET_by_offset(disp, _gloffset_SecondaryColor3ivEXT));
}

static inline void SET_SecondaryColor3ivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3ivEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_SecondaryColor3sEXT)(GLshort, GLshort, GLshort);
#define CALL_SecondaryColor3sEXT(disp, parameters) \
    (* GET_SecondaryColor3sEXT(disp)) parameters
static inline _glptr_SecondaryColor3sEXT GET_SecondaryColor3sEXT(struct _glapi_table *disp) {
   return (_glptr_SecondaryColor3sEXT) (GET_by_offset(disp, _gloffset_SecondaryColor3sEXT));
}

static inline void SET_SecondaryColor3sEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3sEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_SecondaryColor3svEXT)(const GLshort *);
#define CALL_SecondaryColor3svEXT(disp, parameters) \
    (* GET_SecondaryColor3svEXT(disp)) parameters
static inline _glptr_SecondaryColor3svEXT GET_SecondaryColor3svEXT(struct _glapi_table *disp) {
   return (_glptr_SecondaryColor3svEXT) (GET_by_offset(disp, _gloffset_SecondaryColor3svEXT));
}

static inline void SET_SecondaryColor3svEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3svEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_SecondaryColor3ubEXT)(GLubyte, GLubyte, GLubyte);
#define CALL_SecondaryColor3ubEXT(disp, parameters) \
    (* GET_SecondaryColor3ubEXT(disp)) parameters
static inline _glptr_SecondaryColor3ubEXT GET_SecondaryColor3ubEXT(struct _glapi_table *disp) {
   return (_glptr_SecondaryColor3ubEXT) (GET_by_offset(disp, _gloffset_SecondaryColor3ubEXT));
}

static inline void SET_SecondaryColor3ubEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLubyte, GLubyte, GLubyte)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3ubEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_SecondaryColor3ubvEXT)(const GLubyte *);
#define CALL_SecondaryColor3ubvEXT(disp, parameters) \
    (* GET_SecondaryColor3ubvEXT(disp)) parameters
static inline _glptr_SecondaryColor3ubvEXT GET_SecondaryColor3ubvEXT(struct _glapi_table *disp) {
   return (_glptr_SecondaryColor3ubvEXT) (GET_by_offset(disp, _gloffset_SecondaryColor3ubvEXT));
}

static inline void SET_SecondaryColor3ubvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLubyte *)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3ubvEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_SecondaryColor3uiEXT)(GLuint, GLuint, GLuint);
#define CALL_SecondaryColor3uiEXT(disp, parameters) \
    (* GET_SecondaryColor3uiEXT(disp)) parameters
static inline _glptr_SecondaryColor3uiEXT GET_SecondaryColor3uiEXT(struct _glapi_table *disp) {
   return (_glptr_SecondaryColor3uiEXT) (GET_by_offset(disp, _gloffset_SecondaryColor3uiEXT));
}

static inline void SET_SecondaryColor3uiEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3uiEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_SecondaryColor3uivEXT)(const GLuint *);
#define CALL_SecondaryColor3uivEXT(disp, parameters) \
    (* GET_SecondaryColor3uivEXT(disp)) parameters
static inline _glptr_SecondaryColor3uivEXT GET_SecondaryColor3uivEXT(struct _glapi_table *disp) {
   return (_glptr_SecondaryColor3uivEXT) (GET_by_offset(disp, _gloffset_SecondaryColor3uivEXT));
}

static inline void SET_SecondaryColor3uivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLuint *)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3uivEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_SecondaryColor3usEXT)(GLushort, GLushort, GLushort);
#define CALL_SecondaryColor3usEXT(disp, parameters) \
    (* GET_SecondaryColor3usEXT(disp)) parameters
static inline _glptr_SecondaryColor3usEXT GET_SecondaryColor3usEXT(struct _glapi_table *disp) {
   return (_glptr_SecondaryColor3usEXT) (GET_by_offset(disp, _gloffset_SecondaryColor3usEXT));
}

static inline void SET_SecondaryColor3usEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLushort, GLushort, GLushort)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3usEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_SecondaryColor3usvEXT)(const GLushort *);
#define CALL_SecondaryColor3usvEXT(disp, parameters) \
    (* GET_SecondaryColor3usvEXT(disp)) parameters
static inline _glptr_SecondaryColor3usvEXT GET_SecondaryColor3usvEXT(struct _glapi_table *disp) {
   return (_glptr_SecondaryColor3usvEXT) (GET_by_offset(disp, _gloffset_SecondaryColor3usvEXT));
}

static inline void SET_SecondaryColor3usvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLushort *)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3usvEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_SecondaryColorPointerEXT)(GLint, GLenum, GLsizei, const GLvoid *);
#define CALL_SecondaryColorPointerEXT(disp, parameters) \
    (* GET_SecondaryColorPointerEXT(disp)) parameters
static inline _glptr_SecondaryColorPointerEXT GET_SecondaryColorPointerEXT(struct _glapi_table *disp) {
   return (_glptr_SecondaryColorPointerEXT) (GET_by_offset(disp, _gloffset_SecondaryColorPointerEXT));
}

static inline void SET_SecondaryColorPointerEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLenum, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_SecondaryColorPointerEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiDrawArraysEXT)(GLenum, const GLint *, const GLsizei *, GLsizei);
#define CALL_MultiDrawArraysEXT(disp, parameters) \
    (* GET_MultiDrawArraysEXT(disp)) parameters
static inline _glptr_MultiDrawArraysEXT GET_MultiDrawArraysEXT(struct _glapi_table *disp) {
   return (_glptr_MultiDrawArraysEXT) (GET_by_offset(disp, _gloffset_MultiDrawArraysEXT));
}

static inline void SET_MultiDrawArraysEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLint *, const GLsizei *, GLsizei)) {
   SET_by_offset(disp, _gloffset_MultiDrawArraysEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiDrawElementsEXT)(GLenum, const GLsizei *, GLenum, const GLvoid **, GLsizei);
#define CALL_MultiDrawElementsEXT(disp, parameters) \
    (* GET_MultiDrawElementsEXT(disp)) parameters
static inline _glptr_MultiDrawElementsEXT GET_MultiDrawElementsEXT(struct _glapi_table *disp) {
   return (_glptr_MultiDrawElementsEXT) (GET_by_offset(disp, _gloffset_MultiDrawElementsEXT));
}

static inline void SET_MultiDrawElementsEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLsizei *, GLenum, const GLvoid **, GLsizei)) {
   SET_by_offset(disp, _gloffset_MultiDrawElementsEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_FogCoordPointerEXT)(GLenum, GLsizei, const GLvoid *);
#define CALL_FogCoordPointerEXT(disp, parameters) \
    (* GET_FogCoordPointerEXT(disp)) parameters
static inline _glptr_FogCoordPointerEXT GET_FogCoordPointerEXT(struct _glapi_table *disp) {
   return (_glptr_FogCoordPointerEXT) (GET_by_offset(disp, _gloffset_FogCoordPointerEXT));
}

static inline void SET_FogCoordPointerEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_FogCoordPointerEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_FogCoorddEXT)(GLdouble);
#define CALL_FogCoorddEXT(disp, parameters) \
    (* GET_FogCoorddEXT(disp)) parameters
static inline _glptr_FogCoorddEXT GET_FogCoorddEXT(struct _glapi_table *disp) {
   return (_glptr_FogCoorddEXT) (GET_by_offset(disp, _gloffset_FogCoorddEXT));
}

static inline void SET_FogCoorddEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble)) {
   SET_by_offset(disp, _gloffset_FogCoorddEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_FogCoorddvEXT)(const GLdouble *);
#define CALL_FogCoorddvEXT(disp, parameters) \
    (* GET_FogCoorddvEXT(disp)) parameters
static inline _glptr_FogCoorddvEXT GET_FogCoorddvEXT(struct _glapi_table *disp) {
   return (_glptr_FogCoorddvEXT) (GET_by_offset(disp, _gloffset_FogCoorddvEXT));
}

static inline void SET_FogCoorddvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_FogCoorddvEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_FogCoordfEXT)(GLfloat);
#define CALL_FogCoordfEXT(disp, parameters) \
    (* GET_FogCoordfEXT(disp)) parameters
static inline _glptr_FogCoordfEXT GET_FogCoordfEXT(struct _glapi_table *disp) {
   return (_glptr_FogCoordfEXT) (GET_by_offset(disp, _gloffset_FogCoordfEXT));
}

static inline void SET_FogCoordfEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat)) {
   SET_by_offset(disp, _gloffset_FogCoordfEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_FogCoordfvEXT)(const GLfloat *);
#define CALL_FogCoordfvEXT(disp, parameters) \
    (* GET_FogCoordfvEXT(disp)) parameters
static inline _glptr_FogCoordfvEXT GET_FogCoordfvEXT(struct _glapi_table *disp) {
   return (_glptr_FogCoordfvEXT) (GET_by_offset(disp, _gloffset_FogCoordfvEXT));
}

static inline void SET_FogCoordfvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_FogCoordfvEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_PixelTexGenSGIX)(GLenum);
#define CALL_PixelTexGenSGIX(disp, parameters) \
    (* GET_PixelTexGenSGIX(disp)) parameters
static inline _glptr_PixelTexGenSGIX GET_PixelTexGenSGIX(struct _glapi_table *disp) {
   return (_glptr_PixelTexGenSGIX) (GET_by_offset(disp, _gloffset_PixelTexGenSGIX));
}

static inline void SET_PixelTexGenSGIX(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_PixelTexGenSGIX, fn);
}

typedef void (GLAPIENTRYP _glptr_BlendFuncSeparateEXT)(GLenum, GLenum, GLenum, GLenum);
#define CALL_BlendFuncSeparateEXT(disp, parameters) \
    (* GET_BlendFuncSeparateEXT(disp)) parameters
static inline _glptr_BlendFuncSeparateEXT GET_BlendFuncSeparateEXT(struct _glapi_table *disp) {
   return (_glptr_BlendFuncSeparateEXT) (GET_by_offset(disp, _gloffset_BlendFuncSeparateEXT));
}

static inline void SET_BlendFuncSeparateEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_BlendFuncSeparateEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_FlushVertexArrayRangeNV)(void);
#define CALL_FlushVertexArrayRangeNV(disp, parameters) \
    (* GET_FlushVertexArrayRangeNV(disp)) parameters
static inline _glptr_FlushVertexArrayRangeNV GET_FlushVertexArrayRangeNV(struct _glapi_table *disp) {
   return (_glptr_FlushVertexArrayRangeNV) (GET_by_offset(disp, _gloffset_FlushVertexArrayRangeNV));
}

static inline void SET_FlushVertexArrayRangeNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_FlushVertexArrayRangeNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexArrayRangeNV)(GLsizei, const GLvoid *);
#define CALL_VertexArrayRangeNV(disp, parameters) \
    (* GET_VertexArrayRangeNV(disp)) parameters
static inline _glptr_VertexArrayRangeNV GET_VertexArrayRangeNV(struct _glapi_table *disp) {
   return (_glptr_VertexArrayRangeNV) (GET_by_offset(disp, _gloffset_VertexArrayRangeNV));
}

static inline void SET_VertexArrayRangeNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_VertexArrayRangeNV, fn);
}

typedef void (GLAPIENTRYP _glptr_CombinerInputNV)(GLenum, GLenum, GLenum, GLenum, GLenum, GLenum);
#define CALL_CombinerInputNV(disp, parameters) \
    (* GET_CombinerInputNV(disp)) parameters
static inline _glptr_CombinerInputNV GET_CombinerInputNV(struct _glapi_table *disp) {
   return (_glptr_CombinerInputNV) (GET_by_offset(disp, _gloffset_CombinerInputNV));
}

static inline void SET_CombinerInputNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLenum, GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_CombinerInputNV, fn);
}

typedef void (GLAPIENTRYP _glptr_CombinerOutputNV)(GLenum, GLenum, GLenum, GLenum, GLenum, GLenum, GLenum, GLboolean, GLboolean, GLboolean);
#define CALL_CombinerOutputNV(disp, parameters) \
    (* GET_CombinerOutputNV(disp)) parameters
static inline _glptr_CombinerOutputNV GET_CombinerOutputNV(struct _glapi_table *disp) {
   return (_glptr_CombinerOutputNV) (GET_by_offset(disp, _gloffset_CombinerOutputNV));
}

static inline void SET_CombinerOutputNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLenum, GLenum, GLenum, GLenum, GLboolean, GLboolean, GLboolean)) {
   SET_by_offset(disp, _gloffset_CombinerOutputNV, fn);
}

typedef void (GLAPIENTRYP _glptr_CombinerParameterfNV)(GLenum, GLfloat);
#define CALL_CombinerParameterfNV(disp, parameters) \
    (* GET_CombinerParameterfNV(disp)) parameters
static inline _glptr_CombinerParameterfNV GET_CombinerParameterfNV(struct _glapi_table *disp) {
   return (_glptr_CombinerParameterfNV) (GET_by_offset(disp, _gloffset_CombinerParameterfNV));
}

static inline void SET_CombinerParameterfNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_CombinerParameterfNV, fn);
}

typedef void (GLAPIENTRYP _glptr_CombinerParameterfvNV)(GLenum, const GLfloat *);
#define CALL_CombinerParameterfvNV(disp, parameters) \
    (* GET_CombinerParameterfvNV(disp)) parameters
static inline _glptr_CombinerParameterfvNV GET_CombinerParameterfvNV(struct _glapi_table *disp) {
   return (_glptr_CombinerParameterfvNV) (GET_by_offset(disp, _gloffset_CombinerParameterfvNV));
}

static inline void SET_CombinerParameterfvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_CombinerParameterfvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_CombinerParameteriNV)(GLenum, GLint);
#define CALL_CombinerParameteriNV(disp, parameters) \
    (* GET_CombinerParameteriNV(disp)) parameters
static inline _glptr_CombinerParameteriNV GET_CombinerParameteriNV(struct _glapi_table *disp) {
   return (_glptr_CombinerParameteriNV) (GET_by_offset(disp, _gloffset_CombinerParameteriNV));
}

static inline void SET_CombinerParameteriNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_CombinerParameteriNV, fn);
}

typedef void (GLAPIENTRYP _glptr_CombinerParameterivNV)(GLenum, const GLint *);
#define CALL_CombinerParameterivNV(disp, parameters) \
    (* GET_CombinerParameterivNV(disp)) parameters
static inline _glptr_CombinerParameterivNV GET_CombinerParameterivNV(struct _glapi_table *disp) {
   return (_glptr_CombinerParameterivNV) (GET_by_offset(disp, _gloffset_CombinerParameterivNV));
}

static inline void SET_CombinerParameterivNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_CombinerParameterivNV, fn);
}

typedef void (GLAPIENTRYP _glptr_FinalCombinerInputNV)(GLenum, GLenum, GLenum, GLenum);
#define CALL_FinalCombinerInputNV(disp, parameters) \
    (* GET_FinalCombinerInputNV(disp)) parameters
static inline _glptr_FinalCombinerInputNV GET_FinalCombinerInputNV(struct _glapi_table *disp) {
   return (_glptr_FinalCombinerInputNV) (GET_by_offset(disp, _gloffset_FinalCombinerInputNV));
}

static inline void SET_FinalCombinerInputNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_FinalCombinerInputNV, fn);
}

typedef void (GLAPIENTRYP _glptr_GetCombinerInputParameterfvNV)(GLenum, GLenum, GLenum, GLenum, GLfloat *);
#define CALL_GetCombinerInputParameterfvNV(disp, parameters) \
    (* GET_GetCombinerInputParameterfvNV(disp)) parameters
static inline _glptr_GetCombinerInputParameterfvNV GET_GetCombinerInputParameterfvNV(struct _glapi_table *disp) {
   return (_glptr_GetCombinerInputParameterfvNV) (GET_by_offset(disp, _gloffset_GetCombinerInputParameterfvNV));
}

static inline void SET_GetCombinerInputParameterfvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetCombinerInputParameterfvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_GetCombinerInputParameterivNV)(GLenum, GLenum, GLenum, GLenum, GLint *);
#define CALL_GetCombinerInputParameterivNV(disp, parameters) \
    (* GET_GetCombinerInputParameterivNV(disp)) parameters
static inline _glptr_GetCombinerInputParameterivNV GET_GetCombinerInputParameterivNV(struct _glapi_table *disp) {
   return (_glptr_GetCombinerInputParameterivNV) (GET_by_offset(disp, _gloffset_GetCombinerInputParameterivNV));
}

static inline void SET_GetCombinerInputParameterivNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetCombinerInputParameterivNV, fn);
}

typedef void (GLAPIENTRYP _glptr_GetCombinerOutputParameterfvNV)(GLenum, GLenum, GLenum, GLfloat *);
#define CALL_GetCombinerOutputParameterfvNV(disp, parameters) \
    (* GET_GetCombinerOutputParameterfvNV(disp)) parameters
static inline _glptr_GetCombinerOutputParameterfvNV GET_GetCombinerOutputParameterfvNV(struct _glapi_table *disp) {
   return (_glptr_GetCombinerOutputParameterfvNV) (GET_by_offset(disp, _gloffset_GetCombinerOutputParameterfvNV));
}

static inline void SET_GetCombinerOutputParameterfvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetCombinerOutputParameterfvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_GetCombinerOutputParameterivNV)(GLenum, GLenum, GLenum, GLint *);
#define CALL_GetCombinerOutputParameterivNV(disp, parameters) \
    (* GET_GetCombinerOutputParameterivNV(disp)) parameters
static inline _glptr_GetCombinerOutputParameterivNV GET_GetCombinerOutputParameterivNV(struct _glapi_table *disp) {
   return (_glptr_GetCombinerOutputParameterivNV) (GET_by_offset(disp, _gloffset_GetCombinerOutputParameterivNV));
}

static inline void SET_GetCombinerOutputParameterivNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetCombinerOutputParameterivNV, fn);
}

typedef void (GLAPIENTRYP _glptr_GetFinalCombinerInputParameterfvNV)(GLenum, GLenum, GLfloat *);
#define CALL_GetFinalCombinerInputParameterfvNV(disp, parameters) \
    (* GET_GetFinalCombinerInputParameterfvNV(disp)) parameters
static inline _glptr_GetFinalCombinerInputParameterfvNV GET_GetFinalCombinerInputParameterfvNV(struct _glapi_table *disp) {
   return (_glptr_GetFinalCombinerInputParameterfvNV) (GET_by_offset(disp, _gloffset_GetFinalCombinerInputParameterfvNV));
}

static inline void SET_GetFinalCombinerInputParameterfvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetFinalCombinerInputParameterfvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_GetFinalCombinerInputParameterivNV)(GLenum, GLenum, GLint *);
#define CALL_GetFinalCombinerInputParameterivNV(disp, parameters) \
    (* GET_GetFinalCombinerInputParameterivNV(disp)) parameters
static inline _glptr_GetFinalCombinerInputParameterivNV GET_GetFinalCombinerInputParameterivNV(struct _glapi_table *disp) {
   return (_glptr_GetFinalCombinerInputParameterivNV) (GET_by_offset(disp, _gloffset_GetFinalCombinerInputParameterivNV));
}

static inline void SET_GetFinalCombinerInputParameterivNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetFinalCombinerInputParameterivNV, fn);
}

typedef void (GLAPIENTRYP _glptr_ResizeBuffersMESA)(void);
#define CALL_ResizeBuffersMESA(disp, parameters) \
    (* GET_ResizeBuffersMESA(disp)) parameters
static inline _glptr_ResizeBuffersMESA GET_ResizeBuffersMESA(struct _glapi_table *disp) {
   return (_glptr_ResizeBuffersMESA) (GET_by_offset(disp, _gloffset_ResizeBuffersMESA));
}

static inline void SET_ResizeBuffersMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_ResizeBuffersMESA, fn);
}

typedef void (GLAPIENTRYP _glptr_WindowPos2dMESA)(GLdouble, GLdouble);
#define CALL_WindowPos2dMESA(disp, parameters) \
    (* GET_WindowPos2dMESA(disp)) parameters
static inline _glptr_WindowPos2dMESA GET_WindowPos2dMESA(struct _glapi_table *disp) {
   return (_glptr_WindowPos2dMESA) (GET_by_offset(disp, _gloffset_WindowPos2dMESA));
}

static inline void SET_WindowPos2dMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_WindowPos2dMESA, fn);
}

typedef void (GLAPIENTRYP _glptr_WindowPos2dvMESA)(const GLdouble *);
#define CALL_WindowPos2dvMESA(disp, parameters) \
    (* GET_WindowPos2dvMESA(disp)) parameters
static inline _glptr_WindowPos2dvMESA GET_WindowPos2dvMESA(struct _glapi_table *disp) {
   return (_glptr_WindowPos2dvMESA) (GET_by_offset(disp, _gloffset_WindowPos2dvMESA));
}

static inline void SET_WindowPos2dvMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_WindowPos2dvMESA, fn);
}

typedef void (GLAPIENTRYP _glptr_WindowPos2fMESA)(GLfloat, GLfloat);
#define CALL_WindowPos2fMESA(disp, parameters) \
    (* GET_WindowPos2fMESA(disp)) parameters
static inline _glptr_WindowPos2fMESA GET_WindowPos2fMESA(struct _glapi_table *disp) {
   return (_glptr_WindowPos2fMESA) (GET_by_offset(disp, _gloffset_WindowPos2fMESA));
}

static inline void SET_WindowPos2fMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_WindowPos2fMESA, fn);
}

typedef void (GLAPIENTRYP _glptr_WindowPos2fvMESA)(const GLfloat *);
#define CALL_WindowPos2fvMESA(disp, parameters) \
    (* GET_WindowPos2fvMESA(disp)) parameters
static inline _glptr_WindowPos2fvMESA GET_WindowPos2fvMESA(struct _glapi_table *disp) {
   return (_glptr_WindowPos2fvMESA) (GET_by_offset(disp, _gloffset_WindowPos2fvMESA));
}

static inline void SET_WindowPos2fvMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_WindowPos2fvMESA, fn);
}

typedef void (GLAPIENTRYP _glptr_WindowPos2iMESA)(GLint, GLint);
#define CALL_WindowPos2iMESA(disp, parameters) \
    (* GET_WindowPos2iMESA(disp)) parameters
static inline _glptr_WindowPos2iMESA GET_WindowPos2iMESA(struct _glapi_table *disp) {
   return (_glptr_WindowPos2iMESA) (GET_by_offset(disp, _gloffset_WindowPos2iMESA));
}

static inline void SET_WindowPos2iMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint)) {
   SET_by_offset(disp, _gloffset_WindowPos2iMESA, fn);
}

typedef void (GLAPIENTRYP _glptr_WindowPos2ivMESA)(const GLint *);
#define CALL_WindowPos2ivMESA(disp, parameters) \
    (* GET_WindowPos2ivMESA(disp)) parameters
static inline _glptr_WindowPos2ivMESA GET_WindowPos2ivMESA(struct _glapi_table *disp) {
   return (_glptr_WindowPos2ivMESA) (GET_by_offset(disp, _gloffset_WindowPos2ivMESA));
}

static inline void SET_WindowPos2ivMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_WindowPos2ivMESA, fn);
}

typedef void (GLAPIENTRYP _glptr_WindowPos2sMESA)(GLshort, GLshort);
#define CALL_WindowPos2sMESA(disp, parameters) \
    (* GET_WindowPos2sMESA(disp)) parameters
static inline _glptr_WindowPos2sMESA GET_WindowPos2sMESA(struct _glapi_table *disp) {
   return (_glptr_WindowPos2sMESA) (GET_by_offset(disp, _gloffset_WindowPos2sMESA));
}

static inline void SET_WindowPos2sMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_WindowPos2sMESA, fn);
}

typedef void (GLAPIENTRYP _glptr_WindowPos2svMESA)(const GLshort *);
#define CALL_WindowPos2svMESA(disp, parameters) \
    (* GET_WindowPos2svMESA(disp)) parameters
static inline _glptr_WindowPos2svMESA GET_WindowPos2svMESA(struct _glapi_table *disp) {
   return (_glptr_WindowPos2svMESA) (GET_by_offset(disp, _gloffset_WindowPos2svMESA));
}

static inline void SET_WindowPos2svMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_WindowPos2svMESA, fn);
}

typedef void (GLAPIENTRYP _glptr_WindowPos3dMESA)(GLdouble, GLdouble, GLdouble);
#define CALL_WindowPos3dMESA(disp, parameters) \
    (* GET_WindowPos3dMESA(disp)) parameters
static inline _glptr_WindowPos3dMESA GET_WindowPos3dMESA(struct _glapi_table *disp) {
   return (_glptr_WindowPos3dMESA) (GET_by_offset(disp, _gloffset_WindowPos3dMESA));
}

static inline void SET_WindowPos3dMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_WindowPos3dMESA, fn);
}

typedef void (GLAPIENTRYP _glptr_WindowPos3dvMESA)(const GLdouble *);
#define CALL_WindowPos3dvMESA(disp, parameters) \
    (* GET_WindowPos3dvMESA(disp)) parameters
static inline _glptr_WindowPos3dvMESA GET_WindowPos3dvMESA(struct _glapi_table *disp) {
   return (_glptr_WindowPos3dvMESA) (GET_by_offset(disp, _gloffset_WindowPos3dvMESA));
}

static inline void SET_WindowPos3dvMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_WindowPos3dvMESA, fn);
}

typedef void (GLAPIENTRYP _glptr_WindowPos3fMESA)(GLfloat, GLfloat, GLfloat);
#define CALL_WindowPos3fMESA(disp, parameters) \
    (* GET_WindowPos3fMESA(disp)) parameters
static inline _glptr_WindowPos3fMESA GET_WindowPos3fMESA(struct _glapi_table *disp) {
   return (_glptr_WindowPos3fMESA) (GET_by_offset(disp, _gloffset_WindowPos3fMESA));
}

static inline void SET_WindowPos3fMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_WindowPos3fMESA, fn);
}

typedef void (GLAPIENTRYP _glptr_WindowPos3fvMESA)(const GLfloat *);
#define CALL_WindowPos3fvMESA(disp, parameters) \
    (* GET_WindowPos3fvMESA(disp)) parameters
static inline _glptr_WindowPos3fvMESA GET_WindowPos3fvMESA(struct _glapi_table *disp) {
   return (_glptr_WindowPos3fvMESA) (GET_by_offset(disp, _gloffset_WindowPos3fvMESA));
}

static inline void SET_WindowPos3fvMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_WindowPos3fvMESA, fn);
}

typedef void (GLAPIENTRYP _glptr_WindowPos3iMESA)(GLint, GLint, GLint);
#define CALL_WindowPos3iMESA(disp, parameters) \
    (* GET_WindowPos3iMESA(disp)) parameters
static inline _glptr_WindowPos3iMESA GET_WindowPos3iMESA(struct _glapi_table *disp) {
   return (_glptr_WindowPos3iMESA) (GET_by_offset(disp, _gloffset_WindowPos3iMESA));
}

static inline void SET_WindowPos3iMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_WindowPos3iMESA, fn);
}

typedef void (GLAPIENTRYP _glptr_WindowPos3ivMESA)(const GLint *);
#define CALL_WindowPos3ivMESA(disp, parameters) \
    (* GET_WindowPos3ivMESA(disp)) parameters
static inline _glptr_WindowPos3ivMESA GET_WindowPos3ivMESA(struct _glapi_table *disp) {
   return (_glptr_WindowPos3ivMESA) (GET_by_offset(disp, _gloffset_WindowPos3ivMESA));
}

static inline void SET_WindowPos3ivMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_WindowPos3ivMESA, fn);
}

typedef void (GLAPIENTRYP _glptr_WindowPos3sMESA)(GLshort, GLshort, GLshort);
#define CALL_WindowPos3sMESA(disp, parameters) \
    (* GET_WindowPos3sMESA(disp)) parameters
static inline _glptr_WindowPos3sMESA GET_WindowPos3sMESA(struct _glapi_table *disp) {
   return (_glptr_WindowPos3sMESA) (GET_by_offset(disp, _gloffset_WindowPos3sMESA));
}

static inline void SET_WindowPos3sMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_WindowPos3sMESA, fn);
}

typedef void (GLAPIENTRYP _glptr_WindowPos3svMESA)(const GLshort *);
#define CALL_WindowPos3svMESA(disp, parameters) \
    (* GET_WindowPos3svMESA(disp)) parameters
static inline _glptr_WindowPos3svMESA GET_WindowPos3svMESA(struct _glapi_table *disp) {
   return (_glptr_WindowPos3svMESA) (GET_by_offset(disp, _gloffset_WindowPos3svMESA));
}

static inline void SET_WindowPos3svMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_WindowPos3svMESA, fn);
}

typedef void (GLAPIENTRYP _glptr_WindowPos4dMESA)(GLdouble, GLdouble, GLdouble, GLdouble);
#define CALL_WindowPos4dMESA(disp, parameters) \
    (* GET_WindowPos4dMESA(disp)) parameters
static inline _glptr_WindowPos4dMESA GET_WindowPos4dMESA(struct _glapi_table *disp) {
   return (_glptr_WindowPos4dMESA) (GET_by_offset(disp, _gloffset_WindowPos4dMESA));
}

static inline void SET_WindowPos4dMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_WindowPos4dMESA, fn);
}

typedef void (GLAPIENTRYP _glptr_WindowPos4dvMESA)(const GLdouble *);
#define CALL_WindowPos4dvMESA(disp, parameters) \
    (* GET_WindowPos4dvMESA(disp)) parameters
static inline _glptr_WindowPos4dvMESA GET_WindowPos4dvMESA(struct _glapi_table *disp) {
   return (_glptr_WindowPos4dvMESA) (GET_by_offset(disp, _gloffset_WindowPos4dvMESA));
}

static inline void SET_WindowPos4dvMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_WindowPos4dvMESA, fn);
}

typedef void (GLAPIENTRYP _glptr_WindowPos4fMESA)(GLfloat, GLfloat, GLfloat, GLfloat);
#define CALL_WindowPos4fMESA(disp, parameters) \
    (* GET_WindowPos4fMESA(disp)) parameters
static inline _glptr_WindowPos4fMESA GET_WindowPos4fMESA(struct _glapi_table *disp) {
   return (_glptr_WindowPos4fMESA) (GET_by_offset(disp, _gloffset_WindowPos4fMESA));
}

static inline void SET_WindowPos4fMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_WindowPos4fMESA, fn);
}

typedef void (GLAPIENTRYP _glptr_WindowPos4fvMESA)(const GLfloat *);
#define CALL_WindowPos4fvMESA(disp, parameters) \
    (* GET_WindowPos4fvMESA(disp)) parameters
static inline _glptr_WindowPos4fvMESA GET_WindowPos4fvMESA(struct _glapi_table *disp) {
   return (_glptr_WindowPos4fvMESA) (GET_by_offset(disp, _gloffset_WindowPos4fvMESA));
}

static inline void SET_WindowPos4fvMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_WindowPos4fvMESA, fn);
}

typedef void (GLAPIENTRYP _glptr_WindowPos4iMESA)(GLint, GLint, GLint, GLint);
#define CALL_WindowPos4iMESA(disp, parameters) \
    (* GET_WindowPos4iMESA(disp)) parameters
static inline _glptr_WindowPos4iMESA GET_WindowPos4iMESA(struct _glapi_table *disp) {
   return (_glptr_WindowPos4iMESA) (GET_by_offset(disp, _gloffset_WindowPos4iMESA));
}

static inline void SET_WindowPos4iMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_WindowPos4iMESA, fn);
}

typedef void (GLAPIENTRYP _glptr_WindowPos4ivMESA)(const GLint *);
#define CALL_WindowPos4ivMESA(disp, parameters) \
    (* GET_WindowPos4ivMESA(disp)) parameters
static inline _glptr_WindowPos4ivMESA GET_WindowPos4ivMESA(struct _glapi_table *disp) {
   return (_glptr_WindowPos4ivMESA) (GET_by_offset(disp, _gloffset_WindowPos4ivMESA));
}

static inline void SET_WindowPos4ivMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_WindowPos4ivMESA, fn);
}

typedef void (GLAPIENTRYP _glptr_WindowPos4sMESA)(GLshort, GLshort, GLshort, GLshort);
#define CALL_WindowPos4sMESA(disp, parameters) \
    (* GET_WindowPos4sMESA(disp)) parameters
static inline _glptr_WindowPos4sMESA GET_WindowPos4sMESA(struct _glapi_table *disp) {
   return (_glptr_WindowPos4sMESA) (GET_by_offset(disp, _gloffset_WindowPos4sMESA));
}

static inline void SET_WindowPos4sMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_WindowPos4sMESA, fn);
}

typedef void (GLAPIENTRYP _glptr_WindowPos4svMESA)(const GLshort *);
#define CALL_WindowPos4svMESA(disp, parameters) \
    (* GET_WindowPos4svMESA(disp)) parameters
static inline _glptr_WindowPos4svMESA GET_WindowPos4svMESA(struct _glapi_table *disp) {
   return (_glptr_WindowPos4svMESA) (GET_by_offset(disp, _gloffset_WindowPos4svMESA));
}

static inline void SET_WindowPos4svMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_WindowPos4svMESA, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiModeDrawArraysIBM)(const GLenum *, const GLint *, const GLsizei *, GLsizei, GLint);
#define CALL_MultiModeDrawArraysIBM(disp, parameters) \
    (* GET_MultiModeDrawArraysIBM(disp)) parameters
static inline _glptr_MultiModeDrawArraysIBM GET_MultiModeDrawArraysIBM(struct _glapi_table *disp) {
   return (_glptr_MultiModeDrawArraysIBM) (GET_by_offset(disp, _gloffset_MultiModeDrawArraysIBM));
}

static inline void SET_MultiModeDrawArraysIBM(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLenum *, const GLint *, const GLsizei *, GLsizei, GLint)) {
   SET_by_offset(disp, _gloffset_MultiModeDrawArraysIBM, fn);
}

typedef void (GLAPIENTRYP _glptr_MultiModeDrawElementsIBM)(const GLenum *, const GLsizei *, GLenum, const GLvoid * const *, GLsizei, GLint);
#define CALL_MultiModeDrawElementsIBM(disp, parameters) \
    (* GET_MultiModeDrawElementsIBM(disp)) parameters
static inline _glptr_MultiModeDrawElementsIBM GET_MultiModeDrawElementsIBM(struct _glapi_table *disp) {
   return (_glptr_MultiModeDrawElementsIBM) (GET_by_offset(disp, _gloffset_MultiModeDrawElementsIBM));
}

static inline void SET_MultiModeDrawElementsIBM(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLenum *, const GLsizei *, GLenum, const GLvoid * const *, GLsizei, GLint)) {
   SET_by_offset(disp, _gloffset_MultiModeDrawElementsIBM, fn);
}

typedef void (GLAPIENTRYP _glptr_DeleteFencesNV)(GLsizei, const GLuint *);
#define CALL_DeleteFencesNV(disp, parameters) \
    (* GET_DeleteFencesNV(disp)) parameters
static inline _glptr_DeleteFencesNV GET_DeleteFencesNV(struct _glapi_table *disp) {
   return (_glptr_DeleteFencesNV) (GET_by_offset(disp, _gloffset_DeleteFencesNV));
}

static inline void SET_DeleteFencesNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_DeleteFencesNV, fn);
}

typedef void (GLAPIENTRYP _glptr_FinishFenceNV)(GLuint);
#define CALL_FinishFenceNV(disp, parameters) \
    (* GET_FinishFenceNV(disp)) parameters
static inline _glptr_FinishFenceNV GET_FinishFenceNV(struct _glapi_table *disp) {
   return (_glptr_FinishFenceNV) (GET_by_offset(disp, _gloffset_FinishFenceNV));
}

static inline void SET_FinishFenceNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_FinishFenceNV, fn);
}

typedef void (GLAPIENTRYP _glptr_GenFencesNV)(GLsizei, GLuint *);
#define CALL_GenFencesNV(disp, parameters) \
    (* GET_GenFencesNV(disp)) parameters
static inline _glptr_GenFencesNV GET_GenFencesNV(struct _glapi_table *disp) {
   return (_glptr_GenFencesNV) (GET_by_offset(disp, _gloffset_GenFencesNV));
}

static inline void SET_GenFencesNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLuint *)) {
   SET_by_offset(disp, _gloffset_GenFencesNV, fn);
}

typedef void (GLAPIENTRYP _glptr_GetFenceivNV)(GLuint, GLenum, GLint *);
#define CALL_GetFenceivNV(disp, parameters) \
    (* GET_GetFenceivNV(disp)) parameters
static inline _glptr_GetFenceivNV GET_GetFenceivNV(struct _glapi_table *disp) {
   return (_glptr_GetFenceivNV) (GET_by_offset(disp, _gloffset_GetFenceivNV));
}

static inline void SET_GetFenceivNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetFenceivNV, fn);
}

typedef GLboolean (GLAPIENTRYP _glptr_IsFenceNV)(GLuint);
#define CALL_IsFenceNV(disp, parameters) \
    (* GET_IsFenceNV(disp)) parameters
static inline _glptr_IsFenceNV GET_IsFenceNV(struct _glapi_table *disp) {
   return (_glptr_IsFenceNV) (GET_by_offset(disp, _gloffset_IsFenceNV));
}

static inline void SET_IsFenceNV(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_IsFenceNV, fn);
}

typedef void (GLAPIENTRYP _glptr_SetFenceNV)(GLuint, GLenum);
#define CALL_SetFenceNV(disp, parameters) \
    (* GET_SetFenceNV(disp)) parameters
static inline _glptr_SetFenceNV GET_SetFenceNV(struct _glapi_table *disp) {
   return (_glptr_SetFenceNV) (GET_by_offset(disp, _gloffset_SetFenceNV));
}

static inline void SET_SetFenceNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum)) {
   SET_by_offset(disp, _gloffset_SetFenceNV, fn);
}

typedef GLboolean (GLAPIENTRYP _glptr_TestFenceNV)(GLuint);
#define CALL_TestFenceNV(disp, parameters) \
    (* GET_TestFenceNV(disp)) parameters
static inline _glptr_TestFenceNV GET_TestFenceNV(struct _glapi_table *disp) {
   return (_glptr_TestFenceNV) (GET_by_offset(disp, _gloffset_TestFenceNV));
}

static inline void SET_TestFenceNV(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_TestFenceNV, fn);
}

typedef GLboolean (GLAPIENTRYP _glptr_AreProgramsResidentNV)(GLsizei, const GLuint *, GLboolean *);
#define CALL_AreProgramsResidentNV(disp, parameters) \
    (* GET_AreProgramsResidentNV(disp)) parameters
static inline _glptr_AreProgramsResidentNV GET_AreProgramsResidentNV(struct _glapi_table *disp) {
   return (_glptr_AreProgramsResidentNV) (GET_by_offset(disp, _gloffset_AreProgramsResidentNV));
}

static inline void SET_AreProgramsResidentNV(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLsizei, const GLuint *, GLboolean *)) {
   SET_by_offset(disp, _gloffset_AreProgramsResidentNV, fn);
}

typedef void (GLAPIENTRYP _glptr_BindProgramNV)(GLenum, GLuint);
#define CALL_BindProgramNV(disp, parameters) \
    (* GET_BindProgramNV(disp)) parameters
static inline _glptr_BindProgramNV GET_BindProgramNV(struct _glapi_table *disp) {
   return (_glptr_BindProgramNV) (GET_by_offset(disp, _gloffset_BindProgramNV));
}

static inline void SET_BindProgramNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_BindProgramNV, fn);
}

typedef void (GLAPIENTRYP _glptr_DeleteProgramsNV)(GLsizei, const GLuint *);
#define CALL_DeleteProgramsNV(disp, parameters) \
    (* GET_DeleteProgramsNV(disp)) parameters
static inline _glptr_DeleteProgramsNV GET_DeleteProgramsNV(struct _glapi_table *disp) {
   return (_glptr_DeleteProgramsNV) (GET_by_offset(disp, _gloffset_DeleteProgramsNV));
}

static inline void SET_DeleteProgramsNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_DeleteProgramsNV, fn);
}

typedef void (GLAPIENTRYP _glptr_ExecuteProgramNV)(GLenum, GLuint, const GLfloat *);
#define CALL_ExecuteProgramNV(disp, parameters) \
    (* GET_ExecuteProgramNV(disp)) parameters
static inline _glptr_ExecuteProgramNV GET_ExecuteProgramNV(struct _glapi_table *disp) {
   return (_glptr_ExecuteProgramNV) (GET_by_offset(disp, _gloffset_ExecuteProgramNV));
}

static inline void SET_ExecuteProgramNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_ExecuteProgramNV, fn);
}

typedef void (GLAPIENTRYP _glptr_GenProgramsNV)(GLsizei, GLuint *);
#define CALL_GenProgramsNV(disp, parameters) \
    (* GET_GenProgramsNV(disp)) parameters
static inline _glptr_GenProgramsNV GET_GenProgramsNV(struct _glapi_table *disp) {
   return (_glptr_GenProgramsNV) (GET_by_offset(disp, _gloffset_GenProgramsNV));
}

static inline void SET_GenProgramsNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLuint *)) {
   SET_by_offset(disp, _gloffset_GenProgramsNV, fn);
}

typedef void (GLAPIENTRYP _glptr_GetProgramParameterdvNV)(GLenum, GLuint, GLenum, GLdouble *);
#define CALL_GetProgramParameterdvNV(disp, parameters) \
    (* GET_GetProgramParameterdvNV(disp)) parameters
static inline _glptr_GetProgramParameterdvNV GET_GetProgramParameterdvNV(struct _glapi_table *disp) {
   return (_glptr_GetProgramParameterdvNV) (GET_by_offset(disp, _gloffset_GetProgramParameterdvNV));
}

static inline void SET_GetProgramParameterdvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLenum, GLdouble *)) {
   SET_by_offset(disp, _gloffset_GetProgramParameterdvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_GetProgramParameterfvNV)(GLenum, GLuint, GLenum, GLfloat *);
#define CALL_GetProgramParameterfvNV(disp, parameters) \
    (* GET_GetProgramParameterfvNV(disp)) parameters
static inline _glptr_GetProgramParameterfvNV GET_GetProgramParameterfvNV(struct _glapi_table *disp) {
   return (_glptr_GetProgramParameterfvNV) (GET_by_offset(disp, _gloffset_GetProgramParameterfvNV));
}

static inline void SET_GetProgramParameterfvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetProgramParameterfvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_GetProgramStringNV)(GLuint, GLenum, GLubyte *);
#define CALL_GetProgramStringNV(disp, parameters) \
    (* GET_GetProgramStringNV(disp)) parameters
static inline _glptr_GetProgramStringNV GET_GetProgramStringNV(struct _glapi_table *disp) {
   return (_glptr_GetProgramStringNV) (GET_by_offset(disp, _gloffset_GetProgramStringNV));
}

static inline void SET_GetProgramStringNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLubyte *)) {
   SET_by_offset(disp, _gloffset_GetProgramStringNV, fn);
}

typedef void (GLAPIENTRYP _glptr_GetProgramivNV)(GLuint, GLenum, GLint *);
#define CALL_GetProgramivNV(disp, parameters) \
    (* GET_GetProgramivNV(disp)) parameters
static inline _glptr_GetProgramivNV GET_GetProgramivNV(struct _glapi_table *disp) {
   return (_glptr_GetProgramivNV) (GET_by_offset(disp, _gloffset_GetProgramivNV));
}

static inline void SET_GetProgramivNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetProgramivNV, fn);
}

typedef void (GLAPIENTRYP _glptr_GetTrackMatrixivNV)(GLenum, GLuint, GLenum, GLint *);
#define CALL_GetTrackMatrixivNV(disp, parameters) \
    (* GET_GetTrackMatrixivNV(disp)) parameters
static inline _glptr_GetTrackMatrixivNV GET_GetTrackMatrixivNV(struct _glapi_table *disp) {
   return (_glptr_GetTrackMatrixivNV) (GET_by_offset(disp, _gloffset_GetTrackMatrixivNV));
}

static inline void SET_GetTrackMatrixivNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetTrackMatrixivNV, fn);
}

typedef void (GLAPIENTRYP _glptr_GetVertexAttribPointervNV)(GLuint, GLenum, GLvoid **);
#define CALL_GetVertexAttribPointervNV(disp, parameters) \
    (* GET_GetVertexAttribPointervNV(disp)) parameters
static inline _glptr_GetVertexAttribPointervNV GET_GetVertexAttribPointervNV(struct _glapi_table *disp) {
   return (_glptr_GetVertexAttribPointervNV) (GET_by_offset(disp, _gloffset_GetVertexAttribPointervNV));
}

static inline void SET_GetVertexAttribPointervNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLvoid **)) {
   SET_by_offset(disp, _gloffset_GetVertexAttribPointervNV, fn);
}

typedef void (GLAPIENTRYP _glptr_GetVertexAttribdvNV)(GLuint, GLenum, GLdouble *);
#define CALL_GetVertexAttribdvNV(disp, parameters) \
    (* GET_GetVertexAttribdvNV(disp)) parameters
static inline _glptr_GetVertexAttribdvNV GET_GetVertexAttribdvNV(struct _glapi_table *disp) {
   return (_glptr_GetVertexAttribdvNV) (GET_by_offset(disp, _gloffset_GetVertexAttribdvNV));
}

static inline void SET_GetVertexAttribdvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLdouble *)) {
   SET_by_offset(disp, _gloffset_GetVertexAttribdvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_GetVertexAttribfvNV)(GLuint, GLenum, GLfloat *);
#define CALL_GetVertexAttribfvNV(disp, parameters) \
    (* GET_GetVertexAttribfvNV(disp)) parameters
static inline _glptr_GetVertexAttribfvNV GET_GetVertexAttribfvNV(struct _glapi_table *disp) {
   return (_glptr_GetVertexAttribfvNV) (GET_by_offset(disp, _gloffset_GetVertexAttribfvNV));
}

static inline void SET_GetVertexAttribfvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetVertexAttribfvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_GetVertexAttribivNV)(GLuint, GLenum, GLint *);
#define CALL_GetVertexAttribivNV(disp, parameters) \
    (* GET_GetVertexAttribivNV(disp)) parameters
static inline _glptr_GetVertexAttribivNV GET_GetVertexAttribivNV(struct _glapi_table *disp) {
   return (_glptr_GetVertexAttribivNV) (GET_by_offset(disp, _gloffset_GetVertexAttribivNV));
}

static inline void SET_GetVertexAttribivNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetVertexAttribivNV, fn);
}

typedef GLboolean (GLAPIENTRYP _glptr_IsProgramNV)(GLuint);
#define CALL_IsProgramNV(disp, parameters) \
    (* GET_IsProgramNV(disp)) parameters
static inline _glptr_IsProgramNV GET_IsProgramNV(struct _glapi_table *disp) {
   return (_glptr_IsProgramNV) (GET_by_offset(disp, _gloffset_IsProgramNV));
}

static inline void SET_IsProgramNV(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_IsProgramNV, fn);
}

typedef void (GLAPIENTRYP _glptr_LoadProgramNV)(GLenum, GLuint, GLsizei, const GLubyte *);
#define CALL_LoadProgramNV(disp, parameters) \
    (* GET_LoadProgramNV(disp)) parameters
static inline _glptr_LoadProgramNV GET_LoadProgramNV(struct _glapi_table *disp) {
   return (_glptr_LoadProgramNV) (GET_by_offset(disp, _gloffset_LoadProgramNV));
}

static inline void SET_LoadProgramNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLsizei, const GLubyte *)) {
   SET_by_offset(disp, _gloffset_LoadProgramNV, fn);
}

typedef void (GLAPIENTRYP _glptr_ProgramParameters4dvNV)(GLenum, GLuint, GLsizei, const GLdouble *);
#define CALL_ProgramParameters4dvNV(disp, parameters) \
    (* GET_ProgramParameters4dvNV(disp)) parameters
static inline _glptr_ProgramParameters4dvNV GET_ProgramParameters4dvNV(struct _glapi_table *disp) {
   return (_glptr_ProgramParameters4dvNV) (GET_by_offset(disp, _gloffset_ProgramParameters4dvNV));
}

static inline void SET_ProgramParameters4dvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLsizei, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_ProgramParameters4dvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_ProgramParameters4fvNV)(GLenum, GLuint, GLsizei, const GLfloat *);
#define CALL_ProgramParameters4fvNV(disp, parameters) \
    (* GET_ProgramParameters4fvNV(disp)) parameters
static inline _glptr_ProgramParameters4fvNV GET_ProgramParameters4fvNV(struct _glapi_table *disp) {
   return (_glptr_ProgramParameters4fvNV) (GET_by_offset(disp, _gloffset_ProgramParameters4fvNV));
}

static inline void SET_ProgramParameters4fvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLsizei, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_ProgramParameters4fvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_RequestResidentProgramsNV)(GLsizei, const GLuint *);
#define CALL_RequestResidentProgramsNV(disp, parameters) \
    (* GET_RequestResidentProgramsNV(disp)) parameters
static inline _glptr_RequestResidentProgramsNV GET_RequestResidentProgramsNV(struct _glapi_table *disp) {
   return (_glptr_RequestResidentProgramsNV) (GET_by_offset(disp, _gloffset_RequestResidentProgramsNV));
}

static inline void SET_RequestResidentProgramsNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_RequestResidentProgramsNV, fn);
}

typedef void (GLAPIENTRYP _glptr_TrackMatrixNV)(GLenum, GLuint, GLenum, GLenum);
#define CALL_TrackMatrixNV(disp, parameters) \
    (* GET_TrackMatrixNV(disp)) parameters
static inline _glptr_TrackMatrixNV GET_TrackMatrixNV(struct _glapi_table *disp) {
   return (_glptr_TrackMatrixNV) (GET_by_offset(disp, _gloffset_TrackMatrixNV));
}

static inline void SET_TrackMatrixNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_TrackMatrixNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib1dNV)(GLuint, GLdouble);
#define CALL_VertexAttrib1dNV(disp, parameters) \
    (* GET_VertexAttrib1dNV(disp)) parameters
static inline _glptr_VertexAttrib1dNV GET_VertexAttrib1dNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib1dNV) (GET_by_offset(disp, _gloffset_VertexAttrib1dNV));
}

static inline void SET_VertexAttrib1dNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLdouble)) {
   SET_by_offset(disp, _gloffset_VertexAttrib1dNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib1dvNV)(GLuint, const GLdouble *);
#define CALL_VertexAttrib1dvNV(disp, parameters) \
    (* GET_VertexAttrib1dvNV(disp)) parameters
static inline _glptr_VertexAttrib1dvNV GET_VertexAttrib1dvNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib1dvNV) (GET_by_offset(disp, _gloffset_VertexAttrib1dvNV));
}

static inline void SET_VertexAttrib1dvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib1dvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib1fNV)(GLuint, GLfloat);
#define CALL_VertexAttrib1fNV(disp, parameters) \
    (* GET_VertexAttrib1fNV(disp)) parameters
static inline _glptr_VertexAttrib1fNV GET_VertexAttrib1fNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib1fNV) (GET_by_offset(disp, _gloffset_VertexAttrib1fNV));
}

static inline void SET_VertexAttrib1fNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLfloat)) {
   SET_by_offset(disp, _gloffset_VertexAttrib1fNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib1fvNV)(GLuint, const GLfloat *);
#define CALL_VertexAttrib1fvNV(disp, parameters) \
    (* GET_VertexAttrib1fvNV(disp)) parameters
static inline _glptr_VertexAttrib1fvNV GET_VertexAttrib1fvNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib1fvNV) (GET_by_offset(disp, _gloffset_VertexAttrib1fvNV));
}

static inline void SET_VertexAttrib1fvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib1fvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib1sNV)(GLuint, GLshort);
#define CALL_VertexAttrib1sNV(disp, parameters) \
    (* GET_VertexAttrib1sNV(disp)) parameters
static inline _glptr_VertexAttrib1sNV GET_VertexAttrib1sNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib1sNV) (GET_by_offset(disp, _gloffset_VertexAttrib1sNV));
}

static inline void SET_VertexAttrib1sNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLshort)) {
   SET_by_offset(disp, _gloffset_VertexAttrib1sNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib1svNV)(GLuint, const GLshort *);
#define CALL_VertexAttrib1svNV(disp, parameters) \
    (* GET_VertexAttrib1svNV(disp)) parameters
static inline _glptr_VertexAttrib1svNV GET_VertexAttrib1svNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib1svNV) (GET_by_offset(disp, _gloffset_VertexAttrib1svNV));
}

static inline void SET_VertexAttrib1svNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLshort *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib1svNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib2dNV)(GLuint, GLdouble, GLdouble);
#define CALL_VertexAttrib2dNV(disp, parameters) \
    (* GET_VertexAttrib2dNV(disp)) parameters
static inline _glptr_VertexAttrib2dNV GET_VertexAttrib2dNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib2dNV) (GET_by_offset(disp, _gloffset_VertexAttrib2dNV));
}

static inline void SET_VertexAttrib2dNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_VertexAttrib2dNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib2dvNV)(GLuint, const GLdouble *);
#define CALL_VertexAttrib2dvNV(disp, parameters) \
    (* GET_VertexAttrib2dvNV(disp)) parameters
static inline _glptr_VertexAttrib2dvNV GET_VertexAttrib2dvNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib2dvNV) (GET_by_offset(disp, _gloffset_VertexAttrib2dvNV));
}

static inline void SET_VertexAttrib2dvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib2dvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib2fNV)(GLuint, GLfloat, GLfloat);
#define CALL_VertexAttrib2fNV(disp, parameters) \
    (* GET_VertexAttrib2fNV(disp)) parameters
static inline _glptr_VertexAttrib2fNV GET_VertexAttrib2fNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib2fNV) (GET_by_offset(disp, _gloffset_VertexAttrib2fNV));
}

static inline void SET_VertexAttrib2fNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_VertexAttrib2fNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib2fvNV)(GLuint, const GLfloat *);
#define CALL_VertexAttrib2fvNV(disp, parameters) \
    (* GET_VertexAttrib2fvNV(disp)) parameters
static inline _glptr_VertexAttrib2fvNV GET_VertexAttrib2fvNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib2fvNV) (GET_by_offset(disp, _gloffset_VertexAttrib2fvNV));
}

static inline void SET_VertexAttrib2fvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib2fvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib2sNV)(GLuint, GLshort, GLshort);
#define CALL_VertexAttrib2sNV(disp, parameters) \
    (* GET_VertexAttrib2sNV(disp)) parameters
static inline _glptr_VertexAttrib2sNV GET_VertexAttrib2sNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib2sNV) (GET_by_offset(disp, _gloffset_VertexAttrib2sNV));
}

static inline void SET_VertexAttrib2sNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_VertexAttrib2sNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib2svNV)(GLuint, const GLshort *);
#define CALL_VertexAttrib2svNV(disp, parameters) \
    (* GET_VertexAttrib2svNV(disp)) parameters
static inline _glptr_VertexAttrib2svNV GET_VertexAttrib2svNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib2svNV) (GET_by_offset(disp, _gloffset_VertexAttrib2svNV));
}

static inline void SET_VertexAttrib2svNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLshort *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib2svNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib3dNV)(GLuint, GLdouble, GLdouble, GLdouble);
#define CALL_VertexAttrib3dNV(disp, parameters) \
    (* GET_VertexAttrib3dNV(disp)) parameters
static inline _glptr_VertexAttrib3dNV GET_VertexAttrib3dNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib3dNV) (GET_by_offset(disp, _gloffset_VertexAttrib3dNV));
}

static inline void SET_VertexAttrib3dNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_VertexAttrib3dNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib3dvNV)(GLuint, const GLdouble *);
#define CALL_VertexAttrib3dvNV(disp, parameters) \
    (* GET_VertexAttrib3dvNV(disp)) parameters
static inline _glptr_VertexAttrib3dvNV GET_VertexAttrib3dvNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib3dvNV) (GET_by_offset(disp, _gloffset_VertexAttrib3dvNV));
}

static inline void SET_VertexAttrib3dvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib3dvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib3fNV)(GLuint, GLfloat, GLfloat, GLfloat);
#define CALL_VertexAttrib3fNV(disp, parameters) \
    (* GET_VertexAttrib3fNV(disp)) parameters
static inline _glptr_VertexAttrib3fNV GET_VertexAttrib3fNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib3fNV) (GET_by_offset(disp, _gloffset_VertexAttrib3fNV));
}

static inline void SET_VertexAttrib3fNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_VertexAttrib3fNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib3fvNV)(GLuint, const GLfloat *);
#define CALL_VertexAttrib3fvNV(disp, parameters) \
    (* GET_VertexAttrib3fvNV(disp)) parameters
static inline _glptr_VertexAttrib3fvNV GET_VertexAttrib3fvNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib3fvNV) (GET_by_offset(disp, _gloffset_VertexAttrib3fvNV));
}

static inline void SET_VertexAttrib3fvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib3fvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib3sNV)(GLuint, GLshort, GLshort, GLshort);
#define CALL_VertexAttrib3sNV(disp, parameters) \
    (* GET_VertexAttrib3sNV(disp)) parameters
static inline _glptr_VertexAttrib3sNV GET_VertexAttrib3sNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib3sNV) (GET_by_offset(disp, _gloffset_VertexAttrib3sNV));
}

static inline void SET_VertexAttrib3sNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_VertexAttrib3sNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib3svNV)(GLuint, const GLshort *);
#define CALL_VertexAttrib3svNV(disp, parameters) \
    (* GET_VertexAttrib3svNV(disp)) parameters
static inline _glptr_VertexAttrib3svNV GET_VertexAttrib3svNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib3svNV) (GET_by_offset(disp, _gloffset_VertexAttrib3svNV));
}

static inline void SET_VertexAttrib3svNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLshort *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib3svNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib4dNV)(GLuint, GLdouble, GLdouble, GLdouble, GLdouble);
#define CALL_VertexAttrib4dNV(disp, parameters) \
    (* GET_VertexAttrib4dNV(disp)) parameters
static inline _glptr_VertexAttrib4dNV GET_VertexAttrib4dNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib4dNV) (GET_by_offset(disp, _gloffset_VertexAttrib4dNV));
}

static inline void SET_VertexAttrib4dNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4dNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib4dvNV)(GLuint, const GLdouble *);
#define CALL_VertexAttrib4dvNV(disp, parameters) \
    (* GET_VertexAttrib4dvNV(disp)) parameters
static inline _glptr_VertexAttrib4dvNV GET_VertexAttrib4dvNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib4dvNV) (GET_by_offset(disp, _gloffset_VertexAttrib4dvNV));
}

static inline void SET_VertexAttrib4dvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4dvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib4fNV)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat);
#define CALL_VertexAttrib4fNV(disp, parameters) \
    (* GET_VertexAttrib4fNV(disp)) parameters
static inline _glptr_VertexAttrib4fNV GET_VertexAttrib4fNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib4fNV) (GET_by_offset(disp, _gloffset_VertexAttrib4fNV));
}

static inline void SET_VertexAttrib4fNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4fNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib4fvNV)(GLuint, const GLfloat *);
#define CALL_VertexAttrib4fvNV(disp, parameters) \
    (* GET_VertexAttrib4fvNV(disp)) parameters
static inline _glptr_VertexAttrib4fvNV GET_VertexAttrib4fvNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib4fvNV) (GET_by_offset(disp, _gloffset_VertexAttrib4fvNV));
}

static inline void SET_VertexAttrib4fvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4fvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib4sNV)(GLuint, GLshort, GLshort, GLshort, GLshort);
#define CALL_VertexAttrib4sNV(disp, parameters) \
    (* GET_VertexAttrib4sNV(disp)) parameters
static inline _glptr_VertexAttrib4sNV GET_VertexAttrib4sNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib4sNV) (GET_by_offset(disp, _gloffset_VertexAttrib4sNV));
}

static inline void SET_VertexAttrib4sNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLshort, GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4sNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib4svNV)(GLuint, const GLshort *);
#define CALL_VertexAttrib4svNV(disp, parameters) \
    (* GET_VertexAttrib4svNV(disp)) parameters
static inline _glptr_VertexAttrib4svNV GET_VertexAttrib4svNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib4svNV) (GET_by_offset(disp, _gloffset_VertexAttrib4svNV));
}

static inline void SET_VertexAttrib4svNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLshort *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4svNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib4ubNV)(GLuint, GLubyte, GLubyte, GLubyte, GLubyte);
#define CALL_VertexAttrib4ubNV(disp, parameters) \
    (* GET_VertexAttrib4ubNV(disp)) parameters
static inline _glptr_VertexAttrib4ubNV GET_VertexAttrib4ubNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib4ubNV) (GET_by_offset(disp, _gloffset_VertexAttrib4ubNV));
}

static inline void SET_VertexAttrib4ubNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLubyte, GLubyte, GLubyte, GLubyte)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4ubNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttrib4ubvNV)(GLuint, const GLubyte *);
#define CALL_VertexAttrib4ubvNV(disp, parameters) \
    (* GET_VertexAttrib4ubvNV(disp)) parameters
static inline _glptr_VertexAttrib4ubvNV GET_VertexAttrib4ubvNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttrib4ubvNV) (GET_by_offset(disp, _gloffset_VertexAttrib4ubvNV));
}

static inline void SET_VertexAttrib4ubvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLubyte *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4ubvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribPointerNV)(GLuint, GLint, GLenum, GLsizei, const GLvoid *);
#define CALL_VertexAttribPointerNV(disp, parameters) \
    (* GET_VertexAttribPointerNV(disp)) parameters
static inline _glptr_VertexAttribPointerNV GET_VertexAttribPointerNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttribPointerNV) (GET_by_offset(disp, _gloffset_VertexAttribPointerNV));
}

static inline void SET_VertexAttribPointerNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLint, GLenum, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_VertexAttribPointerNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribs1dvNV)(GLuint, GLsizei, const GLdouble *);
#define CALL_VertexAttribs1dvNV(disp, parameters) \
    (* GET_VertexAttribs1dvNV(disp)) parameters
static inline _glptr_VertexAttribs1dvNV GET_VertexAttribs1dvNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttribs1dvNV) (GET_by_offset(disp, _gloffset_VertexAttribs1dvNV));
}

static inline void SET_VertexAttribs1dvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_VertexAttribs1dvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribs1fvNV)(GLuint, GLsizei, const GLfloat *);
#define CALL_VertexAttribs1fvNV(disp, parameters) \
    (* GET_VertexAttribs1fvNV(disp)) parameters
static inline _glptr_VertexAttribs1fvNV GET_VertexAttribs1fvNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttribs1fvNV) (GET_by_offset(disp, _gloffset_VertexAttribs1fvNV));
}

static inline void SET_VertexAttribs1fvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_VertexAttribs1fvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribs1svNV)(GLuint, GLsizei, const GLshort *);
#define CALL_VertexAttribs1svNV(disp, parameters) \
    (* GET_VertexAttribs1svNV(disp)) parameters
static inline _glptr_VertexAttribs1svNV GET_VertexAttribs1svNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttribs1svNV) (GET_by_offset(disp, _gloffset_VertexAttribs1svNV));
}

static inline void SET_VertexAttribs1svNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLshort *)) {
   SET_by_offset(disp, _gloffset_VertexAttribs1svNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribs2dvNV)(GLuint, GLsizei, const GLdouble *);
#define CALL_VertexAttribs2dvNV(disp, parameters) \
    (* GET_VertexAttribs2dvNV(disp)) parameters
static inline _glptr_VertexAttribs2dvNV GET_VertexAttribs2dvNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttribs2dvNV) (GET_by_offset(disp, _gloffset_VertexAttribs2dvNV));
}

static inline void SET_VertexAttribs2dvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_VertexAttribs2dvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribs2fvNV)(GLuint, GLsizei, const GLfloat *);
#define CALL_VertexAttribs2fvNV(disp, parameters) \
    (* GET_VertexAttribs2fvNV(disp)) parameters
static inline _glptr_VertexAttribs2fvNV GET_VertexAttribs2fvNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttribs2fvNV) (GET_by_offset(disp, _gloffset_VertexAttribs2fvNV));
}

static inline void SET_VertexAttribs2fvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_VertexAttribs2fvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribs2svNV)(GLuint, GLsizei, const GLshort *);
#define CALL_VertexAttribs2svNV(disp, parameters) \
    (* GET_VertexAttribs2svNV(disp)) parameters
static inline _glptr_VertexAttribs2svNV GET_VertexAttribs2svNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttribs2svNV) (GET_by_offset(disp, _gloffset_VertexAttribs2svNV));
}

static inline void SET_VertexAttribs2svNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLshort *)) {
   SET_by_offset(disp, _gloffset_VertexAttribs2svNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribs3dvNV)(GLuint, GLsizei, const GLdouble *);
#define CALL_VertexAttribs3dvNV(disp, parameters) \
    (* GET_VertexAttribs3dvNV(disp)) parameters
static inline _glptr_VertexAttribs3dvNV GET_VertexAttribs3dvNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttribs3dvNV) (GET_by_offset(disp, _gloffset_VertexAttribs3dvNV));
}

static inline void SET_VertexAttribs3dvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_VertexAttribs3dvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribs3fvNV)(GLuint, GLsizei, const GLfloat *);
#define CALL_VertexAttribs3fvNV(disp, parameters) \
    (* GET_VertexAttribs3fvNV(disp)) parameters
static inline _glptr_VertexAttribs3fvNV GET_VertexAttribs3fvNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttribs3fvNV) (GET_by_offset(disp, _gloffset_VertexAttribs3fvNV));
}

static inline void SET_VertexAttribs3fvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_VertexAttribs3fvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribs3svNV)(GLuint, GLsizei, const GLshort *);
#define CALL_VertexAttribs3svNV(disp, parameters) \
    (* GET_VertexAttribs3svNV(disp)) parameters
static inline _glptr_VertexAttribs3svNV GET_VertexAttribs3svNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttribs3svNV) (GET_by_offset(disp, _gloffset_VertexAttribs3svNV));
}

static inline void SET_VertexAttribs3svNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLshort *)) {
   SET_by_offset(disp, _gloffset_VertexAttribs3svNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribs4dvNV)(GLuint, GLsizei, const GLdouble *);
#define CALL_VertexAttribs4dvNV(disp, parameters) \
    (* GET_VertexAttribs4dvNV(disp)) parameters
static inline _glptr_VertexAttribs4dvNV GET_VertexAttribs4dvNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttribs4dvNV) (GET_by_offset(disp, _gloffset_VertexAttribs4dvNV));
}

static inline void SET_VertexAttribs4dvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_VertexAttribs4dvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribs4fvNV)(GLuint, GLsizei, const GLfloat *);
#define CALL_VertexAttribs4fvNV(disp, parameters) \
    (* GET_VertexAttribs4fvNV(disp)) parameters
static inline _glptr_VertexAttribs4fvNV GET_VertexAttribs4fvNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttribs4fvNV) (GET_by_offset(disp, _gloffset_VertexAttribs4fvNV));
}

static inline void SET_VertexAttribs4fvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_VertexAttribs4fvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribs4svNV)(GLuint, GLsizei, const GLshort *);
#define CALL_VertexAttribs4svNV(disp, parameters) \
    (* GET_VertexAttribs4svNV(disp)) parameters
static inline _glptr_VertexAttribs4svNV GET_VertexAttribs4svNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttribs4svNV) (GET_by_offset(disp, _gloffset_VertexAttribs4svNV));
}

static inline void SET_VertexAttribs4svNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLshort *)) {
   SET_by_offset(disp, _gloffset_VertexAttribs4svNV, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribs4ubvNV)(GLuint, GLsizei, const GLubyte *);
#define CALL_VertexAttribs4ubvNV(disp, parameters) \
    (* GET_VertexAttribs4ubvNV(disp)) parameters
static inline _glptr_VertexAttribs4ubvNV GET_VertexAttribs4ubvNV(struct _glapi_table *disp) {
   return (_glptr_VertexAttribs4ubvNV) (GET_by_offset(disp, _gloffset_VertexAttribs4ubvNV));
}

static inline void SET_VertexAttribs4ubvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLubyte *)) {
   SET_by_offset(disp, _gloffset_VertexAttribs4ubvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_GetTexBumpParameterfvATI)(GLenum, GLfloat *);
#define CALL_GetTexBumpParameterfvATI(disp, parameters) \
    (* GET_GetTexBumpParameterfvATI(disp)) parameters
static inline _glptr_GetTexBumpParameterfvATI GET_GetTexBumpParameterfvATI(struct _glapi_table *disp) {
   return (_glptr_GetTexBumpParameterfvATI) (GET_by_offset(disp, _gloffset_GetTexBumpParameterfvATI));
}

static inline void SET_GetTexBumpParameterfvATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetTexBumpParameterfvATI, fn);
}

typedef void (GLAPIENTRYP _glptr_GetTexBumpParameterivATI)(GLenum, GLint *);
#define CALL_GetTexBumpParameterivATI(disp, parameters) \
    (* GET_GetTexBumpParameterivATI(disp)) parameters
static inline _glptr_GetTexBumpParameterivATI GET_GetTexBumpParameterivATI(struct _glapi_table *disp) {
   return (_glptr_GetTexBumpParameterivATI) (GET_by_offset(disp, _gloffset_GetTexBumpParameterivATI));
}

static inline void SET_GetTexBumpParameterivATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetTexBumpParameterivATI, fn);
}

typedef void (GLAPIENTRYP _glptr_TexBumpParameterfvATI)(GLenum, const GLfloat *);
#define CALL_TexBumpParameterfvATI(disp, parameters) \
    (* GET_TexBumpParameterfvATI(disp)) parameters
static inline _glptr_TexBumpParameterfvATI GET_TexBumpParameterfvATI(struct _glapi_table *disp) {
   return (_glptr_TexBumpParameterfvATI) (GET_by_offset(disp, _gloffset_TexBumpParameterfvATI));
}

static inline void SET_TexBumpParameterfvATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_TexBumpParameterfvATI, fn);
}

typedef void (GLAPIENTRYP _glptr_TexBumpParameterivATI)(GLenum, const GLint *);
#define CALL_TexBumpParameterivATI(disp, parameters) \
    (* GET_TexBumpParameterivATI(disp)) parameters
static inline _glptr_TexBumpParameterivATI GET_TexBumpParameterivATI(struct _glapi_table *disp) {
   return (_glptr_TexBumpParameterivATI) (GET_by_offset(disp, _gloffset_TexBumpParameterivATI));
}

static inline void SET_TexBumpParameterivATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_TexBumpParameterivATI, fn);
}

typedef void (GLAPIENTRYP _glptr_AlphaFragmentOp1ATI)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint);
#define CALL_AlphaFragmentOp1ATI(disp, parameters) \
    (* GET_AlphaFragmentOp1ATI(disp)) parameters
static inline _glptr_AlphaFragmentOp1ATI GET_AlphaFragmentOp1ATI(struct _glapi_table *disp) {
   return (_glptr_AlphaFragmentOp1ATI) (GET_by_offset(disp, _gloffset_AlphaFragmentOp1ATI));
}

static inline void SET_AlphaFragmentOp1ATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_AlphaFragmentOp1ATI, fn);
}

typedef void (GLAPIENTRYP _glptr_AlphaFragmentOp2ATI)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
#define CALL_AlphaFragmentOp2ATI(disp, parameters) \
    (* GET_AlphaFragmentOp2ATI(disp)) parameters
static inline _glptr_AlphaFragmentOp2ATI GET_AlphaFragmentOp2ATI(struct _glapi_table *disp) {
   return (_glptr_AlphaFragmentOp2ATI) (GET_by_offset(disp, _gloffset_AlphaFragmentOp2ATI));
}

static inline void SET_AlphaFragmentOp2ATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_AlphaFragmentOp2ATI, fn);
}

typedef void (GLAPIENTRYP _glptr_AlphaFragmentOp3ATI)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
#define CALL_AlphaFragmentOp3ATI(disp, parameters) \
    (* GET_AlphaFragmentOp3ATI(disp)) parameters
static inline _glptr_AlphaFragmentOp3ATI GET_AlphaFragmentOp3ATI(struct _glapi_table *disp) {
   return (_glptr_AlphaFragmentOp3ATI) (GET_by_offset(disp, _gloffset_AlphaFragmentOp3ATI));
}

static inline void SET_AlphaFragmentOp3ATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_AlphaFragmentOp3ATI, fn);
}

typedef void (GLAPIENTRYP _glptr_BeginFragmentShaderATI)(void);
#define CALL_BeginFragmentShaderATI(disp, parameters) \
    (* GET_BeginFragmentShaderATI(disp)) parameters
static inline _glptr_BeginFragmentShaderATI GET_BeginFragmentShaderATI(struct _glapi_table *disp) {
   return (_glptr_BeginFragmentShaderATI) (GET_by_offset(disp, _gloffset_BeginFragmentShaderATI));
}

static inline void SET_BeginFragmentShaderATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_BeginFragmentShaderATI, fn);
}

typedef void (GLAPIENTRYP _glptr_BindFragmentShaderATI)(GLuint);
#define CALL_BindFragmentShaderATI(disp, parameters) \
    (* GET_BindFragmentShaderATI(disp)) parameters
static inline _glptr_BindFragmentShaderATI GET_BindFragmentShaderATI(struct _glapi_table *disp) {
   return (_glptr_BindFragmentShaderATI) (GET_by_offset(disp, _gloffset_BindFragmentShaderATI));
}

static inline void SET_BindFragmentShaderATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_BindFragmentShaderATI, fn);
}

typedef void (GLAPIENTRYP _glptr_ColorFragmentOp1ATI)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
#define CALL_ColorFragmentOp1ATI(disp, parameters) \
    (* GET_ColorFragmentOp1ATI(disp)) parameters
static inline _glptr_ColorFragmentOp1ATI GET_ColorFragmentOp1ATI(struct _glapi_table *disp) {
   return (_glptr_ColorFragmentOp1ATI) (GET_by_offset(disp, _gloffset_ColorFragmentOp1ATI));
}

static inline void SET_ColorFragmentOp1ATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_ColorFragmentOp1ATI, fn);
}

typedef void (GLAPIENTRYP _glptr_ColorFragmentOp2ATI)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
#define CALL_ColorFragmentOp2ATI(disp, parameters) \
    (* GET_ColorFragmentOp2ATI(disp)) parameters
static inline _glptr_ColorFragmentOp2ATI GET_ColorFragmentOp2ATI(struct _glapi_table *disp) {
   return (_glptr_ColorFragmentOp2ATI) (GET_by_offset(disp, _gloffset_ColorFragmentOp2ATI));
}

static inline void SET_ColorFragmentOp2ATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_ColorFragmentOp2ATI, fn);
}

typedef void (GLAPIENTRYP _glptr_ColorFragmentOp3ATI)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
#define CALL_ColorFragmentOp3ATI(disp, parameters) \
    (* GET_ColorFragmentOp3ATI(disp)) parameters
static inline _glptr_ColorFragmentOp3ATI GET_ColorFragmentOp3ATI(struct _glapi_table *disp) {
   return (_glptr_ColorFragmentOp3ATI) (GET_by_offset(disp, _gloffset_ColorFragmentOp3ATI));
}

static inline void SET_ColorFragmentOp3ATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_ColorFragmentOp3ATI, fn);
}

typedef void (GLAPIENTRYP _glptr_DeleteFragmentShaderATI)(GLuint);
#define CALL_DeleteFragmentShaderATI(disp, parameters) \
    (* GET_DeleteFragmentShaderATI(disp)) parameters
static inline _glptr_DeleteFragmentShaderATI GET_DeleteFragmentShaderATI(struct _glapi_table *disp) {
   return (_glptr_DeleteFragmentShaderATI) (GET_by_offset(disp, _gloffset_DeleteFragmentShaderATI));
}

static inline void SET_DeleteFragmentShaderATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_DeleteFragmentShaderATI, fn);
}

typedef void (GLAPIENTRYP _glptr_EndFragmentShaderATI)(void);
#define CALL_EndFragmentShaderATI(disp, parameters) \
    (* GET_EndFragmentShaderATI(disp)) parameters
static inline _glptr_EndFragmentShaderATI GET_EndFragmentShaderATI(struct _glapi_table *disp) {
   return (_glptr_EndFragmentShaderATI) (GET_by_offset(disp, _gloffset_EndFragmentShaderATI));
}

static inline void SET_EndFragmentShaderATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_EndFragmentShaderATI, fn);
}

typedef GLuint (GLAPIENTRYP _glptr_GenFragmentShadersATI)(GLuint);
#define CALL_GenFragmentShadersATI(disp, parameters) \
    (* GET_GenFragmentShadersATI(disp)) parameters
static inline _glptr_GenFragmentShadersATI GET_GenFragmentShadersATI(struct _glapi_table *disp) {
   return (_glptr_GenFragmentShadersATI) (GET_by_offset(disp, _gloffset_GenFragmentShadersATI));
}

static inline void SET_GenFragmentShadersATI(struct _glapi_table *disp, GLuint (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_GenFragmentShadersATI, fn);
}

typedef void (GLAPIENTRYP _glptr_PassTexCoordATI)(GLuint, GLuint, GLenum);
#define CALL_PassTexCoordATI(disp, parameters) \
    (* GET_PassTexCoordATI(disp)) parameters
static inline _glptr_PassTexCoordATI GET_PassTexCoordATI(struct _glapi_table *disp) {
   return (_glptr_PassTexCoordATI) (GET_by_offset(disp, _gloffset_PassTexCoordATI));
}

static inline void SET_PassTexCoordATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint, GLenum)) {
   SET_by_offset(disp, _gloffset_PassTexCoordATI, fn);
}

typedef void (GLAPIENTRYP _glptr_SampleMapATI)(GLuint, GLuint, GLenum);
#define CALL_SampleMapATI(disp, parameters) \
    (* GET_SampleMapATI(disp)) parameters
static inline _glptr_SampleMapATI GET_SampleMapATI(struct _glapi_table *disp) {
   return (_glptr_SampleMapATI) (GET_by_offset(disp, _gloffset_SampleMapATI));
}

static inline void SET_SampleMapATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint, GLenum)) {
   SET_by_offset(disp, _gloffset_SampleMapATI, fn);
}

typedef void (GLAPIENTRYP _glptr_SetFragmentShaderConstantATI)(GLuint, const GLfloat *);
#define CALL_SetFragmentShaderConstantATI(disp, parameters) \
    (* GET_SetFragmentShaderConstantATI(disp)) parameters
static inline _glptr_SetFragmentShaderConstantATI GET_SetFragmentShaderConstantATI(struct _glapi_table *disp) {
   return (_glptr_SetFragmentShaderConstantATI) (GET_by_offset(disp, _gloffset_SetFragmentShaderConstantATI));
}

static inline void SET_SetFragmentShaderConstantATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_SetFragmentShaderConstantATI, fn);
}

typedef void (GLAPIENTRYP _glptr_PointParameteriNV)(GLenum, GLint);
#define CALL_PointParameteriNV(disp, parameters) \
    (* GET_PointParameteriNV(disp)) parameters
static inline _glptr_PointParameteriNV GET_PointParameteriNV(struct _glapi_table *disp) {
   return (_glptr_PointParameteriNV) (GET_by_offset(disp, _gloffset_PointParameteriNV));
}

static inline void SET_PointParameteriNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_PointParameteriNV, fn);
}

typedef void (GLAPIENTRYP _glptr_PointParameterivNV)(GLenum, const GLint *);
#define CALL_PointParameterivNV(disp, parameters) \
    (* GET_PointParameterivNV(disp)) parameters
static inline _glptr_PointParameterivNV GET_PointParameterivNV(struct _glapi_table *disp) {
   return (_glptr_PointParameterivNV) (GET_by_offset(disp, _gloffset_PointParameterivNV));
}

static inline void SET_PointParameterivNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_PointParameterivNV, fn);
}

typedef void (GLAPIENTRYP _glptr_ActiveStencilFaceEXT)(GLenum);
#define CALL_ActiveStencilFaceEXT(disp, parameters) \
    (* GET_ActiveStencilFaceEXT(disp)) parameters
static inline _glptr_ActiveStencilFaceEXT GET_ActiveStencilFaceEXT(struct _glapi_table *disp) {
   return (_glptr_ActiveStencilFaceEXT) (GET_by_offset(disp, _gloffset_ActiveStencilFaceEXT));
}

static inline void SET_ActiveStencilFaceEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_ActiveStencilFaceEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_BindVertexArrayAPPLE)(GLuint);
#define CALL_BindVertexArrayAPPLE(disp, parameters) \
    (* GET_BindVertexArrayAPPLE(disp)) parameters
static inline _glptr_BindVertexArrayAPPLE GET_BindVertexArrayAPPLE(struct _glapi_table *disp) {
   return (_glptr_BindVertexArrayAPPLE) (GET_by_offset(disp, _gloffset_BindVertexArrayAPPLE));
}

static inline void SET_BindVertexArrayAPPLE(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_BindVertexArrayAPPLE, fn);
}

typedef void (GLAPIENTRYP _glptr_DeleteVertexArraysAPPLE)(GLsizei, const GLuint *);
#define CALL_DeleteVertexArraysAPPLE(disp, parameters) \
    (* GET_DeleteVertexArraysAPPLE(disp)) parameters
static inline _glptr_DeleteVertexArraysAPPLE GET_DeleteVertexArraysAPPLE(struct _glapi_table *disp) {
   return (_glptr_DeleteVertexArraysAPPLE) (GET_by_offset(disp, _gloffset_DeleteVertexArraysAPPLE));
}

static inline void SET_DeleteVertexArraysAPPLE(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_DeleteVertexArraysAPPLE, fn);
}

typedef void (GLAPIENTRYP _glptr_GenVertexArraysAPPLE)(GLsizei, GLuint *);
#define CALL_GenVertexArraysAPPLE(disp, parameters) \
    (* GET_GenVertexArraysAPPLE(disp)) parameters
static inline _glptr_GenVertexArraysAPPLE GET_GenVertexArraysAPPLE(struct _glapi_table *disp) {
   return (_glptr_GenVertexArraysAPPLE) (GET_by_offset(disp, _gloffset_GenVertexArraysAPPLE));
}

static inline void SET_GenVertexArraysAPPLE(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLuint *)) {
   SET_by_offset(disp, _gloffset_GenVertexArraysAPPLE, fn);
}

typedef GLboolean (GLAPIENTRYP _glptr_IsVertexArrayAPPLE)(GLuint);
#define CALL_IsVertexArrayAPPLE(disp, parameters) \
    (* GET_IsVertexArrayAPPLE(disp)) parameters
static inline _glptr_IsVertexArrayAPPLE GET_IsVertexArrayAPPLE(struct _glapi_table *disp) {
   return (_glptr_IsVertexArrayAPPLE) (GET_by_offset(disp, _gloffset_IsVertexArrayAPPLE));
}

static inline void SET_IsVertexArrayAPPLE(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_IsVertexArrayAPPLE, fn);
}

typedef void (GLAPIENTRYP _glptr_GetProgramNamedParameterdvNV)(GLuint, GLsizei, const GLubyte *, GLdouble *);
#define CALL_GetProgramNamedParameterdvNV(disp, parameters) \
    (* GET_GetProgramNamedParameterdvNV(disp)) parameters
static inline _glptr_GetProgramNamedParameterdvNV GET_GetProgramNamedParameterdvNV(struct _glapi_table *disp) {
   return (_glptr_GetProgramNamedParameterdvNV) (GET_by_offset(disp, _gloffset_GetProgramNamedParameterdvNV));
}

static inline void SET_GetProgramNamedParameterdvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLubyte *, GLdouble *)) {
   SET_by_offset(disp, _gloffset_GetProgramNamedParameterdvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_GetProgramNamedParameterfvNV)(GLuint, GLsizei, const GLubyte *, GLfloat *);
#define CALL_GetProgramNamedParameterfvNV(disp, parameters) \
    (* GET_GetProgramNamedParameterfvNV(disp)) parameters
static inline _glptr_GetProgramNamedParameterfvNV GET_GetProgramNamedParameterfvNV(struct _glapi_table *disp) {
   return (_glptr_GetProgramNamedParameterfvNV) (GET_by_offset(disp, _gloffset_GetProgramNamedParameterfvNV));
}

static inline void SET_GetProgramNamedParameterfvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLubyte *, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetProgramNamedParameterfvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_ProgramNamedParameter4dNV)(GLuint, GLsizei, const GLubyte *, GLdouble, GLdouble, GLdouble, GLdouble);
#define CALL_ProgramNamedParameter4dNV(disp, parameters) \
    (* GET_ProgramNamedParameter4dNV(disp)) parameters
static inline _glptr_ProgramNamedParameter4dNV GET_ProgramNamedParameter4dNV(struct _glapi_table *disp) {
   return (_glptr_ProgramNamedParameter4dNV) (GET_by_offset(disp, _gloffset_ProgramNamedParameter4dNV));
}

static inline void SET_ProgramNamedParameter4dNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLubyte *, GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_ProgramNamedParameter4dNV, fn);
}

typedef void (GLAPIENTRYP _glptr_ProgramNamedParameter4dvNV)(GLuint, GLsizei, const GLubyte *, const GLdouble *);
#define CALL_ProgramNamedParameter4dvNV(disp, parameters) \
    (* GET_ProgramNamedParameter4dvNV(disp)) parameters
static inline _glptr_ProgramNamedParameter4dvNV GET_ProgramNamedParameter4dvNV(struct _glapi_table *disp) {
   return (_glptr_ProgramNamedParameter4dvNV) (GET_by_offset(disp, _gloffset_ProgramNamedParameter4dvNV));
}

static inline void SET_ProgramNamedParameter4dvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLubyte *, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_ProgramNamedParameter4dvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_ProgramNamedParameter4fNV)(GLuint, GLsizei, const GLubyte *, GLfloat, GLfloat, GLfloat, GLfloat);
#define CALL_ProgramNamedParameter4fNV(disp, parameters) \
    (* GET_ProgramNamedParameter4fNV(disp)) parameters
static inline _glptr_ProgramNamedParameter4fNV GET_ProgramNamedParameter4fNV(struct _glapi_table *disp) {
   return (_glptr_ProgramNamedParameter4fNV) (GET_by_offset(disp, _gloffset_ProgramNamedParameter4fNV));
}

static inline void SET_ProgramNamedParameter4fNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLubyte *, GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_ProgramNamedParameter4fNV, fn);
}

typedef void (GLAPIENTRYP _glptr_ProgramNamedParameter4fvNV)(GLuint, GLsizei, const GLubyte *, const GLfloat *);
#define CALL_ProgramNamedParameter4fvNV(disp, parameters) \
    (* GET_ProgramNamedParameter4fvNV(disp)) parameters
static inline _glptr_ProgramNamedParameter4fvNV GET_ProgramNamedParameter4fvNV(struct _glapi_table *disp) {
   return (_glptr_ProgramNamedParameter4fvNV) (GET_by_offset(disp, _gloffset_ProgramNamedParameter4fvNV));
}

static inline void SET_ProgramNamedParameter4fvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLubyte *, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_ProgramNamedParameter4fvNV, fn);
}

typedef void (GLAPIENTRYP _glptr_PrimitiveRestartIndexNV)(GLuint);
#define CALL_PrimitiveRestartIndexNV(disp, parameters) \
    (* GET_PrimitiveRestartIndexNV(disp)) parameters
static inline _glptr_PrimitiveRestartIndexNV GET_PrimitiveRestartIndexNV(struct _glapi_table *disp) {
   return (_glptr_PrimitiveRestartIndexNV) (GET_by_offset(disp, _gloffset_PrimitiveRestartIndexNV));
}

static inline void SET_PrimitiveRestartIndexNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_PrimitiveRestartIndexNV, fn);
}

typedef void (GLAPIENTRYP _glptr_PrimitiveRestartNV)(void);
#define CALL_PrimitiveRestartNV(disp, parameters) \
    (* GET_PrimitiveRestartNV(disp)) parameters
static inline _glptr_PrimitiveRestartNV GET_PrimitiveRestartNV(struct _glapi_table *disp) {
   return (_glptr_PrimitiveRestartNV) (GET_by_offset(disp, _gloffset_PrimitiveRestartNV));
}

static inline void SET_PrimitiveRestartNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_PrimitiveRestartNV, fn);
}

typedef void (GLAPIENTRYP _glptr_DepthBoundsEXT)(GLclampd, GLclampd);
#define CALL_DepthBoundsEXT(disp, parameters) \
    (* GET_DepthBoundsEXT(disp)) parameters
static inline _glptr_DepthBoundsEXT GET_DepthBoundsEXT(struct _glapi_table *disp) {
   return (_glptr_DepthBoundsEXT) (GET_by_offset(disp, _gloffset_DepthBoundsEXT));
}

static inline void SET_DepthBoundsEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLclampd, GLclampd)) {
   SET_by_offset(disp, _gloffset_DepthBoundsEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_BlendEquationSeparateEXT)(GLenum, GLenum);
#define CALL_BlendEquationSeparateEXT(disp, parameters) \
    (* GET_BlendEquationSeparateEXT(disp)) parameters
static inline _glptr_BlendEquationSeparateEXT GET_BlendEquationSeparateEXT(struct _glapi_table *disp) {
   return (_glptr_BlendEquationSeparateEXT) (GET_by_offset(disp, _gloffset_BlendEquationSeparateEXT));
}

static inline void SET_BlendEquationSeparateEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_BlendEquationSeparateEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_BindFramebufferEXT)(GLenum, GLuint);
#define CALL_BindFramebufferEXT(disp, parameters) \
    (* GET_BindFramebufferEXT(disp)) parameters
static inline _glptr_BindFramebufferEXT GET_BindFramebufferEXT(struct _glapi_table *disp) {
   return (_glptr_BindFramebufferEXT) (GET_by_offset(disp, _gloffset_BindFramebufferEXT));
}

static inline void SET_BindFramebufferEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_BindFramebufferEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_BindRenderbufferEXT)(GLenum, GLuint);
#define CALL_BindRenderbufferEXT(disp, parameters) \
    (* GET_BindRenderbufferEXT(disp)) parameters
static inline _glptr_BindRenderbufferEXT GET_BindRenderbufferEXT(struct _glapi_table *disp) {
   return (_glptr_BindRenderbufferEXT) (GET_by_offset(disp, _gloffset_BindRenderbufferEXT));
}

static inline void SET_BindRenderbufferEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_BindRenderbufferEXT, fn);
}

typedef GLenum (GLAPIENTRYP _glptr_CheckFramebufferStatusEXT)(GLenum);
#define CALL_CheckFramebufferStatusEXT(disp, parameters) \
    (* GET_CheckFramebufferStatusEXT(disp)) parameters
static inline _glptr_CheckFramebufferStatusEXT GET_CheckFramebufferStatusEXT(struct _glapi_table *disp) {
   return (_glptr_CheckFramebufferStatusEXT) (GET_by_offset(disp, _gloffset_CheckFramebufferStatusEXT));
}

static inline void SET_CheckFramebufferStatusEXT(struct _glapi_table *disp, GLenum (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_CheckFramebufferStatusEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_DeleteFramebuffersEXT)(GLsizei, const GLuint *);
#define CALL_DeleteFramebuffersEXT(disp, parameters) \
    (* GET_DeleteFramebuffersEXT(disp)) parameters
static inline _glptr_DeleteFramebuffersEXT GET_DeleteFramebuffersEXT(struct _glapi_table *disp) {
   return (_glptr_DeleteFramebuffersEXT) (GET_by_offset(disp, _gloffset_DeleteFramebuffersEXT));
}

static inline void SET_DeleteFramebuffersEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_DeleteFramebuffersEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_DeleteRenderbuffersEXT)(GLsizei, const GLuint *);
#define CALL_DeleteRenderbuffersEXT(disp, parameters) \
    (* GET_DeleteRenderbuffersEXT(disp)) parameters
static inline _glptr_DeleteRenderbuffersEXT GET_DeleteRenderbuffersEXT(struct _glapi_table *disp) {
   return (_glptr_DeleteRenderbuffersEXT) (GET_by_offset(disp, _gloffset_DeleteRenderbuffersEXT));
}

static inline void SET_DeleteRenderbuffersEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_DeleteRenderbuffersEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_FramebufferRenderbufferEXT)(GLenum, GLenum, GLenum, GLuint);
#define CALL_FramebufferRenderbufferEXT(disp, parameters) \
    (* GET_FramebufferRenderbufferEXT(disp)) parameters
static inline _glptr_FramebufferRenderbufferEXT GET_FramebufferRenderbufferEXT(struct _glapi_table *disp) {
   return (_glptr_FramebufferRenderbufferEXT) (GET_by_offset(disp, _gloffset_FramebufferRenderbufferEXT));
}

static inline void SET_FramebufferRenderbufferEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_FramebufferRenderbufferEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_FramebufferTexture1DEXT)(GLenum, GLenum, GLenum, GLuint, GLint);
#define CALL_FramebufferTexture1DEXT(disp, parameters) \
    (* GET_FramebufferTexture1DEXT(disp)) parameters
static inline _glptr_FramebufferTexture1DEXT GET_FramebufferTexture1DEXT(struct _glapi_table *disp) {
   return (_glptr_FramebufferTexture1DEXT) (GET_by_offset(disp, _gloffset_FramebufferTexture1DEXT));
}

static inline void SET_FramebufferTexture1DEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLuint, GLint)) {
   SET_by_offset(disp, _gloffset_FramebufferTexture1DEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_FramebufferTexture2DEXT)(GLenum, GLenum, GLenum, GLuint, GLint);
#define CALL_FramebufferTexture2DEXT(disp, parameters) \
    (* GET_FramebufferTexture2DEXT(disp)) parameters
static inline _glptr_FramebufferTexture2DEXT GET_FramebufferTexture2DEXT(struct _glapi_table *disp) {
   return (_glptr_FramebufferTexture2DEXT) (GET_by_offset(disp, _gloffset_FramebufferTexture2DEXT));
}

static inline void SET_FramebufferTexture2DEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLuint, GLint)) {
   SET_by_offset(disp, _gloffset_FramebufferTexture2DEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_FramebufferTexture3DEXT)(GLenum, GLenum, GLenum, GLuint, GLint, GLint);
#define CALL_FramebufferTexture3DEXT(disp, parameters) \
    (* GET_FramebufferTexture3DEXT(disp)) parameters
static inline _glptr_FramebufferTexture3DEXT GET_FramebufferTexture3DEXT(struct _glapi_table *disp) {
   return (_glptr_FramebufferTexture3DEXT) (GET_by_offset(disp, _gloffset_FramebufferTexture3DEXT));
}

static inline void SET_FramebufferTexture3DEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLuint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_FramebufferTexture3DEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_GenFramebuffersEXT)(GLsizei, GLuint *);
#define CALL_GenFramebuffersEXT(disp, parameters) \
    (* GET_GenFramebuffersEXT(disp)) parameters
static inline _glptr_GenFramebuffersEXT GET_GenFramebuffersEXT(struct _glapi_table *disp) {
   return (_glptr_GenFramebuffersEXT) (GET_by_offset(disp, _gloffset_GenFramebuffersEXT));
}

static inline void SET_GenFramebuffersEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLuint *)) {
   SET_by_offset(disp, _gloffset_GenFramebuffersEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_GenRenderbuffersEXT)(GLsizei, GLuint *);
#define CALL_GenRenderbuffersEXT(disp, parameters) \
    (* GET_GenRenderbuffersEXT(disp)) parameters
static inline _glptr_GenRenderbuffersEXT GET_GenRenderbuffersEXT(struct _glapi_table *disp) {
   return (_glptr_GenRenderbuffersEXT) (GET_by_offset(disp, _gloffset_GenRenderbuffersEXT));
}

static inline void SET_GenRenderbuffersEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLuint *)) {
   SET_by_offset(disp, _gloffset_GenRenderbuffersEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_GenerateMipmapEXT)(GLenum);
#define CALL_GenerateMipmapEXT(disp, parameters) \
    (* GET_GenerateMipmapEXT(disp)) parameters
static inline _glptr_GenerateMipmapEXT GET_GenerateMipmapEXT(struct _glapi_table *disp) {
   return (_glptr_GenerateMipmapEXT) (GET_by_offset(disp, _gloffset_GenerateMipmapEXT));
}

static inline void SET_GenerateMipmapEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_GenerateMipmapEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_GetFramebufferAttachmentParameterivEXT)(GLenum, GLenum, GLenum, GLint *);
#define CALL_GetFramebufferAttachmentParameterivEXT(disp, parameters) \
    (* GET_GetFramebufferAttachmentParameterivEXT(disp)) parameters
static inline _glptr_GetFramebufferAttachmentParameterivEXT GET_GetFramebufferAttachmentParameterivEXT(struct _glapi_table *disp) {
   return (_glptr_GetFramebufferAttachmentParameterivEXT) (GET_by_offset(disp, _gloffset_GetFramebufferAttachmentParameterivEXT));
}

static inline void SET_GetFramebufferAttachmentParameterivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetFramebufferAttachmentParameterivEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_GetRenderbufferParameterivEXT)(GLenum, GLenum, GLint *);
#define CALL_GetRenderbufferParameterivEXT(disp, parameters) \
    (* GET_GetRenderbufferParameterivEXT(disp)) parameters
static inline _glptr_GetRenderbufferParameterivEXT GET_GetRenderbufferParameterivEXT(struct _glapi_table *disp) {
   return (_glptr_GetRenderbufferParameterivEXT) (GET_by_offset(disp, _gloffset_GetRenderbufferParameterivEXT));
}

static inline void SET_GetRenderbufferParameterivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetRenderbufferParameterivEXT, fn);
}

typedef GLboolean (GLAPIENTRYP _glptr_IsFramebufferEXT)(GLuint);
#define CALL_IsFramebufferEXT(disp, parameters) \
    (* GET_IsFramebufferEXT(disp)) parameters
static inline _glptr_IsFramebufferEXT GET_IsFramebufferEXT(struct _glapi_table *disp) {
   return (_glptr_IsFramebufferEXT) (GET_by_offset(disp, _gloffset_IsFramebufferEXT));
}

static inline void SET_IsFramebufferEXT(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_IsFramebufferEXT, fn);
}

typedef GLboolean (GLAPIENTRYP _glptr_IsRenderbufferEXT)(GLuint);
#define CALL_IsRenderbufferEXT(disp, parameters) \
    (* GET_IsRenderbufferEXT(disp)) parameters
static inline _glptr_IsRenderbufferEXT GET_IsRenderbufferEXT(struct _glapi_table *disp) {
   return (_glptr_IsRenderbufferEXT) (GET_by_offset(disp, _gloffset_IsRenderbufferEXT));
}

static inline void SET_IsRenderbufferEXT(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_IsRenderbufferEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_RenderbufferStorageEXT)(GLenum, GLenum, GLsizei, GLsizei);
#define CALL_RenderbufferStorageEXT(disp, parameters) \
    (* GET_RenderbufferStorageEXT(disp)) parameters
static inline _glptr_RenderbufferStorageEXT GET_RenderbufferStorageEXT(struct _glapi_table *disp) {
   return (_glptr_RenderbufferStorageEXT) (GET_by_offset(disp, _gloffset_RenderbufferStorageEXT));
}

static inline void SET_RenderbufferStorageEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLsizei, GLsizei)) {
   SET_by_offset(disp, _gloffset_RenderbufferStorageEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_BlitFramebufferEXT)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);
#define CALL_BlitFramebufferEXT(disp, parameters) \
    (* GET_BlitFramebufferEXT(disp)) parameters
static inline _glptr_BlitFramebufferEXT GET_BlitFramebufferEXT(struct _glapi_table *disp) {
   return (_glptr_BlitFramebufferEXT) (GET_by_offset(disp, _gloffset_BlitFramebufferEXT));
}

static inline void SET_BlitFramebufferEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum)) {
   SET_by_offset(disp, _gloffset_BlitFramebufferEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_BufferParameteriAPPLE)(GLenum, GLenum, GLint);
#define CALL_BufferParameteriAPPLE(disp, parameters) \
    (* GET_BufferParameteriAPPLE(disp)) parameters
static inline _glptr_BufferParameteriAPPLE GET_BufferParameteriAPPLE(struct _glapi_table *disp) {
   return (_glptr_BufferParameteriAPPLE) (GET_by_offset(disp, _gloffset_BufferParameteriAPPLE));
}

static inline void SET_BufferParameteriAPPLE(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_BufferParameteriAPPLE, fn);
}

typedef void (GLAPIENTRYP _glptr_FlushMappedBufferRangeAPPLE)(GLenum, GLintptr, GLsizeiptr);
#define CALL_FlushMappedBufferRangeAPPLE(disp, parameters) \
    (* GET_FlushMappedBufferRangeAPPLE(disp)) parameters
static inline _glptr_FlushMappedBufferRangeAPPLE GET_FlushMappedBufferRangeAPPLE(struct _glapi_table *disp) {
   return (_glptr_FlushMappedBufferRangeAPPLE) (GET_by_offset(disp, _gloffset_FlushMappedBufferRangeAPPLE));
}

static inline void SET_FlushMappedBufferRangeAPPLE(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLintptr, GLsizeiptr)) {
   SET_by_offset(disp, _gloffset_FlushMappedBufferRangeAPPLE, fn);
}

typedef void (GLAPIENTRYP _glptr_BindFragDataLocationEXT)(GLuint, GLuint, const GLchar *);
#define CALL_BindFragDataLocationEXT(disp, parameters) \
    (* GET_BindFragDataLocationEXT(disp)) parameters
static inline _glptr_BindFragDataLocationEXT GET_BindFragDataLocationEXT(struct _glapi_table *disp) {
   return (_glptr_BindFragDataLocationEXT) (GET_by_offset(disp, _gloffset_BindFragDataLocationEXT));
}

static inline void SET_BindFragDataLocationEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint, const GLchar *)) {
   SET_by_offset(disp, _gloffset_BindFragDataLocationEXT, fn);
}

typedef GLint (GLAPIENTRYP _glptr_GetFragDataLocationEXT)(GLuint, const GLchar *);
#define CALL_GetFragDataLocationEXT(disp, parameters) \
    (* GET_GetFragDataLocationEXT(disp)) parameters
static inline _glptr_GetFragDataLocationEXT GET_GetFragDataLocationEXT(struct _glapi_table *disp) {
   return (_glptr_GetFragDataLocationEXT) (GET_by_offset(disp, _gloffset_GetFragDataLocationEXT));
}

static inline void SET_GetFragDataLocationEXT(struct _glapi_table *disp, GLint (GLAPIENTRYP fn)(GLuint, const GLchar *)) {
   SET_by_offset(disp, _gloffset_GetFragDataLocationEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_GetUniformuivEXT)(GLuint, GLint, GLuint *);
#define CALL_GetUniformuivEXT(disp, parameters) \
    (* GET_GetUniformuivEXT(disp)) parameters
static inline _glptr_GetUniformuivEXT GET_GetUniformuivEXT(struct _glapi_table *disp) {
   return (_glptr_GetUniformuivEXT) (GET_by_offset(disp, _gloffset_GetUniformuivEXT));
}

static inline void SET_GetUniformuivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLint, GLuint *)) {
   SET_by_offset(disp, _gloffset_GetUniformuivEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_GetVertexAttribIivEXT)(GLuint, GLenum, GLint *);
#define CALL_GetVertexAttribIivEXT(disp, parameters) \
    (* GET_GetVertexAttribIivEXT(disp)) parameters
static inline _glptr_GetVertexAttribIivEXT GET_GetVertexAttribIivEXT(struct _glapi_table *disp) {
   return (_glptr_GetVertexAttribIivEXT) (GET_by_offset(disp, _gloffset_GetVertexAttribIivEXT));
}

static inline void SET_GetVertexAttribIivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetVertexAttribIivEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_GetVertexAttribIuivEXT)(GLuint, GLenum, GLuint *);
#define CALL_GetVertexAttribIuivEXT(disp, parameters) \
    (* GET_GetVertexAttribIuivEXT(disp)) parameters
static inline _glptr_GetVertexAttribIuivEXT GET_GetVertexAttribIuivEXT(struct _glapi_table *disp) {
   return (_glptr_GetVertexAttribIuivEXT) (GET_by_offset(disp, _gloffset_GetVertexAttribIuivEXT));
}

static inline void SET_GetVertexAttribIuivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLuint *)) {
   SET_by_offset(disp, _gloffset_GetVertexAttribIuivEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_Uniform1uiEXT)(GLint, GLuint);
#define CALL_Uniform1uiEXT(disp, parameters) \
    (* GET_Uniform1uiEXT(disp)) parameters
static inline _glptr_Uniform1uiEXT GET_Uniform1uiEXT(struct _glapi_table *disp) {
   return (_glptr_Uniform1uiEXT) (GET_by_offset(disp, _gloffset_Uniform1uiEXT));
}

static inline void SET_Uniform1uiEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLuint)) {
   SET_by_offset(disp, _gloffset_Uniform1uiEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_Uniform1uivEXT)(GLint, GLsizei, const GLuint *);
#define CALL_Uniform1uivEXT(disp, parameters) \
    (* GET_Uniform1uivEXT(disp)) parameters
static inline _glptr_Uniform1uivEXT GET_Uniform1uivEXT(struct _glapi_table *disp) {
   return (_glptr_Uniform1uivEXT) (GET_by_offset(disp, _gloffset_Uniform1uivEXT));
}

static inline void SET_Uniform1uivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_Uniform1uivEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_Uniform2uiEXT)(GLint, GLuint, GLuint);
#define CALL_Uniform2uiEXT(disp, parameters) \
    (* GET_Uniform2uiEXT(disp)) parameters
static inline _glptr_Uniform2uiEXT GET_Uniform2uiEXT(struct _glapi_table *disp) {
   return (_glptr_Uniform2uiEXT) (GET_by_offset(disp, _gloffset_Uniform2uiEXT));
}

static inline void SET_Uniform2uiEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_Uniform2uiEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_Uniform2uivEXT)(GLint, GLsizei, const GLuint *);
#define CALL_Uniform2uivEXT(disp, parameters) \
    (* GET_Uniform2uivEXT(disp)) parameters
static inline _glptr_Uniform2uivEXT GET_Uniform2uivEXT(struct _glapi_table *disp) {
   return (_glptr_Uniform2uivEXT) (GET_by_offset(disp, _gloffset_Uniform2uivEXT));
}

static inline void SET_Uniform2uivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_Uniform2uivEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_Uniform3uiEXT)(GLint, GLuint, GLuint, GLuint);
#define CALL_Uniform3uiEXT(disp, parameters) \
    (* GET_Uniform3uiEXT(disp)) parameters
static inline _glptr_Uniform3uiEXT GET_Uniform3uiEXT(struct _glapi_table *disp) {
   return (_glptr_Uniform3uiEXT) (GET_by_offset(disp, _gloffset_Uniform3uiEXT));
}

static inline void SET_Uniform3uiEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_Uniform3uiEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_Uniform3uivEXT)(GLint, GLsizei, const GLuint *);
#define CALL_Uniform3uivEXT(disp, parameters) \
    (* GET_Uniform3uivEXT(disp)) parameters
static inline _glptr_Uniform3uivEXT GET_Uniform3uivEXT(struct _glapi_table *disp) {
   return (_glptr_Uniform3uivEXT) (GET_by_offset(disp, _gloffset_Uniform3uivEXT));
}

static inline void SET_Uniform3uivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_Uniform3uivEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_Uniform4uiEXT)(GLint, GLuint, GLuint, GLuint, GLuint);
#define CALL_Uniform4uiEXT(disp, parameters) \
    (* GET_Uniform4uiEXT(disp)) parameters
static inline _glptr_Uniform4uiEXT GET_Uniform4uiEXT(struct _glapi_table *disp) {
   return (_glptr_Uniform4uiEXT) (GET_by_offset(disp, _gloffset_Uniform4uiEXT));
}

static inline void SET_Uniform4uiEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLuint, GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_Uniform4uiEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_Uniform4uivEXT)(GLint, GLsizei, const GLuint *);
#define CALL_Uniform4uivEXT(disp, parameters) \
    (* GET_Uniform4uivEXT(disp)) parameters
static inline _glptr_Uniform4uivEXT GET_Uniform4uivEXT(struct _glapi_table *disp) {
   return (_glptr_Uniform4uivEXT) (GET_by_offset(disp, _gloffset_Uniform4uivEXT));
}

static inline void SET_Uniform4uivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_Uniform4uivEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribI1iEXT)(GLuint, GLint);
#define CALL_VertexAttribI1iEXT(disp, parameters) \
    (* GET_VertexAttribI1iEXT(disp)) parameters
static inline _glptr_VertexAttribI1iEXT GET_VertexAttribI1iEXT(struct _glapi_table *disp) {
   return (_glptr_VertexAttribI1iEXT) (GET_by_offset(disp, _gloffset_VertexAttribI1iEXT));
}

static inline void SET_VertexAttribI1iEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLint)) {
   SET_by_offset(disp, _gloffset_VertexAttribI1iEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribI1ivEXT)(GLuint, const GLint *);
#define CALL_VertexAttribI1ivEXT(disp, parameters) \
    (* GET_VertexAttribI1ivEXT(disp)) parameters
static inline _glptr_VertexAttribI1ivEXT GET_VertexAttribI1ivEXT(struct _glapi_table *disp) {
   return (_glptr_VertexAttribI1ivEXT) (GET_by_offset(disp, _gloffset_VertexAttribI1ivEXT));
}

static inline void SET_VertexAttribI1ivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLint *)) {
   SET_by_offset(disp, _gloffset_VertexAttribI1ivEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribI1uiEXT)(GLuint, GLuint);
#define CALL_VertexAttribI1uiEXT(disp, parameters) \
    (* GET_VertexAttribI1uiEXT(disp)) parameters
static inline _glptr_VertexAttribI1uiEXT GET_VertexAttribI1uiEXT(struct _glapi_table *disp) {
   return (_glptr_VertexAttribI1uiEXT) (GET_by_offset(disp, _gloffset_VertexAttribI1uiEXT));
}

static inline void SET_VertexAttribI1uiEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_VertexAttribI1uiEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribI1uivEXT)(GLuint, const GLuint *);
#define CALL_VertexAttribI1uivEXT(disp, parameters) \
    (* GET_VertexAttribI1uivEXT(disp)) parameters
static inline _glptr_VertexAttribI1uivEXT GET_VertexAttribI1uivEXT(struct _glapi_table *disp) {
   return (_glptr_VertexAttribI1uivEXT) (GET_by_offset(disp, _gloffset_VertexAttribI1uivEXT));
}

static inline void SET_VertexAttribI1uivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLuint *)) {
   SET_by_offset(disp, _gloffset_VertexAttribI1uivEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribI2iEXT)(GLuint, GLint, GLint);
#define CALL_VertexAttribI2iEXT(disp, parameters) \
    (* GET_VertexAttribI2iEXT(disp)) parameters
static inline _glptr_VertexAttribI2iEXT GET_VertexAttribI2iEXT(struct _glapi_table *disp) {
   return (_glptr_VertexAttribI2iEXT) (GET_by_offset(disp, _gloffset_VertexAttribI2iEXT));
}

static inline void SET_VertexAttribI2iEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_VertexAttribI2iEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribI2ivEXT)(GLuint, const GLint *);
#define CALL_VertexAttribI2ivEXT(disp, parameters) \
    (* GET_VertexAttribI2ivEXT(disp)) parameters
static inline _glptr_VertexAttribI2ivEXT GET_VertexAttribI2ivEXT(struct _glapi_table *disp) {
   return (_glptr_VertexAttribI2ivEXT) (GET_by_offset(disp, _gloffset_VertexAttribI2ivEXT));
}

static inline void SET_VertexAttribI2ivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLint *)) {
   SET_by_offset(disp, _gloffset_VertexAttribI2ivEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribI2uiEXT)(GLuint, GLuint, GLuint);
#define CALL_VertexAttribI2uiEXT(disp, parameters) \
    (* GET_VertexAttribI2uiEXT(disp)) parameters
static inline _glptr_VertexAttribI2uiEXT GET_VertexAttribI2uiEXT(struct _glapi_table *disp) {
   return (_glptr_VertexAttribI2uiEXT) (GET_by_offset(disp, _gloffset_VertexAttribI2uiEXT));
}

static inline void SET_VertexAttribI2uiEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_VertexAttribI2uiEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribI2uivEXT)(GLuint, const GLuint *);
#define CALL_VertexAttribI2uivEXT(disp, parameters) \
    (* GET_VertexAttribI2uivEXT(disp)) parameters
static inline _glptr_VertexAttribI2uivEXT GET_VertexAttribI2uivEXT(struct _glapi_table *disp) {
   return (_glptr_VertexAttribI2uivEXT) (GET_by_offset(disp, _gloffset_VertexAttribI2uivEXT));
}

static inline void SET_VertexAttribI2uivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLuint *)) {
   SET_by_offset(disp, _gloffset_VertexAttribI2uivEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribI3iEXT)(GLuint, GLint, GLint, GLint);
#define CALL_VertexAttribI3iEXT(disp, parameters) \
    (* GET_VertexAttribI3iEXT(disp)) parameters
static inline _glptr_VertexAttribI3iEXT GET_VertexAttribI3iEXT(struct _glapi_table *disp) {
   return (_glptr_VertexAttribI3iEXT) (GET_by_offset(disp, _gloffset_VertexAttribI3iEXT));
}

static inline void SET_VertexAttribI3iEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_VertexAttribI3iEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribI3ivEXT)(GLuint, const GLint *);
#define CALL_VertexAttribI3ivEXT(disp, parameters) \
    (* GET_VertexAttribI3ivEXT(disp)) parameters
static inline _glptr_VertexAttribI3ivEXT GET_VertexAttribI3ivEXT(struct _glapi_table *disp) {
   return (_glptr_VertexAttribI3ivEXT) (GET_by_offset(disp, _gloffset_VertexAttribI3ivEXT));
}

static inline void SET_VertexAttribI3ivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLint *)) {
   SET_by_offset(disp, _gloffset_VertexAttribI3ivEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribI3uiEXT)(GLuint, GLuint, GLuint, GLuint);
#define CALL_VertexAttribI3uiEXT(disp, parameters) \
    (* GET_VertexAttribI3uiEXT(disp)) parameters
static inline _glptr_VertexAttribI3uiEXT GET_VertexAttribI3uiEXT(struct _glapi_table *disp) {
   return (_glptr_VertexAttribI3uiEXT) (GET_by_offset(disp, _gloffset_VertexAttribI3uiEXT));
}

static inline void SET_VertexAttribI3uiEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_VertexAttribI3uiEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribI3uivEXT)(GLuint, const GLuint *);
#define CALL_VertexAttribI3uivEXT(disp, parameters) \
    (* GET_VertexAttribI3uivEXT(disp)) parameters
static inline _glptr_VertexAttribI3uivEXT GET_VertexAttribI3uivEXT(struct _glapi_table *disp) {
   return (_glptr_VertexAttribI3uivEXT) (GET_by_offset(disp, _gloffset_VertexAttribI3uivEXT));
}

static inline void SET_VertexAttribI3uivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLuint *)) {
   SET_by_offset(disp, _gloffset_VertexAttribI3uivEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribI4bvEXT)(GLuint, const GLbyte *);
#define CALL_VertexAttribI4bvEXT(disp, parameters) \
    (* GET_VertexAttribI4bvEXT(disp)) parameters
static inline _glptr_VertexAttribI4bvEXT GET_VertexAttribI4bvEXT(struct _glapi_table *disp) {
   return (_glptr_VertexAttribI4bvEXT) (GET_by_offset(disp, _gloffset_VertexAttribI4bvEXT));
}

static inline void SET_VertexAttribI4bvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLbyte *)) {
   SET_by_offset(disp, _gloffset_VertexAttribI4bvEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribI4iEXT)(GLuint, GLint, GLint, GLint, GLint);
#define CALL_VertexAttribI4iEXT(disp, parameters) \
    (* GET_VertexAttribI4iEXT(disp)) parameters
static inline _glptr_VertexAttribI4iEXT GET_VertexAttribI4iEXT(struct _glapi_table *disp) {
   return (_glptr_VertexAttribI4iEXT) (GET_by_offset(disp, _gloffset_VertexAttribI4iEXT));
}

static inline void SET_VertexAttribI4iEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLint, GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_VertexAttribI4iEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribI4ivEXT)(GLuint, const GLint *);
#define CALL_VertexAttribI4ivEXT(disp, parameters) \
    (* GET_VertexAttribI4ivEXT(disp)) parameters
static inline _glptr_VertexAttribI4ivEXT GET_VertexAttribI4ivEXT(struct _glapi_table *disp) {
   return (_glptr_VertexAttribI4ivEXT) (GET_by_offset(disp, _gloffset_VertexAttribI4ivEXT));
}

static inline void SET_VertexAttribI4ivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLint *)) {
   SET_by_offset(disp, _gloffset_VertexAttribI4ivEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribI4svEXT)(GLuint, const GLshort *);
#define CALL_VertexAttribI4svEXT(disp, parameters) \
    (* GET_VertexAttribI4svEXT(disp)) parameters
static inline _glptr_VertexAttribI4svEXT GET_VertexAttribI4svEXT(struct _glapi_table *disp) {
   return (_glptr_VertexAttribI4svEXT) (GET_by_offset(disp, _gloffset_VertexAttribI4svEXT));
}

static inline void SET_VertexAttribI4svEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLshort *)) {
   SET_by_offset(disp, _gloffset_VertexAttribI4svEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribI4ubvEXT)(GLuint, const GLubyte *);
#define CALL_VertexAttribI4ubvEXT(disp, parameters) \
    (* GET_VertexAttribI4ubvEXT(disp)) parameters
static inline _glptr_VertexAttribI4ubvEXT GET_VertexAttribI4ubvEXT(struct _glapi_table *disp) {
   return (_glptr_VertexAttribI4ubvEXT) (GET_by_offset(disp, _gloffset_VertexAttribI4ubvEXT));
}

static inline void SET_VertexAttribI4ubvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLubyte *)) {
   SET_by_offset(disp, _gloffset_VertexAttribI4ubvEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribI4uiEXT)(GLuint, GLuint, GLuint, GLuint, GLuint);
#define CALL_VertexAttribI4uiEXT(disp, parameters) \
    (* GET_VertexAttribI4uiEXT(disp)) parameters
static inline _glptr_VertexAttribI4uiEXT GET_VertexAttribI4uiEXT(struct _glapi_table *disp) {
   return (_glptr_VertexAttribI4uiEXT) (GET_by_offset(disp, _gloffset_VertexAttribI4uiEXT));
}

static inline void SET_VertexAttribI4uiEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint, GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_VertexAttribI4uiEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribI4uivEXT)(GLuint, const GLuint *);
#define CALL_VertexAttribI4uivEXT(disp, parameters) \
    (* GET_VertexAttribI4uivEXT(disp)) parameters
static inline _glptr_VertexAttribI4uivEXT GET_VertexAttribI4uivEXT(struct _glapi_table *disp) {
   return (_glptr_VertexAttribI4uivEXT) (GET_by_offset(disp, _gloffset_VertexAttribI4uivEXT));
}

static inline void SET_VertexAttribI4uivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLuint *)) {
   SET_by_offset(disp, _gloffset_VertexAttribI4uivEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribI4usvEXT)(GLuint, const GLushort *);
#define CALL_VertexAttribI4usvEXT(disp, parameters) \
    (* GET_VertexAttribI4usvEXT(disp)) parameters
static inline _glptr_VertexAttribI4usvEXT GET_VertexAttribI4usvEXT(struct _glapi_table *disp) {
   return (_glptr_VertexAttribI4usvEXT) (GET_by_offset(disp, _gloffset_VertexAttribI4usvEXT));
}

static inline void SET_VertexAttribI4usvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLushort *)) {
   SET_by_offset(disp, _gloffset_VertexAttribI4usvEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_VertexAttribIPointerEXT)(GLuint, GLint, GLenum, GLsizei, const GLvoid *);
#define CALL_VertexAttribIPointerEXT(disp, parameters) \
    (* GET_VertexAttribIPointerEXT(disp)) parameters
static inline _glptr_VertexAttribIPointerEXT GET_VertexAttribIPointerEXT(struct _glapi_table *disp) {
   return (_glptr_VertexAttribIPointerEXT) (GET_by_offset(disp, _gloffset_VertexAttribIPointerEXT));
}

static inline void SET_VertexAttribIPointerEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLint, GLenum, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_VertexAttribIPointerEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_FramebufferTextureLayerEXT)(GLenum, GLenum, GLuint, GLint, GLint);
#define CALL_FramebufferTextureLayerEXT(disp, parameters) \
    (* GET_FramebufferTextureLayerEXT(disp)) parameters
static inline _glptr_FramebufferTextureLayerEXT GET_FramebufferTextureLayerEXT(struct _glapi_table *disp) {
   return (_glptr_FramebufferTextureLayerEXT) (GET_by_offset(disp, _gloffset_FramebufferTextureLayerEXT));
}

static inline void SET_FramebufferTextureLayerEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLuint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_FramebufferTextureLayerEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_ColorMaskIndexedEXT)(GLuint, GLboolean, GLboolean, GLboolean, GLboolean);
#define CALL_ColorMaskIndexedEXT(disp, parameters) \
    (* GET_ColorMaskIndexedEXT(disp)) parameters
static inline _glptr_ColorMaskIndexedEXT GET_ColorMaskIndexedEXT(struct _glapi_table *disp) {
   return (_glptr_ColorMaskIndexedEXT) (GET_by_offset(disp, _gloffset_ColorMaskIndexedEXT));
}

static inline void SET_ColorMaskIndexedEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLboolean, GLboolean, GLboolean, GLboolean)) {
   SET_by_offset(disp, _gloffset_ColorMaskIndexedEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_DisableIndexedEXT)(GLenum, GLuint);
#define CALL_DisableIndexedEXT(disp, parameters) \
    (* GET_DisableIndexedEXT(disp)) parameters
static inline _glptr_DisableIndexedEXT GET_DisableIndexedEXT(struct _glapi_table *disp) {
   return (_glptr_DisableIndexedEXT) (GET_by_offset(disp, _gloffset_DisableIndexedEXT));
}

static inline void SET_DisableIndexedEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_DisableIndexedEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_EnableIndexedEXT)(GLenum, GLuint);
#define CALL_EnableIndexedEXT(disp, parameters) \
    (* GET_EnableIndexedEXT(disp)) parameters
static inline _glptr_EnableIndexedEXT GET_EnableIndexedEXT(struct _glapi_table *disp) {
   return (_glptr_EnableIndexedEXT) (GET_by_offset(disp, _gloffset_EnableIndexedEXT));
}

static inline void SET_EnableIndexedEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_EnableIndexedEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_GetBooleanIndexedvEXT)(GLenum, GLuint, GLboolean *);
#define CALL_GetBooleanIndexedvEXT(disp, parameters) \
    (* GET_GetBooleanIndexedvEXT(disp)) parameters
static inline _glptr_GetBooleanIndexedvEXT GET_GetBooleanIndexedvEXT(struct _glapi_table *disp) {
   return (_glptr_GetBooleanIndexedvEXT) (GET_by_offset(disp, _gloffset_GetBooleanIndexedvEXT));
}

static inline void SET_GetBooleanIndexedvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLboolean *)) {
   SET_by_offset(disp, _gloffset_GetBooleanIndexedvEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_GetIntegerIndexedvEXT)(GLenum, GLuint, GLint *);
#define CALL_GetIntegerIndexedvEXT(disp, parameters) \
    (* GET_GetIntegerIndexedvEXT(disp)) parameters
static inline _glptr_GetIntegerIndexedvEXT GET_GetIntegerIndexedvEXT(struct _glapi_table *disp) {
   return (_glptr_GetIntegerIndexedvEXT) (GET_by_offset(disp, _gloffset_GetIntegerIndexedvEXT));
}

static inline void SET_GetIntegerIndexedvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLint *)) {
   SET_by_offset(disp, _gloffset_GetIntegerIndexedvEXT, fn);
}

typedef GLboolean (GLAPIENTRYP _glptr_IsEnabledIndexedEXT)(GLenum, GLuint);
#define CALL_IsEnabledIndexedEXT(disp, parameters) \
    (* GET_IsEnabledIndexedEXT(disp)) parameters
static inline _glptr_IsEnabledIndexedEXT GET_IsEnabledIndexedEXT(struct _glapi_table *disp) {
   return (_glptr_IsEnabledIndexedEXT) (GET_by_offset(disp, _gloffset_IsEnabledIndexedEXT));
}

static inline void SET_IsEnabledIndexedEXT(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_IsEnabledIndexedEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_ClearColorIiEXT)(GLint, GLint, GLint, GLint);
#define CALL_ClearColorIiEXT(disp, parameters) \
    (* GET_ClearColorIiEXT(disp)) parameters
static inline _glptr_ClearColorIiEXT GET_ClearColorIiEXT(struct _glapi_table *disp) {
   return (_glptr_ClearColorIiEXT) (GET_by_offset(disp, _gloffset_ClearColorIiEXT));
}

static inline void SET_ClearColorIiEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_ClearColorIiEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_ClearColorIuiEXT)(GLuint, GLuint, GLuint, GLuint);
#define CALL_ClearColorIuiEXT(disp, parameters) \
    (* GET_ClearColorIuiEXT(disp)) parameters
static inline _glptr_ClearColorIuiEXT GET_ClearColorIuiEXT(struct _glapi_table *disp) {
   return (_glptr_ClearColorIuiEXT) (GET_by_offset(disp, _gloffset_ClearColorIuiEXT));
}

static inline void SET_ClearColorIuiEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_ClearColorIuiEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_GetTexParameterIivEXT)(GLenum, GLenum, GLint *);
#define CALL_GetTexParameterIivEXT(disp, parameters) \
    (* GET_GetTexParameterIivEXT(disp)) parameters
static inline _glptr_GetTexParameterIivEXT GET_GetTexParameterIivEXT(struct _glapi_table *disp) {
   return (_glptr_GetTexParameterIivEXT) (GET_by_offset(disp, _gloffset_GetTexParameterIivEXT));
}

static inline void SET_GetTexParameterIivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetTexParameterIivEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_GetTexParameterIuivEXT)(GLenum, GLenum, GLuint *);
#define CALL_GetTexParameterIuivEXT(disp, parameters) \
    (* GET_GetTexParameterIuivEXT(disp)) parameters
static inline _glptr_GetTexParameterIuivEXT GET_GetTexParameterIuivEXT(struct _glapi_table *disp) {
   return (_glptr_GetTexParameterIuivEXT) (GET_by_offset(disp, _gloffset_GetTexParameterIuivEXT));
}

static inline void SET_GetTexParameterIuivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLuint *)) {
   SET_by_offset(disp, _gloffset_GetTexParameterIuivEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_TexParameterIivEXT)(GLenum, GLenum, const GLint *);
#define CALL_TexParameterIivEXT(disp, parameters) \
    (* GET_TexParameterIivEXT(disp)) parameters
static inline _glptr_TexParameterIivEXT GET_TexParameterIivEXT(struct _glapi_table *disp) {
   return (_glptr_TexParameterIivEXT) (GET_by_offset(disp, _gloffset_TexParameterIivEXT));
}

static inline void SET_TexParameterIivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_TexParameterIivEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_TexParameterIuivEXT)(GLenum, GLenum, const GLuint *);
#define CALL_TexParameterIuivEXT(disp, parameters) \
    (* GET_TexParameterIuivEXT(disp)) parameters
static inline _glptr_TexParameterIuivEXT GET_TexParameterIuivEXT(struct _glapi_table *disp) {
   return (_glptr_TexParameterIuivEXT) (GET_by_offset(disp, _gloffset_TexParameterIuivEXT));
}

static inline void SET_TexParameterIuivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLuint *)) {
   SET_by_offset(disp, _gloffset_TexParameterIuivEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_BeginConditionalRenderNV)(GLuint, GLenum);
#define CALL_BeginConditionalRenderNV(disp, parameters) \
    (* GET_BeginConditionalRenderNV(disp)) parameters
static inline _glptr_BeginConditionalRenderNV GET_BeginConditionalRenderNV(struct _glapi_table *disp) {
   return (_glptr_BeginConditionalRenderNV) (GET_by_offset(disp, _gloffset_BeginConditionalRenderNV));
}

static inline void SET_BeginConditionalRenderNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum)) {
   SET_by_offset(disp, _gloffset_BeginConditionalRenderNV, fn);
}

typedef void (GLAPIENTRYP _glptr_EndConditionalRenderNV)(void);
#define CALL_EndConditionalRenderNV(disp, parameters) \
    (* GET_EndConditionalRenderNV(disp)) parameters
static inline _glptr_EndConditionalRenderNV GET_EndConditionalRenderNV(struct _glapi_table *disp) {
   return (_glptr_EndConditionalRenderNV) (GET_by_offset(disp, _gloffset_EndConditionalRenderNV));
}

static inline void SET_EndConditionalRenderNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_EndConditionalRenderNV, fn);
}

typedef void (GLAPIENTRYP _glptr_BeginTransformFeedbackEXT)(GLenum);
#define CALL_BeginTransformFeedbackEXT(disp, parameters) \
    (* GET_BeginTransformFeedbackEXT(disp)) parameters
static inline _glptr_BeginTransformFeedbackEXT GET_BeginTransformFeedbackEXT(struct _glapi_table *disp) {
   return (_glptr_BeginTransformFeedbackEXT) (GET_by_offset(disp, _gloffset_BeginTransformFeedbackEXT));
}

static inline void SET_BeginTransformFeedbackEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_BeginTransformFeedbackEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_BindBufferBaseEXT)(GLenum, GLuint, GLuint);
#define CALL_BindBufferBaseEXT(disp, parameters) \
    (* GET_BindBufferBaseEXT(disp)) parameters
static inline _glptr_BindBufferBaseEXT GET_BindBufferBaseEXT(struct _glapi_table *disp) {
   return (_glptr_BindBufferBaseEXT) (GET_by_offset(disp, _gloffset_BindBufferBaseEXT));
}

static inline void SET_BindBufferBaseEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_BindBufferBaseEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_BindBufferOffsetEXT)(GLenum, GLuint, GLuint, GLintptr);
#define CALL_BindBufferOffsetEXT(disp, parameters) \
    (* GET_BindBufferOffsetEXT(disp)) parameters
static inline _glptr_BindBufferOffsetEXT GET_BindBufferOffsetEXT(struct _glapi_table *disp) {
   return (_glptr_BindBufferOffsetEXT) (GET_by_offset(disp, _gloffset_BindBufferOffsetEXT));
}

static inline void SET_BindBufferOffsetEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLuint, GLintptr)) {
   SET_by_offset(disp, _gloffset_BindBufferOffsetEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_BindBufferRangeEXT)(GLenum, GLuint, GLuint, GLintptr, GLsizeiptr);
#define CALL_BindBufferRangeEXT(disp, parameters) \
    (* GET_BindBufferRangeEXT(disp)) parameters
static inline _glptr_BindBufferRangeEXT GET_BindBufferRangeEXT(struct _glapi_table *disp) {
   return (_glptr_BindBufferRangeEXT) (GET_by_offset(disp, _gloffset_BindBufferRangeEXT));
}

static inline void SET_BindBufferRangeEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLuint, GLintptr, GLsizeiptr)) {
   SET_by_offset(disp, _gloffset_BindBufferRangeEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_EndTransformFeedbackEXT)(void);
#define CALL_EndTransformFeedbackEXT(disp, parameters) \
    (* GET_EndTransformFeedbackEXT(disp)) parameters
static inline _glptr_EndTransformFeedbackEXT GET_EndTransformFeedbackEXT(struct _glapi_table *disp) {
   return (_glptr_EndTransformFeedbackEXT) (GET_by_offset(disp, _gloffset_EndTransformFeedbackEXT));
}

static inline void SET_EndTransformFeedbackEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_EndTransformFeedbackEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_GetTransformFeedbackVaryingEXT)(GLuint, GLuint, GLsizei, GLsizei *, GLsizei *, GLenum *, GLchar *);
#define CALL_GetTransformFeedbackVaryingEXT(disp, parameters) \
    (* GET_GetTransformFeedbackVaryingEXT(disp)) parameters
static inline _glptr_GetTransformFeedbackVaryingEXT GET_GetTransformFeedbackVaryingEXT(struct _glapi_table *disp) {
   return (_glptr_GetTransformFeedbackVaryingEXT) (GET_by_offset(disp, _gloffset_GetTransformFeedbackVaryingEXT));
}

static inline void SET_GetTransformFeedbackVaryingEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint, GLsizei, GLsizei *, GLsizei *, GLenum *, GLchar *)) {
   SET_by_offset(disp, _gloffset_GetTransformFeedbackVaryingEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_TransformFeedbackVaryingsEXT)(GLuint, GLsizei, const char **, GLenum);
#define CALL_TransformFeedbackVaryingsEXT(disp, parameters) \
    (* GET_TransformFeedbackVaryingsEXT(disp)) parameters
static inline _glptr_TransformFeedbackVaryingsEXT GET_TransformFeedbackVaryingsEXT(struct _glapi_table *disp) {
   return (_glptr_TransformFeedbackVaryingsEXT) (GET_by_offset(disp, _gloffset_TransformFeedbackVaryingsEXT));
}

static inline void SET_TransformFeedbackVaryingsEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const char **, GLenum)) {
   SET_by_offset(disp, _gloffset_TransformFeedbackVaryingsEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_ProvokingVertexEXT)(GLenum);
#define CALL_ProvokingVertexEXT(disp, parameters) \
    (* GET_ProvokingVertexEXT(disp)) parameters
static inline _glptr_ProvokingVertexEXT GET_ProvokingVertexEXT(struct _glapi_table *disp) {
   return (_glptr_ProvokingVertexEXT) (GET_by_offset(disp, _gloffset_ProvokingVertexEXT));
}

static inline void SET_ProvokingVertexEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_ProvokingVertexEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_GetTexParameterPointervAPPLE)(GLenum, GLenum, GLvoid **);
#define CALL_GetTexParameterPointervAPPLE(disp, parameters) \
    (* GET_GetTexParameterPointervAPPLE(disp)) parameters
static inline _glptr_GetTexParameterPointervAPPLE GET_GetTexParameterPointervAPPLE(struct _glapi_table *disp) {
   return (_glptr_GetTexParameterPointervAPPLE) (GET_by_offset(disp, _gloffset_GetTexParameterPointervAPPLE));
}

static inline void SET_GetTexParameterPointervAPPLE(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLvoid **)) {
   SET_by_offset(disp, _gloffset_GetTexParameterPointervAPPLE, fn);
}

typedef void (GLAPIENTRYP _glptr_TextureRangeAPPLE)(GLenum, GLsizei, GLvoid *);
#define CALL_TextureRangeAPPLE(disp, parameters) \
    (* GET_TextureRangeAPPLE(disp)) parameters
static inline _glptr_TextureRangeAPPLE GET_TextureRangeAPPLE(struct _glapi_table *disp) {
   return (_glptr_TextureRangeAPPLE) (GET_by_offset(disp, _gloffset_TextureRangeAPPLE));
}

static inline void SET_TextureRangeAPPLE(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLvoid *)) {
   SET_by_offset(disp, _gloffset_TextureRangeAPPLE, fn);
}

typedef void (GLAPIENTRYP _glptr_GetObjectParameterivAPPLE)(GLenum, GLuint, GLenum, GLint *);
#define CALL_GetObjectParameterivAPPLE(disp, parameters) \
    (* GET_GetObjectParameterivAPPLE(disp)) parameters
static inline _glptr_GetObjectParameterivAPPLE GET_GetObjectParameterivAPPLE(struct _glapi_table *disp) {
   return (_glptr_GetObjectParameterivAPPLE) (GET_by_offset(disp, _gloffset_GetObjectParameterivAPPLE));
}

static inline void SET_GetObjectParameterivAPPLE(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetObjectParameterivAPPLE, fn);
}

typedef GLenum (GLAPIENTRYP _glptr_ObjectPurgeableAPPLE)(GLenum, GLuint, GLenum);
#define CALL_ObjectPurgeableAPPLE(disp, parameters) \
    (* GET_ObjectPurgeableAPPLE(disp)) parameters
static inline _glptr_ObjectPurgeableAPPLE GET_ObjectPurgeableAPPLE(struct _glapi_table *disp) {
   return (_glptr_ObjectPurgeableAPPLE) (GET_by_offset(disp, _gloffset_ObjectPurgeableAPPLE));
}

static inline void SET_ObjectPurgeableAPPLE(struct _glapi_table *disp, GLenum (GLAPIENTRYP fn)(GLenum, GLuint, GLenum)) {
   SET_by_offset(disp, _gloffset_ObjectPurgeableAPPLE, fn);
}

typedef GLenum (GLAPIENTRYP _glptr_ObjectUnpurgeableAPPLE)(GLenum, GLuint, GLenum);
#define CALL_ObjectUnpurgeableAPPLE(disp, parameters) \
    (* GET_ObjectUnpurgeableAPPLE(disp)) parameters
static inline _glptr_ObjectUnpurgeableAPPLE GET_ObjectUnpurgeableAPPLE(struct _glapi_table *disp) {
   return (_glptr_ObjectUnpurgeableAPPLE) (GET_by_offset(disp, _gloffset_ObjectUnpurgeableAPPLE));
}

static inline void SET_ObjectUnpurgeableAPPLE(struct _glapi_table *disp, GLenum (GLAPIENTRYP fn)(GLenum, GLuint, GLenum)) {
   SET_by_offset(disp, _gloffset_ObjectUnpurgeableAPPLE, fn);
}

typedef void (GLAPIENTRYP _glptr_ActiveProgramEXT)(GLuint);
#define CALL_ActiveProgramEXT(disp, parameters) \
    (* GET_ActiveProgramEXT(disp)) parameters
static inline _glptr_ActiveProgramEXT GET_ActiveProgramEXT(struct _glapi_table *disp) {
   return (_glptr_ActiveProgramEXT) (GET_by_offset(disp, _gloffset_ActiveProgramEXT));
}

static inline void SET_ActiveProgramEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_ActiveProgramEXT, fn);
}

typedef GLuint (GLAPIENTRYP _glptr_CreateShaderProgramEXT)(GLenum, const GLchar *);
#define CALL_CreateShaderProgramEXT(disp, parameters) \
    (* GET_CreateShaderProgramEXT(disp)) parameters
static inline _glptr_CreateShaderProgramEXT GET_CreateShaderProgramEXT(struct _glapi_table *disp) {
   return (_glptr_CreateShaderProgramEXT) (GET_by_offset(disp, _gloffset_CreateShaderProgramEXT));
}

static inline void SET_CreateShaderProgramEXT(struct _glapi_table *disp, GLuint (GLAPIENTRYP fn)(GLenum, const GLchar *)) {
   SET_by_offset(disp, _gloffset_CreateShaderProgramEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_UseShaderProgramEXT)(GLenum, GLuint);
#define CALL_UseShaderProgramEXT(disp, parameters) \
    (* GET_UseShaderProgramEXT(disp)) parameters
static inline _glptr_UseShaderProgramEXT GET_UseShaderProgramEXT(struct _glapi_table *disp) {
   return (_glptr_UseShaderProgramEXT) (GET_by_offset(disp, _gloffset_UseShaderProgramEXT));
}

static inline void SET_UseShaderProgramEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_UseShaderProgramEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_TextureBarrierNV)(void);
#define CALL_TextureBarrierNV(disp, parameters) \
    (* GET_TextureBarrierNV(disp)) parameters
static inline _glptr_TextureBarrierNV GET_TextureBarrierNV(struct _glapi_table *disp) {
   return (_glptr_TextureBarrierNV) (GET_by_offset(disp, _gloffset_TextureBarrierNV));
}

static inline void SET_TextureBarrierNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_TextureBarrierNV, fn);
}

typedef void (GLAPIENTRYP _glptr_StencilFuncSeparateATI)(GLenum, GLenum, GLint, GLuint);
#define CALL_StencilFuncSeparateATI(disp, parameters) \
    (* GET_StencilFuncSeparateATI(disp)) parameters
static inline _glptr_StencilFuncSeparateATI GET_StencilFuncSeparateATI(struct _glapi_table *disp) {
   return (_glptr_StencilFuncSeparateATI) (GET_by_offset(disp, _gloffset_StencilFuncSeparateATI));
}

static inline void SET_StencilFuncSeparateATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint, GLuint)) {
   SET_by_offset(disp, _gloffset_StencilFuncSeparateATI, fn);
}

typedef void (GLAPIENTRYP _glptr_ProgramEnvParameters4fvEXT)(GLenum, GLuint, GLsizei, const GLfloat *);
#define CALL_ProgramEnvParameters4fvEXT(disp, parameters) \
    (* GET_ProgramEnvParameters4fvEXT(disp)) parameters
static inline _glptr_ProgramEnvParameters4fvEXT GET_ProgramEnvParameters4fvEXT(struct _glapi_table *disp) {
   return (_glptr_ProgramEnvParameters4fvEXT) (GET_by_offset(disp, _gloffset_ProgramEnvParameters4fvEXT));
}

static inline void SET_ProgramEnvParameters4fvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLsizei, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_ProgramEnvParameters4fvEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_ProgramLocalParameters4fvEXT)(GLenum, GLuint, GLsizei, const GLfloat *);
#define CALL_ProgramLocalParameters4fvEXT(disp, parameters) \
    (* GET_ProgramLocalParameters4fvEXT(disp)) parameters
static inline _glptr_ProgramLocalParameters4fvEXT GET_ProgramLocalParameters4fvEXT(struct _glapi_table *disp) {
   return (_glptr_ProgramLocalParameters4fvEXT) (GET_by_offset(disp, _gloffset_ProgramLocalParameters4fvEXT));
}

static inline void SET_ProgramLocalParameters4fvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLsizei, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_ProgramLocalParameters4fvEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_GetQueryObjecti64vEXT)(GLuint, GLenum, GLint64EXT *);
#define CALL_GetQueryObjecti64vEXT(disp, parameters) \
    (* GET_GetQueryObjecti64vEXT(disp)) parameters
static inline _glptr_GetQueryObjecti64vEXT GET_GetQueryObjecti64vEXT(struct _glapi_table *disp) {
   return (_glptr_GetQueryObjecti64vEXT) (GET_by_offset(disp, _gloffset_GetQueryObjecti64vEXT));
}

static inline void SET_GetQueryObjecti64vEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLint64EXT *)) {
   SET_by_offset(disp, _gloffset_GetQueryObjecti64vEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_GetQueryObjectui64vEXT)(GLuint, GLenum, GLuint64EXT *);
#define CALL_GetQueryObjectui64vEXT(disp, parameters) \
    (* GET_GetQueryObjectui64vEXT(disp)) parameters
static inline _glptr_GetQueryObjectui64vEXT GET_GetQueryObjectui64vEXT(struct _glapi_table *disp) {
   return (_glptr_GetQueryObjectui64vEXT) (GET_by_offset(disp, _gloffset_GetQueryObjectui64vEXT));
}

static inline void SET_GetQueryObjectui64vEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLuint64EXT *)) {
   SET_by_offset(disp, _gloffset_GetQueryObjectui64vEXT, fn);
}

typedef void (GLAPIENTRYP _glptr_EGLImageTargetRenderbufferStorageOES)(GLenum, GLvoid *);
#define CALL_EGLImageTargetRenderbufferStorageOES(disp, parameters) \
    (* GET_EGLImageTargetRenderbufferStorageOES(disp)) parameters
static inline _glptr_EGLImageTargetRenderbufferStorageOES GET_EGLImageTargetRenderbufferStorageOES(struct _glapi_table *disp) {
   return (_glptr_EGLImageTargetRenderbufferStorageOES) (GET_by_offset(disp, _gloffset_EGLImageTargetRenderbufferStorageOES));
}

static inline void SET_EGLImageTargetRenderbufferStorageOES(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLvoid *)) {
   SET_by_offset(disp, _gloffset_EGLImageTargetRenderbufferStorageOES, fn);
}

typedef void (GLAPIENTRYP _glptr_EGLImageTargetTexture2DOES)(GLenum, GLvoid *);
#define CALL_EGLImageTargetTexture2DOES(disp, parameters) \
    (* GET_EGLImageTargetTexture2DOES(disp)) parameters
static inline _glptr_EGLImageTargetTexture2DOES GET_EGLImageTargetTexture2DOES(struct _glapi_table *disp) {
   return (_glptr_EGLImageTargetTexture2DOES) (GET_by_offset(disp, _gloffset_EGLImageTargetTexture2DOES));
}

static inline void SET_EGLImageTargetTexture2DOES(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLvoid *)) {
   SET_by_offset(disp, _gloffset_EGLImageTargetTexture2DOES, fn);
}


#endif /* !defined( _DISPATCH_H_ ) */
