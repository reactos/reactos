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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"
#include "winreg.h"
#include "winternl.h"
#include "mmddk.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsdriver.h"
#include "dsound_private.h"

/* default intensity level for human ears */
#define DEFAULT_INTENSITY 0.000000000001f
/* default velocity of sound in the air */
#define DEFAULT_VELOCITY 340

WINE_DEFAULT_DEBUG_CHANNEL(dsound3d);

/*******************************************************************************
 *              Auxiliary functions
 */

/* scalar product (i believe it's called dot product in english) */
static inline D3DVALUE ScalarProduct (LPD3DVECTOR a, LPD3DVECTOR b)
{
	D3DVALUE c;
	c = (a->x*b->x) + (a->y*b->y) + (a->z*b->z);
	TRACE("(%f,%f,%f) * (%f,%f,%f) = %f)\n", a->x, a->y, a->z, b->x, b->y, \
	      b->z, c);
	return c;
}

/* vector product (i believe it's called cross product in english */
static inline D3DVECTOR VectorProduct (LPD3DVECTOR a, LPD3DVECTOR b)
{
	D3DVECTOR c;
	c.x = (a->y*b->z) - (a->z*b->y);
	c.y = (a->z*b->x) - (a->x*b->z);
	c.z = (a->x*b->y) - (a->y*b->x);
	TRACE("(%f,%f,%f) x (%f,%f,%f) = (%f,%f,%f)\n", a->x, a->y, a->z, b->x, b->y, \
	      b->z, c.x, c.y, c.z);
	return c;
}

/* magnitude (length) of vector */
static inline D3DVALUE VectorMagnitude (LPD3DVECTOR a)
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

/* conversion between degrees and radians */
static inline D3DVALUE DegToRad (D3DVALUE angle)
{
	D3DVALUE newangle;
	newangle = angle * (2*M_PI/360);
	TRACE("%f deg = %f rad\n", angle, newangle);
	return newangle;
}

/* angle between vectors - deg version */
static inline D3DVALUE AngleBetweenVectorsDeg (LPD3DVECTOR a, LPD3DVECTOR b)
{
	D3DVALUE la, lb, product, angle, cos;
	/* definition of scalar product: a*b = |a|*|b|*cos...therefore: */
	product = ScalarProduct (a,b);
	la = VectorMagnitude (a);
	lb = VectorMagnitude (b);
	cos = product/(la*lb);
	angle = acos(cos);
	/* we now have angle in radians */
	angle = RadToDeg(angle);
	TRACE("angle between (%f,%f,%f) and (%f,%f,%f) = %f degrees\n",  a->x, a->y, a->z, b->x,
	      b->y, b->z, angle);
	return angle;	
}

/* angle between vectors - rad version */
static inline D3DVALUE AngleBetweenVectorsRad (LPD3DVECTOR a, LPD3DVECTOR b)
{
	D3DVALUE la, lb, product, angle, cos;
	/* definition of scalar product: a*b = |a|*|b|*cos...therefore: */
	product = ScalarProduct (a,b);
	la = VectorMagnitude (a);
	lb = VectorMagnitude (b);
	cos = product/(la*lb);
	angle = acos(cos);
	TRACE("angle between (%f,%f,%f) and (%f,%f,%f) = %f radians\n",  a->x, a->y, a->z, b->x,
	      b->y, b->z, angle);
	return angle;	
}

/* calculates vector between two points */
static inline D3DVECTOR VectorBetweenTwoPoints (LPD3DVECTOR a, LPD3DVECTOR b)
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
static inline D3DVALUE ProjectVector (LPD3DVECTOR a, LPD3DVECTOR p)
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
	/* intensity (used for distance related stuff) */
	double flIntensity;
	double flTemp;
	/* stuff for distance related stuff calc. */
	D3DVECTOR vDistance;
	D3DVALUE flDistance = 0;
	/* panning related stuff */
	D3DVALUE flAngle;
	D3DVECTOR vLeft;
	/* doppler shift related stuff */
#if 0
	D3DVALUE flFreq, flBufferVel, flListenerVel;
