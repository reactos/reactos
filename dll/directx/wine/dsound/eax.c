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

/* Taken in large part from OpenAL's Alc/alcReverb.c. */

#include <stdarg.h>
#include <math.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "mmsystem.h"
#include "winternl.h"
#include "vfwmsgs.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsound_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(eax);

static const EAX_REVERBPROPERTIES presets[] = {
    { EAX_ENVIRONMENT_GENERIC, 0.5f, 1.493f, 0.5f },
    { EAX_ENVIRONMENT_PADDEDCELL, 0.25f, 0.1f, 0.0f },
    { EAX_ENVIRONMENT_ROOM, 0.417f, 0.4f, 0.666f },
    { EAX_ENVIRONMENT_BATHROOM, 0.653f, 1.499f, 0.166f },
    { EAX_ENVIRONMENT_LIVINGROOM, 0.208f, 0.478f, 0.0f },
    { EAX_ENVIRONMENT_STONEROOM, 0.5f, 2.309f, 0.888f },
    { EAX_ENVIRONMENT_AUDITORIUM, 0.403f, 4.279f, 0.5f },
    { EAX_ENVIRONMENT_CONCERTHALL, 0.5f, 3.961f, 0.5f },
    { EAX_ENVIRONMENT_CAVE, 0.5f, 2.886f, 1.304f },
    { EAX_ENVIRONMENT_ARENA, 0.361f, 7.284f, 0.332f },
    { EAX_ENVIRONMENT_HANGAR, 0.5f, 10.0f, 0.3f },
    { EAX_ENVIRONMENT_CARPETEDHALLWAY, 0.153f, 0.259f, 2.0f },
    { EAX_ENVIRONMENT_HALLWAY, 0.361f, 1.493f, 0.0f },
    { EAX_ENVIRONMENT_STONECORRIDOR, 0.444f, 2.697f, 0.638f },
    { EAX_ENVIRONMENT_ALLEY, 0.25f, 1.752f, 0.776f },
    { EAX_ENVIRONMENT_FOREST, 0.111f, 3.145f, 0.472f },
    { EAX_ENVIRONMENT_CITY, 0.111f, 2.767f, 0.224f },
    { EAX_ENVIRONMENT_MOUNTAINS, 0.194f, 7.841f, 0.472f },
    { EAX_ENVIRONMENT_QUARRY, 1.0f, 1.499f, 0.5f },
    { EAX_ENVIRONMENT_PLAIN, 0.097f, 2.767f, 0.224f },
    { EAX_ENVIRONMENT_PARKINGLOT, 0.208f, 1.652f, 1.5f },
    { EAX_ENVIRONMENT_SEWERPIPE, 0.652f, 2.886f, 0.25f },
    { EAX_ENVIRONMENT_UNDERWATER, 1.0f, 1.499f, 0.0f },
    { EAX_ENVIRONMENT_DRUGGED, 0.875f, 8.392f, 1.388f },
    { EAX_ENVIRONMENT_DIZZY, 0.139f, 17.234f, 0.666f },
    { EAX_ENVIRONMENT_PSYCHOTIC, 0.486f, 7.563f, 0.806f }
};

