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
# Contributor(s): Erik Stambaugh <erik@dasbistro.com>

################################################################################
# Script Initialization
################################################################################

use strict;

use lib ".";

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Search;
use Bugzilla::User;
use Bugzilla::Mailer;
use Bugzilla::Util;

# create some handles that we'll need
my $template = Bugzilla->template;
my $dbh      = Bugzilla->dbh;
my $sth;

# @seen_schedules is a list of all of the schedules that have already been
# touched by reset_timer.  If reset_timer sees a schedule more than once, it
# sets it to NULL so it won't come up again until the next execution of
# whine.pl
my @seen_schedules = ();

# These statement handles should live outside of their functions in order to
# allow the database to keep their SQL compiled.
my $sth_run_queries =
    $dbh->prepare("SELECT " .
                  "query_name, title, onemailperbug " .
                  "FROM whine_queries " .
                  "WHERE eventid=? " .
                  "ORDER BY sortkey");
my $sth_get_query =
    $dbh->prepare("SELECT query FROM namedqueries " .
                  "WHERE userid = ? AND name = ?");

# get the event that's scheduled with the lowest run_next value
my $sth_next_scheduled_event = $dbh->prepare(
    "SELECT " .
    " whine_schedules.eventid, " .
    " whine_events.owner_userid, " .
    " whine_events.subject, " .
    " whine_events.body " .
    "FROM whine_schedules " .
    "LEFT JOIN whine_events " .
    " ON whine_events.id = whine_schedules.eventid " .
    "WHERE run_next <= NOW() " .
    "ORDER BY run_next " .
    $dbh->sql_limit(1)
);

# get all pending schedules matching an eventid
my $sth_schedules_by_event = $dbh->prepare(
    "SELECT id, mailto_type, mailto " .
    "FROM whine_schedules " .
    "WHERE eventid=? AND run_next <= NOW()"
);


################################################################################
# Main Body Execution
################################################################################

# This script needs to check through the database for schedules that have
# run_next set to NULL, which means that schedule is new or has been altered.
# It then sets it to run immediately if the schedule entry has it running at
# an interval like every hour, otherwise to the appropriate day and time.

# After that, it looks over each user to see if they have schedules that need
# running, then runs those and generates the email messages.

# Send whines from the address in the 'mailfrom' Parameter so that all
# Bugzilla-originated mail appears to come from a single address.
my $fromaddress = Bugzilla->params->{'mailfrom'};

# get the current date and time
my ($now_sec, $now_minute, $now_hour, $now_day, $now_month, $now_year, 
    $now_weekday) = localtime;
# Convert year to two digits
$now_year = sprintf("%02d", $now_year % 100);
# Convert the month to January being "1" instead of January being "0".
$now_month++;

my @daysinmonth = qw(0 31 28 31 30 31 30 31 31 30 31 30 31);
# Alter February in case of a leap year.  This simple way to do it only
# applies if you won't be looking at February of next year, which whining
# doesn't need to do.
if (($now_year % 4 == 0) &&
    (($now_year % 100 != 0) || ($now_year % 400 == 0))) {
    $daysinmonth[2] = 29;
}

# run_day can contain either a calendar day (1, 2, 3...), a day of the week
# (Mon, Tue, Wed...), a range of days (All, MF), or 'last' for the last day of
# the month.
#
# run_time can contain either an hour (0, 1, 2...) or an interval
# (60min, 30min, 15min).
#
# We go over each uninitialized schedule record and use its settings to
# determine what the next time it runs should be
my $sched_h = $dbh->prepare("SELECT id, run_day, run_time " .
                            "FROM whine_schedules " .
                            "WHERE run_next IS NULL" );
