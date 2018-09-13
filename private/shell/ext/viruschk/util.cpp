#include "viruspch.h"
#include "util.h"
#include <wintrust.h>

//=--------------------------------------------------------------------------=
// miscellaneous [useful] numerical constants
//=--------------------------------------------------------------------------=
// the length of a guid once printed out with -'s, leading and trailing bracket,
// plus 1 for NULL
//
#define GUID_STR_LEN    40
#define WINTRUST "wintrust.dll"

// Free us from the tyranny of CRT by defining our own new and delete
#define CPP_FUNCTIONS
//#include <crtfree.h>

HANDLE g_hHeap;
extern HINSTANCE g_hinst;

typedef HRESULT (WINAPI *WINVERIFYTRUST) ( HWND hwnd, GUID *ActionID, LPVOID ActionData);

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

//=--------------------------------------------------------------------------=
// CheckTRust
//=--------------------------------------------------------------------------=
// Verify the code signing.
//

HRESULT CheckTrust( LPSTR szFilename )
{
   WINVERIFYTRUST pfnWinVerifyTrust;
   HINSTANCE hinst;

   hinst = LoadLibrary(WINTRUST);
   if(!hinst)
      return E_FAIL;

   pfnWinVerifyTrust = (WINVERIFYTRUST) GetProcAddress(hinst, "WinVerifyTrust");
   if(!pfnWinVerifyTrust)
   {
      FreeLibrary(hinst);
      return E_FAIL;
   }

   GUID PublishedSoftware = WIN_SPUB_ACTION_PUBLISHED_SOFTWARE;
   GUID * ActionGUID = &PublishedSoftware;

   GUID SubjectPeImage = WIN_TRUST_SUBJTYPE_PE_IMAGE;
   WIN_TRUST_ACTDATA_CONTEXT_WITH_SUBJECT ActionData;
   WIN_TRUST_SUBJECT_FILE Subject;
   HRESULT hr = S_OK;

   Subject.hFile = INVALID_HANDLE_VALUE;
   Subject.lpPath = OLESTRFROMANSI(szFilename);

   ActionData.SubjectType = &SubjectPeImage;

   ActionData.Subject = &Subject;
   ActionData.hClientToken = NULL;

   hr =  pfnWinVerifyTrust( NULL, ActionGUID, &ActionData);

   if (FAILED(hr) && (hr != TRUST_E_SUBJECT_NOT_TRUSTED)) {

       // trust system has failed without UI
       hr = E_FAIL;
   }
   FreeLibrary(hinst);
   return hr;
}


//=--------------------------------------------------------------------------=
// DeleteKeyAndSubKeys
//=--------------------------------------------------------------------------=
// delete's a key and all of it's subkeys.
//
// Parameters:
//    HKEY                - [in] delete the descendant specified
//    LPSTR               - [in] i'm the descendant specified
//
// Output:
//    BOOL                - TRUE OK, FALSE baaaad.
//
// Notes:
//    - I don't feel too bad about implementing this recursively, since the
//      depth isn't likely to get all the great.
//    - RegDeleteKey() works recursivly on Win9x, but not NT
//
BOOL DeleteKeyAndSubKeys
(
    HKEY    hkIn,
    LPSTR   pszSubKey
)
{
    HKEY  hk;
    TCHAR szTmp[MAX_PATH];
    DWORD dwTmpSize;
    long  l;
    BOOL  f;
    int   x;

    l = RegOpenKeyEx(hkIn, pszSubKey, 0, KEY_ALL_ACCESS, &hk);
    if (l != ERROR_SUCCESS) return FALSE;

    // loop through all subkeys, blowing them away.
    //
    f = TRUE;
    x = 0;
    while (f) {
        dwTmpSize = MAX_PATH;
        l = RegEnumKeyEx(hk, x, szTmp, &dwTmpSize, 0, NULL, NULL, NULL);
        if (l != ERROR_SUCCESS) break;
        f = DeleteKeyAndSubKeys(hk, szTmp);
        x++;
    }

    // there are no subkeys left, [or we'll just generate an error and return FALSE].
    // let's go blow this dude away.
    //
    RegCloseKey(hk);
    l = RegDeleteKey(hkIn, pszSubKey);

    return (l == ERROR_SUCCESS) ? TRUE : FALSE;
}

