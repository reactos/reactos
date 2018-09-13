/***
 **
 **   Module: T1Instal
 **
 **   Description:
 **      This is the Win32 DLL (t1instal.dll) interface to the
 **      font converter. All window specific code is located in
 **      this module and the error hadler module (errors.c).
 **
 **   Author: Michael Jansson
 **   Created: 12/18/93
 **
 ***/

/***** INCLUDES */

#include "windows.h"

#include <string.h>
#include <time.h>
#include <limits.h>
#include <ctype.h>

#undef IN

#include "titott.h"
#include "types.h"
#include "t1local.h"
#include "t1instal.h"
#include "fileio.h"
#include "safemem.h"
#include "t1msg.h"

#undef UNICODE


/* The CopyrightCheck/MAYBEOK case always succeeds for now. */
#define MAYBEOK   SUCCESS


/***** LOCAL TYPES */
struct callFrame {
   const void (STDCALL *Progress)(short, void*);
   void *arg;
   int last;
   int done;
};


static short lastCP = FALSE;
static char lastVendor[256] = "Unknown.";

/***** CONSTANTS */
#define MIN_PROGRESS    3
#define DELIMITERS      " ,"
#define COPYSIGN        169
#define TRUE            1
#define FALSE           0
#define DFFACE          139
#define DFDRIVERINFO    101

#define VERSTR "Converter: Windows Type 1 Installer V1.0d.\n" \
               "Font: V"

const char version[] = "\n$VER: 1.0d\n";

#ifndef NOANSIWINMAC
const char *winmac[] = {
   "A",
   "AE",
   "Aacute",
   "Acircumflex",
   "Adieresis",
   "Agrave",
   "Aring",
   "Atilde",
   "B",
   "C",
   "Cacute",
   "Ccaron",
   "Ccedilla",
   "D",
   "Delta",
   "E",
   "Eacute",
   "Ecircumflex",
   "Edieresis",
   "Egrave",
   "Eth",
   "F",
   "G",
   "Gbreve",
   "H",
   "I",
   "Iacute",
   "Icircumflex",
   "Idieresis",
   "Idot",
   "Igrave",
   "J",
   "K",
   "L",
   "Lslash",
   "M",
   "N",
   "Ntilde",
   "O",
   "OE",
   "Oacute",
   "Ocircumflex",
   "Odieresis",
   "Ograve",
   "Oslash",
   "Otilde",
   "P",
   "Q",
   "R",
   "S",
   "Scaron",
   "Scedilla",
   "T",
   "Thorn",
   "U",
   "Uacute",
   "Ucircumflex",
   "Udieresis",
   "Ugrave",
   "V",
   "W",
   "X",
   "Y",
   "Yacute",
   "Ydieresis",
   "Z",
   "a",
   "aacute",
   "acircumflex",
   "acute",
   "adieresis",
   "ae",
   "agrave",
   "ampersand",
   "approxequal",
   "aring",
   "asciicircum",
   "asciitilde",
   "asterisk",
   "at",
   "atilde",
   "b",
   "backslash",
   "bar",
   "braceleft",
   "braceright",
   "bracketleft",
   "bracketright",
   "breve",
   "brokenbar",
   "bullet",
   "c",
   "cacute",
   "caron",
   "ccaron",
   "ccedilla",
   "cedilla",
   "cent",
   "circumflex",
   "colon",
   "comma",
   "copyright",
   "currency",
   "d",
   "dagger",
   "daggerdbl",
   "degree",
   "dieresis",
   "divide",
   "dmacron",
   "dollar",
   "dotaccent",
   "dotlessi",
   "e",
   "eacute",
   "ecircumflex",
   "edieresis",
   "egrave",
   "eight",
   "ellipsis",
   "emdash",
   "endash",
   "equal",
   "eth",
   "exclam",
   "exclamdown",
   "f",
   "fi",
   "five",
   "fl",
   "florin",
   "four",
   "fraction",
   "franc",
   "g",
   "gbreve",
   "germandbls",
   "grave",
   "greater",
   "greaterequal",
   "guillemotleft",
   "guillemotright",
   "guilsinglleft",
   "guilsinglright",
   "h",
   "hungerumlaut",
   "hyphen",
   "i",
   "iacute",
   "icircumflex",
   "idieresis",
   "igrave",
   "infinity",
   "integral",
   "j",
   "k",
   "l",
   "less",
   "lessequal",
   "logicalnot",
   "lozenge",
   "lslash",
   "m",
   "macron",
   "middot",
   "minus",
   "mu",
   "multiply",
   "n",
   "nbspace",
   "nine",
   "notequal",
   "ntilde",
   "numbersign",
   "o",
   "oacute",
   "ocircumflex",
   "odieresis",
   "oe",
   "ogonek",
   "ograve",
   "ohm",
   "one",
   "onehalf",
   "onequarter",
   "onesuperior",
   "ordfeminine",
   "ordmasculine",
   "oslash",
   "otilde",
   "overscore",
   "p",
   "paragraph",
   "parenleft",
   "parenright",
   "partialdiff",
   "percent",
   "period",
   "periodcentered",
   "perthousand",
   "pi",
   "plus",
   "plusminus",
   "product",
   "q",
   "question",
   "questiondown",
   "quotedbl",
   "quotedblbase",
   "quotedblleft",
   "quotedblright",
   "quoteleft",
   "quoteright",
   "quotesinglbase",
   "quotesingle",
   "r",
   "radical",
   "registered",
   "ring",
   "s",
   "scaron",
   "scedilla",
   "section",
   "semicolon",
   "seven",
   "sfthyphen",
   "six",
   "slash",
   "space",
   "sterling",
   "summation",
   "t",
   "thorn",
   "three",
   "threequarters",
   "threesuperior",
   "tilde",
   "trademark",
   "two",
   "twosuperior",
   "u",
   "uacute",
   "ucircumflex",
   "udieresis",
   "ugrave",
   "underscore",
   "v",
   "w",
   "x",
   "y",
   "yacute",
   "ydieresis",
   "yen",
   "z",
   "zero"
};

