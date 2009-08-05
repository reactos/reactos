<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="crt_regtest" type="win32cui" installbase="bin" installname="crt_regtest.exe">
	<include base="crt_regtest">.</include>
	<library>wine</library>
	<library>msvcrt</library>
	<file>iofuncs.c</file>
	<file>testlist.c</file>
	<file>time.c</file>
</module>
</group>
