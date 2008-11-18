<?php
//
// +---------------------------------------------------------------------------+
// | memcached client, PHP                                                     |
// +---------------------------------------------------------------------------+
// | Copyright (c) 2003 Ryan T. Dean <rtdean@cytherianage.net>                 |
// | All rights reserved.                                                      |
// |                                                                           |
// | Redistribution and use in source and binary forms, with or without        |
// | modification, are permitted provided that the following conditions        |
// | are met:                                                                  |
// |                                                                           |
// | 1. Redistributions of source code must retain the above copyright         |
// |    notice, this list of conditions and the following disclaimer.          |
// | 2. Redistributions in binary form must reproduce the above copyright      |
// |    notice, this list of conditions and the following disclaimer in the    |
// |    documentation and/or other materials provided with the distribution.   |
// |                                                                           |
// | THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR      |
// | IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES |
// | OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.   |
// | IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,          |
// | INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT  |
// | NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, |
// | DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY     |
// | THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT       |
// | (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF  |
// | THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.         |
// +---------------------------------------------------------------------------+
// | Author: Ryan T. Dean <rtdean@cytherianage.net>                            |
// | Heavily influenced by the Perl memcached client by Brad Fitzpatrick.      |
// |   Permission granted by Brad Fitzpatrick for relicense of ported Perl     |
// |   client logic under 2-clause BSD license.                                |
// +---------------------------------------------------------------------------+
//
// $TCAnet$
//

/**
 * This is the PHP client for memcached - a distributed memory cache daemon.
 * More information is available at http://www.danga.com/memcached/
 *
 * Usage example:
 *
 * require_once 'memcached.php';
 *
 * $mc = new memcached(array(
 *              'servers' => array('127.0.0.1:10000',
 *                                 array('192.0.0.1:10010', 2),
 *                                 '127.0.0.1:10020'),
 *              'debug'   => false,
 *              'compress_threshold' => 10240,
 *              'persistant' => true));
 *
 * $mc->add('key', array('some', 'array'));
 * $mc->replace('key', 'some random string');
 * $val = $mc->get('key');
 *
 * @author  Ryan T. Dean <rtdean@cytherianage.net>
 * @version 0.1.2
 */

// {{{ requirements
// }}}

// {{{ class memcached
/**
 * memcached client class implemented using (p)fsockopen()
 *
 * @author  Ryan T. Dean <rtdean@cytherianage.net>
 * @ingroup Cache
 */
class memcached
{
   // {{{ properties
   // {{{ public

		// {{{ constants
		// {{{ flags

		/**
		 * Flag: indicates data is serialized
		 */
		const SERIALIZED = 1;

		/**
		 * Flag: indicates data is compressed
		 */
		const COMPRESSED = 2;

		// }}}

		/**
		 * Minimum savings to store data compressed
		 */
		const COMPRESSION_SAVINGS = 0.20;

		// }}}


   /**
    * Command statistics
    *
    * @var     array
    * @access  public
    */
   var $stats;

   // }}}
   // {{{ private

   /**
    * Cached Sockets that are connected
    *
    * @var     array
    * @access  private
    */
   var $_cache_sock;

   /**
    * Current debug status; 0 - none to 9 - profiling
    *
    * @var     boolean
    * @access  private
    */
   var $_debug;

   /**
    * Dead hosts, assoc array, 'host'=>'unixtime when ok to check again'
    *
    * @var     array
    * @access  private
    */
   var $_host_dead;

   /**
    * Is compression available?
    *
    * @var     boolean
    * @access  private
    */
   var $_have_zlib;

   /**
    * Do we want to use compression?
    *
    * @var     boolean
    * @access  private
    */
   var $_compress_enable;

   /**
    * At how many bytes should we compress?
    *
    * @var     integer
    * @access  private
    */
   var $_compress_threshold;

   /**
    * Are we using persistant links?
    *
    * @var     boolean
    * @access  private
    */
   var $_persistant;

   /**
    * If only using one server; contains ip:port to connect to
    *
    * @var     string
    * @access  private
    */
   var $_single_sock;

   /**
    * Array containing ip:port or array(ip:port, weight)
    *
    * @var     array
    * @access  private
    */
   var $_servers;

   /**
    * Our bit buckets
    *
    * @var     array
    * @access  private
    */
   var $_buckets;