#define GLYPHFILTER  &win
const struct GlyphFilter win = {
   sizeof(winmac) / sizeof(winmac[0]),
   winmac
};

#else
#define GLYPHFILER (struct GlyphFilter *)0
#endif /* NOANSIWINMAC */


/***** PROTOTYPES */
extern int __cdecl sprintf(char *, const char *, ...);


/***** MACROS */

#define ReadLittleEndianDword(file,dw)  {          \
        dw  = (DWORD)io_ReadOneByte(file) ;        \
        dw |= (DWORD)io_ReadOneByte(file) << 8;    \
        dw |= (DWORD)io_ReadOneByte(file) << 16;   \
        dw |= (DWORD)io_ReadOneByte(file) << 24;   \
        }
				
#ifndef try
#define try __try
#define except __except
#endif


/***** GLOBALS */
HANDLE hInst;



/***** STATIC FUNCTIONS */


/***
** Function: Decrypt
**
** Description:
**   Decrypt a byte.
***/
static DWORD CSum(char *str)
{
   DWORD sum = 0;

   while (*str)
      sum += *str++;

   return sum;
}


/***
** Function: Decrypt
**
** Description:
**   Decrypt a byte.
***/
static char *Encrypt(char *str, char *out)
{
   const USHORT c1 = 52845;
   const USHORT c2 = 22719;
   UBYTE cipher;
   USHORT r = 8366;
   int i;
   
   for (i=0; i<(int)strlen(str); i++) {
      cipher = (UBYTE)(str[i] ^ (r>>8));
      r = (USHORT)((cipher + r) * c1 + c2);
      out[i] = (char)((cipher & 0x3f) + ' ');

      /* Unmap 'bad' characters, that the Registry DB doesn't like. */
      if (out[i]=='=' || out[i]==' ' || out[i]=='@' || out[i]=='"')
         out[i] = 'M';
   }
   out[i] = '\0';

   return out;
}


static char *stristr(char *src, char *word)
{
	int len = strlen(word);
	char *tmp = src;

	while (*src) {
		if (!_strnicmp(src, word, len))
			break;
		src++;
	}

	return src;
}


/***
 ** Function: GetCompany
 **
 ** Description:
 **   Extract the company name out of a copyright string.
 ***/
char *GetCompany(char *buf)
{
   char *company = NULL;
   int done = FALSE;
   UBYTE *token;
   UBYTE *tmp1;
   UBYTE *tmp2;
   UBYTE *tmp3;
   UBYTE *tmp4;
   int i;

   token = buf;

   while (token && !done) {

	   /* Locate the start of the copyright string. */
	   tmp1 = stristr(token, "copyright");
	   tmp2 = stristr(token, "(c)");
	   tmp3 = stristr(token, " c ");
	   if ((tmp4 = strchr(token, COPYSIGN))==NULL)
		   tmp4 = &token[strlen(token)];
	   if (*tmp1==0 && *tmp2==0 && *tmp3==0 && *tmp4==0) {
		   token = NULL;
		   break;
	   } else if (tmp1<tmp2 && tmp1<tmp3 && tmp1<tmp4)
		   token = tmp1;
	   else if (tmp2<tmp3 && tmp2<tmp4)
		   token = tmp2;
	   else if (tmp3<tmp4)
		   token = tmp3;
	   else
		   token = tmp4;

      /* Skip the leading copyright strings/character. */
      if (token[0]==COPYSIGN && token[1]!='\0') {
         token += 2;
      } else if (!_strnicmp(token, "copyright", strlen("copyright"))) {
		  token += strlen("copyright");
	  } else {
		  token += strlen("(c)");
	  }

	  /* Skip blanks. */
	  while(*token && isspace(*token) || *token==',')
		  token++;

	  /* Another copyright word? */
	  if (!_strnicmp((char*)token, "(c)", strlen("(c)")) ||
		  !_strnicmp((char*)token, "copyright", strlen("copyright")) ||
		  token[0]==COPYSIGN)
		  continue;

      /* Skip the years. */
	  company = token;
      if (isdigit(token[0])) {
         while (isdigit(*company) || isspace(*company) ||
				ispunct(*company) || (*company)=='-')
            company++;

         if (*company=='\0')
            break;

         /* Skip strings like "by", up to the beginning of a name that */
         /* starts with an upper case letter. */         
         while (*company && (company[0]<'A' || company[0]>'Z'))
            company++;

         done = TRUE;
      } else {
         continue;
      }
   } 


   /* Did we find it? */
   if (company) {
      while (*company && isspace(*company))
         company++; 


      if (*company=='\0') {
         company=NULL;
      } else {

         /* Terminate the company name. */
         if ((token = (UBYTE*)strchr(company, '.'))!=NULL) {

            /* Period as an initial delimiter, e.g. James, A. B. ?*/
            if (token[-1]>='A' && token[-1]<='Z') {
               if (strchr((char*)&token[1], '.'))
                  token = (UBYTE*)strchr((char*)&token[1], '.');

               /* Check for "James A. Bently, " */
               else if (strchr((char*)&token[1], ',')) {
                  token = (UBYTE*)strchr((char*)&token[1], ',');
                  token[0] = '.';
               }
            }
			token[1] = '\0';
         } else {
			 /* Name ending with a ';'? */
			 if ((token = (UBYTE*)strrchr(company, ';'))) {
				 *token = '\0';
			 }
		 }

		 /* Truncate some common strings. */
		 tmp1 = stristr(company, "all rights reserved");
		 *tmp1 = '\0';

		 /* Remove trailing punctuation character. */
		 for (i=strlen(company)-1; i>0 &&
				(ispunct(company[i]) || isspace(company[i])); i--) {
			 company[i] = 0;
		 }
      }
   }      
              

   return company;
}




