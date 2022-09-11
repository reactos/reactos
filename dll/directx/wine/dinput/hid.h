/*
 * Wine internal HID structures
 *
 * Copyright 2015 Aric Stewart
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

#ifndef __WINE_PARSE_H
#define __WINE_PARSE_H

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "hidusage.h"
#include "ddk/hidpi.h"

#define HID_MAGIC 0x8491759

struct hid_collection_node
{
    USAGE usage;
    USAGE usage_page;
    USHORT parent;
    USHORT number_of_children;
    USHORT next_sibling;
    USHORT first_child;
    ULONG collection_type;
};

struct hid_value_caps
{
    USHORT usage_page;
    UCHAR report_id;
    UCHAR start_bit;
    USHORT bit_size;
    USHORT report_count;
    USHORT start_byte;
    USHORT total_bits;
    ULONG bit_field;
    USHORT end_byte;
    USHORT link_collection;
    USAGE link_usage_page;
    USAGE link_usage;
    ULONG flags;
    ULONG padding[8];
    USAGE usage_min;
    USAGE usage_max;
    USHORT string_min;
    USHORT string_max;
    USHORT designator_min;
    USHORT designator_max;
    USHORT data_index_min;
    USHORT data_index_max;
    USHORT null_value;
    USHORT unknown;
    LONG logical_min;
    LONG logical_max;
    LONG physical_min;
    LONG physical_max;
    LONG units;
    LONG units_exp;
};

/* named array continues on next caps */
#define HID_VALUE_CAPS_ARRAY_HAS_MORE       0x01
#define HID_VALUE_CAPS_IS_CONSTANT          0x02
#define HID_VALUE_CAPS_IS_BUTTON            0x04
#define HID_VALUE_CAPS_IS_ABSOLUTE          0x08
#define HID_VALUE_CAPS_IS_RANGE             0x10
#define HID_VALUE_CAPS_IS_STRING_RANGE      0x40
#define HID_VALUE_CAPS_IS_DESIGNATOR_RANGE  0x80

#define HID_VALUE_CAPS_HAS_NULL(x) (((x)->bit_field & 0x40) != 0)
#define HID_VALUE_CAPS_IS_ARRAY(c) (((c)->bit_field & 2) == 0)

struct hid_preparsed_data
{
    char magic[8];
    USAGE usage;
    USAGE usage_page;
    USHORT unknown[2];
    USHORT input_caps_start;
    USHORT input_caps_count;
    USHORT input_caps_end;
    USHORT input_report_byte_length;
    USHORT output_caps_start;
    USHORT output_caps_count;
    USHORT output_caps_end;
    USHORT output_report_byte_length;
    USHORT feature_caps_start;
    USHORT feature_caps_count;
    USHORT feature_caps_end;
    USHORT feature_report_byte_length;
    USHORT caps_size;
    USHORT number_link_collection_nodes;
    struct hid_value_caps value_caps[1];
    /* struct hid_collection_node nodes[1] */
};

#define HID_INPUT_VALUE_CAPS(d) ((d)->value_caps + (d)->input_caps_start)
#define HID_OUTPUT_VALUE_CAPS(d) ((d)->value_caps + (d)->output_caps_start)
#define HID_FEATURE_VALUE_CAPS(d) ((d)->value_caps + (d)->feature_caps_start)
#define HID_COLLECTION_NODES(d) (struct hid_collection_node *)((char *)(d)->value_caps + (d)->caps_size)

