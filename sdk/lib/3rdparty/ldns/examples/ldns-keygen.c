/*
 * keygen is a small programs that generate a dnskey and private key
 * for a particular domain.
 *
 * (c) NLnet Labs, 2005 - 2008
 * See the file LICENSE for the license
 */

#include "config.h"

#include <ldns/ldns.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#ifdef HAVE_SSL
static void
usage(FILE *fp, char *prog) {
	fprintf(fp, "%s -a <algorithm> [-b bits] [-r /dev/random] [-s] [-f] [-v] domain\n",
		   prog);
	fprintf(fp, "  generate a new key pair for domain\n");
	fprintf(fp, "  -a <alg>\tuse the specified algorithm (-a list to");
	fprintf(fp, " show a list)\n");
	fprintf(fp, "  -k\t\tset the flags to 257; key signing key\n");
	fprintf(fp, "  -b <bits>\tspecify the keylength\n");
	fprintf(fp, "  -r <random>\tspecify a random device (defaults to /dev/random)\n");
	fprintf(fp, "\t\tto seed the random generator with\n");
	fprintf(fp, "  -s\t\tcreate additional symlinks with constant names\n");
	fprintf(fp, "  -f\t\tforce override of existing symlinks\n");
	fprintf(fp, "  -v\t\tshow the version and exit\n");
	fprintf(fp, "  The following files will be created:\n");
	fprintf(fp, "    K<name>+<alg>+<id>.key\tPublic key in RR format\n");
	fprintf(fp, "    K<name>+<alg>+<id>.private\tPrivate key in key format\n");
	fprintf(fp, "    K<name>+<alg>+<id>.ds\tDS in RR format (only for DNSSEC KSK keys)\n");
	fprintf(fp, "  The base name (K<name>+<alg>+<id> will be printed to stdout\n");
}

static void
show_algorithms(FILE *out)
{
	ldns_lookup_table *lt = ldns_signing_algorithms;
	fprintf(out, "Possible algorithms:\n");

	while (lt->name) {
		fprintf(out, "%s\n", lt->name);
		lt++;
	}
}

static int
remove_symlink(const char *symlink_name)
{
	int result;

	if ((result = unlink(symlink_name)) == -1) {
		if (errno == ENOENT) {
			/* it's OK if the link simply didn't exist */
			result = 0;
		} else {
			/* error if unlink fail */
			fprintf(stderr, "Can't delete symlink %s: %s\n", symlink_name, strerror(errno));
		}
	}
	return result;
}

static int
create_symlink(const char *symlink_destination, const char *symlink_name)
{
	int result = 0;

	if (!symlink_name)
		return result;  /* no arg "-s" at all */

	if ((result = symlink(symlink_destination, symlink_name)) == -1) {
		fprintf(stderr, "Unable to create symlink %s -> %s: %s\n", symlink_name, symlink_destination, strerror(errno));
	}
	return result;
}

