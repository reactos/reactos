/*
 *    Gameux library coclass GameExplorer implementation
 *
 * Copyright (C) 2010 Mariusz Pluciński
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#define COBJMACROS


#include "ole2.h"
#include "sddl.h"

#include "gameux.h"
#include "gameux_private.h"

#include "initguid.h"
#include "msxml2.h"

#include "wine/debug.h"
#include "winreg.h"

WINE_DEFAULT_DEBUG_CHANNEL(gameux);

/*******************************************************************************
 * GameUX helper functions
 */
/*******************************************************************************
 * GAMEUX_initGameData
 *
 * Internal helper function.
 * Initializes GAME_DATA structure fields with proper values. Should be
 * called always before first usage of this structure. Implemented in gameexplorer.c
 *
 * Parameters:
 *  GameData                        [I/O]   pointer to structure to initialize
 */
static void GAMEUX_initGameData(struct GAMEUX_GAME_DATA *GameData)
{
    GameData->sGDFBinaryPath = NULL;
    GameData->sGameInstallDirectory = NULL;
    GameData->bstrName = NULL;
    GameData->bstrDescription = NULL;
}
/*******************************************************************************
 * GAMEUX_uninitGameData
 *
 * Internal helper function.
 * Properly frees all data stored or pointed by fields of GAME_DATA structure.
 * Should be called before freeing this structure. Implemented in gameexplorer.c
 *
 * Parameters:
 *  GameData                        [I/O]   pointer to structure to uninitialize
 */
static void GAMEUX_uninitGameData(struct GAMEUX_GAME_DATA *GameData)
{
    free(GameData->sGDFBinaryPath);
    free(GameData->sGameInstallDirectory);
    SysFreeString(GameData->bstrName);
    SysFreeString(GameData->bstrDescription);
}
/*******************************************************************************
 * GAMEUX_buildGameRegistryPath
 *
 * Internal helper function. Description available in gameux_private.h file
 */
