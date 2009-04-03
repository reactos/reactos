<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="olesvr32" type="win32dll" baseaddress="${BASEADDRESS_OLESVR32}" installbase="system32" installname="olesvr32.dll" allowwarnings="true" entrypoint="0">
	<importlibrary definition="olesvr32.spec" />
	<include base="olesvr32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>olesvr_main.c</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