$sched_h->execute();
while (my ($schedule_id, $day, $time) = $sched_h->fetchrow_array) {
    # fill in some defaults in case they're blank
    $day  ||= '0';
    $time ||= '0';

    # If this schedule is supposed to run today, we see if it's supposed to be
    # run at a particular hour.  If so, we set it for that hour, and if not,
    # it runs at an interval over the course of a day, which means we should
    # set it to run immediately.
    if (&check_today($day)) {
        # Values that are not entirely numeric are intervals, like "30min"
        if ($time !~ /^\d+$/) {
            # set it to now
            $sth = $dbh->prepare( "UPDATE whine_schedules " .
                                  "SET run_next=NOW() " .
                                  "WHERE id=?");
            $sth->execute($schedule_id);
        }
        # A time greater than now means it still has to run today
        elsif ($time >= $now_hour) {
            # set it to today + number of hours
            $sth = $dbh->prepare("UPDATE whine_schedules " .
                                 "SET run_next = CURRENT_DATE + " .
                                 $dbh->sql_interval('?', 'HOUR') .
                                 " WHERE id = ?");
            $sth->execute($time, $schedule_id);
        }
        # the target time is less than the current time
        else { # set it for the next applicable day
            $day = &get_next_date($day);
            $sth = $dbh->prepare("UPDATE whine_schedules " .
                                 "SET run_next = (CURRENT_DATE + " .
                                 $dbh->sql_interval('?', 'DAY') . ") + " .
                                 $dbh->sql_interval('?', 'HOUR') .
                                 " WHERE id = ?");
            $sth->execute($day, $time, $schedule_id);
        }

    }
    # If the schedule is not supposed to run today, we set it to run on the
    # appropriate date and time
    else {
        my $target_date = &get_next_date($day);
        # If configured for a particular time, set it to that, otherwise
        # midnight
        my $target_time = ($time =~ /^\d+$/) ? $time : 0;

        $sth = $dbh->prepare("UPDATE whine_schedules " .
                             "SET run_next = (CURRENT_DATE + " .
                             $dbh->sql_interval('?', 'DAY') . ") + " .
                             $dbh->sql_interval('?', 'HOUR') .
                             " WHERE id = ?");
        $sth->execute($target_date, $target_time, $schedule_id);
    }
}
$sched_h->finish();

# get_next_event
#
# This function will:
#   1. Lock whine_schedules
#   2. Grab the most overdue pending schedules on the same event that must run
#   3. Update those schedules' run_next value
#   4. Unlock the table
#   5. Return an event hashref
#
# The event hashref consists of:
#   eventid - ID of the event 
#   author  - user object for the event's creator
#   users   - array of user objects for recipients
#   subject - Subject line for the email
#   body    - the text inserted above the bug lists

sub get_next_event {
    my $event = {};

    # Loop until there's something to return
    until (scalar keys %{$event}) {

        $dbh->bz_lock_tables('whine_schedules WRITE',
                             'whine_events READ',
                             'profiles READ',
                             'groups READ',
                             'group_group_map READ',
                             'user_group_map READ');

        # Get the event ID for the first pending schedule
        $sth_next_scheduled_event->execute;
        my $fetched = $sth_next_scheduled_event->fetch;
        $sth_next_scheduled_event->finish;
        return undef unless $fetched;
        my ($eventid, $owner_id, $subject, $body) = @{$fetched};

        my $owner = Bugzilla::User->new($owner_id);

        my $whineatothers = $owner->in_group('bz_canusewhineatothers');

        my %user_objects;   # Used for keeping track of who has been added

        # Get all schedules that match that event ID and are pending
        $sth_schedules_by_event->execute($eventid);

        # Add the users from those schedules to the list
        while (my $row = $sth_schedules_by_event->fetch) {
            my ($sid, $mailto_type, $mailto) = @{$row};

            # Only bother doing any work if this user has whine permission
            if ($owner->in_group('bz_canusewhines')) {

                if ($mailto_type == MAILTO_USER) {
                    if (not defined $user_objects{$mailto}) {
                        if ($mailto == $owner_id) {
                            $user_objects{$mailto} = $owner;
                        }
                        elsif ($whineatothers) {
                            $user_objects{$mailto} = Bugzilla::User->new($mailto);
                        }
                    }
                }
                elsif ($mailto_type == MAILTO_GROUP) {
                    my $sth = $dbh->prepare("SELECT name FROM groups " .
                                            "WHERE id=?");
                    $sth->execute($mailto);
                    my $groupname = $sth->fetch->[0];
                    my $group_id = Bugzilla::Group::ValidateGroupName(
                        $groupname, $owner);
                    if ($group_id) {
                        my $glist = join(',',
                            @{Bugzilla::User->flatten_group_membership(
                            $group_id)});
                        $sth = $dbh->prepare("SELECT user_id FROM " .
                                             "user_group_map " .
                                             "WHERE group_id IN ($glist)");
                        $sth->execute();
                        for my $row (@{$sth->fetchall_arrayref}) {
                            if (not defined $user_objects{$row->[0]}) {
                                $user_objects{$row->[0]} =
                                    Bugzilla::User->new($row->[0]);
                            }
                        }
                    }
                }

            }

            reset_timer($sid);
        }

        $dbh->bz_unlock_tables();

        # Only set $event if the user is allowed to do whining
        if ($owner->in_group('bz_canusewhines')) {
            my @users = values %user_objects;
            $event = {
                    'eventid' => $eventid,
                    'author'  => $owner,
                    'mailto'  => \@users,
                    'subject' => $subject,
                    'body'    => $body,
            };
        }
    }
    return $event;
}

