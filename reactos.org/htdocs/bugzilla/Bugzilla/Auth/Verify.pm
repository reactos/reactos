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
# Contributor(s): Max Kanat-Alexander <mkanat@bugzilla.org>

package Bugzilla::Auth::Verify;

use strict;
use fields qw();

use Bugzilla::Constants;
use Bugzilla::Error;
use Bugzilla::User;
use Bugzilla::Util;

use constant user_can_create_account => 1;

sub new {
    my ($class, $login_type) = @_;
    my $self = fields::new($class);
    return $self;
}

sub can_change_password {
    return $_[0]->can('change_password');
}

sub create_or_update_user {
    my ($self, $params) = @_;
    my $dbh = Bugzilla->dbh;

    my $extern_id = $params->{extern_id};
    my $username  = $params->{bz_username} || $params->{username};
    my $password  = $params->{password} || '*';
    my $real_name = $params->{realname} || '';
    my $user_id   = $params->{user_id};

    # A passed-in user_id always overrides anything else, for determining
    # what account we should return.
    if (!$user_id) {
        my $username_user_id = login_to_id($username || '');
        my $extern_user_id;
        if ($extern_id) {
            trick_taint($extern_id);
            $extern_user_id = $dbh->selectrow_array('SELECT userid
                 FROM profiles WHERE extern_id = ?', undef, $extern_id);
        }

        # If we have both a valid extern_id and a valid username, and they are
        # not the same id, then we have a conflict.
        if ($username_user_id && $extern_user_id
            && $username_user_id ne $extern_user_id)
        {
            my $extern_name = Bugzilla::User->new($extern_user_id)->login;
            return { failure => AUTH_ERROR, error => "extern_id_conflict",
                     details => {extern_id   => $extern_id,
                                 extern_user => $extern_name,
                                 username    => $username} };
        }

        # If we have a valid username, but no valid id,
        # then we have to create the user. This happens when we're
        # passed only a username, and that username doesn't exist already.
        if ($username && !$username_user_id && !$extern_user_id) {
            validate_email_syntax($username)
              || return { failure => AUTH_ERROR, 
                          error   => 'auth_invalid_email',
                          details => {addr => $username} };
            # Usually we'd call validate_password, but external authentication
            # systems might follow different standards than ours. So in this
            # place here, we call trick_taint without checks.
            trick_taint($password);

            # XXX Theoretically this could fail with an error, but the fix for
            # that is too involved to be done right now.
            my $user = Bugzilla::User->create({ 
                login_name    => $username, 
                cryptpassword => $password,
                realname      => $real_name});
            $username_user_id = $user->id;
        }

        # If we have a valid username id and an extern_id, but no valid
        # extern_user_id, then we have to set the user's extern_id.
        if ($extern_id && $username_user_id && !$extern_user_id) {
            $dbh->do('UPDATE profiles SET extern_id = ? WHERE userid = ?',
                     undef, $extern_id, $username_user_id);
        }

        # Finally, at this point, one of these will give us a valid user id.
        $user_id = $extern_user_id || $username_user_id;
    }

    # If we still don't have a valid user_id, then we weren't passed
    # enough information in $params, and we should die right here.
    ThrowCodeError('bad_arg', {argument => 'params', function =>
        'Bugzilla::Auth::Verify::create_or_update_user'})
        unless $user_id;

    my $user = new Bugzilla::User($user_id);

    # Now that we have a valid User, we need to see if any data has to be
    # updated.
    if ($username && lc($user->login) ne lc($username)) {
        validate_email_syntax($username)
          || return { failure => AUTH_ERROR, error => 'auth_invalid_email',
                      details => {addr => $username} };
        $dbh->do('UPDATE profiles SET login_name = ? WHERE userid = ?',
                 undef, $username, $user->id);
    }
    if ($real_name && $user->name ne $real_name) {
        # $real_name is more than likely tainted, but we only use it
        # in a placeholder and we never use it after this.
        trick_taint($real_name);
        $dbh->do('UPDATE profiles SET realname = ? WHERE userid = ?',
                 undef, $real_name, $user->id);
    }

    return { user => $user };
}

1;

__END__

=head1 NAME

Bugzilla::Auth::Verify - An object that verifies usernames and passwords.

=head1 DESCRIPTION

Bugzilla::Auth::Verify provides the "Verifier" part of the Bugzilla 
login process. (For details, see the "STRUCTURE" section of 
L<Bugzilla::Auth>.)

It is mostly an abstract class, requiring subclasses to implement
most methods.

Note that callers outside of the C<Bugzilla::Auth> package should never
create this object directly. Just create a C<Bugzilla::Auth> object
and call C<login> on it.

=head1 VERIFICATION METHODS

These are the methods that have to do with the actual verification.

Subclasses MUST implement these methods.

=over 4

=item C<check_credentials($login_data)>

Description: Checks whether or not a username is valid.
Params:      $login_data - A C<$login_data> hashref, as described in
                           L<Bugzilla::Auth>.
                           This C<$login_data> hashref MUST contain
                           C<username>, and SHOULD also contain
                           C<password>.
Returns:     A C<$login_data> hashref with C<bz_username> set. This
             method may also set C<realname>. It must avoid changing
             anything that is already set.

=back

=head1 MODIFICATION METHODS

These are methods that change data in the actual authentication backend.

These methods are optional, they do not have to be implemented by
subclasses.

=over 4

=item C<create_or_update_user($login_data)>

Description: Automatically creates a user account in the database
             if it doesn't already exist, or updates the account
             data if C<$login_data> contains newer information.

Params:      $login_data - A C<$login_data> hashref, as described in
                           L<Bugzilla::Auth>.
                           This C<$login_data> hashref MUST contain
                           either C<user_id>, C<bz_username>, or
                           C<username>. If both C<username> and C<bz_username>
                           are specified, C<bz_username> is used as the
                           login name of the user to create in the database.
                           It MAY also contain C<extern_id>, in which
                           case it still MUST contain C<bz_username> or
                           C<username>.
                           It MAY contain C<password> and C<realname>.

Returns:     A hashref with one element, C<user>, which is a
             L<Bugzilla::User> object. May also return a login error
             as described in L<Bugzilla::Auth>.

Note:        This method is not abstract, it is actually implemented
             and creates accounts in the Bugzilla database. Subclasses
             should probably all call the C<Bugzilla::Auth::Verify>
             version of this function at the end of their own
             C<create_or_update_user>.

=item C<change_password($user, $password)>

Description: Modifies the user's password in the authentication backend.
Params:      $user - A L<Bugzilla::User> object representing the user
                     whose password we want to change.
             $password - The user's new password.
Returns:     Nothing.

=back

=head1 INFO METHODS

These are methods that describe the capabilities of this object.
These are all no-parameter methods that return either C<true> or 
C<false>.

=over 4

=item C<user_can_create_account>

Whether or not users can manually create accounts in this type of
account source. Defaults to C<true>.

=back
