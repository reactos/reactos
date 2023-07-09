ECHO sending mail and backing up database to \\F3QA
SLEEP 5

c:
cd \build
if not exist report.htm goto END
copy \\forms3\content\trident\performance\report.htm \\forms3\content\trident\performance\OldReport.htm
copy report.htm \\forms3\content\trident\performance
start /wait mail sramani, garybu, rodc, valh
rename report.htm report.old
if not exist Err.log goto BACKUP1
echo /T sramani /s MAIL PROBLEMS >>Err.log
move Err.log c:\mailout\pending

:BACKUP1
if not exist Errdb.log goto BACKUP2
echo /T sramani /s PERFBUILD PROBLEMS >>Errdb.log
move Errdb.log c:\mailout\pending

:BACKUP2
copy \\f3qa\autoperform\perfmon.mdb \\f3qa\autoperform\perfmon.BU
copy perfmon.mdb \\f3qa\autoperform

:END
sleep 5