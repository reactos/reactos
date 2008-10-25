<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="schannel" type="win32dll" baseaddress="${BASEADDRESS_SCHANNEL}" installbase="system32" installname="schannel.dll" allowwarnings="true">
	<importlibrary definition="schannel.spec" />
	<include base="schannel">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<library>wine</library>
	<library>secur32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>lsamode.c</file>
	<file>schannel_main.c</file>
	<file>usermode.c</file>
	<file>version.rc</file>
	<file>schannel.spec</file>
</module>
