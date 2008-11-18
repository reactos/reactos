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

package Bugzilla::Auth::Login;

use strict;
use fields qw();

# Determines whether or not a user can logout. It's really a subroutine,
# but we implement it here as a constant. Override it in subclasses if
# that particular type of login method cannot log out.
use constant can_logout => 1;
use constant can_login  => 1;
use constant requires_persistence  => 1;
use constant requires_verification => 1;
use constant user_can_create_account => 0;

sub new {
    my ($class) = @_;
    my $self = fields::new($class);
    return $self;
}

1;

__END__

=head1 NAME

Bugzilla::Auth::Login - Gets username/password data from the user.

=head1 DESCRIPTION

Bugzilla::Auth::Login is used to get information that uniquely identifies
a user and allows us to authorize their Bugzilla access.

It is mostly an abstract class, requiring subclasses to implement
most methods.

Note that callers outside of the C<Bugzilla::Auth> package should never
create this object directly. Just create a C<Bugzilla::Auth> object
and call C<login> on it.

=head1 LOGIN METHODS

These are methods that have to do with getting the actual login data
from the user or handling a login somehow.

These methods are abstract -- they MUST be implemented by a subclass.

=over 4

=item C<get_login_info()>

Description: Gets a username/password from the user, or some other
             information that uniquely identifies them.
Params:      None
Returns:     A C<$login_data> hashref. (See L<Bugzilla::Auth> for details.)
             The hashref MUST contain: C<user_id> *or* C<username>
             If this is a login method that requires verification,
             the hashref MUST contain C<password>.
             The hashref MAY contain C<realname> and C<extern_id>.

=item C<fail_nodata()>

Description: This function is called when Bugzilla doesn't get
             a username/password and the login type is C<LOGIN_REQUIRED>
             (See L<Bugzilla::Auth> for a description of C<LOGIN_REQUIRED>).
             That is, this handles C<AUTH_NODATA> in that situation.

             This function MUST stop CGI execution when it is complete.
             That is, it must call C<exit> or C<ThrowUserError> or some
             such thing.
Params:      None
Returns:     Never Returns.

=back

=head1 INFO METHODS

These are methods that describe the capabilities of this 
C<Bugzilla::Auth::Login> object. These are all no-parameter
methods that return either C<true> or C<false>.

=over 4

=item C<can_logout>

Whether or not users can log out if they logged in using this
object. Defaults to C<true>.

=item C<can_login>

Whether or not users can log in through the web interface using
this object. Defaults to C<true>.

=item C<requires_persistence>

Whether or not we should send the user a cookie if they logged in with
this method. Defaults to C<true>.

=item C<requires_verification>

Whether or not we should check the username/password that we
got from this login method. Defaults to C<true>.

=item C<user_can_create_account>

Whether or not users can create accounts, if this login method is
currently being used by the system. Defaults to C<false>.

=back
