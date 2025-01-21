
#ifndef __DDRAW_INCLUDED__
#define __DDRAW_INCLUDED__

#if defined(_WIN32) && !defined(_NO_COM )
#define COM_NO_WINDOWS_H
#include <objbase.h>
#else
#define IUnknown void
#if !defined(NT_BUILD_ENVIRONMENT) && !defined(WINNT)
  #ifndef CO_E_NOTINITIALIZED /* Avoid conflict warning with _HRESULT_TYPEDEF_(0x800401F0L) in winerror.h */
  #define CO_E_NOTINITIALIZED 0x800401F0L
  #endif
#endif
#endif

#define _FACDD        0x876

#ifndef MAKE_DDHRESULT
#define MAKE_DDHRESULT(c) MAKE_HRESULT(1,_FACDD,c)
#endif

#ifndef DIRECTDRAW_VERSION
  #define DIRECTDRAW_VERSION 0x0700
#endif

#undef ENABLE_NAMELESS_UNION_PRAGMA
#ifdef DIRECTX_REDIST
  #define ENABLE_NAMELESS_UNION_PRAGMA
#endif

#ifdef ENABLE_NAMELESS_UNION_PRAGMA
  #pragma warning(disable:4201)
#endif

#ifndef DUMMYUNIONNAMEN
  #if defined(__cplusplus) || !defined(NONAMELESSUNION)
    #define DUMMYUNIONNAMEN(n)
  #else
    #define DUMMYUNIONNAMEN(n)      u##n
  #endif
#endif

#if defined(WINNT) || !defined(WIN32)
#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
  typedef long HRESULT;
#endif
#endif

/* Helper macro to enable gcc's extension. */
#ifndef __GNU_EXTENSION
#ifdef __GNUC__
#define __GNU_EXTENSION __extension__
#else
#define __GNU_EXTENSION
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* This define are obsolete in ms ddk, but are use internal in Windows NT4/2000/XP/2003/ReactOS */
#define DDCAPS_STEREOVIEW                      0x00040000


/* Orgnal */
#define DDERR_ALREADYINITIALIZED               MAKE_DDHRESULT( 5)
#define DDERR_CANNOTATTACHSURFACE              MAKE_DDHRESULT(10)
#define DDERR_CANNOTDETACHSURFACE              MAKE_DDHRESULT(20)
#define DDERR_CURRENTLYNOTAVAIL                MAKE_DDHRESULT(40)
#define DDERR_EXCEPTION                        MAKE_DDHRESULT(55)
#define DDERR_GENERIC                          E_FAIL
#define DDERR_HEIGHTALIGN                      MAKE_DDHRESULT( 90)
#define DDERR_INCOMPATIBLEPRIMARY              MAKE_DDHRESULT( 95)
#define DDERR_INVALIDCAPS                      MAKE_DDHRESULT(100)
#define DDERR_INVALIDCLIPLIST                  MAKE_DDHRESULT(110)
#define DDERR_INVALIDMODE                      MAKE_DDHRESULT(120)
#define DDERR_INVALIDOBJECT                    MAKE_DDHRESULT(130)
#define DDERR_INVALIDPARAMS                    E_INVALIDARG
#define DDERR_INVALIDPIXELFORMAT               MAKE_DDHRESULT(145)
#define DDERR_INVALIDRECT                      MAKE_DDHRESULT(150)
#define DDERR_LOCKEDSURFACES                   MAKE_DDHRESULT(160)
#define DDERR_NO3D                             MAKE_DDHRESULT(170)
#define DDERR_NOALPHAHW                        MAKE_DDHRESULT(180)
#define DDERR_NOSTEREOHARDWARE                 MAKE_DDHRESULT(181)
#define DDERR_NOSURFACELEFT                    MAKE_DDHRESULT(182)
#define DDERR_NOCLIPLIST                       MAKE_DDHRESULT(205)
#define DDERR_NOCOLORCONVHW                    MAKE_DDHRESULT(210)
#define DDERR_NOCOOPERATIVELEVELSET            MAKE_DDHRESULT(212)
#define DDERR_NOCOLORKEY                       MAKE_DDHRESULT(215)
#define DDERR_NOCOLORKEYHW                     MAKE_DDHRESULT(220)
#define DDERR_NODIRECTDRAWSUPPORT              MAKE_DDHRESULT(222)
#define DDERR_NOEXCLUSIVEMODE                  MAKE_DDHRESULT(225)
#define DDERR_NOFLIPHW                         MAKE_DDHRESULT(230)
#define DDERR_NOGDI                            MAKE_DDHRESULT(240)
#define DDERR_NOMIRRORHW                       MAKE_DDHRESULT(250)
#define DDERR_NOTFOUND                         MAKE_DDHRESULT(255)
#define DDERR_NOOVERLAYHW                      MAKE_DDHRESULT(260)
#define DDERR_OVERLAPPINGRECTS                 MAKE_DDHRESULT(270)
#define DDERR_NORASTEROPHW                     MAKE_DDHRESULT(280)
#define DDERR_NOROTATIONHW                     MAKE_DDHRESULT(290)
#define DDERR_NOSTRETCHHW                      MAKE_DDHRESULT(310)
#define DDERR_NOT4BITCOLOR                     MAKE_DDHRESULT(316)
#define DDERR_NOT4BITCOLORINDEX                MAKE_DDHRESULT(317)
#define DDERR_NOT8BITCOLOR                     MAKE_DDHRESULT(320)
#define DDERR_NOTEXTUREHW                      MAKE_DDHRESULT(330)
#define DDERR_NOVSYNCHW                        MAKE_DDHRESULT(335)
#define DDERR_NOZBUFFERHW                      MAKE_DDHRESULT(340)
#define DDERR_NOZOVERLAYHW                     MAKE_DDHRESULT(350)
#define DDERR_OUTOFCAPS                        MAKE_DDHRESULT(360)
#define DDERR_OUTOFMEMORY                      E_OUTOFMEMORY
#define DDERR_OUTOFVIDEOMEMORY                 MAKE_DDHRESULT(380)
#define DDERR_OVERLAYCANTCLIP                  MAKE_DDHRESULT(382)
#define DDERR_OVERLAYCOLORKEYONLYONEACTIVE     MAKE_DDHRESULT(384)
#define DDERR_PALETTEBUSY                      MAKE_DDHRESULT(387)
#define DDERR_COLORKEYNOTSET                   MAKE_DDHRESULT(400)
#define DDERR_SURFACEALREADYATTACHED           MAKE_DDHRESULT(410)
#define DDERR_SURFACEALREADYDEPENDENT          MAKE_DDHRESULT(420)
#define DDERR_SURFACEBUSY                      MAKE_DDHRESULT(430)
#define DDERR_CANTLOCKSURFACE                  MAKE_DDHRESULT(435)
#define DDERR_SURFACEISOBSCURED                MAKE_DDHRESULT(440)
#define DDERR_SURFACELOST                      MAKE_DDHRESULT(450)
#define DDERR_SURFACENOTATTACHED               MAKE_DDHRESULT(460)
#define DDERR_TOOBIGHEIGHT                     MAKE_DDHRESULT(470)
#define DDERR_TOOBIGSIZE                       MAKE_DDHRESULT(480)
#define DDERR_TOOBIGWIDTH                      MAKE_DDHRESULT(490)
#define DDERR_UNSUPPORTED                      E_NOTIMPL
#define DDERR_UNSUPPORTEDFORMAT                MAKE_DDHRESULT(510)
#define DDERR_UNSUPPORTEDMASK                  MAKE_DDHRESULT(520)
#define DDERR_INVALIDSTREAM                    MAKE_DDHRESULT(521)
#define DDERR_VERTICALBLANKINPROGRESS          MAKE_DDHRESULT(537)
#define DDERR_WASSTILLDRAWING                  MAKE_DDHRESULT(540)
#define DDERR_DDSCAPSCOMPLEXREQUIRED           MAKE_DDHRESULT(542)
#define DDERR_XALIGN                           MAKE_DDHRESULT(560)
#define DDERR_INVALIDDIRECTDRAWGUID            MAKE_DDHRESULT(561)
#define DDERR_DIRECTDRAWALREADYCREATED         MAKE_DDHRESULT(562)
#define DDERR_NODIRECTDRAWHW                   MAKE_DDHRESULT(563)
#define DDERR_PRIMARYSURFACEALREADYEXISTS      MAKE_DDHRESULT(564)
#define DDERR_NOEMULATION                      MAKE_DDHRESULT(565)
#define DDERR_REGIONTOOSMALL                   MAKE_DDHRESULT(566)
#define DDERR_CLIPPERISUSINGHWND               MAKE_DDHRESULT(567)
#define DDERR_NOCLIPPERATTACHED                MAKE_DDHRESULT(568)
#define DDERR_NOHWND                           MAKE_DDHRESULT(569)
#define DDERR_HWNDSUBCLASSED                   MAKE_DDHRESULT(570)
#define DDERR_HWNDALREADYSET                   MAKE_DDHRESULT(571)
#define DDERR_NOPALETTEATTACHED                MAKE_DDHRESULT(572)
#define DDERR_NOPALETTEHW                      MAKE_DDHRESULT(573)
#define DDERR_BLTFASTCANTCLIP                  MAKE_DDHRESULT(574)
#define DDERR_NOBLTHW                          MAKE_DDHRESULT(575)
#define DDERR_NODDROPSHW                       MAKE_DDHRESULT(576)
#define DDERR_OVERLAYNOTVISIBLE                MAKE_DDHRESULT(577)
#define DDERR_NOOVERLAYDEST                    MAKE_DDHRESULT(578)
#define DDERR_INVALIDPOSITION                  MAKE_DDHRESULT(579)
#define DDERR_NOTAOVERLAYSURFACE               MAKE_DDHRESULT(580)
#define DDERR_EXCLUSIVEMODEALREADYSET          MAKE_DDHRESULT(581)
#define DDERR_NOTFLIPPABLE                     MAKE_DDHRESULT(582)
#define DDERR_CANTDUPLICATE                    MAKE_DDHRESULT(583)
#define DDERR_NOTLOCKED                        MAKE_DDHRESULT(584)
#define DDERR_CANTCREATEDC                     MAKE_DDHRESULT(585)
#define DDERR_NODC                             MAKE_DDHRESULT(586)
#define DDERR_WRONGMODE                        MAKE_DDHRESULT(587)
#define DDERR_IMPLICITLYCREATED                MAKE_DDHRESULT(588)
#define DDERR_NOTPALETTIZED                    MAKE_DDHRESULT(589)
#define DDERR_UNSUPPORTEDMODE                  MAKE_DDHRESULT(590)
#define DDERR_NOMIPMAPHW                       MAKE_DDHRESULT(591)
#define DDERR_INVALIDSURFACETYPE               MAKE_DDHRESULT(592)
#define DDERR_NOOPTIMIZEHW                     MAKE_DDHRESULT(600)
#define DDERR_NOTLOADED                        MAKE_DDHRESULT(601)
#define DDERR_NOFOCUSWINDOW                    MAKE_DDHRESULT(602)
#define DDERR_NOTONMIPMAPSUBLEVEL              MAKE_DDHRESULT(603)
#define DDERR_DCALREADYCREATED                 MAKE_DDHRESULT(620)
#define DDERR_NONONLOCALVIDMEM                 MAKE_DDHRESULT(630)
#define DDERR_CANTPAGELOCK                     MAKE_DDHRESULT(640)
#define DDERR_CANTPAGEUNLOCK                   MAKE_DDHRESULT(660)
#define DDERR_NOTPAGELOCKED                    MAKE_DDHRESULT(680)
#define DDERR_MOREDATA                         MAKE_DDHRESULT(690)
#define DDERR_EXPIRED                          MAKE_DDHRESULT(691)
#define DDERR_TESTFINISHED                     MAKE_DDHRESULT(692)
#define DDERR_NEWMODE                          MAKE_DDHRESULT(693)
#define DDERR_D3DNOTINITIALIZED                MAKE_DDHRESULT(694)
#define DDERR_VIDEONOTACTIVE                   MAKE_DDHRESULT(695)
#define DDERR_NOMONITORINFORMATION             MAKE_DDHRESULT(696)
#define DDERR_NODRIVERSUPPORT                  MAKE_DDHRESULT(697)
#define DDERR_DEVICEDOESNTOWNSURFACE           MAKE_DDHRESULT(699)
#define DDERR_NOTINITIALIZED                   CO_E_NOTINITIALIZED
#define DD_OK                                  S_OK
#define DD_FALSE                               S_FALSE
#define DDENUMRET_CANCEL                       0
#define DDENUMRET_OK                           1

#define DDENUM_ATTACHEDSECONDARYDEVICES        0x00000001
#define DDENUM_DETACHEDSECONDARYDEVICES        0x00000002
#define DDENUM_NONDISPLAYDEVICES               0x00000004

#define REGSTR_KEY_DDHW_DESCRIPTION            "Description"
#define REGSTR_KEY_DDHW_DRIVERNAME             "DriverName"
#define REGSTR_PATH_DDHW                       "Hardware\\DirectDrawDrivers"
#define DDCREATE_HARDWAREONLY                  0x00000001
#define DDCREATE_EMULATIONONLY                 0x00000002
#define DD_ROP_SPACE                           (256/32)
#define MAX_DDDEVICEID_STRING                  512
#define DDGDI_GETHOSTIDENTIFIER                0x00000001
#define DDSGR_CALIBRATE                        0x00000001
#define DDSMT_ISTESTREQUIRED                   0x00000001
#define DDEM_MODEPASSED                        0x00000001
#define DDEM_MODEFAILED                        0x00000002

#define DDSD_CAPS                              0x00000001
#define DDSD_HEIGHT                            0x00000002
#define DDSD_WIDTH                             0x00000004
#define DDSD_PITCH                             0x00000008
#define DDSD_BACKBUFFERCOUNT                   0x00000020
#define DDSD_ZBUFFERBITDEPTH                   0x00000040
#define DDSD_ALPHABITDEPTH                     0x00000080
#define DDSD_LPSURFACE                         0x00000800
#define DDSD_PIXELFORMAT                       0x00001000
#define DDSD_CKDESTOVERLAY                     0x00002000
#define DDSD_CKDESTBLT                         0x00004000
#define DDSD_CKSRCOVERLAY                      0x00008000
#define DDSD_CKSRCBLT                          0x00010000
#define DDSD_MIPMAPCOUNT                       0x00020000
#define DDSD_REFRESHRATE                       0x00040000
#define DDSD_LINEARSIZE                        0x00080000
#define DDSD_TEXTURESTAGE                      0x00100000
#define DDSD_FVF                               0x00200000
#define DDSD_SRCVBHANDLE                       0x00400000
#define DDSD_DEPTH                             0x00800000
#define DDSD_ALL                               0x00FFF9EE

#define DDOSD_GUID                             0x00000001
#define DDOSD_COMPRESSION_RATIO                0x00000002
#define DDOSD_SCAPS                            0x00000004
#define DDOSD_OSCAPS                           0x00000008
#define DDOSD_ALL                              0x0000000f
#define DDOSDCAPS_OPTCOMPRESSED                0x00000001
#define DDOSDCAPS_OPTREORDERED                 0x00000002
#define DDOSDCAPS_MONOLITHICMIPMAP             0x00000004
#define DDOSDCAPS_VALIDSCAPS                   0x30004800
#define DDOSDCAPS_VALIDOSCAPS                  0x00000007

#define DDCOLOR_BRIGHTNESS                     0x00000001
#define DDCOLOR_CONTRAST                       0x00000002
#define DDCOLOR_HUE                            0x00000004
#define DDCOLOR_SATURATION                     0x00000008
#define DDCOLOR_SHARPNESS                      0x00000010
#define DDCOLOR_GAMMA                          0x00000020
#define DDCOLOR_COLORENABLE                    0x00000040

#define DDSCAPS_RESERVED1                      0x00000001
#define DDSCAPS_ALPHA                          0x00000002
#define DDSCAPS_BACKBUFFER                     0x00000004
#define DDSCAPS_COMPLEX                        0x00000008
#define DDSCAPS_FLIP                           0x00000010
#define DDSCAPS_FRONTBUFFER                    0x00000020
#define DDSCAPS_OFFSCREENPLAIN                 0x00000040
#define DDSCAPS_OVERLAY                        0x00000080
#define DDSCAPS_PALETTE                        0x00000100
#define DDSCAPS_PRIMARYSURFACE                 0x00000200
#define DDSCAPS_RESERVED3                      0x00000400
#define DDSCAPS_PRIMARYSURFACELEFT             0x00000000
#define DDSCAPS_SYSTEMMEMORY                   0x00000800
#define DDSCAPS_TEXTURE                        0x00001000
#define DDSCAPS_3DDEVICE                       0x00002000
#define DDSCAPS_VIDEOMEMORY                    0x00004000
#define DDSCAPS_VISIBLE                        0x00008000
#define DDSCAPS_WRITEONLY                      0x00010000
#define DDSCAPS_ZBUFFER                        0x00020000
#define DDSCAPS_OWNDC                          0x00040000
#define DDSCAPS_LIVEVIDEO                      0x00080000
#define DDSCAPS_HWCODEC                        0x00100000
#define DDSCAPS_MODEX                          0x00200000
#define DDSCAPS_MIPMAP                         0x00400000
#define DDSCAPS_RESERVED2                      0x00800000
#define DDSCAPS_ALLOCONLOAD                    0x04000000
#define DDSCAPS_VIDEOPORT                      0x08000000
#define DDSCAPS_LOCALVIDMEM                    0x10000000
#define DDSCAPS_NONLOCALVIDMEM                 0x20000000
#define DDSCAPS_STANDARDVGAMODE                0x40000000
#define DDSCAPS_OPTIMIZED                      0x80000000

#define DDSCAPS2_HARDWAREDEINTERLACE           0x00000000
#define DDSCAPS2_RESERVED4                     0x00000002
#define DDSCAPS2_HINTDYNAMIC                   0x00000004
#define DDSCAPS2_HINTSTATIC                    0x00000008
#define DDSCAPS2_TEXTUREMANAGE                 0x00000010
#define DDSCAPS2_RESERVED1                     0x00000020
#define DDSCAPS2_RESERVED2                     0x00000040
#define DDSCAPS2_OPAQUE                        0x00000080
#define DDSCAPS2_HINTANTIALIASING              0x00000100
#define DDSCAPS2_CUBEMAP                       0x00000200
#define DDSCAPS2_CUBEMAP_POSITIVEX             0x00000400
#define DDSCAPS2_CUBEMAP_NEGATIVEX             0x00000800
#define DDSCAPS2_CUBEMAP_POSITIVEY             0x00001000
#define DDSCAPS2_CUBEMAP_NEGATIVEY             0x00002000
#define DDSCAPS2_CUBEMAP_POSITIVEZ             0x00004000
#define DDSCAPS2_CUBEMAP_NEGATIVEZ             0x00008000
#define DDSCAPS2_CUBEMAP_ALLFACES              ( DDSCAPS2_CUBEMAP_POSITIVEX |\
                                                 DDSCAPS2_CUBEMAP_NEGATIVEX |\
                                                 DDSCAPS2_CUBEMAP_POSITIVEY |\
                                                 DDSCAPS2_CUBEMAP_NEGATIVEY |\
                                                 DDSCAPS2_CUBEMAP_POSITIVEZ |\
                                                 DDSCAPS2_CUBEMAP_NEGATIVEZ )

#define DDSCAPS2_MIPMAPSUBLEVEL                0x00010000
#define DDSCAPS2_D3DTEXTUREMANAGE              0x00020000
#define DDSCAPS2_DONOTPERSIST                  0x00040000
#define DDSCAPS2_STEREOSURFACELEFT             0x00080000
#define DDSCAPS2_VOLUME                        0x00200000
#define DDSCAPS2_NOTUSERLOCKABLE               0x00400000
#define DDSCAPS2_POINTS                        0x00800000

#define DDSCAPS2_RTPATCHES                     0x01000000
#define DDSCAPS2_NPATCHES                      0x02000000
#define DDSCAPS2_RESERVED3                     0x04000000
#define DDSCAPS2_DISCARDBACKBUFFER             0x10000000
#define DDSCAPS2_ENABLEALPHACHANNEL            0x20000000
#define DDSCAPS2_EXTENDEDFORMATPRIMARY         0x40000000
#define DDSCAPS2_ADDITIONALPRIMARY             0x80000000

#define DDSCAPS3_MULTISAMPLE_MASK              0x0000001F
#define DDSCAPS3_MULTISAMPLE_QUALITY_MASK      0x000000E0
#define DDSCAPS3_MULTISAMPLE_QUALITY_SHIFT     5
#define DDSCAPS3_RESERVED1                     0x00000100
#define DDSCAPS3_RESERVED2                     0x00000200
#define DDSCAPS3_LIGHTWEIGHTMIPMAP             0x00000400
#define DDSCAPS3_AUTOGENMIPMAP                 0x00000800
#define DDSCAPS3_DMAP                          0x00001000

#define DDCAPS_3D                              0x00000001
#define DDCAPS_ALIGNBOUNDARYDEST               0x00000002
#define DDCAPS_ALIGNSIZEDEST                   0x00000004
#define DDCAPS_ALIGNBOUNDARYSRC                0x00000008
#define DDCAPS_ALIGNSIZESRC                    0x00000010
#define DDCAPS_ALIGNSTRIDE                     0x00000020
#define DDCAPS_BLT                             0x00000040
#define DDCAPS_BLTQUEUE                        0x00000080
#define DDCAPS_BLTFOURCC                       0x00000100
#define DDCAPS_BLTSTRETCH                      0x00000200
#define DDCAPS_GDI                             0x00000400
#define DDCAPS_OVERLAY                         0x00000800
#define DDCAPS_OVERLAYCANTCLIP                 0x00001000
#define DDCAPS_OVERLAYFOURCC                   0x00002000
#define DDCAPS_OVERLAYSTRETCH                  0x00004000
#define DDCAPS_PALETTE                         0x00008000
#define DDCAPS_PALETTEVSYNC                    0x00010000
#define DDCAPS_READSCANLINE                    0x00020000
#define DDCAPS_RESERVED1                       0x00040000
#define DDCAPS_VBI                             0x00080000
#define DDCAPS_ZBLTS                           0x00100000
#define DDCAPS_ZOVERLAYS                       0x00200000
#define DDCAPS_COLORKEY                        0x00400000
#define DDCAPS_ALPHA                           0x00800000
#define DDCAPS_COLORKEYHWASSIST                0x01000000
#define DDCAPS_NOHARDWARE                      0x02000000
#define DDCAPS_BLTCOLORFILL                    0x04000000
#define DDCAPS_BANKSWITCHED                    0x08000000
#define DDCAPS_BLTDEPTHFILL                    0x10000000
#define DDCAPS_CANCLIP                         0x20000000
#define DDCAPS_CANCLIPSTRETCHED                0x40000000
#define DDCAPS_CANBLTSYSMEM                    0x80000000

#define DDCAPS2_CERTIFIED                      0x00000001
#define DDCAPS2_NO2DDURING3DSCENE              0x00000002
#define DDCAPS2_VIDEOPORT                      0x00000004
#define DDCAPS2_AUTOFLIPOVERLAY                0x00000008
#define DDCAPS2_CANBOBINTERLEAVED              0x00000010
#define DDCAPS2_CANBOBNONINTERLEAVED           0x00000020
#define DDCAPS2_COLORCONTROLOVERLAY            0x00000040
#define DDCAPS2_COLORCONTROLPRIMARY            0x00000080
#define DDCAPS2_CANDROPZ16BIT                  0x00000100
#define DDCAPS2_NONLOCALVIDMEM                 0x00000200
#define DDCAPS2_NONLOCALVIDMEMCAPS             0x00000400
#define DDCAPS2_NOPAGELOCKREQUIRED             0x00000800
#define DDCAPS2_WIDESURFACES                   0x00001000
#define DDCAPS2_CANFLIPODDEVEN                 0x00002000
#define DDCAPS2_CANBOBHARDWARE                 0x00004000
#define DDCAPS2_COPYFOURCC                     0x00008000
#define DDCAPS2_PRIMARYGAMMA                   0x00020000
#define DDCAPS2_CANRENDERWINDOWED              0x00080000
#define DDCAPS2_CANCALIBRATEGAMMA              0x00100000
#define DDCAPS2_FLIPINTERVAL                   0x00200000
#define DDCAPS2_FLIPNOVSYNC                    0x00400000
#define DDCAPS2_CANMANAGETEXTURE               0x00800000
#define DDCAPS2_TEXMANINNONLOCALVIDMEM         0x01000000
#define DDCAPS2_STEREO                         0x02000000
#define DDCAPS2_SYSTONONLOCAL_AS_SYSTOLOCAL    0x04000000
#define DDCAPS2_RESERVED1                      0x08000000
#define DDCAPS2_CANMANAGERESOURCE              0x10000000
#define DDCAPS2_DYNAMICTEXTURES                0x20000000
#define DDCAPS2_CANAUTOGENMIPMAP               0x40000000

