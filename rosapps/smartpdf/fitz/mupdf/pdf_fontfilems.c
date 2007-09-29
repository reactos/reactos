#include <fitz.h>
#include <mupdf.h>

#include <windows.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#define SAFE_FZ_READ(file, buf, size)\
	byteread = fz_read((file), (char*)(buf), (size));\
	if(byteread<0) err = fz_ioerror(file);\
	if(byteread != (size)) err = fz_throw("ioerror");\
	if(err) goto cleanup;

#define ARRAY_SIZE(a) sizeof(a) / sizeof(a[0])

#define SWAPWORD(x)		MAKEWORD(HIBYTE(x), LOBYTE(x))
#define SWAPLONG(x)		MAKELONG(SWAPWORD(HIWORD(x)), SWAPWORD(LOWORD(x)))

#define PLATFORM_UNICODE				0
#define PLATFORM_MACINTOSH				1
#define PLATFORM_ISO					2
#define PLATFORM_MICROSOFT				3

#define UNI_ENC_UNI_1					0
#define UNI_ENC_UNI_1_1					1
#define UNI_ENC_ISO						2
#define UNI_ENC_UNI_2_BMP				3
#define UNI_ENC_UNI_2_FULL_REPERTOIRE	4

#define MAC_ROMAN						0
#define MAC_JAPANESE					1
#define MAC_CHINESE_TRADITIONAL			2
#define MAC_KOREAN						3
#define MAC_CHINESE_SIMPLIFIED			25

#define MS_ENC_SYMBOL					0
#define MS_ENC_UNI_BMP					1
#define MS_ENC_SHIFTJIS					2
#define MS_ENC_PRC						3
#define MS_ENC_BIG5						4
#define MS_ENC_WANSUNG					5
#define MS_ENC_JOHAB					6
#define MS_ENC_UNI_FULL_REPETOIRE		10

#define TTC_VERSION1	0x00010000
#define TTC_VERSION2	0x00020000

typedef struct pdf_fontmapMS_s pdf_fontmapMS;
typedef struct pdf_fontlistMS_s pdf_fontlistMS;

struct pdf_fontmapMS_s
{
	char fontface[128];
	char fontpath[MAX_PATH+1];
	int index;
};

struct pdf_fontlistMS_s
{
	pdf_fontmapMS *fontmap;
	int len;
	int cap;	
};

typedef struct _tagTT_OFFSET_TABLE
{
	USHORT	uMajorVersion;
	USHORT	uMinorVersion;
	USHORT	uNumOfTables;
	USHORT	uSearchRange;
	USHORT	uEntrySelector;
	USHORT	uRangeShift;
} TT_OFFSET_TABLE;

typedef struct _tagTT_TABLE_DIRECTORY
{
	char	szTag[4];			//table name
	ULONG	uCheckSum;			//Check sum
	ULONG	uOffset;			//Offset from beginning of file
	ULONG	uLength;			//length of the table in bytes
} TT_TABLE_DIRECTORY;

typedef struct _tagTT_NAME_TABLE_HEADER
{
	USHORT	uFSelector;			//format selector. Always 0
	USHORT	uNRCount;			//Name Records count
	USHORT	uStorageOffset;		//Offset for strings storage, from start of the table
} TT_NAME_TABLE_HEADER;

typedef struct _tagTT_NAME_RECORD
{
	USHORT	uPlatformID;
	USHORT	uEncodingID;
	USHORT	uLanguageID;
	USHORT	uNameID;
	USHORT	uStringLength;
	USHORT	uStringOffset;	//from start of storage area
} TT_NAME_RECORD;

typedef struct _tagFONT_COLLECTION
{
	char	Tag[4];
	ULONG	Version;
	ULONG	NumFonts;
}FONT_COLLECTION;

