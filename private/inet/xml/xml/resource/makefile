#
# Compiles the message table.  The SOURCES file is ignored in this directory
#

!if "$(BUILD_PRODUCT)" == "IE"
XML_MC_OPT = -A
!endif

messages.rc: messages.mc
	mc $(XML_MC_OPT) messages.mc
	copy messages.* ..\dll
	copy messages.* ..\exe
	copy MSG*.bin ..\dll
	copy MSG*.bin ..\exe

