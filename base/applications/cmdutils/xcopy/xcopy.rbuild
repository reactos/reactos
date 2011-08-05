<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="xcopy" type="win32cui" installbase="system32" installname="xcopy.exe" unicode="true">
	<include base="xcopy">.</include>
	<library>wine</library>
	<library>shell32</library>
	<library>user32</library>
	<file>xcopy.c</file>
	<file>rsrc.rc</file>
	<metadata description="xcopy command-line tool" />
</module>
