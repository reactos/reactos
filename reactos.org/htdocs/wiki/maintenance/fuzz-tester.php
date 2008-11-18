<?php
/**
* @file
* @ingroup Maintenance
* @author Nick Jenkins ( http://nickj.org/ ).
* @copyright 2006 Nick Jenkins
* @licence GNU General Public Licence 2.0

Started: 18 May 2006.

Description:
  Performs fuzz-style testing of MediaWiki's parser and forms.

How:
  - Generate lots of nasty wiki text.
  - Ask the Parser to render that wiki text to HTML, or ask MediaWiki's forms 
    to deal with that wiki text.
  - Check MediaWiki's output for problems. 
  - Repeat.

Why:
  - To help find bugs.
  - To help find security issues, or potential security issues.

What type of problems are being checked for:
  - Unclosed tags.
  - Errors or interesting warnings from Tidy.
  - PHP errors / warnings / notices.
  - MediaWiki internal errors.
  - Very slow responses.
  - No response from apache.
  - Optionally checking for malformed HTML using the W3C validator.

Background:
  Many of the wikiFuzz class methods are a modified PHP port, 
  of a "shameless" Python port, of LCAMTUF'S MANGELME:
  - http://www.securiteam.com/tools/6Z00N1PBFK.html
  - http://www.securityfocus.com/archive/1/378632/2004-10-15/2004-10-21/0

Video:
  There's an XviD video discussing this fuzz tester. You can get it from:
  http://files.nickj.org/MediaWiki/Fuzz-Testing-MediaWiki-xvid.avi

Requirements:
  To run this, you will need:
  - Command-line PHP5, with PHP-curl enabled (not all installations have this 
    enabled - try "apt-get install php5-curl" if you're on Debian to install).
  - the Tidy standalone executable. ("apt-get install tidy").

Optional:
  - If you want to run the curl scripts, you'll need standalone curl installed
    ("apt-get install curl")
  - For viewing the W3C validator output on a command line, the "html2text"
    program may be useful ("apt-get install html2text")

Saving tests and test results:
  Any of the fuzz tests which find problems are saved for later review.
  In order to help track down problems, tests are saved in a number of
  different formats. The default filename extensions and their meanings are:
  - ".test.php" : PHP script that reproduces just that one problem using PHP-Curl.
  - ".curl.sh"  : Shell script that reproduces that problem using standalone curl.
  - ".data.bin" : The serialized PHP data so that this script can re-run the test.
  - ".info.txt" : A human-readable text file with details of the field contents.

Wiki configuration for testing:
  You should make some additions to LocalSettings.php in order to catch the most
  errors. Note this configuration is for **TESTING PURPOSES ONLY**, and is IN NO
  WAY, SHAPE, OR FORM suitable for deployment on a hostile network. That said, 
  personally I find these additions to be the most helpful for testing purposes:

  // --------- Start ---------
  // Everyone can do everything. Very useful for testing, yet useless for deployment.
  $wgGroupPermissions['*']['autoconfirmed']   = true;
  $wgGroupPermissions['*']['block']           = true;
  $wgGroupPermissions['*']['bot']             = true;
  $wgGroupPermissions['*']['delete']          = true;
  $wgGroupPermissions['*']['deletedhistory']  = true;
  $wgGroupPermissions['*']['deleterevision']  = true;
  $wgGroupPermissions['*']['editinterface']   = true;
  $wgGroupPermissions['*']['hiderevision']    = true;
  $wgGroupPermissions['*']['import']          = true;
  $wgGroupPermissions['*']['importupload']    = true;
  $wgGroupPermissions['*']['minoredit']       = true;
  $wgGroupPermissions['*']['move']            = true;
  $wgGroupPermissions['*']['patrol']          = true;
  $wgGroupPermissions['*']['protect']         = true;
  $wgGroupPermissions['*']['proxyunbannable'] = true;
  $wgGroupPermissions['*']['renameuser']      = true;
  $wgGroupPermissions['*']['reupload']        = true;
  $wgGroupPermissions['*']['reupload-shared'] = true;
  $wgGroupPermissions['*']['rollback']        = true;
  $wgGroupPermissions['*']['siteadmin']       = true;
  $wgGroupPermissions['*']['trackback']       = true;
  $wgGroupPermissions['*']['unwatchedpages']  = true;
  $wgGroupPermissions['*']['upload']          = true;
  $wgGroupPermissions['*']['userrights']      = true;
  $wgGroupPermissions['*']['renameuser']      = true;
  $wgGroupPermissions['*']['makebot']         = true;
  $wgGroupPermissions['*']['makesysop']       = true;

  // Enable weird and wonderful options:
                              // Increase default error reporting level.
  error_reporting (E_ALL);    // At a later date could be increased to E_ALL | E_STRICT
  $wgBlockOpenProxies = true; // Some block pages require this to be true in order to test.
  $wgEnableUploads = true;    // enable uploads.
  //$wgUseTrackbacks = true;  // enable trackbacks; However this breaks the viewPageTest, so currently disabled.
  $wgDBerrorLog = "/root/mediawiki-db-error-log.txt";  // log DB errors, replace with suitable path.
  $wgShowSQLErrors = true;    // Show SQL errors (instead of saying the query was hidden).
  $wgShowExceptionDetails = true;  // want backtraces.
  $wgEnableAPI = true;        // enable API.
  $wgEnableWriteAPI = true;   // enable API.

  // Install & enable Parser Hook extensions to increase code coverage. E.g.:
  require_once("extensions/ParserFunctions/ParserFunctions.php");
  require_once("extensions/Cite/Cite.php");
  require_once("extensions/inputbox/inputbox.php");
  require_once("extensions/Sort/Sort.php");
  require_once("extensions/wikihiero/wikihiero.php");
  require_once("extensions/CharInsert/CharInsert.php");
  require_once("extensions/FixedImage/FixedImage.php");

  // Install & enable Special Page extensions to increase code coverage. E.g.:
  require_once("extensions/Cite/SpecialCite.php");
  require_once("extensions/Filepath/SpecialFilepath.php");
  require_once("extensions/Makebot/Makebot.php");
  require_once("extensions/Makesysop/SpecialMakesysop.php");
  require_once("extensions/Renameuser/SpecialRenameuser.php");
  require_once("extensions/LinkSearch/LinkSearch.php");
  // --------- End ---------
  
  If you want to try E_STRICT error logging, add this to the above:
  // --------- Start ---------
  error_reporting (E_ALL | E_STRICT);
  set_error_handler( 'error_handler' );
  function error_handler ($type, $message, $file=__FILE__, $line=__LINE__) {
     if ($message == "var: Deprecated. Please use the public/private/protected modifiers") return;
     print "<br />\n<b>Strict Standards:</b> Type: <b>$type</b>:  $message in <b>$file</b> on line <b>$line</b><br />\n";
  }
  // --------- End ---------

  Also add/change this in AdminSettings.php:
  // --------- Start ---------
  $wgEnableProfileInfo = true;
  $wgDBserver = "localhost"; // replace with DB server hostname
  // --------- End ---------

Usage:
  Run with "php fuzz-tester.php".
  To see the various command-line options, run "php fuzz-tester.php --help".
  To stop the script, press Ctrl-C.

Console output:
  - If requested, first any previously failed tests will be rerun.
  - Then new tests will be generated and run. Any tests that fail will be saved,
    and a brief message about why they failed will be printed on the console.
  - The console will show the number of tests run, time run, number of tests
    failed, number of tests being done per minute, and the name of the current test.

TODO:
  Some known things that could improve this script:
  - Logging in with cookie jar storage needed for some tests (as there are some 
    pages that cannot be tested without being logged in, and which are currently 
    untested - e.g. Special:Emailuser, Special:Preferences, adding to Watchist).
  - Testing of Timeline extension (I cannot test as ploticus has/had issues on
    my architecture).

*/

/////////////////////////// COMMAND LINE HELP ////////////////////////////////////

// This is a command line script, load MediaWiki env (gives command line options);
include('commandLine.inc');

// if the user asked for an explanation of command line options.
if ( isset( $options["help"] ) ) {
    print <<<ENDS
MediaWiki $wgVersion fuzz tester
Usage: php {$_SERVER["SCRIPT_NAME"]} [--quiet] [--base-url=<url-to-test-wiki>]
                           [--directory=<failed-test-path>] [--include-binary]
                           [--w3c-validate] [--delete-passed-retests] [--help]
                           [--user=<username>] [--password=<password>]
                           [--rerun-failed-tests] [--max-errors=<int>] 
                           [--max-runtime=<num-minutes>]
                           [--specific-test=<test-name>]

Options:
  --quiet                 : Hides passed tests, shows only failed tests.
  --base-url              : URL to a wiki on which to run the tests. 
                            The "http://" is optional and can be omitted.
  --directory             : Full path to directory for storing failed tests.
                            Will be created if it does not exist.
  --include-binary        : Includes non-alphanumeric characters in the tests.
  --w3c-validate          : Validates pages using the W3C's web validator. 
                            Slow. Currently many pages fail validation.
  --user                  : Login name of a valid user on your test wiki.
  --password              : Password for the valid user on your test wiki. 
  --delete-passed-retests : Will delete retests that now pass.
                            Requires --rerun-failed-tests to be meaningful.
  --rerun-failed-tests    : Whether to rerun any previously failed tests.
  --max-errors            : Maximum number of errors to report before exiting.
                            Does not include errors from --rerun-failed-tests
  --max-runtime           : Maximum runtime, in minutes, to run before exiting.
                            Only applies to new tests, not --rerun-failed-tests
  --specific-test         : Runs only the specified fuzz test. 
                            Only applies to new tests, not --rerun-failed-tests
  --keep-passed-tests     : Saves all test files, even those that pass.
  --help                  : Show this help message.

Example:
  If you wanted to fuzz test a nightly MediaWiki checkout using cron for 1 hour, 
  and only wanted to be informed of errors, and did not want to redo previously
  failed tests, and wanted a maximum of 100 errors, then you could do:
  php {$_SERVER["SCRIPT_NAME"]} --quiet --max-errors=100 --max-runtime=60


ENDS;

    exit( 0 );
}


// if we got command line options, check they look valid.
$validOptions = array ("quiet", "base-url", "directory", "include-binary",
        "w3c-validate", "user", "password", "delete-passed-retests",
        "rerun-failed-tests", "max-errors",
        "max-runtime", "specific-test", "keep-passed-tests", "help" );
if (!empty($options)) {
    $unknownArgs = array_diff (array_keys($options), $validOptions);
    foreach ($unknownArgs as $invalidArg) {
        print "Ignoring invalid command-line option: --$invalidArg\n";
    }
}


///////////////////////////// CONFIGURATION ////////////////////////////////////

// URL to some wiki on which we can run our tests.
if (!empty($options["base-url"])) {
    define("WIKI_BASE_URL", $options["base-url"]);
} else {
    define("WIKI_BASE_URL", $wgServer . $wgScriptPath . '/');
}

// The directory name where we store the output.
// Example for Windows: "c:\\temp\\wiki-fuzz"
if (!empty($options["directory"])) {
    define("DIRECTORY", $options["directory"] );
} else {
    define("DIRECTORY", "{$wgUploadDirectory}/fuzz-tests");
}

// Should our test fuzz data include binary strings?
define("INCLUDE_BINARY",  isset($options["include-binary"]) );

// Whether we want to validate HTML output on the web.
// At the moment very few generated pages will validate, so not recommended.
define("VALIDATE_ON_WEB", isset($options["w3c-validate"]) );
// URL to use to validate our output:
define("VALIDATOR_URL",  "http://validator.w3.org/check");

// Location of Tidy standalone executable.
define("PATH_TO_TIDY",  "/usr/bin/tidy");

// The name of a user who has edited on your wiki. Used 
// when testing the Special:Contributions and Special:Userlogin page.
if (!empty($options["user"])) {
    define("USER_ON_WIKI", $options["user"] );
} else {
    define("USER_ON_WIKI", "nickj");
}

// The password of the above user. Used when testing the login page,
// and to do this we sometimes need to login successfully. 
if (!empty($options["password"])) {
    define("USER_PASSWORD", $options["password"] );
} else {
    // And no, this is not a valid password on any public wiki.
    define("USER_PASSWORD", "nickj");
}

// If we have a test that failed, and then we run it again, and it passes,
// do you want to delete it or keep it?
define("DELETE_PASSED_RETESTS", isset($options["delete-passed-retests"]) );

// Do we want to rerun old saved tests at script startup?
// Set to true to help catch regressions, or false if you only want new stuff.
define("RERUN_OLD_TESTS", isset($options["rerun-failed-tests"]) );

// File where the database errors are logged. Should be defined in LocalSettings.php.
define("DB_ERROR_LOG_FILE", $wgDBerrorLog );

// Run in chatty mode (all output, default), or run in quiet mode (only prints out details of failed tests)?
define("QUIET", isset($options["quiet"]) );

// Keep all test files, even those that pass. Potentially useful to tracking input that causes something
// unusual to happen, if you don't know what "unusual" is until later.
define("KEEP_PASSED_TESTS", isset($options["keep-passed-tests"]) );

// The maximum runtime, if specified.
if (!empty($options["max-runtime"]) && intval($options["max-runtime"])>0) {
    define("MAX_RUNTIME", intval($options["max-runtime"]) );
}

// The maximum number of problems to find, if specified. Excludes retest errors.
if (!empty($options["max-errors"]) && intval($options["max-errors"])>0) {
    define("MAX_ERRORS", intval($options["max-errors"]) );
}

// if the user has requested a specific test (instead of all tests), and the test they asked for looks valid.
if (!empty($options["specific-test"])) {
    if (class_exists($options["specific-test"]) && get_parent_class($options["specific-test"])=="pageTest") {
        define("SPECIFIC_TEST", $options["specific-test"] );
    }
    else {
        print "Ignoring invalid --specific-test\n";
    }
}

// Define the file extensions we'll use:
define("PHP_TEST" , ".test.php");
define("CURL_TEST", ".curl.sh" );
define("DATA_FILE", ".data.bin");
define("INFO_FILE", ".info.txt");
define("HTML_FILE", ".wiki_preview.html");

// If it goes wrong, we want to know about it.
error_reporting(E_ALL | E_STRICT);

////////////////  A CLASS THAT GENERATES RANDOM NASTY WIKI & HTML STRINGS  //////////////////////

