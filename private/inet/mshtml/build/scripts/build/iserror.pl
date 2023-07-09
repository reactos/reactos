#! perl
# ISERROR.PL
# scan BUILD.LOG (or whatever) for errors which are put in BUILD.ERR
#
# Command-line arguments:
# PERL ISERROR.PL <1> <2>
# <1> Source file (default is BUILD.LOG)
# <2> Destination file (default is BUILD.ERR)

if ($ARGV[0]) { $filename = $ARGV[0];}
else {          $filename = "BUILD.LOG";}

if ($ARGV[1]) { $outfile = $ARGV[1];}
else {          $outfile = "ERROR.LOG";}

unlink($outfile);
$fErrOpen = 0;

open(LOG, $filename) || die "ISERROR.PL -- Can't open $filename: $!\n";
while ($line = <LOG>)   {

#	if ($line =~ /.*Build complete.*/) {$complete=-1;}

	if ($line =~ /.*[error|warning|fatal error] [A-Z]+[0-9][0-9][0-9][0-9] ?:.*/) {
	unless (($line =~ /.*warning M0002:.*IDispatch::Invoke/) ||
		($line =~ /.*warning RC4509 : Resource types '10' and '240' were both mapped to 'HEXA'/) ||
		($line =~ /.*warning LNK4099: PDB \"ole2auto.pdb\" was not found with/) ||
		($line =~ /.*More than 64 delegated methods not supported/) ||
		($line =~ /.*LINK : warning LNK4001: no object files specified; libraries used/)) {
		if ($fErrOpen == 0) {
			$fErrOpen = -1;
			open(OUT, ">>$outfile") ||
				die "ISERROR.PL -- Can't create $outfile: $!\n";
			$oldhandle=select(OUT);
			print "The build generated the following messages:\n";
			print "----------------------------------------------------\n";
		}
		print $line;
	}
	}
}
# If we didn't find "Build complete" or any specific errors
#if ($complete == 0 && $fErrOpen == 0) {
#	$fErrOpen = -1;
#	open(OUT, ">>$outfile") || die "ISERROR.PL -- Can't create $outfile: $!\n";
#	$oldhandle=select(OUT);
#	print "Build failed for unknown reasons.\n";
#	print "---------------------------------------------\n";
#	}
close(LOG);
if ($fErrOpen == -1) {
	close(OUT);
    exit(1);
}
exit(0);