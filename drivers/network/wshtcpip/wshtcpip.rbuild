<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="wshtcpip" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_WSHTCPIP}" installbase="system32" installname="wshtcpip.dll">
	<importlibrary definition="wshtcpip.def"></importlibrary>
	<include base="wshtcpip">.</include>
	<define name="UNICODE" />
	<library>ntdll</library>
	<library>kernel32</library>
	<library>ws2_32</library>
	<file>wshtcpip.c</file>
	<file>wshtcpip.rc</file>
</module>
