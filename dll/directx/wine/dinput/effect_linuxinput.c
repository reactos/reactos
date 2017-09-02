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
#include <limits.h>
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
#include "joystick_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

static const IDirectInputEffectVtbl LinuxInputEffectVtbl;
typedef struct LinuxInputEffectImpl LinuxInputEffectImpl;
struct LinuxInputEffectImpl
{
    IDirectInputEffect  IDirectInputEffect_iface;
    LONG                ref;
    GUID                guid;

    struct ff_effect    effect; /* Effect data */
    int                 gain;   /* Effect gain */
    BOOL                first_axis_is_x;
    int*                fd;     /* Parent device */
    struct list        *entry;  /* Entry into the parent's list of effects */
};

static inline LinuxInputEffectImpl *impl_from_IDirectInputEffect(IDirectInputEffect *iface)
{
    return CONTAINING_RECORD(iface, LinuxInputEffectImpl, IDirectInputEffect_iface);
}

static double ff_effect_direction_to_rad(unsigned int dir)
{
    return (dir & 0xffff) * M_PI / 0x8000;
}

static void ff_dump_effect(struct ff_effect *effect)
{
    const char *type = "(Unknown)", *length = "INFINITE";
    struct ff_envelope *env = NULL;
    double angle;
#define FE(x) case x: type = #x; break
    switch (effect->type)
    {
        FE(FF_RUMBLE);
        FE(FF_PERIODIC);
        FE(FF_CONSTANT);
        FE(FF_SPRING);
        FE(FF_FRICTION);
        FE(FF_DAMPER);
        FE(FF_INERTIA);
        FE(FF_RAMP);
    }
#undef FE

    /* rotate so 0 points right */
    angle = 360 - ff_effect_direction_to_rad(effect->direction + 0xc000) * 180 / M_PI;

    if (effect->replay.length)
      length = wine_dbg_sprintf("%u ms", effect->replay.length);

    TRACE("type 0x%x %s, id %d, direction 0x%x (source angle %.2f), time length %s, start delay %u ms\n",
          effect->type, type, effect->id, effect->direction, angle, length, effect->replay.delay);
    if (effect->trigger.button || effect->trigger.interval)
        TRACE(" -> trigger button %u, re-trigger interval %u ms\n",
              effect->trigger.button, effect->trigger.interval);

    if (effect->type == FF_PERIODIC)
    {
        struct ff_periodic_effect *per = &effect->u.periodic;
        const char *wave = "(Unknown)";
#define FE(x) case x: wave = #x; break
        switch (per->waveform)
        {
            FE(FF_SQUARE);
            FE(FF_TRIANGLE);
            FE(FF_SINE);
            FE(FF_SAW_UP);
            FE(FF_SAW_DOWN);
            FE(FF_CUSTOM);
        }
#undef FE
        angle = ff_effect_direction_to_rad(per->phase) * 180 / M_PI;
        TRACE(" -> waveform 0x%x %s, period %u ms, magnitude %d, offset %d, phase 0x%x (angle %.2f), custom len %d\n",
              per->waveform, wave, per->period, per->magnitude, per->offset, per->phase, angle, per->custom_len);
        env = &per->envelope;
    }
    else if (effect->type == FF_CONSTANT)
    {
        struct ff_constant_effect *cons = &effect->u.constant;
        TRACE(" -> level %d\n", cons->level);
        env = &cons->envelope;
    }
    else if (effect->type == FF_RAMP)
    {
        struct ff_ramp_effect *ramp = &effect->u.ramp;
        TRACE(" -> start/end level %d/%d\n", ramp->start_level, ramp->end_level);
        env = &ramp->envelope;
    }
    else if (effect->type == FF_RUMBLE)
    {
        struct ff_rumble_effect *rumble = &effect->u.rumble;
        TRACE(" -> strong/weak magnitude %u/%u\n", rumble->strong_magnitude, rumble->weak_magnitude);
    }
    else if (effect->type == FF_SPRING || effect->type == FF_FRICTION ||
             effect->type == FF_DAMPER || effect->type == FF_INERTIA)
    {
        struct ff_condition_effect *cond = effect->u.condition;
        int i;
        for (i = 0; i < 2; i++)
        {
            /* format numbers here to make them align correctly */
            TRACE(" -> [%d] right/left saturation %5u/%5u, right/left coefficient %5d/%5d,"
                  " deadband %5u, center %5d\n", i, cond[i].right_saturation, cond[i].left_saturation,
                  cond[i].right_coeff, cond[i].left_coeff, cond[i].deadband, cond[i].center);
        }
    }

    if (env)
        TRACE(" -> envelope attack length(ms)/level %u/%u, fade length(ms)/level %u/%u\n",
              env->attack_length, env->attack_level, env->fade_length, env->fade_level);
}

