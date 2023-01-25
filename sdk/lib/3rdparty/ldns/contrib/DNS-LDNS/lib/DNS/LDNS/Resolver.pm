package DNS::LDNS::Resolver;

use 5.008008;
use strict;
use warnings;

use DNS::LDNS ':all';

our $VERSION = '0.61';

sub new {
    my ($class, %args) = @_;
    
    my $file;
    my $status = &LDNS_STATUS_OK;

    if ($args{filename}) {
	unless (open FILE, $args{filename}) {
	    $DNS::LDNS::last_status = &LDNS_STATUS_FILE_ERR;
	    $DNS::LDNS::line_nr = 0;
	    return;
	}

	$file = \*FILE;
    }
    elsif ($args{file}) {
	$file = $args{file};
    }

    my $resolver;
    if ($file) {
	$resolver = _new_from_file($file, $status);
    }
    else {
	$resolver = _new();
    }

    if ($args{filename}) {
	close $file;
    }

    $DNS::LDNS::last_status = $status;
    if (!defined $resolver) {
	return;
    }

    return $resolver;
}

sub dnssec_anchors {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_dnssec_anchors, $self);
}

sub push_dnssec_anchor {
    my ($self, $rr) = @_;
    
    _push_dnssec_anchor($self, my $copy = $rr->clone);
    DNS::LDNS::GC::own($copy, $self);
}

sub set_dnssec_anchors {
    my ($self, $l) = @_;
    DNS::LDNS::GC::disown(my $old = $self->dnssec_anchors);
    $self->_set_dnssec_anchors($l);
    DNS::LDNS::GC::own($l, $self);
    return $l;
}

sub domain {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_domain, $self);
}

sub set_domain {
    my ($self, $dom) = @_;
    DNS::LDNS::GC::disown(my $old = $self->domain);
    _set_domain($self, my $copy = $dom->clone);
    DNS::LDNS::GC::own($copy, $self);
}

sub nameservers {
    my $self = shift;
    my $list = _nameservers($self);
    for my $r (@$list) {
	DNS::LDNS::GC::own($r, $self);
    }
    return wantarray ? @$list : $list;
}

sub push_nameserver {
    my ($self, $n) = @_;

    my $s = _push_nameserver($self, my $copy = $n->clone);
    DNS::LDNS::GC::own($copy, $self);
    $DNS::LDNS::last_status = $s;
    return $s;
}

sub pop_nameserver {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_pop_nameserver);
}

sub push_searchlist {
    my ($self, $rd) = @_;

    _push_searchlist($self, my $copy = $rd->clone);
    DNS::LDNS::GC::own($copy, $self);
}

sub searchlist {
    my $self = shift;
    my $list = _searchlist($self);
    for my $r (@$list) {
	DNS::LDNS::GC::own($r, $self);
    }
    return wantarray ? @$list : $list;
}

sub timeout {
    my $self = shift;
    my $t = _timeout($self);
    return wantarray ? @$t : $t;
}

sub rtt {
    my $self = shift;
    my $list = _rtt($self);
    return wantarray ? @$list : $list;
}

sub set_rtt {
    my ($self, @rtt) = @_;
    # FIXME: Validate @rtt, existence, size
    _set_rtt($self, \@rtt);
}

sub fetch_valid_domain_keys {
    my ($self, $domain, $keys) = @_;

    my $status;
    my $trusted = _fetch_valid_domain_keys($self, $domain, $keys, $status);
    $DNS::LDNS::last_status = $status;
    if (!$trusted) {
	return;
    }

    return DNS::LDNS::GC::own($trusted, $self);
}

sub fetch_valid_domain_keys_time {
    my ($self, $domain, $keys, $checktime) = @_;

    my $status;
    my $trusted = _fetch_valid_domain_keys_time(
	$self, $domain, $keys, $checktime, $status);
    $DNS::LDNS::last_status = $status;
    if (!$trusted) {
	return;
    }

    return DNS::LDNS::GC::own($trusted, $self);
}

sub prepare_query_pkt {
    my ($self, $rdata, $type, $class, $flags) = @_;

    my $s = &LDNS_STATUS_OK;
    my $qry = _prepare_query_pkt($self, $rdata, $type, $class, $flags, $s);
    $DNS::LDNS::last_status = $s;
    if ($s != LDNS_STATUS_OK) {
	return;
    }
    return $qry;
}

sub send {
    my ($self, $rdata, $type, $class, $flags) = @_;

    my $s = &LDNS_STATUS_OK;
    my $ans = _send($self, $rdata, $type, $class, $flags, $s);
    $DNS::LDNS::last_status = $s;
    if ($s != LDNS_STATUS_OK) {
	return;
    }
    return $ans;
}

