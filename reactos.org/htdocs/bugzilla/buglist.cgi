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
#                 Dan Mosedale <dmose@mozilla.org>
#                 Stephan Niemz  <st.n@gmx.net>
#                 Andreas Franke <afranke@mathweb.org>
#                 Myk Melez <myk@mozilla.org>
#                 Max Kanat-Alexander <mkanat@bugzilla.org>

################################################################################
# Script Initialization
################################################################################

# Make it harder for us to do dangerous things in Perl.
use strict;

use lib qw(.);

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Error;
use Bugzilla::Util;
use Bugzilla::Search;
use Bugzilla::Search::Quicksearch;
use Bugzilla::Search::Saved;
use Bugzilla::User;
use Bugzilla::Bug;
use Bugzilla::Product;
use Bugzilla::Keyword;
use Bugzilla::Field;

use Date::Parse;

my $cgi = Bugzilla->cgi;
my $dbh = Bugzilla->dbh;
my $template = Bugzilla->template;
my $vars = {};
my $buffer = $cgi->query_string();

# We have to check the login here to get the correct footer if an error is
# thrown and to prevent a logged out user to use QuickSearch if 'requirelogin'
# is turned 'on'.
Bugzilla->login();

if (length($buffer) == 0) {
    print $cgi->header(-refresh=> '10; URL=query.cgi');
    ThrowUserError("buglist_parameters_required");
}

# Determine whether this is a quicksearch query.
my $searchstring = $cgi->param('quicksearch');
if (defined($searchstring)) {
    $buffer = quicksearch($searchstring);
    # Quicksearch may do a redirect, in which case it does not return.
    # If it does return, it has modified $cgi->params so we can use them here
    # as if this had been a normal query from the beginning.
}

# If configured to not allow empty words, reject empty searches from the
# Find a Specific Bug search form, including words being a single or 
# several consecutive whitespaces only.
if (!Bugzilla->params->{'specific_search_allow_empty_words'}
    && defined($cgi->param('content')) && $cgi->param('content') =~ /^\s*$/)
{
    ThrowUserError("buglist_parameters_required");
}

################################################################################
# Data and Security Validation
################################################################################

# Whether or not the user wants to change multiple bugs.
my $dotweak = $cgi->param('tweak') ? 1 : 0;

# Log the user in
if ($dotweak) {
    Bugzilla->login(LOGIN_REQUIRED);
    Bugzilla->user->in_group("editbugs")
      || ThrowUserError("auth_failure", {group  => "editbugs",
                                         action => "modify",
                                         object => "multiple_bugs"});
}

# Hack to support legacy applications that think the RDF ctype is at format=rdf.
if (defined $cgi->param('format') && $cgi->param('format') eq "rdf"
    && !defined $cgi->param('ctype')) {
    $cgi->param('ctype', "rdf");
    $cgi->delete('format');
}

# Treat requests for ctype=rss as requests for ctype=atom
if (defined $cgi->param('ctype') && $cgi->param('ctype') eq "rss") {
    $cgi->param('ctype', "atom");
}

# The js ctype presents a security risk; a malicious site could use it  
# to gather information about secure bugs. So, we only allow public bugs to be
# retrieved with this format.
#
# Note that if and when this call clears cookies or has other persistent 
# effects, we'll need to do this another way instead.
if ((defined $cgi->param('ctype')) && ($cgi->param('ctype') eq "js")) {
    Bugzilla->logout_request();
}

# An agent is a program that automatically downloads and extracts data
# on its user's behalf.  If this request comes from an agent, we turn off
# various aspects of bug list functionality so agent requests succeed
# and coexist nicely with regular user requests.  Currently the only agent
# we know about is Firefox's microsummary feature.
my $agent = ($cgi->http('X-Moz') && $cgi->http('X-Moz') =~ /\bmicrosummary\b/);

# Determine the format in which the user would like to receive the output.
# Uses the default format if the user did not specify an output format;
# otherwise validates the user's choice against the list of available formats.
my $format = $template->get_format("list/list", scalar $cgi->param('format'),
                                   scalar $cgi->param('ctype'));

# Use server push to display a "Please wait..." message for the user while
# executing their query if their browser supports it and they are viewing
# the bug list as HTML and they have not disabled it by adding &serverpush=0
# to the URL.
#
# Server push is a Netscape 3+ hack incompatible with MSIE, Lynx, and others. 
# Even Communicator 4.51 has bugs with it, especially during page reload.
# http://www.browsercaps.org used as source of compatible browsers.
# Safari (WebKit) does not support it, despite a UA that says otherwise (bug 188712)
# MSIE 5+ supports it on Mac (but not on Windows) (bug 190370)
#
my $serverpush =
  $format->{'extension'} eq "html"
    && exists $ENV{'HTTP_USER_AGENT'} 
      && $ENV{'HTTP_USER_AGENT'} =~ /Mozilla.[3-9]/ 
        && (($ENV{'HTTP_USER_AGENT'} !~ /[Cc]ompatible/) || ($ENV{'HTTP_USER_AGENT'} =~ /MSIE 5.*Mac_PowerPC/))
          && $ENV{'HTTP_USER_AGENT'} !~ /WebKit/
            && !$agent
              && !defined($cgi->param('serverpush'))
                || $cgi->param('serverpush');

my $order = $cgi->param('order') || "";
my $order_from_cookie = 0;  # True if $order set using the LASTORDER cookie

# The params object to use for the actual query itself
my $params;

# If the user is retrieving the last bug list they looked at, hack the buffer
# storing the query string so that it looks like a query retrieving those bugs.
if (defined $cgi->param('regetlastlist')) {
    $cgi->cookie('BUGLIST') || ThrowUserError("missing_cookie");

    $order = "reuse last sort" unless $order;
    my $bug_id = $cgi->cookie('BUGLIST');
    $bug_id =~ s/:/,/g;
    # set up the params for this new query
    $params = new Bugzilla::CGI({
                                 bug_id => $bug_id,
                                 order => $order,
                                });
}

