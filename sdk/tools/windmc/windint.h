/* windint.h -- internal header file for windres program.
   Copyright (C) 1997-2026 Free Software Foundation, Inc.
   Written by Kai Tietz, Onevision.

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

/* ReactOS: reduced to the message table definitions used by windmc.  */

#include "winduni.h"

#ifndef WINDINT_H
#define WINDINT_H

struct bin_messagetable_item
{
  bfd_byte length[2];
  bfd_byte flags[2];
  bfd_byte data[1];
};
#define BIN_MESSAGETABLE_ITEM_SIZE  4

#define MESSAGE_RESOURCE_UNICODE  0x0001

struct bin_messagetable_block
{
  bfd_byte lowid[4];
  bfd_byte highid[4];
  bfd_byte offset[4];
};
#define BIN_MESSAGETABLE_BLOCK_SIZE 12

struct bin_messagetable
{
  bfd_byte cblocks[4];
  struct bin_messagetable_block items[1];
};
#define BIN_MESSAGETABLE_SIZE 8

#endif