HRESULT GAMEUX_buildGameRegistryPath(GAME_INSTALL_SCOPE installScope,
        LPCGUID gameInstanceId,
        LPWSTR* lpRegistryPath)
{
    HRESULT hr = S_OK;
    HANDLE hToken = NULL;
    PTOKEN_USER pTokenUser = NULL;
    DWORD dwLength;
    LPWSTR lpSID = NULL;
    WCHAR sInstanceId[40];
    WCHAR sRegistryPath[8192];

    TRACE("(0x%x, %s, %p)\n", installScope, debugstr_guid(gameInstanceId), lpRegistryPath);

    /* this will make freeing it easier for user */
    *lpRegistryPath = NULL;

    lstrcpyW(sRegistryPath, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\GameUX\\");

    if(installScope == GIS_CURRENT_USER)
    {
        /* build registry path containing user's SID */
        if(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
            hr = HRESULT_FROM_WIN32(GetLastError());

        if(SUCCEEDED(hr))
        {
            if(!GetTokenInformation(hToken, TokenUser, NULL, 0, &dwLength) &&
                    GetLastError()!=ERROR_INSUFFICIENT_BUFFER)
                hr = HRESULT_FROM_WIN32(GetLastError());

            if(SUCCEEDED(hr))
            {
                pTokenUser = malloc(dwLength);
                if(!pTokenUser)
                    hr = E_OUTOFMEMORY;
            }

            if(SUCCEEDED(hr))
                if(!GetTokenInformation(hToken, TokenUser, (LPVOID)pTokenUser, dwLength, &dwLength))
                    hr = HRESULT_FROM_WIN32(GetLastError());

            if(SUCCEEDED(hr))
                if(!ConvertSidToStringSidW(pTokenUser->User.Sid, &lpSID))
                    hr = HRESULT_FROM_WIN32(GetLastError());

            if(SUCCEEDED(hr))
            {
                lstrcatW(sRegistryPath, lpSID);
                LocalFree(lpSID);
            }

            free(pTokenUser);
            CloseHandle(hToken);
        }
    }
    else if(installScope == GIS_ALL_USERS)
        /* build registry path without SID */
        lstrcatW(sRegistryPath, L"Games");
    else
        hr = E_INVALIDARG;

    /* put game's instance id on the end of path, only if instance id was given */
    if(gameInstanceId)
    {
        if(SUCCEEDED(hr))
            hr = (StringFromGUID2(gameInstanceId, sInstanceId, ARRAY_SIZE(sInstanceId)) ? S_OK : E_FAIL);

        if(SUCCEEDED(hr))
        {
            lstrcatW(sRegistryPath, L"\\");
            lstrcatW(sRegistryPath, sInstanceId);
        }
    }

    if(SUCCEEDED(hr))
    {
        *lpRegistryPath = strdupW(sRegistryPath);
        if(!*lpRegistryPath)
            hr = E_OUTOFMEMORY;
    }

    TRACE("result: 0x%lx, path: %s\n", hr, debugstr_w(*lpRegistryPath));
    return hr;
}
/*******************************************************************************
 * GAMEUX_WriteRegistryRecord
 *
 * Helper function, writes data associated with game (stored in GAMEUX_GAME_DATA
 * structure) into expected place in registry.
 *
 * Parameters:
 *  GameData                            [I]     structure with data which will
 *                                              be written into registry.
 *                                              Proper values of fields installScope
 *                                              and guidInstanceId are required
 *                                              to create registry key.
 *
 * Schema of naming registry keys associated with games is available in
 * description of _buildGameRegistryPath internal function.
 *
 * List of registry keys associated with structure fields:
 *  Key                              Field in GAMEUX_GAME_DATA structure
 *   ApplicationId                    guidApplicationId
 *   ConfigApplicationPath            sGameInstallDirectory
 *   ConfigGDFBinaryPath              sGDFBinaryPath
 *   Title                            bstrName
 *
 */
static HRESULT GAMEUX_WriteRegistryRecord(struct GAMEUX_GAME_DATA *GameData)
{
    HRESULT hr, hr2;
    LPWSTR lpRegistryKey;
    HKEY hKey;
    WCHAR sGameApplicationId[40];

    TRACE("(%p)\n", GameData);

    hr = GAMEUX_buildGameRegistryPath(GameData->installScope, &GameData->guidInstanceId, &lpRegistryKey);

    if(SUCCEEDED(hr))
        hr = (StringFromGUID2(&GameData->guidApplicationId, sGameApplicationId, ARRAY_SIZE(sGameApplicationId)) ? S_OK : E_FAIL);

    if(SUCCEEDED(hr))
        hr = HRESULT_FROM_WIN32(RegCreateKeyExW(HKEY_LOCAL_MACHINE, lpRegistryKey,
                                                0, NULL, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, NULL,
                                                &hKey, NULL));

    if(SUCCEEDED(hr))
    {
        /* write game data to registry key */
        hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, L"ConfigApplicationPath", 0,
                                               REG_SZ, (LPBYTE)(GameData->sGameInstallDirectory),
                                               (lstrlenW(GameData->sGameInstallDirectory)+1)*sizeof(WCHAR)));

        if(SUCCEEDED(hr))
            hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, L"ConfigGDFBinaryPath", 0,
                                                   REG_SZ, (LPBYTE)(GameData->sGDFBinaryPath),
                                                   (lstrlenW(GameData->sGDFBinaryPath)+1)*sizeof(WCHAR)));

        if(SUCCEEDED(hr))
            hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, L"ApplicationId", 0,
                                                   REG_SZ, (LPBYTE)(sGameApplicationId),
                                                   (lstrlenW(sGameApplicationId)+1)*sizeof(WCHAR)));

        if(SUCCEEDED(hr))
            hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, L"Title", 0,
                                                   REG_SZ, (LPBYTE)(GameData->bstrName),
                                                   (lstrlenW(GameData->bstrName)+1)*sizeof(WCHAR)));

        if(SUCCEEDED(hr))
            hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, L"Description", 0,
                                                   REG_SZ, (LPBYTE)(GameData->bstrDescription ? GameData->bstrDescription : GameData->bstrName),
                                                   (lstrlenW(GameData->bstrDescription ? GameData->bstrDescription : GameData->bstrName)+1)*sizeof(WCHAR)));

        RegCloseKey(hKey);

        if(FAILED(hr))
        {
            /* if something failed, remove whole key */
            hr2 = RegDeleteKeyExW(HKEY_LOCAL_MACHINE, lpRegistryKey, KEY_WOW64_64KEY, 0);
            /* do not overwrite old failure code with new success code */
            if(FAILED(hr2))
                hr = hr2;
        }
    }

    free(lpRegistryKey);
    TRACE("returning 0x%lx\n", hr);
    return hr;
}
/*******************************************************************************
 * GAMEUX_ProcessGameDefinitionElement
 *
 * Helper function, parses single element from Game Definition
 *
 * Parameters:
 *  lpXMLElement                        [I]     game definition element
 *  GameData                            [O]     structure, where parsed
 *                                              data will be stored
 */
