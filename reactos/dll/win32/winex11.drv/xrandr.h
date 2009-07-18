/*
 * Wine X11drv Xrandr interface
 *
 * Copyright 2003 Alexander James Pasadyn
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
#ifndef __WINE_XRANDR_H
#define __WINE_XRANDR_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#ifdef SONAME_LIBXRANDR

void X11DRV_XRandR_Init(void);

#endif /* SONAME_LIBXRANDR */
#endif /* __WINE_XRANDR_H */