static const EFXEAXREVERBPROPERTIES efx_presets[] = {
    { 1.0000f, 1.0000f, 0.3162f, 0.8913f, 1.0000f, 1.4900f, 0.8300f, 1.0000f, 0.0500f, 0.0070f, { 0.0000f, 0.0000f, 0.0000f }, 1.2589f, 0.0110f, { 0.0000f, 0.0000f, 0.0000f }, 0.2500f, 0.0000f, 0.2500f, 0.0000f, 0.9943f, 5000.0000f, 250.0000f, 0.0000f, 0x1 }, /* generic */
    { 0.1715f, 1.0000f, 0.3162f, 0.0010f, 1.0000f, 0.1700f, 0.1000f, 1.0000f, 0.2500f, 0.0010f, { 0.0000f, 0.0000f, 0.0000f }, 1.2691f, 0.0020f, { 0.0000f, 0.0000f, 0.0000f }, 0.2500f, 0.0000f, 0.2500f, 0.0000f, 0.9943f, 5000.0000f, 250.0000f, 0.0000f, 0x1 }, /* padded cell */
    { 0.4287f, 1.0000f, 0.3162f, 0.5929f, 1.0000f, 0.4000f, 0.8300f, 1.0000f, 0.1503f, 0.0020f, { 0.0000f, 0.0000f, 0.0000f }, 1.0629f, 0.0030f, { 0.0000f, 0.0000f, 0.0000f }, 0.2500f, 0.0000f, 0.2500f, 0.0000f, 0.9943f, 5000.0000f, 250.0000f, 0.0000f, 0x1 }, /* room */
    { 0.1715f, 1.0000f, 0.3162f, 0.2512f, 1.0000f, 1.4900f, 0.5400f, 1.0000f, 0.6531f, 0.0070f, { 0.0000f, 0.0000f, 0.0000f }, 3.2734f, 0.0110f, { 0.0000f, 0.0000f, 0.0000f }, 0.2500f, 0.0000f, 0.2500f, 0.0000f, 0.9943f, 5000.0000f, 250.0000f, 0.0000f, 0x1 }, /* bathroom */
    { 0.9766f, 1.0000f, 0.3162f, 0.0010f, 1.0000f, 0.5000f, 0.1000f, 1.0000f, 0.2051f, 0.0030f, { 0.0000f, 0.0000f, 0.0000f }, 0.2805f, 0.0040f, { 0.0000f, 0.0000f, 0.0000f }, 0.2500f, 0.0000f, 0.2500f, 0.0000f, 0.9943f, 5000.0000f, 250.0000f, 0.0000f, 0x1 }, /* living room */
    { 1.0000f, 1.0000f, 0.3162f, 0.7079f, 1.0000f, 2.3100f, 0.6400f, 1.0000f, 0.4411f, 0.0120f, { 0.0000f, 0.0000f, 0.0000f }, 1.1003f, 0.0170f, { 0.0000f, 0.0000f, 0.0000f }, 0.2500f, 0.0000f, 0.2500f, 0.0000f, 0.9943f, 5000.0000f, 250.0000f, 0.0000f, 0x1 }, /* stone room */
    { 1.0000f, 1.0000f, 0.3162f, 0.5781f, 1.0000f, 4.3200f, 0.5900f, 1.0000f, 0.4032f, 0.0200f, { 0.0000f, 0.0000f, 0.0000f }, 0.7170f, 0.0300f, { 0.0000f, 0.0000f, 0.0000f }, 0.2500f, 0.0000f, 0.2500f, 0.0000f, 0.9943f, 5000.0000f, 250.0000f, 0.0000f, 0x1 }, /* auditorium */
    { 1.0000f, 1.0000f, 0.3162f, 0.5623f, 1.0000f, 3.9200f, 0.7000f, 1.0000f, 0.2427f, 0.0200f, { 0.0000f, 0.0000f, 0.0000f }, 0.9977f, 0.0290f, { 0.0000f, 0.0000f, 0.0000f }, 0.2500f, 0.0000f, 0.2500f, 0.0000f, 0.9943f, 5000.0000f, 250.0000f, 0.0000f, 0x1 }, /* concert hall */
    { 1.0000f, 1.0000f, 0.3162f, 1.0000f, 1.0000f, 2.9100f, 1.3000f, 1.0000f, 0.5000f, 0.0150f, { 0.0000f, 0.0000f, 0.0000f }, 0.7063f, 0.0220f, { 0.0000f, 0.0000f, 0.0000f }, 0.2500f, 0.0000f, 0.2500f, 0.0000f, 0.9943f, 5000.0000f, 250.0000f, 0.0000f, 0x0 }, /* cave */
    { 1.0000f, 1.0000f, 0.3162f, 0.4477f, 1.0000f, 7.2400f, 0.3300f, 1.0000f, 0.2612f, 0.0200f, { 0.0000f, 0.0000f, 0.0000f }, 1.0186f, 0.0300f, { 0.0000f, 0.0000f, 0.0000f }, 0.2500f, 0.0000f, 0.2500f, 0.0000f, 0.9943f, 5000.0000f, 250.0000f, 0.0000f, 0x1 }, /* arena */
    { 1.0000f, 1.0000f, 0.3162f, 0.3162f, 1.0000f, 10.0500f, 0.2300f, 1.0000f, 0.5000f, 0.0200f, { 0.0000f, 0.0000f, 0.0000f }, 1.2560f, 0.0300f, { 0.0000f, 0.0000f, 0.0000f }, 0.2500f, 0.0000f, 0.2500f, 0.0000f, 0.9943f, 5000.0000f, 250.0000f, 0.0000f, 0x1 }, /* hangar */
    { 0.4287f, 1.0000f, 0.3162f, 0.0100f, 1.0000f, 0.3000f, 0.1000f, 1.0000f, 0.1215f, 0.0020f, { 0.0000f, 0.0000f, 0.0000f }, 0.1531f, 0.0300f, { 0.0000f, 0.0000f, 0.0000f }, 0.2500f, 0.0000f, 0.2500f, 0.0000f, 0.9943f, 5000.0000f, 250.0000f, 0.0000f, 0x1 }, /* carpeted hallway */
    { 0.3645f, 1.0000f, 0.3162f, 0.7079f, 1.0000f, 1.4900f, 0.5900f, 1.0000f, 0.2458f, 0.0070f, { 0.0000f, 0.0000f, 0.0000f }, 1.6615f, 0.0110f, { 0.0000f, 0.0000f, 0.0000f }, 0.2500f, 0.0000f, 0.2500f, 0.0000f, 0.9943f, 5000.0000f, 250.0000f, 0.0000f, 0x1 }, /* hallway */
    { 1.0000f, 1.0000f, 0.3162f, 0.7612f, 1.0000f, 2.7000f, 0.7900f, 1.0000f, 0.2472f, 0.0130f, { 0.0000f, 0.0000f, 0.0000f }, 1.5758f, 0.0200f, { 0.0000f, 0.0000f, 0.0000f }, 0.2500f, 0.0000f, 0.2500f, 0.0000f, 0.9943f, 5000.0000f, 250.0000f, 0.0000f, 0x1 }, /* stone corridor */
    { 1.0000f, 0.3000f, 0.3162f, 0.7328f, 1.0000f, 1.4900f, 0.8600f, 1.0000f, 0.2500f, 0.0070f, { 0.0000f, 0.0000f, 0.0000f }, 0.9954f, 0.0110f, { 0.0000f, 0.0000f, 0.0000f }, 0.1250f, 0.9500f, 0.2500f, 0.0000f, 0.9943f, 5000.0000f, 250.0000f, 0.0000f, 0x1 }, /* alley */
    { 1.0000f, 0.3000f, 0.3162f, 0.0224f, 1.0000f, 1.4900f, 0.5400f, 1.0000f, 0.0525f, 0.1620f, { 0.0000f, 0.0000f, 0.0000f }, 0.7682f, 0.0880f, { 0.0000f, 0.0000f, 0.0000f }, 0.1250f, 1.0000f, 0.2500f, 0.0000f, 0.9943f, 5000.0000f, 250.0000f, 0.0000f, 0x1 }, /* forest */
    { 1.0000f, 0.5000f, 0.3162f, 0.3981f, 1.0000f, 1.4900f, 0.6700f, 1.0000f, 0.0730f, 0.0070f, { 0.0000f, 0.0000f, 0.0000f }, 0.1427f, 0.0110f, { 0.0000f, 0.0000f, 0.0000f }, 0.2500f, 0.0000f, 0.2500f, 0.0000f, 0.9943f, 5000.0000f, 250.0000f, 0.0000f, 0x1 }, /* city */
    { 1.0000f, 0.2700f, 0.3162f, 0.0562f, 1.0000f, 1.4900f, 0.2100f, 1.0000f, 0.0407f, 0.3000f, { 0.0000f, 0.0000f, 0.0000f }, 0.1919f, 0.1000f, { 0.0000f, 0.0000f, 0.0000f }, 0.2500f, 1.0000f, 0.2500f, 0.0000f, 0.9943f, 5000.0000f, 250.0000f, 0.0000f, 0x0 }, /* mountains */
    { 1.0000f, 1.0000f, 0.3162f, 0.3162f, 1.0000f, 1.4900f, 0.8300f, 1.0000f, 0.0000f, 0.0610f, { 0.0000f, 0.0000f, 0.0000f }, 1.7783f, 0.0250f, { 0.0000f, 0.0000f, 0.0000f }, 0.1250f, 0.7000f, 0.2500f, 0.0000f, 0.9943f, 5000.0000f, 250.0000f, 0.0000f, 0x1 }, /* quarry */
    { 1.0000f, 0.2100f, 0.3162f, 0.1000f, 1.0000f, 1.4900f, 0.5000f, 1.0000f, 0.0585f, 0.1790f, { 0.0000f, 0.0000f, 0.0000f }, 0.1089f, 0.1000f, { 0.0000f, 0.0000f, 0.0000f }, 0.2500f, 1.0000f, 0.2500f, 0.0000f, 0.9943f, 5000.0000f, 250.0000f, 0.0000f, 0x1 }, /* plain */
    { 1.0000f, 1.0000f, 0.3162f, 1.0000f, 1.0000f, 1.6500f, 1.5000f, 1.0000f, 0.2082f, 0.0080f, { 0.0000f, 0.0000f, 0.0000f }, 0.2652f, 0.0120f, { 0.0000f, 0.0000f, 0.0000f }, 0.2500f, 0.0000f, 0.2500f, 0.0000f, 0.9943f, 5000.0000f, 250.0000f, 0.0000f, 0x0 }, /* parking lot */
    { 0.3071f, 0.8000f, 0.3162f, 0.3162f, 1.0000f, 2.8100f, 0.1400f, 1.0000f, 1.6387f, 0.0140f, { 0.0000f, 0.0000f, 0.0000f }, 3.2471f, 0.0210f, { 0.0000f, 0.0000f, 0.0000f }, 0.2500f, 0.0000f, 0.2500f, 0.0000f, 0.9943f, 5000.0000f, 250.0000f, 0.0000f, 0x1 }, /* sewer pipe */
    { 0.3645f, 1.0000f, 0.3162f, 0.0100f, 1.0000f, 1.4900f, 0.1000f, 1.0000f, 0.5963f, 0.0070f, { 0.0000f, 0.0000f, 0.0000f }, 7.0795f, 0.0110f, { 0.0000f, 0.0000f, 0.0000f }, 0.2500f, 0.0000f, 1.1800f, 0.3480f, 0.9943f, 5000.0000f, 250.0000f, 0.0000f, 0x1 }, /* underwater */
    { 0.4287f, 0.5000f, 0.3162f, 1.0000f, 1.0000f, 8.3900f, 1.3900f, 1.0000f, 0.8760f, 0.0020f, { 0.0000f, 0.0000f, 0.0000f }, 3.1081f, 0.0300f, { 0.0000f, 0.0000f, 0.0000f }, 0.2500f, 0.0000f, 0.2500f, 1.0000f, 0.9943f, 5000.0000f, 250.0000f, 0.0000f, 0x0 }, /* drugged */
    { 0.3645f, 0.6000f, 0.3162f, 0.6310f, 1.0000f, 17.2300f, 0.5600f, 1.0000f, 0.1392f, 0.0200f, { 0.0000f, 0.0000f, 0.0000f }, 0.4937f, 0.0300f, { 0.0000f, 0.0000f, 0.0000f }, 0.2500f, 1.0000f, 0.8100f, 0.3100f, 0.9943f, 5000.0000f, 250.0000f, 0.0000f, 0x0 }, /* dizzy */
    { 0.0625f, 0.5000f, 0.3162f, 0.8404f, 1.0000f, 7.5600f, 0.9100f, 1.0000f, 0.4864f, 0.0200f, { 0.0000f, 0.0000f, 0.0000f }, 2.4378f, 0.0300f, { 0.0000f, 0.0000f, 0.0000f }, 0.2500f, 0.0000f, 4.0000f, 1.0000f, 0.9943f, 5000.0000f, 250.0000f, 0.0000f, 0x0 } /* psychotic */
};

