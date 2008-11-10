<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="winhttp" type="win32dll" baseaddress="${BASEADDRESS_WINHTTP}" installbase="system32" installname="winhttp.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="winhttp.spec" />
	<include base="winhttp">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>handle.c</file>
	<file>main.c</file>
	<file>net.c</file>
	<file>request.c</file>
	<file>session.c</file>
	<library>wine</library>
	<library>wininet</library>
	<library>ws2_32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