/**** FUNCTIONS */

/***
 ** Function: ConvertAnyway
 **
 ** Description:
 **   Ask the user if it is ok to convert. 
 ***/
static errcode ConvertAnyway(const char *vendor, const char *facename)
{
   char tmp[256];
   char msg[1024];
   errcode answer;

   if (vendor==NULL || strlen(vendor)==0) {
      LoadString(hInst, IDS_RECOGNIZE1, tmp, sizeof(tmp));
      sprintf(msg, tmp, facename);
   } else {
      LoadString(hInst, IDS_RECOGNIZE2, tmp, sizeof(tmp));
      sprintf(msg, tmp, facename, vendor);
   }      
   LoadString(hInst, IDS_MAINMSG, tmp, sizeof(tmp));
   strcat(msg, tmp);
   LoadString(hInst, IDS_CAPTION, tmp, sizeof(tmp));
   answer = (errcode)MessageBox(NULL, msg, tmp, QUERY);
   SetLastError(0);

   return answer;
}



/***
 ** Function: CheckCopyright
 **
 ** Description:
 **   This is the callback function that verifies that
 **   the converted font is copyrighted by a company who
 **   has agreed to having their fonts converted by
 **   this software. These companies are registered in the
 **   registry data base.
 ***/


static errcode CheckCopyright(const char *facename,
                              const char *copyright,
                              const char *notice)
{
#ifdef NOCOPYRIGHTS
   return SKIP;
#else
   HKEY key;
   char tmp[256];
   char *company = NULL;
   char buf[1024];
   int done = FALSE;
   short result = FAILURE;
   

   /* Access the REG data base. */
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, SUBKEY_TYPE1COPYRIGHTS, 0,
                    KEY_QUERY_VALUE, &key)==ERROR_SUCCESS) { 


      /* Look for the company name in the /notice string. */
      if (notice && notice[0]) {
         strcpy(buf, notice);
         company = GetCompany(buf);
      }

      /* Look in the /copyright string if the company name was not found. */
      if (company==NULL && copyright && copyright[0]) {
         strcpy(buf, copyright);
         company = GetCompany(buf);
      }


#ifdef SHOWCOPYRIGHTS
      LogError(MSG_INFO, MSG_Copyright, company);
      Encrypt(company, tmp);
      sprintf(&tmp[strlen(tmp)], "(%d)\n", CSum(tmp));
      LogError(MSG_INFO, MSG_Encoding, tmp);
#else

      /* Did not find a company name? */
      if (company==NULL &&
          ((notice==NULL || notice[0]=='\0'||
            strstr(notice, "Copyright")==NULL) &&
           (copyright==NULL || copyright[0]=='\0' ||
            strstr(copyright, "Copyright")==NULL))) {

         /* No known copyright. */
         LogError(MSG_WARNING, MSG_NOCOPYRIGHT, NULL);
         result = MAYBEOK;

      /* Strange copyright format? */
      } else if (company==NULL || company[0]=='\0') {
         if (notice || notice[0])
            LogError(MSG_WARNING, MSG_BADFORMAT, notice);
         else
            LogError(MSG_WARNING, MSG_BADFORMAT, copyright);

         result = MAYBEOK;

      /* Found copyright! */
      } else {
         DWORD size;
         DWORD csum;

         size = 4;
         if (RegQueryValueEx(key, Encrypt(company, tmp), NULL, NULL,
                             (LPBYTE)&csum, &size)==ERROR_SUCCESS) {
            
            /* A positive match -> ok to convert. */
            if (CSum(tmp)==csum) {
               LogError(MSG_INFO, MSG_COPYRIGHT, company);
               result = SUCCESS;
            } else {
               LogError(MSG_ERROR, MSG_BADCOPYRIGHT, company);
               result = SKIP;
            }
         } else {
            LogError(MSG_WARNING, MSG_BADCOPYRIGHT, company);
            result = MAYBEOK;
         }
      }               
#endif

      RegCloseKey(key);

      /* Give the user the final word. */
      if (result==FAILURE) {
         if (ConvertAnyway(company, facename)==TRUE)
            result = SUCCESS;
      }


   /* No copyright key in the registry? */
   } else {
      LogError(MSG_ERROR, MSG_NODB, NULL);
      result = FAILURE;
   }   


   return result;
#endif
}



/***
 ** Function: NTCheckCopyright
 **
 ** Description:
 **   This is the callback function that verifies that
 **   the converted font is copyrighted by a company who
 **   has agreed to having their fonts converted by
 **   this software. These companies are registered in the
 **   registry data base.
 ***/