static const float DECO_FRACTION = 0.15f;
static const float DECO_MULTIPLIER = 2.0f;

static const float EARLY_LINE_LENGTH[4] =
{
    0.0015f, 0.0045f, 0.0135f, 0.0405f
};

static const float ALLPASS_LINE_LENGTH[4] =
{
    0.0151f, 0.0167f, 0.0183f, 0.0200f,
};

static const float LATE_LINE_LENGTH[4] =
{
    0.0211f, 0.0311f, 0.0461f, 0.0680f
};

static const float LATE_LINE_MULTIPLIER = 4.0f;

#define SPEEDOFSOUNDMETRESPERSEC 343.3f

static void ReverbUpdate(IDirectSoundBufferImpl *dsb);

static float lpFilter2P(FILTER *iir, unsigned int offset, float input)
{
    float *history = &iir->history[offset*2];
    float a = iir->coeff;
    float output = input;

    output = output + (history[0]-output)*a;
    history[0] = output;
    output = output + (history[1]-output)*a;
    history[1] = output;

    return output;
}

static float lerp(float val1, float val2, float mu)
{
    return val1 + (val2-val1)*mu;
}

static void DelayLineIn(DelayLine *Delay, unsigned int offset, float in)
{
    Delay->Line[offset&Delay->Mask] = in;
}

static float DelayLineOut(DelayLine *Delay, unsigned int offset)
{
    return Delay->Line[offset&Delay->Mask];
}

static float AttenuatedDelayLineOut(DelayLine *Delay, unsigned int offset, float coeff)
{
    return coeff * Delay->Line[offset&Delay->Mask];
}

static float EarlyDelayLineOut(IDirectSoundBufferImpl* dsb, unsigned int index)
{
    return AttenuatedDelayLineOut(&dsb->eax.Early.Delay[index],
                                  dsb->eax.Offset - dsb->eax.Early.Offset[index],
                                  dsb->eax.Early.Coeff[index]);
}

static void EarlyReflection(IDirectSoundBufferImpl* dsb, float in, float *out)
{
    float d[4], v, f[4];

    /* Obtain the decayed results of each early delay line. */
    d[0] = EarlyDelayLineOut(dsb, 0);
    d[1] = EarlyDelayLineOut(dsb, 1);
    d[2] = EarlyDelayLineOut(dsb, 2);
    d[3] = EarlyDelayLineOut(dsb, 3);

    /* The following uses a lossless scattering junction from waveguide
     * theory.  It actually amounts to a householder mixing matrix, which
     * will produce a maximally diffuse response, and means this can probably
     * be considered a simple feed-back delay network (FDN).
     *          N
     *         ---
     *         \
     * v = 2/N /   d_i
     *         ---
     *         i=1
     */
    v = (d[0] + d[1] + d[2] + d[3]) * 0.5f;
    /* The junction is loaded with the input here. */
    v += in;

    /* Calculate the feed values for the delay lines. */
    f[0] = v - d[0];
    f[1] = v - d[1];
    f[2] = v - d[2];
    f[3] = v - d[3];

    /* Re-feed the delay lines. */
    DelayLineIn(&dsb->eax.Early.Delay[0], dsb->eax.Offset, f[0]);
    DelayLineIn(&dsb->eax.Early.Delay[1], dsb->eax.Offset, f[1]);
    DelayLineIn(&dsb->eax.Early.Delay[2], dsb->eax.Offset, f[2]);
    DelayLineIn(&dsb->eax.Early.Delay[3], dsb->eax.Offset, f[3]);

    /* Output the results of the junction for all four channels. */
    out[0] = dsb->eax.Early.Gain * f[0];
    out[1] = dsb->eax.Early.Gain * f[1];
    out[2] = dsb->eax.Early.Gain * f[2];
    out[3] = dsb->eax.Early.Gain * f[3];
}

static float AllpassInOut(DelayLine *Delay, unsigned int outOffset, unsigned int inOffset, float in, float feedCoeff, float coeff)
{
    float out, feed;

    out = DelayLineOut(Delay, outOffset);
    feed = feedCoeff * in;
    DelayLineIn(Delay, inOffset, (feedCoeff * (out - feed)) + in);

    /* The time-based attenuation is only applied to the delay output to
     * keep it from affecting the feed-back path (which is already controlled
     * by the all-pass feed coefficient). */
    return (coeff * out) - feed;
}

static float LateAllPassInOut(IDirectSoundBufferImpl* dsb, unsigned int index, float in)
{
    return AllpassInOut(&dsb->eax.Late.ApDelay[index],
                        dsb->eax.Offset - dsb->eax.Late.ApOffset[index],
                        dsb->eax.Offset, in, dsb->eax.Late.ApFeedCoeff,
                        dsb->eax.Late.ApCoeff[index]);
}

static float LateDelayLineOut(IDirectSoundBufferImpl* dsb, unsigned int index)
{
    return AttenuatedDelayLineOut(&dsb->eax.Late.Delay[index],
                                  dsb->eax.Offset - dsb->eax.Late.Offset[index],
                                  dsb->eax.Late.Coeff[index]);
}

static float LateLowPassInOut(IDirectSoundBufferImpl* dsb, unsigned int index, float in)
{
    in = lerp(in, dsb->eax.Late.LpSample[index], dsb->eax.Late.LpCoeff[index]);
    dsb->eax.Late.LpSample[index] = in;
    return in;
}

