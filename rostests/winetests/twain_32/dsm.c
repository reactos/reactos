/* Unit test suite for Twain DSM functions
 *
 * Copyright 2009 Jeremy White, CodeWeavers, Inc.
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
 *
 */
#include <stdarg.h>
#include <assert.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"
#include "winuser.h"
#include "twain.h"

#include "wine/test.h"

static DSMENTRYPROC pDSM_Entry;

static BOOL dsm_RegisterWindowClasses(void)
{
    WNDCLASSA cls;

    cls.style = 0;
    cls.lpfnWndProc = DefWindowProc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "TWAIN_dsm_class";
    if (!RegisterClassA(&cls)) return FALSE;

    return TRUE;
}


static void get_condition_code(TW_IDENTITY *appid, TW_IDENTITY *source, TW_STATUS *status)
{
    TW_UINT16 rc;
    rc = pDSM_Entry(appid, source, DG_CONTROL, DAT_STATUS, MSG_GET, status);
    ok(rc == TWRC_SUCCESS, "Condition code not available, rc %d\n", rc);
}

static BOOL get_onevalue(TW_HANDLE hcontainer, TW_UINT32 *ret, TW_UINT16 *type)
{
    TW_ONEVALUE *onev;
    onev = GlobalLock(hcontainer);
    if (onev)
    {
        *ret = onev->Item;
        if (type)
            *type = onev->ItemType;
        GlobalUnlock(hcontainer);
        return TRUE;
    }
    return FALSE;
}

static TW_HANDLE alloc_and_set_onevalue(TW_UINT32 val, TW_UINT16 type)
{
    TW_HANDLE hcontainer;
    TW_ONEVALUE *onev;
    hcontainer = GlobalAlloc(0, sizeof(*onev));
    if (hcontainer)
    {
        onev = GlobalLock(hcontainer);
        if (onev)
        {
            onev->ItemType = type;
            onev->Item = val;
            GlobalUnlock(hcontainer);
        }
        else
        {
            GlobalFree(hcontainer);
            hcontainer = 0;
        }
    }
    return hcontainer;
}

static void check_get(TW_CAPABILITY *pCapability, TW_INT32 actual_support,
                TW_UINT32 orig_value, TW_UINT32 default_value, TW_UINT32 *suggested_set_value)
{
    void *p;
    if (suggested_set_value)
        *suggested_set_value = orig_value + 1;
    p = GlobalLock(pCapability->hContainer);
    if (p)
    {
        if (pCapability->ConType == TWON_ONEVALUE)
        {
            TW_ONEVALUE *onev = (TW_ONEVALUE *) p;
            ok(onev->Item == orig_value || !(actual_support & TWQC_GETCURRENT), "MSG_GET of 0x%x returned 0x%x, expecting 0x%x\n",
                pCapability->Cap, onev->Item, orig_value);
        }
        else if (pCapability->ConType == TWON_ENUMERATION)
        {
            int i;
            TW_UINT8 *p8;
            TW_UINT16 *p16;
            TW_UINT32 *p32;
            TW_ENUMERATION *enumv = (TW_ENUMERATION *) p;
            p8 = enumv->ItemList;
            p16 = (TW_UINT16 *) p8;
            p32 = (TW_UINT32 *) p8;
            trace("MSG_GET of 0x%x returned %d items:\n", pCapability->Cap, enumv->NumItems);
            for (i = 0; i < enumv->NumItems; i++)
            {
                if (enumv->ItemType == TWTY_UINT8 || enumv->ItemType == TWTY_INT8)
                    trace("  %d: 0x%x\n", i, p8[i]);
                if (enumv->ItemType == TWTY_UINT16 || enumv->ItemType == TWTY_INT16)
                    trace("  %d: 0x%x\n", i, p16[i]);
                if (enumv->ItemType == TWTY_UINT32 || enumv->ItemType == TWTY_INT32)
                    trace("  %d: 0x%x\n", i, p32[i]);
            }
            if (enumv->ItemType == TWTY_UINT16 || enumv->ItemType == TWTY_INT16)
            {
                ok(p16[enumv->CurrentIndex] == orig_value,
                    "Type 0x%x, values from MSG_GET (0x%x) and MSG_GETCURRENT (0x%x) do not match.\n",
                    pCapability->Cap, p16[enumv->CurrentIndex], orig_value);
                ok(p16[enumv->DefaultIndex] == default_value,
                    "Type 0x%x, values from MSG_GET (0x%x) and MSG_GETDEFAULT (0x%x) do not match.\n",
                    pCapability->Cap, p16[enumv->DefaultIndex], default_value);
                if (suggested_set_value)
                    *suggested_set_value = p16[(enumv->CurrentIndex + 1) % enumv->NumItems];
            }
            if (enumv->ItemType == TWTY_UINT32 || enumv->ItemType == TWTY_INT32)
            {
                ok(p32[enumv->CurrentIndex] == orig_value,
                    "Type 0x%x, values from MSG_GET (0x%x) and MSG_GETCURRENT (0x%x) do not match.\n",
                    pCapability->Cap, p32[enumv->CurrentIndex], orig_value);
                ok(p32[enumv->DefaultIndex] == default_value,
                    "Type 0x%x, values from MSG_GET (0x%x) and MSG_GETDEFAULT (0x%x) do not match.\n",
                    pCapability->Cap, p32[enumv->DefaultIndex], default_value);
                if (suggested_set_value)
                    *suggested_set_value = p32[(enumv->CurrentIndex + 1) % enumv->NumItems];
            }
        }
        else
            trace("MSG_GET on type 0x%x returned type 0x%x, which we didn't check.\n", pCapability->Cap, pCapability->ConType);
        GlobalUnlock(pCapability->hContainer);
    }
}

