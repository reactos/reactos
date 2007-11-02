<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group>
	<bootstrapfile>services</bootstrapfile>
	<installfile installbase="system32/drivers/etc">services</installfile>
	<if property="KDBG" value="1">
		<bootstrapfile>KDBinit</bootstrapfile>
		<installfile installbase="system32/drivers/etc">KDBinit</installfile>
	</if>
</group>
