use Test::More tests => 24;

use FindBin qw/$Bin/;

use DNS::LDNS ':all';

BEGIN { use_ok('DNS::LDNS') };

# Create list
my $list = new DNS::LDNS::RRList;
isa_ok($list, 'DNS::LDNS::RRList', 'Create an empty rr list');

# Push/pop/count rr
$list->push(new DNS::LDNS::RR(str => 'ns.myzone.org 3600 IN AAAA ::1'));
is($list->rr_count, 1, 'Added one rr');
like($list->rr(0)->to_string, qr/^ns\.myzone\.org\.\s+3600\s+IN\s+AAAA\s+::1$/, 'Added rr is at position 0');
$list->push(new DNS::LDNS::RR(str => 'ns.myzone.org 7200 IN A 192.168.100.2'));
is($list->rr_count, 2, 'Added another rr');
like($list->rr(1)->to_string, qr/^ns\.myzone\.org\.\s+7200\s+IN\s+A\s+192\.168\.100\.2$/, 'Last added rr is at position 1');
like($list->pop->to_string, qr/^ns\.myzone\.org\.\s+7200\s+IN\s+A\s+192\.168\.100\.2$/, 'pop the last element');
is($list->rr_count, 1, '1 element left in the list');

# Push/pop list
my $l2 = new DNS::LDNS::RRList;
$l2->push(new DNS::LDNS::RR(str => 'ns2.myzone.org 3600 IN A 192.168.100.0'));
$l2->push(new DNS::LDNS::RR(str => 'ns2.myzone.org 3600 IN A 192.168.100.1'));
$list->push_list($l2);
is($list->rr_count, 3, 'Pushed two elements. List count is now 3.');
$list->push_list($l2);
$list->push_list($l2);
my $l3 = $list->pop_list(1);
is($list->rr_count, 6, 'Pushed 4 elements, popped 1, count is now 6');
is($l3->rr_count, 1, 'Popped list contains 1 elements');
$l3 = $list->pop_list(3);
is($list->rr_count, 3, 'Popped 3 elements, count is now 3');
is($l3->rr_count, 3, 'Popped list contains 3 elements');

# RRSets
ok($l2->is_rrset, 'List is rrset');
ok(!$list->is_rrset, 'List is no longer an rrset');
my $rrset = $list->pop_rrset;
ok($rrset->is_rrset, 'Popped list is rrset');
is($rrset->rr_count, 2, 'Popped rrset has two elements.');

# Compare, contains, subtype
my $rr = new DNS::LDNS::RR(str => 'ns2.myzone.org 3600 IN A 192.168.100.0');
ok($rrset->contains_rr($rr), 'RRSet contains rr '.$rr->to_string);
is($list->compare($l2), -1, '$list < $l2');
is($l2->compare($list), 1, '$l2 > $list');

$list->push(new DNS::LDNS::RR(str => 'ns3.myzone.org 3600 IN A 192.168.100.0'),
	    new DNS::LDNS::RR(str => 'ns3.myzone.org 3600 IN A 192.168.100.1'),
	    new DNS::LDNS::RR(str => 'ns4.myzone.org 3600 IN A 192.168.100.1'));
my $subtype = $list->subtype_by_rdata(
    new DNS::LDNS::RData(LDNS_RDF_TYPE_A, '192.168.100.1'), 0);
is($subtype->to_string, "ns3.myzone.org.\t3600\tIN\tA\t192.168.100.1\nns4.myzone.org.\t3600\tIN\tA\t192.168.100.1\n", 'Filter rrs by rdata');

# DNSSec signature verification
my $keylist = new DNS::LDNS::RRList;
$keylist->push(
    new DNS::LDNS::RR(str => 'trondheim.no.           3600    IN      DNSKEY  256 3 8 AwEAAZIDdRI8I+F/J6OT8xX7CbGQYRr8rWH9dvloUlRJXcEVE2pRAez6 pJC5Odg+i2WvDUeE4tUO1gwwjU83TIinZxxsDnqr7FzvqpHeJbVd2N3d S4zaJcbjSnwMqdebmTEXSrflp8DeIAH0GQGNQjhOPubbb/nADYP2RS1i CoOADa8P'),
    new DNS::LDNS::RR(str => 'trondheim.no.           3600    IN      DNSKEY  257 3 8 AwEAAax9EgKyRsMpU2B0E2dZ+nkWnmZHjlBO3uXBI+2x33dG8bk+XSqr kyWTelhhsqLqIxsaYSwYgzLtn+/qzlFjKwcaU95p+Tp95MOVXYqUtRyC VyLGkzA7ZDbx7TFCi3PyLDM/Arx+DvOx6nNvA/erqIU5gYEo9Nm1KXEy rhfSn3xc96p1AOhmTuSo6EfYlPY4gxHDgJdHFv7Fi9zV6VFmJ29h0rsG 5g3pV1lvCcGcxfRLJ1u7JRw2BWMo9lgHzGuypEVV7iLnvbfDlXhF+jAS owR2JxlESC3dOgNiNWvc4pbyVXBXpP6h/5JpcxkzF7BNJMZiLN14qvam G1+LuZM8qfc=')
);

my $soalist = new DNS::LDNS::RRList;
$soalist->push(
    new DNS::LDNS::RR(str => 'trondheim.no.           3600    IN      SOA     charm.norid.no. hostmaster.norid.no. 2013021137 14400 1800 2419200 3600')
);

my $siglist = new DNS::LDNS::RRList;
$siglist->push(
    new DNS::LDNS::RR(str => 'trondheim.no.           3600    IN      RRSIG   SOA 8 2 3600 20130227105101 20130213090318 36381 trondheim.no. NbeN8E4pvQSDk3Dn0i8B4e2A3KAY8JrX+zcJazPTgHbT6wjzCncn3ANn 6rs+HdcCLtptyX1QbzlZD/lOY8kjJw5TEUoFX2Q/2sBYdt1aT6qgt/+H o71iUz3bk1V73zjSG/OpqG0oXmjCWSBZgzK6UI+zGlgG0Kvrc7H1pw5S ZBA=')
);

my ($status, $goodkeys) = $soalist->verify_notime($siglist, $keylist);
is ($status, LDNS_STATUS_OK, 'Verification returned status ok.');
is ($goodkeys->rr_count, 1, 'One key matched the signature.');

my $klist = new DNS::LDNS::KeyList;
$klist->push(new DNS::LDNS::Key(filename => "$Bin/testdata/key.private"));
$klist->key(0)->set_pubkey_owner(
	new DNS::LDNS::RData(LDNS_RDF_TYPE_DNAME, 'myzone.org'));
my $sigs = $l2->sign_public($klist);
is($sigs->rr_count, 1, 'Sign public, got 1 rrsig');
