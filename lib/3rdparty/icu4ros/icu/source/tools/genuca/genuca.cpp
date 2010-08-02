/*
*******************************************************************************
*
*   Copyright (C) 2000-2007, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  genuca.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created at the end of XX century
*   created by: Vladimir Weinstein
*
*   This program reads the Franctional UCA table and generates
*   internal format for UCA table as well as inverse UCA table.
*   It then writes binary files containing the data: ucadata.dat 
*   & invuca.dat
*   Change history:
*   02/23/2001  grhoten                 Made it into a tool
*   02/23/2001  weiv                    Moved element & table handling code to i18n
*   05/09/2001  weiv                    Case bits are now in the CEs, not in front
*/

#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "unicode/udata.h"
#include "unicode/uclean.h"
#include "ucol_imp.h"
#include "genuca.h"
#include "uoptions.h"
#include "toolutil.h"
#include "unewdata.h"
#include "cstring.h"
#include "cmemory.h"

#include <stdio.h>

/*
 * Global - verbosity
 */
UBool VERBOSE = FALSE;

static UVersionInfo UCAVersion;

#if UCONFIG_NO_COLLATION

/* dummy UDataInfo cf. udata.h */
static UDataInfo dummyDataInfo = {
    sizeof(UDataInfo),
    0,

    U_IS_BIG_ENDIAN,
    U_CHARSET_FAMILY,
    U_SIZEOF_UCHAR,
    0,

    { 0, 0, 0, 0 },                 /* dummy dataFormat */
    { 0, 0, 0, 0 },                 /* dummy formatVersion */
    { 0, 0, 0, 0 }                  /* dummy dataVersion */
};

#else

static const UDataInfo ucaDataInfo={
    sizeof(UDataInfo),
    0,

    U_IS_BIG_ENDIAN,
    U_CHARSET_FAMILY,
    sizeof(UChar),
    0,

    {UCA_DATA_FORMAT_0, UCA_DATA_FORMAT_1, UCA_DATA_FORMAT_2, UCA_DATA_FORMAT_3},     /* dataFormat="UCol"            */
    /* 03/26/2002 bumped up version since format has changed */
    /* 09/16/2002 bumped up version since we went from UColAttributeValue */
    /*            to int32_t in UColOptionSet */
    /* 05/13/2003 This one also updated since we added UCA and UCD versions */
    /*            to header */
    /* 09/11/2003 Adding information required by data swapper */
    {UCA_FORMAT_VERSION_0, UCA_FORMAT_VERSION_1, UCA_FORMAT_VERSION_2, UCA_FORMAT_VERSION_3},                 /* formatVersion                */
    {0, 0, 0, 0}                  /* dataVersion = Unicode Version*/
};

static const UDataInfo invUcaDataInfo={
    sizeof(UDataInfo),
    0,

    U_IS_BIG_ENDIAN,
    U_CHARSET_FAMILY,
    sizeof(UChar),
    0,

    {INVUCA_DATA_FORMAT_0, INVUCA_DATA_FORMAT_1, INVUCA_DATA_FORMAT_2, INVUCA_DATA_FORMAT_3},     /* dataFormat="InvC"            */
    /* 03/26/2002 bumped up version since format has changed */
    /* 04/29/2003 2.1 format - we have added UCA version to header */
    {INVUCA_FORMAT_VERSION_0, INVUCA_FORMAT_VERSION_1, INVUCA_FORMAT_VERSION_2, INVUCA_FORMAT_VERSION_3},                 /* formatVersion                */
    {0, 0, 0, 0}                  /* dataVersion = Unicode Version*/
};

UCAElements le;

int32_t readElement(char **from, char *to, char separator, UErrorCode *status) {
    if(U_FAILURE(*status)) {
        return 0;
    }
    char buffer[1024];
    int32_t i = 0;
    while(**from != separator) {
        if(**from != ' ') {
            *(buffer+i++) = **from;
        }
        (*from)++;
    }
    (*from)++;
    *(buffer + i) = 0;
    //*to = (char *)malloc(strlen(buffer)+1);
    strcpy(to, buffer);
    return i/2;
}


uint32_t getSingleCEValue(char *primary, char *secondary, char *tertiary, UErrorCode *status) {
    if(U_FAILURE(*status)) {
        return 0;
    }
    uint32_t value = 0;
    char primsave = '\0';
    char secsave = '\0';
    char tersave = '\0';
    char *primend = primary+4;
    if(strlen(primary) > 4) {
        primsave = *primend;
        *primend = '\0';
    }
    char *secend = secondary+2;
    if(strlen(secondary) > 2) {
        secsave = *secend;
        *secend = '\0';
    }
    char *terend = tertiary+2;
    if(strlen(tertiary) > 2) {
        tersave = *terend;
        *terend = '\0';
    }
    uint32_t primvalue = (uint32_t)((*primary!='\0')?strtoul(primary, &primend, 16):0);
    uint32_t secvalue = (uint32_t)((*secondary!='\0')?strtoul(secondary, &secend, 16):0);
    uint32_t tervalue = (uint32_t)((*tertiary!='\0')?strtoul(tertiary, &terend, 16):0);
    if(primvalue <= 0xFF) {
      primvalue <<= 8;
    }

    value = ((primvalue<<UCOL_PRIMARYORDERSHIFT)&UCOL_PRIMARYORDERMASK)|
        ((secvalue<<UCOL_SECONDARYORDERSHIFT)&UCOL_SECONDARYORDERMASK)|
        (tervalue&UCOL_TERTIARYORDERMASK);

    if(primsave!='\0') {
        *primend = primsave;
    }
    if(secsave!='\0') {
        *secend = secsave;
    }
    if(tersave!='\0') {
        *terend = tersave;
    }
    return value;
}

static uint32_t inverseTable[0xFFFF][3];
static uint32_t inversePos = 0;
static UChar stringContinue[0xFFFF];
static uint32_t sContPos = 0;

