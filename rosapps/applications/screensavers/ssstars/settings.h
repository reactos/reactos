/*
 * Star field screensaver
 *
 * Copyright 2011 Carlo Bramini
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

#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#define ROTATION_NONE       0
#define ROTATION_LINEAR     1
#define ROTATION_PERIODIC   2
#define ROTATION_ITEMS      3

#define MIN_STARS           10
#define MAX_STARS           500

#define MIN_SPEED           1
#define MAX_SPEED           100

typedef struct {
    DWORD   uiNumStars;
    DWORD   uiSpeed;
    DWORD   uiRotation;
    DWORD   bDoBlending;
    DWORD   bFinePerspective;
    DWORD   bEnableFiltering;
    DWORD   bSmoothShading;
} SSSTARS;

extern SSSTARS Settings;

void LoadSettings(void);
void SaveSettings(void);

#endif