   /**
    * Total # of bit buckets we have
    *
    * @var     integer
    * @access  private
    */
   var $_bucketcount;

   /**
    * # of total servers we have
    *
    * @var     integer
    * @access  private
    */
   var $_active;

   /**
    * Stream timeout in seconds. Applies for example to fread()
    *
    * @var     integer
    * @access  private
    */
   var $_timeout_seconds;

   /**
    * Stream timeout in microseconds
    *
    * @var     integer
    * @access  private
    */
   var $_timeout_microseconds;

   /**
    * Connect timeout in seconds
    */
   var $_connect_timeout;

   /**
    * Number of connection attempts for each server
    */
   var $_connect_attempts;

   // }}}
   // }}}
   // {{{ methods
   // {{{ public functions
   // {{{ memcached()

   /**
    * Memcache initializer
    *
    * @param   array    $args    Associative array of settings
    *
    * @return  mixed
    * @access  public
    */
   function memcached ($args)
   {
      $this->set_servers(@$args['servers']);
      $this->_debug = @$args['debug'];
      $this->stats = array();
      $this->_compress_threshold = @$args['compress_threshold'];
      $this->_persistant = array_key_exists('persistant', $args) ? (@$args['persistant']) : false;
      $this->_compress_enable = true;
      $this->_have_zlib = function_exists("gzcompress");

      $this->_cache_sock = array();
      $this->_host_dead = array();

      $this->_timeout_seconds = 1;
      $this->_timeout_microseconds = 0;

      $this->_connect_timeout = 0.01;
      $this->_connect_attempts = 3;
   }

   // }}}
   // {{{ add()

   /**
    * Adds a key/value to the memcache server if one isn't already set with
    * that key
    *
    * @param   string  $key     Key to set with data
    * @param   mixed   $val     Value to store
    * @param   integer $exp     (optional) Time to expire data at
    *
    * @return  boolean
    * @access  public
    */
   function add ($key, $val, $exp = 0)
   {
      return $this->_set('add', $key, $val, $exp);
   }

   // }}}
   // {{{ decr()

   /**
    * Decriment a value stored on the memcache server
    *
    * @param   string   $key     Key to decriment
    * @param   integer  $amt     (optional) Amount to decriment
    *
    * @return  mixed    FALSE on failure, value on success
    * @access  public
    */
   function decr ($key, $amt=1)
   {
      return $this->_incrdecr('decr', $key, $amt);
   }

   // }}}
   // {{{ delete()

   /**
    * Deletes a key from the server, optionally after $time
    *
    * @param   string   $key     Key to delete
    * @param   integer  $time    (optional) How long to wait before deleting
    *
    * @return  boolean  TRUE on success, FALSE on failure
    * @access  public
    */
   function delete ($key, $time = 0)
   {
      if (!$this->_active)
         return false;

      $sock = $this->get_sock($key);
      if (!is_resource($sock))
         return false;

      $key = is_array($key) ? $key[1] : $key;

      @$this->stats['delete']++;
      $cmd = "delete $key $time\r\n";
      if(!$this->_safe_fwrite($sock, $cmd, strlen($cmd)))
      {
         $this->_dead_sock($sock);
         return false;
      }
      $res = trim(fgets($sock));

      if ($this->_debug)
         $this->_debugprint(sprintf("MemCache: delete %s (%s)\n", $key, $res));

      if ($res == "DELETED")
         return true;
      return false;
   }

   // }}}
   // {{{ disconnect_all()

   /**
    * Disconnects all connected sockets
    *
    * @access  public
    */
   function disconnect_all ()
   {
      foreach ($this->_cache_sock as $sock)
         fclose($sock);

      $this->_cache_sock = array();
   }

   // }}}
   // {{{ enable_compress()

   /**
    * Enable / Disable compression
    *
    * @param   boolean  $enable  TRUE to enable, FALSE to disable
    *
    * @access  public
    */
   function enable_compress ($enable)
   {
      $this->_compress_enable = $enable;
   }

   // }}}
   // {{{ forget_dead_hosts()

   /**
    * Forget about all of the dead hosts
    *
    * @access  public
    */
   function forget_dead_hosts ()
   {
      $this->_host_dead = array();
   }

   // }}}
   // {{{ get()