#endif

	TRACE("(%p)\n",dsb);

	/* initial buffer volume */
	lVolume = dsb->ds3db_lVolume;
	
	switch (dsb->ds3db_ds3db.dwMode)
	{
		case DS3DMODE_DISABLE:
			TRACE("3D processing disabled\n");
			/* this one is here only to eliminate annoying warning message */
			DSOUND_RecalcVolPan (&dsb->volpan);
			DSOUND_ForceRemix (dsb);
			break;
		case DS3DMODE_NORMAL:
			TRACE("Normal 3D processing mode\n");
			/* we need to calculate distance between buffer and listener*/
			vDistance = VectorBetweenTwoPoints(&dsb->ds3db_ds3db.vPosition, &dsb->dsound->device->ds3dl.vPosition);
			flDistance = VectorMagnitude (&vDistance);
			break;
		case DS3DMODE_HEADRELATIVE:
			TRACE("Head-relative 3D processing mode\n");
			/* distance between buffer and listener is same as buffer's position */
			flDistance = VectorMagnitude (&dsb->ds3db_ds3db.vPosition);
			break;
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
	
	/* the following formula is taken from my physics book. I think it's ok for the *real* world...i hope m$ does it that way */
	lVolume += 10000; /* ms likes working with negative volume...i don't */
	lVolume /= 1000; /* convert hundreths of dB into B */
	/* intensity level (loudness) = log10(Intensity/DefaultIntensity)...therefore */
	flIntensity = pow(10,lVolume)*DEFAULT_INTENSITY;	
	flTemp = (flDistance/dsb->ds3db_ds3db.flMinDistance)*(flDistance/dsb->ds3db_ds3db.flMinDistance);
	flIntensity /= flTemp;
	lVolume = log10(flIntensity/DEFAULT_INTENSITY);
	lVolume *= 1000; /* convert back to hundreths of dB */
	lVolume -= 10000; /* we need to do it in ms way */
	TRACE("dist. att: Distance = %f, MinDistance = %f => adjusting volume %ld to %f\n", flDistance, dsb->ds3db_ds3db.flMinDistance, dsb->ds3db_lVolume, lVolume);

	/* conning */
	/* sometimes it happens that vConeOrientation vector = (0,0,0); in this case angle is "nan" and it's useless*/
	if (dsb->ds3db_ds3db.vConeOrientation.x == 0 && dsb->ds3db_ds3db.vConeOrientation.y == 0 && dsb->ds3db_ds3db.vConeOrientation.z == 0)
	{
		TRACE("conning: cones not set\n");
	}
	else
	{
		/* calculate angle */
		flAngle = AngleBetweenVectorsDeg(&dsb->ds3db_ds3db.vConeOrientation, &vDistance);
		/* if by any chance it happens that OutsideConeAngle = InsideConeAngle (that means that conning has no effect) */
		if (dsb->ds3db_ds3db.dwInsideConeAngle != dsb->ds3db_ds3db.dwOutsideConeAngle)
		{
			/* my test show that for my way of calc., we need only half of angles */
			DWORD dwInsideConeAngle = dsb->ds3db_ds3db.dwInsideConeAngle/2;
			DWORD dwOutsideConeAngle = dsb->ds3db_ds3db.dwOutsideConeAngle/2;
			/* full volume */
			if (flAngle < dwInsideConeAngle)
				flAngle = dwInsideConeAngle;
			/* min (app defined) volume */
			if (flAngle > dwOutsideConeAngle)
				flAngle = dwOutsideConeAngle;
			/* this probably isn't the right thing, but it's ok for the time being */
			lVolume += ((dsb->ds3db_ds3db.lConeOutsideVolume)/((dwOutsideConeAngle) - (dwInsideConeAngle))) * flAngle;
		}
		TRACE("conning: Angle = %f deg; InsideConeAngle(/2) = %ld deg; OutsideConeAngle(/2) = %ld deg; ConeOutsideVolume = %ld => adjusting volume to %f\n",
		       flAngle, dsb->ds3db_ds3db.dwInsideConeAngle/2, dsb->ds3db_ds3db.dwOutsideConeAngle/2, dsb->ds3db_ds3db.lConeOutsideVolume, lVolume);
	}
	dsb->volpan.lVolume = lVolume;
	
	/* panning */
	if (dsb->dsound->device->ds3dl.vPosition.x == dsb->ds3db_ds3db.vPosition.x &&
	    dsb->dsound->device->ds3dl.vPosition.y == dsb->ds3db_ds3db.vPosition.y &&
	    dsb->dsound->device->ds3dl.vPosition.z == dsb->ds3db_ds3db.vPosition.z) {
		dsb->volpan.lPan = 0;
		flAngle = 0.0;
	}
	else
	{
		vDistance = VectorBetweenTwoPoints(&dsb->dsound->device->ds3dl.vPosition, &dsb->ds3db_ds3db.vPosition);
		vLeft = VectorProduct(&dsb->dsound->device->ds3dl.vOrientFront, &dsb->dsound->device->ds3dl.vOrientTop);
		flAngle = AngleBetweenVectorsRad(&vLeft, &vDistance);
		/* for now, we'll use "linear formula" (which is probably incorrect); if someone has it in book, correct it */
		dsb->volpan.lPan = 10000*2*flAngle/M_PI - 10000;
	}
	TRACE("panning: Angle = %f rad, lPan = %ld\n", flAngle, dsb->volpan.lPan);

	/* FIXME: Doppler Effect disabled since i have no idea which frequency to change and how to do it */
#if 0	
	/* doppler shift*/
	if ((VectorMagnitude(&ds3db.vVelocity) == 0) && (VectorMagnitude(&dsb->dsound->device->ds3dl.vVelocity) == 0))
	{
		TRACE("doppler: Buffer and Listener don't have velocities\n");
	}
	else
	{
		/* calculate length of ds3db.vVelocity component which causes Doppler Effect
		   NOTE: if buffer moves TOWARDS the listener, it's velocity component is NEGATIVE
		         if buffer moves AWAY from listener, it's velocity component is POSITIVE */
		flBufferVel = ProjectVector(&dsb->ds3db_ds3db.vVelocity, &vDistance);
		/* calculate length of ds3dl.vVelocity component which causes Doppler Effect
		   NOTE: if listener moves TOWARDS the buffer, it's velocity component is POSITIVE
		         if listener moves AWAY from buffer, it's velocity component is NEGATIVE */
		flListenerVel = ProjectVector(&dsb->dsound->device->ds3dl.vVelocity, &vDistance);
		/* formula taken from Gianicoli D.: Physics, 4th edition: */
		/* FIXME: replace dsb->freq with appropriate frequency ! */
		flFreq = dsb->freq * ((DEFAULT_VELOCITY + flListenerVel)/(DEFAULT_VELOCITY + flBufferVel));
		TRACE("doppler: Buffer velocity (component) = %lf, Listener velocity (component) = %lf => Doppler shift: %ld Hz -> %lf Hz\n", flBufferVel, flListenerVel, \
		      dsb->freq, flFreq);
		/* FIXME: replace following line with correct frequency setting ! */
		dsb->freq = flFreq;
	}
#endif	
	
	/* time for remix */
	DSOUND_RecalcVolPan(&dsb->volpan);
}