#define DDFXALPHACAPS_BLTALPHAEDGEBLEND        0x00000001
#define DDFXALPHACAPS_BLTALPHAPIXELS           0x00000002
#define DDFXALPHACAPS_BLTALPHAPIXELSNEG        0x00000004
#define DDFXALPHACAPS_BLTALPHASURFACES         0x00000008
#define DDFXALPHACAPS_BLTALPHASURFACESNEG      0x00000010
#define DDFXALPHACAPS_OVERLAYALPHAEDGEBLEND    0x00000020
#define DDFXALPHACAPS_OVERLAYALPHAPIXELS       0x00000040
#define DDFXALPHACAPS_OVERLAYALPHAPIXELSNEG    0x00000080
#define DDFXALPHACAPS_OVERLAYALPHASURFACES     0x00000100
#define DDFXALPHACAPS_OVERLAYALPHASURFACESNEG  0x00000200


#define DDFXCAPS_BLTALPHA                      0x00000001
#define DDFXCAPS_OVERLAYALPHA                  0x00000004
#define DDFXCAPS_OVERLAYARITHSTRETCHYN         0x00000008
#define DDFXCAPS_BLTARITHSTRETCHY              0x00000020
#define DDFXCAPS_BLTARITHSTRETCHYN             0x00000010
#define DDFXCAPS_BLTMIRRORLEFTRIGHT            0x00000040
#define DDFXCAPS_BLTMIRRORUPDOWN               0x00000080
#define DDFXCAPS_BLTROTATION                   0x00000100
#define DDFXCAPS_BLTROTATION90                 0x00000200
#define DDFXCAPS_BLTSHRINKX                    0x00000400
#define DDFXCAPS_BLTSHRINKXN                   0x00000800
#define DDFXCAPS_BLTSHRINKY                    0x00001000
#define DDFXCAPS_BLTSHRINKYN                   0x00002000
#define DDFXCAPS_BLTSTRETCHX                   0x00004000
#define DDFXCAPS_BLTSTRETCHXN                  0x00008000
#define DDFXCAPS_BLTSTRETCHY                   0x00010000
#define DDFXCAPS_BLTSTRETCHYN                  0x00020000
#define DDFXCAPS_OVERLAYARITHSTRETCHY          0x00040000
#define DDFXCAPS_OVERLAYSHRINKX                0x00080000
#define DDFXCAPS_OVERLAYSHRINKXN               0x00100000
#define DDFXCAPS_OVERLAYSHRINKY                0x00200000
#define DDFXCAPS_OVERLAYSHRINKYN               0x00400000
#define DDFXCAPS_OVERLAYSTRETCHX               0x00800000
#define DDFXCAPS_OVERLAYSTRETCHXN              0x01000000
#define DDFXCAPS_OVERLAYSTRETCHY               0x02000000
#define DDFXCAPS_OVERLAYSTRETCHYN              0x04000000
#define DDFXCAPS_OVERLAYMIRRORLEFTRIGHT        0x08000000
#define DDFXCAPS_OVERLAYMIRRORUPDOWN           0x10000000
#define DDFXCAPS_OVERLAYDEINTERLACE            0x20000000
#define DDFXCAPS_BLTFILTER                     DDFXCAPS_BLTARITHSTRETCHY
#define DDFXCAPS_OVERLAYFILTER                 DDFXCAPS_OVERLAYARITHSTRETCHY


  #define DDSVCAPS_RESERVED1                     0x00000001
  #define DDSVCAPS_RESERVED2                     0x00000002
  #define DDSVCAPS_RESERVED3                     0x00000004
  #define DDSVCAPS_RESERVED4                     0x00000008
/* rember that DDSVCAPS_ENIGMA is same as DDSVCAPS_RESERVED1 */
  #define DDSVCAPS_ENIGMA                        0x00000001
/* rember that DDSVCAPS_FLICKER is same as DDSVCAPS_RESERVED2 */
  #define DDSVCAPS_FLICKER                       0x00000002
/* rember that DDSVCAPS_REDBLUE is same as DDSVCAPS_RESERVED3 */
  #define DDSVCAPS_REDBLUE                       0x00000004
/* rember that DDSVCAPS_SPLIT is same as DDSVCAPS_RESERVED4 */
  #define DDSVCAPS_SPLIT                         0x00000008

#define DDSVCAPS_STEREOSEQUENTIAL              0x00000010

#define DDPCAPS_INITIALIZE                     0x00000000
#define DDPCAPS_4BIT                           0x00000001
#define DDPCAPS_8BITENTRIES                    0x00000002
#define DDPCAPS_8BIT                           0x00000004
#define DDPCAPS_PRIMARYSURFACE                 0x00000010
#define DDPCAPS_PRIMARYSURFACELEFT             0x00000020
#define DDPCAPS_ALLOW256                       0x00000040
#define DDPCAPS_VSYNC                          0x00000080
#define DDPCAPS_1BIT                           0x00000100
#define DDPCAPS_2BIT                           0x00000200
#define DDPCAPS_ALPHA                          0x00000400

#define DDSPD_IUNKNOWNPOINTER                  0x00000001
#define DDSPD_VOLATILE                         0x00000002

#define DDBD_1                                 0x00004000
#define DDBD_2                                 0x00002000
#define DDBD_4                                 0x00001000
#define DDBD_8                                 0x00000800
#define DDBD_16                                0x00000400
#define DDBD_24                                0x00000200
#define DDBD_32                                0x00000100

#define DDCKEY_COLORSPACE                      0x00000001
#define DDCKEY_DESTBLT                         0x00000002
#define DDCKEY_DESTOVERLAY                     0x00000004
#define DDCKEY_SRCBLT                          0x00000008
#define DDCKEY_SRCOVERLAY                      0x00000010

#define DDCKEYCAPS_DESTBLT                     0x00000001
#define DDCKEYCAPS_DESTBLTCLRSPACE             0x00000002
#define DDCKEYCAPS_DESTBLTCLRSPACEYUV          0x00000004
#define DDCKEYCAPS_DESTBLTYUV                  0x00000008
#define DDCKEYCAPS_DESTOVERLAY                 0x00000010
#define DDCKEYCAPS_DESTOVERLAYCLRSPACE         0x00000020
#define DDCKEYCAPS_DESTOVERLAYCLRSPACEYUV      0x00000040
#define DDCKEYCAPS_DESTOVERLAYONEACTIVE        0x00000080
#define DDCKEYCAPS_DESTOVERLAYYUV              0x00000100
#define DDCKEYCAPS_SRCBLT                      0x00000200
#define DDCKEYCAPS_SRCBLTCLRSPACE              0x00000400
#define DDCKEYCAPS_SRCBLTCLRSPACEYUV           0x00000800
#define DDCKEYCAPS_SRCBLTYUV                   0x00001000
#define DDCKEYCAPS_SRCOVERLAY                  0x00002000
#define DDCKEYCAPS_SRCOVERLAYCLRSPACE          0x00004000
#define DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV       0x00008000
#define DDCKEYCAPS_SRCOVERLAYONEACTIVE         0x00010000
#define DDCKEYCAPS_SRCOVERLAYYUV               0x00020000
#define DDCKEYCAPS_NOCOSTOVERLAY               0x00040000

#define DDPF_ALPHAPIXELS                       0x00000001
#define DDPF_ALPHA                             0x00000002
#define DDPF_FOURCC                            0x00000004
#define DDPF_PALETTEINDEXED4                   0x00000008
#define DDPF_PALETTEINDEXEDTO8                 0x00000010
#define DDPF_PALETTEINDEXED8                   0x00000020
#define DDPF_RGB                               0x00000040
#define DDPF_COMPRESSED                        0x00000080
#define DDPF_RGBTOYUV                          0x00000100
#define DDPF_YUV                               0x00000200
#define DDPF_ZBUFFER                           0x00000400
#define DDPF_PALETTEINDEXED1                   0x00000800
#define DDPF_PALETTEINDEXED2                   0x00001000
#define DDPF_ZPIXELS                           0x00002000
#define DDPF_STENCILBUFFER                     0x00004000
#define DDPF_ALPHAPREMULT                      0x00008000
#define DDPF_LUMINANCE                         0x00020000
#define DDPF_BUMPLUMINANCE                     0x00040000
#define DDPF_BUMPDUDV                          0x00080000

#define DDENUMSURFACES_ALL                     0x00000001
#define DDENUMSURFACES_MATCH                   0x00000002
#define DDENUMSURFACES_NOMATCH                 0x00000004
#define DDENUMSURFACES_CANBECREATED            0x00000008
#define DDENUMSURFACES_DOESEXIST               0x00000010

#define DDSDM_STANDARDVGAMODE                  0x00000001

#define DDEDM_REFRESHRATES                     0x00000001
#define DDEDM_STANDARDVGAMODES                 0x00000002

#define DDSCL_FULLSCREEN                       0x00000001
#define DDSCL_ALLOWREBOOT                      0x00000002
#define DDSCL_NOWINDOWCHANGES                  0x00000004
#define DDSCL_NORMAL                           0x00000008
#define DDSCL_EXCLUSIVE                        0x00000010
#define DDSCL_ALLOWMODEX                       0x00000040
#define DDSCL_SETFOCUSWINDOW                   0x00000080
#define DDSCL_SETDEVICEWINDOW                  0x00000100
#define DDSCL_CREATEDEVICEWINDOW               0x00000200
#define DDSCL_MULTITHREADED                    0x00000400
#define DDSCL_FPUSETUP                         0x00000800
#define DDSCL_FPUPRESERVE                      0x00001000

#define DDBLT_ALPHADEST                        0x00000001
#define DDBLT_ALPHADESTCONSTOVERRIDE           0x00000002
#define DDBLT_ALPHADESTNEG                     0x00000004
#define DDBLT_ALPHADESTSURFACEOVERRIDE         0x00000008
#define DDBLT_ALPHAEDGEBLEND                   0x00000010
#define DDBLT_ALPHASRC                         0x00000020
#define DDBLT_ALPHASRCCONSTOVERRIDE            0x00000040
#define DDBLT_ALPHASRCNEG                      0x00000080
#define DDBLT_ALPHASRCSURFACEOVERRIDE          0x00000100
#define DDBLT_ASYNC                            0x00000200
#define DDBLT_COLORFILL                        0x00000400
#define DDBLT_DDFX                             0x00000800
#define DDBLT_DDROPS                           0x00001000
#define DDBLT_KEYDEST                          0x00002000
#define DDBLT_KEYDESTOVERRIDE                  0x00004000
#define DDBLT_KEYSRC                           0x00008000
#define DDBLT_KEYSRCOVERRIDE                   0x00010000
#define DDBLT_ROP                              0x00020000
#define DDBLT_ROTATIONANGLE                    0x00040000
#define DDBLT_ZBUFFER                          0x00080000
#define DDBLT_ZBUFFERDESTCONSTOVERRIDE         0x00100000
#define DDBLT_ZBUFFERDESTOVERRIDE              0x00200000
#define DDBLT_ZBUFFERSRCCONSTOVERRIDE          0x00400000
#define DDBLT_ZBUFFERSRCOVERRIDE               0x00800000
#define DDBLT_WAIT                             0x01000000
#define DDBLT_DEPTHFILL                        0x02000000
#define DDBLT_DONOTWAIT                        0x08000000
#define DDBLT_PRESENTATION                     0x10000000
#define DDBLT_LAST_PRESENTATION                0x20000000
#define DDBLT_EXTENDED_FLAGS                   0x40000000
#define DDBLT_EXTENDED_LINEAR_CONTENT          0x00000004

#define DDBLTFAST_NOCOLORKEY                   0x00000000
#define DDBLTFAST_SRCCOLORKEY                  0x00000001
#define DDBLTFAST_DESTCOLORKEY                 0x00000002
#define DDBLTFAST_WAIT                         0x00000010
#define DDBLTFAST_DONOTWAIT                    0x00000020

#define DDFLIP_WAIT                            0x00000001
#define DDFLIP_EVEN                            0x00000002
#define DDFLIP_ODD                             0x00000004
#define DDFLIP_NOVSYNC                         0x00000008
#define DDFLIP_STEREO                          0x00000010
#define DDFLIP_DONOTWAIT                       0x00000020
#define DDFLIP_INTERVAL2                       0x02000000
#define DDFLIP_INTERVAL3                       0x03000000
#define DDFLIP_INTERVAL4                       0x04000000


#define DDOVER_ALPHADEST                       0x00000001
#define DDOVER_ALPHADESTCONSTOVERRIDE          0x00000002
#define DDOVER_ALPHADESTNEG                    0x00000004
#define DDOVER_ALPHADESTSURFACEOVERRIDE        0x00000008
#define DDOVER_ALPHAEDGEBLEND                  0x00000010
#define DDOVER_ALPHASRC                        0x00000020
#define DDOVER_ALPHASRCCONSTOVERRIDE           0x00000040
#define DDOVER_ALPHASRCNEG                     0x00000080
#define DDOVER_ALPHASRCSURFACEOVERRIDE         0x00000100
#define DDOVER_HIDE                            0x00000200
#define DDOVER_KEYDEST                         0x00000400
#define DDOVER_KEYDESTOVERRIDE                 0x00000800
#define DDOVER_KEYSRC                          0x00001000
#define DDOVER_KEYSRCOVERRIDE                  0x00002000
#define DDOVER_SHOW                            0x00004000
#define DDOVER_ADDDIRTYRECT                    0x00008000
#define DDOVER_REFRESHDIRTYRECTS               0x00010000
#define DDOVER_REFRESHALL                      0x00020000
#define DDOVER_DDFX                            0x00080000
#define DDOVER_AUTOFLIP                        0x00100000
#define DDOVER_BOB                             0x00200000
#define DDOVER_OVERRIDEBOBWEAVE                0x00400000
#define DDOVER_INTERLEAVED                     0x00800000
#define DDOVER_BOBHARDWARE                     0x01000000
#define DDOVER_ARGBSCALEFACTORS                0x02000000
#define DDOVER_DEGRADEARGBSCALING              0x04000000

#define DDLOCK_SURFACEMEMORYPTR                0x00000000
#define DDLOCK_WAIT                            0x00000001
#define DDLOCK_EVENT                           0x00000002
#define DDLOCK_READONLY                        0x00000010
#define DDLOCK_WRITEONLY                       0x00000020
#define DDLOCK_NOSYSLOCK                       0x00000800
#define DDLOCK_NOOVERWRITE                     0x00001000
#define DDLOCK_DISCARDCONTENTS                 0x00002000
#define DDLOCK_OKTOSWAP                        0x00002000
#define DDLOCK_DONOTWAIT                       0x00004000
#define DDLOCK_HASVOLUMETEXTUREBOXRECT         0x00008000
#define DDLOCK_NODIRTYUPDATE                   0x00010000

#define DDBLTFX_ARITHSTRETCHY                  0x00000001
#define DDBLTFX_MIRRORLEFTRIGHT                0x00000002
#define DDBLTFX_MIRRORUPDOWN                   0x00000004
#define DDBLTFX_NOTEARING                      0x00000008
#define DDBLTFX_ROTATE180                      0x00000010
#define DDBLTFX_ROTATE270                      0x00000020
#define DDBLTFX_ROTATE90                       0x00000040
#define DDBLTFX_ZBUFFERRANGE                   0x00000080
#define DDBLTFX_ZBUFFERBASEDEST                0x00000100

#define DDOVERFX_ARITHSTRETCHY                 0x00000001
#define DDOVERFX_MIRRORLEFTRIGHT               0x00000002
#define DDOVERFX_MIRRORUPDOWN                  0x00000004
#define DDOVERFX_DEINTERLACE                   0x00000008

#define DDWAITVB_BLOCKBEGIN                    0x00000001
#define DDWAITVB_BLOCKBEGINEVENT               0x00000002
#define DDWAITVB_BLOCKEND                      0x00000004

#define DDGFS_CANFLIP                          0x00000001
#define DDGFS_ISFLIPDONE                       0x00000002

#define DDGBS_CANBLT                           0x00000001
#define DDGBS_ISBLTDONE                        0x00000002

#define DDENUMOVERLAYZ_BACKTOFRONT             0x00000000
#define DDENUMOVERLAYZ_FRONTTOBACK             0x00000001

#define DDOVERZ_SENDTOFRONT                    0x00000000
#define DDOVERZ_SENDTOBACK                     0x00000001
#define DDOVERZ_MOVEFORWARD                    0x00000002
#define DDOVERZ_MOVEBACKWARD                   0x00000003
#define DDOVERZ_INSERTINFRONTOF                0x00000004
#define DDOVERZ_INSERTINBACKOF                 0x00000005

#define GET_WHQL_YEAR  (dwWHQLLevel)  ((dwWHQLLevel)/0x10000)
#define GET_WHQL_MONTH (dwWHQLLevel)  (((dwWHQLLevel)/0x100)&0x00ff)
#define GET_WHQL_DAY   (dwWHQLLevel)  ((dwWHQLLevel)&0xff)

#ifndef MAKEFOURCC
#define MAKEFOURCC(c0,c1,c2,c3)  \
    ((DWORD)(BYTE)(c0)|((DWORD)(BYTE)(c1)<<8)|((DWORD)(BYTE)(c2)<< 16)|((DWORD)(BYTE)(c3)<<24))
#endif

#define FOURCC_DXT1 (MAKEFOURCC('D','X','T','1'))
#define FOURCC_DXT2 (MAKEFOURCC('D','X','T','2'))
#define FOURCC_DXT3 (MAKEFOURCC('D','X','T','3'))
#define FOURCC_DXT4 (MAKEFOURCC('D','X','T','4'))
#define FOURCC_DXT5 (MAKEFOURCC('D','X','T','5'))

#if defined(_WIN32)  &&  !defined(_NO_COM)
DEFINE_GUID( CLSID_DirectDraw,            0xD7B70EE0,0x4340,0x11CF,0xB0,0x63,0x00,0x20,0xAF,0xC2,0xCD,0x35 );
DEFINE_GUID( CLSID_DirectDraw7,           0x3C305196,0x50DB,0x11D3,0x9C,0xFE,0x00,0xC0,0x4F,0xD9,0x30,0xC5 );
DEFINE_GUID( CLSID_DirectDrawClipper,     0x593817A0,0x7DB3,0x11CF,0xA2,0xDE,0x00,0xAA,0x00,0xB9,0x33,0x56 );
DEFINE_GUID( IID_IDirectDraw,             0x6C14DB80,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 );
DEFINE_GUID( IID_IDirectDraw2,            0xB3A6F3E0,0x2B43,0x11CF,0xA2,0xDE,0x00,0xAA,0x00,0xB9,0x33,0x56 );
DEFINE_GUID( IID_IDirectDraw3,            0x618F8AD4,0x8B7A,0x11D0,0x8F,0xCC,0x00,0xC0,0x4F,0xD9,0x18,0x9D );
DEFINE_GUID( IID_IDirectDraw4,            0x9C59509A,0x39BD,0x11D1,0x8C,0x4A,0x00,0xC0,0x4F,0xD9,0x30,0xC5 );
DEFINE_GUID( IID_IDirectDraw7,            0x15E65EC0,0x3B9C,0x11D2,0xB9,0x2F,0x00,0x60,0x97,0x97,0xEA,0x5B );
DEFINE_GUID( IID_IDirectDrawSurface,      0x6C14DB81,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 );
DEFINE_GUID( IID_IDirectDrawSurface2,     0x57805885,0x6EEC,0x11CF,0x94,0x41,0xA8,0x23,0x03,0xC1,0x0E,0x27 );
DEFINE_GUID( IID_IDirectDrawSurface3,     0xDA044E00,0x69B2,0x11D0,0xA1,0xD5,0x00,0xAA,0x00,0xB8,0xDF,0xBB );
DEFINE_GUID( IID_IDirectDrawSurface4,     0x0B2B8630,0xAD35,0x11D0,0x8E,0xA6,0x00,0x60,0x97,0x97,0xEA,0x5B );
DEFINE_GUID( IID_IDirectDrawSurface7,     0x06675A80,0x3B9B,0x11D2,0xB9,0x2F,0x00,0x60,0x97,0x97,0xEA,0x5B );
DEFINE_GUID( IID_IDirectDrawPalette,      0x6C14DB84,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 );
DEFINE_GUID( IID_IDirectDrawClipper,      0x6C14DB85,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 );
DEFINE_GUID( IID_IDirectDrawColorControl, 0x4B9F0EE0,0x0D7E,0x11D0,0x9B,0x06,0x00,0xA0,0xC9,0x03,0xA3,0xB8 );
DEFINE_GUID( IID_IDirectDrawGammaControl, 0x69C11C3E,0xB46B,0x11D1,0xAD,0x7A,0x00,0xC0,0x4F,0xC2,0x9B,0x4E );
#endif


struct IDirectDraw;
struct IDirectDrawSurface;
struct IDirectDrawPalette;
struct IDirectDrawClipper;

typedef struct IDirectDraw              *LPDIRECTDRAW;
typedef struct IDirectDraw2             *LPDIRECTDRAW2;
typedef struct IDirectDraw3             *LPDIRECTDRAW3;
typedef struct IDirectDraw4             *LPDIRECTDRAW4;
typedef struct IDirectDraw7             *LPDIRECTDRAW7;
typedef struct IDirectDrawSurface       *LPDIRECTDRAWSURFACE;
typedef struct IDirectDrawSurface2      *LPDIRECTDRAWSURFACE2;
typedef struct IDirectDrawSurface3      *LPDIRECTDRAWSURFACE3;
typedef struct IDirectDrawSurface4      *LPDIRECTDRAWSURFACE4;
typedef struct IDirectDrawSurface7      *LPDIRECTDRAWSURFACE7;
typedef struct IDirectDrawPalette       *LPDIRECTDRAWPALETTE;
typedef struct IDirectDrawClipper       *LPDIRECTDRAWCLIPPER;
typedef struct IDirectDrawColorControl  *LPDIRECTDRAWCOLORCONTROL;
typedef struct IDirectDrawGammaControl  *LPDIRECTDRAWGAMMACONTROL;

typedef struct _DDSCAPS
{
   DWORD dwCaps;
} DDSCAPS, *LPDDSCAPS;

typedef struct _DDSCAPS2
{
   DWORD dwCaps;
   DWORD dwCaps2;
   DWORD dwCaps3;
   union
   {
      DWORD dwCaps4;
      DWORD dwVolumeDepth;
   } DUMMYUNIONNAMEN(1);
} DDSCAPS2, *LPDDSCAPS2;

typedef struct _DDCAPS_DX1
{
   DWORD dwSize;
   DWORD dwCaps;
   DWORD dwCaps2;
   DWORD dwCKeyCaps;
   DWORD dwFXCaps;
   DWORD dwFXAlphaCaps;
   DWORD dwPalCaps;
   DWORD dwSVCaps;
   DWORD dwAlphaBltConstBitDepths;
   DWORD dwAlphaBltPixelBitDepths;
   DWORD dwAlphaBltSurfaceBitDepths;
   DWORD dwAlphaOverlayConstBitDepths;
   DWORD dwAlphaOverlayPixelBitDepths;
   DWORD dwAlphaOverlaySurfaceBitDepths;
   DWORD dwZBufferBitDepths;
   DWORD dwVidMemTotal;
   DWORD dwVidMemFree;
   DWORD dwMaxVisibleOverlays;
   DWORD dwCurrVisibleOverlays;
   DWORD dwNumFourCCCodes;
   DWORD dwAlignBoundarySrc;
   DWORD dwAlignSizeSrc;
   DWORD dwAlignBoundaryDest;
   DWORD dwAlignSizeDest;
   DWORD dwAlignStrideAlign;
   DWORD dwRops[DD_ROP_SPACE];
   DDSCAPS ddsCaps;
   DWORD dwMinOverlayStretch;
   DWORD dwMaxOverlayStretch;
   DWORD dwMinLiveVideoStretch;
   DWORD dwMaxLiveVideoStretch;
   DWORD dwMinHwCodecStretch;
   DWORD dwMaxHwCodecStretch;
   DWORD dwReserved1;
   DWORD dwReserved2;
   DWORD dwReserved3;
} DDCAPS_DX1, *LPDDCAPS_DX1;

