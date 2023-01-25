package DNS::LDNS::RData;

use 5.008008;
use strict;
use warnings;

use DNS::LDNS;

our $VERSION = '0.61';

sub new {
    my ($class, $type, $str) = @_;
    return _new($type, $str);
}

sub cat {
    my ($self, $other) = @_;

    my $s = _cat($self, $other);
    $DNS::LDNS::last_status = $s;
    return $s;
}

sub nsec3_hash_name {
    my ($self, $algorithm, $iterations, $salt) = @_;
    return DNS::LDNS::GC::own(
	$self->_nsec3_hash_name($algorithm, $iterations, $salt), $self);
}

sub DESTROY {
    DNS::LDNS::GC::free($_[0]);
}

1;
__END__

=head1 NAME

DNS::LDNS::RData - Rdata field or a dname in an rr

=head1 SYNOPSIS

  use DNS::LDNS ':all'

  my rd = new DNS::LDNS::RData(rdf_type, str)
  rd2 = rd->clone

  rdf_type = rd->type
  rd->set_type(rdf_type)

  rd->print(\*FILE)
  str = rd->to_string

  count = rd->label_count
  rd2 = rd->label(pos)

  bool = rd->is_wildcard
  bool = rd->matches_wildcard(wildcard)
  bool = rd->is_subdomain(parent)

  rd2 = rd->left_chop

  status = rd->cat(rd2)
  rd->compare(rd2)
  rd2 = rd->address_reverse
  rd2 = rd->dname_reverse

  rd2 = rd->nsec3_hash_name(name, algorithm, iterations, salt)

  epoch = rd->to_unix_time
( epoch = rd->2native_time_t )

  rr_type = rd->to_rr_type

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
