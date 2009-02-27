<module name="inetmib1_winetest" type="win32cui" installbase="bin" installname="inetmib1_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="inetmib1_winetest">.</include>
	<file>main.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>snmpapi</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
