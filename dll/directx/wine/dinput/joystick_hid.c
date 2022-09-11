/*  DirectInput HID Joystick device
 *
 * Copyright 2021 Rémi Bernon for CodeWeavers
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

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winuser.h"
#include "winerror.h"
#include "winreg.h"

#include "ddk/hidsdi.h"
#include "setupapi.h"
#include "devguid.h"
#include "dinput.h"
#include "setupapi.h"

#include "dinput_private.h"
#include "device_private.h"

#include "initguid.h"
#include "devpkey.h"

#include "wine/debug.h"
#include "wine/hid.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

DEFINE_GUID( GUID_DEVINTERFACE_WINEXINPUT,0x6c53d5fd,0x6480,0x440f,0xb6,0x18,0x47,0x67,0x50,0xc5,0xe1,0xa6 );
DEFINE_GUID( hid_joystick_guid, 0x9e573edb, 0x7734, 0x11d2, 0x8d, 0x4a, 0x23, 0x90, 0x3f, 0xb6, 0xbd, 0xf7 );
DEFINE_GUID( device_path_guid, 0x00000000, 0x0000, 0x0000, 0x8d, 0x4a, 0x23, 0x90, 0x3f, 0xb6, 0xbd, 0xf8 );
DEFINE_DEVPROPKEY( DEVPROPKEY_HID_HANDLE, 0xbc62e415, 0xf4fe, 0x405c, 0x8e, 0xda, 0x63, 0x6f, 0xb5, 0x9f, 0x08, 0x98, 2 );

struct pid_control_report
{
    BYTE id;
    UINT collection;
    UINT control_coll;
};

struct pid_effect_update
{
    BYTE id;
    UINT collection;
    UINT type_coll;
    UINT axes_coll;
    UINT axis_count;
    UINT direction_coll;
    UINT direction_count;
    struct hid_value_caps *axis_caps[6];
    struct hid_value_caps *direction_caps[6];
    struct hid_value_caps *duration_caps;
    struct hid_value_caps *gain_caps;
    struct hid_value_caps *sample_period_caps;
    struct hid_value_caps *start_delay_caps;
    struct hid_value_caps *trigger_button_caps;
    struct hid_value_caps *trigger_repeat_interval_caps;
};

struct pid_set_periodic
{
    BYTE id;
    UINT collection;
    struct hid_value_caps *magnitude_caps;
    struct hid_value_caps *period_caps;
    struct hid_value_caps *phase_caps;
    struct hid_value_caps *offset_caps;
};

struct pid_set_envelope
{
    BYTE id;
    UINT collection;
    struct hid_value_caps *attack_level_caps;
    struct hid_value_caps *attack_time_caps;
    struct hid_value_caps *fade_level_caps;
    struct hid_value_caps *fade_time_caps;
};

struct pid_set_condition
{
    BYTE id;
    UINT collection;
    struct hid_value_caps *center_point_offset_caps;
    struct hid_value_caps *positive_coefficient_caps;
    struct hid_value_caps *negative_coefficient_caps;
    struct hid_value_caps *positive_saturation_caps;
    struct hid_value_caps *negative_saturation_caps;
    struct hid_value_caps *dead_band_caps;
};

struct pid_set_constant_force
{
    BYTE id;
    UINT collection;
    struct hid_value_caps *magnitude_caps;
};

struct pid_set_ramp_force
{
    BYTE id;
    UINT collection;
    struct hid_value_caps *start_caps;
    struct hid_value_caps *end_caps;
};

struct pid_device_gain
{
    BYTE id;
    UINT collection;
    struct hid_value_caps *device_gain_caps;
};

struct pid_device_pool
{
    BYTE id;
    UINT collection;
    struct hid_value_caps *device_managed_caps;
};

struct pid_block_free
{
    BYTE id;
    UINT collection;
};

struct pid_block_load
{
    BYTE id;
    UINT collection;
    UINT status_coll;
};

struct pid_new_effect
{
    BYTE id;
    UINT collection;
    UINT type_coll;
};

struct pid_effect_state
{
    BYTE id;
    UINT collection;
    struct hid_value_caps *safety_switch_caps;
    struct hid_value_caps *actuator_power_caps;
    struct hid_value_caps *actuator_override_switch_caps;
};

struct hid_joystick
{
    struct dinput_device base;
    LONG internal_ref;

    HANDLE device;
    OVERLAPPED read_ovl;
    PHIDP_PREPARSED_DATA preparsed;

    WCHAR device_path[MAX_PATH];
    HIDD_ATTRIBUTES attrs;
    HIDP_CAPS caps;

    char *input_report_buf;
    char *output_report_buf;
    char *feature_report_buf;
    USAGE_AND_PAGE *usages_buf;
    UINT usages_count;

    BYTE effect_inuse[255];
    struct list effect_list;
    struct pid_control_report pid_device_control;
    struct pid_control_report pid_effect_control;
    struct pid_effect_update pid_effect_update;
    struct pid_set_periodic pid_set_periodic;
    struct pid_set_envelope pid_set_envelope;
    struct pid_set_condition pid_set_condition;
    struct pid_set_constant_force pid_set_constant_force;
    struct pid_set_ramp_force pid_set_ramp_force;
    struct pid_device_gain pid_device_gain;
    struct pid_device_pool pid_device_pool;
    struct pid_block_free pid_block_free;
    struct pid_block_load pid_block_load;
    struct pid_new_effect pid_new_effect;
    struct pid_effect_state pid_effect_state;
};

static inline struct hid_joystick *impl_from_IDirectInputDevice8W( IDirectInputDevice8W *iface )
{
    return CONTAINING_RECORD( CONTAINING_RECORD( iface, struct dinput_device, IDirectInputDevice8W_iface ),
                              struct hid_joystick, base );
}

struct hid_joystick_effect
{
    IDirectInputEffect IDirectInputEffect_iface;
    LONG ref;
    USAGE type;
    ULONG index;

    struct list entry;
    struct hid_joystick *joystick;

    DWORD axes[6];
    LONG directions[6];
    DICONSTANTFORCE constant_force;
    DIRAMPFORCE ramp_force;
    DICONDITION condition[6];
    DIENVELOPE envelope;
    DIPERIODIC periodic;
    DIEFFECT params;
    DWORD modified;
    DWORD flags;
    DWORD status;

    char *effect_control_buf;
    char *effect_update_buf;
    char *type_specific_buf;
    char *set_envelope_buf;
};

static inline struct hid_joystick_effect *impl_from_IDirectInputEffect( IDirectInputEffect *iface )
{
    return CONTAINING_RECORD( iface, struct hid_joystick_effect, IDirectInputEffect_iface );
}

static inline BOOL is_exclusively_acquired( struct hid_joystick *joystick )
{
    return joystick->base.status == STATUS_ACQUIRED && (joystick->base.dwCoopLevel & DISCL_EXCLUSIVE);
}

static const GUID *object_usage_to_guid( USAGE usage_page, USAGE usage )
{
    switch (usage_page)
    {
    case HID_USAGE_PAGE_BUTTON: return &GUID_Button;
    case HID_USAGE_PAGE_SIMULATION:
        switch (usage)
        {
        case HID_USAGE_SIMULATION_STEERING: return &GUID_XAxis;
        case HID_USAGE_SIMULATION_ACCELERATOR: return &GUID_YAxis;
        case HID_USAGE_SIMULATION_BRAKE: return &GUID_RzAxis;
        case HID_USAGE_SIMULATION_RUDDER: return &GUID_RzAxis;
        case HID_USAGE_SIMULATION_THROTTLE: return &GUID_Slider;
        }
        break;
    case HID_USAGE_PAGE_GENERIC:
        switch (usage)
        {
        case HID_USAGE_GENERIC_X: return &GUID_XAxis;
        case HID_USAGE_GENERIC_Y: return &GUID_YAxis;
        case HID_USAGE_GENERIC_Z: return &GUID_ZAxis;
        case HID_USAGE_GENERIC_WHEEL: return &GUID_ZAxis;
        case HID_USAGE_GENERIC_RX: return &GUID_RxAxis;
        case HID_USAGE_GENERIC_RY: return &GUID_RyAxis;
        case HID_USAGE_GENERIC_RZ: return &GUID_RzAxis;
        case HID_USAGE_GENERIC_SLIDER: return &GUID_Slider;
        case HID_USAGE_GENERIC_HATSWITCH: return &GUID_POV;
        }
        break;
    }

    return &GUID_Unknown;
}

static inline USAGE effect_guid_to_usage( const GUID *guid )
{
    if (IsEqualGUID( guid, &GUID_CustomForce )) return PID_USAGE_ET_CUSTOM_FORCE_DATA;
    if (IsEqualGUID( guid, &GUID_ConstantForce )) return PID_USAGE_ET_CONSTANT_FORCE;
    if (IsEqualGUID( guid, &GUID_RampForce )) return PID_USAGE_ET_RAMP;
    if (IsEqualGUID( guid, &GUID_Square )) return PID_USAGE_ET_SQUARE;
    if (IsEqualGUID( guid, &GUID_Sine )) return PID_USAGE_ET_SINE;
    if (IsEqualGUID( guid, &GUID_Triangle )) return PID_USAGE_ET_TRIANGLE;
    if (IsEqualGUID( guid, &GUID_SawtoothUp )) return PID_USAGE_ET_SAWTOOTH_UP;
    if (IsEqualGUID( guid, &GUID_SawtoothDown )) return PID_USAGE_ET_SAWTOOTH_DOWN;
    if (IsEqualGUID( guid, &GUID_Spring )) return PID_USAGE_ET_SPRING;
    if (IsEqualGUID( guid, &GUID_Damper )) return PID_USAGE_ET_DAMPER;
    if (IsEqualGUID( guid, &GUID_Inertia )) return PID_USAGE_ET_INERTIA;
    if (IsEqualGUID( guid, &GUID_Friction )) return PID_USAGE_ET_FRICTION;
    return 0;
}

static inline const GUID *effect_usage_to_guid( USAGE usage )
{
    switch (usage)
    {
    case PID_USAGE_ET_CUSTOM_FORCE_DATA: return &GUID_CustomForce;
    case PID_USAGE_ET_CONSTANT_FORCE: return &GUID_ConstantForce;
    case PID_USAGE_ET_RAMP: return &GUID_RampForce;
    case PID_USAGE_ET_SQUARE: return &GUID_Square;
    case PID_USAGE_ET_SINE: return &GUID_Sine;
    case PID_USAGE_ET_TRIANGLE: return &GUID_Triangle;
    case PID_USAGE_ET_SAWTOOTH_UP: return &GUID_SawtoothUp;
    case PID_USAGE_ET_SAWTOOTH_DOWN: return &GUID_SawtoothDown;
    case PID_USAGE_ET_SPRING: return &GUID_Spring;
    case PID_USAGE_ET_DAMPER: return &GUID_Damper;
    case PID_USAGE_ET_INERTIA: return &GUID_Inertia;
    case PID_USAGE_ET_FRICTION: return &GUID_Friction;
    }
    return &GUID_Unknown;
}

static const WCHAR *effect_guid_to_string( const GUID *guid )
{
    if (IsEqualGUID( guid, &GUID_CustomForce )) return L"GUID_CustomForce";
    if (IsEqualGUID( guid, &GUID_ConstantForce )) return L"GUID_ConstantForce";
    if (IsEqualGUID( guid, &GUID_RampForce )) return L"GUID_RampForce";
    if (IsEqualGUID( guid, &GUID_Square )) return L"GUID_Square";
    if (IsEqualGUID( guid, &GUID_Sine )) return L"GUID_Sine";
    if (IsEqualGUID( guid, &GUID_Triangle )) return L"GUID_Triangle";
    if (IsEqualGUID( guid, &GUID_SawtoothUp )) return L"GUID_SawtoothUp";
    if (IsEqualGUID( guid, &GUID_SawtoothDown )) return L"GUID_SawtoothDown";
    if (IsEqualGUID( guid, &GUID_Spring )) return L"GUID_Spring";
    if (IsEqualGUID( guid, &GUID_Damper )) return L"GUID_Damper";
    if (IsEqualGUID( guid, &GUID_Inertia )) return L"GUID_Inertia";
    if (IsEqualGUID( guid, &GUID_Friction )) return L"GUID_Friction";
    return L"GUID_Unknown";
}

static const WCHAR *object_usage_to_string( DIDEVICEOBJECTINSTANCEW *instance )
{
    switch (MAKELONG(instance->wUsage, instance->wUsagePage))
    {
    case MAKELONG(HID_USAGE_DIGITIZER_TIP_PRESSURE, HID_USAGE_PAGE_DIGITIZER): return L"Tip Pressure";
    case MAKELONG(HID_USAGE_CONSUMER_VOLUME, HID_USAGE_PAGE_CONSUMER): return L"Volume";

    case MAKELONG(HID_USAGE_GENERIC_HATSWITCH, HID_USAGE_PAGE_GENERIC): return L"Hat Switch";
    case MAKELONG(HID_USAGE_GENERIC_JOYSTICK, HID_USAGE_PAGE_GENERIC): return L"Joystick";
    case MAKELONG(HID_USAGE_GENERIC_RX, HID_USAGE_PAGE_GENERIC): return L"X Rotation";
    case MAKELONG(HID_USAGE_GENERIC_RY, HID_USAGE_PAGE_GENERIC): return L"Y Rotation";
    case MAKELONG(HID_USAGE_GENERIC_RZ, HID_USAGE_PAGE_GENERIC): return L"Z Rotation";
    case MAKELONG(HID_USAGE_GENERIC_WHEEL, HID_USAGE_PAGE_GENERIC): return L"Wheel";
    case MAKELONG(HID_USAGE_GENERIC_X, HID_USAGE_PAGE_GENERIC): return L"X Axis";
    case MAKELONG(HID_USAGE_GENERIC_Y, HID_USAGE_PAGE_GENERIC): return L"Y Axis";
    case MAKELONG(HID_USAGE_GENERIC_Z, HID_USAGE_PAGE_GENERIC): return L"Z Axis";

    case MAKELONG(PID_USAGE_ATTACK_LEVEL, HID_USAGE_PAGE_PID): return L"Attack Level";
    case MAKELONG(PID_USAGE_ATTACK_TIME, HID_USAGE_PAGE_PID): return L"Attack Time";
    case MAKELONG(PID_USAGE_AXES_ENABLE, HID_USAGE_PAGE_PID): return L"Axes Enable";

    case MAKELONG(PID_USAGE_DC_DEVICE_CONTINUE, HID_USAGE_PAGE_PID): return L"DC Device Continue";
    case MAKELONG(PID_USAGE_DC_DEVICE_PAUSE, HID_USAGE_PAGE_PID): return L"DC Device Pause";
    case MAKELONG(PID_USAGE_DC_DEVICE_RESET, HID_USAGE_PAGE_PID): return L"DC Device Reset";
    case MAKELONG(PID_USAGE_DC_DISABLE_ACTUATORS, HID_USAGE_PAGE_PID): return L"DC Disable Actuators";
    case MAKELONG(PID_USAGE_DC_ENABLE_ACTUATORS, HID_USAGE_PAGE_PID): return L"DC Enable Actuators";
    case MAKELONG(PID_USAGE_DC_STOP_ALL_EFFECTS, HID_USAGE_PAGE_PID): return L"DC Stop All Effects";

    case MAKELONG(PID_USAGE_DEVICE_GAIN, HID_USAGE_PAGE_PID): return L"Device Gain";
    case MAKELONG(PID_USAGE_DEVICE_GAIN_REPORT, HID_USAGE_PAGE_PID): return L"Device Gain Report";
    case MAKELONG(PID_USAGE_CP_OFFSET, HID_USAGE_PAGE_PID): return L"CP Offset";
    case MAKELONG(PID_USAGE_DEAD_BAND, HID_USAGE_PAGE_PID): return L"Dead Band";
    case MAKELONG(PID_USAGE_DEVICE_CONTROL, HID_USAGE_PAGE_PID): return L"PID Device Control";
    case MAKELONG(PID_USAGE_DEVICE_CONTROL_REPORT, HID_USAGE_PAGE_PID): return L"PID Device Control Report";
    case MAKELONG(PID_USAGE_DIRECTION, HID_USAGE_PAGE_PID): return L"Direction";
    case MAKELONG(PID_USAGE_DIRECTION_ENABLE, HID_USAGE_PAGE_PID): return L"Direction Enable";
    case MAKELONG(PID_USAGE_DURATION, HID_USAGE_PAGE_PID): return L"Duration";
    case MAKELONG(PID_USAGE_EFFECT_BLOCK_INDEX, HID_USAGE_PAGE_PID): return L"Effect Block Index";
    case MAKELONG(PID_USAGE_EFFECT_OPERATION, HID_USAGE_PAGE_PID): return L"Effect Operation";
    case MAKELONG(PID_USAGE_EFFECT_OPERATION_REPORT, HID_USAGE_PAGE_PID): return L"Effect Operation Report";
    case MAKELONG(PID_USAGE_EFFECT_TYPE, HID_USAGE_PAGE_PID): return L"Effect Type";

    case MAKELONG(PID_USAGE_ET_CONSTANT_FORCE, HID_USAGE_PAGE_PID): return L"ET Constant Force";
    case MAKELONG(PID_USAGE_ET_CUSTOM_FORCE_DATA, HID_USAGE_PAGE_PID): return L"ET Custom Force Data";
    case MAKELONG(PID_USAGE_ET_DAMPER, HID_USAGE_PAGE_PID): return L"ET Damper";
    case MAKELONG(PID_USAGE_ET_FRICTION, HID_USAGE_PAGE_PID): return L"ET Friction";
    case MAKELONG(PID_USAGE_ET_INERTIA, HID_USAGE_PAGE_PID): return L"ET Inertia";
    case MAKELONG(PID_USAGE_ET_RAMP, HID_USAGE_PAGE_PID): return L"ET Ramp";
    case MAKELONG(PID_USAGE_ET_SAWTOOTH_DOWN, HID_USAGE_PAGE_PID): return L"ET Sawtooth Down";
    case MAKELONG(PID_USAGE_ET_SAWTOOTH_UP, HID_USAGE_PAGE_PID): return L"ET Sawtooth Up";
    case MAKELONG(PID_USAGE_ET_SINE, HID_USAGE_PAGE_PID): return L"ET Sine";
    case MAKELONG(PID_USAGE_ET_SPRING, HID_USAGE_PAGE_PID): return L"ET Spring";
    case MAKELONG(PID_USAGE_ET_SQUARE, HID_USAGE_PAGE_PID): return L"ET Square";
    case MAKELONG(PID_USAGE_ET_TRIANGLE, HID_USAGE_PAGE_PID): return L"ET Triangle";

    case MAKELONG(PID_USAGE_NEGATIVE_COEFFICIENT, HID_USAGE_PAGE_PID): return L"Negative Coefficient";
    case MAKELONG(PID_USAGE_NEGATIVE_SATURATION, HID_USAGE_PAGE_PID): return L"Negative Saturation";
    case MAKELONG(PID_USAGE_POSITIVE_COEFFICIENT, HID_USAGE_PAGE_PID): return L"Positive Coefficient";
    case MAKELONG(PID_USAGE_POSITIVE_SATURATION, HID_USAGE_PAGE_PID): return L"Positive Saturation";
    case MAKELONG(PID_USAGE_SET_CONDITION_REPORT, HID_USAGE_PAGE_PID): return L"Set Condition Report";
    case MAKELONG(PID_USAGE_TYPE_SPECIFIC_BLOCK_OFFSET, HID_USAGE_PAGE_PID): return L"Type Specific Block Offset";

    case MAKELONG(PID_USAGE_FADE_LEVEL, HID_USAGE_PAGE_PID): return L"Fade Level";
    case MAKELONG(PID_USAGE_FADE_TIME, HID_USAGE_PAGE_PID): return L"Fade Time";
    case MAKELONG(PID_USAGE_LOOP_COUNT, HID_USAGE_PAGE_PID): return L"Loop Count";
    case MAKELONG(PID_USAGE_MAGNITUDE, HID_USAGE_PAGE_PID): return L"Magnitude";
    case MAKELONG(PID_USAGE_OP_EFFECT_START, HID_USAGE_PAGE_PID): return L"Op Effect Start";
    case MAKELONG(PID_USAGE_OP_EFFECT_START_SOLO, HID_USAGE_PAGE_PID): return L"Op Effect Start Solo";
    case MAKELONG(PID_USAGE_OP_EFFECT_STOP, HID_USAGE_PAGE_PID): return L"Op Effect Stop";
    case MAKELONG(PID_USAGE_SET_EFFECT_REPORT, HID_USAGE_PAGE_PID): return L"Set Effect Report";
    case MAKELONG(PID_USAGE_SET_ENVELOPE_REPORT, HID_USAGE_PAGE_PID): return L"Set Envelope Report";
    case MAKELONG(PID_USAGE_SET_PERIODIC_REPORT, HID_USAGE_PAGE_PID): return L"Set Periodic Report";
    case MAKELONG(PID_USAGE_START_DELAY, HID_USAGE_PAGE_PID): return L"Start Delay";
    case MAKELONG(PID_USAGE_STATE_REPORT, HID_USAGE_PAGE_PID): return L"PID State Report";
    case MAKELONG(PID_USAGE_TRIGGER_BUTTON, HID_USAGE_PAGE_PID): return L"Trigger Button";

    case MAKELONG(HID_USAGE_SIMULATION_RUDDER, HID_USAGE_PAGE_SIMULATION): return L"Rudder";
    case MAKELONG(HID_USAGE_SIMULATION_THROTTLE, HID_USAGE_PAGE_SIMULATION): return L"Throttle";
    case MAKELONG(HID_USAGE_SIMULATION_ACCELERATOR, HID_USAGE_PAGE_SIMULATION): return L"Accelerator";
    case MAKELONG(HID_USAGE_SIMULATION_BRAKE, HID_USAGE_PAGE_SIMULATION): return L"Brake";
    case MAKELONG(HID_USAGE_SIMULATION_CLUTCH, HID_USAGE_PAGE_SIMULATION): return L"Clutch";
    case MAKELONG(HID_USAGE_SIMULATION_STEERING, HID_USAGE_PAGE_SIMULATION): return L"Steering";
    default: return NULL;
    }
}

static HRESULT find_next_effect_id( struct hid_joystick *impl, ULONG *index, USAGE type )
{
    struct pid_device_pool *device_pool = &impl->pid_device_pool;
    struct pid_new_effect *new_effect = &impl->pid_new_effect;
    struct pid_block_load *block_load = &impl->pid_block_load;
    ULONG i, count, report_len = impl->caps.FeatureReportByteLength;
    NTSTATUS status;
    USAGE usage;

    if (!device_pool->device_managed_caps)
    {
        for (i = 0; i < ARRAY_SIZE(impl->effect_inuse); ++i)
            if (!impl->effect_inuse[i]) break;
        if (i == ARRAY_SIZE(impl->effect_inuse)) return DIERR_DEVICEFULL;
        impl->effect_inuse[i] = TRUE;
        *index = i + 1;
    }
    else
    {
        status = HidP_InitializeReportForID( HidP_Feature, new_effect->id, impl->preparsed,
                                             impl->feature_report_buf, report_len );
        if (status != HIDP_STATUS_SUCCESS) return status;

        count = 1;
        status = HidP_SetUsages( HidP_Feature, HID_USAGE_PAGE_PID, new_effect->type_coll,
                                 &type, &count, impl->preparsed, impl->feature_report_buf, report_len );
        if (status != HIDP_STATUS_SUCCESS) return status;

        if (!HidD_SetFeature( impl->device, impl->feature_report_buf, report_len )) return DIERR_INPUTLOST;

        status = HidP_InitializeReportForID( HidP_Feature, block_load->id, impl->preparsed,
                                             impl->feature_report_buf, report_len );
        if (status != HIDP_STATUS_SUCCESS) return status;

        if (!HidD_GetFeature( impl->device, impl->feature_report_buf, report_len )) return DIERR_INPUTLOST;

        count = 1;
        status = HidP_GetUsages( HidP_Feature, HID_USAGE_PAGE_PID, block_load->status_coll,
                                 &usage, &count, impl->preparsed, impl->feature_report_buf, report_len );
        if (status != HIDP_STATUS_SUCCESS) return status;

        if (count != 1 || usage == PID_USAGE_BLOCK_LOAD_ERROR) return DIERR_INPUTLOST;
        if (usage == PID_USAGE_BLOCK_LOAD_FULL) return DIERR_DEVICEFULL;

        status = HidP_GetUsageValue( HidP_Feature, HID_USAGE_PAGE_PID, 0, PID_USAGE_EFFECT_BLOCK_INDEX,
                                     index, impl->preparsed, impl->feature_report_buf, report_len );
        if (status != HIDP_STATUS_SUCCESS) return status;
    }

    return DI_OK;
}

typedef BOOL (*enum_object_callback)( struct hid_joystick *impl, struct hid_value_caps *caps,
                                      DIDEVICEOBJECTINSTANCEW *instance, void *data );

static BOOL enum_object( struct hid_joystick *impl, const DIPROPHEADER *filter, DWORD flags,
                         enum_object_callback callback, struct hid_value_caps *caps,
                         DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    if (flags != DIDFT_ALL && !(flags & DIDFT_GETTYPE( instance->dwType ))) return DIENUM_CONTINUE;

    switch (filter->dwHow)
    {
    case DIPH_DEVICE:
        return callback( impl, caps, instance, data );
    case DIPH_BYOFFSET:
        if (filter->dwObj != instance->dwOfs) return DIENUM_CONTINUE;
        return callback( impl, caps, instance, data );
    case DIPH_BYID:
        if ((filter->dwObj & 0x00ffffff) != (instance->dwType & 0x00ffffff)) return DIENUM_CONTINUE;
        return callback( impl, caps, instance, data );
    case DIPH_BYUSAGE:
        if (HIWORD( filter->dwObj ) != instance->wUsagePage) return DIENUM_CONTINUE;
        if (LOWORD( filter->dwObj ) != instance->wUsage) return DIENUM_CONTINUE;
        return callback( impl, caps, instance, data );
    default:
        FIXME( "unimplemented filter dwHow %#lx dwObj %#lx\n", filter->dwHow, filter->dwObj );
        break;
    }

    return DIENUM_CONTINUE;
}

static void check_pid_effect_axis_caps( struct hid_joystick *impl, DIDEVICEOBJECTINSTANCEW *instance )
{
    struct pid_effect_update *effect_update = &impl->pid_effect_update;
    ULONG i;

    for (i = 0; i < effect_update->axis_count; ++i)
    {
        if (effect_update->axis_caps[i]->usage_page != instance->wUsagePage) continue;
        if (effect_update->axis_caps[i]->usage_min > instance->wUsage) continue;
        if (effect_update->axis_caps[i]->usage_max >= instance->wUsage) break;
    }

    if (i == effect_update->axis_count) return;
    instance->dwType |= DIDFT_FFACTUATOR;
    instance->dwFlags |= DIDOI_FFACTUATOR;
}

static void set_axis_type( DIDEVICEOBJECTINSTANCEW *instance, BOOL *seen, DWORD i, DWORD *count )
{
    if (!seen[i]) instance->dwType = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( i );
    else instance->dwType = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 6 + (*count)++ );
    seen[i] = TRUE;
}

static BOOL enum_objects( struct hid_joystick *impl, const DIPROPHEADER *filter, DWORD flags,
                          enum_object_callback callback, void *data )
{
    DWORD collection = 0, object = 0, axis = 0, button = 0, pov = 0, value_ofs = 0, button_ofs = 0, j, count, len;
    struct hid_preparsed_data *preparsed = (struct hid_preparsed_data *)impl->preparsed;
    DIDEVICEOBJECTINSTANCEW instance = {.dwSize = sizeof(DIDEVICEOBJECTINSTANCEW)};
    struct hid_value_caps *caps, *caps_end, *nary, *nary_end, *effect_caps;
    struct hid_collection_node *node, *node_end;
    WORD version = impl->base.dinput->dwVersion;
    BOOL ret, seen_axis[6] = {0};
    const WCHAR *tmp;

    button_ofs += impl->caps.NumberInputValueCaps * sizeof(LONG);
    if (version >= 0x800)
    {
        button_ofs += impl->caps.NumberOutputValueCaps * sizeof(LONG);
        button_ofs += impl->caps.NumberFeatureValueCaps * sizeof(LONG);
    }

    for (caps = HID_INPUT_VALUE_CAPS( preparsed ), caps_end = caps + preparsed->input_caps_count;
         caps != caps_end; ++caps)
    {
        if (!caps->usage_page) continue;
        if (caps->flags & HID_VALUE_CAPS_IS_BUTTON) continue;

        if (caps->usage_page >= HID_USAGE_PAGE_VENDOR_DEFINED_BEGIN)
            value_ofs += (caps->usage_max - caps->usage_min + 1) * sizeof(LONG);
        else for (j = caps->usage_min; j <= caps->usage_max; ++j)
        {
            instance.dwOfs = value_ofs;
            switch (MAKELONG(j, caps->usage_page))
            {
            case MAKELONG(HID_USAGE_GENERIC_X, HID_USAGE_PAGE_GENERIC):
            case MAKELONG(HID_USAGE_GENERIC_Y, HID_USAGE_PAGE_GENERIC):
            case MAKELONG(HID_USAGE_GENERIC_Z, HID_USAGE_PAGE_GENERIC):
            case MAKELONG(HID_USAGE_GENERIC_RX, HID_USAGE_PAGE_GENERIC):
            case MAKELONG(HID_USAGE_GENERIC_RY, HID_USAGE_PAGE_GENERIC):
            case MAKELONG(HID_USAGE_GENERIC_RZ, HID_USAGE_PAGE_GENERIC):
                set_axis_type( &instance, seen_axis, j - HID_USAGE_GENERIC_X, &axis );
                instance.dwFlags = DIDOI_ASPECTPOSITION;
                break;
            case MAKELONG(HID_USAGE_SIMULATION_STEERING, HID_USAGE_PAGE_SIMULATION):
                set_axis_type( &instance, seen_axis, 0, &axis );
                instance.dwFlags = DIDOI_ASPECTPOSITION;
                break;
            case MAKELONG(HID_USAGE_SIMULATION_ACCELERATOR, HID_USAGE_PAGE_SIMULATION):
                set_axis_type( &instance, seen_axis, 1, &axis );
                instance.dwFlags = DIDOI_ASPECTPOSITION;
                break;
            case MAKELONG(HID_USAGE_GENERIC_WHEEL, HID_USAGE_PAGE_GENERIC):
            case MAKELONG(HID_USAGE_SIMULATION_THROTTLE, HID_USAGE_PAGE_SIMULATION):
                set_axis_type( &instance, seen_axis, 2, &axis );
                instance.dwFlags = DIDOI_ASPECTPOSITION;
                break;
            case MAKELONG(HID_USAGE_SIMULATION_RUDDER, HID_USAGE_PAGE_SIMULATION):
            case MAKELONG(HID_USAGE_SIMULATION_BRAKE, HID_USAGE_PAGE_SIMULATION):
                set_axis_type( &instance, seen_axis, 5, &axis );
                instance.dwFlags = DIDOI_ASPECTPOSITION;
                break;
            case MAKELONG(HID_USAGE_GENERIC_HATSWITCH, HID_USAGE_PAGE_GENERIC):
                instance.dwType = DIDFT_POV | DIDFT_MAKEINSTANCE( pov++ );
                instance.dwFlags = 0;
                break;
            case MAKELONG(HID_USAGE_GENERIC_SLIDER, HID_USAGE_PAGE_GENERIC):
            case MAKELONG(HID_USAGE_GENERIC_DIAL, HID_USAGE_PAGE_GENERIC):
                instance.dwType = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 6 + axis++ );
                instance.dwFlags = DIDOI_ASPECTPOSITION;
                break;
            default:
                instance.dwType = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 6 + axis++ );
                instance.dwFlags = 0;
                break;
            }
            instance.wUsagePage = caps->usage_page;
            instance.wUsage = j;
            instance.guidType = *object_usage_to_guid( instance.wUsagePage, instance.wUsage );
            instance.wReportId = caps->report_id;
            instance.wCollectionNumber = caps->link_collection;
            instance.dwDimension = caps->units;
            instance.wExponent = caps->units_exp;
            if ((tmp = object_usage_to_string( &instance ))) lstrcpynW( instance.tszName, tmp, MAX_PATH );
            else swprintf( instance.tszName, MAX_PATH, L"Unknown %u", DIDFT_GETINSTANCE( instance.dwType ) );
            check_pid_effect_axis_caps( impl, &instance );
            ret = enum_object( impl, filter, flags, callback, caps, &instance, data );
            if (ret != DIENUM_CONTINUE) return ret;
            value_ofs += sizeof(LONG);
            object++;
        }
    }

    effect_caps = impl->pid_effect_update.trigger_button_caps;

    for (caps = HID_INPUT_VALUE_CAPS( preparsed ), caps_end = caps + preparsed->input_caps_count;
         caps != caps_end; ++caps)
    {
        if (!caps->usage_page) continue;
        if (!(caps->flags & HID_VALUE_CAPS_IS_BUTTON)) continue;

        if (caps->usage_page >= HID_USAGE_PAGE_VENDOR_DEFINED_BEGIN)
            button_ofs += caps->usage_max - caps->usage_min + 1;
        else for (j = caps->usage_min; j <= caps->usage_max; ++j)
        {
            instance.dwOfs = button_ofs;
            instance.dwType = DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE( button++ );
            instance.dwFlags = 0;
            if (effect_caps && effect_caps->logical_min <= j && effect_caps->logical_max >= j)
            {
                instance.dwType |= DIDFT_FFEFFECTTRIGGER;
                instance.dwFlags |= DIDOI_FFEFFECTTRIGGER;
            }
            instance.wUsagePage = caps->usage_page;
            instance.wUsage = j;
            instance.guidType = *object_usage_to_guid( instance.wUsagePage, instance.wUsage );
            instance.wReportId = caps->report_id;
            instance.wCollectionNumber = caps->link_collection;
            instance.dwDimension = caps->units;
            instance.wExponent = caps->units_exp;
            swprintf( instance.tszName, MAX_PATH, L"Button %u", DIDFT_GETINSTANCE( instance.dwType ) );
            ret = enum_object( impl, filter, flags, callback, caps, &instance, data );
            if (ret != DIENUM_CONTINUE) return ret;
            button_ofs++;
            object++;
        }
    }

    count = preparsed->output_caps_count + preparsed->feature_caps_count;
    for (caps = HID_OUTPUT_VALUE_CAPS( preparsed ), caps_end = caps + count;
         caps != caps_end; ++caps)
    {
        if (!caps->usage_page) continue;

        if (caps->usage_page >= HID_USAGE_PAGE_VENDOR_DEFINED_BEGIN)
        {
            if (caps->flags & HID_VALUE_CAPS_IS_BUTTON) button_ofs += caps->usage_max - caps->usage_min + 1;
            else value_ofs += (caps->usage_max - caps->usage_min + 1) * sizeof(LONG);
        }
        else if (caps->flags & HID_VALUE_CAPS_ARRAY_HAS_MORE)
        {
            for (nary_end = caps - 1; caps != caps_end; caps++)
                if (!(caps->flags & HID_VALUE_CAPS_ARRAY_HAS_MORE)) break;

            for (nary = caps; nary != nary_end; nary--)
            {
                if (version < 0x800) instance.dwOfs = 0;
                else instance.dwOfs = button_ofs;

                instance.dwType = DIDFT_NODATA | DIDFT_MAKEINSTANCE( object++ ) | DIDFT_OUTPUT;
                instance.dwFlags = 0x80008000;
                instance.wUsagePage = nary->usage_page;
                instance.wUsage = nary->usage_min;
                instance.guidType = GUID_Unknown;
                instance.wReportId = nary->report_id;
                instance.wCollectionNumber = nary->link_collection;
                instance.dwDimension = caps->units;
                instance.wExponent = caps->units_exp;
                if ((tmp = object_usage_to_string( &instance ))) lstrcpynW( instance.tszName, tmp, MAX_PATH );
                else swprintf( instance.tszName, MAX_PATH, L"Unknown %u", DIDFT_GETINSTANCE( instance.dwType ) );
                ret = enum_object( impl, filter, flags, callback, nary, &instance, data );
                if (ret != DIENUM_CONTINUE) return ret;
                button_ofs++;
            }
        }
        else for (j = caps->usage_min; j <= caps->usage_max; ++j)
        {
            if (version < 0x800) instance.dwOfs = 0;
            else if (caps->flags & HID_VALUE_CAPS_IS_BUTTON) instance.dwOfs = button_ofs;
            else instance.dwOfs = value_ofs;

            instance.dwType = DIDFT_NODATA | DIDFT_MAKEINSTANCE( object++ ) | DIDFT_OUTPUT;
            instance.dwFlags = 0x80008000;
            instance.wUsagePage = caps->usage_page;
            instance.wUsage = j;
            instance.guidType = GUID_Unknown;
            instance.wReportId = caps->report_id;
            instance.wCollectionNumber = caps->link_collection;
            instance.dwDimension = caps->units;
            instance.wExponent = caps->units_exp;
            if ((tmp = object_usage_to_string( &instance ))) lstrcpynW( instance.tszName, tmp, MAX_PATH );
            else swprintf( instance.tszName, MAX_PATH, L"Unknown %u", DIDFT_GETINSTANCE( instance.dwType ) );
            ret = enum_object( impl, filter, flags, callback, caps, &instance, data );
            if (ret != DIENUM_CONTINUE) return ret;

            if (caps->flags & HID_VALUE_CAPS_IS_BUTTON) button_ofs++;
            else value_ofs += sizeof(LONG);
        }
    }

    for (node = HID_COLLECTION_NODES( preparsed ), node_end = node + preparsed->number_link_collection_nodes;
         node != node_end; ++node)
    {
        if (!node->usage_page) continue;
        if (node->usage_page < HID_USAGE_PAGE_VENDOR_DEFINED_BEGIN)
        {
            instance.dwOfs = 0;
            instance.dwType = DIDFT_COLLECTION | DIDFT_MAKEINSTANCE( collection++ ) | DIDFT_NODATA;
            instance.dwFlags = 0;
            instance.wUsagePage = node->usage_page;
            instance.wUsage = node->usage;
            instance.guidType = *object_usage_to_guid( instance.wUsagePage, instance.wUsage );
            instance.wReportId = 0;
            instance.wCollectionNumber = node->parent;
            instance.dwDimension = 0;
            instance.wExponent = 0;
            len = swprintf( instance.tszName, MAX_PATH, L"Collection %u - ", DIDFT_GETINSTANCE( instance.dwType ) );
            if ((tmp = object_usage_to_string( &instance ))) lstrcpynW( instance.tszName + len, tmp, MAX_PATH - len );
            else swprintf( instance.tszName + len, MAX_PATH - len, L"Unknown %u", DIDFT_GETINSTANCE( instance.dwType ) );
            ret = enum_object( impl, filter, flags, callback, NULL, &instance, data );
            if (ret != DIENUM_CONTINUE) return ret;
        }
    }

    return DIENUM_CONTINUE;
}

static void set_report_value( struct hid_joystick *impl, char *report_buf,
                              struct hid_value_caps *caps, LONG value )
{
    ULONG report_len = impl->caps.OutputReportByteLength;
    PHIDP_PREPARSED_DATA preparsed = impl->preparsed;
    LONG log_min, log_max, phy_min, phy_max;
    NTSTATUS status;

    if (!caps) return;

    log_min = caps->logical_min;
    log_max = caps->logical_max;
    phy_min = caps->physical_min;
    phy_max = caps->physical_max;

    if (phy_max || phy_min)
    {
        if (value > phy_max || value < phy_min) value = -1;
        else value = log_min + (value - phy_min) * (log_max - log_min) / (phy_max - phy_min);
    }

    status = HidP_SetUsageValue( HidP_Output, caps->usage_page, caps->link_collection,
                                 caps->usage_min, value, preparsed, report_buf, report_len );
    if (status != HIDP_STATUS_SUCCESS) WARN( "HidP_SetUsageValue %04x:%04x returned %#lx\n",
                                             caps->usage_page, caps->usage_min, status );
}

static void hid_joystick_addref( IDirectInputDevice8W *iface )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );
    ULONG ref = InterlockedIncrement( &impl->internal_ref );
    TRACE( "iface %p, internal ref %lu.\n", iface, ref );
}

static void hid_joystick_release( IDirectInputDevice8W *iface )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );
    ULONG ref = InterlockedDecrement( &impl->internal_ref );
    TRACE( "iface %p, internal ref %lu.\n", iface, ref );

    if (!ref)
    {
        free( impl->usages_buf );
        free( impl->feature_report_buf );
        free( impl->output_report_buf );
        free( impl->input_report_buf );
        HidD_FreePreparsedData( impl->preparsed );
        CloseHandle( impl->base.read_event );
        CloseHandle( impl->device );
        dinput_device_destroy( iface );
    }
}

static HRESULT hid_joystick_get_property( IDirectInputDevice8W *iface, DWORD property,
                                          DIPROPHEADER *header, const DIDEVICEOBJECTINSTANCEW *instance )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );

    switch (property)
    {
    case (DWORD_PTR)DIPROP_PRODUCTNAME:
    {
        DIPROPSTRING *value = (DIPROPSTRING *)header;
        lstrcpynW( value->wsz, impl->base.instance.tszProductName, MAX_PATH );
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_INSTANCENAME:
    {
        DIPROPSTRING *value = (DIPROPSTRING *)header;
        lstrcpynW( value->wsz, impl->base.instance.tszInstanceName, MAX_PATH );
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_VIDPID:
    {
        DIPROPDWORD *value = (DIPROPDWORD *)header;
        if (!impl->attrs.VendorID || !impl->attrs.ProductID) return DIERR_UNSUPPORTED;
        value->dwData = MAKELONG( impl->attrs.VendorID, impl->attrs.ProductID );
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_JOYSTICKID:
    {
        DIPROPDWORD *value = (DIPROPDWORD *)header;
        value->dwData = impl->base.instance.guidInstance.Data3;
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_GUIDANDPATH:
    {
        DIPROPGUIDANDPATH *value = (DIPROPGUIDANDPATH *)header;
        value->guidClass = GUID_DEVCLASS_HIDCLASS;
        lstrcpynW( value->wszPath, impl->device_path, MAX_PATH );
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_FFLOAD:
    {
        DIPROPDWORD *value = (DIPROPDWORD *)header;
        if (!(impl->base.caps.dwFlags & DIDC_FORCEFEEDBACK)) return DIERR_UNSUPPORTED;
        if (!is_exclusively_acquired( impl )) return DIERR_NOTEXCLUSIVEACQUIRED;
        value->dwData = 0;
        return DI_OK;
    }
    }

    return DIERR_UNSUPPORTED;
}

static HRESULT hid_joystick_send_device_gain( IDirectInputDevice8W *iface, LONG device_gain )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );
    struct pid_device_gain *report = &impl->pid_device_gain;
    ULONG report_len = impl->caps.OutputReportByteLength;
    char *report_buf = impl->output_report_buf;
    NTSTATUS status;

    TRACE( "iface %p.\n", iface );

    if (!report->id || !report->device_gain_caps) return DI_OK;

    status = HidP_InitializeReportForID( HidP_Output, report->id, impl->preparsed, report_buf, report_len );
    if (status != HIDP_STATUS_SUCCESS) return status;

    set_report_value( impl, report_buf, report->device_gain_caps, device_gain );

    if (!WriteFile( impl->device, report_buf, report_len, NULL, NULL )) return DIERR_INPUTLOST;
    return DI_OK;
}

static HRESULT hid_joystick_acquire( IDirectInputDevice8W *iface )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );
    ULONG report_len = impl->caps.InputReportByteLength;
    BOOL ret;

    if (impl->device == INVALID_HANDLE_VALUE)
    {
        impl->device = CreateFileW( impl->device_path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, 0 );
        if (impl->device == INVALID_HANDLE_VALUE) return DIERR_UNPLUGGED;
    }

    memset( &impl->read_ovl, 0, sizeof(impl->read_ovl) );
    impl->read_ovl.hEvent = impl->base.read_event;
    ret = ReadFile( impl->device, impl->input_report_buf, report_len, NULL, &impl->read_ovl );
    if (!ret && GetLastError() != ERROR_IO_PENDING)
    {
        CloseHandle( impl->device );
        impl->device = INVALID_HANDLE_VALUE;
        return DIERR_UNPLUGGED;
    }

    IDirectInputDevice8_SendForceFeedbackCommand( iface, DISFFC_RESET );
    return DI_OK;
}

static HRESULT hid_joystick_send_force_feedback_command( IDirectInputDevice8W *iface, DWORD command, BOOL unacquire );

static HRESULT hid_joystick_unacquire( IDirectInputDevice8W *iface )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );
    BOOL ret;

    if (impl->device == INVALID_HANDLE_VALUE) return DI_NOEFFECT;

    ret = CancelIoEx( impl->device, &impl->read_ovl );
    if (!ret) WARN( "CancelIoEx failed, last error %lu\n", GetLastError() );
    else WaitForSingleObject( impl->base.read_event, INFINITE );

    if (!(impl->base.caps.dwFlags & DIDC_FORCEFEEDBACK)) return DI_OK;
    if (!is_exclusively_acquired( impl )) return DI_OK;
    hid_joystick_send_force_feedback_command( iface, DISFFC_RESET, TRUE );
    return DI_OK;
}

static HRESULT hid_joystick_create_effect( IDirectInputDevice8W *iface, IDirectInputEffect **out );

static HRESULT hid_joystick_get_effect_info( IDirectInputDevice8W *iface, DIEFFECTINFOW *info, const GUID *guid )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );
    struct pid_effect_update *effect_update = &impl->pid_effect_update;
    struct pid_set_condition *set_condition = &impl->pid_set_condition;
    struct pid_set_periodic *set_periodic = &impl->pid_set_periodic;
    struct pid_set_envelope *set_envelope = &impl->pid_set_envelope;
    PHIDP_PREPARSED_DATA preparsed = impl->preparsed;
    HIDP_BUTTON_CAPS button;
    ULONG type, collection;
    NTSTATUS status;
    USAGE usage = 0;
    USHORT count;

    switch ((usage = effect_guid_to_usage( guid )))
    {
    case PID_USAGE_ET_SQUARE:
    case PID_USAGE_ET_SINE:
    case PID_USAGE_ET_TRIANGLE:
    case PID_USAGE_ET_SAWTOOTH_UP:
    case PID_USAGE_ET_SAWTOOTH_DOWN:
        type = DIEFT_PERIODIC;
        break;
    case PID_USAGE_ET_SPRING:
    case PID_USAGE_ET_DAMPER:
    case PID_USAGE_ET_INERTIA:
    case PID_USAGE_ET_FRICTION:
        type = DIEFT_CONDITION;
        break;
    case PID_USAGE_ET_CONSTANT_FORCE:
        type = DIEFT_CONSTANTFORCE;
        break;
    case PID_USAGE_ET_RAMP:
        type = DIEFT_RAMPFORCE;
        break;
    case PID_USAGE_ET_CUSTOM_FORCE_DATA:
        type = DIEFT_CUSTOMFORCE;
        break;
    default:
        return DIERR_DEVICENOTREG;
    }

    if (!(collection = effect_update->collection)) return DIERR_DEVICENOTREG;

    info->dwDynamicParams = DIEP_TYPESPECIFICPARAMS;
    if (effect_update->axis_count) info->dwDynamicParams |= DIEP_AXES;
    if (effect_update->duration_caps) info->dwDynamicParams |= DIEP_DURATION;
    if (effect_update->gain_caps) info->dwDynamicParams |= DIEP_GAIN;
    if (effect_update->sample_period_caps) info->dwDynamicParams |= DIEP_SAMPLEPERIOD;
    if (effect_update->start_delay_caps)
    {
        type |= DIEFT_STARTDELAY;
        info->dwDynamicParams |= DIEP_STARTDELAY;
    }
    if (effect_update->direction_coll) info->dwDynamicParams |= DIEP_DIRECTION;
    if (effect_update->axes_coll) info->dwDynamicParams |= DIEP_AXES;

    if (!(collection = effect_update->type_coll)) return DIERR_DEVICENOTREG;
    else
    {
        count = 1;
        status = HidP_GetSpecificButtonCaps( HidP_Output, HID_USAGE_PAGE_PID, collection,
                                             usage, &button, &count, preparsed );
        if (status != HIDP_STATUS_SUCCESS)
        {
            WARN( "HidP_GetSpecificButtonCaps %#x returned %#lx\n", usage, status );
            return DIERR_DEVICENOTREG;
        }
        else if (!count)
        {
            WARN( "effect usage %#x not found\n", usage );
            return DIERR_DEVICENOTREG;
        }
    }

    if ((DIEFT_GETTYPE(type) == DIEFT_PERIODIC) && (collection = set_periodic->collection))
    {
        if (set_periodic->magnitude_caps) info->dwDynamicParams |= DIEP_TYPESPECIFICPARAMS;
        if (set_periodic->offset_caps) info->dwDynamicParams |= DIEP_TYPESPECIFICPARAMS;
        if (set_periodic->period_caps) info->dwDynamicParams |= DIEP_TYPESPECIFICPARAMS;
        if (set_periodic->phase_caps) info->dwDynamicParams |= DIEP_TYPESPECIFICPARAMS;
    }

    if ((DIEFT_GETTYPE(type) == DIEFT_PERIODIC ||
         DIEFT_GETTYPE(type) == DIEFT_RAMPFORCE ||
         DIEFT_GETTYPE(type) == DIEFT_CONSTANTFORCE) &&
        (collection = set_envelope->collection))
    {
        info->dwDynamicParams |= DIEP_ENVELOPE;
        if (set_envelope->attack_level_caps) type |= DIEFT_FFATTACK;
        if (set_envelope->attack_time_caps) type |= DIEFT_FFATTACK;
        if (set_envelope->fade_level_caps) type |= DIEFT_FFFADE;
        if (set_envelope->fade_time_caps) type |= DIEFT_FFFADE;
        if (effect_update->trigger_button_caps) info->dwDynamicParams |= DIEP_TRIGGERBUTTON;
        if (effect_update->trigger_repeat_interval_caps) info->dwDynamicParams |= DIEP_TRIGGERREPEATINTERVAL;
    }

    if (DIEFT_GETTYPE(type) == DIEFT_CONDITION && (collection = set_condition->collection))
    {
        if (set_condition->center_point_offset_caps)
            info->dwDynamicParams |= DIEP_TYPESPECIFICPARAMS;
        if (set_condition->positive_coefficient_caps || set_condition->negative_coefficient_caps)
            info->dwDynamicParams |= DIEP_TYPESPECIFICPARAMS;
        if (set_condition->positive_saturation_caps || set_condition->negative_saturation_caps)
        {
            info->dwDynamicParams |= DIEP_TYPESPECIFICPARAMS;
            type |= DIEFT_SATURATION;
        }
        if (set_condition->dead_band_caps)
        {
            info->dwDynamicParams |= DIEP_TYPESPECIFICPARAMS;
            type |= DIEFT_DEADBAND;
        }
    }

    info->guid = *guid;
    info->dwEffType = type;
    info->dwStaticParams = info->dwDynamicParams;
    lstrcpynW( info->tszName, effect_guid_to_string( guid ), MAX_PATH );

    return DI_OK;
}

static BOOL CALLBACK unload_effect_object( IDirectInputEffect *effect, void *context )
{
    IDirectInputEffect_Unload( effect );
    return DIENUM_CONTINUE;
}

static HRESULT hid_joystick_send_force_feedback_command( IDirectInputDevice8W *iface, DWORD command, BOOL unacquire )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );
    struct pid_control_report *report = &impl->pid_device_control;
    ULONG report_len = impl->caps.OutputReportByteLength;
    char *report_buf = impl->output_report_buf;
    NTSTATUS status;
    USAGE usage;
    ULONG count;

    TRACE( "iface %p, command %#lx.\n", iface, command );

    switch (command)
    {
    case DISFFC_RESET: usage = PID_USAGE_DC_DEVICE_RESET; break;
    case DISFFC_STOPALL: usage = PID_USAGE_DC_STOP_ALL_EFFECTS; break;
    case DISFFC_PAUSE: usage = PID_USAGE_DC_DEVICE_PAUSE; break;
    case DISFFC_CONTINUE: usage = PID_USAGE_DC_DEVICE_CONTINUE; break;
    case DISFFC_SETACTUATORSON: usage = PID_USAGE_DC_ENABLE_ACTUATORS; break;
    case DISFFC_SETACTUATORSOFF: usage = PID_USAGE_DC_DISABLE_ACTUATORS; break;
    }

    if (command == DISFFC_RESET)
    {
        IDirectInputDevice8_EnumCreatedEffectObjects( iface, unload_effect_object, NULL, 0 );
        impl->base.force_feedback_state = DIGFFS_STOPPED | DIGFFS_EMPTY;
    }

    count = 1;
    status = HidP_InitializeReportForID( HidP_Output, report->id, impl->preparsed, report_buf, report_len );
    if (status != HIDP_STATUS_SUCCESS) return status;

    status = HidP_SetUsages( HidP_Output, HID_USAGE_PAGE_PID, report->control_coll, &usage,
                             &count, impl->preparsed, report_buf, report_len );
    if (status != HIDP_STATUS_SUCCESS) return status;

    if (!WriteFile( impl->device, report_buf, report_len, NULL, NULL )) return DIERR_INPUTLOST;
    if (!unacquire && command == DISFFC_RESET) hid_joystick_send_device_gain( iface, impl->base.device_gain );

    return DI_OK;
}

static HRESULT hid_joystick_enum_created_effect_objects( IDirectInputDevice8W *iface,
                                                         LPDIENUMCREATEDEFFECTOBJECTSCALLBACK callback,
                                                         void *context, DWORD flags )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );
    struct hid_joystick_effect *effect, *next;

    TRACE( "iface %p, callback %p, context %p, flags %#lx.\n", iface, callback, context, flags );

    LIST_FOR_EACH_ENTRY_SAFE(effect, next, &impl->effect_list, struct hid_joystick_effect, entry)
        if (callback( &effect->IDirectInputEffect_iface, context ) != DIENUM_CONTINUE) break;

    return DI_OK;
}

struct parse_device_state_params
{
    BYTE old_state[DEVICE_STATE_MAX_SIZE];
    BYTE buttons[128];
    DWORD time;
    DWORD seq;
};

static BOOL check_device_state_button( struct hid_joystick *impl, struct hid_value_caps *caps,
                                       DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    IDirectInputDevice8W *iface = &impl->base.IDirectInputDevice8W_iface;
    struct parse_device_state_params *params = data;
    BYTE old_value, value;

    if (instance->wReportId != impl->base.device_state_report_id) return DIENUM_CONTINUE;

    value = params->buttons[instance->wUsage - 1];
    old_value = params->old_state[instance->dwOfs];
    impl->base.device_state[instance->dwOfs] = value;
    if (old_value != value)
        queue_event( iface, instance->dwType, value, params->time, params->seq );

    return DIENUM_CONTINUE;
}

static LONG sign_extend( ULONG value, struct object_properties *properties )
{
    UINT sign = 1 << (properties->bit_size - 1);
    if (sign <= 1 || properties->logical_min >= 0) return value;
    return value - ((value & sign) << 1);
}

static LONG scale_value( ULONG value, struct object_properties *properties )
{
    LONG tmp = sign_extend( value, properties ), log_min, log_max, phy_min, phy_max;
    log_min = properties->logical_min;
    log_max = properties->logical_max;
    phy_min = properties->range_min;
    phy_max = properties->range_max;

    if (log_min > tmp || log_max < tmp) return -1; /* invalid / null value */
    return phy_min + MulDiv( tmp - log_min, phy_max - phy_min, log_max - log_min );
}

