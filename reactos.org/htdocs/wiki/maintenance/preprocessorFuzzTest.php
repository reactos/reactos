<?php
/**
 * @file
 * @ingroup Maintenance
 */

require_once( dirname( __FILE__ ). '/../maintenance/commandLine.inc' );

$wgHooks['BeforeParserFetchTemplateAndtitle'][] = 'PPFuzzTester::templateHook';

class PPFuzzTester {
	var $hairs = array(
		'[[', ']]', '{{', '{{', '}}', '}}', '{{{', '}}}', 
		'<', '>', '<nowiki', '<gallery', '</nowiki>', '</gallery>', '<nOwIkI>', '</NoWiKi>',
		'<!--' , '-->',
		"\n==", "==\n",
		'|', '=', "\n", ' ', "\t", "\x7f",
		'~~', '~~~', '~~~~', 'subst:',
		'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 
		'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',

		// extensions
		//'<ref>', '</ref>', '<references/>',
	);
	var $minLength = 0;
	var $maxLength = 20;
	var $maxTemplates = 5;
	//var $outputTypes = array( 'OT_HTML', 'OT_WIKI', 'OT_PREPROCESS' );
	var $entryPoints = array( 'testSrvus', 'testPst', 'testPreprocess' );
	var $verbose = false;
	static $currentTest = false;

	function execute() {
		if ( !file_exists( 'results' ) ) {
			mkdir( 'results' );
		}
		if ( !is_dir( 'results' ) ) {
			echo "Unable to create 'results' directory\n";
			exit( 1 );
		}
		$overallStart = microtime( true );
		$reportInterval = 1000;
		for ( $i = 1; true; $i++ ) {
			$t = -microtime( true );
			try {
				self::$currentTest = new PPFuzzTest( $this );
				self::$currentTest->execute();
				$passed = 'passed';
			} catch ( MWException $e ) {
				$testReport = self::$currentTest->getReport();
				$exceptionReport = $e->getText();
				$hash = md5( $testReport );
				file_put_contents( "results/ppft-$hash.in", serialize( self::$currentTest ) );
				file_put_contents( "results/ppft-$hash.fail", 
					"Input:\n$testReport\n\nException report:\n$exceptionReport\n" );
				print "Test $hash failed\n";
				$passed = 'failed';
			}
			$t += microtime( true );

			if ( $this->verbose ) {
				printf( "Test $passed in %.3f seconds\n", $t );
				print self::$currentTest->getReport();
			}

			$reportMetric = ( microtime( true ) - $overallStart ) / $i * $reportInterval;
			if ( $reportMetric > 25 ) {
				if ( substr( $reportInterval, 0, 1 ) === '1' ) {
					$reportInterval /= 2;
				} else {
					$reportInterval /= 5;
				}
			} elseif ( $reportMetric < 4 ) {
				if ( substr( $reportInterval, 0, 1 ) === '1' ) {
					$reportInterval *= 5;
				} else {
					$reportInterval *= 2;
				}
			}
			if ( $i % $reportInterval == 0 ) {
				print "$i tests done\n";
				/*
				$testReport = self::$currentTest->getReport();
				$filename = 'results/ppft-' . md5( $testReport ) . '.pass';
				file_put_contents( $filename, "Input:\n$testReport\n" );*/
			}
		}
		wfLogProfilingData();
	}

	function makeInputText( $max = false ) {
		if ( $max === false ) {
			$max = $this->maxLength;
		}
		$length = mt_rand( $this->minLength, $max );
		$s = '';
		for ( $i = 0; $i < $length; $i++ ) {
			$hairIndex = mt_rand( 0, count( $this->hairs ) - 1 );
			$s .= $this->hairs[$hairIndex];
		}
		// Send through the UTF-8 normaliser
		// This resolves a few differences between the old preprocessor and the 
		// XML-based one, which doesn't like illegals and converts line endings.
		// It's done by the MW UI, so it's a reasonably legitimate thing to do.
		$s = UtfNormal::cleanUp( $s );
		return $s;
	}

	function makeTitle() {
		return Title::newFromText( mt_rand( 0, 1000000 ), mt_rand( 0, 10 ) );
	}

