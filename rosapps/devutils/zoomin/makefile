# $Id: makefile,v 1.2 2004/06/02 02:27:04 royce Exp $
#
#  ReactOS zoomin
#
#  Makefile
#
#  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

PATH_TO_TOP = ../../../reactos

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = zoomin

TARGET_CFLAGS = -D_WIN32_IE=0x0501 -D_WIN32_WINNT=0x0501 -D__USE_W32API 

TARGET_SDKLIBS = kernel32.a gdi32.a user32.a

TARGET_OBJECTS = framewnd.o main.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