static void test_onevalue_cap(TW_IDENTITY *appid, TW_IDENTITY *source, TW_UINT16 captype, TW_UINT16 type, TW_INT32 minimum_support)
{
    TW_UINT16 rc;
    TW_UINT16 rtype;
    TW_STATUS status;
    TW_CAPABILITY cap;
    TW_UINT32 orig_value = 0;
    TW_UINT32 new_value;
    TW_UINT32 default_value = 0;
    TW_INT32 actual_support;

    memset(&cap, 0, sizeof(cap));
    cap.Cap = captype;
    cap.ConType = TWON_DONTCARE16;

    rc = pDSM_Entry(appid, source, DG_CONTROL, DAT_CAPABILITY, MSG_QUERYSUPPORT, &cap);
    get_condition_code(appid, source, &status);
    ok(rc == TWRC_SUCCESS && status.ConditionCode == TWCC_SUCCESS,
            "Error [rc %d|cc %d] doing MSG_QUERYSUPPORT for type 0x%x\n", rc, status.ConditionCode, captype);
    if (rc != TWRC_SUCCESS)
        return;
    ok(get_onevalue(cap.hContainer, (TW_UINT32 *) &actual_support, NULL), "Returned cap.hContainer invalid for QuerySupport on type 0x%x\n", captype);
    ok((actual_support & minimum_support) == minimum_support,
            "Error:  minimum support 0x%x for type 0x%x, got 0x%x\n", minimum_support,
            captype, actual_support);


    if (actual_support & TWQC_GETCURRENT)
    {
        memset(&cap, 0, sizeof(cap));
        cap.Cap = captype;
        cap.ConType = TWON_DONTCARE16;

        rc = pDSM_Entry(appid, source, DG_CONTROL, DAT_CAPABILITY, MSG_GETCURRENT, &cap);
        get_condition_code(appid, source, &status);
        ok(rc == TWRC_SUCCESS && status.ConditionCode == TWCC_SUCCESS,
                "Error [rc %d|cc %d] doing MSG_GETCURRENT for type 0x%x\n", rc, status.ConditionCode, captype);
        if (rc == TWRC_SUCCESS)
        {
            ok(get_onevalue(cap.hContainer, &orig_value, &rtype), "Returned cap.hContainer invalid for GETCURRENT on type 0x%x\n", captype);
            ok(rtype == type, "Returned GETCURRENT type 0x%x for cap 0x%x is not expected 0x%x\n", rtype, captype, type);
            GlobalFree(cap.hContainer);
        }
    }

    if (actual_support & TWQC_GETDEFAULT)
    {
        memset(&cap, 0, sizeof(cap));
        cap.Cap = captype;
        cap.ConType = TWON_DONTCARE16;

        rc = pDSM_Entry(appid, source, DG_CONTROL, DAT_CAPABILITY, MSG_GETDEFAULT, &cap);
        get_condition_code(appid, source, &status);
        ok(rc == TWRC_SUCCESS && status.ConditionCode == TWCC_SUCCESS,
                "Error [rc %d|cc %d] doing MSG_GETDEFAULT for type 0x%x\n", rc, status.ConditionCode, captype);
        if (rc == TWRC_SUCCESS)
        {
            ok(get_onevalue(cap.hContainer, &default_value, &rtype), "Returned cap.hContainer invalid for GETDEFAULT on type 0x%x\n", captype);
            ok(rtype == type, "Returned GETDEFAULT type 0x%x for cap 0x%x is not expected 0x%x\n", rtype, captype, type);
            GlobalFree(cap.hContainer);
        }
    }

    new_value = orig_value;
    if (actual_support & TWQC_GET)
    {
        memset(&cap, 0, sizeof(cap));
        cap.Cap = captype;
        cap.ConType = TWON_DONTCARE16;

        rc = pDSM_Entry(appid, source, DG_CONTROL, DAT_CAPABILITY, MSG_GET, &cap);
        get_condition_code(appid, source, &status);
        ok(rc == TWRC_SUCCESS && status.ConditionCode == TWCC_SUCCESS,
                "Error [rc %d|cc %d] doing MSG_GET for type 0x%x\n", rc, status.ConditionCode, captype);
        check_get(&cap, actual_support, orig_value, default_value, &new_value);
        if (rc == TWRC_SUCCESS)
            GlobalFree(cap.hContainer);
    }

    if (actual_support & TWQC_SET)
    {
        memset(&cap, 0, sizeof(cap));
        cap.Cap = captype;
        cap.ConType = TWON_ONEVALUE;
        cap.hContainer = alloc_and_set_onevalue(new_value, type);

        rc = pDSM_Entry(appid, source, DG_CONTROL, DAT_CAPABILITY, MSG_SET, &cap);
        get_condition_code(appid, source, &status);
        ok(rc == TWRC_SUCCESS && status.ConditionCode == TWCC_SUCCESS,
                "Error [rc %d|cc %d] doing MSG_SET for type 0x%x\n", rc, status.ConditionCode, captype);
        GlobalFree(cap.hContainer);
    }

    if (actual_support & TWQC_RESET)
    {
        memset(&cap, 0, sizeof(cap));
        cap.Cap = captype;
        cap.ConType = TWON_DONTCARE16;

        rc = pDSM_Entry(appid, source, DG_CONTROL, DAT_CAPABILITY, MSG_RESET, &cap);
        get_condition_code(appid, source, &status);
        ok(rc == TWRC_SUCCESS && status.ConditionCode == TWCC_SUCCESS,
                "Error [rc %d|cc %d] doing MSG_RESET for type 0x%x\n", rc, status.ConditionCode, captype);
        if (rc == TWRC_SUCCESS)
            GlobalFree(cap.hContainer);
    }
}

