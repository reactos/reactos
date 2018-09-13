
THIS TOOL IS NOT SUPPORTED BY MICROSOFT CORPORATION. IT IS PROVIDED "AS IS" BECAUSE WE BELIEVE IT MAY BE USEFUL TO YOU.

NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE

It is necessary to build FrameWrk before building ToDoSvr or WebImage.
Running "nmake" from the samples\basectl directory will build all
of the sample code in the correct order.  Before building any of the
samples, run setenv.bat to add the ActiveX SDK include, bin, and lib
directories to the front of your environment (they must be ahead of
the Win32 SDK and VC++).

More information on the Internet-aware OLE controls is available in 
readme.txt files in the sample directories.


The ActiveX Controls Framework Sample Code for Authoring non-MFC Controls
=====================================================================


This document presents a short discussion on using the ActiveX Controls Framework to author new ActiveX controls.


0. Contents
=---------=

1.0  Introduction
   1.1  Target Audience
   1.2  Structure of the Framework
   1.3  Target Environment

2.0  Creating an ActiveX Control
   2.1  Building the Control
   2.2  Working with the In-Process Server

3.0  Working with Your ActiveX Control
   3.1  Structure of a Control
   3.2  Painting a Control
   3.3  Handling Messages in a Control
   3.4  Adding a Property
   3.5  Adding a Method
   3.6  Adding an Event
   3.7  Using Standard ActiveX Types
   3.8  Throwing an Exception

4.0  Persistence
   4.1  Text Persistence
   4.2  Binary Persistence

5.0  Property Pages
   5.1 Working with a Property Page
   5.2 Navigating through Associated Objects
   5.3 Marking Your Page as Dirty
  
6.0  String Manipulation
   6.1 Types of Strings
   6.2 Working with Strings

7.0  Localization
   7.1  Setting up for Localization

8.0 Miscellaneous
   8.1 Recommended Reading
   8.2 Host-Specific Notes
      8.2.1 Microsoft Access 95

1.0 Introduction
=--------------=

The ActiveX Controls Framework is a sample code base from which one can author new ActiveX controls for use in existing containers, such as Microsoft Visual Basic, Microsoft Access, and Microsoft Visual FoxPro, or in future containers of ActiveX Controls.

It differs from the Microsoft Visual C++ CDK in many ways.  Most notably, this framework is intended to be considerably more "bare bones."  Only minimal functionality is provided to the programmer.  Little in the way of default handlers for various windows messages, OLE events, et. al. has been provided.  The ability to add them is there, but the code is not.  The code base has also been designed primarily for performance and reduced code size as much as possible.  Whenever a choice between ease of use and
 performance arose, the latter was typically chosen.

In addition, the code is designed to take advantage of whatever OC '96 features are available from the host.  The code will use any available features, and fall back to the 'older' behaviour for those cases where they aren't.

The code base is extremely extensible, however, and of course, all the source code is there -- if something doesn't do what you want it to, make it.



  1.1 Target Audience

This framework targets a slightly more advanced programmer than the Microsoft Visual C++ Control Developers Kit.  Specifically, the programmer will be required to understand some of the fundamentals of OLE automation and dual interfaces.  The user will have to be able to understand and modify an .ODL file on their own.  In addition, the user will be required to understand and be able to work with OLE persistence interfaces, most notably IStream IPersistPropertyBag and IPersistStream.  However, if it is not desired, the user will not be required to have much knowledge of OLE embedding interfaces.

Programmers who do not have specific performance requirements, those not familiar with many of the pertinent OLE technologies, or those who work primarily with the Microsoft Foundation Classes will find the MFC/CDK far more suited to their needs.



   1.2 Structure of the Framework

The directory structure of the ActiveX Controls Framework is as follows:


	 BaseCtl +
		 |- \FrameWrk
		 |
		 |- \IELnk
		 |
		 |- \IEMIME
		 |
		 |- \Include
		 |
		 |- \Lib
		 |
		 |- \ToDoSvr
		 |
		 +- \WebImage


The BaseCtl\Framewrk and BaseCtl\Include directories contain the core code for writing an ActiveX control.  The BaseCtl\Include directory contains the headers that most controls will get their information from, and the BaseCtl\Framewrk directory contains the core functionality (in the COleControl class) which compiles into a static library (.LIB) form.

A few samples are provided with the framework.  

   1.3  Target Environment

This framework was developed assuming you have the Microsoft Visual C++ 4.0 (or later) toolset in your path and the ActiveX SDK installed. Running 'nmake' in BaseCtl or BaseCtl\FrameWrk will build debug a version of the static library.  'nmake nodebug=1' will build a retail (nodebug) static library.

All of the makefiles and build processes are command-line based.  Various people have reported, however, that it's largely trivial to integrate that into their favorite environments.

NOTE: Under Windows 95, Visual C++ will not, by default, register its environment variables to set up for command line builds.  To enable command line building you should go the the readme.txt installed with the ActiveX SDK and read on how to setup your environment variables.   Occasionally on some Windows 95 machines you may get a bunch of "Out of Environment Space" messages when doing this.  In the properties dialog for the Command Prompt, you can increase the size of the environment from "Auto" to some number like 1024, and this takes care of the problem.  None of this should happen when developing under Windows NT.



The file dwinvers.h in Controls\Framewrk is a file that contains version and copyright information that should be updated by you each time you run your builds.  The framework does not do this work for you.


2.0 Creating an ActiveX Control
=-------------------------=

Creating an ActiveX control using this framework proves moderately straightforward.  We will go into the process in brief here.  Templating off the sample code should be able to fill in the holes.

