<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="wsock32" type="win32dll" baseaddress="${BASEADDRESS_WSOCK32}" installbase="system32" installname="wsock32.dll">
	<importlibrary definition="wsock32.def" />
	<include base="wsock32">.</include>
	<define name="UNICODE" />
	<define name="__USE_W32API" />
	<library>ntdll</library>
	<library>kernel32</library>
	<library>ws2_32</library>
	<file>stubs.c</file>
	<file>wsock32.rc</file>
</module>
</rbuild>
