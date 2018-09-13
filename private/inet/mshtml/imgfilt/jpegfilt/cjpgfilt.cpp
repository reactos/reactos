#include "StdAfx.H"
#include "JPEGFilt.H"
#include "Resource.H"
#include "CJPGFilt.H"

#undef  DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

DEFINE_GUID( IID_IDirectDrawSurface,		0x6C14DB81,0xA733,0x11CE,0xA5,0x21,0x00,
0x20,0xAF,0x0B,0xE5,0x60 );


RGBQUAD g_argbGrayScale[] =
{
    { 0x00, 0x00, 0x00, 0x00 },
    { 0x01, 0x01, 0x01, 0x00 },
    { 0x02, 0x02, 0x02, 0x00 },
    { 0x03, 0x03, 0x03, 0x00 },
    { 0x04, 0x04, 0x04, 0x00 },
    { 0x05, 0x05, 0x05, 0x00 },
    { 0x06, 0x06, 0x06, 0x00 },
    { 0x07, 0x07, 0x07, 0x00 },
    { 0x08, 0x08, 0x08, 0x00 },
    { 0x09, 0x09, 0x09, 0x00 },
    { 0x0A, 0x0A, 0x0A, 0x00 },
    { 0x0B, 0x0B, 0x0B, 0x00 },
    { 0x0C, 0x0C, 0x0C, 0x00 },
    { 0x0D, 0x0D, 0x0D, 0x00 },
    { 0x0E, 0x0E, 0x0E, 0x00 },
    { 0x0F, 0x0F, 0x0F, 0x00 },
    { 0x10, 0x10, 0x10, 0x00 },
    { 0x11, 0x11, 0x11, 0x00 },
    { 0x12, 0x12, 0x12, 0x00 },
    { 0x13, 0x13, 0x13, 0x00 },
    { 0x14, 0x14, 0x14, 0x00 },
    { 0x15, 0x15, 0x15, 0x00 },
    { 0x16, 0x16, 0x16, 0x00 },
    { 0x17, 0x17, 0x17, 0x00 },
    { 0x18, 0x18, 0x18, 0x00 },
    { 0x19, 0x19, 0x19, 0x00 },
    { 0x1A, 0x1A, 0x1A, 0x00 },
    { 0x1B, 0x1B, 0x1B, 0x00 },
    { 0x1C, 0x1C, 0x1C, 0x00 },
    { 0x1D, 0x1D, 0x1D, 0x00 },
    { 0x1E, 0x1E, 0x1E, 0x00 },
    { 0x1F, 0x1F, 0x1F, 0x00 },
    { 0x20, 0x20, 0x20, 0x00 },
    { 0x21, 0x21, 0x21, 0x00 },
    { 0x22, 0x22, 0x22, 0x00 },
    { 0x23, 0x23, 0x23, 0x00 },
    { 0x24, 0x24, 0x24, 0x00 },
    { 0x25, 0x25, 0x25, 0x00 },
    { 0x26, 0x26, 0x26, 0x00 },
    { 0x27, 0x27, 0x27, 0x00 },
    { 0x28, 0x28, 0x28, 0x00 },
    { 0x29, 0x29, 0x29, 0x00 },
    { 0x2A, 0x2A, 0x2A, 0x00 },
    { 0x2B, 0x2B, 0x2B, 0x00 },
    { 0x2C, 0x2C, 0x2C, 0x00 },
    { 0x2D, 0x2D, 0x2D, 0x00 },
    { 0x2E, 0x2E, 0x2E, 0x00 },
    { 0x2F, 0x2F, 0x2F, 0x00 },
    { 0x30, 0x30, 0x30, 0x00 },
    { 0x31, 0x31, 0x31, 0x00 },
    { 0x32, 0x32, 0x32, 0x00 },
    { 0x33, 0x33, 0x33, 0x00 },
    { 0x34, 0x34, 0x34, 0x00 },
    { 0x35, 0x35, 0x35, 0x00 },
    { 0x36, 0x36, 0x36, 0x00 },
    { 0x37, 0x37, 0x37, 0x00 },
    { 0x38, 0x38, 0x38, 0x00 },
    { 0x39, 0x39, 0x39, 0x00 },
    { 0x3A, 0x3A, 0x3A, 0x00 },
    { 0x3B, 0x3B, 0x3B, 0x00 },
    { 0x3C, 0x3C, 0x3C, 0x00 },
    { 0x3D, 0x3D, 0x3D, 0x00 },
    { 0x3E, 0x3E, 0x3E, 0x00 },
    { 0x3F, 0x3F, 0x3F, 0x00 },
    { 0x40, 0x40, 0x40, 0x00 },
    { 0x41, 0x41, 0x41, 0x00 },
    { 0x42, 0x42, 0x42, 0x00 },
    { 0x43, 0x43, 0x43, 0x00 },
    { 0x44, 0x44, 0x44, 0x00 },
    { 0x45, 0x45, 0x45, 0x00 },
    { 0x46, 0x46, 0x46, 0x00 },
    { 0x47, 0x47, 0x47, 0x00 },
    { 0x48, 0x48, 0x48, 0x00 },
    { 0x49, 0x49, 0x49, 0x00 },
    { 0x4A, 0x4A, 0x4A, 0x00 },
    { 0x4B, 0x4B, 0x4B, 0x00 },
    { 0x4C, 0x4C, 0x4C, 0x00 },
    { 0x4D, 0x4D, 0x4D, 0x00 },
    { 0x4E, 0x4E, 0x4E, 0x00 },
    { 0x4F, 0x4F, 0x4F, 0x00 },
    { 0x50, 0x50, 0x50, 0x00 },
    { 0x51, 0x51, 0x51, 0x00 },
    { 0x52, 0x52, 0x52, 0x00 },
    { 0x53, 0x53, 0x53, 0x00 },
    { 0x54, 0x54, 0x54, 0x00 },
    { 0x55, 0x55, 0x55, 0x00 },
    { 0x56, 0x56, 0x56, 0x00 },
    { 0x57, 0x57, 0x57, 0x00 },
    { 0x58, 0x58, 0x58, 0x00 },
    { 0x59, 0x59, 0x59, 0x00 },
    { 0x5A, 0x5A, 0x5A, 0x00 },
    { 0x5B, 0x5B, 0x5B, 0x00 },
    { 0x5C, 0x5C, 0x5C, 0x00 },
    { 0x5D, 0x5D, 0x5D, 0x00 },
    { 0x5E, 0x5E, 0x5E, 0x00 },
    { 0x5F, 0x5F, 0x5F, 0x00 },
    { 0x60, 0x60, 0x60, 0x00 },
    { 0x61, 0x61, 0x61, 0x00 },
    { 0x62, 0x62, 0x62, 0x00 },
    { 0x63, 0x63, 0x63, 0x00 },
    { 0x64, 0x64, 0x64, 0x00 },
    { 0x65, 0x65, 0x65, 0x00 },
    { 0x66, 0x66, 0x66, 0x00 },
    { 0x67, 0x67, 0x67, 0x00 },
    { 0x68, 0x68, 0x68, 0x00 },
    { 0x69, 0x69, 0x69, 0x00 },
    { 0x6A, 0x6A, 0x6A, 0x00 },
    { 0x6B, 0x6B, 0x6B, 0x00 },
    { 0x6C, 0x6C, 0x6C, 0x00 },
    { 0x6D, 0x6D, 0x6D, 0x00 },
    { 0x6E, 0x6E, 0x6E, 0x00 },
    { 0x6F, 0x6F, 0x6F, 0x00 },
    { 0x70, 0x70, 0x70, 0x00 },
    { 0x71, 0x71, 0x71, 0x00 },
    { 0x72, 0x72, 0x72, 0x00 },
    { 0x73, 0x73, 0x73, 0x00 },
    { 0x74, 0x74, 0x74, 0x00 },
    { 0x75, 0x75, 0x75, 0x00 },
    { 0x76, 0x76, 0x76, 0x00 },
    { 0x77, 0x77, 0x77, 0x00 },
    { 0x78, 0x78, 0x78, 0x00 },
    { 0x79, 0x79, 0x79, 0x00 },
    { 0x7A, 0x7A, 0x7A, 0x00 },
    { 0x7B, 0x7B, 0x7B, 0x00 },
    { 0x7C, 0x7C, 0x7C, 0x00 },
    { 0x7D, 0x7D, 0x7D, 0x00 },
    { 0x7E, 0x7E, 0x7E, 0x00 },
    { 0x7F, 0x7F, 0x7F, 0x00 },
    { 0x80, 0x80, 0x80, 0x00 },
    { 0x81, 0x81, 0x81, 0x00 },
    { 0x82, 0x82, 0x82, 0x00 },
    { 0x83, 0x83, 0x83, 0x00 },
    { 0x84, 0x84, 0x84, 0x00 },
    { 0x85, 0x85, 0x85, 0x00 },
    { 0x86, 0x86, 0x86, 0x00 },
    { 0x87, 0x87, 0x87, 0x00 },
    { 0x88, 0x88, 0x88, 0x00 },
    { 0x89, 0x89, 0x89, 0x00 },
    { 0x8A, 0x8A, 0x8A, 0x00 },
    { 0x8B, 0x8B, 0x8B, 0x00 },
    { 0x8C, 0x8C, 0x8C, 0x00 },
    { 0x8D, 0x8D, 0x8D, 0x00 },
    { 0x8E, 0x8E, 0x8E, 0x00 },
    { 0x8F, 0x8F, 0x8F, 0x00 },
    { 0x90, 0x90, 0x90, 0x00 },
    { 0x91, 0x91, 0x91, 0x00 },
    { 0x92, 0x92, 0x92, 0x00 },
    { 0x93, 0x93, 0x93, 0x00 },
    { 0x94, 0x94, 0x94, 0x00 },
    { 0x95, 0x95, 0x95, 0x00 },
    { 0x96, 0x96, 0x96, 0x00 },
    { 0x97, 0x97, 0x97, 0x00 },
    { 0x98, 0x98, 0x98, 0x00 },
    { 0x99, 0x99, 0x99, 0x00 },
    { 0x9A, 0x9A, 0x9A, 0x00 },
    { 0x9B, 0x9B, 0x9B, 0x00 },
    { 0x9C, 0x9C, 0x9C, 0x00 },
    { 0x9D, 0x9D, 0x9D, 0x00 },
    { 0x9E, 0x9E, 0x9E, 0x00 },
    { 0x9F, 0x9F, 0x9F, 0x00 },
    { 0xA0, 0xA0, 0xA0, 0x00 },
    { 0xA1, 0xA1, 0xA1, 0x00 },
    { 0xA2, 0xA2, 0xA2, 0x00 },
    { 0xA3, 0xA3, 0xA3, 0x00 },
    { 0xA4, 0xA4, 0xA4, 0x00 },
    { 0xA5, 0xA5, 0xA5, 0x00 },
    { 0xA6, 0xA6, 0xA6, 0x00 },
    { 0xA7, 0xA7, 0xA7, 0x00 },
    { 0xA8, 0xA8, 0xA8, 0x00 },
    { 0xA9, 0xA9, 0xA9, 0x00 },
    { 0xAA, 0xAA, 0xAA, 0x00 },
    { 0xAB, 0xAB, 0xAB, 0x00 },
    { 0xAC, 0xAC, 0xAC, 0x00 },
    { 0xAD, 0xAD, 0xAD, 0x00 },
    { 0xAE, 0xAE, 0xAE, 0x00 },
    { 0xAF, 0xAF, 0xAF, 0x00 },
    { 0xB0, 0xB0, 0xB0, 0x00 },
    { 0xB1, 0xB1, 0xB1, 0x00 },
    { 0xB2, 0xB2, 0xB2, 0x00 },
    { 0xB3, 0xB3, 0xB3, 0x00 },
    { 0xB4, 0xB4, 0xB4, 0x00 },
    { 0xB5, 0xB5, 0xB5, 0x00 },
    { 0xB6, 0xB6, 0xB6, 0x00 },
    { 0xB7, 0xB7, 0xB7, 0x00 },
    { 0xB8, 0xB8, 0xB8, 0x00 },
    { 0xB9, 0xB9, 0xB9, 0x00 },
    { 0xBA, 0xBA, 0xBA, 0x00 },
    { 0xBB, 0xBB, 0xBB, 0x00 },
    { 0xBC, 0xBC, 0xBC, 0x00 },
    { 0xBD, 0xBD, 0xBD, 0x00 },
    { 0xBE, 0xBE, 0xBE, 0x00 },
    { 0xBF, 0xBF, 0xBF, 0x00 },
    { 0xC0, 0xC0, 0xC0, 0x00 },
    { 0xC1, 0xC1, 0xC1, 0x00 },
    { 0xC2, 0xC2, 0xC2, 0x00 },
    { 0xC3, 0xC3, 0xC3, 0x00 },
    { 0xC4, 0xC4, 0xC4, 0x00 },
    { 0xC5, 0xC5, 0xC5, 0x00 },
    { 0xC6, 0xC6, 0xC6, 0x00 },
    { 0xC7, 0xC7, 0xC7, 0x00 },
    { 0xC8, 0xC8, 0xC8, 0x00 },
    { 0xC9, 0xC9, 0xC9, 0x00 },
    { 0xCA, 0xCA, 0xCA, 0x00 },
    { 0xCB, 0xCB, 0xCB, 0x00 },
    { 0xCC, 0xCC, 0xCC, 0x00 },
    { 0xCD, 0xCD, 0xCD, 0x00 },
    { 0xCE, 0xCE, 0xCE, 0x00 },
    { 0xCF, 0xCF, 0xCF, 0x00 },
    { 0xD0, 0xD0, 0xD0, 0x00 },
    { 0xD1, 0xD1, 0xD1, 0x00 },
    { 0xD2, 0xD2, 0xD2, 0x00 },
    { 0xD3, 0xD3, 0xD3, 0x00 },
    { 0xD4, 0xD4, 0xD4, 0x00 },
    { 0xD5, 0xD5, 0xD5, 0x00 },
    { 0xD6, 0xD6, 0xD6, 0x00 },
    { 0xD7, 0xD7, 0xD7, 0x00 },
    { 0xD8, 0xD8, 0xD8, 0x00 },
    { 0xD9, 0xD9, 0xD9, 0x00 },
    { 0xDA, 0xDA, 0xDA, 0x00 },
    { 0xDB, 0xDB, 0xDB, 0x00 },
    { 0xDC, 0xDC, 0xDC, 0x00 },
    { 0xDD, 0xDD, 0xDD, 0x00 },
    { 0xDE, 0xDE, 0xDE, 0x00 },
    { 0xDF, 0xDF, 0xDF, 0x00 },
    { 0xE0, 0xE0, 0xE0, 0x00 },
    { 0xE1, 0xE1, 0xE1, 0x00 },
    { 0xE2, 0xE2, 0xE2, 0x00 },
    { 0xE3, 0xE3, 0xE3, 0x00 },
    { 0xE4, 0xE4, 0xE4, 0x00 },
    { 0xE5, 0xE5, 0xE5, 0x00 },
    { 0xE6, 0xE6, 0xE6, 0x00 },
    { 0xE7, 0xE7, 0xE7, 0x00 },
    { 0xE8, 0xE8, 0xE8, 0x00 },
    { 0xE9, 0xE9, 0xE9, 0x00 },
    { 0xEA, 0xEA, 0xEA, 0x00 },
    { 0xEB, 0xEB, 0xEB, 0x00 },
    { 0xEC, 0xEC, 0xEC, 0x00 },
    { 0xED, 0xED, 0xED, 0x00 },
    { 0xEE, 0xEE, 0xEE, 0x00 },
    { 0xEF, 0xEF, 0xEF, 0x00 },
    { 0xF0, 0xF0, 0xF0, 0x00 },
    { 0xF1, 0xF1, 0xF1, 0x00 },
    { 0xF2, 0xF2, 0xF2, 0x00 },
    { 0xF3, 0xF3, 0xF3, 0x00 },
    { 0xF4, 0xF4, 0xF4, 0x00 },
    { 0xF5, 0xF5, 0xF5, 0x00 },
    { 0xF6, 0xF6, 0xF6, 0x00 },
    { 0xF7, 0xF7, 0xF7, 0x00 },
    { 0xF8, 0xF8, 0xF8, 0x00 },
    { 0xF9, 0xF9, 0xF9, 0x00 },
    { 0xFA, 0xFA, 0xFA, 0x00 },
    { 0xFB, 0xFB, 0xFB, 0x00 },
    { 0xFC, 0xFC, 0xFC, 0x00 },
    { 0xFD, 0xFD, 0xFD, 0x00 },
    { 0xFE, 0xFE, 0xFE, 0x00 },
    { 0xFF, 0xFF, 0xFF, 0x00 },
};


