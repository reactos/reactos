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
# Contributor(s): Christian Reis <kiko@async.com.br>
#                 Shane H. W. Travis <travis@sedsystems.ca>
#
use strict;

use lib qw(.);

use Date::Parse;         # strptime
use Date::Format;        # strftime

use Bugzilla;
use Bugzilla::Constants; # LOGIN_*
use Bugzilla::Bug;       # EmitDependList
use Bugzilla::Util;      # trim
use Bugzilla::Error;
use Bugzilla::User;      # Bugzilla->user->in_group

my $template = Bugzilla->template;
my $vars = {};

#
# Date handling
#

sub date_adjust_down {
   
    my ($year, $month, $day) = @_;

    if ($day == 0) {
        $month -= 1;
        $day = 31;
        # Proper day adjustment is done later.

        if ($month == 0) {
            $year -= 1;
            $month = 12;
        }
    }

    if (($month == 2) && ($day > 28)) {
        if ($year % 4 == 0 && $year % 100 != 0) {
            $day = 29;
        } else {
            $day = 28;
        }
    }

    if (($month == 4 || $month == 6 || $month == 9 || $month == 11) &&
        ($day == 31) ) 
    {
        $day = 30;
    }
    return ($year, $month, $day);
}

sub date_adjust_up {
    my ($year, $month, $day) = @_;

    if ($day > 31) {
        $month += 1;
        $day    = 1;

        if ($month == 13) {
            $month = 1;
            $year += 1;
        }
    }

    if ($month == 2 && $day > 28) {
        if ($year % 4 != 0 || $year % 100 == 0 || $day > 29) {
            $month = 3;
            $day = 1;
        }
    }

    if (($month == 4 || $month == 6 || $month == 9 || $month == 11) &&
        ($day == 31) )
    {
        $month += 1; 
        $day    = 1;
    }

    return ($year, $month, $day);
}

sub check_dates {
    my ($start_date, $end_date) = @_;
    if ($start_date) {
        if (!str2time($start_date)) {
            ThrowUserError("illegal_date", {'date' => $start_date});
        }
        # This code may strike you as funny. It's actually a workaround
        # for an "issue" in str2time. If you enter the date 2004-06-31,
        # even though it's a bogus date (there *are* only 30 days in
        # June), it will parse and return 2004-07-01. To make this
        # less painful to the end-user, I do the "normalization" here,
        # but it might be "surprising" and warrant a warning in the end.
        $start_date = time2str("%Y-%m-%d", str2time($start_date));
    } 
    if ($end_date) {
        if (!str2time($end_date)) {
            ThrowUserError("illegal_date", {'date' => $end_date});
        }
        # see related comment above.
        $end_date = time2str("%Y-%m-%d", str2time($end_date));
    }
    return ($start_date, $end_date);
}

sub split_by_month {
    # Takes start and end dates and splits them into a list of
    # monthly-spaced 2-lists of dates.
    my ($start_date, $end_date) = @_;

    # We assume at this point that the dates are provided and sane
    my (undef, undef, undef, $sd, $sm, $sy, undef) = strptime($start_date);
    my (undef, undef, undef, $ed, $em, $ey, undef) = strptime($end_date);

    # Find out how many months fit between the two dates so we know
    # how many times we loop.
    my $yd = $ey - $sy;
    my $md = 12 * $yd + $em - $sm;
    # If the end day is smaller than the start day, last interval is not a whole month.
    if ($sd > $ed) {
        $md -= 1;
    }

    my (@months, $sub_start, $sub_end);
    # This +1 and +1900 are a result of strptime's bizarre semantics
    my $year = $sy + 1900;
    my $month = $sm + 1;

    # Keep the original date, when the date will be changed in the adjust_date.
    my $sd_tmp = $sd;
    my $month_tmp = $month;
    my $year_tmp = $year;

    # This section handles only the whole months.
    for (my $i=0; $i < $md; $i++) {
        # Start of interval is adjusted up: 31.2. -> 1.3.
        ($year_tmp, $month_tmp, $sd_tmp) = date_adjust_up($year, $month, $sd);
        $sub_start = sprintf("%04d-%02d-%02d", $year_tmp, $month_tmp, $sd_tmp); 
        $month += 1;
        if ($month == 13) {
            $month = 1;
            $year += 1;
        }
        # End of interval is adjusted down: 31.2 -> 28.2.
        ($year_tmp, $month_tmp, $sd_tmp) = date_adjust_down($year, $month, $sd - 1);
        $sub_end = sprintf("%04d-%02d-%02d", $year_tmp, $month_tmp, $sd_tmp);
        push @months, [$sub_start, $sub_end];
    }
    
    # This section handles the last (unfinished) month. 
    $sub_end = sprintf("%04d-%02d-%02d", $ey + 1900, $em + 1, $ed);
    ($year_tmp, $month_tmp, $sd_tmp) = date_adjust_up($year, $month, $sd);
    $sub_start = sprintf("%04d-%02d-%02d", $year_tmp, $month_tmp, $sd_tmp);
    push @months, [$sub_start, $sub_end];

    return @months;
}

