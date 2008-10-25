<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="wshtcpip" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_WSHTCPIP}" installbase="system32" installname="wshtcpip.dll" unicode="yes">
	<importlibrary definition="wshtcpip.spec" />
	<include base="wshtcpip">.</include>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>ws2_32</library>
	<file>wshtcpip.c</file>
	<file>wshtcpip.rc</file>
	<file>wshtcpip.spec</file>
</module>
