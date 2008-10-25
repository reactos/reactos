<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="vgaddi" type="kernelmodedll" entrypoint="DrvEnableDriver@12" installbase="system32" installname="vgaddi.dll">
	<importlibrary definition="vgaddi.spec" />
	<include base="vgaddi">.</include>
	<library>libcntpr</library>
	<library>win32k</library>
	<directory name="main">
		<file>enable.c</file>
	</directory>
	<directory name="objects">
		<file>screen.c</file>
		<file>pointer.c</file>
		<file>lineto.c</file>
		<file>paint.c</file>
		<file>bitblt.c</file>
		<file>transblt.c</file>
		<file>offscreen.c</file>
		<file>copybits.c</file>
	</directory>
	<directory name="vgavideo">
		<file>vgavideo.c</file>
	</directory>
	<file>vgaddi.rc</file>
	<file>vgaddi.spec</file>
</module>
