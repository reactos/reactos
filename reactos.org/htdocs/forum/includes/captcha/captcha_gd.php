<?php
/**
*
* @package VC
* @version $Id: captcha_gd.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2006 phpBB Group
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
* Original Author - Xore (Robert Hetzler)
* With contributions from Neothermic
*
* @package VC
*/
class captcha
{
	var $width = 360;
	var $height = 96;

	/**
	* Create the image containing $code with a seed of $seed
	*/
	function execute($code, $seed)
	{
		global $config;
		srand($seed);
		mt_srand($seed);

		// Create image
		$img = imagecreatetruecolor($this->width, $this->height);

		// Generate colours
		$colour = new colour_manager($img, array(
			'random'	=> true,
			'min_value'	=> 60,
		), 'hsv');

		$scheme = $colour->colour_scheme('background', false);
		$scheme = $colour->mono_range($scheme, 10, false);
		shuffle($scheme);

		$bg_colours = array_splice($scheme, mt_rand(6, 12));

		// Generate code characters
		$characters = $sizes = $bounding_boxes = array();
		$width_avail = $this->width - 15;
		$code_len = strlen($code);

		$captcha_bitmaps = $this->captcha_bitmaps();
		for ($i = 0; $i < $code_len; ++$i)
		{
			$characters[$i] = new char_cube3d($captcha_bitmaps, $code[$i]);

			list($min, $max) = $characters[$i]->range();
			$sizes[$i] = mt_rand($min, $max);

			$box = $characters[$i]->dimensions($sizes[$i]);
			$width_avail -= ($box[2] - $box[0]);
			$bounding_boxes[$i] = $box;
		}

		// Redistribute leftover x-space
		$offset = array();
		for ($i = 0; $i < $code_len; ++$i)
		{
			$denom = ($code_len - $i);
			$denom = max(1.3, $denom);
			$offset[$i] = mt_rand(0, (1.5 * $width_avail) / $denom);
			$width_avail -= $offset[$i];
		}

		if ($config['captcha_gd_x_grid'])
		{
			$grid = (int) $config['captcha_gd_x_grid'];
			for ($y = 0; $y < $this->height; $y += mt_rand($grid - 2, $grid + 2))
			{
				$current_colour = $scheme[array_rand($scheme)];
				imageline($img, mt_rand(0,4), mt_rand($y - 3, $y), mt_rand($this->width - 5, $this->width), mt_rand($y - 3, $y), $current_colour);
			}
		}

		if ($config['captcha_gd_y_grid'])
		{
			$grid = (int) $config['captcha_gd_y_grid'];
			for ($x = 0; $x < $this->width; $x += mt_rand($grid - 2, $grid + 2))
			{
				$current_colour = $scheme[array_rand($scheme)];
				imagedashedline($img, mt_rand($x -3, $x + 3), mt_rand(0, 4), mt_rand($x -3, $x + 3), mt_rand($this->height - 5, $this->height), $current_colour);
			}
		}

		$xoffset = 5;
		for ($i = 0; $i < $code_len; ++$i)
		{
			$dimm = $bounding_boxes[$i];
			$xoffset += ($offset[$i] - $dimm[0]);
			$yoffset = mt_rand(-$dimm[1], $this->height - $dimm[3]);

			$characters[$i]->drawchar($sizes[$i], $xoffset, $yoffset, $img, $colour->get_resource('background'), $scheme);
			$xoffset += $dimm[2];
		}
		
		if ($config['captcha_gd_foreground_noise'])
		{
			$this->noise_line($img, 0, 0, $this->width, $this->height, $colour->get_resource('background'), $scheme, $bg_colours);
		}

		// Send image
		header('Content-Type: image/png');
		header('Cache-control: no-cache, no-store');
		imagepng($img);
		imagedestroy($img);
	}

	/**
	* Noise line
	*/
	function noise_line($img, $min_x, $min_y, $max_x, $max_y, $bg, $font, $non_font)
	{
		imagesetthickness($img, 2);

		$x1 = $min_x;
		$x2 = $max_x;
		$y1 = $min_y;
		$y2 = $min_y;

		do
		{
			$line = array_merge(
				array_fill(0, mt_rand(30, 60), $non_font[array_rand($non_font)]),
				array_fill(0, mt_rand(30, 60), $bg)
			);

			imagesetstyle($img, $line);
			imageline($img, $x1, $y1, $x2, $y2, IMG_COLOR_STYLED);

			$y1 += mt_rand(12, 35);
			$y2 += mt_rand(12, 35);
		}
		while ($y1 < $max_y && $y2 < $max_y);

		$x1 = $min_x;
		$x2 = $min_x;
		$y1 = $min_y;
		$y2 = $max_y;

		do
		{
			$line = array_merge(
				array_fill(0, mt_rand(30, 60), $non_font[array_rand($non_font)]),
				array_fill(0, mt_rand(30, 60), $bg)
			);

			imagesetstyle($img, $line);
			imageline($img, $x1, $y1, $x2, $y2, IMG_COLOR_STYLED);

			$x1 += mt_rand(20, 35);
			$x2 += mt_rand(20, 35);
		}
		while ($x1 < $max_x && $x2 < $max_x);
		imagesetthickness($img, 1);
	}