# Run the queries for each event
#
# $event:
#   eventid (the database ID for this event)
#   author  (user object for who created the event)
#   mailto  (array of user objects for mail targets)
#   subject (subject line for message)
#   body    (text blurb at top of message)
while (my $event = get_next_event) {

    my $eventid = $event->{'eventid'};

    # We loop for each target user because some of the queries will be using
    # subjective pronouns
    $dbh = Bugzilla->switch_to_shadow_db();
    for my $target (@{$event->{'mailto'}}) {
        my $args = {
            'subject'     => $event->{'subject'},
            'body'        => $event->{'body'},
            'eventid'     => $event->{'eventid'},
            'author'      => $event->{'author'},
            'recipient'   => $target,
            'from'        => $fromaddress,
        };

        # run the queries for this schedule
        my $queries = run_queries($args);

        # check to make sure there is something to output
        my $there_are_bugs = 0;
        for my $query (@{$queries}) {
            $there_are_bugs = 1 if scalar @{$query->{'bugs'}};
        }
        next unless $there_are_bugs;

        $args->{'queries'} = $queries;

        mail($args);
    }
    $dbh = Bugzilla->switch_to_main_db();
}

################################################################################
# Functions
################################################################################

# The mail and run_queries functions use an anonymous hash ($args) for their
# arguments, which are then passed to the templates.
#
# When run_queries is run, $args contains the following fields:
#  - body           Message body defined in event
#  - from           Bugzilla system email address
#  - queries        array of hashes containing:
#          - bugs:  array of hashes mapping fieldnames to values for this bug
#          - title: text title given to this query in the whine event
#  - schedule_id    integer id of the schedule being run
#  - subject        Subject line for the message
#  - recipient      user object for the recipient
#  - author         user object of the person who created the whine event
#
# In addition, mail adds two more fields to $args:
#  - alternatives   array of hashes defining mime multipart types and contents
#  - boundary       a MIME boundary generated using the process id and time
#
sub mail {
    my $args = shift;
    my $addressee = $args->{recipient};
    # Don't send mail to someone whose bugmail notification is disabled.
    return if $addressee->email_disabled;

    my $template = Bugzilla->template_inner($addressee->settings->{'lang'}->{'value'});
    my $msg = ''; # it's a temporary variable to hold the template output
    $args->{'alternatives'} ||= [];

    # put together the different multipart mime segments

    $template->process("whine/mail.txt.tmpl", $args, \$msg)
        or die($template->error());
    push @{$args->{'alternatives'}},
        {
            'content' => $msg,
            'type'    => 'text/plain',
        };
    $msg = '';

    $template->process("whine/mail.html.tmpl", $args, \$msg)
        or die($template->error());
    push @{$args->{'alternatives'}},
        {
            'content' => $msg,
            'type'    => 'text/html',
        };
    $msg = '';

    # now produce a ready-to-mail mime-encoded message

    $args->{'boundary'} = "----------" . $$ . "--" . time() . "-----";

    $template->process("whine/multipart-mime.txt.tmpl", $args, \$msg)
        or die($template->error());

    Bugzilla->template_inner("");
    MessageToMTA($msg);

    delete $args->{'boundary'};
    delete $args->{'alternatives'};

}

