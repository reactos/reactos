<?php
/**
 * @ingroup Cache Parser
 * @todo document
 */
class ParserCache {
	/**
	 * Get an instance of this object
	 */
	public static function &singleton() {
		static $instance;
		if ( !isset( $instance ) ) {
			global $parserMemc;
			$instance = new ParserCache( $parserMemc );
		}
		return $instance;
	}

	/**
	 * Setup a cache pathway with a given back-end storage mechanism.
	 * May be a memcached client or a BagOStuff derivative.
	 *
	 * @param object $memCached
	 */
	function __construct( &$memCached ) {
		$this->mMemc =& $memCached;
	}

	function getKey( &$article, &$user ) {
		global $action;
		$hash = $user->getPageRenderingHash();
		if( !$article->mTitle->quickUserCan( 'edit' ) ) {
			// section edit links are suppressed even if the user has them on
			$edit = '!edit=0';
		} else {
			$edit = '';
		}
		$pageid = intval( $article->getID() );
		$renderkey = (int)($action == 'render');
		$key = wfMemcKey( 'pcache', 'idhash', "$pageid-$renderkey!$hash$edit" );
		return $key;
	}

	function getETag( &$article, &$user ) {
		return 'W/"' . $this->getKey($article, $user) . "--" . $article->mTouched. '"';
	}

	function get( &$article, &$user ) {
		global $wgCacheEpoch;
		$fname = 'ParserCache::get';
		wfProfileIn( $fname );

		$key = $this->getKey( $article, $user );

		wfDebug( "Trying parser cache $key\n" );
		$value = $this->mMemc->get( $key );
		if ( is_object( $value ) ) {
			wfDebug( "Found.\n" );
			# Delete if article has changed since the cache was made
			$canCache = $article->checkTouched();
			$cacheTime = $value->getCacheTime();
			$touched = $article->mTouched;
			if ( !$canCache || $value->expired( $touched ) ) {
				if ( !$canCache ) {
					wfIncrStats( "pcache_miss_invalid" );
					wfDebug( "Invalid cached redirect, touched $touched, epoch $wgCacheEpoch, cached $cacheTime\n" );
				} else {
					wfIncrStats( "pcache_miss_expired" );
					wfDebug( "Key expired, touched $touched, epoch $wgCacheEpoch, cached $cacheTime\n" );
				}
				$this->mMemc->delete( $key );
				$value = false;
			} else {
				if ( isset( $value->mTimestamp ) ) {
					$article->mTimestamp = $value->mTimestamp;
				}
				wfIncrStats( "pcache_hit" );
			}
		} else {
			wfDebug( "Parser cache miss.\n" );
			wfIncrStats( "pcache_miss_absent" );
			$value = false;
		}

		wfProfileOut( $fname );
		return $value;
	}

	function save( $parserOutput, &$article, &$user ){
		global $wgParserCacheExpireTime;
		$key = $this->getKey( $article, $user );

		if( $parserOutput->getCacheTime() != -1 ) {

			$now = wfTimestampNow();
			$parserOutput->setCacheTime( $now );

			// Save the timestamp so that we don't have to load the revision row on view
			$parserOutput->mTimestamp = $article->getTimestamp();

			$parserOutput->mText .= "\n<!-- Saved in parser cache with key $key and timestamp $now -->\n";
			wfDebug( "Saved in parser cache with key $key and timestamp $now\n" );

			if( $parserOutput->containsOldMagic() ){
				$expire = 3600; # 1 hour
			} else {
				$expire = $wgParserCacheExpireTime;
			}
			$this->mMemc->set( $key, $parserOutput, $expire );

		} else {
			wfDebug( "Parser output was marked as uncacheable and has not been saved.\n" );
		}
	}

}
