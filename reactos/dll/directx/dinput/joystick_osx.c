/*  DirectInput Joystick device for Mac OS/X
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998,1999 Lionel Ulmer
 * Copyright 2000-2001 TransGaming Technologies Inc.
 * Copyright 2009 CodeWeavers, Aric Stewart
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

#include <config.h>
//#include "wine/port.h"

#if defined(HAVE_IOKIT_HID_IOHIDLIB_H)
#define ULONG __carbon_ULONG
#define E_INVALIDARG __carbon_E_INVALIDARG
#define E_OUTOFMEMORY __carbon_E_OUTOFMEMORY
#define E_HANDLE __carbon_E_HANDLE
#define E_ACCESSDENIED __carbon_E_ACCESSDENIED
#define E_UNEXPECTED __carbon_E_UNEXPECTED
#define E_FAIL __carbon_E_FAIL
#define E_ABORT __carbon_E_ABORT
#define E_POINTER __carbon_E_POINTER
#define E_NOINTERFACE __carbon_E_NOINTERFACE
#define E_NOTIMPL __carbon_E_NOTIMPL
#define S_FALSE __carbon_S_FALSE
#define S_OK __carbon_S_OK
#define HRESULT_FACILITY __carbon_HRESULT_FACILITY
#define IS_ERROR __carbon_IS_ERROR
#define FAILED __carbon_FAILED
#define SUCCEEDED __carbon_SUCCEEDED
#define MAKE_HRESULT __carbon_MAKE_HRESULT
#define HRESULT __carbon_HRESULT
#define STDMETHODCALLTYPE __carbon_STDMETHODCALLTYPE
#include <IOKit/hid/IOHIDLib.h>
#undef ULONG
#undef E_INVALIDARG
#undef E_OUTOFMEMORY
#undef E_HANDLE
#undef E_ACCESSDENIED
#undef E_UNEXPECTED
#undef E_FAIL
#undef E_ABORT
#undef E_POINTER
#undef E_NOINTERFACE
#undef E_NOTIMPL
#undef S_FALSE
#undef S_OK
#undef HRESULT_FACILITY
#undef IS_ERROR
#undef FAILED
#undef SUCCEEDED
#undef MAKE_HRESULT
#undef HRESULT
#undef STDMETHODCALLTYPE
#endif /* HAVE_IOKIT_HID_IOHIDLIB_H */

//#include "wine/debug.h"
//#include "wine/unicode.h"
//#include "windef.h"
//#include "winbase.h"
//#include "winerror.h"
//#include "winreg.h"
//#include "dinput.h"

#include "dinput_private.h"
//#include "device_private.h"
//#include "joystick_private.h"

#ifdef HAVE_IOHIDMANAGERCREATE

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

static IOHIDManagerRef gIOHIDManagerRef = NULL;
static CFArrayRef gCollections = NULL;

typedef struct JoystickImpl JoystickImpl;
static const IDirectInputDevice8AVtbl JoystickAvt;
static const IDirectInputDevice8WVtbl JoystickWvt;

struct JoystickImpl
{
    struct JoystickGenericImpl generic;

    /* osx private */
    int                    id;
    CFMutableArrayRef      elementCFArrayRef;
    ObjProps               **propmap;
};

static inline JoystickImpl *impl_from_IDirectInputDevice8A(IDirectInputDevice8A *iface)
{
    return CONTAINING_RECORD(CONTAINING_RECORD(CONTAINING_RECORD(iface, IDirectInputDeviceImpl, IDirectInputDevice8A_iface),
           JoystickGenericImpl, base), JoystickImpl, generic);
}
static inline JoystickImpl *impl_from_IDirectInputDevice8W(IDirectInputDevice8W *iface)
{
    return CONTAINING_RECORD(CONTAINING_RECORD(CONTAINING_RECORD(iface, IDirectInputDeviceImpl, IDirectInputDevice8W_iface),
           JoystickGenericImpl, base), JoystickImpl, generic);
}

static const GUID DInput_Wine_OsX_Joystick_GUID = { /* 59CAD8F6-E617-41E2-8EB7-47B23EEEDC5A */
  0x59CAD8F6, 0xE617, 0x41E2, {0x8E, 0xB7, 0x47, 0xB2, 0x3E, 0xEE, 0xDC, 0x5A}
};

static void CFSetApplierFunctionCopyToCFArray(const void *value, void *context)
{
    CFArrayAppendValue( ( CFMutableArrayRef ) context, value );
}