sub include_tt_details {
    my ($res, $bugids, $start_date, $end_date) = @_;


    my $dbh = Bugzilla->dbh;
    my ($date_bits, $date_values) = sqlize_dates($start_date, $end_date);
    my $buglist = join ", ", @{$bugids};

    my $q = qq{SELECT bugs.bug_id, profiles.login_name, bugs.deadline,
                      bugs.estimated_time, bugs.remaining_time
               FROM   longdescs
               INNER JOIN bugs
                  ON longdescs.bug_id = bugs.bug_id
               INNER JOIN profiles
                  ON longdescs.who = profiles.userid
               WHERE  longdescs.bug_id in ($buglist) $date_bits};

    my %res = %{$res};
    my $sth = $dbh->prepare($q);
    $sth->execute(@{$date_values});
    while (my $row = $sth->fetch) {
        $res{$row->[0]}{"deadline"} = $row->[2];
        $res{$row->[0]}{"estimated_time"} = $row->[3];
        $res{$row->[0]}{"remaining_time"} = $row->[4];
    }
    return \%res;
}

sub sqlize_dates {
    my ($start_date, $end_date) = @_;
    my $date_bits = "";
    my @date_values;
    if ($start_date) {
        # we've checked, trick_taint is fine
        trick_taint($start_date);
        $date_bits = " AND longdescs.bug_when > ?";
        push @date_values, $start_date;
    } 
    if ($end_date) {
        # we need to add one day to end_date to catch stuff done today
        # do not forget to adjust date if it was the last day of month
        my (undef, undef, undef, $ed, $em, $ey, undef) = strptime($end_date);
        ($ey, $em, $ed) = date_adjust_up($ey+1900, $em+1, $ed+1);
        $end_date = sprintf("%04d-%02d-%02d", $ey, $em, $ed);

        $date_bits .= " AND longdescs.bug_when < ?"; 
        push @date_values, $end_date;
    }
    return ($date_bits, \@date_values);
}

#
# Dependencies
#

sub get_blocker_ids_unique {
    my $bug_id = shift;
    my @ret = ($bug_id);
    get_blocker_ids_deep($bug_id, \@ret);
    my %unique;
    foreach my $blocker (@ret) {
        $unique{$blocker} = $blocker
    }
    return keys %unique;
}

sub get_blocker_ids_deep {
    my ($bug_id, $ret) = @_;
    my $deps = Bugzilla::Bug::EmitDependList("blocked", "dependson", $bug_id);
    push @{$ret}, @$deps;
    foreach $bug_id (@$deps) {
        get_blocker_ids_deep($bug_id, $ret);
    }
}

#
# Queries and data structure assembly
#

sub query_work_by_buglist {
    my ($bugids, $start_date, $end_date) = @_;
    my $dbh = Bugzilla->dbh;

    my ($date_bits, $date_values) = sqlize_dates($start_date, $end_date);

    # $bugids is guaranteed to be non-empty because at least one bug is
    # always provided to this page.
    my $buglist = join ", ", @{$bugids};

    # Returns the total time worked on each bug *per developer*, with
    # bug descriptions and developer address
    my $q = qq{SELECT sum(longdescs.work_time) as total_time,
                      profiles.login_name, 
                      longdescs.bug_id,
                      bugs.short_desc,
                      bugs.bug_status
               FROM   longdescs
               INNER JOIN profiles
                   ON longdescs.who = profiles.userid
               INNER JOIN bugs
                   ON bugs.bug_id = longdescs.bug_id
               WHERE  longdescs.bug_id IN ($buglist)
                      $date_bits } .
            $dbh->sql_group_by('longdescs.bug_id, profiles.login_name',
                'bugs.short_desc, bugs.bug_status, longdescs.bug_when') . qq{
               ORDER BY longdescs.bug_when};
    my $sth = $dbh->prepare($q);
    $sth->execute(@{$date_values});
    return $sth;
}

