<?php
/*
 * @Author "Ashar Voultoiz" <hashar@altern.org>
 * @License GPL v2 or later
 */

// Some regex definition to "play" with IP address and IP address blocks

// An IP is made of 4 bytes from x00 to xFF which is d0 to d255
define( 'RE_IP_BYTE', '(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|0?[0-9]?[0-9])');
define( 'RE_IP_ADD' , RE_IP_BYTE . '\.' . RE_IP_BYTE . '\.' . RE_IP_BYTE . '\.' . RE_IP_BYTE );
// An IPv4 block is an IP address and a prefix (d1 to d32)
define( 'RE_IP_PREFIX', '(3[0-2]|[12]?\d)');
define( 'RE_IP_BLOCK', RE_IP_ADD . '\/' . RE_IP_PREFIX);
// For IPv6 canonicalization (NOT for strict validation; these are quite lax!)
define( 'RE_IPV6_WORD', '([0-9A-Fa-f]{1,4})' );
define( 'RE_IPV6_GAP', ':(?:0+:)*(?::(?:0+:)*)?' );
define( 'RE_IPV6_V4_PREFIX', '0*' . RE_IPV6_GAP . '(?:ffff:)?' );
// An IPv6 block is an IP address and a prefix (d1 to d128)
define( 'RE_IPV6_PREFIX', '(12[0-8]|1[01][0-9]|[1-9]?\d)');
// An IPv6 IP is made up of 8 octets. However abbreviations like "::" can be used. This is lax!
define( 'RE_IPV6_ADD', '(:(:' . RE_IPV6_WORD . '){1,7}|' . RE_IPV6_WORD . '(:{1,2}' . RE_IPV6_WORD . '|::$){1,7})' );
define( 'RE_IPV6_BLOCK', RE_IPV6_ADD . '\/' . RE_IPV6_PREFIX );
// This might be useful for regexps used elsewhere, matches any IPv6 or IPv6 address or network
define( 'IP_ADDRESS_STRING',
	'(?:' .
		RE_IP_ADD . '(\/' . RE_IP_PREFIX . '|)' .
	'|' .
		RE_IPV6_ADD . '(\/' . RE_IPV6_PREFIX . '|)' .
	')' );

/**
 * A collection of public static functions to play with IP address
 * and IP blocks.
 */
class IP {
	/**
	 * Given a string, determine if it as valid IP
	 * Unlike isValid(), this looks for networks too
	 * @param $ip IP address.
	 * @return string
	 */
	public static function isIPAddress( $ip ) {
		if ( !$ip ) return false;
		if ( is_array( $ip ) ) {
		  throw new MWException( "invalid value passed to " . __METHOD__ );
		}
		// IPv6 IPs with two "::" strings are ambiguous and thus invalid
		return preg_match( '/^' . IP_ADDRESS_STRING . '$/', $ip) && ( substr_count($ip, '::') < 2 );
	}

	public static function isIPv6( $ip ) {
		if ( !$ip ) return false;
		if( is_array( $ip ) ) {
		  throw new MWException( "invalid value passed to " . __METHOD__ );
		}
		// IPv6 IPs with two "::" strings are ambiguous and thus invalid
		return preg_match( '/^' . RE_IPV6_ADD . '(\/' . RE_IPV6_PREFIX . '|)$/', $ip) && ( substr_count($ip, '::') < 2);
	}

	public static function isIPv4( $ip ) {
		if ( !$ip ) return false;
		return preg_match( '/^' . RE_IP_ADD . '(\/' . RE_IP_PREFIX . '|)$/', $ip);
	}

