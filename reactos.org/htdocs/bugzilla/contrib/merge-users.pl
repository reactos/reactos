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
# Contributor(s): Myk Melez <myk@mozilla.org>
#                 Frédéric Buclin <LpSolit@gmail.com>

use strict;

=head1 NAME

merge-users.pl - Merge two user accounts.

=head1 SYNOPSIS

 This script moves activity from one user account to another.
 Specify the two accounts on the command line, e.g.:

 ./merge-users.pl old_account@foo.com new_account@bar.com
 or:
 ./merge-users.pl id:old_userid id:new_userid
 or:
 ./merge-users.pl id:old_userid new_account@bar.com

 Notes: - the new account must already exist.
        - the id:old_userid syntax permits you to migrate
          activity from a deleted account to an existing one.

=cut

use lib qw(.);

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Util;
use Bugzilla::User;

use Getopt::Long;
use Pod::Usage;

my $dbh = Bugzilla->dbh;

# Display the help if called with --help or -?.
my $help  = 0;
my $result = GetOptions("help|?" => \$help);
pod2usage(0) if $help;


# Make sure accounts were specified on the command line and exist.
my $old = $ARGV[0] || die "You must specify an old user account.\n";
my $old_id;
if ($old =~ /^id:(\d+)$/) {
    # As the old user account may be a deleted one, we don't
    # check whether this user ID is valid or not.
    # If it never existed, no damage will be done.
    $old_id = $1;
}
else {
    trick_taint($old);
    $old_id = $dbh->selectrow_array('SELECT userid FROM profiles
                                      WHERE login_name = ?',
                                      undef, $old);
}
if ($old_id) {
    print "OK, old user account $old found; user ID: $old_id.\n";
}
else {
    die "The old user account $old does not exist.\n";
}

my $new = $ARGV[1] || die "You must specify a new user account.\n";
my $new_id;
if ($new =~ /^id:(\d+)$/) {
    $new_id = $1;
    # Make sure this user ID exists.
    $new_id = $dbh->selectrow_array('SELECT userid FROM profiles
                                      WHERE userid = ?',
                                      undef, $new_id);
}
else {
    trick_taint($new);
    $new_id = $dbh->selectrow_array('SELECT userid FROM profiles
                                      WHERE login_name = ?',
                                      undef, $new);
}
if ($new_id) {
    print "OK, new user account $new found; user ID: $new_id.\n";
}
else {
    die "The new user account $new does not exist.\n";
}

# Make sure the old and new accounts are different.
if ($old_id == $new_id) {
    die "\nBoth accounts are identical. There is nothing to migrate.\n";
}


# A list of tables and columns to be changed:
# - keys of the hash are table names to be locked/altered;
# - values of the hash contain column names to be updated
#   as well as the columns they depend on:
#   = each array is of the form:
#     ['foo1 bar11 bar12 bar13', 'foo2 bar21 bar22', 'foo3 bar31 bar32']
#     where fooN is the column to update, and barN1, barN2, ... are
#     the columns to take into account to avoid duplicated entries.
#     Note that the barNM columns are optional.
my $changes = {
    # Tables affecting bugs.
    bugs            => ['assigned_to', 'reporter', 'qa_contact'],
    bugs_activity   => ['who'],
    attachments     => ['submitter_id'],
    flags           => ['setter_id', 'requestee_id'],
    cc              => ['who bug_id'],
    longdescs       => ['who'],
    votes           => ['who'],
    # Tables affecting global behavior / other users.
    components      => ['initialowner', 'initialqacontact'],
    component_cc    => ['user_id component_id'],
    quips           => ['userid'],
    series          => ['creator'],
    whine_events    => ['owner_userid'],
    watch           => ['watcher watched', 'watched watcher'],
    # Tables affecting the user directly.
    namedqueries    => ['userid name'],
    namedqueries_link_in_footer => ['user_id namedquery_id'],
    user_group_map  => ['user_id group_id isbless grant_type'],
    email_setting   => ['user_id relationship event'],
    profile_setting => ['user_id setting_name'],
    profiles_activity => ['userid', 'who'], # Should activity be migrated?

    # Only do it if mailto_type = 0, i.e is pointing to a user account!
    # This requires to be done separately due to this condition.
    whine_schedules => [], # ['mailto'],

    # Delete all old records for these tables; no migration.
    logincookies    => [], # ['userid'],
    tokens          => [], # ['userid'],
    profiles        => [], # ['userid'],
};

# Lock tables
my @locked_tables = map {"$_ WRITE"} keys(%$changes);
$dbh->bz_lock_tables(@locked_tables);

# Delete old records from logincookies and tokens tables.
$dbh->do('DELETE FROM logincookies WHERE userid = ?', undef, $old_id);
$dbh->do('DELETE FROM tokens WHERE userid = ?', undef, $old_id);

# Migrate records from old user to new user.
foreach my $table (keys(%$changes)) {
    foreach my $column_list (@{$changes->{$table}}) {
        # Get all columns to consider. There is always at least
        # one column given: the one to update.
        my @columns = split(/[\s]+/, $column_list);
        my $cols_to_check = join(' AND ', map {"$_ = ?"} @columns);
        # The first column of the list is the one to update.
        my $col_to_update = shift @columns;

        # Will be used to migrate the old user account to the new one.
        my $sth_update = $dbh->prepare("UPDATE $table
                                           SET $col_to_update = ?
                                         WHERE $cols_to_check");

        # Do we have additional columns to take care of?
        if (scalar(@columns)) {
            my $cols_to_query = join(', ', @columns);

            # Get existing entries for the old user account.
            my $old_entries = 
                $dbh->selectall_arrayref("SELECT $cols_to_query
                                            FROM $table
                                           WHERE $col_to_update = ?",
                                          undef, $old_id);

            # Will be used to check whether the same entry exists
            # for the new user account.
            my $sth_select = $dbh->prepare("SELECT COUNT(*)
                                              FROM $table
                                             WHERE $cols_to_check");

            # Will be used to delete duplicated entries.
            my $sth_delete = $dbh->prepare("DELETE FROM $table
                                             WHERE $cols_to_check");

            foreach my $entry (@$old_entries) {
                my $exists = $dbh->selectrow_array($sth_select, undef,
                                                   ($new_id, @$entry));

                if ($exists) {
                    $sth_delete->execute($old_id, @$entry);
                }
                else {
                    $sth_update->execute($new_id, $old_id, @$entry);
                }
            }
        }
        # No check required. Update the column directly.
        else {
            $sth_update->execute($new_id, $old_id);
        }
        print "OK, records in the '$col_to_update' column of the '$table' table\n" .
              "have been migrated to the new user account.\n";
    }
}

# Only update 'whine_schedules' if mailto_type = 0.
# (i.e. is pointing to a user ID).
$dbh->do('UPDATE whine_schedules SET mailto = ?
           WHERE mailto = ? AND mailto_type = ?',
          undef, ($new_id, $old_id, 0));
print "OK, records in the 'mailto' column of the 'whine_schedules' table\n" .
      "have been migrated to the new user account.\n";

# Delete the old record from the profiles table.
$dbh->do('DELETE FROM profiles WHERE userid = ?', undef, $old_id);

# Unlock tables
$dbh->bz_unlock_tables();

my $user = new Bugzilla::User($new_id);
$user->derive_regexp_groups();

print "Done.\n";