static char *basenames[13] =
{
	"Courier", 
	"Courier-Bold", 
	"Courier-Oblique",
	"Courier-BoldOblique",
	"Helvetica",
	"Helvetica-Bold",
	"Helvetica-Oblique",
	"Helvetica-BoldOblique",
	"Times-Roman",
	"Times-Bold",
	"Times-Italic",
	"Times-BoldItalic",
	"Symbol", 
};

static char *basepatterns[13] =
{
	"CourierNewPSMT",
	"CourierNewPS-BoldMT",
	"CourierNewPS-ItalicMT",
	"CourierNewPS-BoldItalicMT",
	"ArialMT",
	"Arial-BoldMT",
	"Arial-ItalicMT",
	"Arial-BoldItalicMT",
	"TimesNewRomanPSMT",
	"TimesNewRomanPS-BoldMT",
	"TimesNewRomanPS-ItalicMT",
	"TimesNewRomanPS-BoldItalicMT",
	"SymbolMT"
};

static pdf_fontlistMS fontlistMS =
{
	NULL,
	0,
	0,
};

static int
compare(const void *elem1, const void *elem2)
{
	pdf_fontmapMS *val1 = (pdf_fontmapMS *)elem1;
	pdf_fontmapMS *val2 = (pdf_fontmapMS *)elem2;

	if(val1->fontface[0] == 0)
		return 1;
	if(val2->fontface[0] == 0)
		return -1;

	return stricmp(val1->fontface, val2->fontface);
}

static void * 
localbsearch (const void *key, const void *base, size_t num,
		 size_t width, int (*compare)(const void *, const void *))
{
	char *lo = (char *)base;
	char *hi = (char *)base + (num - 1) * width;
	char *mid;
	unsigned int half;
	int result;
	
	while (lo <= hi)
		if (half = num / 2)
		{
			mid = lo + (num & 1 ? half : (half - 1)) * width;
			if (!(result = (*compare)(key,mid)))
				return(mid);
			else if (result < 0)
			{
				hi = mid - width;
				num = num & 1 ? half : half-1;
			}
			else	{
				lo = mid + width;
				num = half;
			}
		}
		else if (num)
			return((*compare)(key,lo) ? 0 : lo);
		else
			break;
		
		return(0);
}

static void
removeredundancy(pdf_fontlistMS *fl)
{
	int i;
	int roffset = 0;
	int redundancy_count = 0;

	qsort(fl->fontmap,fl->len,sizeof(pdf_fontmapMS),compare);
	for(i = 0; i < fl->len - 1; ++i)
	{
		if(strcmp(fl->fontmap[i].fontface,fl->fontmap[i+1].fontface) == 0)
		{
			fl->fontmap[i].fontface[0] = 0;
			++redundancy_count;
		}
	}
	qsort(fl->fontmap,fl->len,sizeof(pdf_fontmapMS),compare);
	fl->len -= redundancy_count;
#ifndef NDEBUG
	for(i = 0; i < fl->len; ++i)
		fprintf(stdout,"%s , %s , %d\n",fl->fontmap[i].fontface,
			fl->fontmap[i].fontpath,fl->fontmap[i].index);
#endif
}

static fz_error * 
swapword(char* pbyte, int nLen)
{
	int i;
	char tmp;
	int nMax;

	if(nLen%2)
		return fz_throw("fonterror");

	nMax = nLen / 2;
	for(i = 0; i < nLen; ++i) {
		tmp = pbyte[i*2];
		pbyte[i*2] = pbyte[i*2+1];
		pbyte[i*2+1] = tmp;
	}
	return 0;
}

/* pSouce and PDest can be same */
static fz_error * 
decodeunicodeBMP(char* source, int sourcelen,char* dest, int destlen)
{
	wchar_t tmp[1024*2];
	int converted;
	memset(tmp,0,sizeof(tmp));
	memcpy(tmp,source,sourcelen);
	swapword((char*)tmp,sourcelen);

	converted = WideCharToMultiByte(CP_ACP, 0, tmp,
		-1, dest, destlen, NULL, NULL);

	if(converted == 0)
		return fz_throw("fonterror");

	return 0;
}