static LONG scale_axis_value( ULONG value, struct object_properties *properties )
{
    LONG tmp = sign_extend( value, properties ), log_ctr, log_min, log_max, phy_ctr, phy_min, phy_max;
    ULONG bit_max = (1 << properties->bit_size) - 1;

    log_min = properties->logical_min;
    log_max = properties->logical_max;
    phy_min = properties->range_min;
    phy_max = properties->range_max;
    /* xinput HID gamepad have bogus logical value range, let's use the bit range instead */
    if (log_min == 0 && log_max == -1) log_max = bit_max;

    if (phy_min == 0) phy_ctr = phy_max >> 1;
    else phy_ctr = round( (phy_min + phy_max) / 2.0 );
    if (log_min == 0) log_ctr = log_max >> 1;
    else log_ctr = round( (log_min + log_max) / 2.0 );

    tmp -= log_ctr;
    if (tmp <= 0)
    {
        log_max = MulDiv( log_min - log_ctr, properties->deadzone, 10000 );
        log_min = MulDiv( log_min - log_ctr, properties->saturation, 10000 );
        phy_max = phy_ctr;
    }
    else
    {
        log_min = MulDiv( log_max - log_ctr, properties->deadzone, 10000 );
        log_max = MulDiv( log_max - log_ctr, properties->saturation, 10000 );
        phy_min = phy_ctr;
    }

    if (tmp <= log_min) return phy_min;
    if (tmp >= log_max) return phy_max;
    return phy_min + MulDiv( tmp - log_min, phy_max - phy_min, log_max - log_min );
}