typedef struct _DDCAPS_DX3
{
   DWORD dwSize;
   DWORD dwCaps;
   DWORD dwCaps2;
   DWORD dwCKeyCaps;
   DWORD dwFXCaps;
   DWORD dwFXAlphaCaps;
   DWORD dwPalCaps;
   DWORD dwSVCaps;
   DWORD dwAlphaBltConstBitDepths;
   DWORD dwAlphaBltPixelBitDepths;
   DWORD dwAlphaBltSurfaceBitDepths;
   DWORD dwAlphaOverlayConstBitDepths;
   DWORD dwAlphaOverlayPixelBitDepths;
   DWORD dwAlphaOverlaySurfaceBitDepths;
   DWORD dwZBufferBitDepths;
   DWORD dwVidMemTotal;
   DWORD dwVidMemFree;
   DWORD dwMaxVisibleOverlays;
   DWORD dwCurrVisibleOverlays;
   DWORD dwNumFourCCCodes;
   DWORD dwAlignBoundarySrc;
   DWORD dwAlignSizeSrc;
   DWORD dwAlignBoundaryDest;
   DWORD dwAlignSizeDest;
   DWORD dwAlignStrideAlign;
   DWORD dwRops[DD_ROP_SPACE];
   DDSCAPS ddsCaps;
   DWORD dwMinOverlayStretch;
   DWORD dwMaxOverlayStretch;
   DWORD dwMinLiveVideoStretch;
   DWORD dwMaxLiveVideoStretch;
   DWORD dwMinHwCodecStretch;
   DWORD dwMaxHwCodecStretch;
   DWORD dwReserved1;
   DWORD dwReserved2;
   DWORD dwReserved3;
   DWORD   dwSVBCaps;
   DWORD   dwSVBCKeyCaps;
   DWORD   dwSVBFXCaps;
   DWORD   dwSVBRops[DD_ROP_SPACE];
   DWORD   dwVSBCaps;
   DWORD   dwVSBCKeyCaps;
   DWORD   dwVSBFXCaps;
   DWORD   dwVSBRops[DD_ROP_SPACE];
   DWORD   dwSSBCaps;
   DWORD   dwSSBCKeyCaps;
   DWORD   dwSSBFXCaps;
   DWORD   dwSSBRops[DD_ROP_SPACE];
   DWORD   dwReserved4;
   DWORD   dwReserved5;
   DWORD   dwReserved6;
} DDCAPS_DX3, *LPDDCAPS_DX3;

typedef struct _DDCAPS_DX5
{
   DWORD dwSize;
   DWORD dwCaps;
   DWORD dwCaps2;
   DWORD dwCKeyCaps;
   DWORD dwFXCaps;
   DWORD dwFXAlphaCaps;
   DWORD dwPalCaps;
   DWORD dwSVCaps;
   DWORD dwAlphaBltConstBitDepths;
   DWORD dwAlphaBltPixelBitDepths;
   DWORD dwAlphaBltSurfaceBitDepths;
   DWORD dwAlphaOverlayConstBitDepths;
   DWORD dwAlphaOverlayPixelBitDepths;
   DWORD dwAlphaOverlaySurfaceBitDepths;
   DWORD dwZBufferBitDepths;
   DWORD dwVidMemTotal;
   DWORD dwVidMemFree;
   DWORD dwMaxVisibleOverlays;
   DWORD dwCurrVisibleOverlays;
   DWORD dwNumFourCCCodes;
   DWORD dwAlignBoundarySrc;
   DWORD dwAlignSizeSrc;
   DWORD dwAlignBoundaryDest;
   DWORD dwAlignSizeDest;
   DWORD dwAlignStrideAlign;
   DWORD dwRops[DD_ROP_SPACE];
   DDSCAPS ddsCaps;
   DWORD dwMinOverlayStretch;
   DWORD dwMaxOverlayStretch;
   DWORD dwMinLiveVideoStretch;
   DWORD dwMaxLiveVideoStretch;
   DWORD dwMinHwCodecStretch;
   DWORD dwMaxHwCodecStretch;
   DWORD dwReserved1;
   DWORD dwReserved2;
   DWORD dwReserved3;
   DWORD dwSVBCaps;
   DWORD dwSVBCKeyCaps;
   DWORD dwSVBFXCaps;
   DWORD dwSVBRops[DD_ROP_SPACE];
   DWORD dwVSBCaps;
   DWORD dwVSBCKeyCaps;
   DWORD dwVSBFXCaps;
   DWORD dwVSBRops[DD_ROP_SPACE];
   DWORD dwSSBCaps;
   DWORD dwSSBCKeyCaps;
   DWORD dwSSBFXCaps;
   DWORD dwSSBRops[DD_ROP_SPACE];
   DWORD dwMaxVideoPorts;
   DWORD dwCurrVideoPorts;
   DWORD dwSVBCaps2;
   DWORD dwNLVBCaps;
   DWORD dwNLVBCaps2;
   DWORD dwNLVBCKeyCaps;
   DWORD dwNLVBFXCaps;
   DWORD dwNLVBRops[DD_ROP_SPACE];
} DDCAPS_DX5, *LPDDCAPS_DX5;

typedef struct _DDCAPS_DX6
{
   DWORD dwSize;
   DWORD dwCaps;
   DWORD dwCaps2;
   DWORD dwCKeyCaps;
   DWORD dwFXCaps;
   DWORD dwFXAlphaCaps;
   DWORD dwPalCaps;
   DWORD dwSVCaps;
   DWORD dwAlphaBltConstBitDepths;
   DWORD dwAlphaBltPixelBitDepths;
   DWORD dwAlphaBltSurfaceBitDepths;
   DWORD dwAlphaOverlayConstBitDepths;
   DWORD dwAlphaOverlayPixelBitDepths;
   DWORD dwAlphaOverlaySurfaceBitDepths;
   DWORD dwZBufferBitDepths;
   DWORD dwVidMemTotal;
   DWORD dwVidMemFree;
   DWORD dwMaxVisibleOverlays;
   DWORD dwCurrVisibleOverlays;
   DWORD dwNumFourCCCodes;
   DWORD dwAlignBoundarySrc;
   DWORD dwAlignSizeSrc;
   DWORD dwAlignBoundaryDest;
   DWORD dwAlignSizeDest;
   DWORD dwAlignStrideAlign;
   DWORD dwRops[DD_ROP_SPACE];
   DDSCAPS ddsOldCaps;
   DWORD dwMinOverlayStretch;
   DWORD dwMaxOverlayStretch;
   DWORD dwMinLiveVideoStretch;
   DWORD dwMaxLiveVideoStretch;
   DWORD dwMinHwCodecStretch;
   DWORD dwMaxHwCodecStretch;
   DWORD dwReserved1;
   DWORD dwReserved2;
   DWORD dwReserved3;
   DWORD dwSVBCaps;
   DWORD dwSVBCKeyCaps;
   DWORD dwSVBFXCaps;
   DWORD dwSVBRops[DD_ROP_SPACE];
   DWORD dwVSBCaps;
   DWORD dwVSBCKeyCaps;
   DWORD dwVSBFXCaps;
   DWORD dwVSBRops[DD_ROP_SPACE];
   DWORD dwSSBCaps;
   DWORD dwSSBCKeyCaps;
   DWORD dwSSBFXCaps;
   DWORD dwSSBRops[DD_ROP_SPACE];
   DWORD dwMaxVideoPorts;
   DWORD dwCurrVideoPorts;
   DWORD dwSVBCaps2;
   DWORD dwNLVBCaps;
   DWORD dwNLVBCaps2;
   DWORD dwNLVBCKeyCaps;
   DWORD dwNLVBFXCaps;
   DWORD dwNLVBRops[DD_ROP_SPACE];
   DDSCAPS2 ddsCaps;
} DDCAPS_DX6, *LPDDCAPS_DX6;

typedef struct _DDCAPS_DX7
{
   DWORD dwSize;
   DWORD dwCaps;
   DWORD dwCaps2;
   DWORD dwCKeyCaps;
   DWORD dwFXCaps;
   DWORD dwFXAlphaCaps;
   DWORD dwPalCaps;
   DWORD dwSVCaps;
   DWORD dwAlphaBltConstBitDepths;
   DWORD dwAlphaBltPixelBitDepths;
   DWORD dwAlphaBltSurfaceBitDepths;
   DWORD dwAlphaOverlayConstBitDepths;
   DWORD dwAlphaOverlayPixelBitDepths;
   DWORD dwAlphaOverlaySurfaceBitDepths;
   DWORD dwZBufferBitDepths;
   DWORD dwVidMemTotal;
   DWORD dwVidMemFree;
   DWORD dwMaxVisibleOverlays;
   DWORD dwCurrVisibleOverlays;
   DWORD dwNumFourCCCodes;
   DWORD dwAlignBoundarySrc;
   DWORD dwAlignSizeSrc;
   DWORD dwAlignBoundaryDest;
   DWORD dwAlignSizeDest;
   DWORD dwAlignStrideAlign;
   DWORD dwRops[DD_ROP_SPACE];
   DDSCAPS ddsOldCaps;
   DWORD dwMinOverlayStretch;
   DWORD dwMaxOverlayStretch;
   DWORD dwMinLiveVideoStretch;
   DWORD dwMaxLiveVideoStretch;
   DWORD dwMinHwCodecStretch;
   DWORD dwMaxHwCodecStretch;
   DWORD dwReserved1;
   DWORD dwReserved2;
   DWORD dwReserved3;
   DWORD dwSVBCaps;
   DWORD dwSVBCKeyCaps;
   DWORD dwSVBFXCaps;
   DWORD dwSVBRops[DD_ROP_SPACE];
   DWORD dwVSBCaps;
   DWORD dwVSBCKeyCaps;
   DWORD dwVSBFXCaps;
   DWORD dwVSBRops[DD_ROP_SPACE];
   DWORD dwSSBCaps;
   DWORD dwSSBCKeyCaps;
   DWORD dwSSBFXCaps;
   DWORD dwSSBRops[DD_ROP_SPACE];
   DWORD dwMaxVideoPorts;
   DWORD dwCurrVideoPorts;
   DWORD dwSVBCaps2;
   DWORD dwNLVBCaps;
   DWORD dwNLVBCaps2;
   DWORD dwNLVBCKeyCaps;
   DWORD dwNLVBFXCaps;
   DWORD dwNLVBRops[DD_ROP_SPACE];
   DDSCAPS2 ddsCaps;
} DDCAPS_DX7, *LPDDCAPS_DX7;

#if DIRECTDRAW_VERSION<=0x300
   typedef DDCAPS_DX3 DDCAPS;
#elif DIRECTDRAW_VERSION<=0x500
   typedef DDCAPS_DX5 DDCAPS;
#elif DIRECTDRAW_VERSION<=0x600
   typedef DDCAPS_DX6 DDCAPS;
#else
   typedef DDCAPS_DX7 DDCAPS;
#endif
typedef DDCAPS *LPDDCAPS;


typedef struct _DDCOLORCONTROL
{
  DWORD dwSize;
  DWORD dwFlags;
  LONG lBrightness;
  LONG lContrast;
  LONG lHue;
  LONG lSaturation;
  LONG lSharpness;
  LONG lGamma;
  LONG lColorEnable;
  DWORD dwReserved1;
} DDCOLORCONTROL, *LPDDCOLORCONTROL;

typedef struct _DDCOLORKEY
{
   DWORD dwColorSpaceLowValue;
   DWORD dwColorSpaceHighValue;

} DDCOLORKEY, *LPDDCOLORKEY;

typedef struct _DDOSCAPS
{
   DWORD dwCaps;
} DDOSCAPS, *LPDDOSCAPS;

typedef struct _DDSCAPSEX
{
   DWORD dwCaps2;
   DWORD dwCaps3;
   union
   {
      DWORD dwCaps4;
      DWORD dwVolumeDepth;
   } DUMMYUNIONNAMEN(1);
} DDSCAPSEX, *LPDDSCAPSEX;

typedef struct _DDPIXELFORMAT
{
   DWORD dwSize;
   DWORD dwFlags;
   DWORD dwFourCC;
   union
   {
      DWORD dwRGBBitCount;
      DWORD dwYUVBitCount;
      DWORD dwZBufferBitDepth;
      DWORD dwAlphaBitDepth;
      DWORD dwLuminanceBitCount;
      DWORD dwBumpBitCount;
      DWORD dwPrivateFormatBitCount;
   } DUMMYUNIONNAMEN(1);
   union
   {
      DWORD dwRBitMask;
      DWORD dwYBitMask;
      DWORD dwStencilBitDepth;
      DWORD dwLuminanceBitMask;
      DWORD dwBumpDuBitMask;
      DWORD dwOperations;
   } DUMMYUNIONNAMEN(2);
   union
   {
      DWORD dwGBitMask;
      DWORD dwUBitMask;
      DWORD dwZBitMask;
      DWORD dwBumpDvBitMask;
      struct
      {
         WORD wFlipMSTypes;
         WORD wBltMSTypes;
      } MultiSampleCaps;
   } DUMMYUNIONNAMEN(3);
   union
   {
      DWORD dwBBitMask;
      DWORD dwVBitMask;
      DWORD dwStencilBitMask;
      DWORD dwBumpLuminanceBitMask;
   } DUMMYUNIONNAMEN(4);
   union
   {
      DWORD dwRGBAlphaBitMask;
      DWORD dwYUVAlphaBitMask;
      DWORD dwLuminanceAlphaBitMask;
      DWORD dwRGBZBitMask;
      DWORD dwYUVZBitMask;
   } DUMMYUNIONNAMEN(5);
} DDPIXELFORMAT, *LPDDPIXELFORMAT;

typedef struct _DDSURFACEDESC
{
   DWORD dwSize;
   DWORD dwFlags;
   DWORD dwHeight;
   DWORD dwWidth;
   union
   {
      LONG lPitch;
      DWORD dwLinearSize;
   } DUMMYUNIONNAMEN(1);
   DWORD dwBackBufferCount;
   union
   {
      DWORD dwMipMapCount;
      DWORD dwZBufferBitDepth;
      DWORD dwRefreshRate;
   } DUMMYUNIONNAMEN(2);
   DWORD dwAlphaBitDepth;
   DWORD dwReserved;
   LPVOID lpSurface;
   DDCOLORKEY ddckCKDestOverlay;
   DDCOLORKEY ddckCKDestBlt;
   DDCOLORKEY ddckCKSrcOverlay;
   DDCOLORKEY ddckCKSrcBlt;
   DDPIXELFORMAT ddpfPixelFormat;
   DDSCAPS ddsCaps;
} DDSURFACEDESC, *LPDDSURFACEDESC;

typedef struct _DDSURFACEDESC2
{
   DWORD dwSize;
   DWORD dwFlags;
   DWORD dwHeight;
   DWORD dwWidth;
   union
   {
      LONG lPitch;
      DWORD dwLinearSize;
   } DUMMYUNIONNAMEN(1);
   union
   {
      DWORD dwBackBufferCount;
      DWORD dwDepth;
   } DUMMYUNIONNAMEN(5);
   union
   {
      DWORD dwMipMapCount;
      DWORD dwRefreshRate;
      DWORD dwSrcVBHandle;
   } DUMMYUNIONNAMEN(2);
   DWORD dwAlphaBitDepth;
   DWORD dwReserved;
   LPVOID lpSurface;
   union
   {
      DDCOLORKEY ddckCKDestOverlay;
      DWORD dwEmptyFaceColor;
   } DUMMYUNIONNAMEN(3);
   DDCOLORKEY ddckCKDestBlt;
   DDCOLORKEY ddckCKSrcOverlay;
   DDCOLORKEY ddckCKSrcBlt;
   union
   {
      DDPIXELFORMAT ddpfPixelFormat;
      DWORD dwFVF;
   } DUMMYUNIONNAMEN(4);
   DDSCAPS2 ddsCaps;
   DWORD dwTextureStage;
} DDSURFACEDESC2, *LPDDSURFACEDESC2;

typedef struct _DDOPTSURFACEDESC
{
   DWORD dwSize;
   DWORD dwFlags;
   DDSCAPS2 ddSCaps;
   DDOSCAPS ddOSCaps;
   GUID guid;
   DWORD dwCompressionRatio;
} DDOPTSURFACEDESC;

typedef struct _DDARGB
{
    BYTE blue;
    BYTE green;
    BYTE red;
    BYTE alpha;
} DDARGB, *LPDDARGB;

typedef struct _DDRGBA
{
    BYTE red;
    BYTE green;
    BYTE blue;
    BYTE alpha;
} DDRGBA, *LPDDRGBA;


#if (defined (WIN32) || defined( _WIN32 ) ) && !defined( _NO_COM )
  typedef BOOL (WINAPI *LPDDENUMCALLBACKA)(GUID *, LPSTR, LPSTR, LPVOID);
  typedef BOOL (WINAPI *LPDDENUMCALLBACKW)(GUID *, LPWSTR, LPWSTR, LPVOID);
  extern HRESULT WINAPI DirectDrawEnumerateW( LPDDENUMCALLBACKW lpCallback, LPVOID lpContext );
  extern HRESULT WINAPI DirectDrawEnumerateA( LPDDENUMCALLBACKA lpCallback, LPVOID lpContext );

  #if !defined(HMONITOR_DECLARED) && (WINVER < 0x0500)
        #define HMONITOR_DECLARED
        DECLARE_HANDLE(HMONITOR);
  #endif

  typedef BOOL (WINAPI *LPDDENUMCALLBACKEXA)(GUID *, LPSTR, LPSTR, LPVOID, HMONITOR);
  typedef BOOL (WINAPI *LPDDENUMCALLBACKEXW)(GUID *, LPWSTR, LPWSTR, LPVOID, HMONITOR);

  extern HRESULT WINAPI DirectDrawEnumerateExW( LPDDENUMCALLBACKEXW lpCallback, LPVOID lpContext, DWORD dwFlags);
  extern HRESULT WINAPI DirectDrawEnumerateExA( LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags);

  typedef HRESULT (WINAPI * LPDIRECTDRAWENUMERATEEXA)( LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags);
  typedef HRESULT (WINAPI * LPDIRECTDRAWENUMERATEEXW)( LPDDENUMCALLBACKEXW lpCallback, LPVOID lpContext, DWORD dwFlags);

  #ifdef UNICODE
    typedef LPDDENUMCALLBACKW        LPDDENUMCALLBACK;
    #define DirectDrawEnumerate      DirectDrawEnumerateW
    typedef LPDDENUMCALLBACKEXW      LPDDENUMCALLBACKEX;
    typedef LPDIRECTDRAWENUMERATEEXW LPDIRECTDRAWENUMERATEEX;
    #define DirectDrawEnumerateEx    DirectDrawEnumerateExW
  #else
    typedef LPDDENUMCALLBACKA        LPDDENUMCALLBACK;
    #define DirectDrawEnumerate      DirectDrawEnumerateA
    typedef LPDDENUMCALLBACKEXA      LPDDENUMCALLBACKEX;
    typedef LPDIRECTDRAWENUMERATEEXA LPDIRECTDRAWENUMERATEEX;
    #define DirectDrawEnumerateEx    DirectDrawEnumerateExA
  #endif
    extern HRESULT WINAPI DirectDrawCreate(GUID *lpGUID,LPDIRECTDRAW *lplpDD, IUnknown *pUnkOuter );
    extern HRESULT WINAPI DirectDrawCreateEx(GUID * lpGuid,LPVOID  *lplpDD,REFIID iid,IUnknown *pUnkOuter );
    extern HRESULT WINAPI DirectDrawCreateClipper(DWORD dwFlags,LPDIRECTDRAWCLIPPER *lplpDDClipper,IUnknown *pUnkOuter );
#endif


typedef HRESULT (WINAPI *LPDDENUMMODESCALLBACK)(LPDDSURFACEDESC, LPVOID);
typedef HRESULT (WINAPI *LPDDENUMMODESCALLBACK2)(LPDDSURFACEDESC2, LPVOID);
typedef HRESULT (WINAPI *LPDDENUMSURFACESCALLBACK)(LPDIRECTDRAWSURFACE, LPDDSURFACEDESC, LPVOID);
typedef HRESULT (WINAPI *LPDDENUMSURFACESCALLBACK2)(LPDIRECTDRAWSURFACE4, LPDDSURFACEDESC2, LPVOID);
typedef HRESULT (WINAPI *LPDDENUMSURFACESCALLBACK7)(LPDIRECTDRAWSURFACE7, LPDDSURFACEDESC2, LPVOID);

typedef DWORD (*LPCLIPPERCALLBACK)(LPDIRECTDRAWCLIPPER lpDDClipper, HWND hWnd, DWORD code, LPVOID lpContext );
#ifdef STREAMING
typedef DWORD   (*LPSURFACESTREAMINGCALLBACK)(DWORD);
#endif


typedef struct _DDBLTFX
{
   DWORD dwSize;
   DWORD dwDDFX;
   DWORD dwROP;
   DWORD dwDDROP;
   DWORD dwRotationAngle;
   DWORD dwZBufferOpCode;
   DWORD dwZBufferLow;
   DWORD dwZBufferHigh;
   DWORD dwZBufferBaseDest;
   DWORD dwZDestConstBitDepth;
   union
   {
      DWORD dwZDestConst;
      LPDIRECTDRAWSURFACE lpDDSZBufferDest;
   } DUMMYUNIONNAMEN(1);
   DWORD dwZSrcConstBitDepth;
   union
   {
      DWORD dwZSrcConst;
      LPDIRECTDRAWSURFACE lpDDSZBufferSrc;
   } DUMMYUNIONNAMEN(2);
   DWORD dwAlphaEdgeBlendBitDepth;
   DWORD dwAlphaEdgeBlend;
   DWORD dwReserved;
   DWORD dwAlphaDestConstBitDepth;
   union
   {
      DWORD dwAlphaDestConst;
      LPDIRECTDRAWSURFACE lpDDSAlphaDest;
   } DUMMYUNIONNAMEN(3);
   DWORD dwAlphaSrcConstBitDepth;
   union
   {
      DWORD dwAlphaSrcConst;
      LPDIRECTDRAWSURFACE lpDDSAlphaSrc;
   } DUMMYUNIONNAMEN(4);
   union
   {
      DWORD dwFillColor;
      DWORD dwFillDepth;
      DWORD dwFillPixel;
      LPDIRECTDRAWSURFACE lpDDSPattern;
   } DUMMYUNIONNAMEN(5);
   DDCOLORKEY ddckDestColorkey;
   DDCOLORKEY ddckSrcColorkey;
} DDBLTFX, *LPDDBLTFX;

typedef struct _DDGAMMARAMP
{
   WORD red[256];
   WORD green[256];
   WORD blue[256];
} DDGAMMARAMP, *LPDDGAMMARAMP;

typedef struct tagDDDEVICEIDENTIFIER
{
   char szDriver[MAX_DDDEVICEID_STRING];
   char szDescription[MAX_DDDEVICEID_STRING];
#ifdef _WIN32
   LARGE_INTEGER liDriverVersion;
#else
   DWORD dwDriverVersionLowPart;
   DWORD dwDriverVersionHighPart;
#endif
   DWORD dwVendorId;
   DWORD dwDeviceId;
   DWORD dwSubSysId;
   DWORD dwRevision;
   GUID guidDeviceIdentifier;
} DDDEVICEIDENTIFIER, * LPDDDEVICEIDENTIFIER;

typedef struct tagDDDEVICEIDENTIFIER2
{
   char szDriver[MAX_DDDEVICEID_STRING];
   char szDescription[MAX_DDDEVICEID_STRING];
#ifdef _WIN32
   LARGE_INTEGER liDriverVersion;
#else
   DWORD dwDriverVersionLowPart;
   DWORD dwDriverVersionHighPart;
#endif
   DWORD dwVendorId;
   DWORD dwDeviceId;
   DWORD dwSubSysId;
   DWORD dwRevision;
   GUID guidDeviceIdentifier;
   DWORD dwWHQLLevel;
} DDDEVICEIDENTIFIER2, *LPDDDEVICEIDENTIFIER2;

typedef struct _DDBLTBATCH
{
   LPRECT lprDest;
   LPDIRECTDRAWSURFACE lpDDSSrc;
   LPRECT lprSrc;
   DWORD dwFlags;
   LPDDBLTFX lpDDBltFx;
} DDBLTBATCH, *LPDDBLTBATCH;

typedef struct _DDOVERLAYFX
{
   DWORD dwSize;
   DWORD dwAlphaEdgeBlendBitDepth;
   DWORD dwAlphaEdgeBlend;
   DWORD dwReserved;
   DWORD dwAlphaDestConstBitDepth;
   union
   {
      DWORD dwAlphaDestConst;
      LPDIRECTDRAWSURFACE lpDDSAlphaDest;
   } DUMMYUNIONNAMEN(1);
   DWORD dwAlphaSrcConstBitDepth;
   union
   {
      DWORD dwAlphaSrcConst;
      LPDIRECTDRAWSURFACE lpDDSAlphaSrc;
   } DUMMYUNIONNAMEN(2);
   DDCOLORKEY dckDestColorkey;
   DDCOLORKEY dckSrcColorkey;
   DWORD dwDDFX;
   DWORD dwFlags;
} DDOVERLAYFX, *LPDDOVERLAYFX;



