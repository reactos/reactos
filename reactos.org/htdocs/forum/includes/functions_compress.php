<?php
/**
*
* @package phpBB3
* @version $Id: functions_compress.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* @ignore
*/
if (!defined('IN_PHPBB'))
{
	exit;
}

/**
* Class for handling archives (compression/decompression)
* @package phpBB3
*/
class compress
{
	var $fp = 0;

	/**
	* Add file to archive
	*/
	function add_file($src, $src_rm_prefix = '', $src_add_prefix = '', $skip_files = '')
	{
		global $phpbb_root_path;

		$skip_files = explode(',', $skip_files);

		// Remove rm prefix from src path
		$src_path = ($src_rm_prefix) ? preg_replace('#^(' . preg_quote($src_rm_prefix, '#') . ')#', '', $src) : $src;
		// Add src prefix
		$src_path = ($src_add_prefix) ? ($src_add_prefix . ((substr($src_add_prefix, -1) != '/') ? '/' : '') . $src_path) : $src_path;
		// Remove initial "/" if present
		$src_path = (substr($src_path, 0, 1) == '/') ? substr($src_path, 1) : $src_path;

		if (is_file($phpbb_root_path . $src))
		{
			$this->data($src_path, file_get_contents("$phpbb_root_path$src"), false, stat("$phpbb_root_path$src"));
		}
		else if (is_dir($phpbb_root_path . $src))
		{
			// Clean up path, add closing / if not present
			$src_path = ($src_path && substr($src_path, -1) != '/') ? $src_path . '/' : $src_path;

			$filelist = array();
			$filelist = filelist("$phpbb_root_path$src", '', '*');
			krsort($filelist);

			if ($src_path)
			{
				$this->data($src_path, '', true, stat("$phpbb_root_path$src"));
			}

			foreach ($filelist as $path => $file_ary)
			{
				if ($path)
				{
					// Same as for src_path
					$path = (substr($path, 0, 1) == '/') ? substr($path, 1) : $path;
					$path = ($path && substr($path, -1) != '/') ? $path . '/' : $path;

					$this->data("$src_path$path", '', true, stat("$phpbb_root_path$src$path"));
				}

				foreach ($file_ary as $file)
				{
					if (in_array($path . $file, $skip_files))
					{
						continue;
					}

					$this->data("$src_path$path$file", file_get_contents("$phpbb_root_path$src$path$file"), false, stat("$phpbb_root_path$src$path$file"));
				}
			}
		}

		return true;
	}

	/**
	* Add custom file (the filepath will not be adjusted)
	*/
	function add_custom_file($src, $filename)
	{
		$this->data($filename, file_get_contents($src), false, stat($src));
		return true;
	}

	/**
	* Add file data
	*/
	function add_data($src, $name)
	{
		$stat = array();
		$stat[2] = 436; //384
		$stat[4] = $stat[5] = 0;
		$stat[7] = strlen($src);
		$stat[9] = time();
		$this->data($name, $src, false, $stat);
		return true;
	}

	/**
	* Return available methods
	*/
	function methods()
	{
		$methods = array('.tar');
		$available_methods = array('.tar.gz' => 'zlib', '.tar.bz2' => 'bz2', '.zip' => 'zlib');

		foreach ($available_methods as $type => $module)
		{
			if (!@extension_loaded($module))
			{
				continue;
			}
			$methods[] = $type;
		}

		return $methods;
	}
}

/**
* Zip creation class from phpMyAdmin 2.3.0 (c) Tobias Ratschiller, Olivier Müller, Loïc Chapeaux,
* Marc Delisle, http://www.phpmyadmin.net/
*
* Zip extraction function by Alexandre Tedeschi, alexandrebr at gmail dot com
*
* Modified extensively by psoTFX and DavidMJ, (c) phpBB Group, 2003
*
* Based on work by Eric Mueller and Denis125
* Official ZIP file format: http://www.pkware.com/appnote.txt
*
* @package phpBB3
*/
class compress_zip extends compress
{
	var $datasec = array();
	var $ctrl_dir = array();
	var $eof_cdh = "\x50\x4b\x05\x06\x00\x00\x00\x00";