/******************************************************************************
 *      LinuxInputEffectImpl 
 */

static ULONG WINAPI LinuxInputEffectImpl_AddRef(
	LPDIRECTINPUTEFFECT iface)
{
    LinuxInputEffectImpl *This = impl_from_IDirectInputEffect(iface);
    return InterlockedIncrement(&(This->ref));
}

static HRESULT WINAPI LinuxInputEffectImpl_Download(
	LPDIRECTINPUTEFFECT iface)
{
    LinuxInputEffectImpl *This = impl_from_IDirectInputEffect(iface);
    int ret, old_effect_id;

    TRACE("(this=%p)\n", This);
    ff_dump_effect(&This->effect);

    old_effect_id = This->effect.id;
    if (ioctl(*(This->fd), EVIOCSFF, &This->effect) != -1)
        return DI_OK;

    /* Linux kernel < 3.14 has a bug that incorrectly assigns an effect ID even
     * on error, restore it here if that is the case. */
    This->effect.id = old_effect_id;

    switch (errno)
    {
        case EINVAL:
            ret = DIERR_INVALIDPARAM;
            break;
        case ENOSPC:
            ret = DIERR_DEVICEFULL;
            break;
        case ENOMEM:
            ret = DIERR_OUTOFMEMORY;
            break;
        default:
            ret = DIERR_INPUTLOST;
            break;
    }
    TRACE("Could not upload effect to fd %d, errno %d \"%s\", returning 0x%x.\n",
          *This->fd, errno, strerror(errno), ret);
    return ret;
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
    LinuxInputEffectImpl *This = impl_from_IDirectInputEffect(iface);

    TRACE("(this=%p,%p)\n", This, pguid);

    *pguid = This->guid;

    return DI_OK;
}

