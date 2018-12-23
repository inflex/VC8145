# PREFACE

This software is still in the early beta phase but has been tested and seems to be working fine now, particularly the bside-adm20 GUI version.  

A linux version of this software would be very easy to make, the only change would be the serial port related code (setup and data acquisition).



# BSIDE-ADM20
bside-adm20.exe - GUI windowed application

# Requirements

If you want to build this software on Windows, you'll require MinGW https://sourceforge.net/projects/mingw-w64/

# Setup

1) Build bside-adm20 or bk393 ( Linux mingw64 install required, or mingw64 for Windows)
	 

	make bside-adm20
   
   An example of compiling on Windows using mingw-w64 would be:
   
   mingw32-make -f Makefile.win bside-adm20
   
	
2) Run from the command line

	bside-adm20.exe


# Usage


	 bside-adm20 [-p <comport#>] [-s <serial port config>] [-m] [-fn <fontname>] [-fc <#rrggbb>] [-fw <weight>] [-bc <#rrggbb>] [-wx <width>] [-wy <height>] [-d] [-q]

        -h: This help
        -p <comport>: Set the com port for the meter, eg: -p 2
        -s <[9600|4800|2400|1200]:[7|8][o|e|n][1|2]>, eg: -s 2400:7o1
        -m: show multimeter mode (second line of text)
        -z: Font size (default 72, max 256pt)
        -fn <font name>: Font name (default 'Andale')
        -fc <#rrggbb>: Font colour
        -bc <#rrggbb>: Background colour
        -fw <weight>: Font weight, typically 100-to-900 range
        -wx <width>: Force Window width (normally calculated based on font size)
        -wy <height>: Force Window height
        -d: debug enabled
        -q: quiet output
        -v: show version

        Defaults: -s 2400:7o1 -z 72 -fc #10ff10 -bc #000000 -fw 600 -fn Andale

        example: bside-adm20.exe -z 120 -p 4 -m -fc #ff1010 -bc #000000 -fw 600

