package DNS::LDNS::DNSSecDataChain;

use 5.008008;
use strict;
use warnings;

use DNS::LDNS;

our $VERSION = '0.61';

sub rrset {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_rrset, $self);
}

sub signatures {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_signatures, $self);
}

sub parent {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_parent, $self);
}

sub derive_trust_tree {
    my ($self, $rr) = @_;

    if (!DNS::LDNS::GC::is_owned($rr) or DNS::LDNS::GC::owner($rr) ne $self) {
	die "The rr ($rr) must be in the data chain ($self)";
    }
    return DNS::LDNS::GC::own($self->_derive_trust_tree($rr), $self);
}

sub derive_trust_tree_time {
    my ($self, $rr, $checktime) = @_;

    if (!DNS::LDNS::GC::is_owned($rr) or DNS::LDNS::GC::owner($rr) ne $self) {
	die "The rr ($rr) must be in the data chain ($self)";
    }
    return DNS::LDNS::GC::own(
	$self->_derive_trust_tree_time($rr, $checktime), $self);
}


1;
__END__

=head1 NAME

DNS::LDNS::DNSSecDataChain - DNSSec data chain element

=head1 SYNOPSIS

  use DNS::LDNS ':all'

  chain = new DNS::LDNS::DNSSecDataChain
  chain->print(fp)
  chain->derive_trust_tree(rr)
  chain->derive_trust_tree_time(rr, checktime)

  # Node attributes
  rrset = chain->rrset
  rrset = chain->signatures
  rrtype = chain->parent_type
  pchain = chain->parent
  rcode = chain->packet_rcode
  rrtype = chain->packet_qtype
  bool = chain->packet_nodata

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
