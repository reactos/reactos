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

#include "config.h"
#include "wine/port.h"

#if defined(HAVE_IOKIT_HID_IOHIDLIB_H)
#define DWORD UInt32
#define LPDWORD UInt32*
#define LONG SInt32
#define LPLONG SInt32*
#define E_PENDING __carbon_E_PENDING
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
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDLib.h>
#include <ForceFeedback/ForceFeedback.h>
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
#undef DWORD
#undef LPDWORD
#undef LONG
#undef LPLONG
#undef E_PENDING
#endif /* HAVE_IOKIT_HID_IOHIDLIB_H */

#include "wine/debug.h"
#include "wine/unicode.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "devguid.h"
#include "dinput.h"

#include "dinput_private.h"
#include "device_private.h"
#include "joystick_private.h"

#ifdef HAVE_IOHIDMANAGERCREATE

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

static CFMutableArrayRef device_main_elements = NULL;

typedef struct JoystickImpl JoystickImpl;
static const IDirectInputDevice8AVtbl JoystickAvt;
static const IDirectInputDevice8WVtbl JoystickWvt;

struct JoystickImpl
{
    struct JoystickGenericImpl generic;

    /* osx private */
    int                    id;
    CFArrayRef             elements;
    ObjProps               **propmap;
    FFDeviceObjectReference ff;
    struct list effects;
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

static inline IDirectInputDevice8W *IDirectInputDevice8W_from_impl(JoystickImpl *This)
{
    return &This->generic.base.IDirectInputDevice8W_iface;
}

typedef struct _EffectImpl {
    IDirectInputEffect IDirectInputEffect_iface;
    LONG ref;

    JoystickImpl *device;
    FFEffectObjectReference effect;
    GUID guid;

    struct list entry;
} EffectImpl;

static EffectImpl *impl_from_IDirectInputEffect(IDirectInputEffect *iface)
{
    return CONTAINING_RECORD(iface, EffectImpl, IDirectInputEffect_iface);
}

static const IDirectInputEffectVtbl EffectVtbl;

static const GUID DInput_Wine_OsX_Joystick_GUID = { /* 59CAD8F6-E617-41E2-8EB7-47B23EEEDC5A */
  0x59CAD8F6, 0xE617, 0x41E2, {0x8E, 0xB7, 0x47, 0xB2, 0x3E, 0xEE, 0xDC, 0x5A}
};

static HRESULT osx_to_win32_hresult(HRESULT in)
{
    /* OSX returns 16-bit COM runtime errors, which we should
     * convert to win32 */
    switch(in){
    case 0x80000001:
        return E_NOTIMPL;
    case 0x80000002:
        return E_OUTOFMEMORY;
    case 0x80000003:
        return E_INVALIDARG;
    case 0x80000004:
        return E_NOINTERFACE;
    case 0x80000005:
        return E_POINTER;
    case 0x80000006:
        return E_HANDLE;
    case 0x80000007:
        return E_ABORT;
    case 0x80000008:
        return E_FAIL;
    case 0x80000009:
        return E_ACCESSDENIED;
    case 0x8000FFFF:
        return E_UNEXPECTED;
    }
    return in;
}

static long get_device_property_long(IOHIDDeviceRef device, CFStringRef key)
{
    CFTypeRef ref;
    long result = 0;

    if (device)
    {
        assert(IOHIDDeviceGetTypeID() == CFGetTypeID(device));

        ref = IOHIDDeviceGetProperty(device, key);

        if (ref && CFNumberGetTypeID() == CFGetTypeID(ref))
            CFNumberGetValue((CFNumberRef)ref, kCFNumberLongType, &result);
    }

    return result;
}

static CFStringRef copy_device_name(IOHIDDeviceRef device)
{
    CFStringRef name;

    if (device)
    {
        CFTypeRef ref_name;

        assert(IOHIDDeviceGetTypeID() == CFGetTypeID(device));

        ref_name = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey));

        if (ref_name && CFStringGetTypeID() == CFGetTypeID(ref_name))
            name = CFStringCreateCopy(kCFAllocatorDefault, ref_name);
        else
        {
            long vendID, prodID;

            vendID = get_device_property_long(device, CFSTR(kIOHIDVendorIDKey));
            prodID = get_device_property_long(device, CFSTR(kIOHIDProductIDKey));
            name = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("0x%04lx 0x%04lx"), vendID, prodID);
        }
    }
    else
    {
        ERR("NULL device\n");
        name = CFStringCreateCopy(kCFAllocatorDefault, CFSTR(""));
    }

    return name;
}

static long get_device_location_ID(IOHIDDeviceRef device)
{
    return get_device_property_long(device, CFSTR(kIOHIDLocationIDKey));
}

static void copy_set_to_array(const void *value, void *context)
{
    CFArrayAppendValue(context, value);
}

static CFComparisonResult device_name_comparator(IOHIDDeviceRef device1, IOHIDDeviceRef device2)
{
    CFStringRef name1 = copy_device_name(device1), name2 = copy_device_name(device2);
    CFComparisonResult result = CFStringCompare(name1, name2, (kCFCompareForcedOrdering | kCFCompareNumerically));
    CFRelease(name1);
    CFRelease(name2);
    return  result;
}

static CFComparisonResult device_location_name_comparator(const void *val1, const void *val2, void *context)
{
    IOHIDDeviceRef device1 = (IOHIDDeviceRef)val1, device2 = (IOHIDDeviceRef)val2;
    long loc1 = get_device_location_ID(device1), loc2 = get_device_location_ID(device2);

    if (loc1 < loc2)
        return kCFCompareLessThan;
    else if (loc1 > loc2)
        return kCFCompareGreaterThan;
    /* virtual joysticks may not have a kIOHIDLocationIDKey and will default to location ID of 0, this orders virtual joysticks by their name */
    return device_name_comparator(device1, device2);
}

static const char* debugstr_cf(CFTypeRef t)
{
    CFStringRef s;
    const char* ret;

    if (!t) return "(null)";

    if (CFGetTypeID(t) == CFStringGetTypeID())
        s = t;
    else
        s = CFCopyDescription(t);
    ret = CFStringGetCStringPtr(s, kCFStringEncodingUTF8);
    if (ret) ret = debugstr_a(ret);
    if (!ret)
    {
        const UniChar* u = CFStringGetCharactersPtr(s);
        if (u)
            ret = debugstr_wn((const WCHAR*)u, CFStringGetLength(s));
    }
    if (!ret)
    {
        UniChar buf[200];
        int len = min(CFStringGetLength(s), ARRAY_SIZE(buf));
        CFStringGetCharacters(s, CFRangeMake(0, len), buf);
        ret = debugstr_wn(buf, len);
    }
    if (s != t) CFRelease(s);
    return ret;
}

