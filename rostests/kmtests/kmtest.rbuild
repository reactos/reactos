<module name="kmtest" type="win32cui" installbase="bin" installname="kmtest_.exe">
	<include base="kmtest">include</include>
	<library>advapi32</library>
	<library>ntdll</library>
	<library>pseh</library>
	<define name="KMT_USER_MODE" />
	<directory name="kmtest">
		<file>kmtest.c</file>
		<file>service.c</file>
		<file>support.c</file>
		<file>testlist.c</file>
	</directory>
	<directory name="example">
		<file>Example_user.c</file>
		<file>GuardedMemory.c</file>
	</directory>
	<directory name="ntos_io">
		<file>IoDeviceObject_user.c</file>
	</directory>
	<directory name="rtl">
		<file>RtlAvlTree.c</file>
		<file>RtlMemory.c</file>
		<file>RtlSplayTree.c</file>
		<file>RtlUnicodeString.c</file>
	</directory>
</module>
