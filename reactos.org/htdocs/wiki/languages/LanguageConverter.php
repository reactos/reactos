<?php

/**
 * Contains the LanguageConverter class and ConverterRule class
 * @ingroup Language
 *
 * @license http://www.gnu.org/copyleft/gpl.html GNU General Public License
 * @file
 */

/**
 * base class for language convert
 * @ingroup Language
 *
 * @author Zhengzhu Feng <zhengzhu@gmail.com>
 * @maintainers fdcn <fdcn64@gmail.com>, shinjiman <shinjiman@gmail.com>
 */
class LanguageConverter {
	var $mPreferredVariant='';
	var $mMainLanguageCode;
	var $mVariants, $mVariantFallbacks, $mVariantNames;
	var $mTablesLoaded = false;
	var $mTables;
	var $mTitleDisplay='';
	var $mDoTitleConvert=true, $mDoContentConvert=true;
	var $mManualLevel; // 'bidirectional' 'unidirectional' 'disable' for each variants
	var $mTitleFromFlag = false;
	var $mCacheKey;
	var $mLangObj;
	var $mMarkup;
	var $mFlags;
	var $mDescCodeSep = ':',$mDescVarSep = ';';
	var $mUcfirst = false;

	const CACHE_VERSION_KEY = 'VERSION 6';

	/**
	 * Constructor
	 *
	 * @param string $maincode the main language code of this language
	 * @param array $variants the supported variants of this language
	 * @param array $variantfallback the fallback language of each variant
	 * @param array $markup array defining the markup used for manual conversion
	 * @param array $flags array defining the custom strings that maps to the flags
	 * @param array $manualLevel limit for supported variants
	 * @public
	 */
	function __construct($langobj, $maincode,
								$variants=array(),
								$variantfallbacks=array(),
								$markup=array(),
								$flags = array(),
								$manualLevel = array() ) {
		$this->mLangObj = $langobj;
		$this->mMainLanguageCode = $maincode;
		$this->mVariants = $variants;
		$this->mVariantFallbacks = $variantfallbacks;
		global $wgLanguageNames;
		$this->mVariantNames = $wgLanguageNames;
		$this->mCacheKey = wfMemcKey( 'conversiontables', $maincode );
		$m = array(
			'begin'=>'-{', 
			'flagsep'=>'|',
			'unidsep'=>'=>', //for unidirectional conversion
			'codesep'=>':',
			'varsep'=>';',
			'end'=>'}-'
		);
		$this->mMarkup = array_merge($m, $markup);
		$f = array( 
			// 'S' show converted text
			// '+' add rules for alltext
			// 'E' the gave flags is error
			// these flags above are reserved for program
			'A'=>'A',       // add rule for convert code (all text convert)
			'T'=>'T',       // title convert
			'R'=>'R',       // raw content
			'D'=>'D',       // convert description (subclass implement)
			'-'=>'-',       // remove convert (not implement)
			'H'=>'H',       // add rule for convert code (but no display in placed code )
			'N'=>'N'        // current variant name
		);
		$this->mFlags = array_merge($f, $flags);
		foreach( $this->mVariants as $v)
			$this->mManualLevel[$v]=array_key_exists($v,$manualLevel)
								?$manualLevel[$v]
								:'bidirectional';
	}

	/**
	 * @public
	 */
	function getVariants() {
		return $this->mVariants;
	}

	/**
	 * in case some variant is not defined in the markup, we need
	 * to have some fallback. for example, in zh, normally people
	 * will define zh-hans and zh-hant, but less so for zh-sg or zh-hk.
	 * when zh-sg is preferred but not defined, we will pick zh-hans
	 * in this case. right now this is only used by zh.
	 *
	 * @param string $v the language code of the variant
	 * @return string array the code of the fallback language or false if there is no fallback
	 * @public
	 */
	function getVariantFallbacks($v) {
		if( isset( $this->mVariantFallbacks[$v] ) ) {
			return $this->mVariantFallbacks[$v];
		}
		return $this->mMainLanguageCode;
	}

