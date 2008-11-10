<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="getuname" type="win32dll" baseaddress="${BASEADDRESS_GETUNAME}" installbase="system32" installname="getuname.dll" unicode="yes">
	<importlibrary definition="getuname.spec" />
	<include base="getuname">.</include>
	<library>ntdll</library>
	<file>getuname.c</file>
	<file>getuname.rc</file>
</module>
