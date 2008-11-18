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
# Contributor(s): Bradley Baetz <bbaetz@acm.org>
#                 Erik Stambaugh <erik@dasbistro.com>
#                 Max Kanat-Alexander <mkanat@bugzilla.org>

package Bugzilla::Auth;

use strict;
use fields qw(
    _info_getter
    _verifier
    _persister
);

use Bugzilla::Constants;
use Bugzilla::Error;
use Bugzilla::Auth::Login::Stack;
use Bugzilla::Auth::Verify::Stack;
use Bugzilla::Auth::Persist::ROSCMS;

sub new {
    my ($class, $params) = @_;
    my $self = fields::new($class);

    $params            ||= {};
    $params->{Login}   ||= Bugzilla->params->{'user_info_class'} . ',Cookie';
    $params->{Verify}  ||= Bugzilla->params->{'user_verify_class'};

    $self->{_info_getter} = new Bugzilla::Auth::Login::Stack($params->{Login});
    $self->{_verifier} = new Bugzilla::Auth::Verify::Stack($params->{Verify});
    # If we ever have any other login persistence methods besides cookies,
    # this could become more configurable.
    $self->{_persister} = new Bugzilla::Auth::Persist::ROSCMS();

    return $self;
}

sub login {
    my ($self, $type) = @_;
    my $dbh = Bugzilla->dbh;

    # Get login info from the cookie, form, environment variables, etc.
    my $login_info = $self->{_info_getter}->get_login_info();

    if ($login_info->{failure}) {
        return $self->_handle_login_result($login_info, $type);
    }

    # Now verify his username and password against the DB, LDAP, etc.
    if ($self->{_info_getter}->{successful}->requires_verification) {
        $login_info = $self->{_verifier}->check_credentials($login_info);
        if ($login_info->{failure}) {
            return $self->_handle_login_result($login_info, $type);
        }
        $login_info =
          $self->{_verifier}->{successful}->create_or_update_user($login_info);
    }
    else {
        $login_info = $self->{_verifier}->create_or_update_user($login_info);
    }

    if ($login_info->{failure}) {
        return $self->_handle_login_result($login_info, $type);
    }

    # Make sure the user isn't disabled.
    my $user = $login_info->{user};
    if ($user->disabledtext) {
        return $self->_handle_login_result({ failure => AUTH_DISABLED,
                                              user    => $user }, $type);
    }
    $user->set_authorizer($self);

    return $self->_handle_login_result($login_info, $type);
}

sub can_change_password {
    my ($self) = @_;
    my $verifier = $self->{_verifier}->{successful};
    $verifier  ||= $self->{_verifier};
    my $getter   = $self->{_info_getter}->{successful};
    $getter      = $self->{_info_getter} 
        if (!$getter || $getter->isa('Bugzilla::Auth::Login::Cookie'));
    return $verifier->can_change_password &&
           $getter->user_can_create_account;
}

sub can_login {
    my ($self) = @_;
    my $getter = $self->{_info_getter}->{successful};
    $getter    = $self->{_info_getter}
        if (!$getter || $getter->isa('Bugzilla::Auth::Login::Cookie'));
    return $getter->can_login;
}

sub can_logout {
    my ($self) = @_;
    my $getter = $self->{_info_getter}->{successful};
    # If there's no successful getter, we're not logged in, so of
    # course we can't log out!
    return 0 unless $getter;
    return $getter->can_logout;
}

sub user_can_create_account {
    my ($self) = @_;
    my $verifier = $self->{_verifier}->{successful};
    $verifier  ||= $self->{_verifier};
    my $getter   = $self->{_info_getter}->{successful};
    $getter      = $self->{_info_getter}
        if (!$getter || $getter->isa('Bugzilla::Auth::Login::Cookie'));
    return $verifier->user_can_create_account
           && $getter->user_can_create_account;
}

sub can_change_email {
    return $_[0]->user_can_create_account;
}