static void DSOUND_Mix3DBuffer(IDirectSoundBufferImpl *dsb)
{
	TRACE("(%p)\n",dsb);

	DSOUND_Calc3DBuffer(dsb);
	DSOUND_ForceRemix(dsb);			
}

static void WINAPI DSOUND_ChangeListener(IDirectSound3DListenerImpl *ds3dl)
{
	int i;
	TRACE("(%p)\n",ds3dl);
	for (i = 0; i < ds3dl->dsound->device->nrofbuffers; i++)
	{
		/* some buffers don't have 3d buffer (Ultima IX seems to
		crash without the following line) */
		if (ds3dl->dsound->device->buffers[i]->ds3db == NULL)
			continue;
		if (ds3dl->dsound->device->buffers[i]->ds3db_need_recalc)
		{
			DSOUND_Mix3DBuffer(ds3dl->dsound->device->buffers[i]);
		}
	}
}

/*******************************************************************************
 *              IDirectSound3DBuffer
 */

/* IUnknown methods */
static HRESULT WINAPI IDirectSound3DBufferImpl_QueryInterface(
	LPDIRECTSOUND3DBUFFER iface, REFIID riid, LPVOID *ppobj)
{
	IDirectSound3DBufferImpl *This = (IDirectSound3DBufferImpl *)iface;

	TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
	return IDirectSoundBuffer_QueryInterface((LPDIRECTSOUNDBUFFER8)This->dsb, riid, ppobj);
}

