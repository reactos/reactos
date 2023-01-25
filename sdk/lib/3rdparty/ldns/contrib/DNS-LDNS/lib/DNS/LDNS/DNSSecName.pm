package DNS::LDNS::DNSSecName;

use 5.008008;
use strict;
use warnings;

use DNS::LDNS ':all';

our $VERSION = '0.61';

sub new {
    my $class = shift;
    return _new;
}

sub name {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_name, $self);
}

sub set_name {
    my ($self, $name) = @_;

    DNS::LDNS::GC::disown(my $old = $self->name);
    _set_name($self, my $copy = $name->clone);
    DNS::LDNS::GC::own($copy, $self);
}

sub rrsets {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_rrsets, $self);
}

sub add_rr {
    my ($self, $rr) = @_;

    my $s = _add_rr($self, my $copy = $rr->clone);
    DNS::LDNS::GC::own($copy, $self);
    $DNS::LDNS::last_status = $s;
    return $s;
}

sub nsec {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_nsec, $self);
}

sub set_nsec {
    my ($self, $nsec) = @_;

    DNS::LDNS::GC::disown(my $old = $self->nsec);
    _set_nsec($self, my $copy = $nsec->clone);
    DNS::LDNS::GC::own($copy, $self);
}

sub hashed_name {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_hashed_name, $self);
}

sub nsec_signatures {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_nsec_signatures, $self);
}

sub DESTROY {
    DNS::LDNS::GC::free($_[0]);
}

1;
__END__

=head1 NAME

DNS::LDNS::DNSSecName - Dname with rrsets in a dnssec zone

=head1 SYNOPSIS

  use LDNS ':all'

  my name = new DNS::LDNS::DNSSecName

  rdata = name->name
  name->set_name(rdata)
  bool = name->is_glue
  rrsets = name->rrsets
  name->add_rr(rr)

  rr = name->nsec
  name->set_nsec(rr)
  hash = name->hashed_name
  rrs = name->nsec_signatures

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