int
main(int argc, char *argv[])
{
	int c;
	int fd;
	char *prog;

	/* default key size */
	uint16_t def_bits = 1024;
	uint16_t bits = def_bits;
	bool had_bits = false;
	bool ksk;

	FILE *file;
	FILE *random;
	char *filename;
	char *owner;
	bool symlink_create;
	bool symlink_override;

	ldns_signing_algorithm algorithm;
	ldns_rdf *domain;
	ldns_rr *pubkey;
	ldns_key *key;
	ldns_rr *ds;

	prog = _strdup(argv[0]);
	algorithm = 0;
	random = NULL;
	ksk = false; /* don't create a ksk per default */
	symlink_create = false;
	symlink_override = false;

	while ((c = getopt(argc, argv, "a:kb:r:sfv")) != -1) {
		switch (c) {
		case 'a':
			if (algorithm != 0) {
				fprintf(stderr, "The -a argument can only be used once\n");
				exit(1);
			}
			if (strncmp(optarg, "list", 5) == 0) {
				show_algorithms(stdout);
				exit(EXIT_SUCCESS);
			}
			algorithm = ldns_get_signing_algorithm_by_name(optarg);
			if (algorithm == 0) {
				fprintf(stderr, "Algorithm %s not found\n", optarg);
				show_algorithms(stderr);
				exit(EXIT_FAILURE);
			}
			break;
		case 'b':
			bits = (uint16_t) atoi(optarg);
			if (bits == 0) {
				fprintf(stderr, "%s: %s %d", prog, "Can not parse the -b argument, setting it to the default\n", (int) def_bits);
				bits = def_bits;
			} else
				had_bits = true;
			break;
		case 'k':
			ksk = true;
			break;
		case 'r':
			random = fopen(optarg, "r");
			if (!random) {
				fprintf(stderr, "Cannot open random file %s: %s\n", optarg, strerror(errno));
				exit(EXIT_FAILURE);
			}
			break;
		case 's':
			symlink_create = true;
			break;
		case 'f':
			symlink_override = true;
			break;
		case 'v':
			printf("DNSSEC key generator version %s (ldns version %s)\n", LDNS_VERSION, ldns_version());
			exit(EXIT_SUCCESS);
			break;
		default:
			usage(stderr, prog);
			exit(EXIT_FAILURE);
		}
	}
	argc -= optind;
	argv += optind;

	if (algorithm == 0) {
		printf("Please use the -a argument to provide an algorithm\n");
		exit(1);
	}

	if (argc != 1) {
		usage(stderr, prog);
		exit(EXIT_FAILURE);
	}
	free(prog);

	/* check whether key size is within RFC boundaries */
	switch (algorithm) {
	case LDNS_SIGN_RSAMD5:
	case LDNS_SIGN_RSASHA1:
	case LDNS_SIGN_RSASHA1_NSEC3:
	case LDNS_SIGN_RSASHA256:
	case LDNS_SIGN_RSASHA512:
		if (bits < 512 || bits > 4096) {
			fprintf(stderr, "For RSA, the key size must be between ");
			fprintf(stderr, " 512 and 4096 bits. Aborting.\n");
			exit(1);
		}
		break;
#ifdef USE_DSA
	case LDNS_SIGN_DSA:
	case LDNS_SIGN_DSA_NSEC3:
		if (bits < 512 || bits > 1024) {
			fprintf(stderr, "For DSA, the key size must be between ");
			fprintf(stderr, " 512 and 1024 bits. Aborting.\n");
			exit(1);
		}
		break;
#endif /* USE_DSA */
#ifdef USE_GOST
	case LDNS_SIGN_ECC_GOST:
		if(!ldns_key_EVP_load_gost_id()) {
			fprintf(stderr, "error: libcrypto does not provide GOST\n");
			exit(EXIT_FAILURE);
		}
		break;
#endif
#ifdef USE_ECDSA
	case LDNS_SIGN_ECDSAP256SHA256:
	case LDNS_SIGN_ECDSAP384SHA384:
		break;
#endif
	case LDNS_SIGN_HMACMD5:
		if (!had_bits) {
			bits = 512;
		} else if (bits < 1 || bits > 512) {
			fprintf(stderr, "For hmac-md5, the key size must be ");
			fprintf(stderr, "between 1 and 512 bits. Aborting.\n");
			exit(1);
		}
		break;
	case LDNS_SIGN_HMACSHA1:
		if (!had_bits) {
			bits = 160;
		} else if (bits < 1 || bits > 160) {
			fprintf(stderr, "For hmac-sha1, the key size must be ");
			fprintf(stderr, "between 1 and 160 bits. Aborting.\n");
			exit(1);
		}
		break;

	case LDNS_SIGN_HMACSHA224:
		if (!had_bits) {
			bits = 224;
		} else if (bits < 1 || bits > 224) {
			fprintf(stderr, "For hmac-sha224, the key size must be ");
			fprintf(stderr, "between 1 and 224 bits. Aborting.\n");
			exit(1);
		}
		break;

	case LDNS_SIGN_HMACSHA256:
		if (!had_bits) {
			bits = 256;
		} else if (bits < 1 || bits > 256) {
			fprintf(stderr, "For hmac-sha256, the key size must be ");
			fprintf(stderr, "between 1 and 256 bits. Aborting.\n");
			exit(1);
		}
		break;

	case LDNS_SIGN_HMACSHA384:
		if (!had_bits) {
			bits = 384;
		} else if (bits < 1 || bits > 384) {
			fprintf(stderr, "For hmac-sha384, the key size must be ");
			fprintf(stderr, "between 1 and 384 bits. Aborting.\n");
			exit(1);
		}
		break;

	case LDNS_SIGN_HMACSHA512:
		if (!had_bits) {
			bits = 512;
		} else if (bits < 1 || bits > 512) {
			fprintf(stderr, "For hmac-sha512, the key size must be ");
			fprintf(stderr, "between 1 and 512 bits. Aborting.\n");
			exit(1);
		}
		break;
	default:
		break;
	}

	if (!random) {
		random = fopen("/dev/random", "r");
		if (!random) {
			fprintf(stderr, "Cannot open random file %s: %s\n", optarg, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	(void)ldns_init_random(random, (unsigned int) bits/8);
	fclose(random);

	/* create an rdf from the domain name */
	domain = ldns_dname_new_frm_str(argv[0]);

	/* generate a new key */
	key = ldns_key_new_frm_algorithm(algorithm, bits);
	if(!key) {
		fprintf(stderr, "cannot generate key of algorithm %s\n",
			ldns_pkt_algorithm2str((ldns_algorithm)algorithm));
		exit(EXIT_FAILURE);
	}

	/* set the owner name in the key - this is a /separate/ step */
	ldns_key_set_pubkey_owner(key, domain);

	/* ksk flag */
	if (ksk) {
		ldns_key_set_flags(key, ldns_key_flags(key) + 1);
	}

	/* create the public from the ldns_key */
	pubkey = ldns_key2rr(key);
	if (!pubkey) {
		fprintf(stderr, "Could not extract the public key from the key structure...");
		ldns_key_deep_free(key);
		exit(EXIT_FAILURE);
	}
	owner = ldns_rdf2str(ldns_rr_owner(pubkey));

	/* calculate and set the keytag */
	ldns_key_set_keytag(key, ldns_calc_keytag(pubkey));

	/* build the DS record */
	switch (algorithm) {
#ifdef USE_ECDSA
	case LDNS_SIGN_ECDSAP384SHA384:
		ds = ldns_key_rr2ds(pubkey, LDNS_SHA384);
		break;
	case LDNS_SIGN_ECDSAP256SHA256:
#endif
#ifdef USE_ED25519
	case LDNS_SIGN_ED25519:
#endif
#ifdef USE_ED448
	case LDNS_SIGN_ED448:
#endif
	case LDNS_SIGN_RSASHA256:
	case LDNS_SIGN_RSASHA512:
		ds = ldns_key_rr2ds(pubkey, LDNS_SHA256);
		break;
	case LDNS_SIGN_ECC_GOST:
#ifdef USE_GOST
		ds = ldns_key_rr2ds(pubkey, LDNS_HASH_GOST);
#else
		ds = ldns_key_rr2ds(pubkey, LDNS_SHA256);
#endif
		break;
	default:
		ds = ldns_key_rr2ds(pubkey, LDNS_SHA1);
		break;
	}

	/* maybe a symlinks should be removed */
	if (symlink_create && symlink_override) {
		if (remove_symlink(".key") != 0) {
			exit(EXIT_FAILURE);
		}
		if (remove_symlink(".private") != 0) {
			exit(EXIT_FAILURE);
		}
		if (remove_symlink(".ds") != 0) {
			exit(EXIT_FAILURE);
		}
	}

	/* print the public key RR to .key */
	filename = LDNS_XMALLOC(char, strlen(owner) + 17);
	snprintf(filename, strlen(owner) + 16, "K%s+%03u+%05u.key", owner, algorithm, (unsigned int) ldns_key_keytag(key));
	file = fopen(filename, "w");
	if (!file) {
		fprintf(stderr, "Unable to open %s: %s\n", filename, strerror(errno));
		ldns_key_deep_free(key);
		free(owner);
		ldns_rr_free(pubkey);
		ldns_rr_free(ds);
		LDNS_FREE(filename);
		exit(EXIT_FAILURE);
	} else {
		/* temporarily set question so that TTL is not printed */
		ldns_rr_set_question(pubkey, true);
		ldns_rr_print(file, pubkey);
		ldns_rr_set_question(pubkey, false);
		fclose(file);
		if (symlink_create) {
			if (create_symlink(filename, ".key") != 0) {
				goto silentfail;
			}
		}
		LDNS_FREE(filename);
	}

	/* print the priv key to stderr */
	filename = LDNS_XMALLOC(char, strlen(owner) + 21);
	snprintf(filename, strlen(owner) + 20, "K%s+%03u+%05u.private", owner, algorithm, (unsigned int) ldns_key_keytag(key));
	/* use open() here to prevent creating world-readable private keys (CVE-2014-3209)*/
	fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		goto fail;
	}

	file = fdopen(fd, "w");
	if (!file) {
		goto fail;
	}

	ldns_key_print(file, key);
	fclose(file);
	if (symlink_create) {
		if (create_symlink(filename, ".private") != 0) {
			goto silentfail;
		}
	}
	LDNS_FREE(filename);

	/* print the DS to .ds */
	if (ksk && algorithm != LDNS_SIGN_HMACMD5 &&
		algorithm != LDNS_SIGN_HMACSHA1 &&
		algorithm != LDNS_SIGN_HMACSHA224 &&
		algorithm != LDNS_SIGN_HMACSHA256 &&
		algorithm != LDNS_SIGN_HMACSHA384 &&
		algorithm != LDNS_SIGN_HMACSHA512) {
		filename = LDNS_XMALLOC(char, strlen(owner) + 16);
		snprintf(filename, strlen(owner) + 15, "K%s+%03u+%05u.ds", owner, algorithm, (unsigned int) ldns_key_keytag(key));
		file = fopen(filename, "w");
		if (!file) {
			fprintf(stderr, "Unable to open %s: %s\n", filename, strerror(errno));
			ldns_key_deep_free(key);
			free(owner);
			ldns_rr_free(pubkey);
			ldns_rr_free(ds);
			LDNS_FREE(filename);
			exit(EXIT_FAILURE);
		} else {
			/* temporarily set question so that TTL is not printed */
			ldns_rr_set_question(ds, true);
			ldns_rr_print(file, ds);
			ldns_rr_set_question(ds, false);
			fclose(file);
			if (symlink_create) {
				if (create_symlink(filename, ".ds") != 0) {
					goto silentfail;
				}
			}
			LDNS_FREE(filename);
		}
	}

	fprintf(stdout, "K%s+%03u+%05u\n", owner, algorithm, (unsigned int) ldns_key_keytag(key));
	ldns_key_deep_free(key);
	free(owner);
	ldns_rr_free(pubkey);
	ldns_rr_free(ds);
	exit(EXIT_SUCCESS);

fail:
	fprintf(stderr, "Unable to open %s: %s\n", filename, strerror(errno));
silentfail:
	ldns_key_deep_free(key);
	free(owner);
	ldns_rr_free(pubkey);
	ldns_rr_free(ds);
	LDNS_FREE(filename);
	exit(EXIT_FAILURE);
}
#else
int
main(int argc, char **argv)
{
	fprintf(stderr, "ldns-keygen needs OpenSSL support, which has not been compiled in\n");
	return 1;
}
#endif /* HAVE_SSL */
