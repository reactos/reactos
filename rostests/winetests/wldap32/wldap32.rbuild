<module name="wldap32_winetest" type="win32cui" installbase="bin" installname="wldap32_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="wldap32_winetest">.</include>
	<file>parse.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>wldap32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
