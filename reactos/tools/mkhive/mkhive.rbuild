<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../tools/rbuild/project.dtd">
<module name="mkhive" type="buildtool">
	<define name="WINE_UNICODE_API">" "</define>
	<include base="unicode" />
	<include base="newinflibhost">.</include>
	<include base="cmlibhost">.</include>
	<include base="zlibhost">.</include>
	<include base="rtl">.</include>
	<define name="MKHIVE_HOST" />
	<compilerflag compilerset="gcc">-fshort-wchar</compilerflag>
	<library>newinflibhost</library>
	<library>cmlibhost</library>
	<library>unicode</library>
	<file>binhive.c</file>
	<file>cmi.c</file>
	<file>mkhive.c</file>
	<file>reginf.c</file>
	<file>registry.c</file>
	<file>rtl.c</file>
</module>
