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
#                 Matthew Tuck <matty@chariot.net.au>
#                 Max Kanat-Alexander <mkanat@bugzilla.org>
#                 Marc Schumann <wurblzap@gmail.com>
#                 Frédéric Buclin <LpSolit@gmail.com>

use strict;

use lib qw(.);

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::User;

###########################################################################
# General subs
###########################################################################

sub Status {
    my ($str) = (@_);
    print "$str <p>\n";
}

sub Alert {
    my ($str) = (@_);
    Status("<font color=\"red\">$str</font>");
}

sub BugLink {
    my ($id) = (@_);
    return "<a href=\"show_bug.cgi?id=$id\">$id</a>";
}

#
# Parameter is a list of bug ids.
#
# Return is a string containing a list of all the bugids, as hrefs,
# followed by a link to them all as a buglist
sub BugListLinks {
    my @bugs = @_;

    # Historically, GetBugLink() wasn't used here. I'm guessing this
    # was because it didn't exist or is too performance heavy, or just
    # plain unnecessary
    my @bug_links = map(BugLink($_), @bugs);

    return join(', ',@bug_links) . " <a href=\"buglist.cgi?bug_id=" .
        join(',',@bugs) . "\">(as buglist)</a>";
}

###########################################################################
# Start
###########################################################################

Bugzilla->login(LOGIN_REQUIRED);

my $cgi = Bugzilla->cgi;
my $dbh = Bugzilla->dbh;
my $template = Bugzilla->template;
my $user = Bugzilla->user;

# Make sure the user is authorized to access sanitycheck.cgi.
# As this script can now alter the group_control_map table, we no longer
# let users with editbugs privs run it anymore.
$user->in_group("editcomponents")
  || ($user->in_group('editkeywords') && defined $cgi->param('rebuildkeywordcache'))
  || ThrowUserError("auth_failure", {group  => "editcomponents",
                                     action => "run",
                                     object => "sanity_check"});

print $cgi->header();

my @row;

$template->put_header("Sanity Check");

###########################################################################
# Users with 'editkeywords' privs only can only check keywords.
###########################################################################
unless ($user->in_group('editcomponents')) {
    check_votes_or_keywords('keywords');
    Status("Sanity check completed.");
    $template->put_footer();
    exit;
}

###########################################################################
# Fix vote cache
###########################################################################

if (defined $cgi->param('rebuildvotecache')) {
    Status("OK, now rebuilding vote cache.");
    $dbh->bz_lock_tables('bugs WRITE', 'votes READ');
    $dbh->do(q{UPDATE bugs SET votes = 0});
    my $sth_update = $dbh->prepare(q{UPDATE bugs 
                                        SET votes = ? 
                                      WHERE bug_id = ?});
    my $sth = $dbh->prepare(q{SELECT bug_id, SUM(vote_count)
                                FROM votes }. $dbh->sql_group_by('bug_id'));
    $sth->execute();
    while (my ($id, $v) = $sth->fetchrow_array) {
        $sth_update->execute($v, $id);
    }
    $dbh->bz_unlock_tables();
    Status("Vote cache has been rebuilt.");
}

###########################################################################
# Create missing group_control_map entries
###########################################################################