static HRESULT GAMEUX_ProcessGameDefinitionElement(
        IXMLDOMElement *element,
        struct GAMEUX_GAME_DATA *GameData)
{
    HRESULT hr;
    BSTR bstrElementName;

    TRACE("(%p, %p)\n", element, GameData);

    hr = IXMLDOMElement_get_nodeName(element, &bstrElementName);
    if(SUCCEEDED(hr))
    {
        /* check element name */
        if(lstrcmpW(bstrElementName, L"Name") == 0)
            hr = IXMLDOMElement_get_text(element, &GameData->bstrName);

        else if(lstrcmpW(bstrElementName, L"Description") == 0)
            hr = IXMLDOMElement_get_text(element, &GameData->bstrDescription);

        else
            FIXME("entry %s in Game Definition File not yet supported\n", debugstr_w(bstrElementName));

        SysFreeString(bstrElementName);
    }

    return hr;
}
/*******************************************************************************
 * GAMEUX_ParseGameDefinition
 *
 * Helper function, loads data from given XML element into fields of GAME_DATA
 * structure
 *
 * Parameters:
 *  lpXMLGameDefinitionElement          [I]     Game Definition XML element
 *  GameData                            [O]     structure where data loaded from
 *                                              XML element will be stored in
 */
static HRESULT GAMEUX_ParseGameDefinition(IXMLDOMElement *gamedef, struct GAMEUX_GAME_DATA *game_data)
{
    IXMLDOMNodeList *props;
    VARIANT var;
    HRESULT hr;
    BSTR attr;

    TRACE("(%p, %p)\n", gamedef, game_data);

    attr = SysAllocString(L"gameID");
    if (!attr)
        return E_OUTOFMEMORY;

    hr = IXMLDOMElement_getAttribute(gamedef, attr, &var);
    SysFreeString(attr);

    if (SUCCEEDED(hr))
    {
        hr = CLSIDFromString(V_BSTR(&var), &game_data->guidApplicationId);
        VariantClear(&var);
    }

    if (SUCCEEDED(hr))
        hr = IXMLDOMElement_get_childNodes(gamedef, &props);

    if (FAILED(hr))
        return hr;

    do
    {
        IXMLDOMNode *prop;

        hr = IXMLDOMNodeList_nextNode(props, &prop);
        if (hr == S_OK)
        {
            IXMLDOMElement *element;

            hr = IXMLDOMNode_QueryInterface(prop, &IID_IXMLDOMElement, (void**)&element);
            if (hr == S_OK)
            {
                hr = GAMEUX_ProcessGameDefinitionElement(element, game_data);
                IXMLDOMElement_Release(element);
            }

            IXMLDOMNode_Release(prop);
        }
    }
    while (hr == S_OK);
    IXMLDOMNodeList_Release(props);

    return FAILED(hr) ? hr : S_OK;
}

struct parse_gdf_thread_param
{
    struct GAMEUX_GAME_DATA *GameData;
    HRESULT hr;
};

/*******************************************************************************
 * GAMEUX_ParseGDFBinary
 *
 * Helper function, loads given binary and parses embed GDF if there's any.
 *
 * Parameters:
 *  GameData                [I/O]   Structure with game's data. Content of field
 *                                  sGDFBinaryPath defines path to binary, from
 *                                  which embed GDF will be loaded. Data from
 *                                  GDF will be stored in other fields of this
 *                                  structure.
 */