   /**
    * Retrieves the value associated with the key from the memcache server
    *
    * @param  string   $key     Key to retrieve
    *
    * @return  mixed
    * @access  public
    */
   function get ($key)
   {
      $fname = 'memcached::get';
      wfProfileIn( $fname );

      if ( $this->_debug ) {
         $this->_debugprint( "get($key)\n" );
      }

      if (!$this->_active) {
	 wfProfileOut( $fname );
         return false;
      }

      $sock = $this->get_sock($key);

      if (!is_resource($sock)) {
	 wfProfileOut( $fname );
         return false;
      }

      @$this->stats['get']++;

      $cmd = "get $key\r\n";
      if (!$this->_safe_fwrite($sock, $cmd, strlen($cmd)))
      {
         $this->_dead_sock($sock);
	 wfProfileOut( $fname );
         return false;
      }

      $val = array();
      $this->_load_items($sock, $val);

      if ($this->_debug)
         foreach ($val as $k => $v)
            $this->_debugprint(sprintf("MemCache: sock %s got %s\n", serialize($sock), $k));

      wfProfileOut( $fname );
      return @$val[$key];
   }

   // }}}
   // {{{ get_multi()

   /**
    * Get multiple keys from the server(s)
    *
    * @param   array    $keys    Keys to retrieve
    *
    * @return  array
    * @access  public
    */
   function get_multi ($keys)
   {
      if (!$this->_active)
         return false;

      $this->stats['get_multi']++;
      $sock_keys = array();

      foreach ($keys as $key)
      {
         $sock = $this->get_sock($key);
         if (!is_resource($sock)) continue;
         $key = is_array($key) ? $key[1] : $key;
         if (!isset($sock_keys[$sock]))
         {
            $sock_keys[$sock] = array();
            $socks[] = $sock;
         }
         $sock_keys[$sock][] = $key;
      }

      // Send out the requests
      foreach ($socks as $sock)
      {
         $cmd = "get";
         foreach ($sock_keys[$sock] as $key)
         {
            $cmd .= " ". $key;
         }
         $cmd .= "\r\n";

         if ($this->_safe_fwrite($sock, $cmd, strlen($cmd)))
         {
            $gather[] = $sock;
         } else
         {
            $this->_dead_sock($sock);
         }
      }

      // Parse responses
      $val = array();
      foreach ($gather as $sock)
      {
         $this->_load_items($sock, $val);
      }

      if ($this->_debug)
         foreach ($val as $k => $v)
            $this->_debugprint(sprintf("MemCache: got %s\n", $k));

      return $val;
   }

   // }}}
   // {{{ incr()

   /**
    * Increments $key (optionally) by $amt
    *
    * @param   string   $key     Key to increment
    * @param   integer  $amt     (optional) amount to increment
    *
    * @return  integer  New key value?
    * @access  public
    */
   function incr ($key, $amt=1)
   {
      return $this->_incrdecr('incr', $key, $amt);
   }

   // }}}
   // {{{ replace()

   /**
    * Overwrites an existing value for key; only works if key is already set
    *
    * @param   string   $key     Key to set value as
    * @param   mixed    $value   Value to store
    * @param   integer  $exp     (optional) Experiation time
    *
    * @return  boolean
    * @access  public
    */
   function replace ($key, $value, $exp=0)
   {
      return $this->_set('replace', $key, $value, $exp);
   }

   // }}}
   // {{{ run_command()

   /**
    * Passes through $cmd to the memcache server connected by $sock; returns
    * output as an array (null array if no output)
    *
    * NOTE: due to a possible bug in how PHP reads while using fgets(), each
    *       line may not be terminated by a \r\n.  More specifically, my testing
    *       has shown that, on FreeBSD at least, each line is terminated only
    *       with a \n.  This is with the PHP flag auto_detect_line_endings set
    *       to falase (the default).
    *
    * @param   resource $sock    Socket to send command on
    * @param   string   $cmd     Command to run
    *
    * @return  array    Output array
    * @access  public
    */
   function run_command ($sock, $cmd)
   {
      if (!is_resource($sock))
         return array();

      if (!$this->_safe_fwrite($sock, $cmd, strlen($cmd)))
         return array();

      while (true)
      {
         $res = fgets($sock);
         $ret[] = $res;
         if (preg_match('/^END/', $res))
            break;
         if (strlen($res) == 0)
            break;
      }
      return $ret;
   }

   // }}}
   // {{{ set()

