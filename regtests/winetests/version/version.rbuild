<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="version_winetest" type="win32cui" installbase="bin" installname="version_winetest.exe" allowwarnings="true">
    <include base="version_winetest">.</include>
    <define name="__USE_W32API" />
    <library>ntdll</library>
    <library>version</library>
    <file>testlist.c</file>
    <file>info.c</file>
</module>
</rbuild>
