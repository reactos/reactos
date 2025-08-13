/*
 * PROJECT:         ReactOS API Tests
 * LICENSE:         MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:         COM interface test for windows.storage classes
 * COPYRIGHT:       Copyright 2025 Carl Bialorucki <carl.bialorucki@reactos.org>
 */

#include "com_apitest.h"

#define NDEBUG
#include <debug.h>

static const CLASS_AND_INTERFACES ExpectedInterfaces_Win10_1607[] =
{
    {
        ID_NAME(CLSID_MyComputer),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {    0xc,   0x18,   &IID_IObjectWithSite },
            {   0x18,   0x30,   &IID_IPersistFolder2 },
            {   0x18,   0x30,       &IID_IPersistFolder },
            {   0x18,   0x30,           &IID_IPersist },
            {   0x1c,   0x38,   &IID_IPersistPropertyBag },
            {   0x28,   0x50,   &IID_IItemNameLimits },
            {   0x30,   0x60,   &IID_INewItemAdvisor },
            {   0x34,   0x68,   &IID_IContextMenuCB },
            {   0x60,   0xc0,   &IID_IFolderFilter },
            { FARAWY, FARAWY,   &IID_IShellIcon },
            { FARAWY, FARAWY,   &IID_IStorage },
            { FARAWY, FARAWY,   &IID_IObjectWithBackReferences },
            { FARAWY, FARAWY,   &IID_IShellFolder2 },
            { FARAWY, FARAWY,       &IID_IShellFolder },
            { FARAWY, FARAWY,   &IID_IShellIconOverlay },
            { FARAWY, FARAWY,   &IID_IDelegateHostItemContainer },
            { FARAWY, FARAWY,   &IID_IBackReferencedObject },
        }
    },
    {
        ID_NAME(CLSID_QueryAssociations),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {    0x4,    0x8,   &IID_IQueryAssociations },
        }
    },
#if 0 // This is correct for Windows 10 1607, but crashes the test when un-initializing it.
    {
        ID_NAME(CLSID_ShellDesktop),
        {
            {   -0xc,  -0x18,   &CLSID_ShellDesktop },
            {   -0xc,  -0x18,      &IID_IObjectWithSite },
            {    0x0,    0x0,  &IID_IUnknown },
            {   0x10,   0x20,  &IID_IStorage },
            {   0x14,   0x28,  &IID_IPersistFolder2 },
            {   0x14,   0x28,      &IID_IPersistFolder },
            {   0x14,   0x28,          &IID_IPersist },
            {   0x24,   0x48,  &IID_IContextMenuCB },
            {   0x28,   0x50,  &IID_IItemNameLimits },
            {   0x2c,   0x58,  &IID_IOleCommandTarget },
            {   0x30,   0x60,  &IID_IObjectWithBackReferences },
            {   0x58,   0xb0,  &IID_IFolderFilter },
            { FARAWY, FARAWY,  &IID_IShellIcon },
            { FARAWY, FARAWY,  &IID_IShellFolder },
            { FARAWY, FARAWY,  &IID_IShellFolder2 },
            { FARAWY, FARAWY,  &IID_IShellIconOverlay },
            { FARAWY, FARAWY,  &IID_IDelegateHostItemContainer },
            { FARAWY, FARAWY,  &IID_IBackReferencedObject },
        }
    },
#endif
    {
        ID_NAME(CLSID_ShellFSFolder),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {    0xc,   0x18,   &IID_IObjectWithSite },
            {   0x14,   0x28,   &IID_IShellFolder2 },
            {   0x14,   0x28,       &IID_IShellFolder },
            {   0x18,   0x30,   &IID_IShellIcon },
            {   0x1c,   0x38,   &IID_IShellIconOverlay },
            {   0x20,   0x40,   &IID_IPersistFolder3 },
            {   0x20,   0x40,       &IID_IPersistFolder2 },
            {   0x20,   0x40,           &IID_IPersistFolder },
            {   0x20,   0x40,               &IID_IPersist },
            {   0x24,   0x48,   &IID_IStorage },
            {   0x2c,   0x58,   &IID_IItemNameLimits },
            {   0x30,   0x60,   &IID_IContextMenuCB },
            {   0x3c,   0x78,   &IID_IOleCommandTarget },
            {   0x40,   0x80,   &IID_IPersistPropertyBag },
            {   0x44,   0x88,   &IID_IParentAndItem },
            {   0x50,   0xa0,   &IID_IObjectWithBackReferences },
            {   0x54,   0xa8,   &IID_IRemoteComputer },
            {   0x60,   0xc0,   &IID_IFolderType },
            {   0x70,   0xe0,   &IID_IBackReferencedObject },
        }
    },
    {
        ID_NAME(CLSID_ShellItem),
        {
            {   -0xc,  -0x18,   &IID_IMarshal },
            {   -0x8,  -0x10,   &IID_IShellItem2 },
            {   -0x8,  -0x10,       &IID_IShellItem },
            {    0x0,    0x0,   &IID_IPersistIDList },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IParentAndItem },
            {    0x8,   0x10,   &IID_IPersistStream },
            {    0x8,   0x10,       &IID_IPersist },
            {   0x14,   0x28,   &IID_IObjectWithBackReferences },
        },
        L"Both",
    },
    {
        ID_NAME(CLSID_ShellLink),
        {
            {  -0x30,  -0x60,   &IID_IMarshal },
            {  -0x2c,  -0x58,   &IID_IShellLinkA },
            {  -0x28,  -0x50,   &IID_IShellLinkW },
            {  -0x24,  -0x48,   &IID_IPersistStream },
            {  -0x1c,  -0x38,   &IID_IPersistFile },
            {  -0x1c,  -0x38,       &IID_IPersist },
            {  -0x18,  -0x30,   &IID_IShellExtInit },
            {  -0x14,  -0x28,   &IID_IContextMenu3 },
            {  -0x14,  -0x28,       &IID_IContextMenu2 },
            {  -0x14,  -0x28,           &IID_IContextMenu },
            {  -0x10,  -0x20,   &IID_IDropTarget },
            {   -0xc,  -0x18,   &IID_IQueryInfo },
            {   -0x8,  -0x10,   &IID_IShellLinkDataList },
            {   -0x4,   -0x8,   &IID_IExtractIconA },
            {    0x0,    0x0,   &IID_IExtractIconW },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IExtractImage2 },
            {    0x4,    0x8,       &IID_IExtractImage },
            {    0xc,   0x18,   &IID_IPersistPropertyBag },
            {   0x10,   0x20,   &IID_IServiceProvider },
            {   0x14,   0x28,   &IID_IObjectWithSite },
            {   0x1c,   0x38,   &IID_IPropertyStore },
            {   0x20,   0x40,   &IID_ICustomizeInfoTip },
            { FARAWY, FARAWY,   &IID_ISLTracker },
        },
        L"Both",
    },
};

START_TEST(windows_storage)
{
    if (GetNTVersion() <= _WIN32_WINNT_WINBLUE)
        skip("No windows_storage class tests for Windows 8.1 and older!\n");
    else
        TestClassesEx(L"windows.storage", ExpectedInterfaces_Win10_1607, RTL_NUMBER_OF(ExpectedInterfaces_Win10_1607), FALSE);
}