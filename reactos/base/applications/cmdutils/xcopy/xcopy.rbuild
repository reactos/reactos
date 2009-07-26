<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="xcopy" type="win32cui" installbase="system32" installname="xcopy.exe" unicode="true">
	<include base="xcopy">.</include>
	<library>wine</library>
	<library>shell32</library>
	<library>user32</library>
	<library>kernel32</library>
	<file>xcopy.c</file>
	<file>Da.rc</file>
	<file>De.rc</file>
	<file>En.rc</file>
	<file>Fr.rc</file>
	<file>Ko.rc</file>
	<file>Lt.rc</file>
	<file>Nl.rc</file>
	<file>No.rc</file>
	<file>Pl.rc</file>
	<file>Pt.rc</file>
	<file>Ru.rc</file>
	<file>Si.rc</file>
	<metadata description="xcopy command-line tool" />
</module>
