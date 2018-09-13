#ifndef _CPDETECT_H_
#define _CPDETECT_H_

#define MAX_CONFIDENCE     100  
#define MIN_CONFIDENCE     86
#define MIN_DOCPERCENT     60
#define MIN_TEXT_SIZE      200
#define HIGHEST_ASCII      127
#define MIN_CONFIDENCE_ARABIC   60

#define DEFAULT_CPMRU_INIT_HITS 100
#define DEFAULT_CPMRU_FACTOR    20
#define CP_AUTO_MRU_NUM         6
#define DEFAULT_CPMRU_NUM       10
#define MAX_CPMRU_NUM           20
#define MAX_ENTITY_LENTH        10

#define IsNoise(c) ((unsigned)(c) <= 0x20 && (c) != 0 && (c) != 0x1B)
#define IS_ENCODED_ENCODING(cp) ((cp) == CP_ISO_2022_JP || (cp) == CP_CHN_HZ || (cp) == CP_UTF_7 || (cp) == CP_UTF_8 || (cp) == CP_ISO_2022_KR)

typedef struct _CODEPAGE_MRU 
{
    DWORD   dwEncoding;
    DWORD   dwHistoryHits;
} CODEPAGE_MRU, *PCODEPAGE_MRU;

typedef struct tagCpPatch
{
    UINT            srcEncoding;
    UINT            destEncoding;
    UINT            nSize;
    unsigned char   *pszUch;
} CPPATCH;

// Dump everything under
// HKCU\\Software\\Microsoft\\Internet Explorer\\International
// !BUGBUG, is it safe to do so?
#define REGSTR_PATH_CPMRU TSZMICROSOFTPATH TEXT("\\Internet Explorer\\International\\CpMRU")
#define REG_KEY_CPMRU                   TEXT("Cache")
#define REG_KEY_CPMRU_ENABLE            TEXT("Enable")
#define REG_KEY_CPMRU_NUM               TEXT("Size")
#define REG_KEY_CPMRU_INIT_HITS         TEXT("InitHits")
#define REG_KEY_CPMRU_PERCENTAGE_FACTOR TEXT("Factor")


// CCpMRU
class CCpMRU
{
    PCODEPAGE_MRU   _pCpMRU;

    DWORD           dwCpMRUNum;
    DWORD           dwCpMRUInitHits;
    DWORD           dwCpMRUFactor;

public:
    DWORD           dwCpMRUEnable;
    BOOL            bCpUpdated;

    CCpMRU::CCpMRU(void)
    {
        // No update at initial time
        bCpUpdated = FALSE;
        _pCpMRU = NULL;
    }

    ~CCpMRU(void);
    HRESULT Init(void);
    HRESULT GetCpMRU(PCODEPAGE_MRU pCpMRU, UINT *puiCpNum);
    void UpdateCPMRU(DWORD dwEncoding);

};
void RemoveHtmlTags (LPSTR pIn, UINT *pnBytes);
UINT PatchCodePage(UINT uiEncoding, unsigned char *pStr, int nSize);
extern class CCpMRU * g_pCpMRU;

#endif  // _CPDETECT_H_