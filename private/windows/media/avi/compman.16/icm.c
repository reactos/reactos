//////////////////////////////////////////////////////////////////////////////o
//
//  ICM.C
//
//      Helper routines for compressing/decompressing/and choosing compressors.
//
//      (C) Copyright Microsoft Corp. 1991, 1992, 1993.  All rights reserved.
//
//      You have a royalty-free right to use, modify, reproduce and
//      distribute the Sample Files (and/or any modified version) in
//      any way you find useful, provided that you agree that
//      Microsoft has no warranty obligations or liability for any
//      Sample Application Files.
//
//      If you did not get this from Microsoft Sources, then it may not be the
//      most current version.  This sample code in particular will be updated
//      and include more documentation.
//
//      Sources are:
//         CompuServe: WINSDK forum, MDK section.
//         Anonymous FTP from ftp.uu.net vendor\microsoft\multimedia
//
///////////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include <windowsx.h>
#include <win32.h>
#include <mmsystem.h>

#ifdef DEBUG
    static void CDECL dprintf(LPSTR, ...);
    #define DPF(x)  dprintf x
#else
    #define DPF(x)
#endif


//
// define these before compman.h, so are functions get declared right.
//
#ifndef WIN32
#define VFWAPI  FAR PASCAL _loadds
#define VFWAPIV FAR CDECL  _loadds
#endif

#include <vfw.h>
#include "icm.rc"

#define AVIStreamGetFrameOpen   XAVIStreamGetFrameOpen
#define AVIStreamGetFrame   XAVIStreamGetFrame
#define AVIStreamGetFrameClose  XAVIStreamGetFrameClose

HMODULE havifile;
PGETFRAME (STDAPICALLTYPE  *XAVIStreamGetFrameOpen)(PAVISTREAM pavi,
                     LPBITMAPINFOHEADER lpbiWanted);
LPVOID (STDAPICALLTYPE  *XAVIStreamGetFrame)(PGETFRAME pgf, LONG pos);
HRESULT (STDAPICALLTYPE  *XAVIStreamGetFrameClose)(PGETFRAME pgf);

#ifdef WIN32
extern HANDLE ghInst;
#else
extern HINSTANCE ghInst;
#endif

///////////////////////////////////////////////////////////////////////////////
//  DIB Macros
///////////////////////////////////////////////////////////////////////////////

#define WIDTHBYTES(i)           ((unsigned)((i+31)&(~31))/8)  /* ULONG aligned ! */
#define DibWidthBytes(lpbi)     (UINT)WIDTHBYTES((UINT)(lpbi)->biWidth * (UINT)((lpbi)->biBitCount))

#define DibSizeImage(lpbi)      ((DWORD)(UINT)DibWidthBytes(lpbi) * (DWORD)(UINT)((lpbi)->biHeight))
#define DibSize(lpbi)           ((lpbi)->biSize + (lpbi)->biSizeImage + (int)(lpbi)->biClrUsed * sizeof(RGBQUAD))

#define DibPtr(lpbi)            (LPVOID)(DibColors(lpbi) + (UINT)(lpbi)->biClrUsed)
#define DibColors(lpbi)         ((LPRGBQUAD)((LPBYTE)(lpbi) + (int)(lpbi)->biSize))

#define DibNumColors(lpbi)      ((lpbi)->biClrUsed == 0 && (lpbi)->biBitCount <= 8 \
                                    ? (int)(1 << (int)(lpbi)->biBitCount)          \
                                    : (int)(lpbi)->biClrUsed)

// !!! Someday write this so you don't have to call ICCompressorChoose if you
// !!! know what you want.  Choose would then call this.
// InitCompress(pc, hic/fccHandler, lQuality, lKey, lpbiIn, lpbiOut)


/*****************************************************************************
 * @doc EXTERNAL COMPVARS ICAPPS
 *
 * @types COMPVARS | This structure describes
 *         compressor when using functions such as <f ICCompressorChoose>,
 *  <f ICSeqCompressFrame>, or <f ICCompressorFree>.
 *
 * @field LONG | cbSize | Set this to the size of this structure in bytes.
 *        This member must be set to validate the structure
 *        before calling any function using this structure.
 *
 * @field DWORD | dwFlags | Specifies the flags for this structure:
 *
 *   @flag ICMF_COMPVARS_VALID | Indicates this structure has valid data.
 *    Set this flag if you fill out this structure manually before
 *    calling any functions. Do not set this flag if you let
 *         <f ICCompressorChoose> initialize this structure.
 *
 * @field HIC | hic | Specifies the handle of the compressor to use.
 *  The <f ICCompressorChoose> function opens the chosen compressor and
 * returns the handle to the compressor in this
 *  member. The compressor is closed by <t ICCompressorFree>.
 *
 * @field DWORD | fccType | Specifies the type of compressor being used.
 *        Currently only ICTYPE_VIDEO is supported. This can be set to zero.
 *
 * @field DWORD | fccHandler | Specifies the four-character code
 *       of the compressor. NULL indicates the data is not
 *       to be recompressed and and 'DIB ' indicates the data is full framed
 *       (uncompressed). You can use this member to specify which
 *        compressor is selected by default when the dialog box is
 *       displayed.
 *
 * @field LPBITMAPINFO | lpbiIn | Specifies the input format. Used internally.
 *
 * @field LPBITMAPINFO | lpbiOut | Specifies the output format. Ths member
 *        is set by <f ICCompressorChoose>. The <f ICSeqCompressFrameStart>
 *        function uses this member to determine the compressed output format.
 *        If you do not want to use the default format, specify
 *        the preferred one.
 *
 * @field LPVOID | lpBitsOut | Used internally for compression.
 *
 * @field LPVOID | lpBitsPrev | Used internally for temporal compression.
 *
 * @field LONG | lFrame | Used internally to count the number of frames
 *  compressed in a sequence.
 *
 * @field LONG | lKey | Set by <f ICCompressorChoose> to indicate the key frame
 *  rate selected in the dialog box.  The also specifies the rate that
 *  <f ICSeqCompressFrameStart> uses for making key frames.
 *
 * @field LONG | lDataRate | Set by <f ICCompressorChoose> to indicate the
 *  data rate selected in the dialog box. The units are kilobytes per second.
 *
 * @field LONG | lQ | Set by <f ICCompressChoose> to indicate the quality
 *  selected in the dialog box.  This also specifies the quality
 *  <f ICSeqCompressFrameStart> will use. ICQUALITY_DEFAULT specifies
 *  default quality.
 *
 * @field LONG | lKeyCount | Used internally to count key frames.
 *
 * @field LPVOID | lpState | Set by <f ICCompressorChoose> to the state selected
 *  in the configuration dialog box for the compressor. The system
 * uses this information to restore the state of the dialog box if
 * it is redisplayed. Used internally.
 *
 * @field LONG | cbState | Used internally for the size of the state information.
 *
 ***************************************************************************/
/*******************************************************************
* @doc EXTERNAL ICCompressorFree ICAPPS
*
* @api void | ICCompressorFree | This function frees the resources
*   in the <t COMPVARS> structure used by other IC functions.
*
* @parm PCOMPVARS | pc | Specifies a pointer to the <t COMPVARS>
*       structure containing the resources to be freed.
*
* @comm After using the <f ICCompressorChoose>, <f ICSeqCompressFrameStart>,
*       <f ICSeqCompressFrame>, and <f ICSeqCompressFrameEnd> functions, call
*       this function to release the resources in the <t COMPVARS> structure.
*
* @xref <f ICCompressChoose> <f ICSeqCompressFrameStart> <f ICSeqCompressFrame>
*   <f ICSeqCompressFrameEnd>
*
*******************************************************************/
///////////////////////////////////////////////////////////////////////////////
//
//  ICCompressorFree
//
///////////////////////////////////////////////////////////////////////////////
void VFWAPI ICCompressorFree(PCOMPVARS pc)
{
    /* We were passed an invalid COMPPARMS */
    if (pc == NULL || pc->cbSize != sizeof(COMPVARS))
        return;

    // This function frees every god-damn thing in the structure (excuse my
    // french).

    /* Close the compressor */
    if (pc->hic) {
    ICClose(pc->hic);
    pc->hic = NULL;
    }

    /* Free the output format */
    if (pc->lpbiOut) {
    GlobalFreePtr(pc->lpbiOut);
    pc->lpbiOut = NULL;
    }

    /* Free the buffer for compressed image */
    if (pc->lpBitsOut) {
    GlobalFreePtr(pc->lpBitsOut);
    pc->lpBitsOut = NULL;
    }

    /* Free the buffer for the decompressed previous frame */
    if (pc->lpBitsPrev) {
    GlobalFreePtr(pc->lpBitsPrev);
    pc->lpBitsPrev = NULL;
    }

    /* Free the compressor state buffer */
    if (pc->lpState) {
    GlobalFreePtr(pc->lpState);
    pc->lpState = NULL;
    }

    /* This structure is no longer VALID */
    pc->dwFlags = 0;
}

