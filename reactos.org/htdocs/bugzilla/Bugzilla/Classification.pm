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
# Contributor(s): Tiago R. Mello <timello@async.com.br>
#

use strict;

package Bugzilla::Classification;

use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::Product;

###############################
####    Initialization     ####
###############################

use constant DB_COLUMNS => qw(
    classifications.id
    classifications.name
    classifications.description
    classifications.sortkey
);

our $columns = join(", ", DB_COLUMNS);

###############################
####       Methods         ####
###############################

sub new {
    my $invocant = shift;
    my $class = ref($invocant) || $invocant;
    my $self = {};
    bless($self, $class);
    return $self->_init(@_);
}

sub _init {
    my $self = shift;
    my ($param) = @_;
    my $dbh = Bugzilla->dbh;

    my $id = $param unless (ref $param eq 'HASH');
    my $classification;

    if (defined $id) {
        detaint_natural($id)
          || ThrowCodeError('param_must_be_numeric',
                            {function => 'Bugzilla::Classification::_init'});

        $classification = $dbh->selectrow_hashref(qq{
            SELECT $columns FROM classifications
            WHERE id = ?}, undef, $id);

    } elsif (defined $param->{'name'}) {

        trick_taint($param->{'name'});
        $classification = $dbh->selectrow_hashref(qq{
            SELECT $columns FROM classifications
            WHERE name = ?}, undef, $param->{'name'});
    } else {
        ThrowCodeError('bad_arg',
            {argument => 'param',
             function => 'Bugzilla::Classification::_init'});
    }

    return undef unless (defined $classification);

    foreach my $field (keys %$classification) {
        $self->{$field} = $classification->{$field};
    }
    return $self;
}

sub product_count {
    my $self = shift;
    my $dbh = Bugzilla->dbh;

    if (!defined $self->{'product_count'}) {
        $self->{'product_count'} = $dbh->selectrow_array(q{
            SELECT COUNT(*) FROM products
            WHERE classification_id = ?}, undef, $self->id) || 0;
    }
    return $self->{'product_count'};
}

sub products {
    my $self = shift;
    my $dbh = Bugzilla->dbh;

    if (!$self->{'products'}) {
        my $product_ids = $dbh->selectcol_arrayref(q{
            SELECT id FROM products
            WHERE classification_id = ?
            ORDER BY name}, undef, $self->id);

        $self->{'products'} = Bugzilla::Product->new_from_list($product_ids);
    }
    return $self->{'products'};
}

###############################
####      Accessors        ####
###############################

sub id          { return $_[0]->{'id'};          }
sub name        { return $_[0]->{'name'};        }
sub description { return $_[0]->{'description'}; }
sub sortkey     { return $_[0]->{'sortkey'};     }

###############################
####      Subroutines      ####
###############################

sub get_all_classifications {
    my $dbh = Bugzilla->dbh;

    my $ids = $dbh->selectcol_arrayref(q{
        SELECT id FROM classifications ORDER BY sortkey, name});

    my @classifications;
    foreach my $id (@$ids) {
        push @classifications, new Bugzilla::Classification($id);
    }
    return @classifications;
}

sub check_classification {
    my ($class_name) = @_;

    unless ($class_name) {
        ThrowUserError("classification_not_specified");
    }

    my $classification =
        new Bugzilla::Classification({name => $class_name});

    unless ($classification) {
        ThrowUserError("classification_doesnt_exist",
                       { name => $class_name });
    }
    
    return $classification;
}

1;

__END__

=head1 NAME

Bugzilla::Classification - Bugzilla classification class.

=head1 SYNOPSIS

    use Bugzilla::Classification;

    my $classification = new Bugzilla::Classification(1);
    my $classification = new Bugzilla::Classification({name => 'Acme'});

    my $id = $classification->id;
    my $name = $classification->name;
    my $description = $classification->description;
    my $product_count = $classification->product_count;
    my $products = $classification->products;

    my $hash_ref = Bugzilla::Classification::get_all_classifications();
    my $classification = $hash_ref->{1};

    my $classification =
        Bugzilla::Classification::check_classification('AcmeClass');

=head1 DESCRIPTION

Classification.pm represents a Classification object.

A Classification is a higher-level grouping of Products.

=head1 METHODS

=over

=item C<new($param)>

 Description: The constructor is used to load an existing
              classification by passing a classification
              id or classification name using a hash.

 Params:      $param - If you pass an integer, the integer is the
                      classification_id from the database that we
                      want to read in. If you pass in a hash with
                      'name' key, then the value of the name key
                      is the name of a classification from the DB.

 Returns:     A Bugzilla::Classification object.

=item C<product_count()>

 Description: Returns the total number of products that belong to
              the classification.

 Params:      none.

 Returns:     Integer - The total of products inside the classification.

=item C<products>

 Description: Returns all products of the classification.

 Params:      none.

 Returns:     A reference to an array of Bugzilla::Product objects.

=back

=head1 SUBROUTINES

=over

=item C<get_all_classifications()>

 Description: Returns all classifications.

 Params:      none.

 Returns:     Bugzilla::Classification object list.

=item C<check_classification($classification_name)>

 Description: Checks if the classification name passed in is a
              valid classification.

 Params:      $classification_name - String with a classification name.

 Returns:     Bugzilla::Classification object.

=back

=cut
