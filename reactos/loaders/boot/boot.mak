#
# makefile for DOS-C boot
#
# $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/loaders/boot/Attic/boot.mak,v 1.2 1998/08/12 22:43:38 rosmgr Exp $
#
# $Log: boot.mak,v $
# Revision 1.2  1998/08/12 22:43:38  rosmgr
# Update from the kernel team
#
#
#   Rev 1.3   10 Jan 1997  4:51:54   patv
#Changed to use FreeDOS exe2bin and support new boot code
#
#   Rev 1.2   17 Dec 1996 12:52:32   patv
#Converted to FreeDOS exe2bin.
#.
#d
#
#   Rev 1.1   29 Aug 1996 13:06:50   patv
#Bug fixes for v0.91b
#
#   Rev 1.0   02 Jul 1995  9:11:26   patv
#Initial revision.
#

#
# Uncomment the following for a debug version
#
#AFLAGS  =	/zi /DDEBUG
#LFLAGS  =	/v

PRODUCT =	boot.bin

all:		$(PRODUCT)

production:	all
		copy boot.bin ..\..\dist\boot.bin
		del *.bin
		del *.map

boot.bin:	boot.asm
		tasm $(AFLAGS) boot,,
		tlink $(LFLAGS) boot
		..\utils\exe2bin boot boot.bin
		del boot.obj
		del boot.exe

clean:
		del *.lst
		del *.map
		del *.bin
		del *.bak
		del *.las
		del *.obj
		del *.exe
