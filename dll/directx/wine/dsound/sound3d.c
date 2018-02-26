/*  			DirectSound
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Rob Riggs
 * Copyright 2000-2001 TransGaming Technologies, Inc.
 * Copyright 2002-2003 Rok Mandeljc <rok.mandeljc@gimb.org>
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
/*
 * Most thread locking is complete. There may be a few race
 * conditions still lurking.
 *
 * Tested with a Soundblaster clone, a Gravis UltraSound Classic,
 * and a Turtle Beach Tropez+.
 *
 * TODO:
 *	Implement SetCooperativeLevel properly (need to address focus issues)
 *	Implement DirectSound3DBuffers (stubs in place)
 *	Use hardware 3D support if available
 *      Add critical section locking inside Release and AddRef methods
 *      Handle static buffers - put those in hardware, non-static not in hardware
 *      Hardware DuplicateSoundBuffer
 *      Proper volume calculation, and setting volume in HEL primary buffer
 *      Optimize WINMM and negotiate fragment size, decrease DS_HEL_MARGIN
 */

#include <stdarg.h>
#include <math.h>	/* Insomnia - pow() function */

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "mmsystem.h"
#include "winternl.h"
#include "mmddk.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsound_private.h"

/* default velocity of sound in the air */
#define DEFAULT_VELOCITY 340

WINE_DEFAULT_DEBUG_CHANNEL(dsound3d);

/*******************************************************************************
 *              Auxiliary functions
 */

/* scalar product (I believe it's called dot product in English) */
static inline D3DVALUE ScalarProduct (const D3DVECTOR *a, const D3DVECTOR *b)
{
	D3DVALUE c;
	c = (a->x*b->x) + (a->y*b->y) + (a->z*b->z);
	TRACE("(%f,%f,%f) * (%f,%f,%f) = %f)\n", a->x, a->y, a->z, b->x, b->y,
	      b->z, c);
	return c;
}

/* vector product (I believe it's called cross product in English */
static inline D3DVECTOR VectorProduct (const D3DVECTOR *a, const D3DVECTOR *b)
{
	D3DVECTOR c;
	c.x = (a->y*b->z) - (a->z*b->y);
	c.y = (a->z*b->x) - (a->x*b->z);
	c.z = (a->x*b->y) - (a->y*b->x);
	TRACE("(%f,%f,%f) x (%f,%f,%f) = (%f,%f,%f)\n", a->x, a->y, a->z, b->x, b->y,
	      b->z, c.x, c.y, c.z);
	return c;
}

/* magnitude (length) of vector */
static inline D3DVALUE VectorMagnitude (const D3DVECTOR *a)
{
	D3DVALUE l;
	l = sqrt (ScalarProduct (a, a));
	TRACE("|(%f,%f,%f)| = %f\n", a->x, a->y, a->z, l);
	return l;
}

/* conversion between radians and degrees */
static inline D3DVALUE RadToDeg (D3DVALUE angle)
{
	D3DVALUE newangle;
	newangle = angle * (360/(2*M_PI));
	TRACE("%f rad = %f deg\n", angle, newangle);
	return newangle;
}

/* angle between vectors - rad version */
static inline D3DVALUE AngleBetweenVectorsRad (const D3DVECTOR *a, const D3DVECTOR *b)
{
	D3DVALUE la, lb, product, angle, cos;
	/* definition of scalar product: a*b = |a|*|b|*cos... therefore: */
	product = ScalarProduct (a,b);
	la = VectorMagnitude (a);
	lb = VectorMagnitude (b);
	if (!la || !lb)
		return 0;

	cos = product/(la*lb);
	angle = acos(cos);
	TRACE("angle between (%f,%f,%f) and (%f,%f,%f) = %f radians (%f degrees)\n",  a->x, a->y, a->z, b->x,
	      b->y, b->z, angle, RadToDeg(angle));
	return angle;	
}

static inline D3DVALUE AngleBetweenVectorsDeg (const D3DVECTOR *a, const D3DVECTOR *b)
{
	return RadToDeg(AngleBetweenVectorsRad(a, b));
}

/* calculates vector between two points */
static inline D3DVECTOR VectorBetweenTwoPoints (const D3DVECTOR *a, const D3DVECTOR *b)
{
	D3DVECTOR c;
	c.x = b->x - a->x;
	c.y = b->y - a->y;
	c.z = b->z - a->z;
	TRACE("A (%f,%f,%f), B (%f,%f,%f), AB = (%f,%f,%f)\n", a->x, a->y, a->z, b->x, b->y,
	      b->z, c.x, c.y, c.z);
	return c;
}

/* calculates the length of vector's projection on another vector */
static inline D3DVALUE ProjectVector (const D3DVECTOR *a, const D3DVECTOR *p)
{
	D3DVALUE prod, result;
	prod = ScalarProduct(a, p);
	result = prod/VectorMagnitude(p);
	TRACE("length projection of (%f,%f,%f) on (%f,%f,%f) = %f\n", a->x, a->y, a->z, p->x,
              p->y, p->z, result);
	return result;
}

