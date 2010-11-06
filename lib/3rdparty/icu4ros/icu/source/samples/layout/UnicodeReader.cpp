/*
 ******************************************************************************
 * Copyright (C) 1998-2005, International Business Machines Corporation and   *
 * others. All Rights Reserved.                                               *
 ******************************************************************************
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "unicode/utypes.h"
#include "unicode/unistr.h"

#include "layout/LETypes.h"

#include "GUISupport.h"
#include "UnicodeReader.h"

#define BYTE(b) (((int) b) & 0xFF)

/*
 * Read the text from a file. The text must start with a Unicode Byte
 * Order Mark (BOM) so that we know what order to read the bytes in.
 */
const UChar *UnicodeReader::readFile(const char *fileName, GUISupport *guiSupport, int32_t &charCount)
{
    FILE *f;
    int32_t fileSize;
    
    UChar *charBuffer;
    char *byteBuffer;
    char startBytes[4] = {'\xA5', '\xA5', '\xA5', '\xA5'};
    char errorMessage[128];
    const char *cp = "";
    int32_t signatureLength = 0;
    
    f = fopen(fileName, "rb");
    
    if( f == NULL ) {
        sprintf(errorMessage,"Couldn't open %s: %s \n", fileName, strerror(errno));
        guiSupport->postErrorMessage(errorMessage, "Text File Error");
        return 0;
    }
    
    fseek(f, 0, SEEK_END);
    fileSize = ftell(f);

    fseek(f, 0, SEEK_SET);
    fread(startBytes, sizeof(char), 4, f);

    if (startBytes[0] == '\xFE' && startBytes[1] == '\xFF') {
        cp = "UTF-16BE";
        signatureLength = 2;
    } else if (startBytes[0] == '\xFF' && startBytes[1] == '\xFE') {
        if (startBytes[2] == '\x00' && startBytes[3] == '\x00') {
            cp = "UTF-32LE";
            signatureLength = 4;
        } else {
            cp = "UTF-16LE";
            signatureLength = 2;
        }
    } else if (startBytes[0] == '\xEF' && startBytes[1] == '\xBB' && startBytes[2] == '\xBF') {
        cp = "UTF-8";
        signatureLength = 3;
    } else if (startBytes[0] == '\x0E' && startBytes[1] == '\xFE' && startBytes[2] == '\xFF') {
        cp = "SCSU";
        signatureLength = 3;
    } else if (startBytes[0] == '\x00' && startBytes[1] == '\x00' &&
        startBytes[2] == '\xFE' && startBytes[3] == '\xFF') {
        cp = "UTF-32BE";
        signatureLength = 4;
    } else {
        sprintf(errorMessage, "Couldn't detect the encoding of %s: (%2.2X, %2.2X, %2.2X, %2.2X)\n", fileName,
                    BYTE(startBytes[0]), BYTE(startBytes[1]), BYTE(startBytes[2]), BYTE(startBytes[3]));
        guiSupport->postErrorMessage(errorMessage, "Text File Error");
        fclose(f);
        return 0;
    }
        
    fileSize -= signatureLength;
    fseek(f, signatureLength, SEEK_SET);
    byteBuffer = new char[fileSize];
    
    if(byteBuffer == 0) {
        sprintf(errorMessage,"Couldn't get memory for reading %s: %s \n", fileName, strerror(errno));
        guiSupport->postErrorMessage(errorMessage, "Text File Error");
        fclose(f);
        return 0;
    }
    
    fread(byteBuffer, sizeof(char), fileSize, f);
    if( ferror(f) ) {
        sprintf(errorMessage,"Couldn't read %s: %s \n", fileName, strerror(errno));
        guiSupport->postErrorMessage(errorMessage, "Text File Error");
        fclose(f);
        delete[] byteBuffer;
        return 0;
    }
    fclose(f);
    
    UnicodeString myText(byteBuffer, fileSize, cp);

    delete[] byteBuffer;
    
    charCount = myText.length();
    charBuffer = LE_NEW_ARRAY(UChar, charCount + 1);
    if(charBuffer == 0) {
        sprintf(errorMessage,"Couldn't get memory for reading %s: %s \n", fileName, strerror(errno));
        guiSupport->postErrorMessage(errorMessage, "Text File Error");
        return 0;
    }
    
    myText.extract(0, myText.length(), charBuffer);
    charBuffer[charCount] = 0;    // NULL terminate for easier reading in the debugger
    
    return charBuffer;
}

