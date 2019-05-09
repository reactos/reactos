/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for SHEnumClassesOfCategories
 * COPYRIGHT:   Copyright 2019 Serge Gautherie <reactos-git_serge_171003@gautherie.fr>
 */

/*
TODO, if they make sense:
* Test more than 1 Implemented/Required.
  And maybe not "matching" ones.
* Check pEnumGUID after S_OK cases.
*/

#include <apitest.h>

#include <shlobj.h>
#include <atlbase.h>

typedef HRESULT (WINAPI *SHENUMCLASSESOFCATEGORIES)(ULONG cImplemented, CATID *pImplemented, ULONG cRequired, CATID *pRequired, IEnumGUID **out);

static SHENUMCLASSESOFCATEGORIES pSHEnumClassesOfCategories;

START_TEST(SHEnumClassesOfCategories)
{
    HRESULT hr;
    HMODULE hbrowseui;
    CATID CategoryImplemented, CategoryRequired;
    CComPtr<IEnumGUID> pEnumGUID;

    // Set up.

    hbrowseui = LoadLibraryA("browseui.dll");
    ok(hbrowseui != NULL, "LoadLibraryA() failed\n");
    if (!hbrowseui)
    {
        skip("No dll\n");
        return;
    }

    pSHEnumClassesOfCategories = (SHENUMCLASSESOFCATEGORIES)GetProcAddress(hbrowseui, MAKEINTRESOURCEA(136));
    ok(pSHEnumClassesOfCategories != NULL, "GetProcAddress() failed\n");
    if (!pSHEnumClassesOfCategories)
    {
        skip("No function, as on NT 6.1+\n");
        return;
    }

    // Test (mostly) invalid arguments.

    // Implemented, '(ULONG)-1, NULL'.
    // Keep this odd case, as ReactOS used to "implement" it.

    // hr = pSHEnumClassesOfCategories((ULONG)-1, NULL, 0, NULL, NULL);
    // ok_long(hr, E_INVALIDARG);
    hr = pSHEnumClassesOfCategories((ULONG)-1, NULL, 0, NULL, &pEnumGUID);
    ok_long(hr, E_INVALIDARG);

    CategoryRequired = CATID_DeskBand;
    // hr = pSHEnumClassesOfCategories((ULONG)-1, NULL, 1, &CategoryRequired, NULL);
    // ok_long(hr, E_INVALIDARG);
    hr = pSHEnumClassesOfCategories((ULONG)-1, NULL, 1, &CategoryRequired, &pEnumGUID);
    ok_long(hr, E_INVALIDARG);

    // Implemented, '0, NULL'.

    // hr = pSHEnumClassesOfCategories(0, NULL, 0, NULL, NULL);
    // ok_long(hr, E_INVALIDARG);
    hr = pSHEnumClassesOfCategories(0, NULL, 0, NULL, &pEnumGUID);
    ok_long(hr, E_INVALIDARG);

    // CategoryRequired = CATID_DeskBand;
    // hr = pSHEnumClassesOfCategories(0, NULL, 1, &CategoryRequired, NULL);
    // ok_long(hr, E_INVALIDARG);
    // The following case duplicates a later one.
    // hr = pSHEnumClassesOfCategories(0, NULL, 1, &CategoryRequired, &pEnumGUID);
    // ok_long(hr, S_OK);
    // ok(CategoryRequired == CATID_DeskBand, "CategoryRequired was modified\n");
    // pEnumGUID = NULL;

    // Implemented, '0, &CategoryImplemented'.

    CategoryImplemented = CATID_DeskBand;
    // hr = pSHEnumClassesOfCategories(0, &CategoryImplemented, 0, NULL, NULL);
    // ok_long(hr, E_INVALIDARG);
    hr = pSHEnumClassesOfCategories(0, &CategoryImplemented, 0, NULL, &pEnumGUID);
    ok_long(hr, E_INVALIDARG);

    CategoryRequired = CATID_DeskBand;
    // hr = pSHEnumClassesOfCategories(0, &CategoryImplemented, 1, &CategoryRequired, NULL);
    // ok_long(hr, E_INVALIDARG);
    hr = pSHEnumClassesOfCategories(0, &CategoryImplemented, 1, &CategoryRequired, &pEnumGUID);
    ok_long(hr, S_OK);
    ok(CategoryImplemented == CATID_DeskBand, "CategoryImplemented was modified\n");
    ok(CategoryRequired == CATID_DeskBand, "CategoryRequired was modified\n");
    pEnumGUID = NULL;

    CategoryRequired = CATID_InfoBand;
    // hr = pSHEnumClassesOfCategories(0, &CategoryImplemented, 1, &CategoryRequired, NULL);
    // ok_long(hr, E_INVALIDARG);
    hr = pSHEnumClassesOfCategories(0, &CategoryImplemented, 1, &CategoryRequired, &pEnumGUID);
    ok_long(hr, S_OK);
    ok(CategoryImplemented == CATID_DeskBand, "CategoryImplemented was modified\n");
    ok(CategoryRequired == CATID_InfoBand, "CategoryRequired was modified\n");
    pEnumGUID = NULL;

    // Implemented, '1, NULL'.

    // hr = pSHEnumClassesOfCategories(1, NULL, 0, NULL, NULL);
    // ok_long(hr, E_INVALIDARG);
    hr = pSHEnumClassesOfCategories(1, NULL, 0, NULL, &pEnumGUID);
    ok_long(hr, E_INVALIDARG);

    CategoryRequired = CATID_DeskBand;
    // hr = pSHEnumClassesOfCategories(1, NULL, 1, &CategoryRequired, NULL);
    // ok_long(hr, E_INVALIDARG);
    hr = pSHEnumClassesOfCategories(1, NULL, 1, &CategoryRequired, &pEnumGUID);
    ok_long(hr, E_INVALIDARG);

    // Required, '0, &CategoryRequired'.

    CategoryRequired = CATID_DeskBand;

    CategoryImplemented = CATID_DeskBand;
    // hr = pSHEnumClassesOfCategories(1, &CategoryImplemented, 0, &CategoryRequired, NULL);
    // ok_long(hr, E_INVALIDARG);
    hr = pSHEnumClassesOfCategories(1, &CategoryImplemented, 0, &CategoryRequired, &pEnumGUID);
    ok_long(hr, S_OK);
    ok(CategoryImplemented == CATID_DeskBand, "CategoryImplemented was modified\n");
    ok(CategoryRequired == CATID_DeskBand, "CategoryRequired was modified\n");
    pEnumGUID = NULL;

    CategoryImplemented = CATID_InfoBand;
    // hr = pSHEnumClassesOfCategories(1, &CategoryImplemented, 0, &CategoryRequired, NULL);
    // ok_long(hr, E_INVALIDARG);
    hr = pSHEnumClassesOfCategories(1, &CategoryImplemented, 0, &CategoryRequired, &pEnumGUID);
    ok_long(hr, S_OK);
    ok(CategoryImplemented == CATID_InfoBand, "CategoryImplemented was modified\n");
    ok(CategoryRequired == CATID_DeskBand, "CategoryRequired was modified\n");
    pEnumGUID = NULL;

    // Required, '1, NULL'.

    // hr = pSHEnumClassesOfCategories(0, NULL, 1, NULL, NULL);
    // ok_long(hr, E_INVALIDARG);
    hr = pSHEnumClassesOfCategories(0, NULL, 1, NULL, &pEnumGUID);
    ok_long(hr, E_INVALIDARG);

    CategoryImplemented = CATID_DeskBand;
    // hr = pSHEnumClassesOfCategories(1, &CategoryImplemented, 1, NULL, NULL);
    // ok_long(hr, E_INVALIDARG);
    hr = pSHEnumClassesOfCategories(1, &CategoryImplemented, 1, NULL, &pEnumGUID);
    ok_long(hr, E_INVALIDARG);

    // Out, 'NULL'.
    // Previous similar checks are commented out, for usual use.

    CategoryImplemented = CATID_DeskBand;
    hr = pSHEnumClassesOfCategories(1, &CategoryImplemented, 0, NULL, NULL);
    ok_long(hr, E_INVALIDARG);

    CategoryRequired = CATID_DeskBand;
    hr = pSHEnumClassesOfCategories(1, &CategoryImplemented, 1, &CategoryRequired, NULL);
    ok_long(hr, E_INVALIDARG);

    // Test success.
    // CATID_* from sdk/include/psdk/shlguid.h.
    // See also modules/rostests/winetests/shlwapi/clsid.c.

    // Each CATID, as Implemented.

    CategoryImplemented = CATID_BrowsableShellExt;
    hr = pSHEnumClassesOfCategories(1, &CategoryImplemented, 0, NULL, &pEnumGUID);
    ok_long(hr, S_OK);
    ok(CategoryImplemented == CATID_BrowsableShellExt, "CategoryImplemented was modified\n");
    pEnumGUID = NULL;

    CategoryImplemented = CATID_BrowseInPlace;
    hr = pSHEnumClassesOfCategories(1, &CategoryImplemented, 0, NULL, &pEnumGUID);
    ok_long(hr, S_OK);
    ok(CategoryImplemented == CATID_BrowseInPlace, "CategoryImplemented was modified\n");
    pEnumGUID = NULL;

    CategoryImplemented = CATID_DeskBand;
    hr = pSHEnumClassesOfCategories(1, &CategoryImplemented, 0, NULL, &pEnumGUID);
    ok_long(hr, S_OK);
    ok(CategoryImplemented == CATID_DeskBand, "CategoryImplemented was modified\n");
    pEnumGUID = NULL;

    CategoryImplemented = CATID_InfoBand;
    hr = pSHEnumClassesOfCategories(1, &CategoryImplemented, 0, NULL, &pEnumGUID);
    ok_long(hr, S_OK);
    ok(CategoryImplemented == CATID_InfoBand, "CategoryImplemented was modified\n");
    pEnumGUID = NULL;

    CategoryImplemented = CATID_CommBand;
    hr = pSHEnumClassesOfCategories(1, &CategoryImplemented, 0, NULL, &pEnumGUID);
    ok_long(hr, S_OK);
    ok(CategoryImplemented == CATID_CommBand, "CategoryImplemented was modified\n");
    pEnumGUID = NULL;

    // Each CATID, as Required.

    CategoryRequired = CATID_BrowsableShellExt;
    hr = pSHEnumClassesOfCategories(0, NULL, 1, &CategoryRequired, &pEnumGUID);
    ok_long(hr, S_OK);
    ok(CategoryRequired == CATID_BrowsableShellExt, "CategoryRequired was modified\n");
    pEnumGUID = NULL;

    CategoryRequired = CATID_BrowseInPlace;
    hr = pSHEnumClassesOfCategories(0, NULL, 1, &CategoryRequired, &pEnumGUID);
    ok_long(hr, S_OK);
    ok(CategoryRequired == CATID_BrowseInPlace, "CategoryRequired was modified\n");
    pEnumGUID = NULL;

    CategoryRequired = CATID_DeskBand;
    hr = pSHEnumClassesOfCategories(0, NULL, 1, &CategoryRequired, &pEnumGUID);
    ok_long(hr, S_OK);
    ok(CategoryRequired == CATID_DeskBand, "CategoryRequired was modified\n");
    pEnumGUID = NULL;

    CategoryRequired = CATID_InfoBand;
    hr = pSHEnumClassesOfCategories(0, NULL, 1, &CategoryRequired, &pEnumGUID);
    ok_long(hr, S_OK);
    ok(CategoryRequired == CATID_InfoBand, "CategoryRequired was modified\n");
    pEnumGUID = NULL;

    CategoryRequired = CATID_CommBand;
    hr = pSHEnumClassesOfCategories(0, NULL, 1, &CategoryRequired, &pEnumGUID);
    ok_long(hr, S_OK);
    ok(CategoryRequired == CATID_CommBand, "CategoryRequired was modified\n");
    pEnumGUID = NULL;

    // Same CATID, as Implemented and Required.

    CategoryImplemented = CATID_DeskBand;
    CategoryRequired = CATID_DeskBand;
    hr = pSHEnumClassesOfCategories(1, &CategoryImplemented, 1, &CategoryRequired, &pEnumGUID);
    ok_long(hr, S_OK);
    ok(CategoryImplemented == CATID_DeskBand, "CategoryImplemented was modified\n");
    ok(CategoryRequired == CATID_DeskBand, "CategoryRequired was modified\n");
    pEnumGUID = NULL;

    // Clean up.

    ok(FreeLibrary(hbrowseui) != 0, "FreeLibrary() failed\n");
}
