<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="winhstb" type="win32gui" installbase="system32" installname="winhlp32.exe" unicode="no">
	<include base="winhstb">.</include>
	<library>user32</library>
	<library>kernel32</library>
	<library>shell32</library>
	<library>shlwapi</library>
	<file>winhstb.c</file>
	<file>winhstb.rc</file>
</module>
