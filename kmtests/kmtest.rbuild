<module name="kmtest" type="win32cui" installbase="bin" installname="kmtest.exe">
	<include base="kmtest">include</include>
	<library>advapi32</library>
	<define name="KMT_USER_MODE" />
	<directory name="kmtest">
		<file>kmtest.c</file>
		<file>service.c</file>
		<file>support.c</file>
		<file>testlist.c</file>
	</directory>
	<directory name="example">
		<file>Example_user.c</file>
	</directory>
</module>
