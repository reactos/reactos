package DNS::LDNS::RR;

use 5.008008;
use strict;
use warnings;

use DNS::LDNS ':all';

our $VERSION = '0.61';

sub new {
    my $class = shift;

    my $rr;
    my $status = &LDNS_STATUS_OK;

    if (scalar(@_) == 0) {
	$rr = _new;
    }
    elsif (scalar(@_) == 1) {
	$rr = _new_from_str($_[0], 0, 
	    undef, undef,
	    $status);
    }
    else {
	my %args = @_;
	# Perl 5.25 does not allow us to pass read-only undef into a
	# parameter changing function. So we must send it with a variable.
        my $undef = undef;

	if ($args{str}) {
	    $rr = _new_from_str($args{str}, 
		$args{default_ttl} || 0,
		$args{origin},
		$args{prev} ? ${$args{prev}} : $undef,
		$status);
	}
	elsif ($args{filename} or $args{file}) {
	    my $line_nr = 0;
	    my $file = $args{file};
	    if ($args{filename}) {
		unless (open FILE, $args{filename}) {
		    $DNS::LDNS::last_status = &LDNS_STATUS_FILE_ERR;
		    $DNS::LDNS::line_nr = 0;
		    return;
		}
		$file = \*FILE;
	    }

	    my $ttl = 0;
	    $rr = _new_from_file($file, 
		 $args{default_ttl} ? ${$args{default_ttl}} : $ttl,
		 $args{origin} ? ${$args{origin}} : $undef,
		 $args{prev} ? ${$args{prev}} : $undef,
		 $status,
		 $line_nr);
	    if ($args{filename}) {
		close $file;
	    }

	    $DNS::LDNS::line_nr = $line_nr;
	}
	elsif ($args{type}) {
	    $rr = _new_from_type($args{type});
	    if ($args{owner}) {
		$rr->set_owner(ref $args{owner} ? $args{owner} : 
		    new DNS::LDNS::RData(&LDNS_RDF_TYPE_DNAME, $args{owner}));
	    }
	    $rr->set_ttl($args{ttl}) if ($args{ttl});
	    $rr->set_class($args{class}) if ($args{class});

	    if ($args{rdata}) {
		if (!$rr->set_rdata(@{$args{rdata}})) {
		    $DNS::LDNS::last_status = &LDNS_STATUS_SYNTAX_RDATA_ERR;
		    return;
		}
	    }
	}
    }

    if (!defined $rr) {
	$DNS::LDNS::last_status = $status;
	return;
    }
    return $rr;
}

sub owner {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_owner, $self);
}

sub set_owner {
    my ($self, $owner) = @_;
    DNS::LDNS::GC::disown(my $old = $self->owner);
    $self->_set_owner($owner);
    return DNS::LDNS::GC::own($owner, $self);
}

sub dname {
    return $_[0]->owner->to_string;
}

sub rdata {
    my ($self, $index) = @_;
    return DNS::LDNS::GC::own($self->_rdata($index), $self);
}

# replace all existing rdata with new ones. Requires the
# input array to be exactly same length as rd_count
sub set_rdata {
    my ($self, @rdata) = @_;

    if (scalar @rdata != $self->rd_count) {
	# Hopefully this is a proper error to return here...
	$DNS::LDNS::last_status = LDNS_STATUS_SYNTAX_RDATA_ERR;
	return;
    }
    my $i = 0;
    for (@rdata) {
	my $oldrd = _set_rdata($self, my $copy = $_->clone, $i);
	DNS::LDNS::GC::disown(my $old = $oldrd);
	DNS::LDNS::GC::own($copy, $self);
	$i++;
    }

    return 1;
}

sub push_rdata {
    my ($self, @rdata) = @_;

    for (@rdata) {
	# Push a copy in case the input rdata are already owned
	$self->_push_rdata(my $copy = $_->clone);
	DNS::LDNS::GC::own($copy, $self);
    }
}

sub rrsig_typecovered {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_rrsig_typecovered, $self);
}

sub rrsig_set_typecovered {
    my ($self, $type) = shift;
    DNS::LDNS::GC::disown(my $old = $self->rrsig_typecovered);
    my $result = $self->_rrsig_set_typecovered(my $copy = $type->clone);
    DNS::LDNS::GC::own($copy, $self);
    return $result;
}

sub rrsig_algorithm {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_rrsig_algorithm, $self);
}

sub rrsig_set_algorithm {
    my ($self, $algo) = shift;
    DNS::LDNS::GC::disown(my $old = $self->rrsig_algorithm);
    my $result = $self->_rrsig_set_algorithm(my $copy = $algo->clone);
    DNS::LDNS::GC::own($copy, $self);
    return $result;
}

sub rrsig_expiration {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_rrsig_expiration, $self);
}

