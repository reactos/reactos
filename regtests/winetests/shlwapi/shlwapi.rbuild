<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="shlwapi_winetest" type="win32cui" installbase="bin" installname="shlwapi_winetest.exe" allowwarnings="true">
    <include base="shlwapi_winetest">.</include>
    <define name="__USE_W32API" />
    <library>ntdll</library>
    <library>shlwapi</library>
    <library>ole32</library>
    <library>oleaut32</library>
    <file>clist.c</file>
    <file>ordinal.c</file>
    <file>shreg.c</file>
    <file>string.c</file>
    <file>testlist.c</file>
</module>
</rbuild>
