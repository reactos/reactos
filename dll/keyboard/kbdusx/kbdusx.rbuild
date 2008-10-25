<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdusx" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdusx.dll" allowwarnings="true">
	<importlibrary definition="kbdusx.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdusx.c</file>
	<file>kbdusx.rc</file>
</module>
