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
# The Original Code is the Gnats To Bugzilla Conversion Utility.
#
# The Initial Developer of the Original Code is Tom
# Schutter. Portions created by Tom Schutter are
# Copyright (C) 1999 Tom Schutter. All
# Rights Reserved.
#
# Contributor(s): Tom Schutter <tom@platte.com>
#
# Perl script to convert a GNATS database to a Bugzilla database.
# This script generates a file that contains SQL commands for MySQL.
# This script DOES NOT MODIFY the GNATS database.
# This script DOES NOT MODIFY the Bugzilla database.
#
# Usage procedure:
#   1) Regenerate the GNATS index file.  It sometimes has inconsistencies,
#      and this script relies on it being correct.  Use the GNATS command:
#        gen-index --numeric --outfile=$GNATS_DIR/gnats-db/gnats-adm/index
#   2) Modify variables at the beginning of this script to match
#      what your site requires.
#   3) Modify translate_pr() and write_bugs() below to fixup mapping from
#      your GNATS policies to Bugzilla.  For example, how do the
#      Severity/Priority fields map to bug_severity/priority?
#   4) Run this script.
#   5) Fix the problems in the GNATS database identified in the output
#      script file gnats2bz_cleanup.sh.  Fixing problems may be a job
#      for a custom perl script.  If you make changes to GNATS, goto step 2.
#   6) Examine the statistics in the output file gnats2bz_stats.txt.
#      These may indicate some more cleanup that is needed.  For example,
#      you may find that there are invalid "State"s, or not a consistent
#      scheme for "Release"s.  If you make changes to GNATS, goto step 2.
#   7) Examine the output data file gnats2bz_data.sql.  If problems
#      exist, goto step 2.
#   8) Create a new, empty Bugzilla database.
#   9) Import the data using the command:
#      mysql -uroot -p'ROOT_PASSWORD' bugs < gnats2bz_data.sql
#   10) Update the shadow directory with the command:
#       cd $BUGZILLA_DIR; ./processmail regenerate
#   11) Run a sanity check by visiting the sanitycheck.cgi page.
#   12) Manually verify that the database is ok.  If it is not, goto step 2.
#
# Important notes:
#   Confidential is not mapped or exported.
#   Submitter-Id is not mapped or exported.
#
# Design decisions:
#   This script generates a SQL script file rather than dumping the data
#     directly into the database.  This is to allow the user to check
#     and/or modify the results before they are put into the database.
#   The PR number is very important and must be maintained as the Bugzilla
#     bug number, because there are many references to the PR number, such
#     as in code comments, CVS comments, customer communications, etc.
#   Reading ENUMERATED and TEXT fields:
#     1) All leading and trailing whitespace is stripped.
#   Reading MULTITEXT fields:
#     1) All leading blank lines are stripped.
#     2) All trailing whitespace is stripped.
#     3) Indentation is preserved.
#   Audit-Trail is not mapped to bugs_activity table, because there
#     is no place to put the "Why" text, which can have a fair amount
#     of information content.
#
# 15 January 2002 - changes from Andrea Dell'Amico <adellam@link.it>
#
#     * Adapted to the new database structure: now long_descs is a 
#       separate table.
#     * Set a default for the target milestone, otherwise bugzilla
#       doesn't work with the imported database if milestones are used.
#     * In gnats version 3.113 records are separated by "|" and not ":".
#     * userid "1" is for the bugzilla administrator, so it's better to
#       start from 2.
#

use strict;

# Suffix to be appended to username to make it an email address.
my($username_suffix) = "\@platte.com";

# Default organization that should be ignored and not passed on to Bugzilla.
# Only bugs that are reported outside of the default organization will have
# their Originator,Organization fields passed on.
# The assumption here is that if the Organization is identical to the
# $default_organization, then the Originator will most likely be only an
# alias for the From field in the mail header.
my($default_organization) = "Platte River Associates|platte";

# Username for reporter field if unable to determine from mail header
my($gnats_username) = "gnats\@platte.com";

# Flag indicating if cleanup file should use edit-pr or ${EDITOR}.
# Using edit-pr is safer, but may be too slow if there are too many
# PRs that need cleanup.  If you use ${EDITOR}, then you must make
# sure that you have exclusive access to the database, and that you
# do not screw up any fields.
my($cleanup_with_edit_pr) = 0;

