# PERL Script - returns day of the week
# Monday is 1
# Sunday is 7
($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);
if ($wday == 0) {$wday=7;}
exit ($wday);
