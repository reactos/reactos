<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="mpr" type="win32dll" baseaddress="${BASEADDRESS_MPR}" installbase="system32" installname="mpr.dll" allowwarnings="true">
	<importlibrary definition="mpr.spec" />
	<include base="mpr">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<file>auth.c</file>
	<file>mpr_main.c</file>
	<file>multinet.c</file>
	<file>nps.c</file>
	<file>pwcache.c</file>
	<file>wnet.c</file>
	<file>mpr.rc</file>
	<file>mpr.spec</file>
	<library>wine</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