static CFMutableDictionaryRef creates_osx_device_match(int usage)
{
    CFMutableDictionaryRef result;

    result = CFDictionaryCreateMutable( kCFAllocatorDefault, 0,
            &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );

    if ( result )
    {
        int number = kHIDPage_GenericDesktop;
        CFNumberRef pageCFNumberRef = CFNumberCreate( kCFAllocatorDefault,
                          kCFNumberIntType, &number);

        if ( pageCFNumberRef )
        {
            CFNumberRef usageCFNumberRef;

            CFDictionarySetValue( result, CFSTR( kIOHIDDeviceUsagePageKey ),
                pageCFNumberRef );
            CFRelease( pageCFNumberRef );

            usageCFNumberRef = CFNumberCreate( kCFAllocatorDefault,
                        kCFNumberIntType, &usage);
            if ( usageCFNumberRef )
            {
                CFDictionarySetValue( result, CFSTR( kIOHIDDeviceUsageKey ),
                    usageCFNumberRef );
                CFRelease( usageCFNumberRef );
            }
            else
            {
                ERR("CFNumberCreate() failed.\n");
                return NULL;
            }
        }
        else
        {
            ERR("CFNumberCreate failed.\n");
            return NULL;
        }
    }
    else
    {
        ERR("CFDictionaryCreateMutable failed.\n");
        return NULL;
    }

    return result;
}

static CFIndex find_top_level(IOHIDDeviceRef tIOHIDDeviceRef, CFArrayRef topLevels)
{
    CFArrayRef      gElementCFArrayRef;
    CFIndex         numTops = 0;

    if (!tIOHIDDeviceRef)
        return 0;

    gElementCFArrayRef = IOHIDDeviceCopyMatchingElements(tIOHIDDeviceRef, NULL, 0);

    if (gElementCFArrayRef)
    {
        CFIndex idx, cnt = CFArrayGetCount(gElementCFArrayRef);
        for (idx=0; idx<cnt; idx++)
        {
            IOHIDElementRef tIOHIDElementRef = (IOHIDElementRef)CFArrayGetValueAtIndex(gElementCFArrayRef, idx);
            int eleType = IOHIDElementGetType(tIOHIDElementRef);

            /* Check for top-level gaming device collections */
            if (eleType == kIOHIDElementTypeCollection && IOHIDElementGetParent(tIOHIDElementRef) == 0)
            {
                int tUsagePage = IOHIDElementGetUsagePage(tIOHIDElementRef);
                int tUsage = IOHIDElementGetUsage(tIOHIDElementRef);

                if (tUsagePage == kHIDPage_GenericDesktop &&
                     (tUsage == kHIDUsage_GD_Joystick || tUsage == kHIDUsage_GD_GamePad))
                {
                    CFArrayAppendValue((CFMutableArrayRef)topLevels, tIOHIDElementRef);
                    numTops++;
                }
            }
        }
    }
    return numTops;
}

static void get_element_children(IOHIDElementRef tElement, CFArrayRef childElements)
{
    CFIndex    idx, cnt;
    CFArrayRef tElementChildrenArray = IOHIDElementGetChildren(tElement);

    cnt = CFArrayGetCount(tElementChildrenArray);
    if (cnt < 1)
        return;

    /* Either add the element to the array or grab its children */
    for (idx=0; idx<cnt; idx++)
    {
        IOHIDElementRef tChildElementRef;

        tChildElementRef = (IOHIDElementRef)CFArrayGetValueAtIndex(tElementChildrenArray, idx);
        if (IOHIDElementGetType(tChildElementRef) == kIOHIDElementTypeCollection)
            get_element_children(tChildElementRef, childElements);
        else
            CFArrayAppendValue((CFMutableArrayRef)childElements, tChildElementRef);
    }
}

static int find_osx_devices(void)
{
    IOReturn tIOReturn;
    CFMutableDictionaryRef result;
    CFSetRef devset;
    CFArrayRef matching;

    gIOHIDManagerRef = IOHIDManagerCreate( kCFAllocatorDefault, 0L );
    tIOReturn = IOHIDManagerOpen( gIOHIDManagerRef, 0L);
    if ( kIOReturnSuccess != tIOReturn )
    {
        ERR("Couldn't open IOHIDManager.\n");
        return 0;
    }

     matching = CFArrayCreateMutable( kCFAllocatorDefault, 0,
                        &kCFTypeArrayCallBacks );

    /* build matching dictionary */
    result = creates_osx_device_match(kHIDUsage_GD_Joystick);
    if (!result)
    {
        CFRelease(matching);
        return 0;
    }
    CFArrayAppendValue( ( CFMutableArrayRef )matching, result );
    result = creates_osx_device_match(kHIDUsage_GD_GamePad);
    if (!result)
    {
        CFRelease(matching);
        return 0;
    }
    CFArrayAppendValue( ( CFMutableArrayRef )matching, result );

    IOHIDManagerSetDeviceMatchingMultiple( gIOHIDManagerRef, matching);
    devset = IOHIDManagerCopyDevices( gIOHIDManagerRef );
    if (devset)
    {
        CFIndex countDevices, countCollections, idx;
        CFArrayRef gDevices = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
        CFSetApplyFunction(devset, CFSetApplierFunctionCopyToCFArray, (void*)gDevices);
        CFRelease( devset);
        countDevices = CFArrayGetCount(gDevices);

        gCollections = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
        if (!gCollections)
            return 0;

        countCollections = 0;
        for (idx = 0; idx < countDevices; idx++)
        {
            CFIndex tTop;
            IOHIDDeviceRef tDevice;

            tDevice = (IOHIDDeviceRef) CFArrayGetValueAtIndex(gDevices, idx);
            tTop = find_top_level(tDevice, gCollections);
            countCollections += tTop;
        }

        CFRelease(gDevices);

        TRACE("found %i device(s), %i collection(s)\n",(int)countDevices,(int)countCollections);
        return (int)countCollections;
    }
    return 0;
}