static errcode NTCheckCopyright(const char *facename,
                                const char *copyright,
                                const char *notice)
{
   HKEY key;
   char tmp[256];
   char *company = NULL;
   char buf[1024];
   int done = FALSE;
   short result = FAILURE;
   

   /* Access the REG data base. */
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, SUBKEY_TYPE1COPYRIGHTS, 0,
                    KEY_QUERY_VALUE, &key)==ERROR_SUCCESS) { 


      /* Look for the company name in the /notice string. */
      if (notice && notice[0]) {
         strcpy(buf, notice);
         company = GetCompany(buf);
      }

      /* Look in the /copyright string if the company name was not found. */
      if (company==NULL && copyright && copyright[0]) {
         strcpy(buf, copyright);
         company = GetCompany(buf);
      }

      /* Did not find a company name? */
      if (company==NULL &&
          ((notice==NULL || notice[0]=='\0'||
            strstr(notice, "Copyright")==NULL) &&
           (copyright==NULL || copyright[0]=='\0' ||
            strstr(copyright, "Copyright")==NULL))) {

         /* No known copyright. */
         result = MAYBE;

      /* Strange copyright format? */
      } else if (company==NULL || company[0]=='\0') {
         result = MAYBE;

      /* Found copyright! */
      } else {
         DWORD size;
         DWORD csum;

         /* remember for future use. */
         strncpy(lastVendor, company, 256);
         lastVendor[MIN(255, strlen(company))] = '\0';

         size = 4;
         if (RegQueryValueEx(key, Encrypt(company, tmp), NULL, NULL,
                             (LPBYTE)&csum, &size)==ERROR_SUCCESS) {
            
            /* A positive match -> ok to convert. */
            if (CSum(tmp)==csum) {
               result = SUCCESS;
            } else {
               result = FAILURE;
            }
         } else {
            result = MAYBE;
         }
      }               

      RegCloseKey(key);


   /* No copyright key in the registry? */
   } else {
      result = FAILURE;
   }   


   lastCP = result;

   return FAILURE;
}


/***
 ** Function: _Progress
 **
 ** Description:
 **   This is the internal progress callback function that 
 **   computes an percentage-done number, based on the
 **   number of converted glyphs.
 ***/
static void _Progress(short type, void *generic, void *arg)
{
   struct callFrame *f = arg;

   /* Processing glyphs or wrapping up? */
   if (type==0 || type==1) 
      f->done++;
   else
      f->done = MIN(sizeof(winmac)/sizeof(winmac[0]), f->done+10);
   
   if ((f->done-f->last)>MIN_PROGRESS) {
      f->Progress((short)(f->done*100/(sizeof(winmac)/sizeof(winmac[0]))),
                  f->arg);
      f->last = f->done;
   }
   
   UNREFERENCED_PARAMETER(type);
   UNREFERENCED_PARAMETER(generic);
   SetLastError(0L);
}
            
static BOOL ReadStringFromOffset(struct ioFile *file,
                                 const DWORD dwOffset, 
                                 char *pszString,
                                 int cLen,
                                 BOOL bStrip)
{
    BOOL result = TRUE;
    DWORD offset;

    /* Get offset to string. */
    io_FileSeek(file, dwOffset);

    /* Read the offset. */

    ReadLittleEndianDword(file, offset);

    /*  Get the string. */
    (void)io_FileSeek(file, offset);
    if (io_FileError(file) != SUCCESS) {
        result = FALSE;
    } else {
        int i;

        i=0;
        while (io_FileError(file)==SUCCESS && i<cLen) {
            pszString[i] = (UBYTE)io_ReadOneByte(file);
            if (pszString[i]=='\0')
                break;

            /* Replace all dashes with spaces. */
            if (bStrip && pszString[i]=='-')
                pszString[i]=' ';
            i++;
        }
    }

    return TRUE;
}
                                 



/**** FUNCTIONS */

/***
 ** Function: ConvertTypeFaceA
 **
 ** Description:
 **   Convert a T1 font into a TT font file. This is the
 **   simplified interface used by the Win32 DLL, with the
 **   ANSI interface.
 ***/
short STDCALL ConvertTypefaceAInternal(const char *type1,
                               const char *metrics,
                               const char *truetype,
                               const void (STDCALL *Progress)(short, void*),
                               void *arg)
{                        
   struct callFrame f;
   struct callProgress p;
   struct T1Arg t1Arg;
   struct TTArg ttArg;
   short status;


   /* Check parameters. */
   if (type1==NULL || metrics==NULL)
      return FAILURE;

   /* Set up arguments to ConvertTypefaceA() */
   t1Arg.filter = GLYPHFILTER;
   t1Arg.upem = (short)2048;
   t1Arg.name = (char *)type1;
   t1Arg.metrics = (char *)metrics;
   ttArg.precision = (short)50;
   ttArg.name = (char *)truetype;
   ttArg.tag = VERSTR;

   /* Use progress gauge */
   if (Progress) {
      LogError(MSG_INFO, MSG_STARTING, type1);

      f.Progress = Progress;
      f.done = 0;
      f.last = 0;
      f.arg = arg;
      p.arg = &f;
      p.cb = _Progress;
      status = ConvertT1toTT(&ttArg, &t1Arg, CheckCopyright, &p);
      Progress(100, arg);
   } else {
      status = ConvertT1toTT(&ttArg, &t1Arg, CheckCopyright, NULL);
   }

   
   return status;
}



short STDCALL ConvertTypefaceA(const char *type1,
                               const char *metrics,
                               const char *truetype,
                               const void (STDCALL *Progress)(short, void*),
                               void *arg)
{

    short bRet;

    try
    {
        bRet = ConvertTypefaceAInternal(type1,
                                        metrics,
                                        truetype,
                                        Progress,
                                        arg);
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
    #if 0
        ASSERTGDI(
            GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR,
            "ttfd!ttfdSemLoadFontFile, strange exception code\n"
            );
    #endif

        bRet = BADINPUTFILE;

    }

    return bRet;
}


short STDCALL FindPfb (
    char *pszPFM,
    char *achPFB
);