#if defined( _WIN32 ) && !defined( _NO_COM )
    #undef INTERFACE
    #define INTERFACE IDirectDraw

    DECLARE_INTERFACE_( IDirectDraw, IUnknown )
    {
      STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID *ppvObj) PURE;
      STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
      STDMETHOD_(ULONG,Release) (THIS) PURE;

      STDMETHOD(Compact)(THIS) PURE;
      STDMETHOD(CreateClipper)(THIS_ DWORD, LPDIRECTDRAWCLIPPER *, IUnknown * ) PURE;
      STDMETHOD(CreatePalette)(THIS_ DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE *, IUnknown * ) PURE;
      STDMETHOD(CreateSurface)(THIS_  LPDDSURFACEDESC, LPDIRECTDRAWSURFACE *, IUnknown *) PURE;
      STDMETHOD(DuplicateSurface)( THIS_ LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE * ) PURE;
      STDMETHOD(EnumDisplayModes)( THIS_ DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMMODESCALLBACK ) PURE;
      STDMETHOD(EnumSurfaces)(THIS_ DWORD, LPDDSURFACEDESC, LPVOID,LPDDENUMSURFACESCALLBACK ) PURE;
      STDMETHOD(FlipToGDISurface)(THIS) PURE;
      STDMETHOD(GetCaps)( THIS_ LPDDCAPS, LPDDCAPS) PURE;
      STDMETHOD(GetDisplayMode)( THIS_ LPDDSURFACEDESC) PURE;
      STDMETHOD(GetFourCCCodes)(THIS_  LPDWORD, LPDWORD ) PURE;
      STDMETHOD(GetGDISurface)(THIS_ LPDIRECTDRAWSURFACE *) PURE;
      STDMETHOD(GetMonitorFrequency)(THIS_ LPDWORD) PURE;
      STDMETHOD(GetScanLine)(THIS_ LPDWORD) PURE;
      STDMETHOD(GetVerticalBlankStatus)(THIS_ LPBOOL ) PURE;
      STDMETHOD(Initialize)(THIS_ GUID *) PURE;
      STDMETHOD(RestoreDisplayMode)(THIS) PURE;
      STDMETHOD(SetCooperativeLevel)(THIS_ HWND, DWORD) PURE;
      STDMETHOD(SetDisplayMode)(THIS_ DWORD, DWORD,DWORD) PURE;
      STDMETHOD(WaitForVerticalBlank)(THIS_ DWORD, HANDLE ) PURE;
     };

     #if !defined(__cplusplus) || defined(CINTERFACE)
         #define IDirectDraw_QueryInterface(p, a, b)         (p)->lpVtbl->QueryInterface(p, a, b)
         #define IDirectDraw_AddRef(p)                       (p)->lpVtbl->AddRef(p)
         #define IDirectDraw_Release(p)                      (p)->lpVtbl->Release(p)
         #define IDirectDraw_Compact(p)                      (p)->lpVtbl->Compact(p)
         #define IDirectDraw_CreateClipper(p, a, b, c)       (p)->lpVtbl->CreateClipper(p, a, b, c)
         #define IDirectDraw_CreatePalette(p, a, b, c, d)    (p)->lpVtbl->CreatePalette(p, a, b, c, d)
         #define IDirectDraw_CreateSurface(p, a, b, c)       (p)->lpVtbl->CreateSurface(p, a, b, c)
         #define IDirectDraw_DuplicateSurface(p, a, b)       (p)->lpVtbl->DuplicateSurface(p, a, b)
         #define IDirectDraw_EnumDisplayModes(p, a, b, c, d) (p)->lpVtbl->EnumDisplayModes(p, a, b, c, d)
         #define IDirectDraw_EnumSurfaces(p, a, b, c, d)     (p)->lpVtbl->EnumSurfaces(p, a, b, c, d)
         #define IDirectDraw_FlipToGDISurface(p)             (p)->lpVtbl->FlipToGDISurface(p)
         #define IDirectDraw_GetCaps(p, a, b)                (p)->lpVtbl->GetCaps(p, a, b)
         #define IDirectDraw_GetDisplayMode(p, a)            (p)->lpVtbl->GetDisplayMode(p, a)
         #define IDirectDraw_GetFourCCCodes(p, a, b)         (p)->lpVtbl->GetFourCCCodes(p, a, b)
         #define IDirectDraw_GetGDISurface(p, a)             (p)->lpVtbl->GetGDISurface(p, a)
         #define IDirectDraw_GetMonitorFrequency(p, a)       (p)->lpVtbl->GetMonitorFrequency(p, a)
         #define IDirectDraw_GetScanLine(p, a)               (p)->lpVtbl->GetScanLine(p, a)
         #define IDirectDraw_GetVerticalBlankStatus(p, a)    (p)->lpVtbl->GetVerticalBlankStatus(p, a)
         #define IDirectDraw_Initialize(p, a)                (p)->lpVtbl->Initialize(p, a)
         #define IDirectDraw_RestoreDisplayMode(p)           (p)->lpVtbl->RestoreDisplayMode(p)
         #define IDirectDraw_SetCooperativeLevel(p, a, b)    (p)->lpVtbl->SetCooperativeLevel(p, a, b)
         #define IDirectDraw_SetDisplayMode(p, a, b, c)      (p)->lpVtbl->SetDisplayMode(p, a, b, c)
         #define IDirectDraw_WaitForVerticalBlank(p, a, b)   (p)->lpVtbl->WaitForVerticalBlank(p, a, b)
     #else
         #define IDirectDraw_QueryInterface(p, a, b)         (p)->QueryInterface(a, b)
         #define IDirectDraw_AddRef(p)                       (p)->AddRef()
         #define IDirectDraw_Release(p)                      (p)->Release()
         #define IDirectDraw_Compact(p)                      (p)->Compact()
         #define IDirectDraw_CreateClipper(p, a, b, c)       (p)->CreateClipper(a, b, c)
         #define IDirectDraw_CreatePalette(p, a, b, c, d)    (p)->CreatePalette(a, b, c, d)
         #define IDirectDraw_CreateSurface(p, a, b, c)       (p)->CreateSurface(a, b, c)
         #define IDirectDraw_DuplicateSurface(p, a, b)       (p)->DuplicateSurface(a, b)
         #define IDirectDraw_EnumDisplayModes(p, a, b, c, d) (p)->EnumDisplayModes(a, b, c, d)
         #define IDirectDraw_EnumSurfaces(p, a, b, c, d)     (p)->EnumSurfaces(a, b, c, d)
         #define IDirectDraw_FlipToGDISurface(p)             (p)->FlipToGDISurface()
         #define IDirectDraw_GetCaps(p, a, b)                (p)->GetCaps(a, b)
         #define IDirectDraw_GetDisplayMode(p, a)            (p)->GetDisplayMode(a)
         #define IDirectDraw_GetFourCCCodes(p, a, b)         (p)->GetFourCCCodes(a, b)
         #define IDirectDraw_GetGDISurface(p, a)             (p)->GetGDISurface(a)
         #define IDirectDraw_GetMonitorFrequency(p, a)       (p)->GetMonitorFrequency(a)
         #define IDirectDraw_GetScanLine(p, a)               (p)->GetScanLine(a)
         #define IDirectDraw_GetVerticalBlankStatus(p, a)    (p)->GetVerticalBlankStatus(a)
         #define IDirectDraw_Initialize(p, a)                (p)->Initialize(a)
         #define IDirectDraw_RestoreDisplayMode(p)           (p)->RestoreDisplayMode()
         #define IDirectDraw_SetCooperativeLevel(p, a, b)    (p)->SetCooperativeLevel(a, b)
         #define IDirectDraw_SetDisplayMode(p, a, b, c)      (p)->SetDisplayMode(a, b, c)
         #define IDirectDraw_WaitForVerticalBlank(p, a, b)   (p)->WaitForVerticalBlank(a, b)
     #endif
#endif

#if defined( _WIN32 ) && !defined( _NO_COM )
    #undef INTERFACE
    #define INTERFACE IDirectDraw2
    DECLARE_INTERFACE_( IDirectDraw2, IUnknown )
    {
      STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
      STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
      STDMETHOD_(ULONG,Release) (THIS) PURE;
      STDMETHOD(Compact)(THIS) PURE;
      STDMETHOD(CreateClipper)(THIS_ DWORD, LPDIRECTDRAWCLIPPER *, IUnknown  * ) PURE;
      STDMETHOD(CreatePalette)(THIS_ DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE *, IUnknown * ) PURE;
      STDMETHOD(CreateSurface)(THIS_  LPDDSURFACEDESC, LPDIRECTDRAWSURFACE *, IUnknown *) PURE;
      STDMETHOD(DuplicateSurface)( THIS_ LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE * ) PURE;
      STDMETHOD(EnumDisplayModes)( THIS_ DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMMODESCALLBACK ) PURE;
      STDMETHOD(EnumSurfaces)(THIS_ DWORD, LPDDSURFACEDESC, LPVOID,LPDDENUMSURFACESCALLBACK ) PURE;
      STDMETHOD(FlipToGDISurface)(THIS) PURE;
      STDMETHOD(GetCaps)( THIS_ LPDDCAPS, LPDDCAPS) PURE;
      STDMETHOD(GetDisplayMode)( THIS_ LPDDSURFACEDESC) PURE;
      STDMETHOD(GetFourCCCodes)(THIS_  LPDWORD, LPDWORD ) PURE;
      STDMETHOD(GetGDISurface)(THIS_ LPDIRECTDRAWSURFACE *) PURE;
      STDMETHOD(GetMonitorFrequency)(THIS_ LPDWORD) PURE;
      STDMETHOD(GetScanLine)(THIS_ LPDWORD) PURE;
      STDMETHOD(GetVerticalBlankStatus)(THIS_ LPBOOL ) PURE;
      STDMETHOD(Initialize)(THIS_ GUID *) PURE;
      STDMETHOD(RestoreDisplayMode)(THIS) PURE;
      STDMETHOD(SetCooperativeLevel)(THIS_ HWND, DWORD) PURE;
      STDMETHOD(SetDisplayMode)(THIS_ DWORD, DWORD,DWORD, DWORD, DWORD) PURE;
      STDMETHOD(WaitForVerticalBlank)(THIS_ DWORD, HANDLE ) PURE;
      STDMETHOD(GetAvailableVidMem)(THIS_ LPDDSCAPS, LPDWORD, LPDWORD) PURE;
     };

     #if !defined(__cplusplus) || defined(CINTERFACE)
         #define IDirectDraw2_QueryInterface(p, a, b)         (p)->lpVtbl->QueryInterface(p, a, b)
         #define IDirectDraw2_AddRef(p)                       (p)->lpVtbl->AddRef(p)
         #define IDirectDraw2_Release(p)                      (p)->lpVtbl->Release(p)
         #define IDirectDraw2_Compact(p)                      (p)->lpVtbl->Compact(p)
         #define IDirectDraw2_CreateClipper(p, a, b, c)       (p)->lpVtbl->CreateClipper(p, a, b, c)
         #define IDirectDraw2_CreatePalette(p, a, b, c, d)    (p)->lpVtbl->CreatePalette(p, a, b, c, d)
         #define IDirectDraw2_CreateSurface(p, a, b, c)       (p)->lpVtbl->CreateSurface(p, a, b, c)
         #define IDirectDraw2_DuplicateSurface(p, a, b)       (p)->lpVtbl->DuplicateSurface(p, a, b)
         #define IDirectDraw2_EnumDisplayModes(p, a, b, c, d) (p)->lpVtbl->EnumDisplayModes(p, a, b, c, d)
         #define IDirectDraw2_EnumSurfaces(p, a, b, c, d)     (p)->lpVtbl->EnumSurfaces(p, a, b, c, d)
         #define IDirectDraw2_FlipToGDISurface(p)             (p)->lpVtbl->FlipToGDISurface(p)
         #define IDirectDraw2_GetCaps(p, a, b)                (p)->lpVtbl->GetCaps(p, a, b)
         #define IDirectDraw2_GetDisplayMode(p, a)            (p)->lpVtbl->GetDisplayMode(p, a)
         #define IDirectDraw2_GetFourCCCodes(p, a, b)         (p)->lpVtbl->GetFourCCCodes(p, a, b)
         #define IDirectDraw2_GetGDISurface(p, a)             (p)->lpVtbl->GetGDISurface(p, a)
         #define IDirectDraw2_GetMonitorFrequency(p, a)       (p)->lpVtbl->GetMonitorFrequency(p, a)
         #define IDirectDraw2_GetScanLine(p, a)               (p)->lpVtbl->GetScanLine(p, a)
         #define IDirectDraw2_GetVerticalBlankStatus(p, a)    (p)->lpVtbl->GetVerticalBlankStatus(p, a)
         #define IDirectDraw2_Initialize(p, a)                (p)->lpVtbl->Initialize(p, a)
         #define IDirectDraw2_RestoreDisplayMode(p)           (p)->lpVtbl->RestoreDisplayMode(p)
         #define IDirectDraw2_SetCooperativeLevel(p, a, b)    (p)->lpVtbl->SetCooperativeLevel(p, a, b)
         #define IDirectDraw2_SetDisplayMode(p, a, b, c, d, e) (p)->lpVtbl->SetDisplayMode(p, a, b, c, d, e)
         #define IDirectDraw2_WaitForVerticalBlank(p, a, b)   (p)->lpVtbl->WaitForVerticalBlank(p, a, b)
         #define IDirectDraw2_GetAvailableVidMem(p, a, b, c)  (p)->lpVtbl->GetAvailableVidMem(p, a, b, c)
     #else
         #define IDirectDraw2_QueryInterface(p, a, b)         (p)->QueryInterface(a, b)
         #define IDirectDraw2_AddRef(p)                       (p)->AddRef()
         #define IDirectDraw2_Release(p)                      (p)->Release()
         #define IDirectDraw2_Compact(p)                      (p)->Compact()
         #define IDirectDraw2_CreateClipper(p, a, b, c)       (p)->CreateClipper(a, b, c)
         #define IDirectDraw2_CreatePalette(p, a, b, c, d)    (p)->CreatePalette(a, b, c, d)
         #define IDirectDraw2_CreateSurface(p, a, b, c)       (p)->CreateSurface(a, b, c)
         #define IDirectDraw2_DuplicateSurface(p, a, b)       (p)->DuplicateSurface(a, b)
         #define IDirectDraw2_EnumDisplayModes(p, a, b, c, d) (p)->EnumDisplayModes(a, b, c, d)
         #define IDirectDraw2_EnumSurfaces(p, a, b, c, d)     (p)->EnumSurfaces(a, b, c, d)
         #define IDirectDraw2_FlipToGDISurface(p)             (p)->FlipToGDISurface()
         #define IDirectDraw2_GetCaps(p, a, b)                (p)->GetCaps(a, b)
         #define IDirectDraw2_GetDisplayMode(p, a)            (p)->GetDisplayMode(a)
         #define IDirectDraw2_GetFourCCCodes(p, a, b)         (p)->GetFourCCCodes(a, b)
         #define IDirectDraw2_GetGDISurface(p, a)             (p)->GetGDISurface(a)
         #define IDirectDraw2_GetMonitorFrequency(p, a)       (p)->GetMonitorFrequency(a)
         #define IDirectDraw2_GetScanLine(p, a)               (p)->GetScanLine(a)
         #define IDirectDraw2_GetVerticalBlankStatus(p, a)    (p)->GetVerticalBlankStatus(a)
         #define IDirectDraw2_Initialize(p, a)                (p)->Initialize(a)
         #define IDirectDraw2_RestoreDisplayMode(p)           (p)->RestoreDisplayMode()
         #define IDirectDraw2_SetCooperativeLevel(p, a, b)    (p)->SetCooperativeLevel(a, b)
         #define IDirectDraw2_SetDisplayMode(p, a, b, c, d, e) (p)->SetDisplayMode(a, b, c, d, e)
         #define IDirectDraw2_WaitForVerticalBlank(p, a, b)   (p)->WaitForVerticalBlank(a, b)
         #define IDirectDraw2_GetAvailableVidMem(p, a, b, c)  (p)->GetAvailableVidMem(a, b, c)
     #endif
#endif

#if defined( _WIN32 ) && !defined( _NO_COM )
    #undef INTERFACE
    #define INTERFACE IDirectDraw3
    DECLARE_INTERFACE_(IDirectDraw3,IUnknown)
    {
        STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
        STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
        STDMETHOD_(ULONG,Release) (THIS) PURE;
        STDMETHOD(Compact)(THIS) PURE;
        STDMETHOD(CreateClipper)(THIS_ DWORD, LPDIRECTDRAWCLIPPER *, IUnknown  * ) PURE;
        STDMETHOD(CreatePalette)(THIS_ DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE *, IUnknown * ) PURE;
        STDMETHOD(CreateSurface)(THIS_  LPDDSURFACEDESC, LPDIRECTDRAWSURFACE *, IUnknown *) PURE;
        STDMETHOD(DuplicateSurface)( THIS_ LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE * ) PURE;
        STDMETHOD(EnumDisplayModes)( THIS_ DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMMODESCALLBACK ) PURE;
        STDMETHOD(EnumSurfaces)(THIS_ DWORD, LPDDSURFACEDESC, LPVOID,LPDDENUMSURFACESCALLBACK ) PURE;
        STDMETHOD(FlipToGDISurface)(THIS) PURE;
        STDMETHOD(GetCaps)( THIS_ LPDDCAPS, LPDDCAPS) PURE;
        STDMETHOD(GetDisplayMode)( THIS_ LPDDSURFACEDESC) PURE;
        STDMETHOD(GetFourCCCodes)(THIS_  LPDWORD, LPDWORD ) PURE;
        STDMETHOD(GetGDISurface)(THIS_ LPDIRECTDRAWSURFACE *) PURE;
        STDMETHOD(GetMonitorFrequency)(THIS_ LPDWORD) PURE;
        STDMETHOD(GetScanLine)(THIS_ LPDWORD) PURE;
        STDMETHOD(GetVerticalBlankStatus)(THIS_ LPBOOL ) PURE;
        STDMETHOD(Initialize)(THIS_ GUID *) PURE;
        STDMETHOD(RestoreDisplayMode)(THIS) PURE;
        STDMETHOD(SetCooperativeLevel)(THIS_ HWND, DWORD) PURE;
        STDMETHOD(SetDisplayMode)(THIS_ DWORD, DWORD,DWORD, DWORD, DWORD) PURE;
        STDMETHOD(WaitForVerticalBlank)(THIS_ DWORD, HANDLE ) PURE;
        STDMETHOD(GetAvailableVidMem)(THIS_ LPDDSCAPS, LPDWORD, LPDWORD) PURE;
        STDMETHOD(GetSurfaceFromDC)(THIS_ HDC, LPDIRECTDRAWSURFACE*) PURE;
};

    #if !defined(__cplusplus) || defined(CINTERFACE)
        #define IDirectDraw3_QueryInterface(p,a,b)          (p)->lpVtbl->QueryInterface(p,a,b)
        #define IDirectDraw3_AddRef(p)                      (p)->lpVtbl->AddRef(p)
        #define IDirectDraw3_Release(p)                     (p)->lpVtbl->Release(p)
        #define IDirectDraw3_Compact(p)                     (p)->lpVtbl->Compact(p)
        #define IDirectDraw3_CreateClipper(p,a,b,c)         (p)->lpVtbl->CreateClipper(p,a,b,c)
        #define IDirectDraw3_CreatePalette(p,a,b,c,d)       (p)->lpVtbl->CreatePalette(p,a,b,c,d)
        #define IDirectDraw3_CreateSurface(p,a,b,c)         (p)->lpVtbl->CreateSurface(p,a,b,c)
        #define IDirectDraw3_DuplicateSurface(p,a,b)        (p)->lpVtbl->DuplicateSurface(p,a,b)
        #define IDirectDraw3_EnumDisplayModes(p,a,b,c,d)    (p)->lpVtbl->EnumDisplayModes(p,a,b,c,d)
        #define IDirectDraw3_EnumSurfaces(p,a,b,c,d)        (p)->lpVtbl->EnumSurfaces(p,a,b,c,d)
        #define IDirectDraw3_FlipToGDISurface(p)            (p)->lpVtbl->FlipToGDISurface(p)
        #define IDirectDraw3_GetCaps(p,a,b)                 (p)->lpVtbl->GetCaps(p,a,b)
        #define IDirectDraw3_GetDisplayMode(p,a)            (p)->lpVtbl->GetDisplayMode(p,a)
        #define IDirectDraw3_GetFourCCCodes(p,a,b)          (p)->lpVtbl->GetFourCCCodes(p,a,b)
        #define IDirectDraw3_GetGDISurface(p,a)             (p)->lpVtbl->GetGDISurface(p,a)
        #define IDirectDraw3_GetMonitorFrequency(p,a)       (p)->lpVtbl->GetMonitorFrequency(p,a)
        #define IDirectDraw3_GetScanLine(p,a)               (p)->lpVtbl->GetScanLine(p,a)
        #define IDirectDraw3_GetVerticalBlankStatus(p,a)    (p)->lpVtbl->GetVerticalBlankStatus(p,a)
        #define IDirectDraw3_Initialize(p,a)                (p)->lpVtbl->Initialize(p,a)
        #define IDirectDraw3_RestoreDisplayMode(p)          (p)->lpVtbl->RestoreDisplayMode(p)
        #define IDirectDraw3_SetCooperativeLevel(p,a,b)     (p)->lpVtbl->SetCooperativeLevel(p,a,b)
        #define IDirectDraw3_SetDisplayMode(p,a,b,c,d,e)    (p)->lpVtbl->SetDisplayMode(p,a,b,c,d,e)
        #define IDirectDraw3_WaitForVerticalBlank(p,a,b)    (p)->lpVtbl->WaitForVerticalBlank(p,a,b)
        #define IDirectDraw3_GetAvailableVidMem(p,a,b,c)    (p)->lpVtbl->GetAvailableVidMem(p,a,b,c)
        #define IDirectDraw3_GetSurfaceFromDC(p,a,b)        (p)->lpVtbl->GetSurfaceFromDC(p,a,b)
    #else
        #define IDirectDraw3_QueryInterface(p,a,b)          (p)->QueryInterface(a,b)
        #define IDirectDraw3_AddRef(p)                      (p)->AddRef()
        #define IDirectDraw3_Release(p)                     (p)->Release()
        #define IDirectDraw3_Compact(p)                     (p)->Compact()
        #define IDirectDraw3_CreateClipper(p,a,b,c)         (p)->CreateClipper(a,b,c)
        #define IDirectDraw3_CreatePalette(p,a,b,c,d)       (p)->CreatePalette(a,b,c,d)
        #define IDirectDraw3_CreateSurface(p,a,b,c)         (p)->CreateSurface(a,b,c)
        #define IDirectDraw3_DuplicateSurface(p,a,b)        (p)->DuplicateSurface(a,b)
        #define IDirectDraw3_EnumDisplayModes(p,a,b,c,d)    (p)->EnumDisplayModes(a,b,c,d)
        #define IDirectDraw3_EnumSurfaces(p,a,b,c,d)        (p)->EnumSurfaces(a,b,c,d)
        #define IDirectDraw3_FlipToGDISurface(p)            (p)->FlipToGDISurface()
        #define IDirectDraw3_GetCaps(p,a,b)                 (p)->GetCaps(a,b)
        #define IDirectDraw3_GetDisplayMode(p,a)            (p)->GetDisplayMode(a)
        #define IDirectDraw3_GetFourCCCodes(p,a,b)          (p)->GetFourCCCodes(a,b)
        #define IDirectDraw3_GetGDISurface(p,a)             (p)->GetGDISurface(a)
        #define IDirectDraw3_GetMonitorFrequency(p,a)       (p)->GetMonitorFrequency(a)
        #define IDirectDraw3_GetScanLine(p,a)               (p)->GetScanLine(a)
        #define IDirectDraw3_GetVerticalBlankStatus(p,a)    (p)->GetVerticalBlankStatus(a)
        #define IDirectDraw3_Initialize(p,a)                (p)->Initialize(a)
        #define IDirectDraw3_RestoreDisplayMode(p)          (p)->RestoreDisplayMode()
        #define IDirectDraw3_SetCooperativeLevel(p,a,b)     (p)->SetCooperativeLevel(a,b)
        #define IDirectDraw3_SetDisplayMode(p,a,b,c,d,e)    (p)->SetDisplayMode(a,b,c,d,e)
        #define IDirectDraw3_WaitForVerticalBlank(p,a,b)    (p)->WaitForVerticalBlank(a,b)
        #define IDirectDraw3_GetAvailableVidMem(p,a,b,c)    (p)->GetAvailableVidMem(a,b,c)
        #define IDirectDraw3_GetSurfaceFromDC(p,a,b)        (p)->GetSurfaceFromDC(a,b)
    #endif
#endif