static int get_osx_device_name(int id, char *name, int length)
{
    CFStringRef str;
    IOHIDElementRef tIOHIDElementRef;
    IOHIDDeviceRef tIOHIDDeviceRef;

    if (!gCollections)
        return 0;

    tIOHIDElementRef = (IOHIDElementRef)CFArrayGetValueAtIndex(gCollections, id);

    if (!tIOHIDElementRef)
    {
        ERR("Invalid Element requested %i\n",id);
        return 0;
    }

    tIOHIDDeviceRef = IOHIDElementGetDevice(tIOHIDElementRef);

    if (name)
        name[0] = 0;

    if (!tIOHIDDeviceRef)
    {
        ERR("Invalid Device requested %i\n",id);
        return 0;
    }

    str = IOHIDDeviceGetProperty(tIOHIDDeviceRef, CFSTR( kIOHIDProductKey ));
    if (str)
    {
        CFIndex len = CFStringGetLength(str);
        if (length >= len)
        {
            CFStringGetCString(str,name,length,kCFStringEncodingASCII);
            return len;
        }
        else
            return (len+1);
    }
    return 0;
}

static void insert_sort_button(int header, IOHIDElementRef tIOHIDElementRef,
                                CFMutableArrayRef elementCFArrayRef, int index,
                                int target)
{
    IOHIDElementRef targetElement;
    int usage;

    CFArraySetValueAtIndex(elementCFArrayRef, header+index, NULL);
    targetElement = ( IOHIDElementRef ) CFArrayGetValueAtIndex( elementCFArrayRef, header+target);
    if (targetElement == NULL)
    {
        CFArraySetValueAtIndex(elementCFArrayRef, header+target,tIOHIDElementRef);
        return;
    }
    usage = IOHIDElementGetUsage( targetElement );
    usage --; /* usage 1 based index */

    insert_sort_button(header, targetElement, elementCFArrayRef, target, usage);
    CFArraySetValueAtIndex(elementCFArrayRef, header+target,tIOHIDElementRef);
}

static void get_osx_device_elements(JoystickImpl *device, int axis_map[8])
{
    IOHIDElementRef tIOHIDElementRef;
    CFArrayRef      gElementCFArrayRef;
    DWORD           axes = 0;
    DWORD           sliders = 0;
    DWORD           buttons = 0;
    DWORD           povs = 0;

    device->elementCFArrayRef = NULL;

    if (!gCollections)
        return;

    tIOHIDElementRef = (IOHIDElementRef)CFArrayGetValueAtIndex(gCollections, device->id);

    if (!tIOHIDElementRef)
        return;

    gElementCFArrayRef = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
    get_element_children(tIOHIDElementRef, gElementCFArrayRef);

    if (gElementCFArrayRef)
    {
        CFIndex idx, cnt = CFArrayGetCount( gElementCFArrayRef );
        /* build our element array in the order that dinput expects */
        device->elementCFArrayRef = CFArrayCreateMutable(NULL,0,NULL);

        for ( idx = 0; idx < cnt; idx++ )
        {
            IOHIDElementRef tIOHIDElementRef = ( IOHIDElementRef ) CFArrayGetValueAtIndex( gElementCFArrayRef, idx );
            int eleType = IOHIDElementGetType( tIOHIDElementRef );
            switch(eleType)
            {
                case kIOHIDElementTypeInput_Button:
                {
                    int usagePage = IOHIDElementGetUsagePage( tIOHIDElementRef );
                    if (usagePage != kHIDPage_Button)
                    {
                        /* avoid strange elements found on the 360 controller */
                        continue;
                    }

                    if (buttons < 128)
                    {
                        CFArrayInsertValueAtIndex(device->elementCFArrayRef, (axes+povs+buttons), tIOHIDElementRef);
                        buttons++;
                    }
                    break;
                }
                case kIOHIDElementTypeInput_Axis:
                {
                    CFArrayInsertValueAtIndex(device->elementCFArrayRef, axes, tIOHIDElementRef);
                    axes++;
                    break;
                }
                case kIOHIDElementTypeInput_Misc:
                {
                    uint32_t usage = IOHIDElementGetUsage( tIOHIDElementRef );
                    switch(usage)
                    {
                        case kHIDUsage_GD_Hatswitch:
                        {
                            CFArrayInsertValueAtIndex(device->elementCFArrayRef, (axes+povs), tIOHIDElementRef);
                            povs++;
                            break;
                        }
                        case kHIDUsage_GD_Slider:
                            sliders ++;
                            if (sliders > 2)
                                break;
                            /* fallthrough, sliders are axis */
                        case kHIDUsage_GD_X:
                        case kHIDUsage_GD_Y:
                        case kHIDUsage_GD_Z:
                        case kHIDUsage_GD_Rx:
                        case kHIDUsage_GD_Ry:
                        case kHIDUsage_GD_Rz:
                        {
                            CFArrayInsertValueAtIndex(device->elementCFArrayRef, axes, tIOHIDElementRef);
                            axis_map[axes]=usage;
                            axes++;
                            break;
                        }
                        default:
                            FIXME("Unhandled usage %i\n",usage);
                    }
                    break;
                }
                default:
                    FIXME("Unhandled type %i\n",eleType);
            }
        }
    }

    device->generic.devcaps.dwAxes = axes;
    device->generic.devcaps.dwButtons = buttons;
    device->generic.devcaps.dwPOVs = povs;

    /* Sort buttons into correct order */
    for (buttons = 0; buttons < device->generic.devcaps.dwButtons; buttons++)
    {
        IOHIDElementRef tIOHIDElementRef = ( IOHIDElementRef ) CFArrayGetValueAtIndex( device->elementCFArrayRef, axes+povs+buttons);
        uint32_t usage = IOHIDElementGetUsage( tIOHIDElementRef );
        usage --; /* usage is 1 indexed we need 0 indexed */
        if (usage == buttons)
            continue;

        insert_sort_button(axes+povs, tIOHIDElementRef, device->elementCFArrayRef,buttons,usage);
    }
}