# run_queries runs all of the queries associated with a schedule ID, adding
# the results to $args or mailing off the template if a query wants individual
# messages for each bug
sub run_queries {
    my $args = shift;

    my $return_queries = [];

    $sth_run_queries->execute($args->{'eventid'});
    my @queries = ();
    for (@{$sth_run_queries->fetchall_arrayref}) {
        push(@queries,
            {
              'name'          => $_->[0],
              'title'         => $_->[1],
              'onemailperbug' => $_->[2],
              'bugs'          => [],
            }
        );
    }

    foreach my $thisquery (@queries) {
        next unless $thisquery->{'name'};   # named query is blank

        my $savedquery = get_query($thisquery->{'name'}, $args->{'author'});
        next unless $savedquery;    # silently ignore missing queries

        # Execute the saved query
        my @searchfields = (
            'bugs.bug_id',
            'bugs.bug_severity',
            'bugs.priority',
            'bugs.rep_platform',
            'bugs.assigned_to',
            'bugs.bug_status',
            'bugs.resolution',
            'bugs.short_desc',
            'map_assigned_to.login_name',
        );
        # A new Bugzilla::CGI object needs to be created to allow
        # Bugzilla::Search to execute a saved query.  It's exceedingly weird,
        # but that's how it works.
        my $searchparams = new Bugzilla::CGI($savedquery);
        my $search = new Bugzilla::Search(
            'fields' => \@searchfields,
            'params' => $searchparams,
            'user'   => $args->{'recipient'}, # the search runs as the recipient
        );
        my $sqlquery = $search->getSQL();
        $sth = $dbh->prepare($sqlquery);
        $sth->execute;

        while (my @row = $sth->fetchrow_array) {
            my $bug = {};
            for my $field (@searchfields) {
                my $fieldname = $field;
                $fieldname =~ s/^bugs\.//;  # No need for bugs.whatever
                $bug->{$fieldname} = shift @row;
            }

            if ($thisquery->{'onemailperbug'}) {
                $args->{'queries'} = [
                    {
                        'name' => $thisquery->{'name'},
                        'title' => $thisquery->{'title'},
                        'bugs' => [ $bug ],
                    },
                ];
                mail($args);
                delete $args->{'queries'};
            }
            else {  # It belongs in one message with any other lists
                push @{$thisquery->{'bugs'}}, $bug;
            }
        }
        unless ($thisquery->{'onemailperbug'}) {
            push @{$return_queries}, $thisquery;
        }
    }

    return $return_queries;
}

# get_query gets the namedquery.  It's similar to LookupNamedQuery (in
# buglist.cgi), but doesn't care if a query name really exists or not, since
# individual named queries might go away without the whine_queries that point
# to them being removed.
sub get_query {
    my ($name, $user) = @_;
    my $qname = $name;
    $sth_get_query->execute($user->{'id'}, $qname);
    my $fetched = $sth_get_query->fetch;
    $sth_get_query->finish;
    return $fetched ? $fetched->[0] : '';
}

# check_today gets a run day from the schedule and sees if it matches today
# a run day value can contain any of:
#   - a three-letter day of the week
#   - a number for a day of the month
#   - 'last' for the last day of the month
#   - 'All' for every day
#   - 'MF' for every weekday

sub check_today {
    my $run_day  = shift;

    if (($run_day eq 'MF')
     && ($now_weekday > 0)
     && ($now_weekday < 6)) {
        return 1;
    }
    elsif (
         length($run_day) == 3 &&
         index("SunMonTueWedThuFriSat", $run_day)/3 == $now_weekday) {
        return 1;
    }
    elsif  (($run_day eq 'All')
         || (($run_day eq 'last')  &&
             ($now_day == $daysinmonth[$now_month] ))
         || ($run_day eq $now_day)) {
        return 1;
    }
    return 0;
}