/////////////////////////////////////////////////////////////////////////////
//

void CopyPaletteEntriesFromColors(PALETTEENTRY *ppe, const RGBQUAD *prgb,
    UINT uCount)
{
    while (uCount--)
    {
        ppe->peRed   = prgb->rgbRed;
        ppe->peGreen = prgb->rgbGreen;
        ppe->peBlue  = prgb->rgbBlue;
        ppe->peFlags = 0;

        prgb++;
        ppe++;
    }
}

CJPEGFilter::CJPEGFilter() :
   m_pStream( NULL ),
   m_iScanLine( 0 ),
   m_iFirstStaleScanLine( 0 ),
   m_pSource( NULL ),
   m_pbScanLines( NULL ),
   m_bRGB8Allowed( FALSE )
{
}

CJPEGFilter::~CJPEGFilter()
{
}

HRESULT CJPEGFilter::FinalConstruct()
{
   HRESULT hResult;

   m_pSource = new CJPEGSource( this );
   if( m_pSource == NULL )
   {
      return( E_OUTOFMEMORY );
   }

   hResult = m_pSource->Init();
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   return( S_OK );
}

extern "C" void error_exit( j_common_ptr pInfo )
{
   (void)pInfo;
}

extern "C" void output_message( j_common_ptr pInfo )
{
   (void)pInfo;
}

