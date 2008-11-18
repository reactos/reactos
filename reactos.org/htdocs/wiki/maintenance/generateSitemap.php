<?php
define( 'GS_MAIN', -2 );
define( 'GS_TALK', -1 );
/**
 * Creates a sitemap for the site
 *
 * @ingroup Maintenance
 *
 * @copyright Copyright © 2005, Ævar Arnfjörð Bjarmason
 * @copyright Copyright © 2005, Jens Frank <jeluf@gmx.de>
 * @copyright Copyright © 2005, Brion Vibber <brion@pobox.com>
 *
 * @see http://www.sitemaps.org/
 * @see http://www.sitemaps.org/schemas/sitemap/0.9/sitemap.xsd
 *
 * @license http://www.gnu.org/copyleft/gpl.html GNU General Public License 2.0 or later
 */

class GenerateSitemap {
	/**
	 * The maximum amount of urls in a sitemap file
	 *
	 * @link http://www.sitemaps.org/schemas/sitemap/0.9/sitemap.xsd
	 *
	 * @var int
	 */
	var $url_limit;

	/**
	 * The maximum size of a sitemap file
	 *
	 * @link http://www.sitemaps.org/faq.php#faq_sitemap_size
	 *
	 * @var int
	 */
	var $size_limit;

	/**
	 * The path to prepend to the filename
	 *
	 * @var string
	 */
	var $fspath;

	/**
	 * The path to append to the domain name
	 *
	 * @var string
	 */
	var $path;

	/**
	 * Whether or not to use compression
	 *
	 * @var bool
	 */
	var $compress;

	/**
	 * The number of entries to save in each sitemap file
	 *
	 * @var array
	 */
	var $limit = array();

	/**
	 * Key => value entries of namespaces and their priorities
	 *
	 * @var array
	 */
	var $priorities = array(
		// Custom main namespaces
		GS_MAIN			=> '0.5',
		// Custom talk namesspaces
		GS_TALK			=> '0.1',
		// MediaWiki standard namespaces
		NS_MAIN			=> '1.0',
		NS_TALK			=> '0.1',
		NS_USER			=> '0.5',
		NS_USER_TALK		=> '0.1',
		NS_PROJECT		=> '0.5',
		NS_PROJECT_TALK		=> '0.1',
		NS_IMAGE		=> '0.5',
		NS_IMAGE_TALK		=> '0.1',
		NS_MEDIAWIKI		=> '0.0',
		NS_MEDIAWIKI_TALK	=> '0.1',
		NS_TEMPLATE		=> '0.0',
		NS_TEMPLATE_TALK	=> '0.1',
		NS_HELP			=> '0.5',
		NS_HELP_TALK		=> '0.1',
		NS_CATEGORY		=> '0.5',
		NS_CATEGORY_TALK	=> '0.1',
	);

	/**
	 * A one-dimensional array of namespaces in the wiki
	 *
	 * @var array
	 */
	var $namespaces = array();

	/**
	 * When this sitemap batch was generated
	 *
	 * @var string
	 */
	var $timestamp;

	/**
	 * A database slave object
	 *
	 * @var object
	 */
	var $dbr;

	/**
	 * A resource pointing to the sitemap index file
	 *
	 * @var resource
	 */
	var $findex;


	/**
	 * A resource pointing to a sitemap file
	 *
	 * @var resource
	 */
	var $file;

	/**
	 * A resource pointing to php://stderr
	 *
	 * @var resource
	 */
	var $stderr;

	/**
	 * Constructor
	 *
	 * @param string $fspath The path to prepend to the filenames, used to
	 *                     save them somewhere else than in the root directory
	 * @param string $path The path to append to the domain name
	 * @param bool $compress Whether to compress the sitemap files
	 */
	function GenerateSitemap( $fspath, $compress ) {
		global $wgScriptPath;

		$this->url_limit = 50000;
		$this->size_limit = pow( 2, 20 ) * 10;
		$this->fspath = self::init_path( $fspath );

		$this->compress = $compress;

		$this->stderr = fopen( 'php://stderr', 'wt' );
		$this->dbr = wfGetDB( DB_SLAVE );
		$this->generateNamespaces();
		$this->timestamp = wfTimestamp( TS_ISO_8601, wfTimestampNow() );


		$this->findex = fopen( "{$this->fspath}sitemap-index-" . wfWikiID() . ".xml", 'wb' );
	}

	/**
	 * Create directory if it does not exist and return pathname with a trailing slash
	 */
	private static function init_path( $fspath ) {
		if( !isset( $fspath ) ) {
			return null;
		}
		# Create directory if needed
		if( $fspath && !is_dir( $fspath ) ) {
			mkdir( $fspath, 0755 ) or die("Can not create directory $fspath.\n");
		}

		return realpath( $fspath ). DIRECTORY_SEPARATOR ;
	}