# reset_timer sets the next time a whine is supposed to run, assuming it just
# ran moments ago.  Its only parameter is a schedule ID.
#
# reset_timer does not lock the whine_schedules table.  Anything that calls it
# should do that itself.
sub reset_timer {
    my $schedule_id = shift;

    # Schedules may not be executed more than once for each invocation of
    # whine.pl -- there are legitimate circumstances that can cause this, like
    # a set of whines that take a very long time to execute, so it's done
    # quietly.
    if (grep($_ == $schedule_id, @seen_schedules)) {
        null_schedule($schedule_id);
        return;
    }
    push @seen_schedules, $schedule_id;

    $sth = $dbh->prepare( "SELECT run_day, run_time FROM whine_schedules " .
                          "WHERE id=?" );
    $sth->execute($schedule_id);
    my ($run_day, $run_time) = $sth->fetchrow_array;

    # It may happen that the run_time field is NULL or blank due to
    # a bug in editwhines.cgi when this field was initially 0.
    $run_time ||= 0;

    my $run_today = 0;
    my $minute_offset = 0;

    # If the schedule is to run today, and it runs many times per day,
    # it shall be set to run immediately.
    $run_today = &check_today($run_day);
    if (($run_today) && ($run_time !~ /^\d+$/)) {
        # The default of 60 catches any bad value
        my $minute_interval = 60;
        if ($run_time =~ /^(\d+)min$/i) {
            $minute_interval = $1;
        }

        # set the minute offset to the next interval point
        $minute_offset = $minute_interval - ($now_minute % $minute_interval);
    }
    elsif (($run_today) && ($run_time > $now_hour)) {
        # timed event for later today
        # (This should only happen if, for example, an 11pm scheduled event
        #  didn't happen until after midnight)
        $minute_offset = (60 * ($run_time - $now_hour)) - $now_minute;
    }
    else {
        # it's not something that runs later today.
        $minute_offset = 0;

        # Set the target time if it's a specific hour
        my $target_time = ($run_time =~ /^\d+$/) ? $run_time : 0;

        my $nextdate = &get_next_date($run_day);

        $sth = $dbh->prepare("UPDATE whine_schedules " .
                             "SET run_next = (CURRENT_DATE + " .
                             $dbh->sql_interval('?', 'DAY') . ") + " .
                             $dbh->sql_interval('?', 'HOUR') .
                             " WHERE id = ?");
        $sth->execute($nextdate, $target_time, $schedule_id);
        return;
    }

    if ($minute_offset > 0) {
        # Scheduling is done in terms of whole minutes.
        my $next_run = $dbh->selectrow_array('SELECT NOW() + ' .
                                             $dbh->sql_interval('?', 'MINUTE'),
                                             undef, $minute_offset);
        $next_run = format_time($next_run, "%Y-%m-%d %R");

        $sth = $dbh->prepare("UPDATE whine_schedules " .
                             "SET run_next = ? WHERE id = ?");
        $sth->execute($next_run, $schedule_id);
    } else {
        # The minute offset is zero or less, which is not supposed to happen.
        # complain to STDERR
        null_schedule($schedule_id);
        print STDERR "Error: bad minute_offset for schedule ID $schedule_id\n";
    }
}

# null_schedule is used to safeguard against infinite loops.  Schedules with
# run_next set to NULL will not be available to get_next_event until they are
# rescheduled, which only happens when whine.pl starts.
sub null_schedule {
    my $schedule_id = shift;
    $sth = $dbh->prepare("UPDATE whine_schedules " .
                         "SET run_next = NULL " .
                         "WHERE id=?");
    $sth->execute($schedule_id);
}

# get_next_date determines the difference in days between now and the next
# time a schedule should run, excluding today
#
# It takes a run_day argument (see check_today, above, for an explanation),
# and returns an integer, representing a number of days.
sub get_next_date {
    my $day = shift;

    my $add_days = 0;

    if ($day eq 'All') {
        $add_days = 1;
    }
    elsif ($day eq 'last') {
        # next_date should contain the last day of this month, or next month
        # if it's today
        if ($daysinmonth[$now_month] == $now_day) {
            my $month = $now_month + 1;
            $month = 1 if $month > 12;
            $add_days = $daysinmonth[$month] + 1;
        }
        else {
            $add_days = $daysinmonth[$now_month] - $now_day;
        }
    }
    elsif ($day eq 'MF') { # any day Monday through Friday
        if ($now_weekday < 5) { # Sun-Thurs
            $add_days = 1;
        }
        elsif ($now_weekday == 5) { # Friday
            $add_days = 3;
        }
        else { # it's 6, Saturday
            $add_days = 2;
        }
    }
    elsif ($day !~ /^\d+$/) { # A specific day of the week
        # The default is used if there is a bad value in the database, in
        # which case we mark it to a less-popular day (Sunday)
        my $day_num = 0;

        if (length($day) == 3) {
            $day_num = (index("SunMonTueWedThuFriSat", $day)/3) or 0;
        }

        $add_days = $day_num - $now_weekday;
        if ($add_days <= 0) { # it's next week
            $add_days += 7;
        }
    }
    else { # it's a number, so we set it for that calendar day
        $add_days = $day - $now_day;
        # If it's already beyond that day this month, set it to the next one
        if ($add_days <= 0) {
            $add_days += $daysinmonth[$now_month];
        }
    }
    return $add_days;
}
