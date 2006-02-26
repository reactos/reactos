<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="regtests" type="win32dll" baseaddress="${BASEADDRESS_REGTESTS}">
	<importlibrary definition="regtests.def" />
	<include base="regtests">.</include>
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<file>regtests.c</file>
</module>
</rbuild>