/*******************************************************************
* @doc EXTERNAL ICSeqCompressFrameStart ICAPPS
*
* @api BOOL | ICSeqCompressFrameStart | This function initializes the system
*   prior to using <f ICSeqCompressFrame>.
*
* @parm PCOMPVARS | pc | Specifies a pointer to a <t COMPVARS> structure
*       initialized with information for compression.
*
* @parm LPBITMAPINFO | lpbiIn | Specifies the format of the data to be
*       compressed.
*
* @rdesc Returns TRUE if successful; otherwise it returns FALSE.
*
* @comm Prior to using this function, use <f ICCompressorChoose> to let the
*       user specify a compressor, or initialize a <t COMPVARS> structure
*       manually. Use <f ICSeqCompressFrameStart>, <f ICSeqCompressFrame>
*       and <f ICSeqCompressFrameEnd> to compress a sequence of
*       frames to a specified data rate and number of key frames.
*       When finished comressing data, use
*       <f ICCompressorFree> to release the resources
*       specified in the <t COMPVARS> structure.
*
*       If you do not use <f ICCompressorChoose> you must
*       initialize the following members of the <t COMPVARS> structure:
*
*   <e COMPVARS.cbSize> Set to the sizeof(COMPVARS) to validate the structure.
*
*   <e COMPVARS.hic> Set to the handle of a compressor you have opened with
*       <f ICOpen>. You do not need to close it (<f ICCompressorFree>
*       will do this for you).
*
*   <e COMPVARS.lpbiOut> Optionally set this to force the compressor
*       to compress to a specific format instead of the default.
*       This will be freed by <f ICCompressorFree>.
*
*   <e COMPVARS.lKey> Set this to the key-frame frequency you want
*     or zero for none.
*
*   <e COMPVARS.lQ> Set this to the quality level to use or ICQUALITY_DEFAULT.
*
*   <e COMPVARS.dwFlags> Set ICMF_COMPVARS_VALID flag to indicate the structure is initialized.
*
* @xref <f ICCompressorChoose> <f ICSeqCompressFrame> <f ICSeqCompressFrameEnd>
*   <f ICCompressorFree>
*
*******************************************************************/
///////////////////////////////////////////////////////////////////////////////
//
//  ICSeqCompressFrameStart
//
///////////////////////////////////////////////////////////////////////////////
BOOL VFWAPI ICSeqCompressFrameStart(PCOMPVARS pc, LPBITMAPINFO lpbiIn)
{
    DWORD       dwSize;
    ICINFO  icinfo;

    if (pc == NULL || pc->cbSize != sizeof(COMPVARS))
        return FALSE;

    if (pc->hic == NULL || lpbiIn == NULL)
        return FALSE;

    //
    // make sure the found compressor can handle something
    // if not, force back to the default setting
    //
    if (ICCompressQuery(pc->hic, lpbiIn, pc->lpbiOut) != ICERR_OK) {
        // If the input format has changed since the output was selected,
        // force a reinitialization of the output format
        if (pc->lpbiOut) {
            GlobalFreePtr (pc->lpbiOut);
            pc->lpbiOut = NULL;
        }
    }

    //
    // fill in defaults: key frame every frame, and default quality
    //
    if (pc->lKey < 0)
        pc->lKey = 1;

    if (pc->lQ == ICQUALITY_DEFAULT)
        pc->lQ = ICGetDefaultQuality(pc->hic);

    //
    // If no output format is given, use a default
    //
    if (pc->lpbiOut == NULL) {
    dwSize = ICCompressGetFormatSize(pc->hic, lpbiIn);
    if (!(pc->lpbiOut = (LPBITMAPINFO)GlobalAllocPtr(GMEM_MOVEABLE,dwSize)))
        goto StartError;
    ICCompressGetFormat(pc->hic, lpbiIn, pc->lpbiOut);
    }
    pc->lpbiOut->bmiHeader.biSizeImage =
        ICCompressGetSize (pc->hic, lpbiIn, pc->lpbiOut);
    pc->lpbiOut->bmiHeader.biClrUsed = DibNumColors(&(pc->lpbiOut->bmiHeader));

    //
    // Set the input format and initialize the key frame count
    //
    pc->lpbiIn = lpbiIn;
    pc->lKeyCount = pc->lKey;
    pc->lFrame = 0;     // first frame we'll be compressing is 0

    if (ICCompressQuery(pc->hic, lpbiIn, pc->lpbiOut) != ICERR_OK)
        goto StartError;

    //
    // Allocate a buffer for the compressed bits
    //
    dwSize = pc->lpbiOut->bmiHeader.biSizeImage;

    // !!! Hack for VidCap... make it big enough for two RIFF structs and
    // !!! pad records.
    //
    dwSize += 2048 + 16;

    if (!(pc->lpBitsOut = GlobalAllocPtr(GMEM_MOVEABLE, dwSize)))
        goto StartError;

    //
    // Allocate a buffer for the decompressed previous frame if it can do
    // key frames and we want key frames and it needs such a buffer.
    //
    ICGetInfo(pc->hic, &icinfo, sizeof(icinfo));
    if ((pc->lKey != 1) && (icinfo.dwFlags & VIDCF_TEMPORAL) &&
        !(icinfo.dwFlags & VIDCF_FASTTEMPORALC)) {
        dwSize = lpbiIn->bmiHeader.biSizeImage;
        if (!(pc->lpBitsPrev = GlobalAllocPtr(GMEM_MOVEABLE, dwSize)))
            goto StartError;
    }

    //
    // now get compman ready for the big job
    //
    if (ICCompressBegin(pc->hic, lpbiIn, pc->lpbiOut) != ICERR_OK)
        goto StartError;

    //
    // Get ready to decompress previous frames if we're doing key frames
    // If we can't decompress, we must do all key frames
    //
    if (pc->lpBitsPrev) {
        if (ICDecompressBegin(pc->hic, pc->lpbiOut, lpbiIn) != ICERR_OK) {
        pc->lKey = pc->lKeyCount = 1;
        GlobalFreePtr(pc->lpBitsPrev);
        pc->lpBitsPrev = NULL;
    }
    }

    return TRUE;

StartError:

    // !!! Leave stuff allocated because ICCompressorFree() will clear things
    return FALSE;
}

/*******************************************************************
* @doc EXTERNAL ICSeqCompressFrameEnd ICAPPS
*
* @api void | ICSeqCompressFrameEnd | This function terminates sequence
*   compression using <f ICSeqCompressFrame>.
*
* @parm PCOMPVARS | pc | Specifies a pointer to a <t COMPVARS> structure
*       used during sequence compression.
*
* @comm Use <f ICCompressorChoose> to let the
*       user specify a compressor to use, or initialize a <t COMPVARS> structure
*       manually. Use <f ICSeqCompressFrameStart>, <f ICSeqCompressFrame>
*       and <f ICSeqCompressFrameEnd> functions to compress a sequence of
*       frames to a specified data rate and number of key frames. When
*       finished with compression, use <f ICCompressorFree> to
*       release the resources specified by the <t COMPVARS> structure.
*
* @xref <f ICCompressorChoose> <f ICSeqCompressFrame> <f ICCompressorFree>
*   <f ICSeqCompressFrameStart>
*
*******************************************************************/
///////////////////////////////////////////////////////////////////////////////
//
//  ICSeqCompressFrameEnd
//
///////////////////////////////////////////////////////////////////////////////
void VFWAPI ICSeqCompressFrameEnd(PCOMPVARS pc)
{
    if (pc == NULL || pc->cbSize != sizeof(COMPVARS))
        return;

    // This function still leaves pc->hic and pc->lpbiOut alloced and open
    // since they were set by ICCompressorChoose

    // Seems we've already freed everything - don't call ICCompressEnd twice
    if (pc->lpBitsOut == NULL)
        return;

    /* Stop compressing */
    if (pc->hic) {
        ICCompressEnd(pc->hic);

        if (pc->lpBitsPrev)
            ICDecompressEnd(pc->hic);
    }

    /* Free the buffer for compressed image */
    if (pc->lpBitsOut) {
    GlobalFreePtr(pc->lpBitsOut);
    pc->lpBitsOut = NULL;
    }

    /* Free the buffer for the decompressed previous frame */
    if (pc->lpBitsPrev) {
    GlobalFreePtr(pc->lpBitsPrev);
    pc->lpBitsPrev = NULL;
    }
}

