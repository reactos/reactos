//
// Mon-8/4/97
//
// Help ID strings and map numbers for color_cs.rtf
//
//

#include <help.h>

#define WINDOWS_HELP_FILE   TEXT("windows.hlp")

// Help topics.
//
// Click to associate the selected device with the profile displayed on the Profile Information tab.

#define IDH_DISABLED               (-1)


#define IDH_ASSOCDEVICE_ADD        99990000 // Color Mgt, Associate Device tab on Color Profile property sheet

// Removes the association between the selected device and the profile displayed on the Profile Information tab.

#define IDH_ASSOCDEVICE_REMOVE     99990001 // Color Mgt, Associate Device tab on Color Profile property sheet

#if !defined(_WIN95_)

// Displays the name of the color profile you want to associate to a device.
//
// (Winnt Only)

#define IDH_ASSOCDEVICE_NAME       99990002 // Color Mgt, Associate Device tab on Color Profile property sheet

// Lists the hardware devices on this computer that are associated with this color profile.
//
// (Winnt Only)

#define IDH_ASSOCDEVICE_LIST       99982140 // Color Mgt, Associate Device, List 

#endif // !defined(_WIN95_)

// Click to associate a new profile with this printer.

#define IDH_110_132                99810132 // Color Mgt, Printer, Add Button
#define IDH_PRINTERUI_ADD          99810132

// Removes a profile from the list.

#define IDH_110_133                99790133 // Color Mgt, Printer, Remove Button
#define IDH_PRINTERUI_REMOVE       99790133

// Lists all the color profiles currently associated with this printer.
// Click a profile to make it the active profile.

#define IDH_110_141                99960141 // Color Mgt, Printer, List Box, 
#define IDH_PRINTERUI_LIST	       99960141	


// printer ui automatic profile selection radio button

#define IDH_PRINTERUI_AUTOMATIC    99990006

// printer ui manual selection radio button

#define IDH_PRINTERUI_MANUAL       99990007

// Printer UI default text item

#define IDH_PRINTERUI_DEFAULTTEXT  99990008

// Set as default printer ui button.

#define IDH_PRINTERUI_DEFAULTBTN   99990009


// Click to associate a new profile with this monitor.

#define IDH_111_132                99821132 // color mgt, Monitor, Add Button
#define IDH_MONITORUI_ADD          99821132

// Removes a profile from the list.

#define IDH_111_133                99801133 // Color Mgt, Monitor, Remove Button
#define IDH_MONITORUI_REMOVE       99801133

// Makes the selected profile the default profile.

#define IDH_111_134                99781134 // Color Mgt, Set as Default
#define IDH_MONITORUI_DEFAULT      99781134

// Lists all the color profiles currently associated with this monitor.
// Click a profile to make it the active profile. Otherwise, the default profile is the active profile.

#define IDH_111_141                99971141 // Color Mgt, Monitor, List box
#define IDH_MONITORUI_LIST         99971141

// Displays the name of the current monitor.

#define IDH_111_150                99891150 // Color Mgt, Monitor, name here	
#define IDH_MONITORUI_DISPLAY      99891150	

// Displays the name of the current default profile for this monitor.

#define IDH_111_152                99851152 // Color mgt, Monitor, Edit
#define IDH_MONITORUI_PROFILE      99851152

#if defined(_WIN95_)

// Click to associate the selected device with this color profile.

#define IDH_112_130                99832130
#define IDH_ADDDEVICEUI_ADD        99832130

#else

#define IDH_112_130                IDH_ASSOCDEVICE_ADD 
#define IDH_ADDDEVICEUI_ADD        IDH_ASSOCDEVICE_ADD 

#endif // defined(_WIN95_)

#if defined(_WIN95_)

// Lists all the hardware devices on this computer that can be associated with this color profile.

#define IDH_112_140                99982140 // Color Mgt, Add Device, List Box
#define IDH_ADDDEVICEUI_LIST       99982140

#else

// Lists the hardware devices on this computer that can be associated with this color profile.

#define IDH_112_140                99982141 // Color Mgt, Add Device, List Box
#define IDH_ADDDEVICEUI_LIST       99982141

#endif

#if defined(_WIN95_)

// Saves your changes and leaves the dialog box open.

#define IDH_1548_1024              99861024
#define IDH_ICMUI_APPLY            99861024

#else

#define IDH_ICMUI_APPLY            IDH_COMM_APPLYNOW
#define IDH_1548_1024              IDH_COMM_APPLYNOW