	var $old_offset = 0;
	var $datasec_len = 0;

	/**
	* Constructor
	*/
	function compress_zip($mode, $file)
	{
		return $this->fp = @fopen($file, $mode . 'b');
	}

	/**
	* Convert unix to dos time
	*/
	function unix_to_dos_time($time)
	{
		$timearray = (!$time) ? getdate() : getdate($time);

		if ($timearray['year'] < 1980)
		{
			$timearray['year'] = 1980;
			$timearray['mon'] = $timearray['mday'] = 1;
			$timearray['hours'] = $timearray['minutes'] = $timearray['seconds'] = 0;
		}

		return (($timearray['year'] - 1980) << 25) | ($timearray['mon'] << 21) | ($timearray['mday'] << 16) | ($timearray['hours'] << 11) | ($timearray['minutes'] << 5) | ($timearray['seconds'] >> 1);
	}

	/**
	* Extract archive
	*/
	function extract($dst)
	{		
		// Loop the file, looking for files and folders
		$dd_try = false;
		rewind($this->fp);

		while (!feof($this->fp))
		{
			// Check if the signature is valid...
			$signature = fread($this->fp, 4);

			switch ($signature)
			{
				// 'Local File Header'
				case "\x50\x4b\x03\x04":
					// Lets get everything we need.
					// We don't store the version needed to extract, the general purpose bit flag or the date and time fields
					$data = unpack("@4/vc_method/@10/Vcrc/Vc_size/Vuc_size/vname_len/vextra_field", fread($this->fp, 26));
					$file_name = fread($this->fp, $data['name_len']); // filename

					if ($data['extra_field'])
					{
						fread($this->fp, $data['extra_field']); // extra field
					}

					$target_filename = "$dst$file_name";

					if (!$data['uc_size'] && !$data['crc'] && substr($file_name, -1, 1) == '/')
					{
						if (!is_dir($target_filename))
						{
							$str = '';
							$folders = explode('/', $target_filename);

							// Create and folders and subfolders if they do not exist
							foreach ($folders as $folder)
							{
								$str = (!empty($str)) ? $str . '/' . $folder : $folder;
								if (!is_dir($str))
								{
									if (!@mkdir($str, 0777))
									{
										trigger_error("Could not create directory $folder");
									}
									@chmod($str, 0777);
								}
							}
						}
						// This is a directory, we are not writting files
						continue;
					}
					else
					{
						// Some archivers are punks, they don't don't include folders in their archives!
						$str = '';
						$folders = explode('/', pathinfo($target_filename, PATHINFO_DIRNAME));

						// Create and folders and subfolders if they do not exist
						foreach ($folders as $folder)
						{
							$str = (!empty($str)) ? $str . '/' . $folder : $folder;
							if (!is_dir($str))
							{
								if (!@mkdir($str, 0777))
								{
									trigger_error("Could not create directory $folder");
								}
								@chmod($str, 0777);
							}
						}
					}

					if (!$data['uc_size'])
					{
						$content = '';
					}
					else
					{
						$content = fread($this->fp, $data['c_size']);
					}

					$fp = fopen($target_filename, "w");

					switch ($data['c_method'])
					{
						case 0:
							// Not compressed
							fwrite($fp, $content);
						break;
					
						case 8:
							// Deflate
							fwrite($fp, gzinflate($content, $data['uc_size']));
						break;

						case 12:
							// Bzip2
							fwrite($fp, bzdecompress($content));
						break;
					}
					
					fclose($fp);
				break;

				// We hit the 'Central Directory Header', we can stop because nothing else in here requires our attention
				// or we hit the end of the central directory record, we can safely end the loop as we are totally finished with looking for files and folders
				case "\x50\x4b\x01\x02":
				// This case should simply never happen.. but it does exist..
				case "\x50\x4b\x05\x06":
				break 2;
				
				// 'Packed to Removable Disk', ignore it and look for the next signature...
				case 'PK00':
				continue 2;
				
				// We have encountered a header that is weird. Lets look for better data...
				default:
					if (!$dd_try)
					{
						// Unexpected header. Trying to detect wrong placed 'Data Descriptor';
						$dd_try = true;
						fseek($this->fp, 8, SEEK_CUR); // Jump over 'crc-32'(4) 'compressed-size'(4), 'uncompressed-size'(4)
						continue 2;
					}
					trigger_error("Unexpected header, ending loop");
				break 2;
			}

			$dd_try = false;
		}
	}