static fz_error *
decodeunicodeplatform(char* source, int sourcelen,char* dest, int destlen, int enctype)
{
	fz_error *err = nil;
	switch(enctype)
	{
	case UNI_ENC_UNI_1:
	case UNI_ENC_UNI_2_BMP:
		err = decodeunicodeBMP(source,sourcelen,dest,destlen);
		break;
	case UNI_ENC_UNI_2_FULL_REPERTOIRE:
	case UNI_ENC_UNI_1_1:
	case UNI_ENC_ISO:
	default:
		err = fz_throw("fonterror : unsupported encoding");
		break;
	}
	return err;
}

static fz_error *
decodemacintoshplatform(char* source, int sourcelen,char* dest, int destlen, int enctype)
{
	fz_error *err = nil;
	switch(enctype)
	{
	case MAC_ROMAN:
		if(sourcelen + 1 > destlen)
			err = fz_throw("fonterror : short buf lenth");
		else
		{
			memcpy(source,dest,sourcelen);
			dest[sourcelen] = 0;
		}
		break;
	default:
		err = fz_throw("fonterror : unsupported encoding");
		break;
	}
	return err;
}

static fz_error *
decodemicrosoftplatform(char* source, int sourcelen,char* dest, int destlen, int enctype)
{
	fz_error *err = nil;
	switch(enctype)
	{
	case MS_ENC_SYMBOL:
	case MS_ENC_UNI_BMP:
		err = decodeunicodeBMP(source,sourcelen,dest,destlen);
		break;
	default:
		err = fz_throw("fonterror : unsupported encoding");
		break;
	}
	return err;
}

static fz_error *
growfontlist(pdf_fontlistMS *fl)
{
	int newcap;
	pdf_fontmapMS *newitems;

	if(fl->cap == 0)
		newcap = 32;
	else
		newcap = fl->cap * 2;

	newitems = fz_realloc(fl->fontmap, sizeof(pdf_fontmapMS) * newcap);
	if (!newitems)
		return fz_outofmem;

	memset(newitems + fl->cap, 0, 
		sizeof(struct fz_keyval_s) * (newcap - fl->cap));

	fl->fontmap = newitems;
	fl->cap = newcap;

	return nil;
}

static fz_error *
insertmapping(pdf_fontlistMS *fl, char *facename, char *path, int index)
{
	fz_error *err;

	if(fl->len == fl->cap) {
		err = growfontlist(fl);
		if(err) return err;
	}

	if(fl->len >= fl->cap)
		return fz_throw("fonterror : fontlist overflow");

	strlcpy(fl->fontmap[fl->len].fontface, facename,
		sizeof(fl->fontmap[0].fontface));
	strlcpy(fl->fontmap[fl->len].fontpath, path,
		sizeof(fl->fontmap[0].fontpath));
	fl->fontmap[fl->len].index = index;

	++fl->len;

	return nil;
}

