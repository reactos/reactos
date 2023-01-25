use Test::More tests => 10;
use Test::Exception;

use FindBin qw/$Bin/;

use DNS::LDNS ':all';

BEGIN { use_ok('DNS::LDNS') };

# Note: This test makes queries on real internet dns data, and assumes
# that the iis.se domain is signed.

my $r = new DNS::LDNS::Resolver(filename => "/etc/resolv.conf");
$r->set_dnssec(1);
$r->set_random(0);

my $p = $r->query(
    new DNS::LDNS::RData(LDNS_RDF_TYPE_DNAME, 'iis.se.'),
    LDNS_RR_TYPE_SOA, LDNS_RR_CLASS_IN, LDNS_RD);

SKIP: {
    skip "Resolver is not dnssec able. Skip this test.", 9 unless ($p->ad);

    isa_ok($p, 'DNS::LDNS::Packet');

    my $rrset = $p->rr_list_by_type(LDNS_RR_TYPE_SOA, LDNS_SECTION_ANSWER);

    ok($rrset->rr_count > 0, 'Got an answer with some content');

    my $chain = $r->build_data_chain(LDNS_RD, $rrset, $p, undef);

    isa_ok($chain, 'DNS::LDNS::DNSSecDataChain');

    isa_ok($chain->parent, 'DNS::LDNS::DNSSecDataChain');

    dies_ok { 
        my $new_rr = new DNS::LDNS::RR(str => 'test.test. 1234 IN A 10.0.0.1');
        my $t = $chain->derive_trust_tree($new_rr); 
    } 'Making a trust tree with foreign rr fails.';

    my $rr = $chain->rrset->rr(0);

    my $tree = $chain->derive_trust_tree($rr);

    isa_ok($tree, 'DNS::LDNS::DNSSecTrustTree');

    # Get root keys.
    my $root_keys_pk = $r->query(
        new DNS::LDNS::RData(LDNS_RDF_TYPE_DNAME, '.'),
        LDNS_RR_TYPE_DNSKEY, LDNS_RR_CLASS_IN, LDNS_RD);
    my $root_keys = $root_keys_pk->rr_list_by_type(
        LDNS_RR_TYPE_DNSKEY, LDNS_SECTION_ANSWER);

    is($tree->contains_keys($root_keys), LDNS_STATUS_OK, 
        'Root key found in trust chain');

    ok($tree->depth > 1, 'The trust tree is more than one node.');

    isa_ok($tree->parent(0), 'DNS::LDNS::DNSSecTrustTree');
}