static void test_resolution(TW_IDENTITY *appid, TW_IDENTITY *source, TW_UINT16 captype, TW_INT32 minimum_support)
{
    TW_UINT16 rc;
    TW_STATUS status;
    TW_CAPABILITY cap;
    TW_UINT32 val;
    TW_UINT16 type;
    TW_INT32 actual_support;
    TW_FIX32 orig_value = { 0, 0 };
    TW_UINT32 new_value = 0;
    TW_FIX32 default_value = { 0, 0 };

    memset(&cap, 0, sizeof(cap));
    cap.Cap = captype;
    cap.ConType = TWON_DONTCARE16;

    rc = pDSM_Entry(appid, source, DG_CONTROL, DAT_CAPABILITY, MSG_QUERYSUPPORT, &cap);
    get_condition_code(appid, source, &status);
    ok(rc == TWRC_SUCCESS && status.ConditionCode == TWCC_SUCCESS,
            "Error [rc %d|cc %d] doing MSG_QUERYSUPPORT for type 0x%x\n", rc, status.ConditionCode, captype);
    if (rc != TWRC_SUCCESS)
        return;
    ok(get_onevalue(cap.hContainer, (TW_UINT32 *) &actual_support, NULL), "Returned cap.hContainer invalid for QuerySupport on type 0x%x\n", captype);
    ok((actual_support & minimum_support) == minimum_support,
            "Error:  minimum support 0x%x for type 0x%x, got 0x%x\n", minimum_support,
            captype, actual_support);


    if (actual_support & TWQC_GETCURRENT)
    {
        memset(&cap, 0, sizeof(cap));
        cap.Cap = captype;
        cap.ConType = TWON_DONTCARE16;

        rc = pDSM_Entry(appid, source, DG_CONTROL, DAT_CAPABILITY, MSG_GETCURRENT, &cap);
        get_condition_code(appid, source, &status);
        ok(rc == TWRC_SUCCESS && status.ConditionCode == TWCC_SUCCESS,
                "Error [rc %d|cc %d] doing MSG_GETCURRENT for type 0x%x\n", rc, status.ConditionCode, captype);
        if (rc == TWRC_SUCCESS)
        {
            get_onevalue(cap.hContainer, &val, &type);
            ok(type == TWTY_FIX32, "GETCURRENT for RESOLUTION is not type FIX32, is type %d\n", type);
            memcpy(&orig_value, &val, sizeof(orig_value));
            GlobalFree(cap.hContainer);
        }
    }

    if (actual_support & TWQC_GETDEFAULT)
    {
        memset(&cap, 0, sizeof(cap));
        cap.Cap = captype;
        cap.ConType = TWON_DONTCARE16;

        rc = pDSM_Entry(appid, source, DG_CONTROL, DAT_CAPABILITY, MSG_GETDEFAULT, &cap);
        get_condition_code(appid, source, &status);
        ok(rc == TWRC_SUCCESS && status.ConditionCode == TWCC_SUCCESS,
                "Error [rc %d|cc %d] doing MSG_GETDEFAULT for type 0x%x\n", rc, status.ConditionCode, captype);
        if (rc == TWRC_SUCCESS)
        {
            ok(type == TWTY_FIX32, "GETDEFAULT for RESOLUTION is not type FIX32, is type %d\n", type);
            memcpy(&default_value, &val, sizeof(default_value));
            GlobalFree(cap.hContainer);
        }
    }

    if (actual_support & TWQC_GET)
    {
        memset(&cap, 0, sizeof(cap));
        cap.Cap = captype;
        cap.ConType = TWON_DONTCARE16;

        rc = pDSM_Entry(appid, source, DG_CONTROL, DAT_CAPABILITY, MSG_GET, &cap);
        get_condition_code(appid, source, &status);
        ok(rc == TWRC_SUCCESS && status.ConditionCode == TWCC_SUCCESS,
                "Error [rc %d|cc %d] doing MSG_GET for type 0x%x\n", rc, status.ConditionCode, captype);
        if (rc == TWRC_SUCCESS)
        {
            TW_RANGE *range;
            ok(cap.ConType == TWON_RANGE, "MSG_GET for ICAP_[XY]RESOLUTION did not return TWON_RANGE, but %d\n", cap.ConType);
            range = GlobalLock(cap.hContainer);
            trace("MSG_GET of 0x%x returned [ItemType %d|MinValue %d|MaxValue %d|StepSize %d|DefaultValue %d|CurrentValue %d]:\n",
                    cap.Cap, range->ItemType, range->MinValue, range->MaxValue, range->StepSize,
                    range->DefaultValue, range->CurrentValue);
            for (new_value = range->MinValue; new_value < range->MaxValue; new_value += range->StepSize)
                if (new_value != range->CurrentValue)
                    break;
            GlobalUnlock(cap.hContainer);
            GlobalFree(cap.hContainer);
        }
    }

    if (actual_support & TWQC_SET)
    {
        memset(&cap, 0, sizeof(cap));
        cap.Cap = captype;
        cap.ConType = TWON_ONEVALUE;
        cap.hContainer = alloc_and_set_onevalue(new_value, TWTY_FIX32);

        rc = pDSM_Entry(appid, source, DG_CONTROL, DAT_CAPABILITY, MSG_SET, &cap);
        get_condition_code(appid, source, &status);
        ok(rc == TWRC_SUCCESS && status.ConditionCode == TWCC_SUCCESS,
                "Error [rc %d|cc %d] doing MSG_SET for type 0x%x\n", rc, status.ConditionCode, captype);
        GlobalFree(cap.hContainer);

    }

    if (actual_support & TWQC_RESET)
    {
        memset(&cap, 0, sizeof(cap));
        cap.Cap = captype;
        cap.ConType = TWON_DONTCARE16;

        rc = pDSM_Entry(appid, source, DG_CONTROL, DAT_CAPABILITY, MSG_RESET, &cap);
        get_condition_code(appid, source, &status);
        ok(rc == TWRC_SUCCESS && status.ConditionCode == TWCC_SUCCESS,
                "Error [rc %d|cc %d] doing MSG_RESET for type 0x%x\n", rc, status.ConditionCode, captype);
        if (rc == TWRC_SUCCESS)
            GlobalFree(cap.hContainer);
    }
}


