<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="rosautotest" type="win32cui" installbase="system32" installname="rosautotest.exe" unicode="yes">
	<include base="rosautotest">.</include>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>wininet</library>
	<file>main.c</file>
	<file>shutdown.c</file>
	<file>tools.c</file>
	<file>webservice.c</file>
	<file>winetests.c</file>
	<pch>precomp.h</pch>
</module>
