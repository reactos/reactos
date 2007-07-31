<module name="win32ksys" type="staticlibrary">
	<define name="_DISABLE_TIDENTS" />
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />

		<if property="ARCH" value="i386">
			<directory name="i386">
				<file>win32k.S</file>
			</directory>
		</if>
		<if property="ARCH" value="powerpc">
			<directory name="powerpc">
				<file>win32k.S</file>
			</directory>
		</if>
		<if property="ARCH" value="mips">
			<directory name="mips">
				<file>win32k.S</file>
			</directory>
		</if>

</module>