static void addNewInverse(UCAElements *element, UErrorCode *status) {
  if(U_FAILURE(*status)) {
    return;
  }
  if(VERBOSE && isContinuation(element->CEs[1])) {
    //fprintf(stdout, "+");
  }
  inversePos++;
  inverseTable[inversePos][0] = element->CEs[0];
  if(element->noOfCEs > 1 && isContinuation(element->CEs[1])) {
    inverseTable[inversePos][1] = element->CEs[1];
  } else {
    inverseTable[inversePos][1] = 0;
  }
  if(element->cSize < 2) {
    inverseTable[inversePos][2] = element->cPoints[0];
  } else { /* add a new store of cruft */
    inverseTable[inversePos][2] = ((element->cSize+1) << UCOL_INV_SHIFTVALUE) | sContPos;
    memcpy(stringContinue+sContPos, element->cPoints, element->cSize*sizeof(UChar));
    sContPos += element->cSize+1;
  }
}

static void insertInverse(UCAElements *element, uint32_t position, UErrorCode *status) {
  if(U_FAILURE(*status)) {
    return;
  }

  if(VERBOSE && isContinuation(element->CEs[1])) {
    //fprintf(stdout, "+");
  }
  if(position <= inversePos) {
    /*move stuff around */
    uint32_t amountToMove = (inversePos - position+1)*sizeof(inverseTable[0]);
    uprv_memmove(inverseTable[position+1], inverseTable[position], amountToMove);
  }
  inverseTable[position][0] = element->CEs[0];
  if(element->noOfCEs > 1 && isContinuation(element->CEs[1])) {
    inverseTable[position][1] = element->CEs[1];
  } else {
    inverseTable[position][1] = 0;
  }
  if(element->cSize < 2) {
    inverseTable[position][2] = element->cPoints[0];
  } else { /* add a new store of cruft */
    inverseTable[position][2] = ((element->cSize+1) << UCOL_INV_SHIFTVALUE) | sContPos;
    memcpy(stringContinue+sContPos, element->cPoints, element->cSize*sizeof(UChar));
    sContPos += element->cSize+1;
  }
  inversePos++;
}

static void addToExistingInverse(UCAElements *element, uint32_t position, UErrorCode *status) {

  if(U_FAILURE(*status)) {
    return;
  }

      if((inverseTable[position][2] & UCOL_INV_SIZEMASK) == 0) { /* single element, have to make new extension place and put both guys there */
        stringContinue[sContPos] = (UChar)inverseTable[position][2];
        inverseTable[position][2] = ((element->cSize+3) << UCOL_INV_SHIFTVALUE) | sContPos;
        sContPos++;
        stringContinue[sContPos++] = 0xFFFF;
        memcpy(stringContinue+sContPos, element->cPoints, element->cSize*sizeof(UChar));
        sContPos += element->cSize;
        stringContinue[sContPos++] = 0xFFFE;
      } else { /* adding to the already existing continuing table */
        uint32_t contIndex = inverseTable[position][2] & UCOL_INV_OFFSETMASK;
        uint32_t contSize = (inverseTable[position][2] & UCOL_INV_SIZEMASK) >> UCOL_INV_SHIFTVALUE;

        if(contIndex+contSize < sContPos) {
          /*fprintf(stderr, ".", sContPos, contIndex+contSize);*/
          memcpy(stringContinue+contIndex+contSize+element->cSize+1, stringContinue+contIndex+contSize, (element->cSize+1)*sizeof(UChar));
        }

        stringContinue[contIndex+contSize-1] = 0xFFFF;
        memcpy(stringContinue+contIndex+contSize, element->cPoints, element->cSize*sizeof(UChar));
        sContPos += element->cSize+1;
        stringContinue[contIndex+contSize+element->cSize] = 0xFFFE;

        inverseTable[position][2] = ((contSize+element->cSize+1) << UCOL_INV_SHIFTVALUE) | contIndex;
      }
}

/* 
 * Takes two CEs (lead and continuation) and 
 * compares them as CEs should be compared:
 * primary vs. primary, secondary vs. secondary
 * tertiary vs. tertiary
 */
static int32_t compareCEs(uint32_t *source, uint32_t *target) {
  uint32_t s1 = source[0], s2, t1 = target[0], t2;
  if(isContinuation(source[1])) {
    s2 = source[1];
  } else {
    s2 = 0;
  }
  if(isContinuation(target[1])) {
    t2 = target[1];
  } else {
    t2 = 0;
  }
  
  uint32_t s = 0, t = 0;
  if(s1 == t1 && s2 == t2) {
    return 0;
  }
  s = (s1 & 0xFFFF0000)|((s2 & 0xFFFF0000)>>16); 
  t = (t1 & 0xFFFF0000)|((t2 & 0xFFFF0000)>>16); 
  if(s < t) {
    return -1;
  } else if(s > t) {
    return 1;
  } else {
    s = (s1 & 0x0000FF00) | (s2 & 0x0000FF00)>>8;
    t = (t1 & 0x0000FF00) | (t2 & 0x0000FF00)>>8;
    if(s < t) {
      return -1;
    } else if(s > t) {
      return 1;
    } else {
      s = (s1 & 0x000000FF)<<8 | (s2 & 0x000000FF);
      t = (t1 & 0x000000FF)<<8 | (t2 & 0x000000FF);
      if(s < t) {
        return -1;
      } else {
        return 1;
      }
    }
  }
}

static uint32_t addToInverse(UCAElements *element, UErrorCode *status) {
  uint32_t position = inversePos;
  uint32_t saveElement = element->CEs[0];
  int32_t compResult = 0;
  element->CEs[0] &= 0xFFFFFF3F;
  if(element->noOfCEs == 1) {
    element->CEs[1] = 0;
  }
  if(inversePos == 0) {
    inverseTable[0][0] = inverseTable[0][1] = inverseTable[0][2] = 0;
    addNewInverse(element, status);
  } else if(compareCEs(inverseTable[inversePos], element->CEs) > 0) {
    while((compResult = compareCEs(inverseTable[--position], element->CEs)) > 0);
    if(VERBOSE) { fprintf(stdout, "p:%u ", (int)position); }
    if(compResult == 0) {
      addToExistingInverse(element, position, status);
    } else {
      insertInverse(element, position+1, status);
    }
  } else if(compareCEs(inverseTable[inversePos], element->CEs) == 0) {
    addToExistingInverse(element, inversePos, status);
  } else {
    addNewInverse(element, status);
  }
  element->CEs[0] = saveElement;
  if(VERBOSE) { fprintf(stdout, "+"); }
  return inversePos;
}