sub get_work_by_owners {
    my $sth = query_work_by_buglist(@_);
    my %res;
    while (my $row = $sth->fetch) {
        # XXX: Why do we need to check if the total time is positive
        # instead of using SQL to do that?  Simply because MySQL 3.x's
        # GROUP BY doesn't work correctly with aggregates. This is
        # really annoying, but I've spent a long time trying to wrestle
        # with it and it just doesn't seem to work. Should work OK in
        # 4.x, though.
        if ($row->[0] > 0) {
            my $login_name = $row->[1];
            push @{$res{$login_name}}, { total_time => $row->[0],
                                         bug_id     => $row->[2],
                                         short_desc => $row->[3],
                                         bug_status => $row->[4] };
        }
    }
    return \%res;
}

sub get_work_by_bugs {
    my $sth = query_work_by_buglist(@_);
    my %res;
    while (my $row = $sth->fetch) {
        # Perl doesn't let me use arrays as keys :-(
        # merge in ID, status and summary
        my $bug = join ";", ($row->[2], $row->[4], $row->[3]);
        # XXX: see comment in get_work_by_owners
        if ($row->[0] > 0) {
            push @{$res{$bug}}, { total_time => $row->[0],
                                  login_name => $row->[1], };
        }
    }
    return \%res;
}

sub get_inactive_bugs {
    my ($bugids, $start_date, $end_date) = @_;
    my $dbh = Bugzilla->dbh;
    my ($date_bits, $date_values) = sqlize_dates($start_date, $end_date);
    my $buglist = join ", ", @{$bugids};

    my %res;
    # This sucks. I need to make sure that even bugs that *don't* show
    # up in the longdescs query (because no comments were filed during
    # the specified period) but *are* dependent on the parent bug show
    # up in the results if they have no work done; that's why I prefill
    # them in %res here and then remove them below.
    my $q = qq{SELECT DISTINCT bugs.bug_id, bugs.short_desc ,
                               bugs.bug_status
               FROM   longdescs
               INNER JOIN bugs
                    ON longdescs.bug_id = bugs.bug_id
               WHERE  longdescs.bug_id in ($buglist)};
    my $sth = $dbh->prepare($q);
    $sth->execute();
    while (my $row = $sth->fetch) {
        $res{$row->[0]} = [$row->[1], $row->[2]];
    }

    # Returns the total time worked on each bug, with description. This
    # query differs a bit from one in the query_work_by_buglist and I
    # avoided complicating that one just to make it more general.
    $q = qq{SELECT sum(longdescs.work_time) as total_time,
                   longdescs.bug_id,
                   bugs.short_desc,
                   bugs.bug_status
            FROM   longdescs
            INNER JOIN bugs
                ON bugs.bug_id = longdescs.bug_id 
            WHERE  longdescs.bug_id IN ($buglist)
                   $date_bits } .
         $dbh->sql_group_by('longdescs.bug_id',
                            'bugs.short_desc, bugs.bug_status,
                             longdescs.bug_when') . qq{
            ORDER BY longdescs.bug_when};
    $sth = $dbh->prepare($q);
    $sth->execute(@{$date_values});
    while (my $row = $sth->fetch) {
        # XXX: see comment in get_work_by_owners
        if ($row->[0] == 0) {
            $res{$row->[1]} = [$row->[2], $row->[3]];
        } else {
            delete $res{$row->[1]};
        }
    }
    return \%res;
}

#
# Misc
#

sub sort_bug_keys {
    # XXX a hack is the mother of all evils. The fact that we store keys
    # joined by semi-colons in the workdata-by-bug structure forces us to
    # write this evil comparison function to ensure we can process the
    # data timely -- just pushing it through a numerical sort makes TT
    # hang while generating output :-(
    my $list = shift;
    my @a;
    my @b;
    return sort { @a = split(";", $a); 
                  @b = split(";", $b); 
                  $a[0] <=> $b[0] } @{$list};
}

#
# Template code starts here
#

Bugzilla->login(LOGIN_REQUIRED);

my $cgi = Bugzilla->cgi;

Bugzilla->switch_to_shadow_db();

Bugzilla->user->in_group(Bugzilla->params->{"timetrackinggroup"})
    || ThrowUserError("auth_failure", {group  => "time-tracking",
                                       action => "access",
                                       object => "timetracking_summaries"});

