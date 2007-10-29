<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="scrnsave" type="win32scr" installbase="system32" installname="scrnsave.scr" unicode="true">
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>opengl32</library>
	<library>glu32</library>
	<library>winmm</library>

	<file>scrnsave.c</file>
	<file>scrnsave.rc</file>
</module>
