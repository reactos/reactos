<module name="cryptnet_winetest" type="win32cui" installbase="bin" installname="cryptnet_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="cryptnet_winetest">.</include>
	<file>cryptnet.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>cryptnet</library>
	<library>crypt32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
