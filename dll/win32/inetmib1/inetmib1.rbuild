<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="inetmib1" type="win32dll" baseaddress="${BASEADDRESS_INETMIB1}" installbase="system32" installname="inetmib1.dll" allowwarnings="true">
	<importlibrary definition="inetmib1.spec" />
	<include base="inetmib1">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>main.c</file>
	<library>wine</library>
	<library>snmpapi</library>
	<library>kernel32</library>
	<library>iphlpapi</library>
	<library>ntdll</library>
</module>
</group>
