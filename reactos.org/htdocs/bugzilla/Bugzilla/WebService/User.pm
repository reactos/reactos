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
# Contributor(s): Marc Schumann <wurblzap@gmail.com>
#                 Max Kanat-Alexander <mkanat@bugzilla.org>
#                 Mads Bondo Dydensborg <mbd@dbc.dk>

package Bugzilla::WebService::User;

use strict;
use base qw(Bugzilla::WebService);

import SOAP::Data qw(type); 

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Error;
use Bugzilla::User;
use Bugzilla::Util qw(trim);
use Bugzilla::Token;

##############
# User Login #
##############

sub login {
    my ($self, $params) = @_;
    my $remember = $params->{remember};
    
    # Convert $remember from a boolean 0/1 value to a CGI-compatible one.
    if (defined($remember)) {
        $remember = $remember? 'on': '';
    }
    else {
        # Use Bugzilla's default if $remember is not supplied.
        $remember =
            Bugzilla->params->{'rememberlogin'} eq 'defaulton'? 'on': '';
    }

    # Make sure the CGI user info class works if necessary.
    my $cgi = Bugzilla->cgi;
    $cgi->param('Bugzilla_login', $params->{login});
    $cgi->param('Bugzilla_password', $params->{password});
    $cgi->param('Bugzilla_remember', $remember);

    Bugzilla->login;
    return { id => type('int')->value(Bugzilla->user->id) };
}

sub logout {
    my $self = shift;
    Bugzilla->logout;
    return undef;
}

#################
# User Creation #
#################

sub offer_account_by_email {
    my $self = shift;
    my ($params) = @_;
    my $email = trim($params->{email})
        || ThrowCodeError('param_required', { param => 'email' });

    my $createexp = Bugzilla->params->{'createemailregexp'};
    if (!$createexp || $email !~ /$createexp/) {
        ThrowUserError("account_creation_disabled");
    }

    $email = Bugzilla::User->check_login_name_for_creation($email);

    # Create and send a token for this new account.
    Bugzilla::Token::issue_new_user_account_token($email);

    return undef;
}

sub create {
    my $self = shift;
    my ($params) = @_;

    Bugzilla->user->in_group('editusers') 
        || ThrowUserError("auth_failure", { group  => "editusers",
                                            action => "add",
                                            object => "users"});

    my $email = trim($params->{email})
        || ThrowCodeError('param_required', { param => 'email' });
    my $realname = trim($params->{full_name});
    my $password = trim($params->{password}) || '*';

    my $user = Bugzilla::User->create({
        login_name    => $email,
        realname      => $realname,
        cryptpassword => $password
    });

    return { id => type('int')->value($user->id) };
}

1;

__END__

=head1 NAME

Bugzilla::Webservice::User - The User Account and Login API

=head1 DESCRIPTION

This part of the Bugzilla API allows you to create User Accounts and
log in/out using an existing account.

=head1 METHODS

See L<Bugzilla::WebService> for a description of what B<STABLE>, B<UNSTABLE>,
and B<EXPERIMENTAL> mean, and for more information about error codes.

=head2 Logging In and Out

=over

=item C<login> B<EXPERIMENTAL>

=over

=item B<Description>

Logging in, with a username and password, is required for many
Bugzilla installations, in order to search for bugs, post new bugs,
etc. This method logs in an user.

=item B<Params>

=over

=item C<login> (string) - The user's login name. 

=item C<password> (string) - The user's password.

=item C<remember> (bool) B<Optional> - if the cookies returned by the
call to login should expire with the session or not.  In order for
this option to have effect the Bugzilla server must be configured to
allow the user to set this option - the Bugzilla parameter
I<rememberlogin> must be set to "defaulton" or
"defaultoff". Addionally, the client application must implement
management of cookies across sessions.

=back

=item B<Returns>

On success, a hash containing one item, C<id>, the numeric id of the
user that was logged in.  A set of http cookies is also sent with the
response.  These cookies must be sent along with any future requests
to the webservice, for the duration of the session.

=item B<Errors>

=over

=item 300 (Invalid Username or Password)

The username does not exist, or the password is wrong.

=item 301 (Account Disabled)

The account has been disabled.  A reason may be specified with the
error.

=back

=back

=item C<logout> B<EXPERIMENTAL>

=over

=item B<Description>

Log out the user. Does nothing if there is no user logged in.

=item B<Params> (none)

=item B<Returns> (nothing)

=item B<Errors> (none)

=back

=back

=head2 Account Creation

=over

=item C<offer_account_by_email> B<EXPERIMENTAL>

=over

=item B<Description>

Sends an email to the user, offering to create an account.  The user
will have to click on a URL in the email, and choose their password
and real name.

This is the recommended way to create a Bugzilla account.

=item B<Param>

=over

=item C<email> (string) - the email to send the offer to.

=back

=item B<Returns> (nothing)

=item B<Errors>

=over

=item 500 (Illegal Email Address)

This Bugzilla does not allow you to create accounts with the format of
email address you specified. Account creation may be entirely disabled.

=item 501 (Account Already Exists)

An account with that email address already exists in Bugzilla.

=back

=back

=item C<create> B<EXPERIMENTAL>

=over

=item B<Description>

Creates a user account directly in Bugzilla, password and all.
Instead of this, you should use L</offer_account_by_email> when
possible, because that makes sure that the email address specified can
actually receive an email. This function does not check that.

=item B<Params>

=over

=item C<email> (string) - The email address for the new user.

=item C<full_name> (string) B<Optional> - The user's full name. Will
be set to empty if not specified.

=item C<password> (string) B<Optional> - The password for the new user
account, in plain text.  It will be stripped of leading and trailing
whitespace.  If blank or not specified, the newly created account will
exist in Bugzilla, but will not be allowed to log in using DB
authentication until a password is set either by the user (through
resetting their password) or by the administrator.

=back

=item B<Returns>

A hash containing one item, C<id>, the numeric id of the user that was
created.

=item B<Errors>

The same as L</offer_account_by_email>. If a password is specified,
the function may also throw:

=over

=item 502 (Password Too Short)

The password specified is too short. (Usually, this means the
password is under three characters.)

=item 503 (Password Too Long)

The password specified is too long. (Usually, this means the
password is over ten characters.)

=back

=back

=back
