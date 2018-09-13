/*
    Utility to do some processing in generation of the driver cab file
    This tool takes a sorted list of files and checks and strips duplicates.
    In addition, given a set of destination directories it will strip out files that don't exist in
    any one of these directories. The preference is specified in the order that they are listed.


    Author:

    Vijesh Shetty (vijeshs) 29-Sep-1998

    Revision History:


*/
#include "cabprep.h"


#define MAX_LOCATIONS 10
#define MAX_SETS 10

int _cdecl main ( int argc, char *argv[] );

int _cdecl main ( int argc, char *argv[] )
{
    PTCHAR i,c;
    TCHAR PreviousFile[MAX_PATH], FileNam[MAX_PATH], Path[MAX_SETS][MAX_LOCATIONS][MAX_PATH], DriverList[MAX_SETS][MAX_PATH];
    TCHAR CompressedPath[MAX_PATH], Buffer[512], Section[MAX_SETS][MAX_PATH];
    TCHAR CompPrependStr[MAX_PATH];
    FILE *Read_File, *Error_File, *Index_File, *Write_File, *Make_File;
    TCHAR Ddf_name[MAX_PATH];
    struct _tfinddata_t c_file;
    intptr_t h;
    int StrLen,rem,e,count,Found,num_path[MAX_SETS],num_sets=0,count2;



    if (argc < 2){
        _tprintf( TEXT("CABPREP /s:SortedFile,CabName,[SourceLocation1],[SourceLocation2..] /s:[..]\n"));
        _tprintf( TEXT("\n\nUtility to do some processing in generation of driver cab files.\n"));
        _tprintf( TEXT("This tool takes a sorted list of files and checks and strips duplicates.\n"));
        _tprintf( TEXT("In addition, given a set of destination directories it will strip out files\n"));
        _tprintf( TEXT("that don't exist in any one of these directories. The Source location preference follows\n"));
        _tprintf( TEXT("the order that they are specified in. Th tool checks for compressed files first\n"));
        _tprintf( TEXT("and then the real name. Errors reported in cabprep.err and the index file\n"));
        _tprintf( TEXT("created is drvindex.inf. Multiple sets can be given to create a single inf for many cabs.\n"));
        _tprintf( TEXT("Don't use any spaces between parameters for a given set.\n"));

        return 0;
    }



    if( argc >= 2 ){

        num_sets = argc - 1;
        for( count=0; count<num_sets; count++){
            num_path[count] = 0;

#ifdef UNICODE


            StrLen = MultiByteToWideChar(CP_ACP,
                                 MB_PRECOMPOSED,
                                 argv[count+1],
                                 -1,
                                 Buffer,
                                 sizeof(Buffer)/sizeof(WCHAR)
                                 );

            if( !StrLen ){
                _tprintf( TEXT("ERROR: Invalid Argument"));
                return -1;
            }

#else // !UNICODE

            lstrcpy( Buffer, argv[count+1] );

#endif //UNICODE


            Buffer[2] = L'\0';

            if( lstrcmpi( Buffer, TEXT("/S"))){
                _tprintf( TEXT("ERROR: Invalid Argument"));
                return -1;
            }
            else{

                i = Buffer + 3;
                StrLen = lstrlen(i);
                for( count2=0; count2 < StrLen; count2++ ){
                    if( i[count2] == L',' ){
                        i[count2] = L'\0';
                        num_path[count]++;
                    }
                }
                num_path[count] = num_path[count] - 1;
                if( num_path[num_sets] < 0) {
                    _tprintf( TEXT("ERROR: Need at least a source and target file"));
                    return -1;
                }
                lstrcpy( DriverList[count], i );
                if( !DriverList[count] ){
                    _tprintf( TEXT("ERROR: Invalid Source File"));
                    return -1;
                }
                i = i + lstrlen( i ) + 1;
                lstrcpy( Section[count], i );
                if( !Section[count] ){
                    _tprintf( TEXT("ERROR: Invalid Section Name"));
                    return -1;
                }
                i = i + lstrlen( i ) + 1;

                if( num_path[count] > (MAX_LOCATIONS) ){
                    _tprintf( TEXT("ERROR: Exceeded maximum no. of paths"));
                    return( -1 );
                }

                for( count2=0;count2 < num_path[count]; count2++ ){
                    lstrcpy( Path[count][count2], i );
                    if( !Path[count][count2] ){
                        _tprintf( TEXT("ERROR: Invalid Path"));
                        return -1;
                    }
                    i = i + lstrlen( i ) + 1;
                }

            }

        }

    }



    Index_File = _tfopen( TEXT("drvindex.inf"), TEXT("w") );
    Make_File = _tfopen( TEXT("makefil0"), TEXT("w") );

    _ftprintf( Index_File, TEXT("[Version]\n"));
    _ftprintf( Index_File, TEXT("signature=\"$Windows NT$\"\n"));
    _ftprintf( Index_File, TEXT("CabFiles=%s"), Section[0]);
    _ftprintf( Make_File, TEXT("#*****Automatically generated file by cabprep.exe for the build proces************\n\n\n") );
    _ftprintf( Make_File, TEXT("all: %s%s.cab"), Path[0][0], Section[0] );
    for( count=1; count < num_sets; count++ ){
        _ftprintf( Index_File, TEXT(",%s"), Section[count]);
        _ftprintf( Make_File, TEXT(" disk1\\%s.cab"), Section[count] );
    }

    _ftprintf( Make_File, TEXT("\n\n\n") );


    Error_File = _tfopen( TEXT("cabprep.err"), TEXT("w") );
    

    
    
    
    for( count2=0; count2 < num_sets; count2++ ){

        if( num_sets > 1)
            wsprintf( Ddf_name, TEXT("Out%d.ddf"), count2);  //use postfix digit for more than one cab
        else
            lstrcpy( Ddf_name, TEXT("Out.ddf") );

        Write_File = _tfopen( Ddf_name, TEXT("w") );
        _ftprintf( Write_File, TEXT(";*****Automatically generated file by cabprep.exe for Diamond************\n\n\n") );
        
        _ftprintf( Index_File, TEXT("\n\n\n[%s]\n"), Section[count2]);
        _ftprintf( Make_File, TEXT("\n%s%s.cab:  "), Path[count2][0], Section[count2] );



        _ftprintf( Write_File, TEXT(".New Cabinet\n") );
        _ftprintf( Write_File, TEXT(".Set CabinetName%d=%s.cab\n"), (count2+1), Section[count2] );
        _ftprintf( Write_File, TEXT(".Set MaxDiskSize=CDROM\n") );
        _ftprintf( Write_File, TEXT(".Set CompressionType=LZX\n") );
        _ftprintf( Write_File, TEXT(".Set CompressionMemory=21\n") );
        _ftprintf( Write_File, TEXT(".Set CompressionLevel=1\n") );
        _ftprintf( Write_File, TEXT(".Set Compress=ON\n") );
        _ftprintf( Write_File, TEXT(".Set Cabinet=ON\n") );
        _ftprintf( Write_File, TEXT(".Set UniqueFiles=ON\n") );
        _ftprintf( Write_File, TEXT(".Set FolderSizeThreshold=1000000\n") );
        _ftprintf( Write_File, TEXT(".Set MaxErrors=300\n\n\n\n") );



        Read_File = _tfopen( DriverList[count2], TEXT("r") );

        lstrcpy( PreviousFile, TEXT("$$$.#$$") );


        if (Read_File) {

            while( 1 ){

                // HAck because of bug that doesn't allow the use of _TEOF. Bcoz of the bug
                // fscanf returns EOF but fwscanf returns 0 when it should return 0xffff. So _TEOF
                // is useless and causes us to loop.

    #ifdef UNICODE
                if( (_ftscanf( Read_File, TEXT("%s"), FileNam )) == 0 )
    #else  //UNICODE
                if( (_ftscanf( Read_File, TEXT("%s"), FileNam )) == _TEOF )
    #endif //UNICODE
                    break;

                for( i = FileNam; i < FileNam + lstrlen( FileNam ); i++ )   {
                    *i = (TCHAR)towlower( *i );
                }



                if( lstrcmp( PreviousFile, FileNam )){

                    if(!num_path[count2])
                        _ftprintf( Write_File, TEXT("%s\n"), FileNam );

                    Found = 0;

                    // Try for each path that was given to us first with the compressed name and then uncompressed

                    for( count=0; count<num_path[count2]; count++){

                        //
                        //Construct the compressed full pathname
                        //

                        lstrcpy( CompPrependStr, Path[count2][count] );
                        lstrcat( CompPrependStr, FileNam );
                        c = _tcsrchr( CompPrependStr, L'.' );
                        e = lstrlen(CompPrependStr);
                        rem = (int)(CompPrependStr + e - c - 1);
                        if( rem < 3 ){
                            CompPrependStr[e] = '_';
                            CompPrependStr[e+1] = '\0';
                        }
                        else
                            CompPrependStr[e-1] = '_';

                        //Look for the compressed file

                        h=_tfindfirst(CompPrependStr,&c_file);
                        if(h != -1){
                            _ftprintf( Write_File, TEXT("%s\n"), CompPrependStr );
                            _ftprintf( Make_File, TEXT("\\\n\t%s  "), CompPrependStr );
                            _ftprintf( Index_File, TEXT("%s\n"), FileNam );
                            _findclose(h);
                            Found=1;
                            break;
                        }
                        else{

                            // Then the uncompressed path

                            _ftprintf( Error_File, TEXT("%s not found in %s \n"), CompPrependStr, Path[count2][count] );
                            lstrcpy( CompPrependStr, Path[count2][count] );
                            lstrcat( CompPrependStr, FileNam );
                            h=_tfindfirst(CompPrependStr,&c_file);
                            if(h != -1){
                                Found = 1;
                                _ftprintf( Write_File, TEXT("%s\n"), CompPrependStr );
                                _ftprintf( Make_File, TEXT("\\\n\t%s  "), CompPrependStr );
                                _ftprintf( Index_File, TEXT("%s\n"), FileNam );
                                _findclose(h);
                            break;
                            }
                            _ftprintf( Error_File, TEXT("%s not found in %s \n"), CompPrependStr, Path[count2][count] );
                        }

                    }

                    if( !Found )
                        _ftprintf( Error_File, TEXT("%s missing from all locations \n"), CompPrependStr );


                }
                else
                    _ftprintf( Error_File, TEXT("Duplicate found - Removing %s\n"), FileNam );

                lstrcpy( PreviousFile, FileNam );

            }

        }
        clearerr(Read_File);
        fflush(Read_File);
        fclose(Read_File);
        _ftprintf( Write_File, TEXT("\n\n\n\n") );
        fclose(Write_File);
        // The line below will try to run recab to find timestamp diffs only for the first sourcepath
        _ftprintf( Make_File, TEXT("\n\trecab .\\drvindex.inf %s.cab %s delta.lst"),Section[count2], Path[count2][0] );
        _ftprintf( Make_File, TEXT("\n\n"));

        // Below lines are not needed with incremental cabs being available. Build process takes care of it

        /*_ftprintf( Make_File, TEXT("\n\tdiamond /f %s"), Ddf_name );
        _ftprintf( Make_File, TEXT("\n\tcopy /y disk1\\%s.cab %s%s.cab"),Section[count2], Path[count2][0], Section[count2] );
        _ftprintf( Make_File, TEXT("\n\tdel /q disk1\\%s.cab\n\n\n\n"),Section[count2] );*/

        


    }//For (count2)

    _ftprintf( Index_File, TEXT("\n\n\n[Cabs]\n"));
    for( count=0; count < num_sets; count++ ){
        _ftprintf( Index_File, TEXT("%s=%s.cab\n"), Section[count],Section[count]);

    }

    _flushall();
    _fcloseall();





    return 0;



}
