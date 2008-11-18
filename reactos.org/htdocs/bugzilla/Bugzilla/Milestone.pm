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
#                 Max Kanat-Alexander <mkanat@bugzilla.org>

use strict;

package Bugzilla::Milestone;

use base qw(Bugzilla::Object);

use Bugzilla::Util;
use Bugzilla::Error;

################################
#####    Initialization    #####
################################

use constant DEFAULT_SORTKEY => 0;

use constant DB_TABLE => 'milestones';

use constant DB_COLUMNS => qw(
    id
    value
    product_id
    sortkey
);

use constant NAME_FIELD => 'value';
use constant LIST_ORDER => 'sortkey, value';

sub new {
    my $class = shift;
    my $param = shift;
    my $dbh = Bugzilla->dbh;

    my $product;
    if (ref $param) {
        $product = $param->{product};
        my $name = $param->{name};
        if (!defined $product) {
            ThrowCodeError('bad_arg',
                {argument => 'product',
                 function => "${class}::new"});
        }
        if (!defined $name) {
            ThrowCodeError('bad_arg',
                {argument => 'name',
                 function => "${class}::new"});
        }

        my $condition = 'product_id = ? AND value = ?';
        my @values = ($product->id, $name);
        $param = { condition => $condition, values => \@values };
    }

    unshift @_, $param;
    return $class->SUPER::new(@_);
}

sub bug_count {
    my $self = shift;
    my $dbh = Bugzilla->dbh;

    if (!defined $self->{'bug_count'}) {
        $self->{'bug_count'} = $dbh->selectrow_array(q{
            SELECT COUNT(*) FROM bugs
            WHERE product_id = ? AND target_milestone = ?},
            undef, $self->product_id, $self->name) || 0;
    }
    return $self->{'bug_count'};
}

################################
#####      Accessors      ######
################################

sub name       { return $_[0]->{'value'};      }
sub product_id { return $_[0]->{'product_id'}; }
sub sortkey    { return $_[0]->{'sortkey'};    }

################################
#####     Subroutines      #####
################################

sub check_milestone {
    my ($product, $milestone_name) = @_;

    unless ($milestone_name) {
        ThrowUserError('milestone_not_specified');
    }

    my $milestone = new Bugzilla::Milestone({ product => $product,
                                              name    => $milestone_name });
    unless ($milestone) {
        ThrowUserError('milestone_not_valid',
                       {'product' => $product->name,
                        'milestone' => $milestone_name});
    }
    return $milestone;
}

sub check_sort_key {
    my ($milestone_name, $sortkey) = @_;
    # Keep a copy in case detaint_signed() clears the sortkey
    my $stored_sortkey = $sortkey;

    if (!detaint_signed($sortkey) || $sortkey < -32768
        || $sortkey > 32767) {
        ThrowUserError('milestone_sortkey_invalid',
                       {'name' => $milestone_name,
                        'sortkey' => $stored_sortkey});
    }
    return $sortkey;
}

1;

__END__

=head1 NAME

Bugzilla::Milestone - Bugzilla product milestone class.

=head1 SYNOPSIS

    use Bugzilla::Milestone;

    my $milestone = new Bugzilla::Milestone(
        { product => $product, name => 'milestone_value' });

    my $product_id = $milestone->product_id;
    my $value = $milestone->value;

    my $milestone = $hash_ref->{'milestone_value'};

=head1 DESCRIPTION

Milestone.pm represents a Product Milestone object.

=head1 METHODS

=over

=item C<new($product_id, $value)>

 Description: The constructor is used to load an existing milestone
              by passing a product id and a milestone value.

 Params:      $product_id - Integer with a Bugzilla product id.
              $value - String with a milestone value.

 Returns:     A Bugzilla::Milestone object.

=item C<bug_count()>

 Description: Returns the total of bugs that belong to the milestone.

 Params:      none.

 Returns:     Integer with the number of bugs.

=back

=head1 SUBROUTINES

=over

=item C<check_milestone($product, $milestone_name)>

 Description: Checks if a milestone name was passed in
              and if it is a valid milestone.

 Params:      $product - Bugzilla::Product object.
              $milestone_name - String with a milestone name.

 Returns:     Bugzilla::Milestone object.

=back

=cut
