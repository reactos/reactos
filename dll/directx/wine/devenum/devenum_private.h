/*
 *	includes for devenum.dll
 *
 * Copyright (C) 2002 John K. Hohm
 * Copyright (C) 2002 Robert Shearman
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
 *
 * NOTES ON FILE:
 * - Private file where devenum globals are declared
 */

#define COBJMACROS
#include "dshow.h"
#include "dmo.h"
#include "dmodshow.h"

enum device_type
{
    DEVICE_FILTER,
    DEVICE_CODEC,
    DEVICE_DMO,
};

struct moniker
{
    IMoniker IMoniker_iface;
    LONG ref;
    CLSID class;
    BOOL has_class;
    enum device_type type;
    WCHAR *name;    /* for filters and codecs */
    CLSID clsid;    /* for DMOs */

    IPropertyBag IPropertyBag_iface;
};

HRESULT create_filter_data(VARIANT *var, REGFILTER2 *rgf);
struct moniker *dmo_moniker_create(const GUID class, const GUID clsid);
struct moniker *codec_moniker_create(const GUID *class, const WCHAR *name);
struct moniker *filter_moniker_create(const GUID *class, const WCHAR *name);
HRESULT enum_moniker_create(REFCLSID class, IEnumMoniker **enum_mon);

extern ICreateDevEnum devenum_factory;
extern IParseDisplayName devenum_parser;
