#ifndef __WRKSPC_MISC_H__
#define __WRKSPC_MISC_H__


#ifdef _CPPRTTI
BOOL RttiTypesEqual(const type_info & t1, const type_info & t2);
#endif



#define FREE_STR(psz) if (psz) { free(psz); psz = NULL; }


#define STR_LOCAL_DLL   _T("tllocal.dll")
#define STR_PIPE_DLL    _T("tlpipe.dll")
#define STR_SER_DLL     _T("tlser.dll")

//
// Used by windbg & windbgrm
//  Serial transport layers
//

#define ROWS_SERIAL_TRANSPORT_LAYERS     5
#define COLS_SERIAL_TRANSPORT_LAYERS     4

extern LPSTR rgszSerialTransportLayers[ROWS_SERIAL_TRANSPORT_LAYERS][COLS_SERIAL_TRANSPORT_LAYERS];


class CGenInterface_WKSP;
class CIndiv_TL_RM_WKSP;
class CIndiv_TL_WKSP;



//
// Used to enumerate the values or sub-key of a registry.
// The list is NOT recursive. It only list the immediate
// values or sub-keys.
//
// If the values have been enumerated the 'm_hkey' 
// will always be NULL.
//
class CRegEntry
{
public:
    PTSTR   m_pszName; // Must be allocated with malloc NOT 'new'
    HKEY    m_hkey;

    CRegEntry()
    {
        m_pszName = NULL;
        m_hkey = NULL;
    }

    void
    CleanUp()
    {
        if (m_pszName) {
            free(m_pszName);
        }
        if (m_hkey) {
            RegCloseKey(m_hkey);
        }
    }
};




UINT
WKSP_MultiStrSize(PCSTR psz);

UINT
WKSP_StrSize(PCSTR psz);

int
CDECL 
WKSP_DisplayLastErrorMsgBox();

int 
CDECL 
WKSP_MsgBox(PSTR pszTitle, WORD wErrorFormat, ...);

int 
CDECL 
WKSP_VargsMsgBox(PSTR pszTitle, WORD wErrorFormat, va_list vargs);

int 
CDECL 
WKSP_CustMsgBox(UINT uStyle, PSTR pszTitle, WORD wErrorFormat, ...);

int 
CDECL 
WKSP_VargsCustMsgBox(UINT uStyle, PSTR pszTitle, WORD wErrorFormat, va_list vargs);

BOOL
WKSP_RegEnumerate(
    HKEY,
    TList< CRegEntry * > *,
    TList< CRegEntry * > *,
    BOOL
    );

BOOL
WKSP_RegKeyValueInfo(HKEY hkey,
    PDWORD pdwNumSubKeys, 
    PDWORD pdwNumValues,
    PDWORD pdwMaxKeyNameLen, 
    PDWORD pdwMaxValueNameLen
    );

BOOL 
WKSP_RegGetKeyName(
    HKEY hkey, 
    DWORD dwIndex, 
    PSTR pszName, 
    PDWORD pcbName
    );

BOOL 
WKSP_RegGetValueName(
    HKEY hkey, 
    DWORD dwIndex, 
    PSTR pszName, 
    PDWORD pcbName, 
    PDWORD pdwType
    );

void 
WKSP_RegDeleteValues(
    HKEY hkey
    );

void
WKSP_RegDeleteSubKeys(
    HKEY hkey
    );

void
WKSP_RegDeleteContents(
    HKEY hkeyParent
    );

BOOL
WKSP_RegDeleteKey(
    HKEY hkey,
    PSTR pszKeyName
    );

BOOL
WKSP_RegKeyExist(
    HKEY hkeyParent, 
    PCSTR pszKeyName
    );


HKEY
WKSP_RegKeyOpenCreate(
    HKEY    hkeyParent,
    PCSTR   pszKeyName,
    PBOOL   pbRegKeyCreated
    );

PSTR 
WKSP_DynaLoadStringWithArgs(
    HINSTANCE hInstance, 
    UINT uID, 
    ...
    );

PSTR 
WKSP_DynaLoadString(
    HINSTANCE hInstance, 
    UINT uID
    );

PTSTR
WKSP_FormatLastErrorMessage();

void
WKSP_AppendStrToMultiStr(
    PSTR & pszOriginal, 
    PSTR pszNewStrToAppend
    );




#endif