	/**
	* Return bitmaps
	*/
	function captcha_bitmaps()
	{
		return array(
			'width'		=> 9,
			'height'	=> 15,
			'data'		=> array(

			'A' => array(
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,1,0,1,0,0,0),
				array(0,0,0,1,0,1,0,0,0),
				array(0,0,0,1,0,1,0,0,0),
				array(0,0,1,0,0,0,1,0,0),
				array(0,0,1,0,0,0,1,0,0),
				array(0,0,1,0,0,0,1,0,0),
				array(0,1,0,0,0,0,0,1,0),
				array(0,1,0,0,0,0,0,1,0),
				array(0,1,1,1,1,1,1,1,0),
				array(0,1,0,0,0,0,0,1,0),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
			),
			'B' => array(
				array(1,1,1,1,1,1,1,0,0),
				array(1,0,0,0,0,0,0,1,0),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,1,0),
				array(1,1,1,1,1,1,1,0,0),
				array(1,0,0,0,0,0,0,1,0),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,1,0),
				array(1,1,1,1,1,1,1,0,0),
			),
			'C' => array(
				array(0,0,1,1,1,1,1,0,0),
				array(0,1,0,0,0,0,0,1,0),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(0,1,0,0,0,0,0,1,0),
				array(0,0,1,1,1,1,1,0,0),
			),
			'D' => array(
				array(1,1,1,1,1,1,1,0,0),
				array(1,0,0,0,0,0,0,1,0),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,1,0),
				array(1,1,1,1,1,1,1,0,0),
			),
			'E' => array(
				array(1,1,1,1,1,1,1,1,1),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,1,1,1,1,1,1,1,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,1,1,1,1,1,1,1,1),
			),
			'F' => array(
				array(1,1,1,1,1,1,1,1,1),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,1,1,1,1,1,1,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
			),
			'G' => array(
				array(0,0,1,1,1,1,1,0,0),
				array(0,1,0,0,0,0,0,1,0),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,1,1,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(0,1,0,0,0,0,0,1,0),
				array(0,0,1,1,1,1,1,0,0),
			),
			'H' => array(
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,1,1,1,1,1,1,1,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
			),
			'I' => array(
				array(1,1,1,1,1,1,1,1,1),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(1,1,1,1,1,1,1,1,1),
			),
			'J' => array(
				array(1,1,1,1,1,1,1,1,1),
				array(0,0,0,0,0,1,0,0,0),
				array(0,0,0,0,0,1,0,0,0),
				array(0,0,0,0,0,1,0,0,0),
				array(0,0,0,0,0,1,0,0,0),
				array(0,0,0,0,0,1,0,0,0),
				array(0,0,0,0,0,1,0,0,0),
				array(0,0,0,0,0,1,0,0,0),
				array(0,0,0,0,0,1,0,0,0),
				array(0,0,0,0,0,1,0,0,0),
				array(0,0,0,0,0,1,0,0,0),
				array(1,0,0,0,0,1,0,0,0),
				array(1,0,0,0,0,1,0,0,0),
				array(0,1,0,0,1,0,0,0,0),
				array(0,0,1,1,0,0,0,0,0),
			),
			'K' => array(    // New 'K', supplied by NeoThermic
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,1,0),
				array(1,0,0,0,0,0,1,0,0),
				array(1,0,0,0,0,1,0,0,0),
				array(1,0,0,0,1,0,0,0,0),
				array(1,0,0,1,0,0,0,0,0),
				array(1,0,1,0,0,0,0,0,0),
				array(1,1,0,0,0,0,0,0,0),
				array(1,0,1,0,0,0,0,0,0),
				array(1,0,0,1,0,0,0,0,0),
				array(1,0,0,0,1,0,0,0,0),
				array(1,0,0,0,0,1,0,0,0),
				array(1,0,0,0,0,0,1,0,0),
				array(1,0,0,0,0,0,0,1,0),
				array(1,0,0,0,0,0,0,0,1),
			),
			'L' => array(
				array(0,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,1,1,1,1,1,1,1,1),
			),
			'M' => array(
				array(1,1,0,0,0,0,0,1,1),
				array(1,1,0,0,0,0,0,1,1),
				array(1,0,1,0,0,0,1,0,1),
				array(1,0,1,0,0,0,1,0,1),
				array(1,0,1,0,0,0,1,0,1),
				array(1,0,0,1,0,1,0,0,1),
				array(1,0,0,1,0,1,0,0,1),
				array(1,0,0,1,0,1,0,0,1),
				array(1,0,0,0,1,0,0,0,1),
				array(1,0,0,0,1,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
			),
			'N' => array(
				array(1,1,0,0,0,0,0,0,1),
				array(1,1,0,0,0,0,0,0,1),
				array(1,0,1,0,0,0,0,0,1),
				array(1,0,1,0,0,0,0,0,1),
				array(1,0,0,1,0,0,0,0,1),
				array(1,0,0,1,0,0,0,0,1),
				array(1,0,0,0,1,0,0,0,1),
				array(1,0,0,0,1,0,0,0,1),
				array(1,0,0,0,1,0,0,0,1),
				array(1,0,0,0,0,1,0,0,1),
				array(1,0,0,0,0,1,0,0,1),
				array(1,0,0,0,0,0,1,0,1),
				array(1,0,0,0,0,0,1,0,1),
				array(1,0,0,0,0,0,0,1,1),
				array(1,0,0,0,0,0,0,1,1),
			),
			'O' => array(
				array(0,0,1,1,1,1,1,0,0),
				array(0,1,0,0,0,0,0,1,0),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(0,1,0,0,0,0,0,1,0),
				array(0,0,1,1,1,1,1,0,0),
			),
			'P' => array(
				array(1,1,1,1,1,1,1,0,0),
				array(1,0,0,0,0,0,0,1,0),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,1,0),
				array(1,1,1,1,1,1,1,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
			),
			'Q' => array(
				array(0,0,1,1,1,1,1,0,0),
				array(0,1,0,0,0,0,0,1,0),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,1,0,0,1),
				array(1,0,0,0,0,0,1,0,1),
				array(0,1,0,0,0,0,0,1,0),
				array(0,0,1,1,1,1,1,0,1),
			),
			'R' => array(
				array(1,1,1,1,1,1,1,0,0),
				array(1,0,0,0,0,0,0,1,0),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,1,0),
				array(1,1,1,1,1,1,1,0,0),
				array(1,1,1,0,0,0,0,0,0),
				array(1,0,0,1,0,0,0,0,0),
				array(1,0,0,0,1,0,0,0,0),
				array(1,0,0,0,0,1,0,0,0),
				array(1,0,0,0,0,0,1,0,0),
				array(1,0,0,0,0,0,0,1,0),
				array(1,0,0,0,0,0,0,0,1),
			),
			'S' => array(
				array(0,0,1,1,1,1,1,0,0),
				array(0,1,0,0,0,0,0,1,0),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(0,1,0,0,0,0,0,0,0),
				array(0,0,1,1,1,1,1,0,0),
				array(0,0,0,0,0,0,0,1,0),
				array(0,0,0,0,0,0,0,0,1),
				array(0,0,0,0,0,0,0,0,1),
				array(0,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(0,1,0,0,0,0,0,1,0),
				array(0,0,1,1,1,1,1,0,0),
			),
			'T' => array(
				array(1,1,1,1,1,1,1,1,1),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
			),
			'U' => array(
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(0,1,0,0,0,0,0,1,0),
				array(0,0,1,1,1,1,1,0,0),
			),
			'V' => array(
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(0,1,0,0,0,0,0,1,0),
				array(0,1,0,0,0,0,0,1,0),
				array(0,1,0,0,0,0,0,1,0),
				array(0,0,1,0,0,0,1,0,0),
				array(0,0,1,0,0,0,1,0,0),
				array(0,0,1,0,0,0,1,0,0),
				array(0,0,1,0,0,0,1,0,0),
				array(0,0,0,1,0,1,0,0,0),
				array(0,0,0,1,0,1,0,0,0),
				array(0,0,0,1,0,1,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
			),
			'W' => array(    // New 'W', supplied by MHobbit
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,1,0,0,0,1),
				array(1,0,0,0,1,0,0,0,1),
				array(1,0,0,1,0,1,0,0,1),
				array(1,0,0,1,0,1,0,0,1),
				array(1,0,0,1,0,1,0,0,1),
				array(1,0,1,0,0,0,1,0,1),
				array(1,0,1,0,0,0,1,0,1),
				array(1,0,1,0,0,0,1,0,1),
				array(1,1,0,0,0,0,0,1,1),
				array(1,1,0,0,0,0,0,1,1),
			),
			'X' => array(
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(0,1,0,0,0,0,0,1,0),
				array(0,1,0,0,0,0,0,1,0),
				array(0,0,1,0,0,0,1,0,0),
				array(0,0,0,1,0,1,0,0,0),
				array(0,0,0,1,0,1,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,1,0,1,0,0,0),
				array(0,0,0,1,0,1,0,0,0),
				array(0,0,1,0,0,0,1,0,0),
				array(0,1,0,0,0,0,1,0,0),
				array(0,1,0,0,0,0,0,1,0),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
			),
			'Y' => array(
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(0,1,0,0,0,0,0,1,0),
				array(0,1,0,0,0,0,0,1,0),
				array(0,0,1,0,0,0,1,0,0),
				array(0,0,1,0,0,0,1,0,0),
				array(0,0,0,1,0,1,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
			),
			'Z' => array(    // New 'Z' supplied by Anon
				array(1,1,1,1,1,1,1,1,1),
				array(1,0,0,0,0,0,0,0,1),
				array(0,0,0,0,0,0,0,0,1),
				array(0,0,0,0,0,0,0,1,0),
				array(0,0,0,0,0,0,1,0,0),
				array(0,0,0,0,0,1,0,0,0),
				array(0,0,0,0,0,1,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,1,0,0,0,0,0),
				array(0,0,0,1,0,0,0,0,0),
				array(0,0,1,0,0,0,0,0,0),
				array(0,1,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,1),
				array(1,1,1,1,1,1,1,1,1),
			),
			'1' => array(
				array(0,0,0,1,1,0,0,0,0),
				array(0,0,1,0,1,0,0,0,0),
				array(0,1,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,1,1,1,1,1,1,1,0),
			),
			'2' => array(    // New '2' supplied by Anon
				array(0,0,0,1,1,1,0,0,0),
				array(0,0,1,0,0,0,1,0,0),
				array(0,1,0,0,0,0,1,1,0),
				array(0,0,0,0,0,0,0,0,1),
				array(0,0,0,0,0,0,0,0,1),
				array(0,0,0,0,0,0,0,1,1),
				array(0,0,0,0,0,0,0,1,0),
				array(0,0,0,0,0,0,1,0,0),
				array(0,0,0,0,0,1,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,1,0,0,0,0,0),
				array(0,0,1,0,0,0,0,0,0),
				array(0,1,0,0,0,0,0,0,0),
				array(1,1,1,1,1,1,1,1,1),
				array(0,0,0,0,0,0,0,0,0),
			),
			'3' => array(
				array(0,0,1,1,1,1,1,0,0),
				array(0,1,0,0,0,0,0,1,0),
				array(1,0,0,0,0,0,0,0,1),
				array(0,0,0,0,0,0,0,0,1),
				array(0,0,0,0,0,0,0,0,1),
				array(0,0,0,0,0,0,0,0,1),
				array(0,0,0,0,0,0,0,1,0),
				array(0,0,0,0,0,1,1,0,0),
				array(0,0,0,0,0,0,0,1,0),
				array(0,0,0,0,0,0,0,0,1),
				array(0,0,0,0,0,0,0,0,1),
				array(0,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(0,1,0,0,0,0,0,1,0),
				array(0,0,1,1,1,1,1,0,0),
			),
			'4' => array(
				array(0,0,0,0,0,0,1,1,0),
				array(0,0,0,0,0,1,0,1,0),
				array(0,0,0,0,1,0,0,1,0),
				array(0,0,0,1,0,0,0,1,0),
				array(0,0,1,0,0,0,0,1,0),
				array(0,1,0,0,0,0,0,1,0),
				array(1,0,0,0,0,0,0,1,0),
				array(1,0,0,0,0,0,0,1,0),
				array(1,1,1,1,1,1,1,1,1),
				array(0,0,0,0,0,0,0,1,0),
				array(0,0,0,0,0,0,0,1,0),
				array(0,0,0,0,0,0,0,1,0),
				array(0,0,0,0,0,0,0,1,0),
				array(0,0,0,0,0,0,0,1,0),
				array(0,0,0,0,0,0,0,1,0),
			),
			'5' => array(
				array(1,1,1,1,1,1,1,1,1),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(0,1,0,0,0,0,0,0,0),
				array(0,0,1,1,1,1,1,0,0),
				array(0,0,0,0,0,0,0,1,0),
				array(0,0,0,0,0,0,0,0,1),
				array(0,0,0,0,0,0,0,0,1),
				array(0,0,0,0,0,0,0,0,1),
				array(0,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(0,1,0,0,0,0,0,1,0),
				array(0,0,1,1,1,1,1,0,0),
			),
			'6' => array(
				array(0,0,1,1,1,1,1,0,0),
				array(0,1,0,0,0,0,0,1,0),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,1,1,1,1,0,0),
				array(1,0,1,0,0,0,0,1,0),
				array(1,1,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(0,1,0,0,0,0,0,1,0),
				array(0,0,1,1,1,1,1,0,0),
			),
			'7' => array(
				array(1,1,1,1,1,1,1,1,1),
				array(0,0,0,0,0,0,0,0,1),
				array(0,0,0,0,0,0,0,1,0),
				array(0,0,0,0,0,0,0,1,0),
				array(0,0,0,0,0,0,1,0,0),
				array(0,0,0,0,0,1,0,0,0),
				array(0,0,0,0,0,1,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,1,0,0,0,0,0),
				array(0,0,0,1,0,0,0,0,0),
				array(0,0,1,0,0,0,0,0,0),
				array(0,1,0,0,0,0,0,0,0),
				array(0,1,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
			),
			'8' => array(
				array(0,0,1,1,1,1,1,0,0),
				array(0,1,0,0,0,0,0,1,0),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(0,1,0,0,0,0,0,1,0),
				array(0,0,1,1,1,1,1,0,0),
				array(0,1,0,0,0,0,0,1,0),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(0,1,0,0,0,0,0,1,0),
				array(0,0,1,1,1,1,1,0,0),
			),
			'9' => array(
				array(0,0,1,1,1,1,1,0,0),
				array(0,1,0,0,0,0,0,1,0),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,1,1),
				array(0,1,0,0,0,0,1,0,1),
				array(0,0,1,1,1,1,0,0,1),
				array(0,0,0,0,0,0,0,0,1),
				array(0,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(0,1,0,0,0,0,0,1,0),
				array(0,0,1,1,1,1,1,0,0),
			),
			)
		);
	}
}

/**
* @package VC
*/
class char_cube3d
{
	var $bitmap;
	var $bitmap_width;
	var $bitmap_height;

	var $basis_matrix = array(array(1, 0, 0), array(0, 1, 0), array(0, 0, 1));
	var $abs_x = array(1, 0);
	var $abs_y = array(0, 1);
	var $x = 0;
	var $y = 1;
	var $z = 2;
	var $letter = '';

	/**
	*/
	function char_cube3d(&$bitmaps, $letter)
	{
		$this->bitmap			= $bitmaps['data'][$letter];
		$this->bitmap_width		= $bitmaps['width'];
		$this->bitmap_height	= $bitmaps['height'];

		$this->basis_matrix[0][0] = mt_rand(-600, 600);
		$this->basis_matrix[0][1] = mt_rand(-600, 600);
		$this->basis_matrix[0][2] = (mt_rand(0, 1) * 2000) - 1000;
		$this->basis_matrix[1][0] = mt_rand(-1000, 1000);
		$this->basis_matrix[1][1] = mt_rand(-1000, 1000);
		$this->basis_matrix[1][2] = mt_rand(-1000, 1000);

		$this->normalize($this->basis_matrix[0]);
		$this->normalize($this->basis_matrix[1]);
		$this->basis_matrix[2] = $this->cross_product($this->basis_matrix[0], $this->basis_matrix[1]);
		$this->normalize($this->basis_matrix[2]);

		// $this->basis_matrix[1] might not be (probably isn't) orthogonal to $basis_matrix[0]
		$this->basis_matrix[1] = $this->cross_product($this->basis_matrix[0], $this->basis_matrix[2]);
		$this->normalize($this->basis_matrix[1]);

		// Make sure our cube is facing into the canvas (assuming +z == in)
		for ($i = 0; $i < 3; ++$i)
		{
			if ($this->basis_matrix[$i][2] < 0)
			{
				$this->basis_matrix[$i][0] *= -1;
				$this->basis_matrix[$i][1] *= -1;
				$this->basis_matrix[$i][2] *= -1;
			}
		}

		// Force our "z" basis vector to be the one with greatest absolute z value
		$this->x = 0;
		$this->y = 1;
		$this->z = 2;

		// Swap "y" with "z"
		if ($this->basis_matrix[1][2] > $this->basis_matrix[2][2])
		{
			$this->z = 1;
			$this->y = 2;
		}

		// Swap "x" with "z"
		if ($this->basis_matrix[0][2] > $this->basis_matrix[$this->z][2])
		{
			$this->x = $this->z;
			$this->z = 0;
		}

		// Still need to determine which of $x,$y are which.
		// wrong orientation if y's y-component is less than it's x-component
		// likewise if x's x-component is less than it's y-component
		// if they disagree, go with the one with the greater weight difference.
		// rotate if positive
		$weight = (abs($this->basis_matrix[$this->x][1]) - abs($this->basis_matrix[$this->x][0])) + (abs($this->basis_matrix[$this->y][0]) - abs($this->basis_matrix[$this->y][1]));

		// Swap "x" with "y"
		if ($weight > 0)
		{
			list($this->x, $this->y) = array($this->y, $this->x);
		}

		$this->abs_x = array($this->basis_matrix[$this->x][0], $this->basis_matrix[$this->x][1]);
		$this->abs_y = array($this->basis_matrix[$this->y][0], $this->basis_matrix[$this->y][1]);

		if ($this->abs_x[0] < 0)
		{
			$this->abs_x[0] *= -1;
			$this->abs_x[1] *= -1;
		}

		if ($this->abs_y[1] > 0)
		{
			$this->abs_y[0] *= -1;
			$this->abs_y[1] *= -1;
		}

		$this->letter = $letter;
	}

	/**
	* Draw a character
	*/
	function drawchar($scale, $xoff, $yoff, $img, $background, $colours)
	{
		$width	= $this->bitmap_width;
		$height	= $this->bitmap_height;
		$bitmap	= $this->bitmap;

		$colour1 = $colours[array_rand($colours)];
		$colour2 = $colours[array_rand($colours)];

		$swapx = ($this->basis_matrix[$this->x][0] > 0);
		$swapy = ($this->basis_matrix[$this->y][1] < 0);

		for ($y = 0; $y < $height; ++$y)
		{
			for ($x = 0; $x < $width; ++$x)
			{
				$xp = ($swapx) ? ($width - $x - 1) : $x;
				$yp = ($swapy) ? ($height - $y - 1) : $y;

				if ($bitmap[$height - $yp - 1][$xp])
				{
					$dx = $this->scale($this->abs_x, ($xp - ($swapx ? ($width / 2) : ($width / 2) - 1)) * $scale);
					$dy = $this->scale($this->abs_y, ($yp - ($swapy ? ($height / 2) : ($height / 2) - 1)) * $scale);
					$xo = $xoff + $dx[0] + $dy[0];
					$yo = $yoff + $dx[1] + $dy[1];

					$origin = array(0, 0, 0);
					$xvec = $this->scale($this->basis_matrix[$this->x], $scale);
					$yvec = $this->scale($this->basis_matrix[$this->y], $scale);
					$face_corner = $this->sum2($xvec, $yvec);

					$zvec = $this->scale($this->basis_matrix[$this->z], $scale);
					$x_corner = $this->sum2($xvec, $zvec);
					$y_corner = $this->sum2($yvec, $zvec);

					imagefilledpolygon($img, $this->gen_poly($xo, $yo, $origin, $xvec, $x_corner,$zvec), 4, $colour1);
					imagefilledpolygon($img, $this->gen_poly($xo, $yo, $origin, $yvec, $y_corner,$zvec), 4, $colour2);

					$face = $this->gen_poly($xo, $yo, $origin, $xvec, $face_corner, $yvec);

					imagefilledpolygon($img, $face, 4, $background);
					imagepolygon($img, $face, 4, $colour1);
				}
			}
		}
	}

	/*
	* return a roughly acceptable range of sizes for rendering with this texttype
	*/
	function range()
	{
		return array(3, 4);
	}

	/**
	* Vector length
	*/
	function vectorlen($vector)
	{
		return sqrt(pow($vector[0], 2) + pow($vector[1], 2) + pow($vector[2], 2));
	}

	/**
	* Normalize
	*/
	function normalize(&$vector, $length = 1)
	{
		$length = (( $length < 1) ? 1 : $length);
		$length /= $this->vectorlen($vector);
		$vector[0] *= $length;
		$vector[1] *= $length;
		$vector[2] *= $length;
	}

	/**
	*/
	function cross_product($vector1, $vector2)
	{
		$retval = array(0, 0, 0);
		$retval[0] =  (($vector1[1] * $vector2[2]) - ($vector1[2] * $vector2[1]));
		$retval[1] = -(($vector1[0] * $vector2[2]) - ($vector1[2] * $vector2[0]));
		$retval[2] =  (($vector1[0] * $vector2[1]) - ($vector1[1] * $vector2[0]));

		return $retval;
	}

	/**
	*/
	function sum($vector1, $vector2)
	{
		return array($vector1[0] + $vector2[0], $vector1[1] + $vector2[1], $vector1[2] + $vector2[2]);
	}

	/**
	*/
	function sum2($vector1, $vector2)
	{
		return array($vector1[0] + $vector2[0], $vector1[1] + $vector2[1]);
	}

	/**
	*/
	function scale($vector, $length)
	{
		if (sizeof($vector) == 2)
		{
			return array($vector[0] * $length, $vector[1] * $length);
		}

		return array($vector[0] * $length, $vector[1] * $length, $vector[2] * $length);
	}

	/**
	*/
	function gen_poly($xoff, $yoff, &$vec1, &$vec2, &$vec3, &$vec4)
	{
		$poly = array();
		$poly[0] = $xoff + $vec1[0];
		$poly[1] = $yoff + $vec1[1];
		$poly[2] = $xoff + $vec2[0];
		$poly[3] = $yoff + $vec2[1];
		$poly[4] = $xoff + $vec3[0];
		$poly[5] = $yoff + $vec3[1];
		$poly[6] = $xoff + $vec4[0];
		$poly[7] = $yoff + $vec4[1];

		return $poly;
	}

	/**
	* dimensions
	*/
	function dimensions($size)
	{
		$xn = $this->scale($this->basis_matrix[$this->x], -($this->bitmap_width / 2) * $size);
		$xp = $this->scale($this->basis_matrix[$this->x], ($this->bitmap_width / 2) * $size);
		$yn = $this->scale($this->basis_matrix[$this->y], -($this->bitmap_height / 2) * $size);
		$yp = $this->scale($this->basis_matrix[$this->y], ($this->bitmap_height / 2) * $size);

		$p = array();
		$p[0] = $this->sum2($xn, $yn);
		$p[1] = $this->sum2($xp, $yn);
		$p[2] = $this->sum2($xp, $yp);
		$p[3] = $this->sum2($xn, $yp);

		$min_x = $max_x = $p[0][0];
		$min_y = $max_y = $p[0][1];

		for ($i = 1; $i < 4; ++$i)
		{
			$min_x = ($min_x > $p[$i][0]) ? $p[$i][0] : $min_x;
			$min_y = ($min_y > $p[$i][1]) ? $p[$i][1] : $min_y;
			$max_x = ($max_x < $p[$i][0]) ? $p[$i][0] : $max_x;
			$max_y = ($max_y < $p[$i][1]) ? $p[$i][1] : $max_y;
		}

		return array($min_x, $min_y, $max_x, $max_y);
	}
}

/**
* @package VC
*/
class colour_manager
{
	var $img;
	var $mode;
	var $colours;
	var $named_colours;

	/**
	* Create the colour manager, link it to the image resource
	*/
	function colour_manager($img, $background = false, $mode = 'ahsv')
	{
		$this->img = $img;
		$this->mode = $mode;
		$this->colours = array();
		$this->named_colours = array();

		if ($background !== false)
		{
			$bg = $this->allocate_named('background', $background);
			imagefill($this->img, 0, 0, $bg);
		}
	}

	/**
	* Lookup a named colour resource
	*/
	function get_resource($named_colour)
	{
		if (isset($this->named_colours[$named_colour]))
		{
			return $this->named_colours[$named_colour];
		}

		if (isset($this->named_rgb[$named_colour]))
		{
			return $this->allocate_named($named_colour, $this->named_rgb[$named_colour], 'rgb');
		}

		return false;
	}

	/**
	* Assign a name to a colour resource
	*/
	function name_colour($name, $resource)
	{
		$this->named_colours[$name] = $resource;
	}

	/**
	* names and allocates a colour resource
	*/
	function allocate_named($name, $colour, $mode = false)
	{
		$resource = $this->allocate($colour, $mode);

		if ($resource !== false)
		{
			$this->name_colour($name, $resource);
		}
		return $resource;
	}

	/**
	* allocates a specified colour into the image
	*/
	function allocate($colour, $mode = false)
	{
		if ($mode === false)
		{
			$mode = $this->mode;
		}
		
		if (!is_array($colour))
		{
			if (isset($this->named_rgb[$colour]))
			{
				return $this->allocate_named($colour, $this->named_rgb[$colour], 'rgb');
			}

			if (!is_int($colour))
			{
				return false;
			}

			$mode = 'rgb';
			$colour = array(255 & ($colour >> 16), 255 & ($colour >>  8), 255 & $colour);
		}

		if (isset($colour['mode']))
		{
			$mode = $colour['mode'];
			unset($colour['mode']);
		}

		if (isset($colour['random']))
		{
			unset($colour['random']);
			// everything else is params
			return $this->random_colour($colour, $mode);
		}

		$rgb		= colour_manager::model_convert($colour, $mode, 'rgb');
		$store		= ($this->mode == 'rgb') ? $rgb : colour_manager::model_convert($colour, $mode, $this->mode);
		$resource	= imagecolorallocate($this->img, $rgb[0], $rgb[1], $rgb[2]);
		$this->colours[$resource] = $store;

		return $resource;
	}

	/**
	* randomly generates a colour, with optional params
	*/
	function random_colour($params = array(), $mode = false)
	{
		if ($mode === false)
		{
			$mode = $this->mode;
		}

		switch ($mode)
		{
			case 'rgb':
				// @TODO random rgb generation. do we intend to do this, or is it just too tedious?
			break;

			case 'ahsv':
			case 'hsv':
			default:

				$default_params = array(
					'hue_bias'			=> false,	// degree / 'r'/'g'/'b'/'c'/'m'/'y'   /'o'
					'hue_range'			=> false,	// if hue bias, then difference range +/- from bias
					'min_saturation'	=> 30,		// 0 - 100
					'max_saturation'	=> 80,		// 0 - 100
					'min_value'			=> 30,		// 0 - 100
					'max_value'			=> 80,		// 0 - 100
				);

				$alt = ($mode == 'ahsv') ? true : false;
				$params = array_merge($default_params, $params);

				$min_hue		= 0;
				$max_hue		= 359;
				$min_saturation	= max(0, $params['min_saturation']);
				$max_saturation	= min(100, $params['max_saturation']);
				$min_value		= max(0, $params['min_value']);
				$max_value		= min(100, $params['max_value']);

				if ($params['hue_bias'] !== false)
				{
					if (is_numeric($params['hue_bias']))
					{
						$h = intval($params['hue_bias']) % 360;
					}
					else
					{
						switch ($params['hue_bias'])
						{
							case 'o':
								$h = $alt ?  60 :  30;
							break;

							case 'y':
								$h = $alt ? 120 :  60;
							break;

							case 'g':
								$h = $alt ? 180 : 120;
							break;

							case 'c':
								$h = $alt ? 210 : 180;
							break;

							case 'b':
								$h = 240;
							break;

							case 'm':
								$h = 300;
							break;

							case 'r':
							default:
								$h = 0;
							break;
						}
					}

					$min_hue = $h + 360;
					$max_hue = $h + 360;

					if ($params['hue_range'])
					{
						$min_hue -= min(180, $params['hue_range']);
						$max_hue += min(180, $params['hue_range']);
					}
				}

				$h = mt_rand($min_hue, $max_hue);
				$s = mt_rand($min_saturation, $max_saturation);
				$v = mt_rand($min_value, $max_value);

				return $this->allocate(array($h, $s, $v), $mode);

			break;
		}
	}

	/**
	*/
	function colour_scheme($resource, $include_original = true)
	{
		$mode = 'hsv';

		if (($pre = $this->get_resource($resource)) !== false)
		{
			$resource = $pre;
		}

		$colour = colour_manager::model_convert($this->colours[$resource], $this->mode, $mode);
		$results = ($include_original) ? array($resource) : array();
		$colour2 = $colour3 = $colour4 = $colour;
		$colour2[0] += 150;
		$colour3[0] += 180;
		$colour4[0] += 210;


		$results[] = $this->allocate($colour2, $mode);
		$results[] = $this->allocate($colour3, $mode);
		$results[] = $this->allocate($colour4, $mode);

		return $results;
	}

	/**
	*/
	function mono_range($resource, $count = 5, $include_original = true)
	{
		if (is_array($resource))
		{
			$results = array();
			for ($i = 0, $size = sizeof($resource); $i < $size; ++$i)
			{
				$results = array_merge($results, $this->mono_range($resource[$i], $count, $include_original));
			}
			return $results;
		}

		$mode = (in_array($this->mode, array('hsv', 'ahsv'), true) ? $this->mode : 'ahsv');
		if (($pre = $this->get_resource($resource)) !== false)
		{
			$resource = $pre;
		}

		$colour = colour_manager::model_convert($this->colours[$resource], $this->mode, $mode);

		$results = array();
		if ($include_original)
		{
			$results[] = $resource;
			$count--;
		}

		// This is a hard problem. I chicken out and try to maintain readability at the cost of less randomness.
		
		while ($count > 0)
		{
			$colour[1] = ($colour[1] + mt_rand(40,60)) % 99;
			$colour[2] = ($colour[2] + mt_rand(40,60));
			$results[] = $this->allocate($colour, $mode);
			$count--;
		}
		return $results;
	}

	/**
	* Convert from one colour model to another
	*/
	function model_convert($colour, $from_model, $to_model)
	{
		if ($from_model == $to_model)
		{
			return $colour;
		}

		switch ($to_model)
		{
			case 'hsv':

				switch ($from_model)
				{
					case 'ahsv':
						return colour_manager::ah2h($colour);
					break;

					case 'rgb':
						return colour_manager::rgb2hsv($colour);
					break;
				}
			break;

			case 'ahsv':

				switch ($from_model)
				{
					case 'hsv':
						return colour_manager::h2ah($colour);
					break;

					case 'rgb':
						return colour_manager::h2ah(colour_manager::rgb2hsv($colour));
					break;
				}
			break;

			case 'rgb':
				switch ($from_model)
				{
					case 'hsv':
						return colour_manager::hsv2rgb($colour);
					break;

					case 'ahsv':
						return colour_manager::hsv2rgb(colour_manager::ah2h($colour));
					break;
				}
			break;
		}
		return false;
	}

	/**
	* Slightly altered from wikipedia's algorithm
	*/
	function hsv2rgb($hsv)
	{
		colour_manager::normalize_hue($hsv[0]);

		$h = $hsv[0];
		$s = min(1, max(0, $hsv[1] / 100));
		$v = min(1, max(0, $hsv[2] / 100));

		// calculate hue sector
		$hi = floor($hsv[0] / 60);

		// calculate opposite colour
		$p = $v * (1 - $s);

		// calculate distance between hex vertices
		$f = ($h / 60) - $hi;

		// coming in or going out?
		if (!($hi & 1))
		{
			$f = 1 - $f;
		}

		// calculate adjacent colour
		$q = $v * (1 - ($f * $s));

		switch ($hi)
		{
			case 0:
				$rgb = array($v, $q, $p);
			break;

			case 1:
				$rgb = array($q, $v, $p);
			break;

			case 2:
				$rgb = array($p, $v, $q);
			break;

			case 3:
				$rgb = array($p, $q, $v);
			break;

			case 4:
				$rgb = array($q, $p, $v);
			break;

			case 5:
				$rgb = array($v, $p, $q);
			break;

			default:
				return array(0, 0, 0);
			break;
		}

		return array(255 * $rgb[0], 255 * $rgb[1], 255 * $rgb[2]);
	}

	/**
	* (more than) Slightly altered from wikipedia's algorithm
	*/
	function rgb2hsv($rgb)
	{
		$r = min(255, max(0, $rgb[0]));
		$g = min(255, max(0, $rgb[1]));
		$b = min(255, max(0, $rgb[2]));
		$max = max($r, $g, $b);
		$min = min($r, $g, $b);

		$v = $max / 255;
		$s = (!$max) ? 0 : 1 - ($min / $max);

		// if max - min is 0, we want hue to be 0 anyway.
		$h = $max - $min;

		if ($h)
		{
			switch ($max)
			{
				case $g:
					$h = 120 + (60 * ($b - $r) / $h);
				break;

				case $b:
					$h = 240 + (60 * ($r - $g) / $h);
				break;

				case $r:
					$h = 360 + (60 * ($g - $b) / $h);
				break;
			}
		}
		colour_manager::normalize_hue($h);

		return array($h, $s * 100, $v * 100);
	}

	/**
	*/
	function normalize_hue(&$hue)
	{
		$hue %= 360;

		if ($hue < 0)
		{
			$hue += 360;
		}
	}

	/**
	* Alternate hue to hue
	*/
	function ah2h($ahue)
	{
		if (is_array($ahue))
		{
			$ahue[0] = colour_manager::ah2h($ahue[0]);
			return $ahue;
		}
		colour_manager::normalize_hue($ahue);

		// blue through red is already ok
		if ($ahue >= 240)
		{
			return $ahue;
		}

		// ahue green is at 180
		if ($ahue >= 180)
		{
			// return (240 - (2 * (240 - $ahue)));
			return (2 * $ahue) - 240; // equivalent
		}

		// ahue yellow is at 120   (RYB rather than RGB)
		if ($ahue >= 120)
		{
			return $ahue - 60;
		}

		return $ahue / 2;
	}

	/**
	* hue to Alternate hue
	*/
	function h2ah($hue)
	{
		if (is_array($hue))
		{
			$hue[0] = colour_manager::h2ah($hue[0]);
			return $hue;
		}
		colour_manager::normalize_hue($hue);

		// blue through red is already ok
		if ($hue >= 240)
		{
			return $hue;
		}
		else if ($hue <= 60)
		{
			return $hue * 2;
		}
		else if ($hue <= 120)
		{
			return $hue + 60;
		}
		else
		{
			return ($hue + 240) / 2;
		}
	}
}

?>