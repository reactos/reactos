#!/usr/bin/perl
#  ********************************************************************
#  * COPYRIGHT:
#  * Copyright (c) 2002-2007, International Business Machines Corporation and
#  * others. All Rights Reserved.
#  ********************************************************************

# Script to generate the icudata.jar and testdata.jar files.  This file is
# part of icu4j.  It is checked into CVS.  It is generated from
# locale data in the icu4c project.  See usage() notes (below)
# for more information.

# This script requires perl.  For Win32, I recommend www.activestate.com.

# Ram Viswanadha
# copied heavily from genrbjar.pl
use File::Find;
use File::Basename;
use IO::File;
use Cwd;
use File::Copy;
use Getopt::Long;
use File::Path;
use File::Copy;
use Cwd;
use Cwd 'abs_path'; 

main();

#------------------------------------------------------------------
sub main(){

    GetOptions(
             "--icu-root=s" => \$icuRootDir,
             "--jar=s" => \$jarDir,
             "--icu4j-root=s" => \$icu4jDir,
             "--version=s" => \$version,
             "--verbose"   => \$verbose,
             "--help"      => \$help
             );
    $cwd = abs_path(getcwd);
    
    if($help){
        usage();
    }
    unless (defined $icuRootDir){
        $icuRootDir =abs_path($cwd."/../../..");
    }
    unless (defined $icu4jDir){
        $icu4jDir =abs_path($icuRootDir."/../icu4j");
    }
    unless (defined $jarDir){
        if(defined $ENV{'JAVA_HOME'}){
            $jarDir=$ENV{'JAVA_HOME'}."/bin";
        }else{
            print("ERROR: JAVA_HOME enviroment variable undefined and --jar argument not specifed.\n");
            usage(); 
        }
    }
    
    $platform = getPlatform();
    $icuBinDir = $icuRootDir;
    
    $path=$ENV{'PATH'};
    
    if(($platform eq "cygwin") || ($platform eq "linux")){
        $icuBinDir .= "/source/bin";
        $icuLibDir = abs_path($icuBinDir."/../lib");
        $path .=":$icuBinDir:$icuLibDir";
        
        $libpath = $ENV{'LD_LIBRARY_PATH'}.":$icuLibDir";
        $ENV{'LD_LIBRARY_PATH'} = $libpath;
        
        #print ("#####  LD_LIBRARY_PATH = $ENV{'LD_LIBRARY_PATH'}\n");
    
    }elsif($platform eq "aix"){
    
        $icuBinDir .= "/source/bin";
        $icuLibDir = abs_path($icuBinDir."/../lib");
        $path .=":$icuBinDir:$icuLibDir";

        $libpath = $ENV{'LIBPATH'}.":$icuLibDir";
        $ENV{'LIBPATH'} = $libpath;
        #print ("#####  LIBPATH = $ENV{'LIBPATH'}\n");
    }elsif($platform eq "darwin"){ 
        $icuBinDir .= "/source/bin";
        $icuLibDir = abs_path($icuBinDir."/../lib");
        $path .=":$icuBinDir:$icuLibDir";
        
        $libpath = $ENV{'DYLD_LIBRARY_PATH'}.":$icuLibDir";
        $ENV{'DYLD_LIBRARY_PATH'} = $libpath;
       
    }elsif($platform eq "MSWin32"){
        $icuBinDir =$icuRootDir."/bin";
        $path .=$icuBinDir;
        
    }
    $ENV{'PATH'} = $path;
    #print ("#####  PATH = $ENV{'PATH'}\n");
    # TODO add more platforms and test on Linux and Unix
    
    $icuBuildDir =$icuRootDir."/source/data/out/build";
    $icuTestDataSrcDir =$icuRootDir."/source/test/testdata/";
    $icuTestDataDir =$icuRootDir."/source/test/testdata/out/build/";    
    
    # now build ICU
    buildICU($platform, $icuRootDir, $icuTestDataDir, $verbose);
    
    #figure out the version and endianess
    unless (defined $version){
        ($version, $endian) = getVersion();
        #print "#################### $version, $endian ######\n";
    }
    
    $icupkg = $icuBinDir."/icupkg -tb";
    $tempDir = $cwd."/temp";
    $version =~ s/\.//;
    $icu4jImpl = "com/ibm/icu/impl/data/";
    $icu4jDataDir = $icu4jImpl."icudt".$version."b";
    $icu4jDevDataDir = "com/ibm/icu/dev/data/";
    $icu4jTestDataDir = "$icu4jDevDataDir/testdata";
    
    $icuDataDir =$icuBuildDir."/icudt".$version.checkPlatformEndianess();
    
    #remove the stale directories
    unlink($tempDir);
    
    convertData($icuDataDir, $icupkg, $tempDir, $icu4jDataDir, $verbose);
    #convertData($icuDataDir."/coll/", $icupkg, $tempDir, $icu4jDataDir."/coll");
    createJar("\"$jarDir/jar\"", "icudata.jar", $tempDir, $icu4jDataDir, $verbose);
    
    convertTestData($icuTestDataDir, $icupkg, $tempDir, $icu4jTestDataDir, $verbose);
    createJar("\"$jarDir/jar\"", "testdata.jar", $tempDir, $icu4jTestDataDir, $verbose);
    copyData($icu4jDir, $icu4jImpl, $icu4jDevDataDir, $tempDir, $verbose);
}

