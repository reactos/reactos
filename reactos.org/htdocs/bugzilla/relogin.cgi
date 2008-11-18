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
#                 A. Karl Kornel <karl@kornel.name>

use strict;
use lib qw(.);

use Bugzilla;
use Bugzilla::Mailer;
use Bugzilla::Constants;
use Bugzilla::Error;
use Bugzilla::Token;
use Bugzilla::User;
use Bugzilla::Util;
use Date::Format;

my $template = Bugzilla->template;
my $cgi = Bugzilla->cgi;

my $action = $cgi->param('action') || 'logout';

my $vars = {};
my $target;

# prepare-sudo: Display the sudo information & login page
if ($action eq 'prepare-sudo') {
    # We must have a logged-in user to do this
    # That user must be in the 'bz_sudoers' group
    my $user = Bugzilla->login(LOGIN_REQUIRED);
    unless ($user->in_group('bz_sudoers')) {
        ThrowUserError('auth_failure', {  group => 'bz_sudoers',
                                         action => 'begin',
                                         object => 'sudo_session' }
        );
    }
    
    # Do not try to start a new session if one is already in progress!
    if (defined(Bugzilla->sudoer)) {
        ThrowUserError('sudo_in_progress', { target => $user->login });
    }

    # Keep a temporary record of the user visiting this page
    $vars->{'token'} = issue_session_token('sudo_prepared');

    # Show the sudo page
    $vars->{'target_login_default'} = $cgi->param('target_login');
    $vars->{'reason_default'} = $cgi->param('reason');
    $target = 'admin/sudo.html.tmpl';
}
# begin-sudo: Confirm login and start sudo session
elsif ($action eq 'begin-sudo') {
    # We must be sure that the user is authenticating by providing a login
    # and password.
    # We only need to do this for authentication methods that involve Bugzilla 
    # directly obtaining a login (i.e. normal CGI login), as opposed to other 
    # methods (like Environment vars login). 

    # First, record if Bugzilla_login and Bugzilla_password were provided
    my $credentials_provided;
    if (defined($cgi->param('Bugzilla_login'))
        && defined($cgi->param('Bugzilla_password')))
    {
        $credentials_provided = 1;
    }
    
    # Next, log in the user
    my $user = Bugzilla->login(LOGIN_REQUIRED);
    
    # At this point, the user is logged in.  However, if they used a method
    # where they could have provided a username/password (i.e. CGI), but they 
    # did not provide a username/password, then throw an error.
    if ($user->authorizer->can_login && !$credentials_provided) {
        ThrowUserError('sudo_password_required',
                       { target_login => $cgi->param('target_login'),
                               reason => $cgi->param('reason')});
    }
    
    # The user must be in the 'bz_sudoers' group
    unless ($user->in_group('bz_sudoers')) {
        ThrowUserError('auth_failure', {  group => 'bz_sudoers',
                                         action => 'begin',
                                         object => 'sudo_session' }
        );
    }
    
    # Do not try to start a new session if one is already in progress!
    if (defined(Bugzilla->sudoer)) {
        ThrowUserError('sudo_in_progress', { target => $user->login });
    }

    # Did the user actually go trough the 'sudo-prepare' action?  Do some 
    # checks on the token the action should have left.
    my ($token_user, $token_timestamp, $token_data) =
        Bugzilla::Token::GetTokenData($cgi->param('token'));
    unless (defined($token_user)
            && defined($token_data)
            && ($token_user == $user->id)
            && ($token_data eq 'sudo_prepared'))
    {
        ThrowUserError('sudo_preparation_required', 
                       { target_login => scalar $cgi->param('target_login'),
                               reason => scalar $cgi->param('reason')});
    }
    delete_token($cgi->param('token'));

    # Get & verify the target user (the user who we will be impersonating)
    my $target_user = 
        new Bugzilla::User({ name => $cgi->param('target_login') });
    unless (defined($target_user)
            && $target_user->id
            && $user->can_see_user($target_user))
    {
        ThrowUserError('user_match_failed',
                       { 'name' => $cgi->param('target_login') }
        );
    }
    if ($target_user->in_group('bz_sudo_protect')) {
        ThrowUserError('sudo_protected', { login => $target_user->login });
    }

    # If we have a reason passed in, keep it under 200 characters
    my $reason = $cgi->param('reason') || '';
    $reason = substr($reason, $[, 200);
    
    # Calculate the session expiry time (T + 6 hours)
    my $time_string = time2str('%a, %d-%b-%Y %T %Z', time+(6*60*60), 'GMT');

    # For future sessions, store the unique ID of the target user
    $cgi->send_cookie('-name'    => 'sudo',
                      '-expires' => $time_string,
                      '-value'   => $target_user->id
    );
    
    # For the present, change the values of Bugzilla::user & Bugzilla::sudoer
    Bugzilla->sudo_request($target_user, $user);
    
    # NOTE: If you want to log the start of an sudo session, do it here.

    # Go ahead and send out the message now
    my $message;
    my $mail_template = Bugzilla->template_inner($target_user->settings->{'lang'}->{'value'});
    $mail_template->process('email/sudo.txt.tmpl', { reason => $reason }, \$message);
    Bugzilla->template_inner("");
    MessageToMTA($message);

    $vars->{'message'} = 'sudo_started';
    $vars->{'target'} = $target_user->login;
    $target = 'global/message.html.tmpl';
}
# end-sudo: End the current sudo session (if one is in progress)
elsif ($action eq 'end-sudo') {
    # Regardless of our state, delete the sudo cookie if it exists
    $cgi->remove_cookie('sudo');

    # Are we in an sudo session?
    Bugzilla->login(LOGIN_OPTIONAL);
    my $sudoer = Bugzilla->sudoer;
    if (defined($sudoer)) {
        Bugzilla->sudo_request($sudoer, undef);
    }

    # NOTE: If you want to log the end of an sudo session, so it here.
    
    $vars->{'message'} = 'sudo_ended';
    $target = 'global/message.html.tmpl';
}
# Log out the currently logged-in user (this used to be the only thing this did)
elsif ($action eq 'logout') {
    # We don't want to remove a random logincookie from the db, so
    # call Bugzilla->login(). If we're logged in after this, then
    # the logincookie must be correct
    Bugzilla->login(LOGIN_OPTIONAL);

    $cgi->remove_cookie('sudo');

    Bugzilla->logout();

    $vars->{'message'} = "logged_out";
    $target = 'global/message.html.tmpl';
}
# No valid action found
else {
    Bugzilla->login(LOGIN_OPTIONAL);
    ThrowCodeError('unknown_action', {action => $action});
}

# Display the template
print $cgi->header();
$template->process($target, $vars)
      || ThrowTemplateError($template->error());