class wikiFuzz {

    // Only some HTML tags are understood with params by MediaWiki, the rest are ignored.
    // List the tags that accept params below, as well as what those params are.
    public static $data = array(
            "B"          => array("CLASS", "ID", "STYLE", "lang", "dir", "title"),
            "CAPTION"    => array("CLASS", "ID", "STYLE", "align", "lang", "dir", "title"),
            "CENTER"     => array("CLASS", "STYLE", "ID", "lang", "dir", "title"),
            "DIV"        => array("CLASS", "STYLE", "ID", "align", "lang", "dir", "title"),
            "FONT"       => array("CLASS", "STYLE", "ID", "lang", "dir", "title", "face", "size", "color"),
            "H1"         => array("STYLE", "CLASS", "ID", "align", "lang", "dir", "title"),
            "H2"         => array("STYLE", "CLASS", "ID", "align", "lang", "dir", "title"),
            "HR"         => array("STYLE", "CLASS", "ID", "WIDTH", "lang", "dir", "title", "size", "noshade"),
            "LI"         => array("CLASS", "ID", "STYLE", "lang", "dir", "title", "type", "value"),
            "TABLE"      => array("STYLE", "CLASS", "ID", "BGCOLOR", "WIDTH", "ALIGN", "BORDER", "CELLPADDING", 
                                   "CELLSPACING", "lang", "dir", "title", "summary", "frame", "rules"),
            "TD"         => array("STYLE", "CLASS", "ID", "BGCOLOR", "WIDTH", "ALIGN", "COLSPAN", "ROWSPAN",
                                  "VALIGN", "abbr", "axis", "headers", "scope", "nowrap", "height", "lang",
                                  "dir", "title", "char", "charoff"),
            "TH"         => array("STYLE", "CLASS", "ID", "BGCOLOR", "WIDTH", "ALIGN", "COLSPAN", "ROWSPAN",
                                  "VALIGN", "abbr", "axis", "headers", "scope", "nowrap", "height", "lang", 
                                  "dir", "title", "char", "charoff"),
            "TR"         => array("CLASS", "STYLE", "ID", "BGCOLOR", "ALIGN", "VALIGN", "lang", "dir", "title", "char", "charoff"),
            "UL"         => array("CLASS", "STYLE", "ID", "lang", "dir", "title", "type"),
            "P"          => array("style", "class", "id", "align", "lang", "dir", "title"),
            "blockquote" => array("CLASS", "ID", "STYLE", "lang", "dir", "title", "cite"),
            "span"       => array("CLASS", "ID", "STYLE", "align", "lang", "dir", "title"),
            "code"       => array("CLASS", "ID", "STYLE", "lang", "dir", "title"),
            "tt"         => array("CLASS", "ID", "STYLE", "lang", "dir", "title"),
            "small"      => array("CLASS", "ID", "STYLE", "lang", "dir", "title"),
            "big"        => array("CLASS", "ID", "STYLE", "lang", "dir", "title"),
            "s"          => array("CLASS", "ID", "STYLE", "lang", "dir", "title"),
            "u"          => array("CLASS", "ID", "STYLE", "lang", "dir", "title"),
            "del"        => array("CLASS", "ID", "STYLE", "lang", "dir", "title", "datetime", "cite"),
            "ins"        => array("CLASS", "ID", "STYLE", "lang", "dir", "title", "datetime", "cite"),
            "sub"        => array("CLASS", "ID", "STYLE", "lang", "dir", "title"),
            "sup"        => array("CLASS", "ID", "STYLE", "lang", "dir", "title"),
            "ol"         => array("CLASS", "ID", "STYLE", "lang", "dir", "title", "type", "start"),
            "br"         => array("CLASS", "ID", "STYLE", "title", "clear"),
            "cite"       => array("CLASS", "ID", "STYLE", "lang", "dir", "title"),
            "var"        => array("CLASS", "ID", "STYLE", "lang", "dir", "title"),
            "dl"         => array("CLASS", "ID", "STYLE", "lang", "dir", "title"),
            "ruby"       => array("CLASS", "ID", "STYLE", "lang", "dir", "title"),
            "rt"         => array("CLASS", "ID", "STYLE", "lang", "dir", "title"),
            "rp"         => array("CLASS", "ID", "STYLE", "lang", "dir", "title"),
            "dt"         => array("CLASS", "ID", "STYLE", "lang", "dir", "title"),
            "dl"         => array("CLASS", "ID", "STYLE", "lang", "dir", "title"),
            "em"         => array("CLASS", "ID", "STYLE", "lang", "dir", "title"),
            "strong"     => array("CLASS", "ID", "STYLE", "lang", "dir", "title"),
            "i"          => array("CLASS", "ID", "STYLE", "lang", "dir", "title"),
            "thead"      => array("CLASS", "ID", "STYLE", "lang", "dir", "title",  'align', 'char', 'charoff', 'valign'),
            "tfoot"      => array("CLASS", "ID", "STYLE", "lang", "dir", "title",  'align', 'char', 'charoff', 'valign'),
            "tbody"      => array("CLASS", "ID", "STYLE", "lang", "dir", "title",  'align', 'char', 'charoff', 'valign'),
            "colgroup"   => array("CLASS", "ID", "STYLE", "lang", "dir", "title",  'align', 'char', 'charoff', 'valign', 'span', 'width'),
            "col"        => array("CLASS", "ID", "STYLE", "lang", "dir", "title",  'align', 'char', 'charoff', 'valign', 'span', 'width'),
            "pre"        => array("CLASS", "ID", "STYLE", "lang", "dir", "title", "width"),

            // extension tags that accept parameters:
            "sort"         => array("order", "class"),
            "ref"          => array("name"),
            "categorytree" => array("hideroot", "mode", "style"),
            "chemform"     => array("link", "wikilink", "query"),
            "section"      => array("begin", "new"),

            // older MW transclusion.
            "transclude"   => array("page"),            
                );

    // The types of the HTML that we will be testing were defined above
    // Note: this needs to be initialized later to be equal to: array_keys(wikiFuzz::$data);
    // as such, it also needs to also be publicly modifiable.
    public static $types;


    // Some attribute values.
    static private $other = array("&","=",":","?","\"","\n","%n%n%n%n%n%n%n%n%n%n%n%n","\\");
    static private $ints  = array(
            // various numbers
            "0","-1","127","-7897","89000","808080","90928345",
            "0xfffffff","ffff",

            // Different ways of saying: '
            "&#0000039;", // Long UTF-8 Unicode encoding
            "&#39;",  // dec version.
            "&#x27;", // hex version.
            "&#xA7;", // malformed hex variant, MSB not zero.

            // Different ways of saying: "
            "&#0000034;", // Long UTF-8 Unicode encoding
            "&#34;",
            "&#x22;", // hex version.
            "&#xA2;", // malformed hex variant, MSB not zero.

            // Different ways of saying: <
            "<",
            "&#0000060",  // Long UTF-8 Unicode encoding without semicolon (Mediawiki wants the colon)
            "&#0000060;", // Long UTF-8 Unicode encoding with semicolon
            "&#60;",
            "&#x3C;",     // hex version.
            "&#xBC;",     // malformed hex variant, MSB not zero.
            "&#x0003C;",  // mid-length hex version
            "&#X00003C;", // slightly longer hex version, with capital "X"

            // Different ways of saying: >
            ">",
            "&#0000062;", // Long UTF-8 Unicode encoding
            "&#62;",
            "&#x3E;",     // hex version.
            "&#xBE;",     // malformed variant, MSB not zero.

            // Different ways of saying: [
            "&#0000091;", // Long UTF-8 Unicode encoding
            "&#91;",
            "&#x5B;",     // hex version.

            // Different ways of saying: {{
            "&#0000123;&#0000123;", // Long UTF-8 Unicode encoding
            "&#123;&#123;",
            "&#x7B;&#x7B;",         // hex version.

            // Different ways of saying: |
            "&#0000124;", // Long UTF-8 Unicode encoding
            "&#124;",
            "&#x7C;",     // hex version.
            "&#xFC;",     // malformed hex variant, MSB not zero.

            // a "lignature" - http://www.robinlionheart.com/stds/html4/spchars#ligature
            "&zwnj;"
                );

    // Defines various wiki-related bits of syntax, that can potentially cause 
    // MediaWiki to do something other than just print that literal text.
    static private $ext = array(
            // links, templates, parameters.
            "[[", "]]", "{{", "}}", "|", "[", "]", "{{{", "}}}", "|]]", 

            // wiki tables.
            "\n{|", "\n|}",
            "!",
            "\n!",
            "!!",
            "||",
            "\n|-", "| ", "\n|",

            // section headings.
            "=", "==", "===", "====", "=====", "======",

            // lists (ordered and unordered) and indentation.
            "\n*", "*", "\n:", ":", 
            "\n#", "#",

            // definition lists (dl, dt, dd), newline, and newline with pre, and a tab.
            "\n;", ";", "\n ",

            // Whitespace: newline, tab, space.
            "\n", "\t", " ",

            // Some XSS attack vectors from http://ha.ckers.org/xss.html 
            "&#x09;", // tab
            "&#x0A;", // newline
            "&#x0D;", // carriage return
            "\0",     // null character
            " &#14; ", // spaces and meta characters
            "'';!--\"<XSS>=&{()}", // compact injection of XSS & SQL tester
            
            // various NULL fields
            "%00",
            "&#00;",
            "\0",

            // horizontal rule.
            "-----", "\n-----",

            // signature, redirect, bold, italics.
            "~~~~", "#REDIRECT [[", "'''", "''", 

            // comments.
            "<!--", "-->", 

            // quotes.
            "\"", "'",

            // tag start and tag end.
            "<", ">",

            // implicit link creation on URIs.
            "http://",
            "https://",
            "ftp://",
            "irc://",
            "news:",
            'gopher://',
            'telnet://',
            'nntp://',
            'worldwind://',
            'mailto:',

            // images.
            "[[image:",
            ".gif",
            ".png",
            ".jpg",
            ".jpeg",
            'thumbnail=',
            'thumbnail',
            'thumb=',
            'thumb',
            'right',
            'none',
            'left',
            'framed',
            'frame',
            'enframed',
            'centre',
            'center',
            "Image:",
            "[[:Image",
            'px',
            'upright=',
            'border',

            // misc stuff to throw at the Parser.
            '%08X',
            '/',
            ":x{|",
            "\n|+",
            "<noinclude>",
            "</noinclude>",
            " \302\273",
            " :",
            " !",
            " ;",
            "\302\253",
            "[[category:",
            "?=",
            "(",
            ")",
            "]]]",
            "../",
            "{{{{",
            "}}}}",
            "[[Special:",
            "<includeonly>",
            "</includeonly>",
            "<!--MWTEMPLATESECTION=",
            '<!--MWTOC-->',

            // implicit link creation on booknum, RFC, and PubMed ID usage (both with and without IDs)
            "ISBN 2",
            "RFC 000",
            "PMID 000",
            "ISBN ",
            "RFC ",
            "PMID ",

            // magic words:
            '__NOTOC__',
            '__FORCETOC__',
            '__NOEDITSECTION__',
            '__START__',
            '__NOTITLECONVERT__',
            '__NOCONTENTCONVERT__',
            '__END__',
            '__TOC__',
            '__NOTC__',
            '__NOCC__',
            "__FORCETOC__",
            "__NEWSECTIONLINK__",
            "__NOGALLERY__",

            // more magic words / internal templates.
            '{{PAGENAME}}',
            '{{PAGENAMEE}}',
            '{{NAMESPACE}}',
            "{{MSG:",
            "}}",
            "{{MSGNW:",
            "}}",
            "{{INT:",
            "}}",
            '{{SITENAME}}',        
            "{{NS:",        
            "}}",
            "{{LOCALURL:",        
            "}}",
            "{{LOCALURLE:",        
            "}}",
            "{{SCRIPTPATH}}",        
            "{{GRAMMAR:gentiv|",        
            "}}",
            "{{REVISIONID}}",
            "{{SUBPAGENAME}}",
            "{{SUBPAGENAMEE}}",
            "{{ns:0}}",
            "{{fullurle:",
            "}}",
            "{{subst::",
            "}}",
            "{{UCFIRST:",
            "}}",
            "{{UC:",
            '{{SERVERNAME}}',
            '{{SERVER}}',
            "{{RAW:",
            "}}",
            "{{PLURAL:",
            "}}",
            "{{LCFIRST:",
            "}}",
            "{{LC:",
            "}}",
            '{{CURRENTWEEK}}',
            '{{CURRENTDOW}}',
            "{{INT:{{LC:contribs-showhideminor}}|",
            "}}",
            "{{INT:googlesearch|",
            "}}",
            "{{BASEPAGENAME}}",
            "{{CONTENTLANGUAGE}}",
            "{{PAGESINNAMESPACE:}}",
            "{{#language:",
            "}}",
            "{{#special:",
            "}}",
            "{{#special:emailuser",
            "}}",

            // Some raw link for magic words.
            "{{NUMBEROFPAGES:R",
            "}}",
            "{{NUMBEROFUSERS:R",
            "}}",
            "{{NUMBEROFARTICLES:R",
            "}}",
            "{{NUMBEROFFILES:R",
            "}}",
            "{{NUMBEROFADMINS:R",
            "}}",
            "{{padleft:",
            "}}",
            "{{padright:",
            "}}",
            "{{DEFAULTSORT:",
            "}}",

            // internal Math "extension":
            "<math>",
            "</math>",

            // Parser extension functions:
            "{{#expr:",
            "{{#if:",
            "{{#ifeq:",
            "{{#ifexist:",
            "{{#ifexpr:",
            "{{#switch:",
            "{{#time:",
            "}}",

            // references table for the Cite extension.
            "<references/>",

            // Internal Parser tokens - try inserting some of these.
            "UNIQ25f46b0524f13e67NOPARSE",
            "UNIQ17197916557e7cd6-HTMLCommentStrip46238afc3bb0cf5f00000002",
            "\x07UNIQ17197916557e7cd6-HTMLCommentStrip46238afc3bb0cf5f00000002-QINU",

            // Inputbox extension:
            "<inputbox>\ntype=search\nsearchbuttonlabel=\n",
            "</inputbox>",

            // charInsert extension:
            "<charInsert>",
            "</charInsert>",

            // wikiHiero extension:
            "<hiero>",
            "</hiero>",

            // Image gallery:
            "<gallery>",
            "</gallery>",

            // FixedImage extension.
            "<fundraising/>",

            // Timeline extension: currently untested.

            // Nowiki:
            "<nOwIkI>",
            "</nowiki>",

            // an external image to test the external image displaying code
            "http://debian.org/Pics/debian.png",

            // LabeledSectionTransclusion extension.
            "{{#lstx:",
            "}}",
            "{{#lst:",
            "}}",
            "{{#lst:Main Page|",
            "}}"
            );

