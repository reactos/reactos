<?php
/*  Exif reader v 1.2
    By Richard James Kendall 
    Bugs to richard@richardjameskendall.com 
    Free to use, please acknowledge me 
    
    To use, just include this file (with require, include) and call
    
    exif(filename);
    
    An array called $exif_data will be populated with the exif tags and folders from the image.
*/ 

// holds the formatted data read from the EXIF data area
$exif_data = array();

// holds the number format used in the EXIF data (1 == moto, 0 == intel)
$align = 0;

// holds the lengths and names of the data formats
$format_length = array(0, 1, 1, 2, 4, 8, 1, 1, 2, 4, 8, 4, 8);
$format_type = array("", "BYTE", "STRING", "USHORT", "ULONG", "URATIONAL", "SBYTE", "UNDEFINED", "SSHORT", "SLONG", "SRATIONAL", "SINGLE", "DOUBLE");

// data for EXIF enumeations
$Orientation = array("", "Normal (0 deg)", "Mirrored", "Upsidedown", "Upsidedown & Mirrored", "", "", "");
$ResUnit = array("", "inches", "inches", "cm", "mm", "um");
$YCbCrPos = array("", "Centre of Pixel Array", "Datum Points");
$ExpProg = array("", "Manual", "Program", "Apeture Priority", "Shutter Priority", "Program Creative", "Program Action", "Portrait", "Landscape");
$LightSource = array("Unknown", "Daylight", "Fluorescent", "Tungsten (incandescent)", "Flash", "Fine Weather", "Cloudy Weather", "Share", "Daylight Fluorescent", "Day White Fluorescent", "Cool White Fluorescent", "White Fluorescent", "Standard Light A", "Standard Light B", "Standard Light C", "D55", "D65", "D75", "D50", "ISO Studio Tungsten");
$MeterMode = array("Unknown", "Average", "Centre Weighted", "Spot", "Multi-Spot", "Pattern", "Partial");
$RenderingProcess = array("Normal Process", "Custom Process");
$ExposureMode = array("Auto", "Manual", "Auto Bracket");
$WhiteBalance = array("Auto", "Manual");
$SceneCaptureType = array("Standard", "Landscape", "Portrait", "Night Scene");
$GainControl = array("None", "Low Gain Up", "High Gain Up", "Low Gain Down", "High Gain Down");
$Contrast = array("Normal", "Soft", "Hard");
$Saturation = array("Normal", "Low Saturation", "High Saturation");
$Sharpness = array("Normal", "Soft", "Hard");
$SubjectDistanceRange = array("Unknown", "Macro", "Close View", "Distant View");
$FocalPlaneResUnit = array("", "inches", "inches", "cm", "mm", "um");
$SensingMethod = array("", "Not Defined", "One-chip Colour Area Sensor", "Two-chip Colour Area Sensor", "Three-chip Colour Area Sensor", "Colour Sequential Area Sensor", "Trilinear Sensor", "Colour Sequential Linear Sensor");

// gets one byte from the file at handle $fp and converts it to a number
function fgetord($fp) {
	return ord(fgetc($fp));
}

// takes $data and pads it from the left so strlen($data) == $shouldbe
function pad($data, $shouldbe, $put) {
	if (strlen($data) == $shouldbe) {
		return $data;
	} else {
		$padding = "";
		for ($i = strlen($data);$i < $shouldbe;$i++) {
			$padding .= $put;
		}
		return $padding . $data;
	}
}

// converts a number from intel (little endian) to motorola (big endian format)
function ii2mm($intel) {
	$mm = "";
	for ($i = 0;$i <= strlen($intel);$i+=2) {
		$mm .= substr($intel, (strlen($intel) - $i), 2);
	}
	return $mm;
}

// gets a number from the EXIF data and converts if to the correct representation
function getnumber($data, $start, $length, $align) {
	$a = bin2hex(substr($data, $start, $length));
	if (!$align) {
		$a = ii2mm($a);
	}
	return hexdec($a);
}

// gets a rational number (num, denom) from the EXIF data and produces a decimal
function getrational($data, $align, $type) {
	$a = bin2hex($data);
	if (!$align) {
		$a = ii2mm($a);
	}
	if ($align == 1) {
		$n = hexdec(substr($a, 0, 8));
		$d = hexdec(substr($a, 8, 8));
	} else {
		$d = hexdec(substr($a, 0, 8));
		$n = hexdec(substr($a, 8, 8));
	}
	if ($type == "S" && $n > 2147483647) {
		$n = $n - 4294967296;
	}
	if ($n == 0) {
		return 0;
	}
	if ($d != 0) {
		return ($n / $d);
	} else {
		return $n . "/" . $d;
	}
}