   /**
    * Unconditionally sets a key to a given value in the memcache.  Returns true
    * if set successfully.
    *
    * @param   string   $key     Key to set value as
    * @param   mixed    $value   Value to set
    * @param   integer  $exp     (optional) Experiation time
    *
    * @return  boolean  TRUE on success
    * @access  public
    */
   function set ($key, $value, $exp=0)
   {
      return $this->_set('set', $key, $value, $exp);
   }

   // }}}
   // {{{ set_compress_threshold()

   /**
    * Sets the compression threshold
    *
    * @param   integer  $thresh  Threshold to compress if larger than
    *
    * @access  public
    */
   function set_compress_threshold ($thresh)
   {
      $this->_compress_threshold = $thresh;
   }

   // }}}
   // {{{ set_debug()

   /**
    * Sets the debug flag
    *
    * @param   boolean  $dbg     TRUE for debugging, FALSE otherwise
    *
    * @access  public
    *
    * @see     memcahced::memcached
    */
   function set_debug ($dbg)
   {
      $this->_debug = $dbg;
   }

   // }}}
   // {{{ set_servers()

   /**
    * Sets the server list to distribute key gets and puts between
    *
    * @param   array    $list    Array of servers to connect to
    *
    * @access  public
    *
    * @see     memcached::memcached()
    */
   function set_servers ($list)
   {
      $this->_servers = $list;
      $this->_active = count($list);
      $this->_buckets = null;
      $this->_bucketcount = 0;

      $this->_single_sock = null;
      if ($this->_active == 1)
         $this->_single_sock = $this->_servers[0];
   }

   /**
    * Sets the timeout for new connections
    *
    * @param   integer  $seconds Number of seconds
    * @param   integer  $microseconds  Number of microseconds
    *
    * @access  public
    */
   function set_timeout ($seconds, $microseconds)
   {
      $this->_timeout_seconds = $seconds;
      $this->_timeout_microseconds = $microseconds;
   }

   // }}}
   // }}}
   // {{{ private methods
   // {{{ _close_sock()

   /**
    * Close the specified socket
    *
    * @param   string   $sock    Socket to close
    *
    * @access  private
    */
   function _close_sock ($sock)
   {
      $host = array_search($sock, $this->_cache_sock);
      fclose($this->_cache_sock[$host]);
      unset($this->_cache_sock[$host]);
   }

   // }}}
   // {{{ _connect_sock()

   /**
    * Connects $sock to $host, timing out after $timeout
    *
    * @param   integer  $sock    Socket to connect
    * @param   string   $host    Host:IP to connect to
    *
    * @return  boolean
    * @access  private
    */
   function _connect_sock (&$sock, $host)
   {
      list ($ip, $port) = explode(":", $host);
      $sock = false;
      $timeout = $this->_connect_timeout;
      $errno = $errstr = null;
      for ($i = 0; !$sock && $i < $this->_connect_attempts; $i++) {
         if ($i > 0) {
            # Sleep until the timeout, in case it failed fast
            $elapsed = microtime(true) - $t;
            if ( $elapsed < $timeout ) {
               usleep(($timeout - $elapsed) * 1e6);
            }
            $timeout *= 2;
         }
         $t = microtime(true);
         if ($this->_persistant == 1)
         {
            $sock = @pfsockopen($ip, $port, $errno, $errstr, $timeout);
         } else
         {
            $sock = @fsockopen($ip, $port, $errno, $errstr, $timeout);
         }
      }
      if (!$sock) {
         if ($this->_debug)
            $this->_debugprint( "Error connecting to $host: $errstr\n" );
         return false;
      }

      // Initialise timeout
      stream_set_timeout($sock, $this->_timeout_seconds, $this->_timeout_microseconds);

      return true;
   }

   // }}}
   // {{{ _dead_sock()

   /**
    * Marks a host as dead until 30-40 seconds in the future
    *
    * @param   string   $sock    Socket to mark as dead
    *
    * @access  private
    */
   function _dead_sock ($sock)
   {
      $host = array_search($sock, $this->_cache_sock);
      @list ($ip, /* $port */) = explode(":", $host);
      $this->_host_dead[$ip] = time() + 30 + intval(rand(0, 10));
      $this->_host_dead[$host] = $this->_host_dead[$ip];
      unset($this->_cache_sock[$host]);
   }

   // }}}
   // {{{ get_sock()

