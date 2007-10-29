<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="starfield" type="win32scr" installbase="system32" installname="starfield.scr" unicode="true">
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>

	<metadata description = "Starfield simulation screensaver" />

	<file>screensaver.c</file>
	<file>starfield.rc</file>
</module>