static void LateReverb(IDirectSoundBufferImpl* dsb, const float *in, float *out)
{
    float d[4], f[4];

    /* Obtain the decayed results of the cyclical delay lines, and add the
     * corresponding input channels.  Then pass the results through the
     * low-pass filters. */

    /* This is where the feed-back cycles from line 0 to 1 to 3 to 2 and back
     * to 0. */
    d[0] = LateLowPassInOut(dsb, 2, in[2] + LateDelayLineOut(dsb, 2));
    d[1] = LateLowPassInOut(dsb, 0, in[0] + LateDelayLineOut(dsb, 0));
    d[2] = LateLowPassInOut(dsb, 3, in[3] + LateDelayLineOut(dsb, 3));
    d[3] = LateLowPassInOut(dsb, 1, in[1] + LateDelayLineOut(dsb, 1));

    /* To help increase diffusion, run each line through an all-pass filter.
     * When there is no diffusion, the shortest all-pass filter will feed the
     * shortest delay line. */
    d[0] = LateAllPassInOut(dsb, 0, d[0]);
    d[1] = LateAllPassInOut(dsb, 1, d[1]);
    d[2] = LateAllPassInOut(dsb, 2, d[2]);
    d[3] = LateAllPassInOut(dsb, 3, d[3]);

    /* Late reverb is done with a modified feed-back delay network (FDN)
     * topology.  Four input lines are each fed through their own all-pass
     * filter and then into the mixing matrix.  The four outputs of the
     * mixing matrix are then cycled back to the inputs.  Each output feeds
     * a different input to form a circlular feed cycle.
     *
     * The mixing matrix used is a 4D skew-symmetric rotation matrix derived
     * using a single unitary rotational parameter:
     *
     *  [  d,  a,  b,  c ]          1 = a^2 + b^2 + c^2 + d^2
     *  [ -a,  d,  c, -b ]
     *  [ -b, -c,  d,  a ]
     *  [ -c,  b, -a,  d ]
     *
     * The rotation is constructed from the effect's diffusion parameter,
     * yielding:  1 = x^2 + 3 y^2; where a, b, and c are the coefficient y
     * with differing signs, and d is the coefficient x.  The matrix is thus:
     *
     *  [  x,  y, -y,  y ]          n = sqrt(matrix_order - 1)
     *  [ -y,  x,  y,  y ]          t = diffusion_parameter * atan(n)
     *  [  y, -y,  x,  y ]          x = cos(t)
     *  [ -y, -y, -y,  x ]          y = sin(t) / n
     *
     * To reduce the number of multiplies, the x coefficient is applied with
     * the cyclical delay line coefficients.  Thus only the y coefficient is
     * applied when mixing, and is modified to be:  y / x.
     */
    f[0] = d[0] + (dsb->eax.Late.MixCoeff * (         d[1] + -d[2] + d[3]));
    f[1] = d[1] + (dsb->eax.Late.MixCoeff * (-d[0]         +  d[2] + d[3]));
    f[2] = d[2] + (dsb->eax.Late.MixCoeff * ( d[0] + -d[1]         + d[3]));
    f[3] = d[3] + (dsb->eax.Late.MixCoeff * (-d[0] + -d[1] + -d[2]       ));

    /* Output the results of the matrix for all four channels, attenuated by
     * the late reverb gain (which is attenuated by the 'x' mix coefficient). */
    out[0] = dsb->eax.Late.Gain * f[0];
    out[1] = dsb->eax.Late.Gain * f[1];
    out[2] = dsb->eax.Late.Gain * f[2];
    out[3] = dsb->eax.Late.Gain * f[3];

    /* Re-feed the cyclical delay lines. */
    DelayLineIn(&dsb->eax.Late.Delay[0], dsb->eax.Offset, f[0]);
    DelayLineIn(&dsb->eax.Late.Delay[1], dsb->eax.Offset, f[1]);
    DelayLineIn(&dsb->eax.Late.Delay[2], dsb->eax.Offset, f[2]);
    DelayLineIn(&dsb->eax.Late.Delay[3], dsb->eax.Offset, f[3]);
}

static void VerbPass(IDirectSoundBufferImpl* dsb, float in, float* out)
{
    float feed, late[4], taps[4];

    /* Low-pass filter the incoming sample. */
    in = lpFilter2P(&dsb->eax.LpFilter, 0, in);

    /* Feed the initial delay line. */
    DelayLineIn(&dsb->eax.Delay, dsb->eax.Offset, in);

    /* Calculate the early reflection from the first delay tap. */
    in = DelayLineOut(&dsb->eax.Delay, dsb->eax.Offset - dsb->eax.DelayTap[0]);
    EarlyReflection(dsb, in, out);

    /* Feed the decorrelator from the energy-attenuated output of the second
     * delay tap. */
    in = DelayLineOut(&dsb->eax.Delay, dsb->eax.Offset - dsb->eax.DelayTap[1]);
    feed = in * dsb->eax.Late.DensityGain;
    DelayLineIn(&dsb->eax.Decorrelator, dsb->eax.Offset, feed);

    /* Calculate the late reverb from the decorrelator taps. */
    taps[0] = feed;
    taps[1] = DelayLineOut(&dsb->eax.Decorrelator, dsb->eax.Offset - dsb->eax.DecoTap[0]);
    taps[2] = DelayLineOut(&dsb->eax.Decorrelator, dsb->eax.Offset - dsb->eax.DecoTap[1]);
    taps[3] = DelayLineOut(&dsb->eax.Decorrelator, dsb->eax.Offset - dsb->eax.DecoTap[2]);
    LateReverb(dsb, taps, late);

    /* Mix early reflections and late reverb. */
    out[0] += late[0];
    out[1] += late[1];
    out[2] += late[2];
    out[3] += late[3];

    /* Step all delays forward one sample. */
    dsb->eax.Offset++;
}

static unsigned int fastf2u(float f)
{
    return (unsigned int)f;
}

void process_eax_buffer(IDirectSoundBufferImpl *dsb, float *buf, DWORD count)
{
    int i;
    float* out;
    float gain;

    if (dsb->device->eax.volume == 0.0f)
        return;

    if (dsb->mix_channels > 1) {
        WARN("EAX not yet supported for non-mono sources\n");
        return;
    }

    if (dsb->eax.reverb_update) {
        dsb->eax.reverb_update = FALSE;
        ReverbUpdate(dsb);
    }

    out = HeapAlloc(GetProcessHeap(), 0, sizeof(float)*count*4);

    for (i = 0; i < count; i++) {
        VerbPass(dsb, buf[i], &out[i*4]);
    }

    if (dsb->eax.reverb_mix == EAX_REVERBMIX_USEDISTANCE)
        gain = 1.0f; /* FIXME - should be calculated from distance */
    else
        gain = dsb->eax.reverb_mix;

    for (i = 0; i < count; i++) {
        buf[i] += gain * out[i*4];
    }

    HeapFree(GetProcessHeap(), 0, out);
}

static void UpdateDelayLine(float earlyDelay, float lateDelay, unsigned int frequency, eax_buffer_info *State)
{
    State->DelayTap[0] = fastf2u(earlyDelay * frequency);
    State->DelayTap[1] = fastf2u((earlyDelay + lateDelay) * frequency);
}