/***
** Function: CheckPfmA
**
** Description:
**   This function determines if there is a pfm/pfb pair of
**   files that makes up an Adobe Type 1 font, and determins
**   the descriptive face name of it.
**
** Returns: 16-bit encoded value indicating error and type of file where
**          error occurred.  (see fvscodes.h) for definitions.
**          The following table lists the "status" portion of the codes
**          returned.
**
**           FVS_SUCCESS           
**           FVS_INVALID_FONTFILE  
**           FVS_FILE_OPEN_ERR   
**           FVS_INVALID_ARG
**           FVS_FILE_IO_ERR
**           FVS_BAD_VERSION
***/

short STDCALL CheckPfmA(
    char  *pszPFM,
    DWORD  cjDesc,
    char  *pszDesc,
    DWORD  cjPFB,
    char  *pszPFB
)
{
   struct ioFile *file;
   char szDriver[MAX_PATH];
   short result = FVS_MAKE_CODE(FVS_SUCCESS, FVS_FILE_UNK);
   short ver;

   char achPFB[MAX_PATH];

   char  *psz_PFB;
   DWORD  cjPFB1;

   if (pszPFB)
   {
       psz_PFB = pszPFB;
       cjPFB1 = cjPFB;
   }
   else
   {
       psz_PFB = (char *)achPFB;
       cjPFB1 = MAX_PATH;
   }

   /* Check parameter. */
   if (pszPFM==NULL || ((strlen(pszPFM)+3) >= cjPFB1))
      return FVS_MAKE_CODE(FVS_INVALID_ARG, FVS_FILE_UNK);

   // check if pfb file exists and find the path to it:

    result = FindPfb(pszPFM, psz_PFB);
    if (FVS_STATUS(result) != FVS_SUCCESS)
        return result;

   /****
    * Locate the pszDescriptive name of the font.
    */

   if ((file = io_OpenFile(pszPFM, READONLY))==NULL)
      return FVS_MAKE_CODE(FVS_FILE_OPEN_ERR, FVS_FILE_PFM);

   (void)io_ReadOneByte(file);     /* Skip the revision number. */
   ver = (short)io_ReadOneByte(file);

   if (ver > 3) {
      /*  ERROR - unsupported format */
      result = FVS_MAKE_CODE(FVS_BAD_VERSION, FVS_FILE_PFM);
   } else {

      /* Read the driver name. */
      if (!ReadStringFromOffset(file, DFDRIVERINFO, szDriver, 
                                    sizeof(szDriver), FALSE))
      {
          result = FVS_MAKE_CODE(FVS_FILE_IO_ERR, FVS_FILE_PFM);
      }
      /* Is it "PostScript" ? */
      else if (_stricmp(szDriver, "PostScript"))
      {
          result = FVS_MAKE_CODE(FVS_INVALID_FONTFILE, FVS_FILE_PFM);
      }
      /* Only get description if asked to do so. */
      else if (pszDesc && !ReadStringFromOffset(file, DFFACE, pszDesc, cjDesc, TRUE))
      {
          result = FVS_MAKE_CODE(FVS_FILE_IO_ERR, FVS_FILE_PFM);
      }
   }

   (void)io_CloseFile(file);

   return result;
}


/***
 ** Function: DllMain
 **
 ** Description:
 **   Main function of the DLL. Use to cache the module handle,
 **   which is needed to pop-up messages and peek in the registry.
 ***/
BOOL WINAPI DllMain(PVOID hmod,
                    ULONG ulReason,
                    PCONTEXT pctx OPTIONAL)
{
   if (ulReason == DLL_PROCESS_ATTACH) {
      hInst = hmod;
      DisableThreadLibraryCalls(hInst);
   }


   UNREFERENCED_PARAMETER(pctx);

   return TRUE;
}


/***
** Function: CheckCopyrightsA
**
** Description:
**   This function verifies that it is ok to convert the font. This is
**   done by faking an installation.
***/
short STDCALL CheckCopyrightAInternal(const char *szPFB,
                              const DWORD wSize,
                              char *szVendor)
{
   struct T1Arg t1Arg;
   struct TTArg ttArg;
   
   /* Set up arguments to ConvertTypefaceA() */
   t1Arg.metrics = NULL;
   t1Arg.upem = (short)2048;
   t1Arg.filter = GLYPHFILTER;
   t1Arg.name = szPFB;
   ttArg.precision = (short)200;
   ttArg.tag = NULL;
   ttArg.name = "NIL:";
   lastCP = FAILURE;
   strcpy(lastVendor, "");
   (void)ConvertT1toTT(&ttArg, &t1Arg, NTCheckCopyright, NULL);
   strncpy(szVendor, lastVendor, wSize);
   szVendor[MIN(wSize, strlen(lastVendor))] = '\0';
  
   return lastCP;
}


short STDCALL CheckCopyrightA(const char *szPFB,
                              const DWORD wSize,
                              char *szVendor)
{
    short iRet;

    try
    {
        iRet = CheckCopyrightAInternal(szPFB,wSize,szVendor);
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        iRet = BADINPUTFILE;
    }
    return iRet;

}






/******************************Public*Routine******************************\
*
* short STDCALL CheckInfA (
*
* If pfm and inf files are in the same directory only pfm is recognized
* and inf file is ignored.
*
* History:
*  27-Apr-1994 -by- Bodin Dresevic [BodinD]
* Wrote it.
*
* Returns: 16-bit encoded value indicating error and type of file where
*          error occurred.  (see fvscodes.h) for definitions.
*          The following table lists the "status" portion of the codes
*          returned.
*
*           FVS_SUCCESS           
*           FVS_INVALID_FONTFILE  
*           FVS_FILE_OPEN_ERR   
*           FVS_FILE_BUILD_ERR  
*           FVS_FILE_EXISTS
*           FVS_INSUFFICIENT_BUF
*
\**************************************************************************/


