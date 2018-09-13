//---------------------------------------------------------------
//  File: pcximage.cpp
//
//  Image manipulation functions for PCX format images.
//---------------------------------------------------------------
#include "stdafx.h"
#include "global.h"
#include "pbrush.h"
#include "pbrusfrm.h"
#include "pbrusvw.h"
#include "minifwnd.h"
#include "bmobject.h"
#include "imgsuprt.h"
#include "imgwnd.h"
#include "imgbrush.h"
#include "imgwell.h"
#include "imgtools.h"
#include "toolbox.h"
#include "imgfile.h"
#include "imgcolor.h"
#include "undo.h"
#include "props.h"
#include "ferr.h"
#include "ctype.h"
#include "cmpmsg.h"

#define COLORMAPLENGTH 48
#define FILLERLENGTH 58

#ifdef PCX_SUPPORT

struct PCXHeader
    {
    unsigned char   manufacturer;
    unsigned char   version;
    unsigned char   encoding;
    unsigned char   bits_per_pixel_per_plane;
    short           xmin;
    short           ymin;
    short           xmax;
    short           ymax;
    unsigned short  hresolution;
    unsigned short  vresolution;
    unsigned char   colormap[COLORMAPLENGTH];
    unsigned char   reserved;
    unsigned char   nplanes;
    unsigned short  bytes_per_line;
    short           palette_info;
    unsigned char   filler[FILLERLENGTH];   // Header is 128 bytes
    };

#endif

class CFileBuffer : public CObject
    {
    DECLARE_DYNCREATE( CFileBuffer )

    public:

    enum Type
        {
        READ,
        WRITE
        };


    CFileBuffer();
   ~CFileBuffer();

    BOOL  Create( CFile* pfile, Type IO );
    short Get   ( void );
    BOOL  Put   ( BYTE cByte );
    long  Seek  ( long lOff, UINT nFrom );
    BOOL  Flush ( void );

    private:

    void  Fill  ( void );

    enum { MAX_BUFFER = 2048 };

    CFile*      m_pFile;
    int         m_iBuffPos;
    int         m_iBuffSize;
    BYTE*       m_pBuffer;
    };

IMPLEMENT_DYNCREATE( CFileBuffer, CObject )

#include "memtrace.h"

/****************************************************************************/

CFileBuffer::CFileBuffer() : CObject()
    {
    m_pFile     = 0;
    m_iBuffPos  = 0;
    m_iBuffSize = 0;
    m_pBuffer   = 0;
    }

/****************************************************************************/

CFileBuffer::~CFileBuffer()
    {
    if (m_pBuffer)
        delete [] m_pBuffer;
    }

/****************************************************************************/

BOOL CFileBuffer::Create( CFile* pfile, Type IO )
    {
    ASSERT( pfile != NULL );

    if (pfile == NULL)
        return FALSE;

    m_pFile   = pfile;
    m_pBuffer = new BYTE[MAX_BUFFER];

    if (! m_pBuffer)
        {
        theApp.SetMemoryEmergency();
        return FALSE;
        }

    if (IO == READ)
        {
        Fill();

        if (! m_iBuffSize)
            {
            theApp.SetFileError( IDS_ERROR_READLOAD, ferrIllformedFile );
            return FALSE;
            }
        }
    return TRUE;
    }

/****************************************************************************/

short CFileBuffer::Get( void )
    {
    if (! m_iBuffSize)
        return EOF;

    short sByte = (short)(unsigned short)m_pBuffer[m_iBuffPos++];

    if (m_iBuffPos == m_iBuffSize)
        Fill();

    return sByte;
    }

/****************************************************************************/

BOOL CFileBuffer::Put( BYTE cByte )
    {
    m_pBuffer[m_iBuffSize++] = cByte;

    if (m_iBuffSize == MAX_BUFFER)
        return Flush();

    return TRUE;
    }

/****************************************************************************/

long CFileBuffer::Seek( long lOff, UINT nFrom )
    {
    long lPos = m_pFile->Seek( lOff, nFrom );

    Fill();

    return lPos;
    }

