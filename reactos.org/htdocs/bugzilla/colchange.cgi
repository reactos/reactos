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
#                 Gervase Markham <gerv@gerv.net>
#                 Max Kanat-Alexander <mkanat@bugzilla.org>

use strict;

use lib qw(.);

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Error;
use Bugzilla::User;
use Bugzilla::Keyword;

Bugzilla->login();

my $cgi = Bugzilla->cgi;
my $template = Bugzilla->template;
my $vars = {};

# The master list not only says what fields are possible, but what order
# they get displayed in.
my @masterlist = ("opendate", "changeddate", "bug_severity", "priority",
                  "rep_platform", "assigned_to", "assigned_to_realname",
                  "reporter", "reporter_realname", "bug_status",
                  "resolution");

if (Bugzilla->params->{"useclassification"}) {
    push(@masterlist, "classification");
}

push(@masterlist, ("product", "component", "version", "op_sys"));

if (Bugzilla->params->{"usevotes"}) {
    push (@masterlist, "votes");
}
if (Bugzilla->params->{"usebugaliases"}) {
    unshift(@masterlist, "alias");
}
if (Bugzilla->params->{"usetargetmilestone"}) {
    push(@masterlist, "target_milestone");
}
if (Bugzilla->params->{"useqacontact"}) {
    push(@masterlist, "qa_contact");
    push(@masterlist, "qa_contact_realname");
}
if (Bugzilla->params->{"usestatuswhiteboard"}) {
    push(@masterlist, "status_whiteboard");
}
if (Bugzilla::Keyword::keyword_count()) {
    push(@masterlist, "keywords");
}

if (Bugzilla->user->in_group(Bugzilla->params->{"timetrackinggroup"})) {
    push(@masterlist, ("estimated_time", "remaining_time", "actual_time",
                       "percentage_complete", "deadline")); 
}

push(@masterlist, ("short_desc", "short_short_desc"));

push(@masterlist, Bugzilla->custom_field_names);

$vars->{'masterlist'} = \@masterlist;

my @collist;
if (defined $cgi->param('rememberedquery')) {
    my $splitheader = 0;
    if (defined $cgi->param('resetit')) {
        @collist = DEFAULT_COLUMN_LIST;
    } else {
        foreach my $i (@masterlist) {
            if (defined $cgi->param("column_$i")) {
                push @collist, $i;
            }
        }
        if (defined $cgi->param('splitheader')) {
            $splitheader = $cgi->param('splitheader')? 1: 0;
        }
    }
    my $list = join(" ", @collist);
    my $urlbase = Bugzilla->params->{"urlbase"};

    if ($list) {
        $cgi->send_cookie(-name => 'COLUMNLIST',
                          -value => $list,
                          -expires => 'Fri, 01-Jan-2038 00:00:00 GMT');
    }
    else {
        $cgi->remove_cookie('COLUMNLIST');
    }
    if ($splitheader) {
        $cgi->send_cookie(-name => 'SPLITHEADER',
                          -value => $splitheader,
                          -expires => 'Fri, 01-Jan-2038 00:00:00 GMT');
    }
    else {
        $cgi->remove_cookie('SPLITHEADER');
    }

    $vars->{'message'} = "change_columns";
    $vars->{'redirect_url'} = "buglist.cgi?".$cgi->param('rememberedquery');

    # If we're running on Microsoft IIS, using cgi->redirect discards
    # the Set-Cookie lines -- workaround is to use the old-fashioned 
    # redirection mechanism. See bug 214466 for details.
    if ($ENV{'SERVER_SOFTWARE'} =~ /Microsoft-IIS/
        || $ENV{'SERVER_SOFTWARE'} =~ /Sun ONE Web/)
    {
      print $cgi->header(-type => "text/html",
                         -refresh => "0; URL=$vars->{'redirect_url'}");
    }
    else {
      print $cgi->redirect($vars->{'redirect_url'});
    }
    
    $template->process("global/message.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
    exit;
}

if (defined $cgi->cookie('COLUMNLIST')) {
    @collist = split(/ /, $cgi->cookie('COLUMNLIST'));
} else {
    @collist = DEFAULT_COLUMN_LIST;
}

$vars->{'collist'} = \@collist;
$vars->{'splitheader'} = $cgi->cookie('SPLITHEADER') ? 1 : 0;

$vars->{'buffer'} = $cgi->query_string();

# Generate and return the UI (HTML page) from the appropriate template.
print $cgi->header();
$template->process("list/change-columns.html.tmpl", $vars)
  || ThrowTemplateError($template->error());
