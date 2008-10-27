<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdit" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdit.dll" allowwarnings="true">
	<importlibrary definition="kbdit.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdit.c</file>
	<file>kbdit.rc</file>
</module>
