<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdusl" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdusl.dll">
	<importlibrary definition="kbdusl.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbdusl.c</file>
	<file>kbdusl.rc</file>
</module>
