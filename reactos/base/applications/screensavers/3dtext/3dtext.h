/*
 * 3D Text OpenGL Screensaver (3dtext.h)
 *
 * Copyright 2007 Marc Piulachs
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _3DTEXT_H_
#define _3DTEXT_H_

#include <tchar.h>
#include <windef.h>

extern TCHAR m_Text[MAX_PATH];

VOID LoadSettings(VOID);
VOID SaveSettings(VOID);

#endif /* _3DTEXT_H_ */
