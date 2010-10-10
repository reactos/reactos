<?xml version="1.0" ?>
<!DOCTYPE project SYSTEM "../../project.dtd">
<project name="Project" makefile="Makefile">
	<directory name="dir1">
		<module name="module1" type="buildtool">
			<file>file1.c</file>
			<invoke>
				<output>
					<outputfile>file1.c</outputfile>
				</output>
			</invoke>
		</module>
	</directory>
</project>
