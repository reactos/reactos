/*              DirectInput Linux Event Device Effect
 *
 * Copyright 2005 Daniel Remenak
 *
 * Thanks to Google's Summer of Code Program (2005)
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

#ifdef HAVE_STRUCT_FF_EFFECT_DIRECTION

#include <stdarg.h>
#include <string.h>
#ifdef HAVE_LINUX_INPUT_H
#  include <linux/input.h>
#  undef SW_MAX
#endif
#include <errno.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include <math.h>
#include "wine/debug.h"
#include "wine/unicode.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "dinput.h"

#include "device_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

static const IDirectInputEffectVtbl LinuxInputEffectVtbl;
typedef struct LinuxInputEffectImpl LinuxInputEffectImpl;
struct LinuxInputEffectImpl
{
    const void *lpVtbl;
    LONG	ref;
    GUID	guid;

    struct ff_effect    effect; /* Effect data */
    int                 gain;   /* Effect gain */
    int                 first_axis_is_x;
    int*                fd;     /* Parent device */
    struct list        *entry;  /* Entry into the parent's list of effects */
};


/******************************************************************************
 *      DirectInputEffect Functional Helper
 */

static DWORD _typeFromGUID(REFGUID guid)
{
    if (IsEqualGUID(guid, &GUID_ConstantForce)) {
	return DIEFT_CONSTANTFORCE;
    } else if (IsEqualGUID(guid, &GUID_Square)
            || IsEqualGUID(guid, &GUID_Sine)
            || IsEqualGUID(guid, &GUID_Triangle)
            || IsEqualGUID(guid, &GUID_SawtoothUp)
            || IsEqualGUID(guid, &GUID_SawtoothDown)) {
	return DIEFT_PERIODIC;
    } else if (IsEqualGUID(guid, &GUID_RampForce)) {
	return DIEFT_RAMPFORCE;
    } else if (IsEqualGUID(guid, &GUID_Spring)
            || IsEqualGUID(guid, &GUID_Damper)
            || IsEqualGUID(guid, &GUID_Inertia)
            || IsEqualGUID(guid, &GUID_Friction)) {
	return DIEFT_CONDITION;
    } else if (IsEqualGUID(guid, &GUID_CustomForce)) {
	return DIEFT_CUSTOMFORCE;
    } else {
        WARN("GUID (%s) is not a known force type\n", _dump_dinput_GUID(guid));
	return 0;
    }
}


/******************************************************************************
 *      DirectInputEffect debug helpers 
 */

static void _dump_DIEFFECT_flags(DWORD dwFlags)
{
    if (TRACE_ON(dinput)) {
        unsigned int   i;
        static const struct {
            DWORD       mask;
            const char  *name;
        } flags[] = {
#define FE(x) { x, #x}
            FE(DIEFF_CARTESIAN),
            FE(DIEFF_OBJECTIDS),
            FE(DIEFF_OBJECTOFFSETS),
            FE(DIEFF_POLAR),
            FE(DIEFF_SPHERICAL)
#undef FE
        };
        for (i = 0; i < (sizeof(flags) / sizeof(flags[0])); i++)
            if (flags[i].mask & dwFlags)
                TRACE("%s ", flags[i].name);
        TRACE("\n");
    }       
}

static void _dump_DIENVELOPE(LPCDIENVELOPE env)
{
    if (env->dwSize != sizeof(DIENVELOPE)) {
        WARN("Non-standard DIENVELOPE structure size %d.\n", env->dwSize);
    }
    TRACE("Envelope has attack (level: %d time: %d), fade (level: %d time: %d)\n",
	  env->dwAttackLevel, env->dwAttackTime, env->dwFadeLevel, env->dwFadeTime);
} 

static void _dump_DICONSTANTFORCE(LPCDICONSTANTFORCE frc)
{
    TRACE("Constant force has magnitude %d\n", frc->lMagnitude);
}

static void _dump_DIPERIODIC(LPCDIPERIODIC frc)
{
    TRACE("Periodic force has magnitude %d, offset %d, phase %d, period %d\n",
	  frc->dwMagnitude, frc->lOffset, frc->dwPhase, frc->dwPeriod);
}

static void _dump_DIRAMPFORCE(LPCDIRAMPFORCE frc)
{
    TRACE("Ramp force has start %d, end %d\n",
	  frc->lStart, frc->lEnd);
}

static void _dump_DICONDITION(LPCDICONDITION frc)
{
    TRACE("Condition has offset %d, pos/neg coefficients %d and %d, pos/neg saturations %d and %d, deadband %d\n",
	  frc->lOffset, frc->lPositiveCoefficient, frc->lNegativeCoefficient,
	  frc->dwPositiveSaturation, frc->dwNegativeSaturation, frc->lDeadBand);
}

