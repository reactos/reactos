<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../tools/rbuild/project.dtd">
<module name="cmlib" type="staticlibrary">
	<include base="cmlib">.</include>
	<define name="__NO_CTYPE_INLINES" />
	<define name="_NTOSKRNL_" />
	<define name="_NTSYSTEM_" />
	<define name="NASSERT" />
	<pch>cmlib.h</pch>
	<file>cminit.c</file>
	<file>hivebin.c</file>
	<file>hivecell.c</file>
	<file>hiveinit.c</file>
	<file>hivesum.c</file>
	<file>hivewrt.c</file>
</module>
