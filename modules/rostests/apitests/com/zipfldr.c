/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     COM interface test for zipfldr classes
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#include "com_apitest.h"

#define NDEBUG
#include <debug.h>

static const CLASS_AND_INTERFACES ExpectedInterfaces[] =
{
    {
        ID_NAME(CLSID_ZipFolderStorageHandler, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_WIN7SP1,      &IID_IPersistFile },

            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellFolder2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellFolder },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IStorage },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IContextMenu },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellExtInit },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersist },

            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IFolderType},
        },
    },
    {
        ID_NAME(CLSID_ZipFolderSendTo, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDropTarget },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFile },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersist },

            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IObjectWithSite },
        },
    },
    {
        ID_NAME(CLSID_ZipFolderContextMenu, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_WIN7SP1,      &IID_IPersistFile },

            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellFolder2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellFolder },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IStorage },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IContextMenu },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellExtInit },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersist },

            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IFolderType},
        },
    },
    {
        ID_NAME(CLSID_ZipFolderRightDragHandler, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IContextMenu },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellExtInit },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
        },
    },
    {
        ID_NAME(CLSID_ZipFolderDropHandler, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDropTarget },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },

            { NTDDI_MIN,          NTDDI_WIN7SP1,      &IID_IPersistFile },
            { NTDDI_MIN,          NTDDI_WIN7SP1,      &IID_IPersist },
        },
    },
};

START_TEST(zipfldr)
{
    TestClasses(L"zipfldr", ExpectedInterfaces, RTL_NUMBER_OF(ExpectedInterfaces));
}
