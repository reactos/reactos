/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     COM interface test for zipfldr classes
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#include "com_apitest.h"

#define NDEBUG
#include <debug.h>

/*
This is only tested on w10 & 2k3, so the defines might be wrong for the other versions.
CLSID_ZipFolderStorageHandler and CLSID_ZipFolderContextMenu seem to be the same.
*/


static const CLASS_AND_INTERFACES ExpectedInterfaces[] =
{
    {
        ID_NAME(CLSID_ZipFolderStorageHandler),
        {
            {    0x0,   &IID_IShellFolder2 },
            {    0x0,       &IID_IShellFolder },
            {    0x0,           &IID_IUnknown },
            {    0x4,   &IID_IStorage },
            {    0x8,   &IID_IContextMenu },
            {    0xc,   &IID_IShellExtInit },
#if _WIN32_WINNT < 0x0a00
            {   0x10,   &IID_IPersistFile },
            {   0x14,   &IID_IPersistFolder2 },
            {   0x14,       &IID_IPersistFolder },
            {   0x14,           &IID_IPersist },
#else
            {   0x10,   &IID_IPersistFolder2 },
            {   0x10,       &IID_IPersistFolder },
            {   0x10,           &IID_IPersist },
            {   0x14,   &IID_IFolderType },
#endif
        },
        L"Apartment"
    },
    {
        ID_NAME(CLSID_ZipFolderSendTo),
        {
#if _WIN32_WINNT >= 0x0a00
            {   -0x8,   &IID_IObjectWithSite },
#endif
            {    0x0,   &IID_IDropTarget },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_IPersistFile },
            {    0x4,       &IID_IPersist },
        },
        L"Apartment"
    },
    {
        ID_NAME(CLSID_ZipFolderContextMenu),
        {
            {    0x0,   &IID_IShellFolder2 },
            {    0x0,       &IID_IShellFolder },
            {    0x0,           &IID_IUnknown },
            {    0x4,   &IID_IStorage },
            {    0x8,   &IID_IContextMenu },
            {    0xc,   &IID_IShellExtInit },
#if _WIN32_WINNT < 0x0a00
            {   0x10,   &IID_IPersistFile },
            {   0x14,   &IID_IPersistFolder2 },
            {   0x14,       &IID_IPersistFolder },
            {   0x14,           &IID_IPersist },
#else
            {   0x10,   &IID_IPersistFolder2 },
            {   0x10,       &IID_IPersistFolder },
            {   0x10,           &IID_IPersist },
            {   0x14,   &IID_IFolderType },
#endif
        },
        L"Apartment"
    },
    {
        ID_NAME(CLSID_ZipFolderRightDragHandler),
        {
            {   -0x4,   &IID_IContextMenu },
            {    0x0,   &IID_IShellExtInit },
            {    0x0,       &IID_IUnknown },
        },
        L"Apartment"
    },
    {
        ID_NAME(CLSID_ZipFolderDropHandler),
        {
#if _WIN32_WINNT < 0x0a00
            {    0x0,   &IID_IDropTarget },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_IPersistFile },
            {    0x4,       &IID_IPersist },
#else
            {   -0x4,   &IID_IDropTarget },
            {    0x0,   &IID_IUnknown },
#endif
        },
        L"Apartment"
    },
};
static const INT ExpectedInterfaceCount = RTL_NUMBER_OF(ExpectedInterfaces);

START_TEST(zipfldr)
{
    TestClasses(L"zipfldr", ExpectedInterfaces, ExpectedInterfaceCount);
}