static DWORD WINAPI GAMEUX_ParseGDFBinary(void *thread_param)
{
    struct parse_gdf_thread_param *ctx = thread_param;
    struct GAMEUX_GAME_DATA *GameData = ctx->GameData;

    HRESULT hr = S_OK;
    WCHAR sResourcePath[MAX_PATH];
    VARIANT variant;
    VARIANT_BOOL isSuccessful;
    IXMLDOMDocument *document;
    IXMLDOMNode *gdNode;
    IXMLDOMElement *root, *gdElement;

    TRACE("(%p)->sGDFBinaryPath = %s\n", GameData, debugstr_w(GameData->sGDFBinaryPath));

    /* prepare path to GDF, using res:// prefix */
    lstrcpyW(sResourcePath, L"res://");
    lstrcatW(sResourcePath, GameData->sGDFBinaryPath);
    lstrcatW(sResourcePath, L"/DATA/");
    lstrcatW(sResourcePath, ID_GDF_XML_STR);

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER,
            &IID_IXMLDOMDocument, (void**)&document);

    if(SUCCEEDED(hr))
    {
        /* load GDF into MSXML */
        V_VT(&variant) = VT_BSTR;
        V_BSTR(&variant) = SysAllocString(sResourcePath);
        if(!V_BSTR(&variant))
            hr = E_OUTOFMEMORY;

        if(SUCCEEDED(hr))
        {
            hr = IXMLDOMDocument_load(document, variant, &isSuccessful);
            if(hr == S_FALSE || isSuccessful == VARIANT_FALSE)
                hr = E_FAIL;
        }

        SysFreeString(V_BSTR(&variant));

        if(SUCCEEDED(hr))
        {
            hr = IXMLDOMDocument_get_documentElement(document, &root);
            if(hr == S_FALSE)
                hr = E_FAIL;
        }

        if(SUCCEEDED(hr))
        {
            hr = IXMLDOMElement_get_firstChild(root, &gdNode);
            if(hr == S_FALSE)
                hr = E_FAIL;

            if(SUCCEEDED(hr))
            {
                hr = IXMLDOMNode_QueryInterface(gdNode, &IID_IXMLDOMElement, (LPVOID*)&gdElement);
                if(SUCCEEDED(hr))
                {
                    hr = GAMEUX_ParseGameDefinition(gdElement, GameData);
                    IXMLDOMElement_Release(gdElement);
                }

                IXMLDOMNode_Release(gdNode);
            }

            IXMLDOMElement_Release(root);
        }

        IXMLDOMDocument_Release(document);
    }

    CoUninitialize();
    ctx->hr = hr;
    return 0;
}

/*******************************************************************
 * GAMEUX_RemoveRegistryRecord
 *
 * Helper function, removes registry key associated with given game instance
 */
static HRESULT GAMEUX_RemoveRegistryRecord(GUID* pInstanceID)
{
    HRESULT hr;
    LPWSTR lpRegistryPath = NULL;
    TRACE("(%s)\n", debugstr_guid(pInstanceID));

    /* first, check is game installed for all users */
    hr = GAMEUX_buildGameRegistryPath(GIS_ALL_USERS, pInstanceID, &lpRegistryPath);
    if(SUCCEEDED(hr))
        hr = HRESULT_FROM_WIN32(RegDeleteKeyExW(HKEY_LOCAL_MACHINE, lpRegistryPath, KEY_WOW64_64KEY, 0));

    free(lpRegistryPath);

    /* if not, check current user */
    if(FAILED(hr))
    {
        hr = GAMEUX_buildGameRegistryPath(GIS_CURRENT_USER, pInstanceID, &lpRegistryPath);
        if(SUCCEEDED(hr))
            hr = HRESULT_FROM_WIN32(RegDeleteKeyExW(HKEY_LOCAL_MACHINE, lpRegistryPath, KEY_WOW64_64KEY, 0));

        free(lpRegistryPath);
    }

    return hr;
}
/*******************************************************************************
 *  GAMEUX_RegisterGame
 *
 * Internal helper function. Registers game associated with given GDF binary in
 * Game Explorer. Implemented in gameexplorer.c
 *
 * Parameters:
 *  sGDFBinaryPath                  [I]     path to binary containing GDF file in
 *                                          resources
 *  sGameInstallDirectory           [I]     path to directory, where game installed
 *                                          its files.
 *  installScope                    [I]     scope of game installation
 *  pInstanceID                     [I/O]   pointer to game instance identifier.
 *                                          If pointing to GUID_NULL, then new
 *                                          identifier will be generated automatically
 *                                          and returned via this parameter
 */