static void _dump_DICUSTOMFORCE(LPCDICUSTOMFORCE frc)
{
    unsigned int i;
    TRACE("Custom force uses %d channels, sample period %d.  Has %d samples at %p.\n",
	  frc->cChannels, frc->dwSamplePeriod, frc->cSamples, frc->rglForceData);
    if (frc->cSamples % frc->cChannels != 0)
	WARN("Custom force has a non-integral samples-per-channel count!\n");
    if (TRACE_ON(dinput)) {
	TRACE("Custom force data (time aligned, axes in order):\n");
	for (i = 1; i <= frc->cSamples; ++i) {
	    TRACE("%d ", frc->rglForceData[i]);
	    if (i % frc->cChannels == 0)
		TRACE("\n");
	}	
    }
}

static void _dump_DIEFFECT(LPCDIEFFECT eff, REFGUID guid)
{
    unsigned int i;
    DWORD type = _typeFromGUID(guid);

    TRACE("Dumping DIEFFECT structure:\n");
    TRACE("  - dwSize: %d\n", eff->dwSize);
    if ((eff->dwSize != sizeof(DIEFFECT)) && (eff->dwSize != sizeof(DIEFFECT_DX5))) {
        WARN("Non-standard DIEFFECT structure size %d\n", eff->dwSize);
    }
    TRACE("  - dwFlags: %d\n", eff->dwFlags);
    TRACE("    ");
    _dump_DIEFFECT_flags(eff->dwFlags); 
    TRACE("  - dwDuration: %d\n", eff->dwDuration);
    TRACE("  - dwGain: %d\n", eff->dwGain);
    if (eff->dwGain > 10000)
	WARN("dwGain is out of range (>10,000)\n");
    TRACE("  - dwTriggerButton: %d\n", eff->dwTriggerButton);
    TRACE("  - dwTriggerRepeatInterval: %d\n", eff->dwTriggerRepeatInterval);
    TRACE("  - cAxes: %d\n", eff->cAxes);
    TRACE("  - rgdwAxes: %p\n", eff->rgdwAxes);
    if (TRACE_ON(dinput) && eff->rgdwAxes) {
	TRACE("    ");	
	for (i = 0; i < eff->cAxes; ++i)
	    TRACE("%d ", eff->rgdwAxes[i]);
	TRACE("\n");
    }
    TRACE("  - rglDirection: %p\n", eff->rglDirection);
    TRACE("  - lpEnvelope: %p\n", eff->lpEnvelope);
    TRACE("  - cbTypeSpecificParams: %d\n", eff->cbTypeSpecificParams);
    TRACE("  - lpvTypeSpecificParams: %p\n", eff->lpvTypeSpecificParams);
    if (eff->dwSize > sizeof(DIEFFECT_DX5))
    	TRACE("  - dwStartDelay: %d\n", eff->dwStartDelay);
    if (eff->lpEnvelope != NULL)
	_dump_DIENVELOPE(eff->lpEnvelope);
    if (type == DIEFT_CONSTANTFORCE) {
	if (eff->cbTypeSpecificParams != sizeof(DICONSTANTFORCE)) {
	    WARN("Effect claims to be a constant force but the type-specific params are the wrong size!\n"); 
	} else {
	    _dump_DICONSTANTFORCE(eff->lpvTypeSpecificParams);
	}
    } else if (type == DIEFT_PERIODIC) { 
        if (eff->cbTypeSpecificParams != sizeof(DIPERIODIC)) {
            WARN("Effect claims to be a periodic force but the type-specific params are the wrong size!\n");
        } else {
            _dump_DIPERIODIC(eff->lpvTypeSpecificParams);
        }
    } else if (type == DIEFT_RAMPFORCE) {
        if (eff->cbTypeSpecificParams != sizeof(DIRAMPFORCE)) {
            WARN("Effect claims to be a ramp force but the type-specific params are the wrong size!\n");
        } else {
            _dump_DIRAMPFORCE(eff->lpvTypeSpecificParams);
        }
    } else if (type == DIEFT_CONDITION) { 
        if (eff->cbTypeSpecificParams != sizeof(DICONDITION)) {
            WARN("Effect claims to be a condition but the type-specific params are the wrong size!\n");
        } else {
            _dump_DICONDITION(eff->lpvTypeSpecificParams);
        }
    } else if (type == DIEFT_CUSTOMFORCE) {
        if (eff->cbTypeSpecificParams != sizeof(DICUSTOMFORCE)) {
            WARN("Effect claims to be a custom force but the type-specific params are the wrong size!\n");
        } else {
            _dump_DICUSTOMFORCE(eff->lpvTypeSpecificParams);
        }
    }
}


/******************************************************************************
 *      LinuxInputEffectImpl 
 */

static ULONG WINAPI LinuxInputEffectImpl_AddRef(
	LPDIRECTINPUTEFFECT iface)
{
    LinuxInputEffectImpl *This = (LinuxInputEffectImpl *)iface;
    return InterlockedIncrement(&(This->ref));
}

