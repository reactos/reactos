<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="ks" type="kernelmodedriver" installbase="system32/drivers" installname="ks.sys" warnings="true">
	<include base="ks">.</include>
	<include base="ks">..</include>
	<importlibrary definition="ks.def" />
	<library>ntoskrnl</library>
	<define name="__USE_W32API" />
	<!--file>connect.c</file-->
	<file>avstream.c</file>
</module>
</rbuild>