static fz_error *
parseTTF(fz_stream *file, int offset, int index, char *path)
{
	fz_error *err = nil;
	int byteread;

	TT_OFFSET_TABLE ttOffsetTable;
	TT_TABLE_DIRECTORY tblDir;
	TT_NAME_TABLE_HEADER ttNTHeader;
	TT_NAME_RECORD ttRecord;

	char szTemp[4096];
	int found;
	int i;

	fz_seek(file,offset,0);
	SAFE_FZ_READ(file, &ttOffsetTable, sizeof(TT_OFFSET_TABLE));

	ttOffsetTable.uNumOfTables = SWAPWORD(ttOffsetTable.uNumOfTables);
	ttOffsetTable.uMajorVersion = SWAPWORD(ttOffsetTable.uMajorVersion);
	ttOffsetTable.uMinorVersion = SWAPWORD(ttOffsetTable.uMinorVersion);

	//check is this is a true type font and the version is 1.0
	if(ttOffsetTable.uMajorVersion != 1 || ttOffsetTable.uMinorVersion != 0)
		return fz_throw("fonterror : invalid font version");

	found = 0;
	
	for(i = 0; i< ttOffsetTable.uNumOfTables; i++)
	{
		SAFE_FZ_READ(file,&tblDir,sizeof(TT_TABLE_DIRECTORY));

		memcpy(szTemp, tblDir.szTag, 4);
		szTemp[4] = 0;

		if (stricmp(szTemp, "name") == 0)
		{
			found = 1;
			tblDir.uLength = SWAPLONG(tblDir.uLength);
			tblDir.uOffset = SWAPLONG(tblDir.uOffset);
			break;
		}
		else if (szTemp[0] == 0)
		{
			break;
		}
	}

	if (found)
	{
		fz_seek(file,tblDir.uOffset,0);

		SAFE_FZ_READ(file,&ttNTHeader,sizeof(TT_NAME_TABLE_HEADER));

		ttNTHeader.uNRCount = SWAPWORD(ttNTHeader.uNRCount);
		ttNTHeader.uStorageOffset = SWAPWORD(ttNTHeader.uStorageOffset);

		offset = tblDir.uOffset + sizeof(TT_NAME_TABLE_HEADER);

		for(i = 0; i < ttNTHeader.uNRCount && err == nil; ++i)
		{
			fz_seek(file, offset + sizeof(TT_NAME_RECORD)*i, 0);
			SAFE_FZ_READ(file,&ttRecord,sizeof(TT_NAME_RECORD));

			ttRecord.uNameID = SWAPWORD(ttRecord.uNameID);
			ttRecord.uLanguageID = SWAPWORD(ttRecord.uLanguageID);

			// Full Name
			if(ttRecord.uNameID == 6)
			{
				ttRecord.uPlatformID = SWAPWORD(ttRecord.uPlatformID);
				ttRecord.uEncodingID = SWAPWORD(ttRecord.uEncodingID);
				ttRecord.uStringLength = SWAPWORD(ttRecord.uStringLength);
				ttRecord.uStringOffset = SWAPWORD(ttRecord.uStringOffset);

				fz_seek(file, tblDir.uOffset + ttRecord.uStringOffset + ttNTHeader.uStorageOffset, 0);
				SAFE_FZ_READ(file, szTemp, ttRecord.uStringLength);

				switch(ttRecord.uPlatformID)
				{
				case PLATFORM_UNICODE:
					err = decodeunicodeplatform(szTemp, ttRecord.uStringLength, 
						szTemp, sizeof(szTemp), ttRecord.uEncodingID);
					break;
				case PLATFORM_MACINTOSH:
					err = decodemacintoshplatform(szTemp, ttRecord.uStringLength, 
						szTemp, sizeof(szTemp), ttRecord.uEncodingID);
					break;
				case PLATFORM_ISO:
					err = fz_throw("fonterror : unsupported platform");
					break;
				case PLATFORM_MICROSOFT:
					err = decodemicrosoftplatform(szTemp, ttRecord.uStringLength,
						szTemp, sizeof(szTemp), ttRecord.uEncodingID);
					break;
				}

				if(err == nil)
					err = insertmapping(&fontlistMS, szTemp, path, index);
			}
		}
	}

cleanup:
	return err;
}

static fz_error *
parseTTFs(char *path)
{
	fz_error *err = nil;
	fz_stream *file = nil;

	err = fz_openrfile(&file, path);
	if(err)
		goto cleanup;

	err = parseTTF(file,0,0,path);
	if(err)
		goto cleanup;
		
cleanup:
	if(file)
		fz_dropstream(file);

	return err;
}

