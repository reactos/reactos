<?php
/* vim: set expandtab tabstop=4 shiftwidth=4 softtabstop=4: */
// +----------------------------------------------------------------------+
// | PHP version 4                                                        |
// +----------------------------------------------------------------------+
// | Copyright (c) 1997-2004 The PHP Group                                |
// +----------------------------------------------------------------------+
// | This source file is subject to version 3.0 of the PHP license,       |
// | that is bundled with this package in the file LICENSE, and is        |
// | available through the world-wide-web at the following url:           |
// | http://www.php.net/license/3_0.txt.                                  |
// | If you did not receive a copy of the PHP license and are unable to   |
// | obtain it through the world-wide-web, please send a note to          |
// | license@php.net so we can mail you a copy immediately.               |
// +----------------------------------------------------------------------+
// | Authors: Roberto Berto <darkelder.php.net>                           |
// +----------------------------------------------------------------------+
//
// $Id: mbox.php,v 1.13 2004/10/19 16:41:04 darkelder Exp $

require_once "PEAR.php";

    /**
     * Mbox PHP class to Unix MBOX parsing and using
     * 
     * 
     * METHODS:
     * int resource mbox->open(string file)
     *   open a mbox and return a resource id
     *
     * bool mbox->close(resource)
     *   close a mbox resource id
     *
     * int mbox->size(resource)
     *   return mbox number of messages
     *
     * string mbox->get(int resource, messageNumber)
     *   return the message number of the resource
     *
     * bool mbox->update(int resource, int messageNumber, string message)
     *   update the message offset to message (need write permission)
     *
     * bool mbox->remove(int resource, int messageNumber)
     *   remove the message messageNumber (need write permission)
     *
     * bool mbox->insert(int resource, string message[, $offset = null])
     *   add message to the end of the mbox. Offset == 0 message will
     *   be append at first message. If after == null will be the last
     *   one message. (need write permission)
     *
     * RELATED LINKS: 
     * - CPAN Perl Mail::Folder::Mbox Module
     *   Used as a start point to create this class.
     *   http://search.cpan.org/author/KJOHNSON/MailFolder-0.07/Mail/Folder/Mbox.pm
     *
     * - PHP Mime Decode PEAR Module 
     *   Use it to parse headers and body.
     *   http://pear.php.net/package-info.php?pacid=21
     *
     * EXAMPLE: 
     *    // some random content
     *    $content = <<<EOF
     *From Foo@example.com Fri Dec 27 14:31:10 2002
     *Return-Path: 
     *Received: from [unix socket] by campos.example.com (LMTP); Fri, 27 Dec
     *    2002 14:31:10 -0200 (BRST)
     *Date: Fri, 27 Dec 2002 14:31:21 -0500
     *Message-Id: <200212271931.gBRJVL012289@example.com>
     *Received: from  pcp128525pcs.foo.example.com (
     *    pcp128525pcs.example.com [99.99.99.99]) by/
     *    serjolen6com.example.com (v64.19) with ESMTP id
     *    MAILRELAYINZA98-3601058302; Fri, 08 Nov 2002 06:39:05 -0500
     *From: "Foo@example.com"
     *To: fool@example.com
     *Subject: This is A SPAM!!
     *Content-Type: text/plan
     *
     *testing foo spam
     *EOF;
     *
     *    // starting mbox
     *    require_once "mbox.php";
     *    $mbox    =      new Mail_Mbox();
     *
     *    // uncomment to see lots of things
     *    #$mbox->debug    = true;
     *
     *    // opennign file mbox
     *    $mid     =     $mbox->open("mbox");
     *
     *    // uncomment to see internal vars
     *    #print_r($mbox);
     *
     *
     *
     *    // deleting a message (uncomment to test)
     *    #$res1 =  $mbox->remove($mid,0);
     *    if (PEAR::isError($res1))
     *    {
     *        print $res1->getMessage();
     *    }
     *
     *
     *
     *
     *
     *        // changing a message (uncomment to test)
     *    #$res2 = $mbox->update($mid,0,$content);
     *        if (PEAR::isError($res2))
     *        {
     *                print $res2->getMessage();
     *        }
     *
     *
     *        // adding a message (uncomment to test)
     *        $res3 = $mbox->insert($mid,$content,0);
     *        if (PEAR::isError($res3))
     *        {
     *                print $res3->getMessage();
     *        }
     *
     *
     *
     *    require_once "Mail/mimeDecode.php";
     *    // showing current messages with Mail Mime
     *    for ($x = 0; $x < $mbox->size($mid); $x++)
     *    {
     *        printf("Message: %08d<pre>",$x);
     *        $thisMessage     = $mbox->get($mid,$x);
     *        print $thisMessage;    
     *        print "<hr />";
     *        $decode = new Mail_mimeDecode($thisMessage, "\r\n");
     *        $structure = $decode->decode();
     *        print_r($structure);
     *
     *        print "</pre><hr /><hr /><hr />";
     *    }
     *
     *
     *
     * @author   Roberto Berto <darkelder@php.net>
     * @package  Mail
     * @access   public
     */

