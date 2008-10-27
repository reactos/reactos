<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdarme" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdarme.dll" allowwarnings="true">
	<importlibrary definition="kbdarme.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdarme.c</file>
	<file>kbdarme.rc</file>
</module>