my @ids = split(",", $cgi->param('id'));
map { ValidateBugID($_) } @ids;
@ids = map { detaint_natural($_) && $_ } @ids;
@ids = grep { Bugzilla->user->can_see_bug($_) } @ids;

my $group_by = $cgi->param('group_by') || "number";
my $monthly = $cgi->param('monthly');
my $detailed = $cgi->param('detailed');
my $do_report = $cgi->param('do_report');
my $inactive = $cgi->param('inactive');
my $do_depends = $cgi->param('do_depends');
my $ctype = scalar($cgi->param("ctype"));

my ($start_date, $end_date);
if ($do_report && @ids) {
    my @bugs = @ids;

    # Dependency mode requires a single bug and grabs dependents.
    if ($do_depends) {
        if (scalar(@bugs) != 1) {
            ThrowCodeError("bad_arg", { argument=>"id",
                                        function=>"summarize_time"});
        }
        @bugs = get_blocker_ids_unique($bugs[0]);
        @bugs = grep { Bugzilla->user->can_see_bug($_) } @bugs;
    }

    $start_date = trim $cgi->param('start_date');
    $end_date = trim $cgi->param('end_date');

    # Swap dates in case the user put an end_date before the start_date
    if ($start_date && $end_date && 
        str2time($start_date) > str2time($end_date)) {
        $vars->{'warn_swap_dates'} = 1;
        ($start_date, $end_date) = ($end_date, $start_date);
    }
    ($start_date, $end_date) = check_dates($start_date, $end_date);

    if ($detailed) {
        my %detail_data;
        my $res = include_tt_details(\%detail_data, \@bugs, $start_date, $end_date);

        $vars->{'detail_data'} = $res;
    }
  
    # Store dates ia session cookie the dates so re-visiting the page
    # for other bugs keeps them around.
    $cgi->send_cookie(-name => 'time-summary-dates',
                      -value => join ";", ($start_date, $end_date));

    my (@parts, $part_data, @part_list);

    # Break dates apart into months if necessary; if not, we use the
    # same @parts list to allow us to use a common codepath.
    if ($monthly) {
        # unfortunately it's not too easy to guess a start date, since
        # it depends on what bugs we're looking at. We risk bothering
        # the user here. XXX: perhaps run a query to see what the
        # earliest activity in longdescs for all bugs and use that as a
        # start date.
        $start_date || ThrowUserError("illegal_date", {'date' => $start_date});
        # we can, however, provide a default end date. Note that this
        # differs in semantics from the open-ended queries we use when
        # start/end_date aren't provided -- and clock skews will make
        # this evident!
        @parts = split_by_month($start_date, 
                                $end_date || time2str("%Y-%m-%d", time()));
    } else {
        @parts = ([$start_date, $end_date]);
    }

    my %empty_hash;
    # For each of the separate divisions, grab the relevant summaries 
    foreach my $part (@parts) {
        my ($sub_start, $sub_end) = @{$part};
        if (@bugs) {
            if ($group_by eq "owner") {
                $part_data = get_work_by_owners(\@bugs, $sub_start, $sub_end);
            } else {
                $part_data = get_work_by_bugs(\@bugs, $sub_start, $sub_end);
            }
        } else {
            # $part_data must be a reference to a hash
            $part_data = \%empty_hash; 
        }
        push @part_list, $part_data;
    }

    if ($inactive && @bugs) {
        $vars->{'null'} = get_inactive_bugs(\@bugs, $start_date, $end_date);
    } else {
        $vars->{'null'} = \%empty_hash;
    }

    $vars->{'part_list'} = \@part_list;
    $vars->{'parts'} = \@parts;

} elsif ($cgi->cookie("time-summary-dates")) {
    ($start_date, $end_date) = split ";", $cgi->cookie('time-summary-dates');
}

$vars->{'ids'} = \@ids;
$vars->{'start_date'} = $start_date;
$vars->{'end_date'} = $end_date;
$vars->{'group_by'} = $group_by;
$vars->{'monthly'} = $monthly;
$vars->{'detailed'} = $detailed;
$vars->{'inactive'} = $inactive;
$vars->{'do_report'} = $do_report;
$vars->{'do_depends'} = $do_depends;
$vars->{'check_time'} = \&check_time;
$vars->{'sort_bug_keys'} = \&sort_bug_keys;

my $format = $template->get_format("bug/summarize-time", undef, $ctype);

# Get the proper content-type
print $cgi->header(-type=> $format->{'ctype'});
$template->process("$format->{'template'}", $vars)
  || ThrowTemplateError($template->error());
