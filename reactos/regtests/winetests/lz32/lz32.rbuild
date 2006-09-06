<module name="lz32_winetest" type="win32cui" installbase="bin" installname="lz32_winetest.exe" allowwarnings="true">
    <include base="lz32_winetest">.</include>
    <define name="__USE_W32API" />
    <library>kernel32</library>
    <library>ntdll</library>
    <library>lz32</library>
    <file>testlist.c</file>
    <file>lzexpand_main.c</file>
</module>
