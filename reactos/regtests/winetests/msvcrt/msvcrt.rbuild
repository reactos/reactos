<module name="msvcrt_winetest" type="win32cui" installbase="bin" installname="msvcrt_winetest.exe" allowwarnings="true">
    <include base="msvcrt_winetest">.</include>
    <define name="__USE_W32API" />
    <library>kernel32</library>
    <library>ntdll</library>
    <file>cpp.c</file>
    <file>dir.c</file>
    <file>environ.c</file>
    <file>file.c</file>
    <file>heap.c</file>
    <file>printf.c</file>
    <file>scanf.c</file>
    <file>string.c</file>
    <file>testlist.c</file>
    <file>time.c</file>
</module>