// opens the JPEG file and attempts to find the EXIF data
function exif($file) {
	$fp = fopen($file, "rb");
	$a = fgetord($fp);
	if ($a != 255 || fgetord($fp) != 216) {
		return false;
	}
	$ef = false;
	while (!feof($fp)) {
		$section_length = 0;
		$section_marker = 0;
		$lh = 0;
		$ll = 0;
		for ($i = 0;$i < 7;$i++) {
			$section_marker = fgetord($fp);
			if ($section_marker != 255) {
				break;
			}
			if ($i >= 6) {
				return false;
			}
		}
		if ($section_marker == 255) {
			return false;
		}
		$lh = fgetord($fp);
		$ll = fgetord($fp);
		$section_length = ($lh << 8) | $ll;
		$data = chr($lh) . chr($ll);
		$t_data = fread($fp, $section_length - 2);
		$data .= $t_data;
		switch ($section_marker) {
			case 225:
		    	return extractEXIFData(substr($data, 2), $section_length);
		    	$ef = true;
				break;
		}
	}
	fclose($fp);
}

// reads the EXIF header and if it is intact it calls readEXIFDir to get the data
function extractEXIFData($data, $length) {
	global $align;
	if (substr($data, 0, 4) == "Exif") {
		if (substr($data, 6, 2) == "II") {
			$align = 0;
		} else {
			if (substr($data, 6, 2) == "MM") {
				$align = 1;
			} else {
				return false;
			}
		}
		$a = getnumber($data, 8, 2, $align);
		if ($a != 0x2a) {
			return false;
		}
		$first_offset = getnumber($data, 10, 4, $align);
		if ($first_offset < 8 || $first_offset > 16) {
			return false;
		}
		readEXIFDir(substr($data, 14), 8, $length - 6);
		return true;
	} else {
		return false;
	}
}

// takes an EXIF tag id and returns the string name of that tag
function tagid2name($id) {
	switch ($id) {
		case 0x000b: return "ACDComment"; break;
		case 0x00fe: return "ImageType"; break;
		case 0x0106: return "PhotometicInterpret"; break;
		case 0x010e: return "ImageDescription"; break;
		case 0x010f: return "Make"; break;
		case 0x0110: return "Model"; break;
		case 0x0112: return "Orientation"; break;
		case 0x0115: return "SamplesPerPixel"; break;
		case 0x011a: return "XRes"; break;
		case 0x011b: return "YRes"; break;
		case 0x011c: return "PlanarConfig"; break;
		case 0x0128: return "ResUnit"; break;
		case 0x0131: return "Software"; break;
		case 0x0132: return "DateTime"; break;
		case 0x013b: return "Artist"; break;
		case 0x013f: return "WhitePoint"; break;
		case 0x0211: return "YCbCrCoefficients"; break;
		case 0x0213: return "YCbCrPos"; break;
		case 0x0214: return "RefBlackWhite"; break;
		case 0x8298: return "Copyright"; break;
		case 0x829a: return "ExposureTime"; break;
		case 0x829d: return "FNumber"; break;
		case 0x8822: return "ExpProg"; break;
		case 0x8827: return "ISOSpeedRating"; break;
		case 0x9003: return "DTOpticalCapture"; break;
		case 0x9004: return "DTDigitised"; break;
		case 0x9102: return "CompressedBitsPerPixel"; break;
		case 0x9201: return "ShutterSpeed"; break;
		case 0x9202: return "ApertureWidth"; break;
		case 0x9203: return "Brightness"; break;
		case 0x9204: return "ExposureBias"; break;
		case 0x9205: return "MaxApetureWidth"; break;
		case 0x9206: return "SubjectDistance"; break;
		case 0x9207: return "MeterMode"; break;
		case 0x9208: return "LightSource"; break;
		case 0x9209: return "Flash"; break;
		case 0x920a: return "FocalLength"; break;
		case 0x9213: return "ImageHistory"; break;
		case 0x927c: return "MakerNote"; break;
		case 0x9286: return "UserComment"; break;
		case 0x9290: return "SubsecTime"; break;
		case 0x9291: return "SubsecTimeOrig"; break;
		case 0x9292: return "SubsecTimeDigi"; break;
		case 0xa000: return "FlashPixVersion"; break;
		case 0xa001: return "ColourSpace"; break;
		case 0xa002: return "ImageWidth"; break;
		case 0xa003: return "ImageHeight"; break;
		case 0xa20e: return "FocalPlaneXRes"; break;
		case 0xa20f: return "FocalPlaneYRes"; break;
		case 0xa210: return "FocalPlaneResUnit"; break;
		case 0xa217: return "SensingMethod"; break;
		case 0xa300: return "ImageSource"; break;
		case 0xa301: return "SceneType"; break;
		case 0xa401: return "RenderingProcess"; break;
		case 0xa402: return "ExposureMode"; break;
		case 0xa403: return "WhiteBalance"; break;
		case 0xa404: return "DigitalZoomRatio"; break;
		case 0xa405: return "FocalLength35mm"; break;
		case 0xa406: return "SceneCaptureType"; break;
		case 0xa407: return "GainControl"; break;
		case 0xa408: return "Contrast"; break;
		case 0xa409: return "Saturation"; break;
		case 0xa40a: return "Sharpness"; break;
		case 0xa40c: return "SubjectDistanceRange"; break;
	}
}

