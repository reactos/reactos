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
# Contributor(s):    Myk Melez <myk@mozilla.org>
#                    Frédéric Buclin <LpSolit@gmail.com>

################################################################################
# Module Initialization
################################################################################

# Make it harder for us to do dangerous things in Perl.
use strict;

# Bundle the functions in this file together into the "Bugzilla::Token" package.
package Bugzilla::Token;

use Bugzilla::Constants;
use Bugzilla::Error;
use Bugzilla::Mailer;
use Bugzilla::Util;
use Bugzilla::User;

use Date::Format;
use Date::Parse;
use File::Basename;

use base qw(Exporter);

@Bugzilla::Token::EXPORT = qw(issue_session_token check_token_data delete_token);

################################################################################
# Public Functions
################################################################################

# Creates and sends a token to create a new user account.
# It assumes that the login has the correct format and is not already in use.
sub issue_new_user_account_token {
    my $login_name = shift;
    my $dbh = Bugzilla->dbh;
    my $template = Bugzilla->template;
    my $vars = {};

    # Is there already a pending request for this login name? If yes, do not throw
    # an error because the user may have lost his email with the token inside.
    # But to prevent using this way to mailbomb an email address, make sure
    # the last request is at least 10 minutes old before sending a new email.

    my $pending_requests =
        $dbh->selectrow_array('SELECT COUNT(*)
                                 FROM tokens
                                WHERE tokentype = ?
                                  AND ' . $dbh->sql_istrcmp('eventdata', '?') . '
                                  AND issuedate > NOW() - ' . $dbh->sql_interval(10, 'MINUTE'),
                               undef, ('account', $login_name));

    ThrowUserError('too_soon_for_new_token', {'type' => 'account'}) if $pending_requests;

    my ($token, $token_ts) = _create_token(undef, 'account', $login_name);

    $vars->{'email'} = $login_name . Bugzilla->params->{'emailsuffix'};
    $vars->{'token_ts'} = $token_ts;
    $vars->{'token'} = $token;

    my $message;
    $template->process('account/email/request-new.txt.tmpl', $vars, \$message)
      || ThrowTemplateError($template->error());

    # In 99% of cases, the user getting the confirmation email is the same one
    # who made the request, and so it is reasonable to send the email in the same
    # language used to view the "Create a New Account" page (we cannot use his
    # user prefs as the user has no account yet!).
    MessageToMTA($message);
}

sub IssueEmailChangeToken {
    my ($user, $old_email, $new_email) = @_;
    my $email_suffix = Bugzilla->params->{'emailsuffix'};

    my ($token, $token_ts) = _create_token($user->id, 'emailold', $old_email . ":" . $new_email);

    my $newtoken = _create_token($user->id, 'emailnew', $old_email . ":" . $new_email);

    # Mail the user the token along with instructions for using it.

    my $template = Bugzilla->template_inner($user->settings->{'lang'}->{'value'});
    my $vars = {};

    $vars->{'oldemailaddress'} = $old_email . $email_suffix;
    $vars->{'newemailaddress'} = $new_email . $email_suffix;
    
    $vars->{'max_token_age'} = MAX_TOKEN_AGE;
    $vars->{'token_ts'} = $token_ts;

    $vars->{'token'} = $token;
    $vars->{'emailaddress'} = $old_email . $email_suffix;

    my $message;
    $template->process("account/email/change-old.txt.tmpl", $vars, \$message)
      || ThrowTemplateError($template->error());

    MessageToMTA($message);

    $vars->{'token'} = $newtoken;
    $vars->{'emailaddress'} = $new_email . $email_suffix;

    $message = "";
    $template->process("account/email/change-new.txt.tmpl", $vars, \$message)
      || ThrowTemplateError($template->error());

    Bugzilla->template_inner("");
    MessageToMTA($message);
}