The framework implements the core functionality in a few C++ classes, notably CAutomationObject, COleControl (which inherits from CAutomationObject), and CPropertyPage.  All objects inherit from CUnknownObject, which provides the support for aggregation.

So, to write an ActiveX Control, you need to declare a new object which inherits from COleControl.  In addition, you'll need to inherit from some sort of Automation interface that describes the properties and methods for your control, say, IMyControl.  This interface description is generated by MKTYPLIB and will be put in some output file created by MKTYPLIB.  COleControl has a number of virtual methods that are declared as pure, which you simply must implement in your control class.  These include WindowProc, LoadBinaryState, LoadTextState, SaveBinaryState, SaveTextState, OnDraw, and RegisterClassData.

To write a property page, you declare a new object which inherits from CPropertyPage.  This object must implement a DialogProc.  You can also implement automation objects and collections by declaring a new object that inherits from CAutomationObject.

Since an ActiveX control is an In-Process OLE Server, you also need one file to describe all your objects, whether they be controls, automation objects, or property pages.  This file will have a bunch of information in it, including a table of all objects and information about them.  In addition, it'll have information on what sort of localization your server would like to use, and what sort of licensing support you'd like to have.

Finally, you'll need a resource file, an .ODL file to describe your interfaces and event interfaces, a .DEF file for your linking information, and a file to define all the guids that have been declared.

 You can template all this information from one of the sample controls. 

   2.1  Building the Control

To build your control, you first need to generate the libraries for the Framework files.  The libraries are not automatically generated; this allows you to modify the libraries if desired.  Doing this is as simple as going to the BaseCtl\Framewrk directory and typing 'nmake' (for debug) or 'nmake nodebug=1' (for retail). You will only need to recompile the framework files if you make a change to a file in the Controls\Include or Controls\Framewrk directories.

Once the framework files have been built, go back to your control's subdirectory, and type "nmake" or "nmake nodebug=1" there.  This will build your control.


   2.2  Working with the In-Process Server

Your ActiveX control, any property pages, and automation objects are all just OLE COM objects in an in-process server.  All of these objects must be declared in a global table, g_ObjectInfo, which is found in the main in-proc server file.  Each object is declared with a wrapper, one of CONTROLOBJECT, PROPERTYPAGE, or AUTOMATIONOBJECT.  The name of the object is entered as an argument to the macro.  In the header file where you declare the COM object, you'll need to use one of DEFINE_PROPERTYPAGEOBJECT, DEF
INE_CONTROLOBJECT, DEFINE_WINDOWLESSCONTROL, or DEFINE_AUTOMATIONOBJECT to actually declare the object for use in the global table.  If it turns out that you are declaring objects that aren't createable from a class factory, they still do need to be declared in the global table, but the Creation function specified in the structure should be left as NULL.

There is some additional information that must be put in the file for the in-process server.  The LIBID of the type library (.TLB) must be put in the global variable g_pLibid.  You must indicate what sort of localization your control supports by using the variables g_fSatelliteLocalization and g_lcidLocale.   See Section 7.0, "Localization," for more information.

In your in-proc server file, there are a few routines that you can put code in. The first two, InitializeLibrary() and UninitializeLibrary(), are called when the DLL is first loaded into and unloaded from memory.  This is a good place to do any sort of initialization that can't be put off until later.  It is worth noting that for performance reasons, delaying as much as possible is often a good idea.  The CheckForLicense function lets the control decide if it is licensed or not to run.  GetLicenseKey() is called by the class factory.  You should return your license key here if you support licensing, or just return "" otherwise.

The RegisterData and UnregisterData routines are called from DllRegisterServer and DllUnregisterServer, and can be used to register and clean up additional information in the registry.

Finally, two small pieces of code are included in this file so that your project does not have to link with any of the C-runtimes.  This typically results in smaller DLL size, and can help with performance.  If you want to link with the C runtime libraries, or the CRTDLL libraries, then you should remove these last things from the in-proc server file.


3.0  Working with Your ActiveX Control
=--------------------------------=

