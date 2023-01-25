package DNS::LDNS::RBNode;

use 5.008008;
use strict;
use warnings;

use DNS::LDNS;

our $VERSION = '0.61';

# Note: This class does not have a constructor. Thus, it can not be created
# as an individual object. The data structure of the object will always be 
# owned and freed by its parent object.

sub next {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_next, $self);
}

sub previous {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_previous, $self);
}

sub next_nonglue {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_next_nonglue, $self);
}

sub name {
    my ($self) = @_;
    return DNS::LDNS::GC::own($self->_name, $self);
}

sub DESTROY {
    DNS::LDNS::GC::free($_[0]);
}

1;
__END__

=head1 NAME

DNS::LDNS::RBNode - Node in the RBTree

=head1 SYNOPSIS

  use DNS::LDNS ':all'

  node2 = node->next
  node2 = node->next_nonglue
  bool = node->is_null
  dnssec_name = node->name

=head1 SEE ALSO

http://www.nlnetlabs.nl/projects/ldns

=head1 AUTHOR

Erik Pihl Ostlyngen, E<lt>erik.ostlyngen@uninett.noE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2013 by UNINETT Norid AS

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.14.2 or,
at your option, any later version of Perl 5 you may have available.

=cut
