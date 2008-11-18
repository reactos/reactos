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
# Contributor(s): Owen Taylor <otaylor@redhat.com>
#                 Gervase Markham <gerv@gerv.net>
#                 David Fallon <davef@tetsubo.com>
#                 Tobias Burnus <burnus@net-b.de>

use strict;

use lib qw(.);

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::User;

my $user = Bugzilla->login(LOGIN_REQUIRED);

my $cgi = Bugzilla->cgi;
my $dbh = Bugzilla->dbh;
my $template = Bugzilla->template;
my $vars = {};

my $action = $cgi->param('action') || "";

if ($action eq "show") {
    # Read in the entire quip list
    my $quipsref = $dbh->selectall_arrayref(
                       "SELECT quipid, userid, quip, approved FROM quips");

    my $quips;
    my @quipids;
    foreach my $quipref (@$quipsref) {
        my ($quipid, $userid, $quip, $approved) = @$quipref;
        $quips->{$quipid} = {'userid' => $userid, 'quip' => $quip, 
                             'approved' => $approved};
        push(@quipids, $quipid);
    }

    my $users;
    my $sth = $dbh->prepare("SELECT login_name FROM profiles WHERE userid = ?");
    foreach my $quipid (@quipids) {
        my $userid = $quips->{$quipid}{'userid'};
        if ($userid && not defined $users->{$userid}) {
            ($users->{$userid}) = $dbh->selectrow_array($sth, undef, $userid);
        }
    }
    $vars->{'quipids'} = \@quipids;
    $vars->{'quips'} = $quips;
    $vars->{'users'} = $users;
    $vars->{'show_quips'} = 1;
}

if ($action eq "add") {
    (Bugzilla->params->{'quip_list_entry_control'} eq "closed") &&
      ThrowUserError("no_new_quips");

    # Add the quip 
    my $approved = (Bugzilla->params->{'quip_list_entry_control'} eq "open")
                   || Bugzilla->user->in_group('admin') || 0;
    my $comment = $cgi->param("quip");
    $comment || ThrowUserError("need_quip");
    trick_taint($comment); # Used in a placeholder below

    $dbh->do("INSERT INTO quips (userid, quip, approved) VALUES (?, ?, ?)",
             undef, ($user->id, $comment, $approved));

    $vars->{'added_quip'} = $comment;
}

if ($action eq 'approve') {
    # Read in the entire quip list
    my $quipsref = $dbh->selectall_arrayref("SELECT quipid, approved FROM quips");
    
    my %quips;
    foreach my $quipref (@$quipsref) {
        my ($quipid, $approved) = @$quipref;
        $quips{$quipid} = $approved;
    }

    my @approved;
    my @unapproved;
    foreach my $quipid (keys %quips) {
       my $form = $cgi->param('quipid_'.$quipid) ? 1 : 0;
       if($quips{$quipid} ne $form) {
           if($form) { push(@approved, $quipid); }
           else { push(@unapproved, $quipid); }
       }
    }
    $dbh->do("UPDATE quips SET approved = 1 WHERE quipid IN (" .
            join(",", @approved) . ")") if($#approved > -1);
    $dbh->do("UPDATE quips SET approved = 0 WHERE quipid IN (" .
            join(",", @unapproved) . ")") if($#unapproved > -1);
    $vars->{ 'approved' }   = \@approved;
    $vars->{ 'unapproved' } = \@unapproved;
}

if ($action eq "delete") {
    Bugzilla->user->in_group("admin")
      || ThrowUserError("auth_failure", {group  => "admin",
                                         action => "delete",
                                         object => "quips"});
    my $quipid = $cgi->param("quipid");
    ThrowCodeError("need_quipid") unless $quipid =~ /(\d+)/; 
    $quipid = $1;

    ($vars->{'deleted_quip'}) = $dbh->selectrow_array(
                                    "SELECT quip FROM quips WHERE quipid = ?",
                                    undef, $quipid);
    $dbh->do("DELETE FROM quips WHERE quipid = ?", undef, $quipid);
}

print $cgi->header();
$template->process("list/quips.html.tmpl", $vars)
  || ThrowTemplateError($template->error());
