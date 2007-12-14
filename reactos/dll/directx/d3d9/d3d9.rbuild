<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="d3d9" type="win32dll" entrypoint="0" installbase="system32" installname="d3d9.dll">
	<importlibrary definition="d3d9.def" />

	<library>advapi32</library>
	<library>kernel32</library>

	<file>d3d9.c</file>
	<file>d3d9_helpers.c</file>
	<file>d3d9.rc</file>
</module>