extern "C" void emit_message( j_common_ptr pInfo, int iMessageLevel )
{
   (void)pInfo;
   (void)iMessageLevel;
}

extern "C" void format_message( j_common_ptr pInfo, char* pszBuffer )
{
   (void)pInfo;
   (void)pszBuffer;
}

extern "C" void reset_error_mgr( j_common_ptr pInfo )
{
   pInfo->err->num_warnings = 0;
   pInfo->err->msg_code = 0;
}

extern "C" struct jpeg_error_mgr *
jpeg_std_error (struct jpeg_error_mgr * err)
{
  err->error_exit = error_exit;
  err->emit_message = emit_message;
  err->output_message = output_message;
  err->format_message = format_message;
  err->reset_error_mgr = reset_error_mgr;

  err->trace_level = 0;     /* default = no tracing */
  err->num_warnings = 0;    /* no warnings emitted yet */
  err->msg_code = 0;        /* may be useful as a flag for "no error" */

  /* Initialize message table pointers */
  err->jpeg_message_table = NULL;
  err->last_jpeg_message = 0;
//  err->last_jpeg_message = (int) JMSG_LASTMSGCODE - 1;

  err->addon_message_table = NULL;
  err->first_addon_message = 0; /* for safety */
  err->last_addon_message = 0;

  return err;
}

