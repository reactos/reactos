/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     COM interface test for zipfldr classes
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#include "com_apitest.h"

#define NDEBUG
#include <debug.h>

static const CLASS_AND_INTERFACES ExpectedInterfaces_WS03[] =
{
    {
        ID_NAME(CLSID_ZipFolderStorageHandler),
        {
            {    0x0,    0x0,   &IID_IShellFolder2 },
            {    0x0,    0x0,       &IID_IShellFolder },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IStorage },
            {    0x8,   0x10,   &IID_IContextMenu },
            {    0xc,   0x18,   &IID_IShellExtInit },
            {   0x10,   0x20,   &IID_IPersistFile },
            {   0x14,   0x28,   &IID_IPersistFolder2 },
            {   0x14,   0x28,       &IID_IPersistFolder },
            {   0x14,   0x28,           &IID_IPersist },
        },
    },
    {
        ID_NAME(CLSID_ZipFolderSendTo),
        {
            {    0x0,    0x0,   &IID_IDropTarget },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IPersistFile },
            {    0x4,    0x8,       &IID_IPersist },
        },
    },
    {
        ID_NAME(CLSID_ZipFolderContextMenu),
        {
            {    0x0,    0x0,   &IID_IShellFolder2 },
            {    0x0,    0x0,       &IID_IShellFolder },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IStorage },
            {    0x8,   0x10,   &IID_IContextMenu },
            {    0xc,   0x18,   &IID_IShellExtInit },
            {   0x10,   0x20,   &IID_IPersistFile },
            {   0x14,   0x28,   &IID_IPersistFolder2 },
            {   0x14,   0x28,       &IID_IPersistFolder },
            {   0x14,   0x28,           &IID_IPersist },
        },
    },
    {
        ID_NAME(CLSID_ZipFolderRightDragHandler),
        {
            {   -0x4,   -0x8,   &IID_IContextMenu },
            {    0x0,    0x0,   &IID_IShellExtInit },
            {    0x0,    0x0,       &IID_IUnknown },
        },
    },
    {
        ID_NAME(CLSID_ZipFolderDropHandler),
        {
            {    0x0,    0x0,   &IID_IDropTarget },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IPersistFile },
            {    0x4,    0x8,       &IID_IPersist },
        },
    },
};

static const CLASS_AND_INTERFACES ExpectedInterfaces_Vista[] =
{
    {
        ID_NAME(CLSID_ZipFolderStorageHandler),
        {
            {    0x0,    0x0,   &IID_IShellFolder2 },
            {    0x0,    0x0,       &IID_IShellFolder },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IStorage },
            {    0x8,   0x10,   &IID_IContextMenu },
            {    0xc,   0x18,   &IID_IShellExtInit },
            {   0x10,   0x20,   &IID_IPersistFile },
            {   0x14,   0x28,   &IID_IPersistFolder2 },
            {   0x14,   0x28,       &IID_IPersistFolder },
            {   0x14,   0x28,           &IID_IPersist },
            {   0x18,   0x30,   &IID_IFolderType},
        },
    },
    {
        ID_NAME(CLSID_ZipFolderSendTo),
        {
            {   -0x8,  -0x10,   &IID_IObjectWithSite },
            {    0x0,    0x0,   &IID_IDropTarget },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IPersistFile },
            {    0x4,    0x8,       &IID_IPersist },
        },
    },
    {
        ID_NAME(CLSID_ZipFolderContextMenu),
        {
            {    0x0,    0x0,   &IID_IShellFolder2 },
            {    0x0,    0x0,       &IID_IShellFolder },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IStorage },
            {    0x8,   0x10,   &IID_IContextMenu },
            {    0xc,   0x18,   &IID_IShellExtInit },
            {   0x10,   0x20,   &IID_IPersistFile },
            {   0x14,   0x28,   &IID_IPersistFolder2 },
            {   0x14,   0x28,       &IID_IPersistFolder },
            {   0x14,   0x28,           &IID_IPersist },
            {   0x18,   0x30,   &IID_IFolderType},
        },
    },
    {
        ID_NAME(CLSID_ZipFolderRightDragHandler),
        {
            {   -0x4,   -0x8,   &IID_IContextMenu },
            {    0x0,    0x0,   &IID_IShellExtInit },
            {    0x0,    0x0,       &IID_IUnknown },
        },
    },
    {
        ID_NAME(CLSID_ZipFolderDropHandler),
        {
            {    0x0,    0x0,   &IID_IDropTarget },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IPersistFile },
            {    0x4,    0x8,       &IID_IPersist },
        },
    },
};

static const CLASS_AND_INTERFACES ExpectedInterfaces_Win8[] =
{
    {
        ID_NAME(CLSID_ZipFolderStorageHandler),
        {
            {    0x0,    0x0,   &IID_IShellFolder2 },
            {    0x0,    0x0,       &IID_IShellFolder },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IStorage },
            {    0x8,   0x10,   &IID_IContextMenu },
            {    0xc,   0x18,   &IID_IShellExtInit },
            {   0x10,   0x20,   &IID_IPersistFolder2 },
            {   0x10,   0x20,       &IID_IPersistFolder },
            {   0x10,   0x20,           &IID_IPersist },
            {   0x14,   0x28,   &IID_IFolderType},
        },
    },
    {
        ID_NAME(CLSID_ZipFolderSendTo),
        {
            {   -0x8,  -0x10,   &IID_IObjectWithSite },
            {    0x0,    0x0,   &IID_IDropTarget },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IPersistFile },
            {    0x4,    0x8,       &IID_IPersist },
        },
    },
    {
        ID_NAME(CLSID_ZipFolderContextMenu),
        {
            {    0x0,    0x0,   &IID_IShellFolder2 },
            {    0x0,    0x0,       &IID_IShellFolder },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IStorage },
            {    0x8,   0x10,   &IID_IContextMenu },
            {    0xc,   0x18,   &IID_IShellExtInit },
            {   0x10,   0x20,   &IID_IPersistFolder2 },
            {   0x10,   0x20,       &IID_IPersistFolder },
            {   0x10,   0x20,           &IID_IPersist },
            {   0x14,   0x28,   &IID_IFolderType},
        },
    },
    {
        ID_NAME(CLSID_ZipFolderRightDragHandler),
        {
            {   -0x4,   -0x8,   &IID_IContextMenu },
            {    0x0,    0x0,   &IID_IShellExtInit },
            {    0x0,    0x0,       &IID_IUnknown },
        },
    },
    {
        ID_NAME(CLSID_ZipFolderDropHandler),
        {
            {   -0x4,   -0x8,   &IID_IDropTarget },
            {    0x0,    0x0,   &IID_IUnknown },
        },
    },
};

START_TEST(zipfldr)
{
    if (GetNTVersion() <= _WIN32_WINNT_WS03)
        TestClasses(L"zipfldr", ExpectedInterfaces_WS03, RTL_NUMBER_OF(ExpectedInterfaces_WS03));
    else if (GetNTVersion() <= _WIN32_WINNT_WIN7)
        TestClasses(L"zipfldr", ExpectedInterfaces_Vista, RTL_NUMBER_OF(ExpectedInterfaces_Vista));
    else
        TestClasses(L"zipfldr", ExpectedInterfaces_Win8, RTL_NUMBER_OF(ExpectedInterfaces_Win8));
}
