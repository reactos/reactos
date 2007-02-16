<module name="chew" type="staticlibrary">
	<define name="__USE_W32API" />
	<define name="_NTOSKRNL_" />
	<include base="chew">include</include>
	<include base="ReactOS">include/ddk</include>
	<file>workqueue.c</file>
</module>