<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdblr" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdblr.dll" allowwarnings="true">
	<importlibrary definition="kbdblr.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdblr.c</file>
	<file>kbdblr.rc</file>
</module>
