<module name="powrprof_winetest" type="win32cui" installbase="bin" installname="powrprof_winetest.exe" allowwarnings="true">
    <include base="setupapi_winetest">.</include>
    <define name="__USE_W32API" />
	<define name="UNICODE" />
	<define name="_UNICODE" />
    <library>ntdll</library>
    <library>powrprof</library>
    <file>testlist.c</file>
    <file>pwrprof.c</file>
</module>