/****************************************************************************/

void CFileBuffer::Fill()
    {
    m_iBuffSize = m_pFile->Read( m_pBuffer, MAX_BUFFER );
    m_iBuffPos  = 0;
    }

/****************************************************************************/

BOOL CFileBuffer::Flush( void )
    {
    TRY {
        m_pFile->Write( m_pBuffer, m_iBuffSize );
        }
    CATCH( CFileException, ex )
        {
        m_pFile->Abort();
        theApp.SetFileError( IDS_ERROR_SAVE, ex->m_cause );

        return FALSE;
        }
    END_CATCH

    m_iBuffSize = 0;

    return TRUE;
    }

/****************************************************************************/
/****************************************************************************/
#ifdef PCX_SUPPORT

BOOL CBitmapObj::ReadPCX( CFile* pfile )
    {
    if (! pfile->GetLength())
        {
        if (m_lpvThing)
            Free();

        m_bDirty = TRUE;

        return TRUE;
        }

    // if  a PCX extension try to load this as a PCX image.
    PCXHeader hdr;
    PBITMAP   p_dib;   // Device independent bitmap

    short bytes_per_line;

    pfile->Read( (unsigned char*)&hdr, sizeof( PCXHeader ) );

    // Check if image file format is acceptable

    if (hdr.manufacturer != 0x0a)
        {
        theApp.SetFileError( IDS_ERROR_READLOAD, ferrCantDetermineType );
        return FALSE;
        }

    // We only handle 1, 4, 8, or 24-bit images

    short bits_per_pixel = hdr.nplanes * hdr.bits_per_pixel_per_plane;

    if (bits_per_pixel != 1
    &&  bits_per_pixel != 4
    &&  bits_per_pixel != 8
    &&  bits_per_pixel != 24)
        {
        theApp.SetFileError( IDS_ERROR_READLOAD, ferrCantDetermineType );
        return FALSE;
        }

    short image_width  = hdr.xmax - hdr.xmin + 1;
    short image_height = hdr.ymax - hdr.ymin + 1;

    // Allocate space where the PCX image will be unpacked.

    long pcx_image_size = (long) hdr.nplanes *
                          (long) image_height *
                          (long) hdr.bytes_per_line;

    BYTE* image = (BYTE*) new BYTE[pcx_image_size];

    if (image == NULL)
        {
        theApp.SetMemoryEmergency();
        return FALSE;
        }

    // Read in PCX image into this area.
    CFileBuffer FileBuffer;

    if (! FileBuffer.Create( pfile, CFileBuffer::READ ))
        {
        delete [] image;

        return FALSE;
        }

    // Decode run-length encoded image data
    short i;
    short byte;
    short count;
    long  pos = 0L;

    while ((byte = FileBuffer.Get()) != EOF)
        {
        if ((byte & 0xc0) == 0xc0)
            {
            count = byte & 0x3f;

            if ((byte = FileBuffer.Get()) != EOF)
                {
                for (i = 0; i < count; i++)
                    {
                    if (pos >= pcx_image_size)
                        break;

                    image[pos] = (CHAR)byte;
                    pos++;
                    }
                }
            }
        else
            {
            if (pos >= pcx_image_size)
                break;

            image[pos] = (CHAR)byte;
            pos++;
            }
        }

    // Allocate memory for the device independent bitmap (DIB)
    // Note that the number of bytes in each line of a DIB image
    // must be a multiple of 4.

    short bytes_per_line_per_plane = (image_width *
                       hdr.bits_per_pixel_per_plane + 7) / 8;

    short actual_bytes_per_line = (image_width *
                                   hdr.nplanes *
                       hdr.bits_per_pixel_per_plane + 7) / 8;
    bytes_per_line = actual_bytes_per_line;

    if ( bytes_per_line % 4)
         bytes_per_line = 4 * ( bytes_per_line / 4 + 1);

    // Make room for a palette

    short palettesize = 16;

    if (bits_per_pixel == 1)
        palettesize = 2;

    if (hdr.version >= 5
    && bits_per_pixel > 4)
        {
        // Go back 769 bytes from the end of the file

        FileBuffer.Seek( -769, CFile::end );

        if (FileBuffer.Get() == 12)
            {
            // There is a 256-color palette following this byte
            palettesize = 256;
            }
        }
    // If image has more than 256 colors then there is no palette

    if (bits_per_pixel > 8)
        palettesize = 0;

    // Allocate space for the bitmap
    if (m_lpvThing)
        Free();

    m_lMemSize = sizeof( BITMAPINFOHEADER ) + palettesize * sizeof( RGBQUAD )
                                   + (long)bytes_per_line * (long)image_height;
    if (! Alloc())
        return FALSE;

    p_dib = (PBITMAP)m_lpvThing;

    // Set up bitmap info header

    LPBITMAPINFOHEADER p_bminfo = (LPBITMAPINFOHEADER)p_dib;

    p_bminfo->biSize          = sizeof(BITMAPINFOHEADER);
    p_bminfo->biWidth         = image_width;
    p_bminfo->biHeight        = image_height;
    p_bminfo->biPlanes        = 1;
    p_bminfo->biBitCount      = hdr.bits_per_pixel_per_plane * hdr.nplanes;
    p_bminfo->biCompression   = BI_RGB;
    p_bminfo->biSizeImage     = (long)image_height * (long) bytes_per_line;
    p_bminfo->biXPelsPerMeter = (long)hdr.hresolution;
    p_bminfo->biYPelsPerMeter = (long)hdr.vresolution;
    p_bminfo->biClrUsed       = 0;
    p_bminfo->biClrImportant  = 0;

    // Set up the color palette

    if (palettesize > 0)
        {
        //***** RGBQUAD *palette = (RGBQUAD*) ((LPSTR)imdata->p_dib

        LPRGBQUAD palette = LPRGBQUAD((LPSTR)p_dib + sizeof(BITMAPINFOHEADER));

        short palindex;

        for (palindex = 0; palindex < palettesize; palindex++)
            {
            if (palettesize == 256)
                {
                // Read palette from file

                palette[palindex].rgbRed       = (BYTE)FileBuffer.Get();
                palette[palindex].rgbGreen     = (BYTE)FileBuffer.Get();
                palette[palindex].rgbBlue      = (BYTE)FileBuffer.Get();
                palette[palindex].rgbReserved  = 0;
                }
            if (palettesize == 16)
                {
                // 16-color palette from PCX header

                palette[palindex].rgbRed      = (BYTE)hdr.colormap[3*palindex];
                palette[palindex].rgbGreen    = (BYTE)hdr.colormap[3*palindex+1];
                palette[palindex].rgbBlue     = (BYTE)hdr.colormap[3*palindex+2];
                palette[palindex].rgbReserved = 0;
                }
            if (palettesize == 2)
                {
                // Set up palette for black and white images

                palette[palindex].rgbRed      = palindex * 255;
                palette[palindex].rgbGreen    = palindex * 255;
                palette[palindex].rgbBlue     = palindex * 255;
                palette[palindex].rgbReserved = 0;
                }
            }
        }

    // Load image data into the DIB. Note the DIB image must be
    // stored "bottom to top" line order. That's why we position
    // data at the end of the array so that the image can be
    // stored backwards--from the last line to the first.

    BYTE* data = (BYTE*)p_dib + ((long)sizeof( BITMAPINFOHEADER )
                              + palettesize * sizeof( RGBQUAD )
                              + (image_height - 1) * bytes_per_line);

    // Define a macro to access bytes in the PCX image according
    // to specified line and plane index.

    short lineindex, byteindex, planeindex;

    #define bytepos(lineindex, planeindex, byteindex)  \
            ((long)(lineindex)*(long)hdr.bytes_per_line* \
             (long)hdr.nplanes + \
             (long)(planeindex)*(long)hdr.bytes_per_line + \
             (long)(byteindex))

    // Construct packed pixels out of decoded PCX image.

    short loc;
    unsigned short onebyte;
    unsigned short bits_copied;
    unsigned short few_bits;
    unsigned short k;
    unsigned short bbpb = 8/hdr.bits_per_pixel_per_plane;

    // Build a mask to pick out bits from each byte of the PCX image

    unsigned short himask = 0x80, mask;

    if (hdr.bits_per_pixel_per_plane > 1)
        for (i = 0; i < hdr.bits_per_pixel_per_plane - 1;
            i++) himask = 0x80 | (himask >> 1);

    for (lineindex = 0; lineindex < image_height;
         lineindex++, data -= bytes_per_line)
        {
        if (actual_bytes_per_line < bytes_per_line)
            for (loc = actual_bytes_per_line; loc < bytes_per_line; loc++)
                data[loc] = 0;

        loc         = 0;
        onebyte     = 0;
        bits_copied = 0;

        for (byteindex = 0; byteindex < bytes_per_line_per_plane; byteindex++)
            {
            for (k = 0, mask = himask; k < bbpb; k++,
                                        mask >>= hdr.bits_per_pixel_per_plane)
                {
                // Go through all scan line for all planes and copy bits into
                // the data array

                for (planeindex = 0; planeindex < hdr.nplanes; planeindex++)
                    {
                    few_bits = image[bytepos(lineindex,
                                            planeindex, byteindex)] & mask;

                    // Shift the selected bits to the most significant position

                    if (k > 0)
                        few_bits <<= (k*hdr.bits_per_pixel_per_plane);

                    // OR the bits with current pixel after shifting them right

                    if (bits_copied > 0)
                        few_bits >>= bits_copied;

                    onebyte |= few_bits;
                    bits_copied += hdr.bits_per_pixel_per_plane;

                    if (bits_copied >= 8)
                        {
                        data[loc] = (UCHAR)onebyte;
                        loc++;
                        bits_copied = 0;
                        onebyte = 0;
                        }
                    }
                }
            }
        }

    // Success!
    delete [] (BYTE*)image;

    return TRUE;
    }

