<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdindev" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdindev.dll" allowwarnings="true">
	<importlibrary definition="kbdindev.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdindev.c</file>
	<file>kbdindev.rc</file>
</module>
