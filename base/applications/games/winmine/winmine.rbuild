<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="winmine" type="win32gui" installbase="system32" installname="winmine.exe" unicode="no">
	<include base="winmine">.</include>
	<library>wine</library>
	<library>gdi32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>shell32</library>
	<file>main.c</file>
	<file>dialog.c</file>
	<file>rsrc.rc</file>
</module>
