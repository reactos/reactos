/*
 * Copyright (c) 2010-2017 Red Hat, Inc.
 *
 * Author(s):
 *  Miki Mishael <mikim@daynix.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met :
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and / or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of their contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*----------------Define Section--------------------------*/

var ROOT_DIR = WScript.Arguments(0);
var PROJECT_XML_PATH = WScript.Arguments(1);
var PROJECT_DIR_PATH = WScript.Arguments(2);
var PROJECT_NAME = WScript.Arguments(3);

var logFileName = "SDV.log"; /* "SDV.log" */
var debugMode = true;
var ns = "xmlns:sdv = 'http://schemas.microsoft.com/developer/msbuild/2003'";
var exitProgram = ~~0; /* 0 == Success */

function logString(str) {
    if (logFileName != "") {
        var fso = new ActiveXObject("Scripting.FileSystemObject");
        var logFile = fso.OpenTextFile(logFileName, 8, true);
        logFile.WriteLine(str);
        logFile.Close();
    }
    if (debugMode) {
        WScript.Echo(str);
    }
}

function startLog() {
    var now = new Date();
    logString(now.toString());
}

function stopLog() {
    logString("\n---------------------------------------------------------------------------");
}

function parseXmlProject() {

    this.XmlDocument = new ActiveXObject("msxml2.DOMDocument.6.0");
    this.XmlDocument.async = false;
    this.XmlDocument.setProperty('ProhibitDTD', false);
    this.XmlDocument.resolveExternals = false;
    this.XmlDocument.validateOnParse = false;
    this.XmlDocument.load(PROJECT_XML_PATH);
    this.XmlDocument.setProperty("SelectionNamespaces", ns);
    this.node = { Configuration: [], Platform: [] };

    this.getProjectConfigs = function () {
        this.printParseError();
        this.node.Configuration = this.XmlDocument.selectNodes("//sdv:ProjectConfiguration//sdv:Configuration");
        this.node.Platform = this.XmlDocument.selectNodes("//sdv:ProjectConfiguration//sdv:Platform");
        return this.node;
    }

    this.printParseError = function () {
        if (this.XmlDocument.parseError.errorCode != 0) {
            var myErr = this.XmlDocument.parseError;
            throw "Xml parse error: " + myErr.reason;
        }
    }
}