static float CalcDecayCoeff(float length, float decayTime)
{
    return powf(0.001f/*-60 dB*/, length/decayTime);
}

static void UpdateEarlyLines(float reverbGain, float earlyGain, float lateDelay, eax_buffer_info *State)
{
    unsigned int index;

    /* Calculate the early reflections gain (from the master effect gain, and
     * reflections gain parameters) with a constant attenuation of 0.5. */
    State->Early.Gain = 0.5f * reverbGain * earlyGain;

    /* Calculate the gain (coefficient) for each early delay line using the
     * late delay time.  This expands the early reflections to the start of
     * the late reverb. */
    for(index = 0; index < 4; index++)
        State->Early.Coeff[index] = CalcDecayCoeff(EARLY_LINE_LENGTH[index],
                                                   lateDelay);
}

static float lpCoeffCalc(float g, float cw)
{
    float a = 0.0f;

    if (g < 0.9999f) /* 1-epsilon */
    {
        /* Be careful with gains < 0.001, as that causes the coefficient head
         * towards 1, which will flatten the signal */
        if (g < 0.001f) g = 0.001f;
        a = (1 - g*cw - sqrtf(2*g*(1-cw) - g*g*(1 - cw*cw))) /
            (1 - g);
    }

    return a;
}

static void UpdateDecorrelator(float density, unsigned int frequency, eax_buffer_info *State)
{
    unsigned int index;
    float length;

    /* The late reverb inputs are decorrelated to smooth the reverb tail and
     * reduce harsh echos.  The first tap occurs immediately, while the
     * remaining taps are delayed by multiples of a fraction of the smallest
     * cyclical delay time.
     *
     * offset[index] = (FRACTION (MULTIPLIER^index)) smallest_delay
     */
    for (index = 0; index < 3; index++)
    {
        length = (DECO_FRACTION * powf(DECO_MULTIPLIER, (float)index)) *
                 LATE_LINE_LENGTH[0] * (1.0f + (density * LATE_LINE_MULTIPLIER));
        State->DecoTap[index] = fastf2u(length * frequency);
    }
}

static float CalcI3DL2HFreq(float hfRef, unsigned int frequency)
{
    return cosf(M_PI*2.0f * hfRef / frequency);
}

static float CalcDensityGain(float a)
{
    return sqrtf(1.0f - (a * a));
}

static float CalcDampingCoeff(float hfRatio, float length, float decayTime, float decayCoeff, float cw)
{
    float coeff, g;

    coeff = 0.0f;
    if (hfRatio < 1.0f)
    {
        /* Calculate the low-pass coefficient by dividing the HF decay
         * coefficient by the full decay coefficient. */
        g = CalcDecayCoeff(length, decayTime * hfRatio) / decayCoeff;

        /* Damping is done with a 1-pole filter, so g needs to be squared. */
        g *= g;
        coeff = lpCoeffCalc(g, cw);

        /* Very low decay times will produce minimal output, so apply an
         * upper bound to the coefficient. */
        if (coeff > 0.98f) coeff = 0.98f;
    }
    return coeff;
}

static void UpdateLateLines(float reverbGain, float lateGain, float xMix, float density, float decayTime,
                            float diffusion, float hfRatio, float cw, unsigned int frequency, eax_buffer_info *State)
{
    float length;
    unsigned int index;

    /* Calculate the late reverb gain (from the master effect gain, and late
     * reverb gain parameters).  Since the output is tapped prior to the
     * application of the next delay line coefficients, this gain needs to be
     * attenuated by the 'x' mixing matrix coefficient as well.
     */
    State->Late.Gain = reverbGain * lateGain * xMix;

    /* To compensate for changes in modal density and decay time of the late
     * reverb signal, the input is attenuated based on the maximal energy of
     * the outgoing signal.  This approximation is used to keep the apparent
     * energy of the signal equal for all ranges of density and decay time.
     *
     * The average length of the cyclcical delay lines is used to calculate
     * the attenuation coefficient.
     */
    length = (LATE_LINE_LENGTH[0] + LATE_LINE_LENGTH[1] +
              LATE_LINE_LENGTH[2] + LATE_LINE_LENGTH[3]) / 4.0f;
    length *= 1.0f + (density * LATE_LINE_MULTIPLIER);
    State->Late.DensityGain = CalcDensityGain(CalcDecayCoeff(length,
                                                             decayTime));

    /* Calculate the all-pass feed-back and feed-forward coefficient. */
    State->Late.ApFeedCoeff = 0.5f * powf(diffusion, 2.0f);

    for(index = 0; index < 4; index++)
    {
        /* Calculate the gain (coefficient) for each all-pass line. */
        State->Late.ApCoeff[index] = CalcDecayCoeff(ALLPASS_LINE_LENGTH[index],
                                                    decayTime);

        /* Calculate the length (in seconds) of each cyclical delay line. */
        length = LATE_LINE_LENGTH[index] * (1.0f + (density * LATE_LINE_MULTIPLIER));

        /* Calculate the delay offset for each cyclical delay line. */
        State->Late.Offset[index] = fastf2u(length * frequency);

        /* Calculate the gain (coefficient) for each cyclical line. */
        State->Late.Coeff[index] = CalcDecayCoeff(length, decayTime);

        /* Calculate the damping coefficient for each low-pass filter. */
        State->Late.LpCoeff[index] = CalcDampingCoeff(hfRatio, length, decayTime,
                             State->Late.Coeff[index], cw);

        /* Attenuate the cyclical line coefficients by the mixing coefficient
         * (x). */
        State->Late.Coeff[index] *= xMix;
    }
}

static void CalcMatrixCoeffs(float diffusion, float *x, float *y)
{
    float n, t;

    /* The matrix is of order 4, so n is sqrt (4 - 1). */
    n = sqrtf(3.0f);
    t = diffusion * atanf(n);

    /* Calculate the first mixing matrix coefficient. */
    *x = cosf(t);
    /* Calculate the second mixing matrix coefficient. */
    *y = sinf(t) / n;
}

static unsigned int NextPowerOf2(unsigned int value)
{
    if (value > 0)
    {
        value--;
        value |= value>>1;
        value |= value>>2;
        value |= value>>4;
        value |= value>>8;
        value |= value>>16;
    }
    return value+1;
}

static unsigned int CalcLineLength(float length, unsigned int offset, unsigned int frequency, DelayLine *Delay)
{
    unsigned int samples;

    /* All line lengths are powers of 2, calculated from their lengths, with
     * an additional sample in case of rounding errors. */
    samples = NextPowerOf2(fastf2u(length * frequency) + 1);
    /* All lines share a single sample buffer. */
    Delay->Mask = samples - 1;
    Delay->Line = (float *)(ULONG_PTR)offset;
    /* Return the sample count for accumulation. */
    return samples;
}

static void RealizeLineOffset(float *sampleBuffer, DelayLine *Delay)
{
    Delay->Line = &sampleBuffer[(unsigned int)(ULONG_PTR)Delay->Line];
}

