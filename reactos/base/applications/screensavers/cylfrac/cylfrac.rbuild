<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="cylfrac" type="win32scr" installbase="system32" installname="cylfrac.scr">
	<define name="__USE_W32API" />
	<define name="UNICODE" />
	<define name="_UNICODE" />

	<family>screensavers</family>

	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>opengl32</library>
	<library>glu32</library>
	<library>winmm</library>

	<file>cylfrac.c</file>
	<file>cylfrac.rc</file>
</module>