if (defined $cgi->param('createmissinggroupcontrolmapentries')) {
    Status(qq{OK, now creating <code>SHOWN</code> member control entries
              for product/group combinations lacking one.});

    my $na    = CONTROLMAPNA;
    my $shown = CONTROLMAPSHOWN;
    my $insertsth = $dbh->prepare(
        qq{INSERT INTO group_control_map (
                       group_id, product_id, entry,
                       membercontrol, othercontrol, canedit
                      )
               VALUES (
                       ?, ?, 0,
                       $shown, $na, 0
                      )});
    my $updatesth = $dbh->prepare(qq{UPDATE group_control_map
                                        SET membercontrol = $shown
                                      WHERE group_id   = ?
                                        AND product_id = ?});
    my $counter = 0;

    # Find all group/product combinations used for bugs but not set up
    # correctly in group_control_map
    my $invalid_combinations = $dbh->selectall_arrayref(
        qq{    SELECT bugs.product_id,
                      bgm.group_id,
                      gcm.membercontrol,
                      groups.name,
                      products.name
                 FROM bugs
           INNER JOIN bug_group_map AS bgm
                   ON bugs.bug_id = bgm.bug_id
           INNER JOIN groups
                   ON bgm.group_id = groups.id
           INNER JOIN products
                   ON bugs.product_id = products.id
            LEFT JOIN group_control_map AS gcm
                   ON bugs.product_id = gcm.product_id
                  AND    bgm.group_id = gcm.group_id
                WHERE COALESCE(gcm.membercontrol, $na) = $na
          } . $dbh->sql_group_by('bugs.product_id, bgm.group_id',
                                 'gcm.membercontrol, groups.name, products.name'));

    foreach (@$invalid_combinations) {
        my ($product_id, $group_id, $currentmembercontrol,
            $group_name, $product_name) = @$_;

        $counter++;
        if (defined($currentmembercontrol)) {
            Status(qq{Updating <code>NA/<em>xxx</em></code> group control
                      setting for group <em>$group_name</em> to
                      <code>SHOWN/<em>xxx</em></code> in product
                      <em>$product_name</em>.});
            $updatesth->execute($group_id, $product_id);
        }
        else {
            Status(qq{Generating <code>SHOWN/NA</code> group control setting
                      for group <em>$group_name</em> in product
                      <em>$product_name</em>.});
            $insertsth->execute($group_id, $product_id);
        }
    }

    Status("Repaired $counter defective group control settings.");
}

###########################################################################
# Fix missing creation date
###########################################################################

