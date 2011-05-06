<module name="win32k_test" type="test">
	<include base="rtshared">.</include>
	<include base="win32k">.</include>
	<include base="win32k">include</include>
	<include base="ntoskrnl">include</include>
	<include base="freetype">include</include>
	<define name="_WIN32K_" />
	<library>rtshared</library>
	<library>regtests</library>
	<library>win32k_base</library>
	<library>pseh</library>
	<library>freetype</library>
	<library>ntoskrnl</library>
	<file>setup.c</file>
	<xi:include href="stubs.rbuild" />
</module>
