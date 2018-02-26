/*
 * Copyright (c) 2008-2009 Christopher Fitzgerald
 * Copyright (c) 2015 Mark Harmstone
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

/* Taken in large part from OpenAL. */

#ifndef DSOUND_EAX_H_DEFINED
#define DSOUND_EAX_H_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

DEFINE_GUID(DSPROPSETID_EAX_ReverbProperties, 0x4a4e6fc1, 0xc341, 0x11d1, 0xb7, 0x3a, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
DEFINE_GUID(DSPROPSETID_EAXBUFFER_ReverbProperties, 0x4a4e6fc0, 0xc341, 0x11d1, 0xb7, 0x3a, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
DEFINE_GUID(DSPROPSETID_EAX20_ListenerProperties, 0x306a6a8, 0xb224, 0x11d2, 0x99, 0xe5, 0x0, 0x0, 0xe8, 0xd8, 0xc7, 0x22);
DEFINE_GUID(DSPROPSETID_EAX20_BufferProperties, 0x306a6a7, 0xb224, 0x11d2, 0x99, 0xe5, 0x0, 0x0, 0xe8, 0xd8, 0xc7, 0x22);

typedef enum {
    DSPROPERTY_EAX_ALL,
    DSPROPERTY_EAX_ENVIRONMENT,
    DSPROPERTY_EAX_VOLUME,
    DSPROPERTY_EAX_DECAYTIME,
    DSPROPERTY_EAX_DAMPING
} DSPROPERTY_EAX_REVERBPROPERTY;

typedef struct {
    unsigned long environment;
    float fVolume;
    float fDecayTime_sec;
    float fDamping;
} EAX_REVERBPROPERTIES;

enum {
    EAX_ENVIRONMENT_GENERIC,
    EAX_ENVIRONMENT_PADDEDCELL,
    EAX_ENVIRONMENT_ROOM,
    EAX_ENVIRONMENT_BATHROOM,
    EAX_ENVIRONMENT_LIVINGROOM,
    EAX_ENVIRONMENT_STONEROOM,
    EAX_ENVIRONMENT_AUDITORIUM,
    EAX_ENVIRONMENT_CONCERTHALL,
    EAX_ENVIRONMENT_CAVE,
    EAX_ENVIRONMENT_ARENA,
    EAX_ENVIRONMENT_HANGAR,
    EAX_ENVIRONMENT_CARPETEDHALLWAY,
    EAX_ENVIRONMENT_HALLWAY,
    EAX_ENVIRONMENT_STONECORRIDOR,
    EAX_ENVIRONMENT_ALLEY,
    EAX_ENVIRONMENT_FOREST,
    EAX_ENVIRONMENT_CITY,
    EAX_ENVIRONMENT_MOUNTAINS,
    EAX_ENVIRONMENT_QUARRY,
    EAX_ENVIRONMENT_PLAIN,
    EAX_ENVIRONMENT_PARKINGLOT,
    EAX_ENVIRONMENT_SEWERPIPE,
    EAX_ENVIRONMENT_UNDERWATER,
    EAX_ENVIRONMENT_DRUGGED,
    EAX_ENVIRONMENT_DIZZY,
    EAX_ENVIRONMENT_PSYCHOTIC,
    EAX_ENVIRONMENT_COUNT
};

typedef enum {
    DSPROPERTY_EAXBUFFER_ALL,
    DSPROPERTY_EAXBUFFER_REVERBMIX
} DSPROPERTY_EAXBUFFER_REVERBPROPERTY;

typedef struct {
    float fMix;
} EAXBUFFER_REVERBPROPERTIES;

typedef enum
{
    DSPROPERTY_EAX20LISTENER_NONE,
    DSPROPERTY_EAX20LISTENER_ALLPARAMETERS,
    DSPROPERTY_EAX20LISTENER_ROOM,
    DSPROPERTY_EAX20LISTENER_ROOMHF,
    DSPROPERTY_EAX20LISTENER_ROOMROLLOFFFACTOR,
    DSPROPERTY_EAX20LISTENER_DECAYTIME,
    DSPROPERTY_EAX20LISTENER_DECAYHFRATIO,
    DSPROPERTY_EAX20LISTENER_REFLECTIONS,
    DSPROPERTY_EAX20LISTENER_REFLECTIONSDELAY,
    DSPROPERTY_EAX20LISTENER_REVERB,
    DSPROPERTY_EAX20LISTENER_REVERBDELAY,
    DSPROPERTY_EAX20LISTENER_ENVIRONMENT,
    DSPROPERTY_EAX20LISTENER_ENVIRONMENTSIZE,
    DSPROPERTY_EAX20LISTENER_ENVIRONMENTDIFFUSION,
    DSPROPERTY_EAX20LISTENER_AIRABSORPTIONHF,
    DSPROPERTY_EAX20LISTENER_FLAGS
} DSPROPERTY_EAX20_LISTENERPROPERTY;

typedef enum
{
    DSPROPERTY_EAX20BUFFER_NONE,
    DSPROPERTY_EAX20BUFFER_ALLPARAMETERS,
    DSPROPERTY_EAX20BUFFER_DIRECT,
    DSPROPERTY_EAX20BUFFER_DIRECTHF,
    DSPROPERTY_EAX20BUFFER_ROOM,
    DSPROPERTY_EAX20BUFFER_ROOMHF,
    DSPROPERTY_EAX20BUFFER_ROOMROLLOFFFACTOR,
    DSPROPERTY_EAX20BUFFER_OBSTRUCTION,
    DSPROPERTY_EAX20BUFFER_OBSTRUCTIONLFRATIO,
    DSPROPERTY_EAX20BUFFER_OCCLUSION,
    DSPROPERTY_EAX20BUFFER_OCCLUSIONLFRATIO,
    DSPROPERTY_EAX20BUFFER_OCCLUSIONROOMRATIO,
    DSPROPERTY_EAX20BUFFER_OUTSIDEVOLUMEHF,
    DSPROPERTY_EAX20BUFFER_AIRABSORPTIONFACTOR,
    DSPROPERTY_EAX20BUFFER_FLAGS
} DSPROPERTY_EAX20_BUFFERPROPERTY;

#define EAX_REVERBMIX_USEDISTANCE -1.0f

#define AL_EAXREVERB_MAX_REFLECTIONS_DELAY       (0.3f)
#define AL_EAXREVERB_MAX_LATE_REVERB_DELAY       (0.1f)

typedef struct {
    float flDensity;
    float flDiffusion;
    float flGain;
    float flGainHF;
    float flGainLF;
    float flDecayTime;
    float flDecayHFRatio;
    float flDecayLFRatio;
    float flReflectionsGain;
    float flReflectionsDelay;
    float flReflectionsPan[3];
    float flLateReverbGain;
    float flLateReverbDelay;
    float flLateReverbPan[3];
    float flEchoTime;
    float flEchoDepth;
    float flModulationTime;
    float flModulationDepth;
    float flAirAbsorptionGainHF;
    float flHFReference;
    float flLFReference;
    float flRoomRolloffFactor;
    int   iDecayHFLimit;
} EFXEAXREVERBPROPERTIES, *LPEFXEAXREVERBPROPERTIES;

typedef struct DelayLine
{
    unsigned int Mask;
    float *Line;
} DelayLine;

typedef struct {
    float coeff;
    float history[2];
} FILTER;

typedef struct {
    BOOL using_eax;
    unsigned long environment;
    float volume;
    float damping;
    EFXEAXREVERBPROPERTIES eax_props;
} eax_info;

typedef struct {
    BOOL reverb_update;
    float reverb_mix;

    float *SampleBuffer;
    unsigned int TotalSamples;

    FILTER LpFilter;

    DelayLine Delay;
    unsigned int DelayTap[2];

    struct {
        float Gain;
        float Coeff[4];
        DelayLine Delay[4];
        unsigned int Offset[4];
    } Early;

    DelayLine Decorrelator;
    unsigned int DecoTap[3];

    struct {
        float Gain;
        float DensityGain;
        float ApFeedCoeff;
        float MixCoeff;

        float ApCoeff[4];
        DelayLine ApDelay[4];
        unsigned int ApOffset[4];

        float Coeff[4];
        DelayLine Delay[4];
        unsigned int Offset[4];

        float LpCoeff[4];
        float LpSample[4];
    } Late;

    unsigned int Offset;
} eax_buffer_info;

#ifdef __cplusplus
}
#endif

#endif
