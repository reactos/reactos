/*
*******************************************************************************
*
*   Copyright (C) 2003-2007, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  pkgitems.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2005sep18
*   created by: Markus W. Scherer
*
*   Companion file to package.cpp. Deals with details of ICU data item formats.
*   Used for item dependencies.
*   Contains adapted code from uresdata.c and ucnv_bld.c (swapper code from 2003).
*/

#include "unicode/utypes.h"
#include "unicode/ures.h"
#include "unicode/putil.h"
#include "unicode/udata.h"
#include "cstring.h"
#include "ucmndata.h"
#include "udataswp.h"
#include "swapimpl.h"
#include "toolutil.h"
#include "package.h"
#include "pkg_imp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* item formats in common */

#include "uresdata.h"
#include "ucnv_bld.h"
#include "ucnv_io.h"

// general definitions ----------------------------------------------------- ***

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

U_CDECL_BEGIN

static void U_CALLCONV
printError(void *context, const char *fmt, va_list args) {
    vfprintf((FILE *)context, fmt, args);
}

U_CDECL_END

// check a dependency ------------------------------------------------------ ***

/*
 * assemble the target item name from the source item name, an ID
 * and a suffix
 */
static void 
checkIDSuffix(const char *itemName, const char *id, int32_t idLength, const char *suffix,
              CheckDependency check, void *context,
              UErrorCode *pErrorCode) {
    char target[200];
    const char *itemID;
    int32_t treeLength, suffixLength, targetLength;

    // get the item basename
    itemID=strrchr(itemName, '/');
    if(itemID!=NULL) {
        ++itemID;
    } else {
        itemID=itemName;
    }

    // build the target string
    treeLength=(int32_t)(itemID-itemName);
    if(idLength<0) {
        idLength=(int32_t)strlen(id);
    }
    suffixLength=(int32_t)strlen(suffix);
    targetLength=treeLength+idLength+suffixLength;
    if(targetLength>=(int32_t)sizeof(target)) {
        fprintf(stderr, "icupkg/checkIDSuffix(%s) alias target item name length %ld too long\n",
                        itemName, (long)targetLength);
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
        return;
    }

    memcpy(target, itemName, treeLength);
    memcpy(target+treeLength, id, idLength);
    memcpy(target+treeLength+idLength, suffix, suffixLength+1); // +1 includes the terminating NUL

    check(context, itemName, target);
}

/* assemble the target item name from the item's parent item name */
static void 
checkParent(const char *itemName, CheckDependency check, void *context,
            UErrorCode *pErrorCode) {
    const char *itemID, *parent, *parentLimit, *suffix;
    int32_t parentLength;

    // get the item basename
    itemID=strrchr(itemName, '/');
    if(itemID!=NULL) {
        ++itemID;
    } else {
        itemID=itemName;
    }

    // get the item suffix
    suffix=strrchr(itemID, '.');
    if(suffix==NULL) {
        // empty suffix, point to the end of the string
        suffix=strrchr(itemID, 0);
    }

    // get the position of the last '_'
    for(parentLimit=suffix; parentLimit>itemID && *--parentLimit!='_';) {}

    if(parentLimit!=itemID) {
        // get the parent item name by truncating the last part of this item's name */
        parent=itemID;
        parentLength=(int32_t)(parentLimit-itemID);
    } else {
        // no '_' in the item name: the parent is the root bundle
        parent="root";
        parentLength=4;
        if((suffix-itemID)==parentLength && 0==memcmp(itemID, parent, parentLength)) {
            // the item itself is "root", which does not depend on a parent
            return;
        }
    }
    checkIDSuffix(itemName, parent, parentLength, suffix, check, context, pErrorCode);
}

// get dependencies from resource bundles ---------------------------------- ***

static const char *const gAliasKey="%%ALIAS";
enum { gAliasKeyLength=7 };

/*
 * Enumerate one resource item and its children and extract dependencies from
 * aliases.
 * Code adapted from ures_preflightResource() and ures_swapResource().
 */
