<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="lz32" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_LZ32}" installbase="system32" installname="lz32.dll" allowwarnings="true">
	<importlibrary definition="lz32.spec" />
	<include base="lz32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>version.rc</file>
	<file>lz32.spec</file>
</module>
