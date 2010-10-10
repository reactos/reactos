<?xml version="1.0" ?>
<!DOCTYPE project SYSTEM "../../project.dtd">
<project name="Project" makefile="Makefile">
	<directory name="tools">
		<directory name="rbuild">
			<directory name="tests">
				<directory name="data">
					<module name="module1" type="buildtool">
						<include base="module1">.</include>
						<file>sourcefile1.c</file>
					</module>
				</directory>
			</directory>
		</directory>
	</directory>
</project>