// takes a (U/S)(SHORT/LONG) checks if an enumeration for this value exists and if it does returns the enumerated value for $tvalue
function enumvalue($tname, $tvalue) {
	global $Orientation, $ResUnit, $YCbCrPos, $ExpProg, $MeterMode, $LightSource, $RenderingProcess, $ExposureMode, $WhiteBalance, $SceneCaptureType;
	global $GainControl, $Contrast, $Saturation, $Sharpness, $SubjectDistanceRange, $FocalPlaneResUnit, $SensingMethod;
	if (isset($$tname)) {
		$tmp = $$tname;
		return $tmp[$tvalue];
	} else {
		return $tvalue;
	}
}

// takes the USHORT of the flash value, splits it up into itc component bits and returns the string it represents
function flashvalue($bin) {
	$retval = "";
	$bin = pad(decbin($bin), 8, "0");
	$flashfired = substr($bin, 7, 1);
	$returnd = substr($bin, 5, 2);
	$flashmode = substr($bin, 3, 2);
	$redeye = substr($bin, 1, 1);
	if ($flashfired == "1") {
		$reval = "Fired";
	} else {
		if ($flashfired == "0") {
			$retval = "Did not fire";
		}
	}
	if ($returnd == "10") {
		$retval .= ", Strobe return light not detected";
	} else {
		if ($returnd == "11") {
			$retval .= ", Strobe return light detected";
		}
	}
	if ($flashmode == "01" || $flashmode == "10") {
		$retval .= ", Compulsory mode";
	} else {
		if ($flashmode == "11") {
			$retval .= ", Auto mode";
		}
	}
	if ($redeye) {
		$retval .= ", Red eye reduction";
	} else {
		$retval .= ", No red eye reduction";
	}
	return $retval;
}

// takes a tag id along with the format, data and length of the data and deals with it appropriatly
function dealwithtag($tag, $format, $data, $length, $align) {
	global $format_type, $exif_data;
	$w = false;
	$val = "";
	switch ($format_type[$format]) {
		case "STRING":
			$val = trim(substr($data, 0, $length));
			$w = true;
			break;
		case "ULONG":
		case "SLONG":
			$val = enumvalue(tagid2name($tag), getnumber($data, 0, 4, $align));
			$w = true;
			break;
		case "USHORT":
		case "SSHORT":
			switch ($tag) {
				case 0x9209:
					$val = array(getnumber($data, 0, 2, $align), flashvalue(getnumber($data, 0, 2, $align)));
					$w = true;
					break;
				case 0x9214:
					
					break;
				case 0xa001:
					$tmp = getnumber($data, 0, 2, $align);
					if ($tmp == 1) {
						$val = "sRGB";
						$w = true;
					} else {
						$val = "Uncalibrated";
						$w = true;
					}
					break;
				default:
					$val = enumvalue(tagid2name($tag), getnumber($data, 0, 2, $align));
					$w = true;
					break;
			} 
			break;
		case "URATIONAL":
			$val = getrational(substr($data, 0, 8), $align, "U");
			$w = true;
			break;
		case "SRATIONAL":
			$val = getrational(substr($data, 0, 8), $align, "S");
			$w = true;
			break;
		case "UNDEFINED":
			switch ($tag) {
				case 0xa300:
					$tmp = getnumber($data, 0, 2, $align);
					if ($tmp == 3) {
						$val = "Digital Camera";
						$w = true;
					} else {
						$val = "Unknown";
						$w = true;
					}
					break;
				case 0xa301:
					$tmp = getnumber($data, 0, 2, $align);
					if ($tmp == 3) {
						$val = "Directly Photographed";
						$w = true;
					} else {
						$val = "Unknown";
						$w = true;
					}
					break;
			}
			break;
	}
	if ($w) {
		$exif_data[tagid2name($tag)] = $val;
	}
}

// reads the tags from and EXIF IFD and if correct deals with the data
function readEXIFDir($data, $offset_base, $exif_length) {
	global $format_length, $format_type, $align;
	$value_ptr = 0;
	$sofar = 2;
	$data_in = "";
	$number_dir_entries = getnumber($data, 0, 2, $align);
	for ($i = 0;$i < $number_dir_entries;$i++) {
		$sofar += 12;
		$dir_entry = substr($data, 2 + 12 * $i);
		$tag = getnumber($dir_entry, 0, 2, $align);
		$format = getnumber($dir_entry, 2, 2, $align);
		$components = getnumber($dir_entry, 4, 4, $align);
		if (($format - 1) >= 12) {
			return false;
		}
		$byte_count = $components * $format_length[$format];
		if ($byte_count > 4) {
			$offset_val = (getnumber($dir_entry, 8, 4, $align)) - $offset_base;
			if (($offset_val + $byte_count) > $exif_length) {
				return false;
			}
			$data_in = substr($data, $offset_val);
		} else {
			$data_in = substr($dir_entry, 8);
		}
		if ($tag == 0x8769) {
			$tmp = (getnumber($data_in, 0, 4, $align)) - 8;
			readEXIFDir(substr($data, $tmp), $tmp + 8 , $exif_length);
		} else {
			dealwithtag($tag, $format, $data_in, $byte_count, $align);
		}
	}
}
?>