sub rrsig_set_expiration {
    my ($self, $date) = shift;
    DNS::LDNS::GC::disown(my $old = $self->rrsig_expiration);
    my $result = $self->_rrsig_set_expiration(my $copy = $date->clone);
    DNS::LDNS::GC::own($copy, $self);
    return $result;
}

sub rrsig_inception {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_rrsig_inception, $self);
}

sub rrsig_set_inception {
    my ($self, $date) = shift;
    DNS::LDNS::GC::disown(my $old = $self->rrsig_inception);
    my $result = $self->_rrsig_set_inception(my $copy = $date->clone);
    DNS::LDNS::GC::own($copy, $self);
    return $result;
}

sub rrsig_keytag {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_rrsig_keytag, $self);
}

sub rrsig_set_keytag {
    my ($self, $tag) = shift;
    DNS::LDNS::GC::disown(my $old = $self->rrsig_keytag);
    my $result = $self->_rrsig_set_keytag(my $copy = $tag->clone);
    DNS::LDNS::GC::own($copy, $self);
    return $result;
}

sub rrsig_sig {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_rrsig_sig, $self);
}

sub rrsig_set_sig {
    my ($self, $sig) = shift;
    DNS::LDNS::GC::disown(my $old = $self->rrsig_sig);
    my $result = $self->_rrsig_set_sig(my $copy = $sig->clone);
    DNS::LDNS::GC::own($copy, $self);
    return $result;
}

sub rrsig_labels {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_rrsig_labels, $self);
}

sub rrsig_set_labels {
    my ($self, $lab) = shift;
    DNS::LDNS::GC::disown(my $old = $self->rrsig_labels);
    my $result = $self->_rrsig_set_labels(my $copy = $lab->clone);
    DNS::LDNS::GC::own($copy, $self);
    return $result;
}

sub rrsig_origttl {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_rrsig_origttl, $self);
}

sub rrsig_set_origttl {
    my ($self, $ttl) = shift;
    DNS::LDNS::GC::disown(my $old = $self->rrsig_origttl);
    my $result = $self->_rrsig_set_origttl(my $copy = $ttl->clone);
    DNS::LDNS::GC::own($copy, $self);
    return $result;
}

sub rrsig_signame {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_rrsig_signame, $self);
}

sub rrsig_set_signame {
    my ($self, $name) = shift;
    DNS::LDNS::GC::disown(my $old = $self->rrsig_signame);
    my $result = $self->_rrsig_set_signame(my $copy = $name->clone);
    DNS::LDNS::GC::own($copy, $self);
    return $result;
}

sub dnskey_algorithm {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_dnskey_algorithm, $self);
}

sub dnskey_set_algorithm {
    my ($self, $algo) = shift;
    DNS::LDNS::GC::disown(my $old = $self->dnskey_algorithm);
    my $result = $self->_dnskey_set_algorithm(my $copy = $algo->clone);
    DNS::LDNS::GC::own($copy, $self);
    return $result;
}

sub dnskey_flags {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_dnskey_flags, $self);
}

sub dnskey_set_flags {
    my ($self, $flags) = shift;
    DNS::LDNS::GC::disown(my $old = $self->flags);
    my $result = $self->_dnskey_set_flags(my $copy = $flags->clone);
    DNS::LDNS::GC::own($copy, $self);
    return $result;
}

sub dnskey_protocol {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_dnskey_protocol, $self);
}

sub dnskey_set_protocol {
    my ($self, $proto) = shift;
    DNS::LDNS::GC::disown(my $old = $self->dnskey_protocol);
    my $result = $self->_dnskey_set_protocol(my $copy = $proto->clone);
    DNS::LDNS::GC::own($copy, $self);
    return $result;
}

sub dnskey_key {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_dnskey_key, $self);
}

sub dnskey_set_key {
    my ($self, $key) = shift;
    DNS::LDNS::GC::disown(my $old = $self->dnskey_key);
    my $result = $self->_dnskey_set_key(my $copy = $key->clone);
    DNS::LDNS::GC::own($copy, $self);
    return $result;
}

sub nsec3_next_owner {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_nsec3_next_owner, $self);
}

sub nsec3_bitmap {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_nsec3_bitmap, $self);
}

sub nsec3_salt {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_nsec3_salt, $self);
}

sub hash_name_from_nsec3 {
    my ($self, $name) = @_;
    my $hash = $self->_hash_name_from_nsec3($name);
    return DNS::LDNS::GC::own($self->_hash_name_from_nsec3($name), $self);
}

sub verify_denial {
    my ($self, $nsecs, $rrsigs) = @_;
    my $s = _verify_denial($self, $nsecs, $rrsigs);
    $DNS::LDNS::last_status = $s;
    return $s;
}

sub verify_denial_nsec3 {
    my ($self, $nsecs, $rrsigs, $packet_rcode, $packet_qtype, 
	$packet_nodata) = @_;
    my $s = _verify_denial_nsec3($self, $nsecs, $rrsigs, $packet_rcode, 
				 $packet_qtype, $packet_nodata);
    $DNS::LDNS::last_status = $s;
    return $s;
}

