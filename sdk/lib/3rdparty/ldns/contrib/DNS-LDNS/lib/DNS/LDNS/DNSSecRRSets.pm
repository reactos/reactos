package DNS::LDNS::DNSSecRRSets;

use 5.008008;
use strict;
use warnings;

use DNS::LDNS;

our $VERSION = '0.61';

# Note: Since this class does not have a constructor, we can let its child
# objects be owned by the parent. This reduces the recursion depth on
# DESTROY.

sub rrs {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_rrs, DNS::LDNS::GC::owner($self));
}

sub signatures {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_signatures, DNS::LDNS::GC::owner($self));
}

sub next {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_next, DNS::LDNS::GC::owner($self));
}

sub set_type {
    my ($self, $type) = @_;
    my $s = _set_type($self, $type);
    $DNS::LDNS::last_status = $s;
    return $s;
}

sub add_rr {
    my ($self, $rr) = @_;

    my $s = _add_rr($self, my $copy = $rr->clone);
    $DNS::LDNS::last_status = $s;
    DNS::LDNS::GC::own($copy, $self);
    return $s;
}

sub DESTROY {
    DNS::LDNS::GC::free($_[0]);
}

1;
__END__

=head1 NAME

DNS::LDNS::DNSSecRRSets - Linked list of rrsets in a dnssec zone

=head1 SYNOPSIS

  use DNS::LDNS ':all'

  rrs = rrsets->rrs
  rrs = rrsets->signatures
  rrsets2 = rrsets->next
  rrsets->add_rr(rr)
  bool = rrsets->contains_type(rr_type)
  rr_type = rrsets->type
  rrsets->set_type(rr_type)

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
