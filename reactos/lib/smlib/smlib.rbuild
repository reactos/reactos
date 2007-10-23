<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../tools/rbuild/project.dtd">
<module name="smlib" type="staticlibrary">
	<include base="smlib">.</include>
	<include base="ReactOS">include/reactos/subsys</include>
	<define name="_DISABLE_TIDENTS" />
	<file>connect.c</file>
	<file>execpgm.c</file>
	<file>compses.c</file>
	<file>lookupss.c</file>
	<pch>precomp.h</pch>
</module>
