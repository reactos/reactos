<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="setupapi_winetest" type="win32cui" installbase="bin" installname="setupapi_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
    <include base="setupapi_winetest">.</include>
    <define name="__USE_W32API" />
    <library>ntdll</library>
    <library>kernel32</library>
    <library>advapi32</library>
    <library>setupapi</library>
    <library>user32</library>
    <file>devclass.c</file>
    <file>devinst.c</file>
    <file>install.c</file>
    <file>misc.c</file>
    <file>parser.c</file>
    <file>query.c</file>
    <file>stringtable.c</file>
    <file>testlist.c</file>
</module>
</group>
