<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdgrist" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdgrist.dll" allowwarnings="true">
	<importlibrary definition="kbdgrist.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdgrist.c</file>
	<file>kbdgrist.rc</file>
</module>
