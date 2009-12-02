<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="userinit" type="win32gui" installbase="system32" installname="userinit.exe" unicode="yes">
	<include base="userinit">.</include>
	<library>wine</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>shell32</library>
	<library>shlwapi</library>
	<file>userinit.c</file>
	<file>userinit.rc</file>
</module>