Once you have your control up and running, you'll want to start extending its functionality.  The first thing to note is that your control is, in many ways, much like a regular Windows program.  Even if your control doesn't have a window [ie, it's taking advantage of the windowless features of the OC '96 specification], you have a window proc, and you have to paint the client area yourself using regular windows drawing APIs and handles to Device Contexts (DC's).

To declare your control, you'll use one of DEFINE_CONTROLOBJECT and DEFINE_WINDOWLESSCOTNROL in your control's header file.  These structures are mostlystraightforward and self-documenting, but there are two parameters worth a little extra dicussion.

The first is the dwActivationPolicy parameter.  The framework allows your control to take advantage of the delayed activation features provided by IPointerInactive.  If your control wishes to use any of these, then you should insert an activation policy in to this field.  By default, it is 0.  Subclassed windows controls should not change this.  Other control authors may wish to take advantage of this interface, and should consult the OC '96 specification for more information.

The second non-trivial parameter is a boolean passed in to the DEFINE_WINDOWLESSCONTROL macro which indicates whether your control will have any transparent regions or not.  If it is true, the host will have to do a little extra work to ensure that your control paints correctly in the z-order of things.  If it is false, your control is required to paint it's entire client area, but the host has to do a little less work.


   3.1 Structure of a Control

There is a core set of methods that every control in this framework must implement, based on creation semantics and methods that COleControl simply does not provide for you.  There are also a bunch of routines that are interesting to override and provide an implementation for.  The following is a discussion of many of these.

   a. static Create() function. Every control must create their control object in this routine, and then return a pointer to its private unknown (for aggregation support).

   b. Constructor and Destructor.  These are also generated for you, and a control should initialize anything here.  Controls should try to minimize the amount of work that is done here in order to help prevent load time from degrading unacceptably.

   c. RegisterClassData().  All controls must implement this routine.  Even if your control will be windowless for the most part, it is entirely possible that a user will place your control in a host that doesn't support all of the OC '96 features, and the framework will be forced to create an HWND for you control.  In this case, you will need a window class.  This routine will only be called once the first time a control of the given type is loaded in a process in a situation in which it must have an HWND
.  Controls should register their window class (using RegisterClass and the WNDCLASS structure) here.  In addition, subclassed windows controls should get a pointer to the parent control's WindowProc and set that up in the g_ObjectInfo table using the SUBCLASSWNDPROCOFCONTROL() macro.  See the Button and Circle control samples for examples of how this is done.  Invisible at Runtime controls should just return FALSE in this routine, as it should never get called.

   d. BeforeCreateWindow() and AfterCreateWindow() are not mandatory to implement, but are extremely interesting routines.  BeforeCreateWindow() is called right before the call to CreateWindow() if your control is windowed, but after persistent state has been loaded.  Controls should use this opportunity to set the window title for their control in m_szWindowTitle, and can also set up bits in pdwWindowStyle and pdwWindowStyleEx parameters that are passed in.  In addition, controls can set their initial cap
tion here in the parameter passed in for this purpose.  This does, however, have a limit as defined in CreateInPlaceWindow in ctlmisc.cpp.  Control writers should use this opportunity to set up these defaults as much as possible as opposed to in the message handler for WM_CREATE, as it typically results in much better performance.

   e. InternalQueryInterface().  Your control implements this to support the QI for the control's primary automation interface, such as IMyControl.  You can also use this method to support additional interfaces in your control.  For example, if you want to support IPerPropertyBrowsing, you'd have your control's CMyControl class inherit from IPerPropertyBrowsing, and support the QueryInterface for IID_IPerPropertyBrowsing in InternalQueryInterface.  If the control fails the QI, then the control should deleg
ate back to COleControl::InternalQueryInterface to see if it likes the IID.

   f. LoadTextState, LoadBinaryState, SaveTextState, SaveBinaryState.  All controls must implement these persistence routines.  See Section 4.0, "Persistence," for a discussion of these interfaces.

   g. OnDraw.  This routine is called when your control is expected to draw itself.  In Design mode, this call will originate from a container calling IViewObject2::Draw.  In run mode, the controls framework will intercept the WM_PAINT message, and will translate it into a call to your OnDraw routine.  See Section 3.2, "Painting a Control," for more information on this routine.

   h. WindowProc.  Messages that are not handled in the framework code (such as SimpleFrame messages and WM_PAINT), are sent to your control here.  Your control should deal with these here.  See Section 3.3, "Handling Messages in a Control," for a discussion of this routine.  In addition, please see the note below on OnSpecialKey for more information.

   i. OnSpecialKey.  Messages for various keyboard events, such as moving the Cursor keys, function keys, and other non-standard keys do not go to the WindowProc.  Instead, they are sent to the OnSpecialKey routine.  If you want your control to handle special keys and accelerators you should override and implement this routine.  The control should return TRUE if it handles a key, or FALSE if it ignores the key.

   j. DoCustomVerb.  If you choose to have your control implement custom verbs in addition to the property page one that is provided by default (provided your control has a property page), then you should implement this routine and take appropriate action depending on what verb was sent in.  Return OLEOBJ_S_INVALIDVERB if you don't recognize the verb given.

   k. OnSetExtent.  This is called every time your control is resized.  The m_Size SIZEL structure is your control's current size in pixels.  Controls should look in here for their size information and override OnSetExtent if they want control over how their control is sized.  Please see the Invisible sample for an example of a control that is of a fixed size.

   l. OnQuickActivate.  This routine is called if your control is in a host that supports quick activation, and has used IQuickActivate::QuickActivate to activate your control.  Your control will be given a pointer to a QACONTAINER structure, and should examine the data in it which it finds interesting.  A non-trivial number of ambients will be passed to the control in this way, and will save later calls to GetAmbientProperty, which is not cheap.  Please see documentation for IQuickActivate [OC '96 specifi
cation] for more information.


In addition, the following methods/routines can be called by the ActiveX control, and often prove to be extremely useful.

   a. DoSuperClassPaint.  Subclassed window controls can call this from their OnDraw routines to paint themselves.  For most window controls, this routine will paint them correctly in design mode and run mode.  Some Windows controls, however, will prove somewhat moody, and might require a little extra tweaking.

   b. RecreateControlWindow.  Again, used for subclassed controls -- will go and recreate the control's hWnd.  This is useful if the control is changing a style bit that simply can't be changed with a SetWindowLong(GWL_STYLE ...) call.

   c. DesignMode.  Returns a BOOL indicating its best guess as to whether the environment is in design mode or not.  If it can't figure it out, it will return FALSE.

   d. GetAmbientProperty.  This routine is used to get an ambient property from the container.  Not all containers will return these (they might not support them), so be careful to check the return code.

   e. GetAmbientFont.  Gets the current ambient font.  Don't forget to release the font once you're done with it.  Again, the container may simply not implement this.  Controls are encouraged to look for the ambient font in the QACONTAINER structure passed in to OnQuickActivate.  Not calling GetAmbientFont saves time.

   f. ModalDialog.  Controls must call this before they show a modal dialog.  This is seen when the control is about to show its About Box dialog.

   g. InvalidateControl.  Much like the InvalidateRect API, but this also operates in design mode.  This will force the control to be repainted if you pass in NULL for the rectangle, or will just invalidate the given area if it's not NULL.

   h. SetControlSize.  Control's who are changing their size out of OnSetExtent should use this routine to set their size.  The control passes in a SIZEL structure in PIXELS, and should expect a call to OnSetExtent.  Be careful of some recursive situations.

   i. PropertyChanged.  Whenever the value of a property changes, this routine should be called to notify a host.  This will cause hosts to update any property browsers (such as those seen in Microsoft Visual Basic 4.0)

   j. RequestPropertyEdit.  Whenever the control wants to change a property that is marked as requestedit in the .ODL file, the control will need to call this first, and check the return code.

   k. GetResourceHandle.  Controls should call this whenever they're loading a resource that could be localized.  This routine will go and get the handle to the appropriate DLL, and deals with satellite DLLs or the lack thereof.  Please see the Localize sample for how this works.

   l. FireEvent.  The control will pass this routine an EVENTINFO structure, and an event as described in the EVENTINFO will be fired.  The control will also pass parameters to this routine, as it is a varargs method.

   m. ControlFromUnknown.  Property page code often finds it useful to get the COleControl * pointer from the IUnknown for a control object.  This routine does just that.

   n. Exception.  Your control, or any automation objects, can use this to send the user an error message.  Please see Section 3.8, "Throwing an Exception," for more information on using this routine.


Windowless that wish to support windowless in-place activation have a couple of routines they must call instead of regular Win32 APIs in order to get the correct functionality.  The following is a list of these and their replacements:

   a. GetFocus --> Writers should call OcxGetFocus().  This will return a boolean indicating whether or not the control has keyboard focus.
   b. SetFocus --> Writers should call OcxSetFocus().  This routine takes a boolean parameter indicating whether or not you want to take the focus or give it up.
   c. GetCapture --> Use OcxGetCaputre() instead.  This indicates whether or not you have the keyboard capture or not.
   d. SetCapture --> By calling OcxSetCapture(), you can ask for or release the keyboard capture.
   e. GetDC/ReleaseDC.  OcxGetDC/OcxReleaseDC now provide this functionality for windowless controls.
   f. InvalidateRect -->  OcxInvalidateRect() will let you force a repaint of your control.

It is worth noting that these routines will all work correctly if your control does, in fact have an HWND.  If your control doesn't plan on working without an HWND, then you may safely use the regular Win32 APIs.



   3.2  Painting a Control

The OnDraw routine is called whenever the control needs to paint.  Sometimes the origin of the call is from IViewObject2::Draw (as in design mode), and other times it comes from being sent a WM_PAINT message (as handled in ControlWindowProc).

Your control is given a DC, a rectangle to describe where to paint, a rectangle for describing a metafile, and an Information Context (IC, passed in as an HDC) that describes the device.  If the device is a metafile, then the control must do a little different work.  However, if the device is a raster display, the control is typically painting to the screen.

Your control must be careful not to make any assumptions about the DC, except that it will be in MM_TEXT mapping mode.  Often, there will be no default pens, brushes, fonts or colors selected into the DC.  Your control will have to do this work itself.  This will typically manifest itself by having your control look slightly different in design and run modes.

If the fOptimize parameter passed in is TRUE, then the control writer may take advantage of the optimizations described in the OC '96 specification for not fully cleaning up the DC on exit.  [Writers are still responsible for deleting GDI objects they create/load, but they may leave things selected in the DC on exit if this parameter is TRUE.  Please see the OC '96 specification for more information.]

