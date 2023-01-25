package DNS::LDNS::Packet;

use 5.008008;
use strict;
use warnings;

use DNS::LDNS;

our $VERSION = '0.61';

sub new {
    my ($class, %args) = @_;

    if ($args{name}) {
	return _query_new(
	    $args{name}, $args{type}, $args{class}, $args{flags});
    }
    else {
	return _new;
    }
}

sub question {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_question, $self);
}

sub set_question {
    my ($self, $l) = @_;
    DNS::LDNS::GC::disown(my $old = $self->question);
    $self->_set_question($l);
    return DNS::LDNS::GC::own($l, $self);
}

sub answer {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_answer, $self);
}

sub set_answer {
    my ($self, $l) = @_;
    DNS::LDNS::GC::disown(my $old = $self->answer);
    $self->_set_answer($l);
    return DNS::LDNS::GC::own($l, $self);
}

sub authority {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_authority, $self);
}

sub set_authority {
    my ($self, $l) = @_;
    DNS::LDNS::GC::disown(my $old = $self->authority);
    $self->_set_authority($l);
    return DNS::LDNS::GC::own($l, $self);
}

sub additional {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_additional, $self);
}

sub set_additional {
    my ($self, $l) = @_;
    DNS::LDNS::GC::disown(my $old = $self->additional);
    $self->_set_additional($l);
    return DNS::LDNS::GC::own($l, $self);
}

sub answerfrom {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_answerfrom, $self);
}

sub set_answerfrom {
    my ($self, $a) = @_;
    DNS::LDNS::GC::disown(my $old = $self->answerfrom);
    $self->_set_answerfrom($a);
    return DNS::LDNS::GC::own($a, $self);
}


sub timestamp {
    my $self = shift;
    my $t = _timestamp($self);
    return wantarray ? @$t : $t;
}

sub edns_data {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_edns_data, $self);
}

sub set_edns_data {
    my ($self, $data) = @_;
    DNS::LDNS::GC::disown(my $old = $self->edns_data);
    $self->_set_edns_data($data);
    return DNS::LDNS::GC::own($data, $self);
}

sub push_rr {
    my ($self, $sec, $rr) = @_;

    my $ret = $self->_push_rr($sec, my $copy = $_->clone);
    DNS::LDNS::GC::own($copy, $self);
    return $ret;
}

sub safe_push_rr {
    my ($self, $sec, $rr) = @_;

    my $ret = $self->_safe_push_rr($sec, my $copy = $_->clone);
    if ($ret) {
	DNS::LDNS::GC::own($copy, $self);
    }
    return $ret;
}

sub tsig {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_tsig, $self);
}

sub set_tsig {
    my ($self, $rr) = @_;
    DNS::LDNS::GC::disown(my $old = $self->tsig);
    $self->_set_tsig($rr);
    return DNS::LDNS::GC::own($rr, $self);
}

sub DESTROY {
    DNS::LDNS::GC::free($_[0]);
}

1;
__END__

=head1 NAME

DNS::LDNS::Packet - DNS packet

=head1 SYNOPSIS

  use DNS::LDNS ':all'

  my pkt = new DNS::LDNS::Packet(name => rdata, type => LDNS_RR_TYPE_...,
    class => LDNS_RR_CLASS_..., flags => ...)
  my pkt = new DNS::LDNS::Packet

  pkt2 = pkt->clone

  pkt->to_string

  rrlist = pkt->question
  pkt->set_question(rrlist)
'
  rrlist = pkt->answer
  pkt->set_answer(rrlist)

  rrlist = pkt->authority
  pkt->set_authority(rrlist)

  rrlist = pkt->additional
  pkt->set_additional(rrlist)

  rrlist = pkt->all
  rrlist = pkt->all_noquestion

  for (qw/qr aa tc rd cd ra ad/) {
    bool = pkt->$_
    pkt->set_$_(bool)
  }

  id = pkt->id
  pkt->set_id(id)
  pkt->set_random_id

  count = pkt->qdcount
  count = pkt->ancount
  count = pkt->nscount
  count = pkt->arcount

  opcode = pkt->opcode
  pkt->set_opcode(opcode)

  rcode = pkt->rcode                  # Response code
  pkt->set_rcode(rcode)

  size = pkt->size

  epoch = pkt->querytime
  pkt->set_querytime(epoch)

  rdata = pkt->answerfrom
  pkt->set_answerfrom(rdata)

  (sec, usec) = pkt->timestamp
  pkt->set_timestamp(sec, usec)

  bool = pkt->edns

  size = pkt->edns_udp_size
  pkt->set_edns_udp_size(size)

  rcode = pkt->edns_extended_rcode
  pkt->set_edns_extended_rcode(rcode)

  v = pkt->edns_version
  pkt->set_edns_version(v)

  z = pkt->edns_z
  pkt->set_edns_z(z)

  do = pkt->edns_do
  pkt->set_edns_do(do)

  rdata = pkt->edns_data
  pkt->set_edns_data(rdata)

  pkt->set_flags(flags)

  rrlist = pkt->rr_list_by_name(rdata, section)
  rrlist = pkt->rr_list_by_type(type, section)
  rrlist = pkt->rr_list_by_name_and_type(rdata, type, section)

  bool = pkt->rr(section, rr)       # Check if rr exists

  pkt->push_rr(section, rr)
  pkt->safe_push_rr(section, rr)

  count = pkt->section_count(section)
  bool = pkt->empty

  rr = pkt->tsig
  pkt->set_tsig(rr)

  type = pkt->reply_type

  rrlist = pkt->get_rrsigs_for_name_and_type(rdata, rrtype)
  rrlist = pkt->get_rrsigs_for_type(rrtype)

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
