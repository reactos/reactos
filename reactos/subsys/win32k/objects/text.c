

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/dc.h>
#include <win32k/text.h>
#include <win32k/kapi.h>
#include <freetype/freetype.h>

// #define NDEBUG
#include <win32k/debug1.h>

FT_Library  library;

BOOL InitFontSupport()
{
  ULONG error;

  error = FT_Init_FreeType(&library);
  if(error)
  {
    return FALSE;
  } else return TRUE;
}

int
STDCALL
W32kAddFontResource(LPCWSTR  Filename)
{
  UNIMPLEMENTED;
}

HFONT
STDCALL
W32kCreateFont(int  Height,
                      int  Width,
                      int  Escapement,
                      int  Orientation,
                      int  Weight,
                      DWORD  Italic,
                      DWORD  Underline,
                      DWORD  StrikeOut,
                      DWORD  CharSet,
                      DWORD  OutputPrecision,
                      DWORD  ClipPrecision,
                      DWORD  Quality,
                      DWORD  PitchAndFamily,
                      LPCWSTR  Face)
{
  UNIMPLEMENTED;
}

HFONT
STDCALL
W32kCreateFontIndirect(CONST LPLOGFONT  lf)
{
  DbgPrint("WARNING: W32kCreateFontIndirect is current unimplemented\n");
}

BOOL
STDCALL
W32kCreateScalableFontResource(DWORD  Hidden,
                                     LPCWSTR  FontRes,
                                     LPCWSTR  FontFile,
                                     LPCWSTR  CurrentPath)
{
  UNIMPLEMENTED;
}

int
STDCALL
W32kEnumFontFamilies(HDC  hDC,
                          LPCWSTR  Family,
                          FONTENUMPROC  EnumFontFamProc,
                          LPARAM  lParam)
{
  UNIMPLEMENTED;
}

int
STDCALL
W32kEnumFontFamiliesEx(HDC  hDC,
                            LPLOGFONT  Logfont,
                            FONTENUMPROC  EnumFontFamExProc,
                            LPARAM  lParam,
                            DWORD  Flags)
{
  UNIMPLEMENTED;
}

int
STDCALL
W32kEnumFonts(HDC  hDC,
                   LPCWSTR FaceName,
                   FONTENUMPROC  FontFunc,
                   LPARAM  lParam)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kExtTextOut(HDC  hDC,
                     int  X,
                     int  Y,
                     UINT  Options,
                     CONST LPRECT  rc,
                     LPCWSTR  String,
                     UINT  Count,
                     CONST LPINT  Dx)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetAspectRatioFilterEx(HDC  hDC,
                                 LPSIZE  AspectRatio)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetCharABCWidths(HDC  hDC,
                           UINT  FirstChar,
                           UINT  LastChar,
                           LPABC  abc)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetCharABCWidthsFloat(HDC  hDC,
                                UINT  FirstChar,
                                UINT  LastChar,
                                LPABCFLOAT  abcF)
{
  UNIMPLEMENTED;
}

DWORD
STDCALL
W32kGetCharacterPlacement(HDC  hDC,
                                 LPCWSTR  String,
                                 int  Count,
                                 int  MaxExtent,
                                 LPGCP_RESULTS  Results,
                                 DWORD  Flags)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetCharWidth(HDC  hDC,
                       UINT  FirstChar,
                       UINT  LastChar,
                       LPINT  Buffer)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetCharWidth32(HDC  hDC,
                         UINT  FirstChar,
                         UINT  LastChar,
                         LPINT  Buffer)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetCharWidthFloat(HDC  hDC,
                            UINT  FirstChar,
                            UINT  LastChar,
                            PFLOAT  Buffer)
{
  UNIMPLEMENTED;
}

DWORD
STDCALL
W32kGetFontLanguageInfo(HDC  hDC)
{
  UNIMPLEMENTED;
}

DWORD
STDCALL
W32kGetGlyphOutline(HDC  hDC,
                           UINT  Char,
                           UINT  Format,
                           LPGLYPHMETRICS  gm,
                           DWORD  Bufsize,
                           LPVOID  Buffer,
                           CONST LPMAT2 mat2)
{
  UNIMPLEMENTED;


}

DWORD
STDCALL
W32kGetKerningPairs(HDC  hDC,
                           DWORD  NumPairs,
                           LPKERNINGPAIR  krnpair)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
W32kGetOutlineTextMetrics(HDC  hDC,
                                UINT  Data,
                                LPOUTLINETEXTMETRIC  otm)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetRasterizerCaps(LPRASTERIZER_STATUS  rs,
                            UINT  Size)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
