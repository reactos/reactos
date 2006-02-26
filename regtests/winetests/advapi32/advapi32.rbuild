<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="advapi32_winetest" type="win32cui" installbase="bin" installname="advapi32_winetest.exe" allowwarnings="true">
    <include base="advapi32_winetest">.</include>
    <define name="__USE_W32API" />
    <library>advapi32</library>
    <library>ntdll</library>
    <file>registry.c</file>
    <file>security.c</file>
    <file>testlist.c</file>
</module>
</rbuild>
