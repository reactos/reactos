package DNS::LDNS::KeyList;

use 5.008008;
use strict;
use warnings;

use DNS::LDNS ':all';

our $VERSION = '0.61';

sub new {
    my $class = shift;
    
    return _new();
}

sub push {
    my ($self, @keys) = @_;

    for my $k (@keys) {
	if (DNS::LDNS::GC::is_owned($k)) {
	    die "Cannot push a key on multiple lists.";
	}
	$self->_push($k);
	DNS::LDNS::GC::own($k, $self);
    }
}

sub key {
    my ($self, $index) = @_;
    return DNS::LDNS::GC::own($self->_key($index), $self);
}

sub DESTROY {
    DNS::LDNS::GC::free($_[0]);
}

1;
__END__

=head1 NAME

DNS::LDNS::KeyList - Linked list of dnssec keys

=head1 SYNOPSIS

  use DNS::LDNS ':all'

  my l = new DNS::LDNS::KeyList
  l->set_use(bool)
  l->push(@keys)
  key = l->pop
  c = l->count
  key = l->key(index)

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
