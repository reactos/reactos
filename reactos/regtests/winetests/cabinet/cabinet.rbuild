<module name="cabinet_winetest" type="win32cui" installbase="bin" installname="cabinet_winetest.exe" allowwarnings="true">
    <include base="cabinet_winetest">.</include>
    <define name="__USE_W32API" />
    <library>cabinet</library>
    <library>kernel32</library>
    <library>ntdll</library>
    <file>extract.c</file>
    <file>testlist.c</file>
</module>