	/**
	 * get preferred language variants.
	 * @param boolean $fromUser Get it from $wgUser's preferences
	 * @return string the preferred language code
	 * @public
	 */
	function getPreferredVariant( $fromUser = true ) {
		global $wgUser, $wgRequest, $wgVariantArticlePath, $wgDefaultLanguageVariant;

		if($this->mPreferredVariant)
			return $this->mPreferredVariant;

		// figure out user lang without constructing wgLang to avoid infinite recursion
		if( $fromUser )
			$defaultUserLang = $wgUser->getOption( 'language' );
		else
			$defaultUserLang = $this->mMainLanguageCode;
		$userLang = $wgRequest->getVal( 'uselang', $defaultUserLang );
		// see if interface language is same as content, if not, prevent conversion
		if( ! in_array( $userLang, $this->mVariants ) ){ 
			$this->mPreferredVariant = $this->mMainLanguageCode; // no conversion
			return $this->mPreferredVariant;
		}

		// see if the preference is set in the request
		$req = $wgRequest->getText( 'variant' );
		if( in_array( $req, $this->mVariants ) ) {
			$this->mPreferredVariant = $req;
			return $req;
		}

		// check the syntax /code/ArticleTitle
		if($wgVariantArticlePath!=false && isset($_SERVER['SCRIPT_NAME'])){
			// Note: SCRIPT_NAME probably won't hold the correct value if PHP is run as CGI
			// (it will hold path to php.cgi binary), and might not exist on some very old PHP installations
			$scriptBase = basename( $_SERVER['SCRIPT_NAME'] );
			if(in_array($scriptBase,$this->mVariants)){
				$this->mPreferredVariant = $scriptBase;
				return $this->mPreferredVariant;
			}
		}

		// get language variant preference from logged in users
		// Don't call this on stub objects because that causes infinite 
		// recursion during initialisation
		if( $fromUser && $wgUser->isLoggedIn() )  {
			$this->mPreferredVariant = $wgUser->getOption('variant');
			return $this->mPreferredVariant;
		}

		// see if default variant is globaly set
		if($wgDefaultLanguageVariant != false  &&  in_array( $wgDefaultLanguageVariant, $this->mVariants )){
			$this->mPreferredVariant = $wgDefaultLanguageVariant;
			return $this->mPreferredVariant;
		}

		# FIXME rewrite code for parsing http header. The current code
		# is written specific for detecting zh- variants
		if( !$this->mPreferredVariant ) {
			// see if some supported language variant is set in the
			// http header, but we don't set the mPreferredVariant
			// variable in case this is called before the user's
			// preference is loaded
			$pv=$this->mMainLanguageCode;
			if(array_key_exists('HTTP_ACCEPT_LANGUAGE', $_SERVER)) {
				$header = str_replace( '_', '-', strtolower($_SERVER["HTTP_ACCEPT_LANGUAGE"]));
				$zh = strstr($header, $pv.'-');
				if($zh) {
					$pv = substr($zh,0,5);
				}
			}
			// don't try to return bad variant
			if(in_array( $pv, $this->mVariants ))
				return $pv;
		}

		return $this->mMainLanguageCode;

	}
	
	/**
	 * dictionary-based conversion
	 *
	 * @param string $text the text to be converted
	 * @param string $toVariant the target language code
	 * @return string the converted text
	 * @private
	 */
	function autoConvert($text, $toVariant=false) {
		$fname="LanguageConverter::autoConvert";

		wfProfileIn( $fname );

		if(!$this->mTablesLoaded)
			$this->loadTables();

		if(!$toVariant)
			$toVariant = $this->getPreferredVariant();
		if(!in_array($toVariant, $this->mVariants))
			return $text;

		/* we convert everything except:
		   1. html markups (anything between < and >)
		   2. html entities
		   3. place holders created by the parser
		*/
		global $wgParser;
		if (isset($wgParser) && $wgParser->UniqPrefix()!=''){
			$marker = '|' . $wgParser->UniqPrefix() . '[\-a-zA-Z0-9]+';
		} else
			$marker = "";

		// this one is needed when the text is inside an html markup
		$htmlfix = '|<[^>]+$|^[^<>]*>';

		// disable convert to variants between <code></code> tags
		$codefix = '<code>.+?<\/code>|';
		// disable convertsion of <script type="text/javascript"> ... </script>
		$scriptfix = '<script.*?>.*?<\/script>|';
		// disable conversion of <pre xxxx> ... </pre>
		$prefix = '<pre.*?>.*?<\/pre>|';

		$reg = '/'.$codefix . $scriptfix . $prefix . '<[^>]+>|&[a-zA-Z#][a-z0-9]+;' . $marker . $htmlfix . '/s';

		$matches = preg_split($reg, $text, -1, PREG_SPLIT_OFFSET_CAPTURE);

		$m = array_shift($matches);

		$ret = $this->translate($m[0], $toVariant);
		$mstart = $m[1]+strlen($m[0]);
		foreach($matches as $m) {
			$ret .= substr($text, $mstart, $m[1]-$mstart);
			$ret .= $this->translate($m[0], $toVariant);
			$mstart = $m[1] + strlen($m[0]);
		}
		wfProfileOut( $fname );
		return $ret;
	}