static BOOL AllocLines(unsigned int frequency, IDirectSoundBufferImpl *dsb)
{
    unsigned int totalSamples, index;
    float length;
    float *newBuffer = NULL;

    /* All delay line lengths are calculated to accomodate the full range of
     * lengths given their respective paramters. */
    totalSamples = 0;

    /* The initial delay is the sum of the reflections and late reverb
     * delays. */
    length = AL_EAXREVERB_MAX_REFLECTIONS_DELAY +
             AL_EAXREVERB_MAX_LATE_REVERB_DELAY;
    totalSamples += CalcLineLength(length, totalSamples, frequency,
                                   &dsb->eax.Delay);

    /* The early reflection lines. */
    for (index = 0; index < 4; index++)
        totalSamples += CalcLineLength(EARLY_LINE_LENGTH[index], totalSamples,
                                       frequency, &dsb->eax.Early.Delay[index]);

    /* The decorrelator line is calculated from the lowest reverb density (a
     * parameter value of 1). */
    length = (DECO_FRACTION * DECO_MULTIPLIER * DECO_MULTIPLIER) *
             LATE_LINE_LENGTH[0] * (1.0f + LATE_LINE_MULTIPLIER);
    totalSamples += CalcLineLength(length, totalSamples, frequency,
                                   &dsb->eax.Decorrelator);

    /* The late all-pass lines. */
    for(index = 0;index < 4;index++)
        totalSamples += CalcLineLength(ALLPASS_LINE_LENGTH[index], totalSamples,
                                       frequency, &dsb->eax.Late.ApDelay[index]);

    /* The late delay lines are calculated from the lowest reverb density. */
    for (index = 0; index < 4; index++)
    {
        length = LATE_LINE_LENGTH[index] * (1.0f + LATE_LINE_MULTIPLIER);
        totalSamples += CalcLineLength(length, totalSamples, frequency,
                                       &dsb->eax.Late.Delay[index]);
    }

    if (totalSamples != dsb->eax.TotalSamples)
    {
        TRACE("New reverb buffer length: %u samples (%f sec)\n", totalSamples, totalSamples/(float)frequency);

        if (dsb->eax.SampleBuffer)
            newBuffer = HeapReAlloc(GetProcessHeap(), 0, dsb->eax.SampleBuffer, sizeof(float) * totalSamples);
        else
            newBuffer = HeapAlloc(GetProcessHeap(), 0, sizeof(float) * totalSamples);

        if (newBuffer == NULL)
            return FALSE;
        dsb->eax.SampleBuffer = newBuffer;
        dsb->eax.TotalSamples = totalSamples;
    }

    /* Update all delays to reflect the new sample buffer. */
    RealizeLineOffset(dsb->eax.SampleBuffer, &dsb->eax.Delay);
    RealizeLineOffset(dsb->eax.SampleBuffer, &dsb->eax.Decorrelator);
    for(index = 0; index < 4; index++)
    {
        RealizeLineOffset(dsb->eax.SampleBuffer, &dsb->eax.Early.Delay[index]);
        RealizeLineOffset(dsb->eax.SampleBuffer, &dsb->eax.Late.ApDelay[index]);
        RealizeLineOffset(dsb->eax.SampleBuffer, &dsb->eax.Late.Delay[index]);
    }

    /* Clear the sample buffer. */
    for (index = 0; index < dsb->eax.TotalSamples; index++)
        dsb->eax.SampleBuffer[index] = 0.0f;

    return TRUE;
}

static inline float CalcDecayLength(float coeff, float decayTime)
{
    return log10f(coeff) * decayTime / log10f(0.001f)/*-60 dB*/;
}

static float CalcLimitedHfRatio(float hfRatio, float airAbsorptionGainHF, float decayTime)
{
    float limitRatio;

    /* Find the attenuation due to air absorption in dB (converting delay
     * time to meters using the speed of sound).  Then reversing the decay
     * equation, solve for HF ratio.  The delay length is cancelled out of
     * the equation, so it can be calculated once for all lines.
     */
    limitRatio = 1.0f / (CalcDecayLength(airAbsorptionGainHF, decayTime) *
                         SPEEDOFSOUNDMETRESPERSEC);
    /* Using the limit calculated above, apply the upper bound to the HF
     * ratio. Also need to limit the result to a minimum of 0.1, just like the
     * HF ratio parameter. */
    if (limitRatio < 0.1f) limitRatio = 0.1f;
    else if (limitRatio > hfRatio) limitRatio = hfRatio;

    return limitRatio;
}

static void ReverbUpdate(IDirectSoundBufferImpl *dsb)
{
    unsigned int index;
    float cw, hfRatio, x, y;

    /* only called from the mixer thread, no race-conditions possible */
    AllocLines(dsb->device->pwfx->nSamplesPerSec, dsb);

    for(index = 0; index < 4; index++)
    {
        dsb->eax.Early.Offset[index] = fastf2u(EARLY_LINE_LENGTH[index] * dsb->device->pwfx->nSamplesPerSec);
        dsb->eax.Late.ApOffset[index] = fastf2u(ALLPASS_LINE_LENGTH[index] * dsb->device->pwfx->nSamplesPerSec);
    }

    cw = CalcI3DL2HFreq(dsb->device->eax.eax_props.flHFReference, dsb->device->pwfx->nSamplesPerSec);

    dsb->eax.LpFilter.coeff = lpCoeffCalc(dsb->device->eax.eax_props.flGainHF, cw);

    UpdateDelayLine(dsb->device->eax.eax_props.flReflectionsDelay,
                    dsb->device->eax.eax_props.flLateReverbDelay,
                    dsb->device->pwfx->nSamplesPerSec, &dsb->eax);

    UpdateEarlyLines(dsb->device->eax.eax_props.flGain,
                    dsb->device->eax.eax_props.flReflectionsGain,
                    dsb->device->eax.eax_props.flLateReverbDelay, &dsb->eax);

    UpdateDecorrelator(dsb->device->eax.eax_props.flDensity, dsb->device->pwfx->nSamplesPerSec, &dsb->eax);

    CalcMatrixCoeffs(dsb->device->eax.eax_props.flDiffusion, &x, &y);
    dsb->eax.Late.MixCoeff = y / x;

    hfRatio = dsb->device->eax.eax_props.flDecayHFRatio;

    if (dsb->device->eax.eax_props.iDecayHFLimit && dsb->device->eax.eax_props.flAirAbsorptionGainHF < 1.0f) {
        hfRatio = CalcLimitedHfRatio(hfRatio, dsb->device->eax.eax_props.flAirAbsorptionGainHF,
                                     dsb->device->eax.eax_props.flDecayTime);
    }

    UpdateLateLines(dsb->device->eax.eax_props.flGain, dsb->device->eax.eax_props.flLateReverbGain,
                    x, dsb->device->eax.eax_props.flDensity, dsb->device->eax.eax_props.flDecayTime,
                    dsb->device->eax.eax_props.flDiffusion, hfRatio, cw, dsb->device->pwfx->nSamplesPerSec, &dsb->eax);
}

static BOOL ReverbDeviceUpdate(DirectSoundDevice *dev)
{
    int i;

    RtlAcquireResourceShared(&dev->buffer_list_lock, TRUE);
    for (i = 0; i < dev->nrofbuffers; i++) {
        dev->buffers[i]->eax.reverb_update = TRUE;
    }
    RtlReleaseResource(&dev->buffer_list_lock);

    return TRUE;
}