Finally, the dvAspect parameter is passed in to the OnDraw routine.  This is used for multi-pass drawing, as described in the OC '96 specification [IViewObjectEx].  Authors not wishing to support multi-pass drawing should just ignore this parameter, as their control defaults to not supporting it.

   3.3  Handling Messages in a Control

Your control has a method called WindowProc, which is called whenever a message is sent to your control.  Your control should respond to messages in the desired fashion here.  If your control has an HWND, try to reduce the amount of work that is done in the WM_CREATE message handler, and see if it can't be put in BeforeCreateWindow().

For certain types of messages, such as keyboard messages for arrow keys, and other special keys, your WindowProc routine will not get called.  Instead, you'll find OnSpecialKey called instead.  The code in OnSpecialKey should look for WM_KEYDOWN/UP, WM_CHAR, and other messages and deal with them as appropriate.

There is a certain class of messages that typically involve notifying a window about happenings that are usually sent to a window's parent.  These include WM_COMMAND, WM_NOTIFY, WM_CTLCOLOR, etc.  These messages will be reflected by the host to your control in the form of OCM_COMMAND, OCM_NOTIFY, OCM_CTLCOLOR, etc.  Your controls should look for these messages instead of WM_COMMAND, etc.  Please see olectl.h for other OCM_ messages that the control might be interested in.  This, of course, does not apply t
o windowless controls.



   3.4  Adding a Property

One of the more important parts of a control is often the set of properties.   To add properties is a relatively straightforward and simple process.

First, you need to modify the primary dispatch interface for your control in the .ODL file.  For example, let's say you have a control called SuperScroll, and you'd like to add a LargeChange method to the control.  You'd add the following to the ISuperScroll interface description in the .ODL:

	[id(DISPID_LARGECHANGE), propget, helpstring("The largechange property")]
		HRESULT LargeChange([out, retval] long *plLargeChange);
	[id(DISPID_LARGECHANGE), propput]
		HRESULT LargeChange([in] long lLargeChange);