	/**
	 * Translate a string to a variant
	 * Doesn't process markup or do any of that other stuff, for that use convert()
	 *
	 * @param string $text Text to convert
	 * @param string $variant Variant language code
	 * @return string Translated text
	 * @private
	 */
	function translate( $text, $variant ) {
		wfProfileIn( __METHOD__ );
		if( !$this->mTablesLoaded )
			$this->loadTables();
		$text = $this->mTables[$variant]->replace( $text );
		wfProfileOut( __METHOD__ );
		return $text;
	}

	/**
	 * convert text to all supported variants
	 *
	 * @param string $text the text to be converted
	 * @return array of string
	 * @public
	 */
	function autoConvertToAllVariants($text) {
		$fname="LanguageConverter::autoConvertToAllVariants";
		wfProfileIn( $fname );
		if( !$this->mTablesLoaded )
			$this->loadTables();

		$ret = array();
		foreach($this->mVariants as $variant) {
			$ret[$variant] = $this->translate($text, $variant);
		}

		wfProfileOut( $fname );
		return $ret;
	}

	/**
	 * convert link text to all supported variants
	 *
	 * @param string $text the text to be converted
	 * @return array of string
	 * @public
	 */
	function convertLinkToAllVariants($text) {
		if( !$this->mTablesLoaded )
			$this->loadTables();

		$ret = array();
		$tarray = explode($this->mMarkup['begin'], $text);
		$tfirst = array_shift($tarray);

		foreach($this->mVariants as $variant)
			$ret[$variant] = $this->translate($tfirst,$variant);

		foreach($tarray as $txt) {
			$marked = explode($this->mMarkup['end'], $txt, 2);

			foreach($this->mVariants as $variant){
				$ret[$variant] .= $this->mMarkup['begin'].$marked[0].$this->mMarkup['end'];
				if(array_key_exists(1, $marked))
					$ret[$variant] .= $this->translate($marked[1],$variant);
			}
			
		}

		return $ret;
	}


	/**
	 * apply manual conversion
	 * @private
	 */
	function applyManualConv($convRule){
		// use syntax -{T|zh:TitleZh;zh-tw:TitleTw}- for custom conversion in title
		$title = $convRule->getTitle();
		if($title){
			$this->mTitleFromFlag = true;
			$this->mTitleDisplay =  $title;
		}

		//apply manual conversion table to global table
		$convTable = $convRule->getConvTable();
		$action = $convRule->getRulesAction();
		foreach($convTable as $v=>$t) {
			if( !in_array($v,$this->mVariants) )continue;
			if( $action=="add" )
				$this->mTables[$v]->mergeArray($t);
			elseif ( $action=="remove" )
				$this->mTables[$v]->removeArray($t);
		}
	}

	/**
	 * Convert text using a parser object for context
	 * @public
	 */
	function parserConvert( $text, &$parser ) {
		global $wgDisableLangConversion;
		/* don't do anything if this is the conversion table */
		if ( $parser->getTitle()->getNamespace() == NS_MEDIAWIKI &&
				 strpos($parser->mTitle->getText(), "Conversiontable") !== false ) 
		{
			return $text;
		}

		if($wgDisableLangConversion)
			return $text;

		$text = $this->convert( $text );
		$parser->mOutput->setTitleText( $this->mTitleDisplay );
		return $text;
	}

	/**
	 *  convert title
	 * @private
	 */
	function convertTitle($text){
		// check for __NOTC__ tag
		if( !$this->mDoTitleConvert ) {
			$this->mTitleDisplay = $text;
			return $text;
		}

		// use the title from the T flag if any
		if($this->mTitleFromFlag){
			$this->mTitleFromFlag = false;
			return $this->mTitleDisplay;
		}

		global $wgRequest;
		$isredir = $wgRequest->getText( 'redirect', 'yes' );
		$action = $wgRequest->getText( 'action' );
		if ( $isredir == 'no' || $action == 'edit' ) {
			return $text;
		} else {
			$this->mTitleDisplay = $this->convert($text);
			return $this->mTitleDisplay;
		}
	}

