<module name="intrlck" type="staticlibrary">
	<define name="__USE_W32API" />

	<if property="ARCH" value="i386">
		<directory name="i386">
			<file>compareexchange.c</file>
			<file>decrement.c</file>
			<file>exchange.c</file>
			<file>exchangeadd.c</file>
			<file>increment.c</file>
		</directory>
	</if>
	<if property="ARCH" value="ppc">
		<directory name="ppc">
			<file>compareexchange.c</file>
		</directory>
		<file>decrement.c</file>
		<file>exchange.c</file>
		<file>exchangeadd.c</file>
		<file>increment.c</file>
	</if>
</module>
