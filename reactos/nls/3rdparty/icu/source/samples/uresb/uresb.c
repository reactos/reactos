/*
*******************************************************************************
*
*   Copyright (C) 1999-2007, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  uresb.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2000sep6
*   created by: Vladimir Weinstein
*/

/******************************************************************************
 * This program prints out resource bundles - example for
 * ICU workshop
 * TODO: make a complete i18n layout for this program.
 ******************************************************************************/

#include "unicode/putil.h"
#include "unicode/ures.h"
#include "unicode/ustdio.h"
#include "unicode/uloc.h"
#include "unicode/ustring.h"
#include "uoptions.h"
#include "toolutil.h"

#include <string.h>
#include <stdlib.h>
#ifdef WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#define URESB_DEFAULTTRUNC 40

static char *currdir = NULL;
/*--locale sr_YU and --encoding cp855
 * are interesting on Win32
 */

static const char *locale = NULL;
static const char *encoding = NULL;
static const char *resPath = NULL;
static const int32_t indentsize = 4;
static UFILE *outerr = NULL;
static int32_t truncsize = URESB_DEFAULTTRUNC;
static UBool trunc = FALSE;

const UChar baderror[] = { 0x0042, 0x0041, 0x0044, 0x0000 };

const UChar *getErrorName(UErrorCode errorNumber);
void reportError(UErrorCode *status);
static UChar *quotedString(const UChar *string);
void printOutBundle(UFILE *out, UResourceBundle *resource, int32_t indent, UErrorCode *status);
void printIndent(UFILE *out, int32_t indent);
void printHex(UFILE *out, const int8_t *what);

static UOption options[]={
    UOPTION_HELP_H,
    UOPTION_HELP_QUESTION_MARK,
    { "locale", NULL, NULL, NULL, 'l', UOPT_REQUIRES_ARG, 0 },
    UOPTION_ENCODING,
    { "path", NULL, NULL, NULL, 'p', UOPT_OPTIONAL_ARG, 0 },
    { "truncate", NULL, NULL, NULL, 't', UOPT_OPTIONAL_ARG, 0 },
    UOPTION_VERBOSE
};

static UBool VERBOSE = FALSE;

extern int
main(int argc, char* argv[]) {

    UResourceBundle *bundle = NULL;
    UErrorCode status = U_ZERO_ERROR;
    UFILE *out = NULL;
    int32_t i = 0;
    const char* arg;
    char resPathBuffer[1024];
#ifdef WIN32
    currdir = _getcwd(NULL, 0);
#else
    currdir = getcwd(NULL, 0);
#endif

    argc=u_parseArgs(argc, argv, sizeof(options)/sizeof(options[0]), options);

    /* error handling, printing usage message */
    if(argc<0) {
        fprintf(stderr,
            "error in command line argument \"%s\"\n",
            argv[-argc]);
    }
    if(argc<2 || options[0].doesOccur || options[1].doesOccur) {
        fprintf(stderr,
            "usage: %s [-options] locale(s)\n",
            argv[0]);
        return argc<0 ? U_ILLEGAL_ARGUMENT_ERROR : U_ZERO_ERROR;
    }

    if(options[2].doesOccur) {
        locale = options[2].value;
    } else {
        locale = 0;
    }

    if(options[3].doesOccur) {
        encoding = options[3].value;
    } else {
        encoding = NULL;
    }

    if(options[4].doesOccur) {
        if(options[4].value != NULL) {
            resPath = options[4].value; /* we'll use users resources */
        } else {
            resPath = NULL; /* we'll use ICU system resources for dumping */
        }
    } else {
        strcpy(resPathBuffer, currdir);
        /*strcat(resPathBuffer, U_FILE_SEP_STRING);
        strcat(resPathBuffer, "uresb");*/
        resPath = resPathBuffer; /* we'll just dump uresb samples resources */
    }

    if(options[5].doesOccur) {
        trunc = TRUE;
        if(options[5].value != NULL) {
            truncsize = atoi(options[5].value); /* user defined printable size */
        } else {
            truncsize = URESB_DEFAULTTRUNC; /* we'll use default omitting size */
        }
    } else {
        trunc = FALSE;
    }

    if(options[6].doesOccur) {
        VERBOSE = TRUE;
    }

    outerr = u_finit(stderr, locale, encoding);
    out = u_finit(stdout, locale, encoding); 

    for(i = 1; i < argc; ++i) {
        status = U_ZERO_ERROR;
        arg = getLongPathname(argv[i]);

        u_fprintf(out, "uresb: processing file \"%s\" in path \"%s\"\n", arg, resPath);
        bundle = ures_open(resPath, arg, &status);
        if(U_SUCCESS(status)) {
            u_fprintf(out, "%s\n", arg);
            printOutBundle(out, bundle, 0, &status);
        } else {
            reportError(&status);
        }

        ures_close(bundle);
    }



    u_fclose(out);
    u_fclose(outerr);
    return 0;
}

void printIndent(UFILE *out, int32_t indent) {
    char inchar[256];
    int32_t i = 0;
    for(i = 0; i<indent; i++) {
        inchar[i] = ' ';
    }
    inchar[indent] = '\0';
    u_fprintf(out, "%s", inchar);
}

void printHex(UFILE *out, const int8_t *what) {
  u_fprintf(out, "%02X", (uint8_t)*what);
}

