//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       optnobj.cxx
//
//  Contents:   Contains the implementation of the User Option settings for
//              CDoc, including OM access.
//
//----------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_FPRINT_HXX_
#define X_FPRINT_HXX_
#include "fprint.hxx"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifndef X_UNISID_H_
#define X_UNISID_H_
#include <unisid.h>
#endif

#ifdef WIN16
#define MIN_JAVA_MEMORY 8192L+1L        // must be over 8Meg to run Java
extern DWORD WINAPI DetectPhysicalMem();

#ifndef X_INETREG_H_
#define X_INETREG_H_
#include <inetreg.h>
#endif

#endif

#ifdef UNIX
// Unix uses this global variable to memorize users selected Font size View->Font
// and passes this to each new CDoc, if it's been changed.
int g_SelectedFontSize = -1;
#endif

MtDefine(CodePageSettings, CDoc, "CDoc::_pCodePageSettings")
MtDefine(CDocReadContextMenuExtFromRegistry_pCME, Locals, "CDoc::ReadContextMenuExtFromRegistry pCME")

//+---------------------------------------------------------------------------
//
//  Member:     OPTIONSETTINGS::Init, public
//
//  Synopsis:   Initializes data that should only be initialized once.
//
//  Arguments:  [psz] -- String to initialize achKeyPath to. Cannot be NULL.
//
//----------------------------------------------------------------------------

void
OPTIONSETTINGS::Init(
    TCHAR *psz,
    BOOL fUseCodePageBasedFontLinkingArg )
{
    _tcscpy(achKeyPath, psz);
    fSettingsRead = FALSE;
    fUseCodePageBasedFontLinking = !!fUseCodePageBasedFontLinkingArg;
    sBaselineFontDefault = BASELINEFONTDEFAULT;

    memset(alatmProporitionalFonts, -1, sizeof(alatmProporitionalFonts));
    memset(alatmFixedPitchFonts,    -1, sizeof(alatmFixedPitchFonts));
}

//+---------------------------------------------------------------------------
//
//  Member:     CODEPAGESETTINGS::SetDefaults, public
//
//  Synopsis:   Sets the default values for the CODEPAGESETTINGS struct.
//
//----------------------------------------------------------------------------

void
CODEPAGESETTINGS::SetDefaults(
    UINT  uiFamilyCodePageDefault,
    SHORT sOptionSettingsBaselineFontDefault)
{
    fSettingsRead = FALSE;
    bCharSet = DEFAULT_CHARSET;
    sBaselineFontDefault = sOptionSettingsBaselineFontDefault;
    uiFamilyCodePage = uiFamilyCodePageDefault;
    latmFixedFontFace = -1;
    latmPropFontFace  = -1;
}


//+---------------------------------------------------------------------------
//
//  Member:     OPTIONSETTINGS::SetDefaults, public
//
//  Synopsis:   Sets the default values of the struct
//
//----------------------------------------------------------------------------