DISPID_LARGECHANGE is something that you define in dispids.h.  You then regenerate the type library (.TLB) file by typing:

		nmake

More importantly, this regenerates SuperScrollInterfaces.H.  You can then cut and paste the following two lines from SuperScrollInterfaces.H:


	STDMETHOD(get_LargeChange)(THIS_ long FAR* plLargeChange) PURE;
	STDMETHOD(put_LargeChange)(THIS_ long lLargeChange) PURE;

Take these two lines and add them to your class description for CSuperScroll, and make sure that you remove the PURE declarators at the end; i.e.:

	STDMETHOD(get_LargeChange)(long FAR* plLargeChange);
	STDMETHOD(put_LargeChange)(long lLargeChange);


You can now implement these methods in your control file to implement your property.

Please note that there are a few standard dispids defined for you in olectl.h.  Whenever you want to declare a property, take a look in this header first to see if there is a standard dispid for it.



   3.5  Adding a Method

Adding a method is much like adding a property to your control.  First thing you need to do is define a dispid for the method.  Once you've got that, it's as simple as adding the method to the primary interface for your control.  Lets say we'd like to define a method called MooCow, with three parameters, the last of which is optional.  Here is one such method:

	[id(DISPID_MOOCOW), helpstring("Makes your Cow moo")]
		HRESULT MooCow([in] long lSeconds, [in] boolean fLowPitch,
				[in, optional] VARIANT vPitch);


Again, as with properties, you regenerate the type library, and then paste the declaration into your control header (without the PURE declarator) and implement it.



   3.6  Adding an Event

In many situations, a control will want to fire an event.  For example, when a control gets a WM_?BUTTONDOWN message, it often makes sense to fire a MouseDown event.  This turns out to be easy.

The first thing that has to be done is the event has to be defined in an EVENTINFO structure.  There are many ways to do this, including declaring a new global variable for each event type, or using an array.  We will talk about the latter, since it's a little neater.  Let's say you want to have KeyDown, KeyUp, and KeyPushed events.

Here's how you might declare them:

	typedef enum {
		MyCtlEvent_KeyDown = 0,
		MyCtlEvent_KeyUp = 1,
		MyCtlEvent_KeyPushed = 2
	} MYCTLEVENTS;

	VARTYPE rgI2 [] = { VT_I2 };

	EVENTINFO m_rgMyCtlEvents [] = {
		{ DISPID_KEYDOWN, 1, rgI2 },
		{ DISPID_KEYUP, 1, rgI2 },
		{ DISPID_KEYPUSHED, 1, rgI2 }
	};

The EVENTINFO structure has three members;  the dispid of the event, the count of arguments in the event, and a pointer to an array of VARTYPEs that describe the types of the parameters to the event.  Please note, again, that there are a bunch of dispids defined for you in olectl.h.  Whenever you're adding an event, go check there first to see if there's already a dispid for it.


To fire these events from code, just call the following:

	FireEvent(&(m_rgMyCtlEvents[MyCtlEvent_KeyDown]), sKeyValue);



   3.7  Using Standard ActiveX Types

For many controls, you will find it useful to declare properties of types provided by OLE, such as Font, Picture, and Color.  Many hosts will detect properties of these types and put up convenient browsers for the user to select values for these types.

To declare a property of one of these types, you must first make sure their .ODL includes the following at the top:

	importlib(STDTYPE_TLB);

Then, to declare a property of type Font, Picture, or Color, you would do something similar to the following, depending on the type:

	[id(DISPID_FONT), propget]
		HRESULT Font([out, retval] IFontDisp **ppFont);
	[id(DISPID_FONT), propputref]
		HRESULT Font([in] IFontDisp *pFont);

	[id(DISPID_MOUSEICON), propget]
		HRESULT MouseIcon([out, retval] IPictureDisp **ppMouseIcon);
	[id(DISPID_MOUSEICON), propputref]
		HRESULT MouseIcon([in] IPictureDisp *pMouseIcon);

	[id(DISPID_FORECOLOR), propget]
		HRESULT ForeColor([out, retval] OLE_COLOR *pocForeColor);
	[id(DISPID_FORECOLOR), propput]
		HRESULT ForeColor([in] OLE_COLOR ocForeColor);


For the get_ and put_ methods for these types, the control will get a property as declared above.  For fonts and pictures, the control will probably want to QI these for IFont and IPicture respectively.  See the FontColor control sample for an idea of how this is done.

MSDN contains some very good descriptions of how to use these fonts in your application, but the following is a short run-down.

To use a font object in your control, the control will typically call the get_hFont method, and pass the resulting HFONT to the control's DC.  Pictures will be much the same.

To use a color, the control will want to call OleTranslateColor to convert it to a real COLORREF.  OLE_COLORs are basically COLORREFs with some support for "generic" colors, such as COLOR_WINDOW, COLOR_WINDOWTEXT, etc.  To convert one of these into an OLE_COLOR, just OR (|) them with 0x80000000.  For example, to initialize the control's background to COLOR_WINDOW, set the backcolor property to COLOR_WINDOW | 0x80000000.  Then, to paint the backdrop, just call OleTranslateColor(), and use the resulting colo
rref.

NOTE:  The Framework uses dual/vtable bound automation interfaces, and uses OLE Automation functionality to support IDispatch methods on the automation objects. There is a known problem in OLE automation, which will cause problems (unexpected failures and/or crashes) when using the provided ITypeInfo::Invoke on properties that are declared to be of types that are imported from a type library (i.e., any font, picture, or color property has this problem.)

