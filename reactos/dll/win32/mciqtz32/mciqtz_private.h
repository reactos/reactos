/*
 * DirectShow MCI Driver
 *
 * Copyright 2009 Christian Costa
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

#ifndef __WINE_PRIVATE_MCIQTZ_H
#define __WINE_PRIVATE_MCIQTZ_H

#define COBJMACROS

#include "dshow.h"

typedef struct {
    MCIDEVICEID    wDevID;
    IGraphBuilder* pgraph;
    IMediaControl* pmctrl;
    BOOL           started;
} WINE_MCIQTZ;

#endif  /* __WINE_PRIVATE_MCIQTZ_H */