W32kGetTextCharset(HDC  hDC)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
W32kGetTextCharsetInfo(HDC  hDC,
                             LPFONTSIGNATURE  Sig,
                             DWORD  Flags)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetTextExtentExPoint(HDC  hDC,
                               LPCWSTR String,
                               int  Count,
                               int  MaxExtent,
                               LPINT  Fit,
                               LPINT  Dx,
                               LPSIZE  Size)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetTextExtentPoint(HDC  hDC,
                             LPCWSTR  String,
                             int  Count,
                             LPSIZE  Size)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetTextExtentPoint32(HDC  hDC,
                               LPCWSTR  String,
                               int  Count,
                               LPSIZE  Size)
{
  UNIMPLEMENTED;
}

int
STDCALL
W32kGetTextFace(HDC  hDC,
                     int  Count,
                     LPWSTR  FaceName)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetTextMetrics(HDC  hDC,
                         LPTEXTMETRIC  tm)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kPolyTextOut(HDC  hDC,
                      CONST LPPOLYTEXT  txt,
                      int  Count)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kRemoveFontResource(LPCWSTR  FileName)
{
  UNIMPLEMENTED;
}

DWORD
STDCALL
W32kSetMapperFlags(HDC  hDC,
                          DWORD  Flag)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
W32kSetTextAlign(HDC  hDC,
                       UINT  Mode)
{
  UINT prevAlign;
  DC *dc;
  
  dc = DC_HandleToPtr(hDC);
  if (!dc) 
    {
      return  0;
    }
  prevAlign = dc->w.textAlign;
  dc->w.textAlign = Mode;
  DC_UnlockDC (hDC);
  
  return  prevAlign;
}

COLORREF
STDCALL
W32kSetTextColor(HDC hDC, 
                 COLORREF color)
{
  COLORREF  oldColor;
  PDC  dc = DC_HandleToPtr(hDC);
  
  if (!dc) 
  {
    return 0x80000000;
  }

  oldColor = dc->w.textColor;
  dc->w.textColor = color;
  DC_UnlockDC(hDC);

  return  oldColor;
}

