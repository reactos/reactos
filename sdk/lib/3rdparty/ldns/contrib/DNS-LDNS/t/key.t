use Test::More tests => 8;

use FindBin qw/$Bin/;

use DNS::LDNS ':all';

BEGIN { use_ok('DNS::LDNS') };

my $key = new DNS::LDNS::Key(filename => "$Bin/testdata/key.private");
ok($key, 'Created new key object from file');
is($key->algorithm, 7, 'Algorithm is NSEC3RSASHA1');
my $now = time;
$key->set_inception($now);
$key->set_expiration($now + 10000);
is($key->inception, $now, 'Inception time');
is($key->expiration, $now + 10000, 'Expiration time');
like($key->to_rr->to_string, qr|3600\s+IN\s+DNSKEY\s+256\s+3\s+7\s+AwEAAfg/ghOkk|, 'Got rr representation of key');

my $klist = new DNS::LDNS::KeyList;
$klist->push($key);
is($klist->count, 1, 'Keylist has one key');
is($$key, ${$klist->key(0)}, 'Key in keylist is the one we pushed');
# FIXME: pop is buggy in ldns 1.6.12, uncomment when this starts working
# is($klist->pop(), $$key, 'Pop key from list');
# is($klist->count, 0, 'No keys left in list');
