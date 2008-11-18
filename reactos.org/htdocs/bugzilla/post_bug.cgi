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
#                 Joe Robins <jmrobins@tgix.com>
#                 Gervase Markham <gerv@gerv.net>
#                 Marc Schumann <wurblzap@gmail.com>

use strict;
use lib qw(.);

use Bugzilla;
use Bugzilla::Attachment;
use Bugzilla::BugMail;
use Bugzilla::Constants;
use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::Bug;
use Bugzilla::User;
use Bugzilla::Field;
use Bugzilla::Product;
use Bugzilla::Component;
use Bugzilla::Keyword;
use Bugzilla::Token;
use Bugzilla::Flag;

my $user = Bugzilla->login(LOGIN_REQUIRED);

my $cgi = Bugzilla->cgi;
my $dbh = Bugzilla->dbh;
my $template = Bugzilla->template;
my $vars = {};

######################################################################
# Main Script
######################################################################

# Detect if the user already used the same form to submit a bug
my $token = trim($cgi->param('token'));
if ($token) {
    my ($creator_id, $date, $old_bug_id) = Bugzilla::Token::GetTokenData($token);
    unless ($creator_id
              && ($creator_id == $user->id)
              && ($old_bug_id =~ "^createbug:"))
    {
        # The token is invalid.
        ThrowUserError('token_inexistent');
    }

    $old_bug_id =~ s/^createbug://;

    if ($old_bug_id && (!$cgi->param('ignore_token')
                        || ($cgi->param('ignore_token') != $old_bug_id)))
    {
        $vars->{'bugid'} = $old_bug_id;
        $vars->{'allow_override'} = defined $cgi->param('ignore_token') ? 0 : 1;

        print $cgi->header();
        $template->process("bug/create/confirm-create-dupe.html.tmpl", $vars)
           || ThrowTemplateError($template->error());
        exit;
    }
}    

# do a match on the fields if applicable

&Bugzilla::User::match_field ($cgi, {
    'cc'            => { 'type' => 'multi'  },
    'assigned_to'   => { 'type' => 'single' },
    'qa_contact'    => { 'type' => 'single' },
    '^requestee_type-(\d+)$' => { 'type' => 'multi' },
});

