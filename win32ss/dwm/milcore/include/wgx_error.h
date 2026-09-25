// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*=========================================================================*\



\*=========================================================================*/

#pragma once

/*=========================================================================*\
    MIL Status Codes
\*=========================================================================*/

#define FACILITY_WGX 0x898

#define MAKE_WGXHR( sev, code )\
    MAKE_HRESULT( sev, FACILITY_WGX, (code) )

#define MAKE_WGXHR_ERR( code )\
    MAKE_WGXHR( 1, code )

// Non-error codes
#define WGXHR_CLIPPEDTOEMPTY                MAKE_WGXHR(0, 1)
#define WGXHR_EMPTYFILL                     MAKE_WGXHR(0, 2)
#define WGXHR_INTERNALTEMPORARYSUCCESS      MAKE_WGXHR(0, 3)
#define WGXHR_RESETSHAREDHANDLEMANAGER      MAKE_WGXHR(0, 4)

// Generic error codes
#define WGXERR_GENERIC_ERROR                E_FAIL
#define WGXERR_INVALIDPARAMETER             E_INVALIDARG
#define WGXERR_OUTOFMEMORY                  E_OUTOFMEMORY
#define WGXERR_NOTIMPLEMENTED               E_NOTIMPL
#define WGXERR_ABORTED                      E_ABORT
#define WGXERR_ACCESSDENIED                 E_ACCESSDENIED
#define WGXERR_VALUEOVERFLOW                INTSAFE_E_ARITHMETIC_OVERFLOW

// Error codes shared with wincodecs
#define WGXERR_WRONGSTATE                   WINCODEC_ERR_WRONGSTATE
#define WGXERR_UNSUPPORTEDVERSION           WINCODEC_ERR_UNSUPPORTEDVERSION
#define WGXERR_NOTINITIALIZED               WINCODEC_ERR_NOTINITIALIZED
#define WGXERR_UNSUPPORTEDPIXELFORMAT       WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT
#define WGXERR_UNSUPPORTED_OPERATION        WINCODEC_ERR_UNSUPPORTEDOPERATION
#define WGXERR_PALETTEUNAVAILABLE           WINCODEC_ERR_PALETTEUNAVAILABLE

// Unique MIL error codes                   // Value: 0x8898xxxx -200330nnnn
#define WGXERR_OBJECTBUSY                   MAKE_WGXHR_ERR(0x001)   //  4447
#define WGXERR_INSUFFICIENTBUFFER           MAKE_WGXHR_ERR(0x002)   //  4446
#define WGXERR_WIN32ERROR                   MAKE_WGXHR_ERR(0x003)   //  4445
#define WGXERR_SCANNER_FAILED               MAKE_WGXHR_ERR(0x004)   //  4444
#define WGXERR_SCREENACCESSDENIED           MAKE_WGXHR_ERR(0x005)   //  4443
#define WGXERR_DISPLAYSTATEINVALID          MAKE_WGXHR_ERR(0x006)   //  4442
#define WGXERR_NONINVERTIBLEMATRIX          MAKE_WGXHR_ERR(0x007)   //  4441
#define WGXERR_ZEROVECTOR                   MAKE_WGXHR_ERR(0x008)   //  4440
#define WGXERR_TERMINATED                   MAKE_WGXHR_ERR(0x009)   //  4439
#define WGXERR_BADNUMBER                    MAKE_WGXHR_ERR(0x00A)   //  4438
#define WGXERR_UNSUPPORTEDTEXTURESIZE       MAKE_WGXHR_ERR(0x00B)   //  4437


// An internal error (MIL bug) occurred. On checked builds, we would assert.
#define WGXERR_INTERNALERROR                MAKE_WGXHR_ERR(0x080)   //  4320

// This is a presentation error that is recoverable.  The caller needs
// to reattempt present.
// Known cause for this is another process calling PrintWindow on our hwnd
// when we call UpdateLayeredWindow.
#define WGXERR_NEED_REATTEMPT_PRESENT       MAKE_WGXHR_ERR(0x083)   //  4317

// The display format we need to render is not supported by the
// hardware device.
#define WGXERR_DISPLAYFORMATNOTSUPPORTED    MAKE_WGXHR_ERR(0x084)   //  4316

// A call to this method is invalid.
#define WGXERR_INVALIDCALL                  MAKE_WGXHR_ERR(0x085)   //  4315

// Lock attempted on an already locked object.
#define WGXERR_ALREADYLOCKED                MAKE_WGXHR_ERR(0x086)   //  4314

// Unlock attempted on an unlocked object.
#define WGXERR_NOTLOCKED                    MAKE_WGXHR_ERR(0x087)   //  4313

// No algorithm avaliable to render text with this device
#define WGXERR_DEVICECANNOTRENDERTEXT       MAKE_WGXHR_ERR(0x088)   //  4312

// Some glyph bitmaps, required for glyph run rendering, are not
// contained in glyph cache.
#define WGXERR_GLYPHBITMAPMISSED            MAKE_WGXHR_ERR(0x089)   //  4311

