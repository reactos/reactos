#include <iostream.h>
#include <assert.h>
#include <windows.h>
#include "types.h"
#include "array.hxx"

extern bool g_fWriteToStdOut;

//
//
//
bool 
CompareBuffer(char* pBuff1, char* pBuff2, unsigned long nLen)
{
    for ( unsigned long i=0; i<nLen; i++ )
        if (pBuff1[i] != pBuff2[i])
            return false;
    return true;
}

bool 
CompareBufferNoCase( char* pBuff1, char* pBuff2, unsigned long nLen)
{
    // or ing with 0x20 always gives the lower case for ASCII characters.
    // this way we can compare case insensitive.
    for ( unsigned long i=0; i<nLen; i++ )
        if ( (pBuff1[i]|0x20) != (pBuff2[i]|0x20)  )
            return false;
    return true;
}


//
//
//
void 
WriteLine(HANDLE file, char* pBuff, int nLen)
{
    DWORD   dwSize;
    char    szBuff[2] = {0x0d, 0x0a};
    
    if (-1 == nLen)
    {
        nLen = lstrlen( pBuff );
    }
    
    WriteFile(file, pBuff, nLen, &dwSize, NULL);
    WriteFile(file, &szBuff, 2, &dwSize, NULL);
}


//
//
//
unsigned long 
FindEndOfLine( char* pBuff )
{
    char* pWalk = pBuff;
    
    while ( *pWalk != 0x0d )
        pWalk++;
    
    return (pWalk-pBuff);
}

//
//
//
bool 
GetLine(HANDLE file, char** ppTarget)
{
    //buffer
    char* pchBuff;
    unsigned long ulBuffIdx;
    unsigned long ulBuffSize = SZ;
    
    // Read variables
    DWORD   dwRead=0;
    BOOL    bRes;
    
    //allocate the initial buffer
    pchBuff = new char[ulBuffSize];
    
    //if we are currently positioned at wherever the previous 
    //operation left us, skip the carriage returns and 
    //get to the real data.
    pchBuff[0] = 0x0d;
    while (( pchBuff[0] == 0x0d ) || ( pchBuff[0] == 0x0a ))
    {
        bRes = ReadFile( (HANDLE)file, pchBuff, 1, &dwRead, NULL);
        if ( !bRes || !dwRead )
            return false;
    }
    
    
    //get the buffer until you reach the terminating character.
    //the first character of the buffer is filled. ulBuffIdx always points
    //to the char. to be read.
    ulBuffIdx = 1;
    
    BOOL fContinue = true;
    
    while( fContinue )
    {
        bRes = ReadFile( (HANDLE)file, &pchBuff[ulBuffIdx], 1, &dwRead, NULL);
        
        if ( !bRes || !dwRead )
            return false;
        
        //was this the terminating character?
        if ( pchBuff[ulBuffIdx] == 0x0A )
        {
            fContinue = false;
        }
        
        ulBuffIdx++;    //always point to the character to be read.
        
        //did we reach the end of the buffer? If so, enlarge buffer
        if ( 0 == (ulBuffIdx % SZ) )
        {
            //allocate new memory chunk
            char* pTmp = new char[ulBuffSize+SZ];
            
            //copy data
            memcpy( pTmp, pchBuff, ulBuffSize);
            
            //release old memory
            delete [] pchBuff;
            
            //make the new chunk the current one
            pchBuff = pTmp;
            ulBuffSize += SZ;
        }
    }
    
    pchBuff[ulBuffIdx-2] = 0;
    
    //if there is a receiving pointer and it is 
    //initialized to NULL properly
    if ((ppTarget) && (*ppTarget==NULL))
    {
        *ppTarget = pchBuff;
    }
    else
    {
        //since there are no valid receivers of the buffer,
        //we can delete it.
        delete [] pchBuff;
    }
    
    return true;
}

//----------------------------------------------------------------------------
//  Function    :   GetBlockType
//  Description :   Get a block of memory that is the first line 
//                  of a block, and determine the block type, by 
//                  checking the first characters on the line
//  Parameters  :
//          char*   :   pointer to the buf. that contains the line
//
//  Returns     :   Returns the type of the block ( enumeration BLOCK_TYPE)
//----------------------------------------------------------------------------
BLOCK_TYPE 
GetBlockType( char* pchData, char* pchTerm )
{
    assert( pchData );
    assert( pchTerm );
    
    switch (*pchData)
    {
    case 'd':
        *pchTerm = '}';         //end of block
        return BLK_DISPINT;
        break;
        
    case 'i':
        *pchTerm = '}';         //end of block
        return BLK_INTERFACE;
        break;
        
    case 'c':
        *pchTerm = '}';         //end of block
        return BLK_COCLASS;
        break;
        
    case 't':
        *pchTerm = ';';         //end of type definition
        return BLK_TYPEDEF;
        break;
        
    case '[':
        *pchTerm = ']';         //end of attribute
        return BLK_ATTR;
        break;
        
    default:
        *pchTerm = 0x0a;        //end of line
        return BLK_NONE;
        break;
    }
}