static InverseUCATableHeader *assembleInverseTable(UErrorCode *status)
{
  InverseUCATableHeader *result = NULL;
  uint32_t headerByteSize = paddedsize(sizeof(InverseUCATableHeader));
  uint32_t inverseTableByteSize = (inversePos+2)*sizeof(uint32_t)*3;
  uint32_t contsByteSize = sContPos * sizeof(UChar);
  uint32_t i = 0;

  result = (InverseUCATableHeader *)uprv_malloc(headerByteSize + inverseTableByteSize + contsByteSize);
  uprv_memset(result, 0, headerByteSize + inverseTableByteSize + contsByteSize);
  if(result != NULL) {
    result->byteSize = headerByteSize + inverseTableByteSize + contsByteSize;

    inversePos++;
    inverseTable[inversePos][0] = 0xFFFFFFFF;
    inverseTable[inversePos][1] = 0xFFFFFFFF;
    inverseTable[inversePos][2] = 0x0000FFFF;
    inversePos++;

    for(i = 2; i<inversePos; i++) {
      if(compareCEs(inverseTable[i-1], inverseTable[i]) > 0) { 
        fprintf(stderr, "Error at %i: %08X & %08X\n", (int)i, (int)inverseTable[i-1][0], (int)inverseTable[i][0]);
      } else if(inverseTable[i-1][0] == inverseTable[i][0] && !(inverseTable[i-1][1] < inverseTable[i][1])) {
        fprintf(stderr, "Continuation error at %i: %08X %08X & %08X %08X\n", (int)i, (int)inverseTable[i-1][0], (int)inverseTable[i-1][1], (int)inverseTable[i][0], (int)inverseTable[i][1]);
      }
    }

    result->tableSize = inversePos;
    result->contsSize = sContPos;

    result->table = headerByteSize;
    result->conts = headerByteSize + inverseTableByteSize;

    memcpy((uint8_t *)result + result->table, inverseTable, inverseTableByteSize);
    memcpy((uint8_t *)result + result->conts, stringContinue, contsByteSize);

  } else {
    *status = U_MEMORY_ALLOCATION_ERROR;
    return NULL;
  }

  return result; 
}


static void writeOutInverseData(InverseUCATableHeader *data, 
                  const char *outputDir, 
                  const char *copyright,
                  UErrorCode *status)
{
    UNewDataMemory *pData;
    
    long dataLength;

    UDataInfo invUcaInfo;
    uprv_memcpy(&invUcaInfo, &invUcaDataInfo, sizeof(UDataInfo));
    u_getUnicodeVersion(invUcaInfo.dataVersion);

    pData=udata_create(outputDir, INVC_DATA_TYPE, INVC_DATA_NAME, &invUcaInfo,
                       copyright, status);

    if(U_FAILURE(*status)) {
        fprintf(stderr, "Error: unable to create %s"INVC_DATA_NAME", error %s\n", outputDir, u_errorName(*status));
        return;
    }

    /* write the data to the file */
    if (VERBOSE) {
        fprintf(stdout, "Writing out inverse UCA table: %s%c%s.%s\n", outputDir, U_FILE_SEP_CHAR,
                                                                INVC_DATA_NAME,
                                                                INVC_DATA_TYPE);
    }
    udata_writeBlock(pData, data, data->byteSize);

    /* finish up */
    dataLength=udata_finish(pData, status);
    if(U_FAILURE(*status)) {
        fprintf(stderr, "Error: error %d writing the output file\n", *status);
        return;
    }
}



static int32_t hex2num(char hex) {
    if(hex>='0' && hex <='9') {
        return hex-'0';
    } else if(hex>='a' && hex<='f') {
        return hex-'a'+10;
    } else if(hex>='A' && hex<='F') {
        return hex-'A'+10;
    } else {
        return 0;
    }
}