if ($buffer =~ /&cmd-/) {
    my $url = "query.cgi?$buffer#chart";
    print $cgi->redirect(-location => $url);
    # Generate and return the UI (HTML page) from the appropriate template.
    $vars->{'message'} = "buglist_adding_field";
    $vars->{'url'} = $url;
    $template->process("global/message.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
    exit;
}

# Figure out whether or not the user is doing a fulltext search.  If not,
# we'll remove the relevance column from the lists of columns to display
# and order by, since relevance only exists when doing a fulltext search.
my $fulltext = 0;
if ($cgi->param('content')) { $fulltext = 1 }
my @charts = map(/^field(\d-\d-\d)$/ ? $1 : (), $cgi->param());
foreach my $chart (@charts) {
    if ($cgi->param("field$chart") eq 'content' && $cgi->param("value$chart")) {
        $fulltext = 1;
        last;
    }
}

################################################################################
# Utilities
################################################################################

local our @weekday= qw( Sun Mon Tue Wed Thu Fri Sat );
sub DiffDate {
    my ($datestr) = @_;
    my $date = str2time($datestr);
    my $age = time() - $date;
    my ($s,$m,$h,$d,$mo,$y,$wd)= localtime $date;
    if( $age < 18*60*60 ) {
        $date = sprintf "%02d:%02d:%02d", $h,$m,$s;
    } elsif( $age < 6*24*60*60 ) {
        $date = sprintf "%s %02d:%02d", $weekday[$wd],$h,$m;
    } else {
        $date = sprintf "%04d-%02d-%02d", 1900+$y,$mo+1,$d;
    }
    return $date;
}

sub LookupNamedQuery {
    my ($name, $sharer_id, $query_type, $throw_error) = @_;
    my $user = Bugzilla->login(LOGIN_REQUIRED);
    my $dbh = Bugzilla->dbh;
    my $owner_id;
    $throw_error = 1 unless defined $throw_error;

    # $name and $sharer_id are safe -- we only use them below in SELECT
    # placeholders and then in error messages (which are always HTML-filtered).
    $name || ThrowUserError("query_name_missing");
    trick_taint($name);
    if ($sharer_id) {
        $owner_id = $sharer_id;
        detaint_natural($owner_id);
        $owner_id || ThrowUserError('illegal_user_id', {'userid' => $sharer_id});
    }
    else {
        $owner_id = $user->id;
    }

    my @args = ($owner_id, $name);
    my $extra = '';
    # If $query_type is defined, then we restrict our search.
    if (defined $query_type) {
        $extra = ' AND query_type = ? ';
        detaint_natural($query_type);
        push(@args, $query_type);
    }
    my ($id, $result) = $dbh->selectrow_array("SELECT id, query
                                                 FROM namedqueries
                                                WHERE userid = ? AND name = ?
                                                      $extra",
                                               undef, @args);
    if (!defined($result)) {
        return 0 unless $throw_error;
        ThrowUserError("missing_query", {'queryname' => $name,
                                         'sharer_id' => $sharer_id});
    }

    if ($sharer_id) {
        my $group = $dbh->selectrow_array('SELECT group_id
                                             FROM namedquery_group_map
                                            WHERE namedquery_id = ?',
                                          undef, $id);
        if (!grep {$_ == $group} values(%{$user->groups()})) {
            ThrowUserError("missing_query", {'queryname' => $name,
                                             'sharer_id' => $sharer_id});
        }
    }
    
    $result
       || ThrowUserError("buglist_parameters_required", {'queryname' => $name});

    return $result;
}

# Inserts a Named Query (a "Saved Search") into the database, or
# updates a Named Query that already exists..
# Takes four arguments:
# userid - The userid who the Named Query will belong to.
# query_name - A string that names the new Named Query, or the name
#              of an old Named Query to update. If this is blank, we
#              will throw a UserError. Leading and trailing whitespace
#              will be stripped from this value before it is inserted
#              into the DB.
# query - The query part of the buglist.cgi URL, unencoded. Must not be 
#         empty, or we will throw a UserError.
# link_in_footer (optional) - 1 if the Named Query should be 
# displayed in the user's footer, 0 otherwise.
# query_type (optional) - 1 if the Named Query contains a list of
# bug IDs only, 0 otherwise (default).
#
# All parameters are validated before passing them into the database.
#
# Returns: A boolean true value if the query existed in the database 
# before, and we updated it. A boolean false value otherwise.
sub InsertNamedQuery {
    my ($query_name, $query, $link_in_footer, $query_type) = @_;
    my $dbh = Bugzilla->dbh;

    $query_name = trim($query_name);
    my ($query_obj) = grep {$_->name eq $query_name} @{Bugzilla->user->queries};

    if ($query_obj) {
        $query_obj->set_url($query);
        $query_obj->set_query_type($query_type);
        $query_obj->update();
    } else {
        Bugzilla::Search::Saved->create({
            name           => $query_name,
            query          => $query,
            query_type     => $query_type,
            link_in_footer => $link_in_footer
        });
    }

    return $query_obj ? 1 : 0;
}

sub LookupSeries {
    my ($series_id) = @_;
    detaint_natural($series_id) || ThrowCodeError("invalid_series_id");
    
    my $dbh = Bugzilla->dbh;
    my $result = $dbh->selectrow_array("SELECT query FROM series " .
                                       "WHERE series_id = ?"
                                       , undef, ($series_id));
    $result
           || ThrowCodeError("invalid_series_id", {'series_id' => $series_id});
    return $result;
}

sub GetQuip {
    my $dbh = Bugzilla->dbh;
    # COUNT is quick because it is cached for MySQL. We may want to revisit
    # this when we support other databases.
    my $count = $dbh->selectrow_array("SELECT COUNT(quip)"
                                    . " FROM quips WHERE approved = 1");
    my $random = int(rand($count));
    my $quip = 
        $dbh->selectrow_array("SELECT quip FROM quips WHERE approved = 1 " . 
                              $dbh->sql_limit(1, $random));
    return $quip;
}

sub GetGroups {
    my $dbh = Bugzilla->dbh;
    my $user = Bugzilla->user;

    # Create an array where each item is a hash. The hash contains 
    # as keys the name of the columns, which point to the value of 
    # the columns for that row.
    my $grouplist = $user->groups_as_string;
    my $groups = $dbh->selectall_arrayref(
                "SELECT  id, name, description, isactive
                   FROM  groups
                  WHERE  id IN ($grouplist)
                    AND  isbuggroup = 1
               ORDER BY  description "
               , {Slice => {}});

    return $groups;
}


################################################################################
# Command Execution
################################################################################

$cgi->param('cmdtype', "") if !defined $cgi->param('cmdtype');
$cgi->param('remaction', "") if !defined $cgi->param('remaction');

# Backwards-compatibility - the old interface had cmdtype="runnamed" to run
# a named command, and we can't break this because it's in bookmarks.
if ($cgi->param('cmdtype') eq "runnamed") {  
    $cgi->param('cmdtype', "dorem");
    $cgi->param('remaction', "run");
}

# Now we're going to be running, so ensure that the params object is set up,
# using ||= so that we only do so if someone hasn't overridden this 
# earlier, for example by setting up a named query search.

# This will be modified, so make a copy.
$params ||= new Bugzilla::CGI($cgi);

# Generate a reasonable filename for the user agent to suggest to the user
# when the user saves the bug list.  Uses the name of the remembered query
# if available.  We have to do this now, even though we return HTTP headers 
# at the end, because the fact that there is a remembered query gets 
# forgotten in the process of retrieving it.
my @time = localtime(time());
my $date = sprintf "%04d-%02d-%02d", 1900+$time[5],$time[4]+1,$time[3];
my $filename = "bugs-$date.$format->{extension}";
if ($cgi->param('cmdtype') eq "dorem" && $cgi->param('remaction') =~ /^run/) {
    $filename = $cgi->param('namedcmd') . "-$date.$format->{extension}";
    # Remove white-space from the filename so the user cannot tamper
    # with the HTTP headers.
    $filename =~ s/\s/_/g;
}

# Take appropriate action based on user's request.
if ($cgi->param('cmdtype') eq "dorem") {  
    if ($cgi->param('remaction') eq "run") {
        $buffer = LookupNamedQuery(scalar $cgi->param("namedcmd"),
                                   scalar $cgi->param('sharer_id'));
        # If this is the user's own query, remember information about it
        # so that it can be modified easily.
        $vars->{'searchname'} = $cgi->param('namedcmd');
        if (!$cgi->param('sharer_id') ||
            $cgi->param('sharer_id') == Bugzilla->user->id) {
            $vars->{'searchtype'} = "saved";
        }
        $params = new Bugzilla::CGI($buffer);
        $order = $params->param('order') || $order;

    }
    elsif ($cgi->param('remaction') eq "runseries") {
        $buffer = LookupSeries(scalar $cgi->param("series_id"));
        $vars->{'searchname'} = $cgi->param('namedcmd');
        $vars->{'searchtype'} = "series";
        $params = new Bugzilla::CGI($buffer);
        $order = $params->param('order') || $order;
    }
    elsif ($cgi->param('remaction') eq "forget") {
        my $user = Bugzilla->login(LOGIN_REQUIRED);
        # Copy the name into a variable, so that we can trick_taint it for
        # the DB. We know it's safe, because we're using placeholders in 
        # the SQL, and the SQL is only a DELETE.
        my $qname = $cgi->param('namedcmd');
        trick_taint($qname);

        # Do not forget the saved search if it is being used in a whine
        my $whines_in_use = 
            $dbh->selectcol_arrayref('SELECT DISTINCT whine_events.subject
                                                 FROM whine_events
                                           INNER JOIN whine_queries
                                                   ON whine_queries.eventid
                                                      = whine_events.id
                                                WHERE whine_events.owner_userid
                                                      = ?
                                                  AND whine_queries.query_name
                                                      = ?
                                      ', undef, $user->id, $qname);
        if (scalar(@$whines_in_use)) {
            ThrowUserError('saved_search_used_by_whines', 
                           { subjects    => join(',', @$whines_in_use),
                             search_name => $qname                      }
            );
        }

        # If we are here, then we can safely remove the saved search
        my ($query_id) = $dbh->selectrow_array('SELECT id FROM namedqueries
                                                    WHERE userid = ?
                                                      AND name   = ?',
                                                  undef, ($user->id, $qname));
        if (!$query_id) {
            # The user has no query of this name. Play along.
        }
        else {
            $dbh->do('DELETE FROM namedqueries
                            WHERE id = ?',
                     undef, $query_id);
            $dbh->do('DELETE FROM namedqueries_link_in_footer
                            WHERE namedquery_id = ?',
                     undef, $query_id);
            $dbh->do('DELETE FROM namedquery_group_map
                            WHERE namedquery_id = ?',
                     undef, $query_id);
        }

        # Now reset the cached queries
        $user->flush_queries_cache();

        print $cgi->header();
        # Generate and return the UI (HTML page) from the appropriate template.
        $vars->{'message'} = "buglist_query_gone";
        $vars->{'namedcmd'} = $qname;
        $vars->{'url'} = "query.cgi";
        $template->process("global/message.html.tmpl", $vars)
          || ThrowTemplateError($template->error());
        exit;
    }
}
elsif (($cgi->param('cmdtype') eq "doit") && defined $cgi->param('remtype')) {
    if ($cgi->param('remtype') eq "asdefault") {
        my $user = Bugzilla->login(LOGIN_REQUIRED);
        InsertNamedQuery(DEFAULT_QUERY_NAME, $buffer);
        $vars->{'message'} = "buglist_new_default_query";
    }
    elsif ($cgi->param('remtype') eq "asnamed") {
        my $user = Bugzilla->login(LOGIN_REQUIRED);
        my $query_name = $cgi->param('newqueryname');
        my $new_query = $cgi->param('newquery');
        my $query_type = QUERY_LIST;
        # If list_of_bugs is true, we are adding/removing individual bugs
        # to a saved search. We get the existing list of bug IDs (if any)
        # and add/remove the passed ones.
        if ($cgi->param('list_of_bugs')) {
            # We add or remove bugs based on the action choosen.
            my $action = trim($cgi->param('action') || '');
            $action =~ /^(add|remove)$/
              || ThrowCodeError('unknown_action', {'action' => $action});

            # If we are removing bugs, then we must have an existing
            # saved search selected.
            if ($action eq 'remove') {
                $query_name && ThrowUserError('no_bugs_to_remove');
            }

            my %bug_ids;
            my $is_new_name = 0;
            if ($query_name) {
                # Make sure this name is not already in use by a normal saved search.
                if (LookupNamedQuery($query_name, undef, QUERY_LIST, !THROW_ERROR)) {
                    ThrowUserError('query_name_exists', {'name' => $query_name});
                }
                $is_new_name = 1;
            }
            # If no new tag name has been given, use the selected one.
            $query_name ||= $cgi->param('oldqueryname');

            # Don't throw an error if it's a new tag name: if the tag already
            # exists, add/remove bugs to it, else create it. But if we are
            # considering an existing tag, then it has to exist and we throw
            # an error if it doesn't (hence the usage of !$is_new_name).
            if (my $old_query = LookupNamedQuery($query_name, undef, LIST_OF_BUGS, !$is_new_name)) {
                # We get the encoded query. We need to decode it.
                my $old_cgi = new Bugzilla::CGI($old_query);
                foreach my $bug_id (split /[\s,]+/, scalar $old_cgi->param('bug_id')) {
                    $bug_ids{$bug_id} = 1 if detaint_natural($bug_id);
                }
            }

            my $keep_bug = ($action eq 'add') ? 1 : 0;
            my $changes = 0;
            foreach my $bug_id (split(/[\s,]+/, $cgi->param('bug_ids'))) {
                next unless $bug_id;
                ValidateBugID($bug_id);
                $bug_ids{$bug_id} = $keep_bug;
                $changes = 1;
            }
            ThrowUserError('no_bug_ids',
                           {'action' => $action,
                            'tag' => $query_name})
              unless $changes;

            # Only keep bug IDs we want to add/keep. Disregard deleted ones.
            my @bug_ids = grep { $bug_ids{$_} == 1 } keys %bug_ids;
            # If the list is now empty, we could as well delete it completely.
            ThrowUserError('no_bugs_in_list', {'tag' => $query_name})
              unless scalar(@bug_ids);

            $new_query = "bug_id=" . join(',', sort {$a <=> $b} @bug_ids);
            $query_type = LIST_OF_BUGS;
        }
        my $tofooter = 1;
        my $existed_before = InsertNamedQuery($query_name, $new_query,
                                              $tofooter, $query_type);
        if ($existed_before) {
            $vars->{'message'} = "buglist_updated_named_query";
        }
        else {
            $vars->{'message'} = "buglist_new_named_query";
        }

        # Make sure to invalidate any cached query data, so that the footer is
        # correctly displayed
        $user->flush_queries_cache();

        $vars->{'queryname'} = $query_name;
        
        print $cgi->header();
        $template->process("global/message.html.tmpl", $vars)
          || ThrowTemplateError($template->error());
        exit;
    }
}

# backward compatibility hack: if the saved query doesn't say which
# form was used to create it, assume it was on the advanced query
# form - see bug 252295
if (!$params->param('query_format')) {
    $params->param('query_format', 'advanced');
    $buffer = $params->query_string;
}

################################################################################
# Column Definition
################################################################################

# Define the columns that can be selected in a query and/or displayed in a bug
# list.  Column records include the following fields:
#
# 1. ID: a unique identifier by which the column is referred in code;
#
# 2. Name: The name of the column in the database (may also be an expression
#          that returns the value of the column);
#
# 3. Title: The title of the column as displayed to users.
# 
# Note: There are a few hacks in the code that deviate from these definitions.
#       In particular, when the list is sorted by the "votes" field the word 
#       "DESC" is added to the end of the field to sort in descending order, 
#       and the redundant short_desc column is removed when the client
#       requests "all" columns.
# Note: For column names using aliasing (SQL "<field> AS <alias>"), the column
#       ID needs to be identical to the field ID for list ordering to work.

local our $columns = {};
sub DefineColumn {
    my ($id, $name, $title) = @_;
    $columns->{$id} = { 'name' => $name , 'title' => $title };
}

# Column:     ID                    Name                           Title
DefineColumn("bug_id"            , "bugs.bug_id"                , "ID"               );
DefineColumn("alias"             , "bugs.alias"                 , "Alias"            );
DefineColumn("opendate"          , "bugs.creation_ts"           , "Opened"           );
DefineColumn("changeddate"       , "bugs.delta_ts"              , "Changed"          );
DefineColumn("bug_severity"      , "bugs.bug_severity"          , "Severity"         );
DefineColumn("priority"          , "bugs.priority"              , "Priority"         );
DefineColumn("rep_platform"      , "bugs.rep_platform"          , "Hardware"         );
DefineColumn("assigned_to"       , "map_assigned_to.login_name" , "Assignee"         );
DefineColumn("reporter"          , "map_reporter.login_name"    , "Reporter"         );
DefineColumn("qa_contact"        , "map_qa_contact.login_name"  , "QA Contact"       );
if ($format->{'extension'} eq 'html') {
    DefineColumn("assigned_to_realname", "CASE WHEN map_assigned_to.realname = '' THEN map_assigned_to.login_name ELSE map_assigned_to.realname END AS assigned_to_realname", "Assignee"  );
    DefineColumn("reporter_realname"   , "CASE WHEN map_reporter.realname    = '' THEN map_reporter.login_name    ELSE map_reporter.realname    END AS reporter_realname"   , "Reporter"  );
    DefineColumn("qa_contact_realname" , "CASE WHEN map_qa_contact.realname  = '' THEN map_qa_contact.login_name  ELSE map_qa_contact.realname  END AS qa_contact_realname" , "QA Contact");
} else {
    DefineColumn("assigned_to_realname", "map_assigned_to.realname AS assigned_to_realname", "Assignee"  );
    DefineColumn("reporter_realname"   , "map_reporter.realname AS reporter_realname"      , "Reporter"  );
    DefineColumn("qa_contact_realname" , "map_qa_contact.realname AS qa_contact_realname"  , "QA Contact");
}
DefineColumn("bug_status"        , "bugs.bug_status"            , "Status"           );
DefineColumn("resolution"        , "bugs.resolution"            , "Resolution"       );
DefineColumn("short_short_desc"  , "bugs.short_desc"            , "Summary"          );
DefineColumn("short_desc"        , "bugs.short_desc"            , "Summary"          );
DefineColumn("status_whiteboard" , "bugs.status_whiteboard"     , "Whiteboard"       );
DefineColumn("component"         , "map_components.name"        , "Component"        );
DefineColumn("product"           , "map_products.name"          , "Product"          );
DefineColumn("classification"    , "map_classifications.name"   , "Classification"   );
DefineColumn("version"           , "bugs.version"               , "Version"          );
DefineColumn("op_sys"            , "bugs.op_sys"                , "OS"               );
DefineColumn("target_milestone"  , "bugs.target_milestone"      , "Target Milestone" );
DefineColumn("votes"             , "bugs.votes"                 , "Votes"            );
DefineColumn("keywords"          , "bugs.keywords"              , "Keywords"         );
DefineColumn("estimated_time"    , "bugs.estimated_time"        , "Estimated Hours"  );
DefineColumn("remaining_time"    , "bugs.remaining_time"        , "Remaining Hours"  );
DefineColumn("actual_time"       , "(SUM(ldtime.work_time)*COUNT(DISTINCT ldtime.bug_when)/COUNT(bugs.bug_id)) AS actual_time", "Actual Hours");
DefineColumn("percentage_complete",
    "(CASE WHEN (SUM(ldtime.work_time)*COUNT(DISTINCT ldtime.bug_when)/COUNT(bugs.bug_id)) " .
    "            + bugs.remaining_time = 0.0 " .
    "THEN 0.0 " .
    "ELSE 100*((SUM(ldtime.work_time)*COUNT(DISTINCT ldtime.bug_when)/COUNT(bugs.bug_id)) " .
    "     /((SUM(ldtime.work_time)*COUNT(DISTINCT ldtime.bug_when)/COUNT(bugs.bug_id)) + bugs.remaining_time)) " .
    "END) AS percentage_complete"                               , "% Complete"); 
DefineColumn("relevance"         , "relevance"                  , "Relevance"        );
DefineColumn("deadline"          , $dbh->sql_date_format('bugs.deadline', '%Y-%m-%d') . " AS deadline", "Deadline");

foreach my $field (Bugzilla->get_fields({ custom => 1, obsolete => 0})) {
    DefineColumn($field->name, 'bugs.' . $field->name, $field->description);
}

################################################################################
# Display Column Determination
################################################################################

# Determine the columns that will be displayed in the bug list via the 
# columnlist CGI parameter, the user's preferences, or the default.
my @displaycolumns = ();
if (defined $params->param('columnlist')) {
    if ($params->param('columnlist') eq "all") {
        # If the value of the CGI parameter is "all", display all columns,
        # but remove the redundant "short_desc" column.
        @displaycolumns = grep($_ ne 'short_desc', keys(%$columns));
    }
    else {
        @displaycolumns = split(/[ ,]+/, $params->param('columnlist'));
    }
}
elsif (defined $cgi->cookie('COLUMNLIST')) {
    # 2002-10-31 Rename column names (see bug 176461)
    my $columnlist = $cgi->cookie('COLUMNLIST');
    $columnlist =~ s/\bowner\b/assigned_to/;
    $columnlist =~ s/\bowner_realname\b/assigned_to_realname/;
    $columnlist =~ s/\bplatform\b/rep_platform/;
    $columnlist =~ s/\bseverity\b/bug_severity/;
    $columnlist =~ s/\bstatus\b/bug_status/;
    $columnlist =~ s/\bsummaryfull\b/short_desc/;
    $columnlist =~ s/\bsummary\b/short_short_desc/;

    # Use the columns listed in the user's preferences.
    @displaycolumns = split(/ /, $columnlist);
}
else {
    # Use the default list of columns.
    @displaycolumns = DEFAULT_COLUMN_LIST;
}

# Weed out columns that don't actually exist to prevent the user 
# from hacking their column list cookie to grab data to which they 
# should not have access.  Detaint the data along the way.
@displaycolumns = grep($columns->{$_} && trick_taint($_), @displaycolumns);

# Remove the "ID" column from the list because bug IDs are always displayed
# and are hard-coded into the display templates.
@displaycolumns = grep($_ ne 'bug_id', @displaycolumns);

# Add the votes column to the list of columns to be displayed
# in the bug list if the user is searching for bugs with a certain
# number of votes and the votes column is not already on the list.

# Some versions of perl will taint 'votes' if this is done as a single
# statement, because the votes param is tainted at this point
my $votes = $params->param('votes');
$votes ||= "";
if (trim($votes) && !grep($_ eq 'votes', @displaycolumns)) {
    push(@displaycolumns, 'votes');
}

# Remove the timetracking columns if they are not a part of the group
# (happens if a user had access to time tracking and it was revoked/disabled)
if (!Bugzilla->user->in_group(Bugzilla->params->{"timetrackinggroup"})) {
   @displaycolumns = grep($_ ne 'estimated_time', @displaycolumns);
   @displaycolumns = grep($_ ne 'remaining_time', @displaycolumns);
   @displaycolumns = grep($_ ne 'actual_time', @displaycolumns);
   @displaycolumns = grep($_ ne 'percentage_complete', @displaycolumns);
   @displaycolumns = grep($_ ne 'deadline', @displaycolumns);
}

# Remove the relevance column if the user is not doing a fulltext search.
if (grep('relevance', @displaycolumns) && !$fulltext) {
    @displaycolumns = grep($_ ne 'relevance', @displaycolumns);
}


################################################################################
# Select Column Determination
################################################################################

# Generate the list of columns that will be selected in the SQL query.

# The bug ID is always selected because bug IDs are always displayed.
# Severity, priority, resolution and status are required for buglist
# CSS classes.
my @selectcolumns = ("bug_id", "bug_severity", "priority", "bug_status",
                     "resolution");

# if using classification, we also need to look in product.classification_id
if (Bugzilla->params->{"useclassification"}) {
    push (@selectcolumns,"product");
}

# remaining and actual_time are required for percentage_complete calculation:
if (lsearch(\@displaycolumns, "percentage_complete") >= 0) {
    push (@selectcolumns, "remaining_time");
    push (@selectcolumns, "actual_time");
}

# Display columns are selected because otherwise we could not display them.
push (@selectcolumns, @displaycolumns);

# If the user is editing multiple bugs, we also make sure to select the product
# and status because the values of those fields determine what options the user
# has for modifying the bugs.
if ($dotweak) {
    push(@selectcolumns, "product") if !grep($_ eq 'product', @selectcolumns);
    push(@selectcolumns, "bug_status") if !grep($_ eq 'bug_status', @selectcolumns);
}

if ($format->{'extension'} eq 'ics') {
    push(@selectcolumns, "opendate") if !grep($_ eq 'opendate', @selectcolumns);
}

if ($format->{'extension'} eq 'atom') {
    # The title of the Atom feed will be the same one as for the bug list.
    $vars->{'title'} = $cgi->param('title');

    # This is the list of fields that are needed by the Atom filter.
    my @required_atom_columns = (
      'short_desc',
      'opendate',
      'changeddate',
      'reporter_realname',
      'priority',
      'bug_severity',
      'assigned_to_realname',
      'bug_status'
    );

    foreach my $required (@required_atom_columns) {
        push(@selectcolumns, $required) if !grep($_ eq $required,@selectcolumns);
    }
}

################################################################################
# Query Generation
################################################################################

# Convert the list of columns being selected into a list of column names.
my @selectnames = map($columns->{$_}->{'name'}, @selectcolumns);

# Remove columns with no names, such as percentage_complete
#  (or a removed *_time column due to permissions)
@selectnames = grep($_ ne '', @selectnames);

################################################################################
# Sort Order Determination
################################################################################

# Add to the query some instructions for sorting the bug list.

# First check if we'll want to reuse the last sorting order; that happens if
# the order is not defined or its value is "reuse last sort"
if (!$order || $order =~ /^reuse/i) {
    if ($cgi->cookie('LASTORDER')) {
        $order = $cgi->cookie('LASTORDER');
       
        # Cookies from early versions of Specific Search included this text,
        # which is now invalid.
        $order =~ s/ LIMIT 200//;
        
        $order_from_cookie = 1;
    }
    else {
        $order = '';  # Remove possible "reuse" identifier as unnecessary
    }
}

my $db_order = "";  # Modified version of $order for use with SQL query
if ($order) {
    # Convert the value of the "order" form field into a list of columns
    # by which to sort the results.
    ORDER: for ($order) {
        /^Bug Number$/ && do {
            $order = "bugs.bug_id";
            last ORDER;
        };
        /^Importance$/ && do {
            $order = "bugs.priority, bugs.bug_severity";
            last ORDER;
        };
        /^Assignee$/ && do {
            $order = "map_assigned_to.login_name, bugs.bug_status, bugs.priority, bugs.bug_id";
            last ORDER;
        };
        /^Last Changed$/ && do {
            $order = "bugs.delta_ts, bugs.bug_status, bugs.priority, map_assigned_to.login_name, bugs.bug_id";
            last ORDER;
        };
        do {
            my @order;
            my @columnnames = map($columns->{lc($_)}->{'name'}, keys(%$columns));
            # A custom list of columns.  Make sure each column is valid.
            foreach my $fragment (split(/,/, $order)) {
                $fragment = trim($fragment);
                # Accept an order fragment matching a column name, with
                # asc|desc optionally following (to specify the direction)
                if (grep($fragment =~ /^\Q$_\E(\s+(asc|desc))?$/, @columnnames, keys(%$columns))) {
                    next if $fragment =~ /\brelevance\b/ && !$fulltext;
                    push(@order, $fragment);
                }
                else {
                    my $vars = { fragment => $fragment };
                    if ($order_from_cookie) {
                        $cgi->remove_cookie('LASTORDER');
                        ThrowCodeError("invalid_column_name_cookie", $vars);
                    }
                    else {
                        ThrowCodeError("invalid_column_name_form", $vars);
                    }
                }
            }
            $order = join(",", @order);
            # Now that we have checked that all columns in the order are valid,
            # detaint the order string.
            trick_taint($order);
        };
    }
}
else {
    # DEFAULT
    $order = "bugs.bug_status, bugs.priority, map_assigned_to.login_name, bugs.bug_id";
}

# Make sure ORDER BY columns are included in the field list.
foreach my $fragment (split(/,/, $order)) {
    $fragment = trim($fragment);
    if (!grep($fragment =~ /^\Q$_\E(\s+(asc|desc))?$/, @selectnames)) {
        # Add order columns to selectnames
        # The fragment has already been validated
        $fragment =~ s/\s+(asc|desc)$//;

        # While newer fragments contain IDs for aliased columns, older
        # LASTORDER cookies (or bookmarks) may contain full names.
        # Convert them to an ID here.
        if ($fragment =~ / AS (\w+)/) {
            $fragment = $1;
        }

        $fragment =~ tr/a-zA-Z\.0-9\-_//cd;

        # If the order fragment is an ID, we need its corresponding name
        # to be in the field list.
        if (exists($columns->{$fragment})) {
            $fragment = $columns->{$fragment}->{'name'};
        }

        push @selectnames, $fragment;
    }
}

$db_order = $order;  # Copy $order into $db_order for use with SQL query

# If we are sorting by votes, sort in descending order if no explicit
# sort order was given
$db_order =~ s/bugs.votes\s*(,|$)/bugs.votes desc$1/i;
                             
# the 'actual_time' field is defined as an aggregate function, but 
# for order we just need the column name 'actual_time'
my $aggregate_search = quotemeta($columns->{'actual_time'}->{'name'});
$db_order =~ s/$aggregate_search/actual_time/g;

# the 'percentage_complete' field is defined as an aggregate too
$aggregate_search = quotemeta($columns->{'percentage_complete'}->{'name'});
$db_order =~ s/$aggregate_search/percentage_complete/g;

# Now put $db_order into a format that Bugzilla::Search can use.
# (We create $db_order as a string first because that's the way
# we did it before Bugzilla::Search took an "order" argument.)
my @orderstrings = split(/,\s*/, $db_order);

# Generate the basic SQL query that will be used to generate the bug list.
my $search = new Bugzilla::Search('fields' => \@selectnames, 
                                  'params' => $params,
                                  'order' => \@orderstrings);
my $query = $search->getSQL();

if (defined $cgi->param('limit')) {
    my $limit = $cgi->param('limit');
    if (detaint_natural($limit)) {
        $query .= " " . $dbh->sql_limit($limit);
    }
}
elsif ($fulltext) {
    $query .= " " . $dbh->sql_limit(FULLTEXT_BUGLIST_LIMIT);
    $vars->{'sorted_by_relevance'} = 1;
}


################################################################################
# Query Execution
################################################################################

if ($cgi->param('debug')) {
    $vars->{'debug'} = 1;
    $vars->{'query'} = $query;
    $vars->{'debugdata'} = $search->getDebugData();
}

# Time to use server push to display an interim message to the user until
# the query completes and we can display the bug list.
my $disposition = '';
if ($serverpush) {
    $filename =~ s/\\/\\\\/g; # escape backslashes
    $filename =~ s/"/\\"/g; # escape quotes
    $disposition = qq#inline; filename="$filename"#;

    print $cgi->multipart_init(-content_disposition => $disposition);
    print $cgi->multipart_start();

    # Generate and return the UI (HTML page) from the appropriate template.
    $template->process("list/server-push.html.tmpl", $vars)
      || ThrowTemplateError($template->error());

    # Under mod_perl, flush stdout so that the page actually shows up.
    if ($ENV{MOD_PERL}) {
        require Apache2::RequestUtil;
        Apache2::RequestUtil->request->rflush();
    }

    # Don't do multipart_end() until we're ready to display the replacement
    # page, otherwise any errors that happen before then (like SQL errors)
    # will result in a blank page being shown to the user instead of the error.
}

# Connect to the shadow database if this installation is using one to improve
# query performance.
$dbh = Bugzilla->switch_to_shadow_db();

# Normally, we ignore SIGTERM and SIGPIPE, but we need to
# respond to them here to prevent someone DOSing us by reloading a query
# a large number of times.
$::SIG{TERM} = 'DEFAULT';
$::SIG{PIPE} = 'DEFAULT';

# Execute the query.
my $buglist_sth = $dbh->prepare($query);
$buglist_sth->execute();


################################################################################
# Results Retrieval
################################################################################

# Retrieve the query results one row at a time and write the data into a list
# of Perl records.

my $bugowners = {};
my $bugproducts = {};
my $bugstatuses = {};
my @bugidlist;

my @bugs; # the list of records

while (my @row = $buglist_sth->fetchrow_array()) {
    my $bug = {}; # a record

    # Slurp the row of data into the record.
    # The second from last column in the record is the number of groups
    # to which the bug is restricted.
    foreach my $column (@selectcolumns) {
        $bug->{$column} = shift @row;
    }

    # Process certain values further (i.e. date format conversion).
    if ($bug->{'changeddate'}) {
        $bug->{'changeddate'} =~ 
            s/^(\d{4})(\d{2})(\d{2})(\d{2})(\d{2})(\d{2})$/$1-$2-$3 $4:$5:$6/;

        # Put in the change date as a time, so that the template date plugin
        # can format the date in any way needed by the template. ICS and Atom
        # have specific, and different, date and time formatting.
        $bug->{'changedtime'} = str2time($bug->{'changeddate'}, Bugzilla->params->{'timezone'});
        $bug->{'changeddate'} = DiffDate($bug->{'changeddate'});        
    }

    if ($bug->{'opendate'}) {
        # Put in the open date as a time for the template date plugin.
        $bug->{'opentime'} = str2time($bug->{'opendate'}, Bugzilla->params->{'timezone'});
        $bug->{'opendate'} = DiffDate($bug->{'opendate'});
    }

    # Record the assignee, product, and status in the big hashes of those things.
    $bugowners->{$bug->{'assigned_to'}} = 1 if $bug->{'assigned_to'};
    $bugproducts->{$bug->{'product'}} = 1 if $bug->{'product'};
    $bugstatuses->{$bug->{'bug_status'}} = 1 if $bug->{'bug_status'};

    $bug->{'secure_mode'} = undef;

    # Add the record to the list.
    push(@bugs, $bug);

    # Add id to list for checking for bug privacy later
    push(@bugidlist, $bug->{'bug_id'});
}

# Check for bug privacy and set $bug->{'secure_mode'} to 'implied' or 'manual'
# based on whether the privacy is simply product implied (by mandatory groups)
# or because of human choice
my %min_membercontrol;
if (@bugidlist) {
    my $sth = $dbh->prepare(
        "SELECT DISTINCT bugs.bug_id, MIN(group_control_map.membercontrol) " .
          "FROM bugs " .
    "INNER JOIN bug_group_map " .
            "ON bugs.bug_id = bug_group_map.bug_id " .
     "LEFT JOIN group_control_map " .
            "ON group_control_map.product_id = bugs.product_id " .
           "AND group_control_map.group_id = bug_group_map.group_id " .
         "WHERE bugs.bug_id IN (" . join(',',@bugidlist) . ") " .
            $dbh->sql_group_by('bugs.bug_id'));
    $sth->execute();
    while (my ($bug_id, $min_membercontrol) = $sth->fetchrow_array()) {
        $min_membercontrol{$bug_id} = $min_membercontrol || CONTROLMAPNA;
    }
    foreach my $bug (@bugs) {
        next unless defined($min_membercontrol{$bug->{'bug_id'}});
        if ($min_membercontrol{$bug->{'bug_id'}} == CONTROLMAPMANDATORY) {
            $bug->{'secure_mode'} = 'implied';
        }
        else {
            $bug->{'secure_mode'} = 'manual';
        }
    }
}

################################################################################
# Template Variable Definition
################################################################################

# Define the variables and functions that will be passed to the UI template.

$vars->{'bugs'} = \@bugs;
$vars->{'buglist'} = \@bugidlist;
$vars->{'buglist_joined'} = join(',', @bugidlist);
$vars->{'columns'} = $columns;
$vars->{'displaycolumns'} = \@displaycolumns;

my @openstates = BUG_STATE_OPEN;
$vars->{'openstates'} = \@openstates;
$vars->{'closedstates'} = ['CLOSED', 'VERIFIED', 'RESOLVED'];

# The list of query fields in URL query string format, used when creating
# URLs to the same query results page with different parameters (such as
# a different sort order or when taking some action on the set of query
# results).  To get this string, we call the Bugzilla::CGI::canoncalise_query
# function with a list of elements to be removed from the URL.
$vars->{'urlquerypart'} = $params->canonicalise_query('order',
                                                      'cmdtype',
                                                      'query_based_on');
$vars->{'order'} = $order;
$vars->{'caneditbugs'} = Bugzilla->user->in_group('editbugs');

my @bugowners = keys %$bugowners;
if (scalar(@bugowners) > 1 && Bugzilla->user->in_group('editbugs')) {
    my $suffix = Bugzilla->params->{'emailsuffix'};
    map(s/$/$suffix/, @bugowners) if $suffix;
    my $bugowners = join(",", @bugowners);
    $vars->{'bugowners'} = $bugowners;
}

# Whether or not to split the column titles across two rows to make
# the list more compact.
$vars->{'splitheader'} = $cgi->cookie('SPLITHEADER') ? 1 : 0;

$vars->{'quip'} = GetQuip();
$vars->{'currenttime'} = time();

# The following variables are used when the user is making changes to multiple bugs.
if ($dotweak && scalar @bugs) {
    $vars->{'dotweak'} = 1;
    $vars->{'use_keywords'} = 1 if Bugzilla::Keyword::keyword_count();

    $vars->{'products'} = Bugzilla->user->get_enterable_products;
    $vars->{'platforms'} = get_legal_field_values('rep_platform');
    $vars->{'op_sys'} = get_legal_field_values('op_sys');
    $vars->{'priorities'} = get_legal_field_values('priority');
    $vars->{'severities'} = get_legal_field_values('bug_severity');
    $vars->{'resolutions'} = Bugzilla::Bug->settable_resolutions;

    $vars->{'unconfirmedstate'} = 'UNCONFIRMED';

    $vars->{'bugstatuses'} = [ keys %$bugstatuses ];

    # The groups to which the user belongs.
    $vars->{'groups'} = GetGroups();

    # If all bugs being changed are in the same product, the user can change
    # their version and component, so generate a list of products, a list of
    # versions for the product (if there is only one product on the list of
    # products), and a list of components for the product.
    $vars->{'bugproducts'} = [ keys %$bugproducts ];
    if (scalar(@{$vars->{'bugproducts'}}) == 1) {
        my $product = new Bugzilla::Product(
            {name => $vars->{'bugproducts'}->[0]});
        $vars->{'versions'} = [map($_->name ,@{$product->versions})];
        $vars->{'components'} = [map($_->name, @{$product->components})];
        $vars->{'targetmilestones'} = [map($_->name, @{$product->milestones})]
            if Bugzilla->params->{'usetargetmilestone'};
    }
}

# If we're editing a stored query, use the existing query name as default for
# the "Remember search as" field.
$vars->{'defaultsavename'} = $cgi->param('query_based_on');


################################################################################
# HTTP Header Generation
################################################################################

# Generate HTTP headers

my $contenttype;
my $disp = "inline";

if ($format->{'extension'} eq "html" && !$agent) {
    if ($order) {
        $cgi->send_cookie(-name => 'LASTORDER',
                          -value => $order,
                          -expires => 'Fri, 01-Jan-2038 00:00:00 GMT');
    }
    my $bugids = join(":", @bugidlist);
    # See also Bug 111999
    if (length($bugids) == 0) {
        $cgi->remove_cookie('BUGLIST');
    }
    elsif (length($bugids) < 4000) {
        $cgi->send_cookie(-name => 'BUGLIST',
                          -value => $bugids,
                          -expires => 'Fri, 01-Jan-2038 00:00:00 GMT');
    }
    else {
        $cgi->remove_cookie('BUGLIST');
        $vars->{'toolong'} = 1;
    }

    $contenttype = "text/html";
}
else {
    $contenttype = $format->{'ctype'};
}

if ($format->{'extension'} eq "csv") {
    # We set CSV files to be downloaded, as they are designed for importing
    # into other programs.
    $disp = "attachment";
}

if ($serverpush) {
    # close the "please wait" page, then open the buglist page
    print $cgi->multipart_end();
    my @extra;
    push @extra, (-charset => "utf8") if Bugzilla->params->{"utf8"};
    print $cgi->multipart_start(-type => $contenttype, 
                                -content_disposition => $disposition, 
                                @extra);
} else {
    # Suggest a name for the bug list if the user wants to save it as a file.
    # If we are doing server push, then we did this already in the HTTP headers
    # that started the server push, so we don't have to do it again here.
    print $cgi->header(-type => $contenttype,
                       -content_disposition => "$disp; filename=$filename");
}


################################################################################
# Content Generation
################################################################################

# Generate and return the UI (HTML page) from the appropriate template.
$template->process($format->{'template'}, $vars)
  || ThrowTemplateError($template->error());


################################################################################
# Script Conclusion
################################################################################

print $cgi->multipart_final() if $serverpush;
