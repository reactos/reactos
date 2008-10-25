<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="compstui" type="win32dll" baseaddress="${BASEADDRESS_COMPSTUI}" installbase="system32" installname="compstui.dll" allowwarnings="true">
	<importlibrary definition="compstui.spec" />
	<include base="compstui">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>compstui_main.c</file>
	<file>compstui.spec</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