UCAElements *readAnElement(FILE *data, tempUCATable *t, UCAConstants *consts, UErrorCode *status) {
    char buffer[2048], primary[100], secondary[100], tertiary[100];
    UBool detectedContraction;
    int32_t i = 0;
    unsigned int theValue;
    char *pointer = NULL;
    char *commentStart = NULL;
    char *startCodePoint = NULL;
    char *endCodePoint = NULL;
    char *spacePointer = NULL;
    char *result = fgets(buffer, 2048, data);
    int32_t buflen = (int32_t)uprv_strlen(buffer);
    if(U_FAILURE(*status)) {
        return 0;
    }
    *primary = *secondary = *tertiary = '\0';
    if(result == NULL) {
        if(feof(data)) {
            return NULL;
        } else {
            fprintf(stderr, "empty line but no EOF!\n");
            *status = U_INVALID_FORMAT_ERROR;
            return NULL;
        }
    }
    while(buflen>0 && (buffer[buflen-1] == '\r' || buffer[buflen-1] == '\n')) {
      buffer[--buflen] = 0;
    }

    if(buffer[0] == 0 || buffer[0] == '#') {
        return NULL; // just a comment, skip whole line
    }

    UCAElements *element = &le; //(UCAElements *)malloc(sizeof(UCAElements));

    enum ActionType {
      READCE,
      READHEX,
      READUCAVERSION
    };

    // Directives.
    if(buffer[0] == '[') {
      uint32_t cnt = 0;
      static const struct {
        char name[128];
        uint32_t *what;
        ActionType what_to_do;
      } vt[]  = { {"[first tertiary ignorable",  consts->UCA_FIRST_TERTIARY_IGNORABLE,  READCE},
                  {"[last tertiary ignorable",   consts->UCA_LAST_TERTIARY_IGNORABLE,   READCE},
                  {"[first secondary ignorable", consts->UCA_FIRST_SECONDARY_IGNORABLE, READCE},
                  {"[last secondary ignorable",  consts->UCA_LAST_SECONDARY_IGNORABLE,  READCE},
                  {"[first primary ignorable",   consts->UCA_FIRST_PRIMARY_IGNORABLE,   READCE},
                  {"[last primary ignorable",    consts->UCA_LAST_PRIMARY_IGNORABLE,    READCE},
                  {"[first variable",            consts->UCA_FIRST_VARIABLE,            READCE},
                  {"[last variable",             consts->UCA_LAST_VARIABLE,             READCE},
                  {"[first regular",             consts->UCA_FIRST_NON_VARIABLE,        READCE},
                  {"[last regular",              consts->UCA_LAST_NON_VARIABLE,         READCE},
                  {"[first implicit",            consts->UCA_FIRST_IMPLICIT,            READCE},
                  {"[last implicit",             consts->UCA_LAST_IMPLICIT,             READCE},
                  {"[first trailing",            consts->UCA_FIRST_TRAILING,            READCE},
                  {"[last trailing",             consts->UCA_LAST_TRAILING,             READCE},

                  {"[fixed top",                       &consts->UCA_PRIMARY_TOP_MIN,           READHEX},
                  {"[fixed first implicit byte",       &consts->UCA_PRIMARY_IMPLICIT_MIN,      READHEX},
                  {"[fixed last implicit byte",        &consts->UCA_PRIMARY_IMPLICIT_MAX,      READHEX},
                  {"[fixed first trail byte",          &consts->UCA_PRIMARY_TRAILING_MIN,      READHEX},
                  {"[fixed last trail byte",           &consts->UCA_PRIMARY_TRAILING_MAX,      READHEX},
                  {"[fixed first special byte",        &consts->UCA_PRIMARY_SPECIAL_MIN,       READHEX},
                  {"[fixed last special byte",         &consts->UCA_PRIMARY_SPECIAL_MAX,       READHEX},
                  {"[variable top = ",                &t->options->variableTopValue,          READHEX},
                  {"[UCA version = ",                 NULL,                          READUCAVERSION}
      };
      for (cnt = 0; cnt<sizeof(vt)/sizeof(vt[0]); cnt++) {
        uint32_t vtLen = (uint32_t)uprv_strlen(vt[cnt].name);
        if(uprv_strncmp(buffer, vt[cnt].name, vtLen) == 0) {
            element->variableTop = TRUE;
            if(vt[cnt].what_to_do == READHEX) {
              if(sscanf(buffer+vtLen, "%4x", &theValue) != 1) /* read first code point */
              {
                  fprintf(stderr, " scanf(hex) failed on !\n ");
              }
              *(vt[cnt].what) = (UChar)theValue;
              //if(cnt == 1) { // first implicit
                // we need to set the value for top next
                //uint32_t nextTop = ucol_prv_calculateImplicitPrimary(0x4E00); // CJK base
                //consts->UCA_NEXT_TOP_VALUE = theValue<<24 | 0x030303;
              //}
            } else if (vt[cnt].what_to_do == READCE) { /* vt[cnt].what_to_do == READCE */
              pointer = strchr(buffer+vtLen, '[');
              if(pointer) {
                pointer++;
                element->sizePrim[0]=readElement(&pointer, primary, ',', status);
                element->sizeSec[0]=readElement(&pointer, secondary, ',', status);
                element->sizeTer[0]=readElement(&pointer, tertiary, ']', status);

                vt[cnt].what[0] = getSingleCEValue(primary, secondary, tertiary, status);
                if(element->sizePrim[0] > 2 || element->sizeSec[0] > 1 || element->sizeTer[0] > 1) {
                  uint32_t CEi = 1;
                  uint32_t value = UCOL_CONTINUATION_MARKER; /* Continuation marker */
                    if(2*CEi<element->sizePrim[i]) {
                        value |= ((hex2num(*(primary+4*CEi))&0xF)<<28);
                        value |= ((hex2num(*(primary+4*CEi+1))&0xF)<<24);
                    }

                    if(2*CEi+1<element->sizePrim[i]) {
                        value |= ((hex2num(*(primary+4*CEi+2))&0xF)<<20);
                        value |= ((hex2num(*(primary+4*CEi+3))&0xF)<<16);
                    }

                    if(CEi<element->sizeSec[i]) {
                        value |= ((hex2num(*(secondary+2*CEi))&0xF)<<12);
                        value |= ((hex2num(*(secondary+2*CEi+1))&0xF)<<8);
                    }

                    if(CEi<element->sizeTer[i]) {
                        value |= ((hex2num(*(tertiary+2*CEi))&0x3)<<4);
                        value |= (hex2num(*(tertiary+2*CEi+1))&0xF);
                    }

                    CEi++;

                    vt[cnt].what[1] = value;
                    //element->CEs[CEindex++] = value;
                } else {
                  vt[cnt].what[1] = 0;
                }
              } else {
                fprintf(stderr, "Failed to read a CE from line %s\n", buffer);
              }
            } else { //vt[cnt].what_to_do == READUCAVERSION
              u_versionFromString(UCAVersion, buffer+vtLen);
              if(VERBOSE) {
                fprintf(stdout, "UCA version [%hu.%hu.%hu.%hu]\n", UCAVersion[0], UCAVersion[1], UCAVersion[2], UCAVersion[3]);
              }
            }
            //element->cPoints[0] = (UChar)theValue;
            //return element; 
            return NULL;
        }
      }
      fprintf(stderr, "Warning: unrecognized option: %s\n", buffer);
      //*status = U_INVALID_FORMAT_ERROR;
      return NULL;
    }
    element->variableTop = FALSE;

    startCodePoint = buffer;
    endCodePoint = strchr(startCodePoint, ';');

    if(endCodePoint == 0) {
        fprintf(stderr, "error - line with no code point!\n");
        *status = U_INVALID_FORMAT_ERROR; /* No code point - could be an error, but probably only an empty line */
        return NULL;
    } else {
        *(endCodePoint) = 0;
    }

    if(element != NULL) {
        memset(element, 0, sizeof(*element));
    } else {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }

    element->cPoints = element->uchars;

    spacePointer = strchr(buffer, ' ');
    if(sscanf(buffer, "%4x", &theValue) != 1) /* read first code point */
    {
      fprintf(stderr, " scanf(hex) failed!\n ");
    }
    element->cPoints[0] = (UChar)theValue;

    if(spacePointer == 0) {
        detectedContraction = FALSE;
        element->cSize = 1;
    } else {
        i = 1;
        detectedContraction = TRUE;
        while(spacePointer != NULL) {
            sscanf(spacePointer+1, "%4x", &theValue);
            element->cPoints[i++] = (UChar)theValue;
            spacePointer = strchr(spacePointer+1, ' ');
        }

        element->cSize = i;

        //fprintf(stderr, "Number of codepoints in contraction: %i\n", i);
    }

    startCodePoint = endCodePoint+1;

    commentStart = strchr(startCodePoint, '#');
    if(commentStart == NULL) {
        commentStart = strlen(startCodePoint) + startCodePoint;
    }

    i = 0;
    uint32_t CEindex = 0;
    element->noOfCEs = 0;
    for(;;) {
        endCodePoint = strchr(startCodePoint, ']');
        if(endCodePoint == NULL || endCodePoint >= commentStart) {
            break;
        }
        pointer = strchr(startCodePoint, '[');
        pointer++;

        element->sizePrim[i]=readElement(&pointer, primary, ',', status);
        element->sizeSec[i]=readElement(&pointer, secondary, ',', status);
        element->sizeTer[i]=readElement(&pointer, tertiary, ']', status);


        /* I want to get the CEs entered right here, including continuation */
        element->CEs[CEindex++] = getSingleCEValue(primary, secondary, tertiary, status);

        uint32_t CEi = 1;
        while(2*CEi<element->sizePrim[i] || CEi<element->sizeSec[i] || CEi<element->sizeTer[i]) {
          uint32_t value = UCOL_CONTINUATION_MARKER; /* Continuation marker */
            if(2*CEi<element->sizePrim[i]) {
                value |= ((hex2num(*(primary+4*CEi))&0xF)<<28);
                value |= ((hex2num(*(primary+4*CEi+1))&0xF)<<24);
            }

            if(2*CEi+1<element->sizePrim[i]) {
                value |= ((hex2num(*(primary+4*CEi+2))&0xF)<<20);
                value |= ((hex2num(*(primary+4*CEi+3))&0xF)<<16);
            }

            if(CEi<element->sizeSec[i]) {
                value |= ((hex2num(*(secondary+2*CEi))&0xF)<<12);
                value |= ((hex2num(*(secondary+2*CEi+1))&0xF)<<8);
            }

            if(CEi<element->sizeTer[i]) {
                value |= ((hex2num(*(tertiary+2*CEi))&0x3)<<4);
                value |= (hex2num(*(tertiary+2*CEi+1))&0xF);
            }

            CEi++;

            element->CEs[CEindex++] = value;
        }

      startCodePoint = endCodePoint+1;
      i++;
    }
    element->noOfCEs = CEindex;
#if 0
    element->isThai = UCOL_ISTHAIPREVOWEL(element->cPoints[0]);
#endif
    // we don't want any strange stuff after useful data!
    if (pointer == NULL) {
        /* huh? Did we get ']' without the '['? Pair your brackets! */
        *status=U_INVALID_FORMAT_ERROR;
    }
    else {
        while(pointer < commentStart)  {
            if(*pointer != ' ' && *pointer != '\t')
            {
                *status=U_INVALID_FORMAT_ERROR;
                break;
            }
            pointer++;
        }
    }

    if(U_FAILURE(*status)) {
        fprintf(stderr, "problem putting stuff in hash table %s\n", u_errorName(*status));
        *status = U_INTERNAL_PROGRAM_ERROR;
        return NULL;
    }

    return element;
}


