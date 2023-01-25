use Test::More tests => 19;

use FindBin qw/$Bin/;

use DNS::LDNS ':all';

BEGIN { use_ok('DNS::LDNS') };

my $rr1 = new DNS::LDNS::RR;
isa_ok($rr1, 'DNS::LDNS::RR', 'Create empty rr');

$rr1 = new DNS::LDNS::RR(
    type => LDNS_RR_TYPE_SOA,
    class => LDNS_RR_CLASS_CH,
    ttl => 1234,
    owner => 'myzone.org',
    rdata => [
	new DNS::LDNS::RData(LDNS_RDF_TYPE_DNAME, 'hostmaster.myzone.org'),
	new DNS::LDNS::RData(LDNS_RDF_TYPE_DNAME, 'master.myzone.org'),
	new DNS::LDNS::RData(LDNS_RDF_TYPE_INT32, '2012113030'),
	new DNS::LDNS::RData(LDNS_RDF_TYPE_PERIOD, '12345'),
	new DNS::LDNS::RData(LDNS_RDF_TYPE_PERIOD, '1827'),
	new DNS::LDNS::RData(LDNS_RDF_TYPE_PERIOD, '2345678'),
	new DNS::LDNS::RData(LDNS_RDF_TYPE_PERIOD, '87654')
    ],
);

isa_ok($rr1, 'DNS::LDNS::RR', 'Create SOA rr with rdata');

like($rr1->to_string, qr/^myzone\.org\.\s+1234\s+CH\s+SOA\s+hostmaster\.myzone\.org\.\s+master\.myzone\.org\.\s+2012113030\s+12345\s+1827\s+2345678\s+87654$/,
     'Format SOA rr as string');

is($rr1->pop_rdata->to_string, '87654', 'pop rdata');
$rr1->push_rdata(new DNS::LDNS::RData(LDNS_RDF_TYPE_PERIOD, '55667'));
is($rr1->rdata(6)->to_string, '55667', 'push_rdata and access rdata by index');

my $rr2 = new DNS::LDNS::RR(str => 'myzone.org. 1234 IN SOA hostmaster.myzone.org. master.myzone.org. 2012 12345 1827 2345678 87654');
isa_ok($rr2, 'DNS::LDNS::RR', 'Create SOA rr from string');
like($rr2->to_string, qr/^myzone\.org\.\s+1234\s+IN\s+SOA\s+hostmaster\.myzone\.org\.\s+master\.myzone\.org\.\s+2012\s+12345\s+1827\s+2345678\s+87654$/,
     'Format it back to string');

ok($rr1->compare($rr2) > 0, 'Compare rr, greater than');
ok($rr2->compare($rr1) < 0, 'Compare rr, less than');
is($rr1->compare($rr1), 0, 'Compare rr, equal');

my $rr3 = new DNS::LDNS::RR(str => 'ozone.org. 1234 IN SOA hostmaster.ozone.org. master.ozone.org. 2012 12345 1827 2345678 87654');

ok($rr3->compare_dname($rr1) > 0, 'Compare dname, greater than');
ok($rr1->compare_dname($rr3) < 0, 'Compare dname, less than');
is($rr1->compare_dname($rr2), 0, 'Compare dname, equal');

# Read records from a zonefile
my $origin = new DNS::LDNS::RData(LDNS_RDF_TYPE_DNAME, '.');
my $prev = $origin->clone;
my $ttl = 0;
my $count = 0;
open(ZONE, "$Bin/testdata/myzone.org");
my $rr4 = new DNS::LDNS::RR(file => \*ZONE, default_ttl => \$ttl,
    origin => \$origin, prev => \$prev);
is($DNS::LDNS::last_status, LDNS_STATUS_SYNTAX_TTL, "Read ttl statement.");
is($ttl, 4500, "TTL is 4500");

$rr4 = new DNS::LDNS::RR(file => \*ZONE, default_ttl => \$ttl,
    origin => \$origin, prev => \$prev);
is($DNS::LDNS::last_status, LDNS_STATUS_SYNTAX_ORIGIN, "Read origin statement.");
is($origin->to_string, "myzone.org.", "Origin is myzone.org.");

while (!eof(\*ZONE)) {
    $rr4 = new DNS::LDNS::RR(file => \*ZONE, default_ttl => \$ttl,
        origin => \$origin, prev => \$prev);
    last unless ($rr4);
    $count++;
}
is($count, 6);
