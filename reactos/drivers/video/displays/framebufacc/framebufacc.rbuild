<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="framebufacc" type="kernelmodedll" entrypoint="DrvEnableDriver@12" installbase="system32" installname="framebuf.dll" crt="libcntpr">
	<importlibrary definition="framebufacc.spec" />
	<include base="framebufacc">.</include>
	<library>win32k</library>
	<file>enable.c</file>
	<file>palette.c</file>
	<file>pointer.c</file>
	<file>screen.c</file>
	<file>surface.c</file>
	<file>framebufacc.rc</file>
</module>