#if defined( _WIN32 ) && !defined( _NO_COM )
    #undef INTERFACE
    #define INTERFACE IDirectDraw4

    DECLARE_INTERFACE_( IDirectDraw4, IUnknown )
    {
      STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
      STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
      STDMETHOD_(ULONG,Release) (THIS) PURE;
      STDMETHOD(Compact)(THIS) PURE;
      STDMETHOD(CreateClipper)(THIS_ DWORD, LPDIRECTDRAWCLIPPER *, IUnknown * ) PURE;
      STDMETHOD(CreatePalette)(THIS_ DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE *, IUnknown * ) PURE;
      STDMETHOD(CreateSurface)(THIS_  LPDDSURFACEDESC2, LPDIRECTDRAWSURFACE4 *, IUnknown *) PURE;
      STDMETHOD(DuplicateSurface)( THIS_ LPDIRECTDRAWSURFACE4, LPDIRECTDRAWSURFACE4 * ) PURE;
      STDMETHOD(EnumDisplayModes)( THIS_ DWORD, LPDDSURFACEDESC2, LPVOID, LPDDENUMMODESCALLBACK2 ) PURE;
      STDMETHOD(EnumSurfaces)(THIS_ DWORD, LPDDSURFACEDESC2, LPVOID,LPDDENUMSURFACESCALLBACK2 ) PURE;
      STDMETHOD(FlipToGDISurface)(THIS) PURE;
      STDMETHOD(GetCaps)( THIS_ LPDDCAPS, LPDDCAPS) PURE;
      STDMETHOD(GetDisplayMode)( THIS_ LPDDSURFACEDESC2) PURE;
      STDMETHOD(GetFourCCCodes)(THIS_  LPDWORD, LPDWORD ) PURE;
      STDMETHOD(GetGDISurface)(THIS_ LPDIRECTDRAWSURFACE4 *) PURE;
      STDMETHOD(GetMonitorFrequency)(THIS_ LPDWORD) PURE;
      STDMETHOD(GetScanLine)(THIS_ LPDWORD) PURE;
      STDMETHOD(GetVerticalBlankStatus)(THIS_ LPBOOL ) PURE;
      STDMETHOD(Initialize)(THIS_ GUID *) PURE;
      STDMETHOD(RestoreDisplayMode)(THIS) PURE;
      STDMETHOD(SetCooperativeLevel)(THIS_ HWND, DWORD) PURE;
      STDMETHOD(SetDisplayMode)(THIS_ DWORD, DWORD,DWORD, DWORD, DWORD) PURE;
      STDMETHOD(WaitForVerticalBlank)(THIS_ DWORD, HANDLE ) PURE;
      STDMETHOD(GetAvailableVidMem)(THIS_ LPDDSCAPS2, LPDWORD, LPDWORD) PURE;
      STDMETHOD(GetSurfaceFromDC) (THIS_ HDC, LPDIRECTDRAWSURFACE4 *) PURE;
      STDMETHOD(RestoreAllSurfaces)(THIS) PURE;
      STDMETHOD(TestCooperativeLevel)(THIS) PURE;
      STDMETHOD(GetDeviceIdentifier)(THIS_ LPDDDEVICEIDENTIFIER, DWORD ) PURE;
     };
#if !defined(__cplusplus) || defined(CINTERFACE)
         #define IDirectDraw4_QueryInterface(p, a, b)         (p)->lpVtbl->QueryInterface(p, a, b)
         #define IDirectDraw4_AddRef(p)                       (p)->lpVtbl->AddRef(p)
         #define IDirectDraw4_Release(p)                      (p)->lpVtbl->Release(p)
         #define IDirectDraw4_Compact(p)                      (p)->lpVtbl->Compact(p)
         #define IDirectDraw4_CreateClipper(p, a, b, c)       (p)->lpVtbl->CreateClipper(p, a, b, c)
         #define IDirectDraw4_CreatePalette(p, a, b, c, d)    (p)->lpVtbl->CreatePalette(p, a, b, c, d)
         #define IDirectDraw4_CreateSurface(p, a, b, c)       (p)->lpVtbl->CreateSurface(p, a, b, c)
         #define IDirectDraw4_DuplicateSurface(p, a, b)       (p)->lpVtbl->DuplicateSurface(p, a, b)
         #define IDirectDraw4_EnumDisplayModes(p, a, b, c, d) (p)->lpVtbl->EnumDisplayModes(p, a, b, c, d)
         #define IDirectDraw4_EnumSurfaces(p, a, b, c, d)     (p)->lpVtbl->EnumSurfaces(p, a, b, c, d)
         #define IDirectDraw4_FlipToGDISurface(p)             (p)->lpVtbl->FlipToGDISurface(p)
         #define IDirectDraw4_GetCaps(p, a, b)                (p)->lpVtbl->GetCaps(p, a, b)
         #define IDirectDraw4_GetDisplayMode(p, a)            (p)->lpVtbl->GetDisplayMode(p, a)
         #define IDirectDraw4_GetFourCCCodes(p, a, b)         (p)->lpVtbl->GetFourCCCodes(p, a, b)
         #define IDirectDraw4_GetGDISurface(p, a)             (p)->lpVtbl->GetGDISurface(p, a)
         #define IDirectDraw4_GetMonitorFrequency(p, a)       (p)->lpVtbl->GetMonitorFrequency(p, a)
         #define IDirectDraw4_GetScanLine(p, a)               (p)->lpVtbl->GetScanLine(p, a)
         #define IDirectDraw4_GetVerticalBlankStatus(p, a)    (p)->lpVtbl->GetVerticalBlankStatus(p, a)
         #define IDirectDraw4_Initialize(p, a)                (p)->lpVtbl->Initialize(p, a)
         #define IDirectDraw4_RestoreDisplayMode(p)           (p)->lpVtbl->RestoreDisplayMode(p)
         #define IDirectDraw4_SetCooperativeLevel(p, a, b)    (p)->lpVtbl->SetCooperativeLevel(p, a, b)
         #define IDirectDraw4_SetDisplayMode(p, a, b, c, d, e) (p)->lpVtbl->SetDisplayMode(p, a, b, c, d, e)
         #define IDirectDraw4_WaitForVerticalBlank(p, a, b)   (p)->lpVtbl->WaitForVerticalBlank(p, a, b)
         #define IDirectDraw4_GetAvailableVidMem(p, a, b, c)  (p)->lpVtbl->GetAvailableVidMem(p, a, b, c)
         #define IDirectDraw4_GetSurfaceFromDC(p, a, b)       (p)->lpVtbl->GetSurfaceFromDC(p, a, b)
         #define IDirectDraw4_RestoreAllSurfaces(p)           (p)->lpVtbl->RestoreAllSurfaces(p)
         #define IDirectDraw4_TestCooperativeLevel(p)         (p)->lpVtbl->TestCooperativeLevel(p)
         #define IDirectDraw4_GetDeviceIdentifier(p,a,b)      (p)->lpVtbl->GetDeviceIdentifier(p,a,b)
     #else
         #define IDirectDraw4_QueryInterface(p, a, b)         (p)->QueryInterface(a, b)
         #define IDirectDraw4_AddRef(p)                       (p)->AddRef()
         #define IDirectDraw4_Release(p)                      (p)->Release()
         #define IDirectDraw4_Compact(p)                      (p)->Compact()
         #define IDirectDraw4_CreateClipper(p, a, b, c)       (p)->CreateClipper(a, b, c)
         #define IDirectDraw4_CreatePalette(p, a, b, c, d)    (p)->CreatePalette(a, b, c, d)
         #define IDirectDraw4_CreateSurface(p, a, b, c)       (p)->CreateSurface(a, b, c)
         #define IDirectDraw4_DuplicateSurface(p, a, b)       (p)->DuplicateSurface(a, b)
         #define IDirectDraw4_EnumDisplayModes(p, a, b, c, d) (p)->EnumDisplayModes(a, b, c, d)
         #define IDirectDraw4_EnumSurfaces(p, a, b, c, d)     (p)->EnumSurfaces(a, b, c, d)
         #define IDirectDraw4_FlipToGDISurface(p)             (p)->FlipToGDISurface()
         #define IDirectDraw4_GetCaps(p, a, b)                (p)->GetCaps(a, b)
         #define IDirectDraw4_GetDisplayMode(p, a)            (p)->GetDisplayMode(a)
         #define IDirectDraw4_GetFourCCCodes(p, a, b)         (p)->GetFourCCCodes(a, b)
         #define IDirectDraw4_GetGDISurface(p, a)             (p)->GetGDISurface(a)
         #define IDirectDraw4_GetMonitorFrequency(p, a)       (p)->GetMonitorFrequency(a)
         #define IDirectDraw4_GetScanLine(p, a)               (p)->GetScanLine(a)
         #define IDirectDraw4_GetVerticalBlankStatus(p, a)    (p)->GetVerticalBlankStatus(a)
         #define IDirectDraw4_Initialize(p, a)                (p)->Initialize(a)
         #define IDirectDraw4_RestoreDisplayMode(p)           (p)->RestoreDisplayMode()
         #define IDirectDraw4_SetCooperativeLevel(p, a, b)    (p)->SetCooperativeLevel(a, b)
         #define IDirectDraw4_SetDisplayMode(p, a, b, c, d, e) (p)->SetDisplayMode(a, b, c, d, e)
         #define IDirectDraw4_WaitForVerticalBlank(p, a, b)   (p)->WaitForVerticalBlank(a, b)
         #define IDirectDraw4_GetAvailableVidMem(p, a, b, c)  (p)->GetAvailableVidMem(a, b, c)
         #define IDirectDraw4_GetSurfaceFromDC(p, a, b)       (p)->GetSurfaceFromDC(a, b)
         #define IDirectDraw4_RestoreAllSurfaces(p)           (p)->RestoreAllSurfaces()
         #define IDirectDraw4_TestCooperativeLevel(p)         (p)->TestCooperativeLevel()
         #define IDirectDraw4_GetDeviceIdentifier(p,a,b)      (p)->GetDeviceIdentifier(a,b)
     #endif
#endif

#if defined( _WIN32 ) && !defined( _NO_COM )
    #undef INTERFACE
    #define INTERFACE IDirectDraw7

    DECLARE_INTERFACE_( IDirectDraw7, IUnknown )
    {
      STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
      STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
      STDMETHOD_(ULONG,Release) (THIS) PURE;
      STDMETHOD(Compact)(THIS) PURE;
      STDMETHOD(CreateClipper)(THIS_ DWORD, LPDIRECTDRAWCLIPPER *, IUnknown * ) PURE;
      STDMETHOD(CreatePalette)(THIS_ DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE *, IUnknown * ) PURE;
      STDMETHOD(CreateSurface)(THIS_  LPDDSURFACEDESC2, LPDIRECTDRAWSURFACE7 *, IUnknown *) PURE;
      STDMETHOD(DuplicateSurface)( THIS_ LPDIRECTDRAWSURFACE7, LPDIRECTDRAWSURFACE7 * ) PURE;
      STDMETHOD(EnumDisplayModes)( THIS_ DWORD, LPDDSURFACEDESC2, LPVOID, LPDDENUMMODESCALLBACK2 ) PURE;
      STDMETHOD(EnumSurfaces)(THIS_ DWORD, LPDDSURFACEDESC2, LPVOID,LPDDENUMSURFACESCALLBACK7 ) PURE;
      STDMETHOD(FlipToGDISurface)(THIS) PURE;
      STDMETHOD(GetCaps)( THIS_ LPDDCAPS, LPDDCAPS) PURE;
      STDMETHOD(GetDisplayMode)( THIS_ LPDDSURFACEDESC2) PURE;
      STDMETHOD(GetFourCCCodes)(THIS_  LPDWORD, LPDWORD ) PURE;
      STDMETHOD(GetGDISurface)(THIS_ LPDIRECTDRAWSURFACE7 *) PURE;
      STDMETHOD(GetMonitorFrequency)(THIS_ LPDWORD) PURE;
      STDMETHOD(GetScanLine)(THIS_ LPDWORD) PURE;
      STDMETHOD(GetVerticalBlankStatus)(THIS_ LPBOOL ) PURE;
      STDMETHOD(Initialize)(THIS_ GUID *) PURE;
      STDMETHOD(RestoreDisplayMode)(THIS) PURE;
      STDMETHOD(SetCooperativeLevel)(THIS_ HWND, DWORD) PURE;
      STDMETHOD(SetDisplayMode)(THIS_ DWORD, DWORD,DWORD, DWORD, DWORD) PURE;
      STDMETHOD(WaitForVerticalBlank)(THIS_ DWORD, HANDLE ) PURE;
      STDMETHOD(GetAvailableVidMem)(THIS_ LPDDSCAPS2, LPDWORD, LPDWORD) PURE;
      STDMETHOD(GetSurfaceFromDC) (THIS_ HDC, LPDIRECTDRAWSURFACE7 *) PURE;
      STDMETHOD(RestoreAllSurfaces)(THIS) PURE;
      STDMETHOD(TestCooperativeLevel)(THIS) PURE;
      STDMETHOD(GetDeviceIdentifier)(THIS_ LPDDDEVICEIDENTIFIER2, DWORD ) PURE;
      STDMETHOD(StartModeTest)(THIS_ LPSIZE, DWORD, DWORD ) PURE;
      STDMETHOD(EvaluateMode)(THIS_ DWORD, DWORD * ) PURE;
    };
     #if !defined(__cplusplus) || defined(CINTERFACE)
         #define IDirectDraw7_QueryInterface(p, a, b)         (p)->lpVtbl->QueryInterface(p, a, b)
         #define IDirectDraw7_AddRef(p)                       (p)->lpVtbl->AddRef(p)
         #define IDirectDraw7_Release(p)                      (p)->lpVtbl->Release(p)
         #define IDirectDraw7_Compact(p)                      (p)->lpVtbl->Compact(p)
         #define IDirectDraw7_CreateClipper(p, a, b, c)       (p)->lpVtbl->CreateClipper(p, a, b, c)
         #define IDirectDraw7_CreatePalette(p, a, b, c, d)    (p)->lpVtbl->CreatePalette(p, a, b, c, d)
         #define IDirectDraw7_CreateSurface(p, a, b, c)       (p)->lpVtbl->CreateSurface(p, a, b, c)
         #define IDirectDraw7_DuplicateSurface(p, a, b)       (p)->lpVtbl->DuplicateSurface(p, a, b)
         #define IDirectDraw7_EnumDisplayModes(p, a, b, c, d) (p)->lpVtbl->EnumDisplayModes(p, a, b, c, d)
         #define IDirectDraw7_EnumSurfaces(p, a, b, c, d)     (p)->lpVtbl->EnumSurfaces(p, a, b, c, d)
         #define IDirectDraw7_FlipToGDISurface(p)             (p)->lpVtbl->FlipToGDISurface(p)
         #define IDirectDraw7_GetCaps(p, a, b)                (p)->lpVtbl->GetCaps(p, a, b)
         #define IDirectDraw7_GetDisplayMode(p, a)            (p)->lpVtbl->GetDisplayMode(p, a)
         #define IDirectDraw7_GetFourCCCodes(p, a, b)         (p)->lpVtbl->GetFourCCCodes(p, a, b)
         #define IDirectDraw7_GetGDISurface(p, a)             (p)->lpVtbl->GetGDISurface(p, a)
         #define IDirectDraw7_GetMonitorFrequency(p, a)       (p)->lpVtbl->GetMonitorFrequency(p, a)
         #define IDirectDraw7_GetScanLine(p, a)               (p)->lpVtbl->GetScanLine(p, a)
         #define IDirectDraw7_GetVerticalBlankStatus(p, a)    (p)->lpVtbl->GetVerticalBlankStatus(p, a)
         #define IDirectDraw7_Initialize(p, a)                (p)->lpVtbl->Initialize(p, a)
         #define IDirectDraw7_RestoreDisplayMode(p)           (p)->lpVtbl->RestoreDisplayMode(p)
         #define IDirectDraw7_SetCooperativeLevel(p, a, b)    (p)->lpVtbl->SetCooperativeLevel(p, a, b)
         #define IDirectDraw7_SetDisplayMode(p, a, b, c, d, e) (p)->lpVtbl->SetDisplayMode(p, a, b, c, d, e)
         #define IDirectDraw7_WaitForVerticalBlank(p, a, b)   (p)->lpVtbl->WaitForVerticalBlank(p, a, b)
         #define IDirectDraw7_GetAvailableVidMem(p, a, b, c)  (p)->lpVtbl->GetAvailableVidMem(p, a, b, c)
         #define IDirectDraw7_GetSurfaceFromDC(p, a, b)       (p)->lpVtbl->GetSurfaceFromDC(p, a, b)
         #define IDirectDraw7_RestoreAllSurfaces(p)           (p)->lpVtbl->RestoreAllSurfaces(p)
         #define IDirectDraw7_TestCooperativeLevel(p)         (p)->lpVtbl->TestCooperativeLevel(p)
         #define IDirectDraw7_GetDeviceIdentifier(p,a,b)      (p)->lpVtbl->GetDeviceIdentifier(p,a,b)
         #define IDirectDraw7_StartModeTest(p,a,b,c)        (p)->lpVtbl->StartModeTest(p,a,b,c)
         #define IDirectDraw7_EvaluateMode(p,a,b)           (p)->lpVtbl->EvaluateMode(p,a,b)
     #else
         #define IDirectDraw7_QueryInterface(p, a, b)         (p)->QueryInterface(a, b)
         #define IDirectDraw7_AddRef(p)                       (p)->AddRef()
         #define IDirectDraw7_Release(p)                      (p)->Release()
         #define IDirectDraw7_Compact(p)                      (p)->Compact()
         #define IDirectDraw7_CreateClipper(p, a, b, c)       (p)->CreateClipper(a, b, c)
         #define IDirectDraw7_CreatePalette(p, a, b, c, d)    (p)->CreatePalette(a, b, c, d)
         #define IDirectDraw7_CreateSurface(p, a, b, c)       (p)->CreateSurface(a, b, c)
         #define IDirectDraw7_DuplicateSurface(p, a, b)       (p)->DuplicateSurface(a, b)
         #define IDirectDraw7_EnumDisplayModes(p, a, b, c, d) (p)->EnumDisplayModes(a, b, c, d)
         #define IDirectDraw7_EnumSurfaces(p, a, b, c, d)     (p)->EnumSurfaces(a, b, c, d)
         #define IDirectDraw7_FlipToGDISurface(p)             (p)->FlipToGDISurface()
         #define IDirectDraw7_GetCaps(p, a, b)                (p)->GetCaps(a, b)
         #define IDirectDraw7_GetDisplayMode(p, a)            (p)->GetDisplayMode(a)
         #define IDirectDraw7_GetFourCCCodes(p, a, b)         (p)->GetFourCCCodes(a, b)
         #define IDirectDraw7_GetGDISurface(p, a)             (p)->GetGDISurface(a)
         #define IDirectDraw7_GetMonitorFrequency(p, a)       (p)->GetMonitorFrequency(a)
         #define IDirectDraw7_GetScanLine(p, a)               (p)->GetScanLine(a)
         #define IDirectDraw7_GetVerticalBlankStatus(p, a)    (p)->GetVerticalBlankStatus(a)
         #define IDirectDraw7_Initialize(p, a)                (p)->Initialize(a)
         #define IDirectDraw7_RestoreDisplayMode(p)           (p)->RestoreDisplayMode()
         #define IDirectDraw7_SetCooperativeLevel(p, a, b)    (p)->SetCooperativeLevel(a, b)
         #define IDirectDraw7_SetDisplayMode(p, a, b, c, d, e) (p)->SetDisplayMode(a, b, c, d, e)
         #define IDirectDraw7_WaitForVerticalBlank(p, a, b)   (p)->WaitForVerticalBlank(a, b)
         #define IDirectDraw7_GetAvailableVidMem(p, a, b, c)  (p)->GetAvailableVidMem(a, b, c)
         #define IDirectDraw7_GetSurfaceFromDC(p, a, b)       (p)->GetSurfaceFromDC(a, b)
         #define IDirectDraw7_RestoreAllSurfaces(p)           (p)->RestoreAllSurfaces()
         #define IDirectDraw7_TestCooperativeLevel(p)         (p)->TestCooperativeLevel()
         #define IDirectDraw7_GetDeviceIdentifier(p,a,b)      (p)->GetDeviceIdentifier(a,b)
         #define IDirectDraw7_StartModeTest(p,a,b,c)        (p)->lpVtbl->StartModeTest(a,b,c)
         #define IDirectDraw7_EvaluateMode(p,a,b)           (p)->lpVtbl->EvaluateMode(a,b)
     #endif
#endif

#if defined( _WIN32 ) && !defined( _NO_COM )
    #undef INTERFACE
    #define INTERFACE IDirectDrawPalette

    DECLARE_INTERFACE_( IDirectDrawPalette, IUnknown )
    {
      STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
      STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
      STDMETHOD_(ULONG,Release) (THIS) PURE;
      STDMETHOD(GetCaps)(THIS_ LPDWORD) PURE;
      STDMETHOD(GetEntries)(THIS_ DWORD,DWORD,DWORD,LPPALETTEENTRY) PURE;
      STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, DWORD, LPPALETTEENTRY) PURE;
      STDMETHOD(SetEntries)(THIS_ DWORD,DWORD,DWORD,LPPALETTEENTRY) PURE;
    };
     #if !defined(__cplusplus) || defined(CINTERFACE)
         #define IDirectDrawPalette_QueryInterface(p, a, b)      (p)->lpVtbl->QueryInterface(p, a, b)
         #define IDirectDrawPalette_AddRef(p)                    (p)->lpVtbl->AddRef(p)
         #define IDirectDrawPalette_Release(p)                   (p)->lpVtbl->Release(p)
         #define IDirectDrawPalette_GetCaps(p, a)                (p)->lpVtbl->GetCaps(p, a)
         #define IDirectDrawPalette_GetEntries(p, a, b, c, d)    (p)->lpVtbl->GetEntries(p, a, b, c, d)
         #define IDirectDrawPalette_Initialize(p, a, b, c)       (p)->lpVtbl->Initialize(p, a, b, c)
         #define IDirectDrawPalette_SetEntries(p, a, b, c, d)    (p)->lpVtbl->SetEntries(p, a, b, c, d)
     #else
         #define IDirectDrawPalette_QueryInterface(p, a, b)      (p)->QueryInterface(a, b)
         #define IDirectDrawPalette_AddRef(p)                    (p)->AddRef()
         #define IDirectDrawPalette_Release(p)                   (p)->Release()
         #define IDirectDrawPalette_GetCaps(p, a)                (p)->GetCaps(a)
         #define IDirectDrawPalette_GetEntries(p, a, b, c, d)    (p)->GetEntries(a, b, c, d)
         #define IDirectDrawPalette_Initialize(p, a, b, c)       (p)->Initialize(a, b, c)
         #define IDirectDrawPalette_SetEntries(p, a, b, c, d)    (p)->SetEntries(a, b, c, d)
     #endif
#endif

#if defined( _WIN32 ) && !defined( _NO_COM )
    #undef INTERFACE
    #define INTERFACE IDirectDrawGammaControl

    DECLARE_INTERFACE_( IDirectDrawGammaControl, IUnknown )
    {
      STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
      STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
      STDMETHOD_(ULONG,Release) (THIS) PURE;
      STDMETHOD(GetGammaRamp)(THIS_ DWORD, LPDDGAMMARAMP) PURE;
      STDMETHOD(SetGammaRamp)(THIS_ DWORD, LPDDGAMMARAMP) PURE;
     };
     #if !defined(__cplusplus) || defined(CINTERFACE)
         #define IDirectDrawGammaControl_QueryInterface(p, a, b)  (p)->lpVtbl->QueryInterface(p, a, b)
         #define IDirectDrawGammaControl_AddRef(p)                (p)->lpVtbl->AddRef(p)
         #define IDirectDrawGammaControl_Release(p)               (p)->lpVtbl->Release(p)
         #define IDirectDrawGammaControl_GetGammaRamp(p, a, b)    (p)->lpVtbl->GetGammaRamp(p, a, b)
         #define IDirectDrawGammaControl_SetGammaRamp(p, a, b)    (p)->lpVtbl->SetGammaRamp(p, a, b)
     #else
         #define IDirectDrawGammaControl_QueryInterface(p, a, b)  (p)->QueryInterface(a, b)
         #define IDirectDrawGammaControl_AddRef(p)                (p)->AddRef()
         #define IDirectDrawGammaControl_Release(p)               (p)->Release()
         #define IDirectDrawGammaControl_GetGammaRamp(p, a, b)    (p)->GetGammaRamp(a, b)
         #define IDirectDrawGammaControl_SetGammaRamp(p, a, b)    (p)->SetGammaRamp(a, b)
     #endif
#endif

#if defined( _WIN32 ) && !defined( _NO_COM )
     #undef INTERFACE
     #define INTERFACE IDirectDrawColorControl

     DECLARE_INTERFACE_( IDirectDrawColorControl, IUnknown )
     {
       STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
       STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
       STDMETHOD_(ULONG,Release) (THIS) PURE;
       STDMETHOD(GetColorControls)(THIS_ LPDDCOLORCONTROL) PURE;
       STDMETHOD(SetColorControls)(THIS_ LPDDCOLORCONTROL) PURE;
     };
     #if !defined(__cplusplus) || defined(CINTERFACE)
         #define IDirectDrawColorControl_QueryInterface(p, a, b)  (p)->lpVtbl->QueryInterface(p, a, b)
         #define IDirectDrawColorControl_AddRef(p)                (p)->lpVtbl->AddRef(p)
         #define IDirectDrawColorControl_Release(p)               (p)->lpVtbl->Release(p)
         #define IDirectDrawColorControl_GetColorControls(p, a)   (p)->lpVtbl->GetColorControls(p, a)
         #define IDirectDrawColorControl_SetColorControls(p, a)   (p)->lpVtbl->SetColorControls(p, a)
     #else
         #define IDirectDrawColorControl_QueryInterface(p, a, b)  (p)->QueryInterface(a, b)
         #define IDirectDrawColorControl_AddRef(p)                (p)->AddRef()
         #define IDirectDrawColorControl_Release(p)               (p)->Release()
         #define IDirectDrawColorControl_GetColorControls(p, a)   (p)->GetColorControls(a)
         #define IDirectDrawColorControl_SetColorControls(p, a)   (p)->SetColorControls(a)
     #endif
