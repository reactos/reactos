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
#


package Bugzilla::User::Setting::Skin;

use strict;

use base qw(Bugzilla::User::Setting);

use Bugzilla::Constants;
use File::Spec::Functions;
use File::Basename;

use constant BUILTIN_SKIN_NAMES => ['standard'];

sub legal_values {
    my ($self) = @_;

    return $self->{'legal_values'} if defined $self->{'legal_values'};

    my $dirbase = bz_locations()->{'skinsdir'} . '/contrib';
    # Avoid modification of the list BUILTIN_SKIN_NAMES points to by copying the
    # list over instead of simply writing $legal_values = BUILTIN_SKIN_NAMES.
    my @legal_values = @{(BUILTIN_SKIN_NAMES)};

    foreach my $direntry (glob(catdir($dirbase, '*'))) {
        if (-d $direntry) {
            # Stylesheet set
            push(@legal_values, basename($direntry));
        }
        elsif ($direntry =~ /\.css$/) {
            # Single-file stylesheet
            push(@legal_values, basename($direntry));
        }
    }

    return $self->{'legal_values'} = \@legal_values;
}

1;

__END__

=head1 NAME

Bugzilla::User::Setting::Skin - Object for a user preference setting for skins

=head1 DESCRIPTION

Skin.pm extends Bugzilla::User::Setting and implements a class specialized for
skins settings.

=head1 METHODS

=over

=item C<legal_values()>

Description: Returns all legal skins
Params:      none
Returns:     A reference to an array containing the names of all legal skins

=back
