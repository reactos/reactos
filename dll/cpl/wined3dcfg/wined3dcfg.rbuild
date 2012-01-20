<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="wined3dcfg" type="win32dll" extension=".cpl" installbase="system32" installname="wined3dcfg.cpl" crt="msvcrt" unicode="yes">
	<importlibrary definition="wined3dcfg.spec" />
	<include base="wined3dcfg">.</include>
	<library>user32</library>
	<library>comctl32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<file>wined3dcfg.c</file>
	<file>general.c</file>
	<file>wined3dcfg.rc</file>
	<pch>wined3dcfg.h</pch>
</module>