short CreatePFM(char *pszINF, char *pszAFM, char *pszPFM);
BOOL bGetDescFromInf(char * pszINF, DWORD cjDesc, char *pszDesc);

BOOL bFileExists(char *pszFile)
{
    HFILE hf;

    if ((hf = _lopen(pszFile, OF_READ)) != -1)
    {
        _lclose(hf);
        return TRUE;
    }

    return FALSE;
}

short STDCALL CheckInfA (
    char *pszINF,
    DWORD cjDesc,
    char *pszDesc,
    DWORD cjPFM,
    char *pszPFM,
    DWORD cjPFB,
    char *pszPFB,
    BOOL *pbCreatedPFM,
    char *pszFontPath
)
{
    char achPFM[MAX_PATH];
    char achPFB[MAX_PATH];
    char achAFM[MAX_PATH];

    DWORD  cjKey;
    char *pszParent = NULL; // points to the where parent dir of the inf file is
    char *pszBare = NULL; // "bare" .inf name, initialization essential
    short result = FVS_MAKE_CODE(FVS_SUCCESS, FVS_FILE_UNK);
    BOOL bAfmExists = FALSE;
    BOOL bPfbExists = FALSE;

    //
    // This is a real hack use of pbCreatedPFM.
    // It's the best solution with the time we have.
    //
    BOOL bCheckForExistingPFM = *pbCreatedPFM;

    *pbCreatedPFM = FALSE;

// example:
// if pszINF -> "c:\psfonts\fontinfo\foo_____.inf"
// then pszParent -> "fontinfo\foo_____.inf"

    cjKey = strlen(pszINF) + 1;

    if (cjKey < 5)          // 5 = strlen(".pfm") + 1;
        return FVS_MAKE_CODE(FVS_INVALID_FONTFILE, FVS_FILE_INF);

// check if a pfm file exists in the SAME directory.
// Use the buffer on the stack to produce the path for the pfm file:

    strcpy(achPFM, pszINF);
    strcpy(&achPFM[cjKey - 5],".PFM");

// try to open pfm file to check if it exists:

    if (bCheckForExistingPFM && bFileExists(achPFM))
    {
    // we found the pfm file, therefore we do not report this .inf file.

        return FVS_MAKE_CODE(FVS_FILE_EXISTS, FVS_FILE_PFM);
    }

// pfm file is NOT found, go on to check if .afm and .pfb files exists:
// We will first check if .afm and .pfb files exists in the same dir as .inf

    strcpy(achAFM, pszINF);
    strcpy(&achAFM[cjKey - 5],".AFM");

    strcpy(achPFB, pszINF);
    strcpy(&achPFB[cjKey - 5],".PFB");

    bAfmExists = bFileExists(achAFM);
    bPfbExists = bFileExists(achPFB);

    if (!bAfmExists || !bPfbExists)
    {
    // we did not find the .afm and .pfb files in the same dir as .inf
    // we will check two more directories for the .afm and .pfb files
    // 1) the parent directory of the .inf file for .pfb file
    // 2) the afm subdirectory of the .inf parent directory for .afm file
    //
    // This is meant to handle the standard configuration of files produced
    // on user's hard drive by unlocking fonts from Adobe's CD or from a
    // previous installation of atm manager on this machine.
    // This configuration is as follows:
    // c:\psfonts\           *.pfb files are here
    // c:\psfonts\afm        *.afm files are here
    // c:\psfonts\fontinfo   *.inf files are here
    // c:\psfonts\pfm        *.pfm files that are created on the fly
    //                         are PUT here by atm.
    // We will instead put the files in windows\system dir where all other
    // fonts are, it may not be possible to write pmf files on the media
    // from where we are installing fonts

        pszBare = &pszINF[cjKey - 5];
        for ( ; pszBare > pszINF; pszBare--)
        {
            if ((*pszBare == '\\') || (*pszBare == ':'))
            {
                pszBare++; // found it
                break;
            }
        }

    // check if full path to .inf file was passed in or a bare
    // name itself was passed in to look for .inf file in the current dir

        if ((pszBare > pszINF) && (pszBare[-1] == '\\'))
        {
        // skip '\\' and search backwards for another '\\':

            for (pszParent = &pszBare[-2]; pszParent > pszINF; pszParent--)
            {
                if ((*pszParent == '\\') || (*pszParent == ':'))
                {
                    pszParent++; // found it
                    break;
                }
            }

        // create .pfb file name in the .inf parent directory:

            strcpy(&achPFB[pszParent - pszINF], pszBare);
            strcpy(&achPFB[strlen(achPFB) - 4], ".PFB");

        // create .afm file name in the afm subdirectory of the .inf
        // parent directory:

            strcpy(&achAFM[pszParent - pszINF], "afm\\");
            strcpy(&achAFM[pszParent - pszINF + 4], pszBare);
            strcpy(&achAFM[strlen(achAFM) - 4], ".AFM");

        }
        else if (pszBare == pszINF)
        {
        // bare name was passed in, to check for the inf file in the "." dir:

            strcpy(achPFB, "..\\");
            strcpy(&achPFB[3], pszBare);   // 3 == strlen("..\\")
            strcpy(&achPFB[strlen(achPFB) - 4], ".PFB");

            strcpy(achAFM, "..\\afm\\");
            strcpy(&achAFM[7], pszBare);   // 7 == strlen("..\\afm\\")
            strcpy(&achAFM[strlen(achAFM) - 4], ".AFM");
        }
        else
        {
            return FVS_MAKE_CODE(FVS_INVALID_FONTFILE, FVS_FILE_UNK);
        }

   // check again if we can find the files, if not fail.

       if (!bAfmExists && !bFileExists(achAFM))
          return FVS_MAKE_CODE(FVS_FILE_OPEN_ERR, FVS_FILE_AFM);
       if (!bPfbExists && !bFileExists(achPFB))
          return FVS_MAKE_CODE(FVS_FILE_OPEN_ERR, FVS_FILE_PFB);
    }

// now we have paths to .inf .afm and .pfb files. Now let us see
// what the caller wants from us:

    if (pszDesc)
    {
    // we need to return description string in the buffer supplied

        if (!bGetDescFromInf(pszINF, (DWORD)cjDesc, pszDesc))
            return FVS_MAKE_CODE(FVS_INVALID_FONTFILE, FVS_FILE_INF);
    }

// copy pfb file path out if requested

    if (pszPFB)
    {
        if ((strlen(achPFB) + 1) < cjPFB)
            strcpy(pszPFB,achPFB);
        else
            return FVS_MAKE_CODE(FVS_INSUFFICIENT_BUF, FVS_FILE_UNK); 
    }

// the caller wants a pfm file created from inf,afm files
// For now and probably for ever we will put this file in
// the %windir%\system, or %windir%\fonts for the secure system.

    if (pszPFM)
    {
        UINT cjSystemDir;
        char *pszAppendHere;  // append "bare" name here

    // copy the first directory of the font path into the buffer provided
    // It is expected that this routine will get something like
    // "c:\foo" pointing to font path

        strcpy(achPFM,pszFontPath);
        pszAppendHere = &achPFM[strlen(pszFontPath) - 1];

        if (*pszAppendHere != '\\')
        {
             pszAppendHere++;
            *pszAppendHere = '\\';
        }
        pszAppendHere++;

    // find bare name of the .inf file if we do not have already:

        if (!pszBare)
        {
            pszBare = &pszINF[cjKey - 5];
            for ( ; pszBare > pszINF; pszBare--)
            {
                if ((*pszBare == '\\') || (*pszBare == ':'))
                {
                    pszBare++; // found it
                    break;
                }
            }
        }

    // append Bare name to the %windir%system\ path

        strcpy(pszAppendHere, pszBare);

    // finally change .inf extension to .pfm extension

        strcpy(&pszAppendHere[strlen(pszAppendHere) - 4], ".PFM");

    // copy out:

        strcpy(pszPFM, achPFM);
        
        result = CreatePFM(pszINF, achAFM, pszPFM);
        *pbCreatedPFM = (FVS_STATUS(result) == FVS_SUCCESS);

        if (!(*pbCreatedPFM))
            return result;
    }

    return FVS_MAKE_CODE(FVS_SUCCESS, FVS_FILE_UNK);
}


