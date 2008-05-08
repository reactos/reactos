<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="regsvr32" type="win32gui" installbase="system32" installname="regsvr32.exe" unicode="yes">
	<include base="regsvr32">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>shell32</library>
	<library>ole32</library>
	<file>regsvr32.c</file>
	<file>regsvr32.rc</file>
</module>
