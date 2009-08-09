<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="dwmapi" type="win32dll" baseaddress="${BASEADDRESS_DWMAPI}" installbase="system32" installname="dwmapi.dll" allowwarnings="true">
	<importlibrary definition="dwmapi.spec" />
	<include base="dwmapi">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>dwmapi_main.c</file>
	<file>version.rc</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