# Generates a random token, adds it to the tokens table, and sends it
# to the user with instructions for using it to change their password.
sub IssuePasswordToken {
    my $user = shift;
    my $dbh = Bugzilla->dbh;

    my $too_soon =
        $dbh->selectrow_array('SELECT 1 FROM tokens
                                WHERE userid = ?
                                  AND tokentype = ?
                                  AND issuedate > NOW() - ' .
                                      $dbh->sql_interval(10, 'MINUTE'),
                                undef, ($user->id, 'password'));

    ThrowUserError('too_soon_for_new_token', {'type' => 'password'}) if $too_soon;

    my ($token, $token_ts) = _create_token($user->id, 'password', $::ENV{'REMOTE_ADDR'});

    # Mail the user the token along with instructions for using it.
    my $template = Bugzilla->template_inner($user->settings->{'lang'}->{'value'});
    my $vars = {};

    $vars->{'token'} = $token;
    $vars->{'emailaddress'} = $user->email;
    $vars->{'max_token_age'} = MAX_TOKEN_AGE;
    $vars->{'token_ts'} = $token_ts;

    my $message = "";
    $template->process("account/password/forgotten-password.txt.tmpl", 
                                                               $vars, \$message)
      || ThrowTemplateError($template->error());

    Bugzilla->template_inner("");
    MessageToMTA($message);
}

sub issue_session_token {
    # Generates a random token, adds it to the tokens table, and returns
    # the token to the caller.

    my $data = shift;
    return _create_token(Bugzilla->user->id, 'session', $data);
}

sub CleanTokenTable {
    my $dbh = Bugzilla->dbh;
    $dbh->bz_lock_tables('tokens WRITE');
    $dbh->do('DELETE FROM tokens
              WHERE ' . $dbh->sql_to_days('NOW()') . ' - ' .
                        $dbh->sql_to_days('issuedate') . ' >= ?',
              undef, MAX_TOKEN_AGE);
    $dbh->bz_unlock_tables();
}

sub GenerateUniqueToken {
    # Generates a unique random token.  Uses generate_random_password 
    # for the tokens themselves and checks uniqueness by searching for
    # the token in the "tokens" table.  Gives up if it can't come up
    # with a token after about one hundred tries.
    my ($table, $column) = @_;

    my $token;
    my $duplicate = 1;
    my $tries = 0;
    $table ||= "tokens";
    $column ||= "token";

    my $dbh = Bugzilla->dbh;
    my $sth = $dbh->prepare("SELECT userid FROM $table WHERE $column = ?");

    while ($duplicate) {
        ++$tries;
        if ($tries > 100) {
            ThrowCodeError("token_generation_error");
        }
        $token = generate_random_password();
        $sth->execute($token);
        $duplicate = $sth->fetchrow_array;
    }
    return $token;
}

# Cancels a previously issued token and notifies the user.
# This should only happen when the user accidentally makes a token request
# or when a malicious hacker makes a token request on behalf of a user.
sub Cancel {
    my ($token, $cancelaction, $vars) = @_;
    my $dbh = Bugzilla->dbh;
    $vars ||= {};

    # Get information about the token being canceled.
    trick_taint($token);
    my ($issuedate, $tokentype, $eventdata, $userid) =
        $dbh->selectrow_array('SELECT ' . $dbh->sql_date_format('issuedate') . ',
                                      tokentype, eventdata, userid
                                 FROM tokens
                                WHERE token = ?',
                                undef, $token);

    # If we are canceling the creation of a new user account, then there
    # is no entry in the 'profiles' table.
    my $user = new Bugzilla::User($userid);

    $vars->{'emailaddress'} = $userid ? $user->email : $eventdata;
    $vars->{'remoteaddress'} = $::ENV{'REMOTE_ADDR'};
    $vars->{'token'} = $token;
    $vars->{'tokentype'} = $tokentype;
    $vars->{'issuedate'} = $issuedate;
    $vars->{'eventdata'} = $eventdata;
    $vars->{'cancelaction'} = $cancelaction;

    # Notify the user via email about the cancellation.
    my $template = Bugzilla->template_inner($user->settings->{'lang'}->{'value'});

    my $message;
    $template->process("account/cancel-token.txt.tmpl", $vars, \$message)
      || ThrowTemplateError($template->error());

    Bugzilla->template_inner("");
    MessageToMTA($message);

    # Delete the token from the database.
    delete_token($token);
}

sub DeletePasswordTokens {
    my ($userid, $reason) = @_;
    my $dbh = Bugzilla->dbh;

    detaint_natural($userid);
    my $tokens = $dbh->selectcol_arrayref('SELECT token FROM tokens
                                           WHERE userid = ? AND tokentype = ?',
                                           undef, ($userid, 'password'));

    foreach my $token (@$tokens) {
        Bugzilla::Token::Cancel($token, $reason);
    }
}