void CopyWideStr(LPWSTR pwszTarget, LPWSTR pwszSource)
{
   while(*pwszSource != 0)
   {
      *pwszTarget = *pwszSource;
      pwszSource++;
      pwszTarget++;
   }
   *pwszTarget = 0;
}

//=--------------------------------------------------------------------------=
// StringFromGuid
//=--------------------------------------------------------------------------=
// returns an ANSI string from a CLSID or GUID
//
// Parameters:
//    REFIID               - [in]  clsid to make string out of.
//    LPSTR                - [in]  buffer in which to place resultant GUID.
//
// Output:
//    int                  - number of chars written out.
//
// Notes:
//
int StringFromGuid
(
    const CLSID*   piid,
    LPTSTR   pszBuf
)
{
    return wsprintf(pszBuf, TEXT("{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"), piid->Data1,
            piid->Data2, piid->Data3, piid->Data4[0], piid->Data4[1], piid->Data4[2],
            piid->Data4[3], piid->Data4[4], piid->Data4[5], piid->Data4[6], piid->Data4[7]);
}


void ConvertAndSetText(HWND hwnd, UINT resID, LPWSTR pszSource)
{
   LPSTR psz;
   psz = MakeAnsiStrFromWide(pszSource);

   SetDlgItemTextA(hwnd, resID, psz);

   CoTaskMemFree(psz);
}


//=--------------------------------------------------------------------------=
// MakeWideFromAnsi
//=--------------------------------------------------------------------------=
// given a string, make a BSTR out of it.
//
// Parameters:
//    LPSTR         - [in]
//    BYTE          - [in]
//
// Output:
//    LPWSTR        - needs to be cast to final desired result
//
// Notes:
//
LPWSTR MakeWideStrFromAnsi(LPSTR psz)
{
    LPWSTR pwsz;
    int i;

    // arg checking.
    //
    if (!psz) return NULL;

    // compute the length of the required BSTR
    //
    if ((i =  MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0)) <= 0)
        return NULL;

    // allocate the widestr, +1 for terminating null
    //
    pwsz = (LPWSTR) CoTaskMemAlloc(i * sizeof(WCHAR));

    if (!pwsz) return NULL;
    MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, i);
    pwsz[i - 1] = 0;
    return pwsz;
}

LPSTR MakeAnsiStrFromWide(LPWSTR pwsz)
{
    LPSTR psz;
    int i;

    // arg checking.
    //
    if (!pwsz)
        return NULL;

    // compute the length of the required BSTR
    //
    if ((i = WideCharToMultiByte(CP_ACP, 0, pwsz, -1, NULL, 0, NULL, NULL)) <= 0)
        return NULL;

    // allocate the ansistr, +1 for terminating null
    //
    psz = (LPSTR) CoTaskMemAlloc(i * sizeof(CHAR));
    if (!psz) return NULL;

    WideCharToMultiByte(CP_ACP, 0, pwsz, -1, psz, i, NULL, NULL);
    psz[i - 1] = 0;
    return psz;
}

//-----------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------

