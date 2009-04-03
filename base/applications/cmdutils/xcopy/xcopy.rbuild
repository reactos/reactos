<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="xcopy" type="win32cui" installbase="system32" installname="xcopy.exe" unicode="true">
	<include base="xcopy">.</include>
	<library>wine</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<file>xcopy.c</file>
	<file>xcopy.rc</file>
	<metadata description="xcopy command-line tool" />
</module>