/****************************************************************************/
#define WIDTHBYTES(bits) ((((bits) + 31) / 32) * 4)

BOOL CBitmapObj::WritePCX( CFile* pfile )
    {
    if (m_pImg == NULL)
        {
        // The image has not been loaded, so we'll just copy the
        // original out to the file...
        ASSERT( m_lpvThing );

        if (! m_lpvThing)
            return FALSE;
        }
    else
        {
        // The image has been loaded and may have been edited, so
        // we'll convert it back to a dib to save...
        if (! m_lpvThing)
            SaveResource( FALSE );

        if (! m_lpvThing)
            return FALSE;
        }

    // build pcx file from the DIB
    PBITMAP   p_dib = (PBITMAP)m_lpvThing;                   // Device independent bitmap
    PCXHeader hdr;                                           // PCX bitmap header
    LPBITMAPINFOHEADER p_bminfo = (LPBITMAPINFOHEADER)p_dib; // Set up bitmap info header

    short palettesize = DIBNumColors( (LPSTR)m_lpvThing);    // Get palette size

    hdr.manufacturer = 10;
//  hdr.version      = (char)((hPalette || (GetDeviceCaps(fileDC, RASTERCAPS) & RC_PALETTE)) ? 5 : 3);
    hdr.version      = (CHAR)( palettesize ? 5 : 3);
    hdr.encoding     = 1;
    hdr.xmin         = hdr.ymin = 0;
    hdr.xmax         = p_bminfo->biWidth - 1;
    hdr.ymax         = p_bminfo->biHeight- 1;
//  hdr.hresolution  = theApp.ScreenDeviceInfo.iWidthinPels;
//  hdr.vresolution  = theApp.ScreenDeviceInfo.iHeightinPels;
    hdr.hresolution  = (WORD)p_bminfo->biXPelsPerMeter;
    hdr.vresolution  = (WORD)p_bminfo->biYPelsPerMeter;
    hdr.reserved     = 0;
    hdr.nplanes      = (BYTE)p_bminfo->biPlanes; //biPlanes should always be 1
    hdr.palette_info = (BYTE)p_dib->bmWidthBytes;
    hdr.bits_per_pixel_per_plane = (CHAR) p_bminfo->biBitCount;

    hdr.bytes_per_line = WIDTHBYTES( (LONG) (p_bminfo->biBitCount * p_bminfo->biWidth) );

    // Clean up filler
    for (int index = FILLERLENGTH; index--; )
        hdr.filler[index] ='\0';

    //  If there are at most 16 colors place them in header
    LPRGBQUAD palette = LPRGBQUAD((LPSTR)p_dib + sizeof(BITMAPINFOHEADER));
    LPSTR       lpDst = (LPSTR)hdr.colormap;

    // Clean up colormap
    for (index = COLORMAPLENGTH; index--; )
        lpDst[index] ='\0';

    if (palettesize <= 16)
        for (index = palettesize; index--; )
            {
            *lpDst++ = palette->rgbRed;  /* swap RED and BLUE components */
            *lpDst++ = palette->rgbGreen;
            *lpDst++ = palette->rgbBlue;
            palette++;
            }

    pfile->Write( (unsigned char*)&hdr, sizeof( PCXHeader ) );

    // Now pack the image

    // Load image data from the DIB. Note the DIB image is
    // stored "bottom to top" line order. That's why we position
    // data at the end of the array so that the image can be
    // stored backwards--from the last line to the first.

    CFileBuffer FileBuffer;

    if (! FileBuffer.Create( pfile, CFileBuffer::WRITE ))
        return FALSE;

    // find the start of the bitmap data then go to the end of the data
    // the PCX is stored in reverse order of the DIB
    int TopofData = sizeof( BITMAPINFOHEADER ) + palettesize * sizeof( RGBQUAD );
    BYTE* data = (BYTE*)p_dib + TopofData + hdr.bytes_per_line * (p_bminfo->biHeight );

    for (index = p_bminfo->biHeight; index--; )
        {
        data -= hdr.bytes_per_line;

        if (! PackBuff( &FileBuffer, data, hdr.bytes_per_line )) //convert to run length encoding.
            return FALSE;
        }

    if (palettesize == 256) // Write palette to file
        {
        if (! FileBuffer.Put( 12 ))  // Tag number for palette information
            return FALSE;

        for (index = 0; index < palettesize; index++)
            {
            if (! FileBuffer.Put( palette[index].rgbRed   )
            ||  ! FileBuffer.Put( palette[index].rgbGreen )
            ||  ! FileBuffer.Put( palette[index].rgbBlue  ))
                return FALSE;
            }
        }

    return FileBuffer.Flush();
    }

