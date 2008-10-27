<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../tools/rbuild/project.dtd">
<group>
<module name="cmlib" type="staticlibrary">
	<include base="cmlib">.</include>
	<define name="__NO_CTYPE_INLINES" />
	<define name="_NTOSKRNL_" />
	<define name="_NTSYSTEM_" />
	<define name="NASSERT" />
	<file>cminit.c</file>
	<file>hivebin.c</file>
	<file>hivecell.c</file>
	<file>hiveinit.c</file>
	<file>hivesum.c</file>
	<file>hivewrt.c</file>
</module>
<module name="cmlibhost" type="hoststaticlibrary">
	<include base="cmlibhost">.</include>
	<define name="__NO_CTYPE_INLINES" />
	<define name="_NTOSKRNL_" />
	<define name="_NTSYSTEM_" />
	<define name="NASSERT" />
	<compilerflag>-Wwrite-strings</compilerflag>
	<compilerflag>-Wpointer-arith</compilerflag>
	<define name="CMLIB_HOST" />
	<file>cminit.c</file>
	<file>hivebin.c</file>
	<file>hivecell.c</file>
	<file>hiveinit.c</file>
	<file>hivesum.c</file>
	<file>hivewrt.c</file>
</module>
</group>
