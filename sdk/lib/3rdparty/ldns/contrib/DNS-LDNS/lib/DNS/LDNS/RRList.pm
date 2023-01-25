package DNS::LDNS::RRList;

use 5.008008;
use strict;
use warnings;

use DNS::LDNS;

our $VERSION = '0.61';

sub new {
    my ($class, %args) = @_;

    if ($args{hosts_filename} or $args{hosts_file})  {
	my $file = $args{hosts_file};
	if ($args{hosts_filename}) {
	    unless (open FILE, $args{hosts_filename}) {
		$DNS::LDNS::last_status = &LDNS_STATUS_FILE_ERR;
		$DNS::LDNS::line_nr = 0;
		return;
	    }
	    $file = \*FILE;
	}
	my $list = _new_hosts_from_file($file, $DNS::LDNS::line_nr);
	if ($args{hosts_filename}) {
	    close $file;
	}
	return $list;
    }
    
    return _new();
}

sub rr {
    my ($self, $index) = @_;
    return DNS::LDNS::GC::own($self->_rr($index), $self);
}

sub push {
    my ($self, @rrs) = @_;

    for my $rr (@rrs) {
	# Push a copy of the rr in case it is already owned
	$self->_push(my $copy = $rr->clone);
	DNS::LDNS::GC::own($copy, $self);
    }
}

sub push_list {
    my ($self, $list) = @_;

    $self->_push_list(my $copy = $list->clone);
    DNS::LDNS::GC::own($copy, $self);
}

sub verify {
    my ($self, $sig, $keys) = @_;
    my $goodkeys = new DNS::LDNS::RRList;
    my $s = _verify($self, $sig, $keys, $goodkeys);
    $DNS::LDNS::last_status = $s;
    return wantarray ? ($s, $goodkeys) : $s;
}

sub verify_time {
    my ($self, $sig, $keys, $checktime) = @_;
    my $goodkeys = new DNS::LDNS::RRList;
    my $s = _verify_time($self, $sig, $keys, $checktime, $goodkeys);
    $DNS::LDNS::last_status = $s;
    return wantarray ? ($s, $goodkeys) : $s;
}

sub verify_notime {
    my ($self, $sig, $keys) = @_;
    my $goodkeys = new DNS::LDNS::RRList;
    my $s = _verify_notime($self, $sig, $keys, $goodkeys);
    $DNS::LDNS::last_status = $s;
    return wantarray ? ($s, $goodkeys) : $s;
}

sub verify_rrsig_keylist {
    my ($self, $sig, $keys) = @_;
    my $goodkeys = new DNS::LDNS::RRList;
    my $s = _verify_rrsig_keylist($self, $sig, $keys, $goodkeys);
    $DNS::LDNS::last_status = $s;
    return wantarray ? ($s, $goodkeys) : $s;
}

sub verify_rrsig_keylist_notime {
    my ($self, $sig, $keys, $check_time) = @_;
    my $goodkeys = new DNS::LDNS::RRList;
    my $s = _verify_rrsig_keylist_notime($self, $sig, $keys, $goodkeys);
    $DNS::LDNS::last_status = $s;
    return wantarray ? ($s, $goodkeys) : $s;
}

sub get_dnskey_for_rrsig {
    my ($self, $rrsig) = @_;
    return DNS::LDNS::GC::own(_get_dnskey_for_rrsig($rrsig, $self), $self);
}

sub get_rrsig_for_name_and_type {
    my ($self, $name, $type) = @_;
    return DNS::LDNS::GC::own(
	_get_dnskey_for_name_and_type($name, $type, $self), $self);
}

sub DESTROY {
    DNS::LDNS::GC::free($_[0]);
}

1;
__END__

=head1 NAME

DNS::LDNS::RRList - List of rrs

=head1 SYNOPSIS

  use DNS::LDNS ':all'

  my l = new DNS::LDNS::RRList
  my l = new DNS::LDNS::RRList(hosts_file => \*FILE)
  my l = new DNS::LDNS::RRList(hosts_filename => fname)
  my l2 = l->clone

  l->to_string

  l->print(\*FILE)
  count = l->rr_count

  rr = l->rr(index)
  l->push(@rr)
  rr = l->pop

  l->push_list(l2)
  l2 = l->pop_list(count)
  l2 = l->pop_rrset

  l->compare(l2)

  l2 = l->subtype_by_rdata(rdata, pos)

  bool = l->is_rrset

  bool = l->contains_rr(rr)

  (status, goodkeys) = l->verify(sig, keys)
  (status, goodkeys) = l->verify_time(sig, keys, checktime)
  (status, goodkeys) = l->verify_notime(sig, keys)
  (status, goodkeys) = l->verify_rrsig_keylist(sig, keys)
  (status, goodkeys) = l->verify_rrsig_keylist_time(sig, keys, checktime)
  (status, goodkeys) = l->verify_rrsig_keylist_notime(sig, keys)
  status = l->verify_rrsig(sig, keys)
  status = l->verify_rrsig_time(sig, keys, checktime)

  rr = l->create_empty_rrsig(key)
  rrlist = l->sign_public(keylist)

  rrlist->canonicalize
  rrlist->sort
  rrlist->sort_nsec3   # the list must contain only nsec3 rrs

  rr = keylist->get_dnskey_for_rrsig(rrsig)
  rr = keylist->get_rrsig_for_name_and_type(name, type)

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
