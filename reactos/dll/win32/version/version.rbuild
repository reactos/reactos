<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="version" type="win32dll" baseaddress="${BASEADDRESS_VERSION}" installbase="system32" installname="version.dll" allowwarnings="true" entrypoint="0">
	<importlibrary definition="version.spec" />
	<include base="version">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>info.c</file>
	<file>install.c</file>
	<file>resource.c</file>
	<file>version.rc</file>
	<file>version.spec</file>
	<library>wine</library>
	<library>lz32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