void init_eax_device(DirectSoundDevice *dev)
{
    dev->eax.environment = presets[0].environment;
    dev->eax.volume = presets[0].fVolume;
    dev->eax.damping = presets[0].fDamping;
    memcpy(&dev->eax.eax_props, &efx_presets[0], sizeof(dev->eax.eax_props));
    dev->eax.eax_props.flDecayTime = presets[0].fDecayTime_sec;
}

void init_eax_buffer(IDirectSoundBufferImpl *dsb)
{
    unsigned int index;

    dsb->eax.reverb_update = TRUE;
    dsb->eax.reverb_mix = EAX_REVERBMIX_USEDISTANCE;

    dsb->eax.SampleBuffer = NULL;
    dsb->eax.TotalSamples = 0;

    dsb->eax.LpFilter.coeff = 0.0f;
    dsb->eax.LpFilter.history[0] = 0.0f;
    dsb->eax.LpFilter.history[1] = 0.0f;

    dsb->eax.Delay.Mask = 0;
    dsb->eax.Delay.Line = NULL;
    dsb->eax.DelayTap[0] = 0;
    dsb->eax.DelayTap[1] = 0;

    dsb->eax.Early.Gain = 0.0f;
    for(index = 0; index < 4; index++)
    {
        dsb->eax.Early.Coeff[index] = 0.0f;
        dsb->eax.Early.Delay[index].Mask = 0;
        dsb->eax.Early.Delay[index].Line = NULL;
        dsb->eax.Early.Offset[index] = 0;
    }

    dsb->eax.Decorrelator.Mask = 0;
    dsb->eax.Decorrelator.Line = NULL;
    dsb->eax.DecoTap[0] = 0;
    dsb->eax.DecoTap[1] = 0;
    dsb->eax.DecoTap[2] = 0;

    dsb->eax.Late.Gain = 0.0f;
    dsb->eax.Late.DensityGain = 0.0f;
    dsb->eax.Late.ApFeedCoeff = 0.0f;
    dsb->eax.Late.MixCoeff = 0.0f;
    for(index = 0; index < 4; index++)
    {
        dsb->eax.Late.ApCoeff[index] = 0.0f;
        dsb->eax.Late.ApDelay[index].Mask = 0;
        dsb->eax.Late.ApDelay[index].Line = NULL;
        dsb->eax.Late.ApOffset[index] = 0;

        dsb->eax.Late.Coeff[index] = 0.0f;
        dsb->eax.Late.Delay[index].Mask = 0;
        dsb->eax.Late.Delay[index].Line = NULL;
        dsb->eax.Late.Offset[index] = 0;

        dsb->eax.Late.LpCoeff[index] = 0.0f;
        dsb->eax.Late.LpSample[index] = 0.0f;
    }

    dsb->eax.Offset = 0;
}

void free_eax_buffer(IDirectSoundBufferImpl *dsb)
{
    if (dsb->eax.SampleBuffer)
        HeapFree(GetProcessHeap(), 0, dsb->eax.SampleBuffer);
}

BOOL WINAPI EAX_QuerySupport(REFGUID guidPropSet, ULONG dwPropID, ULONG *pTypeSupport)
{
    TRACE("(%s,%d,%p)\n", debugstr_guid(guidPropSet), dwPropID, pTypeSupport);

    if (!ds_eax_enabled)
        return FALSE;

    if (IsEqualGUID(&DSPROPSETID_EAX_ReverbProperties, guidPropSet)) {
        if (dwPropID <= DSPROPERTY_EAX_DAMPING) {
            *pTypeSupport = KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET;
            return TRUE;
        }
    } else if (IsEqualGUID(&DSPROPSETID_EAXBUFFER_ReverbProperties, guidPropSet)) {
        if (dwPropID <= DSPROPERTY_EAXBUFFER_REVERBMIX) {
            *pTypeSupport = KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET;
            return TRUE;
        }
    } else if (IsEqualGUID(&DSPROPSETID_EAX20_ListenerProperties, guidPropSet)) {
        if (dwPropID <= DSPROPERTY_EAX20LISTENER_FLAGS) {
            *pTypeSupport = KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET;
            return TRUE;
        }
    } else if (IsEqualGUID(&DSPROPSETID_EAX20_BufferProperties, guidPropSet)) {
        if (dwPropID <= DSPROPERTY_EAX20BUFFER_FLAGS) {
            *pTypeSupport = KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET;
            return TRUE;
        }
    }

    FIXME("(%s,%d,%p)\n", debugstr_guid(guidPropSet), dwPropID, pTypeSupport);
    return FALSE;
}

HRESULT WINAPI EAX_Get(IDirectSoundBufferImpl *buf, REFGUID guidPropSet,
        ULONG dwPropID, void *pInstanceData, ULONG cbInstanceData, void *pPropData,
        ULONG cbPropData, ULONG *pcbReturned)
{
    TRACE("(buf=%p,guidPropSet=%s,dwPropID=%d,pInstanceData=%p,cbInstanceData=%d,pPropData=%p,cbPropData=%d,pcbReturned=%p)\n",
        buf, debugstr_guid(guidPropSet), dwPropID, pInstanceData, cbInstanceData, pPropData, cbPropData, pcbReturned);

    if (!ds_eax_enabled)
        return E_PROP_ID_UNSUPPORTED;

    *pcbReturned = 0;

    if (IsEqualGUID(&DSPROPSETID_EAX_ReverbProperties, guidPropSet)) {
        EAX_REVERBPROPERTIES *props;
        buf->device->eax.using_eax = TRUE;

        switch (dwPropID) {
            case DSPROPERTY_EAX_ALL:
                if (cbPropData < sizeof(EAX_REVERBPROPERTIES))
                    return E_FAIL;

                props = pPropData;

                props->environment = buf->device->eax.environment;
                props->fVolume = buf->device->eax.volume;
                props->fDecayTime_sec = buf->device->eax.eax_props.flDecayTime;
                props->fDamping = buf->device->eax.damping;

                *pcbReturned = sizeof(EAX_REVERBPROPERTIES);
            break;

            case DSPROPERTY_EAX_ENVIRONMENT:
                if (cbPropData < sizeof(unsigned long))
                    return E_FAIL;

                *(unsigned long*)pPropData = buf->device->eax.environment;

                *pcbReturned = sizeof(unsigned long);
            break;

            case DSPROPERTY_EAX_VOLUME:
                if (cbPropData < sizeof(float))
                    return E_FAIL;

                *(float*)pPropData = buf->device->eax.volume;

                *pcbReturned = sizeof(float);
            break;

            case DSPROPERTY_EAX_DECAYTIME:
                if (cbPropData < sizeof(float))
                    return E_FAIL;

                *(float*)pPropData = buf->device->eax.eax_props.flDecayTime;

                *pcbReturned = sizeof(float);
            break;

            case DSPROPERTY_EAX_DAMPING:
                if (cbPropData < sizeof(float))
                    return E_FAIL;

                *(float*)pPropData = buf->device->eax.damping;

                *pcbReturned = sizeof(float);
            break;

            default:
                return E_PROP_ID_UNSUPPORTED;
        }

        return S_OK;
    } else if (IsEqualGUID(&DSPROPSETID_EAXBUFFER_ReverbProperties, guidPropSet)) {
        EAXBUFFER_REVERBPROPERTIES *props;
        buf->device->eax.using_eax = TRUE;

        switch (dwPropID) {
            case DSPROPERTY_EAXBUFFER_ALL:
                if (cbPropData < sizeof(EAXBUFFER_REVERBPROPERTIES))
                    return E_FAIL;

                props = pPropData;

                props->fMix = buf->eax.reverb_mix;

                *pcbReturned = sizeof(EAXBUFFER_REVERBPROPERTIES);
            break;

            case DSPROPERTY_EAXBUFFER_REVERBMIX:
                if (cbPropData < sizeof(float))
                    return E_FAIL;

                *(float*)pPropData = buf->eax.reverb_mix;

                *pcbReturned = sizeof(float);
            break;

            default:
                return E_PROP_ID_UNSUPPORTED;
        }

        return S_OK;
    } else if (IsEqualGUID(&DSPROPSETID_EAX20_ListenerProperties, guidPropSet)) {
        FIXME("Unsupported DSPROPSETID_EAX20_ListenerProperties: %d\n", dwPropID);
        return E_PROP_ID_UNSUPPORTED;
    } else if (IsEqualGUID(&DSPROPSETID_EAX20_BufferProperties, guidPropSet)) {
        FIXME("Unsupported DSPROPSETID_EAX20_BufferProperties: %d\n", dwPropID);
        return E_PROP_ID_UNSUPPORTED;
    }

    FIXME("(buf=%p,guidPropSet=%s,dwPropID=%d,pInstanceData=%p,cbInstanceData=%d,pPropData=%p,cbPropData=%d,pcbReturned=%p)\n",
        buf, debugstr_guid(guidPropSet), dwPropID, pInstanceData, cbInstanceData, pPropData, cbPropData, pcbReturned);
    return E_PROP_ID_UNSUPPORTED;
}