/*******************************************************************
* @doc EXTERNAL ICSeqCompressFrame ICAPPS
*
* @api LPVOID | ICSeqCompressFrame | This function compresses a
*  frame in a sequence of frames. The data rate for the sequence
*  as well as the key-frame frequency can be specified. Use this function
*  once for each frame to be compressed.
*
* @parm PCOMPVARS | pc | Specifies a pointer to a <t COMPVARS> structure
*       initialized with information about the compression.
*
* @parm UINT | uiFlags | Specifies flags for this function. Set this
*       parameter to zero.
*
* @parm LPVOID | lpBits | Specifies a pointer the data bits to compress.
*       (The data bits excludes header or format information.)
*
* @parm BOOL FAR * | pfKey | Returns whether or not the frame was compressed
*       into a keyframe.
*
* @parm LONG FAR * | plSize | Specifies the maximum size desired for
*       the compressed image. The compressor might not be able to
*       compress the data to within this size. When the function
*       returns, the parameter points to the size of the compressed
*       image. Images sizes are specified in bytes.
*
* @rdesc Returns a pointer to the compressed bits.
*
* @comm Use <f ICCompressorChoose> to let the
*       user specify a compressor to use, or initialize a <t COMPVARS> structure
*       manually. Use <f ICSeqCompressFrameStart>, <f ICSeqCompressFrame>
*       and <f ICSeqCompressFrameEnd> functions to compress a sequence of
*       frames to a specified data rate and number of key frames. When
*       finished with compression, use <f ICCompressorFree> to
*       release the resources specified by the <t COMPVARS> structure.
*
*   Use this function repeatedly to compress a video sequence one
*  frame at a time. Use this function instead of <f ICCompress>
*  to compress a video sequence. This function supports creating key frames
*   in the compressed sequence at any frequency you like and handles
*  much of the initialization process.
* @xref <f ICCompressorChoose> <f ICSeqCompressFrameEnd> <f ICCompressorFree>
*   <f ICCompressorFreeStart>
*
*******************************************************************/
///////////////////////////////////////////////////////////////////////////////
//
//  ICSeqCompressFrame
//
//      compresses a given image but supports KEY FRAMES EVERY
//
//  input:
//      pc          stuff
//      uiFlags     flags (not used, must be 0)
//      lpBits      input DIB bits
//      lQuality    the reqested compression quality
//      pfKey       did this frame end up being a key frame?
//
//  returns:
//      a HANDLE to the converted image.  The handle is a DIB in CF_DIB
//      format, ie a packed DIB.  The caller is responsible for freeing
//      the memory.   NULL is returned if error.
//
///////////////////////////////////////////////////////////////////////////////
LPVOID VFWAPI ICSeqCompressFrame(
    PCOMPVARS               pc,         // junk set up by Start()
    UINT                    uiFlags,    // silly flags
    LPVOID                  lpBits,     // input DIB bits
    BOOL FAR            *pfKey, // did it end up being a key frame?
    LONG FAR            *plSize)    // requested size/size of returned image
{
    LONG    l;
    DWORD   dwFlags = 0;
    DWORD   ckid = 0;
    BOOL    fKey;
    LONG    lSize = plSize ? *plSize : 0;

    // Is it time to make a keyframe?
    // First frame will always be a keyframe cuz they initialize to the same
    // value.
    fKey = (pc->lKeyCount >= pc->lKey);

    l = ICCompress(pc->hic,
            fKey ? ICCOMPRESS_KEYFRAME : 0,   // flags
            (LPBITMAPINFOHEADER)pc->lpbiOut,    // output format
            pc->lpBitsOut,  // output data
            (LPBITMAPINFOHEADER)pc->lpbiIn,     // format of frame to compress
            lpBits,         // frame data to compress
            &ckid,          // ckid for data in AVI file
            &dwFlags,       // flags in the AVI index.
            pc->lFrame,     // frame number of seq.
            lSize,          // reqested size in bytes. (if non zero)
            pc->lQ,         // quality
            fKey ? NULL : (LPBITMAPINFOHEADER)pc->lpbiIn, // fmt of prev frame
            fKey ? NULL : pc->lpBitsPrev);        // previous frame

    if (l < ICERR_OK)
        goto FrameError;

    /* Return the size of the compressed data */
    if (plSize)
    *plSize = pc->lpbiOut->bmiHeader.biSizeImage;

    /* Now decompress the frame into our buffer for the previous frame */
    if (pc->lpBitsPrev) {
    l = ICDecompress(pc->hic,
         0,
         (LPBITMAPINFOHEADER)pc->lpbiOut,
         pc->lpBitsOut,
                 (LPBITMAPINFOHEADER)pc->lpbiIn,  // !!! should check for this.
         pc->lpBitsPrev);

    if (l != ICERR_OK)
        goto FrameError;
    }

    /* Was the compressed image a keyframe? */
    *pfKey = (BOOL)(dwFlags & AVIIF_KEYFRAME);

    /* After making a keyframe, reset our counter that tells us when we MUST */
    /* make another one.    */
    if (*pfKey)
    pc->lKeyCount = 0;

    // Never make a keyframe again after the first one if we don't want them.
    // Increment our counter of how long its been since the last one if we do.
    if (pc->lKey)
        pc->lKeyCount++;
    else
    pc->lKeyCount = -1;

    // Next time we're called we're on the next frame
    pc->lFrame++;

    return (pc->lpBitsOut);

FrameError:

    return NULL;
}


