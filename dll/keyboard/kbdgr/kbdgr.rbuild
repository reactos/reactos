<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdgr" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdgr.dll" allowwarnings="true">
	<importlibrary definition="kbdgr.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdgr.c</file>
	<file>kbdgr.rc</file>
</module>
