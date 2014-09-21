/*
 *  FreeLoader
 *  Copyright (C) 2009  Hervé Poussineau  <hpoussin@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* component.c */
CONFIGURATION_COMPONENT* ArcGetChild(CONFIGURATION_COMPONENT *Current);
CONFIGURATION_COMPONENT* ArcGetParent(CONFIGURATION_COMPONENT *Current);
CONFIGURATION_COMPONENT* ArcGetPeer(CONFIGURATION_COMPONENT *Current);
CONFIGURATION_COMPONENT* 
ArcAddChild(
    CONFIGURATION_COMPONENT *Current,
    CONFIGURATION_COMPONENT *Template,
    VOID *ConfigurationData);
LONG
ArcDeleteComponent(
    CONFIGURATION_COMPONENT *ComponentToDelete);
LONG
ArcGetConfigurationData(
    VOID* ConfigurationData,
    CONFIGURATION_COMPONENT* Component);

/* time.c */
TIMEINFO* ArcGetTime(VOID);
ULONG ArcGetRelativeTime(VOID);