/*******************************************************************
* @doc EXTERNAL ICImageCompress ICAPPS
*
* @api HANDLE | ICImageCompress | This function provides
*  convenient method of compressing an image to a given
*   size. This function does not require use of initialization functions.
*
* @parm HIC | hic | Specifies the handle to a compressor to be
*       opened with <f ICOpen> or NULL.  Use NULL to choose a
*       default compressor for your compression format.
*       Applications can use the compressor handle returned
*       by <f ICCompressorChoose> in the <e COMPVARS.hic> member
*       of the <t COMPVARS> structure if they want the user to
*       select the compressor.  This compressor is already opened.
*
* @parm UINT | uiFlags | Specifies flags for this function.  Set this
*       to zero.
*
* @parm LPBITMAPINFO | lpbiIn | Specifies the input data format.
*
* @parm LPVOID | lpBits | Specifies a pointer to input data bits to compress.
*       (The data bits exclude header or format information.)
*
* @parm LPBITMAPINFO | lpbiOut | Specifies the compressed output format or NULL.
*       If NULL, the compressor uses a default format.
*
* @parm LONG | lQuality | Specifies the quality value the compressor.
*
* @parm LONG FAR * | plSize | Specifies the maximum size desired for
*       the compressed image. The compressor might not be able to
*       compress the data to within this size. When the function
*       returns, the parameter points to the size of the compressed
*       image. Images sizes are specified in bytes.
*
*
* @rdesc Returns a handle to a compressed DIB. The image data follows the
*        format header.
*
* @comm This function returns a DIB with the format and image data.
*  To obtain the format information from the <t LPBITMAPINFOHEADER> structure,
*  use <f GlobalLock> to lock the data. Use <f GlobalFree> to free the
*  DIB when you have finished with it.
*
* @xref <f ICImageDecompress>
*
*******************************************************************/
///////////////////////////////////////////////////////////////////////////////
//
//  ICImageCompress
//
//      compresses a given image.
//
//  input:
//      hic         compressor to use, if NULL is specifed a
//                  compressor will be located that can handle the conversion.
//      uiFlags     flags (not used, must be 0)
//      lpbiIn      input DIB format
//      lpBits      input DIB bits
//      lpbiOut     output format, if NULL is specifed the default
//                  format choosen be the compressor will be used.
//      lQuality    the reqested compression quality
//      plSize      the reqested size for the image/returned size
//
//  returns:
//      a handle to a DIB which is the compressed image.
//
///////////////////////////////////////////////////////////////////////////////
HANDLE VFWAPI ICImageCompress(
    HIC                     hic,        // compressor (NULL if any will do)
    UINT                    uiFlags,    // silly flags
    LPBITMAPINFO        lpbiIn,     // input DIB format
    LPVOID                  lpBits,     // input DIB bits
    LPBITMAPINFO        lpbiOut,    // output format (NULL => default)
    LONG                    lQuality,   // the reqested quality
    LONG FAR *          plSize)     // requested size for compressed frame
{
    LONG    l;
    BOOL    fNuke;
    DWORD   dwFlags = 0;
    DWORD   ckid = 0;
    LONG    lSize = plSize ? *plSize : 0;

    LPBITMAPINFOHEADER lpbi=NULL;

    //
    // either locate a compressor or use the one supplied.
    //
    if (fNuke = (hic == NULL))
    {
        hic = ICLocate(ICTYPE_VIDEO, 0L, (LPBITMAPINFOHEADER)lpbiIn,
        (LPBITMAPINFOHEADER)lpbiOut, ICMODE_COMPRESS);

        if (hic == NULL)
            return NULL;
    }

    //
    // make sure the found compressor can compress something ??? WHY BOTHER ???
    //
    if (ICCompressQuery(hic, lpbiIn, NULL) != ICERR_OK)
        goto error;

    if (lpbiOut)
    {
    l = lpbiOut->bmiHeader.biSize + 256 * sizeof(RGBQUAD);
    }
    else
    {
    //
    //  now make a DIB header big enough to hold the output format
    //
    l = ICCompressGetFormatSize(hic, lpbiIn);

    if (l <= 0)
        goto error;
    }

    lpbi = (LPVOID)GlobalAllocPtr(GHND, l);

    if (lpbi == NULL)
        goto error;

    //
    //  if the compressor likes the passed format, use it else use the default
    //  format of the compressor.
    //
    if (lpbiOut == NULL || ICCompressQuery(hic, lpbiIn, lpbiOut) != ICERR_OK)
        ICCompressGetFormat(hic, lpbiIn, lpbi);
    else
        hmemcpy(lpbi, lpbiOut, lpbiOut->bmiHeader.biSize +
        lpbiOut->bmiHeader.biClrUsed * sizeof(RGBQUAD));

    lpbi->biSizeImage = ICCompressGetSize(hic, lpbiIn, lpbi);
    lpbi->biClrUsed = DibNumColors(lpbi);

    //
    // now resize the DIB to be the maximal size.
    //
    lpbi = (LPVOID)GlobalReAllocPtr(lpbi,DibSize(lpbi), 0);

    if (lpbi == NULL)
        goto error;

    //
    // now compress it.
    //
    if (ICCompressBegin(hic, lpbiIn, lpbi) != ICERR_OK)
        goto error;

    if (lpBits == NULL)
        lpBits = DibPtr((LPBITMAPINFOHEADER)lpbiIn);

    if (lQuality == ICQUALITY_DEFAULT)
        lQuality = ICGetDefaultQuality(hic);

    l = ICCompress(hic,
            0,              // flags
            (LPBITMAPINFOHEADER)lpbi,  // output format
            DibPtr(lpbi),   // output data
            (LPBITMAPINFOHEADER)lpbiIn,// format of frame to compress
            lpBits,         // frame data to compress
            &ckid,          // ckid for data in AVI file
            &dwFlags,       // flags in the AVI index.
            0,              // frame number of seq.
            lSize,          // requested size in bytes. (if non zero)
            lQuality,       // quality
            NULL,           // format of previous frame
            NULL);          // previous frame

    if (l < ICERR_OK) {
    DPF(("ICCompress returned %ld!\n", l));
        ICCompressEnd(hic);
        goto error;
    }

    // Return the size of the compressed data
    if (plSize)
    *plSize = lpbi->biSizeImage;

    if (ICCompressEnd(hic) != ICERR_OK)
        goto error;

    //
    // now resize the DIB to be the real size.
    //
    lpbi = (LPVOID)GlobalReAllocPtr(lpbi, DibSize(lpbi), 0);

    //
    // all done return the result to the caller
    //
    if (fNuke)
        ICClose(hic);

    return GlobalPtrHandle(lpbi);

error:
    if (lpbi)
        GlobalFreePtr(lpbi);

    if (fNuke)
        ICClose(hic);

    return NULL;
}
/*******************************************************************
*
* @doc EXTERNAL ICImageDecompress ICAPPS
*
* @api HANDLE | ICImageDecompress | This function provides
*  convenient method of decompressing an image without
*   using initialization functions.
**
* @parm HIC | hic | Specifies the handle to a decompressor opened
*       with <f ICOpen> or NULL.  Use NULL to choose a default
*       decompressor for your format.
*
* @parm UINT | uiFlags | Specifies flags for this function.  Set this
*       to zero.
*
* @parm LPBITMAPINFO | lpbiIn | Specifies the compressed input data format.
*
* @parm LPVOID | lpBits | Specifies a pointer to input data bits to compress.
*       (The data bits excludes header or format information.)
*
* @parm LPBITMAPINFO | lpbiOut | Specifies the decompressed output format or NULL.
*       If NULL, the decompressor uses  a default format.
*
* @rdesc Returns a handle to an uncompressed DIB in the CF_DIB format,
*        or NULL for an error. The image data follows the format header.
*
* @comm This function returns a DIB with the format and image data.
*  To obtain the format information from the <t LPBITMAPINFOHEADER> structure,
*  use <f GlobalLock> to lock the data. Use <f GlobalFree> to free the
*  DIB when you have finished with it.
*

* @xref <f ICImageCompress>
*
*******************************************************************/
///////////////////////////////////////////////////////////////////////////////
//
//  ICImageDecompress
//
//      decompresses a given image.
//
//  input:
//      hic         compressor to use, if NULL is specifed a
//                  compressor will be located that can handle the conversion.
//      uiFlags     flags (not used, must be 0)
//      lpbiIn      input DIB format
//      lpBits      input DIB bits
//      lpbiOut     output format, if NULL is specifed the default
//                  format choosen be the compressor will be used.
//
//  returns:
//      a HANDLE to the converted image.  The handle is a DIB in CF_DIB
//      format, ie a packed DIB.  The caller is responsible for freeing
//      the memory.   NULL is returned if error.
//
///////////////////////////////////////////////////////////////////////////////
HANDLE VFWAPI ICImageDecompress(
    HIC                     hic,        // compressor (NULL if any will do)
    UINT                    uiFlags,    // silly flags
    LPBITMAPINFO            lpbiIn,     // input DIB format
    LPVOID                  lpBits,     // input DIB bits
    LPBITMAPINFO            lpbiOut)    // output format (NULL => default)
{
    LONG    l;
    BOOL    fNuke;
    DWORD   dwFlags = 0;
    DWORD   ckid = 0;

    LPBITMAPINFOHEADER lpbi=NULL;

    //
    // either locate a compressor or use the one supplied.
    //
    if (fNuke = (hic == NULL))
    {
        hic = ICLocate(ICTYPE_VIDEO, 0L, (LPBITMAPINFOHEADER)lpbiIn,
        (LPBITMAPINFOHEADER)lpbiOut, ICMODE_DECOMPRESS);

        if (hic == NULL)
            return NULL;
    }

    //
    // make sure the found compressor can decompress at all ??? WHY BOTHER ???
    //
    if (ICDecompressQuery(hic, lpbiIn, NULL) != ICERR_OK)
        goto error;

    if (lpbiOut)
    {
    l = lpbiOut->bmiHeader.biSize + 256 * sizeof(RGBQUAD);
    }
    else
    {
    //
    //  now make a DIB header big enough to hold the output format
    //
    l = ICDecompressGetFormatSize(hic, lpbiIn);

    if (l <= 0)
        goto error;
    }

    lpbi = (LPVOID)GlobalAllocPtr(GHND, l);

    if (lpbi == NULL)
        goto error;

    //
    //  if we didn't provide an output format, use a default.
    //
    if (lpbiOut == NULL)
        ICDecompressGetFormat(hic, lpbiIn, lpbi);
    else
        hmemcpy(lpbi, lpbiOut, lpbiOut->bmiHeader.biSize +
        lpbiOut->bmiHeader.biClrUsed * sizeof(RGBQUAD));

    //
    // For decompress make sure the palette (ie color table) is correct
    // just in case they provided an output format and the decompressor used
    // that format but not their palette.
    //
    if (lpbi->biBitCount <= 8)
        ICDecompressGetPalette(hic, lpbiIn, lpbi);

    lpbi->biSizeImage = DibSizeImage(lpbi); // ICDecompressGetSize(hic, lpbi);
    lpbi->biClrUsed = DibNumColors(lpbi);

    //
    // now resize the DIB to be the right size.
    //
    lpbi = (LPVOID)GlobalReAllocPtr(lpbi,DibSize(lpbi),0);

    if (lpbi == NULL)
        goto error;

    //
    // now decompress it.
    //
    if (ICDecompressBegin(hic, lpbiIn, lpbi) != ICERR_OK)
        goto error;

    if (lpBits == NULL)
        lpBits = DibPtr((LPBITMAPINFOHEADER)lpbiIn);

    l = ICDecompress(hic,
            0,              // flags
            (LPBITMAPINFOHEADER)lpbiIn, // format of frame to decompress
            lpBits,         // frame data to decompress
            (LPBITMAPINFOHEADER)lpbi,   // output format
            DibPtr(lpbi));  // output data

    if (l < ICERR_OK) {
    ICDecompressEnd(hic);
        goto error;
    }

    if (ICDecompressEnd(hic) != ICERR_OK)
        goto error;

    //
    // now resize the DIB to be the real size.
    //
    lpbi = (LPVOID)GlobalReAllocPtr(lpbi,DibSize(lpbi),0);

    //
    // all done return the result to the caller
    //
    if (fNuke)
        ICClose(hic);

    return GlobalPtrHandle(lpbi);

error:
    if (lpbi)
        GlobalFreePtr(lpbi);

    if (fNuke)
        ICClose(hic);

    return NULL;
}


///////////////////////////////////////////////////////////////////////////////
//
//  ICCompressorChooseStuff
//
///////////////////////////////////////////////////////////////////////////////

BOOL VFWAPI ICCompressorChooseDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

typedef struct {
    DWORD       fccType;
    DWORD       fccHandler;
    UINT        uiFlags;
    LPVOID      pvIn;
    LPVOID      lpData;
    HWND    hwnd;
    HIC         hic;
    LONG        lQ;
    LONG        lKey;
    LONG        lDataRate;
    ICINFO      icinfo;
    LPSTR       lpszTitle;
    PAVISTREAM  pavi;
    AVISTREAMINFO info;
    HDRAWDIB    hdd;
    PGETFRAME   pgf;
    LPVOID  lpState;
    LONG    cbState;
    BOOL    fClosing;
} ICCompressorChooseStuff, FAR *PICCompressorChooseStuff;

