<?php
/**
 * Rebuild interwiki table using the file on meta and the language list
 * Wikimedia specific!
 *
 * @file
 * @todo document
 * @ingroup Maintenance
 */

/**
 * @todo document
 * @ingroup Maintenance
 */
class Site {
	var $suffix, $lateral, $url;

	function __construct( $s, $l, $u ) {
		$this->suffix = $s;
		$this->lateral = $l;
		$this->url = $u;
	}

	function getURL( $lang ) {
		$xlang = str_replace( '_', '-', $lang );
		return "http://$xlang.{$this->url}/wiki/\$1";
	}
}

function getRebuildInterwikiDump() {
	global $langlist, $languageAliases, $prefixRewrites;

	# Multi-language sites
	# db suffix => db suffix, iw prefix, hostname
	$sites = array(
		'wiki' => new Site( 'wiki', 'w', 'wikipedia.org' ),
		'wiktionary' => new Site( 'wiktionary', 'wikt', 'wiktionary.org' ),
		'wikiquote' => new Site( 'wikiquote', 'q', 'wikiquote.org' ),
		'wikibooks' => new Site( 'wikibooks', 'b', 'wikibooks.org' ),
		'wikinews' => new Site( 'wikinews', 'n', 'wikinews.org' ),
		'wikisource' => new Site( 'wikisource', 's', 'wikisource.org' ),
		'wikimedia' => new Site( 'wikimedia', 'chapter', 'wikimedia.org' ),
		'wikiversity' => new Site( 'wikiversity', 'v', 'wikiversity.org' ),
	);

	# List of language prefixes likely to be found in multi-language sites
	$langlist = array_map( "trim", file( "/home/wikipedia/common/langlist" ) );

	# List of all database names
	$dblist = array_map( "trim", file( "/home/wikipedia/common/all.dblist" ) );

	# Special-case databases
	$specials = array_flip(
		array_map( "trim",
			file( "/home/wikipedia/common/special.dblist" ) ) );

	# Extra interwiki links that can't be in the intermap for some reason
	$extraLinks = array(
		array( 'm', 'http://meta.wikimedia.org/wiki/$1', 1 ),
		array( 'meta', 'http://meta.wikimedia.org/wiki/$1', 1 ),
		array( 'sep11', 'http://sep11.wikipedia.org/wiki/$1', 1 ),
	);

	# Language aliases, usually configured as redirects to the real wiki in apache
	# Interlanguage links are made directly to the real wiki
	# Something horrible happens if you forget to list an alias here, I can't
	#   remember what
	$languageAliases = array(
		'zh-cn' => 'zh',
		'zh-tw' => 'zh',
		'dk' => 'da',
		'nb' => 'no',
	);

	# Special case prefix rewrites, for the benefit of Swedish which uses s:t
	# as an abbreviation for saint
	$prefixRewrites = array(
                'svwiki' => array ( 's' => 'src'),
	); 

	# Construct a list of reserved prefixes
	$reserved = array();
	foreach ( $langlist as $lang ) {
		$reserved[$lang] = 1;
	}
	foreach ( $languageAliases as $alias => $lang ) {
		$reserved[$alias] = 1;
	}
	foreach( $sites as $site ) {
		$reserved[$site->lateral] = 1;
	}

	# Extract the intermap from meta
	$intermap = Http::get( 'http://meta.wikimedia.org/w/index.php?title=Interwiki_map&action=raw', 30 );
	$lines = array_map( 'trim', explode( "\n", trim( $intermap ) ) );

	if ( !$lines || count( $lines ) < 2 ) {
		wfDie( "m:Interwiki_map not found" );
	}

	# Global iterwiki map
	foreach ( $lines as $line ) {
		if ( preg_match( '/^\|\s*(.*?)\s*\|\|\s*(.*?)\s*$/', $line, $matches ) ) {
			$prefix = strtolower( $matches[1] );
			$url = $matches[2];
			if ( preg_match( '/(wikipedia|wiktionary|wikisource|wikiquote|wikibooks|wikimedia)\.org/', $url ) ) {
				$local = 1;
			} else {
				$local = 0;
			}

			if ( empty( $reserved[$prefix] ) ) {
			    $imap  = array( "iw_prefix" => $prefix, "iw_url" => $url, "iw_local" => $local );
                            makeLink ($imap, "__global");
			}
		}
	}

        # Exclude Wikipedia for Wikipedia
        makeLink ( array ('iw_prefix' => 'wikipedia', 'is_url' => null ), "_wiki" );
        
        #Multilanguage sites
        foreach ($sites as $site) 
            makeLanguageLinks ( $site, "_".$site->suffix );


        foreach ( $dblist as $db ) {
		if ( isset( $specials[$db] ) ) {
			# Special wiki
			# Has interwiki links and interlanguage links to wikipedia

                        makeLink( array( 'iw_prefix' => $db, 'iw_url' => "wiki"), "__sites" );
			# Links to multilanguage sites
			foreach ( $sites as $targetSite ) {
				makeLink( array( 'iw_prefix' => $targetSite->lateral, 
                                                    'iw_url' =>$targetSite->getURL( 'en' ),
                                                    'iw_local' => 1 ), $db );
			}

		} else {
			# Find out which site this DB belongs to
			$site = false;
			foreach( $sites as $candidateSite ) {
				$suffix = $candidateSite->suffix;
				if ( preg_match( "/(.*)$suffix$/", $db, $matches ) ) {
				$site = $candidateSite;
					break;
				}
			}
                        makeLink( array( 'iw_prefix' => $db, 'iw_url' => $site->suffix), "__sites" );
			if ( !$site ) {
				print "Invalid database $db\n";
				continue;
			}
			$lang = $matches[1];

			# Lateral links
			foreach ( $sites as $targetSite ) {
				if ( $targetSite->suffix != $site->suffix ) {
					makeLink( array( 'iw_prefix' => $targetSite->lateral, 
                                                            'iw_url' => $targetSite->getURL( $lang ), 
                                                            'iw_local' => 1 ), $db );
				}
			}

			if ( $site->suffix == "wiki" ) {
				makeLink( array('iw_prefix' => 'w', 
                                                'iw_url' => "http://en.wikipedia.org/wiki/$1", 
                                                'iw_local' => 1), $db );
			}

		}
	}
	foreach ( $extraLinks as $link )
		makeLink( $link, "__global" );
}