static long crc_32_tab[] =
{
        0x00000000L, 0x77073096L, 0xEE0E612CL, 0x990951BAL, 0x076DC419L, 0x706AF48FL, 0xE963A535L, 0x9E6495A3L,
        0x0EDB8832L, 0x79DCB8A4L, 0xE0D5E91EL, 0x97D2D988L, 0x09B64C2BL, 0x7EB17CBDL, 0xE7B82D07L, 0x90BF1D91L,
        0x1DB71064L, 0x6AB020F2L, 0xF3B97148L, 0x84BE41DEL, 0x1ADAD47DL, 0x6DDDE4EBL, 0xF4D4B551L, 0x83D385C7L,
        0x136C9856L, 0x646BA8C0L, 0xFD62F97AL, 0x8A65C9ECL, 0x14015C4FL, 0x63066CD9L, 0xFA0F3D63L, 0x8D080DF5L,
        0x3B6E20C8L, 0x4C69105EL, 0xD56041E4L, 0xA2677172L, 0x3C03E4D1L, 0x4B04D447L, 0xD20D85FDL, 0xA50AB56BL,
        0x35B5A8FAL, 0x42B2986CL, 0xDBBBC9D6L, 0xACBCF940L, 0x32D86CE3L, 0x45DF5C75L, 0xDCD60DCFL, 0xABD13D59L,
        0x26D930ACL, 0x51DE003AL, 0xC8D75180L, 0xBFD06116L, 0x21B4F4B5L, 0x56B3C423L, 0xCFBA9599L, 0xB8BDA50FL,
        0x2802B89EL, 0x5F058808L, 0xC60CD9B2L, 0xB10BE924L, 0x2F6F7C87L, 0x58684C11L, 0xC1611DABL, 0xB6662D3DL,

        0x76DC4190L, 0x01DB7106L, 0x98D220BCL, 0xEFD5102AL, 0x71B18589L, 0x06B6B51FL, 0x9FBFE4A5L, 0xE8B8D433L,
        0x7807C9A2L, 0x0F00F934L, 0x9609A88EL, 0xE10E9818L, 0x7F6A0DBBL, 0x086D3D2DL, 0x91646C97L, 0xE6635C01L,
        0x6B6B51F4L, 0x1C6C6162L, 0x856530D8L, 0xF262004EL, 0x6C0695EDL, 0x1B01A57BL, 0x8208F4C1L, 0xF50FC457L,
        0x65B0D9C6L, 0x12B7E950L, 0x8BBEB8EAL, 0xFCB9887CL, 0x62DD1DDFL, 0x15DA2D49L, 0x8CD37CF3L, 0xFBD44C65L,
        0x4DB26158L, 0x3AB551CEL, 0xA3BC0074L, 0xD4BB30E2L, 0x4ADFA541L, 0x3DD895D7L, 0xA4D1C46DL, 0xD3D6F4FBL,
        0x4369E96AL, 0x346ED9FCL, 0xAD678846L, 0xDA60B8D0L, 0x44042D73L, 0x33031DE5L, 0xAA0A4C5FL, 0xDD0D7CC9L,
        0x5005713CL, 0x270241AAL, 0xBE0B1010L, 0xC90C2086L, 0x5768B525L, 0x206F85B3L, 0xB966D409L, 0xCE61E49FL,
        0x5EDEF90EL, 0x29D9C998L, 0xB0D09822L, 0xC7D7A8B4L, 0x59B33D17L, 0x2EB40D81L, 0xB7BD5C3BL, 0xC0BA6CADL,

        0xEDB88320L, 0x9ABFB3B6L, 0x03B6E20CL, 0x74B1D29AL, 0xEAD54739L, 0x9DD277AFL, 0x04DB2615L, 0x73DC1683L,
        0xE3630B12L, 0x94643B84L, 0x0D6D6A3EL, 0x7A6A5AA8L, 0xE40ECF0BL, 0x9309FF9DL, 0x0A00AE27L, 0x7D079EB1L,
        0xF00F9344L, 0x8708A3D2L, 0x1E01F268L, 0x6906C2FEL, 0xF762575DL, 0x806567CBL, 0x196C3671L, 0x6E6B06E7L,
        0xFED41B76L, 0x89D32BE0L, 0x10DA7A5AL, 0x67DD4ACCL, 0xF9B9DF6FL, 0x8EBEEFF9L, 0x17B7BE43L, 0x60B08ED5L,
        0xD6D6A3E8L, 0xA1D1937EL, 0x38D8C2C4L, 0x4FDFF252L, 0xD1BB67F1L, 0xA6BC5767L, 0x3FB506DDL, 0x48B2364BL,
        0xD80D2BDAL, 0xAF0A1B4CL, 0x36034AF6L, 0x41047A60L, 0xDF60EFC3L, 0xA867DF55L, 0x316E8EEFL, 0x4669BE79L,
        0xCB61B38CL, 0xBC66831AL, 0x256FD2A0L, 0x5268E236L, 0xCC0C7795L, 0xBB0B4703L, 0x220216B9L, 0x5505262FL,
        0xC5BA3BBEL, 0xB2BD0B28L, 0x2BB45A92L, 0x5CB36A04L, 0xC2D7FFA7L, 0xB5D0CF31L, 0x2CD99E8BL, 0x5BDEAE1DL,

        0x9B64C2B0L, 0xEC63F226L, 0x756AA39CL, 0x026D930AL, 0x9C0906A9L, 0xEB0E363FL, 0x72076785L, 0x05005713L,
        0x95BF4A82L, 0xE2B87A14L, 0x7BB12BAEL, 0x0CB61B38L, 0x92D28E9BL, 0xE5D5BE0DL, 0x7CDCEFB7L, 0x0BDBDF21L,
        0x86D3D2D4L, 0xF1D4E242L, 0x68DDB3F8L, 0x1FDA836EL, 0x81BE16CDL, 0xF6B9265BL, 0x6FB077E1L, 0x18B74777L,
        0x88085AE6L, 0xFF0F6A70L, 0x66063BCAL, 0x11010B5CL, 0x8F659EFFL, 0xF862AE69L, 0x616BFFD3L, 0x166CCF45L,
        0xA00AE278L, 0xD70DD2EEL, 0x4E048354L, 0x3903B3C2L, 0xA7672661L, 0xD06016F7L, 0x4969474DL, 0x3E6E77DBL,
        0xAED16A4AL, 0xD9D65ADCL, 0x40DF0B66L, 0x37D83BF0L, 0xA9BCAE53L, 0xDEBB9EC5L, 0x47B2CF7FL, 0x30B5FFE9L,
        0xBDBDF21CL, 0xCABAC28AL, 0x53B39330L, 0x24B4A3A6L, 0xBAD03605L, 0xCDD70693L, 0x54DE5729L, 0x23D967BFL,
        0xB3667A2EL, 0xC4614AB8L, 0x5D681B02L, 0x2A6F2B94L, 0xB40BBE37L, 0xC30C8EA1L, 0x5A05DF1BL, 0x2D02EF8DL
};