sub _handle_login_result {
    my ($self, $result, $login_type) = @_;
    my $dbh = Bugzilla->dbh;

    my $user      = $result->{user};
    my $fail_code = $result->{failure};

    if (!$fail_code) {
        if ($self->{_info_getter}->{successful}->requires_persistence) {
            $self->{_persister}->persist_login($user);
        }
    }
    elsif ($fail_code == AUTH_ERROR) {
        ThrowCodeError($result->{error}, $result->{details});
    }
    elsif ($fail_code == AUTH_NODATA) {
        if ($login_type == LOGIN_REQUIRED) {
            # This seems like as good as time as any to get rid of
            # old crufty junk in the logincookies table.  Get rid
            # of any entry that hasn't been used in a month.
            $dbh->do("DELETE FROM logincookies WHERE " .
                     $dbh->sql_to_days('NOW()') . " - " .
                     $dbh->sql_to_days('lastused') . " > 30");
            $self->{_info_getter}->fail_nodata($self);
        }
        # Otherwise, we just return the "default" user.
        $user = Bugzilla->user;
    }
    # The username/password may be wrong
    # Don't let the user know whether the username exists or whether
    # the password was just wrong. (This makes it harder for a cracker
    # to find account names by brute force)
    elsif (($fail_code == AUTH_LOGINFAILED) || ($fail_code == AUTH_NO_SUCH_USER)) {
        ThrowUserError("invalid_username_or_password");
    }
    # The account may be disabled
    elsif ($fail_code == AUTH_DISABLED) {
        $self->{_persister}->logout();
        # XXX This is NOT a good way to do this, architecturally.
        $self->{_persister}->clear_browser_cookies();
        # and throw a user error
        ThrowUserError("account_disabled",
            {'disabled_reason' => $result->{user}->disabledtext});
    }
    # If we get here, then we've run out of options, which shouldn't happen.
    else {
        ThrowCodeError("authres_unhandled", { value => $fail_code });
    }

    return $user;
}

1;

__END__

=head1 NAME

Bugzilla::Auth - An object that authenticates the login credentials for
                 a user.

=head1 DESCRIPTION

Handles authentication for Bugzilla users.

Authentication from Bugzilla involves two sets of modules. One set is
used to obtain the username/password (from CGI, email, etc), and the 
other set uses this data to authenticate against the datasource 
(the Bugzilla DB, LDAP, PAM, etc.).

Modules for obtaining the username/password are subclasses of 
L<Bugzilla::Auth::Login>, and modules for authenticating are subclasses
of L<Bugzilla::Auth::Verify>.

=head1 AUTHENTICATION ERROR CODES

Whenever a method in the C<Bugzilla::Auth> family fails in some way,
it will return a hashref containing at least a single key called C<failure>.
C<failure> will point to an integer error code, and depending on the error
code the hashref may contain more data.

The error codes are explained here below.

=head2 C<AUTH_NODATA>

Insufficient login data was provided by the user. This may happen in several
cases, such as cookie authentication when the cookie is not present.

=head2 C<AUTH_ERROR>

An error occurred when trying to use the login mechanism.

The hashref will also contain an C<error> element, which is the name
of an error from C<template/en/default/global/code-error.html> --
the same type of error that would be thrown by 
L<Bugzilla::Error::ThrowCodeError>.

The hashref *may* contain an element called C<details>, which is a hashref
that should be passed to L<Bugzilla::Error::ThrowCodeError> as the 
various fields to be used in the error message.

=head2 C<AUTH_LOGINFAILED>

An incorrect username or password was given.

=head2 C<AUTH_NO_SUCH_USER>

This is an optional more-specific version of C<AUTH_LOGINFAILED>.
Modules should throw this error when they discover that the
requested user account actually does not exist, according to them.

That is, for example, L<Bugzilla::Auth::Verify::LDAP> would throw
this if the user didn't exist in LDAP.

The difference between C<AUTH_NO_SUCH_USER> and C<AUTH_LOGINFAILED>
should never be communicated to the user, for security reasons.

=head2 C<AUTH_DISABLED>

The user successfully logged in, but their account has been disabled.
Usually this is throw only by C<Bugzilla::Auth::login>.

=head1 LOGIN TYPES

The C<login> function (below) can do different types of login, depending
on what constant you pass into it:

=head2 C<LOGIN_OPTIONAL>

A login is never required to access this data. Attempting to login is
still useful, because this allows the page to be personalised. Note that
an incorrect login will still trigger an error, even though the lack of
a login will be OK.

=head2 C<LOGIN_NORMAL>

A login may or may not be required, depending on the setting of the
I<requirelogin> parameter. This is the default if you don't specify a
type.

=head2 C<LOGIN_REQUIRED>

A login is always required to access this data.

=head1 METHODS

These are methods that can be called on a C<Bugzilla::Auth> object 
itself.

=head2 Login

=over 4

=item C<login($type)>

Description: Logs a user in. For more details on how this works
             internally, see the section entitled "STRUCTURE."
Params:      $type - One of the Login Types from above.
Returns:     An authenticated C<Bugzilla::User>. Or, if the type was
             not C<LOGIN_REQUIRED>, then we return an
             empty C<Bugzilla::User> if no login data was passed in.

=back

=head2 Info Methods

These are methods that give information about the Bugzilla::Auth object.

=over 4

=item C<can_change_password>

