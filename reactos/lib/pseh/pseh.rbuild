<module name="pseh" type="staticlibrary">
	<define name="__USE_W32API" />
	<if property='ARCH' value='i386'>
		<directory name="i386">
			<file>framebased.asm</file>
			<file>setjmp.asm</file>
		</directory>
	</if>
	<if property='ARCH' value='powerpc'>
		<directory name="powerpc">
			<file>framebased.s</file>
			<file>setjmp.s</file>
		</directory>
	</if>
	<file>framebased.c</file>
</module>
