#include <windows.h>
#include <iostream.h>
#include <iomanip.h>
#include <assert.h>
#include <stdio.h>
#include "array.hxx"
#include "types.h"
#include "icomp.hxx"
#include "CoComp.hxx"
#include "errors.hxx"


//
// PROTOTYPES
//
void WriteLine(HANDLE file, char* pBuff, int nLen);
unsigned long FindEndOfLine(char* pBuff );
BLOCK_TYPE GetBlockType(char* pchData, char* pchTerm );
bool GetBlock(HANDLE file, INDEX* pIdx, char** ppTarget = NULL, unsigned long* pulBlockBase=NULL );
void DisplayError(UINT uError, UINT uLineNum, ostream& out = cerr);
extern void TokenizeAttributes( char* pBuf, unsigned long nCnt, CAutoArray<ATTRINFO>* pList );
extern long CompareAttributeBlock(  char * pRefBuf, CAutoArray<ATTRINFO>* pRefList, char * pCurBuf, 
                                                    CAutoArray<ATTRINFO>* pCurList);
//
// GLOBALS
//
bool fgCoClass;
bool fgInterface;
bool fgDispInterface;
bool fgMethodAttribute;
bool fgMethodParameter;
bool fgInterfaceAttr;
bool fgFlush;

bool fgParamNames;
bool fgParamTypes;
bool fgParamNameCase;
bool fgParamTypeCase;

unsigned long g_ulAppRetVal;
bool g_fWriteToStdOut = false;

char g_szDiffFileName[14] = {0};   // Will contain differences 
char g_szNewFileName[14]  = {0};   // Will contain added interfaces, etc.
char g_szCurFileName[256] = {0};   // Contains tlviewer output for current tlb
char g_szRefFileName[256] = {0};   // Contains tlviewer output for new tlb

char* g_pszMethodAttr = NULL;

#define BUFFER_SIZE(buf) (sizeof(buf)/sizeof(buf[0]))