static void
ures_enumDependencies(const UDataSwapper *ds,
                      const char *itemName,
                      const Resource *inBundle, int32_t length,
                      Resource res, const char *inKey, int32_t depth,
                      CheckDependency check, void *context,
                      UErrorCode *pErrorCode) {
    const Resource *p;
    int32_t offset;

    if(res==0 || RES_GET_TYPE(res)==URES_INT) {
        /* empty string or integer, nothing to do */
        return;
    }

    /* all other types use an offset to point to their data */
    offset=(int32_t)RES_GET_OFFSET(res);
    if(0<=length && length<=offset) {
        udata_printError(ds, "icupkg/ures_enumDependencies(%s res=%08x) resource offset exceeds bundle length %d\n",
                         itemName, res, length);
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        return;
    }
    p=inBundle+offset;

    switch(RES_GET_TYPE(res)) {
        /* strings and aliases have physically the same value layout */
    case URES_STRING:
        // we ignore all strings except top-level strings with a %%ALIAS key
        if(depth!=1) {
            break;
        } else {
            char key[8];
            int32_t keyLength;

            keyLength=(int32_t)strlen(inKey);
            if(keyLength!=gAliasKeyLength) {
                break;
            }
            ds->swapInvChars(ds, inKey, gAliasKeyLength+1, key, pErrorCode);
            if(U_FAILURE(*pErrorCode)) {
                udata_printError(ds, "icupkg/ures_enumDependencies(%s res=%08x) string key contains variant characters\n",
                                itemName, res);
                return;
            }
            if(0!=strcmp(key, gAliasKey)) {
                break;
            }
        }
        // for the top-level %%ALIAS string fall through to URES_ALIAS
    case URES_ALIAS:
        {
            char localeID[32];
            const uint16_t *p16;
            int32_t i, stringLength;
            uint16_t u16, ored16;

            stringLength=udata_readInt32(ds, (int32_t)*p);

            /* top=offset+1+(string length +1)/2 rounded up */
            offset+=1+((stringLength+1)+1)/2;
            if(offset>length) {
                break; // the resource does not fit into the bundle, print error below
            }

            // extract the locale ID from alias strings like
            // locale_ID/key1/key2/key3
            // locale_ID
            if(U_IS_BIG_ENDIAN==ds->inIsBigEndian) {
                u16=0x2f;   // slash in local endianness
            } else {
                u16=0x2f00; // slash in opposite endianness
            }
            p16=(const uint16_t *)(p+1); // Unicode string contents

            // search for the first slash
            for(i=0; i<stringLength && p16[i]!=u16; ++i) {}

            if(RES_GET_TYPE(res)==URES_ALIAS) {
                // ignore aliases with an initial slash:
                // /ICUDATA/... and /pkgname/... go to a different package
                // /LOCALE/... are for dynamic sideways fallbacks and don't go to a fixed bundle
                if(i==0) {
                    break; // initial slash ('/')
                }

                // ignore the intra-bundle path starting from the first slash ('/')
                stringLength=i;
            } else /* URES_STRING */ {
                // the whole string should only consist of a locale ID
                if(i!=stringLength) {
                    udata_printError(ds, "icupkg/ures_enumDependencies(%s res=%08x) %%ALIAS contains a '/'\n",
                                    itemName, res);
                    *pErrorCode=U_UNSUPPORTED_ERROR;
                    return;
                }
            }

            // convert the Unicode string to char * and
            // check that it has a bundle path but no package
            if(stringLength>=(int32_t)sizeof(localeID)) {
                udata_printError(ds, "icupkg/ures_enumDependencies(%s res=%08x) alias locale ID length %ld too long\n",
                                itemName, res, stringLength);
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                return;
            }

            // convert the alias Unicode string to US-ASCII
            ored16=0;
            if(U_IS_BIG_ENDIAN==ds->inIsBigEndian) {
                for(i=0; i<stringLength; ++i) {
                    u16=p16[i];
                    ored16|=u16;
                    localeID[i]=(char)u16;
                }
            } else {
                for(i=0; i<stringLength; ++i) {
                    u16=p16[i];
                    ored16|=u16;
                    localeID[i]=(char)(u16>>8);
                }
                ored16=(uint16_t)((ored16<<8)|(ored16>>8));
            }
            localeID[stringLength]=0;
            if(ored16>0x7f) {
                udata_printError(ds, "icupkg/ures_enumDependencies(%s res=%08x) alias string contains non-ASCII characters\n",
                                itemName, res);
                *pErrorCode=U_INVALID_CHAR_FOUND;
                return;
            }

#if (U_CHARSET_FAMILY==U_EBCDIC_FAMILY)
            // swap to EBCDIC
            // our swapper is probably not the right one, but
            // the function uses it only for printing errors
            uprv_ebcdicFromAscii(ds, localeID, stringLength, localeID, pErrorCode);
            if(U_FAILURE(*pErrorCode)) {
                return;
            }
#endif
#if U_CHARSET_FAMILY!=U_ASCII_FAMILY && U_CHARSET_FAMILY!=U_EBCDIC_FAMILY
#           error Unknown U_CHARSET_FAMILY value!
#endif

            checkIDSuffix(itemName, localeID, -1, ".res", check, context, pErrorCode);
        }
        break;
    case URES_TABLE:
    case URES_TABLE32:
        {
            const uint16_t *pKey16;
            const int32_t *pKey32;

            Resource item;
            int32_t i, count;

            if(RES_GET_TYPE(res)==URES_TABLE) {
                /* get table item count */
                pKey16=(const uint16_t *)p;
                count=ds->readUInt16(*pKey16++);

                pKey32=NULL;

                /* top=((1+ table item count)/2 rounded up)+(table item count) */
                offset+=((1+count)+1)/2;
            } else {
                /* get table item count */
                pKey32=(const int32_t *)p;
                count=udata_readInt32(ds, *pKey32++);

                pKey16=NULL;

                /* top=(1+ table item count)+(table item count) */
                offset+=1+count;
            }

            p=inBundle+offset; /* pointer to table resources */
            offset+=count;

            if(offset>length) {
                break; // the resource does not fit into the bundle, print error below
            }

            /* recurse */
            for(i=0; i<count; ++i) {
                item=ds->readUInt32(*p++);
                ures_enumDependencies(
                        ds, itemName, inBundle, length, item,
                        ((const char *)inBundle)+
                            (pKey16!=NULL ?
                                ds->readUInt16(pKey16[i]) :
                                udata_readInt32(ds, pKey32[i])),
                        depth+1,
                        check, context,
                        pErrorCode);
                if(U_FAILURE(*pErrorCode)) {
                    udata_printError(ds, "icupkg/ures_enumDependencies(%s table res=%08x)[%d].recurse(%08x) failed\n",
                                        itemName, res, i, item);
                    break;
                }
            }
        }
        break;
    case URES_ARRAY:
        {
            Resource item;
            int32_t i, count;

            /* top=offset+1+(array length) */
            count=udata_readInt32(ds, (int32_t)*p++);
            offset+=1+count;

            if(offset>length) {
                break; // the resource does not fit into the bundle, print error below
            }

            /* recurse */
            for(i=0; i<count; ++i) {
                item=ds->readUInt32(*p++);
                ures_enumDependencies(
                        ds, itemName, inBundle, length,
                        item, NULL, depth+1,
                        check, context,
                        pErrorCode);
                if(U_FAILURE(*pErrorCode)) {
                    udata_printError(ds, "icupkg/ures_enumDependencies(%s array res=%08x)[%d].recurse(%08x) failed\n",
                                        itemName, res, i, item);
                    break;
                }
            }
        }
        break;
    default:
        break;
    }

    if(U_FAILURE(*pErrorCode)) {
        /* nothing to do */
    } else if(0<=length && length<offset) {
        udata_printError(ds, "icupkg/ures_enumDependencies(%s res=%08x) resource limit exceeds bundle length %d\n",
                         itemName, res, length);
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
    }
}