# ------------------------------------------------------------------------------------------

# Executes part of an INSERT statement, corresponding to all interlanguage links to a particular site
function makeLanguageLinks( &$site, $source ) {
	global $langlist, $languageAliases;
	# Actual languages with their own databases
	foreach ( $langlist as $targetLang ) {
		makeLink( array( $targetLang, $site->getURL( $targetLang ), 1 ), $source );
	}

	# Language aliases
	foreach ( $languageAliases as $alias => $lang ) {
		makeLink( array( $alias, $site->getURL( $lang ), 1 ), $source );
	}
}

function makeLink( $entry, $source ) {
	global $prefixRewrites, $dbFile;
	if ( isset( $prefixRewrites[$source] ) && isset( $prefixRewrites[$source][$entry[0]] ) )
		$entry[0] = $prefixRewrites[$source][$entry[0]];
        if (!array_key_exists("iw_prefix",$entry))
            $entry = array("iw_prefix" => $entry[0], "iw_url" => $entry[1], "iw_local" => $entry[2]);
        if ( array_key_exists($source,$prefixRewrites) && 
                array_key_exists($entry['iw_prefix'],$prefixRewrites[$source]))
                    $entry['iw_prefix'] = $prefixRewrites[$source][$entry['iw_prefix']];
        if ($dbFile)
            dba_insert("{$source}:{$entry['iw_prefix']}", trim("{$entry['iw_local']} {$entry['iw_url']}"),$dbFile);
        else
            print "{$source}:{$entry['iw_prefix']} {$entry['iw_url']} {$entry['iw_local']}\n";

    }