	/**
	 * Generate a one-dimensional array of existing namespaces
	 */
	function generateNamespaces() {
		$fname = 'GenerateSitemap::generateNamespaces';

		// Only generate for specific namespaces if $wgSitemapNamespaces is an array.
		global $wgSitemapNamespaces;
		if( is_array( $wgSitemapNamespaces ) ) {
			$this->namespaces = $wgSitemapNamespaces;
			return;
		}

		$res = $this->dbr->select( 'page',
			array( 'page_namespace' ),
			array(),
			$fname,
			array(
				'GROUP BY' => 'page_namespace',
				'ORDER BY' => 'page_namespace',
			)
		);

		while ( $row = $this->dbr->fetchObject( $res ) )
			$this->namespaces[] = $row->page_namespace;
	}

	/**
	 * Get the priority of a given namespace
	 *
	 * @param int $namespace The namespace to get the priority for
	 +
	 * @return string
	 */

	function priority( $namespace ) {
		return isset( $this->priorities[$namespace] ) ? $this->priorities[$namespace] : $this->guessPriority( $namespace );
	}

	/**
	 * If the namespace isn't listed on the priority list return the
	 * default priority for the namespace, varies depending on whether it's
	 * a talkpage or not.
	 *
	 * @param int $namespace The namespace to get the priority for
	 *
	 * @return string
	 */
	function guessPriority( $namespace ) {
		return MWNamespace::isMain( $namespace ) ? $this->priorities[GS_MAIN] : $this->priorities[GS_TALK];
	}

	/**
	 * Return a database resolution of all the pages in a given namespace
	 *
	 * @param int $namespace Limit the query to this namespace
	 *
	 * @return resource
	 */
	function getPageRes( $namespace ) {
		$fname = 'GenerateSitemap::getPageRes';

		return $this->dbr->select( 'page',
			array(
				'page_namespace',
				'page_title',
				'page_touched',
			),
			array( 'page_namespace' => $namespace ),
			$fname
		);
	}

	/**
	 * Main loop
	 *
	 * @access public
	 */
	function main() {
		global $wgContLang;

		fwrite( $this->findex, $this->openIndex() );

		foreach ( $this->namespaces as $namespace ) {
			$res = $this->getPageRes( $namespace );
			$this->file = false;
			$this->generateLimit( $namespace );
			$length = $this->limit[0];
			$i = $smcount = 0;

			$fns = $wgContLang->getFormattedNsText( $namespace );
			$this->debug( "$namespace ($fns)" );
			while ( $row = $this->dbr->fetchObject( $res ) ) {
				if ( $i++ === 0 || $i === $this->url_limit + 1 || $length + $this->limit[1] + $this->limit[2] > $this->size_limit ) {
					if ( $this->file !== false ) {
						$this->write( $this->file, $this->closeFile() );
						$this->close( $this->file );
					}
					$filename = $this->sitemapFilename( $namespace, $smcount++ );
					$this->file = $this->open( $this->fspath . $filename, 'wb' );
					$this->write( $this->file, $this->openFile() );
					fwrite( $this->findex, $this->indexEntry( $filename ) );
					$this->debug( "\t$this->fspath$filename" );
					$length = $this->limit[0];
					$i = 1;
				}
				$title = Title::makeTitle( $row->page_namespace, $row->page_title );
				$date = wfTimestamp( TS_ISO_8601, $row->page_touched );
				$entry = $this->fileEntry( $title->getFullURL(), $date, $this->priority( $namespace ) );
				$length += strlen( $entry );
				$this->write( $this->file, $entry );
				// generate pages for language variants
				if($wgContLang->hasVariants()){
					$variants = $wgContLang->getVariants();
					foreach($variants as $vCode){
						if($vCode==$wgContLang->getCode()) continue; // we don't want default variant
						$entry = $this->fileEntry( $title->getFullURL('',$vCode), $date, $this->priority( $namespace ) );
						$length += strlen( $entry );
						$this->write( $this->file, $entry );
					}
				}
			}
			if ( $this->file ) {
				$this->write( $this->file, $this->closeFile() );
				$this->close( $this->file );
			}
		}
		fwrite( $this->findex, $this->closeIndex() );
		fclose( $this->findex );
	}

	/**
	 * gzopen() / fopen() wrapper
	 *
	 * @return resource
	 */
	function open( $file, $flags ) {
		return $this->compress ? gzopen( $file, $flags ) : fopen( $file, $flags );
	}

