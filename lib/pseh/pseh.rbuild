<module name="pseh" type="staticlibrary">
	<define name="__USE_W32API" />
	<if property="ARCH" value="i386">
		<directory name="i386">
			<file>framebased.asm</file>
		</directory>
	</if>
	<file>framebased.c</file>
</module>