/*******************************************************************************
 *              3D Buffer and Listener mixing
 */

void DSOUND_Calc3DBuffer(IDirectSoundBufferImpl *dsb)
{
	/* volume, at which the sound will be played after all calcs. */
	D3DVALUE lVolume = 0;
	/* stuff for distance related stuff calc. */
	D3DVECTOR vDistance;
	D3DVALUE flDistance = 0;
	/* panning related stuff */
	D3DVALUE flAngle, flAngle2;
	D3DVECTOR vLeft;
	int i, num_main_speakers;
	float a, ingain;
	/* doppler shift related stuff */

	TRACE("(%p)\n",dsb);

	/* initial buffer volume */
	lVolume = dsb->ds3db_lVolume;
	
	switch (dsb->ds3db_ds3db.dwMode)
	{
		case DS3DMODE_NORMAL:
			TRACE("Normal 3D processing mode\n");
			/* we need to calculate distance between buffer and listener*/
			vDistance = VectorBetweenTwoPoints(&dsb->device->ds3dl.vPosition, &dsb->ds3db_ds3db.vPosition);
			flDistance = VectorMagnitude (&vDistance);
			break;
		case DS3DMODE_HEADRELATIVE:
			TRACE("Head-relative 3D processing mode\n");
			/* distance between buffer and listener is same as buffer's position */
			vDistance = dsb->ds3db_ds3db.vPosition;
			flDistance = VectorMagnitude (&vDistance);
			break;
		default:
			TRACE("3D processing disabled\n");
			/* this one is here only to eliminate annoying warning message */
			dsb->volpan.lVolume = dsb->ds3db_lVolume;
			DSOUND_RecalcVolPan (&dsb->volpan);
			return;
	}
	
	if (flDistance > dsb->ds3db_ds3db.flMaxDistance)
	{
		/* some apps don't want you to hear too distant sounds... */
		if (dsb->dsbd.dwFlags & DSBCAPS_MUTE3DATMAXDISTANCE)
		{
			dsb->volpan.lVolume = DSBVOLUME_MIN;
			DSOUND_RecalcVolPan (&dsb->volpan);		
			/* i guess mixing here would be a waste of power */
			return;
		}
		else
			flDistance = dsb->ds3db_ds3db.flMaxDistance;
	}		

	if (flDistance < dsb->ds3db_ds3db.flMinDistance)
		flDistance = dsb->ds3db_ds3db.flMinDistance;

	flDistance = dsb->ds3db_ds3db.flMinDistance + (flDistance - dsb->ds3db_ds3db.flMinDistance) * dsb->device->ds3dl.flRolloffFactor;
	
	/* attenuation proportional to the distance squared, converted to millibels as in lVolume*/
	lVolume -= log10(flDistance/dsb->ds3db_ds3db.flMinDistance * flDistance/dsb->ds3db_ds3db.flMinDistance)*1000;
	TRACE("dist. att: Distance = %f, MinDistance = %f => adjusting volume %d to %f\n", flDistance, dsb->ds3db_ds3db.flMinDistance, dsb->ds3db_lVolume, lVolume);

	/* conning */
	/* sometimes it happens that vConeOrientation vector = (0,0,0); in this case angle is "nan" and it's useless*/
	if (dsb->ds3db_ds3db.vConeOrientation.x == 0 && dsb->ds3db_ds3db.vConeOrientation.y == 0 && dsb->ds3db_ds3db.vConeOrientation.z == 0)
	{
		TRACE("conning: cones not set\n");
	}
	else
	{
		D3DVECTOR vDistanceInv;

		vDistanceInv.x = -vDistance.x;
		vDistanceInv.y = -vDistance.y;
		vDistanceInv.z = -vDistance.z;

		/* calculate angle */
		flAngle = AngleBetweenVectorsDeg(&dsb->ds3db_ds3db.vConeOrientation, &vDistanceInv);
		/* if by any chance it happens that OutsideConeAngle = InsideConeAngle (that means that conning has no effect) */
		if (dsb->ds3db_ds3db.dwInsideConeAngle != dsb->ds3db_ds3db.dwOutsideConeAngle)
		{
			/* my test show that for my way of calc., we need only half of angles */
			DWORD dwInsideConeAngle = dsb->ds3db_ds3db.dwInsideConeAngle/2;
			DWORD dwOutsideConeAngle = dsb->ds3db_ds3db.dwOutsideConeAngle/2;
			if (dwOutsideConeAngle == dwInsideConeAngle)
				++dwOutsideConeAngle;

			/* full volume */
			if (flAngle < dwInsideConeAngle)
				flAngle = dwInsideConeAngle;
			/* min (app defined) volume */
			if (flAngle > dwOutsideConeAngle)
				flAngle = dwOutsideConeAngle;
			/* this probably isn't the right thing, but it's ok for the time being */
			lVolume += ((flAngle - dwInsideConeAngle)/(dwOutsideConeAngle - dwInsideConeAngle)) * dsb->ds3db_ds3db.lConeOutsideVolume;
		}
		TRACE("conning: Angle = %f deg; InsideConeAngle(/2) = %d deg; OutsideConeAngle(/2) = %d deg; ConeOutsideVolume = %d => adjusting volume to %f\n",
		       flAngle, dsb->ds3db_ds3db.dwInsideConeAngle/2, dsb->ds3db_ds3db.dwOutsideConeAngle/2, dsb->ds3db_ds3db.lConeOutsideVolume, lVolume);
	}
	dsb->volpan.lVolume = lVolume;

	ingain = pow(2.0, dsb->volpan.lVolume / 600.0) * 0xffff;

	if (dsb->device->pwfx->nChannels == 1)
	{
		dsb->volpan.dwTotalAmpFactor[0] = ingain;
		return;
	}
	
	/* panning */
	if (vDistance.x == 0.0f && vDistance.y == 0.0f && vDistance.z == 0.0f)
		flAngle = 0.0;
	else
	{
		vLeft = VectorProduct(&dsb->device->ds3dl.vOrientFront, &dsb->device->ds3dl.vOrientTop);
		/* To calculate angle to sound source we need to:
		 * 1) Get angle between vDistance and a plane on which angle to sound source should be 0.
		 *    Such a plane is given by vectors vOrientFront and vOrientTop, and angle between vector
		 *    and a plane equals to M_PI_2 - angle between vector and normal to this plane (vLeft in this case).
		 * 2) Determine if the source is behind or in front of us by calculating angle between vDistance
		 *    and vOrientFront.
		 */
		flAngle = AngleBetweenVectorsRad(&vLeft, &vDistance);
		flAngle2 = AngleBetweenVectorsRad(&dsb->device->ds3dl.vOrientFront, &vDistance);
		if (flAngle2 > M_PI_2)
			flAngle = -flAngle;
		flAngle -= M_PI_2;
		if (flAngle < -M_PI)
			flAngle += 2*M_PI;
	}
	TRACE("panning: Angle = %f rad, lPan = %d\n", flAngle, dsb->volpan.lPan);

	/* FIXME: Doppler Effect disabled since i have no idea which frequency to change and how to do it */
if(0)
{
	D3DVALUE flFreq, flBufferVel, flListenerVel;
	/* doppler shift*/
	if (!VectorMagnitude(&dsb->ds3db_ds3db.vVelocity) && !VectorMagnitude(&dsb->device->ds3dl.vVelocity))
	{
		TRACE("doppler: Buffer and Listener don't have velocities\n");
	}
	else if (!(dsb->ds3db_ds3db.vVelocity.x == dsb->device->ds3dl.vVelocity.x &&
	           dsb->ds3db_ds3db.vVelocity.y == dsb->device->ds3dl.vVelocity.y &&
	           dsb->ds3db_ds3db.vVelocity.z == dsb->device->ds3dl.vVelocity.z))
	{
		/* calculate length of ds3db_ds3db.vVelocity component which causes Doppler Effect
		   NOTE: if buffer moves TOWARDS the listener, its velocity component is NEGATIVE
		         if buffer moves AWAY from listener, its velocity component is POSITIVE */
		flBufferVel = ProjectVector(&dsb->ds3db_ds3db.vVelocity, &vDistance);
		/* calculate length of ds3dl.vVelocity component which causes Doppler Effect
		   NOTE: if listener moves TOWARDS the buffer, its velocity component is POSITIVE
		         if listener moves AWAY from buffer, its velocity component is NEGATIVE */
		flListenerVel = ProjectVector(&dsb->device->ds3dl.vVelocity, &vDistance);
		/* formula taken from Gianicoli D.: Physics, 4th edition: */
		/* FIXME: replace dsb->freq with appropriate frequency ! */
		flFreq = dsb->freq * ((DEFAULT_VELOCITY + flListenerVel)/(DEFAULT_VELOCITY + flBufferVel));
		TRACE("doppler: Buffer velocity (component) = %f, Listener velocity (component) = %f => Doppler shift: %d Hz -> %f Hz\n",
		      flBufferVel, flListenerVel, dsb->freq, flFreq);
		/* FIXME: replace following line with correct frequency setting ! */
		dsb->freq = flFreq;
		DSOUND_RecalcFormat(dsb);
	}
}

	for (i = 0; i < dsb->device->pwfx->nChannels; i++)
		dsb->volpan.dwTotalAmpFactor[i] = 0;

	num_main_speakers = dsb->device->pwfx->nChannels;

	if (dsb->device->lfe_channel != -1) {
		dsb->volpan.dwTotalAmpFactor[dsb->device->lfe_channel] = ingain;
		num_main_speakers--;
	}

	/* adapted from OpenAL's Alc/panning.c */
	for (i = 0; i < num_main_speakers - 1; i++)
	{
		if(flAngle >= dsb->device->speaker_angles[i] && flAngle < dsb->device->speaker_angles[i+1])
		{
			/* Sound is between speakers i and i+1 */
			a = (flAngle-dsb->device->speaker_angles[i]) / (dsb->device->speaker_angles[i+1]-dsb->device->speaker_angles[i]);
			dsb->volpan.dwTotalAmpFactor[dsb->device->speaker_num[i]] = sqrtf(1.0f-a) * ingain;
			dsb->volpan.dwTotalAmpFactor[dsb->device->speaker_num[i+1]] = sqrtf(a) * ingain;
			return;
		}
	}

	/* Sound is between last and first speakers */
	if (flAngle < dsb->device->speaker_angles[0]) { flAngle += M_PI*2.0f; }
	a = (flAngle-dsb->device->speaker_angles[i]) / (M_PI*2.0f + dsb->device->speaker_angles[0]-dsb->device->speaker_angles[i]);
	dsb->volpan.dwTotalAmpFactor[dsb->device->speaker_num[i]] = sqrtf(1.0f-a) * ingain;
	dsb->volpan.dwTotalAmpFactor[dsb->device->speaker_num[0]] = sqrtf(a) * ingain;
}