	/**
	* Close archive
	*/
	function close()
	{
		// Write out central file directory and footer ... if it exists
		if (sizeof($this->ctrl_dir))
		{
			fwrite($this->fp, $this->file());
		}
		fclose($this->fp);
	}

	/**
	* Create the structures ... note we assume version made by is MSDOS
	*/
	function data($name, $data, $is_dir = false, $stat)
	{
		$name = str_replace('\\', '/', $name);

		$hexdtime = pack('V', $this->unix_to_dos_time($stat[9]));

		if ($is_dir)
		{
			$unc_len = $c_len = $crc = 0;
			$zdata = '';
			$var_ext = 10;
		}
		else
		{
			$unc_len = strlen($data);
			$crc = crc32($data);
			$zdata = gzdeflate($data);
			$c_len = strlen($zdata);
			$var_ext = 20;

			// Did we compress? No, then use data as is
			if ($c_len >= $unc_len)
			{
				$zdata = $data;
				$c_len = $unc_len;
				$var_ext = 10;
			}
		}
		unset($data);

		// If we didn't compress set method to store, else deflate
		$c_method = ($c_len == $unc_len) ? "\x00\x00" : "\x08\x00";

		// Are we a file or a directory? Set archive for file
		$attrib = ($is_dir) ? 16 : 32;

		// File Record Header
		$fr = "\x50\x4b\x03\x04";		// Local file header 4bytes
		$fr .= pack('v', $var_ext);		// ver needed to extract 2bytes
		$fr .= "\x00\x00";				// gen purpose bit flag 2bytes
		$fr .= $c_method;				// compression method 2bytes
		$fr .= $hexdtime;				// last mod time and date 2+2bytes
		$fr .= pack('V', $crc);			// crc32 4bytes
		$fr .= pack('V', $c_len);		// compressed filesize 4bytes
		$fr .= pack('V', $unc_len);		// uncompressed filesize 4bytes
		$fr .= pack('v', strlen($name));// length of filename 2bytes

		$fr .= pack('v', 0);			// extra field length 2bytes
		$fr .= $name;
		$fr .= $zdata;
		unset($zdata);

		$this->datasec_len += strlen($fr);

		// Add data to file ... by writing data out incrementally we save some memory
		fwrite($this->fp, $fr);
		unset($fr);

		// Central Directory Header
		$cdrec = "\x50\x4b\x01\x02";		// header 4bytes
		$cdrec .= "\x00\x00";				// version made by
		$cdrec .= pack('v', $var_ext);		// version needed to extract
		$cdrec .= "\x00\x00";				// gen purpose bit flag
		$cdrec .= $c_method;				// compression method
		$cdrec .= $hexdtime;				// last mod time & date
		$cdrec .= pack('V', $crc);			// crc32
		$cdrec .= pack('V', $c_len);		// compressed filesize
		$cdrec .= pack('V', $unc_len);		// uncompressed filesize
		$cdrec .= pack('v', strlen($name));	// length of filename
		$cdrec .= pack('v', 0);				// extra field length
		$cdrec .= pack('v', 0);				// file comment length
		$cdrec .= pack('v', 0);				// disk number start
		$cdrec .= pack('v', 0);				// internal file attributes
		$cdrec .= pack('V', $attrib);		// external file attributes
		$cdrec .= pack('V', $this->old_offset);	// relative offset of local header
		$cdrec .= $name;

		// Save to central directory
		$this->ctrl_dir[] = $cdrec;

		$this->old_offset = $this->datasec_len;
	}

