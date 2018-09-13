#include "private.h"
#include "detcbase.h"
#include "codepage.h"
#include "detcjpn.h"
#include "detckrn.h"

#include "fechrcnv.h"

#include "msencode.h"
#include "lcdetect.h"
#include "cpdetect.h"

CCpMRU *g_pCpMRU = NULL;



// Get data from registry and construct cache
HRESULT CCpMRU::Init(void)
{
    BOOL    bRegKeyReady = TRUE;
    HRESULT hr = S_OK;
    HKEY    hkey;

    _pCpMRU = NULL;

    // HKCR\\Software\\Microsoft\internet explorer\\international\\CpMRU
    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_CURRENT_USER, 
                         REGSTR_PATH_CPMRU,
                         0, KEY_READ|KEY_SET_VALUE, &hkey)) 
    {
        DWORD dwAction = 0;
        if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CURRENT_USER,
                                REGSTR_PATH_CPMRU,
                                0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, &dwAction))
        {
            bRegKeyReady = FALSE;
            dwCpMRUEnable = 0;
            hr = E_FAIL;
        }
    }

    if (bRegKeyReady)
    {
        DWORD dwType = REG_DWORD;
        DWORD dwSize = sizeof(DWORD);
        BOOL  bUseDefault = FALSE;

        if (ERROR_SUCCESS != RegQueryValueEx(hkey, REG_KEY_CPMRU_ENABLE, 0, &dwType, (LPBYTE)&dwCpMRUEnable, &dwSize))
        {
            dwCpMRUEnable = 1;
            RegSetValueEx(hkey, REG_KEY_CPMRU_ENABLE, 0, REG_DWORD, (LPBYTE)&dwCpMRUEnable, sizeof(dwCpMRUEnable));
        }

        // If fail to open registry data or find unreasonable cache parameters, use default settings
        if ((ERROR_SUCCESS != RegQueryValueEx(hkey, REG_KEY_CPMRU_NUM, 0, &dwType, (LPBYTE)&dwCpMRUNum, &dwSize)) ||
            (ERROR_SUCCESS != RegQueryValueEx(hkey, REG_KEY_CPMRU_INIT_HITS, 0, &dwType, (LPBYTE)&dwCpMRUInitHits, &dwSize)) ||
            (ERROR_SUCCESS != RegQueryValueEx(hkey, REG_KEY_CPMRU_PERCENTAGE_FACTOR, 0, &dwType, (LPBYTE)&dwCpMRUFactor, &dwSize)) ||
            (dwCpMRUNum > MAX_CPMRU_NUM) || !dwCpMRUFactor || !dwCpMRUInitHits)
        {
            dwCpMRUNum = DEFAULT_CPMRU_NUM;
            dwCpMRUInitHits = DEFAULT_CPMRU_INIT_HITS;
            dwCpMRUFactor = DEFAULT_CPMRU_FACTOR;
            bUseDefault = TRUE;

            // Store default value in registry
            RegSetValueEx(hkey, REG_KEY_CPMRU_NUM, 0, REG_DWORD, (LPBYTE)&dwCpMRUNum, sizeof(dwCpMRUNum));
            RegSetValueEx(hkey, REG_KEY_CPMRU_INIT_HITS, 0, REG_DWORD, (LPBYTE)&dwCpMRUInitHits, sizeof(dwCpMRUInitHits));
            RegSetValueEx(hkey, REG_KEY_CPMRU_PERCENTAGE_FACTOR, 0, REG_DWORD, (LPBYTE)&dwCpMRUFactor, sizeof(dwCpMRUFactor));
        }

        dwSize = sizeof(CODEPAGE_MRU)*dwCpMRUNum;

        if (!dwSize || NULL == (_pCpMRU = (PCODEPAGE_MRU)LocalAlloc(LPTR, dwSize)))
        {
            hr = E_FAIL;
            dwCpMRUEnable = 0;
        }

        if (_pCpMRU && !bUseDefault)
        {
            dwType = REG_BINARY;        

            if (ERROR_SUCCESS != RegQueryValueEx(hkey, REG_KEY_CPMRU, 0, &dwType, (LPBYTE)_pCpMRU, &dwSize))
            {
                ZeroMemory(_pCpMRU,sizeof(CODEPAGE_MRU)*dwCpMRUNum);
            }
        }
        RegCloseKey(hkey);      
    }

    return hr;
}