void writeOutData(UCATableHeader *data,
                  UCAConstants *consts,
                  UChar contractions[][3],
                  uint32_t noOfcontractions,
                  const char *outputDir,
                  const char *copyright,
                  UErrorCode *status)
{
    if(U_FAILURE(*status)) {
        return;
    }

    uint32_t size = data->size;

    data->UCAConsts = data->size;
    data->size += paddedsize(sizeof(UCAConstants));

    if(noOfcontractions != 0) {
      contractions[noOfcontractions][0] = 0;
      contractions[noOfcontractions][1] = 0;
      contractions[noOfcontractions][2] = 0;
      noOfcontractions++;


      data->contractionUCACombos = data->size;
      data->contractionUCACombosWidth = 3;
      data->contractionUCACombosSize = noOfcontractions;
      data->size += paddedsize((noOfcontractions*3*sizeof(UChar)));
    }

    UNewDataMemory *pData;
    
    long dataLength;
    UDataInfo ucaInfo;
    uprv_memcpy(&ucaInfo, &ucaDataInfo, sizeof(UDataInfo));
    u_getUnicodeVersion(ucaInfo.dataVersion);

    pData=udata_create(outputDir, UCA_DATA_TYPE, UCA_DATA_NAME, &ucaInfo,
                       copyright, status);

    if(U_FAILURE(*status)) {
        fprintf(stderr, "Error: unable to create %s"UCA_DATA_NAME", error %s\n", outputDir, u_errorName(*status));
        return;
    }

    /* write the data to the file */
    if (VERBOSE) {
        fprintf(stdout, "Writing out UCA table: %s%c%s.%s\n", outputDir,
                                                        U_FILE_SEP_CHAR,
                                                        U_ICUDATA_NAME "_" UCA_DATA_NAME,
                                                        UCA_DATA_TYPE);
    }
    udata_writeBlock(pData, data, size);

    // output the constants here
    udata_writeBlock(pData, consts, sizeof(UCAConstants));

    if(noOfcontractions != 0) {
      udata_writeBlock(pData, contractions, noOfcontractions*3*sizeof(UChar));
      udata_writePadding(pData, paddedsize((noOfcontractions*3*sizeof(UChar))) - noOfcontractions*3*sizeof(uint16_t));
    }

    /* finish up */
    dataLength=udata_finish(pData, status);
    if(U_FAILURE(*status)) {
        fprintf(stderr, "Error: error %d writing the output file\n", *status);
        return;
    }
}