static void get_osx_device_elements_props(JoystickImpl *device)
{
    CFArrayRef gElementCFArrayRef = device->elementCFArrayRef;

    if (gElementCFArrayRef)
    {
        CFIndex idx, cnt = CFArrayGetCount( gElementCFArrayRef );

        for ( idx = 0; idx < cnt; idx++ )
        {
            IOHIDElementRef tIOHIDElementRef = ( IOHIDElementRef ) CFArrayGetValueAtIndex( gElementCFArrayRef, idx );

            device->generic.props[idx].lDevMin = IOHIDElementGetLogicalMin(tIOHIDElementRef);
            device->generic.props[idx].lDevMax = IOHIDElementGetLogicalMax(tIOHIDElementRef);
            device->generic.props[idx].lMin =  0;
            device->generic.props[idx].lMax =  0xffff;
            device->generic.props[idx].lDeadZone = 0;
            device->generic.props[idx].lSaturation = 0;
        }
    }
}

static void poll_osx_device_state(LPDIRECTINPUTDEVICE8A iface)
{
    JoystickImpl *device = impl_from_IDirectInputDevice8A(iface);
    IOHIDElementRef tIOHIDTopElementRef;
    IOHIDDeviceRef tIOHIDDeviceRef;
    CFArrayRef gElementCFArrayRef = device->elementCFArrayRef;

    TRACE("polling device %i\n",device->id);

    if (!gCollections)
        return;

    tIOHIDTopElementRef = (IOHIDElementRef) CFArrayGetValueAtIndex(gCollections, device->id);
    tIOHIDDeviceRef = IOHIDElementGetDevice(tIOHIDTopElementRef);

    if (!tIOHIDDeviceRef)
        return;

    if (gElementCFArrayRef)
    {
        int button_idx = 0;
        int pov_idx = 0;
        int slider_idx = 0;
        int inst_id;
        CFIndex idx, cnt = CFArrayGetCount( gElementCFArrayRef );

        for ( idx = 0; idx < cnt; idx++ )
        {
            IOHIDValueRef valueRef;
            int val, oldVal, newVal;
            IOHIDElementRef tIOHIDElementRef = ( IOHIDElementRef ) CFArrayGetValueAtIndex( gElementCFArrayRef, idx );
            int eleType = IOHIDElementGetType( tIOHIDElementRef );

            switch(eleType)
            {
                case kIOHIDElementTypeInput_Button:
                    if(button_idx < 128)
                    {
                        IOHIDDeviceGetValue(tIOHIDDeviceRef, tIOHIDElementRef, &valueRef);
                        val = IOHIDValueGetIntegerValue(valueRef);
                        newVal = val ? 0x80 : 0x0;
                        oldVal = device->generic.js.rgbButtons[button_idx];
                        device->generic.js.rgbButtons[button_idx] = newVal;
                        if (oldVal != newVal)
                        {
                            inst_id = DIDFT_MAKEINSTANCE(button_idx) | DIDFT_PSHBUTTON;
                            queue_event(iface,inst_id,newVal,GetCurrentTime(),device->generic.base.dinput->evsequence++);
                        }
                        button_idx ++;
                    }
                    break;
                case kIOHIDElementTypeInput_Misc:
                {
                    uint32_t usage = IOHIDElementGetUsage( tIOHIDElementRef );
                    switch(usage)
                    {
                        case kHIDUsage_GD_Hatswitch:
                        {
                            IOHIDDeviceGetValue(tIOHIDDeviceRef, tIOHIDElementRef, &valueRef);
                            val = IOHIDValueGetIntegerValue(valueRef);
                            oldVal = device->generic.js.rgdwPOV[pov_idx];
                            if (val >= 8)
                                newVal = -1;
                            else
                                newVal = val * 4500;
                            device->generic.js.rgdwPOV[pov_idx] = newVal;
                            if (oldVal != newVal)
                            {
                                inst_id = DIDFT_MAKEINSTANCE(pov_idx) | DIDFT_POV;
                                queue_event(iface,inst_id,newVal,GetCurrentTime(),device->generic.base.dinput->evsequence++);
                            }
                            pov_idx ++;
                            break;
                        }
                        case kHIDUsage_GD_X:
                        case kHIDUsage_GD_Y:
                        case kHIDUsage_GD_Z:
                        case kHIDUsage_GD_Rx:
                        case kHIDUsage_GD_Ry:
                        case kHIDUsage_GD_Rz:
                        case kHIDUsage_GD_Slider:
                        {
                            int wine_obj = -1;

                            IOHIDDeviceGetValue(tIOHIDDeviceRef, tIOHIDElementRef, &valueRef);
                            val = IOHIDValueGetIntegerValue(valueRef);
                            newVal = joystick_map_axis(&device->generic.props[idx], val);
                            switch (usage)
                            {
                            case kHIDUsage_GD_X:
                                wine_obj = 0;
                                oldVal = device->generic.js.lX;
                                device->generic.js.lX = newVal;
                                break;
                            case kHIDUsage_GD_Y:
                                wine_obj = 1;
                                oldVal = device->generic.js.lY;
                                device->generic.js.lY = newVal;
                                break;
                            case kHIDUsage_GD_Z:
                                wine_obj = 2;
                                oldVal = device->generic.js.lZ;
                                device->generic.js.lZ = newVal;
                                break;
                            case kHIDUsage_GD_Rx:
                                wine_obj = 3;
                                oldVal = device->generic.js.lRx;
                                device->generic.js.lRx = newVal;
                                break;
                            case kHIDUsage_GD_Ry:
                                wine_obj = 4;
                                oldVal = device->generic.js.lRy;
                                device->generic.js.lRy = newVal;
                                break;
                            case kHIDUsage_GD_Rz:
                                wine_obj = 5;
                                oldVal = device->generic.js.lRz;
                                device->generic.js.lRz = newVal;
                                break;
                            case kHIDUsage_GD_Slider:
                                wine_obj = 6 + slider_idx;
                                oldVal = device->generic.js.rglSlider[slider_idx];
                                device->generic.js.rglSlider[slider_idx] = newVal;
                                slider_idx ++;
                                break;
                            }
                            if ((wine_obj != -1) &&
                                 (oldVal != newVal))
                            {
                                inst_id = DIDFT_MAKEINSTANCE(wine_obj) | DIDFT_ABSAXIS;
                                queue_event(iface,inst_id,newVal,GetCurrentTime(),device->generic.base.dinput->evsequence++);
                            }

                            break;
                        }
                        default:
                            FIXME("unhandled usage %i\n",usage);
                    }
                    break;
                }
                default:
                    FIXME("Unhandled type %i\n",eleType);
            }
        }
    }
}