    /**
     ** Randomly returns one element of the input array.
     */
    static public function chooseInput(array $input) {
        $randindex = wikiFuzz::randnum(count($input) - 1);
        return $input[$randindex];
    }

    // Max number of parameters for HTML attributes.
    static private $maxparams = 10;

    /** 
     ** Returns random number between finish and start.
     */
    static public function randnum($finish,$start=0) {
        return mt_rand($start,$finish);
    }

    /**
     ** Returns a mix of random text and random wiki syntax.
     */
    static private function randstring() {
        $thestring = "";

        for ($i=0; $i<40; $i++) {
            $what = wikiFuzz::randnum(1);

            if ($what == 0) { // include some random wiki syntax
                $which = wikiFuzz::randnum(count(wikiFuzz::$ext) - 1);
                $thestring .= wikiFuzz::$ext[$which];
            }
            else { // include some random text
                $char = INCLUDE_BINARY 
                    // Decimal version:
                    // "&#" . wikiFuzz::randnum(255) . ";"
                    // Hex version:
                    ? "&#x" . str_pad(dechex(wikiFuzz::randnum(255)), wikiFuzz::randnum(2, 7), "0", STR_PAD_LEFT) . ";" 
                    // A truly binary version:
                    // ? chr(wikiFuzz::randnum(0,255))
                    : chr(wikiFuzz::randnum(126,32));

                $length = wikiFuzz::randnum(8);
                $thestring .= str_repeat ($char, $length);
            }
        }
        return $thestring;
    }

    /**
     ** Returns either random text, or random wiki syntax, or random data from "ints",
     **        or random data from "other".
     */
    static private function makestring() {
        $what = wikiFuzz::randnum(2);
        if ($what == 0) {
            return wikiFuzz::randstring();
        }
        elseif ($what == 1) {
            return wikiFuzz::$ints[wikiFuzz::randnum(count(wikiFuzz::$ints) - 1)];
        }
        else {
            return wikiFuzz::$other[wikiFuzz::randnum(count(wikiFuzz::$other) - 1)];
        }
    }


    /**
     ** Strips out the stuff that Mediawiki balks at in a page's title.
     **        Implementation copied/pasted from cleanupTable.inc & cleanupImages.php
     */
    static public function makeTitleSafe($str) {
        $legalTitleChars = " %!\"$&'()*,\\-.\\/0-9:;=?@A-Z\\\\^_`a-z~\\x80-\\xFF";
        return preg_replace_callback(
                "/([^$legalTitleChars])/",
                create_function(
                    // single quotes are essential here,
                    // or alternative escape all $ as \$
                    '$matches',
                    'return sprintf( "\\x%02x", ord( $matches[1] ) );'
                    ),
                $str );
    }

    /**
     ** Returns a string of fuzz text.
     */
    static private function loop() {
        switch ( wikiFuzz::randnum(3) ) {
            case 1: // an opening tag, with parameters.
                $string = "";
                $i = wikiFuzz::randnum(count(wikiFuzz::$types) - 1);
                $t = wikiFuzz::$types[$i];
                $arr = wikiFuzz::$data[$t];
                $string .= "<" . $t . " ";
                $num_params = min(wikiFuzz::$maxparams, count($arr));
                for ($z=0; $z<$num_params; $z++) {
                    $badparam = $arr[wikiFuzz::randnum(count($arr) - 1)];
                    $badstring = wikiFuzz::makestring();
                    $string .= $badparam . "=" . wikiFuzz::getRandQuote() . $badstring . wikiFuzz::getRandQuote() . " ";
                }
                $string .= ">\n";
                return $string;
            case 2: // a closing tag.
                $i = wikiFuzz::randnum(count(wikiFuzz::$types) - 1);
                return "</". wikiFuzz::$types[$i] . ">"; 
            case 3: // a random string, between tags.
                return wikiFuzz::makeString();
        }
        return "";    // catch-all, should never be called.
    }

    /**
     ** Returns one of the three styles of random quote: ', ", and nothing.
     */
    static private function getRandQuote() {
        switch ( wikiFuzz::randnum(3) ) {
            case 1 : return "'";
            case 2 : return "\"";
            default: return "";
        }
    }

    /**
     ** Returns fuzz text, with the parameter indicating approximately how many lines of text you want.
     */
    static public function makeFuzz($maxtypes = 2) {
        $page = "";
        for ($k=0; $k<$maxtypes; $k++) {
            $page .= wikiFuzz::loop();
        }
        return $page;
    }
}


////////   MEDIAWIKI PAGES TO TEST, AND HOW TO TEST THEM  ///////

/**
 ** A page test has just these things:
 **        1) Form parameters.
 **        2) the URL we are going to test those parameters on.
 **        3) Any cookies required for the test.
 **        4) Whether Tidy should validate the page. Defaults to true, but can be turned off.
 **        Declared abstract because it should be extended by a class 
 **        that supplies these parameters.
 */
abstract class pageTest {
    protected $params;
    protected $pagePath;
    protected $cookie = "";
    protected $tidyValidate = true;

    public function getParams() {
        return $this->params;
    }

    public function getPagePath() {
        return $this->pagePath;
    }

    public function getCookie() {
        return $this->cookie;
    }
    
    public function tidyValidate() {
    	return $this->tidyValidate;
    }
}


/**
 ** a page test for the "Edit" page. Tests Parser.php and Sanitizer.php.
 */
class editPageTest extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=WIKIFUZZ";

        $this->params = array (
                "action"        => "submit",
                "wpMinoredit"   => wikiFuzz::makeFuzz(2),
                "wpPreview"     => wikiFuzz::makeFuzz(2),
                "wpSection"     => wikiFuzz::makeFuzz(2),
                "wpEdittime"    => wikiFuzz::makeFuzz(2),
                "wpSummary"     => wikiFuzz::makeFuzz(2),
                "wpScrolltop"   => wikiFuzz::makeFuzz(2),
                "wpStarttime"   => wikiFuzz::makeFuzz(2),
                "wpAutoSummary" => wikiFuzz::makeFuzz(2),
                "wpTextbox1"    => wikiFuzz::makeFuzz(40)  // the main wiki text, need lots of this.
                );

        // sometimes we don't want to specify certain parameters.
        if (wikiFuzz::randnum(6) == 0) unset($this->params["wpSection"]);
        if (wikiFuzz::randnum(6) == 0) unset($this->params["wpEdittime"]);
        if (wikiFuzz::randnum(6) == 0) unset($this->params["wpSummary"]);
        if (wikiFuzz::randnum(6) == 0) unset($this->params["wpScrolltop"]);
        if (wikiFuzz::randnum(6) == 0) unset($this->params["wpStarttime"]);
        if (wikiFuzz::randnum(6) == 0) unset($this->params["wpAutoSummary"]);
        if (wikiFuzz::randnum(6) == 0) unset($this->params["wpTextbox1"]);
    }
}


/**
 ** a page test for "Special:Listusers".
 */
class listusersTest extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Listusers";

        $this->params = array (
                "title"        => wikiFuzz::makeFuzz(2),
                "group"        => wikiFuzz::makeFuzz(2),
                "username"     => wikiFuzz::makeFuzz(2),
                "Go"           => wikiFuzz::makeFuzz(2),
                "limit"        => wikiFuzz::chooseInput( array("0", "-1", "---'----------0", "+1", "8134", wikiFuzz::makeFuzz(2)) ),
                "offset"       => wikiFuzz::chooseInput( array("0", "-1", "--------'-----0", "+1", "81343242346234234", wikiFuzz::makeFuzz(2)) )
                );
    }
}


/**
 ** a page test for "Special:Search".
 */
class searchTest extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Search";

        $this->params = array (
                "action"        => "index.php?title=Special:Search",
                "ns0"           => wikiFuzz::makeFuzz(2),
                "ns1"           => wikiFuzz::makeFuzz(2),
                "ns2"           => wikiFuzz::makeFuzz(2),
                "ns3"           => wikiFuzz::makeFuzz(2),
                "ns4"           => wikiFuzz::makeFuzz(2),
                "ns5"           => wikiFuzz::makeFuzz(2),
                "ns6"           => wikiFuzz::makeFuzz(2),
                "ns7"           => wikiFuzz::makeFuzz(2),
                "ns8"           => wikiFuzz::makeFuzz(2),
                "ns9"           => wikiFuzz::makeFuzz(2),
                "ns10"          => wikiFuzz::makeFuzz(2),
                "ns11"          => wikiFuzz::makeFuzz(2),
                "ns12"          => wikiFuzz::makeFuzz(2),
                "ns13"          => wikiFuzz::makeFuzz(2),
                "ns14"          => wikiFuzz::makeFuzz(2),
                "ns15"          => wikiFuzz::makeFuzz(2),
                "redirs"        => wikiFuzz::makeFuzz(2),
                "search"        => wikiFuzz::makeFuzz(2),
                "offset"        => wikiFuzz::chooseInput( array("", "0", "-1", "--------'-----0", "+1", "81343242346234234", wikiFuzz::makeFuzz(2)) ),
                "fulltext"      => wikiFuzz::chooseInput( array("", "0", "1", "--------'-----0", "+1", wikiFuzz::makeFuzz(2)) ),
                "searchx"       => wikiFuzz::chooseInput( array("", "0", "1", "--------'-----0", "+1", wikiFuzz::makeFuzz(2)) )
                    );
    }
}


/**
 ** a page test for "Special:Recentchanges".
 */
class recentchangesTest extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Recentchanges";

        $this->params = array (
                "action"        => wikiFuzz::makeFuzz(2),
                "title"         => wikiFuzz::makeFuzz(2),
                "namespace"     => wikiFuzz::chooseInput( range(-1, 15) ),
                "Go"            => wikiFuzz::makeFuzz(2),
                "invert"        => wikiFuzz::chooseInput( array("-1", "---'----------0", "+1", "8134", wikiFuzz::makeFuzz(2)) ),
                "hideanons"     => wikiFuzz::chooseInput( array("-1", "------'-------0", "+1", "8134", wikiFuzz::makeFuzz(2)) ),
                'limit'         => wikiFuzz::chooseInput( array("0", "-1", "---------'----0", "+1", "81340909772349234",  wikiFuzz::makeFuzz(2)) ),
                "days"          => wikiFuzz::chooseInput( array("-1", "----------'---0", "+1", "8134", wikiFuzz::makeFuzz(2)) ),
                "hideminor"     => wikiFuzz::chooseInput( array("-1", "-----------'--0", "+1", "8134", wikiFuzz::makeFuzz(2)) ),
                "hidebots"      => wikiFuzz::chooseInput( array("-1", "---------'----0", "+1", "8134", wikiFuzz::makeFuzz(2)) ),
                "hideliu"       => wikiFuzz::chooseInput( array("-1", "-------'------0", "+1", "8134", wikiFuzz::makeFuzz(2)) ),
                "hidepatrolled" => wikiFuzz::chooseInput( array("-1", "-----'--------0", "+1", "8134", wikiFuzz::makeFuzz(2)) ),
                "hidemyself"    => wikiFuzz::chooseInput( array("-1", "--'-----------0", "+1", "8134", wikiFuzz::makeFuzz(2)) ),
                'categories_any'=> wikiFuzz::chooseInput( array("-1", "--'-----------0", "+1", "8134", wikiFuzz::makeFuzz(2)) ),
                'categories'    => wikiFuzz::chooseInput( array("-1", "--'-----------0", "+1", "8134", wikiFuzz::makeFuzz(2)) ),
                'feed'          => wikiFuzz::chooseInput( array("-1", "--'-----------0", "+1", "8134", wikiFuzz::makeFuzz(2)) )
                );
    }
}


/**
 ** a page test for "Special:Prefixindex".
 */
class prefixindexTest extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Prefixindex";

        $this->params = array (
                "title"         => "Special:Prefixindex",
                "namespace"     => wikiFuzz::randnum(-10,101),
                "Go"            => wikiFuzz::makeFuzz(2)
                );

        // sometimes we want 'prefix', sometimes we want 'from', and sometimes we want nothing.
        if (wikiFuzz::randnum(3) == 0) {
            $this->params["prefix"] = wikiFuzz::chooseInput( array("-1", "-----'--------0", "+++--+1",
                                                 wikiFuzz::randnum(-10,8134), wikiFuzz::makeFuzz(2)) );
        }
        if (wikiFuzz::randnum(3) == 0) {
            $this->params["from"]   = wikiFuzz::chooseInput( array("-1", "-----'--------0", "+++--+1", 
                                                wikiFuzz::randnum(-10,8134), wikiFuzz::makeFuzz(2)) );
        }
    }
}


/**
 ** a page test for "Special:MIMEsearch".
 */
class mimeSearchTest extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:MIMEsearch";

        $this->params = array (
                "action"        => "index.php?title=Special:MIMEsearch",
                "mime"          => wikiFuzz::makeFuzz(3),
                'limit'         => wikiFuzz::chooseInput( array("0", "-1", "-------'------0", "+1", "81342321351235325",  wikiFuzz::makeFuzz(2)) ),
                'offset'        => wikiFuzz::chooseInput( array("0", "-1", "-----'--------0", "+1", "81341231235365252234324",  wikiFuzz::makeFuzz(2)) )
                );
    }
}


/**
 ** a page test for "Special:Log".
 */
class specialLogTest extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Log";

        $this->params = array (
                "type"        => wikiFuzz::chooseInput( array("", wikiFuzz::makeFuzz(2)) ),
                "par"         => wikiFuzz::makeFuzz(2),
                "user"        => wikiFuzz::makeFuzz(2),
                "page"        => wikiFuzz::makeFuzz(2),
                "from"        => wikiFuzz::makeFuzz(2),
                "until"       => wikiFuzz::makeFuzz(2),
                "title"       => wikiFuzz::makeFuzz(2)
                );
    }
}