static HRESULT WINAPI LinuxInputEffectImpl_Download(
	LPDIRECTINPUTEFFECT iface)
{
    LinuxInputEffectImpl *This = (LinuxInputEffectImpl *)iface;

    TRACE("(this=%p)\n", This);

    if (ioctl(*(This->fd), EVIOCSFF, &This->effect) == -1) {
	if (errno == ENOMEM) {
	    return DIERR_DEVICEFULL;
	} else {
            FIXME("Could not upload effect. Assuming a disconnected device %d \"%s\".\n", *This->fd, strerror(errno));
	    return DIERR_INPUTLOST;
	}
    }

    return DI_OK;
}

static HRESULT WINAPI LinuxInputEffectImpl_Escape(
	LPDIRECTINPUTEFFECT iface,
        LPDIEFFESCAPE pesc)
{
    WARN("(this=%p,%p): invalid: no hardware-specific escape codes in this" 
         " driver!\n", iface, pesc);

    return DI_OK;
}

static HRESULT WINAPI LinuxInputEffectImpl_GetEffectGuid(
        LPDIRECTINPUTEFFECT iface,
	LPGUID pguid)
{
    LinuxInputEffectImpl *This = (LinuxInputEffectImpl*)iface;

    TRACE("(this=%p,%p)\n", This, pguid);

    pguid = &This->guid;
    
    return DI_OK;
}

static HRESULT WINAPI LinuxInputEffectImpl_GetEffectStatus(
        LPDIRECTINPUTEFFECT iface,
	LPDWORD pdwFlags)
{
    TRACE("(this=%p,%p)\n", iface, pdwFlags);

    /* linux sends the effect status through an event.
     * that event is trapped by our parent joystick driver
     * and there is no clean way to pass it back to us. */
    FIXME("Not enough information to provide a status.\n");

    (*pdwFlags) = 0;

    return DI_OK;
}