/* code adapted from ures_swap() */
static void
ures_enumDependencies(const UDataSwapper *ds,
                      const char *itemName, const UDataInfo *pInfo,
                      const uint8_t *inBytes, int32_t length,
                      CheckDependency check, void *context,
                      UErrorCode *pErrorCode) {
    const Resource *inBundle;
    Resource rootRes;

    /* the following integers count Resource item offsets (4 bytes each), not bytes */
    int32_t bundleLength;

    /* check format version */
    if(pInfo->formatVersion[0]!=1) {
        fprintf(stderr, "icupkg: .res format version %02x not supported\n",
                        pInfo->formatVersion[0]);
        exit(U_UNSUPPORTED_ERROR);
    }

    /* a resource bundle must contain at least one resource item */
    bundleLength=length/4;

    /* formatVersion 1.1 must have a root item and at least 5 indexes */
    if( bundleLength<
            (pInfo->formatVersion[1]==0 ? 1 : 1+5)
    ) {
        fprintf(stderr, "icupkg: too few bytes (%d after header) for a resource bundle\n",
                        length);
        exit(U_INDEX_OUTOFBOUNDS_ERROR);
    }

    inBundle=(const Resource *)inBytes;
    rootRes=ds->readUInt32(*inBundle);

    ures_enumDependencies(
        ds, itemName, inBundle, bundleLength,
        rootRes, NULL, 0,
        check, context,
        pErrorCode);

    /*
     * if the bundle attributes are present and the nofallback flag is not set,
     * then add the parent bundle as a dependency
     */
    if(pInfo->formatVersion[1]>=1) {
        int32_t indexes[URES_INDEX_TOP];
        const int32_t *inIndexes;

        inIndexes=(const int32_t *)inBundle+1;
        indexes[URES_INDEX_LENGTH]=udata_readInt32(ds, inIndexes[URES_INDEX_LENGTH]);
        if(indexes[URES_INDEX_LENGTH]>URES_INDEX_ATTRIBUTES) {
            indexes[URES_INDEX_ATTRIBUTES]=udata_readInt32(ds, inIndexes[URES_INDEX_ATTRIBUTES]);
            if(0==(indexes[URES_INDEX_ATTRIBUTES]&URES_ATT_NO_FALLBACK)) {
                /* this bundle participates in locale fallback */
                checkParent(itemName, check, context, pErrorCode);
            }
        }
    }
}