static ULONG WINAPI IDirectSound3DBufferImpl_AddRef(LPDIRECTSOUND3DBUFFER iface)
{
    IDirectSound3DBufferImpl *This = (IDirectSound3DBufferImpl *)iface;
    ULONG ref = InterlockedIncrement(&(This->ref));
    TRACE("(%p) ref was %ld\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI IDirectSound3DBufferImpl_Release(LPDIRECTSOUND3DBUFFER iface)
{
    IDirectSound3DBufferImpl *This = (IDirectSound3DBufferImpl *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %ld\n", This, ref + 1);

    if (!ref) {
        This->dsb->ds3db = NULL;
        IDirectSoundBuffer_Release((LPDIRECTSOUNDBUFFER8)This->dsb);
        HeapFree(GetProcessHeap(), 0, This);
        TRACE("(%p) released\n", This);
    }
    return ref;
}

/* IDirectSound3DBuffer methods */
static HRESULT WINAPI IDirectSound3DBufferImpl_GetAllParameters(
	LPDIRECTSOUND3DBUFFER iface,
	LPDS3DBUFFER lpDs3dBuffer)
{
	IDirectSound3DBufferImpl *This = (IDirectSound3DBufferImpl *)iface;
	TRACE("(%p,%p)\n",This,lpDs3dBuffer);

	if (lpDs3dBuffer == NULL) {
		WARN("invalid parameter: lpDs3dBuffer == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	if (lpDs3dBuffer->dwSize < sizeof(*lpDs3dBuffer)) {
		WARN("invalid parameter: lpDs3dBuffer->dwSize = %ld < %d\n",lpDs3dBuffer->dwSize, sizeof(*lpDs3dBuffer));
		return DSERR_INVALIDPARAM;
	}
	
	TRACE("returning: all parameters\n");
	*lpDs3dBuffer = This->dsb->ds3db_ds3db;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetConeAngles(
	LPDIRECTSOUND3DBUFFER iface,
	LPDWORD lpdwInsideConeAngle,
	LPDWORD lpdwOutsideConeAngle)
{
	IDirectSound3DBufferImpl *This = (IDirectSound3DBufferImpl *)iface;
	TRACE("returning: Inside Cone Angle = %ld degrees; Outside Cone Angle = %ld degrees\n",
		This->dsb->ds3db_ds3db.dwInsideConeAngle, This->dsb->ds3db_ds3db.dwOutsideConeAngle);
	*lpdwInsideConeAngle = This->dsb->ds3db_ds3db.dwInsideConeAngle;
	*lpdwOutsideConeAngle = This->dsb->ds3db_ds3db.dwOutsideConeAngle;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetConeOrientation(
	LPDIRECTSOUND3DBUFFER iface,
	LPD3DVECTOR lpvConeOrientation)
{
	IDirectSound3DBufferImpl *This = (IDirectSound3DBufferImpl *)iface;
	TRACE("returning: Cone Orientation vector = (%f,%f,%f)\n",
		This->dsb->ds3db_ds3db.vConeOrientation.x,
		This->dsb->ds3db_ds3db.vConeOrientation.y,
		This->dsb->ds3db_ds3db.vConeOrientation.z);
	*lpvConeOrientation = This->dsb->ds3db_ds3db.vConeOrientation;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetConeOutsideVolume(
	LPDIRECTSOUND3DBUFFER iface,
	LPLONG lplConeOutsideVolume)
{
	IDirectSound3DBufferImpl *This = (IDirectSound3DBufferImpl *)iface;
	TRACE("returning: Cone Outside Volume = %ld\n", This->dsb->ds3db_ds3db.lConeOutsideVolume);
	*lplConeOutsideVolume = This->dsb->ds3db_ds3db.lConeOutsideVolume;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetMaxDistance(
	LPDIRECTSOUND3DBUFFER iface,
	LPD3DVALUE lpfMaxDistance)
{
	IDirectSound3DBufferImpl *This = (IDirectSound3DBufferImpl *)iface;
	TRACE("returning: Max Distance = %f\n", This->dsb->ds3db_ds3db.flMaxDistance);
	*lpfMaxDistance = This->dsb->ds3db_ds3db.flMaxDistance;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetMinDistance(
	LPDIRECTSOUND3DBUFFER iface,
	LPD3DVALUE lpfMinDistance)
{
	IDirectSound3DBufferImpl *This = (IDirectSound3DBufferImpl *)iface;
	TRACE("returning: Min Distance = %f\n", This->dsb->ds3db_ds3db.flMinDistance);
	*lpfMinDistance = This->dsb->ds3db_ds3db.flMinDistance;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetMode(
	LPDIRECTSOUND3DBUFFER iface,
	LPDWORD lpdwMode)
{
	IDirectSound3DBufferImpl *This = (IDirectSound3DBufferImpl *)iface;
	TRACE("returning: Mode = %ld\n", This->dsb->ds3db_ds3db.dwMode);
	*lpdwMode = This->dsb->ds3db_ds3db.dwMode;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetPosition(
	LPDIRECTSOUND3DBUFFER iface,
	LPD3DVECTOR lpvPosition)
{
	IDirectSound3DBufferImpl *This = (IDirectSound3DBufferImpl *)iface;
	TRACE("returning: Position vector = (%f,%f,%f)\n",
		This->dsb->ds3db_ds3db.vPosition.x,
		This->dsb->ds3db_ds3db.vPosition.y,
		This->dsb->ds3db_ds3db.vPosition.z);
	*lpvPosition = This->dsb->ds3db_ds3db.vPosition;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_GetVelocity(
	LPDIRECTSOUND3DBUFFER iface,
	LPD3DVECTOR lpvVelocity)
{
	IDirectSound3DBufferImpl *This = (IDirectSound3DBufferImpl *)iface;
	TRACE("returning: Velocity vector = (%f,%f,%f)\n",
		This->dsb->ds3db_ds3db.vVelocity.x,
		This->dsb->ds3db_ds3db.vVelocity.y,
		This->dsb->ds3db_ds3db.vVelocity.z);
	*lpvVelocity = This->dsb->ds3db_ds3db.vVelocity;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetAllParameters(
	LPDIRECTSOUND3DBUFFER iface,
	LPCDS3DBUFFER lpcDs3dBuffer,
	DWORD dwApply)
{
	IDirectSound3DBufferImpl *This = (IDirectSound3DBufferImpl *)iface;
	DWORD status = DSERR_INVALIDPARAM;
	TRACE("(%p,%p,%lx)\n",iface,lpcDs3dBuffer,dwApply);

	if (lpcDs3dBuffer == NULL) {
		WARN("invalid parameter: lpcDs3dBuffer == NULL\n");
		return status;
	}

	if (lpcDs3dBuffer->dwSize != sizeof(DS3DBUFFER)) {
		WARN("invalid parameter: lpcDs3dBuffer->dwSize = %ld != %d\n",
			lpcDs3dBuffer->dwSize, sizeof(DS3DBUFFER));
		return status;
	}

	TRACE("setting: all parameters; dwApply = %ld\n", dwApply);
	This->dsb->ds3db_ds3db = *lpcDs3dBuffer;

	if (dwApply == DS3D_IMMEDIATE)
	{
		DSOUND_Mix3DBuffer(This->dsb);
	}
	This->dsb->ds3db_need_recalc = TRUE;
	status = DS_OK;

	return status;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetConeAngles(
	LPDIRECTSOUND3DBUFFER iface,
	DWORD dwInsideConeAngle,
	DWORD dwOutsideConeAngle,
	DWORD dwApply)
{
	IDirectSound3DBufferImpl *This = (IDirectSound3DBufferImpl *)iface;
	TRACE("setting: Inside Cone Angle = %ld; Outside Cone Angle = %ld; dwApply = %ld\n",
		dwInsideConeAngle, dwOutsideConeAngle, dwApply);
	This->dsb->ds3db_ds3db.dwInsideConeAngle = dwInsideConeAngle;
	This->dsb->ds3db_ds3db.dwOutsideConeAngle = dwOutsideConeAngle;
	if (dwApply == DS3D_IMMEDIATE)
	{
		DSOUND_Mix3DBuffer(This->dsb);
	}
	This->dsb->ds3db_need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetConeOrientation(
	LPDIRECTSOUND3DBUFFER iface,
	D3DVALUE x, D3DVALUE y, D3DVALUE z,
	DWORD dwApply)
{
	IDirectSound3DBufferImpl *This = (IDirectSound3DBufferImpl *)iface;
	TRACE("setting: Cone Orientation vector = (%f,%f,%f); dwApply = %ld\n", x, y, z, dwApply);
	This->dsb->ds3db_ds3db.vConeOrientation.x = x;
	This->dsb->ds3db_ds3db.vConeOrientation.y = y;
	This->dsb->ds3db_ds3db.vConeOrientation.z = z;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->dsb->ds3db_need_recalc = FALSE;
		DSOUND_Mix3DBuffer(This->dsb);
	}
	This->dsb->ds3db_need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetConeOutsideVolume(
	LPDIRECTSOUND3DBUFFER iface,
	LONG lConeOutsideVolume,
	DWORD dwApply)
{
	IDirectSound3DBufferImpl *This = (IDirectSound3DBufferImpl *)iface;
	TRACE("setting: ConeOutsideVolume = %ld; dwApply = %ld\n", lConeOutsideVolume, dwApply);
	This->dsb->ds3db_ds3db.lConeOutsideVolume = lConeOutsideVolume;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->dsb->ds3db_need_recalc = FALSE;
		DSOUND_Mix3DBuffer(This->dsb);
	}
	This->dsb->ds3db_need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetMaxDistance(
	LPDIRECTSOUND3DBUFFER iface,
	D3DVALUE fMaxDistance,
	DWORD dwApply)
{
	IDirectSound3DBufferImpl *This = (IDirectSound3DBufferImpl *)iface;
	TRACE("setting: MaxDistance = %f; dwApply = %ld\n", fMaxDistance, dwApply);
	This->dsb->ds3db_ds3db.flMaxDistance = fMaxDistance;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->dsb->ds3db_need_recalc = FALSE;
		DSOUND_Mix3DBuffer(This->dsb);
	}
	This->dsb->ds3db_need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetMinDistance(
	LPDIRECTSOUND3DBUFFER iface,
	D3DVALUE fMinDistance,
	DWORD dwApply)
{
	IDirectSound3DBufferImpl *This = (IDirectSound3DBufferImpl *)iface;
	TRACE("setting: MinDistance = %f; dwApply = %ld\n", fMinDistance, dwApply);
	This->dsb->ds3db_ds3db.flMinDistance = fMinDistance;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->dsb->ds3db_need_recalc = FALSE;
		DSOUND_Mix3DBuffer(This->dsb);
	}
	This->dsb->ds3db_need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetMode(
	LPDIRECTSOUND3DBUFFER iface,
	DWORD dwMode,
	DWORD dwApply)
{
	IDirectSound3DBufferImpl *This = (IDirectSound3DBufferImpl *)iface;
	TRACE("setting: Mode = %ld; dwApply = %ld\n", dwMode, dwApply);
	This->dsb->ds3db_ds3db.dwMode = dwMode;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->dsb->ds3db_need_recalc = FALSE;
		DSOUND_Mix3DBuffer(This->dsb);
	}
	This->dsb->ds3db_need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetPosition(
	LPDIRECTSOUND3DBUFFER iface,
	D3DVALUE x, D3DVALUE y, D3DVALUE z,
	DWORD dwApply)
{
	IDirectSound3DBufferImpl *This = (IDirectSound3DBufferImpl *)iface;
	TRACE("setting: Position vector = (%f,%f,%f); dwApply = %ld\n", x, y, z, dwApply);
	This->dsb->ds3db_ds3db.vPosition.x = x;
	This->dsb->ds3db_ds3db.vPosition.y = y;
	This->dsb->ds3db_ds3db.vPosition.z = z;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->dsb->ds3db_need_recalc = FALSE;
		DSOUND_Mix3DBuffer(This->dsb);
	}
	This->dsb->ds3db_need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DBufferImpl_SetVelocity(
	LPDIRECTSOUND3DBUFFER iface,
	D3DVALUE x, D3DVALUE y, D3DVALUE z,
	DWORD dwApply)
{
	IDirectSound3DBufferImpl *This = (IDirectSound3DBufferImpl *)iface;
	TRACE("setting: Velocity vector = (%f,%f,%f); dwApply = %ld\n", x, y, z, dwApply);
	This->dsb->ds3db_ds3db.vVelocity.x = x;
	This->dsb->ds3db_ds3db.vVelocity.y = y;
	This->dsb->ds3db_ds3db.vVelocity.z = z;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->dsb->ds3db_need_recalc = FALSE;
		DSOUND_Mix3DBuffer(This->dsb);
	}
	This->dsb->ds3db_need_recalc = TRUE;
	return DS_OK;
}

static const IDirectSound3DBufferVtbl ds3dbvt =
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

HRESULT WINAPI IDirectSound3DBufferImpl_Create(
	IDirectSoundBufferImpl *dsb,
	IDirectSound3DBufferImpl **pds3db)
{
	IDirectSound3DBufferImpl *ds3db;
	TRACE("(%p,%p)\n",dsb,pds3db);

	ds3db = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*ds3db));

	if (ds3db == NULL) {
		WARN("out of memory\n");
		*pds3db = 0;
		return DSERR_OUTOFMEMORY;
	}

	ds3db->ref = 0;
	ds3db->dsb = dsb;
	ds3db->lpVtbl = &ds3dbvt;

	ds3db->dsb->ds3db_ds3db.dwSize = sizeof(DS3DBUFFER);
	ds3db->dsb->ds3db_ds3db.vPosition.x = 0.0;
	ds3db->dsb->ds3db_ds3db.vPosition.y = 0.0;
	ds3db->dsb->ds3db_ds3db.vPosition.z = 0.0;
	ds3db->dsb->ds3db_ds3db.vVelocity.x = 0.0;
	ds3db->dsb->ds3db_ds3db.vVelocity.y = 0.0;
	ds3db->dsb->ds3db_ds3db.vVelocity.z = 0.0;
	ds3db->dsb->ds3db_ds3db.dwInsideConeAngle = DS3D_DEFAULTCONEANGLE;
	ds3db->dsb->ds3db_ds3db.dwOutsideConeAngle = DS3D_DEFAULTCONEANGLE;
	ds3db->dsb->ds3db_ds3db.vConeOrientation.x = 0.0;
	ds3db->dsb->ds3db_ds3db.vConeOrientation.y = 0.0;
	ds3db->dsb->ds3db_ds3db.vConeOrientation.z = 0.0;
	ds3db->dsb->ds3db_ds3db.lConeOutsideVolume = DS3D_DEFAULTCONEOUTSIDEVOLUME;
	ds3db->dsb->ds3db_ds3db.flMinDistance = DS3D_DEFAULTMINDISTANCE;
	ds3db->dsb->ds3db_ds3db.flMaxDistance = DS3D_DEFAULTMAXDISTANCE;
	ds3db->dsb->ds3db_ds3db.dwMode = DS3DMODE_NORMAL;

	ds3db->dsb->ds3db_need_recalc = TRUE;

	IDirectSoundBuffer_AddRef((LPDIRECTSOUNDBUFFER8)dsb);

	*pds3db = ds3db;
	return S_OK;
}

HRESULT WINAPI IDirectSound3DBufferImpl_Destroy(
    IDirectSound3DBufferImpl *pds3db)
{
    TRACE("(%p)\n",pds3db);

    while (IDirectSound3DBufferImpl_Release((LPDIRECTSOUND3DBUFFER)pds3db) > 0);

    return S_OK;
}

/*******************************************************************************
 *	      IDirectSound3DListener
 */

/* IUnknown methods */
static HRESULT WINAPI IDirectSound3DListenerImpl_QueryInterface(
	LPDIRECTSOUND3DLISTENER iface, REFIID riid, LPVOID *ppobj)
{
	IDirectSound3DListenerImpl *This = (IDirectSound3DListenerImpl *)iface;

	TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);

	if (ppobj == NULL) {
		WARN("invalid parameter\n");
		return E_INVALIDARG;
	}

	*ppobj = NULL;  /* assume failure */

	if ( IsEqualGUID(riid, &IID_IUnknown) ||
	     IsEqualGUID(riid, &IID_IDirectSound3DListener ) ) {
                IDirectSound3DListener_AddRef((LPDIRECTSOUND3DLISTENER)This);
		*ppobj = This;
		return S_OK;
	}

	if ( IsEqualGUID(riid, &IID_IDirectSoundBuffer) ) {
		if (!This->dsound->device->primary)
			PrimaryBufferImpl_Create(This->dsound, &(This->dsound->device->primary), &(This->dsound->device->dsbd));
		if (This->dsound->device->primary) {
			*ppobj = This->dsound->device->primary;
			IDirectSoundBuffer_AddRef((LPDIRECTSOUNDBUFFER)*ppobj);
			return S_OK;
		}
	}

        FIXME( "Unknown IID %s\n", debugstr_guid( riid ) );
	return E_NOINTERFACE;
}

static ULONG WINAPI IDirectSound3DListenerImpl_AddRef(LPDIRECTSOUND3DLISTENER iface)
{
    IDirectSound3DListenerImpl *This = (IDirectSound3DListenerImpl *)iface;
    ULONG ref = InterlockedIncrement(&(This->ref));
    TRACE("(%p) ref was %ld\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI IDirectSound3DListenerImpl_Release(LPDIRECTSOUND3DLISTENER iface)
{
    IDirectSound3DListenerImpl *This = (IDirectSound3DListenerImpl *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %ld\n", This, ref + 1);

    if (!ref) {
        This->dsound->device->listener = 0;
        HeapFree(GetProcessHeap(), 0, This);
        TRACE("(%p) released\n", This);
    }
    return ref;
}

/* IDirectSound3DListener methods */
static HRESULT WINAPI IDirectSound3DListenerImpl_GetAllParameter(
	LPDIRECTSOUND3DLISTENER iface,
	LPDS3DLISTENER lpDS3DL)
{
	IDirectSound3DListenerImpl *This = (IDirectSound3DListenerImpl *)iface;
	TRACE("(%p,%p)\n",This,lpDS3DL);

	if (lpDS3DL == NULL) {
		WARN("invalid parameter: lpDS3DL == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	if (lpDS3DL->dwSize < sizeof(*lpDS3DL)) {
		WARN("invalid parameter: lpDS3DL->dwSize = %ld < %d\n",lpDS3DL->dwSize, sizeof(*lpDS3DL));
		return DSERR_INVALIDPARAM;
	}
	
	TRACE("returning: all parameters\n");
	*lpDS3DL = This->dsound->device->ds3dl;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_GetDistanceFactor(
	LPDIRECTSOUND3DLISTENER iface,
	LPD3DVALUE lpfDistanceFactor)
{
	IDirectSound3DListenerImpl *This = (IDirectSound3DListenerImpl *)iface;
	TRACE("returning: Distance Factor = %f\n", This->dsound->device->ds3dl.flDistanceFactor);
	*lpfDistanceFactor = This->dsound->device->ds3dl.flDistanceFactor;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_GetDopplerFactor(
	LPDIRECTSOUND3DLISTENER iface,
	LPD3DVALUE lpfDopplerFactor)
{
	IDirectSound3DListenerImpl *This = (IDirectSound3DListenerImpl *)iface;
	TRACE("returning: Doppler Factor = %f\n", This->dsound->device->ds3dl.flDopplerFactor);
	*lpfDopplerFactor = This->dsound->device->ds3dl.flDopplerFactor;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_GetOrientation(
	LPDIRECTSOUND3DLISTENER iface,
	LPD3DVECTOR lpvOrientFront,
	LPD3DVECTOR lpvOrientTop)
{
	IDirectSound3DListenerImpl *This = (IDirectSound3DListenerImpl *)iface;
	TRACE("returning: OrientFront vector = (%f,%f,%f); OrientTop vector = (%f,%f,%f)\n", This->dsound->device->ds3dl.vOrientFront.x, \
	This->dsound->device->ds3dl.vOrientFront.y, This->dsound->device->ds3dl.vOrientFront.z, This->dsound->device->ds3dl.vOrientTop.x, This->dsound->device->ds3dl.vOrientTop.y, \
	This->dsound->device->ds3dl.vOrientTop.z);
	*lpvOrientFront = This->dsound->device->ds3dl.vOrientFront;
	*lpvOrientTop = This->dsound->device->ds3dl.vOrientTop;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_GetPosition(
	LPDIRECTSOUND3DLISTENER iface,
	LPD3DVECTOR lpvPosition)
{
	IDirectSound3DListenerImpl *This = (IDirectSound3DListenerImpl *)iface;
	TRACE("returning: Position vector = (%f,%f,%f)\n", This->dsound->device->ds3dl.vPosition.x, This->dsound->device->ds3dl.vPosition.y, This->dsound->device->ds3dl.vPosition.z);
	*lpvPosition = This->dsound->device->ds3dl.vPosition;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_GetRolloffFactor(
	LPDIRECTSOUND3DLISTENER iface,
	LPD3DVALUE lpfRolloffFactor)
{
	IDirectSound3DListenerImpl *This = (IDirectSound3DListenerImpl *)iface;
	TRACE("returning: RolloffFactor = %f\n", This->dsound->device->ds3dl.flRolloffFactor);
	*lpfRolloffFactor = This->dsound->device->ds3dl.flRolloffFactor;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_GetVelocity(
	LPDIRECTSOUND3DLISTENER iface,
	LPD3DVECTOR lpvVelocity)
{
	IDirectSound3DListenerImpl *This = (IDirectSound3DListenerImpl *)iface;
	TRACE("returning: Velocity vector = (%f,%f,%f)\n", This->dsound->device->ds3dl.vVelocity.x, This->dsound->device->ds3dl.vVelocity.y, This->dsound->device->ds3dl.vVelocity.z);
	*lpvVelocity = This->dsound->device->ds3dl.vVelocity;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_SetAllParameters(
	LPDIRECTSOUND3DLISTENER iface,
	LPCDS3DLISTENER lpcDS3DL,
	DWORD dwApply)
{
	IDirectSound3DListenerImpl *This = (IDirectSound3DListenerImpl *)iface;
	TRACE("setting: all parameters; dwApply = %ld\n", dwApply);
	This->dsound->device->ds3dl = *lpcDS3DL;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->dsound->device->ds3dl_need_recalc = FALSE;
		DSOUND_ChangeListener(This);
	}
	This->dsound->device->ds3dl_need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_SetDistanceFactor(
	LPDIRECTSOUND3DLISTENER iface,
	D3DVALUE fDistanceFactor,
	DWORD dwApply)
{
	IDirectSound3DListenerImpl *This = (IDirectSound3DListenerImpl *)iface;
	TRACE("setting: Distance Factor = %f; dwApply = %ld\n", fDistanceFactor, dwApply);
	This->dsound->device->ds3dl.flDistanceFactor = fDistanceFactor;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->dsound->device->ds3dl_need_recalc = FALSE;
		DSOUND_ChangeListener(This);
	}
	This->dsound->device->ds3dl_need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_SetDopplerFactor(
	LPDIRECTSOUND3DLISTENER iface,
	D3DVALUE fDopplerFactor,
	DWORD dwApply)
{
	IDirectSound3DListenerImpl *This = (IDirectSound3DListenerImpl *)iface;
	TRACE("setting: Doppler Factor = %f; dwApply = %ld\n", fDopplerFactor, dwApply);
	This->dsound->device->ds3dl.flDopplerFactor = fDopplerFactor;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->dsound->device->ds3dl_need_recalc = FALSE;
		DSOUND_ChangeListener(This);
	}
	This->dsound->device->ds3dl_need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_SetOrientation(
	LPDIRECTSOUND3DLISTENER iface,
	D3DVALUE xFront, D3DVALUE yFront, D3DVALUE zFront,
	D3DVALUE xTop, D3DVALUE yTop, D3DVALUE zTop,
	DWORD dwApply)
{
	IDirectSound3DListenerImpl *This = (IDirectSound3DListenerImpl *)iface;
	TRACE("setting: Front vector = (%f,%f,%f); Top vector = (%f,%f,%f); dwApply = %ld\n", \
	xFront, yFront, zFront, xTop, yTop, zTop, dwApply);
	This->dsound->device->ds3dl.vOrientFront.x = xFront;
	This->dsound->device->ds3dl.vOrientFront.y = yFront;
	This->dsound->device->ds3dl.vOrientFront.z = zFront;
	This->dsound->device->ds3dl.vOrientTop.x = xTop;
	This->dsound->device->ds3dl.vOrientTop.y = yTop;
	This->dsound->device->ds3dl.vOrientTop.z = zTop;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->dsound->device->ds3dl_need_recalc = FALSE;
		DSOUND_ChangeListener(This);
	}
	This->dsound->device->ds3dl_need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_SetPosition(
	LPDIRECTSOUND3DLISTENER iface,
	D3DVALUE x, D3DVALUE y, D3DVALUE z,
	DWORD dwApply)
{
	IDirectSound3DListenerImpl *This = (IDirectSound3DListenerImpl *)iface;
	TRACE("setting: Position vector = (%f,%f,%f); dwApply = %ld\n", x, y, z, dwApply);
	This->dsound->device->ds3dl.vPosition.x = x;
	This->dsound->device->ds3dl.vPosition.y = y;
	This->dsound->device->ds3dl.vPosition.z = z;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->dsound->device->ds3dl_need_recalc = FALSE;
		DSOUND_ChangeListener(This);
	}
	This->dsound->device->ds3dl_need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_SetRolloffFactor(
	LPDIRECTSOUND3DLISTENER iface,
	D3DVALUE fRolloffFactor,
	DWORD dwApply)
{
	IDirectSound3DListenerImpl *This = (IDirectSound3DListenerImpl *)iface;
	TRACE("setting: Rolloff Factor = %f; dwApply = %ld\n", fRolloffFactor, dwApply);
	This->dsound->device->ds3dl.flRolloffFactor = fRolloffFactor;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->dsound->device->ds3dl_need_recalc = FALSE;
		DSOUND_ChangeListener(This);
	}
	This->dsound->device->ds3dl_need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_SetVelocity(
	LPDIRECTSOUND3DLISTENER iface,
	D3DVALUE x, D3DVALUE y, D3DVALUE z,
	DWORD dwApply)
{
	IDirectSound3DListenerImpl *This = (IDirectSound3DListenerImpl *)iface;
	TRACE("setting: Velocity vector = (%f,%f,%f); dwApply = %ld\n", x, y, z, dwApply);
	This->dsound->device->ds3dl.vVelocity.x = x;
	This->dsound->device->ds3dl.vVelocity.y = y;
	This->dsound->device->ds3dl.vVelocity.z = z;
	if (dwApply == DS3D_IMMEDIATE)
	{
		This->dsound->device->ds3dl_need_recalc = FALSE;
		DSOUND_ChangeListener(This);
	}
	This->dsound->device->ds3dl_need_recalc = TRUE;
	return DS_OK;
}

static HRESULT WINAPI IDirectSound3DListenerImpl_CommitDeferredSettings(
	LPDIRECTSOUND3DLISTENER iface)
{
	IDirectSound3DListenerImpl *This = (IDirectSound3DListenerImpl *)iface;
	TRACE("\n");
	DSOUND_ChangeListener(This);
	return DS_OK;
}

static const IDirectSound3DListenerVtbl ds3dlvt =
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

HRESULT WINAPI IDirectSound3DListenerImpl_Create(
	PrimaryBufferImpl *This,
	IDirectSound3DListenerImpl **pdsl)
{
	IDirectSound3DListenerImpl *dsl;
	TRACE("(%p,%p)\n",This,pdsl);

	dsl = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*dsl));

	if (dsl == NULL) {
		WARN("out of memory\n");
		*pdsl = 0;
		return DSERR_OUTOFMEMORY;
	}

	dsl->ref = 0;
	dsl->lpVtbl = &ds3dlvt;

	dsl->dsound = This->dsound;

	dsl->dsound->device->ds3dl.dwSize = sizeof(DS3DLISTENER);
	dsl->dsound->device->ds3dl.vPosition.x = 0.0;
	dsl->dsound->device->ds3dl.vPosition.y = 0.0;
	dsl->dsound->device->ds3dl.vPosition.z = 0.0;
	dsl->dsound->device->ds3dl.vVelocity.x = 0.0;
	dsl->dsound->device->ds3dl.vVelocity.y = 0.0;
	dsl->dsound->device->ds3dl.vVelocity.z = 0.0;
	dsl->dsound->device->ds3dl.vOrientFront.x = 0.0;
	dsl->dsound->device->ds3dl.vOrientFront.y = 0.0;
	dsl->dsound->device->ds3dl.vOrientFront.z = 1.0;
	dsl->dsound->device->ds3dl.vOrientTop.x = 0.0;
	dsl->dsound->device->ds3dl.vOrientTop.y = 1.0;
	dsl->dsound->device->ds3dl.vOrientTop.z = 0.0;
	dsl->dsound->device->ds3dl.flDistanceFactor = DS3D_DEFAULTDISTANCEFACTOR;
	dsl->dsound->device->ds3dl.flRolloffFactor = DS3D_DEFAULTROLLOFFFACTOR;
	dsl->dsound->device->ds3dl.flDopplerFactor = DS3D_DEFAULTDOPPLERFACTOR;

	dsl->dsound->device->ds3dl_need_recalc = TRUE;

	*pdsl = dsl;
	return S_OK;
}
