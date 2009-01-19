<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="cabinet" type="win32dll" baseaddress="${BASEADDRESS_CABINET}" installbase="system32" installname="cabinet.dll" entrypoint="0">
	<importlibrary definition="cabinet.spec" />
	<include base="cabinet">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>cabinet_main.c</file>
	<file>fci.c</file>
	<file>fdi.c</file>
	<file>cabinet.rc</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
