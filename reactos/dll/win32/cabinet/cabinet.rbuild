<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="cabinet" type="win32dll" baseaddress="${BASEADDRESS_CABINET}" installbase="system32" installname="cabinet.dll" allowwarnings="true" entrypoint="0">
	<importlibrary definition="cabinet.spec" />
	<include base="cabinet">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<file>cabinet_main.c</file>
	<file>fci.c</file>
	<file>fdi.c</file>
	<file>cabinet.rc</file>
	<file>cabinet.spec</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