// Update registry's cache value
CCpMRU::~CCpMRU(void)
{
    HKEY hkey;

    if (bCpUpdated)
    {

        if (RegOpenKeyEx(HKEY_CURRENT_USER, 
                     REGSTR_PATH_CPMRU,
                     0, KEY_READ|KEY_SET_VALUE, &hkey) == ERROR_SUCCESS) 
        {                
            DWORD dwType = REG_BINARY;
            DWORD dwSize = sizeof(CODEPAGE_MRU)*dwCpMRUNum;
            if (_pCpMRU)
            {
                RegSetValueEx(hkey, REG_KEY_CPMRU, 0, dwType, (LPBYTE)_pCpMRU, dwSize);
                LocalFree(_pCpMRU);
                _pCpMRU = NULL;
            }

            RegCloseKey(hkey);
        }
        bCpUpdated = FALSE;
            
    }
}

HRESULT CCpMRU::GetCpMRU(PCODEPAGE_MRU pCpMRU, UINT *puiCpNum)
{
        DWORD   dwTotalHits = 0;
        UINT    i;
        HRESULT hr = E_FAIL;

        if (!(*puiCpNum))
            return E_INVALIDARG;

        if (!_pCpMRU)
            return hr;

        if (!dwCpMRUEnable || !dwCpMRUInitHits)
        {
            *puiCpNum = 0;
            return S_FALSE;
        }

        ZeroMemory(pCpMRU, sizeof(CODEPAGE_MRU)*(*puiCpNum));

        // Get total hits acount
        for (i=0; i<dwCpMRUNum; i++)
        {
            if (_pCpMRU[i].dwHistoryHits)
                dwTotalHits += _pCpMRU[i].dwHistoryHits;
            else
                break;  
        }

        // Not enough hits count to determin the result, keep collecting
        if (dwTotalHits < dwCpMRUInitHits)
        {
            *puiCpNum = 0;
            return S_FALSE;
        }

        for (i=0; i<dwCpMRUNum && i<*puiCpNum; i++)
        {
            // Percentage is 1/MIN_CPMRU_FACTOR
            if (_pCpMRU[i].dwHistoryHits*dwCpMRUFactor/dwTotalHits < 1)
                break;
        }

        if (i != 0)
        {
            CopyMemory(pCpMRU, _pCpMRU, sizeof(CODEPAGE_MRU)*(i));
            *puiCpNum = i;
            hr = S_OK;
        }

        return hr;

}

// Update code page MRU
void CCpMRU::UpdateCPMRU(DWORD dwEncoding)
{
        UINT i,j;

        if (!_pCpMRU)
            return;

        if ((dwEncoding == CP_AUTO) ||
            (dwEncoding == CP_JP_AUTO) ||
            (dwEncoding == CP_KR_AUTO))
            return;

        if (!bCpUpdated)
            bCpUpdated = TRUE;


        // Sorted         
        for (i=0; i< dwCpMRUNum; i++)
        {
            if (!_pCpMRU[i].dwEncoding || (_pCpMRU[i].dwEncoding == dwEncoding))
                break;
        }

        // If not found, replace the last encoding
        if (i == dwCpMRUNum)
        {
            _pCpMRU[dwCpMRUNum-1].dwEncoding = dwEncoding;
            _pCpMRU[dwCpMRUNum-1].dwHistoryHits = 1;
        }
        else
        {
            _pCpMRU[i].dwHistoryHits ++;

            // If it is an already exist encoding, change order as needed
            if (_pCpMRU[i].dwEncoding)
            {
                for (j=i; j>0; j--)
                {
                    if (_pCpMRU[j-1].dwHistoryHits >= _pCpMRU[i].dwHistoryHits)
                    {
                        break;
                    }
                }
                if (j < i)
                {
                    // Simple sorting
                    CODEPAGE_MRU tmpCPMRU  = _pCpMRU[i];

                    MoveMemory(&_pCpMRU[j+1], &_pCpMRU[j], (i-j)*sizeof(CODEPAGE_MRU));
                    _pCpMRU[j].dwEncoding = tmpCPMRU.dwEncoding;
                    _pCpMRU[j].dwHistoryHits = tmpCPMRU.dwHistoryHits;

                }

            }
            else
            {
                _pCpMRU[i].dwEncoding = dwEncoding;
            }

        }

        // Cached too many hits?
        if (_pCpMRU[0].dwHistoryHits > 0xFFFFFFF0)
        {
            // Find the smallest one
            for (i=dwCpMRUNum-1; i>=0; i--)
            {
                if (_pCpMRU[i].dwHistoryHits > 1)
                    break;
            }

            // Decrease Cache value
            for (j=0; j<dwCpMRUNum && _pCpMRU[j].dwHistoryHits; j++)
            {
                // We still keep those one hit encodings if any
                _pCpMRU[j].dwHistoryHits /= _pCpMRU[i].dwHistoryHits;
            }
        }
}


