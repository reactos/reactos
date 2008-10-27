<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="reactos" type="win32gui">
	<bootstrap installbase="$(CDOUTPUT)" />
	<include base="reactos">.</include>
	<library>kernel32</library>
	<library>gdi32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>setupapi</library>
	<file>reactos.c</file>
	<file>reactos.rc</file>
</module>