static void DSOUND_Mix3DBuffer(IDirectSoundBufferImpl *dsb)
{
	TRACE("(%p)\n",dsb);

	DSOUND_Calc3DBuffer(dsb);
}

static void DSOUND_ChangeListener(IDirectSoundBufferImpl *ds3dl)
{
	int i;
	TRACE("(%p)\n",ds3dl);
	for (i = 0; i < ds3dl->device->nrofbuffers; i++)
	{
		/* check if this buffer is waiting for recalculation */
		if (ds3dl->device->buffers[i]->ds3db_need_recalc)
		{
			DSOUND_Mix3DBuffer(ds3dl->device->buffers[i]);
		}
	}
}

/*******************************************************************************
 *              IDirectSound3DBuffer
 */
static inline IDirectSoundBufferImpl *impl_from_IDirectSound3DBuffer(IDirectSound3DBuffer *iface)
{
    return CONTAINING_RECORD(iface, IDirectSoundBufferImpl, IDirectSound3DBuffer_iface);
}

/* IUnknown methods */
static HRESULT WINAPI IDirectSound3DBufferImpl_QueryInterface(IDirectSound3DBuffer *iface,
        REFIID riid, void **ppobj)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSound3DBuffer(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppobj);

    return IDirectSoundBuffer8_QueryInterface(&This->IDirectSoundBuffer8_iface, riid, ppobj);
}