HRESULT WINAPI EAX_Set(IDirectSoundBufferImpl *buf, REFGUID guidPropSet,
        ULONG dwPropID, void *pInstanceData, ULONG cbInstanceData, void *pPropData,
        ULONG cbPropData)
{
    EAX_REVERBPROPERTIES *props;

    TRACE("(%p,%s,%d,%p,%d,%p,%d)\n",
        buf, debugstr_guid(guidPropSet), dwPropID, pInstanceData, cbInstanceData, pPropData, cbPropData);

    if (!ds_eax_enabled)
        return E_PROP_ID_UNSUPPORTED;

    if (IsEqualGUID(&DSPROPSETID_EAX_ReverbProperties, guidPropSet)) {
        buf->device->eax.using_eax = TRUE;

        switch (dwPropID) {
            case DSPROPERTY_EAX_ALL:
                if (cbPropData != sizeof(EAX_REVERBPROPERTIES))
                    return E_FAIL;

                props = pPropData;

                TRACE("setting environment = %lu, fVolume = %f, fDecayTime_sec = %f, fDamping = %f\n",
                      props->environment, props->fVolume, props->fDecayTime_sec,
                      props->fDamping);

                buf->device->eax.environment = props->environment;

                if (buf->device->eax.environment < EAX_ENVIRONMENT_COUNT)
                    memcpy(&buf->device->eax.eax_props, &efx_presets[buf->device->eax.environment], sizeof(buf->device->eax.eax_props));

                buf->device->eax.volume = props->fVolume;
                buf->device->eax.eax_props.flDecayTime = props->fDecayTime_sec;
                buf->device->eax.damping = props->fDamping;

                ReverbDeviceUpdate(buf->device);
            break;

            case DSPROPERTY_EAX_ENVIRONMENT:
                if (cbPropData != sizeof(unsigned long))
                    return E_FAIL;

                TRACE("setting environment to %lu\n", *(unsigned long*)pPropData);

                buf->device->eax.environment = *(unsigned long*)pPropData;

                if (buf->device->eax.environment < EAX_ENVIRONMENT_COUNT) {
                    memcpy(&buf->device->eax.eax_props, &efx_presets[buf->device->eax.environment], sizeof(buf->device->eax.eax_props));
                    buf->device->eax.volume = presets[buf->device->eax.environment].fVolume;
                    buf->device->eax.eax_props.flDecayTime = presets[buf->device->eax.environment].fDecayTime_sec;
                    buf->device->eax.damping = presets[buf->device->eax.environment].fDamping;
                }

                ReverbDeviceUpdate(buf->device);
            break;

            case DSPROPERTY_EAX_VOLUME:
                if (cbPropData != sizeof(float))
                    return E_FAIL;

                TRACE("setting volume to %f\n", *(float*)pPropData);

                buf->device->eax.volume = *(float*)pPropData;

                ReverbDeviceUpdate(buf->device);
            break;

            case DSPROPERTY_EAX_DECAYTIME:
                if (cbPropData != sizeof(float))
                    return E_FAIL;

                TRACE("setting decay time to %f\n", *(float*)pPropData);

                buf->device->eax.eax_props.flDecayTime = *(float*)pPropData;

                ReverbDeviceUpdate(buf->device);
            break;

            case DSPROPERTY_EAX_DAMPING:
                if (cbPropData != sizeof(float))
                    return E_FAIL;

                TRACE("setting damping to %f\n", *(float*)pPropData);

                buf->device->eax.damping = *(float*)pPropData;

                ReverbDeviceUpdate(buf->device);
            break;

            default:
                return E_PROP_ID_UNSUPPORTED;
        }

        return S_OK;
    } else if (IsEqualGUID(&DSPROPSETID_EAXBUFFER_ReverbProperties, guidPropSet)) {
        EAXBUFFER_REVERBPROPERTIES *props;
        buf->device->eax.using_eax = TRUE;

        switch (dwPropID) {
            case DSPROPERTY_EAXBUFFER_ALL:
                if (cbPropData != sizeof(EAXBUFFER_REVERBPROPERTIES))
                    return E_FAIL;

                props = pPropData;

                TRACE("setting reverb mix to %f\n", props->fMix);

                buf->eax.reverb_mix = props->fMix;
            break;

            case DSPROPERTY_EAXBUFFER_REVERBMIX:
                if (cbPropData != sizeof(float))
                    return E_FAIL;

                TRACE("setting reverb mix to %f\n", *(float*)pPropData);

                buf->eax.reverb_mix = *(float*)pPropData;
            break;

            default:
                return E_PROP_ID_UNSUPPORTED;
        }

        return S_OK;
    } else if (IsEqualGUID(&DSPROPSETID_EAX20_ListenerProperties, guidPropSet)) {
        FIXME("Unsupported DSPROPSETID_EAX20_ListenerProperties: %d\n", dwPropID);
        return E_PROP_ID_UNSUPPORTED;
    } else if (IsEqualGUID(&DSPROPSETID_EAX20_BufferProperties, guidPropSet)) {
        FIXME("Unsupported DSPROPSETID_EAX20_BufferProperties: %d\n", dwPropID);
        return E_PROP_ID_UNSUPPORTED;
    }

    FIXME("(%p,%s,%d,%p,%d,%p,%d)\n",
        buf, debugstr_guid(guidPropSet), dwPropID, pInstanceData, cbInstanceData, pPropData, cbPropData);
    return E_PROP_ID_UNSUPPORTED;
}
