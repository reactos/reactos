<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="credui" type="win32dll" baseaddress="${BASEADDRESS_CREDUI}" installbase="system32" installname="credui.dll" allowwarnings="true">
	<importlibrary definition="credui.spec" />
	<include base="credui">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>credui_main.c</file>
	<file>credui.rc</file>
	<library>wine</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
