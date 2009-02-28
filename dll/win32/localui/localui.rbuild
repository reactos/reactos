<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="localui" type="win32dll" baseaddress="${BASEADDRESS_LOCALUI}" installbase="system32" installname="localui.dll" allowwarnings="true">
	<importlibrary definition="localui.spec" />
	<include base="localui">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>localui.c</file>
	<file>localui.rc</file>
	<library>wine</library>
	<library>winspool</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>uuid</library>
	<library>ntdll</library>
</module>
</group>