# Component name and description for bugs imported from GNATS.
my($default_component) = "GNATS Import";
my($default_component_description) = "Bugs imported from GNATS.";

# First generated userid. Start from 2: 1 is used for the bugzilla
# administrator.
my($userid_base) = 2;

# Output filenames.
my($cleanup_pathname) = "gnats2bz_cleanup.sh";
my($stats_pathname) = "gnats2bz_stats.txt";
my($data_pathname) = "gnats2bz_data.sql";

# List of ENUMERATED and TEXT fields.
my(@text_fields) = qw(Number Category Synopsis Confidential Severity
                      Priority Responsible State Class Submitter-Id
                      Arrival-Date Originator Release);

# List of MULTITEXT fields.
my(@multitext_fields) = qw(Mail-Header Organization Environment Description
                           How-To-Repeat Fix Audit-Trail Unformatted);

# List of fields to report statistics for.
my(@statistics_fields) = qw(Category Confidential Severity Priority
                            Responsible State Class Submitter-Id Originator
                            Organization Release Environment);

# Array to hold list of GNATS PRs.
my(@pr_list);

# Array to hold list of GNATS categories.
my(@categories_list);

# Array to hold list of GNATS responsible users.
my(@responsible_list);

# Array to hold list of usernames.
my(@username_list);
# Put the gnats_username in first.
get_userid($gnats_username);

# Hash to hold list of versions.
my(%versions_table);

# Hash to hold contents of PR.
my(%pr_data);

# String to hold duplicate fields found during read of PR.
my($pr_data_dup_fields) = "";

# String to hold badly labeled fields found during read of PR.
# This usually happens when the user does not separate the field name
# from the field data with whitespace.
my($pr_data_bad_fields) = " ";

# Hash to hold statistics (note that this a hash of hashes).
my(%pr_stats);

# Process commmand line.
my($gnats_db_dir) = @ARGV;
defined($gnats_db_dir) || die "gnats-db dir not specified";
(-d $gnats_db_dir) || die "$gnats_db_dir is not a directory";

# Load @pr_list from GNATS index file.
my($index_pathname) = $gnats_db_dir . "/gnats-adm/index";
(-f $index_pathname) || die "$index_pathname not found";
print "Reading $index_pathname...\n";
if (!load_index($index_pathname)) {
    return(0);
}

# Load @category_list from GNATS categories file.
my($categories_pathname) = $gnats_db_dir . "/gnats-adm/categories";
(-f $categories_pathname) || die "$categories_pathname not found";
print "Reading $categories_pathname...\n";
if (!load_categories($categories_pathname)) {
    return(0);
}

# Load @responsible_list from GNATS responsible file.
my($responsible_pathname) = $gnats_db_dir . "/gnats-adm/responsible";
(-f $responsible_pathname) || die "$responsible_pathname not found";
print "Reading $responsible_pathname...\n";
if (!load_responsible($responsible_pathname)) {
    return(0);
}

# Open cleanup file.
open(CLEANUP, ">$cleanup_pathname") ||
    die "Unable to open $cleanup_pathname: $!";
chmod(0744, $cleanup_pathname) || warn "Unable to chmod $cleanup_pathname: $!";
print CLEANUP "#!/bin/sh\n";
print CLEANUP "# List of PRs that have problems found by gnats2bz.pl.\n";

# Open data file.
open(DATA, ">$data_pathname") || die "Unable to open $data_pathname: $!";
print DATA "-- Exported data from $gnats_db_dir by gnats2bz.pl.\n";
print DATA "-- Load it into a Bugzilla database using the command:\n";
print DATA "--   mysql -uroot -p'ROOT_PASSWORD' bugs < gnats2bz_data.sql\n";
print DATA "--\n";

# Loop over @pr_list.
my($pr);
foreach $pr (@pr_list) {
    print "Processing $pr...\n";
    if (!read_pr("$gnats_db_dir/$pr")) {
        next;
    }

    translate_pr();

    check_pr($pr);

    collect_stats();

    update_versions();

    write_bugs();

    write_longdescs();

}

write_non_bugs_tables();

close(CLEANUP) || die "Unable to close $cleanup_pathname: $!";
close(DATA) || die "Unable to close $data_pathname: $!";

print "Generating $stats_pathname...\n";
report_stats();