//
//
//
bool 
GetBlock(HANDLE file, INDEX* pIdx, char** ppTarget = NULL, unsigned long* pulBlockBase = NULL )
{
    char *          pchBuff;          //buffer
    unsigned long   ulBuffIdx;        //walking pointer
    unsigned long   ulBuffBase;       //end of the last memory chunk.
    unsigned long   ulBlockBase;      //beginning of the actual block, 

    //after the funny characters are skipped
    unsigned long   ulBuffSize = SZ+1; // the extra character is for the NULL termination.
    unsigned long   ulLines = 0;
    BOOL            fContinue = TRUE;
    
    //read variables
    DWORD           dwRead=0;
    BOOL            bRes;
    char            chTerm;
    
    assert(pIdx);
    
    if ( ppTarget )
        *ppTarget = NULL;       //reset the pointer to be returned.
    
    //allocate the initial buffer
    pchBuff = new char[ulBuffSize];
    ulBuffBase = ulBuffIdx = ulBlockBase = 0;
    
    //we are currently positioned at wherever the previous 
    //operation left us. learn where we are and start to read.
    pIdx->ulStartPos = SetFilePointer( (HANDLE)file, 0, NULL, FILE_CURRENT);
    
    while( fContinue ) 
    {
        //read a block from the file.
        bRes = ReadFile( (HANDLE)file, &pchBuff[ulBuffIdx], SZ, &dwRead, NULL);
        
        //if there was a failure or we had reached the end of the file.
        if ( !bRes || !dwRead )
            return false;
        
        //skip the possible 0d 0a sequences at the beginning and process the rest of 
        //the message.
        if ( ulBuffIdx== 0 )
        {
            while((pchBuff[ulBuffIdx] == 0x0D ) || 
                ( pchBuff[ulBuffIdx] == 0x0A ) ||
                ( pchBuff[ulBuffIdx] == 0x20 ))
            {
                ulBuffIdx++;                    //read position in the buffer
                ulBlockBase++;                  //increment the buffer base position.
                pIdx->ulStartPos++;             //start position in the file.
            }
            
            //if we reached the end of file by doing this, return. There is nothing left.
            if (dwRead==ulBuffIdx)
                return false;
            
            //now we are pointing to the actual data, learn the terminating
            //character and terminate this loop.
            pIdx->blockType = GetBlockType(&pchBuff[ulBuffIdx], &chTerm);
        }
        
        //walk until you reach the terminating character or the end of the read buffer
        while ((ulBuffIdx<(ulBuffBase+dwRead)) && (pchBuff[ulBuffIdx] != chTerm))
        {
            //check for the 0x0A to determine the number of lines
            //the linefeed after the chTerm is found is taken care of
            //in the spin routine for the empty lines.
            if (pchBuff[ulBuffIdx] == 0x0A )
                ulLines++;
            
            ulBuffIdx++;
        }
        
        //did we reach the end of the buffer or found the chTerm character
        if (ulBuffIdx == ulBuffSize - 1)
        {
            char* pTmp = new char[ulBuffSize+SZ];   //allocate new memory chunk
            memcpy( pTmp, pchBuff, ulBuffSize);     //copy data
            delete [] pchBuff;                      //release old memory
            pchBuff = pTmp;                         //make the new chunk the current one
            ulBuffSize += SZ;                       //adjust the buffer size
            ulBuffBase = ulBuffIdx;                 //reset the base to the current limit
        }
        else
        {
            //we found the terminating character.
            ulBuffIdx++;        //always point to the character to be read next.
            break;
        }
    }
    
    // Subtract the block base, If there were 0d0a pairs, we incremented both
    // ulStartPos and ulBuffIdx, and doubled the effect of the skipping these
    // characters. Compensation is provided by -ulBlockBase ...
    pIdx->ulEndPos = pIdx->ulStartPos + ulBuffIdx - ulBlockBase;
    
    // If the terminating character was a line break, we don't want it to go 
    // into the record as a part of this line
    if ( chTerm == 0x0a )
    {
        pIdx->ulEndPos -= 2;
        ulBuffIdx -= 2;
    }
    
    // Reset the pointer to the end of this block.  
    SetFilePointer( (HANDLE)file, pIdx->ulEndPos, NULL, FILE_BEGIN);
    
    // Terminate the buffer, since the index always shows the next 
    // position available, subtract 1 from the index.
    pchBuff[ulBuffIdx] = 0;
    
    // If there is a receiving pointer and it is 
    // initialized to NULL properly
    if ((ppTarget) && (*ppTarget==NULL) && (pulBlockBase))
    {
        *ppTarget = pchBuff;
        *pulBlockBase = ulBlockBase;
    }
    else
    {
        // Since there are no valid receivers of the buffer,
        // we can delete it.
        delete [] pchBuff;
    }
    
    return true;
}

