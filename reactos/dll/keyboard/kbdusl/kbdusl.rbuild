<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdusl" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdusl.dll" allowwarnings="true">
	<importlibrary definition="kbdusl.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdusl.c</file>
	<file>kbdusl.rc</file>
</module>
