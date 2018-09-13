#! perl
# SETVER.PL
# Sets the version, based on the following algorithm:
#
# Command-line arguments:
# PERL SETVER.PL
#
# Assumes that it's being run in a directory containing an
# SLM-maintained version.h file.
#

# Get the current time
($sec, $min, $hour, $mday, $mon, $year, $wday, $yday, $isdst) = localtime(time);

# Dates are based on 9/1/96

$mon += 12*($year - 97) + 4;

# handle single digit days.
if (length($mday) eq 1)
	{
	$mday = "0$mday";
	}

#Create the month/day string in MMDD format
$MMDD = "$mon$mday";

# Open the SLM-mailtained version.h file, assuming it's in
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
print $rmj, " ", $rmm, " ", $rup, "\n";
if ($MMDD eq $rmm)
	{
	print "sadmin setpv ..+1\n";
	$err=system("sadmin setpv -f ..+1")/256;
	}
else
	{
	print "sadmin setpv $rmj.$MMDD.0\n";
	$err=system("sadmin setpv -f $rmj.$MMDD.0")/256;
	}
if ($err>0) {
	print "UNABLE TO UPDATE VERSION\n";
}
exit $err;