class Mail_Mbox extends PEAR
{
    /**
    * Resources data like file name, file resource, mbox number, and other 
    * cacheds things are stored here.
    *
    * Note that it isnt really a valid resource type. It is of array type.
    *
    * @var      array
    * @access   private
    */
    var $_resources;

    /**
     * Debug mode 
     *
     * Set to true to turn on debug mode
     *
     * @var      bool
     * @access   public
     */
     var $debug = false;

    /**
     * Open a Mbox
     *
     * Open the Mbox file and return an resource identificator.
     *
     * Also, this function will process the Mbox and create a cache 
     * that tells each message start and end bytes.
     * 
     * @param  int $file   Mbox file to open
     * @return mixed       ResourceID on success else pear error class
     * @access public
     */
    function open($file)
    {
        // check if file exists else return pear error
        if (!file_exists($file)) {
            return PEAR::raiseError("Cannot open the mbox file: file doesnt exists.");
        }

        // getting next resource it to set
        $resourceId = sizeof($this->_resources) + 1;

        // setting filename to the resource id
        $this->_resources[$resourceId]["filename"] = $file;

        // opening the file
        $this->_resources[$resourceId]["fresource"] = fopen($file, "r");
        if (!is_resource($this->_resources[$resourceId]["fresource"])) {
            return PEAR::raiseError("Cannot open the mbox file: maybe without permission.");
        }

        // process the file and get the messages bytes offsets
        $this->_process($resourceId);

        return $resourceId;
    }

    /**
     * Close a Mbox
     *
     * Close the Mbox file opened by open()
     *
     * @param    int $resourceId     Mbox resouce id created by open
     * @return   mixed               true on success else pear error class
     * @access   public
     */
    function close($resourceId)
    {
        if (!is_resource($this->_resources[$resourceId]["fresource"])) {
            return PEAR::raiseError("Cannot close the mbox file because it wanst open.");
        }

        if (!fclose($this->_resources[$resourceId]["fresource"])) {
            return PEAR::raiseError("Cannot close the mbox, maybe file is being used (?)");
        }

        return true;
    }

    /**
     * Mbox Size
     * 
     * Get Mbox Number of Messages
     *
     * @param    int $resourceId     Mbox resouce id created by open
     * @return   int                 Number of messages on Mbox (starting on 1,
     *                               0 if no message exists)
     * @access   public
     */
    function size($resourceId)
    {
        if (array_key_exists('messages', $this->_resources[$resourceId])) {
            return sizeof($this->_resources[$resourceId]["messages"]);
        } else {
            return 0;
        }
    }    

    /**
     * Mbox Get 
     *
     * Get a Message from Mbox
     *
     * Note: Message number start from 0.
     *
     * @param    int $resourceId     Mbox resouce id created by open
     * @param    int $message        The number of Message
     * @return   string              Return the message else pear error class
     * @access   public
     */
    function get($resourceId, $message)
    {
        // checking if we have bytes locations for this message
        if (!is_array($this->_resources[$resourceId]["messages"][$message])) {
            return PEAR::raiseError("Message doesnt exists.");
        }

        // getting bytes locations
        $bytesStart = $this->_resources[$resourceId]["messages"][$message][0];
        $bytesEnd = $this->_resources[$resourceId]["messages"][$message][1];

        // a debug feature to show the bytes locations
        if ($this->debug) {
            printf("%08d=%08d<br />", $bytesStart, $bytesEnd);
        }

        // seek to start of message
        if (@fseek($this->_resources[$resourceId]["fresource"], $bytesStart) == -1) {
            return PEAR::raiseError("Cannot read message bytes");
        }

        if ($bytesEnd - $bytesStart > 0) {
            // reading and returning message (bytes to read = difference of bytes locations)
            $msg = fread($this->_resources[$resourceId]["fresource"],
                         $bytesEnd - $bytesStart) . "\n";
            return $msg;
        }
    }

