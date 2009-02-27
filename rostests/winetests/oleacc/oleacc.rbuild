<module name="oleacc_winetest" type="win32cui" installbase="bin" installname="oleacc_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="oleacc_winetest">.</include>
	<file>main.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>oleacc</library>
	<library>ntdll</library>
</module>
