package DNS::LDNS::GC;

use strict;
use warnings;

our $VERSION = '0.61';

my %ref_count;
my %owned_by;

sub own {
    my ($obj, $owner) = @_;

#    print STDERR "Owning $obj -> $owner\n";
    return unless (defined $obj);

    if ($owned_by{$$owner}) {
	# If the owner is an owned object, let obj be owned by
	# the owners owner. We want to avoid recursive ownerships.
	$owner = $owned_by{$$owner};
    }

    if (exists $owned_by{$$obj}) {
	$ref_count{$$obj}++;
    }
    else {
	$ref_count{$$obj} = 1;
	$owned_by{$$obj} = $owner;
    }
    return $obj;
}

# Return true if the object is owned by someone
sub is_owned {
    return (exists $owned_by{${$_[0]}});
}

sub owner {
    return $owned_by{${$_[0]}};
}

sub disown {
    return unless (defined $_[0]);
    delete $owned_by{${$_[0]}};
}

my %free_method = (
    'DNS::LDNS::Zone'         => '_zone_deep_free',
    'DNS::LDNS::RRList'       => '_rrlist_deep_free',
    'DNS::LDNS::RR'           => '_rr_free',
    'DNS::LDNS::RData'        => '_rdata_deep_free',
    'DNS::LDNS::DNSSecZone'   => '_dnssec_zone_deep_free',
    'DNS::LDNS::DNSSecName'   => '_dnssec_name_deep_free',
    'DNS::LDNS::Resolver'     => '_resolver_deep_free',
    'DNS::LDNS::Packet'       => '_packet_free',
    'DNS::LDNS::Key'          => '_key_deep_free',
    'DNS::LDNS::KeyList'      => '_keylist_free',
    'DNS::LDNS::DNSSecDataChain' => '_dnssec_datachain',
);

my %not_deleted_by_owner = (
    'DNS::LDNS::DNSSecTrustChain' => 1,
);

sub free {
    my $obj = shift;

#    print STDERR "Freeing $obj\n";

    if (exists $ref_count{$$obj}) {
#	print STDERR "Derefing $obj\n";
	$ref_count{$$obj}--;
	return if ($ref_count{$$obj} > 0);
    }

#    print STDERR "Deleting $obj\n";

    delete $ref_count{$$obj};

    if (exists $owned_by{$$obj}) {
	delete $owned_by{$$obj};
	return unless ($not_deleted_by_owner{ref $obj});
    }

    my $class = ref $obj;
    my $free = $free_method{ref $obj};

    die "Internal error: No freeing method for $obj (".ref $obj.")"
	unless ($free);

    no strict;
    &$free($obj);
}

1;
__END__

=head1 NAME

DNS::LDNS::GC - Garbage collector, used internally by the DNS::LDNS modules

=head1 SYNOPSIS

Garbage collector class for DNS::LDNS objects.

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
