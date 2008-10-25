<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdru" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdru.dll" allowwarnings="true">
	<importlibrary definition="kbdru.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdru.c</file>
	<file>kbdru.rc</file>
</module>
