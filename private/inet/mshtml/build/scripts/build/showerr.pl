#! perl
# SHOWERR.PL
# scan BUILD.LOG (or whatever) for errors which are put in BUILD.ERR
#
# Command-line arguments:
# PERL SHOWERR.PL <1>
# <1> Source file (default is BUILD.LOG)

if ($ARGV[0] =~ /[\-h|\-H|\-?]/) {
	die "\nUsage:\nPERL SHOWERR.PL <Source file>\n\n<Source file> default is BUILD.LOG\n\n"; }

if ($ARGV[0]) {	$filename = $ARGV[0];}
else {		$filename = "BUILD.LOG";}

$first = 0;
$complete = 0;

open(LOG, $filename) || die "SHOWERR.PL -- Can't open $filename: $!\n";
while ($line = <LOG>)	{

	if ($line =~ /.*Build complete.*/) {$complete=-1;}

	if ($line =~ /.*[error|warning|fatal error] [A-Z]+[0-9][0-9][0-9][0-9] ?:/) {
	unless ($line =~ /.*warning M0002:.*IDispatch::Invoke/) {
		if ($first == 0) {
			$first = -1;
			print "\nError message(s) were found in $filename:\n";
			print "---------------------------------------------\n";
		}
		print $line;
	}
	}
}
if ($complete == 0) {
	if ($first == 0) {
		print "\nError message(s) were found in $filename:\n";
		print "---------------------------------------------\n"; }
	print "-Build complete- not found in $filename\n";
	print "---------------------------------------------\n";
	}
close(LOG);