/* update running CRC calculation with contents of a buffer */

ULONG CRC32Compute(BYTE FAR *pb,unsigned cb,ULONG crc32)
{
    // ** Put CRC in form loop want it
    crc32 = (-1L - crc32);

    while (cb--)
    {
        crc32 = crc_32_tab[(BYTE)crc32 ^ *pb++] ^ ((crc32 >> 8) & 0x00FFFFFFL);
    }

    // ** Put CRC in form client wants it
    return (-1L - crc32);
}

//-----------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------

int ErrMsgBox( UINT id, LPCTSTR pszTitle, UINT  mbFlags)
{
    HWND hwndActive;
    TCHAR szMsg[MAX_PATH];

    hwndActive = GetActiveWindow();

    if ( !LoadSz( id, szMsg, ARRAYSIZE(szMsg) ) )
    {
        lstrcpy( szMsg, TEXT("Can not load strings.") );
    }
    id = MessageBox(hwndActive, szMsg, pszTitle, mbFlags | MB_ICONERROR );

    return id;
}

//-----------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------

HRESULT MakeSureKeyExist( HKEY hKey, LPSTR pSubKey )
{
    HRESULT hr = E_FAIL;
    HKEY    hSubKey;
    DWORD   dwDsposition;

    if ( !pSubKey || (*pSubKey == '\0') )
    {
        // ask for nothing and return nothing
        return S_OK;
    }

    if ( RegCreateKeyExA( hKey, pSubKey, 0, "", REG_OPTION_NON_VOLATILE, KEY_READ,
                          NULL, &hSubKey, &dwDsposition) == ERROR_SUCCESS )
    {
        hr = S_OK;
        RegCloseKey( hSubKey );
    }

    return hr;
}

int __cdecl _main() { return 0; }