static HRESULT GAMEUX_RegisterGame(LPCWSTR sGDFBinaryPath,
        LPCWSTR sGameInstallDirectory,
        GAME_INSTALL_SCOPE installScope,
        GUID *pInstanceID)
{
    HRESULT hr = S_OK;
    struct GAMEUX_GAME_DATA GameData;

    TRACE("(%s, %s, 0x%x, %s)\n", debugstr_w(sGDFBinaryPath), debugstr_w(sGameInstallDirectory), installScope, debugstr_guid(pInstanceID));

    GAMEUX_initGameData(&GameData);
    GameData.sGDFBinaryPath = strdupW(sGDFBinaryPath);
    GameData.sGameInstallDirectory = strdupW(sGameInstallDirectory);
    GameData.installScope = installScope;

    /* generate GUID if it was not provided by user */
    if(IsEqualGUID(pInstanceID, &GUID_NULL))
        hr = CoCreateGuid(pInstanceID);

    GameData.guidInstanceId = *pInstanceID;

    /* load data from GDF binary */
    if(SUCCEEDED(hr))
    {
        struct parse_gdf_thread_param thread_param;
        HANDLE thread;
        DWORD ret;

        thread_param.GameData = &GameData;
        if(!(thread = CreateThread(NULL, 0, GAMEUX_ParseGDFBinary, &thread_param, 0, &ret)))
        {
            ERR("Failed to create thread.\n");
            hr = E_FAIL;
            goto done;
        }
        ret = WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
        if(ret != WAIT_OBJECT_0)
        {
            ERR("Wait failed (%#lx).\n", ret);
            hr = E_FAIL;
            goto done;
        }
        hr = thread_param.hr;
    }

    /* save data to registry */
    if(SUCCEEDED(hr))
        hr = GAMEUX_WriteRegistryRecord(&GameData);

done:
    GAMEUX_uninitGameData(&GameData);
    TRACE("returning 0x%08lx\n", hr);
    return hr;
}
/*******************************************************************************
 * GAMEUX_IsGameKeyExist
 *
 * Helper function, checks if game's registry ath exists in given scope
 *
 * Parameters:
 *  installScope            [I]     scope to search game in
 *  InstanceID              [I]     game instance identifier
 *  lpRegistryPath          [O]     place to store address of registry path to
 *                                  the game. It is filled only if key exists.
 *                                  It must be freed by free(...)
 *
 * Returns:
 *  S_OK                key was found properly
 *  S_FALSE             key does not exists
 *
 */