#-----------------------------------------------------------------------
sub buildICU{
    local($platform, $icuRootDir, $icuTestDataDir, $verbose) = @_;
    $icuSrcDir = $icuRootDir."/source";
    $icuSrcDataDir = $icuSrcDir."/data";
    
    chdir($icuSrcDir);
    # clean the data directories
    unlink($icuBuildDir."../"); 
    unlink($icuTestDataDir."../"); 
    
    if(($platform eq "cygwin")||($platform eq "darwin")||($platform eq "linux")){
        
        # make all in ICU
        cmd("make all", $verbose);
        chdir($icuSrcDataDir);
        cmd("make uni-core-data", $verbose);
        if(chdir($icuTestDataSrcDir)){
            print("Invoking make in directory $icuTestDataSrcDir\n");
            cmd("make JAVA_OUT_DIR=\"$icu4jDir/src/com/ibm/icu/dev/test/util/\" all java-output", $verbose);
        }else{
	    die "Could not cd to $icuTestDataSrcDir\n";
        }
    }elsif($platform eq "aix"){
        # make all in ICU
        cmd("gmake all", $verbose);
        chdir($icuSrcDataDir);
        cmd("gmake uni-core-data", $verbose);
        chdir($icuTestDataDir."../../");
        cmd("gmake JAVA_OUT_DIR=\"$icu4jDir/src/com/ibm/icu/dev/test/util/\" all java-output", $verbose);
    }elsif($platform eq "MSWin32"){
        #devenv.com $projectFileName \/build $configurationName > \"$cLogFile\" 2>&1
        cmd("devenv.com allinone/allinone.sln /useenv /build Debug", $verbose);
        # build required data. this is required coz building icu will not build all the data
        chdir($icuSrcDataDir);
        cmd("NMAKE /f makedata.mak ICUMAKE=\"$icuSrcDataDir\" CFG=debug uni-core-data", $verbose);
        print "WARNING: Don't know how to build java-output on $platform. \n";
    }else{
        print "ERROR: Could not build ICU unknown platform $platform. \n";
        exit(-1);
    }
    
    chdir($cwd);    
}
#-----------------------------------------------------------------------
sub getVersion{
    my @list;
    opendir(DIR,$icuBuildDir);
    
    @list =  readdir(DIR);
    closedir(DIR);
    
    if(scalar(@list)>3){
        print("ERROR: More than 1 directory in build. Can't decide the version");
        exit(-1);
    }
    foreach $item (@list){
        next if($item eq "." || $item eq "..");
        my ($ver, $end) =$item =~ m/icudt(.*)(l|b|e)$/;
        return $ver,$end;
    }
}

