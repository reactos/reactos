#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif//_MSC_VER

#include <string>
#include <vector>

#include <stdio.h>

#include "msvc.h"

#if 0

void _generate_dsp ( FILE* OUT, const string& module )
{
    my $dsp_file = modules[module]->{dsp_file};
    my $project = modules[module]->{project};
    my @imports = @{modules[module]->{imports}};

    my $lib = (modules[module]->{type} eq "lib");
    my $dll = (modules[module]->{type} eq "dll");
    my $exe = (modules[module]->{type} eq "exe");

    my $console = $exe; # FIXME: Not always correct

    my $msvc_wine_dir = do
	{
		my @parts = split(m%/%, $dsp_file);
		if($#parts == 1) {
			".." );
		} elsif($#parts == 2) {
			"..\\.." );
		} else {
			"..\\..\\.." );
		}
    };
    my $wine_include_dir = "$msvc_wine_dir\\include" );

    $progress_current++;
    $output->progress("$dsp_file (file $progress_current of $progress_max)");

    my @c_srcs = @{modules[module]->{c_srcs}};
    my @source_files = @{modules[module]->{source_files}};
    my @header_files = @{modules[module]->{header_files}};
    my @resource_files = @{modules[module]->{resource_files}};

    if ($project !~ /^wine(?:_unicode|build|runtests|test)?$/ &&
        $project !~ /^(?:gdi32)_.+?$/ &&
        $project !~ /_test$/)
    {
		push @source_files, "$project.spec" );
		# push @source_files, "$project.spec.c" );
		@source_files = sort(@source_files);
    }

    my $no_cpp = 1;
    my $no_msvc_headers = 1;
    if ($project =~ /^wine(?:runtests|test)$/ || $project =~ /_test$/) {
		$no_msvc_headers = 0;
    }

    my @cfgs;

    push @cfgs, "$project - Win32" );

    if (!$no_cpp) {
		my @_cfgs;
		foreach my $cfg (@cfgs) {
			push @_cfgs, "$cfg C" );
			push @_cfgs, "$cfg C++" );
		}
		@cfgs = @_cfgs;
    }

    if (!$no_release) {
		my @_cfgs;
		foreach my $cfg (@cfgs) {
			push @_cfgs, "$cfg Debug" );
			push @_cfgs, "$cfg Release" );
		}
		@cfgs = @_cfgs;
    }

    if (!$no_msvc_headers) {
		my @_cfgs;
		foreach my $cfg (@cfgs) {
			push @_cfgs, "$cfg MSVC Headers" );
			push @_cfgs, "$cfg Wine Headers" );
		}
		@cfgs = @_cfgs;
    }

    my $default_cfg = $cfgs[$#cfgs];

    fprintf ( OUT, "# Microsoft Developer Studio Project File - Name=\"$project\" - Package Owner=<4>\n" );
    fprintf ( OUT, "# Microsoft Developer Studio Generated Build File, Format Version 6.00\n" );
    fprintf ( OUT, "# ** DO NOT EDIT **\n" );
    fprintf ( OUT, "\n" );

    if ($lib) {
		fprintf ( OUT, "# TARGTYPE \"Win32 (x86) Static Library\" 0x0104\n" );
    } elsif ($dll) {
		fprintf ( OUT, "# TARGTYPE \"Win32 (x86) Dynamic-Link Library\" 0x0102\n" );
    } else {
		fprintf ( OUT, "# TARGTYPE \"Win32 (x86) Console Application\" 0x0103\n" );
    }
    fprintf ( OUT, "\n" );

    fprintf ( OUT, "CFG=$default_cfg\n" );
    fprintf ( OUT, "!MESSAGE This is not a valid makefile. To build this project using NMAKE,\n" );
    fprintf ( OUT, "!MESSAGE use the Export Makefile command and run\n" );
    fprintf ( OUT, "!MESSAGE \n" );
    fprintf ( OUT, "!MESSAGE NMAKE /f \"$project.mak\".\n" );
    fprintf ( OUT, "!MESSAGE \n" );
    fprintf ( OUT, "!MESSAGE You can specify a configuration when running NMAKE\n" );
    fprintf ( OUT, "!MESSAGE by defining the macro CFG on the command line. For example:\n" );
    fprintf ( OUT, "!MESSAGE \n" );
    fprintf ( OUT, "!MESSAGE NMAKE /f \"$project.mak\" CFG=\"$default_cfg\"\n" );
    fprintf ( OUT, "!MESSAGE \n" );
    fprintf ( OUT, "!MESSAGE Possible choices for configuration are:\n" );
    fprintf ( OUT, "!MESSAGE \n" );
    foreach my $cfg (@cfgs) {
		if ($lib) {
			fprintf ( OUT, "!MESSAGE \"$cfg\" (based on \"Win32 (x86) Static Library\")\n" );
		} elsif ($dll) {
			fprintf ( OUT, "!MESSAGE \"$cfg\" (based on \"Win32 (x86) Dynamic-Link Library\")\n" );
		} else {
			fprintf ( OUT, "!MESSAGE \"$cfg\" (based on \"Win32 (x86) Console Application\")\n" );
		}
    }
    fprintf ( OUT, "!MESSAGE \n" );
    fprintf ( OUT, "\n" );

    fprintf ( OUT, "# Begin Project\n" );
    fprintf ( OUT, "# PROP AllowPerConfigDependencies 0\n" );
    fprintf ( OUT, "# PROP Scc_ProjName \"\"\n" );
    fprintf ( OUT, "# PROP Scc_LocalPath \"\"\n" );
    fprintf ( OUT, "CPP=cl.exe\n" );
    fprintf ( OUT, "MTL=midl.exe\n" if !$lib && !$exe;
    fprintf ( OUT, "RSC=rc.exe\n" );

    my $n = 0;

    my $output_dir;
    foreach my $cfg (@cfgs) {
		if($#cfgs == 0) {
			# Nothing
		} elsif($n == 0) {
			fprintf ( OUT, "!IF  \"\$(CFG)\" == \"$cfg\"\n" );
			fprintf ( OUT, "\n" );
		} else {
			fprintf ( OUT, "\n" );
			fprintf ( OUT, "!ELSEIF  \"\$(CFG)\" == \"$cfg\"\n" );
			fprintf ( OUT, "\n" );
		}

		my $debug = ($cfg !~ /Release/);
		my $msvc_headers = ($cfg =~ /MSVC Headers/);

		fprintf ( OUT, "# PROP BASE Use_MFC 0\n" );

		if($debug) {
			fprintf ( OUT, "# PROP BASE Use_Debug_Libraries 1\n" );
		} else {
			fprintf ( OUT, "# PROP BASE Use_Debug_Libraries 0\n" );
		}

		$output_dir = $cfg;
		$output_dir =~ s/^$project - //;
		$output_dir =~ s/ /_/g;
		$output_dir =~ s/C\+\+/Cxx/g;
		if($output_prefix_dir) {
			$output_dir = "$output_prefix_dir\\$output_dir" );
		}

		fprintf ( OUT, "# PROP BASE Output_Dir \"$output_dir\"\n" );
		fprintf ( OUT, "# PROP BASE Intermediate_Dir \"$output_dir\"\n" );

		fprintf ( OUT, "# PROP BASE Target_Dir \"\"\n" );

		fprintf ( OUT, "# PROP Use_MFC 0\n" );
		if($debug) {
			fprintf ( OUT, "# PROP Use_Debug_Libraries 1\n" );
		} else {
			fprintf ( OUT, "# PROP Use_Debug_Libraries 0\n" );
		}
		fprintf ( OUT, "# PROP Output_Dir \"$output_dir\"\n" );
		fprintf ( OUT, "# PROP Intermediate_Dir \"$output_dir\"\n" );

		fprintf ( OUT, "# PROP Ignore_Export_Lib 0\n" if $dll;
		fprintf ( OUT, "# PROP Target_Dir \"\"\n" );

		my @defines;
		if($debug) {
			if($lib || $exe) {
				fprintf ( OUT, "# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od" );
				@defines = (qw(WINVER=0x0501 _WIN32_WINNT=0x0501 _WIN32_IE=0x0600 WIN32 _DEBUG _MBCS _LIB));
			} else {
				fprintf ( OUT, "# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od" );
				@defines = (qw(WINVER=0x0501 _WIN32_WINNT=0x0501 _WIN32_IE=0x0600 WIN32 _DEBUG _WINDOWS _MBCS _USRDLL), ("\U${project}\E_EXPORTS"));
			}
		} else {
			if($lib || $exe) {
				fprintf ( OUT, "# ADD BASE CPP /nologo /W3 /GX /O2" );
				@defines = (qw(WINVER=0x0501 _WIN32_WINNT=0x0501 _WIN32_IE=0x0600 WIN32 NDEBUG _MBCS _LIB));
			} else {
				fprintf ( OUT, "# ADD BASE CPP /nologo /MT /W3 /GX /O2" );
				@defines = (qw(WINVER=0x0501 _WIN32_WINNT=0x0501 _WIN32_IE=0x0600 WIN32 NDEBUG _WINDOWS _MBCS _USRDLL), ("\U${project}\E_EXPORTS"));
			}
		}

		foreach my $define (@defines) {
			fprintf ( OUT, " /D \"$define\"" );
		}
		fprintf ( OUT, " /YX" if $lib || $exe;
		fprintf ( OUT, " /FD" );
		fprintf ( OUT, " /GZ" if $debug;
		fprintf ( OUT, " " if $debug && ($lib || $exe);
		fprintf ( OUT, " /c" );
		fprintf ( OUT, "\n" );

		my @defines2;
		if($debug) {
			if($lib) {
				fprintf ( OUT, "# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od" );
				@defines2 = qw(WINVER=0x0501 _WIN32_WINNT=0x0501 _WIN32_IE=0x0600 WIN32 _DEBUG _WINDOWS _MBCS _LIB);
			} else {
				fprintf ( OUT, "# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od" );
				@defines2 = qw(WINVER=0x0501 _WIN32_WINNT=0x0501 _WIN32_IE=0x0600 _DEBUG WIN32 _WINDOWS _MBCS _USRDLL);
			}
		} else {
			if($lib) {
				fprintf ( OUT, "# ADD CPP /nologo /MT /W3 /GX /O2" );
				@defines2 = qw(WINVER=0x0501 _WIN32_WINNT=0x0501 _WIN32_IE=0x0600 WIN32 NDEBUG _WINDOWS _MBCS _LIB);
			} else {
				fprintf ( OUT, "# ADD CPP /nologo /MT /W3 /GX /O2" );
				@defines2 = qw(WINVER=0x0501 _WIN32_WINNT=0x0501 _WIN32_IE=0x0600 NDEBUG WIN32 _WINDOWS _MBCS _USRDLL);
			}
		}

		my @includes = ();
		if($wine) {
			push @defines2, "_\U${project}\E_" );
			push @defines2, qw(__WINESRC__) if $project !~ /^(?:wine(?:build|test)|.*?_test)$/;
			if ($msvc_headers) {
	    		push @defines2, qw(__WINE_USE_NATIVE_HEADERS);
			}
			my $output_dir2 = $output_dir;
			$output_dir =~ s/\\/\\\\/g;
			push @defines2, "__WINETEST_OUTPUT_DIR=\\\"$output_dir\\\"" );
			push @defines2, qw(__i386__ _X86_);

			if($project =~ /^gdi32_(?:enhmfdrv|mfdrv)$/) {
				push @includes, ".." );
			}

			if ($project =~ /_test$/) {
				push @includes, "$msvc_wine_dir\\$output_dir" );
			}

			if (!$msvc_headers || $project eq "winetest") {
				push @includes, $wine_include_dir;
			}
		}

		if($wine) {
			foreach my $include (@includes) {
				if ($include !~ /[\\\"]/) {
					fprintf ( OUT, " /I \"$include\"" );
				} else {
					fprintf ( OUT, " /I $include" );
				}
			}
		}

		foreach my $define (@defines2) {
			if ($define !~ /[\\\"]/) {
				fprintf ( OUT, " /D \"$define\"" );
			} else {
				fprintf ( OUT, " /D $define" );
			}
		}
		fprintf ( OUT, " /D inline=__inline" if $wine;
		fprintf ( OUT, " /D \"__STDC__\"" if 0 && $wine;

		fprintf ( OUT, " /YX" if $lib;
		fprintf ( OUT, " /FR" if !$lib;
		fprintf ( OUT, " /FD" );
		fprintf ( OUT, " /GZ" if $debug;
		fprintf ( OUT, " " if $debug && $lib;
		fprintf ( OUT, " /c" );
		fprintf ( OUT, " /TP" if !$no_cpp;
		fprintf ( OUT, "\n" );

		if($debug) {
			fprintf ( OUT, "# SUBTRACT CPP /X /YX\n" if $dll;
			fprintf ( OUT, "# ADD BASE MTL /nologo /D \"_DEBUG\" /mktyplib203 /win32\n" if $dll;
			fprintf ( OUT, "# ADD MTL /nologo /D \"_DEBUG\" /mktyplib203 /win32\n" if $dll;
			fprintf ( OUT, "# ADD BASE RSC /l 0x41d /d \"_DEBUG\"\n" );
			fprintf ( OUT, "# ADD RSC /l 0x41d" );
			if($wine) {
				foreach my $include (@includes) {
					fprintf ( OUT, " /i \"$include\"" );
				}
			}
			fprintf ( OUT, " /d \"_DEBUG\"\n" );
		} else {
			fprintf ( OUT, "# SUBTRACT CPP /YX\n" if $dll;
			fprintf ( OUT, "# ADD BASE MTL /nologo /D \"NDEBUG\" /mktyplib203 /win32\n" if $dll;
			fprintf ( OUT, "# ADD MTL /nologo /D \"NDEBUG\" /mktyplib203 /win32\n" if $dll;
			fprintf ( OUT, "# ADD BASE RSC /l 0x41d /d \"NDEBUG\"\n" );
			fprintf ( OUT, "# ADD RSC /l 0x41d" );
			if($wine) {
				foreach my $include (@includes) {
					fprintf ( OUT, " /i \"$include\"" );
				}
			}
			fprintf ( OUT, "/d \"NDEBUG\"\n" );
		}
		fprintf ( OUT, "BSC32=bscmake.exe\n" );
		fprintf ( OUT, "# ADD BASE BSC32 /nologo\n" );
		fprintf ( OUT, "# ADD BSC32 /nologo\n" );

		if($exe || $dll) {
			fprintf ( OUT, "LINK32=link.exe\n" );
			fprintf ( OUT, "# ADD BASE LINK32 " );
			my @libraries = qw(kernel32.lib user32.lib gdi32.lib winspool.lib
						   comdlg32.lib advapi32.lib shell32.lib ole32.lib
						   oleaut32.lib uuid.lib odbc32.lib odbccp32.lib);
			foreach my $library (@libraries) {
				fprintf ( OUT, "$library " );
			}
			fprintf ( OUT, " /nologo" );
			fprintf ( OUT, " /dll" if $dll;
				fprintf ( OUT, " /subsystem:console" if $console;
			fprintf ( OUT, " /debug" if $debug;
			fprintf ( OUT, " /machine:I386" );
			fprintf ( OUT, " /pdbtype:sept" if $debug;
			fprintf ( OUT, "\n" );

			fprintf ( OUT, "# ADD LINK32" );
			fprintf ( OUT, " /nologo" );
			fprintf ( OUT, " libcmt.lib" if $project =~ /^ntdll$/; # FIXME: Kludge
			foreach my $import (@imports) {
				fprintf ( OUT, " $import.lib" if ($import ne "msvcrt");
			}
			fprintf ( OUT, " /dll" if $dll;
				fprintf ( OUT, " /subsystem:console" if $console;
			fprintf ( OUT, " /debug" if $debug;
			fprintf ( OUT, " /machine:I386" );
			fprintf ( OUT, " /nodefaultlib" if $project =~ /^ntdll$/; # FIXME: Kludge
			fprintf ( OUT, " /def:\"$project.def\"" if $dll;
			fprintf ( OUT, " /pdbtype:sept" if $debug;
			fprintf ( OUT, "\n" );
		} else {
			fprintf ( OUT, "LIB32=link.exe -lib\n" );
			fprintf ( OUT, "# ADD BASE LIB32 /nologo\n" );
			fprintf ( OUT, "# ADD LIB32 /nologo\n" );
		}

		$n++;
    }

    if($#cfgs != 0) {
		fprintf ( OUT, "\n" );
		fprintf ( OUT, "!ENDIF \n" );
		fprintf ( OUT, "\n" );
    }

    if ($project eq "winebuild") {
		fprintf ( OUT, "# Begin Special Build Tool\n" );
		fprintf ( OUT, "SOURCE=\"\$(InputPath)\"\n" );
        fprintf ( OUT, "PostBuild_Desc=Copying wine.dll and wine_unicode.dll ...\n" );
		fprintf ( OUT, "PostBuild_Cmds=" );
		fprintf ( OUT, "copy ..\\..\\library\\$output_dir\\wine.dll \$(OutDir)\t" );
		fprintf ( OUT, "copy ..\\..\\unicode\\$output_dir\\wine_unicode.dll \$(OutDir)\n" );
		fprintf ( OUT, "# End Special Build Tool\n" );
    }
    fprintf ( OUT, "# Begin Target\n" );
    fprintf ( OUT, "\n" );
    foreach my $cfg (@cfgs) {
		fprintf ( OUT, "# Name \"$cfg\"\n" );
    }

    fprintf ( OUT, "# Begin Group \"Source Files\"\n" );
    fprintf ( OUT, "\n" );
    fprintf ( OUT, "# PROP Default_Filter \"cpp;c;cxx;rc;def;r;odl;idl;hpj;bat\"\n" );

    foreach my $source_file (@source_files) {
		$source_file =~ s%/%\\%g;
		if($source_file !~ /^\./) {
			$source_file = ".\\$source_file" );
		}

		if($source_file =~ /^(.*?)\.spec$/) {
			my $basename = $1;

			$basename = "$basename.dll" if $basename !~ /\..{1,3}$/;
			my $dbg_c_file = "$basename.dbg.c" );

			fprintf ( OUT, "# Begin Source File\n" );
			fprintf ( OUT, "\n" );
			fprintf ( OUT, "SOURCE=$dbg_c_file\n" );
			fprintf ( OUT, "# End Source File\n" );
		}

		fprintf ( OUT, "# Begin Source File\n" );
		fprintf ( OUT, "\n" );

		fprintf ( OUT, "SOURCE=$source_file\n" );

		if($source_file =~ /^(.*?)\.spec$/) {
			my $basename = $1;

			my $spec_file = $source_file;
			my $def_file = "$basename.def" );

			$basename = "$basename.dll" if $basename !~ /\..{1,3}$/;
			my $dbg_file = "$basename.dbg" );
			my $dbg_c_file = "$basename.dbg.c" );

			my $srcdir = "." ); # FIXME: Is this really always correct?

			fprintf ( OUT, "# Begin Custom Build\n" );
			fprintf ( OUT, "InputPath=$spec_file\n" );
			fprintf ( OUT, "\n" );
			fprintf ( OUT, "BuildCmds= \\\n" );
			fprintf ( OUT, "\t..\\..\\tools\\winebuild\\$output_dir\\winebuild.exe --def $spec_file > $def_file \\\n" );
			
			if($project =~ /^ntdll$/) {
				my $n = 0;
				foreach my $c_src (@c_srcs) {
					if($n++ > 0)  {
						fprintf ( OUT, "\techo $c_src >> $dbg_file \\\n" );
					} else {
						fprintf ( OUT, "\techo $c_src > $dbg_file \\\n" );
					}
				}
				fprintf ( OUT, "\t..\\..\\tools\\winebuild\\$output_dir\\winebuild.exe" );
				fprintf ( OUT, " -o $dbg_c_file --debug -C$srcdir $dbg_file \\\n" );
			} else {
				my $c_srcs = join(" ", grep(/\.c$/, @c_srcs));

				fprintf ( OUT, "\t..\\..\\tools\\winebuild\\$output_dir\\winebuild.exe" );
				fprintf ( OUT, " -o $dbg_c_file --debug -C$srcdir $c_srcs \\\n" );
			}

			fprintf ( OUT, "\t\n" );
			fprintf ( OUT, "\n" );
			fprintf ( OUT, "\"$def_file\" : \$(SOURCE) \"\$(INTDIR)\" \"\$(OUTDIR)\"\n" );
			fprintf ( OUT, "   \$(BuildCmds)\n" );
			fprintf ( OUT, "\n" );
			fprintf ( OUT, "\"$dbg_c_file\" : \$(SOURCE) \"\$(INTDIR)\" \"\$(OUTDIR)\"\n" );
			fprintf ( OUT, "   \$(BuildCmds)\n" );
			fprintf ( OUT, "# End Custom Build\n" );
		} elsif($source_file =~ /([^\\]*?\.h)$/) {
			my $h_file = $1;

			foreach my $cfg (@cfgs) {
				if($#cfgs == 0) {
					# Nothing
				} elsif($n == 0) {
					fprintf ( OUT, "!IF  \"\$(CFG)\" == \"$cfg\"\n" );
					fprintf ( OUT, "\n" );
				} else {
					fprintf ( OUT, "\n" );
					fprintf ( OUT, "!ELSEIF  \"\$(CFG)\" == \"$cfg\"\n" );
					fprintf ( OUT, "\n" );
				}

				$output_dir = $cfg;
				$output_dir =~ s/^$project - //;
				$output_dir =~ s/ /_/g;
				$output_dir =~ s/C\+\+/Cxx/g;
				if($output_prefix_dir) {
					$output_dir = "$output_prefix_dir\\$output_dir" );
				}

				fprintf ( OUT, "# Begin Custom Build\n" );
				fprintf ( OUT, "OutDir=$output_dir\n" );
				fprintf ( OUT, "InputPath=$source_file\n" );
				fprintf ( OUT, "\n" );
				fprintf ( OUT, "\"\$(OutDir)\\wine\\$h_file\" : \$(SOURCE) \"\$(INTDIR)\" \"\$(OUTDIR)\"\n" );
				fprintf ( OUT, "\tcopy \"\$(InputPath)\" \"\$(OutDir)\\wine\"\n" );
				fprintf ( OUT, "\n" );
				fprintf ( OUT, "# End Custom Build\n" );
			}

			if($#cfgs != 0) {
				fprintf ( OUT, "\n" );
				fprintf ( OUT, "!ENDIF \n" );
				fprintf ( OUT, "\n" );
			}
		}

		fprintf ( OUT, "# End Source File\n" );
    }
    fprintf ( OUT, "# End Group\n" );

    fprintf ( OUT, "# Begin Group \"Header Files\"\n" );
    fprintf ( OUT, "\n" );
    fprintf ( OUT, "# PROP Default_Filter \"h;hpp;hxx;hm;inl\"\n" );
    foreach my $header_file (@header_files) {
		fprintf ( OUT, "# Begin Source File\n" );
		fprintf ( OUT, "\n" );
		fprintf ( OUT, "SOURCE=.\\$header_file\n" );
		fprintf ( OUT, "# End Source File\n" );
    }
    fprintf ( OUT, "# End Group\n" );



    fprintf ( OUT, "# Begin Group \"Resource Files\"\n" );
    fprintf ( OUT, "\n" );
    fprintf ( OUT, "# PROP Default_Filter \"ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe\"\n" );
    foreach my $resource_file (@resource_files) {
		fprintf ( OUT, "# Begin Source File\n" );
		fprintf ( OUT, "\n" );
		fprintf ( OUT, "SOURCE=.\\$resource_file\n" );
		fprintf ( OUT, "# End Source File\n" );
    }
    fprintf ( OUT, "# End Group\n" );

    fprintf ( OUT, "# End Target\n" );
    fprintf ( OUT, "# End Project\n" );

    close(OUT);
}
#endif
void
MSVCBackend::_generate_dsw_header ( FILE* OUT )
{
    fprintf ( OUT, "Microsoft Developer Studio Workspace File, Format Version 6.00\n" );
    fprintf ( OUT, "# WARNING: DO NOT EDIT OR DELETE THIS WORKSPACE FILE!\n" );
    fprintf ( OUT, "\n" );
}

void
MSVCBackend::_generate_dsw_project (
	FILE* OUT,
	const Module& module,
	std::string dsp_file,
	const std::vector<Dependency*>& dependencies )
{
    dsp_file = std::string(".\\") + dsp_file;
	// TODO FIXME - what does next line do?
    //$dsp_file =~ y%/%\\%;
    
	// TODO FIXME - must they be sorted?
    //@dependencies = sort(@dependencies);

    fprintf ( OUT, "###############################################################################\n" );
    fprintf ( OUT, "\n" );
    fprintf ( OUT, "Project: \"%s\"=%s - Package Owner=<4>\n", module.name.c_str(), dsp_file.c_str() );
    fprintf ( OUT, "\n" );
    fprintf ( OUT, "Package=<5>\n" );
    fprintf ( OUT, "{{{\n" );
    fprintf ( OUT, "}}}\n" );
    fprintf ( OUT, "\n" );
    fprintf ( OUT, "Package=<4>\n" );
    fprintf ( OUT, "{{{\n" );
	for ( size_t i = 0; i < dependencies.size(); i++ )
	{
		Dependency& dependency = *dependencies[i];
		fprintf ( OUT, "    Begin Project Dependency\n" );
		fprintf ( OUT, "    Project_Dep_Name %s\n", dependency.module.name.c_str() );
		fprintf ( OUT, "    End Project Dependency\n" );
	}
	fprintf ( OUT, "}}}\n" );
	fprintf ( OUT, "\n" );
}

void
MSVCBackend::_generate_dsw_footer ( FILE* OUT )
{
	fprintf ( OUT, "###############################################################################\n" );
	fprintf ( OUT, "\n" );
	fprintf ( OUT, "Global:\n" );
	fprintf ( OUT, "\n" );
	fprintf ( OUT, "Package=<5>\n" );
	fprintf ( OUT, "{{{\n" );
	fprintf ( OUT, "}}}\n" );
	fprintf ( OUT, "\n" );
	fprintf ( OUT, "Package=<3>\n" );
	fprintf ( OUT, "{{{\n" );
	fprintf ( OUT, "}}}\n" );
	fprintf ( OUT, "\n" );
	fprintf ( OUT, "###############################################################################\n" );
	fprintf ( OUT, "\n" );
}

void
MSVCBackend::_generate_wine_dsw ( FILE* OUT )
{
	_generate_dsw_header(OUT);
	// TODO FIXME - is it necessary to sort them?
	for ( size_t i = 0; i < ProjectNode.modules.size(); i++ )
	//foreach my $module (sort(keys(%modules)))
	{
		Module& module = *ProjectNode.modules[i];

		// TODO FIXME - convert next line - I'm no perl/regex expert...
		//next if $module =~ /(?:winetest\.lib|wineruntests\.exe|_test\.exe)$/;

		//my $project = modules[module]->{project};
		std::string dsp_file = DspFileName ( module );

		// TODO FIXME - the following look like wine-specific hacks
		/*my @dependencies;
		if($project =~ /^wine(?:_unicode)?$/) {
			@dependencies = ();
		} elsif($project =~ /^winebuild$/) {
			@dependencies = ("wine", "wine_unicode");
		} elsif($project =~ /^(?:gdi32)_.+?$/) {
			@dependencies = ();
		} else {
			@dependencies = ("wine", "wine_unicode", "winebuild");
		}*/

		// TODO FIXME - more wine hacks?
        /*if($project =~ /^gdi32$/) {
			foreach my $dir (@gdi32_dirs) {
				my $dir2 = $dir;
				$dir2 =~ s%^.*?/([^/]+)$%$1%;

				my $module = "gdi32_$dir2";
				$module =~ s%/%_%g;
				push @dependencies, $module;
			}
        }*/

		_generate_dsw_project ( OUT, module, dsp_file, module.dependencies );
    }
    _generate_dsw_footer ( OUT );
}
