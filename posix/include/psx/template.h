/* $Id: template.h,v 1.2 2002/02/20 09:17:55 hyperion Exp $
 How to create a new header file from this template:
  - copy the template in the new file (never edit this file directly, unless
    that's what you want)
  - search for the string "EDITME" in the file, and follow the instructions
  - remove this comment block, all blocks containing DELETEME, and all EDITME
    instructions
  - save your file, and Have Fun! (TM)
 */
/* $ Id $ (EDITME: replace "$ Id $" with "$Id: template.h,v 1.2 2002/02/20 09:17:55 hyperion Exp $")
 */
/*
 * psx/template.h (EDITME: replace with the real name of the header)
 *
 * template for POSIX headers (EDITME: replace this line with the real file
 * description)
 *
 * This file is part of the ReactOS Operating System.
 *
 * Contributors:
 *  Created by John Doe <john.doe@mail.com> (EDITME: your name and e-mail go
 *  here)
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

/*
 Tags are used to prevent double inclusion of C header files. This
 technique should be documented in all good C manuals

 How to generate an unique tag for your header:
  - uppercase the name of the header, where "name" is the filename and
    the optional relative path (e.g. "stdio.h", "sys/types.h")
  - replace all non-alphanumeric characters in the obtained name with an
    underscore character ("_")
  - prepend a double underscore ("__"), and append the string "_INCLUDED__"
  - replace all occurrences of "__PSX_TEMPLATE_H_INCLUDED__" in this file
    with your tag

 Example tags:
  sys/types.h -> SYS/TYPES.H -> SYS_TYPES_H -> __SYS_TYPES_H_INCLUDED__
  iso646.h -> ISO646.H -> ISO646_H -> __ISO646_H_INCLUDED__

 (REMOVEME)
 */
#ifndef __PSX_TEMPLATE_H_INCLUDED__ /* EDITME: replace macro with unique tag */
#define __PSX_TEMPLATE_H_INCLUDED__ /* EDITME: replace macro with unique tag */
/*
 Explanation of the sections:
  INCLUDES   #include directives should be grouped here
  OBJECTS    declare global variables here
  TYPES      types, structures and unions here
  CONSTANTS  symbolic constants (simple #define's), enums, constants
  PROTOTYPES ANSI C function prototypes
  MACROS     parametrized macros

 (REMOVEME)
 */
/* INCLUDES */

/* OBJECTS */

/* TYPES */

/* CONSTANTS */

/* PROTOTYPES */

/* MACROS */

#endif /* __PSX_TEMPLATE_H_INCLUDED__ */ /* EDITME: replace macro with unique tag */

/* EOF */