static int32_t
write_uca_table(const char *filename,
                const char *outputDir,
                const char *copyright,
                UErrorCode *status)
{
    FILE *data = fopen(filename, "r");
    if(data == NULL) {
        fprintf(stderr, "Couldn't open file: %s\n", filename);
        return -1;
    }
    uint32_t line = 0;
    UCAElements *element = NULL;
    UChar variableTopValue = 0;
    UCATableHeader *myD = (UCATableHeader *)uprv_malloc(sizeof(UCATableHeader));
    /* test for NULL */
    if(myD == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        fclose(data);
        return 0;
    }
    uprv_memset(myD, 0, sizeof(UCATableHeader));
    UColOptionSet *opts = (UColOptionSet *)uprv_malloc(sizeof(UColOptionSet));
    /* test for NULL */
    if(opts == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        uprv_free(myD);
        fclose(data);
        return 0;
    }
    uprv_memset(opts, 0, sizeof(UColOptionSet));
    UChar contractionCEs[512][3];
    uprv_memset(contractionCEs, 0, 512*3*sizeof(UChar));
    uint32_t noOfContractions = 0;
    UCAConstants consts;
    uprv_memset(&consts, 0, sizeof(consts));
#if 0
    UCAConstants consts = {
      UCOL_RESET_TOP_VALUE,
      UCOL_FIRST_PRIMARY_IGNORABLE,
      UCOL_LAST_PRIMARY_IGNORABLE,
      UCOL_LAST_PRIMARY_IGNORABLE_CONT,
      UCOL_FIRST_SECONDARY_IGNORABLE,
      UCOL_LAST_SECONDARY_IGNORABLE,
      UCOL_FIRST_TERTIARY_IGNORABLE,
      UCOL_LAST_TERTIARY_IGNORABLE,
      UCOL_FIRST_VARIABLE,
      UCOL_LAST_VARIABLE,
      UCOL_FIRST_NON_VARIABLE,
      UCOL_LAST_NON_VARIABLE,

      UCOL_NEXT_TOP_VALUE,
/*
      UCOL_NEXT_FIRST_PRIMARY_IGNORABLE,
      UCOL_NEXT_LAST_PRIMARY_IGNORABLE,
      UCOL_NEXT_FIRST_SECONDARY_IGNORABLE,
      UCOL_NEXT_LAST_SECONDARY_IGNORABLE,
      UCOL_NEXT_FIRST_TERTIARY_IGNORABLE,
      UCOL_NEXT_LAST_TERTIARY_IGNORABLE,
      UCOL_NEXT_FIRST_VARIABLE,
      UCOL_NEXT_LAST_VARIABLE,
*/

      PRIMARY_IMPLICIT_MIN,
      PRIMARY_IMPLICIT_MAX
    };
#endif


    uprv_memset(inverseTable, 0xDA, sizeof(int32_t)*3*0xFFFF);

    opts->variableTopValue = variableTopValue;
    opts->strength = UCOL_TERTIARY;
    opts->frenchCollation = UCOL_OFF;
    opts->alternateHandling = UCOL_NON_IGNORABLE; /* attribute for handling variable elements*/
    opts->caseFirst = UCOL_OFF;         /* who goes first, lower case or uppercase */
    opts->caseLevel = UCOL_OFF;         /* do we have an extra case level */
    opts->normalizationMode = UCOL_OFF; /* attribute for normalization */
    opts->hiraganaQ = UCOL_OFF; /* attribute for JIS X 4061, used only in Japanese */
    opts->numericCollation = UCOL_OFF;
    myD->jamoSpecial = FALSE;

    tempUCATable *t = uprv_uca_initTempTable(myD, opts, NULL, IMPLICIT_TAG, LEAD_SURROGATE_TAG, status);
    if(U_FAILURE(*status))
    {
        fprintf(stderr, "Failed to init UCA temp table: %s\n", u_errorName(*status));
        uprv_free(opts);
        uprv_free(myD);
        fclose(data);
        return -1;
    }

#if 0
    IMPLICIT_TAG = 9,
/*
 *****************************************************************************************
 * NON_CHARACTER FDD0 - FDEF, FFFE, FFFF, 1FFFE, 1FFFF, 2FFFE, 2FFFF,...e.g. **FFFE, **FFFF
 ******************************************************************************************
 */
#endif

// * set to zero
struct {
      UChar32 start;
      UChar32 end;
      int32_t value;
    } ranges[] =
    {
#if 0
      {0xAC00, 0xD7AF, UCOL_SPECIAL_FLAG | (HANGUL_SYLLABLE_TAG << 24) },  //0 HANGUL_SYLLABLE_TAG,/* AC00-D7AF*/
      {0xD800, 0xDBFF, UCOL_SPECIAL_FLAG | (LEAD_SURROGATE_TAG << 24)  },  //1 LEAD_SURROGATE_TAG,  /* D800-DBFF*/
      {0xDC00, 0xDFFF, UCOL_SPECIAL_FLAG | (TRAIL_SURROGATE_TAG << 24) },  //2 TRAIL_SURROGATE DC00-DFFF
      {0x3400, 0x4DB5, UCOL_SPECIAL_FLAG | (CJK_IMPLICIT_TAG << 24)    },  //3 CJK_IMPLICIT_TAG,   /* 0x3400-0x4DB5*/
      {0x4E00, 0x9FA5, UCOL_SPECIAL_FLAG | (CJK_IMPLICIT_TAG << 24)    },  //4 CJK_IMPLICIT_TAG,   /* 0x4E00-0x9FA5*/
      {0xF900, 0xFA2D, UCOL_SPECIAL_FLAG | (CJK_IMPLICIT_TAG << 24)    },  //5 CJK_IMPLICIT_TAG,   /* 0xF900-0xFA2D*/
      {0x20000, 0x2A6D6, UCOL_SPECIAL_FLAG | (CJK_IMPLICIT_TAG << 24)  },  //6 CJK_IMPLICIT_TAG,   /* 0x20000-0x2A6D6*/
      {0x2F800, 0x2FA1D, UCOL_SPECIAL_FLAG | (CJK_IMPLICIT_TAG << 24)  },  //7 CJK_IMPLICIT_TAG,   /* 0x2F800-0x2FA1D*/
#endif
      {0xAC00, 0xD7B0, UCOL_SPECIAL_FLAG | (HANGUL_SYLLABLE_TAG << 24) },  //0 HANGUL_SYLLABLE_TAG,/* AC00-D7AF*/
      //{0xD800, 0xDC00, UCOL_SPECIAL_FLAG | (LEAD_SURROGATE_TAG << 24)  },  //1 LEAD_SURROGATE_TAG,  /* D800-DBFF*/
      {0xDC00, 0xE000, UCOL_SPECIAL_FLAG | (TRAIL_SURROGATE_TAG << 24) },  //2 TRAIL_SURROGATE DC00-DFFF
      // Now directly handled in the collation code by the swapCJK function. 
      //{0x3400, 0x4DB6, UCOL_SPECIAL_FLAG | (CJK_IMPLICIT_TAG << 24)    },  //3 CJK_IMPLICIT_TAG,   /* 0x3400-0x4DB5*/
      //{0x4E00, 0x9FA6, UCOL_SPECIAL_FLAG | (CJK_IMPLICIT_TAG << 24)    },  //4 CJK_IMPLICIT_TAG,   /* 0x4E00-0x9FA5*/
      //{0xF900, 0xFA2E, UCOL_SPECIAL_FLAG | (CJK_IMPLICIT_TAG << 24)    },  //5 CJK_IMPLICIT_TAG,   /* 0xF900-0xFA2D*/
      //{0x20000, 0x2A6D7, UCOL_SPECIAL_FLAG | (CJK_IMPLICIT_TAG << 24)  },  //6 CJK_IMPLICIT_TAG,   /* 0x20000-0x2A6D6*/
      //{0x2F800, 0x2FA1E, UCOL_SPECIAL_FLAG | (CJK_IMPLICIT_TAG << 24)  },  //7 CJK_IMPLICIT_TAG,   /* 0x2F800-0x2FA1D*/
    };
    uint32_t i = 0;

    for(i = 0; i<sizeof(ranges)/sizeof(ranges[0]); i++) {
      /*ucmpe32_setRange32(t->mapping, ranges[i].start, ranges[i].end, ranges[i].value); */
      utrie_setRange32(t->mapping, ranges[i].start, ranges[i].end, ranges[i].value, TRUE);
    }


    int32_t surrogateCount = 0;
    while(!feof(data)) {
        if(U_FAILURE(*status)) {
            fprintf(stderr, "Something returned an error %i (%s) while processing line %u of %s. Exiting...\n",
                *status, u_errorName(*status), (int)line, filename);
            exit(*status);
        }

        element = readAnElement(data, t, &consts, status);
        line++;
        if(VERBOSE) {
          fprintf(stdout, "%u ", (int)line);
        }
        if(element != NULL) {
            // we have read the line, now do something sensible with the read data!

            // Below stuff was taken care of in readAnElement
            //if(element->variableTop == TRUE && variableTopValue == 0) {
            //    t->options->variableTopValue = element->cPoints[0];
            //}

            // if element is a contraction, we want to add it to contractions
            if(element->cSize > 1 && element->cPoints[0] != 0xFDD0) { // this is a contraction
              if(UTF_IS_LEAD(element->cPoints[0]) && UTF_IS_TRAIL(element->cPoints[1]) && element->cSize == 2) {
                surrogateCount++;
              } else {
                contractionCEs[noOfContractions][0] = element->cPoints[0];
                contractionCEs[noOfContractions][1] = element->cPoints[1];
                if(element->cSize > 2) { // the third one
                  contractionCEs[noOfContractions][2] = element->cPoints[2];
                } else {
                  contractionCEs[noOfContractions][2] = 0;
                }
                noOfContractions++;
              }
            }

            /* we're first adding to inverse, because addAnElement will reverse the order */
            /* of code points and stuff... we don't want that to happen */
            addToInverse(element, status);
            if(!(element->cSize > 1 && element->cPoints[0] == 0xFDD0)) {
              uprv_uca_addAnElement(t, element, status);
            }
        }
    }

    if(UCAVersion[0] == 0 && UCAVersion[1] == 0 && UCAVersion[2] == 0 && UCAVersion[3] == 0) {
        fprintf(stderr, "UCA version not specified. Cannot create data file!\n");
        uprv_uca_closeTempTable(t);
        uprv_free(opts);
        uprv_free(myD);
        fclose(data);
        return -1;
    }
/*    {
        uint32_t trieWord = utrie_get32(t->mapping, 0xDC01, NULL);
    }*/

    if (VERBOSE) {
        fprintf(stdout, "\nLines read: %u\n", (int)line);
        fprintf(stdout, "Surrogate count: %i\n", (int)surrogateCount);
        fprintf(stdout, "Raw data breakdown:\n");
        /*fprintf(stdout, "Compact array stage1 top: %i, stage2 top: %i\n", t->mapping->stage1Top, t->mapping->stage2Top);*/
        fprintf(stdout, "Number of contractions: %u\n", (int)noOfContractions);
        fprintf(stdout, "Contraction image size: %u\n", (int)t->image->contractionSize);
        fprintf(stdout, "Expansions size: %i\n", (int)t->expansions->position);
    }


    /* produce canonical closure for table */
    /* first set up constants for implicit calculation */
    uprv_uca_initImplicitConstants(consts.UCA_PRIMARY_IMPLICIT_MIN, consts.UCA_PRIMARY_IMPLICIT_MAX, status);
    /* do the closure */
    int32_t noOfClosures = uprv_uca_canonicalClosure(t, NULL, status);
    if(noOfClosures != 0) {
      fprintf(stderr, "Warning: %i canonical closures occured!\n", (int)noOfClosures);
    }

    /* test */
    UCATableHeader *myData = uprv_uca_assembleTable(t, status);  

    if (VERBOSE) {
        fprintf(stdout, "Compacted data breakdown:\n");
        /*fprintf(stdout, "Compact array stage1 top: %i, stage2 top: %i\n", t->mapping->stage1Top, t->mapping->stage2Top);*/
        fprintf(stdout, "Number of contractions: %u\n", (int)noOfContractions);
        fprintf(stdout, "Contraction image size: %u\n", (int)t->image->contractionSize);
        fprintf(stdout, "Expansions size: %i\n", (int)t->expansions->position);
    }

    if(U_FAILURE(*status)) {
        fprintf(stderr, "Error creating table: %s\n", u_errorName(*status));
        uprv_uca_closeTempTable(t);
        uprv_free(opts);
        uprv_free(myD);
        fclose(data);
        return -1;
    }

    /* populate the version info struct with version info*/
    myData->version[0] = UCOL_BUILDER_VERSION;
    myData->version[1] = UCAVersion[0];
    myData->version[2] = UCAVersion[1];
    myData->version[3] = UCAVersion[2];
    /*TODO:The fractional rules version should be taken from FractionalUCA.txt*/
    // Removed this macro. Instead, we use the fields below
    //myD->version[1] = UCOL_FRACTIONAL_UCA_VERSION;
    //myD->UCAVersion = UCAVersion; // out of FractionalUCA.txt
    uprv_memcpy(myData->UCAVersion, UCAVersion, sizeof(UVersionInfo));
    u_getUnicodeVersion(myData->UCDVersion);

    writeOutData(myData, &consts, contractionCEs, noOfContractions, outputDir, copyright, status);

    InverseUCATableHeader *inverse = assembleInverseTable(status);
    uprv_memcpy(inverse->UCAVersion, UCAVersion, sizeof(UVersionInfo));
    writeOutInverseData(inverse, outputDir, copyright, status);

    uprv_uca_closeTempTable(t);
    uprv_free(myD);
    uprv_free(opts);


    uprv_free(myData);
    uprv_free(inverse);
    fclose(data);

    return 0;
}