// get dependencies from conversion tables --------------------------------- ***

/* code adapted from ucnv_swap() */
static void
ucnv_enumDependencies(const UDataSwapper *ds,
                      const char *itemName, const UDataInfo *pInfo,
                      const uint8_t *inBytes, int32_t length,
                      CheckDependency check, void *context,
                      UErrorCode *pErrorCode) {
    uint32_t staticDataSize;

    const UConverterStaticData *inStaticData;

    const _MBCSHeader *inMBCSHeader;
    uint8_t outputType;

    /* check format version */
    if(!(
        pInfo->formatVersion[0]==6 &&
        pInfo->formatVersion[1]>=2
    )) {
        fprintf(stderr, "icupkg/ucnv_enumDependencies(): .cnv format version %02x.%02x not supported\n",
                        pInfo->formatVersion[0], pInfo->formatVersion[1]);
        exit(U_UNSUPPORTED_ERROR);
    }

    /* read the initial UConverterStaticData structure after the UDataInfo header */
    inStaticData=(const UConverterStaticData *)inBytes;

    if( length<(int32_t)sizeof(UConverterStaticData) ||
        (uint32_t)length<(staticDataSize=ds->readUInt32(inStaticData->structSize))
    ) {
        udata_printError(ds, "icupkg/ucnv_enumDependencies(): too few bytes (%d after header) for an ICU .cnv conversion table\n",
                            length);
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        return;
    }

    inBytes+=staticDataSize;
    length-=(int32_t)staticDataSize;

    /* check for supported conversionType values */
    if(inStaticData->conversionType==UCNV_MBCS) {
        /* MBCS data */
        uint32_t mbcsHeaderFlags;
        int32_t extOffset;

        inMBCSHeader=(const _MBCSHeader *)inBytes;

        if(length<(int32_t)sizeof(_MBCSHeader)) {
            udata_printError(ds, "icupkg/ucnv_enumDependencies(): too few bytes (%d after headers) for an ICU MBCS .cnv conversion table\n",
                                length);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return;
        }
        if(!(inMBCSHeader->version[0]==4 && inMBCSHeader->version[1]>=1)) {
            udata_printError(ds, "icupkg/ucnv_enumDependencies(): unsupported _MBCSHeader.version %d.%d\n",
                             inMBCSHeader->version[0], inMBCSHeader->version[1]);
            *pErrorCode=U_UNSUPPORTED_ERROR;
            return;
        }

        mbcsHeaderFlags=ds->readUInt32(inMBCSHeader->flags);
        extOffset=(int32_t)(mbcsHeaderFlags>>8);
        outputType=(uint8_t)mbcsHeaderFlags;

        if(outputType==MBCS_OUTPUT_EXT_ONLY) {
            /*
             * extension-only file,
             * contains a base name instead of normal base table data
             */
            char baseName[32];
            int32_t baseNameLength;

            /* there is extension data after the base data, see ucnv_ext.h */
            if(length<(extOffset+UCNV_EXT_INDEXES_MIN_LENGTH*4)) {
                udata_printError(ds, "icupkg/ucnv_enumDependencies(): too few bytes (%d after headers) for an ICU MBCS .cnv conversion table with extension data\n",
                                 length);
                *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
                return;
            }

            /* swap the base name, between the header and the extension data */
            baseNameLength=(int32_t)strlen((const char *)(inMBCSHeader+1));
            if(baseNameLength>=(int32_t)sizeof(baseName)) {
                udata_printError(ds, "icupkg/ucnv_enumDependencies(%s): base name length %ld too long\n",
                                 itemName, baseNameLength);
                *pErrorCode=U_UNSUPPORTED_ERROR;
                return;
            }
            ds->swapInvChars(ds, inMBCSHeader+1, baseNameLength+1, baseName, pErrorCode);

            checkIDSuffix(itemName, baseName, -1, ".cnv", check, context, pErrorCode);
        }
    }
}

