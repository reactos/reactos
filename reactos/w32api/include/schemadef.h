/*
 * Copyright (C) 2003 Kevin Koltzau
 * Copyright (C) 2004 Francois Gouget
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

#ifndef SCHEMADEF_H
#define SCHEMADEF_H

#define BEGIN_TM_SCHEMA(name)
#define END_TM_SCHEMA(name)

#define BEGIN_TM_ENUM(name)                enum name {
#define TM_ENUM(value, prefix, name)           prefix##_##name = (value),
#define END_TM_ENUM()                      };

#define BEGIN_TM_PROPS()                   enum PropValues { \
                                               DummyProp = 49,
#define TM_PROP(value, prefix, name, type)     prefix##_##name = (value),
#define END_TM_PROPS()                     };

#define BEGIN_TM_CLASS_PARTS(name)         enum name##PARTS { \
                                               name##PartFiller0,
#define TM_PART(value, prefix, name)           prefix##_##name = (value),
#define END_TM_CLASS_PARTS()               };

#define BEGIN_TM_PART_STATES(name)         enum name##STATES { \
                                               name##StateFiller0,
#define TM_STATE(value, prefix, name)          prefix##_##name = (value),
#define END_TM_PART_STATES()               };

#endif