#endif // defined(_WIN95_)

// Click to enable color management for this document.

#define IDH_1548_1040              99941040 // Color Mgt, Enable 
#define IDH_APPUI_ICM              99941040

#if defined(_WIN95_)

// Lists the profiles you can use with this monitor.

#define IDH_1548_1136              99911136
#define IDH_APPUI_MONITOR          99911136

#else

#define IDH_1548_1136              IDH_MONITORUI_LIST
#define IDH_APPUI_MONITOR          IDH_MONITORUI_LIST

#endif // defined(_WIN95_)

#if defined(_WIN95_)

// Lists the profiles you can use with this printer.

#define IDH_1548_1137              99901137
#define IDH_APPUI_PRINTER          99901137

#else

#define IDH_1548_1137              IDH_PRINTERUI_LIST
#define IDH_APPUI_PRINTER          IDH_PRINTERUI_LIST

#endif // defined(_WIN95_)

// Lists the rendering intents you can use.
//
//  A rendering intent is the approach used to map the colors of an image 
// to the color gamut of a monitor or printer. The color gamut is the range
// of color that a device can produce.
//
//  Perceptual matching is best for photographic images.
// All the colors of one gamut are scaled to fit within another gamut.
// The relationship between colors is maintained.
//
//  Saturation matching is best for graphs and pie charts,
// in which vividness is more important than actual color. The relative
// saturation of colors is maintained from gamut to gamut. Colors outside
// the gamut are changed to colors of the same saturation, but different
// degrees of brightness, at the edge of the gamut.
//
//  Relative Colorimetric matching is best for logo images,
// in which a few colors must be matched exactly. The colors that fall
// within the gamuts of both devices are left unchanged. Other colors 
// may map to a single color, however, resulting in tone compression.
//
//  Absolute Colorimetric matching is used for mapping to a device-independent
// color space. The result is an idealized print viewed on a perfect paper with
// a large dynamic range and color gamut.

#define IDH_1548_1138              99881138 // Color Mgt, Rendering Intent
#define IDH_APPUI_INTENT           99881138

// Lists the profiles you can use to emulate another device on your monitor and printer.
// The profile could represent another monitor or printer, a printing press, color space,
// or any other output device.

#define IDH_1548_1139              99871139 // Color Mgt, Profile other device
#define IDH_APPUI_EMULATE          99871139

// Click to turn on basic color management, which coordinates the way a document's 
// colors appear on your monitor and printer.

#define IDH_1548_1056              99771056 // Color Mgt, Basic Color Mgt
#define IDH_APPUI_BASIC            99771056

// Click to turn on proofing, which lets you simulate or emulate
// how colors will appear on a certain printer or monitor.

#define IDH_1548_1057              99991057 // Color Mgt, Proofing
#define IDH_APPUI_PROOF            99991057

// Lists the rendering intents you can use.
//
//  A rendering intent is the approach used to map the colors of an image
// to the color gamut of a monitor or printer. The color gamut is the range
// of color that a device can produce.
//
//  Perceptual matching is best for photographic images. 
// All the colors of one gamut are scaled to fit within another gamut.
// The relationship between colors is maintained.
//
//  Saturation matching is best for graphs and pie charts,
// in which vividness is more important than actual color. The relative
// saturation of colors is maintained from gamut to gamut. Colors outside
// the gamut are changed to colors of the same saturation, but different
// brightness, at the edge of the gamut.
//
//  Relative Colorimetric matching is best for logo images,
// in which a few colors must be matched exactly. The colors that fall 
// within the gamuts of both devices are left unchanged. Other colors
// may map to a single color, however, resulting in tone compression.
//
//  Absolute Colorimetric matching is used for mapping to a device-independent 
// color space. The result is an idealized print viewed on a perfect paper with
// a large dynamic range and color gamut.
 
#define IDH_1548_1140              99841140	// Color Mgt, Rendering Intent
#define IDH_APPUI_INTENT2          99841140 // not used (same as above)

#if !defined(_WIN95_)

// Display the name of the color profile being used by this file.

#define IDH_colormanage_profile_name 99990003 // Color Mgt, Imaging, Profile Name
#define IDH_APPUI_SOURCE             99990003

#define IDH_SCANNERUI_LIST           11023    // Color Mgt, list of profiles
#define IDH_SCANNERUI_ADD            11024    // Color Mgt, add button
#define IDH_SCANNERUI_REMOVE         11025    // Color Mgt, remove button

#endif

