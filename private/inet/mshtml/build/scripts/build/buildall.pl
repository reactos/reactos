# Perl script: BUILDALL.PL
#
# Usage:
#
#     BUILDALL <maximum concurrent>
#
# Must be called from the root of the forms3 project

# Get maximum concurrency parameter
$max = $ARGV[0]+0;
if ($max == 0) { $max=1;}

# Leave windows open only if _TEST=1
$stay = " /C ";
if ($ENV{"_TEST"}==1) {$stay = " /K ";}
if ($ENV{"_test"}==1) {$stay = " /K ";}

# Remove any residue:
unlink(<log\\builddon.*>);

# What are all the possibilities:
$param{"WD6P"}="win\\debug x86 debug";
$param{"WS6P"}="win\\ship x86 ship";
$param{"WP6P"}="win\\profile x86 profile";

# Get priority from environment variables
foreach $i (keys %param) {$pri{$i} = $ENV{"$i"}+0;}

while (&highest() > 0) {
	$h = &highest();
	foreach $i (keys %pri) {
		if ($pri{$i} eq $h && numlogs() < $max) {
			print "\nSTARTING $param{$i} (priority $pri{$i})\n";
			system("echo. >log\\builddon.$i");
			system("start cmd.exe $stay %!F3BUILDDIR%\\bldcore.bat $param{$i} $i");
			$pri{$i} = 0;
			sleep(10);
		}
	}
	print "z";
	sleep(5);
}
while (&numlogs() > 0) {
	print "z";
	sleep(5);
}

#Find the highest priority remaining
sub highest {
	@t=reverse(sort(values(%pri)));
	$t[0];
}

#Count the number of BUILDDON.* files left in the log directory
sub numlogs {
    opendir(LOG, "log") || die "buildall.pl: Forms3 Log path not found.\n";
    $ctr=0;
    while($name=readdir(LOG)) {
	$name =~ tr/a-z/A-Z/;
	if ($name =~ /BUILDDON/) {
		$ctr++;
	}
    }
    closedir(LOG);
    $ctr;
}