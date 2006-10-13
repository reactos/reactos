# $Id$
#
# ReactOS Operating System - .def converter for HAL and the kernel
#

DEF_HAL  = ..\hal\hal.def
DEF_KRNL = ..\ntoskrnl\ntoskrnl.def
CPPFLAGS = /GX

all: def_converter.exe $(DEF_HAL) $(DEF_KRNL)

$(DEF_HAL) : ..\..\reactos\hal\hal\hal.def
	def_converter.exe

$(DEF_KRNL) : ..\..\reactos\ntoskrnl\ntoskrnl.def
	def_converter.exe

def_converter.exe : def_converter.cpp