static BOOL read_device_state_value( struct hid_joystick *impl, struct hid_value_caps *caps,
                                     DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    struct object_properties *properties = impl->base.object_properties + instance->dwOfs / sizeof(LONG);
    IDirectInputDevice8W *iface = &impl->base.IDirectInputDevice8W_iface;
    ULONG logical_value, report_len = impl->caps.InputReportByteLength;
    struct parse_device_state_params *params = data;
    char *report_buf = impl->input_report_buf;
    LONG old_value, value;
    NTSTATUS status;

    if (instance->wReportId != impl->base.device_state_report_id) return DIENUM_CONTINUE;

    status = HidP_GetUsageValue( HidP_Input, instance->wUsagePage, 0, instance->wUsage,
                                 &logical_value, impl->preparsed, report_buf, report_len );
    if (status != HIDP_STATUS_SUCCESS) WARN( "HidP_GetUsageValue %04x:%04x returned %#lx\n",
                                             instance->wUsagePage, instance->wUsage, status );
    if (instance->dwType & DIDFT_AXIS) value = scale_axis_value( logical_value, properties );
    else value = scale_value( logical_value, properties );

    old_value = *(LONG *)(params->old_state + instance->dwOfs);
    *(LONG *)(impl->base.device_state + instance->dwOfs) = value;
    if (old_value != value)
        queue_event( iface, instance->dwType, value, params->time, params->seq );

    return DIENUM_CONTINUE;
}

