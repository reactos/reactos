<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="wshtcpip" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_WSHTCPIP}" installbase="system32" installname="wshtcpip.dll" unicode="yes">
	<importlibrary definition="wshtcpip.spec" />
	<include base="wshtcpip">.</include>
	<include base="tdilib">.</include>
	<library>ntdll</library>
	<library>ws2_32</library>
	<library>tdilib</library>
	<file>wshtcpip.c</file>
	<file>wshtcpip.rc</file>
</module>
