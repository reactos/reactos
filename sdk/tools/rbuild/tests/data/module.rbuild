<?xml version="1.0" ?>
<!DOCTYPE project SYSTEM "../../project.dtd">
<project name="Project" makefile="Makefile">
	<directory name="dir1">
		<module name="module1" type="buildtool">
			<file>file1.c</file>
			<file>file2.c</file>
		</module>
	</directory>
	<directory name="dir2">
		<module name="module2" type="kernelmodedll" entrypoint="0" installbase="reactos" installname="module2.ext">
			<dependency>module1</dependency>
			<library>module1</library>
			<file>file3.c</file>
			<file>file4.c</file>
		</module>
	</directory>
</project>
