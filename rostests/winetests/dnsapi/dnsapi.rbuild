<module name="dnsapi_winetest" type="win32cui" installbase="bin" installname="dnsapi_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="dnsapi_winetest">.</include>
	<file>name.c</file>
	<file>record.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>dnsapi</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
