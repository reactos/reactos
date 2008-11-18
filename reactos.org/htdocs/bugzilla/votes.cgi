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
# Contributor(s): Terry Weissman <terry@mozilla.org>
#                 Stephan Niemz  <st.n@gmx.net>
#                 Christopher Aillon <christopher@aillon.com>
#                 Gervase Markham <gerv@gerv.net>
#                 Frédéric Buclin <LpSolit@gmail.com>

use strict;
use lib ".";

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::Bug;
use Bugzilla::User;
use Bugzilla::Product;

use List::Util qw(min);

my $cgi = Bugzilla->cgi;
local our $vars = {};

# If the action is show_bug, you need a bug_id.
# If the action is show_user, you can supply a userid to show the votes for
# another user, otherwise you see your own.
# If the action is vote, your votes are set to those encoded in the URL as 
# <bug_id>=<votes>.
#
# If no action is defined, we default to show_bug if a bug_id is given,
# otherwise to show_user.
my $bug_id = $cgi->param('bug_id');
my $action = $cgi->param('action') || ($bug_id ? "show_bug" : "show_user");

if ($action eq "show_bug" ||
    ($action eq "show_user" && defined $cgi->param('user')))
{
    Bugzilla->login();
}
else {
    Bugzilla->login(LOGIN_REQUIRED);
}

################################################################################
# Begin Data/Security Validation
################################################################################

# Make sure the bug ID is a positive integer representing an existing
# bug that the user is authorized to access.

ValidateBugID($bug_id) if defined $bug_id;

################################################################################
# End Data/Security Validation
################################################################################

if ($action eq "show_bug") {
    show_bug($bug_id);
} 
elsif ($action eq "show_user") {
    show_user($bug_id);
}
elsif ($action eq "vote") {
    record_votes() if Bugzilla->params->{'usevotes'};
    show_user($bug_id);
}
else {
    ThrowCodeError("unknown_action", {action => $action});
}

exit;