/* Wine-specific Physical Interface Device usages */
/* From USB Device Class Definition for Physical Interface Devices */
/* https://www.usb.org/sites/default/files/documents/pid1_01.pdf */
#define PID_USAGE_UNDEFINED                        ((USAGE) 0x00)
#define PID_USAGE_PID                              ((USAGE) 0x01)
#define PID_USAGE_NORMAL                           ((USAGE) 0x20)
#define PID_USAGE_SET_EFFECT_REPORT                ((USAGE) 0x21)
#define PID_USAGE_EFFECT_BLOCK_INDEX               ((USAGE) 0x22)
#define PID_USAGE_PARAMETER_BLOCK_OFFSET           ((USAGE) 0x23)
#define PID_USAGE_ROM_FLAG                         ((USAGE) 0x24)
#define PID_USAGE_EFFECT_TYPE                      ((USAGE) 0x25)
#define PID_USAGE_ET_CONSTANT_FORCE                ((USAGE) 0x26)
#define PID_USAGE_ET_RAMP                          ((USAGE) 0x27)
#define PID_USAGE_ET_CUSTOM_FORCE_DATA             ((USAGE) 0x28)
#define PID_USAGE_ET_SQUARE                        ((USAGE) 0x30)
#define PID_USAGE_ET_SINE                          ((USAGE) 0x31)
#define PID_USAGE_ET_TRIANGLE                      ((USAGE) 0x32)
#define PID_USAGE_ET_SAWTOOTH_UP                   ((USAGE) 0x33)
#define PID_USAGE_ET_SAWTOOTH_DOWN                 ((USAGE) 0x34)
#define PID_USAGE_ET_SPRING                        ((USAGE) 0x40)
#define PID_USAGE_ET_DAMPER                        ((USAGE) 0x41)
#define PID_USAGE_ET_INERTIA                       ((USAGE) 0x42)
#define PID_USAGE_ET_FRICTION                      ((USAGE) 0x43)
#define PID_USAGE_DURATION                         ((USAGE) 0x50)
#define PID_USAGE_SAMPLE_PERIOD                    ((USAGE) 0x51)
#define PID_USAGE_GAIN                             ((USAGE) 0x52)
#define PID_USAGE_TRIGGER_BUTTON                   ((USAGE) 0x53)
#define PID_USAGE_TRIGGER_REPEAT_INTERVAL          ((USAGE) 0x54)
#define PID_USAGE_AXES_ENABLE                      ((USAGE) 0x55)
#define PID_USAGE_DIRECTION_ENABLE                 ((USAGE) 0x56)
#define PID_USAGE_DIRECTION                        ((USAGE) 0x57)
#define PID_USAGE_TYPE_SPECIFIC_BLOCK_OFFSET       ((USAGE) 0x58)
#define PID_USAGE_BLOCK_TYPE                       ((USAGE) 0x59)
#define PID_USAGE_SET_ENVELOPE_REPORT              ((USAGE) 0x5a)
#define PID_USAGE_ATTACK_LEVEL                     ((USAGE) 0x5b)
#define PID_USAGE_ATTACK_TIME                      ((USAGE) 0x5c)
#define PID_USAGE_FADE_LEVEL                       ((USAGE) 0x5d)
#define PID_USAGE_FADE_TIME                        ((USAGE) 0x5e)
#define PID_USAGE_SET_CONDITION_REPORT             ((USAGE) 0x5f)
#define PID_USAGE_CP_OFFSET                        ((USAGE) 0x60)
#define PID_USAGE_POSITIVE_COEFFICIENT             ((USAGE) 0x61)
#define PID_USAGE_NEGATIVE_COEFFICIENT             ((USAGE) 0x62)
#define PID_USAGE_POSITIVE_SATURATION              ((USAGE) 0x63)
#define PID_USAGE_NEGATIVE_SATURATION              ((USAGE) 0x64)
#define PID_USAGE_DEAD_BAND                        ((USAGE) 0x65)
#define PID_USAGE_DOWNLOAD_FORCE_SAMPLE            ((USAGE) 0x66)
#define PID_USAGE_ISOCH_CUSTOM_FORCE_ENABLE        ((USAGE) 0x67)
#define PID_USAGE_CUSTOM_FORCE_DATA_REPORT         ((USAGE) 0x68)
#define PID_USAGE_CUSTOM_FORCE_DATA                ((USAGE) 0x69)
#define PID_USAGE_CUSTOM_FORCE_VENDOR_DEFINED_DATA ((USAGE) 0x6a)
#define PID_USAGE_SET_CUSTOM_FORCE_REPORT          ((USAGE) 0x6b)
#define PID_USAGE_CUSTOM_FORCE_DATA_OFFSET         ((USAGE) 0x6c)
#define PID_USAGE_SAMPLE_COUNT                     ((USAGE) 0x6d)
#define PID_USAGE_SET_PERIODIC_REPORT              ((USAGE) 0x6e)
#define PID_USAGE_OFFSET                           ((USAGE) 0x6f)
#define PID_USAGE_MAGNITUDE                        ((USAGE) 0x70)
#define PID_USAGE_PHASE                            ((USAGE) 0x71)
#define PID_USAGE_PERIOD                           ((USAGE) 0x72)
#define PID_USAGE_SET_CONSTANT_FORCE_REPORT        ((USAGE) 0x73)
#define PID_USAGE_SET_RAMP_FORCE_REPORT            ((USAGE) 0x74)
#define PID_USAGE_RAMP_START                       ((USAGE) 0x75)
#define PID_USAGE_RAMP_END                         ((USAGE) 0x76)
#define PID_USAGE_EFFECT_OPERATION_REPORT          ((USAGE) 0x77)
#define PID_USAGE_EFFECT_OPERATION                 ((USAGE) 0x78)
#define PID_USAGE_OP_EFFECT_START                  ((USAGE) 0x79)
#define PID_USAGE_OP_EFFECT_START_SOLO             ((USAGE) 0x7a)
#define PID_USAGE_OP_EFFECT_STOP                   ((USAGE) 0x7b)
#define PID_USAGE_LOOP_COUNT                       ((USAGE) 0x7c)
#define PID_USAGE_DEVICE_GAIN_REPORT               ((USAGE) 0x7d)
#define PID_USAGE_DEVICE_GAIN                      ((USAGE) 0x7e)
#define PID_USAGE_POOL_REPORT                      ((USAGE) 0x7f)
#define PID_USAGE_RAM_POOL_SIZE                    ((USAGE) 0x80)
#define PID_USAGE_ROM_POOL_SIZE                    ((USAGE) 0x81)
#define PID_USAGE_ROM_EFFECT_BLOCK_COUNT           ((USAGE) 0x82)
#define PID_USAGE_SIMULTANEOUS_EFFECTS_MAX         ((USAGE) 0x83)
#define PID_USAGE_POOL_ALIGNMENT                   ((USAGE) 0x84)
#define PID_USAGE_POOL_MOVE_REPORT                 ((USAGE) 0x85)
#define PID_USAGE_MOVE_SOURCE                      ((USAGE) 0x86)
#define PID_USAGE_MOVE_DESTINATION                 ((USAGE) 0x87)
#define PID_USAGE_MOVE_LENGTH                      ((USAGE) 0x88)
#define PID_USAGE_BLOCK_LOAD_REPORT                ((USAGE) 0x89)
#define PID_USAGE_BLOCK_LOAD_STATUS                ((USAGE) 0x8b)
#define PID_USAGE_BLOCK_LOAD_SUCCESS               ((USAGE) 0x8c)
#define PID_USAGE_BLOCK_LOAD_FULL                  ((USAGE) 0x8d)
#define PID_USAGE_BLOCK_LOAD_ERROR                 ((USAGE) 0x8e)
#define PID_USAGE_BLOCK_HANDLE                     ((USAGE) 0x8f)
#define PID_USAGE_BLOCK_FREE_REPORT                ((USAGE) 0x90)
#define PID_USAGE_TYPE_SPECIFIC_BLOCK_HANDLE       ((USAGE) 0x91)
#define PID_USAGE_STATE_REPORT                     ((USAGE) 0x92)
#define PID_USAGE_EFFECT_PLAYING                   ((USAGE) 0x94)
#define PID_USAGE_DEVICE_CONTROL_REPORT            ((USAGE) 0x95)
#define PID_USAGE_DEVICE_CONTROL                   ((USAGE) 0x96)
#define PID_USAGE_DC_ENABLE_ACTUATORS              ((USAGE) 0x97)
#define PID_USAGE_DC_DISABLE_ACTUATORS             ((USAGE) 0x98)
#define PID_USAGE_DC_STOP_ALL_EFFECTS              ((USAGE) 0x99)
#define PID_USAGE_DC_DEVICE_RESET                  ((USAGE) 0x9a)
#define PID_USAGE_DC_DEVICE_PAUSE                  ((USAGE) 0x9b)
#define PID_USAGE_DC_DEVICE_CONTINUE               ((USAGE) 0x9c)
#define PID_USAGE_DEVICE_PAUSED                    ((USAGE) 0x9f)
#define PID_USAGE_ACTUATORS_ENABLED                ((USAGE) 0xa0)
#define PID_USAGE_SAFETY_SWITCH                    ((USAGE) 0xa4)
#define PID_USAGE_ACTUATOR_OVERRIDE_SWITCH         ((USAGE) 0xa5)
#define PID_USAGE_ACTUATOR_POWER                   ((USAGE) 0xa6)
#define PID_USAGE_START_DELAY                      ((USAGE) 0xa7)
#define PID_USAGE_PARAMETER_BLOCK_SIZE             ((USAGE) 0xa8)
#define PID_USAGE_DEVICE_MANAGED_POOL              ((USAGE) 0xa9)
#define PID_USAGE_SHARED_PARAMETER_BLOCKS          ((USAGE) 0xaa)
#define PID_USAGE_CREATE_NEW_EFFECT_REPORT         ((USAGE) 0xab)
#define PID_USAGE_RAM_POOL_AVAILABLE               ((USAGE) 0xac)

#endif /* __WINE_PARSE_H */
