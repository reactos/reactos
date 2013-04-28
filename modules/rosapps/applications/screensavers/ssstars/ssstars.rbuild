<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="ssstars" type="win32scr" installbase="system32" installname="ssstars.scr" unicode="yes">
	<library>scrnsave</library>
	<library>chkstk</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>opengl32</library>
	<library>glu32</library>
	<library>advapi32</library>
	<library>comctl32</library>
	<library>shell32</library>

	<file>ssstars.c</file>
	<file>settings.c</file>
	<file>resource.rc</file>

	<metadata description="Starfield screensaver" />
</module>