	/**
	 * Given an IP address in dotted-quad notation, returns an IPv6 octet.
	 * See http://www.answers.com/topic/ipv4-compatible-address
	 * IPs with the first 92 bits as zeros are reserved from IPv6
	 * @param $ip quad-dotted IP address.
	 * @return string
	 */
	public static function IPv4toIPv6( $ip ) {
		if ( !$ip ) return null;
		// Convert only if needed
		if ( self::isIPv6( $ip ) ) return $ip;
		// IPv4 CIDRs
		if ( strpos( $ip, '/' ) !== false ) {
			$parts = explode( '/', $ip, 2 );
			if ( count( $parts ) != 2 ) {
				return false;
			}
			$network = self::toUnsigned( $parts[0] );
			if ( $network !== false && is_numeric( $parts[1] ) && $parts[1] >= 0 && $parts[1] <= 32 ) {
				$bits = $parts[1] + 96;
				return self::toOctet( $network ) . "/$bits";
			} else {
				return false;
			}
		}
		return self::toOctet( self::toUnsigned( $ip ) );
	}

	/**
	 * Given an IPv6 address in octet notation, returns an unsigned integer.
	 * @param $ip octet ipv6 IP address.
	 * @return string
	 */
	public static function toUnsigned6( $ip ) {
		if ( !$ip ) return null;
       	$ip = explode(':', self::sanitizeIP( $ip ) );
       	$r_ip = '';
       	foreach ($ip as $v) {
       		$r_ip .= str_pad( $v, 4, 0, STR_PAD_LEFT );
        }
        $r_ip = wfBaseConvert( $r_ip, 16, 10 );
       	return $r_ip;
	}

	/**
	 * Given an IPv6 address in octet notation, returns the expanded octet.
	 * IPv4 IPs will be trimmed, thats it...
	 * @param $ip octet ipv6 IP address.
	 * @return string
	 */
	public static function sanitizeIP( $ip ) {
		$ip = trim( $ip );
		if ( $ip === '' ) return null;
		// Trim and return IPv4 addresses
		if ( self::isIPv4($ip) ) return $ip;
		// Only IPv6 addresses can be expanded
		if ( !self::isIPv6($ip) ) return $ip;
		// Remove any whitespaces, convert to upper case
		$ip = strtoupper( $ip );
		// Expand zero abbreviations
		if ( strpos( $ip, '::' ) !== false ) {
    		$ip = str_replace('::', str_repeat(':0', 8 - substr_count($ip, ':')) . ':', $ip);
    	}
    	// For IPs that start with "::", correct the final IP so that it starts with '0' and not ':'
    	if ( $ip[0] == ':' ) $ip = "0$ip";
    	// Remove leading zereos from each bloc as needed
    	$ip = preg_replace( '/(^|:)0+' . RE_IPV6_WORD . '/', '$1$2', $ip );
    	return $ip;
	}

	/**
	 * Given an unsigned integer, returns an IPv6 address in octet notation
	 * @param $ip integer IP address.
	 * @return string
	 */
	public static function toOctet( $ip_int ) {
   		// Convert to padded uppercase hex
   		$ip_hex = wfBaseConvert($ip_int, 10, 16, 32, false);
   		// Seperate into 8 octets
   		$ip_oct = substr( $ip_hex, 0, 4 );
   		for ($n=1; $n < 8; $n++) {
   			$ip_oct .= ':' . substr($ip_hex, 4*$n, 4);
   		}
   		// NO leading zeroes
   		$ip_oct = preg_replace( '/(^|:)0+' . RE_IPV6_WORD . '/', '$1$2', $ip_oct );
       	return $ip_oct;
	}

