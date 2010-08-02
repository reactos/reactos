<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdusr" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdusr.dll">
	<importlibrary definition="kbdusr.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbdusr.c</file>
	<file>kbdusr.rc</file>
</module>
