<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="cacls" type="win32cui" installbase="system32" installname="cacls.exe" unicode="true">
	<include base="cacls">.</include>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>ntdll</library>
	<library>user32</library>
	<library>shell32</library>
	<file>cacls.c</file>
	<file>cacls.rc</file>
	<pch>precomp.h</pch>
</module>