The way to get around this problem is to override Invoke(), and to look for the dispids of the control's properties that are of this type.  In this case, the control can quickly dispatch the call to the appropriate member function.  The FontColor sample does just this.  See its implementation of IDispatch::Invoke for sample code on how to work around it.  This problem will not exist in a future version of OLE Automation, but until then, the workaround is necessary -- but fortunately not terribly expensive.



   3.8 Throwing an Exception

Every once in a while, during an operation, your control will find itself rather upset with the state of the union, and will wish to communicate this to the user.

The way to do this is via an exception.  In any of the control's OLE Automation methods or property operators, the control can call the Exception method when exiting, and the method will set up all the appropriate information to trigger the error.

For example:

	CMyControl::put_ButtZilla(long lButtZilla)
	{
		if (lButtZilla == 10)
			return Exception(MYCTL_E_IHATETHENUMBER10, IDS_ERR_IHATETHENUMBER10, 0);
		m_lButtZilla = lButtZilla;
		return S_OK;
	}

The arguments to the Exception routine are as the follows:  the first is the scode of the error the control wishes to trigger.  For errors unique to the control, you should define them something like:

	#define MYCTL_E_IHATETHENUMBER10 MAKE_SCODE(SEVERITY_ERROR, FACILITY_CONTROL, 34500)

The second argument is the resource id of the string that the control should display.  The Exception code will correctly get this information from the control's localized satellite DLL.

Finally, the last argument is the helpcontextid that will be passed to the helpfile that is defined in your control's structure.



4.0 Persistence
=-------------=

One of the most important things your control will do is save out and restore its persistent state.  This will typically be done in one of two ways: through PropertyBags for text persistence, and through Streams for binary persistence.  In the latter, performance is absolutely critical to make your control load quickly.

The ActiveX Controls Framework requires that your control implement four member functions to support persistence.  The control is required to implement LoadTextState, LoadBinaryState, SaveTextState, and SaveBinaryState.  If you are positive that the control is never going to be in a host that uses IPersistPropertyBag, then you can ignore the two text interfaces, but this isn't recommended (they prove sufficiently straightforward to use).



   4.1  Text Persistence

Text persistence in the Framework is done via IPersistPropertyBag and IPropertyBag.  All ActiveX controls have an implementation of IPersistProperty Bag, and are given pointers to PropertyBag objects to do their work.