/**
 ** a page test for "Special:Userlogin", with a successful login.
 */
class successfulUserLoginTest extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Userlogin&action=submitlogin&type=login&returnto=" . wikiFuzz::makeFuzz(2);

        $this->params = array (
                "wpName"          => USER_ON_WIKI,
                // sometimes real password, sometimes not:
                'wpPassword'      => wikiFuzz::chooseInput( array( wikiFuzz::makeFuzz(2), USER_PASSWORD ) ),
                'wpRemember'      => wikiFuzz::makeFuzz(2)
                );

        $this->cookie = "wikidb_session=" .  wikiFuzz::chooseInput( array("1" , wikiFuzz::makeFuzz(2) ) );
    }
}


/**
 ** a page test for "Special:Userlogin".
 */
class userLoginTest extends pageTest {
    function __construct() {

        $this->pagePath = "index.php?title=Special:Userlogin";

        $this->params = array (
                'wpRetype'        => wikiFuzz::makeFuzz(2),
                'wpRemember'      => wikiFuzz::makeFuzz(2),
                'wpRealName'      => wikiFuzz::makeFuzz(2),
                'wpPassword'      => wikiFuzz::makeFuzz(2),
                'wpName'          => wikiFuzz::makeFuzz(2),
                'wpMailmypassword'=> wikiFuzz::makeFuzz(2),
                'wpLoginattempt'  => wikiFuzz::makeFuzz(2),
                'wpEmail'         => wikiFuzz::makeFuzz(2),
                'wpDomain'        => wikiFuzz::chooseInput( array("", "local", wikiFuzz::makeFuzz(2)) ),
                'wpCreateaccountMail' => wikiFuzz::chooseInput( array("", wikiFuzz::makeFuzz(2)) ),
                'wpCreateaccount' => wikiFuzz::chooseInput( array("", wikiFuzz::makeFuzz(2)) ),
                'wpCookieCheck'   => wikiFuzz::chooseInput( array("", wikiFuzz::makeFuzz(2)) ),
                'type'            => wikiFuzz::chooseInput( array("signup", "login", "", wikiFuzz::makeFuzz(2)) ),
                'returnto'        => wikiFuzz::makeFuzz(2),
                'action'          => wikiFuzz::chooseInput( array("", "submitlogin", wikiFuzz::makeFuzz(2)) )
                );

        $this->cookie = "wikidb_session=" . wikiFuzz::chooseInput( array("1" , wikiFuzz::makeFuzz(2) ) );
    }
}


/**
 ** a page test for "Special:Ipblocklist" (also includes unblocking)
 */
class ipblocklistTest extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Ipblocklist";

        $this->params = array (
                'wpUnblockAddress'=> wikiFuzz::makeFuzz(2),
                'ip'              => wikiFuzz::chooseInput( array("20398702394", "", "Nickj2", wikiFuzz::makeFuzz(2),
                                     // something like an IP address, sometimes invalid:
                                     ( wikiFuzz::randnum(300,-20) . "." . wikiFuzz::randnum(300,-20) . "."
                                       . wikiFuzz::randnum(300,-20) . "." .wikiFuzz::randnum(300,-20) ) ) ),
                'id'              => wikiFuzz::makeFuzz(2),
                'wpUnblockReason' => wikiFuzz::makeFuzz(2),
                'action'          => wikiFuzz::chooseInput( array(wikiFuzz::makeFuzz(2), "success", "submit", "unblock") ),
                'wpEditToken'     => wikiFuzz::makeFuzz(2),
                'wpBlock'         => wikiFuzz::chooseInput( array(wikiFuzz::makeFuzz(2), "") ),
                'limit'           => wikiFuzz::chooseInput( array("0", "-1", "--------'-----0", "+1", 
                                                 "09700982312351132098234",  wikiFuzz::makeFuzz(2)) ),
                'offset'          => wikiFuzz::chooseInput( array("0", "-1", "------'-------0", "+1", 
                                                 "09700980982341535324234234", wikiFuzz::makeFuzz(2)) )
                );

        // sometimes we don't want to specify certain parameters.
        if (wikiFuzz::randnum(4) == 0) unset($this->params["action"]);
        if (wikiFuzz::randnum(3) == 0) unset($this->params["ip"]);
        if (wikiFuzz::randnum(2) == 0) unset($this->params["id"]);
        if (wikiFuzz::randnum(3) == 0) unset($this->params["wpUnblockAddress"]);
    }
}


/**
 ** a page test for "Special:Newimages".
 */
class newImagesTest extends  pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Newimages";

        $this->params = array (
                'hidebots'  => wikiFuzz::chooseInput( array(wikiFuzz::makeFuzz(2), "1", "", "-1") ),
                'wpIlMatch' => wikiFuzz::makeFuzz(2),
                'until'     => wikiFuzz::makeFuzz(2),
                'from'      => wikiFuzz::makeFuzz(2)
                );

        // sometimes we don't want to specify certain parameters.
        if (wikiFuzz::randnum(6) == 0) unset($this->params["until"]);
        if (wikiFuzz::randnum(6) == 0) unset($this->params["from"]);
    }
}


/**
 ** a page test for the "Special:Imagelist" page.
 */
class imagelistTest extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Imagelist";

        $this->params = array (
                'sort'      => wikiFuzz::chooseInput( array("bysize", "byname" , "bydate", wikiFuzz::makeFuzz(2)) ),
                'limit'     => wikiFuzz::chooseInput( array("0", "-1", "--------'-----0", "+1", "09700982312351132098234",  wikiFuzz::makeFuzz(2)) ),
                'offset'    => wikiFuzz::chooseInput( array("0", "-1", "------'-------0", "+1", "09700980982341535324234234", wikiFuzz::makeFuzz(2)) ),
                'wpIlMatch' => wikiFuzz::makeFuzz(2)
                );
    }
}


/**
 ** a page test for "Special:Export".
 */
class specialExportTest extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Export";

        $this->params = array (
                'action'      => wikiFuzz::chooseInput( array("submit", "", wikiFuzz::makeFuzz(2)) ),
                'pages'       => wikiFuzz::makeFuzz(2),
                'curonly'     => wikiFuzz::chooseInput( array("", "0", "-1", wikiFuzz::makeFuzz(2)) ),
                'listauthors' => wikiFuzz::chooseInput( array("", "0", "-1", wikiFuzz::makeFuzz(2)) ),
                'history'     => wikiFuzz::chooseInput( array("0", "-1", "------'-------0", "+1", "09700980982341535324234234", wikiFuzz::makeFuzz(2)) ),

                );

        // For the time being, need to disable "submit" action as Tidy barfs on MediaWiki's XML export.
        if ($this->params['action'] == 'submit') $this->params['action'] = '';

        // Sometimes remove the history field.
        if (wikiFuzz::randnum(2) == 0) unset($this->params["history"]);
        
        // page does not produce HTML.
        $this->tidyValidate = false; 
    }
}


/**
 ** a page test for "Special:Booksources".
 */
class specialBooksourcesTest extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Booksources";

        $this->params = array (
                'go'    => wikiFuzz::makeFuzz(2),
                // ISBN codes have to contain some semi-numeric stuff or will be ignored:
                'isbn'  => "0X0" . wikiFuzz::makeFuzz(2)
                );
    }
}


/**
 ** a page test for "Special:Allpages".
 */
class specialAllpagesTest extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special%3AAllpages";

        $this->params = array (
                'from'      => wikiFuzz::makeFuzz(2),
                'namespace' => wikiFuzz::chooseInput( range(-1, 15) ),
                'go'        => wikiFuzz::makeFuzz(2)
                );
    }
}


/**
 ** a page test for the page History.
 */
class pageHistoryTest extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Main_Page&action=history";

        $this->params = array (
                'limit'     => wikiFuzz::chooseInput( array("-1", "0", "-------'------0", "+1", "8134",  wikiFuzz::makeFuzz(2)) ),
                'offset'    => wikiFuzz::chooseInput( array("-1", "0", "------'-------0", "+1", "9823412312312412435", wikiFuzz::makeFuzz(2)) ),
                "go"        => wikiFuzz::chooseInput( array("first", "last", wikiFuzz::makeFuzz(2)) ),
                "dir"       => wikiFuzz::chooseInput( array("prev", "next", wikiFuzz::makeFuzz(2)) ),
                "diff"      => wikiFuzz::chooseInput( array("-1", "--------'-----0", "+1", "8134", wikiFuzz::makeFuzz(2)) ),
                "oldid"     => wikiFuzz::chooseInput( array("prev", "-1", "+1", "8134", wikiFuzz::makeFuzz(2)) ),
                "feed"      => wikiFuzz::makeFuzz(2)
                );
    }
}


/**
 ** a page test for the Special:Contributions".
 */
class contributionsTest extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Contributions/" . USER_ON_WIKI;

        $this->params = array (
                'target'    => wikiFuzz::chooseInput( array(wikiFuzz::makeFuzz(2), "newbies", USER_ON_WIKI) ),
                'namespace' => wikiFuzz::chooseInput( array(-1, 15, 1, wikiFuzz::makeFuzz(2)) ),
                'offset'    => wikiFuzz::chooseInput( array("0", "-1", "------'-------0", "+1", "982342131232131231241", wikiFuzz::makeFuzz(2)) ),
                'bot'       => wikiFuzz::chooseInput( array("", "-1", "0", "1", wikiFuzz::makeFuzz(2)) ),         
                'go'        => wikiFuzz::chooseInput( array("-1", 'prev', 'next', wikiFuzz::makeFuzz(2)) )
                );
    }
}


/**
 ** a page test for viewing a normal page, whilst posting various params.
 */
class viewPageTest extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Main_Page";

        $this->params = array (
                "useskin"        => wikiFuzz::chooseInput( array("chick", "cologneblue", "myskin", 
                                        "nostalgia", "simple", "standard", wikiFuzz::makeFuzz(2)) ),
                "uselang"        => wikiFuzz::chooseInput( array( wikiFuzz::makeFuzz(2),
                        "ab", "af", "an", "ar", "arc", "as", "ast", "av", "ay", "az", "ba",
                        "bat-smg", "be", "bg", "bm", "bn", "bo", "bpy", "br", "bs", "ca",
                        "ce", "cs", "csb", "cv", "cy", "da", "de", "dv", "dz", "el", "en",
                        "eo", "es", "et", "eu", "fa", "fi", "fo", "fr", "fur", "fy", "ga",
                        "gn", "gsw", "gu", "he", "hi", "hr", "hu", "ia", "id", "ii", "is", 
                        "it", "ja", "jv", "ka", "km", "kn", "ko", "ks", "ku", "kv", "la", 
                        "li", "lo", "lt", "lv", "mk", "ml", "ms", "nah", "nap", "nds", 
                        "nds-nl", "nl", "nn", "no", "non", "nv", "oc", "or", "os", "pa", 
                        "pl", "pms", "ps", "pt", "pt-br", "qu", "rmy", "ro", "ru", "sc", 
                        "sd", "sk", "sl", "sq", "sr", "sr-ec", "sr-el",
                        "su", "sv", "ta", "te", "th", "tlh", "tr", "tt", "ty", "tyv", "udm", 
                        "ug", "uk", "ur", "utf8", "vec", "vi", "wa", "xal", "yi", "za", 
                        "zh", "zh-cn", "zh-hk", "zh-sg", "zh-tw", "zh-tw") ),
                "returnto"       => wikiFuzz::makeFuzz(2),
                "feed"           => wikiFuzz::chooseInput( array("atom", "rss", wikiFuzz::makeFuzz(2)) ),
                "rcid"           => wikiFuzz::makeFuzz(2),
                "action"         => wikiFuzz::chooseInput( array("view", "raw", "render", wikiFuzz::makeFuzz(2), "markpatrolled") ),
                "printable"      => wikiFuzz::makeFuzz(2),
                "oldid"          => wikiFuzz::makeFuzz(2),
                "redirect"       => wikiFuzz::makeFuzz(2),
                "diff"           => wikiFuzz::makeFuzz(2),
                "search"         => wikiFuzz::makeFuzz(2),
                "rdfrom"         => wikiFuzz::makeFuzz(2),  // things from Article.php from here on:
                "token"          => wikiFuzz::makeFuzz(2),
                "tbid"           => wikiFuzz::makeFuzz(2),
                "action"         => wikiFuzz::chooseInput( array("purge", wikiFuzz::makeFuzz(2)) ),
                "wpReason"       => wikiFuzz::makeFuzz(2),
                "wpEditToken"    => wikiFuzz::makeFuzz(2),
                "from"           => wikiFuzz::makeFuzz(2),
                "bot"            => wikiFuzz::makeFuzz(2),
                "summary"        => wikiFuzz::makeFuzz(2),
                "direction"      => wikiFuzz::chooseInput( array("next", "prev", wikiFuzz::makeFuzz(2)) ),
                "section"        => wikiFuzz::makeFuzz(2),
                "preload"        => wikiFuzz::makeFuzz(2),

                );

        // Tidy does not know how to valid atom or rss, so exclude from testing for the time being.
        if ($this->params["feed"] == "atom")     { unset($this->params["feed"]); }
        else if ($this->params["feed"] == "rss") { unset($this->params["feed"]); }

        // Raw pages cannot really be validated
        if ($this->params["action"] == "raw") unset($this->params["action"]);

        // sometimes we don't want to specify certain parameters.
        if (wikiFuzz::randnum(6) == 0) unset($this->params["rcid"]);
        if (wikiFuzz::randnum(6) == 0) unset($this->params["diff"]);
        if (wikiFuzz::randnum(6) == 0) unset($this->params["rdfrom"]);
        if (wikiFuzz::randnum(3) == 0) unset($this->params["oldid"]);

        // usually don't want action == purge.
        if (wikiFuzz::randnum(6) > 1) unset($this->params["action"]);
    }
}


/**
 ** a page test for "Special:Allmessages".
 */
class specialAllmessagesTest extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Allmessages";

        // only really has one parameter
        $this->params = array (
                "ot"     => wikiFuzz::chooseInput( array("php", "html", wikiFuzz::makeFuzz(2)) )
                );
    }
}

/**
 ** a page test for "Special:Newpages".
 */