/*******************************************************************
* @doc EXTERNAL ICCompressorChoose ICAPPS
*
* @api BOOL | ICCompressorChoose | Displays a dialog box for choosing a
*   compressor. It optionally provides a data rate box, key frame box, preview
*   window, and filtering to display only compressors that can handle a
*   specific format.
*
* @parm HWND | hwnd | Specifies the parent window for the dialog box.
*
* @parm UINT | uiFlags | Specifies flags for this function. The following
*      flags are defined:
*
* @flag ICMF_CHOOSE_KEYFRAME | Displays a check box and edit box to enter the
*   frequency of key frames.
*
* @flag ICMF_CHOOSE_DATARATE | Displays a check box and edit box to enter the
*   data rate for the movie.
*
* @flag ICMF_CHOOSE_PREVIEW | Displays a button to expand the dialog box to
*        include a preview window. The preview window shows how
*       frames of your movie will appear when compressed with the
*       current settings.
*
* @flag ICMF_CHOOSE_ALLCOMPRESSORS | Indicates all compressors should
*       should appear in the selection list. If this flag is not specified,
*       just the compressors that can handle the input format appear in
*       the selection list.
*
* @parm LPVOID | pvIn | Specifies the uncompressed
*       data input format. This parameter is optional.
*
* @parm LPVOID | lpData | Specifies a <t PAVISTREAM> of type
*       streamtypeVIDEO to use in the preview window. This parameter
*       is optional.
*
* @parm PCOMPVARS | pc | Specifies a pointer to a <t COMPVARS>
*      structure. The information returned initializes the
*      structure for use with other functions.
*
* @parm LPSTR | lpszTitle | Points to a optional zero-terminated string
*       containing a title for the dialog box.
*
* @rdesc Returns TRUE if the user chooses a compressor, and presses OK.  Returns
*   FALSE for an error, or if the user presses CANCEL.
*
* @comm This function lets the user select a compressor from a list.
*   Before using it, set the <e COMPVARS.cbSize> member of the <t COMPVARS>
*  structure to sizeof(COMPVARS). Initialize the rest of the structure
*  to zeros unless you want to specify some valid defaults for
*   the dialog box. If specifing defaults, set the <e COMPVARS.dwFlags>
*   member to ICMF_COMPVARS_VALID, and initialize the other members of
*  the structure. See <f ICSeqCompressorFrameStart> and <t COMPVARS>
*  for more information about initializing the structure.
*
* @xref <f ICCompressorFree> <f ICSeqCompressFrameStart> <f ICSeqCompressFrame>
*   <f ICSeqCompressFrameEnd>
*
*******************************************************************/
///////////////////////////////////////////////////////////////////////////////
//
//  ICCompressorChoose
//
//      Brings up a dialog and allows the user to choose a compression
//      method and a quality level, and/or a key frame frequency.
//  All compressors in the system are displayed or can be optionally
//  filtered by "ability to compress" a specifed format.
//
//      the dialog allows the user to configure or bring up the compressors
//      about box.
//
//      A preview window can be provided to show a preview of a specific
//      compression.
//
//      the selected compressor is opened (via ICOpen) and returned to the
//      caller, it must be disposed of by calling ICCompressorFree.
//
//  input:
//      HWND    hwnd            parent window for dialog box.
//      UINT    uiFlags         flags
//      LPVOID  pvIn            input format (optional), only compressors that
//              handle this format will be displayed.
//      LPVOID  pavi            input stream for the options preview
//      PCOMPVARS pcj           returns COMPVARS struct for use with other APIs
//      LPSTR   lpszTitle   Optional title for dialog box
//
//  returns:
//      TRUE if dialog shown and user chose a compressor.
//      FALSE if dialog was not shown or user hit cancel.
//
///////////////////////////////////////////////////////////////////////////////

BOOL VFWAPI ICCompressorChoose(
    HWND        hwnd,               // parent window for dialog
    UINT        uiFlags,            // flags
    LPVOID      pvIn,               // input format (optional)
    LPVOID      pavi,               // input stream (for preview - optional)
    PCOMPVARS   pcj,                // state of compressor/dlg
    LPSTR       lpszTitle)          // dialog title (if NULL, use default)
{
    BOOL f;
    PICCompressorChooseStuff p;
    DWORD   dwSize;

    if (pcj == NULL || pcj->cbSize != sizeof(COMPVARS))
        return FALSE;

    //
    // !!! Initialize the structure
    //
    if (!(pcj->dwFlags & ICMF_COMPVARS_VALID)) {
        pcj->hic = NULL;
        pcj->fccType = 0;
        pcj->fccHandler = 0;
        pcj->lQ = ICQUALITY_DEFAULT;
        pcj->lKey = 1;
        pcj->lDataRate = 0;
        pcj->lpbiOut = NULL;
        pcj->lpBitsOut = NULL;
        pcj->lpBitsPrev = NULL;
        pcj->dwFlags = 0;
        pcj->lpState = NULL;
        pcj->cbState = 0;
    }

    // Default type is a video compressor
    if (pcj->fccType == 0)
        pcj->fccType = ICTYPE_VIDEO;

    p = (LPVOID)GlobalAllocPtr(GHND, sizeof(ICCompressorChooseStuff));

    if (p == NULL)
        return FALSE;

    p->fccType    = pcj->fccType;
    p->fccHandler = pcj->fccHandler;
    p->uiFlags    = uiFlags;
    p->pvIn       = pvIn;
    p->lQ         = pcj->lQ;
    p->lKey       = pcj->lKey;
    p->lDataRate  = pcj->lDataRate;
    p->lpszTitle  = lpszTitle;
    p->pavi       = (PAVISTREAM)pavi;
    p->hdd        = NULL;
    p->lpState    = pcj->lpState;
    pcj->lpState = NULL;    // so it won't be freed
    p->cbState    = pcj->cbState;
    // !!! Validate this pointer
    // !!! AddRef if it is
    if (p->pavi) {
        if (p->pavi->lpVtbl->Info(p->pavi, &p->info, sizeof(p->info)) !=
        AVIERR_OK || p->info.fccType != streamtypeVIDEO)
        p->pavi = NULL;
    }

    f = DialogBoxParam(ghInst, TEXT("ICCDLG"),
        hwnd, (DLGPROC)ICCompressorChooseDlgProc, (LPARAM)(LPVOID)p);

    // !!! Treat error like cancel
    if (f == -1)
    f = FALSE;

    //
    // if the user picked a compressor then return this info to the caller
    //
    if (f) {

    // If we are called twice in a row, we have good junk in here that
    // needs to be freed before we tromp over it.
    ICCompressorFree(pcj);

        pcj->lQ = p->lQ;
        pcj->lKey = p->lKey;
        pcj->lDataRate = p->lDataRate;
        pcj->hic = p->hic;
        pcj->fccHandler = p->fccHandler;
        pcj->lpState = p->lpState;
        pcj->cbState = p->cbState;

    pcj->dwFlags |= ICMF_COMPVARS_VALID;
    }

    GlobalFreePtr(p);

    if (!f)
    return FALSE;

    if (pcj->hic && pvIn) {  // hic is NULL if no compression selected

        /* Get the format we're going to compress into. */
        dwSize = ICCompressGetFormatSize(pcj->hic, pvIn);
        if ((pcj->lpbiOut =
        (LPBITMAPINFO)GlobalAllocPtr(GMEM_MOVEABLE, dwSize)) == NULL) {
            ICClose(pcj->hic);      // Close this since we're erroring
            pcj->hic = NULL;
            return FALSE;
        }
        ICCompressGetFormat(pcj->hic, pvIn, pcj->lpbiOut);
    }

    return TRUE;
}

void SizeDialog(HWND hwnd, WORD id) {
    RECT    rc;

    GetWindowRect(GetDlgItem(hwnd, id), &rc);

    /* First, get rc in Client co-ords */
    ScreenToClient(hwnd, (LPPOINT)&rc + 1);
    rc.top = 0; rc.left = 0;

    /* Grow by non-client size */
    AdjustWindowRect(&rc, GetWindowLong(hwnd, GWL_STYLE),
    GetMenu(hwnd) !=NULL);

    /* That's the new size for the dialog */
    SetWindowPos(hwnd, NULL, 0, 0, rc.right-rc.left,
            rc.bottom-rc.top,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}


void TermPreview(PICCompressorChooseStuff p)
{
    if (p->hdd)
        DrawDibClose(p->hdd);
    p->hdd = NULL;
}


BOOL InitPreview(HWND hwnd, PICCompressorChooseStuff p) {

    p->hdd = DrawDibOpen();
    if (!p->hdd)
    return FALSE;
}

#ifdef SAFETOYIELD
//
// Code to yield while we're not calling GetMessage.
// Dispatch all messages.  Pressing ESC or closing aborts.
//
BOOL WinYield(HWND hwnd)
{
    MSG msg;
    BOOL fAbort=FALSE;

    while(/* fWait > 0 && */ !fAbort && PeekMessage(&msg,NULL,0,0,PM_REMOVE))
    {
    if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)
            fAbort = TRUE;
    if (msg.message == WM_SYSCOMMAND && (msg.wParam & 0xFFF0) == SC_CLOSE)
        fAbort = TRUE;

    if (msg.hwnd == hwnd) {
        if (msg.message == WM_KEYDOWN ||
        msg.message == WM_SYSKEYDOWN ||
        msg.message == WM_HSCROLL ||
        msg.message == WM_PARENTNOTIFY ||
        msg.message == WM_LBUTTONDOWN) {
        PostMessage(hwnd, msg.message, msg.wParam, msg.lParam);
        return TRUE;
        }
    }

    TranslateMessage(&msg);
    DispatchMessage(&msg);
    }
    return fAbort;
}
#endif

LONG CALLBACK _loadds PreviewStatusProc(LPARAM lParam, UINT message, LONG l)
{
    TCHAR   ach[100], achT[100];
    BOOL    f;
    PICCompressorChooseStuff p = (PICCompressorChooseStuff) lParam;

    if (message != ICSTATUS_STATUS) {
    DPF(("Status callback: lParam = %lx, message = %u, l = %lu\n", lParam, message, l));
    }

    // !!!!
    // !!!! Status messages need to be fixed!!!!!!
    // !!!!

    switch (message) {
    case ICSTATUS_START:
        break;

    case ICSTATUS_STATUS:
            LoadString (ghInst, ID_FRAMECOMPRESSING, achT, sizeof(achT));
        wsprintf(ach, achT, GetScrollPos(GetDlgItem(p->hwnd,
                ID_PREVIEWSCROLL), SB_CTL), l);

        SetDlgItemText(p->hwnd, ID_PREVIEWTEXT, ach);
        break;

    case ICSTATUS_END:
        break;

    case ICSTATUS_YIELD:

        break;
    }

#ifdef SAFETOYIELD
    f = WinYield(p->hwnd);
#else
    f = FALSE;
#endif;

    if (f) {
    DPF(("Aborting from within status proc!\n"));
    }

    return f;
}