static INT find_joystick_devices(void)
{
    static INT joystick_devices_count = -1;

    if (joystick_devices_count != -1) return joystick_devices_count;

    joystick_devices_count = find_osx_devices();

    return  joystick_devices_count;
}

static BOOL joydev_enum_deviceA(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEA lpddi, DWORD version, int id)
{
    if (id >= find_joystick_devices()) return FALSE;

    if (dwFlags & DIEDFL_FORCEFEEDBACK) {
        WARN("force feedback not supported\n");
        return FALSE;
    }

    if ((dwDevType == 0) ||
    ((dwDevType == DIDEVTYPE_JOYSTICK) && (version > 0x0300 && version < 0x0800)) ||
    (((dwDevType == DI8DEVCLASS_GAMECTRL) || (dwDevType == DI8DEVTYPE_JOYSTICK)) && (version >= 0x0800)))
    {
        /* Return joystick */
        lpddi->guidInstance = DInput_Wine_OsX_Joystick_GUID;
        lpddi->guidInstance.Data3 = id;
        lpddi->guidProduct = DInput_Wine_OsX_Joystick_GUID;
        /* we only support traditional joysticks for now */
        if (version >= 0x0800)
            lpddi->dwDevType = DI8DEVTYPE_JOYSTICK | (DI8DEVTYPEJOYSTICK_STANDARD << 8);
        else
            lpddi->dwDevType = DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_TRADITIONAL << 8);
        sprintf(lpddi->tszInstanceName, "Joystick %d", id);

        /* get the device name */
        get_osx_device_name(id, lpddi->tszProductName, MAX_PATH);

        lpddi->guidFFDriver = GUID_NULL;
        return TRUE;
    }

    return FALSE;
}

