<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
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
</rbuild>