	/**
	 * Convert a network specification in IPv6 CIDR notation to an integer network and a number of bits
	 * @return array(string, int)
	 */
	public static function parseCIDR6( $range ) {
		# Expand any IPv6 IP
		$parts = explode( '/', IP::sanitizeIP( $range ), 2 );
		if ( count( $parts ) != 2 ) {
			return array( false, false );
		}
		$network = self::toUnsigned6( $parts[0] );
		if ( $network !== false && is_numeric( $parts[1] ) && $parts[1] >= 0 && $parts[1] <= 128 ) {
			$bits = $parts[1];
			if ( $bits == 0 ) {
				$network = 0;
			} else {
			# Native 32 bit functions WONT work here!!!
			# Convert to a padded binary number
				$network = wfBaseConvert( $network, 10, 2, 128 );
			# Truncate the last (128-$bits) bits and replace them with zeros
				$network = str_pad( substr( $network, 0, $bits ), 128, 0, STR_PAD_RIGHT );
			# Convert back to an integer
				$network = wfBaseConvert( $network, 2, 10 );
			}
		} else {
			$network = false;
			$bits = false;
		}
		return array( $network, $bits );
	}

	/**
	 * Given a string range in a number of formats, return the start and end of
	 * the range in hexadecimal. For IPv6.
	 *
	 * Formats are:
	 *     2001:0db8:85a3::7344/96          			 CIDR
	 *     2001:0db8:85a3::7344 - 2001:0db8:85a3::7344   Explicit range
	 *     2001:0db8:85a3::7344/96             			 Single IP
	 * @return array(string, int)
	 */
	public static function parseRange6( $range ) {
		# Expand any IPv6 IP
		$range = IP::sanitizeIP( $range );
		if ( strpos( $range, '/' ) !== false ) {
			# CIDR
			list( $network, $bits ) = self::parseCIDR6( $range );
			if ( $network === false ) {
				$start = $end = false;
			} else {
				$start = wfBaseConvert( $network, 10, 16, 32, false );
				# Turn network to binary (again)
				$end = wfBaseConvert( $network, 10, 2, 128 );
				# Truncate the last (128-$bits) bits and replace them with ones
				$end = str_pad( substr( $end, 0, $bits ), 128, 1, STR_PAD_RIGHT );
				# Convert to hex
				$end = wfBaseConvert( $end, 2, 16, 32, false );
				# see toHex() comment
				$start = "v6-$start"; $end = "v6-$end";
			}
		} elseif ( strpos( $range, '-' ) !== false ) {
			# Explicit range
			list( $start, $end ) = array_map( 'trim', explode( '-', $range, 2 ) );
			$start = self::toUnsigned6( $start ); $end = self::toUnsigned6( $end );
			if ( $start > $end ) {
				$start = $end = false;
			} else {
				$start = wfBaseConvert( $start, 10, 16, 32, false );
				$end = wfBaseConvert( $end, 10, 16, 32, false );
			}
			# see toHex() comment
			$start = "v6-$start"; $end = "v6-$end";
		} else {
			# Single IP
			$start = $end = self::toHex( $range );
		}
		if ( $start === false || $end === false ) {
			return array( false, false );
		} else {
			return array( $start, $end );
		}
    }

	/**
	 * Validate an IP address.
	 * @return boolean True if it is valid.
	 */
	public static function isValid( $ip ) {
		return ( preg_match( '/^' . RE_IP_ADD . '$/', $ip) || preg_match( '/^' . RE_IPV6_ADD . '$/', $ip) );
	}

	/**
	 * Validate an IP Block.
	 * @return boolean True if it is valid.
	 */
	public static function isValidBlock( $ipblock ) {
		return ( count(self::toArray($ipblock)) == 1 + 5 );
	}

	/**
	 * Determine if an IP address really is an IP address, and if it is public,
	 * i.e. not RFC 1918 or similar
	 * Comes from ProxyTools.php
	 */
	public static function isPublic( $ip ) {
		$n = self::toUnsigned( $ip );
		if ( !$n ) {
			return false;
		}

		// ip2long accepts incomplete addresses, as well as some addresses
		// followed by garbage characters. Check that it's really valid.
		if( $ip != long2ip( $n ) ) {
			return false;
		}

		static $privateRanges = false;
		if ( !$privateRanges ) {
			$privateRanges = array(
				array( '10.0.0.0',    '10.255.255.255' ),   # RFC 1918 (private)
				array( '172.16.0.0',  '172.31.255.255' ),   #     "
				array( '192.168.0.0', '192.168.255.255' ),  #     "
				array( '0.0.0.0',     '0.255.255.255' ),    # this network
				array( '127.0.0.0',   '127.255.255.255' ),  # loopback
			);
		}

		foreach ( $privateRanges as $r ) {
			$start = self::toUnsigned( $r[0] );
			$end = self::toUnsigned( $r[1] );
			if ( $n >= $start && $n <= $end ) {
				return false;
			}
		}
		return true;
	}

