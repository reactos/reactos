<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="blankscr" type="win32scr" installbase="system32" installname="scrnsave.scr">
	<importlibrary definition="scrnsave.def" />
	<library>scrnsave</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>opengl32</library>
	<library>glu32</library>
	<library>advapi32</library>
	<library>shell32</library>

	<file>scrnsave.c</file>
	<file>scrnsave.rc</file>
</module>
