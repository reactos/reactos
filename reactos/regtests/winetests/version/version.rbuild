<module name="version_winetest" type="win32cui" installbase="bin" installname="version_winetest.exe" allowwarnings="true">
    <include base="version_winetest">.</include>
    <define name="__USE_W32API" />
    <library>ntdll</library>
    <library>version</library>
    <library>kernel32</library>
    <file>testlist.c</file>
    <file>info.c</file>
</module>