	/**
	 * convert text to different variants of a language. the automatic
	 * conversion is done in autoConvert(). here we parse the text
	 * marked with -{}-, which specifies special conversions of the
	 * text that can not be accomplished in autoConvert()
	 *
	 * syntax of the markup:
	 * -{code1:text1;code2:text2;...}-  or
	 * -{flags|code1:text1;code2:text2;...}-  or
	 * -{text}- in which case no conversion should take place for text
	 *
	 * @param string $text text to be converted
	 * @param bool $isTitle whether this conversion is for the article title
	 * @return string converted text
	 * @public
	 */
	function convert( $text , $isTitle=false) {

		$mw =& MagicWord::get( 'notitleconvert' );
		if( $mw->matchAndRemove( $text ) )
			$this->mDoTitleConvert = false;
		$mw =& MagicWord::get( 'nocontentconvert' );
		if( $mw->matchAndRemove( $text ) ) {
			$this->mDoContentConvert = false;
		}

		// no conversion if redirecting
		$mw =& MagicWord::get( 'redirect' );
		if( $mw->matchStart( $text ))
			return $text;

		// for title convertion
		if ($isTitle) return $this->convertTitle($text);

		$plang = $this->getPreferredVariant();
		$tarray = explode($this->mMarkup['end'], $text);
		$text = '';
		foreach($tarray as $txt) {
			$marked = explode($this->mMarkup['begin'], $txt, 2);

			if( $this->mDoContentConvert )
				$text .= $this->autoConvert($marked[0],$plang);
			else
				$text .= $marked[0];

			if(array_key_exists(1, $marked)){
				// strip the flags from syntax like -{T| ... }-
				$crule = new ConverterRule($marked[1], $this);
				$crule->parse($plang);

				$text .= $crule->getDisplay();
				$this->applyManualConv($crule);
			}
		}

		return $text;
	}

	/**
	 * if a language supports multiple variants, it is
	 * possible that non-existing link in one variant
	 * actually exists in another variant. this function
	 * tries to find it. See e.g. LanguageZh.php
	 *
	 * @param string $link the name of the link
	 * @param mixed $nt the title object of the link
	 * @return null the input parameters may be modified upon return
	 * @public
	 */
	function findVariantLink( &$link, &$nt ) {
		global $wgDisableLangConversion;
		$linkBatch = new LinkBatch();

		$ns=NS_MAIN;

		if(is_object($nt))
			$ns = $nt->getNamespace();

		$variants = $this->autoConvertToAllVariants($link);
		if($variants == false) //give up
			return;

		$titles = array();

		foreach( $variants as $v ) {
			if($v != $link){
				$varnt = Title::newFromText( $v, $ns );
				if(!is_null($varnt)){
					$linkBatch->addObj($varnt);
					$titles[]=$varnt;
				}
			}
		}

		// fetch all variants in single query
		$linkBatch->execute();

		foreach( $titles as $varnt ) {
			if( $varnt->getArticleID() > 0 ) {
				$nt = $varnt;
				if( !$wgDisableLangConversion )
					$link = $v;
				break;
			}
		}
	}

    /**
	 * returns language specific hash options
	 *
	 * @public
	 */
	function getExtraHashOptions() {
		$variant = $this->getPreferredVariant();
		return '!' . $variant ;
	}

    /**
	 * get title text as defined in the body of the article text
	 *
	 * @public
	 */
	function getParsedTitle() {
		return $this->mTitleDisplay;
	}

	/**
	 * a write lock to the cache
	 *
	 * @private
	 */
	function lockCache() {
		global $wgMemc;
		$success = false;
		for($i=0; $i<30; $i++) {
			if($success = $wgMemc->add($this->mCacheKey . "lock", 1, 10))
				break;
			sleep(1);
		}
		return $success;
	}

	/**
	 * unlock cache
	 *
	 * @private
	 */
	function unlockCache() {
		global $wgMemc;
		$wgMemc->delete($this->mCacheKey . "lock");
	}


	/**
	 * Load default conversion tables
	 * This method must be implemented in derived class
	 *
	 * @private
	 */
	function loadDefaultTables() {
		$name = get_class($this);
		wfDie("Must implement loadDefaultTables() method in class $name");
	}

