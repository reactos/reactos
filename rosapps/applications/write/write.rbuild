<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="write" type="win32gui" installbase="system32" installname="write.exe" unicode="yes">
	<include base="write">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>shell32</library>
	<file>write.c</file>
	<file>rsrc.rc</file>
</module>
