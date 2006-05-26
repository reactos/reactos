<module name="setupapi_winetest" type="win32cui" installbase="bin" installname="setupapi_winetest.exe" allowwarnings="true">
    <include base="setupapi_winetest">.</include>
    <define name="__USE_W32API" />
    <library>ntdll</library>
    <library>setupapi</library>
    <file>parser.c</file>
    <file>query.c</file>
    <file>stringtable.c</file>
    <file>testlist.c</file>
</module>