static HRESULT GAMEUX_IsGameKeyExist(GAME_INSTALL_SCOPE installScope,
    LPCGUID InstanceID,
    LPWSTR* lpRegistryPath) {

    HRESULT hr;
    HKEY hKey;

    hr = GAMEUX_buildGameRegistryPath(installScope, InstanceID, lpRegistryPath);

    if(SUCCEEDED(hr))
        hr = HRESULT_FROM_WIN32(RegOpenKeyExW(HKEY_LOCAL_MACHINE, *lpRegistryPath,
                                              0, KEY_WOW64_64KEY, &hKey));

    if(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        hr = S_FALSE;

    if(hr == S_OK)
        RegCloseKey(hKey);
    else
    {
        /* if the key does not exist or another error occurred, do not return the path */
        free(*lpRegistryPath);
        *lpRegistryPath = NULL;
    }

    return hr;
}
/*******************************************************************************
 * GAMEUX_LoadRegistryString
 *
 * Helper function, loads string from registry value and allocates buffer for it
 */
static HRESULT GAMEUX_LoadRegistryString(HKEY hRootKey,
        LPCWSTR lpRegistryKey,
        LPCWSTR lpRegistryValue,
        LPWSTR* lpValue)
{
    HRESULT hr;
    DWORD dwSize;

    *lpValue = NULL;

    hr = HRESULT_FROM_WIN32(RegGetValueW(hRootKey, lpRegistryKey, lpRegistryValue,
            RRF_RT_REG_SZ, NULL, NULL, &dwSize));

    if(SUCCEEDED(hr))
    {
        *lpValue = malloc(dwSize);
        if(!*lpValue)
            hr = E_OUTOFMEMORY;
    }

    if(SUCCEEDED(hr))
        hr = HRESULT_FROM_WIN32(RegGetValueW(hRootKey, lpRegistryKey, lpRegistryValue,
                RRF_RT_REG_SZ, NULL, *lpValue, &dwSize));

    return hr;
}
/*******************************************************************************
 * GAMEUX_UpdateGame
 *
 * Helper function, updates stored data about game with given InstanceID
 */
static HRESULT GAMEUX_UpdateGame(LPGUID InstanceID) {
    HRESULT hr;
    GAME_INSTALL_SCOPE installScope;
    LPWSTR lpRegistryPath;
    LPWSTR lpGDFBinaryPath;

    TRACE("(%s)\n", debugstr_guid(InstanceID));

    /* first, check is game exists in CURRENT_USER scope  */
    installScope = GIS_CURRENT_USER;
    hr = GAMEUX_IsGameKeyExist(installScope, InstanceID, &lpRegistryPath);

    if(hr == S_FALSE)
    {
        /* game not found in CURRENT_USER scope, let's check in ALL_USERS */
        installScope = GIS_ALL_USERS;
        hr = GAMEUX_IsGameKeyExist(installScope, InstanceID, &lpRegistryPath);
    }

    if(hr == S_FALSE)
        /* still not found? let's inform user that game does not exists */
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    if(SUCCEEDED(hr))
    {
        WCHAR *lpGameInstallDirectory = NULL;

        /* game found, its registry path is in lpRegistryPath and install
         * scope in installScope */
        TRACE("game found in registry (path %s), updating\n", debugstr_w(lpRegistryPath));

        /* first, read required data about game */
        hr = GAMEUX_LoadRegistryString(HKEY_LOCAL_MACHINE, lpRegistryPath,
            L"ConfigGDFBinaryPath", &lpGDFBinaryPath);

        if(SUCCEEDED(hr))
            hr = GAMEUX_LoadRegistryString(HKEY_LOCAL_MACHINE, lpRegistryPath,
                L"ConfigApplicationPath", &lpGameInstallDirectory);

        /* now remove currently existing registry key */
        if(SUCCEEDED(hr))
            hr = GAMEUX_RemoveRegistryRecord(InstanceID);

        /* and add it again, it will cause in reparsing of whole GDF */
        if(SUCCEEDED(hr))
            hr = GAMEUX_RegisterGame(lpGDFBinaryPath, lpGameInstallDirectory,
                                     installScope, InstanceID);

        free(lpGDFBinaryPath);
        free(lpGameInstallDirectory);
    }

    free(lpRegistryPath);
    TRACE("returning 0x%lx\n", hr);
    return hr;
}
/*******************************************************************************
 * GAMEUX_FindGameInstanceId
 *
 * Internal helper function. Description available in gameux_private.h file
 */
HRESULT GAMEUX_FindGameInstanceId(
        LPCWSTR sGDFBinaryPath,
        GAME_INSTALL_SCOPE installScope,
        GUID* pInstanceId)
{
    HRESULT hr;
    BOOL found = FALSE;
    LPWSTR lpRegistryPath = NULL;
    HKEY hRootKey;
    DWORD dwSubKeys, dwSubKeyLen, dwMaxSubKeyLen, i;
    LPWSTR lpName = NULL, lpValue = NULL;

    hr = GAMEUX_buildGameRegistryPath(installScope, NULL, &lpRegistryPath);

    if(SUCCEEDED(hr))
        /* enumerate all subkeys of received one and search them for value "ConfigGGDFBinaryPath" */
        hr = HRESULT_FROM_WIN32(RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                lpRegistryPath, 0, KEY_READ | KEY_WOW64_64KEY, &hRootKey));

    if(SUCCEEDED(hr))
    {
        hr = HRESULT_FROM_WIN32(RegQueryInfoKeyW(hRootKey, NULL, NULL, NULL,
                &dwSubKeys, &dwMaxSubKeyLen, NULL, NULL, NULL, NULL, NULL, NULL));

        if(SUCCEEDED(hr))
        {
            ++dwMaxSubKeyLen; /* for string terminator */
            lpName = malloc(dwMaxSubKeyLen * sizeof(WCHAR));
            if(!lpName) hr = E_OUTOFMEMORY;
        }

        if(SUCCEEDED(hr))
        {
            for(i=0; i<dwSubKeys && !found; ++i)
            {
                dwSubKeyLen = dwMaxSubKeyLen;
                hr = HRESULT_FROM_WIN32(RegEnumKeyExW(hRootKey, i, lpName, &dwSubKeyLen,
                        NULL, NULL, NULL, NULL));

                if(SUCCEEDED(hr))
                    hr = GAMEUX_LoadRegistryString(hRootKey, lpName,
                            L"ConfigGDFBinaryPath", &lpValue);

                if(SUCCEEDED(hr))
                {
                    if(lstrcmpW(lpValue, sGDFBinaryPath)==0)
                    {
                        /* key found, let's copy instance id and exit */
                        hr = CLSIDFromString(lpName, pInstanceId);
                        found = TRUE;
                    }
                    free(lpValue);
                }
            }
        }

        free(lpName);
        RegCloseKey(hRootKey);
    }

    free(lpRegistryPath);

    if((SUCCEEDED(hr) && !found) || hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        hr = S_FALSE;

    return hr;
}
/*******************************************************************************
 * GameExplorer implementation
 */

typedef struct _GameExplorerImpl
{
    IGameExplorer IGameExplorer_iface;
    IGameExplorer2 IGameExplorer2_iface;
    LONG ref;
} GameExplorerImpl;

static inline GameExplorerImpl *impl_from_IGameExplorer(IGameExplorer *iface)
{
    return CONTAINING_RECORD(iface, GameExplorerImpl, IGameExplorer_iface);
}

static inline GameExplorerImpl *impl_from_IGameExplorer2(IGameExplorer2 *iface)
{
    return CONTAINING_RECORD(iface, GameExplorerImpl, IGameExplorer2_iface);
}

static HRESULT WINAPI GameExplorerImpl_QueryInterface(
        IGameExplorer *iface,
        REFIID riid,
        void **ppvObject)
{
    GameExplorerImpl *This = impl_from_IGameExplorer(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppvObject);

    *ppvObject = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown) ||
       IsEqualGUID(riid, &IID_IGameExplorer))
    {
        *ppvObject = &This->IGameExplorer_iface;
    }
    else if(IsEqualGUID(riid, &IID_IGameExplorer2))
    {
        *ppvObject = &This->IGameExplorer2_iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IGameExplorer_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI GameExplorerImpl_AddRef(IGameExplorer *iface)
{
    GameExplorerImpl *This = impl_from_IGameExplorer(iface);
    LONG ref;

    ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): ref=%ld\n", This, ref);
    return ref;
}