#endif


#if defined( _WIN32 ) && !defined( _NO_COM )
     #undef INTERFACE
     #define INTERFACE IDirectDrawClipper
     DECLARE_INTERFACE_( IDirectDrawClipper, IUnknown )
     {
       STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
       STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
       STDMETHOD_(ULONG,Release) (THIS) PURE;
       STDMETHOD(GetClipList)(THIS_ LPRECT, LPRGNDATA, LPDWORD) PURE;
       STDMETHOD(GetHWnd)(THIS_ HWND *) PURE;
       STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, DWORD) PURE;
       STDMETHOD(IsClipListChanged)(THIS_ BOOL *) PURE;
       STDMETHOD(SetClipList)(THIS_ LPRGNDATA,DWORD) PURE;
       STDMETHOD(SetHWnd)(THIS_ DWORD, HWND ) PURE;
     };
     #if !defined(__cplusplus) || defined(CINTERFACE)
         #define IDirectDrawClipper_QueryInterface(p, a, b)  (p)->lpVtbl->QueryInterface(p, a, b)
         #define IDirectDrawClipper_AddRef(p)                (p)->lpVtbl->AddRef(p)
         #define IDirectDrawClipper_Release(p)               (p)->lpVtbl->Release(p)
         #define IDirectDrawClipper_GetClipList(p, a, b, c)  (p)->lpVtbl->GetClipList(p, a, b, c)
         #define IDirectDrawClipper_GetHWnd(p, a)            (p)->lpVtbl->GetHWnd(p, a)
         #define IDirectDrawClipper_Initialize(p, a, b)      (p)->lpVtbl->Initialize(p, a, b)
         #define IDirectDrawClipper_IsClipListChanged(p, a)  (p)->lpVtbl->IsClipListChanged(p, a)
         #define IDirectDrawClipper_SetClipList(p, a, b)     (p)->lpVtbl->SetClipList(p, a, b)
         #define IDirectDrawClipper_SetHWnd(p, a, b)         (p)->lpVtbl->SetHWnd(p, a, b)
     #else
         #define IDirectDrawClipper_QueryInterface(p, a, b)  (p)->QueryInterface(a, b)
         #define IDirectDrawClipper_AddRef(p)                (p)->AddRef()
         #define IDirectDrawClipper_Release(p)               (p)->Release()
         #define IDirectDrawClipper_GetClipList(p, a, b, c)  (p)->GetClipList(a, b, c)
         #define IDirectDrawClipper_GetHWnd(p, a)            (p)->GetHWnd(a)
         #define IDirectDrawClipper_Initialize(p, a, b)      (p)->Initialize(a, b)
         #define IDirectDrawClipper_IsClipListChanged(p, a)  (p)->IsClipListChanged(a)
         #define IDirectDrawClipper_SetClipList(p, a, b)     (p)->SetClipList(a, b)
         #define IDirectDrawClipper_SetHWnd(p, a, b)         (p)->SetHWnd(a, b)
     #endif
#endif

