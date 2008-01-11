<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../tools/rbuild/project.dtd">
<module name="inflib" type="staticlibrary">
	<include base="inflib">.</include>
	<define name="__NO_CTYPE_INLINES" />
	<pch>inflib.h</pch>
	<file>infcore.c</file>
	<file>infget.c</file>
	<file>infput.c</file>
	<file>infrosgen.c</file>
	<file>infrosget.c</file>
	<file>infrosput.c</file>
</module>