CJPEGSource::CJPEGSource( CJPEGFilter* pFilter ) :
   m_pFilter( pFilter ),
   m_pbBuffer( NULL )
{
   _ASSERTE( m_pFilter != NULL );
}

CJPEGSource::~CJPEGSource()
{
   delete m_pbBuffer;
}

#define INPUT_BUF_SIZE  4096    /* choose an efficiently fread'able size */
#define IMGFILT_BUF_SIZE  512   /* choose a size to allow overlap */

HRESULT CJPEGSource::Init()
{
   _ASSERTE( m_pbBuffer == NULL );

   m_pbBuffer = new BYTE[IMGFILT_BUF_SIZE];
   if( m_pbBuffer == NULL )
   {
      return( E_OUTOFMEMORY );
   }

   init_source = InitSource;
   fill_input_buffer = FillInputBuffer;
   skip_input_data = SkipInputData;
   resync_to_restart = jpeg_resync_to_restart;
   term_source = TermSource;
   bytes_in_buffer = 0;
   next_input_byte = NULL;

   return( S_OK );
}

void CJPEGSource::InitSource( j_decompress_ptr pInfo )
{
   (void)pInfo;
}

boolean CJPEGSource::FillInputBuffer( j_decompress_ptr pInfo )
{
   ULONG nBytesRead;
   CJPEGSource* pSource;
   HRESULT hResult;

   pSource = (CJPEGSource*)pInfo->src;

   hResult = pSource->m_pFilter->m_pStream->Read( pSource->m_pbBuffer,
      IMGFILT_BUF_SIZE, &nBytesRead );
   if( FAILED( hResult ) )
   {
      ERREXIT( pInfo, JERR_INPUT_EMPTY );
   }

   pSource->bytes_in_buffer = nBytesRead;
   pSource->next_input_byte = pSource->m_pbBuffer;

   return( TRUE );
}     

