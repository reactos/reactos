/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         COM interface test for browseui classes
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#include "com_apitest.h"

#define NDEBUG
#include <debug.h>

static const CLASS_AND_INTERFACES ExpectedInterfaces[] =
{
    {
        ID_NAME(CLSID_RebarBandSite),
        {
            {    0x0,       &IID_IUnknown },
            {    0xc,   &IID_IBandSite },
            {   0x10,   &IID_IInputObjectSite },
            {   0x14,   &IID_IInputObject },
            {   0x18,   &IID_IDeskBarClient },
            {   0x18,       &IID_IOleWindow },
            {   0x1c,   &IID_IWinEventHandler },
            {   0x20,   &IID_IPersistStream },
            {   0x20,       &IID_IPersist },
            {   0x24,   &IID_IDropTarget },
            {   0x28,   &IID_IServiceProvider },
            {   0x2c,   &IID_IBandSiteHelper },
            {   0x30,   &IID_IOleCommandTarget },
        }
    },
    {
        ID_NAME(CLSID_SH_AddressBand),
        {
            {    0x0,   &IID_IDeskBand },
            {    0x0,       &IID_IDockingWindow },
            {    0x0,           &IID_IOleWindow },
            {    0x0,               &IID_IUnknown },
            {    0x4,   &IID_IObjectWithSite },
            {    0xc,   &IID_IInputObject },
            {   0x10,   &IID_IPersistStream },
            {   0x10,       &IID_IPersist },
            {   0x14,   &IID_IOleCommandTarget },
            {   0x18,   &IID_IServiceProvider },
            {   0x30,   &IID_IWinEventHandler },
            {   0x34,   &IID_IAddressBand },
            {   0x38,   &IID_IInputObjectSite },
        }
    },
    {
        ID_NAME(CLSID_ShellSearchExt),
        {
            {    0x0,   &IID_IContextMenu },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_IObjectWithSite },
        }
    },
};
static const INT ExpectedInterfaceCount = RTL_NUMBER_OF(ExpectedInterfaces);

START_TEST(browseui)
{
    TestClasses(L"browseui", ExpectedInterfaces, ExpectedInterfaceCount);
}