	/**
	* file
	*/
	function file()
	{
		$ctrldir = implode('', $this->ctrl_dir);

		return $ctrldir . $this->eof_cdh .
			pack('v', sizeof($this->ctrl_dir)) .	// total # of entries "on this disk"
			pack('v', sizeof($this->ctrl_dir)) .	// total # of entries overall
			pack('V', strlen($ctrldir)) .			// size of central dir
			pack('V', $this->datasec_len) .			// offset to start of central dir
			"\x00\x00";								// .zip file comment length
	}

	/**
	* Download archive
	*/
	function download($filename, $download_name = false)
	{
		global $phpbb_root_path;

		if ($download_name === false)
		{
			$download_name = $filename;
		}

		$mimetype = 'application/zip';

		header('Pragma: no-cache');
		header("Content-Type: $mimetype; name=\"$download_name.zip\"");
		header("Content-disposition: attachment; filename=$download_name.zip");

		$fp = @fopen("{$phpbb_root_path}store/$filename.zip", 'rb');
		if ($fp)
		{
			while ($buffer = fread($fp, 1024))
			{
				echo $buffer;
			}
			fclose($fp);
		}
	}
}

/**
* Tar/tar.gz compression routine
* Header/checksum creation derived from tarfile.pl, (c) Tom Horsley, 1994
*
* @package phpBB3
*/
class compress_tar extends compress
{
	var $isgz = false;
	var $isbz = false;
	var $filename = '';
	var $mode = '';
	var $type = '';
	var $wrote = false;

	/**
	* Constructor
	*/
	function compress_tar($mode, $file, $type = '')
	{
		$type = (!$type) ? $file : $type;
		$this->isgz = (strpos($type, '.tar.gz') !== false || strpos($type, '.tgz') !== false) ? true : false;
		$this->isbz = (strpos($type, '.tar.bz2') !== false) ? true : false;

		$this->mode = &$mode;
		$this->file = &$file;
		$this->type = &$type;
		$this->open();
	}

	/**
	* Extract archive
	*/
	function extract($dst)
	{
		$fzread = ($this->isbz && function_exists('bzread')) ? 'bzread' : (($this->isgz && @extension_loaded('zlib')) ? 'gzread' : 'fread');

		// Run through the file and grab directory entries
		while ($buffer = $fzread($this->fp, 512))
		{
			$tmp = unpack('A6magic', substr($buffer, 257, 6));

			if (trim($tmp['magic']) == 'ustar')
			{
				$tmp = unpack('A100name', $buffer);
				$filename = trim($tmp['name']);

				$tmp = unpack('Atype', substr($buffer, 156, 1));
				$filetype = (int) trim($tmp['type']);

				$tmp = unpack('A12size', substr($buffer, 124, 12));
				$filesize = octdec((int) trim($tmp['size']));

				if ($filetype == 5)
				{
					if (!is_dir("$dst$filename"))
					{
						$str = '';
						$folders = explode('/', "$dst$filename");

						// Create and folders and subfolders if they do not exist
						foreach ($folders as $folder)
						{
							$str = (!empty($str)) ? $str . '/' . $folder : $folder;
							if (!is_dir($str))
							{
								if (!@mkdir($str, 0777))
								{
									trigger_error("Could not create directory $folder");
								}
								@chmod($str, 0777);
							}
						}
					}
				}
				else if ($filesize != 0 && ($filetype == 0 || $filetype == "\0"))
				{
					// Write out the files
					if (!($fp = fopen("$dst$filename", 'wb')))
					{
						trigger_error("Couldn't create file $filename");
					}
					@chmod("$dst$filename", 0777);

					// Grab the file contents
					fwrite($fp, $fzread($this->fp, ($filesize + 511) &~ 511), $filesize);
					fclose($fp);
				}
			}
		}
	}

	/**
	* Close archive
	*/
	function close()
	{
		$fzclose = ($this->isbz && function_exists('bzclose')) ? 'bzclose' : (($this->isgz && @extension_loaded('zlib')) ? 'gzclose' : 'fclose');

		if ($this->wrote)
		{
			$fzwrite = ($this->isbz && function_exists('bzwrite')) ? 'bzwrite' : (($this->isgz && @extension_loaded('zlib')) ? 'gzwrite' : 'fwrite');

			// The end of a tar archive ends in two records of all NULLs (1024 bytes of \0)
			$fzwrite($this->fp, str_repeat("\0", 1024));
		}

		$fzclose($this->fp);
	}

