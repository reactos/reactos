<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="credui" type="win32dll" baseaddress="${BASEADDRESS_CREDUI}" installbase="system32" installname="credui.dll" allowwarnings="true">
	<importlibrary definition="credui.spec" />
	<include base="credui">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<file>credui_main.c</file>
	<file>credui.rc</file>
	<file>credui.spec</file>
	<library>wine</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
