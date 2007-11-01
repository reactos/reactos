<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="framebuf_acc" type="kernelmodedll" entrypoint="_DrvEnableDriver@12" installbase="system32" installname="framebuf.dll">
	<importlibrary definition="framebuf_acc.def" />
	<include base="framebuf_acc">.</include>
	<library>win32k</library>
	<library>libcntpr</library>
	<file>enable.c</file>
	<file>palette.c</file>
	<file>pointer.c</file>
	<file>screen.c</file>
	<file>surface.c</file>
	<file>framebuf_acc.rc</file>
</module>
