<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="msconfig" type="win32gui" installbase="system32" installname="msconfig.exe" unicode="yes">
	<include base="msconfig">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>version</library>
	<library>comctl32</library>
	<library>shell32</library>
	<library>shlwapi</library>
	<compilationunit name="unit.c">
		<file>toolspage.c</file>
		<file>srvpage.c</file>
		<file>systempage.c</file>
		<file>startuppage.c</file>
		<file>freeldrpage.c</file>
		<file>generalpage.c</file>
		<file>msconfig.c</file>
	</compilationunit>
	<file>msconfig.rc</file>
	<pch>precomp.h</pch>
</module>
