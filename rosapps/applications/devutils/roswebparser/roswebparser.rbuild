<module name="roswebparser" type="win32cui" installbase="system32" installname="roswebparser.exe" allowwarnings ="true">
	<include base="zoomin">.</include>
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<define name="__USE_W32API" />
      <library>kernel32</library>
	<file>roswebparser.c</file>
      <file>utf8.c</file>

</module>
