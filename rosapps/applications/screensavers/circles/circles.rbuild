<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="circles" type="win32scr" installbase="system32" unicode="true">
	<importlibrary definition="circles.spec" />
	<include base="circles">.</include>
	<library>scrnsave</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>

	<metadata description = "Simple Circles GDI screensaver" />

	<file>circles.c</file>
	<file>circles.rc</file>
</module>