class specialNewpages extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Newpages";

        $this->params = array (
                "namespace" => wikiFuzz::chooseInput( range(-1, 15) ),
                "feed"      => wikiFuzz::chooseInput( array("atom", "rss", wikiFuzz::makeFuzz(2)) ),
                'limit'     => wikiFuzz::chooseInput( array("-1", "0", "-------'------0", "+1", "8134",  wikiFuzz::makeFuzz(2)) ),
                'offset'    => wikiFuzz::chooseInput( array("-1", "0", "------'-------0", "+1", "9823412312312412435", wikiFuzz::makeFuzz(2)) )
                );

        // Tidy does not know how to valid atom or rss, so exclude from testing for the time being.
        if ($this->params["feed"] == "atom")     { unset($this->params["feed"]); }
        else if ($this->params["feed"] == "rss") { unset($this->params["feed"]); }
    }
}

/**
 ** a page test for "redirect.php"
 */
class redirectTest extends pageTest {
    function __construct() {
        $this->pagePath = "redirect.php";

        $this->params = array (
                "wpDropdown" => wikiFuzz::makeFuzz(2)
                );

        // sometimes we don't want to specify certain parameters.
        if (wikiFuzz::randnum(6) == 0) unset($this->params["wpDropdown"]);
    }
}


/**
 ** a page test for "Special:Confirmemail"
 */
class confirmEmail extends pageTest {
    function __construct() {
        // sometimes we send a bogus confirmation code, and sometimes we don't.
        $this->pagePath = "index.php?title=Special:Confirmemail" . wikiFuzz::chooseInput( array("", "/" . wikiFuzz::makeTitleSafe(wikiFuzz::makeFuzz(1)) ) );

        $this->params = array (
                "token" => wikiFuzz::makeFuzz(2)
                );
    }
}


/**
 ** a page test for "Special:Watchlist"
 **        Note: this test would be better if we were logged in.
 */
class watchlistTest extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Watchlist";

        $this->params = array (
                "remove"   => wikiFuzz::chooseInput( array("Remove checked items from watchlist", wikiFuzz::makeFuzz(2))),
                'days'     => wikiFuzz::chooseInput( array(0, -1, -230, "--", 3, 9, wikiFuzz::makeFuzz(2)) ),
                'hideOwn'  => wikiFuzz::chooseInput( array("", "0", "1", wikiFuzz::makeFuzz(2)) ),
                'hideBots' => wikiFuzz::chooseInput( array("", "0", "1", wikiFuzz::makeFuzz(2)) ),
                'namespace'=> wikiFuzz::chooseInput( array("", "0", "1", wikiFuzz::makeFuzz(2)) ),
                'action'   => wikiFuzz::chooseInput( array("submit", "clear", wikiFuzz::makeFuzz(2)) ),
                'id[]'     => wikiFuzz::makeFuzz(2),
                'edit'     => wikiFuzz::makeFuzz(2),
                'token'    => wikiFuzz::chooseInput( array("", "1243213", wikiFuzz::makeFuzz(2)) )
                );

        // sometimes we specifiy "reset", and sometimes we don't.
        if (wikiFuzz::randnum(3) == 0) $this->params["reset"] = wikiFuzz::chooseInput( array("", "0", "1", wikiFuzz::makeFuzz(2)) );
    }
}


/**
 ** a page test for "Special:Blockme"
 */
class specialBlockmeTest extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Blockme";

        $this->params = array ( );

        // sometimes we specify "ip", and sometimes we don't.
        if (wikiFuzz::randnum(1) == 0) {
            $this->params["ip"] = wikiFuzz::chooseInput( array("10.12.41.213", wikiFuzz::randnum(-10,8134), wikiFuzz::makeFuzz(2)) );
        }
    }
}


/**
 ** a page test for "Special:Movepage"
 */
class specialMovePage extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Movepage";

        $this->params = array (
                "action"      => wikiFuzz::chooseInput( array("success", "submit", "", wikiFuzz::makeFuzz(2)) ),
                'wpEditToken' => wikiFuzz::chooseInput( array('', 0, 34987987, wikiFuzz::makeFuzz(2)) ),
                'target'      => wikiFuzz::chooseInput( array("x", wikiFuzz::makeTitleSafe(wikiFuzz::makeFuzz(2)) ) ),
                'wpOldTitle'  => wikiFuzz::chooseInput( array("z", wikiFuzz::makeTitleSafe(wikiFuzz::makeFuzz(2)), wikiFuzz::makeFuzz(2) ) ),
                'wpNewTitle'  => wikiFuzz::chooseInput( array("y", wikiFuzz::makeTitleSafe(wikiFuzz::makeFuzz(2)), wikiFuzz::makeFuzz(2) ) ),
                'wpReason'    => wikiFuzz::chooseInput( array(wikiFuzz::makeFuzz(2)) ),
                'wpMovetalk'  => wikiFuzz::chooseInput( array("0", "1", "++--34234", wikiFuzz::makeFuzz(2)) ),
                'wpDeleteAndMove'  => wikiFuzz::chooseInput( array("0", "1", "++--34234", wikiFuzz::makeFuzz(2)) ),
                'wpConfirm'   => wikiFuzz::chooseInput( array("0", "1", "++--34234", wikiFuzz::makeFuzz(2)) ),
                'talkmoved'   => wikiFuzz::chooseInput( array("1", wikiFuzz::makeFuzz(2), "articleexists", 'notalkpage') ),
                'oldtitle'    => wikiFuzz::makeFuzz(2),
                'newtitle'    => wikiFuzz::makeFuzz(2),
                'wpMovetalk'  => wikiFuzz::chooseInput( array("1", "0", wikiFuzz::makeFuzz(2)) )
                );

        // sometimes we don't want to specify certain parameters.
        if (wikiFuzz::randnum(2) == 0) unset($this->params["wpEditToken"]);
        if (wikiFuzz::randnum(3) == 0) unset($this->params["target"]);
        if (wikiFuzz::randnum(3) == 0) unset($this->params["wpNewTitle"]);
        if (wikiFuzz::randnum(4) == 0) unset($this->params["wpReason"]);
        if (wikiFuzz::randnum(4) == 0) unset($this->params["wpOldTitle"]);
    }
}


/**
 ** a page test for "Special:Undelete"
 */
class specialUndelete extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Undelete";

        $this->params = array (
                "action"      => wikiFuzz::chooseInput( array("submit", "", wikiFuzz::makeFuzz(2)) ),
                'wpEditToken' => wikiFuzz::chooseInput( array('', 0, 34987987, wikiFuzz::makeFuzz(2)) ),
                'target'      => wikiFuzz::chooseInput( array("x", wikiFuzz::makeTitleSafe(wikiFuzz::makeFuzz(2)) ) ),
                'timestamp'   => wikiFuzz::chooseInput( array("125223", wikiFuzz::makeFuzz(2) ) ),
                'file'        => wikiFuzz::chooseInput( array("0", "1", "++--34234", wikiFuzz::makeFuzz(2)) ),
                'restore'     => wikiFuzz::chooseInput( array("0", "1", wikiFuzz::makeFuzz(2)) ),
                'preview'     => wikiFuzz::chooseInput( array("0", "1", wikiFuzz::makeFuzz(2)) ),
                'wpComment'   => wikiFuzz::makeFuzz(2)
                );

        // sometimes we don't want to specify certain parameters.
        if (wikiFuzz::randnum(2) == 0) unset($this->params["wpEditToken"]);
        if (wikiFuzz::randnum(4) == 0) unset($this->params["target"]);
        if (wikiFuzz::randnum(1) == 0) unset($this->params["restore"]);
        if (wikiFuzz::randnum(1) == 0) unset($this->params["preview"]);
    }
}


/**
 ** a page test for "Special:Unlockdb"
 */
class specialUnlockdb extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Unlockdb";

        $this->params = array (
                "action"        => wikiFuzz::chooseInput( array("submit", "success", "", wikiFuzz::makeFuzz(2)) ),
                'wpEditToken'   => wikiFuzz::chooseInput( array("20398702394", "", wikiFuzz::makeFuzz(2)) ),
                'wpLockConfirm' => wikiFuzz::chooseInput( array("0", "1", wikiFuzz::makeFuzz(2)) )
                );

        // sometimes we don't want to specify certain parameters.
        if (wikiFuzz::randnum(4) == 0) unset($this->params["wpEditToken"]);
        if (wikiFuzz::randnum(4) == 0) unset($this->params["action"]);
        if (wikiFuzz::randnum(4) == 0) unset($this->params["wpLockConfirm"]);
    }
}


/**
 ** a page test for "Special:Lockdb"
 */
class specialLockdb extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Lockdb";

        $this->params = array (
                "action"       => wikiFuzz::chooseInput( array("submit", "success", "", wikiFuzz::makeFuzz(2)) ),
                'wpEditToken'  => wikiFuzz::chooseInput( array("20398702394", "", wikiFuzz::makeFuzz(2)) ),
                'wpLockReason' => wikiFuzz::makeFuzz(2),
                'wpLockConfirm'=> wikiFuzz::chooseInput( array("0", "1", "++--34234", wikiFuzz::makeFuzz(2)) )
                );

        // sometimes we don't want to specify certain parameters.
        if (wikiFuzz::randnum(4) == 0) unset($this->params["wpEditToken"]);
        if (wikiFuzz::randnum(4) == 0) unset($this->params["action"]);
        if (wikiFuzz::randnum(4) == 0) unset($this->params["wpLockConfirm"]);
    }
}


/**
 ** a page test for "Special:Userrights"
 */
class specialUserrights extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Userrights";

        $this->params = array (
                'wpEditToken'   => wikiFuzz::chooseInput( array("20398702394", "", wikiFuzz::makeFuzz(2)) ),
                'user-editname' => wikiFuzz::chooseInput( array("Nickj2", "Nickj2\n<xyz>", wikiFuzz::makeFuzz(2)) ),
                'ssearchuser'   => wikiFuzz::chooseInput( array("0", "1", "++--34234", wikiFuzz::makeFuzz(2)) ),
                'saveusergroups'=> wikiFuzz::chooseInput( array("0", "1", "++--34234", wikiFuzz::makeFuzz(2)), "Save User Groups"),
                'member[]'      => wikiFuzz::chooseInput( array("0", "bot", "1", "++--34234", wikiFuzz::makeFuzz(2)) ),
                "available[]"   => wikiFuzz::chooseInput( array("0", "sysop", "bureaucrat", "1", "++--34234", wikiFuzz::makeFuzz(2)) )
                );

        // sometimes we don't want to specify certain parameters.
        if (wikiFuzz::randnum(3) == 0) unset($this->params['ssearchuser']);
        if (wikiFuzz::randnum(3) == 0) unset($this->params['saveusergroups']);
    }
}


/**
 ** a test for page protection and unprotection.
 */
class pageProtectionForm extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Main_Page";

        $this->params = array (
                "action"               => "protect",
                'wpEditToken'          => wikiFuzz::chooseInput( array("20398702394", "", wikiFuzz::makeFuzz(2)) ),
                "mwProtect-level-edit" => wikiFuzz::chooseInput( array('', 'autoconfirmed', 'sysop', wikifuzz::makeFuzz(2)) ),
                "mwProtect-level-move" => wikiFuzz::chooseInput( array('', 'autoconfirmed', 'sysop', wikifuzz::makeFuzz(2)) ),
                "mwProtectUnchained"   => wikiFuzz::chooseInput( array("0", "1", "++--34234", wikiFuzz::makeFuzz(2)) ),
                'mwProtect-reason'     => wikiFuzz::chooseInput( array("because it was there", wikifuzz::makeFuzz(2)) )
                );


        // sometimes we don't want to specify certain parameters.
        if (wikiFuzz::randnum(3) == 0) unset($this->params["mwProtectUnchained"]);
        if (wikiFuzz::randnum(3) == 0) unset($this->params['mwProtect-reason']);
    }
}


/**
 ** a page test for "Special:Blockip".
 */
class specialBlockip extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Blockip";

        $this->params = array (
                "action"          => wikiFuzz::chooseInput( array("submit", "",  wikiFuzz::makeFuzz(2)) ),
                'wpEditToken'     => wikiFuzz::chooseInput( array("20398702394", "", wikiFuzz::makeFuzz(2)) ),
                "wpBlockAddress"  => wikiFuzz::chooseInput( array("20398702394", "", "Nickj2", wikiFuzz::makeFuzz(2),
                                      // something like an IP address, sometimes invalid:
                                     ( wikiFuzz::randnum(300,-20) . "." . wikiFuzz::randnum(300,-20) . "." 
                                      . wikiFuzz::randnum(300,-20) . "." .wikiFuzz::randnum(300,-20) ) ) ),
                "ip"              => wikiFuzz::chooseInput( array("20398702394", "", "Nickj2", wikiFuzz::makeFuzz(2),
                                      // something like an IP address, sometimes invalid:
                                     ( wikiFuzz::randnum(300,-20) . "." . wikiFuzz::randnum(300,-20) . "."
                                      . wikiFuzz::randnum(300,-20) . "." .wikiFuzz::randnum(300,-20) ) ) ),
                "wpBlockOther"    => wikiFuzz::chooseInput( array('', 'Nickj2', wikifuzz::makeFuzz(2)) ),
                "wpBlockExpiry"   => wikiFuzz::chooseInput( array("other", "2 hours", "1 day", "3 days", "1 week", "2 weeks",
                                          "1 month", "3 months", "6 months", "1 year", "infinite", wikiFuzz::makeFuzz(2)) ),
                "wpBlockReason"   => wikiFuzz::chooseInput( array("because it was there", wikifuzz::makeFuzz(2)) ),
                "wpAnonOnly"      => wikiFuzz::chooseInput( array("0", "1", "++--34234", wikiFuzz::makeFuzz(2)) ),
                "wpCreateAccount" => wikiFuzz::chooseInput( array("0", "1", "++--34234", wikiFuzz::makeFuzz(2)) ),
                "wpBlock"         => wikiFuzz::chooseInput( array("0", "1", "++--34234", wikiFuzz::makeFuzz(2)) )
                );

        // sometimes we don't want to specify certain parameters.
        if (wikiFuzz::randnum(4) == 0) unset($this->params["wpBlockOther"]);
        if (wikiFuzz::randnum(4) == 0) unset($this->params["wpBlockExpiry"]);
        if (wikiFuzz::randnum(4) == 0) unset($this->params["wpBlockReason"]);
        if (wikiFuzz::randnum(4) == 0) unset($this->params["wpAnonOnly"]);
        if (wikiFuzz::randnum(4) == 0) unset($this->params["wpCreateAccount"]);
        if (wikiFuzz::randnum(4) == 0) unset($this->params["wpBlockAddress"]);
        if (wikiFuzz::randnum(4) == 0) unset($this->params["ip"]);
    }
}