static BOOL joydev_enum_deviceW(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEW lpddi, DWORD version, int id)
{
    char name[MAX_PATH];
    char friendly[32];

    if (id >= find_joystick_devices()) return FALSE;

    if (dwFlags & DIEDFL_FORCEFEEDBACK) {
        WARN("force feedback not supported\n");
        return FALSE;
    }

    if ((dwDevType == 0) ||
    ((dwDevType == DIDEVTYPE_JOYSTICK) && (version > 0x0300 && version < 0x0800)) ||
    (((dwDevType == DI8DEVCLASS_GAMECTRL) || (dwDevType == DI8DEVTYPE_JOYSTICK)) && (version >= 0x0800))) {
        /* Return joystick */
        lpddi->guidInstance = DInput_Wine_OsX_Joystick_GUID;
        lpddi->guidInstance.Data3 = id;
        lpddi->guidProduct = DInput_Wine_OsX_Joystick_GUID;
        /* we only support traditional joysticks for now */
        if (version >= 0x0800)
            lpddi->dwDevType = DI8DEVTYPE_JOYSTICK | (DI8DEVTYPEJOYSTICK_STANDARD << 8);
        else
            lpddi->dwDevType = DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_TRADITIONAL << 8);
        sprintf(friendly, "Joystick %d", id);
        MultiByteToWideChar(CP_ACP, 0, friendly, -1, lpddi->tszInstanceName, MAX_PATH);
        /* get the device name */
        get_osx_device_name(id, name, MAX_PATH);

        MultiByteToWideChar(CP_ACP, 0, name, -1, lpddi->tszProductName, MAX_PATH);
        lpddi->guidFFDriver = GUID_NULL;
        return TRUE;
    }

    return FALSE;
}

static HRESULT alloc_device(REFGUID rguid, IDirectInputImpl *dinput,
                            JoystickImpl **pdev, unsigned short index)
{
    DWORD i;
    JoystickImpl* newDevice;
    char name[MAX_PATH];
    HRESULT hr;
    LPDIDATAFORMAT df = NULL;
    int idx = 0;
    int axis_map[8]; /* max axes */
    int slider_count = 0;

    TRACE("%s %p %p %hu\n", debugstr_guid(rguid), dinput, pdev, index);

    newDevice = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(JoystickImpl));
    if (newDevice == 0) {
        WARN("out of memory\n");
        *pdev = 0;
        return DIERR_OUTOFMEMORY;
    }

    newDevice->id = index;

    newDevice->generic.guidInstance = DInput_Wine_OsX_Joystick_GUID;
    newDevice->generic.guidInstance.Data3 = index;
    newDevice->generic.guidProduct = DInput_Wine_OsX_Joystick_GUID;
    newDevice->generic.joy_polldev = poll_osx_device_state;

    /* get the device name */
    get_osx_device_name(index, name, MAX_PATH);
    TRACE("Name %s\n",name);

    /* copy the device name */
    newDevice->generic.name = HeapAlloc(GetProcessHeap(),0,strlen(name) + 1);
    strcpy(newDevice->generic.name, name);

    memset(axis_map, 0, sizeof(axis_map));
    get_osx_device_elements(newDevice, axis_map);

    TRACE("%i axes %i buttons %i povs\n",newDevice->generic.devcaps.dwAxes,newDevice->generic.devcaps.dwButtons,newDevice->generic.devcaps.dwPOVs);

    if (newDevice->generic.devcaps.dwButtons > 128)
    {
        WARN("Can't support %d buttons. Clamping down to 128\n", newDevice->generic.devcaps.dwButtons);
        newDevice->generic.devcaps.dwButtons = 128;
    }

    newDevice->generic.base.IDirectInputDevice8A_iface.lpVtbl = &JoystickAvt;
    newDevice->generic.base.IDirectInputDevice8W_iface.lpVtbl = &JoystickWvt;
    newDevice->generic.base.ref = 1;
    newDevice->generic.base.dinput = dinput;
    newDevice->generic.base.guid = *rguid;
    InitializeCriticalSection(&newDevice->generic.base.crit);
    newDevice->generic.base.crit.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": JoystickImpl*->generic.base.crit");

    /* Create copy of default data format */
    if (!(df = HeapAlloc(GetProcessHeap(), 0, c_dfDIJoystick2.dwSize))) goto FAILED;
    memcpy(df, &c_dfDIJoystick2, c_dfDIJoystick2.dwSize);

    df->dwNumObjs = newDevice->generic.devcaps.dwAxes + newDevice->generic.devcaps.dwPOVs + newDevice->generic.devcaps.dwButtons;
    if (!(df->rgodf = HeapAlloc(GetProcessHeap(), 0, df->dwNumObjs * df->dwObjSize))) goto FAILED;

    for (i = 0; i < newDevice->generic.devcaps.dwAxes; i++)
    {
        int wine_obj = -1;
        switch (axis_map[i])
        {
            case kHIDUsage_GD_X: wine_obj = 0; break;
            case kHIDUsage_GD_Y: wine_obj = 1; break;
            case kHIDUsage_GD_Z: wine_obj = 2; break;
            case kHIDUsage_GD_Rx: wine_obj = 3; break;
            case kHIDUsage_GD_Ry: wine_obj = 4; break;
            case kHIDUsage_GD_Rz: wine_obj = 5; break;
            case kHIDUsage_GD_Slider:
                wine_obj = 6 + slider_count;
                slider_count++;
                break;
        }
        if (wine_obj < 0 ) continue;

        memcpy(&df->rgodf[idx], &c_dfDIJoystick2.rgodf[wine_obj], df->dwObjSize);
        df->rgodf[idx++].dwType = DIDFT_MAKEINSTANCE(wine_obj) | DIDFT_ABSAXIS;
    }

    for (i = 0; i < newDevice->generic.devcaps.dwPOVs; i++)
    {
        memcpy(&df->rgodf[idx], &c_dfDIJoystick2.rgodf[i + 8], df->dwObjSize);
        df->rgodf[idx++].dwType = DIDFT_MAKEINSTANCE(i) | DIDFT_POV;
    }

    for (i = 0; i < newDevice->generic.devcaps.dwButtons; i++)
    {
        memcpy(&df->rgodf[idx], &c_dfDIJoystick2.rgodf[i + 12], df->dwObjSize);
        df->rgodf[idx  ].pguid = &GUID_Button;
        df->rgodf[idx++].dwType = DIDFT_MAKEINSTANCE(i) | DIDFT_PSHBUTTON;
    }
    newDevice->generic.base.data_format.wine_df = df;

    /* initialize default properties */
    get_osx_device_elements_props(newDevice);

    IDirectInput_AddRef(&newDevice->generic.base.dinput->IDirectInput7A_iface);

    EnterCriticalSection(&dinput->crit);
    list_add_tail(&dinput->devices_list, &newDevice->generic.base.entry);
    LeaveCriticalSection(&dinput->crit);

    newDevice->generic.devcaps.dwSize = sizeof(newDevice->generic.devcaps);
    newDevice->generic.devcaps.dwFlags = DIDC_ATTACHED;
    if (newDevice->generic.base.dinput->dwVersion >= 0x0800)
        newDevice->generic.devcaps.dwDevType = DI8DEVTYPE_JOYSTICK | (DI8DEVTYPEJOYSTICK_STANDARD << 8);
    else
        newDevice->generic.devcaps.dwDevType = DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_TRADITIONAL << 8);
    newDevice->generic.devcaps.dwFFSamplePeriod = 0;
    newDevice->generic.devcaps.dwFFMinTimeResolution = 0;
    newDevice->generic.devcaps.dwFirmwareRevision = 0;
    newDevice->generic.devcaps.dwHardwareRevision = 0;
    newDevice->generic.devcaps.dwFFDriverVersion = 0;

    if (TRACE_ON(dinput)) {
        _dump_DIDATAFORMAT(newDevice->generic.base.data_format.wine_df);
        _dump_DIDEVCAPS(&newDevice->generic.devcaps);
    }

    *pdev = newDevice;

    return DI_OK;

