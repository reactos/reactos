<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="vmx_svga" type="kernelmodedriver" installbase="system32/drivers" installname="vmx_svga.sys">
	<include base="vmx_svga">.</include>
	<library>videoprt</library>
	<file>vmx_svga.c</file>
	<file>vmx_svga.rc</file>
	<pch>precomp.h</pch>
</module>