static HRESULT hid_joystick_read( IDirectInputDevice8W *iface )
{
    static const DIPROPHEADER filter =
    {
        .dwSize = sizeof(filter),
        .dwHeaderSize = sizeof(filter),
        .dwHow = DIPH_DEVICE,
    };
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );
    ULONG i, index, count, report_len = impl->caps.InputReportByteLength;
    DIDATAFORMAT *format = impl->base.device_format;
    char *report_buf = impl->input_report_buf;
    struct parse_device_state_params params;
    struct hid_joystick_effect *effect;
    UINT device_state, effect_state;
    USAGE_AND_PAGE *usages;
    NTSTATUS status;
    HRESULT hr;
    BOOL ret;

    ret = GetOverlappedResult( impl->device, &impl->read_ovl, &count, FALSE );

    EnterCriticalSection( &impl->base.crit );
    while (ret)
    {
        if (TRACE_ON(dinput))
        {
            TRACE( "iface %p, size %lu, report:\n", iface, count );
            for (i = 0; i < count;)
            {
                char buffer[256], *buf = buffer;
                buf += sprintf(buf, "%08lx ", i);
                do { buf += sprintf(buf, " %02x", (BYTE)report_buf[i] ); }
                while (++i % 16 && i < count);
                TRACE("%s\n", buffer);
            }
        }

        count = impl->usages_count;
        memset( impl->usages_buf, 0, count * sizeof(*impl->usages_buf) );
        status = HidP_GetUsagesEx( HidP_Input, 0, impl->usages_buf, &count,
                                   impl->preparsed, report_buf, report_len );
        if (status != HIDP_STATUS_SUCCESS) WARN( "HidP_GetUsagesEx returned %#lx\n", status );

        if (report_buf[0] == impl->base.device_state_report_id)
        {
            params.time = GetCurrentTime();
            params.seq = impl->base.dinput->evsequence++;
            memcpy( params.old_state, impl->base.device_state, format->dwDataSize );
            memset( params.buttons, 0, sizeof(params.buttons) );
            memset( impl->base.device_state, 0, format->dwDataSize );

            while (count--)
            {
                usages = impl->usages_buf + count;
                if (usages->UsagePage != HID_USAGE_PAGE_BUTTON)
                    FIXME( "unimplemented usage page %x.\n", usages->UsagePage );
                else if (usages->Usage >= 128)
                    FIXME( "ignoring extraneous button %d.\n", usages->Usage );
                else
                    params.buttons[usages->Usage - 1] = 0x80;
            }

            enum_objects( impl, &filter, DIDFT_AXIS | DIDFT_POV, read_device_state_value, &params );
            enum_objects( impl, &filter, DIDFT_BUTTON, check_device_state_button, &params );
            if (impl->base.hEvent && memcmp( &params.old_state, impl->base.device_state, format->dwDataSize ))
                SetEvent( impl->base.hEvent );
        }
        else if (report_buf[0] == impl->pid_effect_state.id && is_exclusively_acquired( impl ))
        {
            status = HidP_GetUsageValue( HidP_Input, HID_USAGE_PAGE_PID, 0, PID_USAGE_EFFECT_BLOCK_INDEX,
                                         &index, impl->preparsed, report_buf, report_len );
            if (status != HIDP_STATUS_SUCCESS) WARN( "HidP_GetUsageValue EFFECT_BLOCK_INDEX returned %#lx\n", status );

            effect_state = 0;
            device_state = impl->base.force_feedback_state & DIGFFS_EMPTY;
            while (count--)
            {
                USAGE_AND_PAGE *button = impl->usages_buf + count;
                if (button->UsagePage != HID_USAGE_PAGE_PID)
                    FIXME( "unimplemented usage page %#04x.\n", button->UsagePage );
                else switch (button->Usage)
                {
                case PID_USAGE_DEVICE_PAUSED: device_state |= DIGFFS_PAUSED; break;
                case PID_USAGE_ACTUATORS_ENABLED: device_state |= DIGFFS_ACTUATORSON; break;
                case PID_USAGE_SAFETY_SWITCH: device_state |= DIGFFS_SAFETYSWITCHON; break;
                case PID_USAGE_ACTUATOR_OVERRIDE_SWITCH: device_state |= DIGFFS_USERFFSWITCHON; break;
                case PID_USAGE_ACTUATOR_POWER: device_state |= DIGFFS_POWERON; break;
                case PID_USAGE_EFFECT_PLAYING: effect_state = DIEGES_PLAYING; break;
                default: FIXME( "unimplemented usage %#04x\n", button->Usage ); break;
                }
            }
            if (!(device_state & DIGFFS_ACTUATORSON)) device_state |= DIGFFS_ACTUATORSOFF;
            if (!(device_state & DIGFFS_SAFETYSWITCHON) && impl->pid_effect_state.safety_switch_caps)
                device_state |= DIGFFS_SAFETYSWITCHOFF;
            if (!(device_state & DIGFFS_USERFFSWITCHON) && impl->pid_effect_state.actuator_override_switch_caps)
                device_state |= DIGFFS_USERFFSWITCHOFF;
            if (!(device_state & DIGFFS_POWERON) && impl->pid_effect_state.actuator_power_caps)
                device_state |= DIGFFS_POWEROFF;

            TRACE( "effect %lu state %#x, device state %#x\n", index, effect_state, device_state );

            LIST_FOR_EACH_ENTRY( effect, &impl->effect_list, struct hid_joystick_effect, entry )
                if (effect->index == index) effect->status = effect_state;
            impl->base.force_feedback_state = device_state;
        }

        memset( &impl->read_ovl, 0, sizeof(impl->read_ovl) );
        impl->read_ovl.hEvent = impl->base.read_event;
        ret = ReadFile( impl->device, report_buf, report_len, &count, &impl->read_ovl );
    }

    if (GetLastError() == ERROR_IO_PENDING || GetLastError() == ERROR_OPERATION_ABORTED) hr = DI_OK;
    else
    {
        WARN( "GetOverlappedResult/ReadFile failed, error %lu\n", GetLastError() );
        CloseHandle(impl->device);
        impl->device = INVALID_HANDLE_VALUE;
        hr = DIERR_INPUTLOST;
    }
    LeaveCriticalSection( &impl->base.crit );

    return hr;
}

struct enum_objects_params
{
    LPDIENUMDEVICEOBJECTSCALLBACKW callback;
    void *context;
};

static BOOL enum_objects_callback( struct hid_joystick *impl, struct hid_value_caps *caps,
                                   DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    struct enum_objects_params *params = data;
    if (instance->wUsagePage == HID_USAGE_PAGE_PID && !(instance->dwType & DIDFT_NODATA))
        return DIENUM_CONTINUE;
    return params->callback( instance, params->context );
}

static HRESULT hid_joystick_enum_objects( IDirectInputDevice8W *iface, const DIPROPHEADER *filter,
                                          DWORD flags, LPDIENUMDEVICEOBJECTSCALLBACKW callback, void *context )
{
    struct enum_objects_params params = {.callback = callback, .context = context};
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );
    return enum_objects( impl, filter, flags, enum_objects_callback, &params );
}

static const struct dinput_device_vtbl hid_joystick_vtbl =
{
    hid_joystick_release,
    NULL,
    hid_joystick_read,
    hid_joystick_acquire,
    hid_joystick_unacquire,
    hid_joystick_enum_objects,
    hid_joystick_get_property,
    hid_joystick_get_effect_info,
    hid_joystick_create_effect,
    hid_joystick_send_force_feedback_command,
    hid_joystick_send_device_gain,
    hid_joystick_enum_created_effect_objects,
};

static DWORD device_type_for_version( DWORD type, DWORD version )
{
    if (version >= 0x0800) return type;

    switch (GET_DIDEVICE_TYPE( type ))
    {
    case DI8DEVTYPE_JOYSTICK:
        if (GET_DIDEVICE_SUBTYPE( type ) == DI8DEVTYPEJOYSTICK_LIMITED)
            return DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_UNKNOWN << 8) | DIDEVTYPE_HID;
        return DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_TRADITIONAL << 8) | DIDEVTYPE_HID;

    case DI8DEVTYPE_GAMEPAD:
        return DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_GAMEPAD << 8) | DIDEVTYPE_HID;

    case DI8DEVTYPE_DRIVING:
        return DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_WHEEL << 8) | DIDEVTYPE_HID;

    case DI8DEVTYPE_FLIGHT:
        return DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_FLIGHTSTICK << 8) | DIDEVTYPE_HID;

    case DI8DEVTYPE_SUPPLEMENTAL:
        if (GET_DIDEVICE_SUBTYPE( type ) == DI8DEVTYPESUPPLEMENTAL_HEADTRACKER)
            return DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_HEADTRACKER << 8) | DIDEVTYPE_HID;
        if (GET_DIDEVICE_SUBTYPE( type ) == DI8DEVTYPESUPPLEMENTAL_RUDDERPEDALS)
            return DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_RUDDER << 8) | DIDEVTYPE_HID;
        return DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_UNKNOWN << 8) | DIDEVTYPE_HID;

    case DI8DEVTYPE_1STPERSON:
        return DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_UNKNOWN << 8) | DIDEVTYPE_HID;

    default:
        return DIDEVTYPE_DEVICE | DIDEVTYPE_HID;
    }
}

static BOOL hid_joystick_device_try_open( UINT32 handle, const WCHAR *path, HANDLE *device,
                                          PHIDP_PREPARSED_DATA *preparsed, HIDD_ATTRIBUTES *attrs,
                                          HIDP_CAPS *caps, DIDEVICEINSTANCEW *instance, DWORD version )
{
    BOOL has_accelerator, has_brake, has_clutch, has_z, has_pov;
    PHIDP_PREPARSED_DATA preparsed_data = NULL;
    HIDP_LINK_COLLECTION_NODE nodes[256];
    DWORD type, button_count = 0;
    HIDP_BUTTON_CAPS buttons[10];
    HIDP_VALUE_CAPS value;
    HANDLE device_file;
    ULONG node_count;
    NTSTATUS status;
    USHORT count;

    device_file = CreateFileW( path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, 0 );
    if (device_file == INVALID_HANDLE_VALUE) return FALSE;

    if (!HidD_GetPreparsedData( device_file, &preparsed_data )) goto failed;
    if (!HidD_GetAttributes( device_file, attrs )) goto failed;
    if (HidP_GetCaps( preparsed_data, caps ) != HIDP_STATUS_SUCCESS) goto failed;

    switch (MAKELONG( caps->Usage, caps->UsagePage ))
    {
    case MAKELONG( HID_USAGE_GENERIC_MOUSE, HID_USAGE_PAGE_GENERIC ):  goto failed;
    case MAKELONG( HID_USAGE_GENERIC_KEYBOARD, HID_USAGE_PAGE_GENERIC ):  goto failed;
    case MAKELONG( HID_USAGE_GENERIC_GAMEPAD, HID_USAGE_PAGE_GENERIC ): type = DI8DEVTYPE_GAMEPAD; break;
    case MAKELONG( HID_USAGE_GENERIC_JOYSTICK, HID_USAGE_PAGE_GENERIC ): type = DI8DEVTYPE_JOYSTICK; break;
    default: FIXME( "device usage %04x:%04x not implemented!\n", caps->UsagePage, caps->Usage); goto failed;
    }

    if (!HidD_GetProductString( device_file, instance->tszInstanceName, MAX_PATH * sizeof(WCHAR) )) goto failed;
    if (!HidD_GetProductString( device_file, instance->tszProductName, MAX_PATH * sizeof(WCHAR) )) goto failed;

    instance->guidInstance = hid_joystick_guid;
    instance->guidInstance.Data1 ^= handle;
    instance->guidProduct = dinput_pidvid_guid;
    instance->guidProduct.Data1 = MAKELONG( attrs->VendorID, attrs->ProductID );
    instance->guidFFDriver = GUID_NULL;
    instance->wUsagePage = caps->UsagePage;
    instance->wUsage = caps->Usage;

    node_count = ARRAY_SIZE(nodes);
    status = HidP_GetLinkCollectionNodes( nodes, &node_count, preparsed_data );
    if (status != HIDP_STATUS_SUCCESS) node_count = 0;
    while (node_count--)
    {
        if (nodes[node_count].LinkUsagePage != HID_USAGE_PAGE_SIMULATION) continue;
        if (nodes[node_count].LinkUsage == HID_USAGE_SIMULATION_AUTOMOBILE_SIMULATION_DEVICE) type = DI8DEVTYPE_DRIVING;
        if (nodes[node_count].LinkUsage == HID_USAGE_SIMULATION_FLIGHT_SIMULATION_DEVICE) type = DI8DEVTYPE_FLIGHT;
    }

    count = ARRAY_SIZE(buttons);
    status = HidP_GetSpecificButtonCaps( HidP_Output, HID_USAGE_PAGE_PID, 0,
                                         PID_USAGE_DC_DEVICE_RESET, buttons, &count, preparsed_data );
    if (status == HIDP_STATUS_SUCCESS && count > 0)
        instance->guidFFDriver = IID_IDirectInputPIDDriver;

    count = ARRAY_SIZE(buttons);
    status = HidP_GetSpecificButtonCaps( HidP_Input, HID_USAGE_PAGE_BUTTON, 0, 0, buttons, &count, preparsed_data );
    if (status != HIDP_STATUS_SUCCESS) count = button_count = 0;
    while (count--)
    {
        if (!buttons[count].IsRange) button_count += 1;
        else button_count += buttons[count].Range.UsageMax - buttons[count].Range.UsageMin + 1;
    }

    count = 1;
    status = HidP_GetSpecificValueCaps( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X,
                                        &value, &count, preparsed_data );
    if (status != HIDP_STATUS_SUCCESS || !count) type = DI8DEVTYPE_SUPPLEMENTAL;

    count = 1;
    status = HidP_GetSpecificValueCaps( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Y,
                                        &value, &count, preparsed_data );
    if (status != HIDP_STATUS_SUCCESS || !count) type = DI8DEVTYPE_SUPPLEMENTAL;

    count = 1;
    status = HidP_GetSpecificValueCaps( HidP_Input, HID_USAGE_PAGE_SIMULATION, 0, HID_USAGE_SIMULATION_STEERING,
                                        &value, &count, preparsed_data );
    if (status == HIDP_STATUS_SUCCESS && count) type = DI8DEVTYPE_DRIVING;

    switch (GET_DIDEVICE_TYPE(type))
    {
    case DI8DEVTYPE_SUPPLEMENTAL:
        type |= (DI8DEVTYPESUPPLEMENTAL_UNKNOWN << 8);
        break;
    case DI8DEVTYPE_GAMEPAD:
        if (button_count < 6) type |= (DI8DEVTYPEGAMEPAD_LIMITED << 8);
        else type |= (DI8DEVTYPEGAMEPAD_STANDARD << 8);
        break;
    case DI8DEVTYPE_JOYSTICK:
        count = 1;
        status = HidP_GetSpecificValueCaps( HidP_Input, HID_USAGE_PAGE_GENERIC, 0,
                                            HID_USAGE_GENERIC_Z, &value, &count, preparsed_data );
        has_z = (status == HIDP_STATUS_SUCCESS && count);

        count = 1;
        status = HidP_GetSpecificValueCaps( HidP_Input, HID_USAGE_PAGE_GENERIC, 0,
                                            HID_USAGE_GENERIC_HATSWITCH, &value, &count, preparsed_data );
        has_pov = (status == HIDP_STATUS_SUCCESS && count);

        if (button_count < 5 || !has_z || !has_pov)
            type |= (DI8DEVTYPEJOYSTICK_LIMITED << 8);
        else
            type |= (DI8DEVTYPEJOYSTICK_STANDARD << 8);
        break;
    case DI8DEVTYPE_DRIVING:
        count = 1;
        status = HidP_GetSpecificValueCaps( HidP_Input, HID_USAGE_PAGE_SIMULATION, 0, HID_USAGE_SIMULATION_ACCELERATOR,
                                            &value, &count, preparsed_data );
        has_accelerator = (status == HIDP_STATUS_SUCCESS && count);

        count = 1;
        status = HidP_GetSpecificValueCaps( HidP_Input, HID_USAGE_PAGE_SIMULATION, 0, HID_USAGE_SIMULATION_BRAKE,
                                            &value, &count, preparsed_data );
        has_brake = (status == HIDP_STATUS_SUCCESS && count);

        count = 1;
        status = HidP_GetSpecificValueCaps( HidP_Input, HID_USAGE_PAGE_SIMULATION, 0, HID_USAGE_SIMULATION_CLUTCH,
                                            &value, &count, preparsed_data );
        has_clutch = (status == HIDP_STATUS_SUCCESS && count);

        if (button_count < 4)
            type |= (DI8DEVTYPEDRIVING_LIMITED << 8);
        else if (has_accelerator && has_brake && has_clutch)
            type |= (DI8DEVTYPEDRIVING_THREEPEDALS << 8);
        else if (has_accelerator && has_brake)
            type |= (DI8DEVTYPEDRIVING_DUALPEDALS << 8);
        else
            type |= (DI8DEVTYPEDRIVING_LIMITED << 8);
        break;
    case DI8DEVTYPE_FLIGHT:
        type |= (DI8DEVTYPEFLIGHT_STICK << 8);
        break;
    }

    instance->dwDevType = device_type_for_version( type, version ) | DIDEVTYPE_HID;
    TRACE("detected device type %#lx\n", instance->dwDevType);

    *device = device_file;
    *preparsed = preparsed_data;
    return TRUE;

failed:
    CloseHandle( device_file );
    HidD_FreePreparsedData( preparsed_data );
    return FALSE;
}