if (defined $cgi->param('repair_creation_date')) {
    Status("OK, now fixing missing bug creation dates");

    my $bug_ids = $dbh->selectcol_arrayref('SELECT bug_id FROM bugs
                                            WHERE creation_ts IS NULL');

    my $sth_UpdateDate = $dbh->prepare('UPDATE bugs SET creation_ts = ?
                                        WHERE bug_id = ?');

    # All bugs have an entry in the 'longdescs' table when they are created,
    # even if 'commentoncreate' is turned off.
    my $sth_getDate = $dbh->prepare('SELECT MIN(bug_when) FROM longdescs
                                     WHERE bug_id = ?');

    foreach my $bugid (@$bug_ids) {
        $sth_getDate->execute($bugid);
        my $date = $sth_getDate->fetchrow_array;
        $sth_UpdateDate->execute($date, $bugid);
    }
    Status(scalar(@$bug_ids) . " bugs have been fixed.");
}

###########################################################################
# Send unsent mail
###########################################################################

if (defined $cgi->param('rescanallBugMail')) {
    require Bugzilla::BugMail;

    Status("OK, now attempting to send unsent mail");
    my $time = $dbh->sql_interval(30, 'MINUTE');
    
    my $list = $dbh->selectcol_arrayref(qq{
                                        SELECT bug_id
                                          FROM bugs 
                                         WHERE (lastdiffed IS NULL
                                                OR lastdiffed < delta_ts)
                                           AND delta_ts < now() - $time
                                      ORDER BY bug_id});
         
    Status(scalar(@$list) . ' bugs found with possibly unsent mail.');

    my $vars = {};
    # We cannot simply look at the bugs_activity table to find who did the
    # last change in a given bug, as e.g. adding a comment doesn't add any
    # entry to this table. And some other changes may be private
    # (such as time-related changes or private attachments or comments)
    # and so choosing this user as being the last one having done a change
    # for the bug may be problematic. So the best we can do at this point
    # is to choose the currently logged in user for email notification.
    $vars->{'changer'} = Bugzilla->user->login;

    foreach my $bugid (@$list) {
        Bugzilla::BugMail::Send($bugid, $vars);
    }

    if (scalar(@$list) > 0) {
        Status("Unsent mail has been sent.");
    }

    $template->put_footer();
    exit;
}

###########################################################################
# Remove all references to deleted bugs
###########################################################################

if (defined $cgi->param('remove_invalid_bug_references')) {
    Status("OK, now removing all references to deleted bugs.");

    $dbh->bz_lock_tables('attachments WRITE', 'bug_group_map WRITE',
                         'bugs_activity WRITE', 'cc WRITE',
                         'dependencies WRITE', 'duplicates WRITE',
                         'flags WRITE', 'keywords WRITE',
                         'longdescs WRITE', 'votes WRITE', 'bugs READ');

    foreach my $pair ('attachments/', 'bug_group_map/', 'bugs_activity/', 'cc/',
                      'dependencies/blocked', 'dependencies/dependson',
                      'duplicates/dupe', 'duplicates/dupe_of',
                      'flags/', 'keywords/', 'longdescs/', 'votes/') {

        my ($table, $field) = split('/', $pair);
        $field ||= "bug_id";

        my $bug_ids =
          $dbh->selectcol_arrayref("SELECT $table.$field FROM $table
                                    LEFT JOIN bugs ON $table.$field = bugs.bug_id
                                    WHERE bugs.bug_id IS NULL");

        if (scalar(@$bug_ids)) {
            $dbh->do("DELETE FROM $table WHERE $field IN (" . join(',', @$bug_ids) . ")");
        }
    }

    $dbh->bz_unlock_tables();
    Status("All references to deleted bugs have been removed.");
}

###########################################################################
# Remove all references to deleted attachments
###########################################################################

if (defined $cgi->param('remove_invalid_attach_references')) {
    Status("OK, now removing all references to deleted attachments.");

    $dbh->bz_lock_tables('attachments WRITE', 'attach_data WRITE');

    my $attach_ids =
        $dbh->selectcol_arrayref('SELECT attach_data.id
                                    FROM attach_data
                               LEFT JOIN attachments
                                      ON attachments.attach_id = attach_data.id
                                   WHERE attachments.attach_id IS NULL');

    if (scalar(@$attach_ids)) {
        $dbh->do('DELETE FROM attach_data WHERE id IN (' .
                 join(',', @$attach_ids) . ')');
    }

    $dbh->bz_unlock_tables();
    Status("All references to deleted attachments have been removed.");
}

print "OK, now running sanity checks.<p>\n";

###########################################################################
# Perform referential (cross) checks
###########################################################################

# This checks that a simple foreign key has a valid primary key value.  NULL
# references are acceptable and cause no problem.
#
# The first parameter is the primary key table name.
# The second parameter is the primary key field name.
# Each successive parameter represents a foreign key, it must be a list
# reference, where the list has:
#   the first value is the foreign key table name.
#   the second value is the foreign key field name.
#   the third value is optional and represents a field on the foreign key
#     table to display when the check fails.
#   the fourth value is optional and is a list reference to values that
#     are excluded from checking.
#
# FIXME: The excluded values parameter should go away - the QA contact
#        fields should use NULL instead - see bug #109474.
#        The same goes for series; no bug for that yet.

sub CrossCheck {
    my $table = shift @_;
    my $field = shift @_;
    my $dbh = Bugzilla->dbh;

    Status("Checking references to $table.$field");

    while (@_) {
        my $ref = shift @_;
        my ($refertable, $referfield, $keyname, $exceptions) = @$ref;

        $exceptions ||= [];
        my %exceptions = map { $_ => 1 } @$exceptions;

        Status("... from $refertable.$referfield");
       
        my $query = qq{SELECT DISTINCT $refertable.$referfield} .
            ($keyname ? qq{, $refertable.$keyname } : q{}) .
                     qq{ FROM $refertable
                    LEFT JOIN $table
                           ON $refertable.$referfield = $table.$field
                        WHERE $table.$field IS NULL
                          AND $refertable.$referfield IS NOT NULL};
         
        my $sth = $dbh->prepare($query);
        $sth->execute;

        my $has_bad_references = 0;

        while (my ($value, $key) = $sth->fetchrow_array) {
            next if $exceptions{$value};
            my $alert = "Bad value &quot;$value&quot; found in $refertable.$referfield";
            if ($keyname) {
                if ($keyname eq 'bug_id') {
                    $alert .= ' (bug ' . BugLink($key) . ')';
                } else {
                    $alert .= " ($keyname == '$key')";
                }
            }
            Alert($alert);
            $has_bad_references = 1;
        }
        # References to non existent bugs can be safely removed, bug 288461
        if ($table eq 'bugs' && $has_bad_references) {
            print qq{<a href="sanitycheck.cgi?remove_invalid_bug_references=1">
                     Remove invalid references to non existent bugs.</a><p>\n};
        }
        # References to non existent attachments can be safely removed.
        if ($table eq 'attachments' && $has_bad_references) {
            print qq{<a href="sanitycheck.cgi?remove_invalid_attach_references=1">
                     Remove invalid references to non existent attachments.</a><p>\n};
        }
    }
}

CrossCheck('classifications', 'id',
           ['products', 'classification_id']);

CrossCheck("keyworddefs", "id",
           ["keywords", "keywordid"]);

CrossCheck("fielddefs", "id",
           ["bugs_activity", "fieldid"],
           ['profiles_activity', 'fieldid']);

CrossCheck("flagtypes", "id",
           ["flags", "type_id"]);

CrossCheck("bugs", "bug_id",
           ["bugs_activity", "bug_id"],
           ["bug_group_map", "bug_id"],
           ["attachments", "bug_id"],
           ["cc", "bug_id"],
           ["longdescs", "bug_id"],
           ["dependencies", "blocked"],
           ["dependencies", "dependson"],
           ['flags', 'bug_id'],
           ["votes", "bug_id"],
           ["keywords", "bug_id"],
           ["duplicates", "dupe_of", "dupe"],
           ["duplicates", "dupe", "dupe_of"]);

CrossCheck("groups", "id",
           ["bug_group_map", "group_id"],
           ['category_group_map', 'group_id'],
           ["group_group_map", "grantor_id"],
           ["group_group_map", "member_id"],
           ["group_control_map", "group_id"],
           ["namedquery_group_map", "group_id"],
           ["user_group_map", "group_id"]);

CrossCheck("namedqueries", "id",
           ["namedqueries_link_in_footer", "namedquery_id"],
           ["namedquery_group_map", "namedquery_id"],
          );

CrossCheck("profiles", "userid",
           ['profiles_activity', 'userid'],
           ['profiles_activity', 'who'],
           ['email_setting', 'user_id'],
           ['profile_setting', 'user_id'],
           ["bugs", "reporter", "bug_id"],
           ["bugs", "assigned_to", "bug_id"],
           ["bugs", "qa_contact", "bug_id"],
           ["attachments", "submitter_id", "bug_id"],
           ['flags', 'setter_id', 'bug_id'],
           ['flags', 'requestee_id', 'bug_id'],
           ["bugs_activity", "who", "bug_id"],
           ["cc", "who", "bug_id"],
           ['quips', 'userid'],
           ["votes", "who", "bug_id"],
           ["longdescs", "who", "bug_id"],
           ["logincookies", "userid"],
           ["namedqueries", "userid"],
           ["namedqueries_link_in_footer", "user_id"],
           ['series', 'creator', 'series_id'],
           ["watch", "watcher"],
           ["watch", "watched"],
           ['whine_events', 'owner_userid'],
           ["tokens", "userid"],
           ["user_group_map", "user_id"],
           ["components", "initialowner", "name"],
           ["components", "initialqacontact", "name"],
           ["component_cc", "user_id"]);

CrossCheck("products", "id",
           ["bugs", "product_id", "bug_id"],
           ["components", "product_id", "name"],
           ["milestones", "product_id", "value"],
           ["versions", "product_id", "value"],
           ["group_control_map", "product_id"],
           ["flaginclusions", "product_id", "type_id"],
           ["flagexclusions", "product_id", "type_id"]);

CrossCheck("components", "id",
           ["component_cc", "component_id"]);

# Check the former enum types -mkanat@bugzilla.org
CrossCheck("bug_status", "value",
            ["bugs", "bug_status", "bug_id"]);

CrossCheck("resolution", "value",
            ["bugs", "resolution", "bug_id"]);

CrossCheck("bug_severity", "value",
            ["bugs", "bug_severity", "bug_id"]);

CrossCheck("op_sys", "value",
            ["bugs", "op_sys", "bug_id"]);

CrossCheck("priority", "value",
            ["bugs", "priority", "bug_id"]);

CrossCheck("rep_platform", "value",
            ["bugs", "rep_platform", "bug_id"]);

CrossCheck('series', 'series_id',
           ['series_data', 'series_id']);

CrossCheck('series_categories', 'id',
           ['series', 'category']);

CrossCheck('whine_events', 'id',
           ['whine_queries', 'eventid'],
           ['whine_schedules', 'eventid']);

CrossCheck('attachments', 'attach_id',
           ['attach_data', 'id']);

###########################################################################
# Perform double field referential (cross) checks
###########################################################################
 
# This checks that a compound two-field foreign key has a valid primary key
# value.  NULL references are acceptable and cause no problem.
#
# The first parameter is the primary key table name.
# The second parameter is the primary key first field name.
# The third parameter is the primary key second field name.
# Each successive parameter represents a foreign key, it must be a list
# reference, where the list has:
#   the first value is the foreign key table name
#   the second value is the foreign key first field name.
#   the third value is the foreign key second field name.
#   the fourth value is optional and represents a field on the foreign key
#     table to display when the check fails

sub DoubleCrossCheck {
    my $table = shift @_;
    my $field1 = shift @_;
    my $field2 = shift @_;
    my $dbh = Bugzilla->dbh;
 
    Status("Checking references to $table.$field1 / $table.$field2");
 
    while (@_) {
        my $ref = shift @_;
        my ($refertable, $referfield1, $referfield2, $keyname) = @$ref;
 
        Status("... from $refertable.$referfield1 / $refertable.$referfield2");

        my $d_cross_check = $dbh->selectall_arrayref(qq{
                        SELECT DISTINCT $refertable.$referfield1, 
                                        $refertable.$referfield2 } .
                       ($keyname ? qq{, $refertable.$keyname } : q{}) .
                      qq{ FROM $refertable
                     LEFT JOIN $table
                            ON $refertable.$referfield1 = $table.$field1
                           AND $refertable.$referfield2 = $table.$field2 
                         WHERE $table.$field1 IS NULL 
                           AND $table.$field2 IS NULL 
                           AND $refertable.$referfield1 IS NOT NULL 
                           AND $refertable.$referfield2 IS NOT NULL});

        foreach my $check (@$d_cross_check) {
            my ($value1, $value2, $key) = @$check;
            my $alert = "Bad values &quot;$value1&quot;, &quot;$value2&quot; found in " .
                "$refertable.$referfield1 / $refertable.$referfield2";
            if ($keyname) {
                if ($keyname eq 'bug_id') {
                   $alert .= ' (bug ' . BugLink($key) . ')';
                }
                else {
                    $alert .= " ($keyname == '$key')";
                }
            }
            Alert($alert);
        }
    }
}

DoubleCrossCheck('attachments', 'bug_id', 'attach_id',
                 ['flags', 'bug_id', 'attach_id'],
                 ['bugs_activity', 'bug_id', 'attach_id']);

DoubleCrossCheck("components", "product_id", "id",
                 ["bugs", "product_id", "component_id", "bug_id"],
                 ['flagexclusions', 'product_id', 'component_id'],
                 ['flaginclusions', 'product_id', 'component_id']);

DoubleCrossCheck("versions", "product_id", "value",
                 ["bugs", "product_id", "version", "bug_id"]);
 
DoubleCrossCheck("milestones", "product_id", "value",
                 ["bugs", "product_id", "target_milestone", "bug_id"],
                 ["products", "id", "defaultmilestone", "name"]);

###########################################################################
# Perform login checks
###########################################################################
 
Status("Checking profile logins");

my $sth = $dbh->prepare(q{SELECT userid, login_name FROM profiles});
$sth->execute;

while (my ($id, $email) = $sth->fetchrow_array) {
    validate_email_syntax($email)
      || Alert "Bad profile email address, id=$id,  &lt;$email&gt;.";
}

###########################################################################
# Perform vote/keyword cache checks
###########################################################################

sub AlertBadVoteCache {
    my ($id) = (@_);
    Alert("Bad vote cache for bug " . BugLink($id));
}

check_votes_or_keywords();

sub check_votes_or_keywords {
    my $check = shift || 'all';

    my $dbh = Bugzilla->dbh;
    my $sth = $dbh->prepare(q{SELECT bug_id, votes, keywords
                                FROM bugs
                               WHERE votes != 0 OR keywords != ''});
    $sth->execute;

    my %votes;
    my %keyword;

    while (my ($id, $v, $k) = $sth->fetchrow_array) {
        if ($v != 0) {
            $votes{$id} = $v;
        }
        if ($k) {
            $keyword{$id} = $k;
        }
    }

    # If we only want to check keywords, skip checks about votes.
    _check_votes(\%votes) unless ($check eq 'keywords');
    # If we only want to check votes, skip checks about keywords.
    _check_keywords(\%keyword) unless ($check eq 'votes');
}

sub _check_votes {
    my $votes = shift;

    Status("Checking cached vote counts");
    my $dbh = Bugzilla->dbh;
    my $sth = $dbh->prepare(q{SELECT bug_id, SUM(vote_count)
                                FROM votes }.
                                $dbh->sql_group_by('bug_id'));
    $sth->execute;

    my $offer_votecache_rebuild = 0;

    while (my ($id, $v) = $sth->fetchrow_array) {
        if ($v <= 0) {
            Alert("Bad vote sum for bug $id");
        } else {
            if (!defined $votes->{$id} || $votes->{$id} != $v) {
                AlertBadVoteCache($id);
                $offer_votecache_rebuild = 1;
            }
            delete $votes->{$id};
        }
    }
    foreach my $id (keys %$votes) {
        AlertBadVoteCache($id);
        $offer_votecache_rebuild = 1;
    }

    if ($offer_votecache_rebuild) {
        print qq{<a href="sanitycheck.cgi?rebuildvotecache=1">Click here to rebuild the vote cache</a><p>\n};
    }
}

sub _check_keywords {
    my $keyword = shift;

    Status("Checking keywords table");
    my $dbh = Bugzilla->dbh;
    my $cgi = Bugzilla->cgi;

    my %keywordids;
    my $keywords = $dbh->selectall_arrayref(q{SELECT id, name
                                                FROM keyworddefs});

    foreach (@$keywords) {
        my ($id, $name) = @$_;
        if ($keywordids{$id}) {
            Alert("Duplicate entry in keyworddefs for id $id");
        }
        $keywordids{$id} = 1;
        if ($name =~ /[\s,]/) {
            Alert("Bogus name in keyworddefs for id $id");
        }
    }

    my $sth = $dbh->prepare(q{SELECT bug_id, keywordid
                                FROM keywords
                            ORDER BY bug_id, keywordid});
    $sth->execute;
    my $lastid;
    my $lastk;
    while (my ($id, $k) = $sth->fetchrow_array) {
        if (!$keywordids{$k}) {
            Alert("Bogus keywordids $k found in keywords table");
        }
        if (defined $lastid && $id eq $lastid && $k eq $lastk) {
            Alert("Duplicate keyword ids found in bug " . BugLink($id));
        }
        $lastid = $id;
        $lastk = $k;
    }

    Status("Checking cached keywords");

    if (defined $cgi->param('rebuildkeywordcache')) {
        $dbh->bz_lock_tables('bugs write', 'keywords read', 'keyworddefs read');
    }

    my $query = q{SELECT keywords.bug_id, keyworddefs.name
                    FROM keywords
              INNER JOIN keyworddefs
                      ON keyworddefs.id = keywords.keywordid
              INNER JOIN bugs
                      ON keywords.bug_id = bugs.bug_id
                ORDER BY keywords.bug_id, keyworddefs.name};

    $sth = $dbh->prepare($query);
    $sth->execute;

    my $lastb = 0;
    my @list;
    my %realk;
    while (1) {
        my ($b, $k) = $sth->fetchrow_array;
        if (!defined $b || $b != $lastb) {
            if (@list) {
                $realk{$lastb} = join(', ', @list);
            }
            last unless $b;

            $lastb = $b;
            @list = ();
        }
        push(@list, $k);
    }

    my @badbugs = ();

    foreach my $b (keys(%$keyword)) {
        if (!exists $realk{$b} || $realk{$b} ne $keyword->{$b}) {
            push(@badbugs, $b);
        }
    }
    foreach my $b (keys(%realk)) {
        if (!exists $keyword->{$b}) {
            push(@badbugs, $b);
        }
    }
    if (@badbugs) {
        @badbugs = sort {$a <=> $b} @badbugs;
        Alert(scalar(@badbugs) . " bug(s) found with incorrect keyword cache: " .
              BugListLinks(@badbugs));

        my $sth_update = $dbh->prepare(q{UPDATE bugs
                                            SET keywords = ?
                                          WHERE bug_id = ?});

        if (defined $cgi->param('rebuildkeywordcache')) {
            Status("OK, now fixing keyword cache.");
            foreach my $b (@badbugs) {
                my $k = '';
                if (exists($realk{$b})) {
                    $k = $realk{$b};
                }
                $sth_update->execute($k, $b);
            }
            Status("Keyword cache fixed.");
        } else {
            print qq{<a href="sanitycheck.cgi?rebuildkeywordcache=1">Click here to rebuild the keyword cache</a><p>\n};
        }
    }

    if (defined $cgi->param('rebuildkeywordcache')) {
        $dbh->bz_unlock_tables();
    }
}

###########################################################################
# Check for flags being in incorrect products and components
###########################################################################

Status('Checking for flags being in the wrong product/component');

my $invalid_flags = $dbh->selectall_arrayref(
       'SELECT DISTINCT flags.id, flags.bug_id, flags.attach_id
          FROM flags
    INNER JOIN bugs
            ON flags.bug_id = bugs.bug_id
     LEFT JOIN flaginclusions AS i
            ON flags.type_id = i.type_id
           AND (bugs.product_id = i.product_id OR i.product_id IS NULL)
           AND (bugs.component_id = i.component_id OR i.component_id IS NULL)
         WHERE i.type_id IS NULL');

my @invalid_flags = @$invalid_flags;

$invalid_flags = $dbh->selectall_arrayref(
       'SELECT DISTINCT flags.id, flags.bug_id, flags.attach_id
          FROM flags
    INNER JOIN bugs
            ON flags.bug_id = bugs.bug_id
    INNER JOIN flagexclusions AS e
            ON flags.type_id = e.type_id
         WHERE (bugs.product_id = e.product_id OR e.product_id IS NULL)
           AND (bugs.component_id = e.component_id OR e.component_id IS NULL)');

push(@invalid_flags, @$invalid_flags);

if (scalar(@invalid_flags)) {
    if ($cgi->param('remove_invalid_flags')) {
        Status("OK, now deleting invalid flags.");
        my @flag_ids = map {$_->[0]} @invalid_flags;
        $dbh->bz_lock_tables('flags WRITE');
        # Silently delete these flags, with no notification to requesters/setters.
        $dbh->do('DELETE FROM flags WHERE id IN (' . join(',', @flag_ids) .')');
        $dbh->bz_unlock_tables();
        Status("Invalid flags deleted.");
    }
    else {
        foreach my $flag (@$invalid_flags) {
            my ($flag_id, $bug_id, $attach_id) = @$flag;
            Alert("Invalid flag $flag_id for " .
                  ($attach_id ? "attachment $attach_id in bug " : "bug ") . BugLink($bug_id));
        }
        print qq{<a href="sanitycheck.cgi?remove_invalid_flags=1">Click
                 here to delete invalid flags</a><p>\n};
    }
}

###########################################################################
# General bug checks
###########################################################################

sub BugCheck {
    my ($middlesql, $errortext, $repairparam, $repairtext) = @_;
    my $dbh = Bugzilla->dbh;
 
    my $badbugs = $dbh->selectcol_arrayref(qq{SELECT DISTINCT bugs.bug_id
                                                FROM $middlesql 
                                            ORDER BY bugs.bug_id});

    if (scalar(@$badbugs)) {
        Alert("$errortext: " . BugListLinks(@$badbugs));
        if ($repairparam) {
            $repairtext ||= 'Repair these bugs';
            print qq{<a href="sanitycheck.cgi?$repairparam=1">$repairtext</a>.},
                  '<p>';
        }
    }
}

Status("Checking for bugs with no creation date (which makes them invisible)");

BugCheck("bugs WHERE creation_ts IS NULL", "Bugs with no creation date",
         "repair_creation_date", "Repair missing creation date for these bugs");

Status("Checking resolution/duplicates");

BugCheck("bugs INNER JOIN duplicates ON bugs.bug_id = duplicates.dupe " .
         "WHERE bugs.resolution != 'DUPLICATE'",
         "Bug(s) found on duplicates table that are not marked duplicate");

BugCheck("bugs LEFT JOIN duplicates ON bugs.bug_id = duplicates.dupe WHERE " .
         "bugs.resolution = 'DUPLICATE' AND " .
         "duplicates.dupe IS NULL",
         "Bug(s) found marked resolved duplicate and not on duplicates table");

Status("Checking statuses/resolutions");

my @open_states = map($dbh->quote($_), BUG_STATE_OPEN);
my $open_states = join(', ', @open_states);

BugCheck("bugs WHERE bug_status IN ($open_states) AND resolution != ''",
         "Bugs with open status and a resolution");
BugCheck("bugs WHERE bug_status NOT IN ($open_states) AND resolution = ''",
         "Bugs with non-open status and no resolution");

Status("Checking statuses/everconfirmed");

BugCheck("bugs WHERE bug_status = 'UNCONFIRMED' AND everconfirmed = 1",
         "Bugs that are UNCONFIRMED but have everconfirmed set");
# The below list of resolutions is hard-coded because we don't know if future
# resolutions will be confirmed, unconfirmed or maybeconfirmed.  I suspect
# they will be maybeconfirmed, e.g. ASLEEP and REMIND.  This hardcoding should
# disappear when we have customized statuses.
BugCheck("bugs WHERE bug_status IN ('NEW', 'ASSIGNED', 'REOPENED') AND everconfirmed = 0",
         "Bugs with confirmed status but don't have everconfirmed set"); 

Status("Checking votes/everconfirmed");

BugCheck("bugs INNER JOIN products ON bugs.product_id = products.id " .
         "WHERE everconfirmed = 0 AND votestoconfirm <= votes",
         "Bugs that have enough votes to be confirmed but haven't been");

###########################################################################
# Control Values
###########################################################################

# Checks for values that are invalid OR
# not among the 9 valid combinations
Status("Checking for bad values in group_control_map");
my $groups = join(", ", (CONTROLMAPNA, CONTROLMAPSHOWN, CONTROLMAPDEFAULT,
CONTROLMAPMANDATORY));
my $query = qq{
     SELECT COUNT(product_id) 
       FROM group_control_map 
      WHERE membercontrol NOT IN( $groups )
         OR othercontrol NOT IN( $groups )
         OR ((membercontrol != othercontrol)
             AND (membercontrol != } . CONTROLMAPSHOWN . q{)
             AND ((membercontrol != } . CONTROLMAPDEFAULT . q{)
                  OR (othercontrol = } . CONTROLMAPSHOWN . q{)))};
                  
my $c = $dbh->selectrow_array($query);
if ($c) {
    Alert("Found $c bad group_control_map entries");
}

Status("Checking for bugs with groups violating their product's group controls");
BugCheck("bugs
         INNER JOIN bug_group_map
            ON bugs.bug_id = bug_group_map.bug_id
          LEFT JOIN group_control_map
            ON bugs.product_id = group_control_map.product_id
           AND bug_group_map.group_id = group_control_map.group_id
         WHERE ((group_control_map.membercontrol = " . CONTROLMAPNA . ")
         OR (group_control_map.membercontrol IS NULL))",
         'Have groups not permitted for their products',
         'createmissinggroupcontrolmapentries',
         'Permit the missing groups for the affected products
          (set member control to <code>SHOWN</code>)');

BugCheck("bugs
         INNER JOIN group_control_map
            ON bugs.product_id = group_control_map.product_id
         INNER JOIN groups
            ON group_control_map.group_id = groups.id
          LEFT JOIN bug_group_map
            ON bugs.bug_id = bug_group_map.bug_id
           AND group_control_map.group_id = bug_group_map.group_id
         WHERE group_control_map.membercontrol = " . CONTROLMAPMANDATORY . "
           AND bug_group_map.group_id IS NULL
           AND groups.isactive != 0",
         "Are missing groups required for their products");


###########################################################################
# Unsent mail
###########################################################################

Status("Checking for unsent mail");

my $time = $dbh->sql_interval(30, 'MINUTE');
my $badbugs = $dbh->selectcol_arrayref(qq{
                    SELECT bug_id 
                      FROM bugs 
                     WHERE (lastdiffed IS NULL OR lastdiffed < delta_ts)
                       AND delta_ts < now() - $time
                  ORDER BY bug_id});


if (scalar(@$badbugs > 0)) {
    Alert("Bugs that have changes but no mail sent for at least half an hour: " .
          BugListLinks(@$badbugs));

    print qq{<a href="sanitycheck.cgi?rescanallBugMail=1">Send these mails</a>.<p>\n};
}

###########################################################################
# End
###########################################################################

Status("Sanity check completed.");
$template->put_footer();