static void test_single_source(TW_IDENTITY *appid, TW_IDENTITY *source)
{
    TW_UINT16 rc;
    TW_STATUS status;
    TW_CAPABILITY cap;
    UINT16 capabilities[CAP_CUSTOMBASE];

    memset(&cap, 0, sizeof(cap));
    cap.Cap = CAP_SUPPORTEDCAPS;
    cap.ConType = TWON_DONTCARE16;

    rc = pDSM_Entry(appid, source, DG_CONTROL, DAT_CAPABILITY, MSG_GET, &cap);
    get_condition_code(appid, source, &status);
    ok(rc == TWRC_SUCCESS || status.ConditionCode == TWCC_SUCCESS,
            "Error obtaining CAP_SUPPORTEDCAPS\n");

    memset(capabilities, 0, sizeof(capabilities));
    if (rc == TWRC_SUCCESS && cap.ConType == TWON_ARRAY)
    {
        TW_ARRAY *a;
        a = GlobalLock(cap.hContainer);
        if (a)
        {
            if (a->ItemType == TWTY_UINT16)
            {
                int i;
                UINT16 *u = (UINT16 *) a->ItemList;
                trace("%d Capabilities:\n", a->NumItems);
                for (i = 0; i < a->NumItems; i++)
                    if (u[i] < sizeof(capabilities) / sizeof(capabilities[0]))
                    {
                        capabilities[u[i]] = 1;
                        trace("  %d: 0x%x\n", i, u[i]);
                    }
            }
            GlobalUnlock(cap.hContainer);
        }
    }

    /* For Twain 1.6, all sources must support: */
    ok(capabilities[CAP_SUPPORTEDCAPS], "CAP_SUPPORTEDCAPS not supported\n");
    ok(capabilities[CAP_XFERCOUNT], "CAP_XFERCOUNT not supported\n");
    if (capabilities[CAP_XFERCOUNT])
        test_onevalue_cap(appid, source, CAP_XFERCOUNT, TWTY_INT16,
            TWQC_GET | TWQC_SET | TWQC_GETDEFAULT | TWQC_GETCURRENT | TWQC_RESET);
    ok(capabilities[CAP_UICONTROLLABLE], "CAP_UICONTROLLABLE not supported\n");
    if (capabilities[CAP_UICONTROLLABLE])
        test_onevalue_cap(appid, source, CAP_UICONTROLLABLE, TWTY_BOOL, TWQC_GET);

    if (source->SupportedGroups & DG_IMAGE)
    {
        /* For Twain 1.6:
            Sources that supply image information must support DG_CONTROL / DAT_CAPABILITY /
            MSG_GET, MSG_GETCURRENT, MSG_GETDEFAULT on:
        */
        ok(capabilities[ICAP_COMPRESSION], "ICAP_COMPRESSION not supported\n");
        if (capabilities[ICAP_COMPRESSION])
            test_onevalue_cap(appid, source, ICAP_COMPRESSION, TWTY_UINT16,
                TWQC_GET | TWQC_GETDEFAULT | TWQC_GETCURRENT);
        todo_wine
        ok(capabilities[ICAP_PLANARCHUNKY], "ICAP_PLANARCHUNKY not supported\n");
        todo_wine
        ok(capabilities[ICAP_PHYSICALHEIGHT], "ICAP_PHYSICALHEIGHT not supported\n");
        todo_wine
        ok(capabilities[ICAP_PHYSICALWIDTH], "ICAP_PHYSICALWIDTH not supported\n");
        ok(capabilities[ICAP_PIXELFLAVOR], "ICAP_PIXELFLAVOR not supported\n");
        if (capabilities[ICAP_PIXELFLAVOR])
            test_onevalue_cap(appid, source, ICAP_PIXELFLAVOR, TWTY_UINT16,
                TWQC_GET | TWQC_SET | TWQC_GETDEFAULT | TWQC_GETCURRENT | TWQC_RESET);

        /* For Twain 1.6:
            Sources that supply image information must support DG_CONTROL / DAT_CAPABILITY /
            MSG_GET, MSG_GETCURRENT, MSG_GETDEFAULT, MSG_RESET and MSG_SET on:
        */
        todo_wine
        ok(capabilities[ICAP_BITDEPTH], "ICAP_BITDEPTH not supported\n");
        todo_wine
        ok(capabilities[ICAP_BITORDER], "ICAP_BITORDER not supported\n");
        ok(capabilities[ICAP_PIXELTYPE], "ICAP_PIXELTYPE not supported\n");
        if (capabilities[ICAP_PIXELTYPE])
            test_onevalue_cap(appid, source, ICAP_PIXELTYPE, TWTY_UINT16,
                TWQC_GET | TWQC_SET | TWQC_GETDEFAULT | TWQC_GETCURRENT | TWQC_RESET);
        todo_wine
        ok(capabilities[ICAP_UNITS], "ICAP_UNITS not supported\n");
        ok(capabilities[ICAP_XFERMECH], "ICAP_XFERMECH not supported\n");
        if (capabilities[ICAP_XFERMECH])
            test_onevalue_cap(appid, source, ICAP_XFERMECH, TWTY_UINT16,
                TWQC_GET | TWQC_SET | TWQC_GETDEFAULT | TWQC_GETCURRENT | TWQC_RESET);
        ok(capabilities[ICAP_XRESOLUTION], "ICAP_XRESOLUTION not supported\n");
        if (capabilities[ICAP_XRESOLUTION])
            test_resolution(appid, source, ICAP_XRESOLUTION,
                TWQC_GET | TWQC_SET | TWQC_GETDEFAULT | TWQC_GETCURRENT | TWQC_RESET);
        ok(capabilities[ICAP_YRESOLUTION], "ICAP_YRESOLUTION not supported\n");
        if (capabilities[ICAP_YRESOLUTION])
            test_resolution(appid, source, ICAP_YRESOLUTION,
                TWQC_GET | TWQC_SET | TWQC_GETDEFAULT | TWQC_GETCURRENT | TWQC_RESET);
    }
}

