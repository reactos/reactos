<module name="acpi" type="kernelmodedriver" installbase="system32/drivers" installname="acpi.sys" allowwarnings="true">
	<include base="acpi">include</include>
	<include base="acpi">ospm/include</include>
	<define name="__USE_W32API" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<directory name="dispatcher">
		<file>dsfield.c</file>
		<file>dsmethod.c</file>
		<file>dsmthdat.c</file>
		<file>dsobject.c</file>
		<file>dsopcode.c</file>
		<file>dsutils.c</file>
		<file>dswexec.c</file>
		<file>dswload.c</file>
		<file>dswscope.c</file>
		<file>dswstate.c</file>
	</directory>
	<directory name="events">
		<file>evevent.c</file>
		<file>evmisc.c</file>
		<file>evregion.c</file>
		<file>evrgnini.c</file>
		<file>evsci.c</file>
		<file>evxface.c</file>
		<file>evxfevnt.c</file>
		<file>evxfregn.c</file>
	</directory>
	<directory name="executer">
		<file>amconfig.c</file>
		<file>amconvrt.c</file>
		<file>amcreate.c</file>
		<file>amdump.c</file>
		<file>amdyadic.c</file>
		<file>amfield.c</file>
		<file>amfldio.c</file>
		<file>ammisc.c</file>
		<file>ammonad.c</file>
		<file>ammutex.c</file>
		<file>amnames.c</file>
		<file>amprep.c</file>
		<file>amregion.c</file>
		<file>amresnte.c</file>
		<file>amresolv.c</file>
		<file>amresop.c</file>
		<file>amstore.c</file>
		<file>amstoren.c</file>
		<file>amstorob.c</file>
		<file>amsystem.c</file>
		<file>amutils.c</file>
		<file>amxface.c</file>
	</directory>
	<directory name="hardware">
		<file>hwacpi.c</file>
		<file>hwgpe.c</file>
		<file>hwregs.c</file>
		<file>hwsleep.c</file>
		<file>hwtimer.c</file>
	</directory>
	<directory name="namespace">
		<file>nsaccess.c</file>
		<file>nsalloc.c</file>
		<file>nseval.c</file>
		<file>nsinit.c</file>
		<file>nsload.c</file>
		<file>nsnames.c</file>
		<file>nsobject.c</file>
		<file>nssearch.c</file>
		<file>nsutils.c</file>
		<file>nswalk.c</file>
		<file>nsxfname.c</file>
		<file>nsxfobj.c</file>
	</directory>
	<directory name="ospm">
		<directory name="busmgr">
			<file>bm.c</file>
			<file>bmnotify.c</file>
			<file>bmpm.c</file>
			<file>bmpower.c</file>
			<file>bmrequest.c</file>
			<file>bmsearch.c</file>
			<file>bmutils.c</file>
			<file>bmxface.c</file>
		</directory>
		<file>acpienum.c</file>
		<file>acpisys.c</file>
		<file>bn.c</file>
		<file>fdo.c</file>
		<file>osl.c</file>
		<file>pdo.c</file>
	</directory>
	<directory name="parser">
		<file>psargs.c</file>
		<file>psopcode.c</file>
		<file>psparse.c</file>
		<file>psscope.c</file>
		<file>pstree.c</file>
		<file>psutils.c</file>
		<file>pswalk.c</file>
		<file>psxface.c</file>
	</directory>
	<directory name="resource">
		<file>rsaddr.c</file>
		<file>rscalc.c</file>
		<file>rscreate.c</file>
		<file>rsdump.c</file>
		<file>rsio.c</file>
		<file>rsirq.c</file>
		<file>rslist.c</file>
		<file>rsmemory.c</file>
		<file>rsmisc.c</file>
		<file>rsutils.c</file>
		<file>rsxface.c</file>
	</directory>
	<directory name="tables">
		<file>tbconvrt.c</file>
		<file>tbget.c</file>
		<file>tbinstal.c</file>
		<file>tbutils.c</file>
		<file>tbxface.c</file>
		<file>tbxfroot.c</file>
	</directory>
	<directory name="utils">
		<file>cmalloc.c</file>
		<file>cmclib.c</file>
		<file>cmcopy.c</file>
		<file>cmdebug.c</file>
		<file>cmdelete.c</file>
		<file>cmeval.c</file>
		<file>cmglobal.c</file>
		<file>cminit.c</file>
		<file>cmobject.c</file>
		<file>cmutils.c</file>
		<file>cmxface.c</file>
	</directory>
	<file>acpi.rc</file>
	<pch>include/acpi.h</pch>
</module>