/******************************Public*Routine******************************\
*
* short STDCALL CheckType1AInternal
*
* Effects: See if we are going to report this as a valid type 1 font
*
* Warnings:
*
* History:
*  29-Apr-1994 -by- Bodin Dresevic [BodinD]
* Wrote it.
*
* Returns: 16-bit encoded value indicating error and type of file where
*          error occurred.  (see fvscodes.h) for definitions.
*          The following table lists the "status" portion of the codes
*          returned.
*
*           FVS_SUCCESS           
*           FVS_INVALID_FONTFILE  
*           FVS_FILE_OPEN_ERR   
*           FVS_FILE_BUILD_ERR
*           FVS_INVALID_ARG
*           FVS_FILE_IO_ERR
*           FVS_BAD_VERSION
*           FVS_FILE_EXISTS
*           FVS_INSUFFICIENT_BUF
*
\**************************************************************************/


short STDCALL CheckType1AInternal (
    char *pszKeyFile,
    DWORD cjDesc,
    char *pszDesc,
    DWORD cjPFM,
    char *pszPFM,
    DWORD cjPFB,
    char *pszPFB,
    BOOL *pbCreatedPFM,
    char *pszFontPath

)
{
    DWORD  cjKey;

    *pbCreatedPFM = FALSE; // initialization is essential.

    cjKey = strlen(pszKeyFile) + 1;

    if (cjKey < 5)          // 5 = strlen(".pfm") + 1;
        return FVS_MAKE_CODE(FVS_INVALID_FONTFILE, FVS_FILE_UNK);

    if (!_strcmpi(&pszKeyFile[cjKey - 5], ".PFM"))
    {
    // copy out pfm string when asked to do so:

        if (pszPFM && (cjKey < cjPFM))
        {
            if (cjKey < cjPFM)
                strcpy(pszPFM, pszKeyFile);
            else
                return FVS_MAKE_CODE(FVS_INSUFFICIENT_BUF, FVS_FILE_UNK);
        }

        return CheckPfmA(
                   pszKeyFile,
                   cjDesc,
                   pszDesc,
                   cjPFB,
                   pszPFB
                   );
    }
    else if (!_strcmpi(&pszKeyFile[cjKey - 5], ".INF"))
    {
        return CheckInfA (
                   pszKeyFile,
                   cjDesc,
                   pszDesc,
                   cjPFM,
                   pszPFM,
                   cjPFB,
                   pszPFB,
                   pbCreatedPFM,
                   pszFontPath
                   );
    }
    else
    {
    // this font is not our friend

        return FVS_MAKE_CODE(FVS_INVALID_FONTFILE, FVS_FILE_UNK);
    }
}