   /**
    * get_sock
    *
    * @param   string   $key     Key to retrieve value for;
    *
    * @return  mixed    resource on success, false on failure
    * @access  private
    */
   function get_sock ($key)
   {
      if (!$this->_active)
         return false;

      if ($this->_single_sock !== null) {
         $this->_flush_read_buffer($this->_single_sock);
         return $this->sock_to_host($this->_single_sock);
      }

      $hv = is_array($key) ? intval($key[0]) : $this->_hashfunc($key);

      if ($this->_buckets === null)
      {
         foreach ($this->_servers as $v)
         {
            if (is_array($v))
            {
               for ($i=0; $i<$v[1]; $i++)
                  $bu[] = $v[0];
            } else
            {
               $bu[] = $v;
            }
         }
         $this->_buckets = $bu;
         $this->_bucketcount = count($bu);
      }

      $realkey = is_array($key) ? $key[1] : $key;
      for ($tries = 0; $tries<20; $tries++)
      {
         $host = $this->_buckets[$hv % $this->_bucketcount];
         $sock = $this->sock_to_host($host);
         if (is_resource($sock)) {
            $this->_flush_read_buffer($sock);
            return $sock;
         }
         $hv += $this->_hashfunc($tries . $realkey);
      }

      return false;
   }

   // }}}
   // {{{ _hashfunc()

   /**
    * Creates a hash integer  based on the $key
    *
    * @param   string   $key     Key to hash
    *
    * @return  integer  Hash value
    * @access  private
    */
   function _hashfunc ($key)
   {
      # Hash function must on [0,0x7ffffff]
      # We take the first 31 bits of the MD5 hash, which unlike the hash
      # function used in a previous version of this client, works
      return hexdec(substr(md5($key),0,8)) & 0x7fffffff;
   }

   // }}}
   // {{{ _incrdecr()

   /**
    * Perform increment/decriment on $key
    *
    * @param   string   $cmd     Command to perform
    * @param   string   $key     Key to perform it on
    * @param   integer  $amt     Amount to adjust
    *
    * @return  integer     New value of $key
    * @access  private
    */
   function _incrdecr ($cmd, $key, $amt=1)
   {
      if (!$this->_active)
         return null;

      $sock = $this->get_sock($key);
      if (!is_resource($sock))
         return null;

      $key = is_array($key) ? $key[1] : $key;
      @$this->stats[$cmd]++;
      if (!$this->_safe_fwrite($sock, "$cmd $key $amt\r\n"))
         return $this->_dead_sock($sock);

      stream_set_timeout($sock, 1, 0);
      $line = fgets($sock);
      $match = array();
      if (!preg_match('/^(\d+)/', $line, $match))
         return null;
      return $match[1];
   }

   // }}}
   // {{{ _load_items()

   /**
    * Load items into $ret from $sock
    *
    * @param   resource $sock    Socket to read from
    * @param   array    $ret     Returned values
    *
    * @access  private
    */
   function _load_items ($sock, &$ret)
   {
      while (1)
      {
         $decl = fgets($sock);
         if ($decl == "END\r\n")
         {
            return true;
         } elseif (preg_match('/^VALUE (\S+) (\d+) (\d+)\r\n$/', $decl, $match))
         {
            list($rkey, $flags, $len) = array($match[1], $match[2], $match[3]);
            $bneed = $len+2;
            $offset = 0;

            while ($bneed > 0)
            {
               $data = fread($sock, $bneed);
               $n = strlen($data);
               if ($n == 0)
                  break;
               $offset += $n;
               $bneed -= $n;
               @$ret[$rkey] .= $data;
            }

            if ($offset != $len+2)
            {
               // Something is borked!
               if ($this->_debug)
                  $this->_debugprint(sprintf("Something is borked!  key %s expecting %d got %d length\n", $rkey, $len+2, $offset));

               unset($ret[$rkey]);
               $this->_close_sock($sock);
               return false;
            }

            if ($this->_have_zlib && $flags & memcached::COMPRESSED)
               $ret[$rkey] = gzuncompress($ret[$rkey]);

            $ret[$rkey] = rtrim($ret[$rkey]);

            if ($flags & memcached::SERIALIZED)
               $ret[$rkey] = unserialize($ret[$rkey]);

         } else
         {
            $this->_debugprint("Error parsing memcached response\n");
            return 0;
         }
      }
   }

   // }}}
   // {{{ _set()