	/**
	 * load conversion tables either from the cache or the disk
	 * @private
	 */
	function loadTables($fromcache=true) {
		global $wgMemc;
		if( $this->mTablesLoaded )
			return;
		wfProfileIn( __METHOD__ );
		$this->mTablesLoaded = true;
		$this->mTables = false;
		if($fromcache) {
			wfProfileIn( __METHOD__.'-cache' );
			$this->mTables = $wgMemc->get( $this->mCacheKey );
			wfProfileOut( __METHOD__.'-cache' );
		}
		if ( !$this->mTables || !isset( $this->mTables[self::CACHE_VERSION_KEY] ) ) {
			wfProfileIn( __METHOD__.'-recache' );
			// not in cache, or we need a fresh reload.
			// we will first load the default tables
			// then update them using things in MediaWiki:Zhconversiontable/*
			$this->loadDefaultTables();
			foreach($this->mVariants as $var) {
				$cached = $this->parseCachedTable($var);
				$this->mTables[$var]->mergeArray($cached);
			}

			$this->postLoadTables();
			$this->mTables[self::CACHE_VERSION_KEY] = true;

			if($this->lockCache()) {
				$wgMemc->set($this->mCacheKey, $this->mTables, 43200);
				$this->unlockCache();
			}
			wfProfileOut( __METHOD__.'-recache' );
		}
		wfProfileOut( __METHOD__ );
	}

    /**
	 * Hook for post processig after conversion tables are loaded
	 *
	 */
	function postLoadTables() {}

    /**
	 * Reload the conversion tables
	 *
	 * @private
	 */
	function reloadTables() {
		if($this->mTables)
			unset($this->mTables);
		$this->mTablesLoaded = false;
		$this->loadTables(false);
	}


	/**
	 * parse the conversion table stored in the cache
	 *
	 * the tables should be in blocks of the following form:
	 *		-{
	 *			word => word ;
	 *			word => word ;
	 *			...
	 *		}-
	 *
	 *	to make the tables more manageable, subpages are allowed
	 *	and will be parsed recursively if $recursive=true
	 *
	 */
	function parseCachedTable($code, $subpage='', $recursive=true) {
		global $wgMessageCache;
		static $parsed = array();

		if(!is_object($wgMessageCache))
			return array();

		$key = 'Conversiontable/'.$code;
		if($subpage)
			$key .= '/' . $subpage;

		if(array_key_exists($key, $parsed))
			return array();

		if ( strpos( $code, '/' ) === false ) {
			$txt = $wgMessageCache->get( 'Conversiontable', true, $code );
		} else {
			$title = Title::makeTitleSafe( NS_MEDIAWIKI, "Conversiontable/$code" );
			if ( $title && $title->exists() ) {
				$article = new Article( $title );
				$txt = $article->getContents();
			} else {
				$txt = '';
			}
		}

		// get all subpage links of the form
		// [[MediaWiki:conversiontable/zh-xx/...|...]]
		$linkhead = $this->mLangObj->getNsText(NS_MEDIAWIKI) . ':Conversiontable';
		$subs = explode('[[', $txt);
		$sublinks = array();
		foreach( $subs as $sub ) {
			$link = explode(']]', $sub, 2);
			if(count($link) != 2)
				continue;
			$b = explode('|', $link[0]);
			$b = explode('/', trim($b[0]), 3);
			if(count($b)==3)
				$sublink = $b[2];
			else
				$sublink = '';

			if($b[0] == $linkhead && $b[1] == $code) {
				$sublinks[] = $sublink;
			}
		}


		// parse the mappings in this page
		$blocks = explode($this->mMarkup['begin'], $txt);
		array_shift($blocks);
		$ret = array();
		foreach($blocks as $block) {
			$mappings = explode($this->mMarkup['end'], $block, 2);
			$stripped = str_replace(array("'", '"', '*','#'), '', $mappings[0]);
			$table = explode( ';', $stripped );
			foreach( $table as $t ) {
				$m = explode( '=>', $t );
				if( count( $m ) != 2)
					continue;
				// trim any trailling comments starting with '//'
				$tt = explode('//', $m[1], 2);
				$ret[trim($m[0])] = trim($tt[0]);
			}
		}
		$parsed[$key] = true;


		// recursively parse the subpages
		if($recursive) {
			foreach($sublinks as $link) {
				$s = $this->parseCachedTable($code, $link, $recursive);
				$ret = array_merge($ret, $s);
			}
		}

		if ($this->mUcfirst) {
			foreach ($ret as $k => $v) {
				$ret[Language::ucfirst($k)] = Language::ucfirst($v);
			}
		}
		return $ret;
	}