Description: Tells you whether or not the current login system allows
             changing passwords.
Params:      None
Returns:     C<true> if users and administrators should be allowed to
             change passwords, C<false> otherwise.

=item C<can_login>

Description: Tells you whether or not the current login system allows
             users to log in through the web interface.
Params:      None
Returns:     C<true> if users can log in through the web interface,
             C<false> otherwise.

=item C<can_logout>

Description: Tells you whether or not the current login system allows
             users to log themselves out.
Params:      None
Returns:     C<true> if users can log themselves out, C<false> otherwise.
             If a user isn't logged in, we always return C<false>.

=item C<user_can_create_account>

Description: Tells you whether or not users are allowed to manually create
             their own accounts, based on the current login system in use.
             Note that this doesn't check the C<createemailregexp>
             parameter--you have to do that by yourself in your code.
Params:      None
Returns:     C<true> if users are allowed to create new Bugzilla accounts,
             C<false> otherwise.

=item C<can_change_email>

Description: Whether or not the current login system allows users to
             change their own email address.
Params:      None
Returns:     C<true> if users can change their own email address,
             C<false> otherwise.

=back

=head1 STRUCTURE

This section is mostly interesting to developers who want to implement
a new authentication type. It describes the general structure of the
Bugzilla::Auth family, and how the C<login> function works.

A C<Bugzilla::Auth> object is essentially a collection of a few other
objects: the "Info Getter," the "Verifier," and the "Persistence 
Mechanism."

They are used inside the C<login> function in the following order:

=head2 The Info Getter

This is a C<Bugzilla::Auth::Login> object. Basically, it gets the
username and password from the user, somehow. Or, it just gets enough
information to uniquely identify a user, and passes that on down the line.
(For example, a C<user_id> is enough to uniquely identify a user,
even without a username and password.)

Some Info Getters don't require any verification. For example, if we got
the C<user_id> from a Cookie, we don't need to check the username and 
password.

If an Info Getter returns only a C<user_id> and no username/password,
then it MUST NOT require verification. If an Info Getter requires
verfication, then it MUST return at least a C<username>.

=head2 The Verifier

This verifies that the username and password are valid.

It's possible that some methods of verification don't require a password.

=head2 The Persistence Mechanism

This makes it so that the user doesn't have to log in on every page.
Normally this object just sends a cookie to the user's web browser,
as that's the most common method of "login persistence."

=head2 Other Things We Do

After we verify the username and password, sometimes we automatically
create an account in the Bugzilla database, for certain authentication
types. We use the "Account Source" to get data about the user, and
create them in the database. (Or, if their data has changed since the
last time they logged in, their data gets updated.)

=head2 The C<$login_data> Hash

All of the C<Bugzilla::Auth::Login> and C<Bugzilla::Auth::Verify>
methods take an argument called C<$login_data>. This is basically
a hash that becomes more and more populated as we go through the
C<login> function.

All C<Bugzilla::Auth::Login> and C<Bugzilla::Auth::Verify> methods
also *return* the C<$login_data> structure, when they succeed. They
may have added new data to it.

For all C<Bugzilla::Auth::Login> and C<Bugzilla::Auth::Verify> methods,
the rule is "you must return the same hashref you were passed in." You can
modify the hashref all you want, but you can't create a new one. The only
time you can return a new one is if you're returning some error code
instead of the C<$login_data> structure.

Each C<Bugzilla::Auth::Login> or C<Bugzilla::Auth::Verify> method
explains in its documentation which C<$login_data> elements are
required by it, and which are set by it.

Here are all of the elements that *may* be in C<$login_data>:

=over 4

=item C<user_id>

A Bugzilla C<user_id> that uniquely identifies a user.

=item C<username>

The username that was provided by the user.

=item C<bz_username>

The username of this user inside of Bugzilla. Sometimes this differs from
C<username>.

=item C<password>

The password provided by the user.

=item C<realname>

The real name of the user.

=item C<extern_id>

Some string that uniquely identifies the user in an external account 
source. If this C<extern_id> already exists in the database with
a different username, the username will be *changed* to be the
username specified in this C<$login_data>.

That is, let's my extern_id is C<mkanat>. I already have an account
in Bugzilla with the username of C<mkanat@foo.com>. But this time,
when I log in, I have an extern_id of C<mkanat> and a C<username>
of C<mkanat@bar.org>. So now, Bugzilla will automatically change my 
username to C<mkanat@bar.org> instead of C<mkanat@foo.com>.

=item C<user>

A L<Bugzilla::User> object representing the authenticated user. 
Note that C<Bugzilla::Auth::login> may modify this object at various points.

=back