sub load_index {
    my($pathname) = @_;
    my($record);
    my(@fields);

    open(INDEX, $pathname) || die "Unable to open $pathname: $!";

    while ($record = <INDEX>) {
        @fields = split(/\|/, $record);
        push(@pr_list, $fields[0]);
    }

    close(INDEX) || die "Unable to close $pathname: $!";

    return(1);
}

sub load_categories {
    my($pathname) = @_;
    my($record);

    open(CATEGORIES, $pathname) || die "Unable to open $pathname: $!";

    while ($record = <CATEGORIES>) {
        if ($record =~ /^#/) {
            next;
        }
        push(@categories_list, [split(/:/, $record)]);
    }

    close(CATEGORIES) || die "Unable to close $pathname: $!";

    return(1);
}

sub load_responsible {
    my($pathname) = @_;
    my($record);

    open(RESPONSIBLE, $pathname) || die "Unable to open $pathname: $!";

    while ($record = <RESPONSIBLE>) {
        if ($record =~ /^#/) {
            next;
        }
        push(@responsible_list, [split(/\|/, $record)]);
    }

    close(RESPONSIBLE) || die "Unable to close $pathname: $!";

    return(1);
}

sub read_pr {
    my($pr_filename) = @_;
    my($multitext) = "Mail-Header";
    my($field, $mail_header);

    # Empty the hash.
    %pr_data = ();

    # Empty the list of duplicate fields.
    $pr_data_dup_fields = "";

    # Empty the list of badly labeled fields.
    $pr_data_bad_fields = "";

    unless (open(PR, $pr_filename)) {
        warn "error opening $pr_filename: $!";
        return(0);
    }

    LINELOOP: while (<PR>) {
        chomp;

        if ($multitext eq "Unformatted") {
            # once we reach "Unformatted", rest of file goes there
            $pr_data{$multitext} = append_multitext($pr_data{$multitext}, $_);
            next LINELOOP;
        }

        # Handle ENUMERATED and TEXT fields.
        foreach $field (@text_fields) {
            if (/^>$field:($|\s+)/) {
                $pr_data{$field} = $'; # part of string after match
                $pr_data{$field} =~ s/\s+$//; # strip trailing whitespace
                $multitext = "";
                next LINELOOP;
            }
        }

        # Handle MULTITEXT fields.
        foreach $field (@multitext_fields) {
            if (/^>$field:\s*$/) {
                $_ = $'; # set to part of string after match part
                if (defined($pr_data{$field})) {
                    if ($pr_data_dup_fields eq "") {
                        $pr_data_dup_fields = $field;
                    } else {
                        $pr_data_dup_fields = "$pr_data_dup_fields $field";
                    }
                }
                $pr_data{$field} = $_;
                $multitext = $field;
                next LINELOOP;
            }
        }

        # Check for badly labeled fields.
        foreach $field ((@text_fields, @multitext_fields)) {
            if (/^>$field:/) {
                if ($pr_data_bad_fields eq "") {
                    $pr_data_bad_fields = $field;
                } else {
                    $pr_data_bad_fields = "$pr_data_bad_fields $field";
                }
            }
        }

        # Handle continued MULTITEXT field.
        $pr_data{$multitext} = append_multitext($pr_data{$multitext}, $_);
    }

    close(PR) || warn "error closing $pr_filename: $!";

    # Strip trailing newlines from MULTITEXT fields.
    foreach $field (@multitext_fields) {
        if (defined($pr_data{$field})) {
            $pr_data{$field} =~ s/\s+$//;
        }
    }

    return(1);
}

sub append_multitext {
    my($original, $addition) = @_;

    if (defined($original) && $original ne "") {
        return "$original\n$addition";
    } else {
        return $addition;
    }
}