static fz_error *
parseTTCs(char *path)
{
	fz_error *err = nil;
	int byteread;
	fz_stream *file = nil;
	FONT_COLLECTION fontcollectioin;
	ULONG i;

	err = fz_openrfile(&file, path);
	if(err)
		goto cleanup;

	SAFE_FZ_READ(file, &fontcollectioin, sizeof(FONT_COLLECTION));
	if(memcmp(fontcollectioin.Tag,"ttcf",sizeof(fontcollectioin.Tag)) == 0)
	{
		fontcollectioin.Version = SWAPLONG(fontcollectioin.Version);
		fontcollectioin.NumFonts = SWAPLONG(fontcollectioin.NumFonts);
		if( fontcollectioin.Version == TTC_VERSION1 ||
			fontcollectioin.Version == TTC_VERSION2 )
		{
			ULONG *offsettable = fz_malloc(sizeof(ULONG)*fontcollectioin.NumFonts);
			if(offsettable == nil)
			{
				err = fz_outofmem;
				goto cleanup;
			}

			SAFE_FZ_READ(file, offsettable, sizeof(ULONG)*fontcollectioin.NumFonts);
			for(i = 0; i < fontcollectioin.NumFonts; ++i)
			{
				offsettable[i] = SWAPLONG(offsettable[i]);
				parseTTF(file,offsettable[i],i,path);				
			}
			fz_free(offsettable);
		}
		else
		{
			err = fz_throw("fonterror : invalid version");
			goto cleanup;
		}
	}
	else
	{
		err = fz_throw("fonterror: wrong format");
		goto cleanup;
	}
	
		
cleanup:
	if(file)
		fz_dropstream(file);

	return err;
}

