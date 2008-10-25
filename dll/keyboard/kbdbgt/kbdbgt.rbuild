<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdbgt" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdbgt.dll" allowwarnings="true">
	<importlibrary definition="kbdbgt.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdbgt.c</file>
	<file>kbdbgt.rc</file>
</module>
