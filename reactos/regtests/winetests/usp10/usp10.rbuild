<module name="usp10_winetest" type="win32cui" installbase="bin" installname="usp10_winetest.exe" allowwarnings="true">
    <include base="usp10_winetest">.</include>
    <define name="__USE_W32API" />
    <library>ntdll</library>
    <library>gdi32</library>
    <library>kernel32</library>
    <library>user32</library>
    <library>usp10</library>
    <file>usp10.c</file>
    <file>testlist.c</file>
</module>