static fz_error*
pdf_createfontlistMS()
{
	char szFontDir[MAX_PATH*2];
	char szSearch[MAX_PATH*2];
	char szFile[MAX_PATH*2];
	BOOL fFinished;
	HANDLE hList;
	WIN32_FIND_DATA FileData;
	fz_error *err;

	if (fontlistMS.len != 0)
		return nil;

	GetWindowsDirectory(szFontDir, sizeof(szFontDir));

	// Get the proper directory path
	strcat(szFontDir,"\\Fonts\\");
	sprintf(szSearch,"%s*.tt?",szFontDir);
	// Get the first file
	hList = FindFirstFile(szSearch, &FileData);
	if (hList == INVALID_HANDLE_VALUE)
	{
		/* Don't complain about missing directories */
		if (errno == ENOENT)
			return fz_throw("fonterror : can't find system fonts dir");
		return fz_throw("ioerror");
	}
	// Traverse through the directory structure
	fFinished = FALSE;
	while (!fFinished)
	{
		if (!(FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			// Get the full path for sub directory
			sprintf(szFile,"%s%s", szFontDir, FileData.cFileName);
			if (szFile[strlen(szFile)-1] == 'c' || szFile[strlen(szFile)-1] == 'C')
			{
				err = parseTTCs(szFile);
                // ignore error parsing a given font file
			}
			else if (szFile[strlen(szFile)-1] == 'f'|| szFile[strlen(szFile)-1] == 'F')
			{
				err = parseTTFs(szFile);
                // ignore error parsing a given font file
			}
		}

		if (!FindNextFile(hList, &FileData))
		{
			if (GetLastError() == ERROR_NO_MORE_FILES)
			{
				fFinished = TRUE;
			}
		}
	}

	removeredundancy(&fontlistMS);
	return nil;
}

void
pdf_destoryfontlistMS()
{
	if (fontlistMS.fontmap != nil)
		fz_free(fontlistMS.fontmap);

	fontlistMS.len = 0;
	fontlistMS.cap = 0;
}

#if 0 /* This is for testing if localbsearch() fails unexpectedly */
pdf_fontmapMS *
findlinear(pdf_fontlistMS *list, pdf_fontmapMS *item)
{
    int i, len;
    pdf_fontmapMS *curritem = list->fontmap;

    len = list->len;
    for (i = 0; i < len; i++)
    {
        if (0 == stricmp(curritem->fontface, item->fontface))
            return curritem;
        ++curritem;
    }
    return 0;
}
#endif

fz_error *
pdf_lookupfontMS(char *fontname, char **fontpath, int *index)
{
    pdf_fontmapMS fontmap;
    pdf_fontmapMS *found = nil;
    char *pattern;
    int i;

    if (fontlistMS.len == 0)
        return fz_throw("fonterror : no fonts in the system");

    pattern = fontname;
    for (i = 0; i < ARRAY_SIZE(basenames); i++)
    {
        if (0 == strcmp(fontname, basenames[i]))
        {
            pattern = basepatterns[i];
            break;
        }
    }

    strlcpy(fontmap.fontface,pattern,sizeof(fontmap.fontface));
    found = localbsearch(&fontmap, fontlistMS.fontmap, fontlistMS.len, sizeof(pdf_fontmapMS),compare);

#if 0
    if (!found)
        found = findlinear(&fontlistMS, &fontmap);
#endif

    if (found)
    {
        *fontpath = found->fontpath;
        *index = found->index;
    }
    else
    {
        *fontpath = fontlistMS.fontmap[0].fontpath;
        *index = fontlistMS.fontmap[0].index;
    }

	return nil;
}

static FT_Library ftlib = nil;

static fz_error *initfontlibs(void)
{
	int fterr;
	int maj, min, pat;
	fz_error  *err;

	if (ftlib)
		return nil;

	fterr = FT_Init_FreeType(&ftlib);
	if (fterr)
		return fz_throw("freetype failed initialisation: 0x%x", fterr);

	FT_Library_Version(ftlib, &maj, &min, &pat);
	if (maj == 2 && min == 1 && pat < 7)
		return fz_throw("freetype version too old: %d.%d.%d", maj, min, pat);

	err = pdf_createfontlistMS();
	if(err)
		return err;

	return nil;
}

fz_error *initfontlibs_ms(void)
{
	return initfontlibs();
}

void deinitfontlibs_ms(void)
{
    pdf_destoryfontlistMS();
    FT_Done_FreeType(ftlib);
    ftlib = nil;
}

fz_error *
pdf_loadbuiltinfont(pdf_font *font, char *basefont)
{
	fz_error *error;
	int fterr;

	FT_Face face;
	char *file;
	int index;

	error = initfontlibs();
	if (error)
		return error;

	error = pdf_lookupfontMS(basefont,&file,&index);
	if(error)
		return error;

	fterr = FT_New_Face(ftlib, file, index, &face);
	if (fterr)
		return fz_throw("freetype could not load font file '%s': 0x%x", file, fterr);

	font->ftface = face;

	return nil;
}

fz_error *
pdf_loadsystemfont(pdf_font *font, char *basefont, char *collection)
{
	fz_error *error;
	int fterr;
	FT_Face face;
	char *file;
	int index;

	error = initfontlibs();
	if (error)
		return error;

	error = pdf_lookupfontMS(basefont,&file,&index);
	if(error)
		goto cleanup;

	fterr = FT_New_Face(ftlib, file, index, &face);
	if (fterr) {
		return fz_throw("freetype could not load font file '%s': 0x%x", file, fterr);
	}

	font->ftface = face;

	return nil;

cleanup:
	return error;
}

fz_error *
pdf_loadembeddedfont(pdf_font *font, pdf_xref *xref, fz_obj *stmref)
{
	fz_error *error;
	int fterr;
	FT_Face face;
	fz_buffer *buf;

	error = initfontlibs();
	if (error)
		return error;

	error = pdf_loadstream(&buf, xref, fz_tonum(stmref), fz_togen(stmref));
	if (error)
		return error;

	fterr = FT_New_Memory_Face(ftlib, buf->rp, buf->wp - buf->rp, 0, &face);

	if (fterr) {
		fz_free(buf);
		return fz_throw("freetype could not load embedded font: 0x%x", fterr);
	}

	font->ftface = face;
	font->fontdata = buf;

	return nil;
}