static ULONG WINAPI GameExplorerImpl_Release(IGameExplorer *iface)
{
    GameExplorerImpl *This = impl_from_IGameExplorer(iface);
    LONG ref;

    ref = InterlockedDecrement(&This->ref);
    TRACE("(%p): ref=%ld\n", This, ref);

    if(ref == 0)
    {
        TRACE("freeing GameExplorer object\n");
        free(This);
    }

    return ref;
}

static HRESULT WINAPI GameExplorerImpl_AddGame(
        IGameExplorer *iface,
        BSTR bstrGDFBinaryPath,
        BSTR sGameInstallDirectory,
        GAME_INSTALL_SCOPE installScope,
        GUID *pInstanceID)
{
    GameExplorerImpl *This = impl_from_IGameExplorer(iface);
    TRACE("(%p, %s, %s, 0x%x, %s)\n", This, debugstr_w(bstrGDFBinaryPath), debugstr_w(sGameInstallDirectory), installScope, debugstr_guid(pInstanceID));
    return GAMEUX_RegisterGame(bstrGDFBinaryPath, sGameInstallDirectory, installScope, pInstanceID);
}

static HRESULT WINAPI GameExplorerImpl_RemoveGame(
        IGameExplorer *iface,
        GUID instanceID)
{
    GameExplorerImpl *This = impl_from_IGameExplorer(iface);

    TRACE("(%p, %s)\n", This, debugstr_guid(&instanceID));
    return GAMEUX_RemoveRegistryRecord(&instanceID);
}

static HRESULT WINAPI GameExplorerImpl_UpdateGame(
        IGameExplorer *iface,
        GUID instanceID)
{
    GameExplorerImpl *This = impl_from_IGameExplorer(iface);

    TRACE("(%p, %s)\n", This, debugstr_guid(&instanceID));
    return GAMEUX_UpdateGame(&instanceID);
}

static HRESULT WINAPI GameExplorerImpl_VerifyAccess(
        IGameExplorer *iface,
        BSTR sGDFBinaryPath,
        BOOL *pHasAccess)
{
    GameExplorerImpl *This = impl_from_IGameExplorer(iface);

    FIXME("(%p, %s, %p)\n", This, debugstr_w(sGDFBinaryPath), pHasAccess);
    *pHasAccess = TRUE;
    return S_OK;
}

static const struct IGameExplorerVtbl GameExplorerImplVtbl =
{
    GameExplorerImpl_QueryInterface,
    GameExplorerImpl_AddRef,
    GameExplorerImpl_Release,
    GameExplorerImpl_AddGame,
    GameExplorerImpl_RemoveGame,
    GameExplorerImpl_UpdateGame,
    GameExplorerImpl_VerifyAccess
};


static HRESULT WINAPI GameExplorer2Impl_QueryInterface(
        IGameExplorer2 *iface,
        REFIID riid,
        void **ppvObject)
{
    GameExplorerImpl *This = impl_from_IGameExplorer2(iface);
    return GameExplorerImpl_QueryInterface(&This->IGameExplorer_iface, riid, ppvObject);
}