FAILED:
    hr = DIERR_OUTOFMEMORY;
    if (df) HeapFree(GetProcessHeap(), 0, df->rgodf);
    HeapFree(GetProcessHeap(), 0, df);
    release_DataFormat(&newDevice->generic.base.data_format);
    HeapFree(GetProcessHeap(),0,newDevice->generic.name);
    HeapFree(GetProcessHeap(),0,newDevice);
    *pdev = 0;

    return hr;
}

/******************************************************************************
  *     get_joystick_index : Get the joystick index from a given GUID
  */
static unsigned short get_joystick_index(REFGUID guid)
{
    GUID wine_joystick = DInput_Wine_OsX_Joystick_GUID;
    GUID dev_guid = *guid;

    wine_joystick.Data3 = 0;
    dev_guid.Data3 = 0;

    /* for the standard joystick GUID use index 0 */
    if(IsEqualGUID(&GUID_Joystick,guid)) return 0;

    /* for the wine joystick GUIDs use the index stored in Data3 */
    if(IsEqualGUID(&wine_joystick, &dev_guid)) return guid->Data3;

    return 0xffff;
}

static HRESULT joydev_create_device(IDirectInputImpl *dinput, REFGUID rguid, REFIID riid, LPVOID *pdev, int unicode)
{
    unsigned short index;
    int joystick_devices_count;

    TRACE("%p %s %s %p %i\n", dinput, debugstr_guid(rguid), debugstr_guid(riid), pdev, unicode);
    *pdev = NULL;

    if ((joystick_devices_count = find_joystick_devices()) == 0)
        return DIERR_DEVICENOTREG;

    if ((index = get_joystick_index(rguid)) < 0xffff &&
        joystick_devices_count && index < joystick_devices_count)
    {
        JoystickImpl *This;
        HRESULT hr;

        if (riid == NULL)
            ;/* nothing */
        else if (IsEqualGUID(&IID_IDirectInputDeviceA,  riid) ||
                 IsEqualGUID(&IID_IDirectInputDevice2A, riid) ||
                 IsEqualGUID(&IID_IDirectInputDevice7A, riid) ||
                 IsEqualGUID(&IID_IDirectInputDevice8A, riid))
        {
            unicode = 0;
        }
        else if (IsEqualGUID(&IID_IDirectInputDeviceW,  riid) ||
                 IsEqualGUID(&IID_IDirectInputDevice2W, riid) ||
                 IsEqualGUID(&IID_IDirectInputDevice7W, riid) ||
                 IsEqualGUID(&IID_IDirectInputDevice8W, riid))
        {
            unicode = 1;
        }
        else
        {
            WARN("no interface\n");
            return DIERR_NOINTERFACE;
        }

        hr = alloc_device(rguid, dinput, &This, index);
        if (!This) return hr;

        if (unicode)
            *pdev = &This->generic.base.IDirectInputDevice8W_iface;
        else
            *pdev = &This->generic.base.IDirectInputDevice8A_iface;
        return hr;
    }

    return DIERR_DEVICENOTREG;
}