#endif //PCX_SUPPORT

/****************************************************************************/

/* run length encoding equates */
#define MINcount 2
#define MAXcount 63
#define ESCbits  0xC0
#define BUFFER_SIZE 1024

/* bitmaps are ordered <b, g, r, i> but PCX is ordered <r, g, b, i> ... */

BOOL CBitmapObj::PackBuff(CFileBuffer *FileBuffer, BYTE *PtrDib, int byteWidth )
    {
    BYTE  runChar;
    BYTE  runCount;
    BYTE* endPtr = PtrDib + byteWidth;

    for (runCount = 1, runChar = *PtrDib++; PtrDib <= endPtr; ++PtrDib)
        {
        if (PtrDib != endPtr && *PtrDib == runChar && runCount < MAXcount)
            ++runCount;
        else
            if (*PtrDib != runChar
            &&  runCount < MINcount
            && (runChar & ESCbits) != ESCbits)
                {
                while (runCount--)
                    if (! FileBuffer->Put( runChar ))
                        return FALSE;

                runCount = 1;
                runChar = *PtrDib;
                }
            else
                {
                runCount |= ESCbits;

                if (! FileBuffer->Put( runCount )
                ||  ! FileBuffer->Put( runChar  ))
                    return FALSE;

                runCount = 1;
                runChar  = *PtrDib;
                }
        }

    return TRUE;
    }

/****************************************************************************/