sub check_pr {
    my($pr) = @_;
    my($error_list) = "";

    if ($pr_data_dup_fields ne "") {
        $error_list = append_error($error_list, "Multiple '$pr_data_dup_fields'");
    }

    if ($pr_data_bad_fields ne "") {
        $error_list = append_error($error_list, "Bad field labels '$pr_data_bad_fields'");
    }

    if (!defined($pr_data{"Description"}) || $pr_data{"Description"} eq "") {
        $error_list = append_error($error_list, "Description empty");
    }

    if (defined($pr_data{"Unformatted"}) && $pr_data{"Unformatted"} ne "") {
        $error_list = append_error($error_list, "Unformatted text");
    }

    if (defined($pr_data{"Release"}) && length($pr_data{"Release"}) > 16) {
        $error_list = append_error($error_list, "Release > 16 chars");
    }

    if (defined($pr_data{"Fix"}) && $pr_data{"Fix"} =~ /State-Changed-/) {
        $error_list = append_error($error_list, "Audit in Fix field");
    }

    if (defined($pr_data{"Arrival-Date"})) {
        if ($pr_data{"Arrival-Date"} eq "") {
            $error_list = append_error($error_list, "Arrival-Date empty");

        } elsif (unixdate2datetime($pr, $pr_data{"Arrival-Date"}) eq "") {
            $error_list = append_error($error_list, "Arrival-Date format");
        }
    }

    # More checks should go here.

    if ($error_list ne "") {
        if ($cleanup_with_edit_pr) {
            my(@parts) = split("/", $pr);
            my($pr_num) = $parts[1];
            print CLEANUP "echo \"$error_list\"; edit-pr $pr_num\n";
        } else {
            print CLEANUP "echo \"$error_list\"; \${EDITOR} $pr\n";
        }
    }
}

sub append_error {
    my($original, $addition) = @_;

    if ($original ne "") {
        return "$original, $addition";
    } else {
        return $addition;
    }
}

sub translate_pr {
    # This function performs GNATS -> Bugzilla translations that should
    # happen before collect_stats().

    if (!defined($pr_data{"Organization"})) {
        $pr_data{"Originator"} = "";
    }
    if ($pr_data{"Organization"} =~ /$default_organization/) {
        $pr_data{"Originator"} = "";
        $pr_data{"Organization"} = "";
    }
    $pr_data{"Organization"} =~ s/^\s+//g; # strip leading whitespace

    if (!defined($pr_data{"Release"}) ||
        $pr_data{"Release"} eq "" ||
        $pr_data{"Release"} =~ /^unknown-1.0$/
        ) {
        $pr_data{"Release"} = "unknown";
    }

    if (defined($pr_data{"Responsible"})) {
        $pr_data{"Responsible"} =~ /\w+/;
        $pr_data{"Responsible"} = "$&$username_suffix";
    }

    my($rep_platform, $op_sys) = ("All", "All");
    if (defined($pr_data{"Environment"})) {
        if ($pr_data{"Environment"} =~ /[wW]in.*NT/) {
            $rep_platform = "PC";
            $op_sys = "Windows NT";
        } elsif ($pr_data{"Environment"} =~ /[wW]in.*95/) {
            $rep_platform = "PC";
            $op_sys = "Windows 95";
        } elsif ($pr_data{"Environment"} =~ /[wW]in.*98/) {
            $rep_platform = "PC";
            $op_sys = "Windows 98";
        } elsif ($pr_data{"Environment"} =~ /OSF/) {
            $rep_platform = "DEC";
            $op_sys = "OSF/1";
        } elsif ($pr_data{"Environment"} =~ /AIX/) {
            $rep_platform = "RS/6000";
            $op_sys = "AIX";
        } elsif ($pr_data{"Environment"} =~ /IRIX/) {
            $rep_platform = "SGI";
            $op_sys = "IRIX";
        } elsif ($pr_data{"Environment"} =~ /SunOS.*5\.\d/) {
            $rep_platform = "Sun";
            $op_sys = "Solaris";
        } elsif ($pr_data{"Environment"} =~ /SunOS.*4\.\d/) {
            $rep_platform = "Sun";
            $op_sys = "SunOS";
        }
    }

    $pr_data{"Environment"} = "$rep_platform:$op_sys";
}

sub collect_stats {
    my($field, $value);

    foreach $field (@statistics_fields) {
        $value = $pr_data{$field};
        if (!defined($value)) {
            $value = "";
        }
        if (defined($pr_stats{$field}{$value})) {
            $pr_stats{$field}{$value}++;
        } else {
            $pr_stats{$field}{$value} = 1;
        }
    }
}

sub report_stats {
    my($field, $value, $count);

    open(STATS, ">$stats_pathname") ||
        die "Unable to open $stats_pathname: $!";
    print STATS "Statistics of $gnats_db_dir collated by gnats2bz.pl.\n";

    my($field_stats);
    while (($field, $field_stats) = each(%pr_stats)) {
        print STATS "\n$field:\n";
        while (($value, $count) = each(%$field_stats)) {
            print STATS "  $value: $count\n";
        }
    }

    close(STATS) || die "Unable to close $stats_pathname: $!";
}