UINT CheckEntity(LPSTR pIn, UINT nIn)
{
    UINT uiRet = 0;
    UINT uiSearchRange;
    UINT i;
    
    uiSearchRange = (nIn > MAX_ENTITY_LENTH)? MAX_ENTITY_LENTH:nIn;

    if (*pIn == '&')
    {
        for(i=0; i<uiSearchRange; i++)
        {
            if (pIn[i] == ';')
                break;
        }
        if (i < uiSearchRange)
        {
            uiSearchRange = i+1;
            // NCR Entity
            if (pIn[1] == '#')
            {
                for (i=2; i<uiSearchRange-1; i++)
                    if (!IS_DIGITA(pIn[i]))
                    {
                        uiSearchRange = 0;
                        break;
                    }
            }
            // Name Entity
            else
            {
                for (i=1; i<uiSearchRange-1; i++)
                    if (!IS_CHARA(pIn[i]))
                    {
                        uiSearchRange = 0;
                        break;
                    }
            }
        }
        else
        {
            uiSearchRange = 0;
        }
    }
    else
    {
        uiSearchRange = 0;
    }

    return uiSearchRange;
}

void RemoveHtmlTags (LPSTR pIn, UINT *pnBytes)
//
// Remove HTML tags from pIn and compress whitespace, in-place.
// On input *pnBytes is the input length; on return *pnBytes is 
// set to the resulting length.
//
// Name Entity and NCR Entity strings also removed
{
    UINT    nIn = *pnBytes;
    UINT    nOut = 0;
    UINT    nEntity = 0;
    LPSTR   pOut = pIn;
    BOOL    fSkippedSpace = FALSE;


    while ( nIn > 0 /*&& nOut + 2 < *pnBytes */) {

        if (*pIn == '<' && nIn > 1/* && !IsNoise (pIn[1])*/) {

            // Discard text until the end of this tag.  The handling here
            // is pragmatic and imprecise; what matters is detecting mostly
            // contents text, not tags or comments.
            pIn++;
            nIn--;

            LPCSTR pSkip;
            DWORD nLenSkip;

            if ( nIn > 1 && *pIn == '%' )
            {
                pSkip = "%>";			// Skip <% to %> 
                nLenSkip = 2;
            }
            else if ( nIn > 3 && *pIn == '!' && !LowAsciiStrCmpNIA(pIn, "!--", 3) )
            {
                pSkip = "-->";			// Skip <!-- to -->
                nLenSkip = 3;
            }
            else if ( nIn > 5 && !LowAsciiStrCmpNIA(pIn, "style", 5) )
            {
                pSkip = "</style>";		// Skip <style ...> to </style>
                nLenSkip = 8;
            }
            else if ( nIn > 6 && !LowAsciiStrCmpNIA(pIn, "script", 6) )
            {
                pSkip = "</script>";	// Skip <script ...> to </script>
                nLenSkip = 9;
            }
            else if ( nIn > 3 && !LowAsciiStrCmpNIA(pIn, "xml", 3) )
            {
                pSkip = "</xml>";
                nLenSkip = 6;
            }
            else
            {
                pSkip = ">";			// match any end tag
                nLenSkip = 1;
            }

            // Skip up to a case-insensitive match of pSkip / nLenSkip

            while ( nIn > 0 )
            {
                // Spin fast up to a match of the first char.
                // NOTE: the first-char compare is NOT case insensitive
                // because this char is known to never be alphabetic.

                while ( nIn > 0 && *pIn != *pSkip )
                {
                    pIn++;
                    nIn--;
                }

                if ( nIn > nLenSkip && !LowAsciiStrCmpNIA(pIn, pSkip, nLenSkip) )
                {
                    pIn += nLenSkip;
                    nIn -= nLenSkip;
                    fSkippedSpace = TRUE;

                    break;
                }

                if ( nIn > 0)
                {
                    pIn++;
                    nIn--;
                }
            }

            // *pIn is either one past '>' or at end of input

        } 
        else 
            if (IsNoise (*pIn) || (nEntity = CheckEntity(pIn, nIn)))
            {		
			
			    // Collapse whitespace -- remember it but don't copy it now
			    fSkippedSpace = TRUE;		
                if (nEntity)
                {
                    pIn+=nEntity;
                    nIn-=nEntity;
                    nEntity = 0;
                }
                else
                {
			        while (nIn > 0 && IsNoise (*pIn))
				    pIn++, nIn--;
                }
            } 
            // *pIn is non-ws char
            else 
            {
                // Pass through all other characters
                // Compress all previous noise characters to a white space
			    if (fSkippedSpace) 
                {
                    *pOut++ = ' ';
				    nOut++;
				    fSkippedSpace = FALSE;
                }

                *pOut++ = *pIn++;
                nIn--;
                nOut++;
            }
    }

    *pnBytes = nOut;
}

