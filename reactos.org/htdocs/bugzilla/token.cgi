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

############################################################################
# Script Initialization
############################################################################

# Make it harder for us to do dangerous things in Perl.
use strict;

use lib qw(.);

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::Token;
use Bugzilla::User;

use Date::Parse;

my $dbh = Bugzilla->dbh;
local our $cgi = Bugzilla->cgi;
local our $template = Bugzilla->template;
local our $vars = {};

Bugzilla->login(LOGIN_OPTIONAL);

################################################################################
# Data Validation / Security Authorization
################################################################################

# Throw an error if the form does not contain an "action" field specifying
# what the user wants to do.
$cgi->param('a') || ThrowCodeError("unknown_action");

# Assign the action to a global variable.
$::action = $cgi->param('a');

# If a token was submitted, make sure it is a valid token that exists in the
# database and is the correct type for the action being taken.
if ($cgi->param('t')) {
  # Assign the token and its SQL quoted equivalent to global variables.
  $::token = $cgi->param('t');
  
  # Make sure the token contains only valid characters in the right amount.
  # validate_password will throw an error if token is invalid
  validate_password($::token);

  Bugzilla::Token::CleanTokenTable();

  # Make sure the token exists in the database.
  my ($tokentype) = $dbh->selectrow_array('SELECT tokentype FROM tokens
                                           WHERE token = ?', undef, $::token);
  $tokentype || ThrowUserError("token_inexistent");

  # Make sure the token is the correct type for the action being taken.
  if ( grep($::action eq $_ , qw(cfmpw cxlpw chgpw)) && $tokentype ne 'password' ) {
    Bugzilla::Token::Cancel($::token, "wrong_token_for_changing_passwd");
    ThrowUserError("wrong_token_for_changing_passwd");
  }
  if ( ($::action eq 'cxlem') 
      && (($tokentype ne 'emailold') && ($tokentype ne 'emailnew')) ) {
    Bugzilla::Token::Cancel($::token, "wrong_token_for_cancelling_email_change");
    ThrowUserError("wrong_token_for_cancelling_email_change");
  }
  if ( grep($::action eq $_ , qw(cfmem chgem)) 
      && ($tokentype ne 'emailnew') ) {
    Bugzilla::Token::Cancel($::token, "wrong_token_for_confirming_email_change");
    ThrowUserError("wrong_token_for_confirming_email_change");
  }
  if (($::action =~ /^(request|confirm|cancel)_new_account$/)
      && ($tokentype ne 'account'))
  {
      Bugzilla::Token::Cancel($::token, 'wrong_token_for_creating_account');
      ThrowUserError('wrong_token_for_creating_account');
  }
}


# If the user is requesting a password change, make sure they submitted
# their login name and it exists in the database, and that the DB module is in
# the list of allowed verification methods.
my $user_account;
if ( $::action eq 'reqpw' ) {
    my $login_name = $cgi->param('loginname')
                       || ThrowUserError("login_needed_for_password_change");

    # check verification methods
    unless (Bugzilla->user->authorizer->can_change_password) {
        ThrowUserError("password_change_requests_not_allowed");
    }

    validate_email_syntax($login_name)
        || ThrowUserError('illegal_email_address', {addr => $login_name});

    $user_account = new Bugzilla::User({name => $login_name});
    $user_account || ThrowUserError("account_inexistent");
}

# If the user is changing their password, make sure they submitted a new
# password and that the new password is valid.
my $password;
if ( $::action eq 'chgpw' ) {
    $password = $cgi->param('password');
    defined $password
      && defined $cgi->param('matchpassword')
      || ThrowUserError("require_new_password");

    validate_password($password, $cgi->param('matchpassword'));
}

################################################################################
# Main Body Execution
################################################################################

# All calls to this script should contain an "action" variable whose value
# determines what the user wants to do.  The code below checks the value of
# that variable and runs the appropriate code.

if ($::action eq 'reqpw') { 
    requestChangePassword($user_account);
} elsif ($::action eq 'cfmpw') { 
    confirmChangePassword(); 
} elsif ($::action eq 'cxlpw') { 
    cancelChangePassword(); 
} elsif ($::action eq 'chgpw') { 
    changePassword($password);
} elsif ($::action eq 'cfmem') {
    confirmChangeEmail();
} elsif ($::action eq 'cxlem') {
    cancelChangeEmail();
} elsif ($::action eq 'chgem') {
    changeEmail();
} elsif ($::action eq 'request_new_account') {
    request_create_account();
} elsif ($::action eq 'confirm_new_account') {
    confirm_create_account();
} elsif ($::action eq 'cancel_new_account') {
    cancel_create_account();
} else { 
    # If the action that the user wants to take (specified in the "a" form field)
    # is none of the above listed actions, display an error telling the user 
    # that we do not understand what they would like to do.
    ThrowCodeError("unknown_action", { action => $::action });
}

exit;

################################################################################
# Functions
################################################################################

sub requestChangePassword {
    my ($user) = @_;
    Bugzilla::Token::IssuePasswordToken($user);

    $vars->{'message'} = "password_change_request";

    print $cgi->header();
    $template->process("global/message.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
}

sub confirmChangePassword {
    $vars->{'token'} = $::token;
    
    print $cgi->header();
    $template->process("account/password/set-forgotten-password.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
}

sub cancelChangePassword {    
    $vars->{'message'} = "password_change_cancelled";
    Bugzilla::Token::Cancel($::token, $vars->{'message'});

    print $cgi->header();
    $template->process("global/message.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
}

sub changePassword {
    my ($password) = @_;
    my $dbh = Bugzilla->dbh;

    # Create a crypted version of the new password
    my $cryptedpassword = bz_crypt($password);

    # Get the user's ID from the tokens table.
    my ($userid) = $dbh->selectrow_array('SELECT userid FROM tokens
                                          WHERE token = ?', undef, $::token);
    
    # Update the user's password in the profiles table and delete the token
    # from the tokens table.
    $dbh->bz_lock_tables('profiles WRITE', 'tokens WRITE');
    $dbh->do(q{UPDATE   profiles
               SET      cryptpassword = ?
               WHERE    userid = ?},
             undef, ($cryptedpassword, $userid) );
    $dbh->do('DELETE FROM tokens WHERE token = ?', undef, $::token);
    $dbh->bz_unlock_tables();

    Bugzilla->logout_user_by_id($userid);

    $vars->{'message'} = "password_changed";

    print $cgi->header();
    $template->process("global/message.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
}

sub confirmChangeEmail {
    # Return HTTP response headers.
    print $cgi->header();

    $vars->{'token'} = $::token;

    $template->process("account/email/confirm.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
}

sub changeEmail {
    my $dbh = Bugzilla->dbh;

    # Get the user's ID from the tokens table.
    my ($userid, $eventdata) = $dbh->selectrow_array(
                                 q{SELECT userid, eventdata FROM tokens
                                   WHERE token = ?}, undef, $::token);
    my ($old_email, $new_email) = split(/:/,$eventdata);

    # Check the user entered the correct old email address
    if(lc($cgi->param('email')) ne lc($old_email)) {
        ThrowUserError("email_confirmation_failed");
    }
    # The new email address should be available as this was 
    # confirmed initially so cancel token if it is not still available
    if (! is_available_username($new_email,$old_email)) {
        $vars->{'email'} = $new_email; # Needed for Bugzilla::Token::Cancel's mail
        Bugzilla::Token::Cancel($::token, "account_exists", $vars);
        ThrowUserError("account_exists", { email => $new_email } );
    } 

    # Update the user's login name in the profiles table and delete the token
    # from the tokens table.
    $dbh->bz_lock_tables('profiles WRITE', 'tokens WRITE');
    $dbh->do(q{UPDATE   profiles
               SET      login_name = ?
               WHERE    userid = ?},
             undef, ($new_email, $userid));
    $dbh->do('DELETE FROM tokens WHERE token = ?', undef, $::token);
    $dbh->do(q{DELETE FROM tokens WHERE userid = ?
               AND tokentype = 'emailnew'}, undef, $userid);
    $dbh->bz_unlock_tables();

    # The email address has been changed, so we need to rederive the groups
    my $user = new Bugzilla::User($userid);
    $user->derive_regexp_groups;

    # Return HTTP response headers.
    print $cgi->header();

    # Let the user know their email address has been changed.

    $vars->{'message'} = "login_changed";

    $template->process("global/message.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
}

sub cancelChangeEmail {
    my $dbh = Bugzilla->dbh;

    # Get the user's ID from the tokens table.
    my ($userid, $tokentype, $eventdata) = $dbh->selectrow_array(
                              q{SELECT userid, tokentype, eventdata FROM tokens
                                WHERE token = ?}, undef, $::token);
    my ($old_email, $new_email) = split(/:/,$eventdata);

    if($tokentype eq "emailold") {
        $vars->{'message'} = "emailold_change_cancelled";

        my $actualemail = $dbh->selectrow_array(
                            q{SELECT login_name FROM profiles
                              WHERE userid = ?}, undef, $userid);
        
        # check to see if it has been altered
        if($actualemail ne $old_email) {
            $dbh->bz_lock_tables('profiles WRITE');
            $dbh->do(q{UPDATE   profiles
                       SET      login_name = ?
                       WHERE    userid = ?},
                     undef, ($old_email, $userid));
            $dbh->bz_unlock_tables();

            # email has changed, so rederive groups
            # Note that this is done _after_ the tables are unlocked
            # This is sort of a race condition (given the lack of transactions)
            # but the user had access to it just now, so it's not a security
            # issue

            my $user = new Bugzilla::User($userid);
            $user->derive_regexp_groups;

            $vars->{'message'} = "email_change_cancelled_reinstated";
        } 
    } 
    else {
        $vars->{'message'} = 'email_change_cancelled'
     }

    $vars->{'old_email'} = $old_email;
    $vars->{'new_email'} = $new_email;
    Bugzilla::Token::Cancel($::token, $vars->{'message'}, $vars);

    $dbh->bz_lock_tables('tokens WRITE');
    $dbh->do(q{DELETE FROM tokens WHERE userid = ?
               AND tokentype = 'emailold' OR tokentype = 'emailnew'},
             undef, $userid);
    $dbh->bz_unlock_tables();

    # Return HTTP response headers.
    print $cgi->header();

    $template->process("global/message.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
}

sub request_create_account {
    my (undef, $date, $login_name) = Bugzilla::Token::GetTokenData($::token);
    $vars->{'token'} = $::token;
    $vars->{'email'} = $login_name . Bugzilla->params->{'emailsuffix'};
    $vars->{'date'} = str2time($date);

    # We require a HTTPS connection if possible.
    if (Bugzilla->params->{'sslbase'} ne ''
        && Bugzilla->params->{'ssl'} ne 'never')
    {
        $cgi->require_https(Bugzilla->params->{'sslbase'});
    }
    print $cgi->header();

    $template->process('account/email/confirm-new.html.tmpl', $vars)
      || ThrowTemplateError($template->error());
}

sub confirm_create_account {
    my (undef, undef, $login_name) = Bugzilla::Token::GetTokenData($::token);

    my $password = $cgi->param('passwd1') || '';
    validate_password($password, $cgi->param('passwd2') || '');

    my $otheruser = Bugzilla::User->create({
        login_name => $login_name, 
        realname   => $cgi->param('realname'), 
        cryptpassword => $password});

    # Now delete this token.
    delete_token($::token);

    # Let the user know that his user account has been successfully created.
    $vars->{'message'} = 'account_created';
    $vars->{'otheruser'} = $otheruser;
    $vars->{'login_info'} = 1;

    print $cgi->header();

    $template->process('global/message.html.tmpl', $vars)
      || ThrowTemplateError($template->error());
}

sub cancel_create_account {
    my (undef, undef, $login_name) = Bugzilla::Token::GetTokenData($::token);

    $vars->{'message'} = 'account_creation_cancelled';
    $vars->{'account'} = $login_name;
    Bugzilla::Token::Cancel($::token, $vars->{'message'});

    print $cgi->header();
    $template->process('global/message.html.tmpl', $vars)
      || ThrowTemplateError($template->error());
}
