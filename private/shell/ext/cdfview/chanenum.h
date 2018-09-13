//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// chanenum.h 
//
//   The definition of the channel enumerator.
//
//   History:
//
//       8/6/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Check for previous includes of this file.
//

#ifndef _CHANENUM_H_

#define _CHANENUM_H_

/*
//
// Defines
//

#define TSTR_CHANNEL_KEY   TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Channels")

//
// Helper functions.
//

HKEY    Reg_GetChannelKey(void);
HRESULT Reg_WriteChannel(LPCTSTR pszPath,LPCTSTR pszURL);
HRESULT Reg_RemoveChannel(LPCTSTR pszPath);
*/

//
// Structures.
//

typedef struct _tagSTACKENTRY
{
    LPTSTR          pszPath;
    _tagSTACKENTRY* pNext;
} STACKENTRY;

typedef enum _tagINIVALUE
{
    INI_GUID = 0,
    INI_URL  = 1
} INIVALUE;        


//
// Class definition for the channel enumerator class.
//


class CChannelEnum : public IEnumChannels
{

//
// Methods
//

public:

    // Constructor
    CChannelEnum(DWORD dwEnumFlags, LPCWSTR pszURL);

    // IUnknown
    STDMETHODIMP         QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IEnumIDList
    STDMETHODIMP Next(ULONG celt, CHANNELENUMINFO* rgInfo, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset(void);
    STDMETHODIMP Clone(IEnumChannels **ppenum);
 
private:

    // Destructor
    ~CChannelEnum(void);

    // Helper methods
    inline BOOL DirectoryStack_IsEmpty(void);
    void        DirectoryStack_FreeEntry(STACKENTRY* pse);
    void        DirectoryStack_FreeStack(void);
    STACKENTRY* DirectoryStack_Pop(void);
    BOOL        DirectoryStack_Push(LPCTSTR pszPath);
    BOOL        DirectoryStack_InitFromFlags(DWORD dwEnumFlags);
    BOOL        DirectoryStack_PushSubdirectories(LPCTSTR pszPath);

    BOOL        FindNextChannel(CHANNELENUMINFO* pInfo);
    BOOL        ReadChannelInfo(LPCTSTR pszPath, CHANNELENUMINFO* pInfo);
    BOOL        ContainsChannelDesktopIni(LPCTSTR pszPath);
    BOOL        URLMatchesIni(LPCTSTR pszPath, LPCTSTR pszURL);
    BOOL        ReadFromIni(LPCTSTR pszPath, LPTSTR pszOut, int cch, INIVALUE iv);
    LPOLESTR    OleAllocString(LPCTSTR psz);

    SUBSCRIPTIONSTATE GetSubscriptionState(LPCTSTR pszURL);

//
// Member variables.
//

private:

    ULONG        m_cRef;
    STACKENTRY*  m_pseDirectoryStack;
    LPTSTR       m_pszURL;
    DWORD        m_dwEnumFlags;
};


#endif // _CHANENUM_H_