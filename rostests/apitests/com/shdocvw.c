/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         COM interface test for shdocvw classes
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#include "com_apitest.h"

#define NDEBUG
#include <debug.h>

static const CLASS_AND_INTERFACES ExpectedInterfaces[] =
{
    {
        ID_NAME(CLSID_FontsFolderShortcut),
        {
            {    0x0,   &IID_IShellFolder2 },
            {    0x0,       &IID_IShellFolder },
            {    0x0,           &IID_IUnknown },
            {    0x4,   &IID_IPersistFolder3 },
            {    0x4,       &IID_IPersistFolder2 },
            {    0x4,           &IID_IPersistFolder },
            {    0x4,               &IID_IPersist },
            {    0x8,   &IID_IShellLinkA },
            {    0xc,   &IID_IShellLinkW },
            {   0x10,   &IID_IPersistFile },
            {   0x14,   &IID_IExtractIconW },
            {   0x18,   &IID_IQueryInfo },
            {   0x20,   &IID_IPersistStream },
            {   0x20,   &IID_IPersistStreamInit },
            {   0x24,   &IID_IPersistPropertyBag },
        }
    },
#if 0 // E_OUTOFMEMORY?
    {
        ID_NAME(CLSID_ShellDispatchInproc),
        {
            {    0x0,                       &IID_IUnknown },
        }
    },
#endif
    {
        ID_NAME(CLSID_TaskbarList),
        {
            {    0x0,   &IID_ITaskbarList2 },
            {    0x0,       &IID_ITaskbarList },
            {    0x0,           &IID_IUnknown },
        }
    },
};
static const INT ExpectedInterfaceCount = RTL_NUMBER_OF(ExpectedInterfaces);

START_TEST(shdocvw)
{
    TestClasses(L"shdocvw", ExpectedInterfaces, ExpectedInterfaceCount);
}