if (defined $cgi->param('maketemplate')) {
    $vars->{'url'} = $cgi->canonicalise_query('token');
    $vars->{'short_desc'} = $cgi->param('short_desc');
    
    print $cgi->header();
    $template->process("bug/create/make-template.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
    exit;
}

umask 0;

# get current time
my $timestamp = $dbh->selectrow_array(q{SELECT NOW()});

# Group Validation
my @selected_groups;
foreach my $group (grep(/^bit-\d+$/, $cgi->param())) {
    $group =~ /^bit-(\d+)$/;
    push(@selected_groups, $1);
}

# The format of the initial comment can be structured by adding fields to the
# enter_bug template and then referencing them in the comment template.
my $comment;
my $format = $template->get_format("bug/create/comment",
                                   scalar($cgi->param('format')), "txt");
$template->process($format->{'template'}, $vars, \$comment)
    || ThrowTemplateError($template->error());

# Include custom fields editable on bug creation.
my @custom_bug_fields = Bugzilla->get_fields(
    { custom => 1, obsolete => 0, enter_bug => 1 });

# Undefined custom fields are ignored to ensure they will get their default
# value (e.g. "---" for custom single select fields).
my @bug_fields = grep { defined $cgi->param($_->name) } @custom_bug_fields;
@bug_fields = map { $_->name } @bug_fields;

push(@bug_fields, qw(
    product
    component

    assigned_to
    qa_contact

    alias
    blocked
    commentprivacy
    bug_file_loc
    bug_severity
    bug_status
    dependson
    keywords
    short_desc
    op_sys
    priority
    rep_platform
    version
    target_milestone
    status_whiteboard

    estimated_time
    deadline
));
my %bug_params;
foreach my $field (@bug_fields) {
    $bug_params{$field} = $cgi->param($field);
}
$bug_params{'creation_ts'} = $timestamp;
$bug_params{'cc'}          = [$cgi->param('cc')];
$bug_params{'groups'}      = \@selected_groups;
$bug_params{'comment'}     = $comment;

my $bug = Bugzilla::Bug->create(\%bug_params);

# Get the bug ID back.
my $id = $bug->bug_id;

# Set Version cookie, but only if the user actually selected
# a version on the page.
if (defined $cgi->param('version')) {
    $cgi->send_cookie(-name => "VERSION-" . $bug->product,
                      -value => $bug->version,
                      -expires => "Fri, 01-Jan-2038 00:00:00 GMT");
}

# We don't have to check if the user can see the bug, because a user filing
# a bug can always see it. You can't change reporter_accessible until
# after the bug is filed.

# Add an attachment if requested.
if (defined($cgi->upload('data')) || $cgi->param('attachurl')) {
    $cgi->param('isprivate', $cgi->param('commentprivacy'));
    my $attach_id = Bugzilla::Attachment->insert_attachment_for_bug(!THROW_ERROR,
                                                  $bug, $user, $timestamp, \$vars);

    if ($attach_id) {
        # Update the comment to include the new attachment ID.
        # This string is hardcoded here because Template::quoteUrls()
        # expects to find this exact string.
        my $new_comment = "Created an attachment (id=$attach_id)\n" .
                          $cgi->param('description') . "\n";
        # We can use $bug->longdescs here because we are sure that the bug
        # description is of type CMT_NORMAL. No need to include it if it's
        # empty, though.
        if ($bug->longdescs->[0]->{'body'} !~ /^\s+$/) {
            $new_comment .= "\n" . $bug->longdescs->[0]->{'body'};
        }
        $bug->update_comment($bug->longdescs->[0]->{'id'}, $new_comment);
    }
    else {
        $vars->{'message'} = 'attachment_creation_failed';
    }

    # Determine if Patch Viewer is installed, for Diff link
    eval {
        require PatchReader;
        $vars->{'patchviewerinstalled'} = 1;
    };
}

# Add flags, if any. To avoid dying if something goes wrong
# while processing flags, we will eval() flag validation.
# This requires errors to die().
# XXX: this can go away as soon as flag validation is able to
#      fail without dying.
my $error_mode_cache = Bugzilla->error_mode;
Bugzilla->error_mode(ERROR_MODE_DIE);
eval {
    Bugzilla::Flag::validate($cgi, $id, undef, SKIP_REQUESTEE_ON_ERROR);
    Bugzilla::Flag::process($bug, undef, $timestamp, $cgi);
};
Bugzilla->error_mode($error_mode_cache);
if ($@) {
    $vars->{'message'} = 'flag_creation_failed';
    $vars->{'flag_creation_error'} = $@;
}

# Email everyone the details of the new bug 
$vars->{'mailrecipients'} = {'changer' => $user->login};

$vars->{'id'} = $id;
$vars->{'bug'} = $bug;

ThrowCodeError("bug_error", { bug => $bug }) if $bug->error;

$vars->{'sentmail'} = [];

push (@{$vars->{'sentmail'}}, { type => 'created',
                                id => $id,
                              });

foreach my $i (@{$bug->dependson || []}, @{$bug->blocked || []}) {
    push (@{$vars->{'sentmail'}}, { type => 'dep', id => $i, });
}

my @bug_list;
if ($cgi->cookie("BUGLIST")) {
    @bug_list = split(/:/, $cgi->cookie("BUGLIST"));
}
$vars->{'bug_list'} = \@bug_list;
$vars->{'use_keywords'} = 1 if Bugzilla::Keyword::keyword_count();

if ($token) {
    trick_taint($token);
    $dbh->do('UPDATE tokens SET eventdata = ? WHERE token = ?', undef, 
             ("createbug:$id", $token));
}

if (Bugzilla->usage_mode == USAGE_MODE_EMAIL) {
    Bugzilla::BugMail::Send($id, $vars->{'mailrecipients'});
}
else {
    print $cgi->header();
    $template->process("bug/create/created.html.tmpl", $vars)
        || ThrowTemplateError($template->error());
}

1;
