<?php
/**
 * MacBinary signature checker and data fork extractor, for files
 * uploaded from Internet Explorer for Mac.
 *
 * Copyright (C) 2005 Brion Vibber <brion@pobox.com>
 * Portions based on Convert::BinHex by Eryq et al
 * http://www.mediawiki.org/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @ingroup SpecialPage
 */

class MacBinary {
	function __construct( $filename ) {
		$this->open( $filename );
		$this->loadHeader();
	}

	/**
	 * The file must be seekable, such as local filesystem.
	 * Remote URLs probably won't work.
	 *
	 * @param string $filename
	 */
	function open( $filename ) {
		$this->valid = false;
		$this->version = 0;
		$this->filename = '';
		$this->dataLength = 0;
		$this->resourceLength = 0;
		$this->handle = fopen( $filename, 'rb' );
	}

	/**
	 * Does this appear to be a valid MacBinary archive?
	 * @return bool
	 */
	function isValid() {
		return $this->valid;
	}

	/**
	 * Get length of data fork
	 * @return int
	 */
	function dataForkLength() {
		return $this->dataLength;
	}

	/**
	 * Copy the data fork to an external file or resource.
	 * @param resource $destination
	 * @return bool
	 */
	function extractData( $destination ) {
		if( !$this->isValid() ) {
			return false;
		}

		// Data fork appears immediately after header
		fseek( $this->handle, 128 );
		return $this->copyBytesTo( $destination, $this->dataLength );
	}

	/**
	 *
	 */
	function close() {
		fclose( $this->handle );
	}

	// --------------------------------------------------------------

	/**
	 * Check if the given file appears to be MacBinary-encoded,
	 * as Internet Explorer on Mac OS may provide for unknown types.
	 * http://www.lazerware.com/formats/macbinary/macbinary_iii.html
	 * If ok, load header data.
	 *
	 * @return bool
	 * @access private
	 */
	function loadHeader() {
		$fname = 'MacBinary::loadHeader';

		fseek( $this->handle, 0 );
		$head = fread( $this->handle, 128 );
		#$this->hexdump( $head );

		if( strlen( $head ) < 128 ) {
			wfDebug( "$fname: couldn't read full MacBinary header\n" );
			return false;
		}

		if( $head{0} != "\x00" || $head{74} != "\x00" ) {
			wfDebug( "$fname: header bytes 0 and 74 not null\n" );
			return false;
		}

		$signature = substr( $head, 102, 4 );
		$a = unpack( "ncrc", substr( $head, 124, 2 ) );
		$storedCRC = $a['crc'];
		$calculatedCRC = $this->calcCRC( substr( $head, 0, 124 ) );
		if( $storedCRC == $calculatedCRC ) {
			if( $signature == 'mBIN' ) {
				$this->version = 3;
			} else {
				$this->version = 2;
			}
		} else {
			$crc = sprintf( "%x != %x", $storedCRC, $calculatedCRC );
			if( $storedCRC == 0 && $head{82} == "\x00" &&
				substr( $head, 101, 24 ) == str_repeat( "\x00", 24 ) ) {
				wfDebug( "$fname: no CRC, looks like MacBinary I\n" );
				$this->version = 1;
			} elseif( $signature == 'mBIN' && $storedCRC == 0x185 ) {
				// Mac IE 5.0 seems to insert this value in the CRC field.
				// 5.2.3 works correctly; don't know about other versions.
				wfDebug( "$fname: CRC doesn't match ($crc), looks like Mac IE 5.0\n" );
				$this->version = 3;
			} else {
				wfDebug( "$fname: CRC doesn't match ($crc) and not MacBinary I\n" );
				return false;
			}
		}

		$nameLength = ord( $head{1} );
		if( $nameLength < 1 || $nameLength > 63 ) {
			wfDebug( "$fname: invalid filename size $nameLength\n" );
			return false;
		}
		$this->filename = substr( $head, 2, $nameLength );

		$forks = unpack( "Ndata/Nresource", substr( $head, 83, 8 ) );
		$this->dataLength = $forks['data'];
		$this->resourceLength = $forks['resource'];
		$maxForkLength = 0x7fffff;

		if( $this->dataLength < 0 || $this->dataLength > $maxForkLength ) {
			wfDebug( "$fname: invalid data fork length $this->dataLength\n" );
			return false;
		}

		if( $this->resourceLength < 0 || $this->resourceLength > $maxForkLength ) {
			wfDebug( "$fname: invalid resource fork size $this->resourceLength\n" );
			return false;
		}

		wfDebug( "$fname: appears to be MacBinary $this->version, data length $this->dataLength\n" );
		$this->valid = true;
		return true;
	}

	/**
	 * Calculate a 16-bit CRC value as for MacBinary headers.
	 * Adapted from perl5 Convert::BinHex by Eryq,
	 * based on the mcvert utility (Doug Moore, April '87),
	 * with magic array thingy by Jim Van Verth.
	 * http://search.cpan.org/~eryq/Convert-BinHex-1.119/lib/Convert/BinHex.pm
	 *
	 * @param string $data
	 * @param int $seed
	 * @return int
	 * @access private
	 */
	function calcCRC( $data, $seed = 0 ) {
		# An array useful for CRC calculations that use 0x1021 as the "seed":
		$MAGIC = array(
			0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
			0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
			0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
			0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
			0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
			0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
			0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
			0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
			0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
			0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
			0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
			0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
			0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
			0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
			0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
			0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
			0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
			0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
			0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
			0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
			0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
			0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
			0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
			0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
			0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
			0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
			0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
			0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
			0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
			0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
			0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
			0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
		);
		$len = strlen( $data );
		$crc = $seed;
		for( $i = 0; $i < $len; $i++ ) {
			$crc ^= ord( $data{$i} ) << 8;
			$crc &= 0xFFFF;
			$crc = ($crc << 8) ^ $MAGIC[$crc >> 8];
			$crc &= 0xFFFF;
		}
		return $crc;
	}

	/**
	 * @param resource $destination
	 * @param int $bytesToCopy
	 * @return bool
	 * @access private
	 */
	function copyBytesTo( $destination, $bytesToCopy ) {
		$bufferSize = 65536;
		for( $remaining = $bytesToCopy; $remaining > 0; $remaining -= $bufferSize ) {
			$thisChunkSize = min( $remaining, $bufferSize );
			$buffer = fread( $this->handle, $thisChunkSize );
			fwrite( $destination, $buffer );
		}
	}

	/**
	 * Hex dump of the header for debugging
	 * @access private
	 */
	function hexdump( $data ) {
		global $wgDebugLogFile;
		if( !$wgDebugLogFile ) return;

		$width = 16;
		$at = 0;
		for( $remaining = strlen( $data ); $remaining > 0; $remaining -= $width ) {
			$line = sprintf( "%04x:", $at );
			$printable = '';
			for( $i = 0; $i < $width && $remaining - $i > 0; $i++ ) {
				$byte = ord( $data{$at++} );
				$line .= sprintf( " %02x", $byte );
				$printable .= ($byte >= 32 && $byte <= 126 )
					? chr( $byte )
					: '.';
			}
			if( $i < $width ) {
				$line .= str_repeat( '   ', $width - $i );
			}
			wfDebug( "MacBinary: $line $printable\n" );
		}
	}
}
