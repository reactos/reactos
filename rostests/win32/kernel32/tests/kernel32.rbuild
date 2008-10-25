<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="kernel32_test" type="test">
	<include base="rtshared">.</include>
	<include base="kernel32">.</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_SEH_NO_NATIVE_NLG" />
	<define name="__USE_W32API" />
	<define name="__NO_CTYPE_INLINES" />
	<library>rtshared</library>
	<library>regtests</library>
	<library>kernel32_base</library>
	<library>pseh</library>
	<library>libcntpr</library>
	<library>msvcrt</library>
	<linkerflag>-lgcc</linkerflag>
	<linkerflag>-nostartfiles</linkerflag>
	<linkerflag>-nostdlib</linkerflag>
	<file>setup.c</file>
	<file>CreateFile.c</file>
	<xi:include href="stubs.rbuild" />
</module>
</group>
