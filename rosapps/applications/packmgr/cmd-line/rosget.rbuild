<module name="rosget" type="win32cui" installbase="system32" installname="rosget.exe">
	<include base="package">.</include>
	<define name="UNICODE" />

	<library>kernel32</library>
	<library>package</library>
	<file>main.c</file>
	<file>ros-get.rc</file>
</module>
