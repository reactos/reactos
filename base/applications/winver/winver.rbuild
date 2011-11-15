<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="winver" type="win32gui" installbase="system32" installname="winver.exe" unicode="yes">
	<include base="winver">.</include>
	<library>shell32</library>
	<file>winver.c</file>
</module>