static unsigned char szKoi8ru[] = {0xA4, 0xA6, 0xA7, 0xB4, 0xB6, 0xB7, 0xAD, 0xAE, 0xBD, 0xBE};
static unsigned char sz28592[]  = {0xA1, 0xA6, /*0xAB,*/ 0xAC, 0xB1, 0xB5, 0xB6, 0xB9, /*0xBB, 0xE1*/}; // Need to fine tune this data

const CPPATCH CpData[] = 
{
    {CP_KOI8R,  CP_KOI8RU,      ARRAYSIZE(szKoi8ru),    szKoi8ru},
    {CP_1250,   CP_ISO_8859_2,  ARRAYSIZE(sz28592),     sz28592},
};


// Distinguish similar western encodings
UINT PatchCodePage(UINT uiEncoding, unsigned char *pStr, int nSize)
{
    int i, l,m, n, iPatch=0;

    while (iPatch < ARRAYSIZE(CpData))
    {
        if (uiEncoding == CpData[iPatch].srcEncoding)
        { 
            for (i=0; i<nSize; i++)
            {
                if (*pStr > HIGHEST_ASCII)
                {
                    l = 0;
                    m = CpData[iPatch].nSize-1;
                    n = m / 2;
                    while (l <= m)
                    {
                        if (*pStr == CpData[iPatch].pszUch[n])
                            return CpData[iPatch].destEncoding;
                        else
                        {
                            if (*pStr < CpData[iPatch].pszUch[n])
                            {
                                m = n-1;
                            }
                            else
                            {
                                l = n+1;
                            }
                            n = (l+m)/2;
                        }
                    }
                }
                pStr++;
            }
        }
        iPatch++;
    }

    return uiEncoding;
}



#if 0

const unsigned char szKOIRU[] = {0xA4, 0xA6, 0xA7, 0xB4, 0xB6, 0xB7, 0xAD, 0xAE, 0xBD, 0xBE};

BOOL _IsKOI8RU(unsigned char *pStr, int nSize)
{
    int     i,j;
    BOOL    bRet = FALSE;

    // Skip parameter check since this is internal
    for (i=0; i<nSize; i++)
    {
        if (*pStr >= szKOIRU[0] && *pStr <= szKOIRU[ARRAYSIZE(szKOIRU)-1])
        {
            for (j=0; j<ARRAYSIZE(szKOIRU); j++)
            {
                if (*pStr == szKOIRU[j])
                {
                    bRet = TRUE;
                    break;
                    
                }
            }
        }

        if (bRet)
            break;

        pStr++;
    }

    return bRet;
}

#endif