function runSdv(confList) {

    this.WshShell = new ActiveXObject("WScript.Shell");
    this.fso = new ActiveXObject("Scripting.FileSystemObject");
    this.projectDirPath = PROJECT_DIR_PATH;
    this.projectXmlPath = PROJECT_XML_PATH;
    this.projectName = PROJECT_NAME;
    this.target = 'sdv';
    this.inputClean = '/clean';
    this.inputCheck = '/check:*';
    this.configuration = null;
    this.platform = null;
    this.cmd = null;
    this.src = null;
    this.dest = null;
    this.res = null;
    this.out = " >> ../SdvShellOutLog.log";
    this.copyCmd = 'CMD /C xcopy /i/e/h/y ';
    this.cmdCdToProjectDir = "cd " + '"' + this.projectDirPath + '"' +
                             " && title DO Not Close!-Running SDV on " + this.projectName + " && ";

    this.buildCheckCmd = function () {
        this.cmd = 'CMD /C ' + this.cmdCdToProjectDir + 'msbuild' + ' ' +
    '"' + this.projectXmlPath + '"' + ' ' + '/t:' + this.target + ' ' +
    '/p:inputs=' + '"' + this.inputCheck + '"' + ' ' + '/p:configuration=' +
    '"' + this.configuration + '"' + ' ' + '/p:platform=' + '"' +
    this.platform + '"' + this.out;
    }

    this.buildCleanCmd = function () {
        this.cmd = 'CMD /C ' + this.cmdCdToProjectDir + 'msbuild' + ' ' +
    '"' + this.projectXmlPath + '"' + ' ' + '/t:' + this.target + ' ' +
    '/p:inputs=' + '"' + this.inputClean + '"' + ' ' + '/p:configuration=' +
    '"' + this.configuration + '"' + ' ' + '/p:platform=' + '"' +
    this.platform + '"' + this.out;
    }

    this.buildCopyCmd = function () {
        this.src = this.projectDirPath + "\\sdv\\* ";
        this.des = ROOT_DIR + "\SDV\\" + this.projectName + "\\" + '"' + this.configuration +
        '"' + "\\" + this.platform;
        this.cmd = this.copyCmd + this.src + this.des + this.out;
    }

    this.exeCheckCmd = function () {
        this.buildCheckCmd();
        logString("\nRunning " + '"' + this.inputCheck + '"' + ' ' + "SDV on: " + ' Project: "' +
        this.projectName + '" Configuration: "' +
        this.configuration + '" Platform: "' + this.platform + '" ...');

        this.res = this.WshShell.run(this.cmd, 1, true);
        logString("Res " + this.res);
        if (this.res != 0)
            throw "Error Running SDV logs" + ' Project: "' + this.projectName + '" Configuration: "' +
            this.configuration + '" Platform: "' + this.platform + '"';
    }

    this.exeCleanCmd = function () {
        this.buildCleanCmd();
        logString("\n\nCleaning SDV logs" + ' Project: "' + this.projectName + '" Configuration: "' +
        this.configuration + '" Platform: "' + this.platform + '" ...');
        logString(this.cmd);
        this.res = this.WshShell.run(this.cmd, 1, true);
        logString("Res " + this.res);
        if (this.res != 0)
            throw "Error Cleaning SDV logs" + ' Project: "' + this.projectName + '" Configuration: "' +
            this.configuration + '" Platform: "' + this.platform + '"';
    }


    this.exeCopySdvDir = function () {
        this.buildCopyCmd();
        logString("\nCopying SDV folder results from SRC: " + this.src + "to DES: " + this.des + ".");
        this.res = this.WshShell.run(this.cmd, 1, true);
        logString("Res " + this.res);
        if (this.res != 0)
            throw "Error Copying SDV folder results from SRC: " + this.src + "to DES: " + this.des + ".";
    }

    this.resetRunSdv = function () {
        this.WshShell = new ActiveXObject("WScript.Shell");
        this.fso = new ActiveXObject("Scripting.FileSystemObject");
        this.configuration = null;
        this.platform = null;
        this.cmd = null;
        this.src = null;
        this.dest = null;
        this.res = null;
    }

    this.iterateConfs = function () {
        for (var idx = 0; idx < confList.Configuration.length; idx++) {
            this.configuration = confList.Configuration[idx].text;
            if (this.configuration.indexOf("Debug") != -1)
            continue;
            this.platform = confList.Platform[idx].text;
            try {
                stopLog();
                startLog();
                this.exeCleanCmd();
                this.exeCheckCmd();
                this.exeCopySdvDir();
                stopLog();
            }
            catch (exception) {
                logString("\t" + exception);
                this.resetRunSdv();
                exitProgram = ~~1;
                stopLog();
                continue;
            }
        }
    }

    this.test = function () {
        this.exeCleanCmd();
        this.exeCheckCmd();
        this.exeCopySdvDir();
        this.exeCleanCmd();
    }
}

function start() {
    logString("ROOT_DIR:" + ROOT_DIR);
    logString("PROJECT_XML_PATH:" + PROJECT_XML_PATH);
    logString("PROJECT_DIR_PATH:" + PROJECT_DIR_PATH);
    logString("PROJECT_NAME:" + PROJECT_NAME);
    var res = new parseXmlProject();
    var confList = res.getProjectConfigs();
    if (confList.Configuration.length < 1)
        throw " Could not find Configurations for this project.";
    var sh = new runSdv(confList);
    sh.iterateConfs();
}
try {
    startLog();
    start();
    stopLog();
    WScript.quit(exitProgram);
}
catch (exception) {
    logString("ERROR: " + exception);
    logString("**** ERROR Info ****");
    logString("\tMessage    : " + exception.message);
    logString("\tName       : " + exception.name);
    logString("\tCode       : " + (exception.number & 0xFFFF));
    logString("\tFacility   : " + (exception.number >> 16 & 0x1FFF));
    logString("\tDescription: " + exception.description);
    logString("********************");
    stopLog();
    WScript.StdErr.WriteLine("ERROR: " + exception);
    WScript.quit(2);
}
