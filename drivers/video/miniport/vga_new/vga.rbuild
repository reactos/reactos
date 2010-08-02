<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="vga" type="kernelmodedriver" installbase="system32/drivers" installname="vga.sys">
	<include base="vga">.</include>
	<library>videoprt</library>
	<library>libcntpr</library>
	<file>modeset.c</file>
	<file>vgadata.c</file>
	<file>vga.c</file>
	<file>vbemodes.c</file>
	<file>vbe.c</file>
	<file>vga.rc</file>
	<pch>vga.h</pch>
	<group compilerset="gcc">
        <compilerflag>-mrtd</compilerflag>
        <compilerflag>-fno-builtin</compilerflag>
    </group>
</module>
