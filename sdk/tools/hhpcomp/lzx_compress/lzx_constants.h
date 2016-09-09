/*
    File lzx_constants.h, part of lzxcomp library

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; version 2.1 only

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    --------------------------------------
    The above lines apply to the lzxcomp library as a whole.  This file,
    lzx_constants.h, however, is probably uncopyrightable, and in any
    case I explicitly place it in the public domain. 

    Matthew T. Russotto
*/

/* these named constants are from the Microsoft LZX documentation */
#define MIN_MATCH                            2
#define MAX_MATCH                          257
#define NUM_CHARS                          256
#define NUM_PRIMARY_LENGTHS                  7
#define NUM_SECONDARY_LENGTHS              249

/* the names of these constants are specific to this library */
#define LZX_MAX_CODE_LENGTH                 16
#define LZX_FRAME_SIZE                   32768
#define LZX_PRETREE_SIZE                    20
#define LZX_ALIGNED_BITS                     3
#define LZX_ALIGNED_SIZE                     8

#define LZX_VERBATIM_BLOCK                   1
#define LZX_ALIGNED_OFFSET_BLOCK             2
