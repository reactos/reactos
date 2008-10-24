<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="rasapi32" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_RASAPI32}" installbase="system32" installname="rasapi32.dll" allowwarnings="true">
	<importlibrary definition="rasapi32.spec" />
	<include base="rasapi32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<file>rasapi.c</file>
	<file>rasapi32.spec</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
