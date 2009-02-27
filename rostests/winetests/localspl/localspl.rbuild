<module name="localspl_winetest" type="win32cui" installbase="bin" installname="localspl_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="localspl_winetest">.</include>
	<file>localmon.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>ntdll</library>
</module>
