<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="getuname" type="win32dll" baseaddress="${BASEADDRESS_GETUNAME}" installbase="system32" installname="getuname.dll" unicode="yes">
	<importlibrary definition="getuname.spec.def" />
	<include base="getuname">.</include>
	<define name="_WIN32_IE">0x600</define>
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<library>ntdll</library>
	<file>getuname.c</file>
	<file>getuname.rc</file>
	<file>getuname.spec</file>
</module>