sub verify_denial_nsec3_match {
    my ($self, $nsecs, $rrsigs, $packet_rcode, $packet_qtype, 
	$packet_nodata) = @_;

    my $status;
    my $match = _verify_denial_nsec3_match($self, $nsecs, $rrsigs, $packet_rcode, $packet_qtype, $packet_nodata, $status);
    $DNS::LDNS::last_status = $status;
    if ($status != &LDNS_STATUS_OK) {
	return;
    }

    # $match is an RR owned by the $nsecs list.
    return DNS::LDNS::GC::own($match, $nsecs);
}

sub DESTROY {
    DNS::LDNS::GC::free($_[0]);
}

1;
__END__

=head1 NAME

DNS::LDNS::RR - Resource record

=head1 SYNOPSIS

  use DNS::LDNS ':all'

  my rr = new DNS::LDNS::RR('mylabel 3600 IN A 168.10.10.10')
  my rr = new DNS::LDNS::RR(
    str => 'mylabel 3600 IN A 168.10.10.10',
    default_ttl => 3600,     # optional
    origin => $origin_rdata, # optional
    prev => \$prev_rdata,    # optional
  )
  my rr = new DNS::LDNS::RR(
    filename => '/path/to/rr',
    default_ttl => \$ttl,     # optional
    origin => \$origin_rdata, # optional
    prev => \$prev_rdata)     # optional
  my rr = new DNS::LDNS::RR(
    file => \*FILE,
    default_ttl => \$ttl,     # optional
    origin => \$origin_rdata, # optional
    prev => \$prev_rdata)     # optional
  my rr = new DNS::LDNS::RR(
    type => LDNS_RR_TYPE_A,
    rdata => [new DNS::LDNS::RData(...), new DNS::LDNS::RData(...), ...],
    class => LDNS_RR_CLASS_IN, # optional
    ttl => 3600, # optional
    owner => new DNS::LDNS::RData(LDNS_RDF_TYPE_DNAME, 'mylabel'), # optional)
  my rr = new DNS::LDNS::RR

  rr2 = rr->clone

  rr->print(\*FILE)
  rr->to_string

  ttl = rr->ttl
  rr->set_ttl(ttl)

  type = rr->type
  rr->set_type(type)

  class = rr->class
  rr->set_class(class)

  rdata = rr->owner
  rr->set_owner(rdata)
  str = rr->dname

  count = rr->rd_count
  rdata = rr->rdata(index)
  rr->set_rdata(rd1, rd2, rd3, ...)
  rr->push_rdata(rd1, rd2, rd3, ...)
  rdata = rr->pop_rdata

  rr->compare(rr2)
  rr->compare_dname(rr2)
  rr->compare_no_rdata(rr2)
  rr->compare_ds(rr2)

  hash = rr->hash_name_from_nsec3(dname)

  status = rr->verify_denial(nsecs, rrsigs)
  status = rr->verify_denial_nsec3(nsecs, rrsigs, packet_rcode, packet_qtype, packet_nodata)
  match = rr->verify_denial_nsec3_match(nsecs, rrsigs, packet_rcode, packet_qtype, packet_nodata)

  rr->nsec3_add_param_rdfs(algorithm, flags, iterations, salt)
  a = rr->nsec3_algorithm
  f = rr->nsec3_flags
  o = rr->nsec3_optout
  i = rr->nsec3_iterations
  rdata = rr->nsec3_next_owner
  rdata = rr->nsec3_bitmap
  rdata = rr->nsec3_salt

  rdata = rr->rrsig_keytag
  bool = rr->rrsig_set_keytag(rdata)
  rdata = rr->rrsig_signame
  bool = rr->rrsig_set_signame(rdata)
  rdata = rr->rrsig_sig
  bool = rr->rrsig_set_sig(rdata)
  rdata = rr->rrsig_algorithm
  bool = rr->rrsig_set_algorithm(rdata)
  rdata = rr->rrsig_inception
  bool = rr->rrsig_set_inception(rdata)
  rdata = rr->rrsig_expiration
  bool = rr->rrsig_set_expiration(rdata)
  rdata = rr->rrsig_labels
  bool = rr->rrsig_set_labels(rdata)
  rdata = rr->rrsig_origttl
  bool = rr->rrsig_set_origttl(rdata)
  key = rr->get_dnskey_for_rrsig(rrlist)

  rdata = rr->dnskey_algorithm
  bool = rr->dnskey_set_algorithm(rdata)
  rdata = rr->dnskey_flags
  bool = rr->dnskey_set_flags(rdata)
  rdata = rr->dnskey_protocol
  bool = rr->dnskey_set_protocol(rdata)
  rdata = rr->dnskey_key
  bool = rr->dnskey_set_key(rdata)
  bits = rr->dnskey_key_size
  tag = rr->calc_keytag
  ds = rr->key_to_ds(hash)

  rr->is_question

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
