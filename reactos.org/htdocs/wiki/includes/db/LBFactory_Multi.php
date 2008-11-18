<?php
/**
 * @file
 * @ingroup Database
 */


/**
 * A multi-wiki, multi-master factory for Wikimedia and similar installations.
 * Ignores the old configuration globals
 *
 * Configuration:
 *     sectionsByDB                A map of database names to section names
 *
 *     sectionLoads                A 2-d map. For each section, gives a map of server names to load ratios.
 *                                 For example: array( 'section1' => array( 'db1' => 100, 'db2' => 100 ) )
 *
 *     serverTemplate              A server info associative array as documented for $wgDBservers. The host,
 *                                 hostName and load entries will be overridden.
 *
 *     groupLoadsBySection         A 3-d map giving server load ratios for each section and group. For example:
 *                                 array( 'section1' => array( 'group1' => array( 'db1' => 100, 'db2' => 100 ) ) )
 *
 *     groupLoadsByDB              A 3-d map giving server load ratios by DB name.
 *
 *     hostsByName                 A map of hostname to IP address.
 *
 *     externalLoads               A map of external storage cluster name to server load map
 *
 *     externalTemplateOverrides   A set of server info keys overriding serverTemplate for external storage
 *
 *     templateOverridesByServer   A 2-d map overriding serverTemplate and externalTemplateOverrides on a
 *                                 server-by-server basis. Applies to both core and external storage.
 *
 *     templateOverridesByCluster  A 2-d map overriding the server info by external storage cluster
 *
 *     masterTemplateOverrides     An override array for all master servers.
 *
 * @ingroup Database
 */
class LBFactory_Multi extends LBFactory {
	// Required settings
	var $sectionsByDB, $sectionLoads, $serverTemplate;
	// Optional settings
	var $groupLoadsBySection = array(), $groupLoadsByDB = array(), $hostsByName = array();
	var $externalLoads = array(), $externalTemplateOverrides, $templateOverridesByServer;
	var $templateOverridesByCluster, $masterTemplateOverrides;
	// Other stuff
	var $conf, $mainLBs = array(), $extLBs = array();
	var $lastWiki, $lastSection;

	function __construct( $conf ) {
		$this->chronProt = new ChronologyProtector;
		$this->conf = $conf;
		$required = array( 'sectionsByDB', 'sectionLoads', 'serverTemplate' );
		$optional = array( 'groupLoadsBySection', 'groupLoadsByDB', 'hostsByName',
			'externalLoads', 'externalTemplateOverrides', 'templateOverridesByServer',
			'templateOverridesByCluster', 'masterTemplateOverrides' );

		foreach ( $required as $key ) {
			if ( !isset( $conf[$key] ) ) {
				throw new MWException( __CLASS__.": $key is required in configuration" );
			}
			$this->$key = $conf[$key];
		}

		foreach ( $optional as $key ) {
			if ( isset( $conf[$key] ) ) {
				$this->$key = $conf[$key];
			}
		}
	}

	function getSectionForWiki( $wiki = false ) {
		if ( $this->lastWiki === $wiki ) {
			return $this->lastSection;
		}
		list( $dbName, $prefix ) = $this->getDBNameAndPrefix( $wiki );
		if ( isset( $this->sectionsByDB[$dbName] ) ) {
			$section = $this->sectionsByDB[$dbName];
		} else {
			$section = 'DEFAULT';
		}
		$this->lastSection = $section;
		$this->lastWiki = $wiki;
		return $section;
	}

	function newMainLB( $wiki = false ) {
		list( $dbName, $prefix ) = $this->getDBNameAndPrefix( $wiki );
		$section = $this->getSectionForWiki( $wiki );
		$groupLoads = array();
		if ( isset( $this->groupLoadsByDB[$dbName] ) ) {
			$groupLoads = $this->groupLoadsByDB[$dbName];
		}
		if ( isset( $this->groupLoadsBySection[$section] ) ) {
			$groupLoads = array_merge_recursive( $groupLoads, $this->groupLoadsBySection[$section] );
		}
		return $this->newLoadBalancer( $this->serverTemplate, $this->sectionLoads[$section], $groupLoads );
	}

	function getMainLB( $wiki = false ) {
		$section = $this->getSectionForWiki( $wiki );
		if ( !isset( $this->mainLBs[$section] ) ) {
			$lb = $this->newMainLB( $wiki, $section );
			$this->chronProt->initLB( $lb );
			$lb->parentInfo( array( 'id' => "main-$section" ) );
			$this->mainLBs[$section] = $lb;
		}
		return $this->mainLBs[$section];
	}