	/**
	 * Split out an IP block as an array of 4 bytes and a mask,
	 * return false if it can't be determined
	 *
	 * @param $ip string A quad dotted/octet IP address
	 * @return array
	 */
	public static function toArray( $ipblock ) {
		$matches = array();
		if( preg_match( '/^' . RE_IP_ADD . '(?:\/(?:'.RE_IP_PREFIX.'))?' . '$/', $ipblock, $matches ) ) {
			return $matches;
		} else if ( preg_match( '/^' . RE_IPV6_ADD . '(?:\/(?:'.RE_IPV6_PREFIX.'))?' . '$/', $ipblock, $matches ) ) {
			return $matches;
		} else {
			return false;
		}
	}

	/**
	 * Return a zero-padded hexadecimal representation of an IP address.
	 *
	 * Hexadecimal addresses are used because they can easily be extended to
	 * IPv6 support. To separate the ranges, the return value from this
	 * function for an IPv6 address will be prefixed with "v6-", a non-
	 * hexadecimal string which sorts after the IPv4 addresses.
	 *
	 * @param $ip Quad dotted/octet IP address.
	 * @return hexidecimal
	 */
	public static function toHex( $ip ) {
		$n = self::toUnsigned( $ip );
		if ( $n !== false ) {
			$n = ( self::isIPv6($ip) ) ? "v6-" . wfBaseConvert( $n, 10, 16, 32, false ) : wfBaseConvert( $n, 10, 16, 8, false );
		}
		return $n;
	}

	/**
	 * Given an IP address in dotted-quad/octet notation, returns an unsigned integer.
	 * Like ip2long() except that it actually works and has a consistent error return value.
	 * Comes from ProxyTools.php
	 * @param $ip Quad dotted IP address.
	 * @return integer
	 */
	public static function toUnsigned( $ip ) {
		// Use IPv6 functions if needed
		if ( self::isIPv6( $ip ) ) {
			return self::toUnsigned6( $ip );
		}
		if ( $ip == '255.255.255.255' ) {
			$n = -1;
		} else {
			$n = ip2long( $ip );
			if ( $n == -1 || $n === false ) { # Return value on error depends on PHP version
				$n = false;
			}
		}
		if ( $n < 0 ) {
			$n += pow( 2, 32 );
		}
		return $n;
	}

	/**
	 * Convert a dotted-quad IP to a signed integer
	 * Returns false on failure
	 */
	public static function toSigned( $ip ) {
		if ( $ip == '255.255.255.255' ) {
			$n = -1;
		} else {
			$n = ip2long( $ip );
			if ( $n == -1 ) {
				$n = false;
			}
		}
		return $n;
	}

	/**
	 * Convert a network specification in CIDR notation to an integer network and a number of bits
	 * @return array(string, int)
	 */
	public static function parseCIDR( $range ) {
		$parts = explode( '/', $range, 2 );
		if ( count( $parts ) != 2 ) {
			return array( false, false );
		}
		$network = self::toSigned( $parts[0] );
		if ( $network !== false && is_numeric( $parts[1] ) && $parts[1] >= 0 && $parts[1] <= 32 ) {
			$bits = $parts[1];
			if ( $bits == 0 ) {
				$network = 0;
			} else {
				$network &= ~((1 << (32 - $bits)) - 1);
			}
			# Convert to unsigned
			if ( $network < 0 ) {
				$network += pow( 2, 32 );
			}
		} else {
			$network = false;
			$bits = false;
		}
		return array( $network, $bits );
	}

