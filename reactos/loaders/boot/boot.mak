#
# makefile for DOS-C boot
#
# $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/loaders/boot/Attic/boot.mak,v 1.4 2000/06/25 03:59:14 dwelch Exp $
#
# $Log: boot.mak,v $
# Revision 1.4  2000/06/25 03:59:14  dwelch
# Removed from redundant files from the mm directory
# Added some preliminary work on the pager
# Fixed ntoskrnl/mm/npool.c (This may have been the cause of the
# problems reported with loading win32k.sys)
# Fixed problems with reporting space used to store physical page
# information
# Added code to support MmSafeCopy{To/From}User interface work
# (untested)
# Added Event member of the PHYSICAL_PAGE structure to implement Philip
# Susi's suggestion
# Reworked section page-in code (not really tested)
# Replaced inline string functions with gcc builtins to make debugging easier
#
# Revision 1.3  1998/08/25 04:39:40  rex
# Release cleanup
#
# Revision 1.1.1.2  1998/08/25 04:27:38  rex
# A much Needed Update
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

