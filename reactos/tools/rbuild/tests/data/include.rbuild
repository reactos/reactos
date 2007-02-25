<?xml version="1.0" ?>
<project name="Project" makefile="Makefile">
	<include>include1</include>
	<directory name="dir1">
		<module name="module1" type="buildtool">
			<include>include2</include>
		</module>
	</directory>
	<directory name="dir2">
		<module name="module2" type="buildtool">
			<include base="module1">include3</include>
		</module>
	</directory>
</project>