	/**
	 * Enclose a string with the "no conversion" tag. This is used by
	 * various functions in the Parser
	 *
	 * @param string $text text to be tagged for no conversion
	 * @return string the tagged text
	 * @public
	 */
	function markNoConversion($text, $noParse=false) {
		# don't mark if already marked
		if(strpos($text, $this->mMarkup['begin']) ||
 		   strpos($text, $this->mMarkup['end']))
			return $text;

		$ret = $this->mMarkup['begin'] .'R|'. $text . $this->mMarkup['end'];
		return $ret;
	}

	/**
	 * convert the sorting key for category links. this should make different
	 * keys that are variants of each other map to the same key
	 */
	function convertCategoryKey( $key ) {
		return $key;
	}
	/**
	 * hook to refresh the cache of conversion tables when
	 * MediaWiki:conversiontable* is updated
	 * @private
	 */
	function OnArticleSaveComplete($article, $user, $text, $summary, $isminor, $iswatch, $section, $flags, $revision) {
		$titleobj = $article->getTitle();
		if($titleobj->getNamespace() == NS_MEDIAWIKI) {
			$title = $titleobj->getDBkey();
			$t = explode('/', $title, 3);
			$c = count($t);
			if( $c > 1 && $t[0] == 'Conversiontable' ) {
				if(in_array($t[1], $this->mVariants)) {
					$this->reloadTables();
				}
			}
		}
		return true;
	}

	/** 
	 * Armour rendered math against conversion
	 * Wrap math into rawoutput -{R| math }- syntax
	 * @public
	 */
 	function armourMath($text){ 
		$ret = $this->mMarkup['begin'] . 'R|' . $text . $this->mMarkup['end'];
		return $ret;
	}
}

/**
 * parser for rules of language conversion , parse rules in -{ }- tag
 * @ingroup Language
 * @author  fdcn <fdcn64@gmail.com>
 */
class ConverterRule {
	var $mText; // original text in -{text}-
	var $mConverter; // LanguageConverter object 
	var $mManualCodeError='<strong class="error">code error!</strong>';
	var $mRuleDisplay = '',$mRuleTitle=false;
	var $mRules = '';// string : the text of the rules
	var $mRulesAction = 'none';
	var $mFlags = array();
	var $mConvTable = array();
	var $mBidtable = array();// array of the translation in each variant
	var $mUnidtable = array();// array of the translation in each variant

	/**
	 * Constructor
	 *
	 * @param string $text the text between -{ and }-
	 * @param object $converter a  LanguageConverter object 
	 * @access public
	 */
	function __construct($text,$converter){
		$this->mText = $text;
		$this->mConverter=$converter;
		foreach($converter->mVariants as $v){
			$this->mConvTable[$v]=array();
		}
	}

	/**
	 * check if variants array in convert array
	 *
	 * @param string $variant Variant language code
	 * @return string Translated text
	 * @public
	 */
	function getTextInBidtable($variants){
		if(is_string($variants)){ $variants=array($variants); }
		if(!is_array($variants)) return false;
		foreach ($variants as $variant){
			if(array_key_exists($variant, $this->mBidtable)){
				return $this->mBidtable[$variant];
			}
		}
		return false;
	}
	
	/**
	 * Parse flags with syntax -{FLAG| ... }-
	 * @private
	 */
	function parseFlags(){
		$text = $this->mText;
		if(strlen($text) < 2 ) {
			$this->mFlags = array( 'R' );
			$this->mRules = $text;
			return;
		}

		$flags = array();
		$markup = $this->mConverter->mMarkup;
		$validFlags = $this->mConverter->mFlags;

		$tt = explode($markup['flagsep'], $text, 2);
		if(count($tt) == 2) {
			$f = explode($markup['varsep'], $tt[0]);
			foreach($f as $ff) {
				$ff = trim($ff);
				if(array_key_exists($ff, $validFlags) &&
							!in_array($validFlags[$ff], $flags))
					$flags[] = $validFlags[$ff];
			}
			$rules = $tt[1];
		} else {
			$rules = $text;
		}

		//check flags
		if( in_array('R',$flags) ){
			$flags = array('R');// remove other flags
		} elseif ( in_array('N',$flags) ){
			$flags = array('N');// remove other flags
		} elseif ( in_array('-',$flags) ){
			$flags = array('-');// remove other flags
		} elseif (count($flags)==1 && $flags[0]=='T'){
			$flags[]='H'; 
		} elseif ( in_array('H',$flags) ){
			// replace A flag, and remove other flags except T
			$temp=array('+','H');
			if(in_array('T',$flags)) $temp[] = 'T';
			if(in_array('D',$flags)) $temp[] = 'D';
			$flags = $temp;
		} else {
			if ( in_array('A',$flags)) {
				$flags[]='+';
				$flags[]='S';
			}
			if ( in_array('D',$flags) )
				$flags=array_diff($flags,array('S'));
		}
		if ( count($flags)==0 )
			$flags = array('S');
		$this->mRules=$rules;
		$this->mFlags=$flags;
	}
	
