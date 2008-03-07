<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../tools/rbuild/project.dtd">
<module name="winebuild" type="buildtool">
	<include base="ReactOS">include/reactos/wine</include>
	<include base="ReactOS">include/reactos</include>
	<include base="ReactOS">include</include>
	<include base="ReactOS" root="intermediate">include</include>
	<file>import.c</file>
	<file>main.c</file>
	<file>parser.c</file>
	<file>res16.c</file>
	<file>res32.c</file>
	<file>spec32.c</file>
	<file>utils.c</file>
	<file>mkstemps.c</file>
</module>
