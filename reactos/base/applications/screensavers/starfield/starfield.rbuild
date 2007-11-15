<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="starfield" type="win32scr" installbase="system32" installname="starfield.scr">
	<define name="__USE_W32API" />
	<define name="UNICODE" />
	<define name="_UNICODE" />

	<family>screensavers</family>

	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>

	<metadata description = "Starfield simulation screensaver" />

	<file>screensaver.c</file>
	<file>starfield.rc</file>
</module>
