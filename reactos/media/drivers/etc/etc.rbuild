<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group>
	<cdfile base="$(CDOUTPUT)">services</cdfile>
	<installfile base="system32/drivers/etc">services</installfile>
	<if property="KDBG" value="1">
		<cdfile base="$(CDOUTPUT)">KDBinit</cdfile>
		<installfile base="system32/drivers/etc">KDBinit</installfile>
	</if>
</group>
