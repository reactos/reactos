<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="acpica" type="staticlibrary" allowwarnings="true">
	<define name="ACPI_USE_LOCAL_CACHE"/>
	<include base="acpica">include</include>
	<directory name="dispatcher">
		<file>dsfield.c</file>
		<file>dsinit.c</file>
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
		<file>evgpe.c</file>
		<file>evgpeblk.c</file>
		<file>evmisc.c</file>
		<file>evregion.c</file>
		<file>evrgnini.c</file>
		<file>evsci.c</file>
		<file>evxface.c</file>
		<file>evxfevnt.c</file>
		<file>evxfregn.c</file>
	</directory>
	<directory name="executer">
		<file>exconfig.c</file>
		<file>exconvrt.c</file>
		<file>excreate.c</file>
		<file>exdump.c</file>
		<file>exfield.c</file>
		<file>exfldio.c</file>
		<file>exmisc.c</file>
		<file>exmutex.c</file>
		<file>exoparg1.c</file>
		<file>exoparg2.c</file>
		<file>exoparg3.c</file>
		<file>exoparg6.c</file>
		<file>exnames.c</file>
		<file>exprep.c</file>
		<file>exregion.c</file>
		<file>exresnte.c</file>
		<file>exresolv.c</file>
		<file>exresop.c</file>
		<file>exstore.c</file>
		<file>exstoren.c</file>
		<file>exstorob.c</file>
		<file>exsystem.c</file>
		<file>exutils.c</file>
	</directory>
	<directory name="hardware">
		<file>hwacpi.c</file>
		<file>hwgpe.c</file>
		<file>hwregs.c</file>
		<file>hwsleep.c</file>
		<file>hwtimer.c</file>
		<file>hwvalid.c</file>
		<file>hwxface.c</file>
	</directory>
	<directory name="namespace">
		<file>nsaccess.c</file>
		<file>nsalloc.c</file>
		<file>nsdump.c</file>
		<file>nsdumpdv.c</file>
		<file>nseval.c</file>
		<file>nsinit.c</file>
		<file>nsload.c</file>
		<file>nsnames.c</file>
		<file>nsobject.c</file>
		<file>nsparse.c</file>
		<file>nspredef.c</file>
		<file>nsrepair.c</file>
		<file>nsrepair2.c</file>
		<file>nssearch.c</file>
		<file>nsutils.c</file>
		<file>nswalk.c</file>
		<file>nsxfeval.c</file>
		<file>nsxfname.c</file>
		<file>nsxfobj.c</file>
	</directory>
	<directory name="parser">
		<file>psargs.c</file>
		<file>psloop.c</file>
		<file>psopcode.c</file>
		<file>psparse.c</file>
		<file>psscope.c</file>
		<file>pstree.c</file>
		<file>psutils.c</file>
		<file>pswalk.c</file>
		<file>psxface.c</file>
	</directory>
	<directory name="resources">
		<file>rsaddr.c</file>
		<file>rscalc.c</file>
		<file>rscreate.c</file>
		<file>rsdump.c</file>
		<file>rsinfo.c</file>
		<file>rsio.c</file>
		<file>rsirq.c</file>
		<file>rslist.c</file>
		<file>rsmemory.c</file>
		<file>rsmisc.c</file>
		<file>rsutils.c</file>
		<file>rsxface.c</file>
	</directory>
	<directory name="tables">
		<file>tbfadt.c</file>
		<file>tbfind.c</file>
		<file>tbinstal.c</file>
		<file>tbutils.c</file>
		<file>tbxface.c</file>
		<file>tbxfroot.c</file>
	</directory>
	<directory name="utilities">
		<file>utalloc.c</file>
		<file>utcache.c</file>
		<file>utclib.c</file>
		<file>utcopy.c</file>
		<file>utdebug.c</file>
		<file>utdelete.c</file>
		<file>uteval.c</file>
		<file>utglobal.c</file>
		<file>utids.c</file>
		<file>utinit.c</file>
		<file>utlock.c</file>
		<file>utmath.c</file>
		<file>utmisc.c</file>
		<file>utmutex.c</file>
		<file>utobject.c</file>
		<file>utresrc.c</file>
		<file>utstate.c</file>
		<file>uttrack.c</file>
		<file>utxface.c</file>
	</directory>
</module>
