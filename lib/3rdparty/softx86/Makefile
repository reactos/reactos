# Linux makefile for compiling SOFTX86
# (C) 2003 Jonathan Campbell all rights reserved.
#
# Requirements:
# GCC 3.xx
# NASM 0.95 and better
#
# This Makefile really just calls Make to compile sub-projects.

# make it all
all:: __autodetect_softx87 __autodetect_nasm
	make -C samples
	make -C softx86
	if [ -d softx87 ]; then make -C softx87; fi
	make -C softx86dbg

# clean it all
clean:: __fake_makes
	make -C samples clean
	make -C softx86 clean
	if [ -d softx87 ]; then make -C softx87 clean; fi
	make -C softx86dbg clean

# clean it all for distribution
distclean:: __fake_makes
	make -C samples distclean
	make -C softx86 distclean
	if [ -d softx87 ]; then make -C softx87 distclean; fi
	make -C softx86dbg distclean
	rm -f softx86dbg/Makefile.softx86
	rm -f samples/Makefile.nasm
	rm -f lib/*.exp		# clean up excess files from MSVC++
	rm -f softx86.ncb
	rm -f softx86.opt

# copy libsoftx86 and friends to the /usr/include and /usr/lib directories.
install::
	if [ -f install.sh ]; then sh install.sh; fi

# run shell script that looks for soft87 project source
# tree (to be extracted within this source tree)
__autodetect_softx87:
	./autodetect_softx87.sh

# run shell script to look for NASM
__autodetect_nasm:
	./autodetect_nasm.sh

# create temporary empty versions of the
# include Makefiles that some rely on
# so they work. They will be deleted when
# "make clean" or "make distclean" is finished
__fake_makes:
	if [[ !( -f samples/Makefile.nasm ) ]]; then echo "# nothing" >samples/Makefile.nasm; fi
	if [[ !( -f softx86dbg/Makefile.softx86 ) ]]; then echo "# nothing" >softx86dbg/Makefile.softx86; fi