void Preview(HWND hwnd, PICCompressorChooseStuff p, BOOL fCompress)
{
    RECT    rc;
    HDC     hdc;
    int     pos;
    HANDLE  h;
    HCURSOR hcur = NULL;
    LPBITMAPINFOHEADER  lpbi, lpbiU, lpbiC = NULL;
    TCHAR       ach[120], achT[100];
    LONG    lsizeD = 0;
    LONG    lSize;
    int     x;

    // Not previewing right now!
    if (!p->hdd || !p->pgf)
    return;

    pos = GetScrollPos(GetDlgItem(hwnd, ID_PREVIEWSCROLL), SB_CTL);
    lpbi = lpbiU = AVIStreamGetFrame(p->pgf, pos);
    if (!lpbi)
    return;

    //
    // What would the image look like compressed?
    //
    if (fCompress && (int)p->hic > 0) {
    LRESULT     lRet;

    lRet = ICSetStatusProc(p->hic, 0, p, PreviewStatusProc);
    if (lRet != 0) {
        hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
    }

    // !!! Gives whole data rate to this stream
    // !!! What to do if Rate or Scale is zero?
    lSize = (GetDlgItemInt(hwnd, ID_DATARATE, NULL, FALSE)  * 1024L) /
            ((p->info.dwScale && p->info.dwRate) ?
            (p->info.dwRate / p->info.dwScale) : 1L);
        h = ICImageCompress(p->hic,
        0,
        (LPBITMAPINFO)lpbi,
        (LPBYTE)lpbi + lpbi->biSize + lpbi->biClrUsed * sizeof(RGBQUAD),
        NULL,
        GetScrollPos(GetDlgItem(hwnd, ID_QUALITY), SB_CTL) * 100,
        &lSize);
    if (hcur)
        SetCursor(hcur);
        if (h)
            lpbiC = (LPBITMAPINFOHEADER)GlobalLock(h);
        // Use the compressed image if we have one.. else use the original frame
        if (lpbiC)
        lpbi = lpbiC;
    }

    //
    // If we chose NO COMPRESSION, tell them the size of the data as its
    // compressed now.  Otherwise, use the size it will become when compressed
    // or the full frame size.
    //
    if (fCompress && (int)p->hic == 0) {
    p->pavi->lpVtbl->Read(p->pavi, pos, 1, NULL, 0, &lsizeD, NULL);
    } else {
    lsizeD = (lpbiC ? lpbiC->biSizeImage : lpbiU->biSizeImage);
    }

    hdc = GetDC(GetDlgItem(hwnd, ID_PREVIEWWIN));
    GetClientRect(GetDlgItem(hwnd, ID_PREVIEWWIN), &rc);

    // Clip regions aren't set up right for windows in a dialog, so make sure
    // we'll only paint into the window and not spill around it.
    IntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);

    // Now go ahead and draw a miniature frame that preserves the aspect ratio
    // centred in our preview window
    x = MulDiv((int)lpbi->biWidth, 3, 4);
    if (x <= (int)lpbi->biHeight) {
    rc.left = (rc.right - MulDiv(rc.right, x, (int)lpbi->biHeight)) / 2;
    rc.right -= rc.left;
    } else {
    x = MulDiv((int)lpbi->biHeight, 4, 3);
    rc.top = (rc.bottom - MulDiv(rc.bottom, x, (int)lpbi->biWidth)) / 2;
    rc.bottom -= rc.top;
    }
    DrawDibDraw(p->hdd, hdc, rc.left, rc.top, rc.right - rc.left,
    rc.bottom - rc.top, lpbi, NULL, 0, 0, -1, -1, 0);

    // Print the sizes and ratio for this frame
    LoadString (ghInst, ID_FRAMESIZE, achT, sizeof(achT));
    wsprintf(ach, achT,
    GetScrollPos(GetDlgItem(hwnd, ID_PREVIEWSCROLL), SB_CTL),
    lsizeD,
    lpbiU->biSizeImage,
    lsizeD * 100 / lpbiU->biSizeImage);
    SetDlgItemText(hwnd, ID_PREVIEWTEXT, ach);
    if (lpbiC)
        GlobalFreePtr(lpbiC);
    ReleaseDC(GetDlgItem(hwnd, ID_PREVIEWWIN), hdc);
}


///////////////////////////////////////////////////////////////////////////////
//
//  ICCompressorChooseDlgProc
//
//  dialog box procedure for ICCompressorChoose, a pointer to a
//  ICCompressorChooseStuff pointer must be passed to initialize this
//  dialog.
//
//  NOTE: this dialog box procedure does not use any globals
//  so I did not bother to _export it or use MakeProcAddress() if
//  you change this code to use globals, etc, be aware of this fact.
//
///////////////////////////////////////////////////////////////////////////////

