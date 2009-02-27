<module name="secur32_winetest" type="win32cui" installbase="bin" installname="secur32_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="secur32_winetest">.</include>
	<file>main.c</file>
	<file>ntlm.c</file>
	<file>schannel.c</file>
	<file>secur32.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