sub get_userid {
    my($responsible) = @_;
    my($username, $userid);

    if (!defined($responsible)) {
        return(-1);
    }

    # Search for current username in the list.
    $userid = $userid_base;
    foreach $username (@username_list) {
        if ($username eq $responsible) {
            return($userid);
        }
        $userid++;
    }

    push(@username_list, $responsible);
    return($userid);
}

sub update_versions {

    if (!defined($pr_data{"Release"}) || !defined($pr_data{"Category"})) {
        return;
    }

    my($curr_product) = $pr_data{"Category"};
    my($curr_version) = $pr_data{"Release"};

    if ($curr_version eq "") {
        return;
    }

    if (!defined($versions_table{$curr_product})) {
        $versions_table{$curr_product} = [ ];
    }

    my($version_list) = $versions_table{$curr_product};
    my($version);
    foreach $version (@$version_list) {
        if ($version eq $curr_version) {
            return;
        }
    }

    push(@$version_list, $curr_version);
}

sub write_bugs {
    my($bug_id) = $pr_data{"Number"};

    my($userid) = get_userid($pr_data{"Responsible"});

    # Mapping from Class,Severity to bug_severity
    # At our site, the Severity,Priority fields have degenerated
    # into a 9-level priority field.
    my($bug_severity) = "normal";
    if ($pr_data{"Class"} eq "change-request") {
        $bug_severity = "enhancement";
    } elsif (defined($pr_data{"Synopsis"})) {
        if ($pr_data{"Synopsis"} =~ /crash|assert/i) {
            $bug_severity = "critical";
        } elsif ($pr_data{"Synopsis"} =~ /wrong|error/i) {
            $bug_severity = "major";
        }
    }
    $bug_severity = SqlQuote($bug_severity);

    # Mapping from Severity,Priority to priority
    # At our site, the Severity,Priority fields have degenerated
    # into a 9-level priority field.
    my($priority) = "P1";
    if (defined($pr_data{"Severity"}) && defined($pr_data{"Severity"})) {
        if ($pr_data{"Severity"} eq "critical") {
            if ($pr_data{"Priority"} eq "high") {
                $priority = "P1";
            } else {
                $priority = "P2";
            }
        } elsif ($pr_data{"Severity"} eq "serious") {
            if ($pr_data{"Priority"} eq "low") {
                $priority = "P4";
            } else {
                $priority = "P3";
            }
        } else {
            if ($pr_data{"Priority"} eq "high") {
                $priority = "P4";
            } else {
                $priority = "P5";
            }
        }
    }
    $priority = SqlQuote($priority);

    # Map State,Class to bug_status,resolution
    my($bug_status, $resolution);
    if ($pr_data{"State"} eq "open" || $pr_data{"State"} eq "analyzed") {
        $bug_status = "ASSIGNED";
        $resolution = "";
    } elsif ($pr_data{"State"} eq "feedback") {
        $bug_status = "RESOLVED";
        $resolution = "FIXED";
    } elsif ($pr_data{"State"} eq "closed") {
        $bug_status = "CLOSED";
        if (defined($pr_data{"Class"}) && $pr_data{"Class"} =~ /^duplicate/) {
            $resolution = "DUPLICATE";
        } elsif (defined($pr_data{"Class"}) && $pr_data{"Class"} =~ /^mistaken/) {
            $resolution = "INVALID";
        } else {
            $resolution = "FIXED";
        }
    } elsif ($pr_data{"State"} eq "suspended") {
        $bug_status = "RESOLVED";
        $resolution = "WONTFIX";
    } else {
        $bug_status = "NEW";
        $resolution = "";
    }
    $bug_status = SqlQuote($bug_status);
    $resolution = SqlQuote($resolution);

    my($creation_ts) = "";
    if (defined($pr_data{"Arrival-Date"}) && $pr_data{"Arrival-Date"} ne "") {
        $creation_ts = unixdate2datetime($bug_id, $pr_data{"Arrival-Date"});
    }
    $creation_ts = SqlQuote($creation_ts);

    my($delta_ts) = "";
    if (defined($pr_data{"Audit-Trail"})) {
        # note that (?:.|\n)+ is greedy, so this should match the
        # last Changed-When
        if ($pr_data{"Audit-Trail"} =~ /(?:.|\n)+-Changed-When: (.+)/) {
            $delta_ts = unixdate2timestamp($bug_id, $1);
        }
    }
    if ($delta_ts eq "") {
        if (defined($pr_data{"Arrival-Date"}) && $pr_data{"Arrival-Date"} ne "") {
            $delta_ts = unixdate2timestamp($bug_id, $pr_data{"Arrival-Date"});
        }
    }
    $delta_ts = SqlQuote($delta_ts);

    my($short_desc) = SqlQuote($pr_data{"Synopsis"});

    my($rep_platform, $op_sys) = split(/\|/, $pr_data{"Environment"});
    $rep_platform = SqlQuote($rep_platform);
    $op_sys = SqlQuote($op_sys);

    my($reporter) = get_userid($gnats_username);
    if (
        defined($pr_data{"Mail-Header"}) &&
        $pr_data{"Mail-Header"} =~ /From ([\w.]+\@[\w.]+)/
        ) {
        $reporter = get_userid($1);
    }

    my($version) = "";
    if (defined($pr_data{"Release"})) {
        $version = substr($pr_data{"Release"}, 0, 16);
    }
    $version = SqlQuote($version);

    my($product) = "";
    if (defined($pr_data{"Category"})) {
        $product = $pr_data{"Category"};
    }
    $product = SqlQuote($product);

    my($component) = SqlQuote($default_component);

    my($target_milestone) = "0";
    # $target_milestone = SqlQuote($target_milestone);

    my($qa_contact) = "0";

    # my($bug_file_loc) = "";
    # $bug_file_loc = SqlQuote($bug_file_loc);

    # my($status_whiteboard) = "";
    # $status_whiteboard = SqlQuote($status_whiteboard);

    print DATA "\ninsert into bugs (\n";
    print DATA "  bug_id, assigned_to, bug_severity, priority, bug_status, creation_ts, delta_ts,\n";
    print DATA "  short_desc,\n";
    print DATA "  rep_platform, op_sys, reporter, version,\n";
    print DATA "  product, component, resolution, target_milestone, qa_contact\n";
    print DATA ") values (\n";
    print DATA "  $bug_id, $userid, $bug_severity, $priority, $bug_status, $creation_ts, $delta_ts,\n";
    print DATA "  $short_desc,\n";
    print DATA "  $rep_platform, $op_sys, $reporter, $version,\n";
    print DATA "  $product, $component, $resolution, $target_milestone, $qa_contact\n";
    print DATA ");\n";
}