	/**
	* Create the structures
	*/
	function data($name, $data, $is_dir = false, $stat)
	{
		$this->wrote = true;
		$fzwrite = 	($this->isbz && function_exists('bzwrite')) ? 'bzwrite' : (($this->isgz && @extension_loaded('zlib')) ? 'gzwrite' : 'fwrite');

		$typeflag = ($is_dir) ? '5' : '';

		// This is the header data, it contains all the info we know about the file or folder that we are about to archive
		$header = '';
		$header .= pack('a100', $name);						// file name
		$header .= pack('a8', sprintf("%07o", $stat[2]));	// file mode
		$header .= pack('a8', sprintf("%07o", $stat[4]));	// owner id
		$header .= pack('a8', sprintf("%07o", $stat[5]));	// group id
		$header .= pack('a12', sprintf("%011o", $stat[7]));	// file size
		$header .= pack('a12', sprintf("%011o", $stat[9]));	// last mod time

		// Checksum
		$checksum = 0;
		for ($i = 0; $i < 148; $i++)
		{
			$checksum += ord($header[$i]);
		}

		// We precompute the rest of the hash, this saves us time in the loop and allows us to insert our hash without resorting to string functions
		$checksum += 2415 + (($is_dir) ? 53 : 0);

		$header .= pack('a8', sprintf("%07o", $checksum));	// checksum
		$header .= pack('a1', $typeflag);					// link indicator
		$header .= pack('a100', '');						// name of linked file
		$header .= pack('a6', 'ustar');						// ustar indicator
		$header .= pack('a2', '00');						// ustar version
		$header .= pack('a32', 'Unknown');					// owner name
		$header .= pack('a32', 'Unknown');					// group name
		$header .= pack('a8', '');							// device major number
		$header .= pack('a8', '');							// device minor number
		$header .= pack('a155', '');						// filename prefix
		$header .= pack('a12', '');							// end

		// This writes the entire file in one shot. Header, followed by data and then null padded to a multiple of 512
		$fzwrite($this->fp, $header . (($stat[7] !== 0 && !$is_dir) ? $data . str_repeat("\0", (($stat[7] + 511) &~ 511) - $stat[7]) : ''));
		unset($data);
	}

	/**
	* Open archive
	*/
	function open()
	{
		$fzopen = ($this->isbz && function_exists('bzopen')) ? 'bzopen' : (($this->isgz && @extension_loaded('zlib')) ? 'gzopen' : 'fopen');
		$this->fp = @$fzopen($this->file, $this->mode . (($fzopen == 'bzopen') ? '' : 'b') . (($fzopen == 'gzopen') ? '9' : ''));

		if (!$this->fp)
		{
			trigger_error('Unable to open file ' . $this->file . ' [' . $fzopen . ' - ' . $this->mode . 'b]');
		}
	}

	/**
	* Download archive
	*/
	function download($filename, $download_name = false)
	{
		global $phpbb_root_path;

		if ($download_name === false)
		{
			$download_name = $filename;
		}

		switch ($this->type)
		{
			case '.tar':
				$mimetype = 'application/x-tar';
			break;

			case '.tar.gz':
				$mimetype = 'application/x-gzip';
			break;

			case '.tar.bz2':
				$mimetype = 'application/x-bzip2';
			break;

			default:
				$mimetype = 'application/octet-stream';
			break;
		}

		header('Pragma: no-cache');
		header("Content-Type: $mimetype; name=\"$download_name$this->type\"");
		header("Content-disposition: attachment; filename=$download_name$this->type");

		$fp = @fopen("{$phpbb_root_path}store/$filename$this->type", 'rb');
		if ($fp)
		{
			while ($buffer = fread($fp, 1024))
			{
				echo $buffer;
			}
			fclose($fp);
		}
	}
}

?>