	function newExternalLB( $cluster, $wiki = false ) {
		if ( !isset( $this->externalLoads[$cluster] ) ) {
			throw new MWException( __METHOD__.": Unknown cluster \"$cluster\"" );
		}
		$template = $this->serverTemplate;
		if ( isset( $this->externalTemplateOverrides ) ) {
			$template = $this->externalTemplateOverrides + $template;
		}
		if ( isset( $this->templateOverridesByCluster[$cluster] ) ) {
			$template = $this->templateOverridesByCluster[$cluster] + $template;
		}
		return $this->newLoadBalancer( $template, $this->externalLoads[$cluster], array() );
	}

	function &getExternalLB( $cluster, $wiki = false ) {
		if ( !isset( $this->extLBs[$cluster] ) ) {
			$this->extLBs[$cluster] = $this->newExternalLB( $cluster, $wiki );
			$this->extLBs[$cluster]->parentInfo( array( 'id' => "ext-$cluster" ) );
		}
		return $this->extLBs[$cluster];
	}

	/**
	 * Make a new load balancer object based on template and load array
	 */
	function newLoadBalancer( $template, $loads, $groupLoads ) {
		global $wgMasterWaitTimeout;
		$servers = $this->makeServerArray( $template, $loads, $groupLoads );
		$lb = new LoadBalancer( array(
			'servers' => $servers,
			'masterWaitTimeout' => $wgMasterWaitTimeout 
		));
		return $lb;
	}

	/**
	 * Make a server array as expected by LoadBalancer::__construct, using a template and load array
	 */
	function makeServerArray( $template, $loads, $groupLoads ) {
		$servers = array();
		$master = true;
		$groupLoadsByServer = $this->reindexGroupLoads( $groupLoads );
		foreach ( $groupLoadsByServer as $server => $stuff ) {
			if ( !isset( $loads[$server] ) ) {
				$loads[$server] = 0;
			}
		}
		foreach ( $loads as $serverName => $load ) {
			$serverInfo = $template;
			if ( $master ) {
				$serverInfo['master'] = true;
				if ( isset( $this->masterTemplateOverrides ) ) {
					$serverInfo = $this->masterTemplateOverrides + $serverInfo;
				}
				$master = false;
			}
			if ( isset( $this->templateOverridesByServer[$serverName] ) ) {
				$serverInfo = $this->templateOverridesByServer[$serverName] + $serverInfo;
			}
			if ( isset( $groupLoadsByServer[$serverName] ) ) {
				$serverInfo['groupLoads'] = $groupLoadsByServer[$serverName];
			}
			if ( isset( $this->hostsByName[$serverName] ) ) {
				$serverInfo['host'] = $this->hostsByName[$serverName];
			} else {
				$serverInfo['host'] = $serverName;
			}
			$serverInfo['hostName'] = $serverName;
			$serverInfo['load'] = $load;
			$servers[] = $serverInfo;
		}
		return $servers;
	}

	/**
	 * Take a group load array indexed by group then server, and reindex it by server then group
	 */
	function reindexGroupLoads( $groupLoads ) {
		$reindexed = array();
		foreach ( $groupLoads as $group => $loads ) {
			foreach ( $loads as $server => $load ) {
				$reindexed[$server][$group] = $load;
			}
		}
		return $reindexed;
	}

	/**
	 * Get the database name and prefix based on the wiki ID
	 */
	function getDBNameAndPrefix( $wiki = false ) {
		if ( $wiki === false ) {
			global $wgDBname, $wgDBprefix;
			return array( $wgDBname, $wgDBprefix );
		} else {
			return wfSplitWikiID( $wiki );
		}
	}

	/**
	 * Execute a function for each tracked load balancer
	 * The callback is called with the load balancer as the first parameter,
	 * and $params passed as the subsequent parameters.
	 */
	function forEachLB( $callback, $params = array() ) {
		foreach ( $this->mainLBs as $lb ) {
			call_user_func_array( $callback, array_merge( array( $lb ), $params ) );
		}
		foreach ( $this->extLBs as $lb ) {
			call_user_func_array( $callback, array_merge( array( $lb ), $params ) );
		}
	}

	function shutdown() {
		foreach ( $this->mainLBs as $lb ) {
			$this->chronProt->shutdownLB( $lb );
		}
		$this->chronProt->shutdown();
		$this->commitMasterChanges();
	}
}