static void test_sources(TW_IDENTITY *appid)
{
    TW_UINT16 rc;
    TW_IDENTITY source;
    TW_STATUS status;
    int scannercount = 0;

    memset(&source, 0, sizeof(source));
    rc = pDSM_Entry(appid, NULL, DG_CONTROL, DAT_IDENTITY, MSG_GETFIRST, &source);
    get_condition_code(appid, NULL, &status);
    ok( (rc == TWRC_SUCCESS && status.ConditionCode == TWCC_SUCCESS) ||
        (rc == TWRC_FAILURE && status.ConditionCode == TWCC_NODS),
            "Get first invalid condition code, rc %d, cc %d\n", rc, status.ConditionCode);

    while (rc == TWRC_SUCCESS)
    {
        scannercount++;
        trace("[Scanner %d|Version %d.%d(%s)|Protocol %d.%d|SupportedGroups 0x%x|Manufacturer %s|Family %s|ProductName %s]\n",
            scannercount,
            source.Version.MajorNum, source.Version.MinorNum, source.Version.Info,
            source.ProtocolMajor, source.ProtocolMinor, source.SupportedGroups,
            source.Manufacturer, source.ProductFamily, source.ProductName);
        memset(&source, 0, sizeof(source));
        rc = pDSM_Entry(appid, NULL, DG_CONTROL, DAT_IDENTITY, MSG_GETNEXT, &source);
        get_condition_code(appid, NULL, &status);
        ok(rc == TWRC_SUCCESS || rc == TWRC_ENDOFLIST, "Get next source failed, rc %d, cc %d\n", rc, status.ConditionCode);
    }

    memset(&source, 0, sizeof(source));
    rc = pDSM_Entry(appid, NULL, DG_CONTROL, DAT_IDENTITY, MSG_GETDEFAULT, &source);
    get_condition_code(appid, NULL, &status);
    ok( (rc == TWRC_SUCCESS && status.ConditionCode == TWCC_SUCCESS) ||
        (rc == TWRC_FAILURE && status.ConditionCode == TWCC_NODS),
            "Get default invalid condition code, rc %d, cc %d\n", rc, status.ConditionCode);

    if (rc == TWRC_SUCCESS && status.ConditionCode == TWCC_SUCCESS)
    {
        rc = pDSM_Entry(appid, NULL, DG_CONTROL, DAT_IDENTITY, MSG_OPENDS, &source);
        get_condition_code(appid, NULL, &status);

        if (rc == TWRC_SUCCESS && status.ConditionCode == TWCC_SUCCESS)
        {
            rc = pDSM_Entry(appid, NULL, DG_CONTROL, DAT_IDENTITY, MSG_CLOSEDS, &source);
            get_condition_code(appid, NULL, &status);
            ok(rc == TWRC_SUCCESS, "Close DS Failed, rc %d, cc %d\n", rc, status.ConditionCode);
        }
    }

    if (winetest_interactive)
    {
        trace("Interactive, so trying userselect\n");
        memset(&source, 0, sizeof(source));
        rc = pDSM_Entry(appid, NULL, DG_CONTROL, DAT_IDENTITY, MSG_USERSELECT, &source);
        get_condition_code(appid, NULL, &status);
        ok(rc == TWRC_SUCCESS || rc == TWRC_CANCEL, "Userselect failed, rc %d, cc %d\n", rc, status.ConditionCode);

        if (rc == TWRC_SUCCESS && status.ConditionCode == TWCC_SUCCESS)
        {
            rc = pDSM_Entry(appid, NULL, DG_CONTROL, DAT_IDENTITY, MSG_OPENDS, &source);
            get_condition_code(appid, NULL, &status);
            if (rc == TWRC_SUCCESS && status.ConditionCode == TWCC_SUCCESS)
            {
                test_single_source(appid, &source);
                rc = pDSM_Entry(appid, NULL, DG_CONTROL, DAT_IDENTITY, MSG_CLOSEDS, &source);
                get_condition_code(appid, NULL, &status);
                ok(rc == TWRC_SUCCESS, "Close DS Failed, rc %d, cc %d\n", rc, status.ConditionCode);
            }
        }
    }

}

