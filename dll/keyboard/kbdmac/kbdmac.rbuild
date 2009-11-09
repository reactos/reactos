<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdmac" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdmac.dll">
	<importlibrary definition="kbdmac.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbdmac.c</file>
	<file>kbdmac.rc</file>
</module>