#endif /* #if !UCONFIG_NO_COLLATION */

static UOption options[]={
    UOPTION_HELP_H,              /* 0  Numbers for those who*/ 
    UOPTION_HELP_QUESTION_MARK,  /* 1   can't count. */
    UOPTION_COPYRIGHT,           /* 2 */
    UOPTION_VERSION,             /* 3 */
    UOPTION_DESTDIR,             /* 4 */
    UOPTION_SOURCEDIR,           /* 5 */
    UOPTION_VERBOSE,             /* 6 */
    UOPTION_ICUDATADIR           /* 7 */
    /* weiv can't count :))))) */
};

int main(int argc, char* argv[]) {
    UErrorCode status = U_ZERO_ERROR;
    const char* destdir = NULL;
    const char* srcDir = NULL;
    char filename[300];
    char *basename = NULL;
    const char *copyright = NULL;
    uprv_memset(&UCAVersion, 0, 4);

    U_MAIN_INIT_ARGS(argc, argv);

    /* preset then read command line options */
    options[4].value=u_getDataDirectory();
    options[5].value="";
    argc=u_parseArgs(argc, argv, sizeof(options)/sizeof(options[0]), options);

    /* error handling, printing usage message */
    if(argc<0) {
        fprintf(stderr,
            "error in command line argument \"%s\"\n",
            argv[-argc]);
    } else if(argc<2) {
        argc=-1;
    }
    if(options[0].doesOccur || options[1].doesOccur) {
        fprintf(stderr,
            "usage: %s [-options] file\n"
            "\tRead in UCA collation text data and write out the binary collation data\n"
            "options:\n"
            "\t-h or -? or --help  this usage text\n"
            "\t-V or --version     show a version message\n"
            "\t-c or --copyright   include a copyright notice\n"
            "\t-d or --destdir     destination directory, followed by the path\n"
            "\t-s or --sourcedir   source directory, followed by the path\n"
            "\t-v or --verbose     turn on verbose output\n"
            "\t-i or --icudatadir  directory for locating any needed intermediate data files,\n"
            "\t                    followed by path, defaults to %s\n",
            argv[0], u_getDataDirectory());
        return argc<0 ? U_ILLEGAL_ARGUMENT_ERROR : U_ZERO_ERROR;
    }
    if(options[3].doesOccur) {
        fprintf(stdout, "genuca version %hu.%hu, ICU tool to read UCA text data and create UCA data tables for collation.\n",
#if UCONFIG_NO_COLLATION
            0, 0
#else
            UCA_FORMAT_VERSION_0, UCA_FORMAT_VERSION_1
#endif
            );
        fprintf(stdout, U_COPYRIGHT_STRING"\n");
        exit(0);
    }

    /* get the options values */
    destdir = options[4].value;
    srcDir = options[5].value;
    VERBOSE = options[6].doesOccur;

    if (options[2].doesOccur) {
        copyright = U_COPYRIGHT_STRING;
    }

    if (options[7].doesOccur) {
        u_setDataDirectory(options[7].value);
    }
    /* Initialize ICU */
    u_init(&status);
    if (U_FAILURE(status) && status != U_FILE_ACCESS_ERROR) {
        fprintf(stderr, "%s: can not initialize ICU.  status = %s\n",
            argv[0], u_errorName(status));
        exit(1);
    }
    status = U_ZERO_ERROR;


    /* prepare the filename beginning with the source dir */
    uprv_strcpy(filename, srcDir);
    basename=filename+uprv_strlen(filename);

    if(basename>filename && *(basename-1)!=U_FILE_SEP_CHAR) {
        *basename++ = U_FILE_SEP_CHAR;
    }

    if(argc < 0) { 
      uprv_strcpy(basename, "FractionalUCA.txt");
    } else {
      argv++;
      uprv_strcpy(basename, getLongPathname(*argv));
    }

#if 0
    if(u_getCombiningClass(0x0053) == 0)
    {
        fprintf(stderr, "SEVERE ERROR: Normalization data is not functioning! Bailing out.  Was not able to load unorm.dat.\n");
        exit(1);
    }
#endif 

#if UCONFIG_NO_COLLATION

    UNewDataMemory *pData;
    const char *msg;
    
    msg = "genuca writes dummy " UCA_DATA_NAME "." UCA_DATA_TYPE " because of UCONFIG_NO_COLLATION, see uconfig.h";
    fprintf(stderr, "%s\n", msg);
    pData = udata_create(destdir, UCA_DATA_TYPE, UCA_DATA_NAME, &dummyDataInfo,
                         NULL, &status);
    udata_writeBlock(pData, msg, strlen(msg));
    udata_finish(pData, &status);

    msg = "genuca writes dummy " INVC_DATA_NAME "." INVC_DATA_TYPE " because of UCONFIG_NO_COLLATION, see uconfig.h";
    fprintf(stderr, "%s\n", msg);
    pData = udata_create(destdir, INVC_DATA_TYPE, INVC_DATA_NAME, &dummyDataInfo,
                         NULL, &status);
    udata_writeBlock(pData, msg, strlen(msg));
    udata_finish(pData, &status);

    return (int)status;

#else

    return write_uca_table(filename, destdir, copyright, &status);

#endif
}

/*
 * Hey, Emacs, please set the following:
 *
 * Local Variables:
 * indent-tabs-mode: nil
 * End:
 *
 */
