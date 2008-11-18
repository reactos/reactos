<?php
/**
 * @file
 * @ingroup Database
 */

/**
 * An interface for generating database load balancers
 * @ingroup Database
 */
abstract class LBFactory {
	static $instance;

	/**
	 * Get an LBFactory instance
	 */
	static function &singleton() {
		if ( is_null( self::$instance ) ) {
			global $wgLBFactoryConf;
			$class = $wgLBFactoryConf['class'];
			self::$instance = new $class( $wgLBFactoryConf );
		}
		return self::$instance;
	}

	/**
	 * Shut down, close connections and destroy the cached instance.
	 * 
	 */
	static function destroyInstance() {
		if ( self::$instance ) {
			self::$instance->shutdown();
			self::$instance->forEachLBCallMethod( 'closeAll' );
			self::$instance = null;
		}
	}

	/**
	 * Construct a factory based on a configuration array (typically from $wgLBFactoryConf)
	 */
	abstract function __construct( $conf );

	/**
	 * Create a new load balancer object. The resulting object will be untracked, 
	 * not chronology-protected, and the caller is responsible for cleaning it up.
	 *
	 * @param string $wiki Wiki ID, or false for the current wiki
	 * @return LoadBalancer
	 */
	abstract function newMainLB( $wiki = false );

	/**
	 * Get a cached (tracked) load balancer object.
	 *
	 * @param string $wiki Wiki ID, or false for the current wiki
	 * @return LoadBalancer
	 */
	abstract function getMainLB( $wiki = false );

	/*
	 * Create a new load balancer for external storage. The resulting object will be 
	 * untracked, not chronology-protected, and the caller is responsible for 
	 * cleaning it up.
	 *
	 * @param string $cluster External storage cluster, or false for core
	 * @param string $wiki Wiki ID, or false for the current wiki
	 */
	abstract function newExternalLB( $cluster, $wiki = false );

	/*
	 * Get a cached (tracked) load balancer for external storage
	 *
	 * @param string $cluster External storage cluster, or false for core
	 * @param string $wiki Wiki ID, or false for the current wiki
	 */
	abstract function &getExternalLB( $cluster, $wiki = false );

	/**
	 * Execute a function for each tracked load balancer
	 * The callback is called with the load balancer as the first parameter,
	 * and $params passed as the subsequent parameters.
	 */
	abstract function forEachLB( $callback, $params = array() );

	/**
	 * Prepare all tracked load balancers for shutdown
	 * STUB
	 */
	function shutdown() {}

	/**
	 * Call a method of each tracked load balancer
	 */
	function forEachLBCallMethod( $methodName, $args = array() ) {
		$this->forEachLB( array( $this, 'callMethod' ), array( $methodName, $args ) );
	}

	/**
	 * Private helper for forEachLBCallMethod
	 */
	function callMethod( $loadBalancer, $methodName, $args ) {
		call_user_func_array( array( $loadBalancer, $methodName ), $args );
	}

	/**
	 * Commit changes on all master connections
	 */
	function commitMasterChanges() {
		$this->forEachLBCallMethod( 'commitMasterChanges' );
	}
}

/**
 * A simple single-master LBFactory that gets its configuration from the b/c globals
 */
class LBFactory_Simple extends LBFactory {
	var $mainLB;
	var $extLBs = array();

	# Chronology protector
	var $chronProt;

	function __construct( $conf ) {
		$this->chronProt = new ChronologyProtector;
	}

	function newMainLB( $wiki = false ) {
		global $wgDBservers, $wgMasterWaitTimeout;
		if ( $wgDBservers ) {
			$servers = $wgDBservers;
		} else {
			global $wgDBserver, $wgDBuser, $wgDBpassword, $wgDBname, $wgDBtype, $wgDebugDumpSql;
			$servers = array(array(
				'host' => $wgDBserver,
				'user' => $wgDBuser,
				'password' => $wgDBpassword,
				'dbname' => $wgDBname,
				'type' => $wgDBtype,
				'load' => 1,
				'flags' => ($wgDebugDumpSql ? DBO_DEBUG : 0) | DBO_DEFAULT
			));
		}

		return new LoadBalancer( array(
			'servers' => $servers, 
			'masterWaitTimeout' => $wgMasterWaitTimeout 
		));
	}

