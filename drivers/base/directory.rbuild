<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
<directory name="beep">
	<xi:include href="beep/beep.rbuild" />
</directory>
<directory name="bootvid">
	<xi:include href="bootvid/bootvid.rbuild" />
</directory>
<ifnot property="_WINKD_" value="1">
	<directory name="kdcom">
		<xi:include href="kdcom/kdcom.rbuild" />
	</directory>
</ifnot>
<if property="_WINKD_" value="1">
	<directory name="kddll">
		<xi:include href="kddll/kddll.rbuild" />
	</directory>
</if>
<directory name="null">
	<xi:include href="null/null.rbuild" />
</directory>
<directory name="nmidebug">
	<xi:include href="nmidebug/nmidebug.rbuild" />
</directory>
</group>
