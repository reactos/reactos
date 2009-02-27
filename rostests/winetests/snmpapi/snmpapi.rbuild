<module name="snmpapi_winetest" type="win32cui" installbase="bin" installname="snmpapi_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="snmpapi_winetest">.</include>
	<file>util.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>snmpapi</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