void CJPEGSource::SkipInputData( j_decompress_ptr pInfo, long nBytes )
{
   CJPEGSource* pSource;

   pSource = (CJPEGSource*)pInfo->src;

  /* Just a dumb implementation for now.  Could use fseek() except
   * it doesn't work on pipes.  Not clear that being smart is worth
   * any trouble anyway --- large skips are infrequent.
   */
   if( nBytes > 0 ) 
   {
      while( ULONG( nBytes ) > pSource->bytes_in_buffer )
      {
         nBytes -= pSource->bytes_in_buffer;
         FillInputBuffer( pInfo );
      }

      pSource->next_input_byte += nBytes;
      pSource->bytes_in_buffer -= nBytes;
   }
}

void CJPEGSource::TermSource( j_decompress_ptr pInfo )
{
   (void)pInfo;
}

void CJPEGFilter::ErrorExit( j_common_ptr pInfo )
{
   (void)pInfo;

   RaiseException( JPEG_EXCEPTION, 0, 0, NULL );

   _ASSERT( FALSE );
}

HRESULT CJPEGFilter::FireOnBitsCompleteEvent()
{
   HRESULT hResult;

   if( !(m_dwEvents & IMGDECODE_EVENT_BITSCOMPLETE) )
   {
      return( S_OK );
   }

   hResult = m_pEventSink->OnBitsComplete();
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   return( S_OK );
}