BOOL
STDCALL
W32kSetTextJustification(HDC  hDC,
                               int  BreakExtra,
                               int  BreakCount)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kTextOut(HDC  hDC,
                  int  XStart,
                  int  YStart,
                  LPCWSTR  String,
                  int  Count)
{
  // Fixme: Call EngTextOut, which does the real work (calling DrvTextOut where appropriate)

  DC *dc = DC_HandleToPtr(hDC);
  SURFOBJ *SurfObj = AccessUserObject(dc->Surface);
  UNICODE_STRING FileName;
  int error, glyph_index, n, load_flags = FT_LOAD_RENDER, i, j;
  FT_Face face;
  FT_GlyphSlot glyph;
  NTSTATUS Status;
  HANDLE FileHandle;
  OBJECT_ATTRIBUTES ObjectAttributes;
  FILE_STANDARD_INFORMATION FileStdInfo;
  PVOID buffer;
  ULONG size, TextLeft = XStart, TextTop = YStart, SpaceBetweenChars = 5, StringLength;
  FT_Vector origin;
  FT_Bitmap bit, bit2, bit3;
  POINTL SourcePoint;
  RECTL DestRect;
  HBITMAP HSourceGlyph;
  PSURFOBJ SourceGlyphSurf;
  SIZEL bitSize;
  FT_CharMap found = 0;
  FT_CharMap charmap;
  PCHAR bitbuf;

  // For now we're just going to use an internal font
//  grWriteCellString(SurfObj, XStart, YStart, AString->Buffer, 0xffffff);

  //  Prepare the Unicode FileName
  RtlCreateUnicodeString(&FileName, L"\\SystemRoot\\fonts\\arial.ttf");

  //  Open the Module
  InitializeObjectAttributes(&ObjectAttributes, &FileName, 0, NULL, NULL);

  Status = NtOpenFile(&FileHandle, FILE_ALL_ACCESS, &ObjectAttributes, NULL, 0, 0);

  if (!NT_SUCCESS(Status))
  {
    DbgPrint("Could not open module file: %wZ\n", L"c:/reactos/fonts/arial.ttf");
    return  0;
  }

  //  Get the size of the file
  Status = NtQueryInformationFile(FileHandle, NULL, &FileStdInfo, sizeof(FileStdInfo), FileStandardInformation);
  if (!NT_SUCCESS(Status))
  {
    DbgPrint("Could not get file size\n");
    return  0;
  }

  //  Allocate nonpageable memory for driver
  size = FileStdInfo.EndOfFile.u.LowPart;
  buffer = ExAllocatePool(NonPagedPool, size);

  if (buffer == NULL)
  {
    DbgPrint("could not allocate memory for module");
    return  0;
  }
   
  //  Load driver into memory chunk
  Status = NtReadFile(FileHandle, 0, 0, 0, 0, buffer, FileStdInfo.EndOfFile.u.LowPart, 0, 0);
  if (!NT_SUCCESS(Status))
  {
    DbgPrint("could not read module file into memory");
    ExFreePool(buffer);

    return  0;
  }

  NtClose(FileHandle);

  error = FT_New_Memory_Face(library,
                             buffer,    // first byte in memory
                             size,      // size in bytes
                             0,         // face_index
                             &face );
  if ( error == FT_Err_Unknown_File_Format )
  {
    DbgPrint("Unknown font file format\n");
  }
  else if ( error )
  {
    DbgPrint("Error reading font file\n");
  }

  DbgPrint("Family name: %s\n", face->family_name);
  DbgPrint("Style name: %s\n", face->style_name);
  DbgPrint("Num glyphs: %u\n", face->num_glyphs);
  DbgPrint("Height: %d\n", face->height);

  if (face->charmap == NULL)
  {
    DbgPrint("WARNING: No charmap selected!\n");
    DbgPrint("This font face has %d charmaps\n", face->num_charmaps);

    for ( n = 0; n < face->num_charmaps; n++ )
    {
      charmap = face->charmaps[n];
      DbgPrint("found charmap encoding: %u\n", charmap->encoding);
      if (charmap->encoding != 0)
      {
        found = charmap;
        break;
      }
    }
  }
  if (!found) DbgPrint("WARNING: Could not find desired charmap!\n");
                 
  error = FT_Set_Charmap(face, found);
  if (error) DbgPrint("WARNING: Could not set the charmap!\n");

  error = FT_Set_Char_Size(
              face,    // handle to face object
              16*64,   // char_width in 1/64th of points
              16*64,   // char_height in 1/64th of points
              300,     // horizontal device resolution
              300 );   // vertical device resolution

  StringLength = wcslen(String);
  DbgPrint("StringLength: %u\n", StringLength);

  for(i=0; i<StringLength; i++)
  {
    glyph_index = FT_Get_Char_Index(face, *String);

    error = FT_Load_Glyph( 
              face,			// handle to face object
              glyph_index,		// glyph index
              FT_LOAD_DEFAULT );	// load flags (erase previous glyph)
    if(error) DbgPrint("WARNING: Failed to load and render glyph!\n");

    glyph = face->glyph;

    if ( glyph->format == ft_glyph_format_outline )
    {
      DbgPrint("Outline Format Font\n");
      error = FT_Render_Glyph( glyph, ft_render_mode_normal );
      if(error) DbgPrint("WARNING: Failed to render glyph!\n");
    } else {
      DbgPrint("Bitmap Format Font\n");
      bit3.rows   = glyph->bitmap.rows;
      bit3.width  = glyph->bitmap.width;
      bit3.pitch  = glyph->bitmap.pitch;
      bit3.buffer = glyph->bitmap.buffer;
    }

    SourcePoint.x	= 0;
    SourcePoint.y	= 0;
    DestRect.left	= TextLeft;
    DestRect.top	= TextTop;
    DestRect.right	= TextLeft + glyph->bitmap.width-1;
    DestRect.bottom	= TextTop  + glyph->bitmap.rows-1;
    bitSize.cx		= glyph->bitmap.width-1;
    bitSize.cy		= glyph->bitmap.rows-1;

    HSourceGlyph = EngCreateBitmap(bitSize, glyph->bitmap.pitch /* -1 */ , BMF_8BPP, 0, glyph->bitmap.buffer);
    SourceGlyphSurf = AccessUserObject(HSourceGlyph);

    EngBitBlt(SurfObj, SourceGlyphSurf,
              NULL, NULL, NULL, &DestRect, &SourcePoint, NULL, NULL, NULL, NULL);

    TextLeft += glyph->bitmap.width + SpaceBetweenChars;
    String++;
  }

DbgPrint("BREAK\n"); for (;;) ;

/*  RtlFreeAnsiString(AString);
    RtlFreeUnicodeString(UString); */
}

UINT
STDCALL
W32kTranslateCharsetInfo(PDWORD  Src,
                               LPCHARSETINFO  CSI,   
                               DWORD  Flags)
{
  UNIMPLEMENTED;
}