#-----------------------------------------------------------------------
sub getPlatform{
    $platform = $^O;
    return $platform;
}
#-----------------------------------------------------------------------
sub createJar{
    local($jar, $jarFile, $tempDir, $dirToJar, $verbose) = @_;
    chdir($tempDir);
    $command="";
    print "INFO: Creating $jarFile\n";
    if($platform eq "cygwin") {
        $jar = `cygpath -au $jar`;
        chop($jar);
        $tempDir = `cygpath -aw $tempDir`;
        chop($tempDir);
        $tempDir =~ s/\\/\\\\/g;
    }
    if(defined $verbose){
        $command = "$jar cvf $jarFile -C $tempDir $dirToJar";
    }else{
        $command = "$jar cf $jarFile -C $tempDir $dirToJar";
    }
    cmd($command, $verbose);
}
#-----------------------------------------------------------------------
sub checkPlatformEndianess {
    my $is_big_endian = unpack("h*", pack("s", 1)) =~ /01/;
    if ($is_big_endian) {
        return "b";
    }else{
        return "l";
    }
}
#-----------------------------------------------------------------------
sub copyData{
    local($icu4jDir, $icu4jImpl, $icu4jDevDataDir, $tempDir) =@_;
    print("INFO: Copying $tempDir/icudata.jar to $icu4jDir/src/$icu4jImpl\n");
    mkpath("$icu4jDir/src/$icu4jImpl");
    copy("$tempDir/icudata.jar", "$icu4jDir/src/$icu4jImpl"); 
    print("INFO: Copying $tempDir/testdata.jar $icu4jDir/src/$icu4jDevDataDir\n");
    mkpath("$icu4jDir/src/$icu4jDevDataDir");
    copy("$tempDir/testdata.jar","$icu4jDir/src/$icu4jDevDataDir");
}
#-----------------------------------------------------------------------
sub convertData{
    local($icuDataDir, $icupkg, $tempDir, $icu4jDataDir)  =@_;
    my $dir = $tempDir."/".$icu4jDataDir;
    # create the temp directory
    mkpath($dir) ;
    # cd to the temp directory
    chdir($tempDir);
    my $endian = checkPlatformEndianess();
    my @list;
    opendir(DIR,$icuDataDir);
    #print $icuDataDir;
    @list =  readdir(DIR);
    closedir(DIR);
    my $op = $icupkg;
    #print "####### $endian ############\n";
    if($endian eq "l"){
        print "INFO: {Command: $op $icuDataDir/*.*}\n";
    }else{
       print "INFO: {Command: copy($icuDataDir/*.*, $tempDir/$icu4jDataDir/*)}\n";
    } 
    
    $i=0;
    # now convert
    foreach $item (@list){
        next if($item eq "." || $item eq "..");	 
       # next if($item =~ /^t_.*$\.res/ ||$item =~ /^translit_.*$\.res/ ||
       #         $item=~/$\.crs/ || $item=~ /$\.txt/ ||
       #         $item=~/icudata\.res/ || $item=~/$\.exp/ || $item=~/$\.lib/ ||
       #         $item=~/$\.obj/ || $item=~/$\.lst/);
        next if($item =~ /^t_.*$\.res/ ||$item =~ /^translit_.*$\.res/  ||
               $item=~/$\.crs/ || $item=~ /$\.txt/ ||
               $item=~/icudata\.res/ || $item=~/$\.exp/ || $item=~/$\.lib/ || $item=~/$\.obj/ ||
               $item=~/$\.lst/);
        if(-d "$icuDataDir/$item"){
            convertData("$icuDataDir/$item/", $icupkg, $tempDir, "$icu4jDataDir/$item/");
            next;
        }
        if($endian eq "l"){
           $command = $icupkg." $icuDataDir/$item $tempDir/$icu4jDataDir/$item";
           cmd($command, $verbose);
        }else{
           $rc = copy("$icuDataDir/$item", "$tempDir/$icu4jDataDir/$item");
           if($rc==1){
             #die "ERROR: Could not copy $icuDataDir/$item to $tempDir/$icu4jDataDir/$item, $!";
           }
        }

    }
    chdir("..");
    print "INFO: DONE\n";
}
#-----------------------------------------------------------------------
sub convertTestData{
    local($icuDataDir, $icupkg, $tempDir, $icu4jDataDir)  =@_;
    my $dir = $tempDir."/".$icu4jDataDir;
    # create the temp directory
    mkpath($dir);
    # cd to the temp directory
    chdir($tempDir);
    my $op = $icupkg;
    print "INFO: {Command: $op $icuDataDir/*.*}\n";
    my @list;
    opendir(DIR,$icuDataDir) or die "ERROR: Could not open the $icuDataDir directory for reading $!";
    #print $icuDataDir;
    @list =  readdir(DIR);
    closedir(DIR);
    my $endian = checkPlatformEndianess();
    $i=0;
    # now convert
    foreach $item (@list){
        next if($item eq "." || $item eq "..");
        next if( item=~/$\.crs/ || $item=~ /$\.txt/ ||
                $item=~/$\.exp/ || $item=~/$\.lib/ || $item=~/$\.obj/ ||
                $item=~/$\.mak/ || $item=~/test\.icu/ || $item=~/$\.lst/);
        $file = $item;
        $file =~ s/testdata_//g;
        if($endian eq "l"){ 
            $command = "$icupkg $icuDataDir/$item $tempDir/$icu4jDataDir/$file";
            cmd($command, $verbose);
        }else{
            #print("Copying $icuDataDir/$item $tempDir/$icu4jDataDir/$file\n");
            copy("$icuDataDir/$item", "$tempDir/$icu4jDataDir/$file");
        }
        

    }
    chdir("..");
    print "INFO: DONE\n";
}
#------------------------------------------------------------------------------------------------
sub cmd {
    my $cmd = shift;
    my $verbose = shift;
    my $prompt = shift;
    
    $prompt = "Command: $cmd.." unless ($prompt);
    if(defined $verbose){
        print $prompt."\n";
    }
    system($cmd);
    my $exit_value  = $? >> 8;
    #my $signal_num  = $? & 127;
    #my $dumped_core = $? & 128;
    if ($exit_value == 0) {
        if(defined $verbose){
            print "ok\n";
        }
    } else {
        ++$errCount;
        print "ERROR: Execution of $prompt returned ($exit_value)\n";
        exit(1);
    }
}
#-----------------------------------------------------------------------
sub usage {
    print << "END";
Usage:
gendtjar.pl
Options:
        --icu-root=<directory where icu4c lives>
        --jar=<directory where jar.exe lives>
        --icu4j-root=<directory>
        --version=<ICU4C version>
        --verbose
        --help
e.g:
gendtjar.pl --icu-root=\\work\\icu --jar=\\jdk1.4.1\\bin --icu4j-root=\\work\\icu4j --version=3.0
END
  exit(0);
}


