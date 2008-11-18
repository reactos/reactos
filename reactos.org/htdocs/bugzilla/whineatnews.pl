#!/usr/bin/perl -w
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
# Contributor(s): Terry Weissman <terry@mozilla.org>
#                 Joseph Heenan <joseph@heenan.me.uk>
#                 Frédéric Buclin <LpSolit@gmail.com>


# This is a script suitable for running once a day from a cron job.  It 
# looks at all the bugs, and sends whiny mail to anyone who has a bug 
# assigned to them that has status NEW or REOPENED that has not been 
# touched for more than the number of days specified in the whinedays param.

use strict;
use lib '.';

use Bugzilla;
use Bugzilla::Mailer;
use Bugzilla::Util;
use Bugzilla::User;

# Whining is disabled if whinedays is zero
exit unless Bugzilla->params->{'whinedays'} >= 1;

my $dbh = Bugzilla->dbh;
my $query = q{SELECT bug_id, short_desc, login_name
                FROM bugs
          INNER JOIN profiles
                  ON userid = assigned_to
               WHERE (bug_status = ? OR bug_status = ?)
                 AND disable_mail = 0
                 AND } . $dbh->sql_to_days('NOW()') . " - " .
                       $dbh->sql_to_days('delta_ts') . " > " .
                       Bugzilla->params->{'whinedays'} .
          " ORDER BY bug_id";

my %bugs;
my %desc;

my $slt_bugs = $dbh->selectall_arrayref($query, undef, 'NEW', 'REOPENED');

foreach my $bug (@$slt_bugs) {
    my ($id, $desc, $email) = @$bug;
    if (!defined $bugs{$email}) {
        $bugs{$email} = [];
    }
    if (!defined $desc{$email}) {
        $desc{$email} = [];
    }
    push @{$bugs{$email}}, $id;
    push @{$desc{$email}}, $desc;
}


foreach my $email (sort (keys %bugs)) {
    my $user = new Bugzilla::User({name => $email});
    next if $user->email_disabled;

    my $vars = {'email' => $email};

    my @bugs = ();
    foreach my $i (@{$bugs{$email}}) {
        my $bug = {};
        $bug->{'summary'} = shift(@{$desc{$email}});
        $bug->{'id'} = $i;
        push @bugs, $bug;
    }
    $vars->{'bugs'} = \@bugs;

    my $msg;
    my $template = Bugzilla->template_inner($user->settings->{'lang'}->{'value'});
    $template->process("email/whine.txt.tmpl", $vars, \$msg)
      or die($template->error());

    Bugzilla->template_inner("");
    MessageToMTA($msg);

    print "$email      " . join(" ", @{$bugs{$email}}) . "\n";
}