BOOL VFWAPI ICCompressorChooseDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int i,n;
    int pos;
    HWND hwndC;
    PICCompressorChooseStuff p;
    HIC hic;
    BOOL fConfig, fAbout, fQuality, fKey, fDataRate;
    BOOL fShowKeyFrame, fShowDataRate, fShowPreview;
    int nSelectMe = -1;
    TCHAR ach[120], achT[80];
    RECT    rc;
    UINT    id;
    HDC     hdc;
    BOOL    f = FALSE, fCanDecompress = FALSE;
    LONG    lsize;
    LPBITMAPINFOHEADER lpbi = NULL;
    BOOL    fStreamIsCompressed = FALSE;
    HRESULT hr;

    p = (PICCompressorChooseStuff)GetWindowLong(hwnd,DWL_USER);


    switch (msg)
    {
        case WM_INITDIALOG:
        #define but &&
        #define and &&
        #define is ==
        #define isnt !=

            if (lParam == 0)
                return FALSE;

            SetWindowLong(hwnd,DWL_USER,lParam);
            p = (PICCompressorChooseStuff)lParam;

        p->hwnd = hwnd;

            // Let the user change the title of the dialog
            if (p->lpszTitle != NULL)
                SetWindowTextA(hwnd, p->lpszTitle);

        havifile = GetModuleHandle("avifile");

        if (havifile) {
            (FARPROC)AVIStreamGetFrameOpen =
            GetProcAddress((HINSTANCE)havifile,
            (LPCSTR)"AVIStreamGetFrameOpen");
            (FARPROC)AVIStreamGetFrame =
            GetProcAddress((HINSTANCE)havifile,
            (LPCSTR)"AVIStreamGetFrame");
            (FARPROC)AVIStreamGetFrameClose =
            GetProcAddress((HINSTANCE)havifile,
            (LPCSTR)"AVIStreamGetFrameClose");
            if (p->pavi)
                p->pgf = AVIStreamGetFrameOpen(p->pavi, NULL);
        }

        // We weren't passed in an input format but we have a PAVI we
        // can get a format from
        if (p->pvIn is NULL but p->pavi isnt NULL and p->pgf isnt NULL) {

        // We need to nuke pvIn later
        f = TRUE;

        // Find out if the AVI Stream is compressed or not
        p->pavi->lpVtbl->ReadFormat(p->pavi, 0, NULL, &lsize);
        if (lsize)
            lpbi = (LPBITMAPINFOHEADER)GlobalAllocPtr(GMEM_MOVEABLE,
                lsize);
        if (lpbi) {
            hr = p->pavi->lpVtbl->ReadFormat(p->pavi, 0, lpbi, &lsize);
            if (hr == AVIERR_OK)
            fStreamIsCompressed = lpbi->biCompression != BI_RGB;
            GlobalFreePtr(lpbi);
        }

        // Get the decompressed format of the AVI stream
        lpbi = AVIStreamGetFrame(p->pgf, 0);
        if (lpbi) {
            lsize = lpbi->biSize +
                lpbi->biClrUsed * sizeof(PALETTEENTRY);
            p->pvIn = (LPBITMAPINFOHEADER)GlobalAllocPtr(GMEM_MOVEABLE,
                lsize);
            if (p->pvIn)
                hmemcpy(p->pvIn, lpbi, lsize);
        }
        }

            //
            // now fill the combo box with all compressors
            //
            hwndC = GetDlgItem(hwnd, ID_COMPRESSOR);

            for (i=0; ICInfo(p->fccType, i, &p->icinfo); i++)
            {
                hic = ICOpen(p->icinfo.fccType, p->icinfo.fccHandler,
            ICMODE_COMPRESS);

                if (hic)
                {
                    //
                    // skip this compressor if it can't handle the
                    // specified format and we want to skip such compressors
                    //
                    if (!(p->uiFlags & ICMF_CHOOSE_ALLCOMPRESSORS) &&
            p->pvIn != NULL &&
                        ICCompressQuery(hic, p->pvIn, NULL) != ICERR_OK)
                    {
                        ICClose(hic);
                        continue;
                    }

                    //
                    // find out the compressor name.
                    //
                    ICGetInfo(hic, &p->icinfo, sizeof(p->icinfo));

                    //
                    // stuff it into the combo box and remember which one it was
                    //
                    n = ComboBox_AddString(hwndC,p->icinfo.szDescription);
#ifdef WIN32
barf    // Making a LONG out of a hic and an int just won't cut it
barf    // Look at the GetItemData's as well which will also break
#endif
                    ComboBox_SetItemData(hwndC, n, MAKELONG(hic, i));

            // This compressor is the one we want to come up default ?
            // Set its state
                // !!! Combo Box better not be sorted!
            // Convert both to upper case for an insensitive compare
            AnsiUpperBuff((LPSTR)&p->icinfo.fccHandler, sizeof(FOURCC));
            AnsiUpperBuff((LPSTR)&p->fccHandler, sizeof(FOURCC));
            if (p->icinfo.fccHandler == p->fccHandler) {
                nSelectMe = n;
            if (p->lpState)
                ICSetState(hic, p->lpState, p->cbState);
            }
                }
            }

        //
        // Next add a "No Recompression" item unless they passed in an
        // uncompressed format
        //
        if (!p->pvIn || fStreamIsCompressed ||
            ((LPBITMAPINFOHEADER)p->pvIn)->biCompression != BI_RGB) {
                LoadString (ghInst, ID_NOCOMPSTRING, ach, sizeof (ach));
                n = ComboBox_AddString(hwndC, ach);
            ComboBox_SetItemData(hwndC, n, 0);
            // Select "No Recompression" as the default if nobody else has
        // set it.
        if (nSelectMe == -1)
            nSelectMe = n;
        }
        //
        // Now add a "Full Frames (Uncompressed)" item unless we can't
        // decompress this format and they don't want all choices anyway
        //
            if (!(p->uiFlags & ICMF_CHOOSE_ALLCOMPRESSORS) && p->pvIn) {
        // If it's RGB, of course, just offer the option.
        if (((LPBITMAPINFOHEADER)p->pvIn)->biCompression != BI_RGB) {
            if ((hic = ICLocate(ICTYPE_VIDEO, 0, p->pvIn, NULL,
                ICMODE_DECOMPRESS)) == NULL)
            goto SkipFF;
            else
            ICClose(hic);
        }
        }

            LoadString (ghInst, ID_FULLFRAMESSTRING, ach, sizeof (ach));
            n = ComboBox_AddString(hwndC, ach);
        ComboBox_SetItemData(hwndC, n, MAKELONG(-1, 0));

        // Select "Full Frames" if that was the last one chosen
        // !!! Combo Box better not be sorted!
        if (nSelectMe == -1 &&
            (p->fccHandler == comptypeDIB || p->fccHandler == 0))
        nSelectMe = n;
        fCanDecompress = TRUE;

SkipFF:
        // If we haven't selected anything yet, choose something at random.
        if (nSelectMe == -1)
        nSelectMe = 0;

        fShowKeyFrame = p->uiFlags & ICMF_CHOOSE_KEYFRAME;
        fShowDataRate = p->uiFlags & ICMF_CHOOSE_DATARATE;
        // Don't show a preview if we can't draw the damn thing!
        fShowPreview  = (p->uiFlags & ICMF_CHOOSE_PREVIEW) && p->pavi &&
        fCanDecompress;

        // Hide our secret small place holders
        ShowWindow(GetDlgItem(hwnd, ID_CHOOSE_SMALL), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, ID_CHOOSE_NORMAL), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, ID_CHOOSE_BIG), SW_HIDE);

        if (!fShowKeyFrame) {
        ShowWindow(GetDlgItem(hwnd, ID_KEYFRAME), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, ID_KEYFRAMEBOX), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, ID_KEYFRAMETEXT), SW_HIDE);
        }

        if (!fShowDataRate) {
        ShowWindow(GetDlgItem(hwnd, ID_DATARATE), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, ID_DATARATEBOX), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, ID_DATARATETEXT), SW_HIDE);
        }

        if (!fShowPreview) {
        ShowWindow(GetDlgItem(hwnd, ID_PREVIEW), SW_HIDE);
        }

        // We start without these
        ShowWindow(GetDlgItem(hwnd, ID_PREVIEWWIN), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, ID_PREVIEWSCROLL), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, ID_PREVIEWTEXT), SW_HIDE);

        //
        // What size dialog do we need?
        //
        if (!fShowPreview && (!fShowDataRate || !fShowKeyFrame))
        SizeDialog(hwnd, ID_CHOOSE_SMALL);
        else
        SizeDialog(hwnd, ID_CHOOSE_NORMAL);

        //
        // Swap places for KeyFrameEvery and DataRate
        //
        if (fShowDataRate && !fShowKeyFrame) {
        GetWindowRect(GetDlgItem(hwnd, ID_KEYFRAME), &rc);
        ScreenToClient(hwnd, (LPPOINT)&rc);
        ScreenToClient(hwnd, (LPPOINT)&rc + 1);
        MoveWindow(GetDlgItem(hwnd, ID_DATARATE), rc.left, rc.top,
            rc.right - rc.left, rc.bottom - rc.top, FALSE);
        GetWindowRect(GetDlgItem(hwnd, ID_KEYFRAMEBOX), &rc);
        ScreenToClient(hwnd, (LPPOINT)&rc);
        ScreenToClient(hwnd, (LPPOINT)&rc + 1);
        MoveWindow(GetDlgItem(hwnd, ID_DATARATEBOX), rc.left, rc.top,
            rc.right - rc.left, rc.bottom - rc.top, FALSE);
        GetWindowRect(GetDlgItem(hwnd, ID_KEYFRAMETEXT), &rc);
        ScreenToClient(hwnd, (LPPOINT)&rc);
        ScreenToClient(hwnd, (LPPOINT)&rc + 1);
        MoveWindow(GetDlgItem(hwnd, ID_DATARATETEXT), rc.left, rc.top,
            rc.right - rc.left, rc.bottom - rc.top, TRUE);
        }

        //
        // Restore the dlg to the settings found in the structure
        //
        SetScrollRange(GetDlgItem(hwnd, ID_QUALITY), SB_CTL, 0, 100, FALSE);
        CheckDlgButton(hwnd, ID_KEYFRAMEBOX, (BOOL)(p->lKey));
        CheckDlgButton(hwnd, ID_DATARATEBOX, (BOOL)(p->lDataRate));
        SetDlgItemInt(hwnd, ID_KEYFRAME, (int)p->lKey, FALSE);
        SetDlgItemInt(hwnd, ID_DATARATE, (int)p->lDataRate, FALSE);
        ComboBox_SetCurSel(GetDlgItem(hwnd, ID_COMPRESSOR), nSelectMe);
            SendMessage(hwnd, WM_COMMAND, ID_COMPRESSOR,
        MAKELONG(hwndC, CBN_SELCHANGE));

        // We alloced this ourselves and need to free it now
        if (f && p->pvIn)
        GlobalFreePtr(p->pvIn);

            return TRUE;

        case WM_PALETTECHANGED:

        // It came from us.  Ignore it
            if (wParam == (WORD)hwnd)
                break;

    case WM_QUERYNEWPALETTE:

        if (!p->hdd)
        break;

            hdc = GetDC(hwnd);

        //
        // Realize the palette of the first video stream
        // !!! If first stream isn't video, we're DEAD!
        //
            if (f = DrawDibRealize(p->hdd, hdc, FALSE))
                InvalidateRect(hwnd, NULL, FALSE);

            ReleaseDC(hwnd, hdc);

            return f;

    case WM_PAINT:
        if (!p->hdd)
        break;
        // Paint everybody else before the Preview window since that'll
        // take awhile, and we don't want an ugly window during it.
        DefWindowProc(hwnd, msg, wParam, lParam);
        UpdateWindow(hwnd);
        Preview(hwnd, p, TRUE);
        return 0;

        case WM_HSCROLL:
#ifdef WIN32
            id = GetWindowLong((HWND)HIWORD(lParam), GWL_ID);
#else
            id = GetWindowWord((HWND)HIWORD(lParam), GWW_ID);
