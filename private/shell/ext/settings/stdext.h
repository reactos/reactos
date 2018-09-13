#ifndef __STANDARD_SETTINGS_FOLDER_EXTENSION_H
#define __STANDARD_SETTINGS_FOLDER_EXTENSION_H

#include "enumtmpl.hxx"

typedef struct
{
    DWORD dwCookie;   
    INT idsTitle;
    INT idsCaption;
    INT idBitmap;
    RESTRICTIONS restPolicy;

} NEW_TOPIC_DATA, *PNEW_TOPIC_DATA;


class SettingsFolderExt : public ComObject, public ISettingsFolderExt
{
    public:
        SettingsFolderExt(VOID);
        ~SettingsFolderExt(VOID);

        //
        // IUnknown methods
        //
        STDMETHODIMP         
        QueryInterface(
            REFIID, 
            LPVOID *);

        STDMETHODIMP_(ULONG) 
        AddRef(
            VOID)
            {
                DebugMsg(DM_OLE | DM_NONEWLINE, TEXT("SettingsFolderExt"));
                return ComObject::AddRef();
            }

        STDMETHODIMP_(ULONG) 
        Release(
            VOID)
            {
                DebugMsg(DM_OLE | DM_NONEWLINE, TEXT("SettingsFolderExt"));
                return ComObject::Release();
            }

        //
        // ISettingsFolderExt methods.
        //
        STDMETHODIMP
        EnumTopics(
            PENUM_SETTINGS_TOPICS *);

    private:
        PointerList m_lstTopics;        // List of topic interface ptrs.

        INT CreateTopicObjects(PSETTINGS_FOLDER_TOPIC **pppITopics);
        INT RemoveRestrictedItems(NEW_TOPIC_DATA **pprgOut, NEW_TOPIC_DATA *prgIn, INT cIn);
};


class EnumSettingsTopics : public ComObject, public IEnumSettingsTopics
{
    private:
        EnumIFacePtrs<PSETTINGS_FOLDER_TOPIC> *m_pEnum;

    public:
        EnumSettingsTopics(PSETTINGS_FOLDER_TOPIC *ppITopics, UINT cItems);
        EnumSettingsTopics(const EnumSettingsTopics& rhs);
        ~EnumSettingsTopics(VOID);

        //
        // IUnknown methods
        //
        STDMETHODIMP         
        QueryInterface(
            REFIID riid, 
            LPVOID *ppvOut);

        STDMETHODIMP_(ULONG) 
        AddRef(
            VOID)
            {
                DebugMsg(DM_OLE | DM_NONEWLINE, TEXT("EnumSettingsTopics"));
                return ComObject::AddRef();
            }

        STDMETHODIMP_(ULONG) 
        Release(
            VOID)
            {
                DebugMsg(DM_OLE | DM_NONEWLINE, TEXT("EnumSettingsTopics"));
                return ComObject::Release();
            }

        //
        // IEnumXXXXX methods.
        //
        STDMETHODIMP
        Next(
            DWORD cRequested, 
            PSETTINGS_FOLDER_TOPIC *ppITopics, 
            LPDWORD pcCreated)
            {
                return m_pEnum->Next(cRequested, ppITopics, pcCreated);
            }
            
        STDMETHODIMP
        Skip(
            DWORD cSkip)
            {
                return m_pEnum->Skip(cSkip); 
            }

        STDMETHODIMP
        Reset(
            VOID)
            {
                return m_pEnum->Reset();
            }

        STDMETHODIMP
        Clone(
            IEnumSettingsTopics **ppEnum);
};


class SettingsFolderTopic : public ComObject, public ISettingsFolderTopic
{
    private:
        DWORD   m_dwCookie;         // Unique topic identifier for extension.
        LPTSTR  m_pszTitle;         // Displayed on the topic menu.
        LPTSTR  m_pszCaption;       // Displayed under the topic image.
        INT     m_idBitmap;         // Resource ID for image bitmap.
        CDIB    m_dib;              // Topic's image bitmap and palette.

        HRESULT GetStringMember(LPTSTR pszBuffer, UINT *pcchBufer, LPTSTR pszSource);
        BOOL ShellExec(SHELLEXECUTEINFO *psei);
        BOOL ShellExecFileAndArgs(LPCTSTR pszFile, LPCTSTR pszArgs = NULL);
        BOOL RunDll32(LPCTSTR pszArgs);
        BOOL RunCPL(LPCTSTR pszCPLName, LPCTSTR pszArgs = NULL);
        BOOL RunExe(LPCTSTR pszExeName, LPCTSTR pszArgs = NULL);
        BOOL RunHelpShortcut(LPCTSTR pszShortcut, LPCTSTR pszArgs = NULL);
        BOOL RunSpecialFolder(INT IDFolder);
        VOID RunTaskbarProperties(VOID);
        BOOL RunAddNewHardwareWizard(VOID);
        BOOL OpenControlPanel(VOID);

        SettingsFolderTopic(const SettingsFolderTopic& rhs);
        SettingsFolderTopic& operator = (const SettingsFolderTopic& rhs);

    public:

        SettingsFolderTopic::SettingsFolderTopic(const NEW_TOPIC_DATA *pTopicData);
        ~SettingsFolderTopic(VOID);

        //
        // IUnknown methods
        //
        STDMETHODIMP         
        QueryInterface(
            REFIID riid, 
            LPVOID *ppvOut);

        STDMETHODIMP_(ULONG) 
        AddRef(
            VOID)
            {
                DebugMsg(DM_OLE | DM_NONEWLINE, TEXT("SettingsFolderTopic"));
                return ComObject::AddRef();
            }

        STDMETHODIMP_(ULONG) 
        Release(
            VOID)
            {
                DebugMsg(DM_OLE | DM_NONEWLINE, TEXT("SettingsFolderTopic"));
                return ComObject::Release();
            }

        //
        // ISettingsFolderTopic methods.
        //
        STDMETHODIMP_(DWORD)
        GetCookie(
            VOID);

        STDMETHODIMP
        GetTitle(
            LPTSTR pszBuffer, 
            UINT *pcchBuffer);

        STDMETHODIMP
        GetTitle(
            LPCTSTR& pszBuffer);

        STDMETHODIMP
        GetCaption(
            LPTSTR pszCaption, 
            UINT *pcchBuffer);

        STDMETHODIMP
        GetCaption(
            LPCTSTR& pszCaption);

        STDMETHODIMP
        DrawImage(
            HDC hdc, 
            LPRECT prc);

        STDMETHODIMP
        TopicSelected(
            VOID);

        STDMETHODIMP
        PaletteChanged(
            HWND hwnd);

        STDMETHODIMP
        DisplayChanged(
            VOID);

};



#endif // __STANDARD_SETTINGS_FOLDER_EXTENSION_H