static const char* debugstr_device(IOHIDDeviceRef device)
{
    return wine_dbg_sprintf("<IOHIDDevice %p product %s IOHIDLocationID %lu>", device,
                            debugstr_cf(IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey))),
                            get_device_location_ID(device));
}

static const char* debugstr_element(IOHIDElementRef element)
{
    return wine_dbg_sprintf("<IOHIDElement %p type %d usage %u/%u device %p>", element,
                            IOHIDElementGetType(element), IOHIDElementGetUsagePage(element),
                            IOHIDElementGetUsage(element), IOHIDElementGetDevice(element));
}

static IOHIDDeviceRef get_device_ref(int id)
{
    IOHIDElementRef device_main_element;
    IOHIDDeviceRef hid_device;

    TRACE("id %d\n", id);

    if (!device_main_elements || id >= CFArrayGetCount(device_main_elements))
        return 0;

    device_main_element = (IOHIDElementRef)CFArrayGetValueAtIndex(device_main_elements, id);
    if (!device_main_element)
    {
        ERR("Invalid Element requested %i\n",id);
        return 0;
    }

    hid_device = IOHIDElementGetDevice(device_main_element);
    if (!hid_device)
    {
        ERR("Invalid Device requested %i\n",id);
        return 0;
    }

    TRACE("-> %s\n", debugstr_device(hid_device));
    return hid_device;
}

static HRESULT get_ff(IOHIDDeviceRef device, FFDeviceObjectReference *ret)
{
    io_service_t service;
    CFMutableDictionaryRef matching;
    CFTypeRef location_id;
    HRESULT hr;

    TRACE("device %s\n", debugstr_device(device));

    matching = IOServiceMatching(kIOHIDDeviceKey);
    if(!matching){
        WARN("IOServiceMatching failed, force feedback disabled\n");
        return DIERR_DEVICENOTREG;
    }

    location_id = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDLocationIDKey));
    if(!location_id){
        CFRelease(matching);
        WARN("IOHIDDeviceGetProperty failed, force feedback disabled\n");
        return DIERR_DEVICENOTREG;
    }

    CFDictionaryAddValue(matching, CFSTR(kIOHIDLocationIDKey), location_id);

    service = IOServiceGetMatchingService(kIOMasterPortDefault, matching);

    if (ret)
        hr = osx_to_win32_hresult(FFCreateDevice(service, ret));
    else
        hr = FFIsForceFeedback(service) == FF_OK ? S_OK : S_FALSE;

    IOObjectRelease(service);
    TRACE("-> hr 0x%08x *ret %p\n", hr, ret ? *ret : NULL);
    return hr;
}

