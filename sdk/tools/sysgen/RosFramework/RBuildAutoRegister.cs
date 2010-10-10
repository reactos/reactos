using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    /// <summary>
    /// Type of registration
    /// </summary>
    public enum AutoRegisterType : int
    {
        DllRegisterServer = 1,
        DllInstall = 2,
        Both = 3
    }

    public enum SetupApiFolder : int
    {
        SourceDrive = 1,   //(the directory from which the INF file was installed) 
        OS          = 10,  //(%SystemRoot%) 
        System      = 11,  //(%SystemRoot%\system32) 
        Drivers     = 12,  //(%SystemRoot%\system32\drivers)
        Inf         = 17,  //(%SystemRoot%\inf)
        Help        = 18,  //(%SystemRoot%\Help)
        Fonts       = 20,  //(%SystemRoot%\Fonts)
        Root        = 24,  //(%SystemDrive%)
        Shared      = 25,  //(%ALLUSERSPROFILE%\Shared Documents)
        UserProfile = 53   //(%USERPROFILE%)
    }

    public enum SetupApiShellFolder : int
    {
        AllUsersApplicationData,                // 16419 %ALLUSERSPROFILE%\Application Data
        AllUsersDesktop,                        // 16409 %ALLUSERSPROFILE%\Desktop
        AllUsersMyDocuments,                    // 16430 %ALLUSERSPROFILE%\Documents
        AllUsersMyMusic,                        // 16437 %ALLUSERSPROFILE%\Documents\My Music
        AllUsersMyPictures,                     // 16438 %ALLUSERSPROFILE%\Documents\My Pictures
        AllUsersMyVideos,                       // 16439 %ALLUSERSPROFILE%\Documents\My Videos
        AllUsersFavourites,                     // 16415 %ALLUSERSPROFILE%\Favorites
        AllUsersStartMenu,                      // 16406 %ALLUSERSPROFILE%\Start Menu
        AllUsersStartMenuPrograms,              // 16407 %ALLUSERSPROFILE%\Start Menu\Programs
        AllUsersStartMenuAdministrativeTools,   // 16431 %ALLUSERSPROFILE%\Start Menu\Programs\Administrative Tools
        AllUsersStartMenuStartup,               // 16408 %ALLUSERSPROFILE%\Start Menu\Programs\Startup
        AllUsersTemplates,                      // 16429 %ALLUSERSPROFILE%\Templates
        UserApplicationData,                    // 16410 %USERPROFILE%\Application Data
        UserCookies,                            // 16417 %USERPROFILE%\Cookies
        UserDesktop,                            // 16384 %USERPROFILE%\Desktop
        UserDesktop2,                           // 16400 %USERPROFILE%\Desktop
        UserFavourites,                         // 16390 %USERPROFILE%\Favorites
        UserLocalSettingsApplicationData,       // 16412 %USERPROFILE%\Local Settings\Application Data
        UserLocalSettingsMSCDBruning,           // 16443 %USERPROFILE%\Local Settings\Application Data\Microsoft\CD Burning
        UserHistory,                            // 16418 %USERPROFILE%\Local Settings\History
        UserTemporaryInternetFiles,             // 16416 %USERPROFILE%\Local Settings\Temporary Internet Files
        UserMyDocuments,                        // 16389 %USERPROFILE%\My Documents
        UserMyMusic,                            // 16397 %USERPROFILE%\My Documents\My Music
        UserMyPictures,                         // 16423 %USERPROFILE%\My Documents\My Pictures
        UserMyVideos,                           // 16398 %USERPROFILE%\My Documents\My Videos
        UserNetHood,                            // 16403 %USERPROFILE%\NetHood
        UserPrintHood,                          // 16411 %USERPROFILE%\PrintHood
        UserRecent,                             // 16392 %USERPROFILE%\Recent
        UserSendTo,                             // 16393 %USERPROFILE%\SendTo
        UserStartMenu,                          // 16395 %USERPROFILE%\Start Menu
        UserStartMenuPrograms,                  // 16386 %USERPROFILE%\Start Menu\Programs
        UserStartMenuAdministrativeTools,       // 16432 %USERPROFILE%\Start Menu\Programs\Administrative Tools
        UserStartMenuStartup,                   // 16391 %USERPROFILE%\Start Menu\Programs\Startup
        UserTemplates,                          // 16405 %USERPROFILE%\Templates
        ProgramFiles,                           // 16422 %ProgramFiles%
        ProgramFilesCommonFiles,                // 16427 %ProgramFiles%\Common Files
        SystenResources,                        // 16440 %SystemRoot%\Resources
        SystemEnglishResources                  // 16441 %SystemRoot%\Resources\0409
    }

    /// <summary>
    /// An autoregister element specifies that the generated executable should be 
    /// registered in the registry during second stage setup.
    /// </summary>
    public class RBuildAutoRegister
    {
        private AutoRegisterType m_AutoRegisterType = AutoRegisterType.Both;
        private string m_InfSection = null;

        /// <summary>
        /// Name of section in syssetup.inf.
        /// </summary>
        public string InfSection
        {
            get { return m_InfSection; }
            set { m_InfSection = value; }
        }

        /// <summary>
        /// Type of registration.
        /// </summary>
        public AutoRegisterType Type
        {
            get { return m_AutoRegisterType; }
            set { m_AutoRegisterType = value; }
        }

        public int RegistrationType
        {
            get { return (int)Type; }
        }
    }
}
