<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group>
	<cdfile installbase="$(CDOUTPUT)">services</cdfile>
	<installfile installbase="system32/drivers/etc">services</installfile>
	<if property="KDBG" value="1">
		<cdfile installbase="$(CDOUTPUT)">KDBinit</cdfile>
		<installfile installbase="system32/drivers/etc">KDBinit</installfile>
	</if>
</group>