	/**
	 * generate conversion table
	 * @private
	 */
	function parseRules() {
		$rules = $this->mRules;
		$flags = $this->mFlags;
		$bidtable = array();
		$unidtable = array();
		$markup = $this->mConverter->mMarkup;

		$choice = explode($markup['varsep'], $rules );
		foreach($choice as $c) {
			$v = explode($markup['codesep'], $c);
			if(count($v) != 2) 
				continue;// syntax error, skip
			$to=trim($v[1]);
			$v=trim($v[0]);
			$u = explode($markup['unidsep'], $v);
			if(count($u) == 1) {
				$bidtable[$v] = $to;
			} else if(count($u) == 2){
				$from=trim($u[0]);$v=trim($u[1]);
				if( array_key_exists($v,$unidtable) && !is_array($unidtable[$v]) )
					$unidtable[$v]=array($from=>$to);
				else
					$unidtable[$v][$from]=$to;
			}
			// syntax error, pass
			if (!array_key_exists($v,$this->mConverter->mVariantNames)){
				$bidtable = array();
				$unidtable = array();
				break;
			}
		}
		$this->mBidtable = $bidtable;
		$this->mUnidtable = $unidtable;
	}

	/**
	 * @private
	 */
	function getRulesDesc(){
		$codesep = $this->mConverter->mDescCodeSep;
		$varsep = $this->mConverter->mDescVarSep;
		$text='';
		foreach($this->mBidtable as $k => $v)
			$text .= $this->mConverter->mVariantNames[$k]."$codesep$v$varsep";
		foreach($this->mUnidtable as $k => $a)
			foreach($a as $from=>$to)
				$text.=$from.'⇒'.$this->mConverter->mVariantNames[$k]."$codesep$to$varsep";
		return $text;
	}

	/**
	 *  Parse rules conversion
	 * @private
	 */
	function getRuleConvertedStr($variant,$doConvert){
		$bidtable = $this->mBidtable;
		$unidtable = $this->mUnidtable;

		if( count($bidtable) + count($unidtable) == 0 ){
			return $this->mRules;
		} elseif ($doConvert){// the text converted 
			// display current variant in bidirectional array
			$disp = $this->getTextInBidtable($variant);
			// or display current variant in fallbacks
			if(!$disp)
				$disp = $this->getTextInBidtable(
						$this->mConverter->getVariantFallbacks($variant));
			// or display current variant in unidirectional array
			if(!$disp && array_key_exists($variant,$unidtable)){
				$disp = array_values($unidtable[$variant]);
				$disp = $disp[0];
			}
			// or display frist text under disable manual convert
			if(!$disp && $this->mConverter->mManualLevel[$variant]=='disable') {
				if(count($bidtable)>0){
					$disp = array_values($bidtable);
					$disp = $disp[0];
				} else {
					$disp = array_values($unidtable);
					$disp = array_values($disp[0]);
					$disp = $disp[0];
				}
			}
			return $disp;
		} else {// no convert
			return $this->mRules;
		}
	}

