<module name="msctf_winetest" type="win32cui" installbase="bin" installname="msctf_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="msctf_winetest">.</include>
	<file>inputprocessor.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>ole32</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
