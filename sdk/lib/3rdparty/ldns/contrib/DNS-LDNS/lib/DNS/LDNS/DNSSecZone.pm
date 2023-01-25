package DNS::LDNS::DNSSecZone;

use 5.008008;
use strict;
use warnings;

use DNS::LDNS ':all';

our $VERSION = '0.61';

sub new {
    my ($class, %args) = @_;

    my $line_nr;
    my $status = &LDNS_STATUS_OK;
    my $zone;
    my $file;

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

    if ($file) {
	$zone = _new_from_file($file, 
			       $args{origin}, 
			       $args{ttl} || 0, 
			       $args{class} || 0, 
			       $status, $line_nr);
    }
    else {
	$zone = _new();
    }

    if ($args{filename}) {
	close $file;
    }

    $DNS::LDNS::last_status = $status;
    $DNS::LDNS::line_nr = $line_nr;
    if (!defined $zone) {
	return;
    }

    return $zone;
}

sub soa {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_soa, $self);
}

sub names {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_names, $self);
}

sub find_rrset {
    my ($self, $name, $type) = @_;
    return DNS::LDNS::GC::own($self->_find_rrset($name, $type), $self);
}

sub add_rr {
    my ($self, $rr) = @_;

    # Set a copy of the rr in case it is already owned
    my $s = _add_rr($self, my $copy = $rr->clone);
    $DNS::LDNS::last_status = $s;
    DNS::LDNS::GC::own($copy, $self);
    return $s;
}

sub add_empty_nonterminals {
    my $self = shift;
    my $s = _add_empty_nonterminals($self);
    $DNS::LDNS::last_status = $s;
    return $s;
}

sub mark_glue {
    my $self = shift;
    my $s = _mark_glue($self);
    $DNS::LDNS::last_status = $s;
    return $s;
}

sub sign {
    my ($self, $keylist, $policy, $flags) = @_;
    my $s = _sign($self, $keylist, $policy, $flags);
    $DNS::LDNS::last_status = $s;
    return $s;
}

sub sign_nsec3 {
    my ($self, $keylist, $policy, $algorithm, $flags, $iterations, $salt,
	$signflags) = @_;
    my $s = _sign_nsec3($self, $keylist, $policy, $algorithm, $flags, 
	$iterations, $salt, $signflags);
    $DNS::LDNS::last_status = $s;
    return $s;
}

sub to_string {
    return "DNS::LDNS::DNSSecZone::to_string is not yet implemented";
}

sub DESTROY {
    DNS::LDNS::GC::free($_[0]);
}

1;
__END__

=head1 NAME

DNS::LDNS::DNSSecZone - Zone with dnssec data

=head1 SYNOPSIS

  use DNS::LDNS ':all'

  my z = new DNS::LDNS::DNSSecZone(
    filename => '/path/to/myzone',
    origin => new DNS::LDNS::RData(LDNS_RDF_TYPE_DNAME, 'myzone'), #optional
    ttl => 3600, #optional
    class => LDNS_RR_CLASS_, #optional
  )
  my z = new DNS::LDNS::DNSSecZone(
    file => \*FILE,
    origin => ..., ttl => ..., class => ...
  )
  my z = new DNS::LDNS::DNSSecZone

  rr = z->soa
  rbtree = z->names
  rrsets = z->find_rrset
  z->add_rr(rr)
  z->create_from_zone(zone)
  z->add_empty_nonterminals

  z->sign(keylist, policy)
  z->sign_nsec3(keylist, policy, algorithm, flags, iterations, salt)

  z->create_nsecs
  z->create_nsec3s(algorithm, flags, iterations, salt)
  z->create_rrsigs(key_list, policy, flags)

=head1 TODO

  z->to_string

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