HRESULT WINAPI _DetectInputCodepage(DWORD dwFlag, DWORD uiPrefWinCodepage, CHAR *pSrcStr, INT *pcSrcSize, DetectEncodingInfo *lpEncoding, INT *pnScores)
{

    HRESULT hr = S_OK;
    IStream *pstmTmp = NULL;
    BOOL bGuess = FALSE;
    BOOL bLCDetectSucceed = FALSE;
    int nBufSize = *pnScores;
    CHAR *_pSrcStr = pSrcStr;
    UINT nSrcSize;
    int  i;
    BOOL bMayBeAscii = FALSE;

    // Check parameters
    if (!pSrcStr || !(*pcSrcSize) || !lpEncoding || *pnScores == 0)
        return E_INVALIDARG;

    nSrcSize = *pcSrcSize;

    // Zero out return buffer
    ZeroMemory(lpEncoding, sizeof(DetectEncodingInfo)*(*pnScores));
    
    // HTML: take off HTML 'decoration'
    if (dwFlag & MLDETECTCP_HTML)
    {
        // Dup buffer for HTML parser
        if (NULL == (_pSrcStr = (char *)LocalAlloc(LPTR, nSrcSize)))
            return E_OUTOFMEMORY;        
        CopyMemory(_pSrcStr, pSrcStr, nSrcSize);
        RemoveHtmlTags (_pSrcStr, &nSrcSize);
    }

    // if blank page/file...
    if (!nSrcSize)
        return E_FAIL;

    if (nSrcSize >= MIN_TEXT_SIZE)
    {
        // Initialize LCDetect
        if (NULL == g_pLCDetect) 
        {
            EnterCriticalSection(&g_cs);
            if (NULL == g_pLCDetect)
            {
                LCDetect *pLC = new LCDetect ((HMODULE)g_hInst);
                if (pLC)
                {
                    if (pLC->LoadState() == NO_ERROR)
                        g_pLCDetect = pLC;
                    else
                    {
                        delete pLC;                    
                    }
                }
            }
            LeaveCriticalSection(&g_cs);
        }

        if (g_pLCDetect)
        {
            LCD_Detect(_pSrcStr, nSrcSize, (PLCDScore)lpEncoding, pnScores, NULL);
            if (*pnScores)
            {
                hr = S_OK;
                bLCDetectSucceed = TRUE;
            }
        }
    }

    if (!bLCDetectSucceed)
    {
        *pnScores = 0;
        hr = E_FAIL;
    }
    
    unsigned int uiCodepage = 0;        
    LARGE_INTEGER li = {0,0};
    ULARGE_INTEGER uli = {0,0};


    if (S_OK == CreateStreamOnHGlobal(NULL, TRUE, &pstmTmp))
    {
        ULONG cb = (ULONG) nSrcSize ;
        if (S_OK == pstmTmp->Write(_pSrcStr,cb,&cb))
        {
            uli.LowPart = cb ;
            if (S_OK != pstmTmp->SetSize(uli))
            {
                hr = E_OUTOFMEMORY;
                goto DETECT_DONE;
            }
        }
        else
        {
            goto DETECT_DONE;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
        goto DETECT_DONE;
    }
       
    pstmTmp->Seek(li,STREAM_SEEK_SET, NULL);

    switch (CceDetectInputCode(pstmTmp, grfDetectResolveAmbiguity|grfDetectUseCharMapping|grfDetectIgnoreEof, (EFam) 0, 0, &uiCodepage, &bGuess))
    {
        case cceSuccess:  
            if (*pnScores)
            {   
                // LCDETECT never detects wrong on Arabic and Russian, don't consider it as DBCS in this case
                // because MSEncode might misdetect Arabic and Russian as Japanese
                if (((lpEncoding[0].nLangID == LANG_ARABIC )|| (lpEncoding[0].nLangID == LANG_RUSSIAN)) &&
                    (lpEncoding[0].nConfidence >= MIN_CONFIDENCE_ARABIC) 
                    && (lpEncoding[0].nDocPercent >= MIN_DOCPERCENT) && !bGuess)
                    bGuess = TRUE;

                for (i=0;i<*pnScores;i++)
                {
                    if (lpEncoding[i].nCodePage == uiCodepage)
                    {
                        if ((i != 0) && !bGuess)
                        {
                            DetectEncodingInfo TmpEncoding;
                            // Re-arrange lanugage list for MSEncode result
                            MoveMemory(&TmpEncoding, &lpEncoding[0], sizeof(DetectEncodingInfo));
                            MoveMemory(&lpEncoding[0], &lpEncoding[i], sizeof(DetectEncodingInfo));
                            MoveMemory(&lpEncoding[i], &TmpEncoding, sizeof(DetectEncodingInfo));
                        }
                        // Boost confidence for double hits
                        lpEncoding[0].nDocPercent = 100;
                        if (lpEncoding[0].nConfidence < 100)
                            lpEncoding[0].nConfidence = 100;
                        break;
                    }
                }

                if (i == *pnScores)
                {
                    if (bGuess)
                    {
                        if (nBufSize > *pnScores)
                        {
                            lpEncoding[*pnScores].nCodePage = uiCodepage;
                            lpEncoding[*pnScores].nConfidence = MIN_CONFIDENCE;
                            lpEncoding[*pnScores].nDocPercent = MIN_DOCPERCENT;
                            lpEncoding[*pnScores].nLangID = -1;
                            (*pnScores)++;
                        }
                    }
                    else
                    {
                        if (nBufSize > *pnScores)
                        {
                            MoveMemory(lpEncoding+1, lpEncoding, sizeof(DetectEncodingInfo) * (*pnScores));
                            (*pnScores)++;
                        }
                        else
                        {
                            MoveMemory(lpEncoding+1, lpEncoding, sizeof(DetectEncodingInfo) * (*pnScores-1));
                        }
                        lpEncoding[0].nCodePage = uiCodepage;
                        lpEncoding[0].nConfidence = 100;
                        lpEncoding[0].nDocPercent = MIN_DOCPERCENT;
                        lpEncoding[0].nLangID = -1;
                    }
                }

            }
            else
            {
                lpEncoding[0].nCodePage = uiCodepage;
                if (bGuess) 
                    lpEncoding[0].nConfidence = MIN_CONFIDENCE;
                else
                    lpEncoding[0].nConfidence = 100;
                lpEncoding[0].nDocPercent = MIN_DOCPERCENT;
                lpEncoding[0].nLangID = -1;
                (*pnScores)++;
            }

            //hr = (g_pLCDetect || (nSrcSize < MIN_TEXT_SIZE)) ? S_OK : S_FALSE;
            hr = (!g_pLCDetect || (bGuess && !bLCDetectSucceed )) ? S_FALSE : S_OK;
            break;

        // Currently MSEncode doesn't provide any useful information in 'cceAmbiguousInput' case.
        // We may update our code here if Office team enhance MSEncode for ambiguous input later.
        case cceAmbiguousInput:
            break;

        case cceMayBeAscii:
            bMayBeAscii = TRUE;
            if (!(*pnScores))
            {
                lpEncoding[0].nCodePage = uiCodepage;
                lpEncoding[0].nConfidence = MIN_CONFIDENCE;
                lpEncoding[0].nDocPercent = -1;
                lpEncoding[0].nLangID = -1;
                (*pnScores)++;
            }
            else
            {
                for (i=0;i<*pnScores;i++)
                {
                    if (lpEncoding[i].nCodePage == uiCodepage)
                    {
                        break;
                    }
                }

                if (i == *pnScores)
                {
                    if(nBufSize > *pnScores) // Append MSEncode result to the language list
                    {
                       lpEncoding[i].nCodePage = uiCodepage;
                       lpEncoding[i].nConfidence = -1;
                       lpEncoding[i].nDocPercent = -1;
                       lpEncoding[i].nLangID = -1;
                       (*pnScores)++;
                    }
                }
            }
            hr = bLCDetectSucceed ? S_OK : S_FALSE;
            break;

        // MSEncode failed
        default:
            break;
    }




    for (i=0; i<*pnScores; i++)
    {
        switch (lpEncoding[i].nCodePage) {

            case 850:
                if ((*pnScores>1) && (lpEncoding[1].nConfidence >= MIN_CONFIDENCE))
                {
                    // Remove 850 from detection result if there is other detection results
                    (*pnScores)--;
                    if (i < *pnScores)
                        MoveMemory(&lpEncoding[i], &lpEncoding[i+1], (*pnScores-i)*sizeof(DetectEncodingInfo));
                    ZeroMemory(&lpEncoding[*pnScores], sizeof(DetectEncodingInfo));
                }
                else
                {
                    // Replace it with 1252 if it is the only result we get
                    lpEncoding[0].nCodePage = CP_1252; 
                    lpEncoding[0].nConfidence =
                    lpEncoding[0].nDocPercent = 100;
                    lpEncoding[0].nLangID = LANG_ENGLISH;
                }
                break;

            case CP_1250:
            case CP_KOI8R:
                lpEncoding[i].nCodePage = PatchCodePage(lpEncoding[i].nCodePage, (unsigned char *)_pSrcStr, nSrcSize);
                break;

            default:
                break;
        }
    }

    // If not a high confidence CP_1254 (Windows Turkish), 
    // we'll check if there're better detection results, and swap results if needed
    if ((lpEncoding[0].nCodePage == CP_1254) &&
        (*pnScores>1) && 
        ((lpEncoding[0].nDocPercent < 90) || (lpEncoding[1].nCodePage == CP_CHN_GB) || 
        (lpEncoding[1].nCodePage == CP_TWN) || (lpEncoding[1].nCodePage == CP_JPN_SJ) || (lpEncoding[1].nCodePage == CP_KOR_5601)))
    {
        MoveMemory(&lpEncoding[0], &lpEncoding[1], sizeof(DetectEncodingInfo)*(*pnScores-1));
        lpEncoding[*pnScores-1].nCodePage = CP_1254;
        lpEncoding[*pnScores-1].nLangID = LANG_TURKISH;
    }

    // 852 and 1258 text only have one sure detection result
    if (((lpEncoding[0].nCodePage == CP_852) || (lpEncoding[0].nCodePage == CP_1258)) &&
        (*pnScores>1) && 
        (lpEncoding[1].nConfidence >= MIN_CONFIDENCE))
    {
        DetectEncodingInfo tmpDetect = {0};
        MoveMemory(&tmpDetect, &lpEncoding[0], sizeof(DetectEncodingInfo));
        MoveMemory(&lpEncoding[0], &lpEncoding[1], sizeof(DetectEncodingInfo));
        MoveMemory(&lpEncoding[1], &tmpDetect, sizeof(DetectEncodingInfo));
    }

// Considering guessed value from MSENCODE is pretty accurate, we don't change S_OK to S_FALSE
#if 0
    if ((S_OK == hr) && !bLCDetectSucceed && bGuess) 
    {
        hr = S_FALSE;
    }
#endif

    if (uiPrefWinCodepage && *pnScores)
    {
        if (uiPrefWinCodepage == CP_AUTO && g_pCpMRU && !IS_ENCODED_ENCODING(lpEncoding[0].nCodePage))
        {
            UINT uiCpNum = CP_AUTO_MRU_NUM;
            CODEPAGE_MRU CpMRU[CP_AUTO_MRU_NUM];

            if (S_OK == g_pCpMRU->GetCpMRU(CpMRU, &uiCpNum))
            {
                for (i = 0; i<*pnScores; i++)
                {
                    for (UINT j = 0; j < uiCpNum; j++)
                    {
                        if (lpEncoding[i].nCodePage == CpMRU[j].dwEncoding)
                        {
                            uiPrefWinCodepage = CpMRU[j].dwEncoding;
                            break;
                        }
                    }
                    if (uiPrefWinCodepage != CP_AUTO)
                        break;
                }

                // If detection result is not in MRU
                if (uiPrefWinCodepage == CP_AUTO)
                {
                    // Don't take Unicode as perferred encoding if it is not in detection results for following reasons
                    // 1. Unicode is usually tagged with charset or Unicode BOM
                    // 2. Currently, we don't support Unicode detection in all detection engines
                    if (CpMRU[0].dwEncoding != CP_UCS_2 && CpMRU[0].dwEncoding != CP_UCS_2_BE)
                        uiPrefWinCodepage = CpMRU[0].dwEncoding;
                    else
                        goto PREFERCPCHECK_DONE;
                }

            }
            else
            {
                goto PREFERCPCHECK_DONE;
            }
        }

        for (i = 1; i<*pnScores; i++)
        {
            if (uiPrefWinCodepage == lpEncoding[i].nCodePage)
            {
                DetectEncodingInfo TmpEncoding;
                // Re-arrange lanugage list for prefered codepage
                TmpEncoding = lpEncoding[i];
                MoveMemory(&lpEncoding[1], &lpEncoding[0], sizeof(DetectEncodingInfo)*i);
                lpEncoding[0] = TmpEncoding;
                break;
            }
        }

        if ((uiPrefWinCodepage != lpEncoding[0].nCodePage) &&
            ((bMayBeAscii && (lpEncoding[0].nConfidence <= MIN_CONFIDENCE)) ||
            (hr != S_OK && nSrcSize >= MIN_TEXT_SIZE)))
        {
            lpEncoding[0].nCodePage = uiPrefWinCodepage;
            lpEncoding[0].nConfidence = -1;
            lpEncoding[0].nDocPercent = -1;
            lpEncoding[0].nLangID = -1;
            *pnScores = 1;
        }
    }

PREFERCPCHECK_DONE:

    // Assume LCDETECT won't misdetect 1252 for files over MIN_TEXT_SIZE
    // and MSENCODE can handle encoded text even they're below MIN_TEXT_SIZE
    if (((nSrcSize < MIN_TEXT_SIZE) && (bMayBeAscii || E_FAIL == hr)) ||
        (lpEncoding[0].nCodePage == CP_1252))
    {
        UINT j;
        for (j=0; j < nSrcSize; j++)
            if (*((LPBYTE)(_pSrcStr+j)) > HIGHEST_ASCII)
                break;
        if (j == nSrcSize)
        {
            if (lpEncoding[0].nCodePage == CP_1252)
            {
                lpEncoding[0].nCodePage = CP_20127;
            }
            else
            {
                *pnScores = 1;
                lpEncoding[0].nCodePage = CP_20127; 
                lpEncoding[0].nConfidence =
                lpEncoding[0].nDocPercent = 100;
                lpEncoding[0].nLangID = LANG_ENGLISH;
                hr = S_OK;
            }
        }
    }

    // UTF-8 doesn't really have distinctive signatures, 
    // if text amout is small, we won't return low confidence UTF-8 detection result.
    if (hr == S_FALSE && IS_ENCODED_ENCODING(lpEncoding[0].nCodePage) &&
        !((nSrcSize < MIN_TEXT_SIZE) && (lpEncoding[0].nCodePage == CP_UTF_8)))
        hr = S_OK;

DETECT_DONE:

    if ((dwFlag & MLDETECTCP_HTML) && _pSrcStr)
        LocalFree(_pSrcStr);

    if (pstmTmp)
    {
        pstmTmp->Release();
    }

    return hr ;
}

HRESULT WINAPI _DetectCodepageInIStream(DWORD dwFlag, DWORD uiPrefWinCodepage, IStream *pstmIn, DetectEncodingInfo *lpEncoding, INT *pnScores)
{

    HRESULT hr= S_OK, hrWarnings=S_OK;
    LARGE_INTEGER  libOrigin = { 0, 0 };
    ULARGE_INTEGER  ulPos = {0, 0};
    LPSTR lpstrIn = NULL ; 
    ULONG nlSrcSize ;
    INT nSrcUsed ;

    if (!pstmIn)
        return E_INVALIDARG ;

    // get size
    hr = pstmIn->Seek(libOrigin, STREAM_SEEK_END,&ulPos);
    if (S_OK != hr)
        hrWarnings = hr;

    if ( ulPos.LowPart == 0 && ulPos.HighPart == 0 )
        return E_INVALIDARG ;

    nlSrcSize = ulPos.LowPart ;

    // allocate a temp input buffer 
    if ( (lpstrIn = (LPSTR) LocalAlloc(LPTR, nlSrcSize )) == NULL )
    {
        hrWarnings = E_OUTOFMEMORY ;
        goto exit;
    }

    // reset the pointer
    hr = pstmIn->Seek(libOrigin, STREAM_SEEK_SET, NULL);
    if (S_OK != hr)
        hrWarnings = hr;

    hr = pstmIn->Read(lpstrIn, nlSrcSize, &nlSrcSize);
    if (S_OK != hr)
        hrWarnings = hr;

    nSrcUsed = (INT) nlSrcSize ;
    hr = _DetectInputCodepage(dwFlag, uiPrefWinCodepage, lpstrIn, &nSrcUsed, lpEncoding, pnScores);

exit :
    if (lpstrIn)
    {
        LocalFree(lpstrIn);
    }

    return (hr == S_OK) ? hrWarnings : hr;
}