static HRESULT WINAPI LinuxInputEffectImpl_GetParameters(
	LPDIRECTINPUTEFFECT iface,
	LPDIEFFECT peff,
	DWORD dwFlags)
{
    HRESULT diErr = DI_OK;
    LinuxInputEffectImpl *This = (LinuxInputEffectImpl *)iface;
    TRACE("(this=%p,%p,%d)\n", This, peff, dwFlags);

    /* Major conversion factors are:
     * times: millisecond (linux) -> microsecond (windows) (x * 1000)
     * forces: scale 0x7FFF (linux) -> scale 10000 (windows) approx ((x / 33) * 10)
     * angles: scale 0x7FFF (linux) -> scale 35999 (windows) approx ((x / 33) * 36)
     * angle bases: 0 -> -y (down) (linux) -> 0 -> +x (right) (windows)
     */

    if (dwFlags & DIEP_AXES) {
	if (peff->cAxes < 2 /* linuxinput effects always use 2 axes, x and y */)
	    diErr = DIERR_MOREDATA;
	peff->cAxes = 2; 
	if (diErr)
	    return diErr;
	else {
	    peff->rgdwAxes[0] = DIJOFS_X;
	    peff->rgdwAxes[1] = DIJOFS_Y;
	}
    }
 
    if (dwFlags & DIEP_DIRECTION) {
        if (peff->cAxes < 2)
            diErr = DIERR_MOREDATA;
        peff->cAxes = 2; 
        if (diErr)
            return diErr;
        else {
	    if (peff->dwFlags & DIEFF_CARTESIAN) {
		peff->rglDirection[0] = (long)(sin(M_PI * 3 * This->effect.direction / 0x7FFF) * 1000);
		peff->rglDirection[1] = (long)(cos(M_PI * 3 * This->effect.direction / 0x7FFF) * 1000);
	    } else {
		/* Polar and spherical coordinates are the same for two or less
		 * axes.
		 * Note that we also use this case if NO flags are marked.
		 * According to MSDN, we should return the direction in the
		 * format that it was specified in, if no flags are marked.
		 */
		peff->rglDirection[0] = (This->effect.direction / 33) * 36 + 9000;
		if (peff->rglDirection[0] > 35999)
		    peff->rglDirection[0] -= 35999;
	    }
	}
    }

    if (dwFlags & DIEP_DURATION) {
	peff->dwDuration = (DWORD)This->effect.replay.length * 1000;
    }

    if (dwFlags & DIEP_ENVELOPE) {
	struct ff_envelope* env;
	if (This->effect.type == FF_CONSTANT) env = &This->effect.u.constant.envelope;
	else if (This->effect.type == FF_PERIODIC) env = &This->effect.u.periodic.envelope;
	else if (This->effect.type == FF_RAMP) env = &This->effect.u.ramp.envelope;
	else env = NULL;
	if (env == NULL) {
	    peff->lpEnvelope = NULL;
	} else if (peff->lpEnvelope == NULL) {
	    return DIERR_INVALIDPARAM;
	} else { 
	    peff->lpEnvelope->dwAttackLevel = (env->attack_level / 33) * 10;
	    peff->lpEnvelope->dwAttackTime = env->attack_length * 1000;
	    peff->lpEnvelope->dwFadeLevel = (env->fade_level / 33) * 10;
	    peff->lpEnvelope->dwFadeTime = env->fade_length * 1000;
	}
    }

    if (dwFlags & DIEP_GAIN) {
	peff->dwGain = This->gain * 10000 / 0xFFFF;
    }

    if (dwFlags & DIEP_SAMPLEPERIOD) {
	/* the linux input ff driver has no support for setting
	 * the playback sample period.  0 means default. */
    	peff->dwSamplePeriod = 0;
    }

    if (dwFlags & DIEP_STARTDELAY) {
	peff->dwStartDelay = This->effect.replay.delay * 1000;
    }

    if (dwFlags & DIEP_TRIGGERBUTTON) {
	FIXME("LinuxInput button mapping needs redoing; for now, assuming we're using an actual joystick.\n");
	peff->dwTriggerButton = DIJOFS_BUTTON(This->effect.trigger.button - BTN_JOYSTICK);
    }

    if (dwFlags & DIEP_TRIGGERREPEATINTERVAL) {
	peff->dwTriggerRepeatInterval = This->effect.trigger.interval * 1000;
    }

    if (dwFlags & DIEP_TYPESPECIFICPARAMS) {
	DWORD expectedsize = 0;
	if (This->effect.type == FF_PERIODIC) {
	    expectedsize = sizeof(DIPERIODIC);
	} else if (This->effect.type == FF_CONSTANT) {
	    expectedsize = sizeof(DICONSTANTFORCE);
	} else if (This->effect.type == FF_SPRING 
		|| This->effect.type == FF_FRICTION 
		|| This->effect.type == FF_INERTIA 
		|| This->effect.type == FF_DAMPER) {
	    expectedsize = sizeof(DICONDITION) * 2;
	} else if (This->effect.type == FF_RAMP) {
	    expectedsize = sizeof(DIRAMPFORCE);
	}
	if (expectedsize > peff->cbTypeSpecificParams)
	    diErr = DIERR_MOREDATA;
	peff->cbTypeSpecificParams = expectedsize;
	if (diErr)
	    return diErr;
	else {
	    if (This->effect.type == FF_PERIODIC) {
                LPDIPERIODIC tsp = peff->lpvTypeSpecificParams;
		tsp->dwMagnitude = (This->effect.u.periodic.magnitude / 33) * 10;
		tsp->lOffset = (This->effect.u.periodic.offset / 33) * 10;
		tsp->dwPhase = (This->effect.u.periodic.phase / 33) * 36;
		tsp->dwPeriod = (This->effect.u.periodic.period * 1000);
	    } else if (This->effect.type == FF_CONSTANT) {
                LPDICONSTANTFORCE tsp = peff->lpvTypeSpecificParams;
		tsp->lMagnitude = (This->effect.u.constant.level / 33) * 10;
	    } else if (This->effect.type == FF_SPRING 
		    || This->effect.type == FF_FRICTION 
		    || This->effect.type == FF_INERTIA 
		    || This->effect.type == FF_DAMPER) {
                LPDICONDITION tsp = peff->lpvTypeSpecificParams;
		int i;
		for (i = 0; i < 2; ++i) {
		    tsp[i].lOffset = (This->effect.u.condition[i].center / 33) * 10; 
		    tsp[i].lPositiveCoefficient = (This->effect.u.condition[i].right_coeff / 33) * 10;
		    tsp[i].lNegativeCoefficient = (This->effect.u.condition[i].left_coeff / 33) * 10; 
		    tsp[i].dwPositiveSaturation = (This->effect.u.condition[i].right_saturation / 33) * 10;
		    tsp[i].dwNegativeSaturation = (This->effect.u.condition[i].left_saturation / 33) * 10;
		    tsp[i].lDeadBand = (This->effect.u.condition[i].deadband / 33) * 10;
		}
	    } else if (This->effect.type == FF_RAMP) {
                LPDIRAMPFORCE tsp = peff->lpvTypeSpecificParams;
		tsp->lStart = (This->effect.u.ramp.start_level / 33) * 10;
		tsp->lEnd = (This->effect.u.ramp.end_level / 33) * 10;
	    }
	}
    } 

    return diErr;
}

static HRESULT WINAPI LinuxInputEffectImpl_Initialize(
        LPDIRECTINPUTEFFECT iface,
	HINSTANCE hinst,
	DWORD dwVersion,
	REFGUID rguid)
{
    FIXME("(this=%p,%p,%d,%s): stub!\n",
	 iface, hinst, dwVersion, debugstr_guid(rguid));

    return DI_OK;
}

static HRESULT WINAPI LinuxInputEffectImpl_QueryInterface(
	LPDIRECTINPUTEFFECT iface,
	REFIID riid,
	void **ppvObject)
{
    LinuxInputEffectImpl* This = (LinuxInputEffectImpl*)iface;

    TRACE("(this=%p,%s,%p)\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(&IID_IUnknown, riid) ||
	IsEqualGUID(&IID_IDirectInputEffect, riid)) {
	    LinuxInputEffectImpl_AddRef(iface);
	    *ppvObject = This;
	    return 0;
    }

    TRACE("Unsupported interface!\n");
    return E_FAIL;
}

