<module name="userenv_winetest" type="win32cui" installbase="bin" installname="userenv_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="userenv_winetest">.</include>
	<file>userenv.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>userenv</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
