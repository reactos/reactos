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
# Contributor(s): Bradley Baetz <bbaetz@student.usyd.edu.au>
#                 Joel Peshkin <bugreport@peshkin.net>
#

package Bugzilla::Template::Plugin::User;

use strict;

use base qw(Template::Plugin);

use Bugzilla::User;

sub new {
    my ($class, $context) = @_;

    return bless {}, $class;
}

sub AUTOLOAD {
    my $class = shift;
    our $AUTOLOAD;

    $AUTOLOAD =~ s/^.*:://;

    return if $AUTOLOAD eq 'DESTROY';

    return Bugzilla::User->$AUTOLOAD(@_);
}

1;

__END__

=head1 NAME

Bugzilla::Template::Plugin::User

=head1 DESCRIPTION

Template Toolkit plugin to allow access to the C<User>
object.

=head1 SEE ALSO

L<Bugzilla::User>, L<Template::Plugin>