/**
 ** a test for the imagepage.
 */
class imagepageTest extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Image:Small-email.png";

        $this->params = array (
                "image"         => wikiFuzz::chooseInput( array("Small-email.png", wikifuzz::makeFuzz(2)) ),
                "wpReason"      => wikifuzz::makeFuzz(2),
                "oldimage"      => wikiFuzz::chooseInput( array("Small-email.png", wikifuzz::makeFuzz(2)) ),
                "wpEditToken"   => wikiFuzz::chooseInput( array("20398702394", "", wikiFuzz::makeFuzz(2)) ),
                );

        // sometimes we don't want to specify certain parameters.
        if (wikiFuzz::randnum(6) == 0) unset($this->params["image"]);
        if (wikiFuzz::randnum(6) == 0) unset($this->params["wpReason"]);
        if (wikiFuzz::randnum(6) == 0) unset($this->params["oldimage"]);
        if (wikiFuzz::randnum(6) == 0) unset($this->params["wpEditToken"]);
    }
}


/**
 ** a test for page deletion form.
 */
class pageDeletion extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Main_Page&action=delete";

        $this->params = array (
                "wpEditToken" => wikiFuzz::chooseInput( array("20398702394", "", wikiFuzz::makeFuzz(2)) ),
                "wpReason"    => wikiFuzz::chooseInput( array("0", "1", "++--34234", wikiFuzz::makeFuzz(2)) ),
                "wpConfirm"   => wikiFuzz::chooseInput( array("0", "1", "++--34234", wikiFuzz::makeFuzz(2)) ),
                );

        // sometimes we don't want to specify certain parameters.
        if (wikiFuzz::randnum(5) == 0) unset($this->params["wpReason"]);
        if (wikiFuzz::randnum(5) == 0) unset($this->params["wpEditToken"]);
        if (wikiFuzz::randnum(5) == 0) unset($this->params["wpConfirm"]);
    }
}



/**
 ** a test for Revision Deletion.
 */
class specialRevisionDelete extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Revisiondelete";

        $this->params = array (
                "target"               => wikiFuzz::chooseInput( array("Main Page", wikifuzz::makeFuzz(2)) ),
                "oldid"                => wikifuzz::makeFuzz(2),
                "oldid[]"              => wikifuzz::makeFuzz(2),
                "wpReason"             => wikiFuzz::chooseInput( array("0", "1", "++--34234", wikiFuzz::makeFuzz(2)) ),
                "revdelete-hide-text"  => wikiFuzz::chooseInput( array("0", "1", "++--34234", wikiFuzz::makeFuzz(2)) ),
                "revdelete-hide-comment" => wikiFuzz::chooseInput( array("0", "1", "++--34234", wikiFuzz::makeFuzz(2)) ),
                "revdelete-hide-user"  => wikiFuzz::chooseInput( array("0", "1", "++--34234", wikiFuzz::makeFuzz(2)) ),
                "revdelete-hide-restricted" => wikiFuzz::chooseInput( array("0", "1", "++--34234", wikiFuzz::makeFuzz(2)) ),
                );

        // sometimes we don't want to specify certain parameters.
        if (wikiFuzz::randnum(3) == 0) unset($this->params["target"]);
        if (wikiFuzz::randnum(6) == 0) unset($this->params["oldid"]);
        if (wikiFuzz::randnum(6) == 0) unset($this->params["oldid[]"]);
        if (wikiFuzz::randnum(6) == 0) unset($this->params["wpReason"]);
        if (wikiFuzz::randnum(6) == 0) unset($this->params["revdelete-hide-text"]);
        if (wikiFuzz::randnum(6) == 0) unset($this->params["revdelete-hide-comment"]);
        if (wikiFuzz::randnum(6) == 0) unset($this->params["revdelete-hide-user"]);
        if (wikiFuzz::randnum(6) == 0) unset($this->params["revdelete-hide-restricted"]);
    }
}


/**
 ** a test for Special:Import.
 */
class specialImport extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Import";

        $this->params = array (
                "action"         => "submit",
                "source"         => wikiFuzz::chooseInput( array("upload", "interwiki", wikifuzz::makeFuzz(2)) ),
                "MAX_FILE_SIZE"  => wikiFuzz::chooseInput( array("0", "1", "++--34234", wikifuzz::makeFuzz(2)) ),
                "xmlimport"      => wikiFuzz::chooseInput( array("/var/www/hosts/mediawiki/wiki/AdminSettings.php", "1", "++--34234", wikiFuzz::makeFuzz(2)) ),
                "namespace"      => wikiFuzz::chooseInput( array(wikiFuzz::randnum(30,-6), wikiFuzz::makeFuzz(2)) ),
                "interwiki"      => wikiFuzz::makeFuzz(2),
                "interwikiHistory" => wikiFuzz::makeFuzz(2),
                "frompage"       => wikiFuzz::makeFuzz(2),
                );

        // sometimes we don't want to specify certain parameters.
        if (wikiFuzz::randnum(6) == 0) unset($this->params["action"]);
        if (wikiFuzz::randnum(6) == 0) unset($this->params["source"]);
        if (wikiFuzz::randnum(6) == 0) unset($this->params["MAX_FILE_SIZE"]);
        if (wikiFuzz::randnum(6) == 0) unset($this->params["xmlimport"]);
        if (wikiFuzz::randnum(6) == 0) unset($this->params["interwiki"]);
        if (wikiFuzz::randnum(6) == 0) unset($this->params["interwikiHistory"]);
        if (wikiFuzz::randnum(6) == 0) unset($this->params["frompage"]);

        // Note: Need to do a file upload to fully test this Special page.
    }
}


/**
 ** a test for thumb.php
 */
class thumbTest extends pageTest {
    function __construct() {
        $this->pagePath = "thumb.php";

        $this->params = array (
                "f"  => wikiFuzz::chooseInput( array("..", "\\", "small-email.png", wikifuzz::makeFuzz(2)) ),
                "w"  => wikiFuzz::chooseInput( array("80", wikiFuzz::randnum(6000,-200), wikifuzz::makeFuzz(2)) ),
                "r"  => wikiFuzz::chooseInput( array("0", wikifuzz::makeFuzz(2)) ),
                );

        // sometimes we don't want to specify certain parameters.
        if (wikiFuzz::randnum(6) == 0) unset($this->params["f"]);
        if (wikiFuzz::randnum(6) == 0) unset($this->params["w"]);
        if (wikiFuzz::randnum(6) == 0) unset($this->params["r"]);
    }
}


/**
 ** a test for trackback.php
 */
class trackbackTest extends pageTest {
    function __construct() {
        $this->pagePath = "trackback.php";

        $this->params = array (
                "url"       => wikifuzz::makeFuzz(2),
                "blog_name" => wikiFuzz::chooseInput( array("80", wikiFuzz::randnum(6000,-200), wikifuzz::makeFuzz(2)) ),
                "article"   => wikiFuzz::chooseInput( array("Main Page", wikifuzz::makeFuzz(2)) ),
                "title"     => wikiFuzz::chooseInput( array("Main Page", wikifuzz::makeFuzz(2)) ),
                "excerpt"   => wikifuzz::makeFuzz(2),
                );

        // sometimes we don't want to specify certain parameters.
        if (wikiFuzz::randnum(3) == 0) unset($this->params["title"]);
        if (wikiFuzz::randnum(3) == 0) unset($this->params["excerpt"]);
        
        // page does not produce HTML.
        $this->tidyValidate = false; 
    }
}


/**
 ** a test for profileinfo.php
 */
class profileInfo extends pageTest {
    function __construct() {
        $this->pagePath = "profileinfo.php";

        $this->params = array (
                "expand"  => wikifuzz::makeFuzz(2),
                "sort"    => wikiFuzz::chooseInput( array("time", "count", "name", wikifuzz::makeFuzz(2)) ),
                "filter"  => wikiFuzz::chooseInput( array("Main Page", wikifuzz::makeFuzz(2)) ),
                );

        // sometimes we don't want to specify certain parameters.
        if (wikiFuzz::randnum(3) == 0) unset($this->params["sort"]);
        if (wikiFuzz::randnum(3) == 0) unset($this->params["filter"]);
    }
}


/**
 ** a test for Special:Cite (extension Special page).
 */
class specialCite extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Cite";

        $this->params = array (
                "page"    => wikiFuzz::chooseInput( array("\" onmouseover=\"alert(1);\"", "Main Page", wikifuzz::makeFuzz(2)) ),
                "id"      => wikiFuzz::chooseInput( array("-1", "0", "------'-------0", "+1", "-9823412312312412435", wikiFuzz::makeFuzz(2)) ),
                );

        // sometimes we don't want to specify certain parameters.
        if (wikiFuzz::randnum(6) == 0) unset($this->params["page"]);
        if (wikiFuzz::randnum(6) == 0) unset($this->params["id"]);
    }
}


/**
 ** a test for Special:Filepath (extension Special page).
 */
class specialFilepath extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Filepath";

        $this->params = array (
                "file"    => wikiFuzz::chooseInput( array("Small-email.png", "Small-email.png" . wikifuzz::makeFuzz(1), wikiFuzz::makeFuzz(2)) ),
                );
    }
}


/**
 ** a test for Special:Makebot (extension Special page).
 */
class specialMakebot extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Makebot";

        $this->params = array (
                "username" => wikiFuzz::chooseInput( array("Nickj2", "192.168.0.2", wikifuzz::makeFuzz(1) ) ),
                "dosearch" => wikiFuzz::chooseInput( array("0", "1", "++--34234", wikifuzz::makeFuzz(2)) ),
                "grant"    => wikiFuzz::chooseInput( array("0", "1", "++--34234", wikifuzz::makeFuzz(2)) ),
                "comment"  => wikiFuzz::chooseInput( array("20398702394", "", wikiFuzz::makeFuzz(2)) ), 
                "token"    => wikiFuzz::chooseInput( array("20398702394", "", wikiFuzz::makeFuzz(2)) ),
                );

        // sometimes we don't want to specify certain parameters.
        if (wikiFuzz::randnum(2) == 0) unset($this->params["dosearch"]);
        if (wikiFuzz::randnum(2) == 0) unset($this->params["grant"]);
        if (wikiFuzz::randnum(5) == 0) unset($this->params["token"]);
    }
}


/**
 ** a test for Special:Makesysop (extension Special page).
 */
class specialMakesysop extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Makesysop";

        $this->params = array (
                "wpMakesysopUser"   => wikiFuzz::chooseInput( array("Nickj2", "192.168.0.2", wikifuzz::makeFuzz(1) ) ),
                "action"            => wikiFuzz::chooseInput( array("0", "1", "++--34234", wikifuzz::makeFuzz(2)) ),
                "wpMakesysopSubmit" => wikiFuzz::chooseInput( array("0", "1", "++--34234", wikifuzz::makeFuzz(2)) ),
                "wpEditToken"       => wikiFuzz::chooseInput( array("20398702394", "", wikiFuzz::makeFuzz(2)) ),
                "wpSetBureaucrat"   => wikiFuzz::chooseInput( array("20398702394", "", wikiFuzz::makeFuzz(2)) ),
                );

        // sometimes we don't want to specify certain parameters.
        if (wikiFuzz::randnum(3) == 0) unset($this->params["wpMakesysopSubmit"]);
        if (wikiFuzz::randnum(3) == 0) unset($this->params["wpEditToken"]);
        if (wikiFuzz::randnum(3) == 0) unset($this->params["wpSetBureaucrat"]);
    }
}


/**
 ** a test for Special:Renameuser (extension Special page).
 */
class specialRenameuser extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:Renameuser";

        $this->params = array (
                "oldusername"   => wikiFuzz::chooseInput( array("Nickj2", "192.168.0.2", wikifuzz::makeFuzz(1) ) ),
                "newusername"   => wikiFuzz::chooseInput( array("Nickj2", "192.168.0.2", wikifuzz::makeFuzz(1) ) ),
                "token"         => wikiFuzz::chooseInput( array("20398702394", "", wikiFuzz::makeFuzz(2)) ),
                );
    }
}


/**
 ** a test for Special:Linksearch (extension Special page).
 */
class specialLinksearch extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special%3ALinksearch";

        $this->params = array (
                "target" => wikifuzz::makeFuzz(2),
                );

        // sometimes we don't want to specify certain parameters.
        if (wikiFuzz::randnum(10) == 0) unset($this->params["target"]);
    }
}


/**
 ** a test for Special:CategoryTree (extension Special page).
 */
class specialCategoryTree extends pageTest {
    function __construct() {
        $this->pagePath = "index.php?title=Special:CategoryTree";

        $this->params = array (
                "target" => wikifuzz::makeFuzz(2),
                "from"   => wikifuzz::makeFuzz(2),
                "until"  => wikifuzz::makeFuzz(2),
                "showas" => wikifuzz::makeFuzz(2),
                "mode"   => wikiFuzz::chooseInput( array("pages", "categories", "all", wikifuzz::makeFuzz(2)) ),
                );

        // sometimes we do want to specify certain parameters.
        if (wikiFuzz::randnum(5) == 0) $this->params["notree"] = wikiFuzz::chooseInput( array("1", 0, "", wikiFuzz::makeFuzz(2)) );
    }
}


/**
 ** a test for "Special:Chemicalsources" (extension Special page).
 */
class specialChemicalsourcesTest extends pageTest {
    function __construct() {
    	$this->pagePath = "index.php?title=Special:Chemicalsources";

    	// choose an input format to use.
    	$format =  wikiFuzz::chooseInput(
			    	array(  'go',
			    	        'CAS',
			    	        'EINECS',
			    	        'CHEBI',
			    	        'PubChem',
			    	        'SMILES',
			    	        'InChI',
			    	        'ATCCode',
			    	        'KEGG',
			    	        'RTECS',
			    	        'ECNumber',
			    	        'DrugBank',
			    	        'Formula',
			    	        'Name'
			    	     )
			    	);

        // values for different formats usually start with either letters or numbers.
        switch ($format) {
        	case 'Name'   : $value = "A"; break;
        	case 'InChI'  :
        	case 'SMILES' :
        	case 'Formula': $value = "C"; break;
        	default       : $value = "0"; break;
        }

        // and then we append the fuzz input.
        $this->params = array ($format => $value . wikifuzz::makeFuzz(2) );
    }
}