	/**
	 * gzwrite() / fwrite() wrapper
	 */
	function write( &$handle, $str ) {
		if ( $this->compress )
			gzwrite( $handle, $str );
		else
			fwrite( $handle, $str );
	}

	/**
	 * gzclose() / fclose() wrapper
	 */
	function close( &$handle ) {
		if ( $this->compress )
			gzclose( $handle );
		else
			fclose( $handle );
	}

	/**
	 * Get a sitemap filename
	 *
	 * @static
	 *
	 * @param int $namespace The namespace
	 * @param int $count The count
	 *
	 * @return string
	 */
	function sitemapFilename( $namespace, $count ) {
		$ext = $this->compress ? '.gz' : '';
		return "sitemap-".wfWikiID()."-NS_$namespace-$count.xml$ext";
	}

	/**
	 * Return the XML required to open an XML file
	 *
	 * @static
	 *
	 * @return string
	 */
	function xmlHead() {
		return '<?xml version="1.0" encoding="UTF-8"?>' . "\n";
	}

	/**
	 * Return the XML schema being used
	 *
	 * @static
	 *
	 * @returns string
	 */
	function xmlSchema() {
		return 'http://www.sitemaps.org/schemas/sitemap/0.9';
	}

	/**
	 * Return the XML required to open a sitemap index file
	 *
	 * @return string
	 */
	function openIndex() {
		return $this->xmlHead() . '<sitemapindex xmlns="' . $this->xmlSchema() . '">' . "\n";
	}

	/**
	 * Return the XML for a single sitemap indexfile entry
	 *
	 * @static
	 *
	 * @param string $filename The filename of the sitemap file
	 *
	 * @return string
	 */
	function indexEntry( $filename ) {
		return
			"\t<sitemap>\n" .
			"\t\t<loc>$filename</loc>\n" .
			"\t\t<lastmod>{$this->timestamp}</lastmod>\n" .
			"\t</sitemap>\n";
	}

	/**
	 * Return the XML required to close a sitemap index file
	 *
	 * @static
	 *
	 * @return string
	 */
	function closeIndex() {
		return "</sitemapindex>\n";
	}

	/**
	 * Return the XML required to open a sitemap file
	 *
	 * @return string
	 */
	function openFile() {
		return $this->xmlHead() . '<urlset xmlns="' . $this->xmlSchema() . '">' . "\n";
	}

	/**
	 * Return the XML for a single sitemap entry
	 *
	 * @static
	 *
	 * @param string $url An RFC 2396 compilant URL
	 * @param string $date A ISO 8601 date
	 * @param string $priority A priority indicator, 0.0 - 1.0 inclusive with a 0.1 stepsize
	 *
	 * @return string
	 */
	function fileEntry( $url, $date, $priority ) {
		return
			"\t<url>\n" .
			"\t\t<loc>$url</loc>\n" .
			"\t\t<lastmod>$date</lastmod>\n" .
			"\t\t<priority>$priority</priority>\n" .
			"\t</url>\n";
	}

	/**
	 * Return the XML required to close sitemap file
	 *
	 * @static
	 * @return string
	 */
	function closeFile() {
		return "</urlset>\n";
	}

	/**
	 * Write a string to stderr followed by a UNIX newline
	 */
	function debug( $str ) {
		fwrite( $this->stderr, "$str\n" );
	}

	/**
	 * Populate $this->limit
	 */
	function generateLimit( $namespace ) {
		$title = Title::makeTitle( $namespace, str_repeat( "\xf0\xa8\xae\x81", 63 ) . "\xe5\x96\x83" );

		$this->limit = array(
			strlen( $this->openFile() ),
			strlen( $this->fileEntry( $title->getFullUrl(), wfTimestamp( TS_ISO_8601, wfTimestamp() ), $this->priority( $namespace ) ) ),
			strlen( $this->closeFile() )
		);
	}
}

if ( in_array( '--help', $argv ) ) {
	echo <<<EOT
Usage: php generateSitemap.php [options]
	--help			show this message

	--fspath=<path>		The file system path to save to, e.g /tmp/sitemap

	--server=<server>	The protocol and server name to use in URLs, e.g.
		http://en.wikipedia.org. This is sometimes necessary because
		server name detection may fail in command line scripts.

	--compress=[yes|no]	compress the sitemap files, default yes

EOT;
	die( -1 );
}

$optionsWithArgs = array( 'fspath', 'server', 'compress' );
require_once( dirname( __FILE__ ) . '/commandLine.inc' );

if ( isset( $options['server'] ) ) {
	$wgServer = $options['server'];
}

$gs = new GenerateSitemap( @$options['fspath'], @$options['compress'] !== 'no' );
$gs->main();

