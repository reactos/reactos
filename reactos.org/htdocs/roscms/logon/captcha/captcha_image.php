<?php
session_start();

$output = new CaptchaSecurityImages();

class CaptchaSecurityImages {

	/* This function and bitmaps were taken from phpBB 3.0 and modified, copyright (c) 2006 phpBB Group, licensed under GNU GPL */
	function captcha_bitmaps()
	{
		return array(
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
				array(1,0,0,0,0,1,1,1,1),
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
				array(0,1,1,1,1,1,1,1,0),
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
				array(0,1,1,1,1,1,1,1,0),
			),
			'J' => array(
				array(0,1,1,1,1,1,1,1,0),
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
				array(0,1,0,0,0,0,0,0,0),
				array(0,1,0,0,0,0,0,0,0),
				array(0,1,0,0,0,0,0,0,0),
				array(0,1,0,0,0,0,0,0,0),
				array(0,1,0,0,0,0,0,0,0),
				array(0,1,0,0,0,0,0,0,0),
				array(0,1,0,0,0,0,0,0,0),
				array(0,1,0,0,0,0,0,0,0),
				array(0,1,0,0,0,0,0,0,0),
				array(0,1,0,0,0,0,0,0,0),
				array(0,1,0,0,0,0,0,0,0),
				array(0,1,0,0,0,0,0,0,0),
				array(0,1,0,0,0,0,0,0,0),
				array(0,1,0,0,0,0,0,0,0),
				array(0,1,1,1,1,1,1,1,1),
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
				array(0,0,0,0,0,0,0,0,1),
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
				array(1,0,0,0,0,0,0,0,0),
				array(1,1,1,1,1,1,1,1,1),
			),
			'2' => array(    // New '2' supplied by Anon
				array(0,0,0,1,1,1,1,0,0),
				array(0,0,1,0,0,0,0,1,0),
				array(0,1,0,0,0,0,0,0,1),
				array(1,0,0,0,0,0,0,0,1),
				array(0,0,0,0,0,0,0,0,1),
				array(0,0,0,0,0,0,0,0,1),
				array(0,0,0,0,0,0,0,1,0),
				array(0,0,0,0,0,0,1,0,0),
				array(0,0,0,0,0,1,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,1,0,0,0,0,0),
				array(0,0,1,0,0,0,0,0,0),
				array(0,1,0,0,0,0,0,0,0),
				array(1,0,0,0,0,0,0,0,0),
				array(1,1,1,1,1,1,1,1,1),
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
				array(1,1,1,1,1,1,1,1,0),
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
				array(1,0,0,0,0,0,0,0,0),
				array(1,1,1,1,1,1,1,0,0),
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
				array(0,0,0,0,0,1,1,0,0),
				array(0,0,0,0,0,1,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,0,1,0,0,0,0),
				array(0,0,0,1,0,0,0,0,0),
				array(0,0,0,1,0,0,0,0,0),
				array(0,0,1,0,0,0,0,0,0),
				array(0,0,1,0,0,0,0,0,0),
				array(0,1,0,0,0,0,0,0,0),
				array(0,1,0,0,0,0,0,0,0),
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
			)
		);
	}
	
	function CaptchaSecurityImages()
	{
		$width = 260;
		$height = 80;
		$letter_width = 9;
		$letter_height = 15;
		$ellipse_thickness = 3;
		
		$image = @imagecreatetruecolor($width, $height) or die('Cannot initialize new GD image stream');
		
		/* Set the background color */
		$background_color = imagecolorallocate($image, 255, 255, 255);
		imagefill($image, 0, 0, $background_color);
		
		/* Generate random dots in background */
		for($i = 0; $i < ($width * $height) / 3; ++$i)
		{
			$color = imagecolorallocate($image, mt_rand(0, 255), mt_rand(0, 255), mt_rand(0, 255));
			imagefilledellipse($image, mt_rand(0, $width), mt_rand(0, $height), 1, 1, $color);
		}
		
		/* Generate random lines in background */
		for($i = 0; $i < ($width * $height) / 150; ++$i)
		{
			$color = imagecolorallocate($image, mt_rand(0, 255), mt_rand(0, 255), mt_rand(0, 255));
			imageline($image, mt_rand(0, $width), mt_rand(0, $height), mt_rand(0, $width), mt_rand(0, $height), $color);
		}
		
		/* Add the text */
		$possible = "23456789ABCEFGHJKLMNPRSTUVWXYZ";
		$possible_end = strlen($possible) - 1;
		$letters = mt_rand(4, 6);
		$bitmaps = $this->captcha_bitmaps();
		$code = "";
		$letterparts = array();
		$xpos = 0;
		$codewidth = 0;
		
		for($i = 0; $i < $letters; ++$i)
		{
			$letter = substr($possible, mt_rand(0, $possible_end), 1);
			
			$letterpart_dimension = mt_rand(3, 4);
			$letter_spacing = mt_rand(2, 4);
			$letter_bending = mt_rand(-1, 1) / 2.0;
			
			$codewidth += $letter_width * $letterpart_dimension + $letter_spacing;
			
			/* Corner cases:
			   - If we are at the first letter and we have a left bending, the letter will be painted out of the image by default, correct this here.
			     Also add this to the total code width.
			   - If we are at the last letter and we have a right bending, only add this to the total code width
			*/
			if($i == 0 && $letter_bending < 0)
			{
				$codewidth += $letter_height * -$letter_bending;
				$xpos += $letter_height * -$letter_bending;
			}
			else if($i == $letters - 1 && $letter_bending > 0)
			{
				$codewidth += $letter_height * $letter_bending;
			}
			
			$ypos = mt_rand(0, $height - $letter_height * $letterpart_dimension);
			$color_type = mt_rand(0, 2);
			
			/* Draw this letter */
			for($y = 0; $y < $letter_height; ++$y)
			{
				for($x = 0; $x < $letter_width; ++$x)
				{
					if($bitmaps[$letter][$y][$x])
					{
						if($color_type == 0)
							$color = imagecolorallocate($image, mt_rand(0, 20), mt_rand(0, 40), mt_rand(0, 255));
						else if($color_type == 1)
							$color = imagecolorallocate($image, mt_rand(0, 200), mt_rand(0, 20), mt_rand(0, 40));
						else
							$color = imagecolorallocate($image, mt_rand(0, 20), mt_rand(0, 150), mt_rand(0, 40));
						
						$letterparts[] = array(
							'color' => $color,
							'xpos' => $xpos + $y * $letter_bending,
							'ypos' => $ypos,
							'dimension' => $letterpart_dimension + $ellipse_thickness
						);
					}
					
					$xpos += $letterpart_dimension;
				}
				
				$xpos -= $letter_width * $letterpart_dimension;
				$ypos += $letterpart_dimension;
			}
			
			$xpos += $letter_width * $letterpart_dimension + $letter_spacing + 1;
			$code .= $letter;
		}
		
		/* Draw all letter parts in a different order, so different parts overlap */
		$x_start = mt_rand(0, $width - $codewidth);
		shuffle($letterparts);
		
		foreach($letterparts as $item)
			imagefilledellipse($image, $x_start + $item['xpos'], $item['ypos'], $item['dimension'], $item['dimension'], $item['color']);
		
		/* Generate some random arcs in foreground */
		$num = mt_rand(30, 50);
		$x = $width;
		$y = $height;
		$s = mt_rand(0, 270);
		
		for($n = 0; $n < $num; ++$n)
		{
			$color = imagecolorallocate($image, mt_rand(0, 255), mt_rand(0, 255), mt_rand(0, 255));
			
			imagearc($image,
				mt_rand(0.1 * $x, 0.9 * $x), mt_rand(0.1 * $y, 0.9 * $y),  //x,y
				mt_rand(0.1 * $x, 0.3 * $x), mt_rand(0.1 * $y, 0.3 * $y),  //w,h
				$s, mt_rand($s + 5, $s + 90),     // s,e
				$color
			);
		}
		
		/* output captcha image to browser */
		header('Content-Type: image/png');
		header('Cache-control: no-cache, no-store');
		imagepng($image);
		imagedestroy($image);
		$_SESSION['rdf_security_code'] = $code;
	}
}

?>