HRESULT CJPEGFilter::LockBits(RECT *prcBounds, DWORD dwLockFlags, void **ppBits, long *pPitch)
{
    HRESULT hResult;
    
    if (m_pDDrawSurface)
    {
        DDSURFACEDESC   ddsd;

        ddsd.dwSize = sizeof(ddsd);
        hResult = m_pDDrawSurface->Lock(prcBounds, &ddsd, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
        if (FAILED(hResult))
            return hResult;

        *ppBits = ddsd.lpSurface;
        *pPitch = ddsd.lPitch;

        return S_OK;
    }
    else
    {
        return m_pBitmapSurface->LockBits(prcBounds, dwLockFlags, ppBits, pPitch);
    }
}

HRESULT CJPEGFilter::UnlockBits(RECT *prcBounds, void *pBits)
{
    if (m_pDDrawSurface)
    {
        return m_pDDrawSurface->Unlock(pBits);
    }
    else
    {
        return m_pBitmapSurface->UnlockBits(prcBounds, pBits);
    }
}


HRESULT CJPEGFilter::FireGetSurfaceEvent()
{
    HRESULT hResult;
    CComPtr< IUnknown > pSurface;
    BFID    bfid;

    _ASSERTE(m_pEventSink != NULL);
    _ASSERTE(m_pBitmapSurface == NULL);
    _ASSERTE(m_pDDrawSurface == NULL);

    if (m_jpegInfo.out_color_space == JCS_GRAYSCALE && m_bRGB8Allowed)
    {
        bfid = BFID_RGB_8;
        m_nBytesPerPixel = 1;
    }
    else
    {
        bfid = BFID_RGB_24;
        m_nBytesPerPixel = 3;
    }
    
    hResult = m_pEventSink->GetSurface(m_nWidth, m_nHeight, bfid, 1,
                    IMGDECODE_HINT_TOPDOWN|IMGDECODE_HINT_FULLWIDTH, 
                    &pSurface);

    if (FAILED(hResult))
        return hResult;

    if (m_dwEvents & IMGDECODE_EVENT_USEDDRAW)
        pSurface->QueryInterface(IID_IDirectDrawSurface, 
                                 (void **)&m_pDDrawSurface);

    if (m_pDDrawSurface == NULL)
    {
        hResult = pSurface->QueryInterface(IID_IBitmapSurface, 
                                           (void **)&m_pBitmapSurface);
        if (FAILED(hResult))
        {
            return(hResult);
        }
    }

    // If this is a grayscale JPEG set the color table to a gray palette,
    //   if the surface is an indexed surface as well.  If the caller only
    //   allows 24bpp surfaces, we will setup the JPEG decoder to do the
    //   work for us.

    if (bfid == BFID_RGB_8)
    {
        if (m_pDDrawSurface)
        {
            LPDIRECTDRAWPALETTE pDDPalette;
            PALETTEENTRY        ape[256];

		    hResult = m_pDDrawSurface->GetPalette(&pDDPalette);
		    if (SUCCEEDED(hResult))
		    {
                CopyPaletteEntriesFromColors(ape, g_argbGrayScale, 256);
		        pDDPalette->SetEntries(0, 0, 256, ape);
		        pDDPalette->Release();
		    }
        }
        else
        {
            CComPtr< IRGBColorTable > pColorTable;
        
            hResult = m_pBitmapSurface->QueryInterface(IID_IRGBColorTable,
                            (void**)&pColorTable);
            if (FAILED(hResult))
            {
                return (hResult);
            }

            if (pColorTable != NULL)
            {
                hResult = pColorTable->SetColors(0, 256, g_argbGrayScale);
                if (FAILED(hResult))
                {
                    return (hResult);
                }
            }
        }

        if (m_dwEvents & IMGDECODE_EVENT_PALETTE)
        {
            hResult = m_pEventSink->OnPalette();
            if (FAILED(hResult))
            {
                return (hResult);
            }
        }
    }
    
    
    return (S_OK);
}

HRESULT CJPEGFilter::FireOnProgressEvent()
{
   HRESULT hResult;
   RECT rect;

   if( !(m_dwEvents & IMGDECODE_EVENT_PROGRESS) )
   {
      return( S_OK );
   }

   rect.left = 0;
   rect.top = m_iFirstStaleScanLine;
   rect.right = m_nWidth;
   rect.bottom = m_iScanLine;

   hResult = m_pEventSink->OnProgress( &rect, TRUE );
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   m_iFirstStaleScanLine = m_iScanLine;

   return( S_OK );
}


///////////////////////////////////////////////////////////////////////////////
// IImageDecodeFilter methods
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CJPEGFilter::Initialize( IImageDecodeEventSink* pEventSink )
{
   HRESULT hResult;
   ULONG nFormats;
   GUID* pFormats;
   ULONG iFormat;
   BOOL bFound;

   if( pEventSink == NULL )
   {
      return( E_INVALIDARG );
   }

   m_pEventSink = pEventSink;

   hResult = m_pEventSink->OnBeginDecode( &m_dwEvents, &nFormats, &pFormats );
   if( FAILED( hResult ) )
   {
      return( hResult );
   }
   bFound = FALSE;
   for( iFormat = 0; (iFormat < nFormats); iFormat++ )
   {
      if( IsEqualGUID( pFormats[iFormat], BFID_RGB_24 ) )
      {
         bFound = TRUE;
      }

        if (IsEqualGUID(pFormats[iFormat], BFID_RGB_8))
            m_bRGB8Allowed = TRUE;      
   }
   CoTaskMemFree( pFormats );
   if( !bFound )
   {
      return( E_FAIL );
   }

   m_jpegInfo.err = jpeg_std_error( &m_jpegError );
   m_jpegError.error_exit = ErrorExit;

   __try
   {
      jpeg_create_decompress( &m_jpegInfo );
   }
   __except( (GetExceptionCode() == JPEG_EXCEPTION) ? 
      EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
   {
      return( E_FAIL );
   }

   m_pSource = new CJPEGSource( this );
   if( m_pSource == NULL )
   {
      return( E_OUTOFMEMORY );
   }

   hResult = m_pSource->Init();
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   m_jpegInfo.src = m_pSource;

   return( S_OK );
}

#define NUM_SCANLINES   16

STDMETHODIMP CJPEGFilter::Process( IStream* pStream )
{
   HRESULT hResult;
   BYTE* pbSrcPixel;
   BYTE* pbDestPixel;
   void* pBits;
   LONG nPitch;
   LONG nBufferPitch;
   BYTE* apbBuffer[NUM_SCANLINES];
   ULONG iScanLine;
   ULONG iPixel;
   ULONG nScanLinesRead;
   RECT rect;

   // We have to do this every time.  We don't AddRef, since we don't hold onto
   // the stream.
   m_pStream = pStream;

   __try
   {
      jpeg_read_header( &m_jpegInfo, TRUE );

        m_jpegInfo.dct_method = JDCT_ISLOW;
        m_jpegInfo.quantize_colors = FALSE;
        
        if (m_jpegInfo.jpeg_color_space == JCS_GRAYSCALE)
        {
            m_jpegInfo.out_color_space = JCS_GRAYSCALE;
        }
        else
        {
            m_jpegInfo.out_color_space = JCS_RGB;
        }
        
      jpeg_start_decompress( &m_jpegInfo );

      m_nWidth = m_jpegInfo.output_width;
      m_nHeight = m_jpegInfo.output_height;

      hResult = FireGetSurfaceEvent();
      if( FAILED( hResult ) )
      {
         return( hResult );
      }

      nBufferPitch = ((m_nWidth*m_nBytesPerPixel)+3)&(~0x03);
      m_pbScanLines = new BYTE[NUM_SCANLINES*nBufferPitch];
      if( m_pbScanLines == NULL )
      {
         return( E_OUTOFMEMORY );
      }

      for( iScanLine = 0; iScanLine < NUM_SCANLINES; iScanLine++ )
      {
         apbBuffer[iScanLine] = m_pbScanLines+(iScanLine*nBufferPitch);
      }

      rect.left = 0;
      rect.right = m_nWidth;
      m_iScanLine = 0;
      while( m_iScanLine < m_nHeight )
      {
         nScanLinesRead = jpeg_read_scanlines( &m_jpegInfo, apbBuffer, NUM_SCANLINES );

         rect.top = m_iScanLine;
         rect.bottom = m_iScanLine+nScanLinesRead;

         hResult = LockBits( &rect, SURFACE_LOCK_EXCLUSIVE,
            &pBits, &nPitch );
         if( FAILED( hResult ) )
         {
            return( hResult );
         }

         for( iScanLine = 0; iScanLine < nScanLinesRead; iScanLine++ )
         {
            pbSrcPixel = apbBuffer[iScanLine];
            pbDestPixel = LPBYTE( pBits )+(iScanLine*nPitch);
            if (m_nBytesPerPixel == 3)
            {
                if (m_jpegInfo.out_color_space == JCS_RGB)
                {
                    // Full color image
                    for( iPixel = 0; iPixel < m_nWidth; iPixel++ )
                    {
                        pbDestPixel[0] = pbSrcPixel[2];
                        pbDestPixel[1] = pbSrcPixel[1];
                        pbDestPixel[2] = pbSrcPixel[0];

                        pbDestPixel += 3;
                        pbSrcPixel += 3;
                    }
                }
                else
                {
                    _ASSERTE(m_jpegInfo.out_color_space == JCS_GRAYSCALE);

                    // Grayscale image for client that only allows 24bpp
                    // surfaces.
                    for( iPixel = 0; iPixel < m_nWidth; iPixel++ )
                    {
                        pbDestPixel[0] = pbSrcPixel[0];
                        pbDestPixel[1] = pbSrcPixel[0];
                        pbDestPixel[2] = pbSrcPixel[0];

                        pbDestPixel += 3;
                        ++pbSrcPixel;
                    }
                }
            }
            else
            {
                // Indexed grayscale image 
                memcpy(pbDestPixel, pbSrcPixel, m_nWidth);
            }
         }

         UnlockBits( &rect, pBits );
         m_iScanLine += nScanLinesRead;

         hResult = FireOnProgressEvent();
         if( FAILED( hResult ) )
         {
            return( hResult );
         }
      }

      jpeg_finish_decompress( &m_jpegInfo );
   }
   __except( (GetExceptionCode() == JPEG_EXCEPTION) ? 
      EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
   {
      return( E_FAIL );
   }

   hResult = FireOnBitsCompleteEvent();
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   m_pStream = NULL;

   return( S_OK );
}

STDMETHODIMP CJPEGFilter::Terminate( HRESULT hrStatus )
{
   ATLTRACE( "Image decode terminated.  Status: %x\n", hrStatus );

   delete m_pbScanLines;
   m_pbScanLines = NULL;

   if( m_pBitmapSurface != NULL )
   {
      m_pBitmapSurface.Release();
   }

    if (m_pDDrawSurface != NULL)
    {
        m_pDDrawSurface.Release();
    }
    
   if( m_pEventSink != NULL )
   {
      m_pEventSink->OnDecodeComplete( hrStatus );
      m_pEventSink.Release();
   }

   jpeg_destroy_decompress( &m_jpegInfo );

   delete m_pSource;
   m_pSource = NULL;

   return( S_OK );
}


