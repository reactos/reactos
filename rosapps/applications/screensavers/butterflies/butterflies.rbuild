<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="butterflies" type="win32scr" installbase="system32" installname="butterflies.scr" allowwarnings="true">
	<importlibrary definition="butterflies.def" />
	<include base="butterflies">.</include>
	<library>scrnsave</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>opengl32</library>
	<library>glu32</library>
	<library>advapi32</library>
        <library>shell32</library>

	<metadata description = "OpenGL Butterflies screensaver" />

	<file>butterflies.c</file>
	<file>butterflies.rc</file>
</module>