/**
 ** A test for api.php (programmatic interface to MediaWiki in XML/JSON/RSS/etc formats).
 ** Quite involved to test because there are lots of options/parameters, and because
 ** for a lot of the functionality if all the parameters don't make sense then it just
 ** returns the help screen - so currently a lot of the tests aren't actually doing much
 ** because something wasn't right in the query.
 **
 ** @todo: Incomplete / unfinished; Runs too fast (suggests not much testing going on).
 */
class api extends pageTest {

    // API login mode.
    private static function loginMode() {
        $arr =  array ( "lgname"        => wikifuzz::makeFuzz(2),
                        "lgpassword"    => wikifuzz::makeFuzz(2), 
                       );
        // sometimes we want to specify the extra "lgdomain" parameter.
        if (wikiFuzz::randnum(3) == 0) {
        	$arr["lgdomain"] = wikiFuzz::chooseInput( array("1", 0, "", wikiFuzz::makeFuzz(2)) );
        }

        return $arr;
    }

    // API OpenSearch mode.
    private static function opensearchMode() {
    	return array ("search"        => wikifuzz::makeFuzz(2));
    }

    // API watchlist feed mode.
    private static function feedwatchlistMode() {
    	// FIXME: add "wikifuzz::makeFuzz(2)" as possible value below?
    	return array ("feedformat"    => wikiFuzz::chooseInput( array("rss", "atom") ) );
    }

    // API query mode.
    private static function queryMode() {
    	// FIXME: add "wikifuzz::makeFuzz(2)" as possible params for the elements below?
    	//        Suspect this will stuff up the tests more, but need to check.
    	$params = array (
	                 // FIXME: More titles.
	                 "titles"        => wikiFuzz::chooseInput( array("Main Page")),
	                 // FIXME: More pageids.	                 
	                 "pageids"       => 1,
	                 "prop"          => wikiFuzz::chooseInput( array("info", "revisions", "watchlist")),
	                 "list"          => wikiFuzz::chooseInput( array("allpages", "logevents", "watchlist", "usercontribs", "recentchanges", "backlinks", "embeddedin", "imagelinks") ),
	                 "meta"          => wikiFuzz::chooseInput( array("siteinfo")),
	                 "generator"     => wikiFuzz::chooseInput( array("allpages", "logevents", "watchlist", "info", "revisions") ),
	                 "siprop"        => wikiFuzz::chooseInput( array("general", "namespaces", "general|namespaces") ),
                   );
         
         // Add extra parameters based on what list choice we got.
         switch ($params["list"]) {
         	case "usercontribs" : self::addListParams ($params, "uc", array("limit", "start", "end", "user", "dir") ); break;
         	case "allpages"     : self::addListParams ($params, "ap", array("from", "prefix", "namespace", "filterredir", "limit") ); break;
         	case "watchlist"    : self::addListParams ($params, "wl", array("allrev", "start", "end", "namespace", "dir", "limit", "prop") ); break;
         	case "logevents"    : self::addListParams ($params, "le", array("limit", "type", "start", "end", "user", "dir") ); break;
         	case "recentchanges": self::addListParams ($params, "rc", array("limit", "prop", "show", "namespace", "start", "end", "dir") ); break;
         	case "backlinks"    : self::addListParams ($params, "bl", array("continue", "namespace", "redirect", "limit") ); break;
         	case "embeddedin"   : self::addListParams ($params, "ei", array("continue", "namespace", "redirect", "limit") ); break;
         	case "imagelinks"   : self::addListParams ($params, "il", array("continue", "namespace", "redirect", "limit") ); break;
         }

         if ($params["prop"] == "revisions") {
         	self::addListParams ($params, "rv", array("prop", "limit", "startid", "endid", "end", "dir") );
         }

         // Sometimes we want redirects, sometimes we don't.
         if (wikiFuzz::randnum(3) == 0) {
         	$params["redirects"] = wikiFuzz::chooseInput( array("1", 0, "", wikiFuzz::makeFuzz(2)) );
         }

         return $params;
    }

    // Adds all the elements to the array, using the specified prefix.
    private static function addListParams(&$array, $prefix, $elements)  {
    	foreach ($elements as $element) {
    		$array[$prefix . $element] = self::getParamDetails($element);
    	}
    }

    // For a given element name, returns the data for that element.
    private static function getParamDetails($element) {
    	switch ($element) {
    		case 'startid'    :
    		case 'endid'      :
    		case 'start'      :
    		case 'end'        :
    		case 'limit'      : return wikiFuzz::chooseInput( array("0", "-1", "---'----------0", "+1", "8134", "320742734234235", "20060230121212", wikiFuzz::randnum(9000, -100), wikiFuzz::makeFuzz(2)) );
    		case 'dir'        : return wikiFuzz::chooseInput( array("newer", "older", wikifuzz::makeFuzz(2) ) );
    		case 'user'       : return wikiFuzz::chooseInput( array(USER_ON_WIKI, wikifuzz::makeFuzz(2) ) );
    		case 'namespace'  : return wikiFuzz::chooseInput( array(-2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 200000, wikifuzz::makeFuzz(2)) );
    		case 'filterredir': return wikiFuzz::chooseInput( array("all", "redirects", "nonredirectsallpages", wikifuzz::makeFuzz(2)) );
    		case 'allrev'     : return wikiFuzz::chooseInput( array("1", 0, "", wikiFuzz::makeFuzz(2)) );
    		case 'prop'       : return wikiFuzz::chooseInput( array("user", "comment", "timestamp", "patrol", "flags", "user|user|comment|flags", wikifuzz::makeFuzz(2) ) );
    		case 'type'       : return wikiFuzz::chooseInput( array("block", "protect", "rights", "delete", "upload", "move", "import", "renameuser", "newusers", "makebot", wikifuzz::makeFuzz(2) ) );
    		case 'hide'       : return wikiFuzz::chooseInput( array("minor", "bots", "anons", "liu", "liu|bots|", wikifuzz::makeFuzz(2) ) );
    		case 'show'       : return wikiFuzz::chooseInput( array('minor', '!minor', 'bot', '!bot', 'anon', '!anon', wikifuzz::makeFuzz(2) ) );
    		default           : return wikifuzz::makeFuzz(2);
    	}
    }

    // Entry point.
    function __construct() {
    	$this->pagePath = "api.php";

        $modes = array ("help",
                        "login",
                        "opensearch",
                        "feedwatchlist",
                        "query");
        $action = wikiFuzz::chooseInput( array_merge ($modes, array(wikifuzz::makeFuzz(2))) );
        
        switch ($action) {
            case "login"         : $this->params = self::loginMode();
                                   break;
            case "opensearch"    : $this->params = self::opensearchMode();
                                   break;
            case "feedwatchlist" : $this->params = self::feedwatchlistMode();
                                   break;
            case "query"         : $this->params = self::queryMode();
                                   break;
            case "help"         : 
            default             :  // Do something random - "Crazy Ivan" mode.
                                   $random_mode = wikiFuzz::chooseInput( $modes ) . "Mode";
                                   // There is no "helpMode".
                                   if ($random_mode == "helpMode") $random_mode = "queryMode"; 
                                   $this->params = self::$random_mode();
                                   break;
        }
        
        // Save the selected action.
        $this->params["action"] = $action;

        // Set the cookie:
        // FIXME: need to get this cookie dynamically set, rather than hard-coded.
        $this->cookie = "wikidbUserID=10001; wikidbUserName=Test; wikidb_session=178df0fe68c75834643af65dec9ec98a; wikidbToken=1adc6753d62c44aec950c024d7ae0540";

        // Output format
        $this->params["format"] = wikiFuzz::chooseInput( array("json", "jsonfm", "php", "phpfm",
                                                               "wddx", "wddxfm", "xml", "xmlfm", 
                                                               "yaml", "yamlfm", "raw", "rawfm",
                                                               wikifuzz::makeFuzz(2) ) );

        // Page does not produce HTML (sometimes).
        $this->tidyValidate = false;
    }
}


/**
 ** a page test for the GeSHi extension.
 */
class GeSHi_Test extends pageTest {
	
    private function getGeSHiContent() {
        return "<source lang=\"" . $this->getLang() . "\" "
               . (wikiFuzz::randnum(2) == 0 ? "line " : "")
               . (wikiFuzz::randnum(2) == 0 ? "strict " : "")
               . "start=" . wikiFuzz::chooseInput( array(wikiFuzz::randnum(-6000,6000), wikifuzz::makeFuzz(2)) )
               . ">"
               . wikiFuzz::makeFuzz(2)
               . "</source>";
    }
	
    private function getLang() {
	return wikiFuzz::chooseInput( array( "actionscript", "ada", "apache", "applescript", "asm", "asp", "autoit", "bash", "blitzbasic", "bnf", "c", "c_mac", "caddcl", "cadlisp",
                "cfdg", "cfm", "cpp", "cpp-qt", "csharp", "css", "d", "delphi", "diff", "div", "dos", "eiffel", "fortran", "freebasic", "gml", "groovy", "html4strict", "idl", 
                "ini", "inno", "io", "java", "java5", "javascript", "latex", "lisp", "lua", "matlab", "mirc", "mpasm", "mysql", "nsis", "objc", "ocaml", "ocaml-brief", "oobas",
                "oracle8", "pascal", "perl", "php", "php-brief", "plsql", "python", "qbasic", "rails", "reg", "robots", "ruby", "sas", "scheme", "sdlbasic", "smalltalk", "smarty",
                "sql", "tcl", "text", "thinbasic", "tsql", "vb", "vbnet", "vhdl", "visualfoxpro", "winbatch", "xml", "xpp", "z80", wikifuzz::makeFuzz(1) ) );
    }
	
    function __construct() {
        $this->pagePath = "index.php?title=WIKIFUZZ";

        $this->params = array (
                "action"        => "submit",
                "wpMinoredit"   => "test",
                "wpPreview"     => "test",
                "wpSection"     => "test",
                "wpEdittime"    => "test",
                "wpSummary"     => "test",
                "wpScrolltop"   => "test",
                "wpStarttime"   => "test",
                "wpAutoSummary" => "test",
                "wpTextbox1"    => $this->getGeSHiContent() // the main wiki text, contains fake GeSHi content.
                );
    }
}


/**
 ** selects a page test to run.
 */
function selectPageTest($count) {

    // if the user only wants a specific test, then only ever give them that.
    if (defined("SPECIFIC_TEST")) {
        $testType = SPECIFIC_TEST;
        return new $testType ();
    }

    // Some of the time we test Special pages, the remaining
    // time we test using the standard edit page.
    switch ($count % 100) {
        case 0 : return new successfulUserLoginTest();
        case 1 : return new listusersTest();
        case 2 : return new searchTest();
        case 3 : return new recentchangesTest();
        case 4 : return new prefixindexTest();
        case 5 : return new mimeSearchTest();
        case 6 : return new specialLogTest();
        case 7 : return new userLoginTest();
        case 8 : return new ipblocklistTest();
        case 9 : return new newImagesTest();
        case 10: return new imagelistTest();
        case 11: return new specialExportTest();
        case 12: return new specialBooksourcesTest();
        case 13: return new specialAllpagesTest();
        case 14: return new pageHistoryTest();
        case 15: return new contributionsTest();
        case 16: return new viewPageTest();
        case 17: return new specialAllmessagesTest();
        case 18: return new specialNewpages();
        case 19: return new searchTest();
        case 20: return new redirectTest();
        case 21: return new confirmEmail();
        case 22: return new watchlistTest();
        case 23: return new specialBlockmeTest();
        case 24: return new specialUndelete();
        case 25: return new specialMovePage();
        case 26: return new specialUnlockdb();
        case 27: return new specialLockdb();
        case 28: return new specialUserrights();
        case 29: return new pageProtectionForm();
        case 30: return new specialBlockip();
        case 31: return new imagepageTest();
        case 32: return new pageDeletion();
        case 33: return new specialRevisionDelete();
        case 34: return new specialImport();
        case 35: return new thumbTest();
        case 36: return new trackbackTest();
        case 37: return new profileInfo();
        case 38: return new specialCite();
        case 39: return new specialFilepath();
        case 40: return new specialMakebot();
        case 41: return new specialMakesysop();
        case 42: return new specialRenameuser();
        case 43: return new specialLinksearch();
        case 44: return new specialCategoryTree();
        case 45: return new api();
        case 45: return new specialChemicalsourcesTest();
        default: return new editPageTest();
    }
}


///////////////////////  SAVING OUTPUT  /////////////////////////

/**
 ** Utility function for saving a file. Currently has no error checking.
 */
function saveFile($data, $name) {
    file_put_contents($name, $data);
}


/**
 ** Returns a test as an experimental GET-to-POST URL.
 **        This doesn't seem to always work though, and sometimes the output is too long 
 **        to be a valid GET URL, so we also save in other formats.
 */
function getAsURL(pageTest $test) {
    $used_question_mark = (strpos($test->getPagePath(), "?") !== false);
    $retval = "http://get-to-post.nickj.org/?" . WIKI_BASE_URL . $test->getPagePath();
    foreach ($test->getParams() as $param => $value) {
        if (!$used_question_mark) {
            $retval .= "?";
            $used_question_mark = true;
        }
        else {
            $retval .= "&";
        }
        $retval .= $param . "=" . urlencode($value);
    }
    return $retval;
}


/**
 ** Saves a plain-text human-readable version of a test.
 */
function saveTestAsText(pageTest $test, $filename) {
    $str = "Test: " . $test->getPagePath();
    foreach ($test->getParams() as $param => $value) {
        $str .= "\n$param: $value";
    }
    $str .= "\nGet-to-post URL: " . getAsURL($test) . "\n";
    saveFile($str, $filename);
}


/**
 ** Saves a test as a standalone basic PHP script that shows this one problem.
 **        Resulting script requires PHP-Curl be installed in order to work.
 */
