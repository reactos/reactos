<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<module name="ntoskrnl" type="kernel" installbase="system32" installname="ntoskrnl.exe">
		<xi:include href="ntoskrnl-generic.rbuild" />
	</module>
	<module name="ntdllsys" type="staticlibrary">
		<include base="ntoskrnl">include</include>
		<file>ntdll.S</file>
	</module>
</group>
