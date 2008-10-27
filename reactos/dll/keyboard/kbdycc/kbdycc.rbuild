<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdycc" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdycc.dll" allowwarnings="true">
	<importlibrary definition="kbdycc.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdycc.c</file>
	<file>kbdycc.rc</file>
</module>