sub send_pkt {
    my ($self, $qry) = @_;

    my $s = &LDNS_STATUS_OK;
    my $ans = _send_pkt($self, $qry, $s);
    $DNS::LDNS::last_status = $s;
    if ($s != LDNS_STATUS_OK) {
	return;
    }
    return $ans;
}

sub verify_trusted {
    my ($self, $rrset, $rrsigs, $validating_keys) = @_;
    my $s = _verify_trusted($self, $rrset, $rrsigs, $validating_keys);
    $DNS::LDNS::last_status = $s;
    return $s;
}

sub verify_trusted_time {
    my ($self, $rrset, $rrsigs, $check_time, $validating_keys) = @_;
    my $s = _verify_trusted_time($self, $rrset, $rrsigs, $check_time, 
			    $validating_keys);
    $DNS::LDNS::last_status = $s;
    return $s;
}

sub DESTROY {
    DNS::LDNS::GC::free($_[0]);
}

1;
__END__

=head1 NAME

DNS::LDNS::Resolver - DNS resolver

=head1 SYNOPSIS

  use DNS::LDNS ':all'

  my r = new DNS::LDNS::Resolver(filename => '/my/resolv.conf')
  my r = new DNS::LDNS::Resolver(file => \*FILE)
  my r = new DNS::LDNS::Resolver

  bool = r->dnssec
  r->set_dnssec(bool)

  bool = r->dnssec_cd        # Resolver sets the CD bit
  r->set_dnssec_cd(bool)

  port = r->port
  r->set_port(port)

  bool = r->recursive
  r->set_recursive(bool)

  bool = r->debug
  r->set_debug(bool)

  count = r->retry
  r->set_retry(count)

  count = r->retrans
  r->set_retrans(count)

  bool = r->fallback          # Resolver truncation fallback mechanism
  r->set_fallback(bool)

  bool = r->ip6
  r->set_ip6(bool)

  size = r->edns_udp_size
  r->set_edns_udp_size(size)

  bool = r->usevc             # Use virtual circuit (TCP)
  r->set_usevc(bool)

  r->fail
  r->set_fail

  r->defnames
  r->set_defnames

  r->dnsrch
  r->set_dnsrch

  r->igntc
  r->set_igntc

  bool = r->random            # randomize nameserver before usage
  r->set_random(bool)

  rrlist = r->dnssec_anchors  # list of trusted DNSSEC anchors
  r->push_dnssec_anchor(rr)
  r->set_dnssec_anchors(rrlist)

  rdata = r->domain           # Domain to add to relative queries
  r->set_domain(rdata)

  @rdata = r->nameservers
  count = r->nameserver_count
  r->push_nameserver(rdata)
  rdata = r->pop_nameserver
  r->nameservers_randomize

  str = r->tsig_keyname
  r->set_tsig_keyname(str)

  str = r->tsig_algorithm
  r->set_tsig_algorithm(str)

  str = r->tsig_keydata
  r->set_tsig_keydata(str)

  count = r->searchlist_count
  r->push_searchlist(rdata)
  @rdata = r->searchlist

  @times = r->rtt              # Round trip times
  r->set_rtt(@rtt)
  time = r->nameserver_rtt(pos)
  r->set_nameserver_rtt(pos, time)

  (sec, usec) = r->timeout
  r->set_timeout(sec, usec)

  # DNSSec validation
  rrlist = r->fetch_valid_domain_keys(domain, keys)
  rrlist = r->fetch_valid_domain_keys_time(domain, keys, checktime)
  rrlist = r->validate_domain_ds(domain, keys)
  rrlist = r->validate_domain_ds_time(domain, keys, checktime)
  rrlist = r->validate_domain_dnskey(domain, keys)
  rrlist = r->validate_domain_dnskey_time(domain, keys, checktime)
  status = r->verify_trusted(rrset, rrsigs, validation_keys)
  status = r->verify_trusted_time(rrset, rrsigs, checktime, validation_keys)
  bool = r->trusted_key(keys, trusted_keys)
  chain = r->build_data_chain(qflags, dataset, pkt, orig_rr)

  # Query
  pkt = r->query(rdata, type, class, flags)
  pkt = r->search(rdata, type, class, flags)
  query = r->prepare_query_pkt(rdata, type, class, flags)
  answer = r->send(rdata, type, class, flags)
  answer = r->send_pkt(query)
  rrlist = r->get_rr_list_addr_by_name(name, class, flags)
  rrlist = r->get_rr_list_name_by_addr(addr, class, flags)

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