static CFMutableDictionaryRef create_osx_device_match(int usage)
{
    CFMutableDictionaryRef result;

    TRACE("usage %d\n", usage);

    result = CFDictionaryCreateMutable( kCFAllocatorDefault, 0,
            &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );

    if ( result )
    {
        int number = kHIDPage_GenericDesktop;
        CFNumberRef page = CFNumberCreate( kCFAllocatorDefault,
                          kCFNumberIntType, &number);

        if (page)
        {
            CFNumberRef cf_usage;

            CFDictionarySetValue( result, CFSTR( kIOHIDDeviceUsagePageKey ), page );
            CFRelease( page );

            cf_usage = CFNumberCreate( kCFAllocatorDefault,
                        kCFNumberIntType, &usage);
            if (cf_usage)
            {
                CFDictionarySetValue( result, CFSTR( kIOHIDDeviceUsageKey ), cf_usage );
                CFRelease( cf_usage );
            }
            else
            {
                ERR("CFNumberCreate() failed.\n");
                CFRelease(result);
                return NULL;
            }
        }
        else
        {
            ERR("CFNumberCreate failed.\n");
            CFRelease(result);
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

static CFIndex find_top_level(IOHIDDeviceRef hid_device, CFMutableArrayRef main_elements)
{
    CFArrayRef      elements;
    CFIndex         total = 0;

    TRACE("hid_device %s\n", debugstr_device(hid_device));

    if (!hid_device)
        return 0;

    elements = IOHIDDeviceCopyMatchingElements(hid_device, NULL, 0);

    if (elements)
    {
        CFIndex idx, cnt = CFArrayGetCount(elements);
        for (idx=0; idx<cnt; idx++)
        {
            IOHIDElementRef element = (IOHIDElementRef)CFArrayGetValueAtIndex(elements, idx);
            int type = IOHIDElementGetType(element);

            TRACE("element %s\n", debugstr_element(element));

            /* Check for top-level gaming device collections */
            if (type == kIOHIDElementTypeCollection && IOHIDElementGetParent(element) == 0)
            {
                int usage_page = IOHIDElementGetUsagePage(element);
                int usage = IOHIDElementGetUsage(element);

                if (usage_page == kHIDPage_GenericDesktop &&
                    (usage == kHIDUsage_GD_Joystick || usage == kHIDUsage_GD_GamePad))
                {
                    CFArrayAppendValue(main_elements, element);
                    total++;
                }
            }
        }
        CFRelease(elements);
    }

    TRACE("-> total %d\n", (int)total);
    return total;
}

static void get_element_children(IOHIDElementRef element, CFMutableArrayRef all_children)
{
    CFIndex    idx, cnt;
    CFArrayRef element_children = IOHIDElementGetChildren(element);

    TRACE("element %s\n", debugstr_element(element));

    cnt = CFArrayGetCount(element_children);

    /* Either add the element to the array or grab its children */
    for (idx=0; idx<cnt; idx++)
    {
        IOHIDElementRef child;

        child = (IOHIDElementRef)CFArrayGetValueAtIndex(element_children, idx);
        TRACE("child %s\n", debugstr_element(child));
        if (IOHIDElementGetType(child) == kIOHIDElementTypeCollection)
            get_element_children(child, all_children);
        else
            CFArrayAppendValue(all_children, child);
    }
}

static int find_osx_devices(void)
{
    IOHIDManagerRef hid_manager;
    CFMutableDictionaryRef result;
    CFSetRef devset;
    CFMutableArrayRef matching;

    TRACE("()\n");

    hid_manager = IOHIDManagerCreate( kCFAllocatorDefault, 0L );
    if (IOHIDManagerOpen( hid_manager, 0 ) != kIOReturnSuccess)
    {
        ERR("Couldn't open IOHIDManager.\n");
        CFRelease( hid_manager );
        return 0;
    }

     matching = CFArrayCreateMutable( kCFAllocatorDefault, 0,
                        &kCFTypeArrayCallBacks );

    /* build matching dictionary */
    result = create_osx_device_match(kHIDUsage_GD_Joystick);
    if (!result)
    {
        CFRelease(matching);
        goto fail;
    }
    CFArrayAppendValue( matching, result );
    CFRelease( result );
    result = create_osx_device_match(kHIDUsage_GD_GamePad);
    if (!result)
    {
        CFRelease(matching);
        goto fail;
    }
    CFArrayAppendValue( matching, result );
    CFRelease( result );

    IOHIDManagerSetDeviceMatchingMultiple( hid_manager, matching);
    CFRelease( matching );
    devset = IOHIDManagerCopyDevices( hid_manager );
    if (devset)
    {
        CFIndex num_devices, num_main_elements, idx;
        CFMutableArrayRef devices;

        num_devices = CFSetGetCount(devset);
        devices = CFArrayCreateMutable(kCFAllocatorDefault, num_devices, &kCFTypeArrayCallBacks);
        CFSetApplyFunction(devset, copy_set_to_array, devices);
        CFRelease(devset);
        CFArraySortValues(devices, CFRangeMake(0, num_devices), device_location_name_comparator, NULL);

        device_main_elements = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
        if (!device_main_elements)
        {
            CFRelease( devices );
            goto fail;
        }

        num_main_elements = 0;
        for (idx = 0; idx < num_devices; idx++)
        {
            CFIndex top;
            IOHIDDeviceRef hid_device;

            hid_device = (IOHIDDeviceRef) CFArrayGetValueAtIndex(devices, idx);
            TRACE("hid_device %s\n", debugstr_device(hid_device));
            top = find_top_level(hid_device, device_main_elements);
            num_main_elements += top;
        }

        CFRelease(devices);

        TRACE("found %i device(s), %i collection(s)\n",(int)num_devices,(int)num_main_elements);
        return (int)num_main_elements;
    }

fail:
    IOHIDManagerClose( hid_manager, 0 );
    CFRelease( hid_manager );
    return 0;
}

static int get_osx_device_name(int id, char *name, int length)
{
    CFStringRef str;
    IOHIDDeviceRef hid_device;

    hid_device = get_device_ref(id);

    TRACE("id %d hid_device %s\n", id, debugstr_device(hid_device));

    if (name)
        name[0] = 0;

    if (!hid_device)
        return 0;

    str = IOHIDDeviceGetProperty(hid_device, CFSTR( kIOHIDProductKey ));
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

static CFComparisonResult button_usage_comparator(const void *val1, const void *val2, void *context)
{
    IOHIDElementRef element1 = (IOHIDElementRef)val1, element2 = (IOHIDElementRef)val2;
    int usage1 = IOHIDElementGetUsage(element1), usage2 = IOHIDElementGetUsage(element2);

    if (usage1 < usage2)
        return kCFCompareLessThan;
    if (usage1 > usage2)
        return kCFCompareGreaterThan;
    return kCFCompareEqualTo;
}

static void get_osx_device_elements(JoystickImpl *device, int axis_map[8])
{
    IOHIDElementRef device_main_element;
    CFMutableArrayRef elements;
    DWORD           sliders = 0;

    TRACE("device %p device->id %d\n", device, device->id);

    device->elements = NULL;

    if (!device_main_elements || device->id >= CFArrayGetCount(device_main_elements))
        return;

    device_main_element = (IOHIDElementRef)CFArrayGetValueAtIndex(device_main_elements, device->id);
    TRACE("device_main_element %s\n", debugstr_element(device_main_element));
    if (!device_main_element)
        return;

    elements = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
    get_element_children(device_main_element, elements);

    if (elements)
    {
        CFIndex idx, cnt = CFArrayGetCount( elements );
        CFMutableArrayRef axes = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
        CFMutableArrayRef buttons = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
        CFMutableArrayRef povs = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);

        for ( idx = 0; idx < cnt; idx++ )
        {
            IOHIDElementRef element = ( IOHIDElementRef ) CFArrayGetValueAtIndex( elements, idx );
            int type = IOHIDElementGetType( element );
            int usage_page = IOHIDElementGetUsagePage( element );

            TRACE("element %s\n", debugstr_element(element));

            if (usage_page >= kHIDPage_VendorDefinedStart)
            {
                /* vendor pages can repurpose type ids, resulting in incorrect case matches below (e.g. ds4 controllers) */
                continue;
            }

            switch(type)
            {
                case kIOHIDElementTypeInput_Button:
                {
                    TRACE("kIOHIDElementTypeInput_Button usage_page %d\n", usage_page);
                    if (usage_page != kHIDPage_Button)
                    {
                        /* avoid strange elements found on the 360 controller */
                        continue;
                    }

                    if (CFArrayGetCount(buttons) < 128)
                        CFArrayAppendValue(buttons, element);
                    break;
                }
                case kIOHIDElementTypeInput_Axis:
                {
                    TRACE("kIOHIDElementTypeInput_Axis\n");
                    CFArrayAppendValue(axes, element);
                    break;
                }
                case kIOHIDElementTypeInput_Misc:
                {
                    uint32_t usage = IOHIDElementGetUsage( element );
                    switch(usage)
                    {
                        case kHIDUsage_GD_Hatswitch:
                        {
                            TRACE("kIOHIDElementTypeInput_Misc / kHIDUsage_GD_Hatswitch\n");
                            CFArrayAppendValue(povs, element);
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
                            TRACE("kIOHIDElementTypeInput_Misc / kHIDUsage_GD_* (%d)\n", usage);
                            axis_map[CFArrayGetCount(axes)]=usage;
                            CFArrayAppendValue(axes, element);
                            break;
                        }
                        default:
                            FIXME("kIOHIDElementTypeInput_Misc / Unhandled usage %i\n", usage);
                    }
                    break;
                }
                default:
                    FIXME("Unhandled type %i\n",type);
            }
        }

        /* Sort buttons into correct order */
        CFArraySortValues(buttons, CFRangeMake(0, CFArrayGetCount(buttons)), button_usage_comparator, NULL);

        device->generic.devcaps.dwAxes = CFArrayGetCount(axes);
        device->generic.devcaps.dwButtons = CFArrayGetCount(buttons);
        device->generic.devcaps.dwPOVs = CFArrayGetCount(povs);

        TRACE("axes %u povs %u buttons %u\n", device->generic.devcaps.dwAxes, device->generic.devcaps.dwPOVs,
              device->generic.devcaps.dwButtons);

        /* build our element array in the order that dinput expects */
        CFArrayAppendArray(axes, povs, CFRangeMake(0, device->generic.devcaps.dwPOVs));
        CFArrayAppendArray(axes, buttons, CFRangeMake(0, device->generic.devcaps.dwButtons));
        device->elements = axes;
        axes = NULL;

        CFRelease(povs);
        CFRelease(buttons);
        CFRelease(elements);
    }
    else
    {
        device->generic.devcaps.dwAxes = 0;
        device->generic.devcaps.dwButtons = 0;
        device->generic.devcaps.dwPOVs = 0;
    }
}

static void get_osx_device_elements_props(JoystickImpl *device)
{
    TRACE("device %p\n", device);

    if (device->elements)
    {
        CFIndex idx, cnt = CFArrayGetCount( device->elements );

        for ( idx = 0; idx < cnt; idx++ )
        {
            IOHIDElementRef element = ( IOHIDElementRef ) CFArrayGetValueAtIndex( device->elements, idx );

            TRACE("element %s\n", debugstr_element(element));

            device->generic.props[idx].lDevMin = IOHIDElementGetLogicalMin(element);
            device->generic.props[idx].lDevMax = IOHIDElementGetLogicalMax(element);
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
    IOHIDElementRef device_main_element;
    IOHIDDeviceRef hid_device;

    TRACE("device %p device->id %i\n", device, device->id);

    if (!device_main_elements || device->id >= CFArrayGetCount(device_main_elements))
        return;

    device_main_element = (IOHIDElementRef) CFArrayGetValueAtIndex(device_main_elements, device->id);
    hid_device = IOHIDElementGetDevice(device_main_element);
    TRACE("main element %s hid_device %s\n", debugstr_element(device_main_element), debugstr_device(hid_device));
    if (!hid_device)
        return;

    if (device->elements)
    {
        int button_idx = 0;
        int pov_idx = 0;
        int slider_idx = 0;
        int inst_id;
        CFIndex idx, cnt = CFArrayGetCount( device->elements );

        for ( idx = 0; idx < cnt; idx++ )
        {
            IOHIDValueRef valueRef;
            int val, oldVal, newVal;
            IOHIDElementRef element = ( IOHIDElementRef ) CFArrayGetValueAtIndex( device->elements, idx );
            int type = IOHIDElementGetType( element );

            TRACE("element %s\n", debugstr_element(element));

            switch(type)
            {
                case kIOHIDElementTypeInput_Button:
                    TRACE("kIOHIDElementTypeInput_Button\n");
                    if(button_idx < 128)
                    {
                        valueRef = NULL;
                        if (IOHIDDeviceGetValue(hid_device, element, &valueRef) != kIOReturnSuccess)
                            return;
                        if (valueRef == NULL)
                            return;
                        val = IOHIDValueGetIntegerValue(valueRef);
                        newVal = val ? 0x80 : 0x0;
                        oldVal = device->generic.js.rgbButtons[button_idx];
                        device->generic.js.rgbButtons[button_idx] = newVal;
                        TRACE("valueRef %s val %d oldVal %d newVal %d\n", debugstr_cf(valueRef), val, oldVal, newVal);
                        if (oldVal != newVal)
                        {
                            button_idx = device->generic.button_map[button_idx];

                            inst_id = DIDFT_MAKEINSTANCE(button_idx) | DIDFT_PSHBUTTON;
                            queue_event(iface,inst_id,newVal,GetCurrentTime(),device->generic.base.dinput->evsequence++);
                        }
                        button_idx ++;
                    }
                    break;
                case kIOHIDElementTypeInput_Misc:
                {
                    uint32_t usage = IOHIDElementGetUsage( element );
                    switch(usage)
                    {
                        case kHIDUsage_GD_Hatswitch:
                        {
                            TRACE("kIOHIDElementTypeInput_Misc / kHIDUsage_GD_Hatswitch\n");
                            valueRef = NULL;
                            if (IOHIDDeviceGetValue(hid_device, element, &valueRef) != kIOReturnSuccess)
                                return;
                            if (valueRef == NULL)
                                return;
                            val = IOHIDValueGetIntegerValue(valueRef);
                            oldVal = device->generic.js.rgdwPOV[pov_idx];
                            if (val >= 8)
                                newVal = -1;
                            else
                                newVal = val * 4500;
                            device->generic.js.rgdwPOV[pov_idx] = newVal;
                            TRACE("valueRef %s val %d oldVal %d newVal %d\n", debugstr_cf(valueRef), val, oldVal, newVal);
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

                            valueRef = NULL;
                            if (IOHIDDeviceGetValue(hid_device, element, &valueRef) != kIOReturnSuccess)
                                return;
                            if (valueRef == NULL)
                                return;
                            val = IOHIDValueGetIntegerValue(valueRef);
                            newVal = joystick_map_axis(&device->generic.props[idx], val);
                            switch (usage)
                            {
                            case kHIDUsage_GD_X:
                                TRACE("kIOHIDElementTypeInput_Misc / kHIDUsage_GD_X\n");
                                wine_obj = 0;
                                oldVal = device->generic.js.lX;
                                device->generic.js.lX = newVal;
                                break;
                            case kHIDUsage_GD_Y:
                                TRACE("kIOHIDElementTypeInput_Misc / kHIDUsage_GD_Y\n");
                                wine_obj = 1;
                                oldVal = device->generic.js.lY;
                                device->generic.js.lY = newVal;
                                break;
                            case kHIDUsage_GD_Z:
                                TRACE("kIOHIDElementTypeInput_Misc / kHIDUsage_GD_Z\n");
                                wine_obj = 2;
                                oldVal = device->generic.js.lZ;
                                device->generic.js.lZ = newVal;
                                break;
                            case kHIDUsage_GD_Rx:
                                TRACE("kIOHIDElementTypeInput_Misc / kHIDUsage_GD_Rx\n");
                                wine_obj = 3;
                                oldVal = device->generic.js.lRx;
                                device->generic.js.lRx = newVal;
                                break;
                            case kHIDUsage_GD_Ry:
                                TRACE("kIOHIDElementTypeInput_Misc / kHIDUsage_GD_Ry\n");
                                wine_obj = 4;
                                oldVal = device->generic.js.lRy;
                                device->generic.js.lRy = newVal;
                                break;
                            case kHIDUsage_GD_Rz:
                                TRACE("kIOHIDElementTypeInput_Misc / kHIDUsage_GD_Rz\n");
                                wine_obj = 5;
                                oldVal = device->generic.js.lRz;
                                device->generic.js.lRz = newVal;
                                break;
                            case kHIDUsage_GD_Slider:
                                TRACE("kIOHIDElementTypeInput_Misc / kHIDUsage_GD_Slider\n");
                                wine_obj = 6 + slider_idx;
                                oldVal = device->generic.js.rglSlider[slider_idx];
                                device->generic.js.rglSlider[slider_idx] = newVal;
                                slider_idx ++;
                                break;
                            }
                            TRACE("valueRef %s val %d oldVal %d newVal %d\n", debugstr_cf(valueRef), val, oldVal, newVal);
                            if ((wine_obj != -1) &&
                                 (oldVal != newVal))
                            {
                                inst_id = DIDFT_MAKEINSTANCE(wine_obj) | DIDFT_ABSAXIS;
                                queue_event(iface,inst_id,newVal,GetCurrentTime(),device->generic.base.dinput->evsequence++);
                            }

                            break;
                        }
                        default:
                            FIXME("kIOHIDElementTypeInput_Misc / unhandled usage %i\n", usage);
                    }
                    break;
                }
                default:
                    FIXME("Unhandled type %i\n",type);
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

static DWORD make_vid_pid(IOHIDDeviceRef device)
{
    long vendID, prodID;

    vendID = get_device_property_long(device, CFSTR(kIOHIDVendorIDKey));
    prodID = get_device_property_long(device, CFSTR(kIOHIDProductIDKey));

    return MAKELONG(vendID, prodID);
}

static HRESULT joydev_enum_deviceA(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEA lpddi, DWORD version, int id)
{
    IOHIDDeviceRef device;
    BOOL is_joystick;

    TRACE("dwDevType %u dwFlags 0x%08x version 0x%04x id %d\n", dwDevType, dwFlags, version, id);

    if (id >= find_joystick_devices()) return E_FAIL;

    device = get_device_ref(id);

    if ((dwDevType == 0) ||
    ((dwDevType == DIDEVTYPE_JOYSTICK) && (version >= 0x0300 && version < 0x0800)) ||
    (((dwDevType == DI8DEVCLASS_GAMECTRL) || (dwDevType == DI8DEVTYPE_JOYSTICK)) && (version >= 0x0800)))
    {
        if (dwFlags & DIEDFL_FORCEFEEDBACK) {
            if(!device)
                return S_FALSE;
            if(get_ff(device, NULL) != S_OK)
                return S_FALSE;
        }
        is_joystick = get_device_property_long(device, CFSTR(kIOHIDDeviceUsageKey)) == kHIDUsage_GD_Joystick;
        /* Return joystick */
        lpddi->guidInstance = DInput_Wine_OsX_Joystick_GUID;
        lpddi->guidInstance.Data3 = id;
        lpddi->guidProduct = DInput_PIDVID_Product_GUID;
        lpddi->guidProduct.Data1 = make_vid_pid(device);
        lpddi->dwDevType = get_device_type(version, is_joystick);
        lpddi->dwDevType |= DIDEVTYPE_HID;
        lpddi->wUsagePage = 0x01; /* Desktop */
        if (is_joystick)
            lpddi->wUsage = 0x04; /* Joystick */
        else
            lpddi->wUsage = 0x05; /* Game Pad */
        sprintf(lpddi->tszInstanceName, "Joystick %d", id);

        /* get the device name */
        get_osx_device_name(id, lpddi->tszProductName, MAX_PATH);

        lpddi->guidFFDriver = GUID_NULL;
        return S_OK;
    }

    return S_FALSE;
}

static HRESULT joydev_enum_deviceW(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEW lpddi, DWORD version, int id)
{
    char name[MAX_PATH];
    char friendly[32];
    IOHIDDeviceRef device;
    BOOL is_joystick;

    TRACE("dwDevType %u dwFlags 0x%08x version 0x%04x id %d\n", dwDevType, dwFlags, version, id);

    if (id >= find_joystick_devices()) return E_FAIL;

    device = get_device_ref(id);

    if ((dwDevType == 0) ||
    ((dwDevType == DIDEVTYPE_JOYSTICK) && (version >= 0x0300 && version < 0x0800)) ||
    (((dwDevType == DI8DEVCLASS_GAMECTRL) || (dwDevType == DI8DEVTYPE_JOYSTICK)) && (version >= 0x0800))) {

        if (dwFlags & DIEDFL_FORCEFEEDBACK) {
            if(!device)
                return S_FALSE;
            if(get_ff(device, NULL) != S_OK)
                return S_FALSE;
        }
        is_joystick = get_device_property_long(device, CFSTR(kIOHIDDeviceUsageKey)) == kHIDUsage_GD_Joystick;
        /* Return joystick */
        lpddi->guidInstance = DInput_Wine_OsX_Joystick_GUID;
        lpddi->guidInstance.Data3 = id;
        lpddi->guidProduct = DInput_PIDVID_Product_GUID;
        lpddi->guidProduct.Data1 = make_vid_pid(device);
        lpddi->dwDevType = get_device_type(version, is_joystick);
        lpddi->dwDevType |= DIDEVTYPE_HID;
        lpddi->wUsagePage = 0x01; /* Desktop */
        if (is_joystick)
            lpddi->wUsage = 0x04; /* Joystick */
        else
            lpddi->wUsage = 0x05; /* Game Pad */
        sprintf(friendly, "Joystick %d", id);
        MultiByteToWideChar(CP_ACP, 0, friendly, -1, lpddi->tszInstanceName, MAX_PATH);

        /* get the device name */
        get_osx_device_name(id, name, MAX_PATH);

        MultiByteToWideChar(CP_ACP, 0, name, -1, lpddi->tszProductName, MAX_PATH);
        lpddi->guidFFDriver = GUID_NULL;
        return S_OK;
    }

    return S_FALSE;
}

static const char *osx_ff_axis_name(UInt8 axis)
{
    static char ret[6];
    switch(axis){
    case FFJOFS_X:
        return "FFJOFS_X";
    case FFJOFS_Y:
        return "FFJOFS_Y";
    case FFJOFS_Z:
        return "FFJOFS_Z";
    }
    sprintf(ret, "%u", (unsigned int)axis);
    return ret;
}

static BOOL osx_axis_has_ff(FFCAPABILITIES *ffcaps, UInt8 axis)
{
    int i;
    for(i = 0; i < ffcaps->numFfAxes; ++i)
        if(ffcaps->ffAxes[i] == axis)
            return TRUE;
    return FALSE;
}

static HRESULT alloc_device(REFGUID rguid, IDirectInputImpl *dinput,
                            JoystickImpl **pdev, unsigned short index)
{
    DWORD i;
    IOHIDDeviceRef device;
    JoystickImpl* newDevice;
    char name[MAX_PATH];
    HRESULT hr;
    LPDIDATAFORMAT df = NULL;
    int idx = 0;
    int axis_map[8]; /* max axes */
    int slider_count = 0;
    FFCAPABILITIES ffcaps;

    TRACE("%s %p %p %hu\n", debugstr_guid(rguid), dinput, pdev, index);

    newDevice = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(JoystickImpl));
    if (newDevice == 0) {
        WARN("out of memory\n");
        *pdev = 0;
        return DIERR_OUTOFMEMORY;
    }

    newDevice->id = index;

    device = get_device_ref(index);

    newDevice->generic.guidInstance = DInput_Wine_OsX_Joystick_GUID;
    newDevice->generic.guidInstance.Data3 = index;
    newDevice->generic.guidProduct = DInput_PIDVID_Product_GUID;
    newDevice->generic.guidProduct.Data1 = make_vid_pid(device);
    newDevice->generic.joy_polldev = poll_osx_device_state;

    /* get the device name */
    get_osx_device_name(index, name, MAX_PATH);
    TRACE("Name %s\n",name);

    /* copy the device name */
    newDevice->generic.name = HeapAlloc(GetProcessHeap(),0,strlen(name) + 1);
    strcpy(newDevice->generic.name, name);

    list_init(&newDevice->effects);
    if(get_ff(device, &newDevice->ff) == S_OK){
        newDevice->generic.devcaps.dwFlags |= DIDC_FORCEFEEDBACK;

        hr = FFDeviceGetForceFeedbackCapabilities(newDevice->ff, &ffcaps);
        if(SUCCEEDED(hr)){
            TRACE("FF Capabilities:\n");
            TRACE("\tsupportedEffects: 0x%x\n", (unsigned int)ffcaps.supportedEffects);
            TRACE("\temulatedEffects: 0x%x\n", (unsigned int)ffcaps.emulatedEffects);
            TRACE("\tsubType: 0x%x\n", (unsigned int)ffcaps.subType);
            TRACE("\tnumFfAxes: %u\n", (unsigned int)ffcaps.numFfAxes);
            TRACE("\tffAxes: [");
            for(i = 0; i < ffcaps.numFfAxes; ++i){
                TRACE("%s", osx_ff_axis_name(ffcaps.ffAxes[i]));
                if(i < ffcaps.numFfAxes - 1)
                    TRACE(", ");
            }
            TRACE("]\n");
            TRACE("\tstorageCapacity: %u\n", (unsigned int)ffcaps.storageCapacity);
            TRACE("\tplaybackCapacity: %u\n", (unsigned int)ffcaps.playbackCapacity);
        }

        hr = FFDeviceSendForceFeedbackCommand(newDevice->ff, FFSFFC_RESET);
        if(FAILED(hr))
            WARN("FFDeviceSendForceFeedbackCommand(FFSFFC_RESET) failed: %08x\n", hr);

        hr = FFDeviceSendForceFeedbackCommand(newDevice->ff, FFSFFC_SETACTUATORSON);
        if(FAILED(hr))
            WARN("FFDeviceSendForceFeedbackCommand(FFSFFC_SETACTUATORSON) failed: %08x\n", hr);
    }

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
        BOOL has_ff  = FALSE;
        switch (axis_map[i])
        {
            case kHIDUsage_GD_X:
                wine_obj = 0;
                has_ff = (newDevice->ff != 0) && osx_axis_has_ff(&ffcaps, FFJOFS_X);
                break;
            case kHIDUsage_GD_Y:
                wine_obj = 1;
                has_ff = (newDevice->ff != 0) && osx_axis_has_ff(&ffcaps, FFJOFS_Y);
                break;
            case kHIDUsage_GD_Z:
                wine_obj = 2;
                has_ff = (newDevice->ff != 0) && osx_axis_has_ff(&ffcaps, FFJOFS_Z);
                break;
            case kHIDUsage_GD_Rx:
                wine_obj = 3;
                has_ff = (newDevice->ff != 0) && osx_axis_has_ff(&ffcaps, FFJOFS_RX);
                break;
            case kHIDUsage_GD_Ry:
                wine_obj = 4;
                has_ff = (newDevice->ff != 0) && osx_axis_has_ff(&ffcaps, FFJOFS_RY);
                break;
            case kHIDUsage_GD_Rz:
                wine_obj = 5;
                has_ff = (newDevice->ff != 0) && osx_axis_has_ff(&ffcaps, FFJOFS_RZ);
                break;
            case kHIDUsage_GD_Slider:
                wine_obj = 6 + slider_count;
                has_ff = (newDevice->ff != 0) && osx_axis_has_ff(&ffcaps, FFJOFS_SLIDER(slider_count));
                slider_count++;
                break;
        }
        if (wine_obj < 0 ) continue;

        memcpy(&df->rgodf[idx], &c_dfDIJoystick2.rgodf[wine_obj], df->dwObjSize);
        df->rgodf[idx].dwType = DIDFT_MAKEINSTANCE(wine_obj) | DIDFT_ABSAXIS;
        if(has_ff)
            df->rgodf[idx].dwFlags |= DIDOI_FFACTUATOR;
        ++idx;
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
    newDevice->generic.devcaps.dwFlags |= DIDC_ATTACHED;
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
        TRACE("allocated device %p\n", newDevice);
        _dump_DIDATAFORMAT(newDevice->generic.base.data_format.wine_df);
        _dump_DIDEVCAPS(&newDevice->generic.devcaps);
    }

    *pdev = newDevice;

    return DI_OK;

FAILED:
    hr = DIERR_OUTOFMEMORY;
    if (newDevice->ff) FFReleaseDevice(newDevice->ff);
    if (newDevice->elements) CFRelease(newDevice->elements);
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

static HRESULT WINAPI JoystickWImpl_GetProperty(LPDIRECTINPUTDEVICE8W iface, REFGUID rguid, LPDIPROPHEADER pdiph)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8W(iface);

    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(rguid), pdiph);
    _dump_DIPROPHEADER(pdiph);

    if (!IS_DIPROP(rguid)) return DI_OK;

    switch (LOWORD(rguid)) {
    case (DWORD_PTR) DIPROP_GUIDANDPATH:
    {
        static const WCHAR formatW[] = {'\\','\\','?','\\','h','i','d','#','v','i','d','_','%','0','4','x','&',
                                        'p','i','d','_','%','0','4','x','&','%','s','_','%','i',0};
        static const WCHAR miW[] = {'m','i',0};
        static const WCHAR igW[] = {'i','g',0};

        BOOL is_gamepad;
        IOHIDDeviceRef device = get_device_ref(This->id);
        LPDIPROPGUIDANDPATH pd = (LPDIPROPGUIDANDPATH)pdiph;
        WORD vid = get_device_property_long(device, CFSTR(kIOHIDVendorIDKey));
        WORD pid = get_device_property_long(device, CFSTR(kIOHIDProductIDKey));

        if (!pid || !vid)
            return DIERR_UNSUPPORTED;

        is_gamepad = is_xinput_device(&This->generic.devcaps, vid, pid);
        pd->guidClass = GUID_DEVCLASS_HIDCLASS;
        sprintfW(pd->wszPath, formatW, vid, pid, is_gamepad ? igW : miW, This->id);

        TRACE("DIPROP_GUIDANDPATH(%s, %s): returning fake path\n", debugstr_guid(&pd->guidClass), debugstr_w(pd->wszPath));
        break;
    }

    default:
        return JoystickWGenericImpl_GetProperty(iface, rguid, pdiph);
    }

    return DI_OK;
}

static HRESULT WINAPI JoystickAImpl_GetProperty(LPDIRECTINPUTDEVICE8A iface, REFGUID rguid, LPDIPROPHEADER pdiph)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8A(iface);
    return JoystickWImpl_GetProperty(IDirectInputDevice8W_from_impl(This), rguid, pdiph);
}

static HRESULT osx_set_autocenter(JoystickImpl *This,
        const DIPROPDWORD *header)
{
    UInt32 v;
    HRESULT hr;
    if(!This->ff)
        return DIERR_UNSUPPORTED;
    v = header->dwData;
    hr = osx_to_win32_hresult(FFDeviceSetForceFeedbackProperty(This->ff, FFPROP_AUTOCENTER, &v));
    TRACE("returning: %08x\n", hr);
    return hr;
}

static HRESULT osx_set_ffgain(JoystickImpl *This, const DIPROPDWORD *header)
{
    UInt32 v;
    HRESULT hr;
    if(!This->ff)
        return DIERR_UNSUPPORTED;
    v = header->dwData;
    hr = osx_to_win32_hresult(FFDeviceSetForceFeedbackProperty(This->ff, FFPROP_FFGAIN, &v));
    TRACE("returning: %08x\n", hr);
    return hr;
}

static HRESULT WINAPI JoystickWImpl_SetProperty(IDirectInputDevice8W *iface,
        const GUID *prop, const DIPROPHEADER *header)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8W(iface);

    TRACE("%p %s %p\n", This, debugstr_guid(prop), header);

    switch(LOWORD(prop))
    {
    case (DWORD_PTR)DIPROP_AUTOCENTER:
        return osx_set_autocenter(This, (const DIPROPDWORD *)header);
    case (DWORD_PTR)DIPROP_FFGAIN:
        return osx_set_ffgain(This, (const DIPROPDWORD *)header);
    }

    return JoystickWGenericImpl_SetProperty(iface, prop, header);
}

static HRESULT WINAPI JoystickAImpl_SetProperty(IDirectInputDevice8A *iface,
        const GUID *prop, const DIPROPHEADER *header)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8A(iface);

    TRACE("%p %s %p\n", This, debugstr_guid(prop), header);

    switch(LOWORD(prop))
    {
    case (DWORD_PTR)DIPROP_AUTOCENTER:
        return osx_set_autocenter(This, (const DIPROPDWORD *)header);
    case (DWORD_PTR)DIPROP_FFGAIN:
        return osx_set_ffgain(This, (const DIPROPDWORD *)header);
    }

    return JoystickAGenericImpl_SetProperty(iface, prop, header);
}

static CFUUIDRef effect_win_to_mac(const GUID *effect)
{
#define DO_MAP(X) \
    if(IsEqualGUID(&GUID_##X, effect)) \
        return kFFEffectType_##X##_ID;
    DO_MAP(ConstantForce)
    DO_MAP(RampForce)
    DO_MAP(Square)
    DO_MAP(Sine)
    DO_MAP(Triangle)
    DO_MAP(SawtoothUp)
    DO_MAP(SawtoothDown)
    DO_MAP(Spring)
    DO_MAP(Damper)
    DO_MAP(Inertia)
    DO_MAP(Friction)
    DO_MAP(CustomForce)
#undef DO_MAP
    WARN("Unknown effect GUID! %s\n", debugstr_guid(effect));
    return 0;
}

static HRESULT WINAPI JoystickWImpl_CreateEffect(IDirectInputDevice8W *iface,
        const GUID *type, const DIEFFECT *params, IDirectInputEffect **out,
        IUnknown *outer)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8W(iface);
    EffectImpl *effect;
    HRESULT hr;

    TRACE("(%p)->(%s %p %p %p)\n", This, debugstr_guid(type), params, out, outer);
    dump_DIEFFECT(params, type, 0);

    if(!This->ff){
        TRACE("No force feedback support\n");
        *out = NULL;
        return DIERR_UNSUPPORTED;
    }

    if(outer)
        WARN("aggregation not implemented\n");

    effect = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*This));
    effect->IDirectInputEffect_iface.lpVtbl = &EffectVtbl;
    effect->ref = 1;
    effect->guid = *type;
    effect->device = This;

    /* Mac's FFEFFECT and Win's DIEFFECT are binary identical. */
    hr = osx_to_win32_hresult(FFDeviceCreateEffect(This->ff,
                effect_win_to_mac(type), (FFEFFECT*)params, &effect->effect));
    if(FAILED(hr)){
        WARN("FFDeviceCreateEffect failed: %08x\n", hr);
        HeapFree(GetProcessHeap(), 0, effect);
        return hr;
    }

    list_add_tail(&This->effects, &effect->entry);
    *out = &effect->IDirectInputEffect_iface;

    TRACE("allocated effect: %p\n", effect);

    return S_OK;
}

static HRESULT WINAPI JoystickAImpl_CreateEffect(IDirectInputDevice8A *iface,
        const GUID *type, const DIEFFECT *params, IDirectInputEffect **out,
        IUnknown *outer)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8A(iface);

    TRACE("(%p)->(%s %p %p %p)\n", This, debugstr_guid(type), params, out, outer);

    return JoystickWImpl_CreateEffect(&This->generic.base.IDirectInputDevice8W_iface,
            type, params, out, outer);
}

static HRESULT WINAPI JoystickWImpl_SendForceFeedbackCommand(IDirectInputDevice8W *iface,
        DWORD flags)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8W(iface);
    HRESULT hr;

    TRACE("%p 0x%x\n", This, flags);

    if(!This->ff)
        return DI_NOEFFECT;

    hr = osx_to_win32_hresult(FFDeviceSendForceFeedbackCommand(This->ff, flags));
    if(FAILED(hr)){
        WARN("FFDeviceSendForceFeedbackCommand failed: %08x\n", hr);
        return hr;
    }

    return S_OK;
}