	function getMainLB( $wiki = false ) {
		if ( !isset( $this->mainLB ) ) {
			$this->mainLB = $this->newMainLB( $wiki );
			$this->mainLB->parentInfo( array( 'id' => 'main' ) );
			$this->chronProt->initLB( $this->mainLB );
		}
		return $this->mainLB;
	}

	function newExternalLB( $cluster, $wiki = false ) {
		global $wgExternalServers;
		if ( !isset( $wgExternalServers[$cluster] ) ) {
			throw new MWException( __METHOD__.": Unknown cluster \"$cluster\"" );
		}
		return new LoadBalancer( array(
			'servers' => $wgExternalServers[$cluster] 
		));
	}

	function &getExternalLB( $cluster, $wiki = false ) {
		if ( !isset( $this->extLBs[$cluster] ) ) {
			$this->extLBs[$cluster] = $this->newExternalLB( $cluster, $wiki );
			$this->extLBs[$cluster]->parentInfo( array( 'id' => "ext-$cluster" ) );
		}
		return $this->extLBs[$cluster];
	}

	/**
	 * Execute a function for each tracked load balancer
	 * The callback is called with the load balancer as the first parameter,
	 * and $params passed as the subsequent parameters.
	 */
	function forEachLB( $callback, $params = array() ) {
		if ( isset( $this->mainLB ) ) {
			call_user_func_array( $callback, array_merge( array( $this->mainLB ), $params ) );
		}
		foreach ( $this->extLBs as $lb ) {
			call_user_func_array( $callback, array_merge( array( $lb ), $params ) );
		}
	}

	function shutdown() {
		if ( $this->mainLB ) {
			$this->chronProt->shutdownLB( $this->mainLB );
		}
		$this->chronProt->shutdown();
		$this->commitMasterChanges();
	}
}

/**
 * Class for ensuring a consistent ordering of events as seen by the user, despite replication.
 * Kind of like Hawking's [[Chronology Protection Agency]].
 */
class ChronologyProtector {
	var $startupPos;
	var $shutdownPos = array();

	/**
	 * Initialise a LoadBalancer to give it appropriate chronology protection.
	 *
	 * @param LoadBalancer $lb
	 */
	function initLB( $lb ) {
		if ( $this->startupPos === null ) {
			if ( !empty( $_SESSION[__CLASS__] ) ) {
				$this->startupPos = $_SESSION[__CLASS__];
			}
		}
		if ( !$this->startupPos ) {
			return;
		}
		$masterName = $lb->getServerName( 0 );

		if ( $lb->getServerCount() > 1 && !empty( $this->startupPos[$masterName] ) ) {
			$info = $lb->parentInfo();
			$pos = $this->startupPos[$masterName];
			wfDebug( __METHOD__.": LB " . $info['id'] . " waiting for master pos $pos\n" );
			$lb->waitFor( $this->startupPos[$masterName] );
		}
	}

	/**
	 * Notify the ChronologyProtector that the LoadBalancer is about to shut
	 * down. Saves replication positions.
	 *
	 * @param LoadBalancer $lb
	 */
	function shutdownLB( $lb ) {
		if ( session_id() != '' && $lb->getServerCount() > 1 ) {
			$masterName = $lb->getServerName( 0 );
			if ( !isset( $this->shutdownPos[$masterName] ) ) {
				$pos = $lb->getMasterPos();
				$info = $lb->parentInfo();
				wfDebug( __METHOD__.": LB " . $info['id'] . " has master pos $pos\n" );
				$this->shutdownPos[$masterName] = $pos;
			}
		}
	}

	/**
	 * Notify the ChronologyProtector that the LBFactory is done calling shutdownLB() for now.
	 * May commit chronology data to persistent storage.
	 */
	function shutdown() {
		if ( session_id() != '' && count( $this->shutdownPos ) ) {
			wfDebug( __METHOD__.": saving master pos for " .
				count( $this->shutdownPos ) . " master(s)\n" );
			$_SESSION[__CLASS__] = $this->shutdownPos;
		}
	}
}