	/**
	 * generate conversion table for all text
	 * @private
	 */
	function generateConvTable(){
		$flags = $this->mFlags;
		$bidtable = $this->mBidtable;
		$unidtable = $this->mUnidtable;
		$manLevel = $this->mConverter->mManualLevel;

		$vmarked=array();
		foreach($this->mConverter->mVariants as $v) {
			/* for bidirectional array
				fill in the missing variants, if any,
				with fallbacks */
			if(!array_key_exists($v, $bidtable)) {
				$variantFallbacks = $this->mConverter->getVariantFallbacks($v);
				$vf = $this->getTextInBidtable($variantFallbacks);
				if($vf) $bidtable[$v] = $vf;
			}

			if(array_key_exists($v,$bidtable)){
				foreach($vmarked as $vo){
					// use syntax: -{A|zh:WordZh;zh-tw:WordTw}- 
					// or -{H|zh:WordZh;zh-tw:WordTw}- or -{-|zh:WordZh;zh-tw:WordTw}-
					// to introduce a custom mapping between
					// words WordZh and WordTw in the whole text 
					if($manLevel[$v]=='bidirectional'){
						$this->mConvTable[$v][$bidtable[$vo]]=$bidtable[$v];
					}
					if($manLevel[$vo]=='bidirectional'){
						$this->mConvTable[$vo][$bidtable[$v]]=$bidtable[$vo];
					}
				}
				$vmarked[]=$v;
			}
			/*for unidirectional array
				fill to convert tables */
			$allow_unid = $manLevel[$v]=='bidirectional' 
					|| $manLevel[$v]=='unidirectional';
			if($allow_unid && array_key_exists($v,$unidtable)){
				$ct=$this->mConvTable[$v];
				$this->mConvTable[$v] = array_merge($ct,$unidtable[$v]);
			}
		}
	}

	/**
	 * Parse rules and flags
	 * @public
	 */
	function parse($variant){
		if(!$variant) $variant = $this->mConverter->getPreferredVariant();

		$this->parseFlags();
		$flags = $this->mFlags;

		if( !in_array('R',$flags) || !in_array('N',$flags) ){
			//FIXME: may cause trouble here...
			//strip &nbsp; since it interferes with the parsing, plus,
			//all spaces should be stripped in this tag anyway.
			$this->mRules = str_replace('&nbsp;', '', $this->mRules);
			// decode => HTML entities modified by Sanitizer::removeHTMLtags 
			$this->mRules = str_replace('=&gt;','=>',$this->mRules);

			$this->parseRules();
		}
		$rules = $this->mRules;

		if(count($this->mBidtable)==0 && count($this->mUnidtable)==0){
			if(in_array('+',$flags) || in_array('-',$flags))
				// fill all variants if text in -{A/H/-|text} without rules
				foreach($this->mConverter->mVariants as $v)
					$this->mBidtable[$v] = $rules;
			elseif (!in_array('N',$flags) && !in_array('T',$flags) )
				$this->mFlags = $flags = array('R');
		}

		if( in_array('R',$flags) ) {
			// if we don't do content convert, still strip the -{}- tags
			$this->mRuleDisplay = $rules;
		} elseif ( in_array('N',$flags) ){
			// proces N flag: output current variant name
			$this->mRuleDisplay = $this->mConverter->mVariantNames[trim($rules)];
		} elseif ( in_array('D',$flags) ){
			// proces D flag: output rules description
			$this->mRuleDisplay = $this->getRulesDesc();
		} elseif ( in_array('H',$flags) || in_array('-',$flags) ) {
			// proces H,- flag or T only: output nothing
			$this->mRuleDisplay = '';
		} elseif ( in_array('S',$flags) ){
			$this->mRuleDisplay = $this->getRuleConvertedStr($variant,
							$this->mConverter->mDoContentConvert);
		} else {
			$this->mRuleDisplay= $this->mManualCodeError;
		}
		// proces T flag
		if ( in_array('T',$flags) ) {
			$this->mRuleTitle = $this->getRuleConvertedStr($variant,
							$this->mConverter->mDoTitleConvert);
		}

		if (in_array('-', $flags))
			$this->mRulesAction='remove';
		if (in_array('+', $flags))
			$this->mRulesAction='add';
		
		$this->generateConvTable();
	}
	
	/**
	 * @public
	 */
	function hasRules(){
		// TODO:
	}

	/**
	 * get display text on markup -{...}-
	 * @public
	 */
	function getDisplay(){
		return $this->mRuleDisplay;
	}
	/**
	 * get converted title
	 * @public
	 */
	function getTitle(){
		return $this->mRuleTitle;
	}

	/**
	 * return how deal with conversion rules
	 * @public
	 */
	function getRulesAction(){
		return $this->mRulesAction;
	}

	/**
	 * get conversion table ( bidirectional and unidirectional conversion table )
	 * @public
	 */
	function getConvTable(){
		return $this->mConvTable;
	}

	/**
	 * get conversion rules string
	 * @public
	 */
	function getRules(){
		return $this->mRules;
	}

	/**
	 * get conversion flags
	 * @public
	 */
	function getFlags(){
		return $this->mFlags;
	}
}
