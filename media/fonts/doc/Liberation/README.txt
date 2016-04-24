   1. What's this?
  =================

  The Liberation Fonts is font collection which aims to provide document 
  layout compatibility as usage of Times New Roman, Arial, Courier New.


   2. Requirements
  =================

  * fontforge is installed.
    (http://fontforge.sourceforge.net)


   3. Install
  ============

  3.1 Decompress tarball

    You can extract the files by following command:

      $ tar zxvf liberation-fonts-[VERSION].tar.gz

  3.2 Build from the source

    Change into directory liberation-fonts-[VERSION]/ and build from sources by 
    following commands:

      $ cd liberation-fonts-[VERSION]
      $ make

    The built font files will be available in 'build' directory.

  3.3 Install to system

    For Fedora, you could manually install the fonts by copying the TTFs to 
    ~/.fonts for user wide usage, or to /usr/share/fonts/truetype/liberation 
    for system-wide availability. Then, run "fc-cache" to let that cached.

    For other distributions, please check out corresponding documentation.


   4. Usage
  ==========

  Simply select preferred liberation font in applications and start using.


   5. License
  ============

  This Font Software is licensed under the SIL Open Font License,
  Version 1.1.

  Please read file "LICENSE" for details.


   6. For Maintainers
  ====================

  Before packaging a new release based on a new source tarball, you have to
  update the version suffix in the Makefile:

    VER = [VERSION]

  Make sure that the defined version corresponds to the font software metadata
  which you can check with ftinfo/otfinfo or fontforge itself. It is highly 
  recommended that file 'ChangeLog' is updated to reflect changes.

  Create a tarball with the following command:

    $ make dist

  The new versioned tarball will be available in the dist/ folder as
  'liberation-fonts-[NEW_VERSION].tar.gz'.

  7. Credits
 ============

  Please read file "AUTHORS" for list of contributors.
