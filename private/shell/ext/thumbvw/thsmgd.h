/* sample source code for IE4 view extension
 * 
 * Copyright Microsoft 1996
 *
 * The Guid that represents the view extension server..
 */

#ifndef _THSMGD_H
#define _THSMGD_H

#define VID_Thumbnails  CLSID_ThumbnailViewExt

// html thumbnail extractor
// {EAB841A0-9550-11cf-8C16-00805F1408F3}
DEFINE_GUID( CLSID_HtmlThumbnailExtractor, 
    0xeab841a0, 0x9550, 0x11cf, 0x8c, 0x16, 0x0, 0x80, 0x5f, 0x14, 0x8, 0xf3);
#define CLSIDSTR_HtmlThumbnailExtractor     "{EAB841A0-9550-11cf-8C16-00805F1408F3}"

// BMP thumbnail extractor
// {D76FDCA0-592A-11d0-B7FD-00C04FD706EC}
DEFINE_GUID( CLSID_BmpThumbnailExtractor, 0xd76fdca0, 0x592a, 0x11d0, 0xb7, 0xfd, 0x0,
    0xc0, 0x4f, 0xd7, 0x6, 0xec);
#define CLSIDSTR_BmpThumbnailExtractor      "{D76FDCA0-592A-11d0-B7FD-00C04FD706EC}"

// Office graphics filters thumbnail extractor .....
// {1AEB1360-5AFC-11d0-B806-00C04FD706EC}
DEFINE_GUID( CLSID_OfficeGrfxFilterThumbnailExtractor, 0x1aeb1360, 0x5afc, 0x11d0, 0xb8, 0x6,
    0x0, 0xc0, 0x4f, 0xd7, 0x6, 0xec);
#define CLSIDSTR_OfficeGrfxFilterExtractor  "{1AEB1360-5AFC-11d0-B806-00C04FD706EC}"

// handles the thumbnail in FMTID_SummaryInfo property set on docfiles...
// {9DBD2C50-62AD-11d0-B806-00C04FD706EC}
DEFINE_GUID( CLSID_DocfileThumbnailHandler, 0x9dbd2c50, 0x62ad, 0x11d0, 0xb8, 0x6, 0x0, 0xc0,
    0x4f, 0xd7, 0x6, 0xec);
#define CLSIDSTR_DocfileThumbnailHandler    "{9DBD2C50-62AD-11d0-B806-00C04FD706EC}"

// delegates both IExtractThumbnail and ICustomThumbnailExtractor for lnk files...
// {500202A0-731E-11d0-B829-00C04FD706EC}
DEFINE_GUID(CLSID_LnkThumbnailDelegator, 0x500202a0, 0x731e, 0x11d0, 0xb8, 0x29,
    0x0, 0xc0, 0x4f, 0xd7, 0x6, 0xec);
#define CLSIDSTR_LnkThumbnailDelegator      "{500202A0-731E-11d0-B829-00C04FD706EC}"

#define IIDSTR_IExtractImage                "{BB2E617C-0920-11d1-9A0B-00C04FC2D6C1}"

DEFINE_GUID(IID_IThumbnailMaker, 0x7aaa28d2, 0x3bf2, 0x11cf, 0xb6, 0xe6, 0x0, 0xaa, 0x0, 0xbb,
    0xba, 0x9e);

// {7376D660-C583-11d0-A3A5-00C04FD706EC}
DEFINE_GUID(CLSID_ImgCtxThumbnailExtractor, 0x7376d660, 0xc583, 0x11d0, 0xa3, 0xa5, 0x0, 0xc0, 0x4f, 0xd7, 0x6, 0xec);

#define CLSIDSTR_ImgCtxThumbnailExtractor      "{7376D660-C583-11d0-A3A5-00C04FD706EC}"

// the Media Manager Thumbnail Property set FormatID
// {4A839CC0-F8FF-11ce-A06B-00AA00A71191}
DEFINE_GUID( FMTID_CmsThumbnailPropertySet, 0x4a839cc0, 0xf8ff, 0x11ce, 
			 0xa0, 0x6b, 0x0, 0xaa, 0x0, 0xa7, 0x11, 0x91 );

// {2D09F2E0-6846-11d0-B811-00C04FD706EC}
DEFINE_GUID( TOID_DiskCacheTask, 0x2d09f2e0, 0x6846, 0x11d0, 0xb8, 0x11, 
    0x0, 0xc0, 0x4f, 0xd7, 0x6, 0xec);

// {1728D630-69E3-11d0-B815-00C04FD706EC}
DEFINE_GUID( TOID_ImgCacheTidyup, 0x1728d630, 0x69e3, 0x11d0, 0xb8, 0x15,
    0x0, 0xc0, 0x4f, 0xd7, 0x6, 0xec);

// {78212180-BF15-11d0-A3A5-00C04FD706EC}
DEFINE_GUID(TOID_ExtractImageTask, 0x78212180, 0xbf15, 0x11d0, 0xa3, 0xa5, 0x0,
    0xc0, 0x4f, 0xd7, 0x6, 0xec);

// {6DFD582C-92E3-11d1-98A3-00C04FB687DA}
DEFINE_GUID(TOID_DiskCacheCleanup, 
0x6dfd582c, 0x92e3, 0x11d1, 0x98, 0xa3, 0x0, 0xc0, 0x4f, 0xb6, 0x87, 0xda);

// {EFC5437C-9847-11d1-98A4-00C04FB687DA}
DEFINE_GUID(TOID_UpdateDirHandler, 
0xefc5437c, 0x9847, 0x11d1, 0x98, 0xa4, 0x0, 0xc0, 0x4f, 0xb6, 0x87, 0xda);

// {7EC9321A-0E09-11d2-81F8-00C04FB687DA}
DEFINE_GUID(TOID_CheckCacheTask, 
0x7ec9321a, 0xe09, 0x11d2, 0x81, 0xf8, 0x0, 0xc0, 0x4f, 0xb6, 0x87, 0xda);

#endif