static HRESULT WINAPI LinuxInputEffectImpl_GetEffectStatus(
        LPDIRECTINPUTEFFECT iface,
	LPDWORD pdwFlags)
{
    LinuxInputEffectImpl *This = impl_from_IDirectInputEffect(iface);

    TRACE("(this=%p,%p)\n", This, pdwFlags);

    if (!pdwFlags)
        return E_POINTER;

    if (This->effect.id == -1)
        return DIERR_NOTDOWNLOADED;

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
    LinuxInputEffectImpl *This = impl_from_IDirectInputEffect(iface);
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
		/* rotate so 0 points right */
		double angle = ff_effect_direction_to_rad(This->effect.direction + 0xc000);
		peff->rglDirection[0] = sin(angle) * 1000;
		peff->rglDirection[1] = -cos(angle) * 1000;
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

    if (dwFlags & DIEP_DURATION)
    {
        if (!This->effect.replay.length) /* infinite for the linux driver */
            peff->dwDuration = INFINITE;
        else
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

    if ((dwFlags & DIEP_STARTDELAY) && peff->dwSize > sizeof(DIEFFECT_DX5))
        peff->dwStartDelay = This->effect.replay.delay * 1000;

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
    LinuxInputEffectImpl *This = impl_from_IDirectInputEffect(iface);

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
    LinuxInputEffectImpl *This = impl_from_IDirectInputEffect(iface);

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
    event.value = min( dwIterations, INT_MAX );
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
    LinuxInputEffectImpl *This = impl_from_IDirectInputEffect(iface);
    DWORD type = typeFromGUID(&This->guid);
    HRESULT retval = DI_OK;

    TRACE("(this=%p,%p,%d)\n", This, peff, dwFlags);

    dump_DIEFFECT(peff, &This->guid, dwFlags);

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
        }
        /* two axes */
        else
        {
            if (peff->dwFlags & DIEFF_CARTESIAN)
            {
                LONG x, y;
                if (This->first_axis_is_x)
                {
                    x = peff->rglDirection[0];
                    y = peff->rglDirection[1];
                }
                else
                {
                    x = peff->rglDirection[1];
                    y = peff->rglDirection[0];
                }
                This->effect.direction = (unsigned int)((M_PI / 2 + atan2(y, x)) * 0x8000 / M_PI);
            }
            else
            {
                /* Polar and spherical are the same for 2 axes */
                /* Precision is important here, so we do double math with exact constants */
                This->effect.direction = (unsigned int)(((double)peff->rglDirection[0] / 18000) * 0x8000);
            }
        }
    }

    if (dwFlags & DIEP_DURATION)
    {
        if (peff->dwDuration == INFINITE)
            This->effect.replay.length = 0; /* infinite for the linux driver */
        else if(peff->dwDuration > 1000)
            This->effect.replay.length = peff->dwDuration / 1000;
        else
            This->effect.replay.length = 1;
    }

    if (dwFlags & DIEP_ENVELOPE)
    {
        struct ff_envelope* env;
        if (This->effect.type == FF_CONSTANT)
            env = &This->effect.u.constant.envelope;
        else if (This->effect.type == FF_PERIODIC)
            env = &This->effect.u.periodic.envelope;
        else if (This->effect.type == FF_RAMP)
            env = &This->effect.u.ramp.envelope;
        else
            env = NULL;

        /* copy the envelope if it is present and the linux effect supports it */
        if (peff->lpEnvelope && env)
        {
            env->attack_length = peff->lpEnvelope->dwAttackTime / 1000;
            env->attack_level = (peff->lpEnvelope->dwAttackLevel / 10) * 32;
            env->fade_length = peff->lpEnvelope->dwFadeTime / 1000;
            env->fade_level = (peff->lpEnvelope->dwFadeLevel / 10) * 32;
        }
        /* if the dinput envelope is NULL we will clear the linux envelope */
        else if (env)
        {
            env->attack_length = 0;
            env->attack_level = 0;
            env->fade_length = 0;
            env->fade_level = 0;
        }
        else if(peff->lpEnvelope)
        {
            if(peff->lpEnvelope->dwAttackTime || peff->lpEnvelope->dwAttackLevel ||
               peff->lpEnvelope->dwFadeTime || peff->lpEnvelope->dwFadeLevel)
                WARN("Ignoring dinput envelope not supported in the linux effect\n");
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
    if ((dwFlags & DIEP_STARTDELAY) && peff->dwSize > sizeof(DIEFFECT_DX5))
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

    if (dwFlags & DIEP_TYPESPECIFICPARAMS)
    {
        if (!(peff->lpvTypeSpecificParams))
            return DIERR_INCOMPLETEEFFECT;

        if (type == DIEFT_PERIODIC)
        {
            DIPERIODIC *tsp;
            if (peff->cbTypeSpecificParams != sizeof(DIPERIODIC))
                return DIERR_INVALIDPARAM;
            tsp = peff->lpvTypeSpecificParams;

            This->effect.u.periodic.magnitude = (tsp->dwMagnitude / 10) * 32;
            This->effect.u.periodic.offset = (tsp->lOffset / 10) * 32;
            /* phase ranges from 0 - 35999 in dinput and 0 - 65535 on Linux */
            This->effect.u.periodic.phase = (tsp->dwPhase / 36) * 65;
            /* dinput uses microseconds, Linux uses milliseconds */
            if (tsp->dwPeriod <= 1000)
                This->effect.u.periodic.period = 1;
            else
                This->effect.u.periodic.period = tsp->dwPeriod / 1000;
        }
        else if (type == DIEFT_CONSTANTFORCE)
        {
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
	    This->effect.u.ramp.end_level = (tsp->lEnd / 10) * 32;
        }
        else if (type == DIEFT_CONDITION)
        {
            DICONDITION *tsp = peff->lpvTypeSpecificParams;
            struct ff_condition_effect *cond = This->effect.u.condition;
            int i, j, sources;
            double factor[2];

            if (peff->cbTypeSpecificParams == sizeof(DICONDITION))
            {
                /* One condition block.  This needs to be rotated to direction,
                 * and expanded to separate x and y conditions. Ensures 0 points right */
                double angle = ff_effect_direction_to_rad(This->effect.direction + 0xc000);
                factor[0] = sin(angle);
                factor[1] = -cos(angle);
                sources = 1;
            }
            else if (peff->cbTypeSpecificParams == 2 * sizeof(DICONDITION))
            {
                /* Direct parameter copy without changes */
                factor[0] = factor[1] = 1;
                sources = 2;
            }
            else
                return DIERR_INVALIDPARAM;

            for (i = j = 0; i < 2; ++i)
            {
                cond[i].center = (int)(factor[i] * (tsp[j].lOffset / 10) * 32);
                cond[i].right_coeff = (int)(factor[i] * (tsp[j].lPositiveCoefficient / 10) * 32);
                cond[i].left_coeff = (int)(factor[i] * (tsp[j].lNegativeCoefficient / 10) * 32);
                cond[i].right_saturation = (int)(factor[i] * (tsp[j].dwPositiveSaturation / 10) * 65);
                cond[i].left_saturation = (int)(factor[i] * (tsp[j].dwNegativeSaturation / 10) * 65);
                cond[i].deadband = (int)(factor[i] * (tsp[j].lDeadBand / 10) * 32);
                if (sources == 2)
                    j++;
            }
        }
        else
        {
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
    LinuxInputEffectImpl *This = impl_from_IDirectInputEffect(iface);

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
    LinuxInputEffectImpl *This = impl_from_IDirectInputEffect(iface);
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
    LinuxInputEffectImpl *This = impl_from_IDirectInputEffect(iface);
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

DECLSPEC_HIDDEN HRESULT linuxinput_create_effect(
	int* fd,
	REFGUID rguid,
        struct list *parent_list_entry,
	LPDIRECTINPUTEFFECT* peff)
{
    LinuxInputEffectImpl* newEffect = HeapAlloc(GetProcessHeap(), 
	HEAP_ZERO_MEMORY, sizeof(LinuxInputEffectImpl));
    DWORD type = typeFromGUID(rguid);

    newEffect->IDirectInputEffect_iface.lpVtbl = &LinuxInputEffectVtbl;
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

    *peff = &newEffect->IDirectInputEffect_iface;

    TRACE("Creating linux input system effect (%p) with guid %s\n", 
	  *peff, _dump_dinput_GUID(rguid));

    return DI_OK;
}

DECLSPEC_HIDDEN HRESULT linuxinput_get_info_A(
	int fd,
	REFGUID rguid,
	LPDIEFFECTINFOA info)
{
    DWORD type = typeFromGUID(rguid);

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

DECLSPEC_HIDDEN HRESULT linuxinput_get_info_W(
	int fd,
	REFGUID rguid,
	LPDIEFFECTINFOW info)
{
    DWORD type = typeFromGUID(rguid);

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