static UChar *quotedString(const UChar *string) {
    int len = u_strlen(string);
    int alen = len;
    const UChar *sp;
    UChar *newstr, *np;

    for (sp = string; *sp; ++sp) {
        switch (*sp) {
            case '\n':
            case 0x0022:
                ++alen;
                break;
        }
    }

    newstr = (UChar *) malloc((1 + alen) * sizeof(*newstr));
    for (sp = string, np = newstr; *sp; ++sp) {
        switch (*sp) {
            case '\n':
                *np++ = 0x005C;
                *np++ = 0x006E;
                break;

            case 0x0022:
                *np++ = 0x005C;
                
            default:
                *np++ = *sp;
                break;
        }
    }
    *np = 0;

    return newstr;
}

void printOutBundle(UFILE *out, UResourceBundle *resource, int32_t indent, UErrorCode *status) {
    int32_t i = 0;
    const char *key = ures_getKey(resource);

    switch(ures_getType(resource)) {
    case URES_STRING :
        {
            int32_t len=0;
            const UChar*thestr = ures_getString(resource, &len, status);
            UChar *string = quotedString(thestr);

            /* TODO: String truncation */
            /*
            if(trunc && len > truncsize) {
                printIndent(out, indent);
                u_fprintf(out, "// WARNING: this string, size %d is truncated to %d\n", len, truncsize/2);
                len = truncsize/2;
            }
            */
            printIndent(out, indent);
            if(key != NULL) {
                u_fprintf(out, "%s { \"%S\" } ", key, string);
            } else {
                u_fprintf(out, "\"%S\",", string);
            }
            if(VERBOSE) {
                u_fprintf(out, " // STRING");
            }
            u_fprintf(out, "\n");
            free(string);
        }
        break;
    case URES_INT :
        printIndent(out, indent);
        if(key != NULL) {
            u_fprintf(out, "%s", key);
        }
        u_fprintf(out, ":int { %li } ", ures_getInt(resource, status));
        
        if(VERBOSE) {
            u_fprintf(out, " // INT");
        }
        u_fprintf(out, "\n");
        break;
    case URES_BINARY :
        {
            int32_t len = 0;
            const int8_t *data = (const int8_t *)ures_getBinary(resource, &len, status);
            if(trunc && len > truncsize) {
                printIndent(out, indent);
                u_fprintf(out, "// WARNING: this resource, size %li is truncated to %li\n", len, truncsize/2);
                len = truncsize/2;
            }
            if(U_SUCCESS(*status)) {
                printIndent(out, indent);
                if(key != NULL) {
                    u_fprintf(out, "%s", key);
                }
                u_fprintf(out, ":binary { ");
                for(i = 0; i<len; i++) {
                    printHex(out, data++);
                }
                u_fprintf(out, " }");
                if(VERBOSE) {
                    u_fprintf(out, " // BINARY");
                }
                u_fprintf(out, "\n");
                
            } else {
                reportError(status);
            }
        }
        break;
    case URES_INT_VECTOR :
      {
          int32_t len = 0;
          const int32_t *data = ures_getIntVector(resource, &len, status);
          if(U_SUCCESS(*status)) {
              printIndent(out, indent);
              if(key != NULL) {
                  u_fprintf(out, "%s", key);
              } 
              u_fprintf(out, ":intvector { ");
              for(i = 0; i<len-1; i++) {
                  u_fprintf(out, "%d, ", data[i]);
              }
              if(len > 0) {
                  u_fprintf(out, "%d ", data[len-1]);
              }
              u_fprintf(out, "}");
              if(VERBOSE) {
                  u_fprintf(out, " // INTVECTOR");
              }
              u_fprintf(out, "\n");
              
          } else {
              reportError(status);
          }
      }
      break;
    case URES_TABLE :
    case URES_ARRAY :
        {
            UResourceBundle *t = NULL;
            ures_resetIterator(resource);
            printIndent(out, indent);
            if(key != NULL) {
                u_fprintf(out, "%s ", key);
            }
            u_fprintf(out, "{");
            if(VERBOSE) {
                if(ures_getType(resource) == URES_TABLE) {
                    u_fprintf(out, " // TABLE");
                } else {
                    u_fprintf(out, " // ARRAY");
                }
            }
            u_fprintf(out, "\n");

            while(ures_hasNext(resource)) {
                t = ures_getNextResource(resource, t, status);
                printOutBundle(out, t, indent+indentsize, status);
            }

            printIndent(out, indent);
            u_fprintf(out, "}\n");
            ures_close(t);
        }
        break;
    default:
        break;
    }

}

void reportError(UErrorCode *status) {
    u_fprintf(outerr, "Error %d(%s) : %U happened!\n", *status, u_errorName(*status), getErrorName(*status));
}


const UChar *getErrorName(UErrorCode errorNumber) {
    UErrorCode status = U_ZERO_ERROR;
    int32_t len = 0;

    UResourceBundle *error = ures_open(currdir, locale, &status);

    UResourceBundle *errorcodes = ures_getByKey(error, "errorcodes", NULL, &status);

    const UChar *result = ures_getStringByIndex(errorcodes, errorNumber, &len, &status);

    ures_close(errorcodes);
    ures_close(error);

    if(U_SUCCESS(status)) {
        return result;
    } else {
        return baderror;
    }

}
