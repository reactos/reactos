<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="oleaut32_winetest" type="win32cui" installbase="bin" installname="oleaut32_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="oleaut32_winetest">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<include base="oleaut32_winetest" root="intermediate">.</include>
	<library>wine</library>
	<library>oleaut32</library>
	<library>ole32</library>
	<library>rpcrt4</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>uuid</library>
	<library>ntdll</library>
	<file>olefont.c</file>
	<file>olepicture.c</file>
	<file>safearray.c</file>
	<file>tmarshal.c</file>
	<file>typelib.c</file>
	<file>usrmarshal.c</file>
	<file>varformat.c</file>
	<file>vartest.c</file>
	<file>vartype.c</file>
	<file>tmarshal.rc</file>
	<file>testlist.c</file>
	<dependency>tmarshal_header</dependency>
	<dependency>tmarshal</dependency>
	<dependency>test_tlb</dependency>
	<dependency>stdole2.tlb</dependency>
</module>
<module name="tmarshal_header" type="idlheader">
	<file>tmarshal.idl</file>
</module>
<module name="test_tlb" type="embeddedtypelib" allowwarnings="true">
	<file>test_tlb.idl</file>
</module>
<module name="tmarshal" type="embeddedtypelib" allowwarnings="true">
	<file>tmarshal.idl</file>
</module>
</group>
