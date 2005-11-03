/* $Id: path.h,v 1.4 2002/10/29 04:45:13 rex Exp $
 */
/*
 * psx/path.h
 *
 * POSIX+ subsystem path functions
 *
 * This file is part of the ReactOS Operating System.
 *
 * Contributors:
 *  Created by KJK::Hyperion <noog@libero.it>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#ifndef __PSX_PATH_H_INCLUDED__
#define __PSX_PATH_H_INCLUDED__

/* INCLUDES */
#include <ddk/ntddk.h>

/* OBJECTS */

/* TYPES */

/* CONSTANTS */

/* PROTOTYPES */
BOOLEAN
__PdxPosixPathGetNextComponent_U
(
 IN UNICODE_STRING PathName,
 IN OUT PUNICODE_STRING PathComponent,
 OUT PBOOLEAN TrailingDelimiter OPTIONAL
);

BOOLEAN
__PdxPosixPathResolve_U
(
 IN UNICODE_STRING PathName,
 OUT PUNICODE_STRING ResolvedPathName,
 IN WCHAR PathDelimiter OPTIONAL
);

BOOLEAN
__PdxPosixPathGetNextComponent_A
(
 IN ANSI_STRING PathName,
 IN OUT PANSI_STRING PathComponent,
 OUT PBOOLEAN TrailingDelimiter OPTIONAL
);

BOOLEAN
__PdxPosixPathResolve_A
(
 IN ANSI_STRING PathName,
 OUT PANSI_STRING ResolvedPathName,
 IN CHAR PathDelimiter OPTIONAL
);

BOOLEAN
__PdxPosixPathNameToNtPathName
(
 IN PWCHAR PosixPath,
	OUT PUNICODE_STRING NativePath,
	IN PUNICODE_STRING CurDir OPTIONAL,
 IN PUNICODE_STRING RootDir OPTIONAL
);

/* MACROS */
/* returns non-zero if the argument is a path delimiter */
#define IS_CHAR_DELIMITER_U(WCH) (((WCH) == L'/') || ((WCH) == L'\\'))
#define IS_CHAR_DELIMITER_A(CH)  (((CH) == '/') || ((CH) == '\\'))

/* returns non-zero if the argument is an empty path component */
#define IS_COMPONENT_EMPTY_U(WCOMPONENT) (WCOMPONENT.Length == 0)
#define IS_COMPONENT_EMPTY_A(COMPONENT)  (COMPONENT.Length == 0)

/* returns non-zero if the argument is "." */
#define IS_COMPONENT_DOT_U(WCOMPONENT) \
((WCOMPONENT.Length == sizeof(WCHAR)) && (WCOMPONENT.Buffer[0] == L'.'))

#define IS_COMPONENT_DOT_A(COMPONENT) \
((COMPONENT.Length == 1) && (COMPONENT.Buffer[0] == '.'))

/* returns non-zero if the argument is ".." */
#define IS_COMPONENT_DOTDOT_U(WCOMPONENT) \
( \
 (WCOMPONENT.Length == (sizeof(WCHAR) * 2)) && \
 (WCOMPONENT.Buffer[0] == L'.') && \
 (WCOMPONENT.Buffer[1] == L'.') \
)

#define IS_COMPONENT_DOTDOT_A(COMPONENT) \
( \
 (COMPONENT.Length == 2) && \
 (COMPONENT.Buffer[0] == '.') && \
 (COMPONENT.Buffer[1] == '.') \
)

#endif /* __PSX_PATH_H_INCLUDED__ */

/* EOF */

