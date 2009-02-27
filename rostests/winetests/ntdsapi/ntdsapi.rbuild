<module name="ntdsapi_winetest" type="win32cui" installbase="bin" installname="ntdsapi_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="ntdsapi_winetest">.</include>
	<file>ntdsapi.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>ntdsapi</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
