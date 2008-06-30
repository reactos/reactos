<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<cdfile>autorun.inf</cdfile>
	<cdfile>icon.ico</cdfile>
	<cdfile>readme.txt</cdfile>

	<if property="ARCH" value="i386">
		<directory name="i386">
			<xi:include href="i386/directory.rbuild" />
		</directory>
	</if>
<!--
	<if property="ARCH" value="powerpc">
		<directory name="ppc">
			<xi:include href="ppc/directory.rbuild" />
		</directory>
	</if>
-->
	<if property="ARCH" value="arm">
		<directory name="arm">
			<xi:include href="arm/directory.rbuild" />
		</directory>
	</if>

	<directory name="bootcd">
		<xi:include href="bootcd/bootcd.rbuild" />
	</directory>
	<directory name="livecd">
		<xi:include href="livecd/livecd.rbuild" />
	</directory>
	<directory name="bootcdregtest">
		<xi:include href="bootcdregtest/bootcdregtest.rbuild" />
	</directory>
	<directory name="livecdregtest">
		<xi:include href="livecdregtest/livecdregtest.rbuild" />
	</directory>
</group>