	/*
	function pickOutputType() {
		$count = count( $this->outputTypes );
		return $this->outputTypes[ mt_rand( 0, $count - 1 ) ];
	}*/

	function pickEntryPoint() {
		$count = count( $this->entryPoints );
		return $this->entryPoints[ mt_rand( 0, $count - 1 ) ];
	}
}

class PPFuzzTest {
	var $templates, $mainText, $title, $entryPoint, $output;

	function __construct( $tester ) {
		global $wgMaxSigChars;
		$this->parent = $tester;
		$this->mainText = $tester->makeInputText();
		$this->title = $tester->makeTitle();
		//$this->outputType = $tester->pickOutputType();
		$this->entryPoint = $tester->pickEntryPoint();
		$this->nickname = $tester->makeInputText( $wgMaxSigChars + 10);
		$this->fancySig = (bool)mt_rand( 0, 1 );
		$this->templates = array();
	}

	function templateHook( $title ) {
		$titleText = $title->getPrefixedDBkey();
		
		if ( !isset( $this->templates[$titleText] ) ) {
			$finalTitle = $title;
			if ( count( $this->templates ) >= $this->parent->maxTemplates ) {
				// Too many templates
				$text = false;
			} else {
				if ( !mt_rand( 0, 1 ) ) {
					// Redirect
					$finalTitle = $this->parent->makeTitle();
				}
				if ( !mt_rand( 0, 5 ) ) {
					// Doesn't exist
					$text = false;
				} else {
					$text = $this->parent->makeInputText();
				}
			}
			$this->templates[$titleText] = array(
				'text' => $text,
				'finalTitle' => $finalTitle );
		}
		return $this->templates[$titleText];
	}

	function execute() {
		global $wgParser, $wgUser;

		$wgUser = new PPFuzzUser;
		$wgUser->mName = 'Fuzz';
		$wgUser->mFrom = 'name';
		$wgUser->ppfz_test = $this;

		$options = new ParserOptions;
		$options->setTemplateCallback( array( $this, 'templateHook' ) );
		$options->setTimestamp( wfTimestampNow() );
		$this->output = call_user_func( array( $wgParser, $this->entryPoint ), $this->mainText, $this->title->getPrefixedText(), $options );
		return $this->output;
	}

	function getReport() {
		$s = "Title: " . $this->title->getPrefixedDBkey() . "\n" .
//			"Output type: {$this->outputType}\n" . 
			"Entry point: {$this->entryPoint}\n" . 
			"User: " . ( $this->fancySig ? 'fancy' : 'no-fancy' ) . ' ' . var_export( $this->nickname, true ) . "\n" . 
			"Main text: " . var_export( $this->mainText, true ) . "\n";
		foreach ( $this->templates as $titleText => $template ) {
			$finalTitle = $template['finalTitle'];
			if ( $finalTitle != $titleText ) {
				$s .= "[[$titleText]] -> [[$finalTitle]]: " . var_export( $template['text'], true ) . "\n";
			} else {
				$s .= "[[$titleText]]: " . var_export( $template['text'], true ) . "\n";
			}
		}
		$s .= "Output: " . var_export( $this->output, true ) . "\n";
		return $s;
	}
}

class PPFuzzUser extends User {
	var $ppfz_test;

	function load() {
		if ( $this->mDataLoaded ) {
			return;
		}
		$this->mDataLoaded = true;
		$this->loadDefaults( $this->mName );
	}

	function getOption( $option, $defaultOverride = '' ) {
		if ( $option === 'fancysig' ) {
			return $this->ppfz_test->fancySig;
		} elseif ( $option === 'nickname' ) {
			return $this->ppfz_test->nickname;
		} else {
			return parent::getOption( $option, $defaultOverride );
		}
	}
}

ini_set( 'memory_limit', '50M' );
if ( isset( $args[0] ) ) {
	$testText = file_get_contents( $args[0] );
	if ( !$testText ) {
		print "File not found\n";
		exit( 1 );
	}
	$test = unserialize( $testText );
	$result = $test->execute();
	print "Test passed.\n";
} else {
	$tester = new PPFuzzTester;
	$tester->verbose = isset( $options['verbose'] );
	$tester->execute();
}