sub write_longdescs {

    my($bug_id) = $pr_data{"Number"};
    my($who) = get_userid($pr_data{"Responsible"});;
    my($bug_when) = "";
    if (defined($pr_data{"Arrival-Date"}) && $pr_data{"Arrival-Date"} ne "") {
        $bug_when = unixdate2datetime($bug_id, $pr_data{"Arrival-Date"});
    }
    $bug_when = SqlQuote($bug_when);
    my($thetext) = $pr_data{"Description"};
    if (defined($pr_data{"How-To-Repeat"}) && $pr_data{"How-To-Repeat"} ne "") {
        $thetext =
            $thetext . "\n\nHow-To-Repeat:\n" . $pr_data{"How-To-Repeat"};
    }
    if (defined($pr_data{"Fix"}) && $pr_data{"Fix"} ne "") {
        $thetext = $thetext . "\n\nFix:\n" . $pr_data{"Fix"};
    }
    if (defined($pr_data{"Originator"}) && $pr_data{"Originator"} ne "") {
        $thetext = $thetext . "\n\nOriginator:\n" . $pr_data{"Originator"};
    }
    if (defined($pr_data{"Organization"}) && $pr_data{"Organization"} ne "") {
        $thetext = $thetext . "\n\nOrganization:\n" . $pr_data{"Organization"};
    }
    if (defined($pr_data{"Audit-Trail"}) && $pr_data{"Audit-Trail"} ne "") {
        $thetext = $thetext . "\n\nAudit-Trail:\n" . $pr_data{"Audit-Trail"};
    }
    if (defined($pr_data{"Unformatted"}) && $pr_data{"Unformatted"} ne "") {
        $thetext = $thetext . "\n\nUnformatted:\n" . $pr_data{"Unformatted"};
    }
    $thetext = SqlQuote($thetext);

    print DATA "\ninsert into longdescs (\n";
    print DATA "  bug_id, who, bug_when, thetext\n";
    print DATA ") values (\n";
    print DATA "  $bug_id, $who, $bug_when, $thetext\n";
    print DATA ");\n";

}

