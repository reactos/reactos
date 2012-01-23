<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="cabinet" type="win32dll" baseaddress="${BASEADDRESS_CABINET}" installbase="system32" installname="cabinet.dll" allowwarnings="true" entrypoint="0">
	<importlibrary definition="cabinet.spec" />
	<include base="cabinet">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<include base="ReactOS">include/reactos/libs/zlib</include>
	<define name="__WINESRC__" />
	<define name="HAVE_ZLIB" />
	<file>cabinet_main.c</file>
	<file>fci.c</file>
	<file>fdi.c</file>
	<file>stubs.c</file>
	<file>cabinet.rc</file>
	<pch>cabinet.h</pch>
	<library>wine</library>
	<library>zlib</library>
	<library>ntdll</library>
</module>
</group>
