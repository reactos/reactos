<module name="apitest" type="staticlibrary">
	<include base="apitest">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x0501</define>
	<file>apitest.c</file>
</module>

<directory name="gdi32api">
	<xi:include href="gdi32api/gdi32api.rbuild" />
</directory>

<directory name="user32api">
	<xi:include href="user32api/user32api.rbuild" />
</directory>

<directory name="w32kdll">
	<xi:include href="w32kdll/directory.rbuild" />
</directory>

<directory name="w32knapi">
	<xi:include href="w32knapi/w32knapi.rbuild" />
</directory>
