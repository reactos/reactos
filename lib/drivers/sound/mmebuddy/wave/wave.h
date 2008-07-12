/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/wave/wave.h
 *
 * PURPOSE:     Provides prototypes and structures relevant to the internal
 *              implementation of the MME wave device support functions.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#ifndef ROS_MMEBUDDY_WAVE_H
#define ROS_MMEBUDDY_WAVE_H

MMRESULT
QueueBuffer_Request(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID Parameter);

#endif
