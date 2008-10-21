<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="blankscr" type="win32scr" installbase="system32" installname="scrnsave.scr" unicode="yes">
	<importlibrary definition="scrnsave.spec" />
	<library>scrnsave</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>

	<file>scrnsave.c</file>
	<file>scrnsave.rc</file>
</module>