sub write_non_bugs_tables {

    my($categories_record);
    foreach $categories_record (@categories_list) {
        my($component) = SqlQuote($default_component);
        my($product) = SqlQuote(@$categories_record[0]);
        my($description) = SqlQuote(@$categories_record[1]);
        my($initialowner) = SqlQuote(@$categories_record[2] . $username_suffix);

        print DATA "\ninsert into products (\n";
        print DATA
            "  product, description, milestoneurl, disallownew\n";
        print DATA ") values (\n";
        print DATA
            "  $product, $description, '', 0\n";
        print DATA ");\n";

        print DATA "\ninsert into components (\n";
        print DATA
            "  value, program, initialowner, initialqacontact, description\n";
        print DATA ") values (\n";
        print DATA
            "  $component, $product, $initialowner, '', $description\n";
        print DATA ");\n";

        print DATA "\ninsert into milestones (\n";
        print DATA
            "  value, product, sortkey\n";
        print DATA ") values (\n";
        print DATA
            "  0, $product, 0\n";
        print DATA ");\n";
    }

    my($username);
    my($userid) = $userid_base;
    my($password) = "password";
    my($realname);
    my($groupset) = 0;
    foreach $username (@username_list) {
        $realname = map_username_to_realname($username);
        $username = SqlQuote($username);
        $realname = SqlQuote($realname);
        print DATA "\ninsert into profiles (\n";
        print DATA
            "  userid, login_name, cryptpassword, realname, groupset\n";
        print DATA ") values (\n";
        print DATA
            "  $userid, $username, encrypt('$password'), $realname, $groupset\n";
        print DATA ");\n";
        $userid++;
    }

    my($product);
    my($version_list);
    while (($product, $version_list) = each(%versions_table)) {
        $product = SqlQuote($product);

        my($version);
        foreach $version (@$version_list) {
            $version = SqlQuote($version);

            print DATA "\ninsert into versions (value, program) ";
            print DATA "values ($version, $product);\n";
        }
    }

}

sub map_username_to_realname() {
    my($username) = @_;
    my($name, $realname);

    # get the portion before the @
    $name = $username;
    $name =~ s/\@.*//;

    my($responsible_record);
    foreach $responsible_record (@responsible_list) {
        if (@$responsible_record[0] eq $name) {
            return(@$responsible_record[1]);
        }
        if (defined(@$responsible_record[2])) {
            if (@$responsible_record[2] eq $username) {
                return(@$responsible_record[1]);
            }
        }
    }

    return("");
}

sub detaint_string {
    my ($str) = @_;
    $str =~ m/^(.*)$/s;
    $str = $1;
}