// ICU data formats -------------------------------------------------------- ***

static const struct {
    uint8_t dataFormat[4];
} dataFormats[]={
    { { 0x52, 0x65, 0x73, 0x42 } },     /* dataFormat="ResB" */
    { { 0x63, 0x6e, 0x76, 0x74 } },     /* dataFormat="cnvt" */
    { { 0x43, 0x76, 0x41, 0x6c } }      /* dataFormat="CvAl" */
};

enum {
    FMT_RES,
    FMT_CNV,
    FMT_ALIAS,
    FMT_COUNT
};

static int32_t
getDataFormat(const uint8_t dataFormat[4]) {
    int32_t i;

    for(i=0; i<FMT_COUNT; ++i) {
        if(0==memcmp(dataFormats[i].dataFormat, dataFormat, 4)) {
            return i;
        }
    }
    return -1;
}

// enumerate dependencies of a package item -------------------------------- ***

U_NAMESPACE_BEGIN

void
Package::enumDependencies(Item *pItem, void *context, CheckDependency check) {
    const UDataInfo *pInfo;
    const uint8_t *inBytes;
    int32_t format, length, infoLength, itemHeaderLength;
    UErrorCode errorCode;

    errorCode=U_ZERO_ERROR;
    pInfo=getDataInfo(pItem->data,pItem->length, infoLength, itemHeaderLength, &errorCode);
    if(U_FAILURE(errorCode)) {
        return; // should not occur because readFile() checks headers
    }

    // find the data format and call the corresponding function, if any
    format=getDataFormat(pInfo->dataFormat);
    if(format>=0) {
        UDataSwapper *ds;

        // TODO: share/cache swappers
        ds=udata_openSwapper((UBool)pInfo->isBigEndian, pInfo->charsetFamily, U_IS_BIG_ENDIAN, U_CHARSET_FAMILY, &errorCode);
        if(U_FAILURE(errorCode)) {
            fprintf(stderr, "icupkg: udata_openSwapper(\"%s\") failed - %s\n",
                    pItem->name, u_errorName(errorCode));
            exit(errorCode);
        }

        ds->printError=printError;
        ds->printErrorContext=stderr;

        inBytes=pItem->data+itemHeaderLength;
        length=pItem->length-itemHeaderLength;

        switch(format) {
        case FMT_RES:
            ures_enumDependencies(ds, pItem->name, pInfo, inBytes, length, check, context, &errorCode);
            break;
        case FMT_CNV:
            ucnv_enumDependencies(ds, pItem->name, pInfo, inBytes, length, check, context, &errorCode);
            break;
        default:
            break;
        }

        udata_closeSwapper(ds);

        if(U_FAILURE(errorCode)) {
            exit(errorCode);
        }
    }
}
U_NAMESPACE_END