    /**
     * Delete Message
     *
     * Remove a message from Mbox and save it.
     *
     * Note: messages start with 0.
     *
     * @param    int $resourceId     Mbox resouce id created by open
     * @param    int $message        The number of Message to remove, or
     *                               array of message ids to remove
     * @return   mixed               Return true else pear error class
     * @access   public
     */
    function remove($resourceId, $message)
    {
        // convert single message to array
        if (!is_array($message)) {
            $message = array($message);
        }

        // checking if we have bytes locations for this message
        foreach ($message as $msg) {
            if (!is_array($this->_resources[$resourceId]["messages"][$msg])) {
                return PEAR::raiseError("Message $msg doesn't exist.");
            }
        }

        // changing umask for security reasons
        $umaskOld   = umask(077);
        // creating temp file
        $ftempname  = tempnam ("/tmp", rand(0, 9));
        // returning to old umask
        umask($umaskOld);

        $ftemp      = fopen($ftempname, "w");
        if ($ftemp == false) {
            return PEAR::raiseError("Cannot create a temp file. Cannot handle this error.");            
        }

        // writing only undeleted messages 
        $messages = $this->size($resourceId);

        for ($x = 0; $x < $messages; $x++) {
            if (in_array($x, $message)) {
                continue;    
            }

            $messageThis = $this->get($resourceId, $x);
            if (is_string($messageThis)) {
                fwrite($ftemp, $messageThis, strlen($messageThis));
            }
        }

        // closing file
        $filename = $this->_resources[$resourceId]["filename"];
        $this->close($resourceId);
        fclose($ftemp);

        return $this->_move($resourceId, $ftempname, $filename);
    }

    /**
     * Update a message
     *
     * Note: Mail_Mbox auto adds \n\n at end of the message
     *
     * Note: messages start with 0.
     *
     * @param    int $resourceId     Mbox resouce id created by open
     * @param    int $message        The number of Message to updated
     * @param    string $content     The new content of the Message
     * @return   mixed               Return true else pear error class
     * @access   public
     */
    function update($resourceId, $message, $content)
    {
        // checking if we have bytes locations for this message
        if (!is_array($this->_resources[$resourceId]["messages"][$message])) {
            return PEAR::raiseError("Message doesnt exists.");
        }

        // creating temp file
        $ftempname  = tempnam ("/tmp", rand(0, 9));
        $ftemp = fopen($ftempname, "w");
        if ($ftemp == false) {
            return PEAR::raiseError("Cannot create a temp file. Cannot handle this error.");
        }

        // writing only undeleted messages
        $messages = $this->size($resourceId);

        for ($x = 0; $x < $messages; $x++) {
            if ($x == $message) {
                $messageThis = $content . "\n\n";
            } else {
                $messageThis = $this->get($resourceId, $x);
            }

            if (is_string($messageThis)) {
                fwrite($ftemp, $messageThis, strlen($messageThis));
            }
        }

        // closing file
        $filename = $this->_resources[$resourceId]["filename"];
        $this->close($resourceId);
        fclose($ftemp);

        return $this->_move($resourceId, $ftempname, $filename);
    }

