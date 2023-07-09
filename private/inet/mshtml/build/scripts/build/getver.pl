#! perl
# GETVER.PL
# Sets the _DROPNAME variable, based on version.h
#
# Command-line arguments:
# PERL GETVER.PL
#
# Assumes that it's being run in a directory containing a
# valid version.h file.
#

# Open the SLM-maintained version.h file, assuming it's in
# the current directory.
open(VERSIONH, "version.h") || die "SETVER.PL -- Can't open version.h\n";

# Scan version.h for the #define rup value
while($line = <VERSIONH>)
    {
    chop($line);
	if ($line =~ /#define[ |\t]+rmj\b/)
		{
		# Strip out everything but the numeric value
		$rmj = $line;
		$rmj =~ s/#define rmj[ |\t]+([0-9]*)/\1/;
		}
	elsif ($line =~ /#define[ |\t]+rmm\b/)
		{
		# Strip out everything but the numeric value
		$rmm = $line;
		$rmm =~ s/#define rmm[ |\t]+([0-9]*)/\1/;
		}
	elsif ($line =~ /#define[ |\t]+rup\b/)
		{
		# Strip out everything but the numeric value
		$rup = $line;
		$rup =~ s/#define rup[ |\t]+([0-9]*)/\1/;
		}
    }

close(VERSIONH);

if ($rmm < 1000)
	{
	$rmm = "0".$rmm;
	}

if ($rup eq 0)
	{
	$letter="";
	}
else
	{
	$letter=".$rup";
	}

print "SET _DROPNAME=Bldrmm$letter\n";
open(VBAT, ">~V.BAT") || die "SETVER.PL -- Can't open ~V.BAT\n";
$oldhandle=select(VBAT);
print "SET _DROPNAME=Bld$rmm$letter\n";
close(VBAT);

if ($err>0) {
	print "UNABLE TO UPDATE VERSION\n";
}
exit $err;