<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="ntdll_winetest" type="win32cui" installbase="bin" installname="ntdll_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
    <include base="ntdll_winetest">.</include>
    <define name="__USE_W32API" />
    <library>kernel32</library>
    <library>ntdll</library>
    <file>atom.c</file>
    <file>change.c</file>
    <file>env.c</file>
    <file>error.c</file>
    <file>exception.c</file>
    <file>file.c</file>
    <file>info.c</file>
    <file>large_int.c</file>
    <file>om.c</file>
    <file>path.c</file>
    <file>port.c</file>
    <file>reg.c</file>
    <file>rtlbitmap.c</file>
    <file>rtl.c</file>
    <file>rtlstr.c</file>
    <file>string.c</file>
    <file>time.c</file>
    <file>testlist.c</file>
</module>
</group>