START_TEST(dsm)
{
    TW_IDENTITY appid;
    TW_UINT16 rc;
    HANDLE hwnd;
    HMODULE htwain;

    if (!dsm_RegisterWindowClasses()) assert(0);

    htwain = LoadLibraryA("twain_32.dll");
    if (! htwain)
    {
        skip("twain_32.dll not available, skipping tests\n");
        return;
    }
    pDSM_Entry = (void*)GetProcAddress(htwain, "DSM_Entry");
    ok(pDSM_Entry != NULL, "Unable to GetProcAddress DSM_Entry\n");
    if (! pDSM_Entry)
    {
        skip("DSM_Entry not available, skipping tests\n");
        return;
    }

    memset(&appid, 0, sizeof(appid));
    appid.Version.Language = TWLG_ENGLISH_USA;
    appid.Version.Country = TWCY_USA;
    appid.ProtocolMajor = TWON_PROTOCOLMAJOR;
    appid.ProtocolMinor = TWON_PROTOCOLMINOR;
    appid.SupportedGroups = DG_CONTROL | DG_IMAGE;

    hwnd = CreateWindow("TWAIN_dsm_class", "Twain Test", 0,
                        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                        NULL, NULL, GetModuleHandleA(0), NULL);

    rc = pDSM_Entry(&appid, NULL, DG_CONTROL, DAT_PARENT, MSG_OPENDSM, (TW_MEMREF) &hwnd);
    ok(rc == TWRC_SUCCESS, "MSG_OPENDSM returned %d\n", rc);

    test_sources(&appid);

    rc = pDSM_Entry(&appid, NULL, DG_CONTROL, DAT_PARENT, MSG_CLOSEDSM, (TW_MEMREF) &hwnd);
    ok(rc == TWRC_SUCCESS, "MSG_CLOSEDSM returned %d\n", rc);

    DestroyWindow(hwnd);
    FreeLibrary(htwain);
}
