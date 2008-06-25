<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="framebufacc" type="kernelmodedll" entrypoint="_DrvEnableDriver@12" installbase="system32" installname="framebuf.dll">
	<importlibrary definition="framebufacc.def" />
	<include base="framebufacc">.</include>
	<library>win32k</library>
	<library>libcntpr</library>
	<file>enable.c</file>
	<file>palette.c</file>
	<file>pointer.c</file>
	<file>screen.c</file>
	<file>surface.c</file>
	<file>framebufacc.rc</file>
</module>