// Some glyph bitmaps in glyph cache are unexpectedly big.
#define WGXERR_MALFORMEDGLYPHCACHE          MAKE_WGXHR_ERR(0x08A)   //  4310

// Marker error for known Win32 errors that are currently being ignored
// by the compositor. This is to avoid returning S_OK when an error has occurred,
// but still unwind the stack in the correct location
#define WGXERR_GENERIC_IGNORE               MAKE_WGXHR_ERR(0x08B)   //  4309

// Guideline coordinates are not sorted properly or contain NaNs.
#define WGXERR_MALFORMED_GUIDELINE_DATA     MAKE_WGXHR_ERR(0x08C)   //  4308

// No HW rendering device is available for this operation
#define WGXERR_NO_HARDWARE_DEVICE           MAKE_WGXHR_ERR(0x08D)   //  4307

// There has been a presentation error that may be recoverable. The caller
// needs to recreate, rerender the entire frame, and reattempt present.
// There are two known case for this:
//  1) D3D Driver Internal error - should be investigated by DXG/IHV
//  2) D3D E_FAIL
//      a) Unknown root cause - should be investigated by DXG
//      b) When resizing too quickly for DWM and D3D stay in sync
#define WGXERR_NEED_RECREATE_AND_PRESENT    MAKE_WGXHR_ERR(0x08E)   //  4306

// The object has already been initialized
#define WGXERR_ALREADY_INITIALIZED          MAKE_WGXHR_ERR(0x08F)   //  4305

// The size of the object does not match the expected size
#define WGXERR_MISMATCHED_SIZE              MAKE_WGXHR_ERR(0x090)   //  4304

// No Redirection surface avaiable
#define WGXERR_NO_REDIRECTION_SURFACE_AVAILABLE MAKE_WGXHR_ERR(0x091) //4303

// Remoting of this content is not supported
#define WGXERR_REMOTING_NOT_SUPPORTED       MAKE_WGXHR_ERR(0x092)   //  4302

// Queued Presents are not being used
#define WGXERR_QUEUED_PRESENT_NOT_SUPPORTED MAKE_WGXHR_ERR(0x093)   //  4301

// Queued Presents are not being used
#define WGXERR_NOT_QUEUING_PRESENTS         MAKE_WGXHR_ERR(0x094)   //  4300

// No redirection surface was available retry the call
#define WGXERR_NO_REDIRECTION_SURFACE_RETRY_LATER    MAKE_WGXHR_ERR(0x095)  // 4299

// Shader construction failed because it was too complex
#define WGXERR_TOOMANYSHADERELEMNTS         MAKE_WGXHR_ERR(0x096)   //  4298

// AVAILABLE         MAKE_WGXHR_ERR(0x097)   //  4297
// AVAILABLE         MAKE_WGXHR_ERR(0x098)   //  4296

// Shader compilation failed
#define WGXERR_SHADER_COMPILE_FAILED        MAKE_WGXHR_ERR(0x099)   //  4295

// Requested DX redirection surface size exceeded maximum texture size
#define WGXERR_MAX_TEXTURE_SIZE_EXCEEDED    MAKE_WGXHR_ERR(0x09A)   //  4294

// AVAILABLE                                MAKE_WGXHR_ERR(0x09B)   //  4293

// Caps don't meet min WPF requirement for hw rendering
#define WGXERR_INSUFFICIENT_GPU_CAPS        MAKE_WGXHR_ERR(0x09C) // 4292

// Composition engine errors