    /**
     * Insert a message
     *
     * PEAR::Mail_Mbox will insert the message according its offset. 
     * 0 means before the actual message 0. 3 means before the message 3
     * (Remember: message 3 is the forth message). The default is put 
     * AFTER the last message.
     *
     * Note: PEAR::Mail_Mbox auto adds \n\n at end of the message
     *
     * @param    int $resourceId     Mbox resouce id created by open
     * @param    string $content     The content of the new Message
     * @param    int offset          Before the offset. Default: last message
     * @return   mixed               Return true else pear error class
     * @access   public
     */
    function insert($resourceId, $content, $offset = NULL)
    {
        // checking if we have bytes locations for this message
        if (!is_array($this->_resources[$resourceId])) {
            return PEAR::raiseError("ResourceId doesnt exists.");
        }

        // creating temp file
        $ftempname  = tempnam ("/tmp", rand(0, 9));
        $ftemp = fopen($ftempname, "w");
        if ($ftemp == false) {
            return PEAR::raiseError("Cannot create a temp file. Cannot handle this error.");
        }

        // writing only undeleted messages
        $messages = $this->size($resourceId);
        $content .= "\n\n";

        if ($messages == 0 && $offset !== NULL) {
            fwrite($ftemp, $content, strlen($content));
        } else {
            for ($x = 0; $x < $messages; $x++)  {
                if ($offset !== NULL && $x == $offset) {
                    fwrite($ftemp, $content, strlen($content));
                }
                $messageThis = $this->get($resourceId, $x);
    
                if (is_string($messageThis)) {
                    fwrite($ftemp, $messageThis, strlen($messageThis));
                }
            }
        }

        if ($offset === NULL) {
            fwrite($ftemp, $content, strlen($content));
        }

        // closing file
        $filename = $this->_resources[$resourceId]["filename"];
        $this->close($resourceId);
        fclose($ftemp);

        return $this->_move($resourceId, $ftempname, $filename);
    }

    /**
     * Copy a file to another
     *
     * Used internally to copy the content of the temp file to the mbox file
     *
     * @parm     int $resourceId     Resource file
     * @parm     string $ftempname   Source file - will be removed
     * @param    string $filename    Output file
     * @access   private
     */
    function _move($resourceId, $ftempname, $filename) 
    {
        // opening ftemp to read
        $ftemp = fopen($ftempname, "r");

        if ($ftemp == false) {
            return PEAR::raiseError("Cannot open temp file.");
        }

        // copy from ftemp to fp
        $fp = @fopen($filename, "w");
        if ($fp == false) {
            return PEAR::raiseError("Cannot write on mbox file.");
        }

        while (feof($ftemp) != true) {
            $strings = fread($ftemp, 4096);
            if (fwrite($fp, $strings, strlen($strings)) === false) {
                return PEAR::raiseError("Cannot write to file.");
            }
        }

        fclose($fp);
        fclose($ftemp);
        unlink($ftempname);

        // open another resource and substitute it to the old one
        $mid = $this->open($filename);
        $this->_resources[$resourceId] = $this->_resources[$mid];
        unset($this->_resources[$mid]);

        return true;
    }

    /**
     * Process the Mbox
     *
     * Roles:
     * - Count the messages
     * - Get start bytes and end bytes of each messages
     *
     * @param    int $resourceId     Mbox resouce id created by open
     * @access   private
     */
    function _process($resourceId)
    {
        // sanity check
        if (!is_resource($this->_resources[$resourceId]['fresource'])) {
            return PEAR::raiseError("Resource isn't valid.");
        }

        // going to start
        if (@fseek($this->_resources[$resourceId]['fresource']) == -1) {
            return PEAR::raiseError("Cannot read mbox");
        }

        // current start byte position
        $start      = 0;
        // last start byte position
        $laststart  = 0;
        // there aren't any message
        $hasmessage = false;

        while ($line = fgets($this->_resources[$resourceId]['fresource'], 4096)) {
            // if line start with "From ", it is a new message
            if (0 === strncmp($line, 'From ', 5)) {
                // save last start byte position
                $laststart  = $start;
        
                // new start byte position is the start of the line 
                $start      = ftell($this->_resources[$resourceId]['fresource']) - strlen($line);

                // if it is not the first message add message positions
                if ($start > 0) {
                    $this->_resources[$resourceId]["messages"][] = array($laststart, $start - 1);
                } else {
                    // tell that there is really a message on the file
                    $hasmessage = true;
                }
            }
        }

        // if there are just one message, or if it's the last one,
        // add it to messages positions
        if (($start == 0 && $hasmessage === true) || ($start > 0)) {
            $this->_resources[$resourceId]["messages"][] = array($start, ftell($this->_resources[$resourceId]['fresource']));
        }
    }
}
  
?>
