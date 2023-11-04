/*
 *    Gameux library private header
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

extern HRESULT GameExplorer_create(IUnknown* pUnkOuter, IUnknown **ppObj) DECLSPEC_HIDDEN;
extern HRESULT GameStatistics_create(IUnknown* pUnkOuter, IUnknown **ppObj) DECLSPEC_HIDDEN;

/*******************************************************************************
 * Helper functions and structures
 *
 * These are helper function and structures, which  are used widely in gameux
 * implementation. Details about usage and place of implementation is
 * in description of each function/structure.
 */

/*******************************************************************************
 * struct GAMEUX_GAME_DATA
 *
 * Structure which contains data about single game. It is used to transfer
 * data inside of gameux module in various places.
 */
struct GAMEUX_GAME_DATA
{
    LPWSTR sGDFBinaryPath;          /* path to binary containing GDF */
    LPWSTR sGameInstallDirectory;   /* directory passed to AddGame/InstallGame methods */
    GAME_INSTALL_SCOPE installScope;/* game's installation scope */
    GUID guidInstanceId;            /* game installation instance identifier */
    GUID guidApplicationId;         /* game's application identifier */
    BSTR bstrName;                  /* game's title */
    BSTR bstrDescription;           /* game's description */
};
/*******************************************************************************
 * GAMEUX_FindGameInstanceId
 *
 * Helper function. Searches for instance identifier of given game in given
 * installation scope. Implemented in gameexplorer.c
 *
 * Parameters:
 *  sGDFBinaryPath                          [I]     path to binary containing GDF
 *  installScope                            [I]     game install scope to search in
 *  pInstanceId                             [O]     instance identifier of given game
 *
 * Returns:
 *  S_OK                    id was returned properly
 *  S_FALSE                 id was not found in the registry
 *  E_OUTOFMEMORY           problem while memory allocation
 */
HRESULT GAMEUX_FindGameInstanceId(
        LPCWSTR sGDFBinaryPath,
        GAME_INSTALL_SCOPE installScope,
        GUID* pInstanceId) DECLSPEC_HIDDEN;
/*******************************************************************************
 * GAMEUX_buildGameRegistryPath
 *
 * Helper function, builds registry path to key, where game's data are stored.
 * Implemented in gameexplorer.c
 *
 * Parameters:
 *  installScope                [I]     the scope which was used in AddGame/InstallGame call
 *  gameInstanceId              [I]     game instance GUID. If NULL, then only
 *                                      path to scope will be returned
 *  lpRegistryPath              [O]     pointer which will receive address to string
 *                                      containing expected registry path. Path
 *                                      is relative to HKLM registry key. It
 *                                      must be freed by calling free(...)
 *
 * Name of game's registry key always follows patterns below:
 *  When game is installed for current user only (installScope is GIS_CURRENT_USER):
 *      HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\
 *          GameUX\[user's security ID]\[game instance ID]
 *
 *  When game is installed for all users (installScope is GIS_ALL_USERS):
 *      HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\
 *          GameUX\Games\[game instance ID]
 *
 *
 */
HRESULT GAMEUX_buildGameRegistryPath(GAME_INSTALL_SCOPE installScope,
        LPCGUID gameInstanceId,
        LPWSTR* lpRegistryPath) DECLSPEC_HIDDEN;

static inline WCHAR *strdupW(const WCHAR *src)
{
    WCHAR *dst = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(src) + 1) * sizeof(WCHAR));
    if (dst) lstrcpyW(dst, src);
    return dst;
}
