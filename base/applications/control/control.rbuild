<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="control" type="win32gui" baseaddress="${BASEADDRESS_CONTROL}" installbase="system32" installname="control.exe" unicode="yes">
	<include base="control">.</include>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>shell32</library>
	<file>control.c</file>
	<file>control.rc</file>
</module>
