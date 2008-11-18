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
#                 Mads Bondo Dydensborg <mbd@dbc.dk>

package Bugzilla::WebService::Product;

use strict;
use base qw(Bugzilla::WebService);
use Bugzilla::Product;
use Bugzilla::User;
import SOAP::Data qw(type);

# Get the ids of the products the user can search
sub get_selectable_products {
    return {ids => [map {$_->id} @{Bugzilla->user->get_selectable_products}]}; 
}

# Get the ids of the products the user can enter bugs against
sub get_enterable_products {
    return {ids => [map {$_->id} @{Bugzilla->user->get_enterable_products}]}; 
}

# Get the union of the products the user can search and enter bugs against.
sub get_accessible_products {
    return {ids => [map {$_->id} @{Bugzilla->user->get_accessible_products}]}; 
}

# Get a list of actual products, based on list of ids
sub get_products {
    my ($self, $params) = @_;
    
    # Only products that are in the users accessible products, 
    # can be allowed to be returned
    my $accessible_products = Bugzilla->user->get_accessible_products;

    # Create a hash with the ids the user wants
    my %ids = map { $_ => 1 } @{$params->{ids}};
    
    # Return the intersection of this, by grepping the ids from 
    # accessible products.
    my @requested_accessible = grep { $ids{$_->id} } @$accessible_products;

    # Now create a result entry for each.
    my @products = 
        map {{
               internals   => $_,
               id          => type('int')->value($_->id),
               name        => type('string')->value($_->name),
               description => type('string')->value($_->description), 
             }
        } @requested_accessible;

    return { products => \@products };
}

1;

__END__

=head1 NAME

Bugzilla::Webservice::Product - The Product API

=head1 DESCRIPTION

This part of the Bugzilla API allows you to list the available Products and
get information about them.

=head1 METHODS

See L<Bugzilla::WebService> for a description of what B<STABLE>, B<UNSTABLE>,
and B<EXPERIMENTAL> mean, and for more information about error codes.

=head2 List Products

=over

=item C<get_selectable_products> B<UNSTABLE>

=over

=item B<Description>

Returns a list of the ids of the products the user can search on.

=item B<Params> (none)

=item B<Returns>    

A hash containing one item, C<ids>, that contains an array of product
ids.

=item B<Errors> (none)

=back

=item C<get_enterable_products> B<UNSTABLE>

=over

=item B<Description>

Returns a list of the ids of the products the user can enter bugs
against.

=item B<Params> (none)

=item B<Returns>

A hash containing one item, C<ids>, that contains an array of product
ids.

=item B<Errors> (none)

=back

=item C<get_accessible_products> B<UNSTABLE>

=over

=item B<Description>

Returns a list of the ids of the products the user can search or enter
bugs against.

=item B<Params> (none)

=item B<Returns>

A hash containing one item, C<ids>, that contains an array of product
ids.

=item B<Errors> (none)

=back

=item C<get_products> B<UNSTABLE>

=over

=item B<Description>

Returns a list of information about the products passed to it.

=item B<Params>

A hash containing one item, C<ids>, that is an array of product ids. 

=item B<Returns> 

A hash containing one item, C<products>, that is an array of
hashes. Each hash describes a product, and has the following items:
C<id>, C<name>, C<description>, and C<internals>. The C<id> item is
the id of the product. The C<name> item is the name of the
product. The C<description> is the description of the
product. Finally, the C<internals> is an internal representation of
the product.

Note, that if the user tries to access a product that is not in the
list of accessible products for the user, or a product that does not
exist, that is silently ignored, and no information about that product
is returned.

=item B<Errors> (none)

=back

=back