#endif
            pos = GetScrollPos((HWND)HIWORD(lParam), SB_CTL);

            switch (wParam)
            {
                case SB_LINEDOWN:       pos += 1; break;
                case SB_LINEUP:         pos -= 1; break;
                case SB_PAGEDOWN:       pos += (id == ID_QUALITY) ? 10 :
                    (int)p->info.dwLength / 10; break;
                case SB_PAGEUP:         pos -= (id == ID_QUALITY) ? 10 :
                    (int)p->info.dwLength / 10; break;
                case SB_THUMBTRACK:
                case SB_THUMBPOSITION:  pos = LOWORD(lParam); break;
        case SB_ENDSCROLL:
            Preview(hwnd, p, TRUE); // Draw this compressed frame
            return TRUE;    // don't fall through and invalidate
                default:
                    return TRUE;
            }

        if (id == ID_QUALITY) {
                if (pos < 0)
                    pos = 0;
                if (pos > (ICQUALITY_HIGH/100))
                    pos = (ICQUALITY_HIGH/100);
                SetDlgItemInt(hwnd, ID_QUALITYTEXT, pos, FALSE);
                SetScrollPos((HWND)HIWORD(lParam), SB_CTL, pos, TRUE);

            } else if (id == ID_PREVIEWSCROLL) {

        // !!! round off !!!
                if (pos < (int)p->info.dwStart)
                    pos = (int)p->info.dwStart;
                if (pos >= (int)p->info.dwStart + (int)p->info.dwLength)
                    pos = (int)(p->info.dwStart + p->info.dwLength - 1);
                SetScrollPos((HWND)HIWORD(lParam), SB_CTL, pos, TRUE);

                LoadString (ghInst, ID_FRAME, achT, sizeof(achT));
        wsprintf(ach, achT, pos);
        SetDlgItemText(hwnd, ID_PREVIEWTEXT, ach);

        //Drawing while scrolling flashes palettes because they aren't
        //compressed.
        //Preview(hwnd, p, FALSE);
        }

            break;

        case WM_COMMAND:
            hwndC = GetDlgItem(hwnd, ID_COMPRESSOR);
            n = ComboBox_GetCurSel(hwndC);
            hic = (n == -1) ? NULL : (HIC)LOWORD(ComboBox_GetItemData(hwndC,n));
        if (!p->fClosing)
        p->hic = hic;

            switch ((int)wParam)
            {
        // When data rate box loses focus, update our preview
        case ID_DATARATE:
            if (HIWORD(lParam) == EN_KILLFOCUS)
            Preview(hwnd, p, TRUE);
            break;

                case ID_COMPRESSOR:
                    if (HIWORD(lParam) != CBN_SELCHANGE)
                        break;

                    if ((int)p->hic > 0) {
                        ICGetInfo(p->hic, &p->icinfo, sizeof(p->icinfo));

                        fConfig  = (BOOL)ICQueryConfigure(p->hic);
                        fAbout   = ICQueryAbout(p->hic);
                        fQuality = (p->icinfo.dwFlags & VIDCF_QUALITY) != 0;
                        fKey     = (p->icinfo.dwFlags & VIDCF_TEMPORAL) != 0;
            // if they do quality we fake crunch
                        fDataRate= (p->icinfo.dwFlags &
                    (VIDCF_QUALITY|VIDCF_CRUNCH)) != 0;
            } else {
            fConfig = fAbout = fQuality = fKey = fDataRate = FALSE;
            }

                    EnableWindow(GetDlgItem(hwnd, ID_CONFIG), fConfig);
                    EnableWindow(GetDlgItem(hwnd, ID_ABOUT), fAbout);
                    EnableWindow(GetDlgItem(hwnd, ID_QUALITY), fQuality);
                    EnableWindow(GetDlgItem(hwnd, ID_QUALITYLABEL), fQuality);
                    EnableWindow(GetDlgItem(hwnd, ID_QUALITYTEXT), fQuality);
                    EnableWindow(GetDlgItem(hwnd, ID_KEYFRAMEBOX), fKey);
                    EnableWindow(GetDlgItem(hwnd, ID_KEYFRAME), fKey);
                    EnableWindow(GetDlgItem(hwnd, ID_KEYFRAMETEXT), fKey);
                    EnableWindow(GetDlgItem(hwnd, ID_DATARATEBOX), fDataRate);
                    EnableWindow(GetDlgItem(hwnd, ID_DATARATE), fDataRate);
                    EnableWindow(GetDlgItem(hwnd, ID_DATARATETEXT), fDataRate);

                    if (fQuality)
            {
            if (p->lQ == ICQUALITY_DEFAULT && (int)p->hic > 0)
            {
                SetScrollPos(GetDlgItem(hwnd, ID_QUALITY), SB_CTL,
                (int)ICGetDefaultQuality(p->hic) / 100, TRUE);
            }
            else
            {
                SetScrollPos(GetDlgItem(hwnd, ID_QUALITY), SB_CTL,
                (int)p->lQ / 100, TRUE);
            }

            pos = GetScrollPos(GetDlgItem(hwnd, ID_QUALITY),SB_CTL);
            SetDlgItemInt(hwnd, ID_QUALITYTEXT, pos, FALSE);
            }

            // redraw with new compressor
            Preview(hwnd, p, TRUE);

                    break;

                case ID_CONFIG:
                    if ((int)p->hic > 0) {
                        ICConfigure(p->hic, hwnd);
            Preview(hwnd, p, TRUE);
            }
                    break;

                case ID_ABOUT:
                    if ((int)p->hic > 0)
                        ICAbout(p->hic, hwnd);
                    break;

        case ID_PREVIEW:
            ShowWindow(GetDlgItem(hwnd, ID_PREVIEW), SW_HIDE);
            ShowWindow(GetDlgItem(hwnd, ID_PREVIEWWIN), SW_SHOW);
            ShowWindow(GetDlgItem(hwnd, ID_PREVIEWSCROLL), SW_SHOW);
            ShowWindow(GetDlgItem(hwnd, ID_PREVIEWTEXT), SW_SHOW);
            SizeDialog(hwnd, ID_CHOOSE_BIG);
            // !!! truncation
                SetScrollRange(GetDlgItem(hwnd, ID_PREVIEWSCROLL), SB_CTL,
            (int)p->info.dwStart,
            (int)(p->info.dwStart + p->info.dwLength - 1),
            FALSE);
                    SetScrollPos(GetDlgItem(hwnd, ID_PREVIEWSCROLL), SB_CTL,
            (int)p->info.dwStart, TRUE);
                    LoadString (ghInst, ID_FRAME, achT, sizeof(achT));
            wsprintf(ach, achT, p->info.dwStart);
            SetDlgItemText(hwnd, ID_PREVIEWTEXT, ach);
            InitPreview(hwnd, p);
            break;

                case IDOK:

            // !!! We need to call ICInfo to get the FOURCC used
            // in system.ini.  Calling ICGetInfo will return the
            // FOURCC the compressor thinks it is, which won't
            // work.  Compman is stupid.
            // Get the HIWORD before we nuke it.
                    i = HIWORD(ComboBox_GetItemData(hwndC, n));

            //
            // Don't close the current compressor in our CANCEL loop
            //
                    ComboBox_SetItemData(hwndC, n, 0);

            //
            // Return the values of the dlg to the caller
            //
                    p->hic = hic;

                    p->lQ = 100 *
            GetScrollPos(GetDlgItem(hwnd, ID_QUALITY), SB_CTL);

            if (IsDlgButtonChecked(hwnd, ID_KEYFRAMEBOX))
                p->lKey = GetDlgItemInt(hwnd, ID_KEYFRAME, NULL, FALSE);
            else
            p->lKey = 0;

            if (IsDlgButtonChecked(hwnd, ID_DATARATEBOX))
                p->lDataRate = GetDlgItemInt(hwnd, ID_DATARATE, NULL,
                FALSE);
            else
            p->lDataRate = 0;

            // We've chosen a valid compressor.  Do stuff.
            if ((int)p->hic > 0) {

                // !!! We need to call ICInfo to get the FOURCC used
                // in system.ini.  Calling ICGetInfo will return the
                // FOURCC the compressor thinks it is, which won't
                // work.  Compman is stupid.
                        ICInfo(p->fccType, i, &p->icinfo);
                p->fccHandler = p->icinfo.fccHandler;   // identify it

            // Free the old state
            if (p->lpState)
                GlobalFreePtr(p->lpState);
            p->lpState = NULL;
            // Get the new state
            p->cbState = ICGetStateSize(p->hic);
            if (p->cbState) {   // Remember it's config state
                p->lpState = GlobalAllocPtr(GMEM_MOVEABLE,
                p->cbState);
                if (p->lpState) {
                ICGetState(p->hic, p->lpState, p->cbState);
                }
            }
            } else if ((int)p->hic == -1) { // "Full Frames"
            p->fccHandler = comptypeDIB;
            p->hic = 0;
            } else {                // "No Compression"
            p->fccHandler = 0L;
            p->hic = 0;
            }

                    // fall through

                case IDCANCEL:
            p->fClosing = TRUE;

            if (wParam == IDCANCEL)
                p->hic = NULL;

                    n = ComboBox_GetCount(hwndC);
                    for (i=0; i<n; i++)
                    {
                        if ((int)(hic =
                (HIC)LOWORD(ComboBox_GetItemData(hwndC,i))) > 0)
                            ICClose(hic);
                    }

            TermPreview(p);
            if (p->pgf)
            AVIStreamGetFrameClose(p->pgf);
            p->pgf = NULL;
                    EndDialog(hwnd, wParam == IDOK);
                    break;
            }
            break;
    }

    return FALSE;
}




/*****************************************************************************
 *
 * dprintf() is called by the DPF macro if DEBUG is defined at compile time.
 *
 * The messages will be send to COM1: like any debug message. To
 * enable debug output, add the following to WIN.INI :
 *
 * [debug]
 * ICSAMPLE=1
 *
 ****************************************************************************/

#ifdef DEBUG

static BOOL  fDebug = -1;

#define MODNAME "ICM"

static void cdecl dprintf(LPSTR szFormat, ...)
{
    char ach[128];

#ifdef WIN32
    va_list va;
    if (fDebug == -1)
        fDebug = GetProfileIntA("Debug",MODNAME, FALSE);

    if (!fDebug)
        return;

    va_start(va, szFormat);
    if (szFormat[0] == '!')
        ach[0]=0, szFormat++;
    else
        lstrcpyA(ach, MODNAME ": ");

    wvsprintfA(ach+lstrlenA(ach),szFormat,(LPSTR)va);
    va_end(va);
//  lstrcat(ach, "\r\r\n");
#else
    if (fDebug == -1)
        fDebug = GetProfileInt("Debug",MODNAME, FALSE);

    if (!fDebug)
        return;

    if (szFormat[0] == '!')
        ach[0]=0, szFormat++;
    else
        lstrcpy(ach, MODNAME ": ");

    wvsprintf(ach+lstrlen(ach),szFormat,(LPSTR)(&szFormat+1));
//  lstrcat(ach, "\r\r\n");
#endif

    OutputDebugStringA(ach);
}

#endif
