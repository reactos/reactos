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

package Bugzilla::WebService::Bugzilla;

use strict;
use base qw(Bugzilla::WebService);
use Bugzilla::Constants;
import SOAP::Data qw(type);

use Time::Zone;

sub version {
    return { version => type('string')->value(BUGZILLA_VERSION) };
}

sub timezone {
    my $offset = tz_offset();
    $offset = (($offset / 60) / 60) * 100;
    $offset = sprintf('%+05d', $offset);
    return { timezone => type('string')->value($offset) };
}

1;

__END__

=head1 NAME

Bugzilla::WebService::Bugzilla - Global functions for the webservice interface.

=head1 DESCRIPTION

This provides functions that tell you about Bugzilla in general.

=head1 METHODS

See L<Bugzilla::WebService> for a description of what B<STABLE>, B<UNSTABLE>,
and B<EXPERIMENTAL> mean.

=over

=item C<version> B<EXPERIMENTAL>

=over

=item B<Description>

Returns the current version of Bugzilla.

=item B<Params> (none)

=item B<Returns>

A hash with a single item, C<version>, that is the version as a
string.

=item B<Errors> (none)

=back

=item C<timezone> B<EXPERIMENTAL>

=over

=item B<Description>

Returns the timezone of the server Bugzilla is running on. This is
important because all dates/times that the webservice interface
returns will be in this timezone.

=item B<Params> (none)

=item B<Returns>

A hash with a single item, C<timezone>, that is the timezone as a
string in (+/-)XXXX (RFC 2822) format.

=back

=back
