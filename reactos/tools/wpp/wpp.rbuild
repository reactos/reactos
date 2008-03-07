<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../tools/rbuild/project.dtd">
<module name="wpp" type="hoststaticlibrary" allowwarnings="true">
	<include base="ReactOS">include/reactos/wine</include>
	<include base="ReactOS">include/reactos</include>
	<include base="ReactOS">include</include>
	<include base="ReactOS" root="intermediate">include</include>
	<file>lex.yy.c</file>
	<file>preproc.c</file>
	<file>wpp.c</file>
	<file>ppy.tab.c</file>
</module>