# Returns an email change token if the user has one. 
sub HasEmailChangeToken {
    my $userid = shift;
    my $dbh = Bugzilla->dbh;

    my $token = $dbh->selectrow_array('SELECT token FROM tokens
                                       WHERE userid = ?
                                       AND (tokentype = ? OR tokentype = ?) ' .
                                       $dbh->sql_limit(1),
                                       undef, ($userid, 'emailnew', 'emailold'));
    return $token;
}

# Returns the userid, issuedate and eventdata for the specified token
sub GetTokenData {
    my ($token) = @_;
    my $dbh = Bugzilla->dbh;

    return unless defined $token;
    $token = clean_text($token);
    trick_taint($token);

    return $dbh->selectrow_array(
        "SELECT userid, " . $dbh->sql_date_format('issuedate') . ", eventdata 
         FROM   tokens 
         WHERE  token = ?", undef, $token);
}

# Deletes specified token
sub delete_token {
    my ($token) = @_;
    my $dbh = Bugzilla->dbh;

    return unless defined $token;
    trick_taint($token);

    $dbh->bz_lock_tables('tokens WRITE');
    $dbh->do("DELETE FROM tokens WHERE token = ?", undef, $token);
    $dbh->bz_unlock_tables();
}

# Given a token, makes sure it comes from the currently logged in user
# and match the expected event. Returns 1 on success, else displays a warning.
# Note: this routine must not be called while tables are locked as it will try
# to lock some tables itself, see CleanTokenTable().
sub check_token_data {
    my ($token, $expected_action) = @_;
    my $user = Bugzilla->user;
    my $template = Bugzilla->template;
    my $cgi = Bugzilla->cgi;

    my ($creator_id, $date, $token_action) = GetTokenData($token);
    unless ($creator_id
            && $creator_id == $user->id
            && $token_action eq $expected_action)
    {
        # Something is going wrong. Ask confirmation before processing.
        # It is possible that someone tried to trick an administrator.
        # In this case, we want to know his name!
        require Bugzilla::User;

        my $vars = {};
        $vars->{'abuser'} = Bugzilla::User->new($creator_id)->identity;
        $vars->{'token_action'} = $token_action;
        $vars->{'expected_action'} = $expected_action;
        $vars->{'script_name'} = basename($0);

        # Now is a good time to remove old tokens from the DB.
        CleanTokenTable();

        # If no token was found, create a valid token for the given action.
        unless ($creator_id) {
            $token = issue_session_token($expected_action);
            $cgi->param('token', $token);
        }

        print $cgi->header();

        $template->process('admin/confirm-action.html.tmpl', $vars)
          || ThrowTemplateError($template->error());
        exit;
    }
    return 1;
}

################################################################################
# Internal Functions
################################################################################

# Generates a unique token and inserts it into the database
# Returns the token and the token timestamp
sub _create_token {
    my ($userid, $tokentype, $eventdata) = @_;
    my $dbh = Bugzilla->dbh;

    detaint_natural($userid) if defined $userid;
    trick_taint($tokentype);
    trick_taint($eventdata);

    $dbh->bz_lock_tables('tokens WRITE');

    my $token = GenerateUniqueToken();

    $dbh->do("INSERT INTO tokens (userid, issuedate, token, tokentype, eventdata)
        VALUES (?, NOW(), ?, ?, ?)", undef, ($userid, $token, $tokentype, $eventdata));

    $dbh->bz_unlock_tables();

    if (wantarray) {
        my (undef, $token_ts, undef) = GetTokenData($token);
        $token_ts = str2time($token_ts);
        return ($token, $token_ts);
    } else {
        return $token;
    }
}

1;

__END__

=head1 NAME

Bugzilla::Token - Provides different routines to manage tokens.

=head1 SYNOPSIS

    use Bugzilla::Token;

    Bugzilla::Token::issue_new_user_account_token($login_name);
    Bugzilla::Token::IssueEmailChangeToken($user, $old_email, $new_email);
    Bugzilla::Token::IssuePasswordToken($user);
    Bugzilla::Token::DeletePasswordTokens($user_id, $reason);
    Bugzilla::Token::Cancel($token, $cancelaction, $vars);

    Bugzilla::Token::CleanTokenTable();

    my $token = issue_session_token($event);
    check_token_data($token, $event)
    delete_token($token);

    my $token = Bugzilla::Token::GenerateUniqueToken($table, $column);
    my $token = Bugzilla::Token::HasEmailChangeToken($user_id);
    my ($token, $date, $data) = Bugzilla::Token::GetTokenData($token);

=head1 SUBROUTINES

=over

=item C<issue_new_user_account_token($login_name)>

 Description: Creates and sends a token per email to the email address
              requesting a new user account. It doesn't check whether
              the user account already exists. The user will have to
              use this token to confirm the creation of his user account.

 Params:      $login_name - The new login name requested by the user.

 Returns:     Nothing. It throws an error if the same user made the same
              request in the last few minutes.

=item C<sub IssueEmailChangeToken($user, $old_email, $new_email)>

 Description: Sends two distinct tokens per email to the old and new email
              addresses to confirm the email address change for the given
              user. These tokens remain valid for the next MAX_TOKEN_AGE days.

 Params:      $user      - User object of the user requesting a new
                           email address.
              $old_email - The current (old) email address of the user.
              $new_email - The new email address of the user.

 Returns:     Nothing.

=item C<IssuePasswordToken($user)>

 Description: Sends a token per email to the given user. This token
              can be used to change the password (e.g. in case the user
              cannot remember his password and wishes to enter a new one).

 Params:      $user - User object of the user requesting a new password.

 Returns:     Nothing. It throws an error if the same user made the same
              request in the last few minutes.

=item C<CleanTokenTable()>

 Description: Removes all tokens older than MAX_TOKEN_AGE days from the DB.
              This means that these tokens will now be considered as invalid.

 Params:      None.

 Returns:     Nothing.

=item C<GenerateUniqueToken($table, $column)>

 Description: Generates and returns a unique token. This token is unique
              in the $column of the $table. This token is NOT stored in the DB.

 Params:      $table (optional): The table to look at (default: tokens).
              $column (optional): The column to look at for uniqueness (default: token).

 Returns:     A token which is unique in $column.

=item C<Cancel($token, $cancelaction, $vars)>

 Description: Invalidates an existing token, generally when the token is used
              for an action which is not the one expected. An email is sent
              to the user who originally requested this token to inform him
              that this token has been invalidated (e.g. because an hacker
              tried to use this token for some malicious action).

 Params:      $token:        The token to invalidate.
              $cancelaction: The reason why this token is invalidated.
              $vars:         Some additional information about this action.

 Returns:     Nothing.

=item C<DeletePasswordTokens($user_id, $reason)>

 Description: Cancels all password tokens for the given user. Emails are sent
              to the user to inform him about this action.

 Params:      $user_id: The user ID of the user account whose password tokens
                        are canceled.
              $reason:  The reason why these tokens are canceled.

 Returns:     Nothing.

=item C<HasEmailChangeToken($user_id)>

 Description: Returns any existing token currently used for an email change
              for the given user.

 Params:      $user_id - A user ID.

 Returns:     A token if it exists, else undef.

=item C<GetTokenData($token)>

 Description: Returns all stored data for the given token.

 Params:      $token - A valid token.

 Returns:     The user ID, the date and time when the token was created and
              the (event)data stored with that token.

=back

=head2 Security related routines

The following routines have been written to be used together as described below,
although they can be used separately.

=over

=item C<issue_session_token($event)>

 Description: Creates and returns a token used internally.

 Params:      $event - The event which needs to be stored in the DB for future
                       reference/checks.

 Returns:     A unique token.

=item C<check_token_data($token, $event)>

 Description: Makes sure the $token has been created by the currently logged in
              user and to be used for the given $event. If this token is used for
              an unexpected action (i.e. $event doesn't match the information stored
              with the token), a warning is displayed asking whether the user really
              wants to continue. On success, it returns 1.
              This is the routine to use for security checks, combined with
              issue_session_token() and delete_token() as follows:

              1. First, create a token for some coming action.
              my $token = issue_session_token($action);
              2. Some time later, it's time to make sure that the expected action
                 is going to be executed, and by the expected user.
              check_token_data($token, $action);
              3. The check has been done and we no longer need this token.
              delete_token($token);

 Params:      $token - The token used for security checks.
              $event - The expected event to be run.

 Returns:     1 on success, else a warning is thrown.

=item C<delete_token($token)>

 Description: Deletes the specified token. No notification is sent.

 Params:      $token - The token to delete.

 Returns:     Nothing.

=back

=cut