function saveTestAsPHP(pageTest $test, $filename) {
    $str = "<?php\n"
        . "\$params = " . var_export(escapeForCurl($test->getParams()), true) . ";\n"
        . "\$ch = curl_init();\n"
        . "curl_setopt(\$ch, CURLOPT_POST, 1);\n"
        . "curl_setopt(\$ch, CURLOPT_POSTFIELDS, \$params );\n"
        . "curl_setopt(\$ch, CURLOPT_URL, " . var_export(WIKI_BASE_URL . $test->getPagePath(), true) . ");\n"
        . "curl_setopt(\$ch, CURLOPT_RETURNTRANSFER,1);\n"
        .  ($test->getCookie() ? "curl_setopt(\$ch, CURLOPT_COOKIE, " . var_export($test->getCookie(), true) . ");\n" : "")
        . "\$result=curl_exec(\$ch);\n"
        . "curl_close (\$ch);\n"
        . "print \$result;\n"
        . "?>\n";
    saveFile($str, $filename);
}


/**
 ** Escapes a value so that it can be used on the command line by Curl.
 **        Specifically, "<" and "@" need to be escaped if they are the first character, 
 **        otherwise  curl interprets these as meaning that we want to insert a file.
 */
function escapeForCurl(array $input_params) {
    $output_params = array();
    foreach ($input_params as $param => $value) {
        if (strlen($value) > 0 && ( $value[0] == "@" || $value[0] == "<")) {
            $value = "\\" . $value;
        }
        $output_params[$param] = $value;
    }
    return $output_params;
}


/**
 ** Saves a test as a standalone CURL shell script that shows this one problem.
 **        Resulting script requires standalone Curl be installed in order to work.
 */
function saveTestAsCurl(pageTest $test, $filename) {
    $str = "#!/bin/bash\n"
        . "curl --silent --include --globoff \\\n"
        . ($test->getCookie() ? " --cookie " . escapeshellarg($test->getCookie()) . " \\\n" : "");
    foreach (escapeForCurl($test->getParams()) as $param => $value) {
        $str .= " -F " . escapeshellarg($param) . "=" . escapeshellarg($value) . " \\\n";
    }
    $str .= " " . escapeshellarg(WIKI_BASE_URL . $test->getPagePath()); // beginning space matters.
    $str .= "\n";
    saveFile($str, $filename);
    chmod($filename, 0755); // make executable
}


/**
 ** Saves the internal data structure to file.
 */
function saveTestData (pageTest $test, $filename) {
    saveFile(serialize($test),  $filename);
}


/**
 ** saves a test in the various formats.
 */
function saveTest(pageTest $test, $testname) {
    $base_name = DIRECTORY . "/" . $testname;
    saveTestAsText($test, $base_name . INFO_FILE);
    saveTestAsPHP ($test, $base_name . PHP_TEST );
    saveTestAsCurl($test, $base_name . CURL_TEST);
    saveTestData  ($test, $base_name . DATA_FILE);
}


//////////////////// MEDIAWIKI OUTPUT /////////////////////////

/**
 ** Asks MediaWiki for the HTML output of a test.
 */
function wikiTestOutput(pageTest $test) {

    $ch = curl_init();

    // specify the cookie, if required.
    if ($test->getCookie()) curl_setopt($ch, CURLOPT_COOKIE, $test->getCookie());
    curl_setopt($ch, CURLOPT_POST, 1);                          // save form using a POST

    $params = escapeForCurl($test->getParams());
    curl_setopt($ch, CURLOPT_POSTFIELDS, $params );             // load the POST variables

    curl_setopt($ch, CURLOPT_URL, WIKI_BASE_URL . $test->getPagePath() );  // set url to post to
    curl_setopt($ch, CURLOPT_RETURNTRANSFER,1);                 // return into a variable

    $result=curl_exec ($ch);

    // if we encountered an error, then say so, and return an empty string.
    if (curl_error($ch)) {
        print "\nCurl error #: " . curl_errno($ch) . " - " . curl_error ($ch);
        $result = "";
    }

    curl_close ($ch);

    return $result;
}


//////////////////// HTML VALIDATION /////////////////////////

/*
 ** Asks the validator whether this is valid HTML, or not.
 */
function validateHTML($text) {

    $params = array ("fragment"   => $text);

    $ch = curl_init();

    curl_setopt($ch, CURLOPT_POST, 1);                    // save form using a POST
    curl_setopt($ch, CURLOPT_POSTFIELDS, $params);        // load the POST variables
    curl_setopt($ch, CURLOPT_URL, VALIDATOR_URL);         // set url to post to
    curl_setopt($ch, CURLOPT_RETURNTRANSFER,1);           // return into a variable

    $result=curl_exec ($ch);

    // if we encountered an error, then log it, and exit.
    if (curl_error($ch)) {
        trigger_error("Curl error #: " . curl_errno($ch) . " - " . curl_error ($ch) );
        print "Curl error #: " . curl_errno($ch) . " - " . curl_error ($ch) . " - exiting.\n";
        exit();
    }

    curl_close ($ch);

    $valid = (strpos($result, "Failed validation") === false ? true : false);

    return array($valid, $result);
}


/**
 ** Get tidy to check for no HTML errors in the output file (e.g. unescaped strings).
 */
function tidyCheckFile($name) {
    $file = DIRECTORY . "/" . $name;
    $command = PATH_TO_TIDY . " -output /tmp/out.html -quiet $file 2>&1";
    $x = `$command`;

    // Look for the most interesting Tidy errors and warnings.
    if (   strpos($x,"end of file while parsing attributes") !== false 
            || strpos($x,"attribute with missing trailing quote mark") !== false
            || strpos($x,"missing '>' for end of tag") !== false 
            || strpos($x,"Error:") !== false) {
        print "\nTidy found something - view details with: $command";
        return false;
    } else {
        return true;
    }
}


/**
 ** Returns whether or not an database error log file has changed in size since
 **        the last time this was run. This is used to tell if a test caused a DB error.
 */
function dbErrorLogged() {
    static $filesize;

    // first time running this function
    if (!isset($filesize)) {
        // create log if it does not exist
        if (!file_exists(DB_ERROR_LOG_FILE)) {
            saveFile("", DB_ERROR_LOG_FILE);
        }
        $filesize = filesize(DB_ERROR_LOG_FILE);
        return false;
    }

    $newsize = filesize(DB_ERROR_LOG_FILE);
    // if the log has grown, then assume the current test caused it.
    if ($newsize != $filesize) {
        $filesize = $newsize;
        return true;
    }

    return false;
}

////////////////// TOP-LEVEL PROBLEM-FINDING FUNCTION ////////////////////////

/**
 ** takes a page test, and runs it and tests it for problems in the output.
 **        Returns: False on finding a problem, or True on no problems being found.
 */
function runWikiTest(pageTest $test, &$testname, $can_overwrite = false) {

    // by default don't overwrite a previous test of the same name.
    while ( ! $can_overwrite && file_exists(DIRECTORY . "/" . $testname . DATA_FILE)) {
        $testname .= "-" . mt_rand(0,9);
    }

    $filename = DIRECTORY . "/" . $testname . DATA_FILE;

    // Store the time before and after, to find slow pages.
    $before = microtime(true);

    // Get MediaWiki to give us the output of this test.
    $wiki_preview = wikiTestOutput($test);

    $after = microtime(true);

    // if we received no response, then that's interesting.
    if ($wiki_preview == "") {
        print "\nNo response received for: $filename";
        return false;
    }

    // save output HTML to file.
    $html_file = DIRECTORY . "/" . $testname . HTML_FILE;
    saveFile($wiki_preview,  $html_file);

    // if there were PHP errors in the output, then that's interesting too.
    if (       strpos($wiki_preview, "<b>Warning</b>: "        ) !== false 
            || strpos($wiki_preview, "<b>Fatal error</b>: "    ) !== false
            || strpos($wiki_preview, "<b>Notice</b>: "         ) !== false
            || strpos($wiki_preview, "<b>Error</b>: "          ) !== false 
            || strpos($wiki_preview, "<b>Strict Standards:</b>") !== false
            ) {
        $error = substr($wiki_preview, strpos($wiki_preview, "</b>:") + 7, 50);
        // Avoid probable PHP bug with bad session ids; http://bugs.php.net/bug.php?id=38224 
        if ($error != "Unknown: The session id contains illegal character") {
            print "\nPHP error/warning/notice in HTML output: $html_file ; $error";
            return false;
        }
    }

    // if there was a MediaWiki Backtrace message in the output, then that's also interesting.
    if( strpos($wiki_preview, "Backtrace:") !== false ) {
        print "\nInternal MediaWiki error in HTML output: $html_file";
        return false;
    }

    // if there was a Parser error comment in the output, then that's potentially interesting.
    if( strpos($wiki_preview, "!-- ERR") !== false ) {
        print "\nParser Error comment in HTML output: $html_file";
        return false;
    }

    // if a database error was logged, then that's definitely interesting.
    if( dbErrorLogged() ) {
        print "\nDatabase Error logged for: $filename";
        return false;
    }

    // validate result
    $valid = true;
    if( VALIDATE_ON_WEB ) {
        list ($valid, $validator_output) = validateHTML($wiki_preview);
        if (!$valid) print "\nW3C web validation failed - view details with: html2text " . DIRECTORY . "/" . $testname . ".validator_output.html";
    }

    // Get tidy to check the page, unless we already know it produces non-XHTML output.
    if( $test->tidyValidate() ) {
        $valid = tidyCheckFile( $testname . HTML_FILE ) && $valid;
    }

    // if it took more than 2 seconds to render, then it may be interesting too. (Possible DoS attack?)
    if (($after - $before) >= 2) {
        print "\nParticularly slow to render (" . round($after - $before, 2) . " seconds): $filename";
        return false;
    }

    if( $valid ) {
        // Remove temp HTML file if test was valid:
        unlink( $html_file );
    } elseif( VALIDATE_ON_WEB ) {
        saveFile($validator_output,   DIRECTORY . "/" . $testname . ".validator_output.html");
    }

    return $valid;
}


/////////////////// RERUNNING OLD TESTS ///////////////////

/**
 ** We keep our failed tests so that they can be rerun.
 **        This function does that retesting.
 */
function rerunPreviousTests() {
    print "Retesting previously found problems.\n";

    $dir_contents = scandir (DIRECTORY);

    // sort file into the order a normal person would use.
    natsort ($dir_contents);

    foreach ($dir_contents as $file) {

        // if file is not a test, then skip it. 
        // Note we need to escape any periods or will be treated as "any character".
        $matches = array();
        if (!ereg("(.*)" . str_replace(".", "\.", DATA_FILE) . "$", $file, $matches)) continue;

        // reload the test.
        $full_path = DIRECTORY . "/" . $file;
        $test = unserialize(file_get_contents($full_path));

        // if this is not a valid test, then skip it.
        if (! $test instanceof pageTest) {
            print "\nSkipping invalid test - $full_path";
            continue;
        }

        // The date format is in Apache log format, which makes it easier to locate 
        // which retest caused which error in the Apache logs (only happens usually if 
        // apache segfaults).
        if (!QUIET) print "[" . date ("D M d H:i:s Y") . "] Retesting $file (" . get_class($test) . ")";

        // run test
        $testname = $matches[1];
        $valid = runWikiTest($test, $testname, true);

        if (!$valid) {
            saveTest($test, $testname);
            if (QUIET) {
                print "\nTest: " . get_class($test) . " ; Testname: $testname\n------";
            } else {
                print "\n";
            }
        }
        else {
            if (!QUIET) print "\r";
            if (DELETE_PASSED_RETESTS) {
                $prefix = DIRECTORY . "/" . $testname;
                if (is_file($prefix . DATA_FILE)) unlink($prefix . DATA_FILE);
                if (is_file($prefix . PHP_TEST )) unlink($prefix . PHP_TEST );
                if (is_file($prefix . CURL_TEST)) unlink($prefix . CURL_TEST);
                if (is_file($prefix . INFO_FILE)) unlink($prefix . INFO_FILE);
            }
        }
    }

    print "\nDone retesting.\n";
}


//////////////////////  MAIN LOOP  ////////////////////////


// first check whether CURL is installed, because sometimes it's not.
if( ! function_exists('curl_init') ) {
    die("Could not find 'curl_init' function. Is the curl extension compiled into PHP?\n");
}

// Initialization of types. wikiFuzz doesn't have a constructor because we want to 
// access it staticly and not have any globals.
wikiFuzz::$types = array_keys(wikiFuzz::$data);

// Make directory if doesn't exist
if (!is_dir(DIRECTORY)) {
    mkdir (DIRECTORY, 0700 );
}
// otherwise, we first retest the things that we have found in previous runs
else if (RERUN_OLD_TESTS) {
    rerunPreviousTests();
}

// main loop.
$start_time = date("U");
$num_errors = 0;
if (!QUIET) {
    print "Beginning main loop. Results are stored in the " . DIRECTORY . " directory.\n";
    print "Press CTRL+C to stop testing.\n";
}

for ($count=0; true; $count++) {
    if (!QUIET) {
        // spinning progress indicator.
        switch( $count % 4 ) {
            case '0': print "\r/";  break;
            case '1': print "\r-";  break;
            case '2': print "\r\\"; break;
            case '3': print "\r|";  break;
        }
        print " $count";
    }

    // generate a page test to run.
    $test = selectPageTest($count);

    $mins = ( date("U") - $start_time ) / 60;
    if (!QUIET && $mins > 0) {
        print ".  $num_errors poss errors. " 
            . floor($mins) . " mins. " 
            . round ($count / $mins, 0) . " tests/min. " 
            . get_class($test); // includes the current test name.
    }

    // run this test against MediaWiki, and see if the output was valid.
    $testname = $count;
    $valid = runWikiTest($test, $testname, false);

    // save the failed test
    if ( ! $valid ) {
        if (QUIET) {
            print "\nTest: " . get_class($test) . " ; Testname: $testname\n------";
        } else {
            print "\n";
        }
        saveTest($test, $testname);
        $num_errors += 1;
    } else if ( KEEP_PASSED_TESTS ) {
        // print current time, with microseconds (matches "strace" format), and the test name.
        print " " . date("H:i:s.") . substr(current(explode(" ", microtime())), 2) . " " . $testname;
        saveTest($test, $testname);
    }

    // stop if we have reached max number of errors.
    if (defined("MAX_ERRORS") && $num_errors>=MAX_ERRORS) {
        break;
    }

    // stop if we have reached max number of mins runtime.
    if (defined("MAX_RUNTIME") && $mins>=MAX_RUNTIME) {
        break;
    }
}


