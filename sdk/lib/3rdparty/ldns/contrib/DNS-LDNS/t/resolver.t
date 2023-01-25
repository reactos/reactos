use Test::More tests => 3;

use FindBin qw/$Bin/;

use DNS::LDNS ':all';

BEGIN { use_ok('DNS::LDNS') };

my $r = new DNS::LDNS::Resolver(filename => "/etc/resolv.conf");

$r->set_random(0);

my $p = $r->query(
   new DNS::LDNS::RData(LDNS_RDF_TYPE_DNAME, 'org'),
   LDNS_RR_TYPE_SOA, LDNS_RR_CLASS_IN, LDNS_RD);

isa_ok($p, 'DNS::LDNS::Packet', 'Make a simple query');

my $r2 = new DNS::LDNS::Resolver(filename => "$Bin/testdata/resolv.conf");

$r2->set_rtt(2, 3);
my @rtt = $r2->rtt;
is_deeply(\@rtt, [2, 3], "set_rtt and rtt");
