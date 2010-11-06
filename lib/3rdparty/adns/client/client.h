/*
 * clients.h
 * - useful declarations and definitions for adns client programs
 */
/*
 *  This file is
 *    Copyright (C) 1997-2000 Ian Jackson <ian@davenant.greenend.org.uk>
 *
 *  It is part of adns, which is
 *    Copyright (C) 1997-2000 Ian Jackson <ian@davenant.greenend.org.uk>
 *    Copyright (C) 1999-2000 Tony Finch <dot@dotat.at>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDED

#define ADNS_VERSION_STRING "1.0"

#define COPYRIGHT_MESSAGE \
 "Copyright (C) 1997-2000 Ian Jackson <ian@davenant.greenend.org.uk>\n" \
 "Copyright (C) 1999-2000 Tony Finch <dot@dotat.at>\n" \
 "This is free software; see the source for copying conditions.  There is NO\n" \
 "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"

#define VERSION_MESSAGE(program) \
 program " (GNU adns) " ADNS_VERSION_STRING "\n\n" COPYRIGHT_MESSAGE

#define VERSION_PRINT_QUIT(program)                               \
  if (fputs(VERSION_MESSAGE(program),stdout) == EOF ||            \
      fclose(stdout)) {                                           \
    perror(program ": write version message");                    \
    quitnow(-1);                                                  \
  }                                                               \
  quitnow(0);

void quitnow(int rc) NONRETURNING;

#endif
