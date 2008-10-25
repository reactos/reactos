<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="msi_winetest" type="win32cui" installbase="bin" installname="msi_winetest.exe" allowwarnings="true">
	<include base="msi_winetest">.</include>
	<file>automation.c</file>
	<file>db.c</file>
	<file>format.c</file>
	<file>install.c</file>
	<file>msi.c</file>
	<file>package.c</file>
	<file>record.c</file>
	<file>source.c</file>
	<file>suminfo.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>uuid</library>
	<library>cabinet</library>
	<library>msi</library>
	<library>shell32</library>
	<library>ole32</library>
	<library>oleaut32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>version</library>
	<library>ntdll</library>
</module>
</group>