# Display the names of all the people voting for this one bug.
sub show_bug {
    my ($bug_id) = @_;
    my $cgi = Bugzilla->cgi;
    my $dbh = Bugzilla->dbh;
    my $template = Bugzilla->template;

    ThrowCodeError("missing_bug_id") unless defined $bug_id;

    $vars->{'bug_id'} = $bug_id;
    $vars->{'users'} =
        $dbh->selectall_arrayref('SELECT profiles.login_name, votes.vote_count 
                                    FROM votes
                              INNER JOIN profiles 
                                      ON profiles.userid = votes.who
                                   WHERE votes.bug_id = ?',
                                  {'Slice' => {}}, $bug_id);

    print $cgi->header();
    $template->process("bug/votes/list-for-bug.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
}

# Display all the votes for a particular user. If it's the user
# doing the viewing, give them the option to edit them too.
sub show_user {
    my ($bug_id) = @_;
    my $cgi = Bugzilla->cgi;
    my $dbh = Bugzilla->dbh;
    my $user = Bugzilla->user;
    my $template = Bugzilla->template;

    # If a bug_id is given, and we're editing, we'll add it to the votes list.
    $bug_id ||= "";

    my $name = $cgi->param('user') || $user->login;
    my $who = login_to_id($name, THROW_ERROR);
    my $userid = $user->id;

    my $canedit = (Bugzilla->params->{'usevotes'} && $userid == $who) ? 1 : 0;

    $dbh->bz_lock_tables('bugs READ', 'products READ', 'votes WRITE',
             'cc READ', 'bug_group_map READ', 'user_group_map READ',
             'group_group_map READ', 'groups READ', 'group_control_map READ');

    if ($canedit && $bug_id) {
        # Make sure there is an entry for this bug
        # in the vote table, just so that things display right.
        my $has_votes = $dbh->selectrow_array('SELECT vote_count FROM votes 
                                               WHERE bug_id = ? AND who = ?',
                                               undef, ($bug_id, $who));
        if (!$has_votes) {
            $dbh->do('INSERT INTO votes (who, bug_id, vote_count) 
                      VALUES (?, ?, 0)', undef, ($who, $bug_id));
        }
    }

    my @products;
    my $products = $user->get_selectable_products;
    # Read the votes data for this user for each product.
    foreach my $product (@$products) {
        next unless ($product->votes_per_user > 0);

        my @bugs;
        my $total = 0;
        my $onevoteonly = 0;

        my $vote_list =
            $dbh->selectall_arrayref('SELECT votes.bug_id, votes.vote_count,
                                             bugs.short_desc
                                        FROM votes
                                  INNER JOIN bugs
                                          ON votes.bug_id = bugs.bug_id
                                       WHERE votes.who = ?
                                         AND bugs.product_id = ?
                                    ORDER BY votes.bug_id',
                                      undef, ($who, $product->id));

        foreach (@$vote_list) {
            my ($id, $count, $summary) = @$_;
            $total += $count;

            # Next if user can't see this bug. So, the totals will be correct
            # and they can see there are votes 'missing', but not on what bug
            # they are. This seems a reasonable compromise; the alternative is
            # to lie in the totals.
            next if !$user->can_see_bug($id);

            push (@bugs, { id => $id, 
                           summary => $summary,
                           count => $count });
        }

        $onevoteonly = 1 if (min($product->votes_per_user,
                                 $product->max_votes_per_bug) == 1);

        # Only add the product for display if there are any bugs in it.
        if ($#bugs > -1) {
            push (@products, { name => $product->name,
                               bugs => \@bugs,
                               onevoteonly => $onevoteonly,
                               total => $total,
                               maxvotes => $product->votes_per_user,
                               maxperbug => $product->max_votes_per_bug });
        }
    }

    $dbh->do('DELETE FROM votes WHERE vote_count <= 0');
    $dbh->bz_unlock_tables();

    $vars->{'canedit'} = $canedit;
    $vars->{'voting_user'} = { "login" => $name };
    $vars->{'products'} = \@products;
    $vars->{'bug_id'} = $bug_id;

    print $cgi->header();
    $template->process("bug/votes/list-for-user.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
}

# Update the user's votes in the database.
sub record_votes {
    ############################################################################
    # Begin Data/Security Validation
    ############################################################################

    my $cgi = Bugzilla->cgi;
    my $dbh = Bugzilla->dbh;
    my $template = Bugzilla->template;

    # Build a list of bug IDs for which votes have been submitted.  Votes
    # are submitted in form fields in which the field names are the bug 
    # IDs and the field values are the number of votes.

    my @buglist = grep {/^[1-9][0-9]*$/} $cgi->param();

    # If no bugs are in the buglist, let's make sure the user gets notified
    # that their votes will get nuked if they continue.
    if (scalar(@buglist) == 0) {
        if (!defined $cgi->param('delete_all_votes')) {
            print $cgi->header();
            $template->process("bug/votes/delete-all.html.tmpl", $vars)
              || ThrowTemplateError($template->error());
            exit();
        }
        elsif ($cgi->param('delete_all_votes') == 0) {
            print $cgi->redirect("votes.cgi");
            exit();
        }
    }

    # Call ValidateBugID on each bug ID to make sure it is a positive
    # integer representing an existing bug that the user is authorized 
    # to access, and make sure the number of votes submitted is also
    # a non-negative integer (a series of digits not preceded by a
    # minus sign).
    my %votes;
    foreach my $id (@buglist) {
      ValidateBugID($id);
      $votes{$id} = $cgi->param($id);
      detaint_natural($votes{$id}) 
        || ThrowUserError("votes_must_be_nonnegative");
    }

    ############################################################################
    # End Data/Security Validation
    ############################################################################
    my $who = Bugzilla->user->id;

    # If the user is voting for bugs, make sure they aren't overstuffing
    # the ballot box.
    if (scalar(@buglist)) {
        my %prodcount;
        my %products;
        # XXX - We really need a $bug->product() method.
        foreach my $bug_id (@buglist) {
            my $bug = new Bugzilla::Bug($bug_id);
            my $prod = $bug->product;
            $products{$prod} ||= new Bugzilla::Product({name => $prod});
            $prodcount{$prod} ||= 0;
            $prodcount{$prod} += $votes{$bug_id};

            # Make sure we haven't broken the votes-per-bug limit
            ($votes{$bug_id} <= $products{$prod}->max_votes_per_bug)
              || ThrowUserError("too_many_votes_for_bug",
                                {max => $products{$prod}->max_votes_per_bug,
                                 product => $prod,
                                 votes => $votes{$bug_id}});
        }

        # Make sure we haven't broken the votes-per-product limit
        foreach my $prod (keys(%prodcount)) {
            ($prodcount{$prod} <= $products{$prod}->votes_per_user)
              || ThrowUserError("too_many_votes_for_product",
                                {max => $products{$prod}->votes_per_user,
                                 product => $prod,
                                 votes => $prodcount{$prod}});
        }
    }

    # Update the user's votes in the database.  If the user did not submit 
    # any votes, they may be using a form with checkboxes to remove all their
    # votes (checkboxes are not submitted along with other form data when
    # they are not checked, and Bugzilla uses them to represent single votes
    # for products that only allow one vote per bug).  In that case, we still
    # need to clear the user's votes from the database.
    my %affected;
    $dbh->bz_lock_tables('bugs WRITE', 'bugs_activity WRITE',
                         'votes WRITE', 'longdescs WRITE',
                         'products READ', 'fielddefs READ');
    
    # Take note of, and delete the user's old votes from the database.
    my $bug_list = $dbh->selectcol_arrayref('SELECT bug_id FROM votes
                                             WHERE who = ?', undef, $who);

    foreach my $id (@$bug_list) {
        $affected{$id} = 1;
    }
    $dbh->do('DELETE FROM votes WHERE who = ?', undef, $who);

    my $sth_insertVotes = $dbh->prepare('INSERT INTO votes (who, bug_id, vote_count)
                                         VALUES (?, ?, ?)');
    # Insert the new values in their place
    foreach my $id (@buglist) {
        if ($votes{$id} > 0) {
            $sth_insertVotes->execute($who, $id, $votes{$id});
        }
        $affected{$id} = 1;
    }

    # Update the cached values in the bugs table
    print $cgi->header();
    my @updated_bugs = ();

    my $sth_getVotes = $dbh->prepare("SELECT SUM(vote_count) FROM votes
                                      WHERE bug_id = ?");

    my $sth_updateVotes = $dbh->prepare("UPDATE bugs SET votes = ?
                                         WHERE bug_id = ?");

    foreach my $id (keys %affected) {
        $sth_getVotes->execute($id);
        my $v = $sth_getVotes->fetchrow_array || 0;
        $sth_updateVotes->execute($v, $id);

        my $confirmed = CheckIfVotedConfirmed($id, $who);
        push (@updated_bugs, $id) if $confirmed;
    }
    $dbh->bz_unlock_tables();

    $vars->{'type'} = "votes";
    $vars->{'mailrecipients'} = { 'changer' => Bugzilla->user->login };
    $vars->{'title_tag'} = 'change_votes';

    foreach my $bug_id (@updated_bugs) {
        $vars->{'id'} = $bug_id;
        $template->process("bug/process/results.html.tmpl", $vars)
          || ThrowTemplateError($template->error());
        # Set header_done to 1 only after the first bug.
        $vars->{'header_done'} = 1;
    }
    $vars->{'votes_recorded'} = 1;
}
