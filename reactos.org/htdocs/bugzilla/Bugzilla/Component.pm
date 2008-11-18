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
#                 Frédéric Buclin <LpSolit@gmail.com>
#                 Max Kanat-Alexander <mkanat@bugzilla.org>
#                 Akamai Technologies <bugzilla-dev@akamai.com>

use strict;

package Bugzilla::Component;

use base qw(Bugzilla::Object);

use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::User;
use Bugzilla::FlagType;

###############################
####    Initialization     ####
###############################

use constant DB_TABLE => 'components';

use constant DB_COLUMNS => qw(
    id
    name
    product_id
    initialowner
    initialqacontact
    description
);

###############################
####       Methods         ####
###############################

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

        my $condition = 'product_id = ? AND name = ?';
        my @values = ($product->id, $name);
        $param = { condition => $condition, values => \@values };
    }

    unshift @_, $param;
    my $component = $class->SUPER::new(@_);
    # Add the product object as attribute only if the component exists.
    $component->{product} = $product if ($component && $product);
    return $component;
}

sub bug_count {
    my $self = shift;
    my $dbh = Bugzilla->dbh;

    if (!defined $self->{'bug_count'}) {
        $self->{'bug_count'} = $dbh->selectrow_array(q{
            SELECT COUNT(*) FROM bugs
            WHERE component_id = ?}, undef, $self->id) || 0;
    }
    return $self->{'bug_count'};
}

sub bug_ids {
    my $self = shift;
    my $dbh = Bugzilla->dbh;

    if (!defined $self->{'bugs_ids'}) {
        $self->{'bugs_ids'} = $dbh->selectcol_arrayref(q{
            SELECT bug_id FROM bugs
            WHERE component_id = ?}, undef, $self->id);
    }
    return $self->{'bugs_ids'};
}

sub default_assignee {
    my $self = shift;

    if (!defined $self->{'default_assignee'}) {
        $self->{'default_assignee'} =
            new Bugzilla::User($self->{'initialowner'});
    }
    return $self->{'default_assignee'};
}

sub default_qa_contact {
    my $self = shift;

    if (!defined $self->{'default_qa_contact'}) {
        $self->{'default_qa_contact'} =
            new Bugzilla::User($self->{'initialqacontact'});
    }
    return $self->{'default_qa_contact'};
}

sub flag_types {
    my $self = shift;

    if (!defined $self->{'flag_types'}) {
        $self->{'flag_types'} = {};
        $self->{'flag_types'}->{'bug'} =
          Bugzilla::FlagType::match({ 'target_type'  => 'bug',
                                      'product_id'   => $self->product_id,
                                      'component_id' => $self->id });

        $self->{'flag_types'}->{'attachment'} =
          Bugzilla::FlagType::match({ 'target_type'  => 'attachment',
                                      'product_id'   => $self->product_id,
                                      'component_id' => $self->id });
    }
    return $self->{'flag_types'};
}

sub initial_cc {
    my $self = shift;

    my $dbh = Bugzilla->dbh;

    if (!defined $self->{'initial_cc'}) {
        my $cc_ids = $dbh->selectcol_arrayref(
            "SELECT user_id FROM component_cc WHERE component_id = ?",
            undef, $self->id);
        my $initial_cc = Bugzilla::User->new_from_list($cc_ids);
        $self->{'initial_cc'} = $initial_cc;
    }
    return $self->{'initial_cc'};
}

sub product {
    my $self = shift;
    if (!defined $self->{'product'}) {
        require Bugzilla::Product; # We cannot |use| it.
        $self->{'product'} = new Bugzilla::Product($self->product_id);
    }
    return $self->{'product'};
}

###############################
####      Accessors        ####
###############################

sub id          { return $_[0]->{'id'};          }
sub name        { return $_[0]->{'name'};        }
sub description { return $_[0]->{'description'}; }
sub product_id  { return $_[0]->{'product_id'};  }

###############################
####      Subroutines      ####
###############################

sub check_component {
    my ($product, $comp_name) = @_;

    $comp_name || ThrowUserError('component_blank_name');

    if (length($comp_name) > 64) {
        ThrowUserError('component_name_too_long',
                       {'name' => $comp_name});
    }

    my $component =
        new Bugzilla::Component({product => $product,
                                 name    => $comp_name});
    unless ($component) {
        ThrowUserError('component_not_valid',
                       {'product' => $product->name,
                        'name' => $comp_name});
    }
    return $component;
}

1;

__END__

=head1 NAME

Bugzilla::Component - Bugzilla product component class.

=head1 SYNOPSIS

    use Bugzilla::Component;

    my $component = new Bugzilla::Component(1);
    my $component = new Bugzilla::Component({product => $product,
                                             name    => 'AcmeComp'});

    my $bug_count          = $component->bug_count();
    my $bug_ids            = $component->bug_ids();
    my $id                 = $component->id;
    my $name               = $component->name;
    my $description        = $component->description;
    my $product_id         = $component->product_id;
    my $default_assignee   = $component->default_assignee;
    my $default_qa_contact = $component->default_qa_contact;
    my $initial_cc         = $component->initial_cc;
    my $product            = $component->product;
    my $bug_flag_types     = $component->flag_types->{'bug'};
    my $attach_flag_types  = $component->flag_types->{'attachment'};

    my $component  = Bugzilla::Component::check_component($product, 'AcmeComp');

=head1 DESCRIPTION

Component.pm represents a Product Component object.

=head1 METHODS

=over

=item C<new($param)>

 Description: The constructor is used to load an existing component
              by passing a component id or a hash with the product
              id and the component name.

 Params:      $param - If you pass an integer, the integer is the
                       component id from the database that we want to
                       read in. If you pass in a hash with 'name' key,
                       then the value of the name key is the name of a
                       component from the DB.

 Returns:     A Bugzilla::Component object.

=item C<bug_count()>

 Description: Returns the total of bugs that belong to the component.

 Params:      none.

 Returns:     Integer with the number of bugs.

=item C<bugs_ids()>

 Description: Returns all bug IDs that belong to the component.

 Params:      none.

 Returns:     A reference to an array of bug IDs.

=item C<default_assignee()>

 Description: Returns a user object that represents the default assignee for
              the component.

 Params:      none.

 Returns:     A Bugzilla::User object.

=item C<default_qa_contact()>

 Description: Returns a user object that represents the default QA contact for
              the component.

 Params:      none.

 Returns:     A Bugzilla::User object.

=item C<initial_cc>

Returns an arrayref of L<Bugzilla::User> objects representing the
Initial CC List.

=item C<flag_types()>

  Description: Returns all bug and attachment flagtypes available for
               the component.

  Params:      none.

  Returns:     Two references to an array of flagtype objects.

=item C<product()>

  Description: Returns the product the component belongs to.

  Params:      none.

  Returns:     A Bugzilla::Product object.

=back

=head1 SUBROUTINES

=over

=item C<check_component($product, $comp_name)>

 Description: Checks if the component name was passed in and if it is a valid
              component.

 Params:      $product - A Bugzilla::Product object.
              $comp_name - String with a component name.

 Returns:     Bugzilla::Component object.

=back

=cut
