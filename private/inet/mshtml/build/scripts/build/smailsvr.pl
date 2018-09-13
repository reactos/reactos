#perl script - smailsvr.pl
# Scans given share for files of any name, and attempts to feed them
# to sndmail.exe

system("TITLE SndMail Server");
system("SETMAIL -x -b -m $ARGV[0]\\frm3bld.mmf");

$pending = $ARGV[0]."\\"."pending";
$sent = $ARGV[0]."\\"."sent";
$dead = $ARGV[0]."\\"."dead";

while(1==1) {
    opendir(MAIL, $pending) || die "Pending: path not found";
    while($name=readdir(MAIL)) {
	unless (length($name)<3) {
#           print "Sending from command file $name\n";
	    system("now");
	    print " sndmail /Z ".$pending."\\".$name."\n";
	    system("sleep 1");
	    $err=(system("%_F3QADIR%\\TOOLS\\X86\\sndmail /Z ".$pending."\\".$name))/256;

	    if ($err>0) {
		print "\nERROR CODE RETURNED: $err\n"; }
	    if ($err==0) {    #Sent
		unlink($sent."\\".$name);
		rename($pending."\\".$name,$sent."\\".$name);
	    }
	    if ($err>5) {    #No need to try again-dead letter office
		unlink($dead."\\".$name);
		rename($pending."\\".$name,$dead."\\".$name);
	    }
	    # Errors 1-4 may be a fluke, keep trying...
	}
    }
    system("sleep 120"); #Wait 2 minutes before re-reading directory
    closedir(MAIL);
}