static ULONG WINAPI IDirectSound3DBufferImpl_AddRef(IDirectSound3DBuffer *iface)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSound3DBuffer(iface);
    ULONG ref = InterlockedIncrement(&This->ref3D);

    TRACE("(%p) ref was %d\n", This, ref - 1);

    if(ref == 1)
        InterlockedIncrement(&This->numIfaces);

    return ref;
}

static ULONG WINAPI IDirectSound3DBufferImpl_Release(IDirectSound3DBuffer *iface)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSound3DBuffer(iface);
    ULONG ref = InterlockedDecrement(&This->ref3D);

    TRACE("(%p) ref was %d\n", This, ref + 1);

    if (!ref && !InterlockedDecrement(&This->numIfaces))
        secondarybuffer_destroy(This);

    return ref;
}

/* IDirectSound3DBuffer methods */
static HRESULT WINAPI IDirectSound3DBufferImpl_GetAllParameters(IDirectSound3DBuffer *iface,
	DS3DBUFFER *lpDs3dBuffer)
{
	IDirectSoundBufferImpl *This = impl_from_IDirectSound3DBuffer(iface);

	TRACE("(%p,%p)\n",This,lpDs3dBuffer);

	if (lpDs3dBuffer == NULL) {
		WARN("invalid parameter: lpDs3dBuffer == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	if (lpDs3dBuffer->dwSize < sizeof(*lpDs3dBuffer)) {
		WARN("invalid parameter: lpDs3dBuffer->dwSize = %d\n",lpDs3dBuffer->dwSize);
		return DSERR_INVALIDPARAM;
	}
	
	TRACE("returning: all parameters\n");
	*lpDs3dBuffer = This->ds3db_ds3db;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetConeAngles(IDirectSound3DBuffer *iface,
        DWORD *lpdwInsideConeAngle, DWORD *lpdwOutsideConeAngle)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSound3DBuffer(iface);

    TRACE("returning: Inside Cone Angle = %d degrees; Outside Cone Angle = %d degrees\n",
            This->ds3db_ds3db.dwInsideConeAngle, This->ds3db_ds3db.dwOutsideConeAngle);
    *lpdwInsideConeAngle = This->ds3db_ds3db.dwInsideConeAngle;
    *lpdwOutsideConeAngle = This->ds3db_ds3db.dwOutsideConeAngle;
    return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetConeOrientation(IDirectSound3DBuffer *iface,
        D3DVECTOR *lpvConeOrientation)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSound3DBuffer(iface);

    TRACE("returning: Cone Orientation vector = (%f,%f,%f)\n",
            This->ds3db_ds3db.vConeOrientation.x,
            This->ds3db_ds3db.vConeOrientation.y,
            This->ds3db_ds3db.vConeOrientation.z);
    *lpvConeOrientation = This->ds3db_ds3db.vConeOrientation;
    return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetConeOutsideVolume(IDirectSound3DBuffer *iface,
        LONG *lplConeOutsideVolume)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSound3DBuffer(iface);

    TRACE("returning: Cone Outside Volume = %d\n", This->ds3db_ds3db.lConeOutsideVolume);
    *lplConeOutsideVolume = This->ds3db_ds3db.lConeOutsideVolume;
    return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetMaxDistance(IDirectSound3DBuffer *iface,
        D3DVALUE *lpfMaxDistance)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSound3DBuffer(iface);

    TRACE("returning: Max Distance = %f\n", This->ds3db_ds3db.flMaxDistance);
    *lpfMaxDistance = This->ds3db_ds3db.flMaxDistance;
    return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetMinDistance(IDirectSound3DBuffer *iface,
        D3DVALUE *lpfMinDistance)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSound3DBuffer(iface);

    TRACE("returning: Min Distance = %f\n", This->ds3db_ds3db.flMinDistance);
    *lpfMinDistance = This->ds3db_ds3db.flMinDistance;
    return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetMode(IDirectSound3DBuffer *iface,
        DWORD *lpdwMode)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSound3DBuffer(iface);

    TRACE("returning: Mode = %d\n", This->ds3db_ds3db.dwMode);
    *lpdwMode = This->ds3db_ds3db.dwMode;
    return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetPosition(IDirectSound3DBuffer *iface,
        D3DVECTOR *lpvPosition)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSound3DBuffer(iface);

    TRACE("returning: Position vector = (%f,%f,%f)\n", This->ds3db_ds3db.vPosition.x,
            This->ds3db_ds3db.vPosition.y, This->ds3db_ds3db.vPosition.z);
    *lpvPosition = This->ds3db_ds3db.vPosition;
    return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetVelocity(IDirectSound3DBuffer *iface,
        D3DVECTOR *lpvVelocity)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSound3DBuffer(iface);

    TRACE("returning: Velocity vector = (%f,%f,%f)\n", This->ds3db_ds3db.vVelocity.x,
            This->ds3db_ds3db.vVelocity.y, This->ds3db_ds3db.vVelocity.z);
    *lpvVelocity = This->ds3db_ds3db.vVelocity;
    return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetAllParameters(IDirectSound3DBuffer *iface,
	const DS3DBUFFER *lpcDs3dBuffer, DWORD dwApply)
{
	IDirectSoundBufferImpl *This = impl_from_IDirectSound3DBuffer(iface);
	DWORD status = DSERR_INVALIDPARAM;

	TRACE("(%p,%p,%x)\n",iface,lpcDs3dBuffer,dwApply);

	if (lpcDs3dBuffer == NULL) {
		WARN("invalid parameter: lpcDs3dBuffer == NULL\n");
		return status;
	}

	if (lpcDs3dBuffer->dwSize != sizeof(DS3DBUFFER)) {
		WARN("invalid parameter: lpcDs3dBuffer->dwSize = %d\n", lpcDs3dBuffer->dwSize);
		return status;
	}

	TRACE("setting: all parameters; dwApply = %d\n", dwApply);
	This->ds3db_ds3db = *lpcDs3dBuffer;

	if (dwApply == DS3D_IMMEDIATE)
	{
		DSOUND_Mix3DBuffer(This);
	}
	This->ds3db_need_recalc = TRUE;
	status = DS_OK;

	return status;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetConeAngles(IDirectSound3DBuffer *iface,
        DWORD dwInsideConeAngle, DWORD dwOutsideConeAngle, DWORD dwApply)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSound3DBuffer(iface);

    TRACE("setting: Inside Cone Angle = %d; Outside Cone Angle = %d; dwApply = %d\n",
            dwInsideConeAngle, dwOutsideConeAngle, dwApply);
    This->ds3db_ds3db.dwInsideConeAngle = dwInsideConeAngle;
    This->ds3db_ds3db.dwOutsideConeAngle = dwOutsideConeAngle;
    if (dwApply == DS3D_IMMEDIATE)
        DSOUND_Mix3DBuffer(This);
    This->ds3db_need_recalc = TRUE;
    return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetConeOrientation(IDirectSound3DBuffer *iface,
        D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSound3DBuffer(iface);

    TRACE("setting: Cone Orientation vector = (%f,%f,%f); dwApply = %d\n", x, y, z, dwApply);
    This->ds3db_ds3db.vConeOrientation.x = x;
    This->ds3db_ds3db.vConeOrientation.y = y;
    This->ds3db_ds3db.vConeOrientation.z = z;
    if (dwApply == DS3D_IMMEDIATE)
    {
        This->ds3db_need_recalc = FALSE;
        DSOUND_Mix3DBuffer(This);
    }
    This->ds3db_need_recalc = TRUE;
    return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetConeOutsideVolume(IDirectSound3DBuffer *iface,
        LONG lConeOutsideVolume, DWORD dwApply)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSound3DBuffer(iface);

    TRACE("setting: ConeOutsideVolume = %d; dwApply = %d\n", lConeOutsideVolume, dwApply);
    This->ds3db_ds3db.lConeOutsideVolume = lConeOutsideVolume;
    if (dwApply == DS3D_IMMEDIATE)
    {
        This->ds3db_need_recalc = FALSE;
        DSOUND_Mix3DBuffer(This);
    }
    This->ds3db_need_recalc = TRUE;
    return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetMaxDistance(IDirectSound3DBuffer *iface,
        D3DVALUE fMaxDistance, DWORD dwApply)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSound3DBuffer(iface);

    TRACE("setting: MaxDistance = %f; dwApply = %d\n", fMaxDistance, dwApply);
    This->ds3db_ds3db.flMaxDistance = fMaxDistance;
    if (dwApply == DS3D_IMMEDIATE)
    {
        This->ds3db_need_recalc = FALSE;
        DSOUND_Mix3DBuffer(This);
    }
    This->ds3db_need_recalc = TRUE;
    return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetMinDistance(IDirectSound3DBuffer *iface,
        D3DVALUE fMinDistance, DWORD dwApply)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSound3DBuffer(iface);

    TRACE("setting: MinDistance = %f; dwApply = %d\n", fMinDistance, dwApply);
    This->ds3db_ds3db.flMinDistance = fMinDistance;
    if (dwApply == DS3D_IMMEDIATE)
    {
        This->ds3db_need_recalc = FALSE;
        DSOUND_Mix3DBuffer(This);
    }
    This->ds3db_need_recalc = TRUE;
    return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetMode(IDirectSound3DBuffer *iface, DWORD dwMode,
        DWORD dwApply)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSound3DBuffer(iface);

    TRACE("setting: Mode = %d; dwApply = %d\n", dwMode, dwApply);
    This->ds3db_ds3db.dwMode = dwMode;
    if (dwApply == DS3D_IMMEDIATE)
    {
        This->ds3db_need_recalc = FALSE;
        DSOUND_Mix3DBuffer(This);
    }
    This->ds3db_need_recalc = TRUE;
    return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetPosition(IDirectSound3DBuffer *iface, D3DVALUE x,
        D3DVALUE y, D3DVALUE z, DWORD dwApply)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSound3DBuffer(iface);

    TRACE("setting: Position vector = (%f,%f,%f); dwApply = %d\n", x, y, z, dwApply);
    This->ds3db_ds3db.vPosition.x = x;
    This->ds3db_ds3db.vPosition.y = y;
    This->ds3db_ds3db.vPosition.z = z;
    if (dwApply == DS3D_IMMEDIATE)
    {
        This->ds3db_need_recalc = FALSE;
        DSOUND_Mix3DBuffer(This);
    }
    This->ds3db_need_recalc = TRUE;
    return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetVelocity(IDirectSound3DBuffer *iface,
        D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSound3DBuffer(iface);

    TRACE("setting: Velocity vector = (%f,%f,%f); dwApply = %d\n", x, y, z, dwApply);
    This->ds3db_ds3db.vVelocity.x = x;
    This->ds3db_ds3db.vVelocity.y = y;
    This->ds3db_ds3db.vVelocity.z = z;
    if (dwApply == DS3D_IMMEDIATE)
    {
        This->ds3db_need_recalc = FALSE;
        DSOUND_Mix3DBuffer(This);
    }
    This->ds3db_need_recalc = TRUE;
    return DS_OK;
}

const IDirectSound3DBufferVtbl ds3dbvt =
{
	/* IUnknown methods */
	IDirectSound3DBufferImpl_QueryInterface,
	IDirectSound3DBufferImpl_AddRef,
	IDirectSound3DBufferImpl_Release,
	/* IDirectSound3DBuffer methods */
	IDirectSound3DBufferImpl_GetAllParameters,
	IDirectSound3DBufferImpl_GetConeAngles,
	IDirectSound3DBufferImpl_GetConeOrientation,
	IDirectSound3DBufferImpl_GetConeOutsideVolume,
	IDirectSound3DBufferImpl_GetMaxDistance,
	IDirectSound3DBufferImpl_GetMinDistance,
	IDirectSound3DBufferImpl_GetMode,
	IDirectSound3DBufferImpl_GetPosition,
	IDirectSound3DBufferImpl_GetVelocity,
	IDirectSound3DBufferImpl_SetAllParameters,
	IDirectSound3DBufferImpl_SetConeAngles,
	IDirectSound3DBufferImpl_SetConeOrientation,
	IDirectSound3DBufferImpl_SetConeOutsideVolume,
	IDirectSound3DBufferImpl_SetMaxDistance,
	IDirectSound3DBufferImpl_SetMinDistance,
	IDirectSound3DBufferImpl_SetMode,
	IDirectSound3DBufferImpl_SetPosition,
	IDirectSound3DBufferImpl_SetVelocity,
};


/*******************************************************************************
 *	      IDirectSound3DListener
 */
static inline IDirectSoundBufferImpl *impl_from_IDirectSound3DListener(IDirectSound3DListener *iface)
{
    return CONTAINING_RECORD(iface, IDirectSoundBufferImpl, IDirectSound3DListener_iface);
}


/* IUnknown methods */
static HRESULT WINAPI IDirectSound3DListenerImpl_QueryInterface(IDirectSound3DListener *iface,
        REFIID riid, void **ppobj)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSound3DListener(iface);

        TRACE("(%p,%s,%p)\n", iface, debugstr_guid(riid), ppobj);

        return IDirectSoundBuffer_QueryInterface(&This->IDirectSoundBuffer8_iface, riid, ppobj);
}

static ULONG WINAPI IDirectSound3DListenerImpl_AddRef(IDirectSound3DListener *iface)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSound3DListener(iface);
    ULONG ref = InterlockedIncrement(&This->ref3D);

    TRACE("(%p) ref was %d\n", This, ref - 1);

    if(ref == 1)
        InterlockedIncrement(&This->numIfaces);

    return ref;
}

static ULONG WINAPI IDirectSound3DListenerImpl_Release(IDirectSound3DListener *iface)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSound3DListener(iface);
    ULONG ref;

    ref = capped_refcount_dec(&This->ref3D);
    if(!ref)
        capped_refcount_dec(&This->numIfaces);

    TRACE("(%p) ref is now %d\n", This, ref);

    return ref;
}

/* IDirectSound3DListener methods */
static HRESULT WINAPI IDirectSound3DListenerImpl_GetAllParameter(IDirectSound3DListener *iface,
        DS3DLISTENER *lpDS3DL)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSound3DListener(iface);

	TRACE("(%p,%p)\n",This,lpDS3DL);

	if (lpDS3DL == NULL) {
		WARN("invalid parameter: lpDS3DL == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	if (lpDS3DL->dwSize < sizeof(*lpDS3DL)) {
		WARN("invalid parameter: lpDS3DL->dwSize = %d\n",lpDS3DL->dwSize);
		return DSERR_INVALIDPARAM;
	}
	
	TRACE("returning: all parameters\n");
	*lpDS3DL = This->device->ds3dl;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_GetDistanceFactor(IDirectSound3DListener *iface,
        D3DVALUE *lpfDistanceFactor)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSound3DListener(iface);

	TRACE("returning: Distance Factor = %f\n", This->device->ds3dl.flDistanceFactor);
	*lpfDistanceFactor = This->device->ds3dl.flDistanceFactor;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_GetDopplerFactor(IDirectSound3DListener *iface,
        D3DVALUE *lpfDopplerFactor)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSound3DListener(iface);

	TRACE("returning: Doppler Factor = %f\n", This->device->ds3dl.flDopplerFactor);
	*lpfDopplerFactor = This->device->ds3dl.flDopplerFactor;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_GetOrientation(IDirectSound3DListener *iface,
        D3DVECTOR *lpvOrientFront, D3DVECTOR *lpvOrientTop)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSound3DListener(iface);

	TRACE("returning: OrientFront vector = (%f,%f,%f); OrientTop vector = (%f,%f,%f)\n", This->device->ds3dl.vOrientFront.x,
	This->device->ds3dl.vOrientFront.y, This->device->ds3dl.vOrientFront.z, This->device->ds3dl.vOrientTop.x, This->device->ds3dl.vOrientTop.y,
	This->device->ds3dl.vOrientTop.z);
	*lpvOrientFront = This->device->ds3dl.vOrientFront;
	*lpvOrientTop = This->device->ds3dl.vOrientTop;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_GetPosition(IDirectSound3DListener *iface,
        D3DVECTOR *lpvPosition)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSound3DListener(iface);

	TRACE("returning: Position vector = (%f,%f,%f)\n", This->device->ds3dl.vPosition.x, This->device->ds3dl.vPosition.y, This->device->ds3dl.vPosition.z);
	*lpvPosition = This->device->ds3dl.vPosition;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_GetRolloffFactor(IDirectSound3DListener *iface,
        D3DVALUE *lpfRolloffFactor)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSound3DListener(iface);

	TRACE("returning: RolloffFactor = %f\n", This->device->ds3dl.flRolloffFactor);
	*lpfRolloffFactor = This->device->ds3dl.flRolloffFactor;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_GetVelocity(IDirectSound3DListener *iface,
        D3DVECTOR *lpvVelocity)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSound3DListener(iface);

	TRACE("returning: Velocity vector = (%f,%f,%f)\n", This->device->ds3dl.vVelocity.x, This->device->ds3dl.vVelocity.y, This->device->ds3dl.vVelocity.z);
	*lpvVelocity = This->device->ds3dl.vVelocity;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_SetAllParameters(IDirectSound3DListener *iface,
        const DS3DLISTENER *lpcDS3DL, DWORD dwApply)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSound3DListener(iface);

	TRACE("setting: all parameters; dwApply = %d\n", dwApply);
	This->device->ds3dl = *lpcDS3DL;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->device->ds3dl_need_recalc = FALSE;
		DSOUND_ChangeListener(This);
	}
	This->device->ds3dl_need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_SetDistanceFactor(IDirectSound3DListener *iface,
        D3DVALUE fDistanceFactor, DWORD dwApply)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSound3DListener(iface);

	TRACE("setting: Distance Factor = %f; dwApply = %d\n", fDistanceFactor, dwApply);
	This->device->ds3dl.flDistanceFactor = fDistanceFactor;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->device->ds3dl_need_recalc = FALSE;
		DSOUND_ChangeListener(This);
	}
	This->device->ds3dl_need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_SetDopplerFactor(IDirectSound3DListener *iface,
        D3DVALUE fDopplerFactor, DWORD dwApply)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSound3DListener(iface);

	TRACE("setting: Doppler Factor = %f; dwApply = %d\n", fDopplerFactor, dwApply);
	This->device->ds3dl.flDopplerFactor = fDopplerFactor;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->device->ds3dl_need_recalc = FALSE;
		DSOUND_ChangeListener(This);
	}
	This->device->ds3dl_need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_SetOrientation(IDirectSound3DListener *iface,
        D3DVALUE xFront, D3DVALUE yFront, D3DVALUE zFront, D3DVALUE xTop, D3DVALUE yTop,
        D3DVALUE zTop, DWORD dwApply)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSound3DListener(iface);

	TRACE("setting: Front vector = (%f,%f,%f); Top vector = (%f,%f,%f); dwApply = %d\n",
	xFront, yFront, zFront, xTop, yTop, zTop, dwApply);
	This->device->ds3dl.vOrientFront.x = xFront;
	This->device->ds3dl.vOrientFront.y = yFront;
	This->device->ds3dl.vOrientFront.z = zFront;
	This->device->ds3dl.vOrientTop.x = xTop;
	This->device->ds3dl.vOrientTop.y = yTop;
	This->device->ds3dl.vOrientTop.z = zTop;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->device->ds3dl_need_recalc = FALSE;
		DSOUND_ChangeListener(This);
	}
	This->device->ds3dl_need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_SetPosition(IDirectSound3DListener *iface,
        D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSound3DListener(iface);

	TRACE("setting: Position vector = (%f,%f,%f); dwApply = %d\n", x, y, z, dwApply);
	This->device->ds3dl.vPosition.x = x;
	This->device->ds3dl.vPosition.y = y;
	This->device->ds3dl.vPosition.z = z;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->device->ds3dl_need_recalc = FALSE;
		DSOUND_ChangeListener(This);
	}
	This->device->ds3dl_need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_SetRolloffFactor(IDirectSound3DListener *iface,
        D3DVALUE fRolloffFactor, DWORD dwApply)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSound3DListener(iface);

	TRACE("setting: Rolloff Factor = %f; dwApply = %d\n", fRolloffFactor, dwApply);
	This->device->ds3dl.flRolloffFactor = fRolloffFactor;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->device->ds3dl_need_recalc = FALSE;
		DSOUND_ChangeListener(This);
	}
	This->device->ds3dl_need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_SetVelocity(IDirectSound3DListener *iface,
        D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSound3DListener(iface);

	TRACE("setting: Velocity vector = (%f,%f,%f); dwApply = %d\n", x, y, z, dwApply);
	This->device->ds3dl.vVelocity.x = x;
	This->device->ds3dl.vVelocity.y = y;
	This->device->ds3dl.vVelocity.z = z;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->device->ds3dl_need_recalc = FALSE;
		DSOUND_ChangeListener(This);
	}
	This->device->ds3dl_need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_CommitDeferredSettings(IDirectSound3DListener *iface)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSound3DListener(iface);

	TRACE("\n");
	DSOUND_ChangeListener(This);
	return DS_OK;
}

const IDirectSound3DListenerVtbl ds3dlvt =
{
	/* IUnknown methods */
	IDirectSound3DListenerImpl_QueryInterface,
	IDirectSound3DListenerImpl_AddRef,
	IDirectSound3DListenerImpl_Release,
	/* IDirectSound3DListener methods */
	IDirectSound3DListenerImpl_GetAllParameter,
	IDirectSound3DListenerImpl_GetDistanceFactor,
	IDirectSound3DListenerImpl_GetDopplerFactor,
	IDirectSound3DListenerImpl_GetOrientation,
	IDirectSound3DListenerImpl_GetPosition,
	IDirectSound3DListenerImpl_GetRolloffFactor,
	IDirectSound3DListenerImpl_GetVelocity,
	IDirectSound3DListenerImpl_SetAllParameters,
	IDirectSound3DListenerImpl_SetDistanceFactor,
	IDirectSound3DListenerImpl_SetDopplerFactor,
	IDirectSound3DListenerImpl_SetOrientation,
	IDirectSound3DListenerImpl_SetPosition,
	IDirectSound3DListenerImpl_SetRolloffFactor,
	IDirectSound3DListenerImpl_SetVelocity,
	IDirectSound3DListenerImpl_CommitDeferredSettings,
};
