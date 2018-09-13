#ifndef __SETTINGS_FOLDER_EXTENSIONS_H
#define __SETTINGS_FOLDER_EXTENSIONS_H

//
// These interfaces must be implemented by anyone extending the Shell Settings Folder
// as follows:
// 
// ISettingsFolderExt      - Implemented by any settings folder extension.
// ISettingsFolderTopic    - Implemented by extensions supplying new topics.
// IEnumSettingsTopics     - Implemented by extensions supplying new topics.
//

//-----------------------------------------------------------------------------
// ISettingsFolderTopic
//-----------------------------------------------------------------------------

#undef  INTERFACE
#define INTERFACE ISettingsFolderTopic

DECLARE_INTERFACE_(ISettingsFolderTopic, IUnknown)
{
    STDMETHOD_(DWORD,GetCookie)(VOID) PURE;
    STDMETHOD(GetTitle)(THIS_ LPTSTR pszBuffer, UINT *pcchBuffer) PURE;
    STDMETHOD(GetTitle)(THIS_ LPCTSTR& pszBuffer) PURE;
    STDMETHOD(GetCaption)(THIS_ LPTSTR pszCaption, UINT *pcchBuffer) PURE;
    STDMETHOD(GetCaption)(THIS_ LPCTSTR& pszCaption) PURE;
    STDMETHOD(DrawImage)(THIS_ HDC hdc, LPRECT prc) PURE;
    STDMETHOD(TopicSelected)(THIS_ VOID) PURE;
    STDMETHOD(PaletteChanged)(THIS_ HWND hwnd) PURE;
    STDMETHOD(DisplayChanged)(THIS_ VOID) PURE;
};

typedef ISettingsFolderTopic SETTINGS_FOLDER_TOPIC, *PSETTINGS_FOLDER_TOPIC;

//
// Forward decl required by ISettingsFolderCategory decl.
//
typedef struct IEnumSettingsTopics IEnumSettingsTopics;
typedef struct IEnumSettingsTopics ENUM_SETTINGS_TOPICS, *PENUM_SETTINGS_TOPICS;


//-----------------------------------------------------------------------------
// IEnumSettingsTopics
//-----------------------------------------------------------------------------

#undef  INTERFACE
#define INTERFACE IEnumSettingsTopics

DECLARE_INTERFACE_(IEnumSettingsTopics, IUnknown)
{
    STDMETHOD(Next)(THIS_ DWORD, PSETTINGS_FOLDER_TOPIC *, LPDWORD) PURE;
    STDMETHOD(Skip)(THIS_ DWORD) PURE;
    STDMETHOD(Reset)(THIS) PURE;
    STDMETHOD(Clone)(THIS_ IEnumSettingsTopics **) PURE;
};


//-----------------------------------------------------------------------------
// ISettingsFolderExt
//-----------------------------------------------------------------------------

#undef  INTERFACE
#define INTERFACE ISettingsFolderExt

DECLARE_INTERFACE_(ISettingsFolderExt, IUnknown)
{
    STDMETHOD(EnumTopics)(THIS_ PENUM_SETTINGS_TOPICS *) PURE;
};

typedef ISettingsFolderExt SETTINGS_FOLDER_EXT, *PSETTINGS_FOLDER_EXT;



#endif // __SETTINGS_FOLDER_EXTENSIONS_H
