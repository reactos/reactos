<module name="setupapi_winetest" type="win32cui" installbase="bin" installname="setupapi_winetest.exe" allowwarnings="true">
    <include base="setupapi_winetest">.</include>
    <define name="__USE_W32API" />
    <library>ntdll</library>
    <library>kernel32</library>
    <library>advapi32</library>
    <library>setupapi</library>
    <file>devclass.c</file>
    <file>install.c</file>
    <file>parser.c</file>
    <file>query.c</file>
    <file>stringtable.c</file>
    <file>testlist.c</file>
</module>