#define WGXERR_UCE_INVALIDPACKETHEADER          MAKE_WGXHR_ERR(0x400)   //  3424
#define WGXERR_UCE_UNKNOWNPACKET                MAKE_WGXHR_ERR(0x401)   //  3423
#define WGXERR_UCE_ILLEGALPACKET                MAKE_WGXHR_ERR(0x402)   //  3422
#define WGXERR_UCE_MALFORMEDPACKET              MAKE_WGXHR_ERR(0x403)   //  3421
#define WGXERR_UCE_ILLEGALHANDLE                MAKE_WGXHR_ERR(0x404)   //  3420
#define WGXERR_UCE_HANDLELOOKUPFAILED           MAKE_WGXHR_ERR(0x405)   //  3419
#define WGXERR_UCE_RENDERTHREADFAILURE          MAKE_WGXHR_ERR(0x406)   //  3418
#define WGXERR_UCE_CTXSTACKFRSTTARGETNULL       MAKE_WGXHR_ERR(0x407)   //  3417
#define WGXERR_UCE_CONNECTIONIDLOOKUPFAILED     MAKE_WGXHR_ERR(0x408)   //  3416
#define WGXERR_UCE_BLOCKSFULL                   MAKE_WGXHR_ERR(0x409)   //  3415
#define WGXERR_UCE_MEMORYFAILURE                MAKE_WGXHR_ERR(0x40A)   //  3414
#define WGXERR_UCE_PACKETRECORDOUTOFRANGE       MAKE_WGXHR_ERR(0x40B)   //  3413
#define WGXERR_UCE_ILLEGALRECORDTYPE            MAKE_WGXHR_ERR(0x40C)   //  3412
#define WGXERR_UCE_OUTOFHANDLES                 MAKE_WGXHR_ERR(0x40D)   //  3411
#define WGXERR_UCE_UNCHANGABLE_UPDATE_ATTEMPTED MAKE_WGXHR_ERR(0x40E)   //  3410
#define WGXERR_UCE_NO_MULTIPLE_WORKER_THREADS   MAKE_WGXHR_ERR(0x40F)   //  3409
#define WGXERR_UCE_REMOTINGNOTSUPPORTED         MAKE_WGXHR_ERR(0x410)   //  3408
#define WGXERR_UCE_MISSINGENDCOMMAND            MAKE_WGXHR_ERR(0x411)   //  3407
#define WGXERR_UCE_MISSINGBEGINCOMMAND          MAKE_WGXHR_ERR(0x412)   //  3406
#define WGXERR_UCE_CHANNELSYNCTIMEDOUT          MAKE_WGXHR_ERR(0x413)   //  3405
#define WGXERR_UCE_CHANNELSYNCABANDONED         MAKE_WGXHR_ERR(0x414)   //  3404
#define WGXERR_UCE_UNSUPPORTEDTRANSPORTVERSION  MAKE_WGXHR_ERR(0x415)   //  3403
#define WGXERR_UCE_TRANSPORTUNAVAILABLE         MAKE_WGXHR_ERR(0x416)   //  3402
#define WGXERR_UCE_FEEDBACK_UNSUPPORTED         MAKE_WGXHR_ERR(0x417)   //  3401
#define WGXERR_UCE_COMMANDTRANSPORTDENIED       MAKE_WGXHR_ERR(0x418)   //  3400
#define WGXERR_UCE_GRAPHICSSTREAMUNAVAILABLE    MAKE_WGXHR_ERR(0x419)   //  3399
#define WGXERR_UCE_GRAPHICSSTREAMALREADYOPEN    MAKE_WGXHR_ERR(0x420)   //  3398
#define WGXERR_UCE_TRANSPORTDISCONNECTED        MAKE_WGXHR_ERR(0x421)   //  3397
#define WGXERR_UCE_TRANSPORTOVERLOADED          MAKE_WGXHR_ERR(0x422)   //  3396
#define WGXERR_UCE_PARTITION_ZOMBIED            MAKE_WGXHR_ERR(0x423)   //  3395



// MIL AV Specific errors

#define WGXERR_AV_NOCLOCK                        MAKE_WGXHR_ERR(0x500)
#define WGXERR_AV_NOMEDIATYPE                    MAKE_WGXHR_ERR(0x501)
#define WGXERR_AV_NOVIDEOMIXER                   MAKE_WGXHR_ERR(0x502)
#define WGXERR_AV_NOVIDEOPRESENTER               MAKE_WGXHR_ERR(0x503)
#define WGXERR_AV_NOREADYFRAMES                  MAKE_WGXHR_ERR(0x504)
#define WGXERR_AV_MODULENOTLOADED                MAKE_WGXHR_ERR(0x505)
#define WGXERR_AV_WMPFACTORYNOTREGISTERED        MAKE_WGXHR_ERR(0x506)
#define WGXERR_AV_INVALIDWMPVERSION              MAKE_WGXHR_ERR(0x507)
#define WGXERR_AV_INSUFFICIENTVIDEORESOURCES     MAKE_WGXHR_ERR(0x508)
#define WGXERR_AV_VIDEOACCELERATIONNOTAVAILABLE  MAKE_WGXHR_ERR(0x509)
#define WGXERR_AV_REQUESTEDTEXTURETOOBIG         MAKE_WGXHR_ERR(0x50A)
#define WGXERR_AV_SEEKFAILED                     MAKE_WGXHR_ERR(0x50B)
#define WGXERR_AV_UNEXPECTEDWMPFAILURE           MAKE_WGXHR_ERR(0x50C)
#define WGXERR_AV_MEDIAPLAYERCLOSED              MAKE_WGXHR_ERR(0x50D)
#define WGXERR_AV_UNKNOWNHARDWAREERROR           MAKE_WGXHR_ERR(0x50E)

// Unused 0x60E - 0x61b

// D3DImage specific errors
#define WGXERR_D3DI_INVALIDSURFACEUSAGE                 MAKE_WGXHR_ERR(0x800)
#define WGXERR_D3DI_INVALIDSURFACESIZE                  MAKE_WGXHR_ERR(0x801)
#define WGXERR_D3DI_INVALIDSURFACEPOOL                  MAKE_WGXHR_ERR(0x802)
#define WGXERR_D3DI_INVALIDSURFACEDEVICE                MAKE_WGXHR_ERR(0x803)
#define WGXERR_D3DI_INVALIDANTIALIASINGSETTINGS         MAKE_WGXHR_ERR(0x804)