#if defined( _WIN32 ) && !defined( _NO_COM )
     #undef INTERFACE
     #define INTERFACE IDirectDrawSurface

     DECLARE_INTERFACE_( IDirectDrawSurface, IUnknown )
     {
       STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
       STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
       STDMETHOD_(ULONG,Release) (THIS) PURE;
       STDMETHOD(AddAttachedSurface)(THIS_ LPDIRECTDRAWSURFACE) PURE;
       STDMETHOD(AddOverlayDirtyRect)(THIS_ LPRECT) PURE;
       STDMETHOD(Blt)(THIS_ LPRECT,LPDIRECTDRAWSURFACE, LPRECT,DWORD, LPDDBLTFX) PURE;
       STDMETHOD(BltBatch)(THIS_ LPDDBLTBATCH, DWORD, DWORD ) PURE;
       STDMETHOD(BltFast)(THIS_ DWORD,DWORD,LPDIRECTDRAWSURFACE, LPRECT,DWORD) PURE;
       STDMETHOD(DeleteAttachedSurface)(THIS_ DWORD,LPDIRECTDRAWSURFACE) PURE;
       STDMETHOD(EnumAttachedSurfaces)(THIS_ LPVOID,LPDDENUMSURFACESCALLBACK) PURE;
       STDMETHOD(EnumOverlayZOrders)(THIS_ DWORD,LPVOID,LPDDENUMSURFACESCALLBACK) PURE;
       STDMETHOD(Flip)(THIS_ LPDIRECTDRAWSURFACE, DWORD) PURE;
       STDMETHOD(GetAttachedSurface)(THIS_ LPDDSCAPS, LPDIRECTDRAWSURFACE *) PURE;
       STDMETHOD(GetBltStatus)(THIS_ DWORD) PURE;
       STDMETHOD(GetCaps)(THIS_ LPDDSCAPS) PURE;
       STDMETHOD(GetClipper)(THIS_ LPDIRECTDRAWCLIPPER *) PURE;
       STDMETHOD(GetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
       STDMETHOD(GetDC)(THIS_ HDC *) PURE;
       STDMETHOD(GetFlipStatus)(THIS_ DWORD) PURE;
       STDMETHOD(GetOverlayPosition)(THIS_ LPLONG, LPLONG ) PURE;
       STDMETHOD(GetPalette)(THIS_ LPDIRECTDRAWPALETTE *) PURE;
       STDMETHOD(GetPixelFormat)(THIS_ LPDDPIXELFORMAT) PURE;
       STDMETHOD(GetSurfaceDesc)(THIS_ LPDDSURFACEDESC) PURE;
       STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, LPDDSURFACEDESC) PURE;
       STDMETHOD(IsLost)(THIS) PURE;
       STDMETHOD(Lock)(THIS_ LPRECT,LPDDSURFACEDESC,DWORD,HANDLE) PURE;
       STDMETHOD(ReleaseDC)(THIS_ HDC) PURE;
       STDMETHOD(Restore)(THIS) PURE;
       STDMETHOD(SetClipper)(THIS_ LPDIRECTDRAWCLIPPER) PURE;
       STDMETHOD(SetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
       STDMETHOD(SetOverlayPosition)(THIS_ LONG, LONG ) PURE;
       STDMETHOD(SetPalette)(THIS_ LPDIRECTDRAWPALETTE) PURE;
       STDMETHOD(Unlock)(THIS_ LPVOID) PURE;
       STDMETHOD(UpdateOverlay)(THIS_ LPRECT, LPDIRECTDRAWSURFACE,LPRECT,DWORD, LPDDOVERLAYFX) PURE;
       STDMETHOD(UpdateOverlayDisplay)(THIS_ DWORD) PURE;
       STDMETHOD(UpdateOverlayZOrder)(THIS_ DWORD, LPDIRECTDRAWSURFACE) PURE;
     };
     #if !defined(__cplusplus) || defined(CINTERFACE)
         #define IDirectDrawSurface_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
         #define IDirectDrawSurface_AddRef(p)                    (p)->lpVtbl->AddRef(p)
         #define IDirectDrawSurface_Release(p)                   (p)->lpVtbl->Release(p)
         #define IDirectDrawSurface_AddAttachedSurface(p,a)      (p)->lpVtbl->AddAttachedSurface(p,a)
         #define IDirectDrawSurface_AddOverlayDirtyRect(p,a)     (p)->lpVtbl->AddOverlayDirtyRect(p,a)
         #define IDirectDrawSurface_Blt(p,a,b,c,d,e)             (p)->lpVtbl->Blt(p,a,b,c,d,e)
         #define IDirectDrawSurface_BltBatch(p,a,b,c)            (p)->lpVtbl->BltBatch(p,a,b,c)
         #define IDirectDrawSurface_BltFast(p,a,b,c,d,e)         (p)->lpVtbl->BltFast(p,a,b,c,d,e)
         #define IDirectDrawSurface_DeleteAttachedSurface(p,a,b) (p)->lpVtbl->DeleteAttachedSurface(p,a,b)
         #define IDirectDrawSurface_EnumAttachedSurfaces(p,a,b)  (p)->lpVtbl->EnumAttachedSurfaces(p,a,b)
         #define IDirectDrawSurface_EnumOverlayZOrders(p,a,b,c)  (p)->lpVtbl->EnumOverlayZOrders(p,a,b,c)
         #define IDirectDrawSurface_Flip(p,a,b)                  (p)->lpVtbl->Flip(p,a,b)
         #define IDirectDrawSurface_GetAttachedSurface(p,a,b)    (p)->lpVtbl->GetAttachedSurface(p,a,b)
         #define IDirectDrawSurface_GetBltStatus(p,a)            (p)->lpVtbl->GetBltStatus(p,a)
         #define IDirectDrawSurface_GetCaps(p,b)                 (p)->lpVtbl->GetCaps(p,b)
         #define IDirectDrawSurface_GetClipper(p,a)              (p)->lpVtbl->GetClipper(p,a)
         #define IDirectDrawSurface_GetColorKey(p,a,b)           (p)->lpVtbl->GetColorKey(p,a,b)
         #define IDirectDrawSurface_GetDC(p,a)                   (p)->lpVtbl->GetDC(p,a)
         #define IDirectDrawSurface_GetFlipStatus(p,a)           (p)->lpVtbl->GetFlipStatus(p,a)
         #define IDirectDrawSurface_GetOverlayPosition(p,a,b)    (p)->lpVtbl->GetOverlayPosition(p,a,b)
         #define IDirectDrawSurface_GetPalette(p,a)              (p)->lpVtbl->GetPalette(p,a)
         #define IDirectDrawSurface_GetPixelFormat(p,a)          (p)->lpVtbl->GetPixelFormat(p,a)
         #define IDirectDrawSurface_GetSurfaceDesc(p,a)          (p)->lpVtbl->GetSurfaceDesc(p,a)
         #define IDirectDrawSurface_Initialize(p,a,b)            (p)->lpVtbl->Initialize(p,a,b)
         #define IDirectDrawSurface_IsLost(p)                    (p)->lpVtbl->IsLost(p)
         #define IDirectDrawSurface_Lock(p,a,b,c,d)              (p)->lpVtbl->Lock(p,a,b,c,d)
         #define IDirectDrawSurface_ReleaseDC(p,a)               (p)->lpVtbl->ReleaseDC(p,a)
         #define IDirectDrawSurface_Restore(p)                   (p)->lpVtbl->Restore(p)
         #define IDirectDrawSurface_SetClipper(p,a)              (p)->lpVtbl->SetClipper(p,a)
         #define IDirectDrawSurface_SetColorKey(p,a,b)           (p)->lpVtbl->SetColorKey(p,a,b)
         #define IDirectDrawSurface_SetOverlayPosition(p,a,b)    (p)->lpVtbl->SetOverlayPosition(p,a,b)
         #define IDirectDrawSurface_SetPalette(p,a)              (p)->lpVtbl->SetPalette(p,a)
         #define IDirectDrawSurface_Unlock(p,b)                  (p)->lpVtbl->Unlock(p,b)
         #define IDirectDrawSurface_UpdateOverlay(p,a,b,c,d,e)   (p)->lpVtbl->UpdateOverlay(p,a,b,c,d,e)
         #define IDirectDrawSurface_UpdateOverlayDisplay(p,a)    (p)->lpVtbl->UpdateOverlayDisplay(p,a)
         #define IDirectDrawSurface_UpdateOverlayZOrder(p,a,b)   (p)->lpVtbl->UpdateOverlayZOrder(p,a,b)
     #else
         #define IDirectDrawSurface_QueryInterface(p,a,b)        (p)->QueryInterface(a,b)
         #define IDirectDrawSurface_AddRef(p)                    (p)->AddRef()
         #define IDirectDrawSurface_Release(p)                   (p)->Release()
         #define IDirectDrawSurface_AddAttachedSurface(p,a)      (p)->AddAttachedSurface(a)
         #define IDirectDrawSurface_AddOverlayDirtyRect(p,a)     (p)->AddOverlayDirtyRect(a)
         #define IDirectDrawSurface_Blt(p,a,b,c,d,e)             (p)->Blt(a,b,c,d,e)
         #define IDirectDrawSurface_BltBatch(p,a,b,c)            (p)->BltBatch(a,b,c)
         #define IDirectDrawSurface_BltFast(p,a,b,c,d,e)         (p)->BltFast(a,b,c,d,e)
         #define IDirectDrawSurface_DeleteAttachedSurface(p,a,b) (p)->DeleteAttachedSurface(a,b)
         #define IDirectDrawSurface_EnumAttachedSurfaces(p,a,b)  (p)->EnumAttachedSurfaces(a,b)
         #define IDirectDrawSurface_EnumOverlayZOrders(p,a,b,c)  (p)->EnumOverlayZOrders(a,b,c)
         #define IDirectDrawSurface_Flip(p,a,b)                  (p)->Flip(a,b)
         #define IDirectDrawSurface_GetAttachedSurface(p,a,b)    (p)->GetAttachedSurface(a,b)
         #define IDirectDrawSurface_GetBltStatus(p,a)            (p)->GetBltStatus(a)
         #define IDirectDrawSurface_GetCaps(p,b)                 (p)->GetCaps(b)
         #define IDirectDrawSurface_GetClipper(p,a)              (p)->GetClipper(a)
         #define IDirectDrawSurface_GetColorKey(p,a,b)           (p)->GetColorKey(a,b)
         #define IDirectDrawSurface_GetDC(p,a)                   (p)->GetDC(a)
         #define IDirectDrawSurface_GetFlipStatus(p,a)           (p)->GetFlipStatus(a)
         #define IDirectDrawSurface_GetOverlayPosition(p,a,b)    (p)->GetOverlayPosition(a,b)
         #define IDirectDrawSurface_GetPalette(p,a)              (p)->GetPalette(a)
         #define IDirectDrawSurface_GetPixelFormat(p,a)          (p)->GetPixelFormat(a)
         #define IDirectDrawSurface_GetSurfaceDesc(p,a)          (p)->GetSurfaceDesc(a)
         #define IDirectDrawSurface_Initialize(p,a,b)            (p)->Initialize(a,b)
         #define IDirectDrawSurface_IsLost(p)                    (p)->IsLost()
         #define IDirectDrawSurface_Lock(p,a,b,c,d)              (p)->Lock(a,b,c,d)
         #define IDirectDrawSurface_ReleaseDC(p,a)               (p)->ReleaseDC(a)
         #define IDirectDrawSurface_Restore(p)                   (p)->Restore()
         #define IDirectDrawSurface_SetClipper(p,a)              (p)->SetClipper(a)
         #define IDirectDrawSurface_SetColorKey(p,a,b)           (p)->SetColorKey(a,b)
         #define IDirectDrawSurface_SetOverlayPosition(p,a,b)    (p)->SetOverlayPosition(a,b)
         #define IDirectDrawSurface_SetPalette(p,a)              (p)->SetPalette(a)
         #define IDirectDrawSurface_Unlock(p,b)                  (p)->Unlock(b)
         #define IDirectDrawSurface_UpdateOverlay(p,a,b,c,d,e)   (p)->UpdateOverlay(a,b,c,d,e)
         #define IDirectDrawSurface_UpdateOverlayDisplay(p,a)    (p)->UpdateOverlayDisplay(a)
         #define IDirectDrawSurface_UpdateOverlayZOrder(p,a,b)   (p)->UpdateOverlayZOrder(a,b)
     #endif
#endif

#if defined( _WIN32 ) && !defined( _NO_COM )
     #undef INTERFACE
     #define INTERFACE IDirectDrawSurface2
     DECLARE_INTERFACE_( IDirectDrawSurface2, IUnknown )
     {
       STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
       STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
       STDMETHOD_(ULONG,Release) (THIS) PURE;
       STDMETHOD(AddAttachedSurface)(THIS_ LPDIRECTDRAWSURFACE2) PURE;
       STDMETHOD(AddOverlayDirtyRect)(THIS_ LPRECT) PURE;
       STDMETHOD(Blt)(THIS_ LPRECT,LPDIRECTDRAWSURFACE2, LPRECT,DWORD, LPDDBLTFX) PURE;
       STDMETHOD(BltBatch)(THIS_ LPDDBLTBATCH, DWORD, DWORD ) PURE;
       STDMETHOD(BltFast)(THIS_ DWORD,DWORD,LPDIRECTDRAWSURFACE2, LPRECT,DWORD) PURE;
       STDMETHOD(DeleteAttachedSurface)(THIS_ DWORD,LPDIRECTDRAWSURFACE2) PURE;
       STDMETHOD(EnumAttachedSurfaces)(THIS_ LPVOID,LPDDENUMSURFACESCALLBACK) PURE;
       STDMETHOD(EnumOverlayZOrders)(THIS_ DWORD,LPVOID,LPDDENUMSURFACESCALLBACK) PURE;
       STDMETHOD(Flip)(THIS_ LPDIRECTDRAWSURFACE2, DWORD) PURE;
       STDMETHOD(GetAttachedSurface)(THIS_ LPDDSCAPS, LPDIRECTDRAWSURFACE2 *) PURE;
       STDMETHOD(GetBltStatus)(THIS_ DWORD) PURE;
       STDMETHOD(GetCaps)(THIS_ LPDDSCAPS) PURE;
       STDMETHOD(GetClipper)(THIS_ LPDIRECTDRAWCLIPPER *) PURE;
       STDMETHOD(GetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
       STDMETHOD(GetDC)(THIS_ HDC *) PURE;
       STDMETHOD(GetFlipStatus)(THIS_ DWORD) PURE;
       STDMETHOD(GetOverlayPosition)(THIS_ LPLONG, LPLONG ) PURE;
       STDMETHOD(GetPalette)(THIS_ LPDIRECTDRAWPALETTE *) PURE;
       STDMETHOD(GetPixelFormat)(THIS_ LPDDPIXELFORMAT) PURE;
       STDMETHOD(GetSurfaceDesc)(THIS_ LPDDSURFACEDESC) PURE;
       STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, LPDDSURFACEDESC) PURE;
       STDMETHOD(IsLost)(THIS) PURE;
       STDMETHOD(Lock)(THIS_ LPRECT,LPDDSURFACEDESC,DWORD,HANDLE) PURE;
       STDMETHOD(ReleaseDC)(THIS_ HDC) PURE;
       STDMETHOD(Restore)(THIS) PURE;
       STDMETHOD(SetClipper)(THIS_ LPDIRECTDRAWCLIPPER) PURE;
       STDMETHOD(SetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
       STDMETHOD(SetOverlayPosition)(THIS_ LONG, LONG ) PURE;
       STDMETHOD(SetPalette)(THIS_ LPDIRECTDRAWPALETTE) PURE;
       STDMETHOD(Unlock)(THIS_ LPVOID) PURE;
       STDMETHOD(UpdateOverlay)(THIS_ LPRECT, LPDIRECTDRAWSURFACE2,LPRECT,DWORD, LPDDOVERLAYFX) PURE;
       STDMETHOD(UpdateOverlayDisplay)(THIS_ DWORD) PURE;
       STDMETHOD(UpdateOverlayZOrder)(THIS_ DWORD, LPDIRECTDRAWSURFACE2) PURE;
       STDMETHOD(GetDDInterface)(THIS_ LPVOID *) PURE;
       STDMETHOD(PageLock)(THIS_ DWORD) PURE;
       STDMETHOD(PageUnlock)(THIS_ DWORD) PURE;
     };

     #if !defined(__cplusplus) || defined(CINTERFACE)
         #define IDirectDrawSurface2_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
         #define IDirectDrawSurface2_AddRef(p)                    (p)->lpVtbl->AddRef(p)
         #define IDirectDrawSurface2_Release(p)                   (p)->lpVtbl->Release(p)
         #define IDirectDrawSurface2_AddAttachedSurface(p,a)      (p)->lpVtbl->AddAttachedSurface(p,a)
         #define IDirectDrawSurface2_AddOverlayDirtyRect(p,a)     (p)->lpVtbl->AddOverlayDirtyRect(p,a)
         #define IDirectDrawSurface2_Blt(p,a,b,c,d,e)             (p)->lpVtbl->Blt(p,a,b,c,d,e)
         #define IDirectDrawSurface2_BltBatch(p,a,b,c)            (p)->lpVtbl->BltBatch(p,a,b,c)
         #define IDirectDrawSurface2_BltFast(p,a,b,c,d,e)         (p)->lpVtbl->BltFast(p,a,b,c,d,e)
         #define IDirectDrawSurface2_DeleteAttachedSurface(p,a,b) (p)->lpVtbl->DeleteAttachedSurface(p,a,b)
         #define IDirectDrawSurface2_EnumAttachedSurfaces(p,a,b)  (p)->lpVtbl->EnumAttachedSurfaces(p,a,b)
         #define IDirectDrawSurface2_EnumOverlayZOrders(p,a,b,c)  (p)->lpVtbl->EnumOverlayZOrders(p,a,b,c)
         #define IDirectDrawSurface2_Flip(p,a,b)                  (p)->lpVtbl->Flip(p,a,b)
         #define IDirectDrawSurface2_GetAttachedSurface(p,a,b)    (p)->lpVtbl->GetAttachedSurface(p,a,b)
         #define IDirectDrawSurface2_GetBltStatus(p,a)            (p)->lpVtbl->GetBltStatus(p,a)
         #define IDirectDrawSurface2_GetCaps(p,b)                 (p)->lpVtbl->GetCaps(p,b)
         #define IDirectDrawSurface2_GetClipper(p,a)              (p)->lpVtbl->GetClipper(p,a)
         #define IDirectDrawSurface2_GetColorKey(p,a,b)           (p)->lpVtbl->GetColorKey(p,a,b)
         #define IDirectDrawSurface2_GetDC(p,a)                   (p)->lpVtbl->GetDC(p,a)
         #define IDirectDrawSurface2_GetFlipStatus(p,a)           (p)->lpVtbl->GetFlipStatus(p,a)
         #define IDirectDrawSurface2_GetOverlayPosition(p,a,b)    (p)->lpVtbl->GetOverlayPosition(p,a,b)
         #define IDirectDrawSurface2_GetPalette(p,a)              (p)->lpVtbl->GetPalette(p,a)
         #define IDirectDrawSurface2_GetPixelFormat(p,a)          (p)->lpVtbl->GetPixelFormat(p,a)
         #define IDirectDrawSurface2_GetSurfaceDesc(p,a)          (p)->lpVtbl->GetSurfaceDesc(p,a)
         #define IDirectDrawSurface2_Initialize(p,a,b)            (p)->lpVtbl->Initialize(p,a,b)
         #define IDirectDrawSurface2_IsLost(p)                    (p)->lpVtbl->IsLost(p)
         #define IDirectDrawSurface2_Lock(p,a,b,c,d)              (p)->lpVtbl->Lock(p,a,b,c,d)
         #define IDirectDrawSurface2_ReleaseDC(p,a)               (p)->lpVtbl->ReleaseDC(p,a)
         #define IDirectDrawSurface2_Restore(p)                   (p)->lpVtbl->Restore(p)
         #define IDirectDrawSurface2_SetClipper(p,a)              (p)->lpVtbl->SetClipper(p,a)
         #define IDirectDrawSurface2_SetColorKey(p,a,b)           (p)->lpVtbl->SetColorKey(p,a,b)
         #define IDirectDrawSurface2_SetOverlayPosition(p,a,b)    (p)->lpVtbl->SetOverlayPosition(p,a,b)
         #define IDirectDrawSurface2_SetPalette(p,a)              (p)->lpVtbl->SetPalette(p,a)
         #define IDirectDrawSurface2_Unlock(p,b)                  (p)->lpVtbl->Unlock(p,b)
         #define IDirectDrawSurface2_UpdateOverlay(p,a,b,c,d,e)   (p)->lpVtbl->UpdateOverlay(p,a,b,c,d,e)
         #define IDirectDrawSurface2_UpdateOverlayDisplay(p,a)    (p)->lpVtbl->UpdateOverlayDisplay(p,a)
         #define IDirectDrawSurface2_UpdateOverlayZOrder(p,a,b)   (p)->lpVtbl->UpdateOverlayZOrder(p,a,b)
         #define IDirectDrawSurface2_GetDDInterface(p,a)          (p)->lpVtbl->GetDDInterface(p,a)
         #define IDirectDrawSurface2_PageLock(p,a)                (p)->lpVtbl->PageLock(p,a)
         #define IDirectDrawSurface2_PageUnlock(p,a)              (p)->lpVtbl->PageUnlock(p,a)
     #else
         #define IDirectDrawSurface2_QueryInterface(p,a,b)        (p)->QueryInterface(a,b)
         #define IDirectDrawSurface2_AddRef(p)                    (p)->AddRef()
         #define IDirectDrawSurface2_Release(p)                   (p)->Release()
         #define IDirectDrawSurface2_AddAttachedSurface(p,a)      (p)->AddAttachedSurface(a)
         #define IDirectDrawSurface2_AddOverlayDirtyRect(p,a)     (p)->AddOverlayDirtyRect(a)
         #define IDirectDrawSurface2_Blt(p,a,b,c,d,e)             (p)->Blt(a,b,c,d,e)
         #define IDirectDrawSurface2_BltBatch(p,a,b,c)            (p)->BltBatch(a,b,c)
         #define IDirectDrawSurface2_BltFast(p,a,b,c,d,e)         (p)->BltFast(a,b,c,d,e)
         #define IDirectDrawSurface2_DeleteAttachedSurface(p,a,b) (p)->DeleteAttachedSurface(a,b)
         #define IDirectDrawSurface2_EnumAttachedSurfaces(p,a,b)  (p)->EnumAttachedSurfaces(a,b)
         #define IDirectDrawSurface2_EnumOverlayZOrders(p,a,b,c)  (p)->EnumOverlayZOrders(a,b,c)
         #define IDirectDrawSurface2_Flip(p,a,b)                  (p)->Flip(a,b)
         #define IDirectDrawSurface2_GetAttachedSurface(p,a,b)    (p)->GetAttachedSurface(a,b)
         #define IDirectDrawSurface2_GetBltStatus(p,a)            (p)->GetBltStatus(a)
         #define IDirectDrawSurface2_GetCaps(p,b)                 (p)->GetCaps(b)
         #define IDirectDrawSurface2_GetClipper(p,a)              (p)->GetClipper(a)
         #define IDirectDrawSurface2_GetColorKey(p,a,b)           (p)->GetColorKey(a,b)
         #define IDirectDrawSurface2_GetDC(p,a)                   (p)->GetDC(a)
         #define IDirectDrawSurface2_GetFlipStatus(p,a)           (p)->GetFlipStatus(a)
         #define IDirectDrawSurface2_GetOverlayPosition(p,a,b)    (p)->GetOverlayPosition(a,b)
         #define IDirectDrawSurface2_GetPalette(p,a)              (p)->GetPalette(a)
         #define IDirectDrawSurface2_GetPixelFormat(p,a)          (p)->GetPixelFormat(a)
         #define IDirectDrawSurface2_GetSurfaceDesc(p,a)          (p)->GetSurfaceDesc(a)
         #define IDirectDrawSurface2_Initialize(p,a,b)            (p)->Initialize(a,b)
         #define IDirectDrawSurface2_IsLost(p)                    (p)->IsLost()
         #define IDirectDrawSurface2_Lock(p,a,b,c,d)              (p)->Lock(a,b,c,d)
         #define IDirectDrawSurface2_ReleaseDC(p,a)               (p)->ReleaseDC(a)
         #define IDirectDrawSurface2_Restore(p)                   (p)->Restore()
         #define IDirectDrawSurface2_SetClipper(p,a)              (p)->SetClipper(a)
         #define IDirectDrawSurface2_SetColorKey(p,a,b)           (p)->SetColorKey(a,b)
         #define IDirectDrawSurface2_SetOverlayPosition(p,a,b)    (p)->SetOverlayPosition(a,b)
         #define IDirectDrawSurface2_SetPalette(p,a)              (p)->SetPalette(a)
         #define IDirectDrawSurface2_Unlock(p,b)                  (p)->Unlock(b)
         #define IDirectDrawSurface2_UpdateOverlay(p,a,b,c,d,e)   (p)->UpdateOverlay(a,b,c,d,e)
         #define IDirectDrawSurface2_UpdateOverlayDisplay(p,a)    (p)->UpdateOverlayDisplay(a)
         #define IDirectDrawSurface2_UpdateOverlayZOrder(p,a,b)   (p)->UpdateOverlayZOrder(a,b)
         #define IDirectDrawSurface2_GetDDInterface(p,a)          (p)->GetDDInterface(a)
         #define IDirectDrawSurface2_PageLock(p,a)                (p)->PageLock(a)
         #define IDirectDrawSurface2_PageUnlock(p,a)              (p)->PageUnlock(a)
     #endif
#endif

#if defined( _WIN32 ) && !defined( _NO_COM )
#undef INTERFACE
#define INTERFACE IDirectDrawSurface3
DECLARE_INTERFACE_( IDirectDrawSurface3, IUnknown )
{
       STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
       STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
       STDMETHOD_(ULONG,Release) (THIS) PURE;
       STDMETHOD(AddAttachedSurface)(THIS_ LPDIRECTDRAWSURFACE3) PURE;
       STDMETHOD(AddOverlayDirtyRect)(THIS_ LPRECT) PURE;
       STDMETHOD(Blt)(THIS_ LPRECT,LPDIRECTDRAWSURFACE3, LPRECT,DWORD, LPDDBLTFX) PURE;
       STDMETHOD(BltBatch)(THIS_ LPDDBLTBATCH, DWORD, DWORD ) PURE;
       STDMETHOD(BltFast)(THIS_ DWORD,DWORD,LPDIRECTDRAWSURFACE3, LPRECT,DWORD) PURE;
       STDMETHOD(DeleteAttachedSurface)(THIS_ DWORD,LPDIRECTDRAWSURFACE3) PURE;
       STDMETHOD(EnumAttachedSurfaces)(THIS_ LPVOID,LPDDENUMSURFACESCALLBACK) PURE;
       STDMETHOD(EnumOverlayZOrders)(THIS_ DWORD,LPVOID,LPDDENUMSURFACESCALLBACK) PURE;
       STDMETHOD(Flip)(THIS_ LPDIRECTDRAWSURFACE3, DWORD) PURE;
       STDMETHOD(GetAttachedSurface)(THIS_ LPDDSCAPS, LPDIRECTDRAWSURFACE3 *) PURE;
       STDMETHOD(GetBltStatus)(THIS_ DWORD) PURE;
       STDMETHOD(GetCaps)(THIS_ LPDDSCAPS) PURE;
       STDMETHOD(GetClipper)(THIS_ LPDIRECTDRAWCLIPPER *) PURE;
       STDMETHOD(GetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
       STDMETHOD(GetDC)(THIS_ HDC *) PURE;
       STDMETHOD(GetFlipStatus)(THIS_ DWORD) PURE;
       STDMETHOD(GetOverlayPosition)(THIS_ LPLONG, LPLONG ) PURE;
       STDMETHOD(GetPalette)(THIS_ LPDIRECTDRAWPALETTE *) PURE;
       STDMETHOD(GetPixelFormat)(THIS_ LPDDPIXELFORMAT) PURE;
       STDMETHOD(GetSurfaceDesc)(THIS_ LPDDSURFACEDESC) PURE;
       STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, LPDDSURFACEDESC) PURE;
       STDMETHOD(IsLost)(THIS) PURE;
       STDMETHOD(Lock)(THIS_ LPRECT,LPDDSURFACEDESC,DWORD,HANDLE) PURE;
       STDMETHOD(ReleaseDC)(THIS_ HDC) PURE;
       STDMETHOD(Restore)(THIS) PURE;
       STDMETHOD(SetClipper)(THIS_ LPDIRECTDRAWCLIPPER) PURE;
       STDMETHOD(SetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
       STDMETHOD(SetOverlayPosition)(THIS_ LONG, LONG ) PURE;
       STDMETHOD(SetPalette)(THIS_ LPDIRECTDRAWPALETTE) PURE;
       STDMETHOD(Unlock)(THIS_ LPVOID) PURE;
       STDMETHOD(UpdateOverlay)(THIS_ LPRECT, LPDIRECTDRAWSURFACE3,LPRECT,DWORD, LPDDOVERLAYFX) PURE;
       STDMETHOD(UpdateOverlayDisplay)(THIS_ DWORD) PURE;
       STDMETHOD(UpdateOverlayZOrder)(THIS_ DWORD, LPDIRECTDRAWSURFACE3) PURE;
       STDMETHOD(GetDDInterface)(THIS_ LPVOID *) PURE;
       STDMETHOD(PageLock)(THIS_ DWORD) PURE;
       STDMETHOD(PageUnlock)(THIS_ DWORD) PURE;
       STDMETHOD(SetSurfaceDesc)(THIS_ LPDDSURFACEDESC, DWORD) PURE;
     };

     #if !defined(__cplusplus) || defined(CINTERFACE)
         #define IDirectDrawSurface3_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
         #define IDirectDrawSurface3_AddRef(p)                    (p)->lpVtbl->AddRef(p)
         #define IDirectDrawSurface3_Release(p)                   (p)->lpVtbl->Release(p)
         #define IDirectDrawSurface3_AddAttachedSurface(p,a)      (p)->lpVtbl->AddAttachedSurface(p,a)
         #define IDirectDrawSurface3_AddOverlayDirtyRect(p,a)     (p)->lpVtbl->AddOverlayDirtyRect(p,a)
         #define IDirectDrawSurface3_Blt(p,a,b,c,d,e)             (p)->lpVtbl->Blt(p,a,b,c,d,e)
         #define IDirectDrawSurface3_BltBatch(p,a,b,c)            (p)->lpVtbl->BltBatch(p,a,b,c)
         #define IDirectDrawSurface3_BltFast(p,a,b,c,d,e)         (p)->lpVtbl->BltFast(p,a,b,c,d,e)
         #define IDirectDrawSurface3_DeleteAttachedSurface(p,a,b) (p)->lpVtbl->DeleteAttachedSurface(p,a,b)
         #define IDirectDrawSurface3_EnumAttachedSurfaces(p,a,b)  (p)->lpVtbl->EnumAttachedSurfaces(p,a,b)
         #define IDirectDrawSurface3_EnumOverlayZOrders(p,a,b,c)  (p)->lpVtbl->EnumOverlayZOrders(p,a,b,c)
         #define IDirectDrawSurface3_Flip(p,a,b)                  (p)->lpVtbl->Flip(p,a,b)
         #define IDirectDrawSurface3_GetAttachedSurface(p,a,b)    (p)->lpVtbl->GetAttachedSurface(p,a,b)
         #define IDirectDrawSurface3_GetBltStatus(p,a)            (p)->lpVtbl->GetBltStatus(p,a)
         #define IDirectDrawSurface3_GetCaps(p,b)                 (p)->lpVtbl->GetCaps(p,b)
         #define IDirectDrawSurface3_GetClipper(p,a)              (p)->lpVtbl->GetClipper(p,a)
         #define IDirectDrawSurface3_GetColorKey(p,a,b)           (p)->lpVtbl->GetColorKey(p,a,b)
         #define IDirectDrawSurface3_GetDC(p,a)                   (p)->lpVtbl->GetDC(p,a)
         #define IDirectDrawSurface3_GetFlipStatus(p,a)           (p)->lpVtbl->GetFlipStatus(p,a)
         #define IDirectDrawSurface3_GetOverlayPosition(p,a,b)    (p)->lpVtbl->GetOverlayPosition(p,a,b)
         #define IDirectDrawSurface3_GetPalette(p,a)              (p)->lpVtbl->GetPalette(p,a)
         #define IDirectDrawSurface3_GetPixelFormat(p,a)          (p)->lpVtbl->GetPixelFormat(p,a)
         #define IDirectDrawSurface3_GetSurfaceDesc(p,a)          (p)->lpVtbl->GetSurfaceDesc(p,a)
         #define IDirectDrawSurface3_Initialize(p,a,b)            (p)->lpVtbl->Initialize(p,a,b)
         #define IDirectDrawSurface3_IsLost(p)                    (p)->lpVtbl->IsLost(p)
         #define IDirectDrawSurface3_Lock(p,a,b,c,d)              (p)->lpVtbl->Lock(p,a,b,c,d)
         #define IDirectDrawSurface3_ReleaseDC(p,a)               (p)->lpVtbl->ReleaseDC(p,a)
         #define IDirectDrawSurface3_Restore(p)                   (p)->lpVtbl->Restore(p)
         #define IDirectDrawSurface3_SetClipper(p,a)              (p)->lpVtbl->SetClipper(p,a)
         #define IDirectDrawSurface3_SetColorKey(p,a,b)           (p)->lpVtbl->SetColorKey(p,a,b)
         #define IDirectDrawSurface3_SetOverlayPosition(p,a,b)    (p)->lpVtbl->SetOverlayPosition(p,a,b)
         #define IDirectDrawSurface3_SetPalette(p,a)              (p)->lpVtbl->SetPalette(p,a)
         #define IDirectDrawSurface3_Unlock(p,b)                  (p)->lpVtbl->Unlock(p,b)
         #define IDirectDrawSurface3_UpdateOverlay(p,a,b,c,d,e)   (p)->lpVtbl->UpdateOverlay(p,a,b,c,d,e)
         #define IDirectDrawSurface3_UpdateOverlayDisplay(p,a)    (p)->lpVtbl->UpdateOverlayDisplay(p,a)
         #define IDirectDrawSurface3_UpdateOverlayZOrder(p,a,b)   (p)->lpVtbl->UpdateOverlayZOrder(p,a,b)
         #define IDirectDrawSurface3_GetDDInterface(p,a)          (p)->lpVtbl->GetDDInterface(p,a)
         #define IDirectDrawSurface3_PageLock(p,a)                (p)->lpVtbl->PageLock(p,a)
         #define IDirectDrawSurface3_PageUnlock(p,a)              (p)->lpVtbl->PageUnlock(p,a)
         #define IDirectDrawSurface3_SetSurfaceDesc(p,a,b)        (p)->lpVtbl->SetSurfaceDesc(p,a,b)
     #else
         #define IDirectDrawSurface3_QueryInterface(p,a,b)        (p)->QueryInterface(a,b)
         #define IDirectDrawSurface3_AddRef(p)                    (p)->AddRef()
         #define IDirectDrawSurface3_Release(p)                   (p)->Release()
         #define IDirectDrawSurface3_AddAttachedSurface(p,a)      (p)->AddAttachedSurface(a)
         #define IDirectDrawSurface3_AddOverlayDirtyRect(p,a)     (p)->AddOverlayDirtyRect(a)
         #define IDirectDrawSurface3_Blt(p,a,b,c,d,e)             (p)->Blt(a,b,c,d,e)
         #define IDirectDrawSurface3_BltBatch(p,a,b,c)            (p)->BltBatch(a,b,c)
         #define IDirectDrawSurface3_BltFast(p,a,b,c,d,e)         (p)->BltFast(a,b,c,d,e)
         #define IDirectDrawSurface3_DeleteAttachedSurface(p,a,b) (p)->DeleteAttachedSurface(a,b)
         #define IDirectDrawSurface3_EnumAttachedSurfaces(p,a,b)  (p)->EnumAttachedSurfaces(a,b)
         #define IDirectDrawSurface3_EnumOverlayZOrders(p,a,b,c)  (p)->EnumOverlayZOrders(a,b,c)
         #define IDirectDrawSurface3_Flip(p,a,b)                  (p)->Flip(a,b)
         #define IDirectDrawSurface3_GetAttachedSurface(p,a,b)    (p)->GetAttachedSurface(a,b)
         #define IDirectDrawSurface3_GetBltStatus(p,a)            (p)->GetBltStatus(a)
         #define IDirectDrawSurface3_GetCaps(p,b)                 (p)->GetCaps(b)
         #define IDirectDrawSurface3_GetClipper(p,a)              (p)->GetClipper(a)
         #define IDirectDrawSurface3_GetColorKey(p,a,b)           (p)->GetColorKey(a,b)
         #define IDirectDrawSurface3_GetDC(p,a)                   (p)->GetDC(a)
         #define IDirectDrawSurface3_GetFlipStatus(p,a)           (p)->GetFlipStatus(a)
         #define IDirectDrawSurface3_GetOverlayPosition(p,a,b)    (p)->GetOverlayPosition(a,b)
         #define IDirectDrawSurface3_GetPalette(p,a)              (p)->GetPalette(a)
         #define IDirectDrawSurface3_GetPixelFormat(p,a)          (p)->GetPixelFormat(a)
         #define IDirectDrawSurface3_GetSurfaceDesc(p,a)          (p)->GetSurfaceDesc(a)
         #define IDirectDrawSurface3_Initialize(p,a,b)            (p)->Initialize(a,b)
         #define IDirectDrawSurface3_IsLost(p)                    (p)->IsLost()
         #define IDirectDrawSurface3_Lock(p,a,b,c,d)              (p)->Lock(a,b,c,d)
         #define IDirectDrawSurface3_ReleaseDC(p,a)               (p)->ReleaseDC(a)
         #define IDirectDrawSurface3_Restore(p)                   (p)->Restore()
         #define IDirectDrawSurface3_SetClipper(p,a)              (p)->SetClipper(a)
         #define IDirectDrawSurface3_SetColorKey(p,a,b)           (p)->SetColorKey(a,b)
         #define IDirectDrawSurface3_SetOverlayPosition(p,a,b)    (p)->SetOverlayPosition(a,b)
         #define IDirectDrawSurface3_SetPalette(p,a)              (p)->SetPalette(a)
         #define IDirectDrawSurface3_Unlock(p,b)                  (p)->Unlock(b)
         #define IDirectDrawSurface3_UpdateOverlay(p,a,b,c,d,e)   (p)->UpdateOverlay(a,b,c,d,e)
         #define IDirectDrawSurface3_UpdateOverlayDisplay(p,a)    (p)->UpdateOverlayDisplay(a)
         #define IDirectDrawSurface3_UpdateOverlayZOrder(p,a,b)   (p)->UpdateOverlayZOrder(a,b)
         #define IDirectDrawSurface3_GetDDInterface(p,a)          (p)->GetDDInterface(a)
         #define IDirectDrawSurface3_PageLock(p,a)                (p)->PageLock(a)
         #define IDirectDrawSurface3_PageUnlock(p,a)              (p)->PageUnlock(a)
         #define IDirectDrawSurface3_SetSurfaceDesc(p,a,b)        (p)->SetSurfaceDesc(a,b)
     #endif
#endif

#if defined( _WIN32 ) && !defined( _NO_COM )
     #undef INTERFACE
     #define INTERFACE IDirectDrawSurface4

     DECLARE_INTERFACE_( IDirectDrawSurface4, IUnknown )
     {
       STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
       STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
       STDMETHOD_(ULONG,Release) (THIS) PURE;
       STDMETHOD(AddAttachedSurface)(THIS_ LPDIRECTDRAWSURFACE4) PURE;
       STDMETHOD(AddOverlayDirtyRect)(THIS_ LPRECT) PURE;
       STDMETHOD(Blt)(THIS_ LPRECT,LPDIRECTDRAWSURFACE4, LPRECT,DWORD, LPDDBLTFX) PURE;
       STDMETHOD(BltBatch)(THIS_ LPDDBLTBATCH, DWORD, DWORD ) PURE;
       STDMETHOD(BltFast)(THIS_ DWORD,DWORD,LPDIRECTDRAWSURFACE4, LPRECT,DWORD) PURE;
       STDMETHOD(DeleteAttachedSurface)(THIS_ DWORD,LPDIRECTDRAWSURFACE4) PURE;
       STDMETHOD(EnumAttachedSurfaces)(THIS_ LPVOID,LPDDENUMSURFACESCALLBACK2) PURE;
       STDMETHOD(EnumOverlayZOrders)(THIS_ DWORD,LPVOID,LPDDENUMSURFACESCALLBACK2) PURE;
       STDMETHOD(Flip)(THIS_ LPDIRECTDRAWSURFACE4, DWORD) PURE;
       STDMETHOD(GetAttachedSurface)(THIS_ LPDDSCAPS2, LPDIRECTDRAWSURFACE4 *) PURE;
       STDMETHOD(GetBltStatus)(THIS_ DWORD) PURE;
       STDMETHOD(GetCaps)(THIS_ LPDDSCAPS2) PURE;
       STDMETHOD(GetClipper)(THIS_ LPDIRECTDRAWCLIPPER *) PURE;
       STDMETHOD(GetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
       STDMETHOD(GetDC)(THIS_ HDC *) PURE;
       STDMETHOD(GetFlipStatus)(THIS_ DWORD) PURE;
       STDMETHOD(GetOverlayPosition)(THIS_ LPLONG, LPLONG ) PURE;
       STDMETHOD(GetPalette)(THIS_ LPDIRECTDRAWPALETTE *) PURE;
       STDMETHOD(GetPixelFormat)(THIS_ LPDDPIXELFORMAT) PURE;
       STDMETHOD(GetSurfaceDesc)(THIS_ LPDDSURFACEDESC2) PURE;
       STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, LPDDSURFACEDESC2) PURE;
       STDMETHOD(IsLost)(THIS) PURE;
       STDMETHOD(Lock)(THIS_ LPRECT,LPDDSURFACEDESC2,DWORD,HANDLE) PURE;
       STDMETHOD(ReleaseDC)(THIS_ HDC) PURE;
       STDMETHOD(Restore)(THIS) PURE;
       STDMETHOD(SetClipper)(THIS_ LPDIRECTDRAWCLIPPER) PURE;
       STDMETHOD(SetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
       STDMETHOD(SetOverlayPosition)(THIS_ LONG, LONG ) PURE;
       STDMETHOD(SetPalette)(THIS_ LPDIRECTDRAWPALETTE) PURE;
       STDMETHOD(Unlock)(THIS_ LPRECT) PURE;
       STDMETHOD(UpdateOverlay)(THIS_ LPRECT, LPDIRECTDRAWSURFACE4,LPRECT,DWORD, LPDDOVERLAYFX) PURE;
       STDMETHOD(UpdateOverlayDisplay)(THIS_ DWORD) PURE;
       STDMETHOD(UpdateOverlayZOrder)(THIS_ DWORD, LPDIRECTDRAWSURFACE4) PURE;
       STDMETHOD(GetDDInterface)(THIS_ LPVOID *) PURE;
       STDMETHOD(PageLock)(THIS_ DWORD) PURE;
       STDMETHOD(PageUnlock)(THIS_ DWORD) PURE;
       STDMETHOD(SetSurfaceDesc)(THIS_ LPDDSURFACEDESC2, DWORD) PURE;
       STDMETHOD(SetPrivateData)(THIS_ REFGUID, LPVOID, DWORD, DWORD) PURE;
       STDMETHOD(GetPrivateData)(THIS_ REFGUID, LPVOID, LPDWORD) PURE;
       STDMETHOD(FreePrivateData)(THIS_ REFGUID) PURE;
       STDMETHOD(GetUniquenessValue)(THIS_ LPDWORD) PURE;
       STDMETHOD(ChangeUniquenessValue)(THIS) PURE;
     };
     #if !defined(__cplusplus) || defined(CINTERFACE)
         #define IDirectDrawSurface4_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
         #define IDirectDrawSurface4_AddRef(p)                    (p)->lpVtbl->AddRef(p)
         #define IDirectDrawSurface4_Release(p)                   (p)->lpVtbl->Release(p)
         #define IDirectDrawSurface4_AddAttachedSurface(p,a)      (p)->lpVtbl->AddAttachedSurface(p,a)
         #define IDirectDrawSurface4_AddOverlayDirtyRect(p,a)     (p)->lpVtbl->AddOverlayDirtyRect(p,a)
         #define IDirectDrawSurface4_Blt(p,a,b,c,d,e)             (p)->lpVtbl->Blt(p,a,b,c,d,e)
         #define IDirectDrawSurface4_BltBatch(p,a,b,c)            (p)->lpVtbl->BltBatch(p,a,b,c)
         #define IDirectDrawSurface4_BltFast(p,a,b,c,d,e)         (p)->lpVtbl->BltFast(p,a,b,c,d,e)
         #define IDirectDrawSurface4_DeleteAttachedSurface(p,a,b) (p)->lpVtbl->DeleteAttachedSurface(p,a,b)
         #define IDirectDrawSurface4_EnumAttachedSurfaces(p,a,b)  (p)->lpVtbl->EnumAttachedSurfaces(p,a,b)
         #define IDirectDrawSurface4_EnumOverlayZOrders(p,a,b,c)  (p)->lpVtbl->EnumOverlayZOrders(p,a,b,c)
         #define IDirectDrawSurface4_Flip(p,a,b)                  (p)->lpVtbl->Flip(p,a,b)
         #define IDirectDrawSurface4_GetAttachedSurface(p,a,b)    (p)->lpVtbl->GetAttachedSurface(p,a,b)
         #define IDirectDrawSurface4_GetBltStatus(p,a)            (p)->lpVtbl->GetBltStatus(p,a)
         #define IDirectDrawSurface4_GetCaps(p,b)                 (p)->lpVtbl->GetCaps(p,b)
         #define IDirectDrawSurface4_GetClipper(p,a)              (p)->lpVtbl->GetClipper(p,a)
         #define IDirectDrawSurface4_GetColorKey(p,a,b)           (p)->lpVtbl->GetColorKey(p,a,b)
         #define IDirectDrawSurface4_GetDC(p,a)                   (p)->lpVtbl->GetDC(p,a)
         #define IDirectDrawSurface4_GetFlipStatus(p,a)           (p)->lpVtbl->GetFlipStatus(p,a)
         #define IDirectDrawSurface4_GetOverlayPosition(p,a,b)    (p)->lpVtbl->GetOverlayPosition(p,a,b)
         #define IDirectDrawSurface4_GetPalette(p,a)              (p)->lpVtbl->GetPalette(p,a)
         #define IDirectDrawSurface4_GetPixelFormat(p,a)          (p)->lpVtbl->GetPixelFormat(p,a)
         #define IDirectDrawSurface4_GetSurfaceDesc(p,a)          (p)->lpVtbl->GetSurfaceDesc(p,a)
         #define IDirectDrawSurface4_Initialize(p,a,b)            (p)->lpVtbl->Initialize(p,a,b)
         #define IDirectDrawSurface4_IsLost(p)                    (p)->lpVtbl->IsLost(p)
         #define IDirectDrawSurface4_Lock(p,a,b,c,d)              (p)->lpVtbl->Lock(p,a,b,c,d)
         #define IDirectDrawSurface4_ReleaseDC(p,a)               (p)->lpVtbl->ReleaseDC(p,a)
         #define IDirectDrawSurface4_Restore(p)                   (p)->lpVtbl->Restore(p)
         #define IDirectDrawSurface4_SetClipper(p,a)              (p)->lpVtbl->SetClipper(p,a)
         #define IDirectDrawSurface4_SetColorKey(p,a,b)           (p)->lpVtbl->SetColorKey(p,a,b)
         #define IDirectDrawSurface4_SetOverlayPosition(p,a,b)    (p)->lpVtbl->SetOverlayPosition(p,a,b)
         #define IDirectDrawSurface4_SetPalette(p,a)              (p)->lpVtbl->SetPalette(p,a)
         #define IDirectDrawSurface4_Unlock(p,b)                  (p)->lpVtbl->Unlock(p,b)
         #define IDirectDrawSurface4_UpdateOverlay(p,a,b,c,d,e)   (p)->lpVtbl->UpdateOverlay(p,a,b,c,d,e)
         #define IDirectDrawSurface4_UpdateOverlayDisplay(p,a)    (p)->lpVtbl->UpdateOverlayDisplay(p,a)
         #define IDirectDrawSurface4_UpdateOverlayZOrder(p,a,b)   (p)->lpVtbl->UpdateOverlayZOrder(p,a,b)
         #define IDirectDrawSurface4_GetDDInterface(p,a)          (p)->lpVtbl->GetDDInterface(p,a)
         #define IDirectDrawSurface4_PageLock(p,a)                (p)->lpVtbl->PageLock(p,a)
         #define IDirectDrawSurface4_PageUnlock(p,a)              (p)->lpVtbl->PageUnlock(p,a)
         #define IDirectDrawSurface4_SetSurfaceDesc(p,a,b)        (p)->lpVtbl->SetSurfaceDesc(p,a,b)
         #define IDirectDrawSurface4_SetPrivateData(p,a,b,c,d)    (p)->lpVtbl->SetPrivateData(p,a,b,c,d)
         #define IDirectDrawSurface4_GetPrivateData(p,a,b,c)      (p)->lpVtbl->GetPrivateData(p,a,b,c)
         #define IDirectDrawSurface4_FreePrivateData(p,a)         (p)->lpVtbl->FreePrivateData(p,a)
         #define IDirectDrawSurface4_GetUniquenessValue(p, a)     (p)->lpVtbl->GetUniquenessValue(p, a)
         #define IDirectDrawSurface4_ChangeUniquenessValue(p)     (p)->lpVtbl->ChangeUniquenessValue(p)
     #else
         #define IDirectDrawSurface4_QueryInterface(p,a,b)        (p)->QueryInterface(a,b)
         #define IDirectDrawSurface4_AddRef(p)                    (p)->AddRef()
         #define IDirectDrawSurface4_Release(p)                   (p)->Release()
         #define IDirectDrawSurface4_AddAttachedSurface(p,a)      (p)->AddAttachedSurface(a)
         #define IDirectDrawSurface4_AddOverlayDirtyRect(p,a)     (p)->AddOverlayDirtyRect(a)
         #define IDirectDrawSurface4_Blt(p,a,b,c,d,e)             (p)->Blt(a,b,c,d,e)
         #define IDirectDrawSurface4_BltBatch(p,a,b,c)            (p)->BltBatch(a,b,c)
         #define IDirectDrawSurface4_BltFast(p,a,b,c,d,e)         (p)->BltFast(a,b,c,d,e)
         #define IDirectDrawSurface4_DeleteAttachedSurface(p,a,b) (p)->DeleteAttachedSurface(a,b)
         #define IDirectDrawSurface4_EnumAttachedSurfaces(p,a,b)  (p)->EnumAttachedSurfaces(a,b)
         #define IDirectDrawSurface4_EnumOverlayZOrders(p,a,b,c)  (p)->EnumOverlayZOrders(a,b,c)
         #define IDirectDrawSurface4_Flip(p,a,b)                  (p)->Flip(a,b)
         #define IDirectDrawSurface4_GetAttachedSurface(p,a,b)    (p)->GetAttachedSurface(a,b)
         #define IDirectDrawSurface4_GetBltStatus(p,a)            (p)->GetBltStatus(a)
         #define IDirectDrawSurface4_GetCaps(p,b)                 (p)->GetCaps(b)
         #define IDirectDrawSurface4_GetClipper(p,a)              (p)->GetClipper(a)
         #define IDirectDrawSurface4_GetColorKey(p,a,b)           (p)->GetColorKey(a,b)
         #define IDirectDrawSurface4_GetDC(p,a)                   (p)->GetDC(a)
         #define IDirectDrawSurface4_GetFlipStatus(p,a)           (p)->GetFlipStatus(a)
         #define IDirectDrawSurface4_GetOverlayPosition(p,a,b)    (p)->GetOverlayPosition(a,b)
         #define IDirectDrawSurface4_GetPalette(p,a)              (p)->GetPalette(a)
         #define IDirectDrawSurface4_GetPixelFormat(p,a)          (p)->GetPixelFormat(a)
         #define IDirectDrawSurface4_GetSurfaceDesc(p,a)          (p)->GetSurfaceDesc(a)
         #define IDirectDrawSurface4_Initialize(p,a,b)            (p)->Initialize(a,b)
         #define IDirectDrawSurface4_IsLost(p)                    (p)->IsLost()
         #define IDirectDrawSurface4_Lock(p,a,b,c,d)              (p)->Lock(a,b,c,d)
         #define IDirectDrawSurface4_ReleaseDC(p,a)               (p)->ReleaseDC(a)
         #define IDirectDrawSurface4_Restore(p)                   (p)->Restore()
         #define IDirectDrawSurface4_SetClipper(p,a)              (p)->SetClipper(a)
         #define IDirectDrawSurface4_SetColorKey(p,a,b)           (p)->SetColorKey(a,b)
         #define IDirectDrawSurface4_SetOverlayPosition(p,a,b)    (p)->SetOverlayPosition(a,b)
         #define IDirectDrawSurface4_SetPalette(p,a)              (p)->SetPalette(a)
         #define IDirectDrawSurface4_Unlock(p,b)                  (p)->Unlock(b)
         #define IDirectDrawSurface4_UpdateOverlay(p,a,b,c,d,e)   (p)->UpdateOverlay(a,b,c,d,e)
         #define IDirectDrawSurface4_UpdateOverlayDisplay(p,a)    (p)->UpdateOverlayDisplay(a)
         #define IDirectDrawSurface4_UpdateOverlayZOrder(p,a,b)   (p)->UpdateOverlayZOrder(a,b)
         #define IDirectDrawSurface4_GetDDInterface(p,a)          (p)->GetDDInterface(a)
         #define IDirectDrawSurface4_PageLock(p,a)                (p)->PageLock(a)
         #define IDirectDrawSurface4_PageUnlock(p,a)              (p)->PageUnlock(a)
         #define IDirectDrawSurface4_SetSurfaceDesc(p,a,b)        (p)->SetSurfaceDesc(a,b)
         #define IDirectDrawSurface4_SetPrivateData(p,a,b,c,d)    (p)->SetPrivateData(a,b,c,d)
         #define IDirectDrawSurface4_GetPrivateData(p,a,b,c)      (p)->GetPrivateData(a,b,c)
         #define IDirectDrawSurface4_FreePrivateData(p,a)         (p)->FreePrivateData(a)
         #define IDirectDrawSurface4_GetUniquenessValue(p, a)     (p)->GetUniquenessValue(a)
         #define IDirectDrawSurface4_ChangeUniquenessValue(p)     (p)->ChangeUniquenessValue()
     #endif
#endif

#if defined( _WIN32 ) && !defined( _NO_COM )
     #undef INTERFACE
     #define INTERFACE IDirectDrawSurface7

     DECLARE_INTERFACE_( IDirectDrawSurface7, IUnknown )
     {
       STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
       STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
       STDMETHOD_(ULONG,Release) (THIS) PURE;
       STDMETHOD(AddAttachedSurface)(THIS_ LPDIRECTDRAWSURFACE7) PURE;
       STDMETHOD(AddOverlayDirtyRect)(THIS_ LPRECT) PURE;
       STDMETHOD(Blt)(THIS_ LPRECT,LPDIRECTDRAWSURFACE7, LPRECT,DWORD, LPDDBLTFX) PURE;
       STDMETHOD(BltBatch)(THIS_ LPDDBLTBATCH, DWORD, DWORD ) PURE;
       STDMETHOD(BltFast)(THIS_ DWORD,DWORD,LPDIRECTDRAWSURFACE7, LPRECT,DWORD) PURE;
       STDMETHOD(DeleteAttachedSurface)(THIS_ DWORD,LPDIRECTDRAWSURFACE7) PURE;
       STDMETHOD(EnumAttachedSurfaces)(THIS_ LPVOID,LPDDENUMSURFACESCALLBACK7) PURE;
       STDMETHOD(EnumOverlayZOrders)(THIS_ DWORD,LPVOID,LPDDENUMSURFACESCALLBACK7) PURE;
       STDMETHOD(Flip)(THIS_ LPDIRECTDRAWSURFACE7, DWORD) PURE;
       STDMETHOD(GetAttachedSurface)(THIS_ LPDDSCAPS2, LPDIRECTDRAWSURFACE7 *) PURE;
       STDMETHOD(GetBltStatus)(THIS_ DWORD) PURE;
       STDMETHOD(GetCaps)(THIS_ LPDDSCAPS2) PURE;
       STDMETHOD(GetClipper)(THIS_ LPDIRECTDRAWCLIPPER *) PURE;
       STDMETHOD(GetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
       STDMETHOD(GetDC)(THIS_ HDC *) PURE;
       STDMETHOD(GetFlipStatus)(THIS_ DWORD) PURE;
       STDMETHOD(GetOverlayPosition)(THIS_ LPLONG, LPLONG ) PURE;
       STDMETHOD(GetPalette)(THIS_ LPDIRECTDRAWPALETTE *) PURE;
       STDMETHOD(GetPixelFormat)(THIS_ LPDDPIXELFORMAT) PURE;
       STDMETHOD(GetSurfaceDesc)(THIS_ LPDDSURFACEDESC2) PURE;
       STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, LPDDSURFACEDESC2) PURE;
       STDMETHOD(IsLost)(THIS) PURE;
       STDMETHOD(Lock)(THIS_ LPRECT,LPDDSURFACEDESC2,DWORD,HANDLE) PURE;
       STDMETHOD(ReleaseDC)(THIS_ HDC) PURE;
       STDMETHOD(Restore)(THIS) PURE;
       STDMETHOD(SetClipper)(THIS_ LPDIRECTDRAWCLIPPER) PURE;
       STDMETHOD(SetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
       STDMETHOD(SetOverlayPosition)(THIS_ LONG, LONG ) PURE;
       STDMETHOD(SetPalette)(THIS_ LPDIRECTDRAWPALETTE) PURE;
       STDMETHOD(Unlock)(THIS_ LPRECT) PURE;
       STDMETHOD(UpdateOverlay)(THIS_ LPRECT, LPDIRECTDRAWSURFACE7,LPRECT,DWORD, LPDDOVERLAYFX) PURE;
       STDMETHOD(UpdateOverlayDisplay)(THIS_ DWORD) PURE;
       STDMETHOD(UpdateOverlayZOrder)(THIS_ DWORD, LPDIRECTDRAWSURFACE7) PURE;
       STDMETHOD(GetDDInterface)(THIS_ LPVOID *) PURE;
       STDMETHOD(PageLock)(THIS_ DWORD) PURE;
       STDMETHOD(PageUnlock)(THIS_ DWORD) PURE;
       STDMETHOD(SetSurfaceDesc)(THIS_ LPDDSURFACEDESC2, DWORD) PURE;
       STDMETHOD(SetPrivateData)(THIS_ REFGUID, LPVOID, DWORD, DWORD) PURE;
       STDMETHOD(GetPrivateData)(THIS_ REFGUID, LPVOID, LPDWORD) PURE;
       STDMETHOD(FreePrivateData)(THIS_ REFGUID) PURE;
       STDMETHOD(GetUniquenessValue)(THIS_ LPDWORD) PURE;
       STDMETHOD(ChangeUniquenessValue)(THIS) PURE;
       STDMETHOD(SetPriority)(THIS_ DWORD) PURE;
       STDMETHOD(GetPriority)(THIS_ LPDWORD) PURE;
       STDMETHOD(SetLOD)(THIS_ DWORD) PURE;
       STDMETHOD(GetLOD)(THIS_ LPDWORD) PURE;
     };
     #if !defined(__cplusplus) || defined(CINTERFACE)
         #define IDirectDrawSurface7_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
         #define IDirectDrawSurface7_AddRef(p)                    (p)->lpVtbl->AddRef(p)
         #define IDirectDrawSurface7_Release(p)                   (p)->lpVtbl->Release(p)
         #define IDirectDrawSurface7_AddAttachedSurface(p,a)      (p)->lpVtbl->AddAttachedSurface(p,a)
         #define IDirectDrawSurface7_AddOverlayDirtyRect(p,a)     (p)->lpVtbl->AddOverlayDirtyRect(p,a)
         #define IDirectDrawSurface7_Blt(p,a,b,c,d,e)             (p)->lpVtbl->Blt(p,a,b,c,d,e)
         #define IDirectDrawSurface7_BltBatch(p,a,b,c)            (p)->lpVtbl->BltBatch(p,a,b,c)
         #define IDirectDrawSurface7_BltFast(p,a,b,c,d,e)         (p)->lpVtbl->BltFast(p,a,b,c,d,e)
         #define IDirectDrawSurface7_DeleteAttachedSurface(p,a,b) (p)->lpVtbl->DeleteAttachedSurface(p,a,b)
         #define IDirectDrawSurface7_EnumAttachedSurfaces(p,a,b)  (p)->lpVtbl->EnumAttachedSurfaces(p,a,b)
         #define IDirectDrawSurface7_EnumOverlayZOrders(p,a,b,c)  (p)->lpVtbl->EnumOverlayZOrders(p,a,b,c)
         #define IDirectDrawSurface7_Flip(p,a,b)                  (p)->lpVtbl->Flip(p,a,b)
         #define IDirectDrawSurface7_GetAttachedSurface(p,a,b)    (p)->lpVtbl->GetAttachedSurface(p,a,b)
         #define IDirectDrawSurface7_GetBltStatus(p,a)            (p)->lpVtbl->GetBltStatus(p,a)
         #define IDirectDrawSurface7_GetCaps(p,b)                 (p)->lpVtbl->GetCaps(p,b)
         #define IDirectDrawSurface7_GetClipper(p,a)              (p)->lpVtbl->GetClipper(p,a)
         #define IDirectDrawSurface7_GetColorKey(p,a,b)           (p)->lpVtbl->GetColorKey(p,a,b)
         #define IDirectDrawSurface7_GetDC(p,a)                   (p)->lpVtbl->GetDC(p,a)
         #define IDirectDrawSurface7_GetFlipStatus(p,a)           (p)->lpVtbl->GetFlipStatus(p,a)
         #define IDirectDrawSurface7_GetOverlayPosition(p,a,b)    (p)->lpVtbl->GetOverlayPosition(p,a,b)
         #define IDirectDrawSurface7_GetPalette(p,a)              (p)->lpVtbl->GetPalette(p,a)
         #define IDirectDrawSurface7_GetPixelFormat(p,a)          (p)->lpVtbl->GetPixelFormat(p,a)
         #define IDirectDrawSurface7_GetSurfaceDesc(p,a)          (p)->lpVtbl->GetSurfaceDesc(p,a)
         #define IDirectDrawSurface7_Initialize(p,a,b)            (p)->lpVtbl->Initialize(p,a,b)
         #define IDirectDrawSurface7_IsLost(p)                    (p)->lpVtbl->IsLost(p)
         #define IDirectDrawSurface7_Lock(p,a,b,c,d)              (p)->lpVtbl->Lock(p,a,b,c,d)
         #define IDirectDrawSurface7_ReleaseDC(p,a)               (p)->lpVtbl->ReleaseDC(p,a)
         #define IDirectDrawSurface7_Restore(p)                   (p)->lpVtbl->Restore(p)
         #define IDirectDrawSurface7_SetClipper(p,a)              (p)->lpVtbl->SetClipper(p,a)
         #define IDirectDrawSurface7_SetColorKey(p,a,b)           (p)->lpVtbl->SetColorKey(p,a,b)
         #define IDirectDrawSurface7_SetOverlayPosition(p,a,b)    (p)->lpVtbl->SetOverlayPosition(p,a,b)
         #define IDirectDrawSurface7_SetPalette(p,a)              (p)->lpVtbl->SetPalette(p,a)
         #define IDirectDrawSurface7_Unlock(p,b)                  (p)->lpVtbl->Unlock(p,b)
         #define IDirectDrawSurface7_UpdateOverlay(p,a,b,c,d,e)   (p)->lpVtbl->UpdateOverlay(p,a,b,c,d,e)
         #define IDirectDrawSurface7_UpdateOverlayDisplay(p,a)    (p)->lpVtbl->UpdateOverlayDisplay(p,a)
         #define IDirectDrawSurface7_UpdateOverlayZOrder(p,a,b)   (p)->lpVtbl->UpdateOverlayZOrder(p,a,b)
         #define IDirectDrawSurface7_GetDDInterface(p,a)          (p)->lpVtbl->GetDDInterface(p,a)
         #define IDirectDrawSurface7_PageLock(p,a)                (p)->lpVtbl->PageLock(p,a)
         #define IDirectDrawSurface7_PageUnlock(p,a)              (p)->lpVtbl->PageUnlock(p,a)
         #define IDirectDrawSurface7_SetSurfaceDesc(p,a,b)        (p)->lpVtbl->SetSurfaceDesc(p,a,b)
         #define IDirectDrawSurface7_SetPrivateData(p,a,b,c,d)    (p)->lpVtbl->SetPrivateData(p,a,b,c,d)
         #define IDirectDrawSurface7_GetPrivateData(p,a,b,c)      (p)->lpVtbl->GetPrivateData(p,a,b,c)
         #define IDirectDrawSurface7_FreePrivateData(p,a)         (p)->lpVtbl->FreePrivateData(p,a)
         #define IDirectDrawSurface7_GetUniquenessValue(p, a)     (p)->lpVtbl->GetUniquenessValue(p, a)
         #define IDirectDrawSurface7_ChangeUniquenessValue(p)     (p)->lpVtbl->ChangeUniquenessValue(p)
         #define IDirectDrawSurface7_SetPriority(p,a)             (p)->lpVtbl->SetPriority(p,a)
         #define IDirectDrawSurface7_GetPriority(p,a)             (p)->lpVtbl->GetPriority(p,a)
         #define IDirectDrawSurface7_SetLOD(p,a)                  (p)->lpVtbl->SetLOD(p,a)
         #define IDirectDrawSurface7_GetLOD(p,a)                  (p)->lpVtbl->GetLOD(p,a)
     #else
         #define IDirectDrawSurface7_QueryInterface(p,a,b)        (p)->QueryInterface(a,b)
         #define IDirectDrawSurface7_AddRef(p)                    (p)->AddRef()
         #define IDirectDrawSurface7_Release(p)                   (p)->Release()
         #define IDirectDrawSurface7_AddAttachedSurface(p,a)      (p)->AddAttachedSurface(a)
         #define IDirectDrawSurface7_AddOverlayDirtyRect(p,a)     (p)->AddOverlayDirtyRect(a)
         #define IDirectDrawSurface7_Blt(p,a,b,c,d,e)             (p)->Blt(a,b,c,d,e)
         #define IDirectDrawSurface7_BltBatch(p,a,b,c)            (p)->BltBatch(a,b,c)
         #define IDirectDrawSurface7_BltFast(p,a,b,c,d,e)         (p)->BltFast(a,b,c,d,e)
         #define IDirectDrawSurface7_DeleteAttachedSurface(p,a,b) (p)->DeleteAttachedSurface(a,b)
         #define IDirectDrawSurface7_EnumAttachedSurfaces(p,a,b)  (p)->EnumAttachedSurfaces(a,b)
         #define IDirectDrawSurface7_EnumOverlayZOrders(p,a,b,c)  (p)->EnumOverlayZOrders(a,b,c)
         #define IDirectDrawSurface7_Flip(p,a,b)                  (p)->Flip(a,b)
         #define IDirectDrawSurface7_GetAttachedSurface(p,a,b)    (p)->GetAttachedSurface(a,b)
         #define IDirectDrawSurface7_GetBltStatus(p,a)            (p)->GetBltStatus(a)
         #define IDirectDrawSurface7_GetCaps(p,b)                 (p)->GetCaps(b)
         #define IDirectDrawSurface7_GetClipper(p,a)              (p)->GetClipper(a)
         #define IDirectDrawSurface7_GetColorKey(p,a,b)           (p)->GetColorKey(a,b)
         #define IDirectDrawSurface7_GetDC(p,a)                   (p)->GetDC(a)
         #define IDirectDrawSurface7_GetFlipStatus(p,a)           (p)->GetFlipStatus(a)
         #define IDirectDrawSurface7_GetOverlayPosition(p,a,b)    (p)->GetOverlayPosition(a,b)
         #define IDirectDrawSurface7_GetPalette(p,a)              (p)->GetPalette(a)
         #define IDirectDrawSurface7_GetPixelFormat(p,a)          (p)->GetPixelFormat(a)
         #define IDirectDrawSurface7_GetSurfaceDesc(p,a)          (p)->GetSurfaceDesc(a)
         #define IDirectDrawSurface7_Initialize(p,a,b)            (p)->Initialize(a,b)
         #define IDirectDrawSurface7_IsLost(p)                    (p)->IsLost()
         #define IDirectDrawSurface7_Lock(p,a,b,c,d)              (p)->Lock(a,b,c,d)
         #define IDirectDrawSurface7_ReleaseDC(p,a)               (p)->ReleaseDC(a)
         #define IDirectDrawSurface7_Restore(p)                   (p)->Restore()
         #define IDirectDrawSurface7_SetClipper(p,a)              (p)->SetClipper(a)
         #define IDirectDrawSurface7_SetColorKey(p,a,b)           (p)->SetColorKey(a,b)
         #define IDirectDrawSurface7_SetOverlayPosition(p,a,b)    (p)->SetOverlayPosition(a,b)
         #define IDirectDrawSurface7_SetPalette(p,a)              (p)->SetPalette(a)
         #define IDirectDrawSurface7_Unlock(p,b)                  (p)->Unlock(b)
         #define IDirectDrawSurface7_UpdateOverlay(p,a,b,c,d,e)   (p)->UpdateOverlay(a,b,c,d,e)
         #define IDirectDrawSurface7_UpdateOverlayDisplay(p,a)    (p)->UpdateOverlayDisplay(a)
         #define IDirectDrawSurface7_UpdateOverlayZOrder(p,a,b)   (p)->UpdateOverlayZOrder(a,b)
         #define IDirectDrawSurface7_GetDDInterface(p,a)          (p)->GetDDInterface(a)
         #define IDirectDrawSurface7_PageLock(p,a)                (p)->PageLock(a)
         #define IDirectDrawSurface7_PageUnlock(p,a)              (p)->PageUnlock(a)
         #define IDirectDrawSurface7_SetSurfaceDesc(p,a,b)        (p)->SetSurfaceDesc(a,b)
         #define IDirectDrawSurface7_SetPrivateData(p,a,b,c,d)    (p)->SetPrivateData(a,b,c,d)
         #define IDirectDrawSurface7_GetPrivateData(p,a,b,c)      (p)->GetPrivateData(a,b,c)
         #define IDirectDrawSurface7_FreePrivateData(p,a)         (p)->FreePrivateData(a)
         #define IDirectDrawSurface7_GetUniquenessValue(p, a)     (p)->GetUniquenessValue(a)
         #define IDirectDrawSurface7_ChangeUniquenessValue(p)     (p)->ChangeUniquenessValue()
         #define IDirectDrawSurface7_SetPriority(p,a)             (p)->SetPriority(a)
         #define IDirectDrawSurface7_GetPriority(p,a)             (p)->GetPriority(a)
         #define IDirectDrawSurface7_SetLOD(p,a)                  (p)->SetLOD(a)
         #define IDirectDrawSurface7_GetLOD(p,a)                  (p)->GetLOD(a)
     #endif
#endif
#undef INTERFACE

#ifdef __cplusplus
}
#endif

#ifdef ENABLE_NAMELESS_UNION_PRAGMA
  #pragma warning(default:4201)
#endif

#endif