   /**
    * Performs the requested storage operation to the memcache server
    *
    * @param   string   $cmd     Command to perform
    * @param   string   $key     Key to act on
    * @param   mixed    $val     What we need to store
    * @param   integer  $exp     When it should expire
    *
    * @return  boolean
    * @access  private
    */
   function _set ($cmd, $key, $val, $exp)
   {
      if (!$this->_active)
         return false;

      $sock = $this->get_sock($key);
      if (!is_resource($sock))
         return false;

      @$this->stats[$cmd]++;

      $flags = 0;

      if (!is_scalar($val))
      {
         $val = serialize($val);
         $flags |= memcached::SERIALIZED;
         if ($this->_debug)
            $this->_debugprint(sprintf("client: serializing data as it is not scalar\n"));
      }

      $len = strlen($val);

      if ($this->_have_zlib && $this->_compress_enable &&
          $this->_compress_threshold && $len >= $this->_compress_threshold)
      {
         $c_val = gzcompress($val, 9);
         $c_len = strlen($c_val);

         if ($c_len < $len*(1 - memcached::COMPRESSION_SAVINGS))
         {
            if ($this->_debug)
               $this->_debugprint(sprintf("client: compressing data; was %d bytes is now %d bytes\n", $len, $c_len));
            $val = $c_val;
            $len = $c_len;
            $flags |= memcached::COMPRESSED;
         }
      }
      if (!$this->_safe_fwrite($sock, "$cmd $key $flags $exp $len\r\n$val\r\n"))
         return $this->_dead_sock($sock);

      $line = trim(fgets($sock));

      if ($this->_debug)
      {
         $this->_debugprint(sprintf("%s %s (%s)\n", $cmd, $key, $line));
      }
      if ($line == "STORED")
         return true;
      return false;
   }

   // }}}
   // {{{ sock_to_host()

   /**
    * Returns the socket for the host
    *
    * @param   string   $host    Host:IP to get socket for
    *
    * @return  mixed    IO Stream or false
    * @access  private
    */
   function sock_to_host ($host)
   {
      if (isset($this->_cache_sock[$host]))
         return $this->_cache_sock[$host];

      $sock = null;
      $now = time();
      list ($ip, /* $port */) = explode (":", $host);
      if (isset($this->_host_dead[$host]) && $this->_host_dead[$host] > $now ||
          isset($this->_host_dead[$ip]) && $this->_host_dead[$ip] > $now)
         return null;

      if (!$this->_connect_sock($sock, $host))
         return $this->_dead_sock($host);

      // Do not buffer writes
      stream_set_write_buffer($sock, 0);

      $this->_cache_sock[$host] = $sock;

      return $this->_cache_sock[$host];
   }

   function _debugprint($str){
      print($str);
   }

   /**
    * Write to a stream, timing out after the correct amount of time
    *
    * @return bool false on failure, true on success
    */
    /*
   function _safe_fwrite($f, $buf, $len = false) {
      stream_set_blocking($f, 0);

      if ($len === false) {
         wfDebug("Writing " . strlen( $buf ) . " bytes\n");
         $bytesWritten = fwrite($f, $buf);
      } else {
         wfDebug("Writing $len bytes\n");
         $bytesWritten = fwrite($f, $buf, $len);
      }
      $n = stream_select($r=NULL, $w = array($f), $e = NULL, 10, 0);
      #   $this->_timeout_seconds, $this->_timeout_microseconds);

      wfDebug("stream_select returned $n\n");
      stream_set_blocking($f, 1);
      return $n == 1;
      return $bytesWritten;
   }*/

   /**
    * Original behaviour
    */
   function _safe_fwrite($f, $buf, $len = false) {
      if ($len === false) {
         $bytesWritten = fwrite($f, $buf);
      } else {
         $bytesWritten = fwrite($f, $buf, $len);
      }
      return $bytesWritten;
   }

   /**
    * Flush the read buffer of a stream
    */
   function _flush_read_buffer($f) {
      if (!is_resource($f)) {
         return;
      }
      $n = stream_select($r=array($f), $w = NULL, $e = NULL, 0, 0);
      while ($n == 1 && !feof($f)) {
         fread($f, 1024);
         $n = stream_select($r=array($f), $w = NULL, $e = NULL, 0, 0);
      }
   }

   // }}}
   // }}}
   // }}}
}

// vim: sts=3 sw=3 et

// }}}
