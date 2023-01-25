package DNS::LDNS::Key;

use 5.008008;
use strict;
use warnings;

use DNS::LDNS ':all';

our $VERSION = '0.61';

sub new {
    my ($class, %args) = @_;

    my $key;

    if ($args{filename} or $args{file}) {
	my $status = &LDNS_STATUS_OK;
	my $line_nr = 0;
	my $file = $args{file};
	if ($args{filename}) {
	    unless (open FILE, $args{filename}) {
		$DNS::LDNS::last_status = &LDNS_STATUS_FILE_ERR;
		return;
	    }
	    $file = \*FILE;
	}

	$key = _new_from_file($file, $line_nr, $status);
	if ($args{filename}) {
	    close $file;
	}

	$DNS::LDNS::last_status = $status;
	$DNS::LDNS::line_nr = $line_nr;
	if (!defined $key) {
	    return;
	}
    }
    else {
	$key = _new();
    }

    return $key;
}

sub set_pubkey_owner {
    my ($self, $owner) = @_;
    my $oldowner = $self->pubkey_owner;
    DNS::LDNS::GC::disown(my $old = $self->pubkey_owner);
    $self->_set_pubkey_owner($owner);
    return DNS::LDNS::GC::own($owner, $self);
}

sub pubkey_owner {
    my $self = shift;
    return DNS::LDNS::GC::own($self->_pubkey_owner, $self);
}

sub DESTROY {
    DNS::LDNS::GC::free($_[0]);
}

1;
__END__

=head1 NAME

DNS::LDNS::Key - DNSSec private key

=head1 SYNOPSIS

  use DNS::LDNS ':all'

  key = new DNS::LDNS::Key
  key = new DNS::LDNS::Key(file => \*FILE)
  key = new DNS::LDNS::Key(filename => 'keyfile')

  str = key->to_string
  key->print(\*OUTPUT)

  key->set_algorithm(alg)
  alg = key->algorithm
  key->set_flags(flags)
  flags = key->flags
  key->set_hmac_key(hmac)
  hmac = key->hmac_key
  key->set_hmac_size(size)
  size = key->hmac_size
  key->set_origttl(ttl)
  ttl = key->origttl
  key->set_inception(epoch)
  epoch = key->inception
  key->set_expiration(epoch)
  epoch = key->expiration
  key->set_pubkey_owner(rdata)
  rdata = key->pubkey_owner
  key->set_keytag(tag)
  tag = key->keytag
  key->set_use(bool)
  bool = key->use

  str = key->get_file_base_name

  rr = key->to_rr

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