static HRESULT hid_joystick_device_open( int index, DIDEVICEINSTANCEW *filter, WCHAR *device_path,
                                         HANDLE *device, PHIDP_PREPARSED_DATA *preparsed,
                                         HIDD_ATTRIBUTES *attrs, HIDP_CAPS *caps, DWORD version )
{
    char buffer[sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) + MAX_PATH * sizeof(WCHAR)];
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *detail = (void *)buffer;
    SP_DEVICE_INTERFACE_DATA iface = {.cbSize = sizeof(iface)};
    SP_DEVINFO_DATA devinfo = {.cbSize = sizeof(devinfo)};
    DIDEVICEINSTANCEW instance = *filter;
    WCHAR device_id[MAX_PATH], *tmp;
    HDEVINFO set, xi_set;
    UINT32 i = 0, handle;
    BOOL override;
    DWORD type;
    GUID hid;

    TRACE( "index %d, product %s, instance %s\n", index, debugstr_guid( &filter->guidProduct ),
           debugstr_guid( &filter->guidInstance ) );

    HidD_GetHidGuid( &hid );

    set = SetupDiGetClassDevsW( &hid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT );
    if (set == INVALID_HANDLE_VALUE) return DIERR_DEVICENOTREG;
    xi_set = SetupDiGetClassDevsW( &GUID_DEVINTERFACE_WINEXINPUT, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT );

    *device = NULL;
    *preparsed = NULL;
    while (SetupDiEnumDeviceInterfaces( set, NULL, &hid, i++, &iface ))
    {
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (!SetupDiGetDeviceInterfaceDetailW( set, &iface, detail, sizeof(buffer), NULL, &devinfo ))
            continue;
        if (!SetupDiGetDevicePropertyW( set, &devinfo, &DEVPROPKEY_HID_HANDLE, &type,
                                        (BYTE *)&handle, sizeof(handle), NULL, 0 ) ||
            type != DEVPROP_TYPE_UINT32)
            continue;
        if (!hid_joystick_device_try_open( handle, detail->DevicePath, device, preparsed,
                                           attrs, caps, &instance, version ))
            continue;

        if (device_instance_is_disabled( &instance, &override ))
            goto next;

        if (override && SetupDiGetDeviceInstanceIdW( set, &devinfo, device_id, MAX_PATH, NULL ) &&
            (tmp = wcsstr( device_id, L"&IG_" )))
        {
            memcpy( tmp, L"&XI_", sizeof(L"&XI_") - sizeof(WCHAR) );
            if (!SetupDiOpenDeviceInfoW( xi_set, device_id, NULL, 0, &devinfo ))
                goto next;
            if (!SetupDiEnumDeviceInterfaces( xi_set, &devinfo, &GUID_DEVINTERFACE_WINEXINPUT, 0, &iface ))
                goto next;
            if (!SetupDiGetDeviceInterfaceDetailW( xi_set, &iface, detail, sizeof(buffer), NULL, &devinfo ))
                goto next;

            CloseHandle( *device );
            HidD_FreePreparsedData( *preparsed );
            if (!hid_joystick_device_try_open( handle, detail->DevicePath, device, preparsed,
                                               attrs, caps, &instance, version ))
                continue;
        }

        /* enumerate device by GUID */
        if (index < 0 && IsEqualGUID( &filter->guidProduct, &instance.guidProduct )) break;
        if (index < 0 && IsEqualGUID( &filter->guidInstance, &instance.guidInstance )) break;

        /* enumerate all devices */
        if (index >= 0 && !index--) break;

    next:
        CloseHandle( *device );
        HidD_FreePreparsedData( *preparsed );
        *device = NULL;
        *preparsed = NULL;
    }

    if (xi_set != INVALID_HANDLE_VALUE) SetupDiDestroyDeviceInfoList( xi_set );
    SetupDiDestroyDeviceInfoList( set );
    if (!*device || !*preparsed) return DIERR_DEVICENOTREG;

    lstrcpynW( device_path, detail->DevicePath, MAX_PATH );
    *filter = instance;
    return DI_OK;
}

HRESULT hid_joystick_enum_device( DWORD type, DWORD flags, DIDEVICEINSTANCEW *instance, DWORD version, int index )
{
    HIDD_ATTRIBUTES attrs = {.Size = sizeof(attrs)};
    PHIDP_PREPARSED_DATA preparsed;
    WCHAR device_path[MAX_PATH];
    HIDP_CAPS caps;
    HANDLE device;
    HRESULT hr;

    TRACE( "type %#lx, flags %#lx, instance %p, version %#lx, index %d\n", type, flags, instance, version, index );

    hr = hid_joystick_device_open( index, instance, device_path, &device, &preparsed,
                                   &attrs, &caps, version );
    if (hr != DI_OK) return hr;

    HidD_FreePreparsedData( preparsed );
    CloseHandle( device );

    TRACE( "found device %s, usage %04x:%04x, product %s, instance %s, name %s\n", debugstr_w(device_path),
           instance->wUsagePage, instance->wUsage, debugstr_guid( &instance->guidProduct ),
           debugstr_guid( &instance->guidInstance ), debugstr_w(instance->tszInstanceName) );

    return DI_OK;
}

static BOOL init_object_properties( struct hid_joystick *impl, struct hid_value_caps *caps,
                                    DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    struct object_properties *properties = impl->base.object_properties + instance->dwOfs / sizeof(LONG);
    LONG tmp;

    properties->bit_size = caps->bit_size;
    properties->physical_min = caps->physical_min;
    properties->physical_max = caps->physical_max;
    properties->logical_min = caps->logical_min;
    properties->logical_max = caps->logical_max;

    if (instance->dwType & DIDFT_AXIS) properties->range_max = 65535;
    else
    {
        properties->range_max = 36000;
        tmp = caps->logical_max - caps->logical_min;
        if (tmp > 0) properties->range_max -= 36000 / (tmp + 1);
    }

    properties->saturation = 10000;
    return DIENUM_CONTINUE;
}

