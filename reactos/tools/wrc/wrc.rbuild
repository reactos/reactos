<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../tools/rbuild/project.dtd">
<module name="wrc" type="buildtool" allowwarnings="true">
	<library>unicode</library>
	<library>wpp</library>
	<file>dumpres.c</file>
	<file>genres.c</file>
	<file>newstruc.c</file>
	<file>po.c</file>
	<file>readres.c</file>
	<file>translation.c</file>
	<file>utils.c</file>
	<file>wrc.c</file>
	<file>writeres.c</file>
	<file>parser.tab.c</file>
	<file>lex.yy.c</file>
	<directory name="port">
		<file>mkstemps.c</file>
	</directory>
</module>