static HRESULT WINAPI LinuxInputEffectImpl_Start(
	LPDIRECTINPUTEFFECT iface,
	DWORD dwIterations,
	DWORD dwFlags)
{
    struct input_event event;
    LinuxInputEffectImpl* This = (LinuxInputEffectImpl*)iface;

    TRACE("(this=%p,%d,%d)\n", This, dwIterations, dwFlags);

    if (!(dwFlags & DIES_NODOWNLOAD)) {
	/* Download the effect if necessary */
	if (This->effect.id == -1) {
	    HRESULT res = LinuxInputEffectImpl_Download(iface);
	    if (res != DI_OK)
		return res;
	}
    }

    if (dwFlags & DIES_SOLO) {
	FIXME("Solo mode requested: should be stopping all effects here!\n");
    }

    event.type = EV_FF;
    event.code = This->effect.id;
    event.value = dwIterations;
    if (write(*(This->fd), &event, sizeof(event)) == -1) {
	FIXME("Unable to write event.  Assuming device disconnected.\n");
	return DIERR_INPUTLOST;
    }

    return DI_OK;
}

static HRESULT WINAPI LinuxInputEffectImpl_SetParameters(
        LPDIRECTINPUTEFFECT iface,
        LPCDIEFFECT peff,
        DWORD dwFlags)
{       
    LinuxInputEffectImpl* This = (LinuxInputEffectImpl*)iface; 
    DWORD type = _typeFromGUID(&This->guid);
    HRESULT retval = DI_OK;

    TRACE("(this=%p,%p,%d)\n", This, peff, dwFlags);

    _dump_DIEFFECT(peff, &This->guid);

    if ((dwFlags & ~DIEP_NORESTART & ~DIEP_NODOWNLOAD & ~DIEP_START) == 0) {
	/* set everything */
	dwFlags = DIEP_AXES | DIEP_DIRECTION | DIEP_DURATION | DIEP_ENVELOPE |
	    DIEP_GAIN | DIEP_SAMPLEPERIOD | DIEP_STARTDELAY | DIEP_TRIGGERBUTTON |
	    DIEP_TRIGGERREPEATINTERVAL | DIEP_TYPESPECIFICPARAMS;
    }

    if (dwFlags & DIEP_AXES) {
	/* the linux input effect system only supports one or two axes */
	if (peff->cAxes > 2)
	    return DIERR_INVALIDPARAM;
	else if (peff->cAxes < 1)
	    return DIERR_INCOMPLETEEFFECT;
	This->first_axis_is_x = peff->rgdwAxes[0] == DIJOFS_X;
    }

    /* some of this may look funky, but it's 'cause the linux driver and directx have
     * different opinions about which way direction "0" is.  directx has 0 along the x
     * axis (left), linux has it along the y axis (down). */ 
    if (dwFlags & DIEP_DIRECTION) {
	if (peff->cAxes == 1) {
	    if (peff->dwFlags & DIEFF_CARTESIAN) {
		if (dwFlags & DIEP_AXES) {
		    if (peff->rgdwAxes[0] == DIJOFS_X && peff->rglDirection[0] >= 0)
			This->effect.direction = 0x4000;
		    else if (peff->rgdwAxes[0] == DIJOFS_X && peff->rglDirection[0] < 0)
			This->effect.direction = 0xC000;
		    else if (peff->rgdwAxes[0] == DIJOFS_Y && peff->rglDirection[0] >= 0)
			This->effect.direction = 0;
		    else if (peff->rgdwAxes[0] == DIJOFS_Y && peff->rglDirection[0] < 0)
			This->effect.direction = 0x8000;
		}
	    } else {
		/* one-axis effects must use cartesian coords */
		return DIERR_INVALIDPARAM;
	    }
	} else { /* two axes */
	    if (peff->dwFlags & DIEFF_CARTESIAN) {
		LONG x, y;
		if (This->first_axis_is_x) {
		    x = peff->rglDirection[0];
		    y = peff->rglDirection[1];
		} else {
		    x = peff->rglDirection[1];
		    y = peff->rglDirection[0];
		}
		This->effect.direction = (int)((3 * M_PI / 2 - atan2(y, x)) * -0x7FFF / M_PI);
	    } else {
		/* Polar and spherical are the same for 2 axes */
		/* Precision is important here, so we do double math with exact constants */
		This->effect.direction = (int)(((double)peff->rglDirection[0] - 90) / 35999) * 0x7FFF;
	    }
	}
    }

    if (dwFlags & DIEP_DURATION)
	This->effect.replay.length = peff->dwDuration / 1000;

    if (dwFlags & DIEP_ENVELOPE) {
        struct ff_envelope* env;
        if (This->effect.type == FF_CONSTANT) env = &This->effect.u.constant.envelope;
        else if (This->effect.type == FF_PERIODIC) env = &This->effect.u.periodic.envelope;
        else if (This->effect.type == FF_RAMP) env = &This->effect.u.ramp.envelope;
        else env = NULL; 

	if (peff->lpEnvelope == NULL) {
	    /* if this type had an envelope, reset it
	     * note that length can never be zero, so we set it to something minuscule */
	    if (env) {
		env->attack_length = 0x10;
		env->attack_level = 0x7FFF;
		env->fade_length = 0x10;
		env->fade_level = 0x7FFF;
	    }
	} else {
	    /* did we get passed an envelope for a type that doesn't even have one? */
	    if (!env) return DIERR_INVALIDPARAM;
	    /* copy the envelope */
	    env->attack_length = peff->lpEnvelope->dwAttackTime / 1000;
	    env->attack_level = (peff->lpEnvelope->dwAttackLevel / 10) * 32;
	    env->fade_length = peff->lpEnvelope->dwFadeTime / 1000;
	    env->fade_level = (peff->lpEnvelope->dwFadeLevel / 10) * 32;
	}
    }

    /* Gain and Sample Period settings are not supported by the linux
     * event system */
    if (dwFlags & DIEP_GAIN) {
	This->gain = 0xFFFF * peff->dwGain / 10000;
	TRACE("Effect gain requested but no effect gain functionality present.\n");
    }

    if (dwFlags & DIEP_SAMPLEPERIOD)
	TRACE("Sample period requested but no sample period functionality present.\n");

    if (dwFlags & DIEP_STARTDELAY)
	This->effect.replay.delay = peff->dwStartDelay / 1000;

    if (dwFlags & DIEP_TRIGGERBUTTON) {
	if (peff->dwTriggerButton != -1) {
	    FIXME("Linuxinput button mapping needs redoing, assuming we're using a joystick.\n");
	    FIXME("Trigger button translation not yet implemented!\n");
	}
	This->effect.trigger.button = 0;
    }

    if (dwFlags & DIEP_TRIGGERREPEATINTERVAL)
	This->effect.trigger.interval = peff->dwTriggerRepeatInterval / 1000;

    if (dwFlags & DIEP_TYPESPECIFICPARAMS) {
	if (!(peff->lpvTypeSpecificParams))
	    return DIERR_INCOMPLETEEFFECT;
	if (type == DIEFT_PERIODIC) {
            LPCDIPERIODIC tsp;
            if (peff->cbTypeSpecificParams != sizeof(DIPERIODIC))
                return DIERR_INVALIDPARAM;
            tsp = peff->lpvTypeSpecificParams;
	    This->effect.u.periodic.magnitude = (tsp->dwMagnitude / 10) * 32;
	    This->effect.u.periodic.offset = (tsp->lOffset / 10) * 32;
	    This->effect.u.periodic.phase = (tsp->dwPhase / 9) * 8; /* == (/ 36 * 32) */
	    This->effect.u.periodic.period = tsp->dwPeriod / 1000;
	} else if (type == DIEFT_CONSTANTFORCE) {
            LPCDICONSTANTFORCE tsp;
            if (peff->cbTypeSpecificParams != sizeof(DICONSTANTFORCE))
                return DIERR_INVALIDPARAM;
            tsp = peff->lpvTypeSpecificParams;
	    This->effect.u.constant.level = (max(min(tsp->lMagnitude, 10000), -10000) / 10) * 32;
	} else if (type == DIEFT_RAMPFORCE) {
            LPCDIRAMPFORCE tsp;
            if (peff->cbTypeSpecificParams != sizeof(DIRAMPFORCE))
                return DIERR_INVALIDPARAM;
            tsp = peff->lpvTypeSpecificParams;
	    This->effect.u.ramp.start_level = (tsp->lStart / 10) * 32;
	    This->effect.u.ramp.end_level = (tsp->lStart / 10) * 32;
	} else if (type == DIEFT_CONDITION) {
            LPCDICONDITION tsp = peff->lpvTypeSpecificParams;
            if (peff->cbTypeSpecificParams == sizeof(DICONDITION)) {
		/* One condition block.  This needs to be rotated to direction,
		 * and expanded to separate x and y conditions. */
		int i;
		double factor[2];
		factor[0] = asin((This->effect.direction * 3.0 * M_PI) / 0x7FFF);
		factor[1] = acos((This->effect.direction * 3.0 * M_PI) / 0x7FFF);
                for (i = 0; i < 2; ++i) {
                    This->effect.u.condition[i].center = (int)(factor[i] * (tsp->lOffset / 10) * 32);
                    This->effect.u.condition[i].right_coeff = (int)(factor[i] * (tsp->lPositiveCoefficient / 10) * 32);
                    This->effect.u.condition[i].left_coeff = (int)(factor[i] * (tsp->lNegativeCoefficient / 10) * 32); 
                    This->effect.u.condition[i].right_saturation = (int)(factor[i] * (tsp->dwPositiveSaturation / 10) * 32);
                    This->effect.u.condition[i].left_saturation = (int)(factor[i] * (tsp->dwNegativeSaturation / 10) * 32);
                    This->effect.u.condition[i].deadband = (int)(factor[i] * (tsp->lDeadBand / 10) * 32);
                }
	    } else if (peff->cbTypeSpecificParams == 2 * sizeof(DICONDITION)) {
		/* Two condition blocks.  Direct parameter copy. */
		int i;
                for (i = 0; i < 2; ++i) {
		    This->effect.u.condition[i].center = (tsp[i].lOffset / 10) * 32;
		    This->effect.u.condition[i].right_coeff = (tsp[i].lPositiveCoefficient / 10) * 32;
		    This->effect.u.condition[i].left_coeff = (tsp[i].lNegativeCoefficient / 10) * 32;
		    This->effect.u.condition[i].right_saturation = (tsp[i].dwPositiveSaturation / 10) * 32;
		    This->effect.u.condition[i].left_saturation = (tsp[i].dwNegativeSaturation / 10) * 32;
		    This->effect.u.condition[i].deadband = (tsp[i].lDeadBand / 10) * 32;
		}
	    } else {
                return DIERR_INVALIDPARAM;
	    }
	} else {
	    FIXME("Custom force types are not supported\n");	
	    return DIERR_INVALIDPARAM;
	}
    }

    if (!(dwFlags & DIEP_NODOWNLOAD))
	retval = LinuxInputEffectImpl_Download(iface);
    if (retval != DI_OK)
	return DI_DOWNLOADSKIPPED;

    if (dwFlags & DIEP_NORESTART)
	TRACE("DIEP_NORESTART: not handled (we have no control of that).\n");

    if (dwFlags & DIEP_START)
	retval = LinuxInputEffectImpl_Start(iface, 1, 0);
    if (retval != DI_OK)
	return retval;
 
    return DI_OK;
}   