static BOOL init_pid_reports( struct hid_joystick *impl, struct hid_value_caps *caps,
                              DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    struct pid_set_constant_force *set_constant_force = &impl->pid_set_constant_force;
    struct pid_set_ramp_force *set_ramp_force = &impl->pid_set_ramp_force;
    struct pid_control_report *device_control = &impl->pid_device_control;
    struct pid_control_report *effect_control = &impl->pid_effect_control;
    struct pid_effect_update *effect_update = &impl->pid_effect_update;
    struct pid_set_condition *set_condition = &impl->pid_set_condition;
    struct pid_set_periodic *set_periodic = &impl->pid_set_periodic;
    struct pid_set_envelope *set_envelope = &impl->pid_set_envelope;
    struct pid_device_gain *device_gain = &impl->pid_device_gain;
    struct pid_device_pool *device_pool = &impl->pid_device_pool;
    struct pid_block_free *block_free = &impl->pid_block_free;
    struct pid_block_load *block_load = &impl->pid_block_load;
    struct pid_new_effect *new_effect = &impl->pid_new_effect;
    struct pid_effect_state *effect_state = &impl->pid_effect_state;

#define SET_COLLECTION( rep )                                          \
    do                                                                 \
    {                                                                  \
        if (rep->collection) FIXME( "duplicate " #rep " report!\n" );  \
        else rep->collection = DIDFT_GETINSTANCE( instance->dwType );  \
    } while (0)

#define SET_SUB_COLLECTION( rep, sub )                                 \
    do {                                                               \
        if (instance->wCollectionNumber != rep->collection)            \
            FIXME( "unexpected " #rep "." #sub " parent!\n" );         \
        else if (rep->sub)                                             \
            FIXME( "duplicate " #rep "." #sub " collection!\n" );      \
        else                                                           \
            rep->sub = DIDFT_GETINSTANCE( instance->dwType );          \
    } while (0)

    if (instance->wUsagePage == HID_USAGE_PAGE_PID)
    {
        switch (instance->wUsage)
        {
        case PID_USAGE_DEVICE_CONTROL_REPORT: SET_COLLECTION( device_control ); break;
        case PID_USAGE_EFFECT_OPERATION_REPORT: SET_COLLECTION( effect_control ); break;
        case PID_USAGE_SET_EFFECT_REPORT: SET_COLLECTION( effect_update ); break;
        case PID_USAGE_SET_PERIODIC_REPORT: SET_COLLECTION( set_periodic ); break;
        case PID_USAGE_SET_ENVELOPE_REPORT: SET_COLLECTION( set_envelope ); break;
        case PID_USAGE_SET_CONDITION_REPORT: SET_COLLECTION( set_condition ); break;
        case PID_USAGE_SET_CONSTANT_FORCE_REPORT: SET_COLLECTION( set_constant_force ); break;
        case PID_USAGE_SET_RAMP_FORCE_REPORT: SET_COLLECTION( set_ramp_force ); break;
        case PID_USAGE_DEVICE_GAIN_REPORT: SET_COLLECTION( device_gain ); break;
        case PID_USAGE_POOL_REPORT: SET_COLLECTION( device_pool ); break;
        case PID_USAGE_BLOCK_FREE_REPORT: SET_COLLECTION( block_free ); break;
        case PID_USAGE_BLOCK_LOAD_REPORT: SET_COLLECTION( block_load ); break;
        case PID_USAGE_CREATE_NEW_EFFECT_REPORT: SET_COLLECTION( new_effect ); break;
        case PID_USAGE_STATE_REPORT: SET_COLLECTION( effect_state ); break;

        case PID_USAGE_DEVICE_CONTROL: SET_SUB_COLLECTION( device_control, control_coll ); break;
        case PID_USAGE_EFFECT_OPERATION: SET_SUB_COLLECTION( effect_control, control_coll ); break;
        case PID_USAGE_EFFECT_TYPE:
            if (instance->wCollectionNumber == effect_update->collection)
                SET_SUB_COLLECTION( effect_update, type_coll );
            else if (instance->wCollectionNumber == new_effect->collection)
                SET_SUB_COLLECTION( new_effect, type_coll );
            break;
        case PID_USAGE_AXES_ENABLE: SET_SUB_COLLECTION( effect_update, axes_coll ); break;
        case PID_USAGE_DIRECTION: SET_SUB_COLLECTION( effect_update, direction_coll ); break;
        case PID_USAGE_BLOCK_LOAD_STATUS: SET_SUB_COLLECTION( block_load, status_coll ); break;
        }
    }

#undef SET_SUB_COLLECTION
#undef SET_COLLECTION

    return DIENUM_CONTINUE;
}

static BOOL init_pid_caps( struct hid_joystick *impl, struct hid_value_caps *caps,
                           DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    struct pid_set_constant_force *set_constant_force = &impl->pid_set_constant_force;
    struct pid_set_ramp_force *set_ramp_force = &impl->pid_set_ramp_force;
    struct pid_control_report *device_control = &impl->pid_device_control;
    struct pid_control_report *effect_control = &impl->pid_effect_control;
    struct pid_effect_update *effect_update = &impl->pid_effect_update;
    struct pid_set_condition *set_condition = &impl->pid_set_condition;
    struct pid_set_periodic *set_periodic = &impl->pid_set_periodic;
    struct pid_set_envelope *set_envelope = &impl->pid_set_envelope;
    struct pid_device_gain *device_gain = &impl->pid_device_gain;
    struct pid_device_pool *device_pool = &impl->pid_device_pool;
    struct pid_block_free *block_free = &impl->pid_block_free;
    struct pid_block_load *block_load = &impl->pid_block_load;
    struct pid_new_effect *new_effect = &impl->pid_new_effect;
    struct pid_effect_state *effect_state = &impl->pid_effect_state;

#define SET_REPORT_ID( rep )                                           \
    do                                                                 \
    {                                                                  \
        if (!rep->id)                                                  \
            rep->id = instance->wReportId;                             \
        else if (rep->id != instance->wReportId)                       \
            FIXME( "multiple " #rep " report ids!\n" );                \
    } while (0)

    if (!instance->wCollectionNumber)
        return DIENUM_CONTINUE;

    if (instance->wCollectionNumber == effect_state->collection)
    {
        SET_REPORT_ID( effect_state );
        if (instance->wUsage == PID_USAGE_SAFETY_SWITCH)
            effect_state->safety_switch_caps = caps;
        if (instance->wUsage == PID_USAGE_ACTUATOR_POWER)
            effect_state->actuator_power_caps = caps;
        if (instance->wUsage == PID_USAGE_ACTUATOR_OVERRIDE_SWITCH)
            effect_state->actuator_override_switch_caps = caps;
    }

    if (!(instance->dwType & DIDFT_OUTPUT)) return DIENUM_CONTINUE;

    if (instance->wCollectionNumber == device_control->control_coll)
        SET_REPORT_ID( device_control );
    if (instance->wCollectionNumber == effect_control->control_coll)
        SET_REPORT_ID( effect_control );
    if (instance->wCollectionNumber == effect_update->type_coll)
        SET_REPORT_ID( effect_update );
    if (instance->wCollectionNumber == effect_update->collection)
    {
        SET_REPORT_ID( effect_update );
        if (instance->wUsage == PID_USAGE_DURATION)
            effect_update->duration_caps = caps;
        if (instance->wUsage == PID_USAGE_GAIN)
        {
            caps->physical_min = 0;
            caps->physical_max = 10000;
            effect_update->gain_caps = caps;
        }
        if (instance->wUsage == PID_USAGE_SAMPLE_PERIOD)
            effect_update->sample_period_caps = caps;
        if (instance->wUsage == PID_USAGE_START_DELAY)
            effect_update->start_delay_caps = caps;
        if (instance->wUsage == PID_USAGE_TRIGGER_BUTTON)
            effect_update->trigger_button_caps = caps;
        if (instance->wUsage == PID_USAGE_TRIGGER_REPEAT_INTERVAL)
            effect_update->trigger_repeat_interval_caps = caps;
    }
    if (instance->wCollectionNumber == effect_update->axes_coll)
    {
        SET_REPORT_ID( effect_update );
        caps->physical_min = 0;
        caps->physical_max = 36000;
        if (effect_update->axis_count >= 6) FIXME( "more than 6 PID axes detected\n" );
        else effect_update->axis_caps[effect_update->axis_count] = caps;
        effect_update->axis_count++;
    }
    if (instance->wCollectionNumber == effect_update->direction_coll)
    {
        SET_REPORT_ID( effect_update );
        caps->physical_min = 0;
        caps->physical_max = 35900;
        if (effect_update->direction_count >= 6) FIXME( "more than 6 PID directions detected\n" );
        else effect_update->direction_caps[effect_update->direction_count] = caps;
        effect_update->direction_count++;
    }
    if (instance->wCollectionNumber == set_periodic->collection)
    {
        SET_REPORT_ID( set_periodic );
        if (instance->wUsage == PID_USAGE_MAGNITUDE)
        {
            caps->physical_min = 0;
            caps->physical_max = 10000;
            set_periodic->magnitude_caps = caps;
        }
        if (instance->wUsage == PID_USAGE_PERIOD)
            set_periodic->period_caps = caps;
        if (instance->wUsage == PID_USAGE_PHASE)
        {
            caps->physical_min = 0;
            caps->physical_max = 35900;
            set_periodic->phase_caps = caps;
        }
        if (instance->wUsage == PID_USAGE_OFFSET)
        {
            caps->physical_min = -10000;
            caps->physical_max = 10000;
            set_periodic->offset_caps = caps;
        }
    }
    if (instance->wCollectionNumber == set_envelope->collection)
    {
        SET_REPORT_ID( set_envelope );
        if (instance->wUsage == PID_USAGE_ATTACK_LEVEL)
        {
            caps->physical_min = 0;
            caps->physical_max = 10000;
            set_envelope->attack_level_caps = caps;
        }
        if (instance->wUsage == PID_USAGE_ATTACK_TIME)
            set_envelope->attack_time_caps = caps;
        if (instance->wUsage == PID_USAGE_FADE_LEVEL)
        {
            caps->physical_min = 0;
            caps->physical_max = 10000;
            set_envelope->fade_level_caps = caps;
        }
        if (instance->wUsage == PID_USAGE_FADE_TIME)
            set_envelope->fade_time_caps = caps;
    }
    if (instance->wCollectionNumber == set_condition->collection)
    {
        SET_REPORT_ID( set_condition );
        if (instance->wUsage == PID_USAGE_CP_OFFSET)
        {
            caps->physical_min = -10000;
            caps->physical_max = 10000;
            set_condition->center_point_offset_caps = caps;
        }
        if (instance->wUsage == PID_USAGE_POSITIVE_COEFFICIENT)
        {
            caps->physical_min = -10000;
            caps->physical_max = 10000;
            set_condition->positive_coefficient_caps = caps;
        }
        if (instance->wUsage == PID_USAGE_NEGATIVE_COEFFICIENT)
        {
            caps->physical_min = -10000;
            caps->physical_max = 10000;
            set_condition->negative_coefficient_caps = caps;
        }
        if (instance->wUsage == PID_USAGE_POSITIVE_SATURATION)
        {
            caps->physical_min = 0;
            caps->physical_max = 10000;
            set_condition->positive_saturation_caps = caps;
        }
        if (instance->wUsage == PID_USAGE_NEGATIVE_SATURATION)
        {
            caps->physical_min = 0;
            caps->physical_max = 10000;
            set_condition->negative_saturation_caps = caps;
        }
        if (instance->wUsage == PID_USAGE_DEAD_BAND)
        {
            caps->physical_min = 0;
            caps->physical_max = 10000;
            set_condition->dead_band_caps = caps;
        }
    }
    if (instance->wCollectionNumber == set_constant_force->collection)
    {
        SET_REPORT_ID( set_constant_force );
        if (instance->wUsage == PID_USAGE_MAGNITUDE)
        {
            caps->physical_min = -10000;
            caps->physical_max = 10000;
            set_constant_force->magnitude_caps = caps;
        }
    }
    if (instance->wCollectionNumber == set_ramp_force->collection)
    {
        SET_REPORT_ID( set_ramp_force );
        if (instance->wUsage == PID_USAGE_RAMP_START)
        {
            caps->physical_min = -10000;
            caps->physical_max = 10000;
            set_ramp_force->start_caps = caps;
        }
        if (instance->wUsage == PID_USAGE_RAMP_END)
        {
            caps->physical_min = -10000;
            caps->physical_max = 10000;
            set_ramp_force->end_caps = caps;
        }
    }
    if (instance->wCollectionNumber == device_gain->collection)
    {
        SET_REPORT_ID( device_gain );
        if (instance->wUsage == PID_USAGE_DEVICE_GAIN)
        {
            caps->physical_min = 0;
            caps->physical_max = 10000;
            device_gain->device_gain_caps = caps;
        }
    }
    if (instance->wCollectionNumber == device_pool->collection)
    {
        SET_REPORT_ID( device_pool );
        if (instance->wUsage == PID_USAGE_DEVICE_MANAGED_POOL)
            device_pool->device_managed_caps = caps;
    }
    if (instance->wCollectionNumber == block_free->collection)
        SET_REPORT_ID( block_free );
    if (instance->wCollectionNumber == block_load->collection)
        SET_REPORT_ID( block_load );
    if (instance->wCollectionNumber == block_load->status_coll)
        SET_REPORT_ID( block_load );
    if (instance->wCollectionNumber == new_effect->collection)
        SET_REPORT_ID( new_effect );
    if (instance->wCollectionNumber == new_effect->type_coll)
        SET_REPORT_ID( new_effect );

#undef SET_REPORT_ID

    return DIENUM_CONTINUE;
}

HRESULT hid_joystick_create_device( struct dinput *dinput, const GUID *guid, IDirectInputDevice8W **out )
{
    static const DIPROPHEADER filter =
    {
        .dwSize = sizeof(filter),
        .dwHeaderSize = sizeof(filter),
        .dwHow = DIPH_DEVICE,
    };
    DIDEVICEINSTANCEW instance =
    {
        .dwSize = sizeof(instance),
        .guidProduct = *guid,
        .guidInstance = *guid
    };
    DIPROPRANGE range =
    {
        .diph =
        {
            .dwSize = sizeof(range),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_DEVICE,
        },
    };
    HIDD_ATTRIBUTES attrs = {.Size = sizeof(attrs)};
    struct object_properties *object_properties;
    struct hid_preparsed_data *preparsed;
    struct hid_joystick *impl = NULL;
    USAGE_AND_PAGE *usages;
    char *buffer;
    HRESULT hr;
    DWORD size;

    TRACE( "dinput %p, guid %s, out %p\n", dinput, debugstr_guid( guid ), out );

    *out = NULL;
    instance.guidProduct.Data1 = dinput_pidvid_guid.Data1;
    instance.guidInstance.Data1 = hid_joystick_guid.Data1;
    if (IsEqualGUID( &dinput_pidvid_guid, &instance.guidProduct ))
        instance.guidProduct = *guid;
    else if (IsEqualGUID( &hid_joystick_guid, &instance.guidInstance ))
        instance.guidInstance = *guid;
    else
    {
        instance.guidInstance.Data1 = device_path_guid.Data1;
        instance.guidInstance.Data2 = device_path_guid.Data2;
        instance.guidInstance.Data3 = device_path_guid.Data3;
        if (!IsEqualGUID( &device_path_guid, &instance.guidInstance )) return DIERR_DEVICENOTREG;
    }

    hr = dinput_device_alloc( sizeof(struct hid_joystick), &hid_joystick_vtbl, guid, dinput, (void **)&impl );
    if (FAILED(hr)) return hr;
    impl->base.crit.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": hid_joystick.base.crit");
    impl->base.dwCoopLevel = DISCL_NONEXCLUSIVE | DISCL_BACKGROUND;
    impl->base.read_event = CreateEventW( NULL, TRUE, FALSE, NULL );
    impl->internal_ref = 1;

    if (!IsEqualGUID( &device_path_guid, &instance.guidInstance ))
        hr = hid_joystick_device_open( -1, &instance, impl->device_path, &impl->device, &impl->preparsed,
                                       &attrs, &impl->caps, dinput->dwVersion );
    else
    {
        wcscpy( impl->device_path, *(const WCHAR **)guid );
        if (!hid_joystick_device_try_open( 0, impl->device_path, &impl->device, &impl->preparsed, &attrs,
                                           &impl->caps, &instance, dinput->dwVersion ))
            hr = DIERR_DEVICENOTREG;
    }
    if (hr != DI_OK) goto failed;

    impl->base.instance = instance;
    impl->base.caps.dwDevType = instance.dwDevType;
    impl->attrs = attrs;
    list_init( &impl->effect_list );

    preparsed = (struct hid_preparsed_data *)impl->preparsed;
    size = preparsed->input_caps_count * sizeof(struct object_properties);
    if (!(object_properties = calloc( 1, size ))) goto failed;
    impl->base.object_properties = object_properties;
    enum_objects( impl, &filter, DIDFT_AXIS | DIDFT_POV, init_object_properties, NULL );

    size = impl->caps.InputReportByteLength;
    if (!(buffer = malloc( size ))) goto failed;
    impl->input_report_buf = buffer;
    size = impl->caps.OutputReportByteLength;
    if (!(buffer = malloc( size ))) goto failed;
    impl->output_report_buf = buffer;
    size = impl->caps.FeatureReportByteLength;
    if (!(buffer = malloc( size ))) goto failed;
    impl->feature_report_buf = buffer;
    impl->usages_count = HidP_MaxUsageListLength( HidP_Input, 0, impl->preparsed );
    size = impl->usages_count * sizeof(USAGE_AND_PAGE);
    if (!(usages = malloc( size ))) goto failed;
    impl->usages_buf = usages;

    enum_objects( impl, &filter, DIDFT_COLLECTION, init_pid_reports, NULL );
    enum_objects( impl, &filter, DIDFT_NODATA | DIDFT_BUTTON | DIDFT_AXIS, init_pid_caps, NULL );

    TRACE( "device control id %u, coll %u, control coll %u\n", impl->pid_device_control.id,
           impl->pid_device_control.collection, impl->pid_device_control.control_coll );
    TRACE( "effect control id %u, coll %u\n", impl->pid_effect_control.id, impl->pid_effect_control.collection );
    TRACE( "effect update id %u, coll %u, type_coll %u\n", impl->pid_effect_update.id,
           impl->pid_effect_update.collection, impl->pid_effect_update.type_coll );
    TRACE( "set periodic id %u, coll %u\n", impl->pid_set_periodic.id, impl->pid_set_periodic.collection );
    TRACE( "set envelope id %u, coll %u\n", impl->pid_set_envelope.id, impl->pid_set_envelope.collection );
    TRACE( "set condition id %u, coll %u\n", impl->pid_set_condition.id, impl->pid_set_condition.collection );
    TRACE( "set constant force id %u, coll %u\n", impl->pid_set_constant_force.id,
           impl->pid_set_constant_force.collection );
    TRACE( "set ramp force id %u, coll %u\n", impl->pid_set_ramp_force.id, impl->pid_set_ramp_force.collection );
    TRACE( "device gain id %u, coll %u\n", impl->pid_device_gain.id, impl->pid_device_gain.collection );
    TRACE( "device pool id %u, coll %u\n", impl->pid_device_pool.id, impl->pid_device_pool.collection );
    TRACE( "block free id %u, coll %u\n", impl->pid_block_free.id, impl->pid_block_free.collection );
    TRACE( "block load id %u, coll %u, status_coll %u\n", impl->pid_block_load.id,
           impl->pid_block_load.collection, impl->pid_block_load.status_coll );
    TRACE( "create new effect id %u, coll %u, type_coll %u\n", impl->pid_new_effect.id,
           impl->pid_new_effect.collection, impl->pid_new_effect.type_coll );
    TRACE( "effect state id %u, coll %u\n", impl->pid_effect_state.id, impl->pid_effect_state.collection );

    if (impl->pid_device_control.id)
    {
        impl->base.caps.dwFlags |= DIDC_FORCEFEEDBACK;
        if (impl->pid_effect_update.start_delay_caps)
            impl->base.caps.dwFlags |= DIDC_STARTDELAY;
        if (impl->pid_set_envelope.attack_level_caps ||
            impl->pid_set_envelope.attack_time_caps)
            impl->base.caps.dwFlags |= DIDC_FFATTACK;
        if (impl->pid_set_envelope.fade_level_caps ||
            impl->pid_set_envelope.fade_time_caps)
            impl->base.caps.dwFlags |= DIDC_FFFADE;
        if (impl->pid_set_condition.positive_saturation_caps ||
            impl->pid_set_condition.negative_saturation_caps)
            impl->base.caps.dwFlags |= DIDC_SATURATION;
        if (impl->pid_set_condition.dead_band_caps)
            impl->base.caps.dwFlags |= DIDC_DEADBAND;
        impl->base.caps.dwFFSamplePeriod = 1000000;
        impl->base.caps.dwFFMinTimeResolution = 1000000;
        impl->base.caps.dwHardwareRevision = 1;
        impl->base.caps.dwFFDriverVersion = 1;
    }

    if (FAILED(hr = dinput_device_init( &impl->base.IDirectInputDevice8W_iface ))) goto failed;

    *out = &impl->base.IDirectInputDevice8W_iface;
    return DI_OK;

failed:
    IDirectInputDevice_Release( &impl->base.IDirectInputDevice8W_iface );
    return hr;
}

static HRESULT WINAPI hid_joystick_effect_QueryInterface( IDirectInputEffect *iface, REFIID iid, void **out )
{
    TRACE( "iface %p, iid %s, out %p\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IDirectInputEffect ))
    {
        IDirectInputEffect_AddRef( iface );
        *out = iface;
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI hid_joystick_effect_AddRef( IDirectInputEffect *iface )
{
    struct hid_joystick_effect *impl = impl_from_IDirectInputEffect( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI hid_joystick_effect_Release( IDirectInputEffect *iface )
{
    struct hid_joystick_effect *impl = impl_from_IDirectInputEffect( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    if (!ref)
    {
        IDirectInputEffect_Unload( iface );
        EnterCriticalSection( &impl->joystick->base.crit );
        list_remove( &impl->entry );
        LeaveCriticalSection( &impl->joystick->base.crit );
        hid_joystick_release( &impl->joystick->base.IDirectInputDevice8W_iface );
        free( impl->set_envelope_buf );
        free( impl->type_specific_buf );
        free( impl->effect_update_buf );
        free( impl->effect_control_buf );
        free( impl );
    }
    return ref;
}

static HRESULT WINAPI hid_joystick_effect_Initialize( IDirectInputEffect *iface, HINSTANCE inst,
                                                      DWORD version, REFGUID guid )
{
    struct hid_joystick_effect *impl = impl_from_IDirectInputEffect( iface );
    struct hid_joystick *joystick = impl->joystick;
    ULONG count, report_len = joystick->caps.OutputReportByteLength;
    NTSTATUS status;
    USAGE type;

    TRACE( "iface %p, inst %p, version %#lx, guid %s\n", iface, inst, version, debugstr_guid( guid ) );

    if (!inst) return DIERR_INVALIDPARAM;
    if (!guid) return E_POINTER;
    if (!(type = effect_guid_to_usage( guid ))) return DIERR_DEVICENOTREG;

    status = HidP_InitializeReportForID( HidP_Output, joystick->pid_effect_update.id,
                                         joystick->preparsed, impl->effect_update_buf, report_len );
    if (status != HIDP_STATUS_SUCCESS) return DIERR_DEVICENOTREG;

    impl->type_specific_buf[0] = 0;
    impl->set_envelope_buf[0] = 0;

    switch (type)
    {
    case PID_USAGE_ET_SQUARE:
    case PID_USAGE_ET_SINE:
    case PID_USAGE_ET_TRIANGLE:
    case PID_USAGE_ET_SAWTOOTH_UP:
    case PID_USAGE_ET_SAWTOOTH_DOWN:
        status = HidP_InitializeReportForID( HidP_Output, joystick->pid_set_periodic.id,
                                             joystick->preparsed, impl->type_specific_buf, report_len );
        if (status != HIDP_STATUS_SUCCESS) return DIERR_DEVICENOTREG;
        impl->params.lpvTypeSpecificParams = &impl->periodic;
        break;
    case PID_USAGE_ET_SPRING:
    case PID_USAGE_ET_DAMPER:
    case PID_USAGE_ET_INERTIA:
    case PID_USAGE_ET_FRICTION:
        status = HidP_InitializeReportForID( HidP_Output, joystick->pid_set_condition.id, joystick->preparsed,
                                             impl->type_specific_buf, report_len );
        if (status != HIDP_STATUS_SUCCESS) return DIERR_DEVICENOTREG;
        impl->params.lpvTypeSpecificParams = &impl->condition;
        break;
    case PID_USAGE_ET_CONSTANT_FORCE:
        status = HidP_InitializeReportForID( HidP_Output, joystick->pid_set_constant_force.id, joystick->preparsed,
                                             impl->type_specific_buf, report_len );
        if (status != HIDP_STATUS_SUCCESS) return DIERR_DEVICENOTREG;
        impl->params.lpvTypeSpecificParams = &impl->constant_force;
        break;
    case PID_USAGE_ET_RAMP:
        status = HidP_InitializeReportForID( HidP_Output, joystick->pid_set_ramp_force.id, joystick->preparsed,
                                             impl->type_specific_buf, report_len );
        if (status != HIDP_STATUS_SUCCESS) return DIERR_DEVICENOTREG;
        impl->params.lpvTypeSpecificParams = &impl->ramp_force;
        break;
    case PID_USAGE_ET_CUSTOM_FORCE_DATA:
        FIXME( "effect type %#x not implemented!\n", type );
        break;
    }

    switch (type)
    {
    case PID_USAGE_ET_SQUARE:
    case PID_USAGE_ET_SINE:
    case PID_USAGE_ET_TRIANGLE:
    case PID_USAGE_ET_SAWTOOTH_UP:
    case PID_USAGE_ET_SAWTOOTH_DOWN:
    case PID_USAGE_ET_CONSTANT_FORCE:
    case PID_USAGE_ET_RAMP:
        status = HidP_InitializeReportForID( HidP_Output, joystick->pid_set_envelope.id, joystick->preparsed,
                                             impl->set_envelope_buf, report_len );
        if (status != HIDP_STATUS_SUCCESS) return DIERR_DEVICENOTREG;
        break;
    }

    count = 1;
    status = HidP_SetUsages( HidP_Output, HID_USAGE_PAGE_PID, joystick->pid_effect_update.type_coll,
                             &type, &count, joystick->preparsed, impl->effect_update_buf, report_len );
    if (status != HIDP_STATUS_SUCCESS) return DIERR_DEVICENOTREG;

    impl->type = type;
    return DI_OK;
}

static HRESULT WINAPI hid_joystick_effect_GetEffectGuid( IDirectInputEffect *iface, GUID *guid )
{
    struct hid_joystick_effect *impl = impl_from_IDirectInputEffect( iface );

    TRACE( "iface %p, guid %p.\n", iface, guid );

    if (!guid) return E_POINTER;
    *guid = *effect_usage_to_guid( impl->type );

    return DI_OK;
}

static BOOL get_parameters_object_id( struct hid_joystick *impl, struct hid_value_caps *caps,
                                      DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    *(DWORD *)data = instance->dwType;
    return DIENUM_STOP;
}

static BOOL get_parameters_object_ofs( struct hid_joystick *impl, struct hid_value_caps *caps,
                                       DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    DIDATAFORMAT *device_format = impl->base.device_format, *user_format = impl->base.user_format;
    DIOBJECTDATAFORMAT *device_obj, *user_obj;

    if (!user_format) return DIENUM_CONTINUE;

    user_obj = user_format->rgodf + device_format->dwNumObjs;
    device_obj = device_format->rgodf + device_format->dwNumObjs;
    while (user_obj-- > user_format->rgodf && device_obj-- > device_format->rgodf)
    {
        if (!user_obj->dwType) continue;
        if (device_obj->dwType == instance->dwType) break;
    }
    if (user_obj < user_format->rgodf) return DIENUM_CONTINUE;

    *(DWORD *)data = user_obj->dwOfs;
    return DIENUM_STOP;
}

static void convert_directions_to_spherical( const DIEFFECT *in, DIEFFECT *out )
{
    DWORD i, j, direction_flags = DIEFF_CARTESIAN | DIEFF_POLAR | DIEFF_SPHERICAL;
    double tmp;

    switch (in->dwFlags & direction_flags)
    {
    case DIEFF_CARTESIAN:
        for (i = 1; i < in->cAxes; ++i)
        {
            tmp = in->rglDirection[0];
            for (j = 1; j < i; ++j) tmp = sqrt( tmp * tmp + in->rglDirection[j] * in->rglDirection[j] );
            tmp = atan2( in->rglDirection[i], tmp );
            out->rglDirection[i - 1] = tmp * 18000 / M_PI;
        }
        if (in->cAxes) out->rglDirection[in->cAxes - 1] = 0;
        out->cAxes = in->cAxes;
        break;
    case DIEFF_POLAR:
        out->rglDirection[0] = (in->rglDirection[0] % 36000) - 9000;
        if (out->rglDirection[0] < 0) out->rglDirection[0] += 36000;
        for (i = 1; i < in->cAxes; ++i) out->rglDirection[i] = 0;
        out->cAxes = in->cAxes;
        break;
    case DIEFF_SPHERICAL:
        if (!in->cAxes) i = 0;
        else for (i = 0; i < in->cAxes - 1; ++i)
        {
            out->rglDirection[i] = in->rglDirection[i] % 36000;
            if (out->rglDirection[i] < 0) out->rglDirection[i] += 36000;
        }
        out->rglDirection[i] = 0;
        out->cAxes = in->cAxes;
        break;
    }
}

static void convert_directions_from_spherical( const DIEFFECT *in, DIEFFECT *out )
{
    DWORD i, j, direction_flags = DIEFF_CARTESIAN | DIEFF_POLAR | DIEFF_SPHERICAL;
    LONG tmp;

    switch (out->dwFlags & direction_flags)
    {
    case DIEFF_CARTESIAN:
        out->rglDirection[0] = 10000;
        for (i = 1; i <= in->cAxes; ++i)
        {
            tmp = cos( in->rglDirection[i - 1] * M_PI / 18000 ) * 10000;
            for (j = 0; j < i; ++j)
                out->rglDirection[j] = round( out->rglDirection[j] * tmp / 10000.0 );
            out->rglDirection[i] = sin( in->rglDirection[i - 1] * M_PI / 18000 ) * 10000;
        }
        out->cAxes = in->cAxes;
        break;
    case DIEFF_POLAR:
        out->rglDirection[0] = (in->rglDirection[0] + 9000) % 36000;
        if (out->rglDirection[0] < 0) out->rglDirection[0] += 36000;
        out->rglDirection[1] = 0;
        out->cAxes = 2;
        break;
    case DIEFF_SPHERICAL:
        for (i = 0; i < in->cAxes; ++i)
        {
            out->rglDirection[i] = in->rglDirection[i] % 36000;
            if (out->rglDirection[i] < 0) out->rglDirection[i] += 36000;
        }
        out->cAxes = in->cAxes;
        break;
    }
}

static void convert_directions( const DIEFFECT *in, DIEFFECT *out )
{
    DWORD direction_flags = DIEFF_CARTESIAN | DIEFF_POLAR | DIEFF_SPHERICAL;
    LONG directions[6] = {0};
    DIEFFECT spherical = {.rglDirection = directions};

    switch (in->dwFlags & direction_flags)
    {
    case DIEFF_CARTESIAN:
        switch (out->dwFlags & direction_flags)
        {
        case DIEFF_CARTESIAN:
            memcpy( out->rglDirection, in->rglDirection, in->cAxes * sizeof(LONG) );
            out->cAxes = in->cAxes;
            break;
        case DIEFF_POLAR:
            convert_directions_to_spherical( in, &spherical );
            convert_directions_from_spherical( &spherical, out );
            break;
        case DIEFF_SPHERICAL:
            convert_directions_to_spherical( in, out );
            break;
        }
        break;

    case DIEFF_POLAR:
        switch (out->dwFlags & direction_flags)
        {
        case DIEFF_POLAR:
            memcpy( out->rglDirection, in->rglDirection, in->cAxes * sizeof(LONG) );
            out->cAxes = in->cAxes;
            break;
        case DIEFF_CARTESIAN:
            convert_directions_to_spherical( in, &spherical );
            convert_directions_from_spherical( &spherical, out );
            break;
        case DIEFF_SPHERICAL:
            convert_directions_to_spherical( in, out );
            break;
        }
        break;

    case DIEFF_SPHERICAL:
        switch (out->dwFlags & direction_flags)
        {
        case DIEFF_POLAR:
        case DIEFF_CARTESIAN:
            convert_directions_from_spherical( in, out );
            break;
        case DIEFF_SPHERICAL:
            convert_directions_to_spherical( in, out );
            break;
        }
        break;
    }
}

static HRESULT WINAPI hid_joystick_effect_GetParameters( IDirectInputEffect *iface, DIEFFECT *params, DWORD flags )
{
    DIPROPHEADER filter =
    {
        .dwSize = sizeof(DIPROPHEADER),
        .dwHeaderSize = sizeof(DIPROPHEADER),
        .dwHow = DIPH_BYUSAGE,
    };
    struct hid_joystick_effect *impl = impl_from_IDirectInputEffect( iface );
    ULONG i, count, capacity, object_flags, direction_flags;
    BOOL ret;

    TRACE( "iface %p, params %p, flags %#lx.\n", iface, params, flags );

    if (!params) return DI_OK;
    if (params->dwSize != sizeof(DIEFFECT_DX6) && params->dwSize != sizeof(DIEFFECT_DX5)) return DIERR_INVALIDPARAM;
    capacity = params->cAxes;
    object_flags = params->dwFlags & (DIEFF_OBJECTIDS | DIEFF_OBJECTOFFSETS);
    direction_flags = params->dwFlags & (DIEFF_CARTESIAN | DIEFF_POLAR | DIEFF_SPHERICAL);

    if (flags & DIEP_AXES)
    {
        if (!object_flags) return DIERR_INVALIDPARAM;
        params->cAxes = impl->params.cAxes;
        if (capacity < impl->params.cAxes) return DIERR_MOREDATA;

        for (i = 0; i < impl->params.cAxes; ++i)
        {
            if (!params->rgdwAxes) return DIERR_INVALIDPARAM;
            filter.dwObj = impl->params.rgdwAxes[i];
            if (object_flags & DIEFF_OBJECTIDS)
                ret = enum_objects( impl->joystick, &filter, DIDFT_AXIS, get_parameters_object_id,
                                    &params->rgdwAxes[i] );
            else
                ret = enum_objects( impl->joystick, &filter, DIDFT_AXIS, get_parameters_object_ofs,
                                    &params->rgdwAxes[i] );
            if (ret != DIENUM_STOP) params->rgdwAxes[i] = 0;
        }
    }

    if (flags & DIEP_DIRECTION)
    {
        if (!direction_flags) return DIERR_INVALIDPARAM;

        count = params->cAxes = impl->params.cAxes;
        if (!count) params->dwFlags &= ~(DIEFF_CARTESIAN | DIEFF_POLAR | DIEFF_SPHERICAL);
        if ((direction_flags & DIEFF_POLAR) && count != 2) return DIERR_INVALIDPARAM;
        if (capacity < params->cAxes) return DIERR_MOREDATA;

        if (!count) params->rglDirection = NULL;
        else if (!params->rglDirection) return DIERR_INVALIDPARAM;
        else convert_directions( &impl->params, params );
    }

    if (flags & DIEP_TYPESPECIFICPARAMS)
    {
        capacity = params->cbTypeSpecificParams;
        params->cbTypeSpecificParams = impl->params.cbTypeSpecificParams;
        if (capacity < impl->params.cbTypeSpecificParams) return DIERR_MOREDATA;
        if (impl->params.lpvTypeSpecificParams)
        {
            if (!params->lpvTypeSpecificParams) return E_POINTER;
            memcpy( params->lpvTypeSpecificParams, impl->params.lpvTypeSpecificParams,
                    impl->params.cbTypeSpecificParams );
        }
    }

    if (flags & DIEP_ENVELOPE)
    {
        if (!params->lpEnvelope) return E_POINTER;
        if (params->lpEnvelope->dwSize != sizeof(DIENVELOPE)) return DIERR_INVALIDPARAM;
        if (!impl->params.lpEnvelope) params->lpEnvelope = NULL;
        else memcpy( params->lpEnvelope, impl->params.lpEnvelope, sizeof(DIENVELOPE) );
    }

    if (flags & DIEP_DURATION) params->dwDuration = impl->params.dwDuration;
    if (flags & DIEP_GAIN) params->dwGain = impl->params.dwGain;
    if (flags & DIEP_SAMPLEPERIOD) params->dwSamplePeriod = impl->params.dwSamplePeriod;
    if (flags & DIEP_STARTDELAY)
    {
        if (params->dwSize != sizeof(DIEFFECT_DX6)) return DIERR_INVALIDPARAM;
        params->dwStartDelay = impl->params.dwStartDelay;
    }
    if (flags & DIEP_TRIGGERREPEATINTERVAL) params->dwTriggerRepeatInterval = impl->params.dwTriggerRepeatInterval;

    if (flags & DIEP_TRIGGERBUTTON)
    {
        if (!object_flags) return DIERR_INVALIDPARAM;

        filter.dwObj = impl->params.dwTriggerButton;
        if (object_flags & DIEFF_OBJECTIDS)
            ret = enum_objects( impl->joystick, &filter, DIDFT_BUTTON, get_parameters_object_id,
                                &params->dwTriggerButton );
        else
            ret = enum_objects( impl->joystick, &filter, DIDFT_BUTTON, get_parameters_object_ofs,
                                &params->dwTriggerButton );
        if (ret != DIENUM_STOP) params->dwTriggerButton = -1;
    }

    return DI_OK;
}

static BOOL set_parameters_object( struct hid_joystick *impl, struct hid_value_caps *caps,
                                   DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    DWORD usages = MAKELONG( instance->wUsage, instance->wUsagePage );
    *(DWORD *)data = usages;
    return DIENUM_STOP;
}

static HRESULT WINAPI hid_joystick_effect_SetParameters( IDirectInputEffect *iface,
                                                         const DIEFFECT *params, DWORD flags )
{
    DIPROPHEADER filter =
    {
        .dwSize = sizeof(DIPROPHEADER),
        .dwHeaderSize = sizeof(DIPROPHEADER),
        .dwHow = DIPH_BYUSAGE,
    };
    struct hid_joystick_effect *impl = impl_from_IDirectInputEffect( iface );
    ULONG i, count, old_value, object_flags, direction_flags;
    HRESULT hr;
    BOOL ret;

    TRACE( "iface %p, params %p, flags %#lx.\n", iface, params, flags );

    if (!params) return E_POINTER;
    if (params->dwSize != sizeof(DIEFFECT_DX6) && params->dwSize != sizeof(DIEFFECT_DX5)) return DIERR_INVALIDPARAM;
    object_flags = params->dwFlags & (DIEFF_OBJECTIDS | DIEFF_OBJECTOFFSETS);
    direction_flags = params->dwFlags & (DIEFF_CARTESIAN | DIEFF_POLAR | DIEFF_SPHERICAL);

    if (object_flags & DIEFF_OBJECTIDS) filter.dwHow = DIPH_BYID;
    else filter.dwHow = DIPH_BYOFFSET;

    if (flags & DIEP_AXES)
    {
        if (!object_flags) return DIERR_INVALIDPARAM;
        if (!params->rgdwAxes) return DIERR_INVALIDPARAM;
        if (impl->params.cAxes) return DIERR_ALREADYINITIALIZED;
        count = impl->joystick->pid_effect_update.axis_count;
        if (params->cAxes > count) return DIERR_INVALIDPARAM;

        impl->params.cAxes = params->cAxes;
        for (i = 0; i < params->cAxes; ++i)
        {
            filter.dwObj = params->rgdwAxes[i];
            ret = enum_objects( impl->joystick, &filter, DIDFT_AXIS, set_parameters_object,
                                &impl->params.rgdwAxes[i] );
            if (ret != DIENUM_STOP) impl->params.rgdwAxes[i] = 0;
        }

        impl->modified |= DIEP_AXES;
    }

    if (flags & DIEP_DIRECTION)
    {
        if (!direction_flags) return DIERR_INVALIDPARAM;
        if (!params->rglDirection) return DIERR_INVALIDPARAM;

        count = impl->params.cAxes;
        if (params->cAxes < count) return DIERR_INVALIDPARAM;
        if ((direction_flags & DIEFF_POLAR) && count != 2) return DIERR_INVALIDPARAM;
        if ((direction_flags & DIEFF_CARTESIAN) && params->cAxes != count) return DIERR_INVALIDPARAM;

        impl->params.dwFlags &= ~(DIEFF_CARTESIAN | DIEFF_POLAR | DIEFF_SPHERICAL);
        impl->params.dwFlags |= direction_flags;
        if (memcmp( impl->params.rglDirection, params->rglDirection, count * sizeof(LONG) ))
            impl->modified |= DIEP_DIRECTION;
        memcpy( impl->params.rglDirection, params->rglDirection, count * sizeof(LONG) );
    }

    if (flags & DIEP_TYPESPECIFICPARAMS)
    {
        if (!params->lpvTypeSpecificParams) return E_POINTER;
        switch (impl->type)
        {
        case PID_USAGE_ET_SQUARE:
        case PID_USAGE_ET_SINE:
        case PID_USAGE_ET_TRIANGLE:
        case PID_USAGE_ET_SAWTOOTH_UP:
        case PID_USAGE_ET_SAWTOOTH_DOWN:
            if (params->cbTypeSpecificParams != sizeof(DIPERIODIC))
                return DIERR_INVALIDPARAM;
            break;
        case PID_USAGE_ET_SPRING:
        case PID_USAGE_ET_DAMPER:
        case PID_USAGE_ET_INERTIA:
        case PID_USAGE_ET_FRICTION:
            if (params->cbTypeSpecificParams != sizeof(DICONDITION) && impl->params.cAxes &&
                params->cbTypeSpecificParams != impl->params.cAxes * sizeof(DICONDITION))
                return DIERR_INVALIDPARAM;
            break;
        case PID_USAGE_ET_CONSTANT_FORCE:
            if (params->cbTypeSpecificParams != sizeof(DICONSTANTFORCE))
                return DIERR_INVALIDPARAM;
            break;
        case PID_USAGE_ET_RAMP:
            if (params->cbTypeSpecificParams != sizeof(DIRAMPFORCE))
                return DIERR_INVALIDPARAM;
            break;
        case PID_USAGE_ET_CUSTOM_FORCE_DATA:
            FIXME( "custom force data not implemented!\n" );
            return DIERR_UNSUPPORTED;
        }

        if (memcmp( impl->params.lpvTypeSpecificParams, params->lpvTypeSpecificParams,
                    params->cbTypeSpecificParams ))
            impl->modified |= DIEP_TYPESPECIFICPARAMS;
        memcpy( impl->params.lpvTypeSpecificParams, params->lpvTypeSpecificParams,
                params->cbTypeSpecificParams );
        impl->params.cbTypeSpecificParams = params->cbTypeSpecificParams;
    }

    if ((flags & DIEP_ENVELOPE) && params->lpEnvelope)
    {
        if (params->lpEnvelope->dwSize != sizeof(DIENVELOPE)) return DIERR_INVALIDPARAM;
        impl->params.lpEnvelope = &impl->envelope;
        if (memcmp( impl->params.lpEnvelope, params->lpEnvelope, sizeof(DIENVELOPE) ))
            impl->modified |= DIEP_ENVELOPE;
        memcpy( impl->params.lpEnvelope, params->lpEnvelope, sizeof(DIENVELOPE) );
    }

    if (flags & DIEP_DURATION)
    {
        impl->modified |= DIEP_DURATION;
        impl->params.dwDuration = params->dwDuration;
    }
    if (flags & DIEP_GAIN)
    {
        if (impl->params.dwGain != params->dwGain) impl->modified |= DIEP_GAIN;
        impl->params.dwGain = params->dwGain;
    }
    if (flags & DIEP_SAMPLEPERIOD)
    {
        if (impl->params.dwSamplePeriod != params->dwSamplePeriod) impl->modified |= DIEP_SAMPLEPERIOD;
        impl->params.dwSamplePeriod = params->dwSamplePeriod;
    }
    if (flags & DIEP_STARTDELAY)
    {
        if (params->dwSize != sizeof(DIEFFECT_DX6)) return DIERR_INVALIDPARAM;
        if (impl->params.dwStartDelay != params->dwStartDelay) impl->modified |= DIEP_STARTDELAY;
        impl->params.dwStartDelay = params->dwStartDelay;
    }
    if (flags & DIEP_TRIGGERREPEATINTERVAL)
    {
        if (impl->params.dwTriggerRepeatInterval != params->dwTriggerRepeatInterval)
            impl->modified |= DIEP_TRIGGERREPEATINTERVAL;
        impl->params.dwTriggerRepeatInterval = params->dwTriggerRepeatInterval;
    }

    if (flags & DIEP_TRIGGERBUTTON)
    {
        if (!object_flags) return DIERR_INVALIDPARAM;

        filter.dwObj = params->dwTriggerButton;
        old_value = impl->params.dwTriggerButton;
        ret = enum_objects( impl->joystick, &filter, DIDFT_BUTTON, set_parameters_object,
                            &impl->params.dwTriggerButton );
        if (ret != DIENUM_STOP) impl->params.dwTriggerButton = -1;
        if (impl->params.dwTriggerButton != old_value) impl->modified |= DIEP_TRIGGERBUTTON;
    }

    impl->flags |= flags;

    if (flags & DIEP_NODOWNLOAD) return DI_DOWNLOADSKIPPED;
    if (flags & DIEP_START) hr = IDirectInputEffect_Start( iface, 1, 0 );
    else hr = IDirectInputEffect_Download( iface );
    if (hr == DIERR_NOTEXCLUSIVEACQUIRED) return DI_DOWNLOADSKIPPED;
    if (FAILED(hr)) return hr;
    return DI_OK;
}

static HRESULT WINAPI hid_joystick_effect_Start( IDirectInputEffect *iface, DWORD iterations, DWORD flags )
{
    struct hid_joystick_effect *impl = impl_from_IDirectInputEffect( iface );
    struct pid_control_report *effect_control = &impl->joystick->pid_effect_control;
    ULONG count, report_len = impl->joystick->caps.OutputReportByteLength;
    PHIDP_PREPARSED_DATA preparsed = impl->joystick->preparsed;
    HANDLE device = impl->joystick->device;
    NTSTATUS status;
    USAGE control;
    HRESULT hr;

    TRACE( "iface %p, iterations %lu, flags %#lx.\n", iface, iterations, flags );

    if ((flags & ~(DIES_NODOWNLOAD|DIES_SOLO))) return DIERR_INVALIDPARAM;
    if (flags & DIES_SOLO) control = PID_USAGE_OP_EFFECT_START_SOLO;
    else control = PID_USAGE_OP_EFFECT_START;

    EnterCriticalSection( &impl->joystick->base.crit );
    if (!is_exclusively_acquired( impl->joystick ))
        hr = DIERR_NOTEXCLUSIVEACQUIRED;
    else if ((flags & DIES_NODOWNLOAD) && !impl->index)
        hr = DIERR_NOTDOWNLOADED;
    else if ((flags & DIES_NODOWNLOAD) || SUCCEEDED(hr = IDirectInputEffect_Download( iface )))
    {
        count = 1;
        status = HidP_InitializeReportForID( HidP_Output, effect_control->id, preparsed,
                                             impl->effect_control_buf, report_len );

        if (status != HIDP_STATUS_SUCCESS) hr = status;
        else status = HidP_SetUsageValue( HidP_Output, HID_USAGE_PAGE_PID, 0, PID_USAGE_EFFECT_BLOCK_INDEX,
                                          impl->index, preparsed, impl->effect_control_buf, report_len );

        if (status != HIDP_STATUS_SUCCESS) hr = status;
        else status = HidP_SetUsages( HidP_Output, HID_USAGE_PAGE_PID, effect_control->control_coll,
                                      &control, &count, preparsed, impl->effect_control_buf, report_len );

        if (status != HIDP_STATUS_SUCCESS) hr = status;
        else status = HidP_SetUsageValue( HidP_Output, HID_USAGE_PAGE_PID, 0, PID_USAGE_LOOP_COUNT,
                                          iterations, preparsed, impl->effect_control_buf, report_len );

        if (status != HIDP_STATUS_SUCCESS) hr = status;
        else if (WriteFile( device, impl->effect_control_buf, report_len, NULL, NULL )) hr = DI_OK;
        else hr = DIERR_INPUTLOST;

        if (SUCCEEDED(hr)) impl->status |= DIEGES_PLAYING;
        else impl->status &= ~DIEGES_PLAYING;
    }
    LeaveCriticalSection( &impl->joystick->base.crit );

    return hr;
}

static HRESULT WINAPI hid_joystick_effect_Stop( IDirectInputEffect *iface )
{
    struct hid_joystick_effect *impl = impl_from_IDirectInputEffect( iface );
    struct pid_control_report *effect_control = &impl->joystick->pid_effect_control;
    ULONG count, report_len = impl->joystick->caps.OutputReportByteLength;
    PHIDP_PREPARSED_DATA preparsed = impl->joystick->preparsed;
    HANDLE device = impl->joystick->device;
    NTSTATUS status;
    USAGE control;
    HRESULT hr;

    TRACE( "iface %p.\n", iface );

    EnterCriticalSection( &impl->joystick->base.crit );
    if (!is_exclusively_acquired( impl->joystick ))
        hr = DIERR_NOTEXCLUSIVEACQUIRED;
    else if (!impl->index)
        hr = DIERR_NOTDOWNLOADED;
    else
    {
        count = 1;
        control = PID_USAGE_OP_EFFECT_STOP;
        status = HidP_InitializeReportForID( HidP_Output, effect_control->id, preparsed,
                                             impl->effect_control_buf, report_len );

        if (status != HIDP_STATUS_SUCCESS) hr = status;
        else status = HidP_SetUsageValue( HidP_Output, HID_USAGE_PAGE_PID, 0, PID_USAGE_EFFECT_BLOCK_INDEX,
                                          impl->index, preparsed, impl->effect_control_buf, report_len );

        if (status != HIDP_STATUS_SUCCESS) hr = status;
        else status = HidP_SetUsages( HidP_Output, HID_USAGE_PAGE_PID, effect_control->control_coll,
                                      &control, &count, preparsed, impl->effect_control_buf, report_len );

        if (status != HIDP_STATUS_SUCCESS) hr = status;
        else status = HidP_SetUsageValue( HidP_Output, HID_USAGE_PAGE_PID, 0, PID_USAGE_LOOP_COUNT,
                                          0, preparsed, impl->effect_control_buf, report_len );

        if (status != HIDP_STATUS_SUCCESS) hr = status;
        else if (WriteFile( device, impl->effect_control_buf, report_len, NULL, NULL )) hr = DI_OK;
        else hr = DIERR_INPUTLOST;

        impl->status &= ~DIEGES_PLAYING;
    }
    LeaveCriticalSection( &impl->joystick->base.crit );

    return hr;
}

static HRESULT WINAPI hid_joystick_effect_GetEffectStatus( IDirectInputEffect *iface, DWORD *status )
{
    struct hid_joystick_effect *impl = impl_from_IDirectInputEffect( iface );
    HRESULT hr = DI_OK;

    TRACE( "iface %p, status %p.\n", iface, status );

    if (!status) return E_POINTER;
    *status = 0;

    EnterCriticalSection( &impl->joystick->base.crit );
    if (!is_exclusively_acquired( impl->joystick ))
        hr = DIERR_NOTEXCLUSIVEACQUIRED;
    else if (!impl->index)
        hr = DIERR_NOTDOWNLOADED;
    else
        *status = impl->status;
    LeaveCriticalSection( &impl->joystick->base.crit );

    return hr;
}

static void set_parameter_value( struct hid_joystick_effect *impl, char *report_buf,
                                 struct hid_value_caps *caps, LONG value )
{
    return set_report_value( impl->joystick, report_buf, caps, value );
}

static void set_parameter_value_angle( struct hid_joystick_effect *impl, char *report_buf,
                                       struct hid_value_caps *caps, LONG value )
{
    LONG exp;
    if (!caps) return;
    exp = caps->units_exp;
    if (caps->units != 0x14) WARN( "unknown angle unit caps %#lx\n", caps->units );
    else if (exp < -2) while (exp++ < -2) value *= 10;
    else if (exp > -2) while (exp-- > -2) value /= 10;
    set_parameter_value( impl, report_buf, caps, value );
}

static void set_parameter_value_us( struct hid_joystick_effect *impl, char *report_buf,
                                    struct hid_value_caps *caps, LONG value )
{
    LONG exp;
    if (!caps) return;
    exp = caps->units_exp;
    if (value == INFINITE) value = caps->physical_min - 1;
    else if (caps->units != 0x1003) WARN( "unknown time unit caps %#lx\n", caps->units );
    else if (exp < -6) while (exp++ < -6) value *= 10;
    else if (exp > -6) while (exp-- > -6) value /= 10;
    set_parameter_value( impl, report_buf, caps, value );
}

static BOOL is_axis_usage_enabled( struct hid_joystick_effect *impl, USAGE usage )
{
    DWORD i = impl->params.cAxes;
    while (i--) if (LOWORD(impl->params.rgdwAxes[i]) == usage) return TRUE;
    return FALSE;
}

static HRESULT WINAPI hid_joystick_effect_Download( IDirectInputEffect *iface )
{
    static const DWORD complete_mask = DIEP_AXES | DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS;
    struct hid_joystick_effect *impl = impl_from_IDirectInputEffect( iface );
    struct pid_set_constant_force *set_constant_force = &impl->joystick->pid_set_constant_force;
    struct pid_set_ramp_force *set_ramp_force = &impl->joystick->pid_set_ramp_force;
    struct pid_effect_update *effect_update = &impl->joystick->pid_effect_update;
    struct pid_set_condition *set_condition = &impl->joystick->pid_set_condition;
    struct pid_set_periodic *set_periodic = &impl->joystick->pid_set_periodic;
    struct pid_set_envelope *set_envelope = &impl->joystick->pid_set_envelope;
    ULONG report_len = impl->joystick->caps.OutputReportByteLength;
    HANDLE device = impl->joystick->device;
    struct hid_value_caps *caps;
    LONG directions[4] = {0};
    DWORD i, tmp, count;
    DIEFFECT spherical;
    NTSTATUS status;
    USAGE usage;
    HRESULT hr;

    TRACE( "iface %p\n", iface );

    EnterCriticalSection( &impl->joystick->base.crit );
    if (impl->modified) hr = DI_OK;
    else hr = DI_NOEFFECT;

    if (!is_exclusively_acquired( impl->joystick ))
        hr = DIERR_NOTEXCLUSIVEACQUIRED;
    else if ((impl->flags & complete_mask) != complete_mask)
        hr = DIERR_INCOMPLETEEFFECT;
    else if (!impl->index && SUCCEEDED(hr = find_next_effect_id( impl->joystick, &impl->index, impl->type )))
    {
        if (!impl->type_specific_buf[0]) status = HIDP_STATUS_SUCCESS;
        else status = HidP_SetUsageValue( HidP_Output, HID_USAGE_PAGE_PID, 0, PID_USAGE_EFFECT_BLOCK_INDEX,
                                          impl->index, impl->joystick->preparsed, impl->type_specific_buf, report_len );
        if (status != HIDP_STATUS_SUCCESS) WARN( "HidP_SetUsageValue returned %#lx\n", status );

        if (!impl->set_envelope_buf[0]) status = HIDP_STATUS_SUCCESS;
        else status = HidP_SetUsageValue( HidP_Output, HID_USAGE_PAGE_PID, 0, PID_USAGE_EFFECT_BLOCK_INDEX,
                                          impl->index, impl->joystick->preparsed, impl->set_envelope_buf, report_len );
        if (status != HIDP_STATUS_SUCCESS) WARN( "HidP_SetUsageValue returned %#lx\n", status );

        status = HidP_SetUsageValue( HidP_Output, HID_USAGE_PAGE_PID, 0, PID_USAGE_EFFECT_BLOCK_INDEX,
                                     impl->index, impl->joystick->preparsed, impl->effect_update_buf, report_len );
        if (status != HIDP_STATUS_SUCCESS) hr = status;
        else hr = DI_OK;
    }

    if (hr == DI_OK)
    {
        switch (impl->type)
        {
        case PID_USAGE_ET_SQUARE:
        case PID_USAGE_ET_SINE:
        case PID_USAGE_ET_TRIANGLE:
        case PID_USAGE_ET_SAWTOOTH_UP:
        case PID_USAGE_ET_SAWTOOTH_DOWN:
            if (!(impl->modified & DIEP_TYPESPECIFICPARAMS)) break;

            set_parameter_value( impl, impl->type_specific_buf, set_periodic->magnitude_caps,
                                 impl->periodic.dwMagnitude );
            set_parameter_value_us( impl, impl->type_specific_buf, set_periodic->period_caps,
                                    impl->periodic.dwPeriod );
            set_parameter_value( impl, impl->type_specific_buf, set_periodic->phase_caps,
                                 impl->periodic.dwPhase );
            set_parameter_value( impl, impl->type_specific_buf, set_periodic->offset_caps,
                                 impl->periodic.lOffset );

            if (!WriteFile( device, impl->type_specific_buf, report_len, NULL, NULL )) hr = DIERR_INPUTLOST;
            else impl->modified &= ~DIEP_TYPESPECIFICPARAMS;
            break;
        case PID_USAGE_ET_SPRING:
        case PID_USAGE_ET_DAMPER:
        case PID_USAGE_ET_INERTIA:
        case PID_USAGE_ET_FRICTION:
            if (!(impl->modified & DIEP_TYPESPECIFICPARAMS)) break;

            for (i = 0; i < impl->params.cbTypeSpecificParams / sizeof(DICONDITION); ++i)
            {
                status = HidP_SetUsageValue( HidP_Output, HID_USAGE_PAGE_PID, 0, PID_USAGE_PARAMETER_BLOCK_OFFSET,
                                             i, impl->joystick->preparsed, impl->type_specific_buf, report_len );
                if (status != HIDP_STATUS_SUCCESS) WARN( "HidP_SetUsageValue %04x:%04x returned %#lx\n",
                                                         HID_USAGE_PAGE_PID, PID_USAGE_PARAMETER_BLOCK_OFFSET, status );
                set_parameter_value( impl, impl->type_specific_buf, set_condition->center_point_offset_caps,
                                     impl->condition[i].lOffset );
                set_parameter_value( impl, impl->type_specific_buf, set_condition->positive_coefficient_caps,
                                     impl->condition[i].lPositiveCoefficient );
                set_parameter_value( impl, impl->type_specific_buf, set_condition->negative_coefficient_caps,
                                     impl->condition[i].lNegativeCoefficient );
                set_parameter_value( impl, impl->type_specific_buf, set_condition->positive_saturation_caps,
                                     impl->condition[i].dwPositiveSaturation );
                set_parameter_value( impl, impl->type_specific_buf, set_condition->negative_saturation_caps,
                                     impl->condition[i].dwNegativeSaturation );
                set_parameter_value( impl, impl->type_specific_buf, set_condition->dead_band_caps,
                                     impl->condition[i].lDeadBand );

                if (!WriteFile( device, impl->type_specific_buf, report_len, NULL, NULL )) hr = DIERR_INPUTLOST;
                else impl->modified &= ~DIEP_TYPESPECIFICPARAMS;
            }
            break;
        case PID_USAGE_ET_CONSTANT_FORCE:
            if (!(impl->modified & DIEP_TYPESPECIFICPARAMS)) break;

            set_parameter_value( impl, impl->type_specific_buf, set_constant_force->magnitude_caps,
                                 impl->constant_force.lMagnitude );

            if (!WriteFile( device, impl->type_specific_buf, report_len, NULL, NULL )) hr = DIERR_INPUTLOST;
            else impl->modified &= ~DIEP_TYPESPECIFICPARAMS;
            break;
        case PID_USAGE_ET_RAMP:
            if (!(impl->modified & DIEP_TYPESPECIFICPARAMS)) break;

            set_parameter_value( impl, impl->type_specific_buf, set_ramp_force->start_caps,
                                 impl->ramp_force.lStart );
            set_parameter_value( impl, impl->type_specific_buf, set_ramp_force->end_caps,
                                 impl->ramp_force.lEnd );

            if (!WriteFile( device, impl->type_specific_buf, report_len, NULL, NULL )) hr = DIERR_INPUTLOST;
            else impl->modified &= ~DIEP_TYPESPECIFICPARAMS;
            break;
        }
    }

    if (hr == DI_OK)
    {
        switch (impl->type)
        {
        case PID_USAGE_ET_SQUARE:
        case PID_USAGE_ET_SINE:
        case PID_USAGE_ET_TRIANGLE:
        case PID_USAGE_ET_SAWTOOTH_UP:
        case PID_USAGE_ET_SAWTOOTH_DOWN:
        case PID_USAGE_ET_CONSTANT_FORCE:
        case PID_USAGE_ET_RAMP:
            if (!(impl->modified & DIEP_ENVELOPE)) break;

            set_parameter_value( impl, impl->set_envelope_buf, set_envelope->attack_level_caps,
                                 impl->envelope.dwAttackLevel );
            set_parameter_value_us( impl, impl->set_envelope_buf, set_envelope->attack_time_caps,
                                    impl->envelope.dwAttackTime );
            set_parameter_value( impl, impl->set_envelope_buf, set_envelope->fade_level_caps,
                                 impl->envelope.dwFadeLevel );
            set_parameter_value_us( impl, impl->set_envelope_buf, set_envelope->fade_time_caps,
                                    impl->envelope.dwFadeTime );

            if (!WriteFile( device, impl->set_envelope_buf, report_len, NULL, NULL )) hr = DIERR_INPUTLOST;
            else impl->modified &= ~DIEP_ENVELOPE;
            break;
        }
    }

    if (hr == DI_OK && impl->modified)
    {
        set_parameter_value_us( impl, impl->effect_update_buf, effect_update->duration_caps,
                                impl->params.dwDuration );
        set_parameter_value( impl, impl->effect_update_buf, effect_update->gain_caps,
                             impl->params.dwGain );
        set_parameter_value_us( impl, impl->effect_update_buf, effect_update->sample_period_caps,
                                impl->params.dwSamplePeriod );
        set_parameter_value_us( impl, impl->effect_update_buf, effect_update->start_delay_caps,
                                impl->params.dwStartDelay );
        set_parameter_value_us( impl, impl->effect_update_buf, effect_update->trigger_repeat_interval_caps,
                                impl->params.dwTriggerRepeatInterval );

        count = 1;
        usage = PID_USAGE_DIRECTION_ENABLE;
        status = HidP_SetUsages( HidP_Output, HID_USAGE_PAGE_PID, 0, &usage, &count,
                                 impl->joystick->preparsed, impl->effect_update_buf, report_len );
        if (status != HIDP_STATUS_SUCCESS) WARN( "HidP_SetUsages returned %#lx\n", status );

        spherical.rglDirection = directions;
        convert_directions_to_spherical( &impl->params, &spherical );

        /* FIXME: as far as the test cases go, directions are only written if
         * either X or Y axes are enabled, maybe need more tests though */
        if (!is_axis_usage_enabled( impl, HID_USAGE_GENERIC_X ) &&
            !is_axis_usage_enabled( impl, HID_USAGE_GENERIC_Y ))
            WARN( "neither X or Y axes are selected, skipping direction\n" );
        else for (i = 0; i < min( effect_update->direction_count, spherical.cAxes ); ++i)
        {
            tmp = directions[i] + (i == 0 ? 9000 : 0);
            caps = effect_update->direction_caps[effect_update->direction_count - i - 1];
            set_parameter_value_angle( impl, impl->effect_update_buf, caps, tmp % 36000 );
        }

        status = HidP_SetUsageValue( HidP_Output, HID_USAGE_PAGE_PID, 0, PID_USAGE_TRIGGER_BUTTON,
                                     impl->params.dwTriggerButton, impl->joystick->preparsed,
                                     impl->effect_update_buf, report_len );
        if (status != HIDP_STATUS_SUCCESS) WARN( "HidP_SetUsageValue returned %#lx\n", status );

        if (!WriteFile( device, impl->effect_update_buf, report_len, NULL, NULL )) hr = DIERR_INPUTLOST;
        else impl->modified = 0;

        if (SUCCEEDED(hr)) impl->joystick->base.force_feedback_state &= ~DIGFFS_EMPTY;
    }
    LeaveCriticalSection( &impl->joystick->base.crit );

    return hr;
}

static void check_empty_force_feedback_state( struct hid_joystick *joystick )
{
    struct hid_joystick_effect *effect;
    LIST_FOR_EACH_ENTRY( effect, &joystick->effect_list, struct hid_joystick_effect, entry )
        if (effect->index) return;
    joystick->base.force_feedback_state |= DIGFFS_EMPTY;
}

static HRESULT WINAPI hid_joystick_effect_Unload( IDirectInputEffect *iface )
{
    struct hid_joystick_effect *impl = impl_from_IDirectInputEffect( iface );
    struct hid_joystick *joystick = impl->joystick;
    struct pid_device_pool *device_pool = &joystick->pid_device_pool;
    struct pid_block_free *block_free = &joystick->pid_block_free;
    ULONG report_len = joystick->caps.OutputReportByteLength;
    HRESULT hr = DI_OK;
    NTSTATUS status;

    TRACE( "iface %p\n", iface );

    EnterCriticalSection( &joystick->base.crit );
    if (!impl->index)
        hr = DI_NOEFFECT;
    else if (SUCCEEDED(hr = IDirectInputEffect_Stop( iface )))
    {
        if (!device_pool->device_managed_caps)
            joystick->effect_inuse[impl->index - 1] = FALSE;
        else if (block_free->id)
        {
            status = HidP_InitializeReportForID( HidP_Output, block_free->id, joystick->preparsed,
                                                 joystick->output_report_buf, report_len );

            if (status != HIDP_STATUS_SUCCESS) hr = status;
            else status = HidP_SetUsageValue( HidP_Output, HID_USAGE_PAGE_PID, 0, PID_USAGE_EFFECT_BLOCK_INDEX,
                                              impl->index, joystick->preparsed, joystick->output_report_buf, report_len );

            if (status != HIDP_STATUS_SUCCESS) hr = status;
            else if (WriteFile( joystick->device, joystick->output_report_buf, report_len, NULL, NULL )) hr = DI_OK;
            else hr = DIERR_INPUTLOST;
        }

        impl->modified = impl->flags;
        impl->index = 0;
        check_empty_force_feedback_state( joystick );
    }
    LeaveCriticalSection( &joystick->base.crit );

    return hr;
}

static HRESULT WINAPI hid_joystick_effect_Escape( IDirectInputEffect *iface, DIEFFESCAPE *escape )
{
    FIXME( "iface %p, escape %p stub!\n", iface, escape );
    return DIERR_UNSUPPORTED;
}

static IDirectInputEffectVtbl hid_joystick_effect_vtbl =
{
    /*** IUnknown methods ***/
    hid_joystick_effect_QueryInterface,
    hid_joystick_effect_AddRef,
    hid_joystick_effect_Release,
    /*** IDirectInputEffect methods ***/
    hid_joystick_effect_Initialize,
    hid_joystick_effect_GetEffectGuid,
    hid_joystick_effect_GetParameters,
    hid_joystick_effect_SetParameters,
    hid_joystick_effect_Start,
    hid_joystick_effect_Stop,
    hid_joystick_effect_GetEffectStatus,
    hid_joystick_effect_Download,
    hid_joystick_effect_Unload,
    hid_joystick_effect_Escape,
};

static HRESULT hid_joystick_create_effect( IDirectInputDevice8W *iface, IDirectInputEffect **out )
{
    struct hid_joystick *joystick = impl_from_IDirectInputDevice8W( iface );
    struct hid_joystick_effect *impl;
    ULONG report_len;

    if (!(impl = calloc( 1, sizeof(*impl) ))) return DIERR_OUTOFMEMORY;
    impl->IDirectInputEffect_iface.lpVtbl = &hid_joystick_effect_vtbl;
    impl->ref = 1;
    impl->joystick = joystick;
    hid_joystick_addref( &joystick->base.IDirectInputDevice8W_iface );

    EnterCriticalSection( &joystick->base.crit );
    list_add_tail( &joystick->effect_list, &impl->entry );
    LeaveCriticalSection( &joystick->base.crit );

    report_len = joystick->caps.OutputReportByteLength;
    if (!(impl->effect_control_buf = malloc( report_len ))) goto failed;
    if (!(impl->effect_update_buf = malloc( report_len ))) goto failed;
    if (!(impl->type_specific_buf = malloc( report_len ))) goto failed;
    if (!(impl->set_envelope_buf = malloc( report_len ))) goto failed;

    impl->envelope.dwSize = sizeof(DIENVELOPE);
    impl->params.dwSize = sizeof(DIEFFECT);
    impl->params.rgdwAxes = impl->axes;
    impl->params.rglDirection = impl->directions;
    impl->params.dwTriggerButton = -1;
    impl->status = 0;

    *out = &impl->IDirectInputEffect_iface;
    return DI_OK;

failed:
    IDirectInputEffect_Release( &impl->IDirectInputEffect_iface );
    return DIERR_OUTOFMEMORY;
}