MSDN and Kraig Brockschmidt's book, "Inside OLE 2 (2nd edition)," both have descriptions of the IPropertyBag interface.  Effectively, there are two routines that the programmer will use:  Read and Write.  In both cases, the programmer will pass in a VARIANT.  In the Read case, the property, if it was saved out, will be put in the VARIANT.  If the property couldn't be found (it wasn't ever saved out), then the default value for that property should be used, and an error should probably not be returned.  For the Write, a VARIANT with the data is passed in.  To persist out a collection or an object (such as a Font or Picture object), the programmer can pass in a VT_UNKNOWN object, and the PropertyBag will then QI that object for IPersistPropertyBag or IPersistStream and persist it.  This actually proves effective for persisting collections -- they can just support IPersistPropertyBag.

All of the samples except the Localize and Circle sample have examples of how to persist out properties using PropertyBags.  For controls with many properties, it often makes sense to head to some sort of table-driven persistence to reduce code size and bug potential.



   4.2  Binary Persistence

Binary persistence turns out to be the more critical thing to work on when implementing an ActiveX control.  Control load speed can be severely hampered by a poorly written LoadBinaryState routine, so it's rather critical to spend some time thinking about how to keep this routine fast. The binary persistence code is used by many hosts all the time, and by other hosts when including the control in a generated executable file.

In both routines, the control is handed a pointer to an IStream object.  The key to load speed here is to reduce the number of operations on the stream.  For save, this is slightly less critical.

Typically, a control will want to save out the following information (often in the given order):

   - Some sort of header with a magic number, version, and size information
   - Fixed size state information, such as longs, floats, colors, strings, etc.
   - Variable sized persistent state, such as fonts, pictures, collections, etc.

Most control writers will want to start out the control's binary persistent state with some sort of header structure that includes a "magic" number that the control can check for sanity when the control is loading.  You'll also want to include some sort of version number so that future versions of your control can deal with older versions, and finally, you will often want the control to write out the number of bytes of data that were written.

The samples in the framework that have a binary persistent state use the following structure:

	#define STREAMHDR_MAGIC    0x12345678

	typedef struct {
		DWORD dwMagic;
		DWORD dwVersion;
		DWORD cbSize;
	} STREAMHDR;

The SaveBinaryState routine saves out this information, and the LoadBinaryState routine looks for it.

One way to write out all the fixed-size information (and therefore load it efficiently) is to do it all in one chunk -- if all the control's fixed persistent state is in a structure in the control object, then you can just write out the structure in the persistence code.   The sample controls show the properties grouped together in a m_state structure in the control declaration.

	hr = pStream->Write(&m_state, sizeof(m_state), NULL);

	For loading, it becomes as simple as:

	hr = pStream->Read(&(m_state), sizeof(m_state), NULL);

	Extremely efficient and simple.

For fonts and pictures, it's slightly more complicated.  Effectively, the control has to QI those objects for IPersistStream, and then call the Load or Save routine with the stream that the control has been given.  Typically, this can be done after the control has written out all other information.  The FontColor control sample does just this.

If you follow the above suggestions for persistent state structure, you can typically have the control's load routine look something like as follows:


	IPersistStream *pps;
	STREAMHDR sh;
	HRESULT   hr;

	// first read in the streamhdr, and make sure we like what we're getting

	hr = pStream->Read(&sh, sizeof(sh), NULL);
	RETURN_ON_FAILURE(hr);

	// sanity check

	if (sh.dwMagic != STREAMHDR_MAGIC || sh.cbSize != sizeof(m_state))
		return E_UNEXPECTED;

	// read in the control state information

	hr = pStream->Read(&(m_state), sizeof(m_state), NULL);
	RETURN_ON_FAILURE(hr);


	// now read in the font!

	OleCreateFontIndirect(&_fdDefault, IID_IFont, (void **)&m_pFont);
	RETURN_ON_NULLALLOC(m_pFont);

	// qi it for ipersiststream and load it in.

	hr = m_pFont->QueryInterface(IID_IPersistStream, (void **)&pps);
	RETURN_ON_FAILURE(hr);

	hr = pps->Load(pStream);
	pps->Release();

	return hr;


This proves to be acceptably fast and robust.


5.0 Property Pages
=----------------=

Most ActiveX Controls will find property pages an invaluable addition to their design time functionality.  Fortunately, implementing them proves to be relatively straightforward.  To do so, you merely need to declare an object that inherits from CPropertyPage.



   5.1 Working with a Property Page

The control's property page is declared in the header file using the DEFINE_PROPERTYPAGE macro, which puts it into the g_ObjectInfo table.  The framework supports the creation of the property page object, but you are required to implement the static Create() function.

The property page is created much like a regular windows dialog box would be -- you use your favorite resource editor to create a DIALOG resource, and then just cut and paste it into your control's resource (.RC) file.

The most important method you'll have to implement will be the DialogProc method, which is where all the work will take place.  In addition to the regular windows messages that one would expect in a DialogProc, there are a few additional ones which people working with this framework will expect:

   a. PPM_NEWOBJECTS -- your control has been given some new objects.  The control is expected to go and populate its page's controls with information from this object.  Using the FirstControl() and NextControl() methods from the CPropertyPage class, the control can get the relevant information.

   b. PPM_APPLY -- the control has to apply any changes that have occurred now.  Again, you can use the FirstControl() and NextControl() routines to loop through all the objects for which the property pages were visible and apply the values (note that it's possible for there to be more than one object for which a property page is being displayed).

   c. PPM_EDITPROPERTY -- when the control is sent this message, the control is expected to set the focus to the control instance which represents the property of the given dispid.  You will typically only see this message called if you implement IPerPropertyBrowsing and return a value in MapPropertyToPage.

   d. PPM_FREEOBJECTS -- Various people will find it interesting to stash, in some way, the pointers they receive in PPM_NEWOBJECTS.  This message tells them that it is time to free them, as the objects are no longer valid.  This can be called from the property page's destructor, so people should be careful not to make too many assumptions about the property page.

Please see one of the sample controls for exact details on these messages.



   5.2 Navigating through Associated Objects

Your property pages will operate on one or more controls.  When initializing, you will typically get some values from the first control that you are given.  You can use the FirstControl() method to get the object pointer for this control.  You can then QI it for your primary dispatch interface to get properties to populate the page with.

When told to apply the values (PPM_APPLY), you'll want to apply them to all objects, which means you'll want to loop using FirstControl() and NextControl(), as follows:

	for (pUnk = FirstControl(&dwCookie) ; pUnk; pUnk = NextControl(&dwCookie)) {
		hr = pUnk->QueryInterface(IID_IButton, (void **)&pButton);
		if (FAILED(hr)) continue;

		GetDlgItemText(hwnd, IDC_CAPTION, szTmp, 128);
		bstr = BSTRFROMANSI(szTmp);
		ASSERT(bstr, "Maggots!");
		pButton->put_Caption(bstr);
		SysFreeString(bstr);
		pButton->Release();
	}

Please note that the return values of FirstControl() and NextControl() don't need to be Release()'d.



   5.3 Marking Your Page as Dirty.

It is moderately important to correctly mark the control's property page as dirty at the appropriate times.  This must be done manually.  Typically, the control will do this in response to a windows notification message, such as EN_CHANGE or BN_CLICKED.  When the control wishes to mark its page as dirty, the MakeDirty() routine should be called.  This will cause the Apply button to be enabled, if it was previously disabled, and will tell the host that the state should be saved before destroying the page.

The following code causes the page to mark itself as dirty when the user changes the text in a Text box in the property page:

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
			case IDC_CAPTION:
				if (HIWORD(wParam) == EN_CHANGE)
					MakeDirty();
				break;
		}
		break;



6.0  String Manipulation
=----------------------=

The ActiveX Controls Framework provides a robust system of macros for manipulating strings in pretty much all of the ways that you'll run into while working with an ActiveX control.



   6.1 Types of Strings

Under Win32, there are a few different types of strings, and understanding these tends to be pretty important when working with OLE, since there is great potential for memory leaks and bugs associated with strings.

There are two fundamental types of strings -- multi-byte (which can be ANSI or double byte) and Unicode strings.  Of the former, one almost always works with some sort of char * pointer (LPSTR, LPCSTR).  Of the latter, there are a few types that are commonly used.  Most notably, there are WCHAR * (LPWSTR, LPWCSTR), BSTR, and OLESTR strings.

LPWSTR pointers are just that -- a pointer to a wide string.  An LPOLESTR pointer is much the same, with some additional OLE rules added to it.  An OLESTR is merely a wide string, but when it's an out-parameter to a function, it should be allocated using the host's IMalloc allocator (i.e., CoTaskMemAlloc).

A BSTR is a string with a little more structure (specifically, a length prefix).  To work with BSTRs, you need to use special APIs designed exclusively for them, notably SysAllocString, SysFreeString, and SysStringLen (there are a few others.  See the "OLE Programmers Reference, Volume II," published by Microsoft Press, for more details).

These data types are fully interchangeable as far as compares and copies go, but they are NOT interchangeable as far as allocation and freeing go -- it is not acceptable to call SysFreeString on an OLESTR or LPWSTR string.

Both BSTRs and OLESTRs as in-parameters to functions should not be freed (as per standard OLE COM conventions).  By the same token, BSTRs and OLESTRs as out params should be expected to be freed, and should thus be allocated appropriately.



   6.2 Working with Strings

Now, the problem is that, for the most part, your controls will be working with multi-byte strings, except for where you work with OLE.  Therefore, there will be various scenarios where you'll either be given a wide string, and need the multi-byte version of it, or you'll have a multi-byte string, and need a wide string for it.

To solve these problems, the ActiveX Controls Framework includes the following macros to work with:

	MAKE_WIDEPTR_FROMANSI(newstringname, convertme)
	MAKE_ANSIPTR_FROMWIDE(newstringname, convertme)

	BSTRFROMANSI(ansistr)
	OLESTRFROMANSI(ansistr)
	BSTRFROMRESID(resourceid)
	OLESTRFROMRESID(resourceid)
	COPYOLESTR(copyme)
	COPYBSTR(copyme)

The first two macros will take a string of a given type and a name, and create a variable of the new name (do -not- declare a variable of this name yourself), and then convert the other string into the new variable.  This cannot be used as an rvalue in C/C++ expressions, nor can it be an lvalue (it pretty much needs to sit on a line by itself).

The last set of macros pretty much do all the remaining interesting work.  You can get BSTRs or IMalloc'd OLESTRs from an ANSI string, or copy OLESTRs and BSTRs.  The only additional functions of real interest are those that take a WORD which is a resource id, and loads in a string from your localization DLL (or the main if you don't do satellite localization) and makes either a BSTR or OLESTR out of it.  This proves useful in a few places where you need a localized string.