const struct dinput_device joystick_osx_device = {
  "Wine OS X joystick driver",
  joydev_enum_deviceA,
  joydev_enum_deviceW,
  joydev_create_device
};

static const IDirectInputDevice8AVtbl JoystickAvt =
{
    IDirectInputDevice2AImpl_QueryInterface,
    IDirectInputDevice2AImpl_AddRef,
    IDirectInputDevice2AImpl_Release,
    JoystickAGenericImpl_GetCapabilities,
    IDirectInputDevice2AImpl_EnumObjects,
    JoystickAGenericImpl_GetProperty,
    JoystickAGenericImpl_SetProperty,
    IDirectInputDevice2AImpl_Acquire,
    IDirectInputDevice2AImpl_Unacquire,
    JoystickAGenericImpl_GetDeviceState,
    IDirectInputDevice2AImpl_GetDeviceData,
    IDirectInputDevice2AImpl_SetDataFormat,
    IDirectInputDevice2AImpl_SetEventNotification,
    IDirectInputDevice2AImpl_SetCooperativeLevel,
    JoystickAGenericImpl_GetObjectInfo,
    JoystickAGenericImpl_GetDeviceInfo,
    IDirectInputDevice2AImpl_RunControlPanel,
    IDirectInputDevice2AImpl_Initialize,
    IDirectInputDevice2AImpl_CreateEffect,
    IDirectInputDevice2AImpl_EnumEffects,
    IDirectInputDevice2AImpl_GetEffectInfo,
    IDirectInputDevice2AImpl_GetForceFeedbackState,
    IDirectInputDevice2AImpl_SendForceFeedbackCommand,
    IDirectInputDevice2AImpl_EnumCreatedEffectObjects,
    IDirectInputDevice2AImpl_Escape,
    JoystickAGenericImpl_Poll,
    IDirectInputDevice2AImpl_SendDeviceData,
    IDirectInputDevice7AImpl_EnumEffectsInFile,
    IDirectInputDevice7AImpl_WriteEffectToFile,
    JoystickAGenericImpl_BuildActionMap,
    JoystickAGenericImpl_SetActionMap,
    IDirectInputDevice8AImpl_GetImageInfo
};

static const IDirectInputDevice8WVtbl JoystickWvt =
{
    IDirectInputDevice2WImpl_QueryInterface,
    IDirectInputDevice2WImpl_AddRef,
    IDirectInputDevice2WImpl_Release,
    JoystickWGenericImpl_GetCapabilities,
    IDirectInputDevice2WImpl_EnumObjects,
    JoystickWGenericImpl_GetProperty,
    JoystickWGenericImpl_SetProperty,
    IDirectInputDevice2WImpl_Acquire,
    IDirectInputDevice2WImpl_Unacquire,
    JoystickWGenericImpl_GetDeviceState,
    IDirectInputDevice2WImpl_GetDeviceData,
    IDirectInputDevice2WImpl_SetDataFormat,
    IDirectInputDevice2WImpl_SetEventNotification,
    IDirectInputDevice2WImpl_SetCooperativeLevel,
    JoystickWGenericImpl_GetObjectInfo,
    JoystickWGenericImpl_GetDeviceInfo,
    IDirectInputDevice2WImpl_RunControlPanel,
    IDirectInputDevice2WImpl_Initialize,
    IDirectInputDevice2WImpl_CreateEffect,
    IDirectInputDevice2WImpl_EnumEffects,
    IDirectInputDevice2WImpl_GetEffectInfo,
    IDirectInputDevice2WImpl_GetForceFeedbackState,
    IDirectInputDevice2WImpl_SendForceFeedbackCommand,
    IDirectInputDevice2WImpl_EnumCreatedEffectObjects,
    IDirectInputDevice2WImpl_Escape,
    JoystickWGenericImpl_Poll,
    IDirectInputDevice2WImpl_SendDeviceData,
    IDirectInputDevice7WImpl_EnumEffectsInFile,
    IDirectInputDevice7WImpl_WriteEffectToFile,
    JoystickWGenericImpl_BuildActionMap,
    JoystickWGenericImpl_SetActionMap,
    IDirectInputDevice8WImpl_GetImageInfo
};

#else /* HAVE_IOHIDMANAGERCREATE */

const struct dinput_device joystick_osx_device = {
  "Wine OS X joystick driver",
  NULL,
  NULL,
  NULL
};

#endif /* HAVE_IOHIDMANAGERCREATE */