static HRESULT WINAPI LinuxInputEffectImpl_Stop(
        LPDIRECTINPUTEFFECT iface)
{
    struct input_event event;
    LinuxInputEffectImpl *This = (LinuxInputEffectImpl *)iface;

    TRACE("(this=%p)\n", This);

    event.type = EV_FF;
    event.code = This->effect.id;
    event.value = 0;
    /* we don't care about the success or failure of this call */
    write(*(This->fd), &event, sizeof(event));

    return DI_OK;
}

static HRESULT WINAPI LinuxInputEffectImpl_Unload(
	LPDIRECTINPUTEFFECT iface)
{
    LinuxInputEffectImpl *This = (LinuxInputEffectImpl *)iface;
    TRACE("(this=%p)\n", This);

    /* Erase the downloaded effect */
    if (ioctl(*(This->fd), EVIOCRMFF, This->effect.id) == -1)
	return DIERR_INVALIDPARAM;

    /* Mark the effect as deallocated */
    This->effect.id = -1;

    return DI_OK;
}

static ULONG WINAPI LinuxInputEffectImpl_Release(LPDIRECTINPUTEFFECT iface)
{
    LinuxInputEffectImpl *This = (LinuxInputEffectImpl *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));

    if (ref == 0)
    {
        LinuxInputEffectImpl_Stop(iface);
        LinuxInputEffectImpl_Unload(iface);
        list_remove(This->entry);
        HeapFree(GetProcessHeap(), 0, LIST_ENTRY(This->entry, effect_list_item, entry));
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/******************************************************************************
 *      LinuxInputEffect
 */

HRESULT linuxinput_create_effect(
	int* fd,
	REFGUID rguid,
        struct list *parent_list_entry,
	LPDIRECTINPUTEFFECT* peff)
{
    LinuxInputEffectImpl* newEffect = HeapAlloc(GetProcessHeap(), 
	HEAP_ZERO_MEMORY, sizeof(LinuxInputEffectImpl));
    DWORD type = _typeFromGUID(rguid);

    newEffect->lpVtbl = &LinuxInputEffectVtbl;
    newEffect->ref = 1;
    newEffect->guid = *rguid;
    newEffect->fd = fd;
    newEffect->gain = 0xFFFF;

    /* set the type.  this cannot be changed over the effect's life. */
    switch (type) {
	case DIEFT_PERIODIC: 
	    newEffect->effect.type = FF_PERIODIC;
	    if (IsEqualGUID(rguid, &GUID_Sine)) {
		newEffect->effect.u.periodic.waveform = FF_SINE;
	    } else if (IsEqualGUID(rguid, &GUID_Triangle)) {
                newEffect->effect.u.periodic.waveform = FF_TRIANGLE;
            } else if (IsEqualGUID(rguid, &GUID_Square)) {
                newEffect->effect.u.periodic.waveform = FF_SQUARE;
            } else if (IsEqualGUID(rguid, &GUID_SawtoothUp)) {
                newEffect->effect.u.periodic.waveform = FF_SAW_UP;
            } else if (IsEqualGUID(rguid, &GUID_SawtoothDown)) {
                newEffect->effect.u.periodic.waveform = FF_SAW_DOWN;
	    }
	    break;
	case DIEFT_CONSTANTFORCE: 
            newEffect->effect.type = FF_CONSTANT;
	    break;
	case DIEFT_RAMPFORCE: 
	    newEffect->effect.type = FF_RAMP;
	    break;
	case DIEFT_CONDITION: 
	    if (IsEqualGUID(rguid, &GUID_Spring)) {
		newEffect->effect.type = FF_SPRING;
            } else if (IsEqualGUID(rguid, &GUID_Friction)) {
		newEffect->effect.type = FF_FRICTION;
            } else if (IsEqualGUID(rguid, &GUID_Inertia)) {
		newEffect->effect.type = FF_INERTIA;
            } else if (IsEqualGUID(rguid, &GUID_Damper)) {
		newEffect->effect.type = FF_DAMPER;
	    }
	    break;
	case DIEFT_CUSTOMFORCE:
	    FIXME("Custom forces are not supported.\n");
	    HeapFree(GetProcessHeap(), 0, newEffect);
	    return DIERR_INVALIDPARAM;
	default:
            FIXME("Unknown force type 0x%x.\n", type);
            HeapFree(GetProcessHeap(), 0, newEffect);
	    return DIERR_INVALIDPARAM;
    }

    /* mark as non-uploaded */
    newEffect->effect.id = -1;

    newEffect->entry = parent_list_entry;

    *peff = (LPDIRECTINPUTEFFECT)newEffect; 

    TRACE("Creating linux input system effect (%p) with guid %s\n", 
	  *peff, _dump_dinput_GUID(rguid));

    return DI_OK;
}

HRESULT linuxinput_get_info_A(
	int fd,
	REFGUID rguid,
	LPDIEFFECTINFOA info)
{
    DWORD type = _typeFromGUID(rguid);

    TRACE("(%d, %s, %p) type=%d\n", fd, _dump_dinput_GUID(rguid), info, type);

    if (!info) return E_POINTER;

    if (info->dwSize != sizeof(DIEFFECTINFOA)) return DIERR_INVALIDPARAM;

    info->guid = *rguid;
    
    info->dwEffType = type; 
    /* the event device API does not support querying for all these things
     * therefore we assume that we have support for them
     * that's not as dangerous as it sounds, since drivers are allowed to
     * ignore parameters they claim to support anyway */
    info->dwEffType |= DIEFT_DEADBAND | DIEFT_FFATTACK | DIEFT_FFFADE 
                    | DIEFT_POSNEGCOEFFICIENTS | DIEFT_POSNEGSATURATION
		    | DIEFT_SATURATION | DIEFT_STARTDELAY; 

    /* again, assume we have support for everything */
    info->dwStaticParams = DIEP_ALLPARAMS;
    info->dwDynamicParams = info->dwStaticParams;

    /* yes, this is windows behavior (print the GUID_Name for name) */
    strcpy(info->tszName, _dump_dinput_GUID(rguid));

    return DI_OK;
}

HRESULT linuxinput_get_info_W(
	int fd,
	REFGUID rguid,
	LPDIEFFECTINFOW info)
{
    DWORD type = _typeFromGUID(rguid);

    TRACE("(%d, %s, %p) type=%d\n", fd, _dump_dinput_GUID(rguid), info, type);

    if (!info) return E_POINTER;

    if (info->dwSize != sizeof(DIEFFECTINFOW)) return DIERR_INVALIDPARAM;

    info->guid = *rguid;

    info->dwEffType = type;
    /* the event device API does not support querying for all these things
     * therefore we assume that we have support for them
     * that's not as dangerous as it sounds, since drivers are allowed to
     * ignore parameters they claim to support anyway */
    info->dwEffType |= DIEFT_DEADBAND | DIEFT_FFATTACK | DIEFT_FFFADE
                    | DIEFT_POSNEGCOEFFICIENTS | DIEFT_POSNEGSATURATION
                    | DIEFT_SATURATION | DIEFT_STARTDELAY; 

    /* again, assume we have support for everything */
    info->dwStaticParams = DIEP_ALLPARAMS;
    info->dwDynamicParams = info->dwStaticParams;

    /* yes, this is windows behavior (print the GUID_Name for name) */
    MultiByteToWideChar(CP_ACP, 0, _dump_dinput_GUID(rguid), -1, 
                        info->tszName, MAX_PATH);

    return DI_OK;
}

static const IDirectInputEffectVtbl LinuxInputEffectVtbl = {
    LinuxInputEffectImpl_QueryInterface,
    LinuxInputEffectImpl_AddRef,
    LinuxInputEffectImpl_Release,
    LinuxInputEffectImpl_Initialize,
    LinuxInputEffectImpl_GetEffectGuid,
    LinuxInputEffectImpl_GetParameters,
    LinuxInputEffectImpl_SetParameters,
    LinuxInputEffectImpl_Start,
    LinuxInputEffectImpl_Stop,
    LinuxInputEffectImpl_GetEffectStatus,
    LinuxInputEffectImpl_Download,
    LinuxInputEffectImpl_Unload,
    LinuxInputEffectImpl_Escape
};

#endif /* HAVE_STRUCT_FF_EFFECT_DIRECTION */
