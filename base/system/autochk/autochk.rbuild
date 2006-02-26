<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="autochk" type="nativecui" installbase="system32" installname="autochk.exe">
	<include base="autochk">.</include>
	<define name="__USE_W32API" />
	<define name="_DISABLE_TIDENTS" />
	<library>nt</library>
	<library>ntdll</library>
	<file>autochk.c</file>
	<file>autochk.rc</file>
</module>
</rbuild>