//
// Test function to test the index structure on the original MSHTML.OUT
// without debugging the system. The mirror file and the original fileCur
// should only be different with their blanks.
//
void CreateTestMirror(HANDLE fileCur, CAutoArray<INDEX>* rgFileIdx)
{
    INDEX    idxTmp;
    char*    pTmp;
    DWORD    dwRead;
    
    HANDLE  fileData = CreateFile("mirror", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (INVALID_HANDLE_VALUE == fileData)
    {
        DisplayError(ERR_OPENINGMIRROR, __LINE__);
        return;
    }
    
    // Write this stuff to a file.
    for (int i = 0; i < rgFileIdx->Size(); i++)
    {
        long nLen;
        
        //get records
        rgFileIdx->GetAt(i, &idxTmp);
        
        nLen = idxTmp.ulEndPos - idxTmp.ulAttrStartPos;

        //allocate memory to read
        pTmp = new char[nLen+1];
        
        if (!pTmp)  // Hopefully will never happen but just in case.
        {
            DisplayError(ERR_OUTOFMEMORY, __LINE__);
            return;
        }
        
        // Read data
        SetFilePointer( (HANDLE)fileCur, idxTmp.ulAttrStartPos, NULL, FILE_BEGIN);
        ReadFile( (HANDLE)fileCur, pTmp, nLen, &dwRead, NULL);
        
        // Terminate string
        pTmp[ nLen ]=0;
        
        // Write to file
        WriteLine(fileData, pTmp, nLen);
        
        // Release mem.
        delete [] pTmp;
    }
    
    CloseHandle(fileData);
}

//
//
//
void MarkAttrChanges(HANDLE fileCur, CAutoArray<INDEX>* rgFileIdx, char* pszDiffFileName)
{
   INDEX   idxTmp1, idxTmp2;
   DWORD   dwRead;
   HANDLE  fileLocal = INVALID_HANDLE_VALUE;
   char   szEntry[160];
   char   szTmp[80];

   // First, add the modified ATTR tags to the difference file.
   if (g_fWriteToStdOut)
   {
       fileLocal = GetStdHandle(STD_OUTPUT_HANDLE);
   }
   else
   {
      fileLocal = CreateFile(pszDiffFileName, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, NULL);
   }

   if (INVALID_HANDLE_VALUE == fileLocal)
   {
       DisplayError(ERR_OPENINGDIFF, __LINE__);
       return;
   }

   SetFilePointer(fileLocal, 0, NULL, FILE_END);

   // An attribute cannot be the last block in a file, so we
   // can go until size - 1
   for ( int i = 0; i < rgFileIdx->Size() - 1; i++)
   {
      // Get record 
      rgFileIdx->GetAt(i, &idxTmp1);

      // If it is an unmatched attribute, analyze
      if ((!idxTmp1.fCopied) && (idxTmp1.blockType == BLK_ATTR))
      {
         // Get the next block
         rgFileIdx->GetAt(i + 1, &idxTmp2);

         // If the next block is valid and it is matched, then the attributes
         // for it must have changed.
            //
         if ((idxTmp2.blockType != BLK_NONE) && 
            (idxTmp2.fCopied))
         {
            // Take a note of this in the differences file
            lstrcpy(szEntry, "Attributes for ");

            // Read the data
            SetFilePointer((HANDLE)fileCur, idxTmp2.ulStartPos, NULL, FILE_BEGIN);
            ReadFile((HANDLE)fileCur, szTmp, 79, &dwRead, NULL);

            // Add the type and the name of the block here.
            unsigned int j = 0;
            int nSpaceCnt = 0;

            while (j < min(79, dwRead))
            {
               if (szTmp[j] == ' ') 
                  nSpaceCnt++;

               if (nSpaceCnt == 2)
                  break;

               j++;
            }

            if (nSpaceCnt!=2)
               break;

            szTmp[j] = 0;

            lstrcat(szEntry, szTmp);
            lstrcat(szEntry, " changed ");

            WriteLine(fileLocal, szEntry, -1);

            idxTmp1.fCopied = true;
            rgFileIdx->Set(i, idxTmp1);
         }
      }
   }

    // Don't want to close the stdout. Otherwise, write
    // operations go nowhere. We will close stdout
    // at the end of this program.
    //
    if (!g_fWriteToStdOut)
        CloseHandle(fileLocal);
}

//
//
//
void FlushDifferences(HANDLE fileCur, CAutoArray<INDEX>* rgFileIdx)
{
    INDEX   idxTmp;
    DWORD   dwRead;
    char*   pTmp;
    bool    fCheck;
    HANDLE  fileDiff = INVALID_HANDLE_VALUE;

    if (g_fWriteToStdOut)
    {
        fileDiff = GetStdHandle(STD_OUTPUT_HANDLE);
    }
    else
    {
       fileDiff = CreateFile(g_szNewFileName, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    }

    if (INVALID_HANDLE_VALUE == fileDiff)
    {
        DisplayError(ERR_OPENINGNEW, __LINE__);
        return;
    }
    
    // Write this stuff to a file.
    bool fFirst = true;

    for (int i = 0; i < rgFileIdx->Size(); i++)
    {
        // Get records
        rgFileIdx->GetAt(i, &idxTmp);

        switch ( idxTmp.blockType )
        {
             case BLK_INTERFACE:
                if (!fgInterface)
                   fCheck = false;
                break;

             case BLK_DISPINT:
                if (!fgDispInterface)
                   fCheck = false;
                break;

             case BLK_COCLASS:
                if (!fgCoClass)
                   fCheck = false;

             default:
                fCheck = true;
        }

        if ((!idxTmp.fCopied) && (fCheck))
        {
            // Allocate memory to read
            pTmp = new char[idxTmp.ulEndPos - idxTmp.ulStartPos+1];
         
            // Read data
            SetFilePointer((HANDLE)fileCur, idxTmp.ulStartPos, NULL, FILE_BEGIN);
            ReadFile((HANDLE)fileCur, pTmp, idxTmp.ulEndPos - idxTmp.ulStartPos, &dwRead, NULL);
         
            // Terminate string
            pTmp[idxTmp.ulEndPos - idxTmp.ulStartPos ]=0;
         
            if (fFirst)
            {
                fFirst = false;
            
                // Write out a blank line if writing to stdout
                //
                if (g_fWriteToStdOut)
                    WriteLine(fileDiff, "", -1);
            
                WriteLine(fileDiff, "------------------------------------------", -1);
                WriteLine(fileDiff, "NEW BLOCKS", -1);
                WriteLine(fileDiff, "------------------------------------------", -1);
                WriteLine(fileDiff, "", -1);
            }

            // Write the block
            WriteLine(fileDiff, pTmp, idxTmp.ulEndPos - idxTmp.ulStartPos);
         
            // Put some empty lines after the block to make it easier to read
            WriteLine(fileDiff, "", -1);
         
         // ScotRobe: only want to set g_ulAppRetVal to an error 
         // code if something was changed. We don't want to stop the
         // build process if something was added. Adding is okay,
         // change is bad.
         //
/*       switch (idxTmp.blockType)
         {
            case BLK_INTERFACE:
               ulTmp |= CHANGE_ADDINTERFACE;
               break;

            case BLK_DISPINT:
               ulTmp |= CHANGE_ADDDISPINT;
               break;

            case BLK_COCLASS:
               ulTmp |= CHANGE_ADDCOCLASS;
               break;

            case BLK_ATTR:
               ulTmp |= CHANGE_ADDATTRIBUTE;
               break;

            default:
               break;
         }
*/

            // Release memory.
            delete [] pTmp;
        }
    }

    // Don't want to close the stdout if we are writing to it.
    // Otherwise, write operations go nowhere. We will close
    // stdout at the end of main.
    //
    if (!g_fWriteToStdOut)
        CloseHandle(fileDiff);

    // Uncomment this line if you uncomment the above switch statement.
    //
    //g_ulAppRetVal |= ulTmp;  
}

//----------------------------------------------------------------------------
// Function :  PrepareIndex
// Description :  Prepare the index for a given file by creating an array of
//             INDEX structures.
// Parameters  :  
//       fileCur              : handle of current file
//       CAutoArray<INDEX>*   : pointer to the index array
// 
// Returns     :  void
//----------------------------------------------------------------------------
void PrepareIndex (HANDLE fileCur, CAutoArray<INDEX>* rgFileIdx)
{
    INDEX    idxCurr = {BLK_NONE, 0};
    INDEX    idxAttr;
    
    bool    fContinue = true;
    
    // Create an index for the file that is to be analyzed.
    while (fContinue)
    {
        fContinue = GetBlock(fileCur, &idxCurr);
        
        if (fContinue)
        {
            switch (idxCurr.blockType)
            {
            case BLK_ATTR:
                idxAttr = idxCurr;
                break;
                
            case BLK_DISPINT:
            case BLK_INTERFACE:
            case BLK_COCLASS:
                assert(idxAttr.blockType == BLK_ATTR);
                
                // copy the attribute start and end positions to the current block's
                // index structure.
                idxCurr.ulAttrStartPos = idxAttr.ulStartPos;
                idxCurr.ulAttrEndPos = idxAttr.ulEndPos;
                
                //add the data to the index 
                rgFileIdx->Append(idxCurr);

                // reset
                idxAttr.blockType = BLK_NONE;
                idxAttr.ulStartPos = idxAttr.ulEndPos = 0;
                
                break;

/* LEAVE TYPEDEFs out of this for now, since there are problems with them.
                The end of the typedef block is coded as it should be a semicolon. However, the semicolon could be
                used for anything inside the typedef when the typedef is a structure. This causes a false termination
                of the typedef.
    Solution:
                Find the '}', however, end the block at the end of the line that contains the '}'

            case BLK_TYPEDEF:
                // copy the block as it is. However, for the mirror file use, make sure the
                // attribute information starts and ends with the actual buffer.
                idxCurr.ulAttrStartPos = idxCurr.ulStartPos;
                
                //add the data to the index 
                rgFileIdx->Append(idxCurr);

                // reset
                idxAttr.blockType = BLK_NONE;
                idxAttr.ulStartPos = idxAttr.ulEndPos = 0;
                break;
*/

            }
        }
    }
}

//
//
//
bool 
CompareCoClass( char* pCurBuf, char* pRefBuf, 
               char * pAttrCur, char * pAttrRef, HANDLE fileDiff)
{
    bool                    bRes = true;
    char                    szClassName[64] = {0};
    CCompareCoClass *       pCompareCoClass = NULL;
    CAutoArray<ATTRINFO>*   pCurList = new CAutoArray<ATTRINFO>;
    CAutoArray<ATTRINFO>*   pRefList = new CAutoArray<ATTRINFO>;
    long                    lRes;
    
    // compare the names of the coclasses. The comparison is done until we reach the 
    // end of the longer name
    for ( unsigned long ulIdx = LEN_COCLASS+1; 
    (pCurBuf[ulIdx] != 0x20) || (pRefBuf[ulIdx] != 0x20); 
    ulIdx++ )
    {
        if ( pCurBuf[ulIdx] != pRefBuf[ulIdx] )
            goto Fail;        //if the names don't match, these are not the coclasses that we
        //want to compare.
        
        szClassName[ulIdx-LEN_COCLASS-1] = pCurBuf[ulIdx]; //get the coclass name 
        
        //OVERFLOW check
        assert( ulIdx<64 );              //sanity check to stop a corrupt file from killing us.
        if ( ulIdx==64 )
            goto Fail;
    }
    
    TokenizeAttributes( pAttrRef+1, lstrlen(pAttrRef-2), pRefList );
    TokenizeAttributes( pAttrCur+1, lstrlen(pAttrCur-2), pCurList );

    lRes = CompareAttributeBlock( pAttrRef+1, pRefList, pAttrCur+1, pCurList);

    if (lRes != 0)
    {
        char    szBuf[MAX_PATH];

        if (lRes == CHANGE_UUIDHASCHANGED)
        {
            sprintf(szBuf, "\nGUID was modified for the coclass %s", szClassName);
        }
        else
        {
            sprintf(szBuf, "\nUnexpected error when comparing attributes for coclass %s", szClassName);
        }

        WriteLine(fileDiff, szBuf, -1);

        g_ulAppRetVal |= CHANGE_BLOCKREMOVED;

        goto Fail;
    }

    pCompareCoClass = new CCompareCoClass( pCurBuf, pRefBuf, fileDiff, szClassName );
    
    pCompareCoClass->FindAdditionsAndChanges();
    pCompareCoClass->FindRemovals();
    
Cleanup:
    if (pCompareCoClass)
        delete pCompareCoClass;
    return bRes;
    
Fail:
    bRes= false;
    goto Cleanup;
}

//
//
//
bool 
CompareInterface( BLOCK_TYPE blockType, char* pCurBuf, char* pRefBuf, 
                      char * pAttrCur, char * pAttrRef, HANDLE fileDiff)
{
    bool                    bRes = true;
    unsigned long           ulNameOffset=0; //offset to start reading for the name of the interface on both blocks.
    char                    szIntName[64] = {0};    //initialize to zero so we don't deal with termination later.
    CCompareInterface *     pCompareInterface = NULL;
    CAutoArray<ATTRINFO>*   pCurList = new CAutoArray<ATTRINFO>;
    CAutoArray<ATTRINFO>*   pRefList = new CAutoArray<ATTRINFO>;
    long                    lRes;
    
    //assign the offset for the first space
    ulNameOffset = ( blockType==BLK_DISPINT ) ? LEN_DISPINT+1 : LEN_INTERFACE+1;
    
    // The format of the name is: interface/dispinterface_name_{
    // Compare until the space character is reached for either of the names. 
    for (unsigned long ulIdx = ulNameOffset; 
    (pCurBuf[ulIdx] != 0x20) || (pRefBuf[ulIdx] != 0x20); 
    ulIdx++ )
    {
        if ( pCurBuf[ulIdx] != pRefBuf[ulIdx] )
            goto Fail;                     //if the name does not match, this is not 
        //the interface we are looking to compare with
        
        szIntName[ulIdx-ulNameOffset] = pCurBuf[ulIdx]; //get the interface name simultaneously
        
        //OVERFLOW CHECK
        assert( ulIdx<64 );              //sanity check to stop a corrupt file from killing us.
        if ( ulIdx==64 )
            goto Fail;
    }
    
    TokenizeAttributes( pAttrRef+1, lstrlen(pAttrRef-2), pRefList );
    TokenizeAttributes( pAttrCur+1, lstrlen(pAttrCur-2), pCurList );

    lRes = CompareAttributeBlock( pAttrRef+1, pRefList, pAttrCur+1, pCurList);

    if (lRes != 0)
    {
        char    szBuf[MAX_PATH];

        switch(lRes)
        {
        case CHANGE_DUALATTRADDED:
            sprintf(szBuf, "\nDual attribute was added to the interface %s", szIntName);
            break;

        case CHANGE_DUALATTRREMOVED:
            sprintf(szBuf, "\nDual attribute was removed from the interface %s", szIntName);
            break;

        case CHANGE_UUIDHASCHANGED:
            sprintf(szBuf, "\nIID was changed for the interface %s", szIntName);
            break;

        default:
            break;
        }

        WriteLine(fileDiff, szBuf, -1);

        g_ulAppRetVal |= CHANGE_BLOCKREMOVED;

        goto Fail;
    }

    pCompareInterface = new CCompareInterface(pCurBuf, pRefBuf, fileDiff, szIntName, blockType, g_pszMethodAttr);

    pCompareInterface->FindAdditionsAndChanges();
    pCompareInterface->FindRemovals();
    
Cleanup:
    if ( pCompareInterface )
        delete pCompareInterface;
    return bRes;
    
Fail:
    bRes= false;
    goto Cleanup;
}

//
//
//
bool CompareBlocks(HANDLE fileCur, CAutoArray<INDEX>* rgFileIdx, INDEX* pIdxRef, 
                   char* pRefBuf, char * pAttrRef, HANDLE fileDiff)
{
    INDEX   idxCur  = {BLK_NONE, 0}; // Index record for the current file's block
    char *  pCurBuf = NULL;
    char *  pAttrCur = NULL;
    DWORD   dwRead;
    BOOL    bRes = FALSE;
    int     nRes = 1;
    
    //we have index information about the reference file block
    //use it to narrow down the possibilities
    //
    //walk through the index and find the matching block types
    //with matching lengths.
    for (int i = 0; i < rgFileIdx->Size(); i++)
    {
        rgFileIdx->GetAt(i, &idxCur);
        
        //if the block we read from the reference file matches this
        //block's type 
        if ((idxCur.blockType == pIdxRef->blockType) &&
            (!idxCur.fCopied))
        {
            //read the block from the current file, using the
            //index record
            int nLen = idxCur.ulEndPos - idxCur.ulStartPos;
            pCurBuf = new char[nLen+1];
            
            int nAttrLen = idxCur.ulAttrEndPos - idxCur.ulAttrStartPos;
            pAttrCur = new char[nAttrLen+1];
            
            SetFilePointer((HANDLE)fileCur, idxCur.ulStartPos, NULL, FILE_BEGIN);
            bRes = ReadFile((HANDLE)fileCur, pCurBuf, nLen, &dwRead, NULL);
            
            SetFilePointer((HANDLE)fileCur, idxCur.ulAttrStartPos+1, NULL, FILE_BEGIN);
            bRes = ReadFile((HANDLE)fileCur, pAttrCur, nAttrLen, &dwRead, NULL);
            
            // Terminate the string that was just read from the current file.
            pCurBuf[nLen] = 0;
            pAttrCur[nAttrLen] = 0;
            
            //comparison behavior may change by the type of the block to be
            //compared
            switch ( pIdxRef->blockType )
            {
            case BLK_COCLASS:
                if (fgCoClass)
                    nRes = !CompareCoClass(pCurBuf, pRefBuf, pAttrCur, pAttrRef, fileDiff);
                else
                    nRes = 0;
                break;
                
            case BLK_DISPINT:
                if (fgDispInterface)
                {
                    //if the length of two blocks is the same, there is a chance that 
                    //they are the same block. So, compare the memory before going into
                    //a long and detailed analysis
                    nRes = 1;
                    if (( pIdxRef->ulEndPos - pIdxRef->ulStartPos) == (unsigned long)nLen )
                    {
                        nRes = memcmp(pCurBuf, pRefBuf, nLen);
                    }

                    if (!nRes)
                        nRes = memcmp( pAttrCur, pAttrRef, max(lstrlen(pAttrCur), lstrlen(pAttrRef)));
                    
                    // The memory comparison failed. Go into details.
                    if ( nRes )
                        nRes = !CompareInterface(pIdxRef->blockType, pCurBuf, pRefBuf, pAttrCur, pAttrRef, fileDiff);
                }
                break;
                
            case BLK_INTERFACE:
                if (fgInterface)
                {
                    //if the length of two blocks is the same, there is a chance that 
                    //they are the same block. So, compare the memory before going into
                    //a long and detailed analysis
                    nRes = 1;
                    if ((pIdxRef->ulEndPos - pIdxRef->ulStartPos) == (unsigned long)nLen)
                    {
                        nRes = memcmp(pCurBuf, pRefBuf, nLen);
                    }
                    
                    if (!nRes)
                        nRes = memcmp( pAttrCur, pAttrRef, max(lstrlen(pAttrCur), lstrlen(pAttrRef)));

                    //the memory comparison failed. Go into details.
                    if ( nRes )
                        nRes = !CompareInterface(pIdxRef->blockType, pCurBuf, pRefBuf, pAttrCur, pAttrRef, fileDiff);
                }
                break;
                
            default:
                nRes = memcmp(pCurBuf, pRefBuf, nLen);
                break;
            }
            
            if (nRes == 0)
            {
                // Mark the current index record, so it's not used anymore.
                // This is true if the block name is found, regardless of the content 
                // comparison results.
                idxCur.fCopied = true;
                rgFileIdx->Set(i, idxCur);
                break;
            }
            
            
            delete [] pCurBuf;
            pCurBuf = NULL;
        }
    }
    
    if (pCurBuf)
    {
        delete [] pCurBuf;
        pCurBuf = NULL;
    }
    
    return (nRes ? false : true);
}

//
// Compare two files, by walking through the reference file and using the index
// to read the current file.
//
void CompareFiles(HANDLE fileCur, HANDLE fileRef, 
                  CAutoArray<INDEX>* rgFileIdx, HANDLE fileDiff)
{
    INDEX           idxRef = {BLK_NONE, 0};        // index record for the reference file block
    INDEX           idxAttr = {BLK_NONE, 0};
    
    bool            fContinue = true;
    unsigned long   ulOffset;
    char *          pRefBuf = NULL;
    char *          pAttrBuf = NULL;
    bool            fFound = false;
    
    
    //create an index for the file that is to be analyzed.
    while ( fContinue )
    {
        ulOffset = 0;
        fContinue = GetBlock(fileRef, &idxRef, &pRefBuf, &ulOffset);
        
        if (fContinue)
        {
            switch (idxRef.blockType)
            {
            case BLK_ATTR:
                int nLen;

                idxAttr = idxRef;   // the last attribute block is remembered, to be used with the 
                                    // interface, dispinteface and coclass comparison.
                // if we currently have a block, release it
                if (pAttrBuf)
                {
                    delete [] pAttrBuf;
                    pAttrBuf = NULL;
                }

                nLen = idxAttr.ulEndPos - idxAttr.ulStartPos;
                
                pAttrBuf = new char[nLen+1];

                memcpy( pAttrBuf, pRefBuf+ulOffset, nLen);

                pAttrBuf[nLen] = 0;

                break;
                
            case BLK_DISPINT:
            case BLK_INTERFACE:
            case BLK_COCLASS:
                
                assert(idxAttr.blockType == BLK_ATTR);

                idxRef.ulAttrStartPos = idxAttr.ulStartPos;
                idxRef.ulAttrEndPos = idxAttr.ulEndPos;

                fFound = CompareBlocks( fileCur, rgFileIdx, &idxRef, &pRefBuf[ulOffset], pAttrBuf, fileDiff);
                
                if (!fFound)
                {
                    char szBuf[MAX_PATH];
                    
                    sprintf(szBuf, "\n%s- Removed", strtok(&pRefBuf[ulOffset], "{"));
                    WriteLine(fileDiff, szBuf, -1);
                    g_ulAppRetVal |= CHANGE_BLOCKREMOVED;
                }
                
                //release the memory allocated by the GetBlock function
                delete [] pRefBuf;
                pRefBuf = NULL;

                delete [] pAttrBuf;
                pAttrBuf = NULL;

                idxAttr.blockType = BLK_NONE;
                break;
                
/* BUGBUG : ferhane: see comments in PrepareIndex
                case BLK_TYPEDEF:
                break;
*/ 
            }
        }
    }
}

void DisplayError(UINT uError, UINT uLineNum /* = __LINE__ */, ostream& out /* = cerr*/)
{
    assert(uError < BUFFER_SIZE(pszErrors));

    out.fill('0');
    out << "TlDiff.exe(" << uLineNum << ") : error E9"
        << setw(3) << uError << ": " << pszErrors[uError] << endl;
}

/*----------------------------------------------------------------------------
 -----------------------------------------------------------------------------*/
bool ProcessINIFile(char* lpszIniFile)
{
    bool    fRet = true;
    char    szFileNames[] = {"FileNames"};
    char    szCheckRules[] = {"CheckRules"};
    char    szMethodAttr[] = {"BreakingMethodAttributes"};
    DWORD   dwBuffSize = 256;
    DWORD   dwBuffSizeOld;

    char*  pszDirectory = new char[dwBuffSize + 13];

    dwBuffSize = GetCurrentDirectory(dwBuffSize, pszDirectory);
    if (dwBuffSize > 256)
    {
        // we need a larger buffer for path.
        delete pszDirectory;
        pszDirectory = NULL;

        pszDirectory = new char[dwBuffSize+13];
        if (!pszDirectory)
        {
            DisplayError(ERR_OUTOFMEMORY, __LINE__);
            fRet = false;
            goto Error;
        }

        GetCurrentDirectory( dwBuffSize, pszDirectory );
    }

    lstrcat(pszDirectory, "\\");
    lstrcat(pszDirectory, lpszIniFile);

    // Make sure the INI file exists
    // GetFileAttributes returns 0xFFFFFFFF if the 
    // file doesn't exist.
    //
    if (0xFFFFFFFF == GetFileAttributes(pszDirectory))
    {
        DisplayError(ERR_INIFILENOTFOUND, __LINE__);
        fRet = false;
        goto Error;
    }

/*
    if (!GetPrivateProfileString(szFileNames, "CurrentFile", "cur.out", g_szCurFileName, 255, pszDirectory))
    {
        DisplayError( ERR_CURRFILENAME );
        bRet = FALSE;
        goto Error;
    }

    if (!GetPrivateProfileString(szFileNames, "ReferenceFile", "ref.out", g_szRefFileName, 255, pszDirectory))
    {
        DisplayError( ERR_REFFILENAME );
        bRet = FALSE;
        goto Error;
    }
*/

    if (!g_fWriteToStdOut)
    {
        // Specify a NULL default name so that we can check to see
        // if the file name wasn't specified in the INI file.
        //
        // If either the difference file or new file name is missing
        // from the INI, we write to stdout.
        //
        GetPrivateProfileString(szFileNames, "DifferenceFile", NULL, g_szDiffFileName,
                                BUFFER_SIZE(g_szDiffFileName) - 1, pszDirectory);

        if (NULL == g_szDiffFileName[0])
        {
            g_fWriteToStdOut = true;
        }
        else  
        {
            GetPrivateProfileString(szFileNames, "AdditionsFile", NULL, g_szNewFileName, 
                                    BUFFER_SIZE(g_szNewFileName) - 1, pszDirectory);

            if (NULL == g_szNewFileName[0])
            {
                g_fWriteToStdOut = true;
            }
        }
    }

    fgCoClass = !!(GetPrivateProfileInt(szCheckRules, "Coclass", 1, pszDirectory));

    fgInterface = !!(GetPrivateProfileInt(szCheckRules, "Interface", 1, pszDirectory));

    fgDispInterface = !!(GetPrivateProfileInt(szCheckRules, "Dispinterface", 1, pszDirectory));

    fgMethodParameter = !!(GetPrivateProfileInt(szCheckRules, "MethodParameter", 1, pszDirectory));

    fgMethodAttribute = !!(GetPrivateProfileInt(szCheckRules, "MethodAttribute", 1, pszDirectory));

    fgInterfaceAttr = !!(GetPrivateProfileInt(szCheckRules, "InterfaceAttribute", 1, pszDirectory));

    fgFlush = !!(GetPrivateProfileInt(szCheckRules, "GenerateAdditionsFile", 1, pszDirectory));

    fgParamNames = !!(GetPrivateProfileInt(szCheckRules, "ParameterName", 1, pszDirectory));
    fgParamTypes = !!(GetPrivateProfileInt(szCheckRules, "ParameterType", 1, pszDirectory));
    fgParamNameCase= !!(GetPrivateProfileInt(szCheckRules, "ParameterNameCase", 1, pszDirectory));
    fgParamTypeCase= !!(GetPrivateProfileInt(szCheckRules, "ParameterTypeCase", 1, pszDirectory));

    // get breaking method attributes.
    g_pszMethodAttr = new char[dwBuffSize];
    dwBuffSizeOld = 0;

    // Stupidity of the APIs... It won't tell it needs more, it returns size - 1 or
    // size - 2 to indicate that it does not have enough buffer.
    //
    while (dwBuffSize < dwBuffSizeOld - 2)
    {
        dwBuffSizeOld = dwBuffSize;     // reset the condition.

        // Read into the buffer.
        dwBuffSize = GetPrivateProfileString(szMethodAttr, NULL, "none", g_pszMethodAttr,
                                             dwBuffSize, pszDirectory);

        // Did we have enough buffer size? 
        //
        if  (dwBuffSize < dwBuffSizeOld - 2)
        {
            break;
        }

        // Reallocate and try again.
        //
        delete g_pszMethodAttr;
        dwBuffSize = dwBuffSize * 2;
        g_pszMethodAttr = new char[dwBuffSize];
    }

Error:
    if (pszDirectory)
        delete pszDirectory;

    return fRet;
}

void ShowUsage()
{
    cout << endl 
         << "Usage: tldiff [-?|h] [-s] -f SettingsFile CurFile.out RefFile.out" << endl << endl
         << "Where: -?|h = Show usage. (These arguments are exclusive from the rest)" << endl
         << "       -s = write output to stdout." << endl
         << "       -f = use SettingsFile for initialization data." << endl
         << "       CurFile.out = Tlviewer file containing data for current tlb" << endl
         << "       RefFile.out = Tlviewer file containing data for new tlb"     << endl
         << endl;
}

bool ProcessArgs(int argc, char* argv[])
{
    char*  pszIniFile = NULL;
    bool    fSuccess   = true;
    int     i;

    // Check for "Show usage" request.
    //
    if ((argc >= 2) && ('-' == argv[1][0]) && ('?' == argv[1][1] || 'h' == argv[1][1]))
    {
        ShowUsage();
        return false;
    }

    if (argc < 5)
    {
        DisplayError(ERR_MISSINGARGS, __LINE__);
        goto Error;
    }

    // Note: i is initialized up top.
    //
    for (i=1; i < argc; i++)
    {
        if ('-' == argv[i][0])
        {
            switch(argv[i][1])
            {
            case 'f': 
                {
                    // The file is the next argument
                    i++;
                    if (i >= argc)
                    {
                        DisplayError(ERR_INIFILENAME, __LINE__);
                        goto Error;
                    }

                    int nBufSize = lstrlen(argv[i]) + 1;
                    pszIniFile = new char[nBufSize];

                    if (!pszIniFile)
                    {
                        DisplayError(ERR_OUTOFMEMORY, __LINE__);
                        goto Error;
                    }

                    lstrcpyn(pszIniFile, argv[i], nBufSize);

                    break;
                }

            case 's':
                g_fWriteToStdOut = true;
                break;

            // Stop the build process for invalid arguments.
            // This ensures that somebody didn't make a typo.
            // 
            default:
                DisplayError(ERR_INVALIDARG, __LINE__);
                goto Error;
            }
        }
        else
        {
            // Retrieve the current and new tlviewer output files.
            // These files must be in order. Therefore, I assume that
            // if the current file name variable is not set, then it's time
            // to set the current file name variable. If it is set, it's time
            // to set the new file name variable.
            //
            if (NULL == g_szCurFileName[0])
            {
                lstrcpyn(g_szCurFileName, argv[i], BUFFER_SIZE(g_szCurFileName));
            }
            else
            {
                lstrcpyn(g_szRefFileName, argv[i], BUFFER_SIZE(g_szRefFileName));
            }
        }
    }

    // This should never happen because the current file name
    // is the first file name in the arg list. I check anyway just
    // in case. If this every happens, that means there's a 
    // nasty bug somewhere in the code.
    //
    if (NULL == g_szCurFileName[0])
    {
        DisplayError(ERR_CURFILENAME, __LINE__);
        goto Error;
    }

    if (NULL == g_szRefFileName[0]) 
    {
        DisplayError(ERR_REFFILENAME, __LINE__);
        goto Error;
    }

    if (pszIniFile)
    {
        fSuccess = ProcessINIFile(pszIniFile);
    }
    else
    {
        DisplayError(ERR_INIFILENAME, __LINE__);
        goto Error;
    }

Cleanup:
    if (pszIniFile)
        delete pszIniFile;

    return fSuccess;

Error:
    ShowUsage();
    fSuccess = false;
    goto Cleanup;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int __cdecl main(int argc, char* argv[])
{
    
    HANDLE fileRef  = INVALID_HANDLE_VALUE;
    HANDLE fileCur  = INVALID_HANDLE_VALUE;
    HANDLE fileDiff = INVALID_HANDLE_VALUE;
    
    CAutoArray<INDEX>* rgFileIdx = NULL;
    
    //initialize global variables
    fgCoClass = fgInterface = fgDispInterface = fgMethodAttribute = fgMethodParameter
              = fgFlush = fgInterfaceAttr = true;
    
    g_ulAppRetVal = 0;
    
    //
    // Get and process the input arguments
    //
    if (!ProcessArgs(argc, argv))
    {
        goto Cleanup;
    }
    
    rgFileIdx = new CAutoArray<INDEX>;
    
    // Open the reference file and the current file
    //
    fileCur = CreateFile(g_szCurFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (INVALID_HANDLE_VALUE == fileCur)
    {
        DisplayError(ERR_OPENINGCUR, __LINE__);
        goto Cleanup;
    }
    
    fileRef = CreateFile(g_szRefFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (INVALID_HANDLE_VALUE == fileRef)
    {
        DisplayError(ERR_OPENINGREF, __LINE__);
        goto Cleanup;
    }
    
    // Prepare the index for the current file.
    cout << "Preparing index" << endl;
    PrepareIndex(fileCur, rgFileIdx);
    
    /* This was to test if the index algorithm would work on different files. 
    The mirror file should only be different from the file passed in as the
    current file, by its blanks and new lines.
    */

    //create a test mirror file using the index information we have.
    /*cout << "Creating test mirror file" << endl;
    CreateTestMirror( fileCur, rgFileIdx );
    */
    
    // Compare files.
    //
    if (g_fWriteToStdOut)
    {
        fileDiff = GetStdHandle(STD_OUTPUT_HANDLE);
    }
    else
    {
        fileDiff = CreateFile(g_szDiffFileName, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    }
    
    if (INVALID_HANDLE_VALUE == fileDiff)
    {
        DisplayError(ERR_OPENINGDIFF, __LINE__);
        goto Cleanup;
    }
    
    cout << "Comparing Files" << endl;
    
    CompareFiles(fileCur, fileRef, rgFileIdx, fileDiff);
    
    // Only close here if we aren't writing to stdout.
    // Otherwise, we won't be able to write to stdout anymore.
    //
    if ((!g_fWriteToStdOut) && (INVALID_HANDLE_VALUE != fileDiff))
    {
        CloseHandle(fileDiff);
        fileDiff = INVALID_HANDLE_VALUE;
    }
    
    // Compare the attributes of interfaces. 
    // These contain the GUIDs of interfaces too.
    //
    if (fgInterfaceAttr)
        MarkAttrChanges(fileCur, rgFileIdx, g_szDiffFileName);
    
    // Flush out the blocks that are completely new for the current file 
    // and do not exist on the reference file.
    if (fgFlush)
        FlushDifferences(fileCur, rgFileIdx);
    
    if (g_ulAppRetVal)
    {
        DisplayError(ERR_DIFFERENCES, __LINE__);
    }
    else
    {
        cout << "Comparison Complete: No Errors" << endl;
    }
    
Cleanup:
    
    if (INVALID_HANDLE_VALUE != fileCur)
        CloseHandle(fileCur);
    
    if (INVALID_HANDLE_VALUE != fileRef)
        CloseHandle(fileRef);
    
    // Due to the fact that we may be writing to 
    // stdout, we should only close it once at the
    // very end of this program. It is not specified
    // if closing the console is actually necessary.
    //
    if (INVALID_HANDLE_VALUE != fileDiff)
        CloseHandle(fileDiff);
    
    // Release the memory allocated for the index array
    if (rgFileIdx)
        delete [] rgFileIdx;
    
    if (g_pszMethodAttr)
        delete g_pszMethodAttr;
    
    return g_ulAppRetVal;
}