void
OPTIONSETTINGS::SetDefaults( )
{
    colorBack            = OLECOLOR_FROM_SYSCOLOR(COLOR_WINDOW);
    colorText            = OLECOLOR_FROM_SYSCOLOR(COLOR_WINDOWTEXT);
    colorAnchor          = RGB(0, 0, 0xFF);
    colorAnchorVisited   = RGB(0x80, 0, 0x80);
    colorAnchorHovered   = RGB(0, 0, 0x80);

    fUseDlgColors        = TRUE;
    fExpandAltText       = FALSE;
    fShowImages          = TRUE;
#ifndef NO_AVI
    fShowVideos          = TRUE;
#endif // ndef NO_AVI
    fPlaySounds          = TRUE;
    fPlayAnimations      = TRUE;
    fUseStylesheets      = TRUE;
    fSmoothScrolling     = TRUE;
    fShowImagePlaceholder = FALSE;
    fShowFriendlyUrl     = FALSE;
    fSmartDithering      = TRUE;
    fAlwaysUseMyColors   = FALSE;
    fAlwaysUseMyFontSize = FALSE;
    fAlwaysUseMyFontFace = FALSE;
    fUseMyStylesheet     = FALSE;
    fUseHoverColor       = FALSE;
    fDisableScriptDebugger = TRUE;
    fMoveSystemCaret     = FALSE;
    fHaveAcceptLanguage  = FALSE;
    fCpAutoDetect        = FALSE;
    fAllowCutCopyPaste   = FALSE;

    nAnchorUnderline     = ANCHORUNDERLINE_YES;

    // HACKHACK (johnv) For japanese, default to Autodetect.  This hack
    // can be removed once setup does its job properly.
    codepageDefault      = (g_cpDefault == 932) ? CP_AUTO_JP : g_cpDefault;

    dwMaxStatements      = CDoc::RUNAWAY_SCRIPT_STATEMENTCOUNT;

    dwRtfConverterf      = RTFCONVF_ENABLED;    // enabled, but not for dbcs

    dwMiscFlags          = 0;

    dwNoChangingWallpaper = 0;

    // Free the context menu extension array
    {
        CONTEXTMENUEXT **   ppCME;
        int                 n;

        for (ppCME = aryContextMenuExts,
             n = aryContextMenuExts.Size();
             n;
             n--, ppCME++)
        {
            delete *ppCME;
        }

        aryContextMenuExts.DeleteAll();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::EnsureOptionSettings, public
//
//  Synopsis:   Ensures that this CDoc is pointing to a valid OPTIONSETTINGS
//              object. Should only be called by ReadOptionSettingsFromRegistry.
//
//  Arguments:  (none)
//
//----------------------------------------------------------------------------

HRESULT
CDoc::EnsureOptionSettings()
{
    HRESULT             hr = S_OK;
    int                 c;
    OPTIONSETTINGS   *  pOS;
    OPTIONSETTINGS   ** ppOS;
    TCHAR             * pstr=NULL;
    BOOL                fUseCodePageBasedFontLinking;

    static TCHAR pszDefaultKey[] = _T("Software\\Microsoft\\Internet Explorer");
    static TCHAR pszOE4Key[] = _T("Software\\Microsoft\\Outlook Express\\Trident");

    if (_pOptionSettings)
        return S_OK;

    if (_pHostUIHandler)
    {
        _pHostUIHandler->GetOptionKeyPath(&pstr, 0);
    }

    if (!pstr)
    {
        pstr = pszDefaultKey;
    }

    _fOE4 = (0 == StrCmpC(pstr, pszOE4Key));

#ifndef UNIX
    // On Unix, we create new option setting for each new CDoc, because all IEwindows
    // run on the same thread.

    for (c = TLS(optionSettingsInfo.pcache).Size(),
         ppOS = TLS(optionSettingsInfo.pcache);
         c > 0;
         c--, ppOS++)
    {
        if (!StrCmpC((*ppOS)->achKeyPath, pstr))
        {
            _pOptionSettings = *ppOS;
            goto Cleanup;
        }
    }
#endif
    // We only make it here if we didn't find an existing entry.

    // OPTIONSETTINGS has one character already in it, which accounts for a
    // NULL terminator.

    pOS = new ( _tcslen(pstr) * sizeof(TCHAR) ) OPTIONSETTINGS;
    if (!pOS)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // New clients, such as OE5, will set the UIHost flag.  For compatibility with OE4, however,
    // we need to employ some trickery to get the old font regkey values.

    fUseCodePageBasedFontLinking = 0 != (_dwFlagsHostInfo & DOCHOSTUIFLAG_CODEPAGELINKEDFONTS);
    if (!fUseCodePageBasedFontLinking)
    {
        fUseCodePageBasedFontLinking = _fOE4;
    }

    MemSetName((pOS, "OPTIONSETTINGS object, index %d",
                    TLS(optionSettingsInfo.pcache).Size()));

    pOS->Init(pstr, fUseCodePageBasedFontLinking);
    pOS->SetDefaults( );

    hr = TLS(optionSettingsInfo.pcache).Append(pOS);
    if (hr)
        goto Cleanup;

    _pOptionSettings = pOS;

    ClearDefaultCharFormat();

Cleanup:
    if (pstr != pszDefaultKey)
        CoTaskMemFree(pstr);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::EnsureCodepageSettings, public
//
//  Synopsis:   Ensures that this CDoc is pointing to a valid CODEPAGESETTINGS
//              object. Should only be called by ReadCodepageSettingsFromRegistry.
//
//  Arguments:  uiFamilyCodePage - the family to check for
//
//----------------------------------------------------------------------------

HRESULT
CDoc::EnsureCodepageSettings( UINT uiFamilyCodePage )
{
    HRESULT            hr = S_OK;
    int                n;
    CODEPAGESETTINGS** ppCS, *pCS;

    // Make sure we have a valid _pOptionSettings object
    Assert( _pOptionSettings );

    // The first step is to look up the entry in the codepage cache

    for (n = _pOptionSettings->aryCodepageSettingsCache.Size(),
                ppCS = _pOptionSettings->aryCodepageSettingsCache;
                n > 0;
                n--, ppCS++)
    {
        if ( (*ppCS)->uiFamilyCodePage == uiFamilyCodePage )
        {
            _pCodepageSettings = *ppCS;
            goto Cleanup;
        }
    }

    // We're out of luck, need to read in the codepage setting from the registry

    pCS = (CODEPAGESETTINGS *) MemAlloc(Mt(CodePageSettings), sizeof(CODEPAGESETTINGS) );
    if (!pCS)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    MemSetName((pCS, "CODEPAGESETTINGS"));

    pCS->Init( );
    pCS->SetDefaults( uiFamilyCodePage, _pOptionSettings->sBaselineFontDefault );

    hr = _pOptionSettings->aryCodepageSettingsCache.Append(pCS);
    if (hr)
        goto Cleanup;

    _pCodepageSettings = pCS;

    ClearDefaultCharFormat();

Cleanup:
    RRETURN(hr);
}

#ifndef NO_RTF
//+-------------------------------------------------------------------------
//
//  Method:     CDoc::RtfConverterEnabled
//
//  Synopsis:   TRUE if this rtf conversions are enabled, FALSE otherwise.
//
//--------------------------------------------------------------------------

BOOL
CDoc::RtfConverterEnabled()
{
    DWORD  dwConvf = _pOptionSettings->dwRtfConverterf; // for shorthand
    CPINFO cpinfo;

    //
    // Rtf conversions can be disabled on an sbcs-dbcs basis, or even
    // completely.  See the RTFCONVF flags in formkrnl.hxx.
    //
    return (dwConvf & RTFCONVF_ENABLED) &&
        ((dwConvf & RTFCONVF_DBCSENABLED) ||
        (GetCPInfo(g_cpDefault, &cpinfo) && cpinfo.MaxCharSize == 1));
}
#endif // ndef NO_RTF

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::UpdateFromRegistry, public
//
//  Synopsis:   Load configuration information from the registry. Needs to be
//              called after we have our client site so we can do a
//              QueryService.
//
//  Arguments:  flags - REGUPDATE_REFRESH - read even if we find a cache entry
//                    - REGUPDATE_KEEPLOCALSTATE - true if we do not want to
//                                                 override local state
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

struct SAVEDSETTINGS
{
    OLE_COLOR colorBack;
    OLE_COLOR colorText;
    OLE_COLOR colorAnchor;
    OLE_COLOR colorAnchorVisited;
    OLE_COLOR colorAnchorHovered;
    LONG      latmFixedFontFace;
    LONG      latmPropFontFace;
    LONG      nAnchorUnderline;
    SHORT     sBaselineFontDefault;
    BYTE      fAlwaysUseMyColors;
    BYTE      fAlwaysUseMyFontSize;
    BYTE      fAlwaysUseMyFontFace;
    BYTE      fUseMyStylesheet;
    BYTE      bCharSet;
};

static void
SaveSettings(OPTIONSETTINGS *pOptionSettings,
             CODEPAGESETTINGS *pCodepageSettings,
             SAVEDSETTINGS *pSavedSettings)
{
    memset(pSavedSettings, 0, sizeof(SAVEDSETTINGS));
    pSavedSettings->colorBack             = pOptionSettings->colorBack;
    pSavedSettings->colorText             = pOptionSettings->colorText;
    pSavedSettings->colorAnchor           = pOptionSettings->colorAnchor;
    pSavedSettings->colorAnchorVisited    = pOptionSettings->colorAnchorVisited;
    pSavedSettings->colorAnchorHovered    = pOptionSettings->colorAnchorHovered;
    pSavedSettings->nAnchorUnderline      = pOptionSettings->nAnchorUnderline;
    pSavedSettings->fAlwaysUseMyColors    = pOptionSettings->fAlwaysUseMyColors;
    pSavedSettings->fAlwaysUseMyFontSize  = pOptionSettings->fAlwaysUseMyFontSize;
    pSavedSettings->fAlwaysUseMyFontFace  = pOptionSettings->fAlwaysUseMyFontFace;
    pSavedSettings->fUseMyStylesheet      = pOptionSettings->fUseMyStylesheet;
    pSavedSettings->bCharSet              = pCodepageSettings->bCharSet;
    pSavedSettings->latmFixedFontFace     = pCodepageSettings->latmFixedFontFace;
    pSavedSettings->latmPropFontFace      = pCodepageSettings->latmPropFontFace;
    pSavedSettings->sBaselineFontDefault  = pCodepageSettings->sBaselineFontDefault;
}

HRESULT
CDoc::UpdateFromRegistry(DWORD dwFlags, BOOL *pfNeedLayout)
{
    CODEPAGE        codepage;
    BOOL            fFirstTime = !_pOptionSettings;
    SAVEDSETTINGS   savedSettings1;
    SAVEDSETTINGS   savedSettings2;

    // Use the cached values unless we are forced to re-read
    if (!fFirstTime && !(dwFlags & REGUPDATE_REFRESH))
        return S_OK;

    if (pfNeedLayout)
    {
        if (!_pOptionSettings)
        {
            // We assume that in this context, the caller is interested
            // only in whether or not we want to relayout, and not to
            // force us to read in registry settings.
            *pfNeedLayout = FALSE;
            return S_OK;
        }
        SaveSettings(_pOptionSettings, _pCodepageSettings, &savedSettings1);
    }

    // First read in the standard option settings
    IGNORE_HR( ReadOptionSettingsFromRegistry( dwFlags ) );

    if (g_fTerminalServer)
        _pOptionSettings->fSmoothScrolling = FALSE;

    // For HTML applications, override registry settings for multimedia components.
    // We don't want (for example) images in an HTA to fail to appear due to
    // custom internet options settings...

    if (_fTrustedDoc)
    {
        _pOptionSettings->fShowImages      = TRUE;
#ifndef NO_AVI
        _pOptionSettings->fShowVideos      = TRUE;
#endif // ndef NO_AVI
        _pOptionSettings->fPlaySounds      = TRUE;
        _pOptionSettings->fPlayAnimations  = TRUE;
        _pOptionSettings->fSmartDithering  = TRUE;
    }

    _dwMiscFlags() = _pOptionSettings->dwMiscFlags;

    // If we are getting settings for the first time, use the default
    //  codepage.  Otherwise, read based on our current setting.
    codepage = _pCodepageSettings ? _codepage :
                                    _pOptionSettings->codepageDefault;
    IGNORE_HR( ReadCodepageSettingsFromRegistry( codepage, WindowsCodePageFromCodePage( codepage ), dwFlags ) );

    if (_pCodepageSettings)
    {
        // Set the baseline font only the first time we read from the registry
#ifdef UNIX
        if (g_SelectedFontSize != -1) // Copy the previous selected font size.
            _sBaselineFont = _pCodepageSettings->sBaselineFontDefault = g_SelectedFontSize;
    else
#endif
        _sBaselineFont = _pCodepageSettings->sBaselineFontDefault;
    }

    // Print documents obtain their font size when they were enqueued which is
    // cached in the printinfobag.
    if (IsPrintDoc())
    {
        _sBaselineFont = (SHORT)DYNCAST(CPrintDoc, GetRootDoc())->_PrintInfoBag.iFontScaling;
    }

#ifdef WIN16
    // BUGWIN16: need to implement GetAcceptLanguages.
    _pOptionSettings->fHaveAcceptLanguage = FALSE;
#else
    // Request the accept language header info from shlwapi.
    {
        TCHAR achLang[256];
        DWORD cchLang = ARRAY_SIZE(achLang);
        _pOptionSettings->fHaveAcceptLanguage = (GetAcceptLanguages(achLang, &cchLang) == S_OK)
                                                && (_pOptionSettings->cstrLang.Set(achLang, cchLang) == S_OK);
    }
#endif

    if (pfNeedLayout)
    {
        SaveSettings(_pOptionSettings, _pCodepageSettings, &savedSettings2);
        *pfNeedLayout = !!memcmp(&savedSettings1, &savedSettings2, sizeof(SAVEDSETTINGS));
    }

    _view.RefreshSettings();

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::ReadSettingsFromRegistry, public
//
//  Synopsis:   A generic function to read from the registry.
//
//  Arguments:  pAryKeys - a pointer to an array of keys
//              iKeyCount - the number of keys in this array
//              pBase - a pointer to which offsets in pAryKeys are relative.
//              dwFlags - see UpdateWithRegistry
//              fSettingsRead - True if the settings were already read
//              pUserData - may hold some user data.  for now if we are looking
//                          for a RPI_CPKEY, this is assumed to point to a codepage enum.
//
//----------------------------------------------------------------------------

#define MAX_REG_VALUE_LENGTH   50

enum RKI_TYPE
{
    RKI_KEY,
    RKI_CPKEY,
    RKI_BOOL,
    RKI_COLOR,
    RKI_FONT,
    RKI_SIZE,
    RKI_CP,
    RKI_BYTEBOOL,
    RKI_STRING,
    RKI_ANCHORUNDERLINE,
    RKI_DWORD,
    RKI_TYPE_Last_Enum
};

struct REGKEYINFORMATION
{
    TCHAR *   pszName;            // Name of the value or key
    BYTE      rkiType;            // Type of entry
    size_t    cbOffset;           // Offset of member to store data in
    size_t    cbOffsetCondition;  // Offset of member that must be false (true only
                                  //  for type RKI_STRING) to use this value. If
                                  //  it's true (false for type RKI_STRING)
                                  //  this value is left as its default value. Assumed
                                  //  to be data that's sizeof(BYTE). If 0 then
                                  //  no condition is used.
    BOOL      fLocalState;        // True if this is local state which may not always get
                                  //  updated when read again from the registry.
};

HRESULT
ReadSettingsFromRegistry(
    TCHAR * pchKeyPath,
    const REGKEYINFORMATION* pAryKeys, int iKeyCount,
    void* pBase, DWORD dwFlags, BOOL fSettingsRead,
    void* pUserData )
{
    LONG                lRet;
    HKEY                hKeyRoot = NULL;
    HKEY                hKeySub  = NULL;
    int                 i;
    DWORD               dwType;
    DWORD               dwSize;

    //
    // IEUNIX
    // Note we access dwDataBuf as a byte array through bDataBuf
    // but DWORD align it by declaring it a DWORD array (dwDataBuf).
    //
    DWORD               dwDataBuf[pdlUrlLen / sizeof(DWORD) +1 ];
    BYTE              * bDataBuf = (BYTE*) dwDataBuf;

    TCHAR               achCustomKey[64], * pch;
    BYTE              * pbData;
    BYTE                bCondition;
    BOOL                fUpdateLocalState;
    const REGKEYINFORMATION * prki;
    LONG              * pl;

    Assert( pBase );

    // Do not re-read unless explictly asked to do so.
    if( fSettingsRead && !(dwFlags & REGUPDATE_REFRESH) )
        return S_OK;

    // Always read local settings at least once
    fUpdateLocalState = !fSettingsRead || (dwFlags & REGUPDATE_OVERWRITELOCALSTATE );

    // Get a registry key handle

    lRet = RegOpenKeyEx(HKEY_CURRENT_USER, pchKeyPath, 0, KEY_READ, &hKeyRoot);
    if( lRet != ERROR_SUCCESS )
        return S_FALSE;

    for (i = 0; i < iKeyCount; i++)
    {
        prki = &pAryKeys[i];
        // Do not update local state unless asked to do so.
        if( !fUpdateLocalState && prki->fLocalState )
            continue;
        switch (prki->rkiType)
        {
        case RKI_KEY:
        case RKI_CPKEY:
            if (!prki->pszName)
            {
                hKeySub = hKeyRoot;
            }
            else
            {
                if (hKeySub && (hKeySub != hKeyRoot))
                {
                    RegCloseKey(hKeySub);
                    hKeySub = NULL;
                }

                if (prki->rkiType == RKI_CPKEY)
                {
                    // N.B. (johnv) It is assumed here that pUserData points
                    // to a codepage if we are looking for codepage settings.
                    // RKI_CPKEY entries are per family codepage.

                    Assert(pUserData);

                    Format( 0, achCustomKey, ARRAY_SIZE( achCustomKey ),
                            prki->pszName,
                            *(DWORD*)pUserData );

                    pch = achCustomKey;
                }
                else
                {
                    pch = prki->pszName;
                }

                lRet = RegOpenKeyEx(hKeyRoot,
                                    pch,
                                    0,
                                    KEY_READ,
                                    &hKeySub);

                if (lRet != ERROR_SUCCESS)
                {
                    // We couldn't get this key, skip it.
                    i++;
                    while (i < iKeyCount &&
                           pAryKeys[i].rkiType != RKI_KEY &&
                           pAryKeys[i].rkiType != RKI_CPKEY )
                    {
                        i++;
                    }

                    i--; // Account for the fact that continue will increment i again.
                    hKeySub = NULL;
                    continue;
                }
            }
            break;

        case RKI_SIZE:
            Assert(hKeySub);

            dwSize = MAX_REG_VALUE_LENGTH;

            lRet = RegQueryValueEx(hKeySub,
                                   prki->pszName,
                                   0,
                                   &dwType,
                                   bDataBuf,
                                   &dwSize);

            if (lRet == ERROR_SUCCESS)
            {
                short s;

                if (dwType == REG_BINARY)
                {
                    s = (short)*(BYTE *)bDataBuf;
                }
                else if (dwType == REG_DWORD)
                {
                    s = (short)*(DWORD *)bDataBuf;
                }
                else
                {
                    break;
                }

                *(short*)((BYTE *)pBase + prki->cbOffset) =
                    min( short(BASELINEFONTMAX), max( short(BASELINEFONTMIN), s ) );
            }
            break;

        case RKI_BOOL:
            Assert(hKeySub);

            dwSize = MAX_REG_VALUE_LENGTH;
            lRet = RegQueryValueEx(hKeySub,
                                   prki->pszName,
                                   0,
                                   &dwType,
                                   bDataBuf,
                                   &dwSize);

            if (lRet == ERROR_SUCCESS)
            {
                pbData = (BYTE*)((BYTE *)pBase + prki->cbOffset);

                if (dwType == REG_DWORD)
                {
                    *pbData = (*(DWORD*)bDataBuf != 0);
                }
                else if (dwType == REG_SZ)
                {
                    TCHAR ch = *(TCHAR *)bDataBuf;

                    if (ch == _T('1') ||
                        ch == _T('y') ||
                        ch == _T('Y'))
                    {
                        *pbData = TRUE;
                    }
                    else
                    {
                        *pbData = FALSE;
                    }
                } else if (dwType == REG_BINARY)
                {
                    *pbData = (*(BYTE*)bDataBuf != 0);
                }

                // Can't convert other types. Just leave it the default.
            }
            break;

        case RKI_FONT:
            Assert(hKeySub);
            dwSize = LF_FACESIZE * sizeof(TCHAR);

            lRet = RegQueryValueEx(hKeySub,
                                   prki->pszName,
                                   0,
                                   &dwType,
                                   bDataBuf,
                                   &dwSize);

            pl = (LONG *)((BYTE *)pBase + prki->cbOffset);

            if (lRet == ERROR_SUCCESS && dwType == REG_SZ && *(TCHAR *)bDataBuf)
            {
                *pl = fc().GetAtomFromFaceName((TCHAR *)bDataBuf);
            }
            break;

        case RKI_COLOR:
            Assert(hKeySub);

            dwSize = MAX_REG_VALUE_LENGTH;

            bCondition = *(BYTE*)((BYTE *)pBase + prki->cbOffsetCondition);
            if (prki->cbOffsetCondition && bCondition)
            {
                //
                // The appropriate flag is set that says we should not pay
                // attention to this value, so just skip it and keep the
                // default.
                //
                break;
            }

            lRet = RegQueryValueEx(hKeySub,
                                   prki->pszName,
                                   0,
                                   &dwType,
                                   bDataBuf,
                                   &dwSize);

            if (lRet == ERROR_SUCCESS)
            {
                if (dwType == REG_SZ)
                {
                    //
                    // Crack the registry format for colors which is a string
                    // of the form "R,G,B" where R, G, and B are decimal
                    // values for Red, Green, and Blue, respectively.
                    //

                    DWORD   colors[3];
                    TCHAR * pchStart  = (TCHAR*)bDataBuf;
                    TCHAR * pchEnd;
                    int     i;

                    pbData = (BYTE*)((BYTE *)pBase + prki->cbOffset);

                    for (i = 0; i < 3; i++)
                    {
                        colors[i] = wcstol(pchStart, &pchEnd, 10);

                        if (*pchEnd == _T('\0') && i != 2)
                            break;

                        pchStart  = pchEnd + 1;
                    }

                    if (i != 3) // We didn't get all the colors. Abort.
                        break;

                    *(COLORREF*)pbData = RGB(colors[0], colors[1], colors[2]);
                }
                // Can't convert other types. Just leave it the default.
            }
            break;

        case RKI_CP:
            Assert(hKeySub);

            dwSize = sizeof(DWORD);

            lRet = RegQueryValueEx(hKeySub,
                                   prki->pszName,
                                   0,
                                   &dwType,
                                   bDataBuf,
                                   &dwSize);

            if (lRet == ERROR_SUCCESS && dwType == REG_BINARY)
            {
                *(CODEPAGE*)((BYTE *)pBase + prki->cbOffset) = *(CODEPAGE *)bDataBuf;
            }
            break;

        case RKI_BYTEBOOL:
            Assert(hKeySub);

            dwSize = sizeof(DWORD);

            lRet = RegQueryValueEx(hKeySub,
                                   prki->pszName,
                                   0,
                                   &dwType,
                                   bDataBuf,
                                   &dwSize);

            if (lRet == ERROR_SUCCESS && dwType == REG_BINARY)
            {
                *(BYTE*)((BYTE *)pBase + prki->cbOffset) = (BYTE) !! (*((DWORD*)bDataBuf));
            }
            break;

        case RKI_DWORD:
            Assert(hKeySub);

            dwSize = sizeof(DWORD);

            lRet = RegQueryValueEx(hKeySub,
                                   prki->pszName,
                                   0,
                                   &dwType,
                                   bDataBuf,
                                   &dwSize);

            if (lRet == ERROR_SUCCESS && (dwType == REG_BINARY || dwType == REG_DWORD))
            {
                *(DWORD*)((BYTE *)pBase + prki->cbOffset) = *(DWORD*)bDataBuf;
            }
            break;

        case RKI_ANCHORUNDERLINE:
            Assert(hKeySub);

            dwSize = MAX_REG_VALUE_LENGTH;

            lRet = RegQueryValueEx(hKeySub,
                                   prki->pszName,
                                   0,
                                   &dwType,
                                   bDataBuf,
                                   &dwSize);

            if (lRet == ERROR_SUCCESS && dwType == REG_SZ)
            {
                int nAnchorunderline = ANCHORUNDERLINE_YES;

                LPTSTR pchBuffer = (TCHAR *)bDataBuf;
                Assert (pchBuffer != NULL);

                if (pchBuffer)
                {
                    if (_tcsicmp(pchBuffer, _T("yes")) == 0)
                        nAnchorunderline = ANCHORUNDERLINE_YES;
                    else if (_tcsicmp(pchBuffer, _T("no")) == 0)
                        nAnchorunderline = ANCHORUNDERLINE_NO;
                    else if (_tcsicmp(pchBuffer, _T("hover")) == 0)
                        nAnchorunderline = ANCHORUNDERLINE_HOVER;
                }

                *(int*)((BYTE *)pBase + prki->cbOffset) = nAnchorunderline;

            }
            break;

        case RKI_STRING:
            Assert(hKeySub);

            dwSize = 0;
            bCondition = *(BYTE*)((BYTE *)pBase + prki->cbOffsetCondition);

            if ((prki->cbOffsetCondition && bCondition) || !prki->cbOffsetCondition)
            {
                //
                // The appropriate flag is set that says we should pay
                // attention to this value or the flag is 0, then do it
                // else skip and keep the default.
                //

                // get the size of string
                lRet = RegQueryValueEx(hKeySub,
                                       prki->pszName,
                                       0,
                                       &dwType,
                                       NULL,
                                       &dwSize);

                if (lRet == ERROR_SUCCESS)
                {
                    lRet = RegQueryValueEx(hKeySub,
                                           prki->pszName,
                                           0,
                                           &dwType,
                                           bDataBuf,
                                           &dwSize);

                    if (lRet == ERROR_SUCCESS && dwType == REG_SZ)
                    {
                        ((CStr *)((BYTE *)pBase + prki->cbOffset))->Set((LPCTSTR)bDataBuf);
                    }
                }
            }

            break;

        default:
            AssertSz(FALSE, "Unrecognized RKI Type");
            break;
        }
    }

    if (hKeySub && (hKeySub != hKeyRoot))
        RegCloseKey(hKeySub);

    RegCloseKey( hKeyRoot );

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::ReadCodepageSettingsFromRegistry, public
//
//  Synopsis:   Read settings for a particular codepage from the registry.
//
//  Arguments:  cp - the codepage to read
//              dwFlags - See UpdateFromRegistry
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CDoc::ReadCodepageSettingsFromRegistry(
    CODEPAGE cp,
    UINT uiFamilyCodePage,
    DWORD dwFlags )
{
    HRESULT    hr = S_OK;
    SCRIPT_ID  sid = RegistryAppropriateSidFromSid(DefaultSidForCodePage(uiFamilyCodePage));

    Assert( uiFamilyCodePage != CP_UNDEFINED && uiFamilyCodePage != CP_ACP );

    hr = THR( EnsureCodepageSettings( uiFamilyCodePage ) );
    if( hr )
        goto Cleanup;

    _pOptionSettings->ReadCodepageSettingsFromRegistry( _pCodepageSettings, dwFlags, sid );

    // Remember if we were autodetected
    _fCodePageWasAutoDetect = _codepage == CP_AUTO_JP;

    // Set the codepage on the doc to the actual codepage requested
    _codepage = cp;
    _codepageFamily = uiFamilyCodePage;

Cleanup:
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     OPTIONSETTINGS::ReadCodePageSettingsFromRegistry
//
//  Synopsis:   Read the fixed and proportional
//  
//----------------------------------------------------------------------------

void
   OPTIONSETTINGS::ReadCodepageSettingsFromRegistry(
    CODEPAGESETTINGS * pCS,
    DWORD dwFlags,
    SCRIPT_ID sid )
{
    static const REGKEYINFORMATION aScriptBasedFontKeys[] =
    {
        { _T("International\\Scripts\\<0d>"),      RKI_CPKEY, (long)0 },
        { _T("IEFontSize"),                        RKI_SIZE, offsetof(CODEPAGESETTINGS, sBaselineFontDefault),    0, FALSE },
        { _T("IEPropFontName"),                    RKI_FONT, offsetof(CODEPAGESETTINGS, latmPropFontFace),  1, FALSE },
        { _T("IEFixedFontName"),                   RKI_FONT, offsetof(CODEPAGESETTINGS, latmFixedFontFace), 0, FALSE },
    };

    static const REGKEYINFORMATION aCodePageBasedFontKeys[] =
    {
        { _T("International\\<0d>"),               RKI_CPKEY, (long)0 },
        { _T("IEFontSize"),                        RKI_SIZE, offsetof(CODEPAGESETTINGS, sBaselineFontDefault),    0, FALSE },
        { _T("IEPropFontName"),                    RKI_FONT, offsetof(CODEPAGESETTINGS, latmPropFontFace),  1, FALSE },
        { _T("IEFixedFontName"),                   RKI_FONT, offsetof(CODEPAGESETTINGS, latmFixedFontFace), 0, FALSE },
    };

    // NB (cthrash) (CP_UCS_2,sidLatin) is for Unicode documents.  So for OE, pick the Unicode font.
    // (CP_UCS_2,!sidAsciiLatin), on the other hand, is for codepageless fontlinking.  In OE, we obviously
	// can't use codepage-based fontlinking; use instead IE5 fontlinking.
    
    fUseCodePageBasedFontLinking &=    sid == sidAsciiLatin
                                    || sid == sidLatin
                                    || DefaultCharSetFromScriptAndCharset(sid, DEFAULT_CHARSET) != DEFAULT_CHARSET;
    
    DWORD dwArg = fUseCodePageBasedFontLinking ? DWORD(pCS->uiFamilyCodePage) : DWORD(sid);

    IGNORE_HR( ReadSettingsFromRegistry( achKeyPath,
                                         fUseCodePageBasedFontLinking ? aCodePageBasedFontKeys : aScriptBasedFontKeys,
                                         ARRAY_SIZE(aCodePageBasedFontKeys),
                                         pCS,
                                         dwFlags,
                                         pCS->fSettingsRead,
                                         (void *)&dwArg ) );

    // Determine the appropriate GDI charset

    pCS->bCharSet = DefaultCharSetFromScriptAndCodePage( sid, pCS->uiFamilyCodePage );

    // Do a little fixup on the fonts if not present.  Note that we avoid
    // doing this in CODEPAGESETTINGS::SetDefault as this could be expensive
    // and often unnecessary.

    if (   pCS->latmFixedFontFace == -1
           || pCS->latmPropFontFace == -1)
    {
        SCRIPTINFO si;
        HRESULT hr;

        hr = THR( MlangGetDefaultFont( sid, &si ) );

        if (pCS->latmFixedFontFace == -1)
        {
            pCS->latmFixedFontFace = OK(hr)
                                     ? fc().GetAtomFromFaceName(si.wszFixedWidthFont)
                                     : 0; // 'System'
        }

        if (pCS->latmPropFontFace == -1)
        {
            pCS->latmPropFontFace = OK(hr)
                                    ? fc().GetAtomFromFaceName(si.wszProportionalFont)
                                    : 0; // 'System'
        }
    }

    pCS->fSettingsRead = TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::ReadContextMenuExtFromRegistry, public
//
//  Synopsis:   Load information about context menu extensions
//              from the registry
//
//  Arguments:  dwFlags - See UpdateFromRegistry.
//
//  Returns:    HRESULT, S_FALSE says nothing was there to read
//
//----------------------------------------------------------------------------
HRESULT
CDoc::ReadContextMenuExtFromRegistry( DWORD dwFlags /* = 0 */)
{
    HRESULT             hr = S_OK;
    CONTEXTMENUEXT *    pCME = NULL;
    int                 nExtMax, nExtCur;
    HKEY                hKeyRoot = NULL;
    HKEY                hKeyMenuExt = NULL;
    HKEY                hKeySub = NULL;
    TCHAR               achSubName[MAX_PATH + 1];
    long                lRegRet;
    DWORD               dwType;
    DWORD               dwSize;
    BYTE                bDataBuf[pdlUrlLen];

    // Do not re-read unless explictly asked to do so.
    if( _pOptionSettings->fSettingsRead && !(dwFlags & REGUPDATE_REFRESH) )
        return S_OK;

    //
    //  Open up our root key
    //
    lRegRet = RegOpenKeyEx(HKEY_CURRENT_USER, _pOptionSettings->achKeyPath,
                        0, KEY_READ, &hKeyRoot);
    if( lRegRet != ERROR_SUCCESS )
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    //
    // Get the menu extensions sub key
    //
    lRegRet = RegOpenKeyEx(hKeyRoot, _T("MenuExt"),
                           0, KEY_READ, &hKeyMenuExt);
    if( lRegRet != ERROR_SUCCESS )
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    //
    // Make sure our array is big enough
    //
    nExtMax = IDM_MENUEXT_LAST__ - IDM_MENUEXT_FIRST__;
    hr = _pOptionSettings->aryContextMenuExts.EnsureSize(nExtMax);
    if(hr)
        goto Cleanup;

    //
    // add an entry for each sub key
    //

    for(nExtCur = 0; nExtCur < nExtMax; nExtCur++)
    {
        lRegRet = RegEnumKey(hKeyMenuExt, nExtCur, achSubName, MAX_PATH+1);
        if(lRegRet != ERROR_SUCCESS)
        {
            break;
        }

        // Open the sub key
        lRegRet = RegOpenKeyEx(hKeyMenuExt, achSubName, 0, KEY_READ, &hKeySub);
        if(lRegRet != ERROR_SUCCESS)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        // we have a key so create an extension object
        pCME = new(Mt(CDocReadContextMenuExtFromRegistry_pCME)) CONTEXTMENUEXT;
        if(!pCME)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        // set the menu name
        pCME->cstrMenuValue.Set(achSubName);

        // read the default value
        dwSize = pdlUrlLen;
        bDataBuf[0] = 0;
        lRegRet = RegQueryValueEx(hKeySub, NULL,
                                  0, &dwType, bDataBuf, &dwSize);
        if(lRegRet == ERROR_SUCCESS && dwType == REG_SZ)
        {
            pCME->cstrActionUrl.Set((LPCTSTR)bDataBuf);
        }

        // look for flags
        dwSize = sizeof(DWORD);
        lRegRet = RegQueryValueEx(hKeySub, _T("Flags"),
                                  0, &dwType, bDataBuf, &dwSize);
        if(lRegRet == ERROR_SUCCESS &&
           (dwType == REG_DWORD || dwType == REG_BINARY) )
        {
            pCME->dwFlags = *((DWORD*)bDataBuf);
        }

        // look for contexts
        dwSize = sizeof(DWORD);
        lRegRet = RegQueryValueEx(hKeySub, _T("Contexts"),
                                  NULL, &dwType, bDataBuf, &dwSize);
        if(lRegRet == ERROR_SUCCESS &&
           (dwType == REG_DWORD || dwType == REG_BINARY) )
        {
            pCME->dwContexts = *((DWORD*)bDataBuf);
        }

        // check to make sure we have a good extension
        if(pCME->cstrMenuValue.Length() != 0 &&
           pCME->cstrActionUrl.Length() != 0)
        {
            _pOptionSettings->aryContextMenuExts.Append(pCME);
        }
        else
        {
            delete pCME;
        }

        pCME = NULL;
        RegCloseKey(hKeySub);
        hKeySub = NULL;
    }

    // delete the context menu so that these changes will be relfected
    if(IsMenu(TLS(hMenuCtx_Browse)))
    {
        DestroyMenu(TLS(hMenuCtx_Browse));
        TLS(hMenuCtx_Browse) = NULL;
    }

Cleanup:
    delete pCME;

    if(hKeySub)
        RegCloseKey(hKeySub);

    if(hKeyMenuExt)
        RegCloseKey(hKeyMenuExt);

    if(hKeyRoot)
        RegCloseKey(hKeyRoot);

    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::ReadOptionSettingsFromRegistry, public
//
//  Synopsis:   Read the general option settings from the registry.
//
//  Arguments:  dwFlags - See UpdateFromRegistry.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CDoc::ReadOptionSettingsFromRegistry( DWORD dwFlags )
{
    HRESULT hr = S_OK;

    //
    // keys in main registry location
    //

    static const REGKEYINFORMATION aOptionKeys[] =
    {
        { NULL,                         RKI_KEY, 0 },
        { _T("Show_FullURL"),           RKI_BOOL,  offsetof(OPTIONSETTINGS, fShowFriendlyUrl), 0, FALSE },
        { _T("SmartDithering"),         RKI_BOOL,  offsetof(OPTIONSETTINGS, fSmartDithering),  0, FALSE },
        { _T("RtfConverterFlags"),      RKI_DWORD, offsetof(OPTIONSETTINGS, dwRtfConverterf),  0, FALSE },

        { _T("Main"),                   RKI_KEY, 0 },
        { _T("Use_DlgBox_Colors"),      RKI_BOOL, offsetof(OPTIONSETTINGS, fUseDlgColors),    0, FALSE },
        { _T("Anchor Underline"),       RKI_ANCHORUNDERLINE, offsetof(OPTIONSETTINGS, nAnchorUnderline), 0, FALSE },
        { _T("Expand Alt Text"),        RKI_BOOL, offsetof(OPTIONSETTINGS, fExpandAltText),   0, FALSE },
        { _T("Display Inline Images"),  RKI_BOOL, offsetof(OPTIONSETTINGS, fShowImages),      0, FALSE },
#ifndef NO_AVI
        { _T("Display Inline Videos"),  RKI_BOOL, offsetof(OPTIONSETTINGS, fShowVideos),      0, FALSE },
#endif // ndef NO_AVI
        { _T("Play_Background_Sounds"), RKI_BOOL, offsetof(OPTIONSETTINGS, fPlaySounds),      0, FALSE },
        { _T("Play_Animations"),        RKI_BOOL, offsetof(OPTIONSETTINGS, fPlayAnimations),  0, FALSE },
        { _T("Use Stylesheets"),        RKI_BOOL, offsetof(OPTIONSETTINGS, fUseStylesheets),  0, FALSE },
        { _T("SmoothScroll"),           RKI_BOOL, offsetof(OPTIONSETTINGS, fSmoothScrolling), 0, FALSE },
        { _T("Show image placeholders"),   RKI_BOOL, offsetof(OPTIONSETTINGS, fShowImagePlaceholder), 0, FALSE },
        { _T("Disable Script Debugger"),    RKI_BOOL, offsetof(OPTIONSETTINGS, fDisableScriptDebugger), 0, TRUE },
        { _T("Move System Caret"),      RKI_BOOL, offsetof(OPTIONSETTINGS, fMoveSystemCaret), 0, FALSE },

        { _T("International"),          RKI_KEY,   0 },
        { _T("Default_CodePage"),       RKI_CP,   offsetof(OPTIONSETTINGS, codepageDefault),  0, TRUE },
#ifndef UNIX // Unix doesn't support AutoDetect
        { _T("AutoDetect"),    RKI_BOOL, offsetof(OPTIONSETTINGS, fCpAutoDetect),    0, FALSE },
#endif

        { _T("International\\Scripts"), RKI_KEY,   0 },
        { _T("Default_IEFontSize"),     RKI_SIZE,   offsetof(OPTIONSETTINGS, sBaselineFontDefault),  0, FALSE },

        { _T("Settings"),               RKI_KEY, 0 },
        { _T("Background Color"),       RKI_COLOR, offsetof(OPTIONSETTINGS, colorBack),          offsetof(OPTIONSETTINGS, fUseDlgColors), FALSE },
        { _T("Text Color"),             RKI_COLOR, offsetof(OPTIONSETTINGS, colorText),          offsetof(OPTIONSETTINGS, fUseDlgColors), FALSE },
        { _T("Anchor Color"),           RKI_COLOR, offsetof(OPTIONSETTINGS, colorAnchor),        0, FALSE },
        { _T("Anchor Color Visited"),   RKI_COLOR, offsetof(OPTIONSETTINGS, colorAnchorVisited), 0, FALSE },
        { _T("Anchor Color Hover"),     RKI_COLOR, offsetof(OPTIONSETTINGS, colorAnchorHovered), 0, FALSE },
        { _T("Always Use My Colors"),   RKI_BOOL, offsetof(OPTIONSETTINGS, fAlwaysUseMyColors),  0, FALSE },
        { _T("Always Use My Font Size"),   RKI_BOOL, offsetof(OPTIONSETTINGS, fAlwaysUseMyFontSize),  0, FALSE },
        { _T("Always Use My Font Face"),   RKI_BOOL, offsetof(OPTIONSETTINGS, fAlwaysUseMyFontFace),  0, FALSE },
        { _T("Use Anchor Hover Color"),    RKI_BOOL, offsetof(OPTIONSETTINGS, fUseHoverColor),        0, FALSE },
        { _T("MiscFlags"),              RKI_DWORD,   offsetof(OPTIONSETTINGS, dwMiscFlags),   0, TRUE },

        { _T("Styles"),                 RKI_KEY, 0 },
        { _T("Use My Stylesheet"),      RKI_BOOL, offsetof(OPTIONSETTINGS, fUseMyStylesheet),  0, FALSE },
        { _T("User Stylesheet"),        RKI_STRING, offsetof(OPTIONSETTINGS, cstrUserStylesheet),  offsetof(OPTIONSETTINGS, fUseMyStylesheet), FALSE },
        { _T("MaxScriptStatements"),    RKI_DWORD,   offsetof(OPTIONSETTINGS, dwMaxStatements),   0, FALSE },
    };

    //
    // keys in windows location
    //

    static TCHAR achWindowsSettingsPath [] = _T("Software\\Microsoft\\Windows\\CurrentVersion");

    static const REGKEYINFORMATION aOptionKeys2[] =
    {
        { _T("Policies\\ActiveDesktop"),RKI_KEY, 0 },
        { _T("NoChangingWallpaper"),    RKI_DWORD,  offsetof(OPTIONSETTINGS, dwNoChangingWallpaper), 0, FALSE },
        { _T("Policies"), RKI_KEY, 0 },
        { _T("Allow Programmatic Cut_Copy_Paste"), RKI_BOOL, offsetof(OPTIONSETTINGS, fAllowCutCopyPaste), 0, FALSE },
    };

    hr = THR(EnsureOptionSettings());
    if (hr)
        goto Cleanup;

    // Make sure we get back the default windows colors, etc.
    if(dwFlags & REGUPDATE_REFRESH)
        _pOptionSettings->SetDefaults();
    IGNORE_HR( ReadSettingsFromRegistry(
        _pOptionSettings->achKeyPath,
        aOptionKeys,
        ARRAY_SIZE(aOptionKeys),
        _pOptionSettings,
        dwFlags,
        _pOptionSettings->fSettingsRead,
        (void *)&_pOptionSettings->codepageDefault ) );

    IGNORE_HR( ReadSettingsFromRegistry(
        achWindowsSettingsPath,
        aOptionKeys2,
        ARRAY_SIZE(aOptionKeys2),
        _pOptionSettings, dwFlags,
        _pOptionSettings->fSettingsRead, NULL ) );

    // Look at the registry for context menu extensions
    IGNORE_HR( ReadContextMenuExtFromRegistry( dwFlags ) );

    _pOptionSettings->fSettingsRead = TRUE;

#ifdef WIN16_NEVER
    // BUGWIN16: the Flag in OPTIONSETTINGS just went away ??!! in beta2 Trident code !!
    // Make sure that we have enough memory to run Java
    if (_pOptionSettings->fRunJava && (DetectPhysicalMem() < MIN_JAVA_MEMORY))
    {
        _pOptionSettings->fRunJava = FALSE;

        // Update the registry to disable java
        HKEY    hkey;
        DWORD dw;

        if (RegCreateKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_INTERNETSETTINGS,
                0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey, &dw) == ERROR_SUCCESS)
        {
            dw = FALSE;
            RegSetValueEx(hkey, REGSTR_VAL_SECURITYJAVA, 0,
                REGSTR_VAL_SECURITYJAVA_TYPE, (LPBYTE)&dw, sizeof(dw));
        }
    }
#endif


Cleanup:
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Function:   DeinitOptionSettings
//
//  Synopsis:   Frees up memory stored in the TLS(optionSettingsInfo) struct.
//
//  Notes:      Called by the DllThreadDetach code.
//
//----------------------------------------------------------------------------

void
DeinitOptionSettings( THREADSTATE* pts )
{
    int c, n;
    OPTIONSETTINGS ** ppOS;
    CODEPAGESETTINGS ** ppCS;
    CONTEXTMENUEXT **   ppCME;

    // Free all entries in the options cache
    for (c = pts->optionSettingsInfo.pcache.Size(),
         ppOS = pts->optionSettingsInfo.pcache;
         c;
         c--, ppOS++)
    {
        (*ppOS)->cstrUserStylesheet.Free();

        // Free all entries in the codepage settings cache
        for (ppCS = (*ppOS)->aryCodepageSettingsCache,
            n = (*ppOS)->aryCodepageSettingsCache.Size();
            n;
            n--, ppCS++ )
        {
            MemFree(*ppCS);
        }

        // Free the context menu extension array
        for (ppCME = (*ppOS)->aryContextMenuExts,
             n = (*ppOS)->aryContextMenuExts.Size();
             n;
             n--, ppCME++)
        {
            delete *ppCME;
        }

        delete (*ppOS);
    }

    pts->optionSettingsInfo.pcache.DeleteAll();
}
