/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: splaytree.c,v 1.1 2004/06/20 23:27:21 gdalsnes Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Splay-Tree implementation
 * FILE:              lib/rtl/splaytree.c
 */

#include <ddk/ntddk.h>

/* FUNCTIONS *****************************************************************/

PRTL_SPLAY_LINKS STDCALL
RtlSubtreePredecessor (IN PRTL_SPLAY_LINKS Links)
{
   PRTL_SPLAY_LINKS Child;

   Child = Links->RightChild;
   if (Child == NULL)
      return NULL;

   if (Child->LeftChild == NULL)
      return Child;

   /* Get left-most child */
   while (Child->LeftChild != NULL)
      Child = Child->LeftChild;

   return Child;
}


PRTL_SPLAY_LINKS STDCALL
RtlSubtreeSuccessor (IN PRTL_SPLAY_LINKS Links)
{
   PRTL_SPLAY_LINKS Child;

   Child = Links->LeftChild;
   if (Child == NULL)
      return NULL;

   if (Child->RightChild == NULL)
      return Child;

   /* Get right-most child */
   while (Child->RightChild != NULL)
      Child = Child->RightChild;

   return Child;
}

/* EOF */