//
//  Compare two attribute blocks. Attribute blocks are placed before interface descriptions 
//  and coclass descriptions.
//  dual --> breaker for both coclass and interface if added/removed
//  uuid(...) --> breaker for interfaces of all kinds if the contents of the uuid is changed
//
//  Return Value:
//  The return value from this function 0 if there are no differences between these two blocks
//  otherwise an error code is returned.

long
CompareAttributeBlock(  char * pRefBuf, 
                        CAutoArray<ATTRINFO>*   pRefList, 
                        char * pCurBuf,
                        CAutoArray<ATTRINFO>*   pCurList)
{
    char        szDual[] = {"dual"};
    char        szUuid[] = {"uuid"};
    char *      pszAttr;
    ATTRINFO    attrCur, attrRef;
    int         i, j, k;
    long        lErr;


    // For all of the attributes that are in the reference list, if the attribute is one of 
    // the attributes that we care about, we will make sure that the current list contains 
    // the same attribute.
    for ( i=0; i < pRefList->Size(); i++ )
    {
        pRefList->GetAt(i, &attrRef);

        if (attrRef.ulAttrLength == 4)
        {
            pszAttr = szDual;
            lErr    = CHANGE_DUALATTRREMOVED;
        }
        else if (attrRef.ulAttrLength == 42)
        {
            pszAttr = szUuid;
            lErr    = CHANGE_UUIDHASCHANGED;
        }
        else
            continue;

        assert(pszAttr);

        // is this attribute one of those that we want to check for?
        for (j=0; j<4; j++)
            if( pszAttr[j] != pRefBuf[attrRef.ulAttrStart+j])
                break;

        // We don't want to compare this attribute.
        if (j != 4)
            continue;

        // we care about this attribute. It MUST be in the current attribute block 
        for (k=0; k < pCurList->Size(); k++)
        {
            pCurList->GetAt(k, &attrCur);

            if ((!attrCur.fUsed) && 
                (0 == memcmp( pRefBuf+attrRef.ulAttrStart, 
                                pCurBuf+attrCur.ulAttrStart, 
                                attrRef.ulAttrLength)))
            {
                // if the attribute is found, then mark the current list so we skip
                // over this one next time.
                attrCur.fUsed = true;
                pCurList->Set(k, attrCur);

                // shortcut...
                break;
            }
        }
        
        // if we did not find the same attribute, return the error code that is in lErr
        if (k == pCurList->Size())
            return lErr;
    }

    // check for things that are added to the current list, 
    // but don't exist in the reference list. If something we check for is added, this is a breaker.
    for ( i=0; i < pCurList->Size(); i++)
    {
        pCurList->GetAt(i, &attrCur);

        if (!attrCur.fUsed)
        {
            // only check for "dual" here.
            pszAttr = szDual;

            for (j=0; j<4; j++)
                if( pszAttr[j] != pCurBuf[attrCur.ulAttrStart+j])
                    break;

            if (j==4)
                return CHANGE_DUALATTRADDED;
        }
    }

    return 0;
}

void 
TokenizeAttributes( char* pBuf, unsigned long nCnt, CAutoArray<ATTRINFO>* pList )
{
    unsigned long i,j;
    ATTRINFO    attrInfo;

    attrInfo.ulAttrStart = 0;

    // the first attribute is always at 0
    for ( i=0, j=0; i<= nCnt; i++, j++ )
    {
        if ( ( pBuf[i] == ',' ) || (i==nCnt) )
        {
            attrInfo.ulAttrLength = i- attrInfo.ulAttrStart;
            attrInfo.fUsed = false;

            pList->Append( attrInfo );

            i += 2;

            attrInfo.ulAttrStart = i;
        }
    }
}