<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="framebuf_new" type="kernelmodedll" entrypoint="DrvEnableDriver@12" installbase="system32" installname="framebuf_new.dll" crt="libcntpr">
	<importlibrary definition="framebuf_new.spec" />
	<include base="framebuf_new">.</include>
	<library>win32k</library>
	<file>debug.c</file>
	<file>enable.c</file>
	<file>palette.c</file>
	<file>pointer.c</file>
	<file>screen.c</file>
	<file>framebuf_new.rc</file>
	<group compilerset="gcc">
        <compilerflag>-mrtd</compilerflag>
        <compilerflag>-fno-builtin</compilerflag>
		<compilerflag>-Wno-unused-variable</compilerflag>
	</group>
	<pch>driver.h</pch>
</module>
