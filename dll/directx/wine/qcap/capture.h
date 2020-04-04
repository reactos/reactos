/* DirectShow private capture header (QCAP.DLL)
 *
 * Copyright 2005 Maarten Lankhorst
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

#ifndef __QCAP_CAPTURE_H__
#define __QCAP_CAPTURE_H__

struct _Capture;
typedef struct _Capture Capture;

Capture *qcap_driver_init(IPin*,USHORT) DECLSPEC_HIDDEN;
HRESULT qcap_driver_destroy(Capture*) DECLSPEC_HIDDEN;
HRESULT qcap_driver_set_format(Capture*,AM_MEDIA_TYPE*) DECLSPEC_HIDDEN;
HRESULT qcap_driver_get_format(const Capture*,AM_MEDIA_TYPE**) DECLSPEC_HIDDEN;
HRESULT qcap_driver_get_prop_range(Capture*,VideoProcAmpProperty,LONG*,LONG*,LONG*,LONG*,LONG*) DECLSPEC_HIDDEN;
HRESULT qcap_driver_get_prop(Capture*,VideoProcAmpProperty,LONG*,LONG*) DECLSPEC_HIDDEN;
HRESULT qcap_driver_set_prop(Capture*,VideoProcAmpProperty,LONG,LONG) DECLSPEC_HIDDEN;
HRESULT qcap_driver_run(Capture*,FILTER_STATE*) DECLSPEC_HIDDEN;
HRESULT qcap_driver_pause(Capture*,FILTER_STATE*) DECLSPEC_HIDDEN;
HRESULT qcap_driver_stop(Capture*,FILTER_STATE*) DECLSPEC_HIDDEN;

#endif /* __QCAP_CAPTURE_H__ */
