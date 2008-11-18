<?php
/**
 * @file
 * @ingroup Templates
 */

# Prevent XSS
if ( isset( $wgVersion ) ) {
	$wgVersion = htmlspecialchars( $wgVersion );
} else {
	$wgVersion = 'VERSION';
}
# Set the path in case we hit a page such as /index.php/Main_Page
# Could use <base href> but then we have to worry about http[s]/port #/etc.
$ext = strpos( $_SERVER['SCRIPT_NAME'], 'index.php5' ) === false ? 'php' : 'php5';
$path = '';
if( isset( $_SERVER['SCRIPT_NAME'] )) {
	$path = htmlspecialchars( preg_replace('/index.php5?/', '', $_SERVER['SCRIPT_NAME']) );
}
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns='http://www.w3.org/1999/xhtml' xml:lang='en' lang='en'>
	<head>
		<title>MediaWiki <?php echo $wgVersion ?></title>
		<meta http-equiv='Content-Type' content='text/html; charset=utf-8' />
		<style type='text/css' media='screen, projection'>
			html, body {
				color: #000;
				background-color: #fff;
				font-family: sans-serif;
				text-align: center;
			}

			h1 {
				font-size: 150%;
			}
		</style>
	</head>
	<body>
		<img src="<?php echo $path ?>skins/common/images/mediawiki.png" alt='The MediaWiki logo' />

		<h1>MediaWiki <?php echo $wgVersion ?></h1>
		<div class='error'>
		<?php
		if ( file_exists( 'config/LocalSettings.php' ) ) {
			echo( 'To complete the installation, move <tt>config/LocalSettings.php</tt> to the parent directory.' );
		} else {
			echo( "Please <a href=\"${path}config/index.{$ext}\" title='setup'> set up the wiki</a> first." );
		}
		?>

		</div>
	</body>
</html>
