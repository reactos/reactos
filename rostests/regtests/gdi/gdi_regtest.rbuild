<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="gdi_regtest" type="win32cui" installbase="bin" installname="gdi_regtest.exe">
	<include base="gdi_regtest">.</include>
	<library>wine</library>
	<library>gdi32</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>ntdll</library>

	<file>xlate.c</file>
	<file>testlist.c</file>
</module>
</group>