Remember that while these macros were designed with a certain amount of speed in mind, converting strings is still not a ridiculously cheap operation, and control writers should try to be moderately conservative in string conversions.


7.0  Localization
=---------------=

The ActiveX Controls Framework also has some support for robust localization of your control; most notably, property pages and anything other resources you find interesting to localize.

The scheme is follows:  the resources for the default language (typically English) are in the main in-proc server's resources.  Then, for each additional language supported, there is a satellite DLL which contains the resources for that language.  The name for this satellite DLL is generated by taking the "base" name for the DLL from a string resource in the main in-proc server file, and then adding on a three letter language code as generated by the function GetLocaleInfo with the LCTYPE LOCALE_SABBREVLAN
GNAME.

The code will first look for the specific language that is set up in the global variable g_lcidLocale. This should be initialized by all servers.  For ActiveX controls and property pages, you'll want to get this from the host.  For OLE automation servers, you'll want to set this up some other way, often by just using the system language.  If the satellite DLL for the specific langauge is not found, the framework will look for the primary language ID with SUBLANG_DEFAULT instead.  If that satellite is not f
ound, a handle to the default resources will then be returned.

Please note that fully localized type libraries are not supported, as most hosts will ignore these and just use the default one.


   7.1  Setting up for Localization

If you want to support satellite localization in your in-proc server, then you need to set up a couple of things.

First, you'll need to set the variable g_fSatelliteLocalization in your in-proc server file to TRUE.  This will instruct all further code paths to use satellite localization.

You will also, at some point, need to set up the g_lcidLocale variable, as described above.  For ActiveX controls, you can use GetAmbientProperty() and ask for AMBIENT_DISPID_LOCALE.  For property pages, there is a method on the IPropertyPageSite interface that will let you obtain the locale id, and for automation objects, you will be required to set this up yourself -- typically by obtaining the system locale id.

Whenever you want to load a resource, make sure you use GetResourceHandle() to get the instance handle for the localized resources.  This will generate the appropriate file name for the satellite DLL and attempt to load it.

Please see the Localize sample included with the framework for an example of how this works, along with an actual localized satellite DLL (in the French subdirectory). The satellite DLL should contain all localized resources you're interested in.  



8.0 Miscellaneous
=---------------=


   8.1  Recommended Reading

Inside OLE2, Second Edition, Kraig Brockschmidt, Microsoft Press.  Chapters 3, 13-15 and 24 contain most of the information you'll need to work with an ActiveX control and/or automation server.  The chapters on persistence should also prove useful for those unfamiliar with them.

OLE2 Programmers Reference, Volume I and II, Microsoft Press.

MFC Source Code, Visual C++ 4.x.  When looking for ideas on how to do something, the CDK source code proves to be an invaluable resource.  Use it -- frequently.


   8.2 Host-Specific Notes

Then following are notes, things to consider, or known problems with specific ActiveX Controls hosts:

   8.2.1 Microsoft Access 95

Access 95 -requires- IPerPropertyBrowsing.  If you do not implement this interface, you will not able to put one of your controls on a form.  See MSDN and the MFC source code for documentation on this interface.

In addition, you cannot return error scodes from GetPredefinedStrings. You must return either S_OK or S_FALSE.  Returning an error scode will crash.

MapPropertyToPage should still return PERPROP_E_NOPAGEAVAILABLE if there are no pages available.