/******************************Public*Routine******************************\
*
* CheckType1WithStatusA, try / except wrapper
*
* Effects:
*
* Warnings:
*
* History:
*  14-Jun-1994 -by- Bodin Dresevic [BodinD]
* Wrote it.
*
* Returns: 16-bit encoded value indicating error and type of file where
*          error occurred.  (see fvscodes.h) for definitions.
*          The following table lists the "status" portion of the codes
*          returned.
*
*           FVS_SUCCESS           
*           FVS_INVALID_FONTFILE  
*           FVS_FILE_OPEN_ERR   
*           FVS_FILE_BUILD_ERR
*           FVS_INVALID_ARG
*           FVS_FILE_IO_ERR
*           FVS_BAD_VERSION
*           FVS_FILE_EXISTS
*           FVS_INSUFFICIENT_BUF
*           FVS_EXCEPTION
*         
\**************************************************************************/

short STDCALL CheckType1WithStatusA (
    char *pszKeyFile,
    DWORD cjDesc,
    char *pszDesc,
    DWORD cjPFM,
    char *pszPFM,
    DWORD cjPFB,
    char *pszPFB,
    BOOL *pbCreatedPFM,
    char *pszFontPath
)
{
    short status;
    try
    {
        status = CheckType1AInternal (
                   pszKeyFile,
                   cjDesc,
                   pszDesc,
                   cjPFM,
                   pszPFM,
                   cjPFB,
                   pszPFB,
                   pbCreatedPFM,
                   pszFontPath);
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = FVS_MAKE_CODE(FVS_EXCEPTION, FVS_FILE_UNK);
    }

    return status;
}

/******************************Public*Routine******************************\
*
* CheckType1A, try / except wrapper
*
* Effects:
*
* Warnings:
*
* History:
*  14-Jun-1994 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL STDCALL CheckType1A (
    char *pszKeyFile,
    DWORD cjDesc,
    char *pszDesc,
    DWORD cjPFM,
    char *pszPFM,
    DWORD cjPFB,
    char *pszPFB,
    BOOL *pbCreatedPFM,
    char *pszFontPath
)
{
    short status = CheckType1WithStatusA(pszKeyFile,
                                         cjDesc,
                                         pszDesc,
                                         cjPFM,
                                         pszPFM,
                                         cjPFB,
                                         pszPFB,
                                         pbCreatedPFM,
                                         pszFontPath);

    return (FVS_STATUS(status) == FVS_SUCCESS);
}


/******************************Public*Routine******************************\
*
* FindPfb, given pfm file, see if pfb file exists in the same dir or in the
* parent directory of the pfm file
*
* History:
*  14-Jun-1994 -by- Bodin Dresevic [BodinD]
* Wrote it.
*
* Returns: 16-bit encoded value indicating error and type of file where
*          error occurred.  (see fvscodes.h) for definitions.
*          The following table lists the "status" portion of the codes
*          returned.
*
*           FVS_SUCCESS           
*           FVS_INVALID_FONTFILE  
*           FVS_FILE_OPEN_ERR   
*
\**************************************************************************/


short STDCALL FindPfb (
    char *pszPFM,
    char *achPFB
)
{
    DWORD  cjKey;
    char *pszParent = NULL; // points to the where parent dir of the inf file is
    char *pszBare = NULL;   // "bare" .inf name, initialization essential

// example:
// if pszPFM -> "c:\psfonts\pfm\foo_____.pfm"
// then pszParent -> "pfm\foo_____.pfm"

    cjKey = strlen(pszPFM) + 1;

    if (cjKey < 5)          // 5 = strlen(".pfm") + 1;
        return FVS_MAKE_CODE(FVS_INVALID_FONTFILE, FVS_FILE_PFM);

// go on to check if .pfb file exists:
// We will first check .pfb file exists in the same dir as .pfm

    strcpy(achPFB, pszPFM);
    strcpy(&achPFB[cjKey - 5],".PFB");

    if (!bFileExists(achPFB))
    {
    // we did not find the .pfb file in the same dir as .pfm
    // Now check the parent directory of the .pfm file

        pszBare = &pszPFM[cjKey - 5];
        for ( ; pszBare > pszPFM; pszBare--)
        {
            if ((*pszBare == '\\') || (*pszBare == ':'))
            {
                pszBare++; // found it
                break;
            }
        }

    // check if full path to .pfm was passed in or a bare
    // name itself was passed in to look for .pfm file in the current dir

        if ((pszBare > pszPFM) && (pszBare[-1] == '\\'))
        {
        // skip '\\' and search backwards for another '\\':

            for (pszParent = &pszBare[-2]; pszParent > pszPFM; pszParent--)
            {
                if ((*pszParent == '\\') || (*pszParent == ':'))
                {
                    pszParent++; // found it
                    break;
                }
            }

        // create .pfb file name in the .pfm parent directory:

            strcpy(&achPFB[pszParent - pszPFM], pszBare);
            strcpy(&achPFB[strlen(achPFB) - 4], ".PFB");

        }
        else if (pszBare == pszPFM)
        {
        // bare name was passed in, to check for the inf file in the "." dir:

            strcpy(achPFB, "..\\");
            strcpy(&achPFB[3], pszBare);   // 3 == strlen("..\\")
            strcpy(&achPFB[strlen(achPFB) - 4], ".PFB");
        }
        else
        {
            return FVS_MAKE_CODE(FVS_INVALID_FONTFILE, FVS_FILE_PFM); // We should never get here.
        }

   // check again if we can find the file, if not fail.

       if (!bFileExists(achPFB))
       {
           return FVS_MAKE_CODE(FVS_FILE_OPEN_ERR, FVS_FILE_PFB);
       }
    }

// now we have paths to .pfb file in the buffer provided by the caller.

    return FVS_MAKE_CODE(FVS_SUCCESS, FVS_FILE_UNK);
}
