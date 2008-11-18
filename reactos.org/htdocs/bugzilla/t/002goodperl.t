# -*- Mode: perl; indent-tabs-mode: nil -*-
# 
# The contents of this file are subject to the Mozilla Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/MPL/
# 
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
# 
# The Original Code are the Bugzilla Tests.
# 
# The Initial Developer of the Original Code is Zach Lipton
# Portions created by Zach Lipton are 
# Copyright (C) 2001 Zach Lipton.  All
# Rights Reserved.
# 
# Contributor(s): Zach Lipton <zach@zachlipton.com>
#                 Jacob Steenhagen <jake@bugzilla.org>
#                 David D. Kilzer <ddkilzer@theracingworld.com>


#################
#Bugzilla Test 2#
####GoodPerl#####

use strict;

use lib 't';

use Support::Files;

use Test::More tests => (scalar(@Support::Files::testitems) * 3);

my @testitems = @Support::Files::testitems; # get the files to test.

foreach my $file (@testitems) {
    $file =~ s/\s.*$//; # nuke everything after the first space (#comment)
    next if (!$file); # skip null entries
    if (! open (FILE, $file)) {
        ok(0,"could not open $file --WARNING");
    }
    my $file_line1 = <FILE>;
    close (FILE);

    $file =~ m/.*\.(.*)/;
    my $ext = $1;

    if ($file_line1 !~ m/^#\!/) {
        ok(1,"$file does not have a shebang");	
    } else {
        my $flags;
        if (!defined $ext || $ext eq "pl") {
            # standalone programs aren't taint checked yet
            $flags = "w";
        } elsif ($ext eq "pm") {
            ok(0, "$file is a module, but has a shebang");
            next;
        } elsif ($ext eq "cgi") {
            # cgi files must be taint checked
            $flags = "wT";
        } else {
            ok(0, "$file has shebang but unknown extension");
            next;
        }

        if ($file_line1 =~ m#^\#\!/usr/bin/perl\s#) {
            if ($file_line1 =~ m#\s-$flags#) {
                ok(1,"$file uses standard perl location and -$flags");
            } else {
                ok(0,"$file is MISSING -$flags --WARNING");
            }
        } else {
            ok(0,"$file uses non-standard perl location");
        }
    }
}

foreach my $file (@testitems) {
    my $found_use_strict = 0;
    $file =~ s/\s.*$//; # nuke everything after the first space (#comment)
    next if (!$file); # skip null entries
    if (! open (FILE, $file)) {
        ok(0,"could not open $file --WARNING");
        next;
    }
    while (my $file_line = <FILE>) {
        if ($file_line =~ m/^\s*use strict/) {
            $found_use_strict = 1;
            last;
        }
    }
    close (FILE);
    if ($found_use_strict) {
        ok(1,"$file uses strict");
    } else {
        ok(0,"$file DOES NOT use strict --WARNING");
    }
}

# Check to see that all error messages use tags (for l10n reasons.)
foreach my $file (@testitems) {
    $file =~ s/\s.*$//; # nuke everything after the first space (#comment)
    next if (!$file); # skip null entries
    if (! open (FILE, $file)) {
        ok(0,"could not open $file --WARNING");
        next;
    }
    my $lineno = 0;
    my $error = 0;
    
    while (!$error && (my $file_line = <FILE>)) {
        $lineno++;
        if ($file_line =~ /Throw.*Error\("(.*?)"/) {
            if ($1 =~ /\s/) {
                ok(0,"$file has a Throw*Error call on line $lineno 
                      which doesn't use a tag --ERROR");
                $error = 1;       
            }
        }
    }
    
    ok(1,"$file uses Throw*Error calls correctly") if !$error;
    
    close(FILE);
}
exit 0;
