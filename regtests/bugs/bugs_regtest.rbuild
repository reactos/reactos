<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="bugs_regtest" type="win32cui" installbase="bin" installname="bugs_regtest.exe">
	<include base="bugs_regtest">.</include>
	<library>wine</library>
	<library>gdi32</library>
	<file>testlist.c</file>

	<file>bug3481.c</file>
</module>
</group>
