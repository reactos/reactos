#!/usr/bin/perl -wT
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
# The Original Code is the Bugzilla Bug Tracking System.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): Dave Miller <justdave@bugzilla.org>
#                 Myk Melez <myk@mozilla.org>

use strict;

use lib qw(.);

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::BugMail;

my $dbh = Bugzilla->dbh;

my $list = $dbh->selectcol_arrayref(
        'SELECT bug_id FROM bugs 
          WHERE lastdiffed IS NULL
             OR lastdiffed < delta_ts 
            AND delta_ts < NOW() - ' . $dbh->sql_interval(30, 'MINUTE') .
     ' ORDER BY bug_id');

if (scalar(@$list) > 0) {
    print "OK, now attempting to send unsent mail\n";
    print scalar(@$list) . " bugs found with possibly unsent mail.\n\n";
    foreach my $bugid (@$list) {
        my $start_time = time;
        print "Sending mail for bug $bugid...\n";
        my $outputref = Bugzilla::BugMail::Send($bugid);
        if ($ARGV[0] && $ARGV[0] eq "--report") {
          print "Mail sent to:\n";
          foreach (sort @{$outputref->{sent}}) {
              print $_ . "\n";
          }
          
          print "Excluded:\n";
          foreach (sort @{$outputref->{excluded}}) {
              print $_ . "\n";
          }
        }
        else {
            my ($sent, $excluded) = (scalar(@{$outputref->{sent}}),scalar(@{$outputref->{excluded}}));
            print "$sent mails sent, $excluded people excluded.\n";
            print "Took " . (time - $start_time) . " seconds.\n\n";
        }    
    }
    print "Unsent mail has been sent.\n";
}