static ULONG WINAPI GameExplorer2Impl_AddRef(IGameExplorer2 *iface)
{
    GameExplorerImpl *This = impl_from_IGameExplorer2(iface);
    return GameExplorerImpl_AddRef(&This->IGameExplorer_iface);
}

static ULONG WINAPI GameExplorer2Impl_Release(IGameExplorer2 *iface)
{
    GameExplorerImpl *This = impl_from_IGameExplorer2(iface);
    return GameExplorerImpl_Release(&This->IGameExplorer_iface);
}

static HRESULT WINAPI GameExplorer2Impl_CheckAccess(
        IGameExplorer2 *iface,
        LPCWSTR binaryGDFPath,
        BOOL *pHasAccess)
{
    GameExplorerImpl *This = impl_from_IGameExplorer2(iface);
    FIXME("stub (%p, %s, %p)\n", This, debugstr_w(binaryGDFPath), pHasAccess);
    return E_NOTIMPL;
}

static HRESULT WINAPI GameExplorer2Impl_InstallGame(
        IGameExplorer2 *iface,
        LPCWSTR binaryGDFPath,
        LPCWSTR installDirectory,
        GAME_INSTALL_SCOPE installScope)
{
    HRESULT hr;
    GUID instanceId;
    GameExplorerImpl *This = impl_from_IGameExplorer2(iface);

    TRACE("(%p, %s, %s, 0x%x)\n", This, debugstr_w(binaryGDFPath), debugstr_w(installDirectory), installScope);

    if(!binaryGDFPath)
        return E_INVALIDARG;

    hr = GAMEUX_FindGameInstanceId(binaryGDFPath, GIS_CURRENT_USER, &instanceId);

    if(hr == S_FALSE)
        hr = GAMEUX_FindGameInstanceId(binaryGDFPath, GIS_ALL_USERS, &instanceId);

    if(hr == S_FALSE)
    {
        /* if game isn't yet registered, then install it */
        instanceId = GUID_NULL;
        hr = GAMEUX_RegisterGame(binaryGDFPath, installDirectory, installScope, &instanceId);
    }
    else if(hr == S_OK)
        /* otherwise, update game */
        hr = GAMEUX_UpdateGame(&instanceId);

    return hr;
}

static HRESULT WINAPI GameExplorer2Impl_UninstallGame(
        IGameExplorer2 *iface,
        LPCWSTR binaryGDFPath)
{
    HRESULT hr;
    GUID instanceId;
    GameExplorerImpl *This = impl_from_IGameExplorer2(iface);
    TRACE("(%p, %s)\n", This, debugstr_w(binaryGDFPath));

    if(!binaryGDFPath)
        return E_INVALIDARG;

    hr = GAMEUX_FindGameInstanceId(binaryGDFPath, GIS_CURRENT_USER, &instanceId);

    if(hr == S_FALSE)
        hr = GAMEUX_FindGameInstanceId(binaryGDFPath, GIS_ALL_USERS, &instanceId);

    if(hr == S_OK)
        hr = GAMEUX_RemoveRegistryRecord(&instanceId);

    return hr;
}

static const struct IGameExplorer2Vtbl GameExplorer2ImplVtbl =
{
    GameExplorer2Impl_QueryInterface,
    GameExplorer2Impl_AddRef,
    GameExplorer2Impl_Release,
    GameExplorer2Impl_InstallGame,
    GameExplorer2Impl_UninstallGame,
    GameExplorer2Impl_CheckAccess
};

/*
 * Construction routine
 */
HRESULT GameExplorer_create(
        IUnknown* pUnkOuter,
        IUnknown** ppObj)
{
    GameExplorerImpl *pGameExplorer;

    TRACE("(%p, %p)\n", pUnkOuter, ppObj);

    pGameExplorer = malloc(sizeof(*pGameExplorer));

    if(!pGameExplorer)
        return E_OUTOFMEMORY;

    pGameExplorer->IGameExplorer_iface.lpVtbl = &GameExplorerImplVtbl;
    pGameExplorer->IGameExplorer2_iface.lpVtbl = &GameExplorer2ImplVtbl;
    pGameExplorer->ref = 1;

    *ppObj = (IUnknown*)&pGameExplorer->IGameExplorer_iface;

    TRACE("returning iface: %p\n", *ppObj);
    return S_OK;
}