static HRESULT WINAPI JoystickAImpl_SendForceFeedbackCommand(IDirectInputDevice8A *iface,
        DWORD flags)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8A(iface);

    TRACE("%p 0x%x\n", This, flags);

    return JoystickWImpl_SendForceFeedbackCommand(&This->generic.base.IDirectInputDevice8W_iface, flags);
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
    JoystickAImpl_GetProperty,
    JoystickAImpl_SetProperty,
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
    JoystickAImpl_CreateEffect,
    IDirectInputDevice2AImpl_EnumEffects,
    IDirectInputDevice2AImpl_GetEffectInfo,
    IDirectInputDevice2AImpl_GetForceFeedbackState,
    JoystickAImpl_SendForceFeedbackCommand,
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
    JoystickWImpl_GetProperty,
    JoystickWImpl_SetProperty,
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
    JoystickWImpl_CreateEffect,
    IDirectInputDevice2WImpl_EnumEffects,
    IDirectInputDevice2WImpl_GetEffectInfo,
    IDirectInputDevice2WImpl_GetForceFeedbackState,
    JoystickWImpl_SendForceFeedbackCommand,
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

static HRESULT WINAPI effect_QueryInterface(IDirectInputEffect *iface,
        const GUID *guid, void **out)
{
    EffectImpl *This = impl_from_IDirectInputEffect(iface);

    TRACE("%p %s %p\n", This, debugstr_guid(guid), out);

    if(IsEqualIID(guid, &IID_IUnknown) || IsEqualIID(guid, &IID_IDirectInputEffect)){
        *out = iface;
        IDirectInputEffect_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI effect_AddRef(IDirectInputEffect *iface)
{
    EffectImpl *This = impl_from_IDirectInputEffect(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("%p, ref is now: %u\n", This, ref);
    return ref;
}

static ULONG WINAPI effect_Release(IDirectInputEffect *iface)
{
    EffectImpl *This = impl_from_IDirectInputEffect(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("%p, ref is now: %u\n", This, ref);

    if(!ref){
        list_remove(&This->entry);
        FFDeviceReleaseEffect(This->device->ff, This->effect);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI effect_Initialize(IDirectInputEffect *iface, HINSTANCE hinst,
        DWORD version, const GUID *guid)
{
    EffectImpl *This = impl_from_IDirectInputEffect(iface);
    TRACE("%p %p 0x%x, %s\n", This, hinst, version, debugstr_guid(guid));
    return S_OK;
}

static HRESULT WINAPI effect_GetEffectGuid(IDirectInputEffect *iface, GUID *out)
{
    EffectImpl *This = impl_from_IDirectInputEffect(iface);
    TRACE("%p %p\n", This, out);
    *out = This->guid;
    return S_OK;
}

static HRESULT WINAPI effect_GetParameters(IDirectInputEffect *iface,
        DIEFFECT *effect, DWORD flags)
{
    EffectImpl *This = impl_from_IDirectInputEffect(iface);
    TRACE("%p %p 0x%x\n", This, effect, flags);
    return osx_to_win32_hresult(FFEffectGetParameters(This->effect, (FFEFFECT*)effect, flags));
}

static HRESULT WINAPI effect_SetParameters(IDirectInputEffect *iface,
        const DIEFFECT *effect, DWORD flags)
{
    EffectImpl *This = impl_from_IDirectInputEffect(iface);
    TRACE("%p %p 0x%x\n", This, effect, flags);
    dump_DIEFFECT(effect, &This->guid, flags);
    return osx_to_win32_hresult(FFEffectSetParameters(This->effect, (FFEFFECT*)effect, flags));
}

static HRESULT WINAPI effect_Start(IDirectInputEffect *iface, DWORD iterations,
        DWORD flags)
{
    EffectImpl *This = impl_from_IDirectInputEffect(iface);
    TRACE("%p 0x%x 0x%x\n", This, iterations, flags);
    return osx_to_win32_hresult(FFEffectStart(This->effect, iterations, flags));
}

static HRESULT WINAPI effect_Stop(IDirectInputEffect *iface)
{
    EffectImpl *This = impl_from_IDirectInputEffect(iface);
    TRACE("%p\n", This);
    return osx_to_win32_hresult(FFEffectStop(This->effect));
}

static HRESULT WINAPI effect_GetEffectStatus(IDirectInputEffect *iface, DWORD *flags)
{
    EffectImpl *This = impl_from_IDirectInputEffect(iface);
    TRACE("%p %p\n", This, flags);
    return osx_to_win32_hresult(FFEffectGetEffectStatus(This->effect, (UInt32*)flags));
}

static HRESULT WINAPI effect_Download(IDirectInputEffect *iface)
{
    EffectImpl *This = impl_from_IDirectInputEffect(iface);
    TRACE("%p\n", This);
    return osx_to_win32_hresult(FFEffectDownload(This->effect));
}

static HRESULT WINAPI effect_Unload(IDirectInputEffect *iface)
{
    EffectImpl *This = impl_from_IDirectInputEffect(iface);
    TRACE("%p\n", This);
    return osx_to_win32_hresult(FFEffectUnload(This->effect));
}

static HRESULT WINAPI effect_Escape(IDirectInputEffect *iface, DIEFFESCAPE *escape)
{
    EffectImpl *This = impl_from_IDirectInputEffect(iface);
    TRACE("%p %p\n", This, escape);
    return osx_to_win32_hresult(FFEffectEscape(This->effect, (FFEFFESCAPE*)escape));
}

static const IDirectInputEffectVtbl EffectVtbl = {
    effect_QueryInterface,
    effect_AddRef,
    effect_Release,
    effect_Initialize,
    effect_GetEffectGuid,
    effect_GetParameters,
    effect_SetParameters,
    effect_Start,
    effect_Stop,
    effect_GetEffectStatus,
    effect_Download,
    effect_Unload,
    effect_Escape
};

#else /* HAVE_IOHIDMANAGERCREATE */

const struct dinput_device joystick_osx_device = {
  "Wine OS X joystick driver",
  NULL,
  NULL,
  NULL
};

#endif /* HAVE_IOHIDMANAGERCREATE */
