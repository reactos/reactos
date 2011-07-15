<?xml version="1.0" ?>
<!DOCTYPE project SYSTEM "../../project.dtd">
<project name="Project" makefile="Makefile">
	<directory name="tools">
		<directory name="rbuild">
			<directory name="tests">
				<directory name="data">
					<module name="module1" type="buildtool">
						<include base="module1">.</include>
						<include base="module1">sourcefile1</include>
						<file>sourcefile_include.c</file>
					</module>
				</directory>
			</directory>
		</directory>
	</directory>
</project>