sub SqlQuote {
    my ($str) = (@_);
    $str =~ s/([\\\'])/\\$1/g;
    $str =~ s/\0/\\0/g;
    # If it's been SqlQuote()ed, then it's safe, so we tell -T that.
    $str = detaint_string($str);
    return "'$str'";
}

sub unixdate2datetime {
    my($bugid, $unixdate) = @_;
    my($year, $month, $day, $hour, $min, $sec);

    if (!split_unixdate($bugid, $unixdate, \$year, \$month, \$day, \$hour, \$min, \$sec)) {
        return("");
    }

    return("$year-$month-$day $hour:$min:$sec");
}

sub unixdate2timestamp {
    my($bugid, $unixdate) = @_;
    my($year, $month, $day, $hour, $min, $sec);

    if (!split_unixdate($bugid, $unixdate, \$year, \$month, \$day, \$hour, \$min, \$sec)) {
        return("");
    }

    return("$year$month$day$hour$min$sec");
}

sub split_unixdate {
    # "Tue Jun  6 14:50:00 1995"
    # "Mon Nov 20 17:03:11 [MST] 1995"
    # "12/13/94"
    # "jan 1, 1995"
    my($bugid, $unixdate, $year, $month, $day, $hour, $min, $sec) = @_;
    my(@parts);

    $$hour = "00";
    $$min = "00";
    $$sec = "00";

    @parts = split(/ +/, $unixdate);
    if (@parts >= 5) {
        # year
        $$year = $parts[4];
        if ($$year =~ /[A-Z]{3}/) {
            # Must be timezone, try next field.
            $$year = $parts[5];
        }
        if ($$year =~ /\D/) {
            warn "$bugid: Error processing year part '$$year' of date '$unixdate'\n";
            return(0);
        }
        if ($$year < 30) {
            $$year = "20" . $$year;
        } elsif ($$year < 100) {
            $$year = "19" . $$year;
        } elsif ($$year < 1970 || $$year > 2029) {
            warn "$bugid: Error processing year part '$$year' of date '$unixdate'\n";
            return(0);
        }

        # month
        $$month = $parts[1];
        if ($$month =~ /\D/) {
            if (!month2number($month)) {
                warn "$bugid: Error processing month part '$$month' of date '$unixdate'\n";
                return(0);
            }

        } elsif ($$month < 1 || $$month > 12) {
            warn "$bugid: Error processing month part '$$month' of date '$unixdate'\n";
            return(0);

        } elsif (length($$month) == 1) {
            $$month = "0" . $$month;
        }

        # day
        $$day = $parts[2];
        if ($$day < 1 || $$day > 31) {
            warn "$bugid: Error processing day part '$day' of date '$unixdate'\n";
            return(0);

        } elsif (length($$day) == 1) {
            $$day = "0" . $$day;
        }

        @parts = split(/:/, $parts[3]);
        $$hour = $parts[0];
        $$min = $parts[1];
        $$sec = $parts[2];

        return(1);

    } elsif (@parts == 3) {
        # year
        $$year = $parts[2];
        if ($$year =~ /\D/) {
            warn "$bugid: Error processing year part '$$year' of date '$unixdate'\n";
            return(0);
        }
        if ($$year < 30) {
            $$year = "20" . $$year;
        } elsif ($$year < 100) {
            $$year = "19" . $$year;
        } elsif ($$year < 1970 || $$year > 2029) {
            warn "$bugid: Error processing year part '$$year' of date '$unixdate'\n";
            return(0);
        }

        # month
        $$month = $parts[0];
        if ($$month =~ /\D/) {
            if (!month2number($month)) {
                warn "$bugid: Error processing month part '$$month' of date '$unixdate'\n";
                return(0);
            }

        } elsif ($$month < 1 || $$month > 12) {
            warn "$bugid: Error processing month part '$$month' of date '$unixdate'\n";
            return(0);

        } elsif (length($$month) == 1) {
            $$month = "0" . $$month;
        }

        # day
        $$day = $parts[1];
        $$day =~ s/,//;
        if ($$day < 1 || $$day > 31) {
            warn "$bugid: Error processing day part '$day' of date '$unixdate'\n";
            return(0);

        } elsif (length($$day) == 1) {
            $$day = "0" . $$day;
        }

        return(1);
    }

    @parts = split(/:/, $unixdate);
    if (@parts == 3 && length($unixdate) <= 8) {
        $$year = "19" . $parts[2];

        $$month = $parts[0];
        if (length($$month) == 1) {
            $$month = "0" . $$month;
        }

        $$day = $parts[1];
        if (length($$day) == 1) {
            $$day = "0" . $$day;
        }

        return(1);
    }

    warn "$bugid: Error processing date '$unixdate'\n";
    return(0);
}

sub month2number {
    my($month) = @_;

    if ($$month =~ /jan/i) {
        $$month = "01";
    } elsif ($$month =~ /feb/i) {
        $$month = "02";
    } elsif ($$month =~ /mar/i) {
        $$month = "03";
    } elsif ($$month =~ /apr/i) {
        $$month = "04";
    } elsif ($$month =~ /may/i) {
        $$month = "05";
    } elsif ($$month =~ /jun/i) {
        $$month = "06";
    } elsif ($$month =~ /jul/i) {
        $$month = "07";
    } elsif ($$month =~ /aug/i) {
        $$month = "08";
    } elsif ($$month =~ /sep/i) {
        $$month = "09";
    } elsif ($$month =~ /oct/i) {
        $$month = "10";
    } elsif ($$month =~ /nov/i) {
        $$month = "11";
    } elsif ($$month =~ /dec/i) {
        $$month = "12";
    } else {
        return(0);
    }

    return(1);
}

