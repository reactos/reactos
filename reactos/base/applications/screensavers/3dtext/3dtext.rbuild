<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="3dtext" type="win32scr" installbase="system32" installname="3dtext.scr" unicode="yes">
	<library>scrnsave</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>opengl32</library>
	<library>glu32</library>
	<library>advapi32</library>

	<file>3dtext.c</file>
	<file>settings.c</file>
	<file>rsrc.rc</file>

	<metadata description="3D text OpenGL screensaver" />
</module>
