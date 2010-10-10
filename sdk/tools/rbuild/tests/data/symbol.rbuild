<?xml version="1.0" ?>
<!DOCTYPE project SYSTEM "../../project.dtd">
<project name="Project" makefile="Makefile">
	<module name="module1" type="test">
		<component name="ntdll.dll">
			<symbol newname="RtlAllocateHeap">HeapAlloc@12</symbol>
			<symbol>LdrAccessResource@16</symbol>
		</component>
	</module>
</project>