	/**
	 * Given a string range in a number of formats, return the start and end of
	 * the range in hexadecimal.
	 *
	 * Formats are:
	 *     1.2.3.4/24          CIDR
	 *     1.2.3.4 - 1.2.3.5   Explicit range
	 *     1.2.3.4             Single IP
	 *
	 *     2001:0db8:85a3::7344/96          			 CIDR
	 *     2001:0db8:85a3::7344 - 2001:0db8:85a3::7344   Explicit range
	 *     2001:0db8:85a3::7344             			 Single IP
	 * @return array(string, int)
	 */
	public static function parseRange( $range ) {
		// Use IPv6 functions if needed
		if ( self::isIPv6( $range ) ) {
			return self::parseRange6( $range );
		}
		if ( strpos( $range, '/' ) !== false ) {
			# CIDR
			list( $network, $bits ) = self::parseCIDR( $range );
			if ( $network === false ) {
				$start = $end = false;
			} else {
				$start = sprintf( '%08X', $network );
				$end = sprintf( '%08X', $network + pow( 2, (32 - $bits) ) - 1 );
			}
		} elseif ( strpos( $range, '-' ) !== false ) {
			# Explicit range
			list( $start, $end ) = array_map( 'trim', explode( '-', $range, 2 ) );
			$start = self::toUnsigned( $start ); $end = self::toUnsigned( $end );
			if ( $start > $end ) {
				$start = $end = false;
			} else {
				$start = sprintf( '%08X', $start );
				$end = sprintf( '%08X', $end );
			}
		} else {
			# Single IP
			$start = $end = self::toHex( $range );
		}
		if ( $start === false || $end === false ) {
			return array( false, false );
		} else {
			return array( $start, $end );
		}
    }

    /**
     * Determine if a given IPv4/IPv6 address is in a given CIDR network
     * @param $addr The address to check against the given range.
     * @param $range The range to check the given address against.
     * @return bool Whether or not the given address is in the given range.
     */
    public static function isInRange( $addr, $range ) {
    // Convert to IPv6 if needed
        $unsignedIP = self::toHex( $addr );
        list( $start, $end ) = self::parseRange( $range );
        return (($unsignedIP >= $start) && ($unsignedIP <= $end));
    }

    /**
     * Convert some unusual representations of IPv4 addresses to their
     * canonical dotted quad representation.
     *
     * This currently only checks a few IPV4-to-IPv6 related cases.  More
     * unusual representations may be added later.
     *
     * @param $addr something that might be an IP address
     * @return valid dotted quad IPv4 address or null
     */
    public static function canonicalize( $addr ) {
		if ( self::isValid( $addr ) )
			return $addr;

		// Annoying IPv6 representations like ::ffff:1.2.3.4
		if ( strpos($addr,':') !==false && strpos($addr,'.') !==false ) {
			$addr = str_replace( '.', ':', $addr );
			if( IP::isIPv6( $addr ) )
				return $addr;
		}

		// IPv6 loopback address
		$m = array();
		if ( preg_match( '/^0*' . RE_IPV6_GAP . '1$/', $addr, $m ) )
	   		return '127.0.0.1';

		// IPv4-mapped and IPv4-compatible IPv6 addresses
		if ( preg_match( '/^' . RE_IPV6_V4_PREFIX . '(' . RE_IP_ADD . ')$/i', $addr, $m ) )
		    return $m[1];
		if ( preg_match( '/^' . RE_IPV6_V4_PREFIX . RE_IPV6_WORD . ':' . RE_IPV6_WORD . '$/i', $addr, $m ) )
		    return long2ip( ( hexdec( $m[1] ) << 16 ) + hexdec( $m[2] ) );

		return null;  // give up
    }
}
