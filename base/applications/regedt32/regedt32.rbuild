<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="regedt32" type="win32gui" installbase="system32" installname="regedt32.exe" unicode="yes">
	<include base="regedt32">.</include>
	<library>kernel32</library>
	<library>shell32</library>
	<library>shlwapi</library>
	<file>regedt32.c</file>
	<file>resource.rc</file>
</module>
