JP-8000 Software Update Procedure

WARNING: The JP-8000 may not start up if the information is not transmitted properly. 
It is very important not to turn the power off of the JP-8000 during the update 
procedure. This procedure will also erase the data in the User memory. Make sure to
back up important data via sysex before completing the update. The bender and ribbon 
sensor settings will also be lost during this procedure. A procedure for resetting
these settings is included at the end of this document.

The File names included are as follows:

_00001.mid
_00002.mid
_00003.mid
_00004.mid
_00005.mid
_00006.mid
_00007.mid
_00008.mid

Update Procedure:

1)  Connect the sequencer MIDI-OUT to the JP-8000 MIDI-IN.
2)  Turn on the power of the JP-8000 while holding down the [LFO1 WAVEFORM], 
    [OSC1 WAVEFORM], [OSC2 SYNC] and [OSC2 WAVEFORM] buttons. 
3)  Press REC. 
4)  When 'Ready to update' appears on the screen, send the first file (_00001.mid) 
    from your sequencer.
5)  When the file is completed, verify that the checksum is indicated as below. 

Block1 ( chksum = 49F8 )  => filename : '_00001.mid'.
Block2 ( chksum = 12E3 )  => filename : '_00002.mid'.
Block3 ( chksum = A146 )  => filename : '_00003.mid'.
Block4 ( chksum = E6EB )  => filename : '_00004.mid'.
Block5 ( chksum = 5455 )  => filename : '_00005.mid'.
Block6 ( chksum = 672D )  => filename : '_00006.mid'.
Block7 ( chksum = C131 )  => filename : '_00007.mid'.
Block8 ( chksum = F294 )  => filename : '_00008.mid'.

If the value is different, press [EXIT] and repeat steps 4 and 5.

6)  If the value is correct, press the [REC] button.
7)  Load the files one at a time, in sequence _00002.mid to _00008.mid and repeat 
    steps 4-6.
8)  Once all the files have been loaded, 'Completed' will be indicated on the screen.
    Turn off the power of the JP-8000.

Reset Pitch Bender and Ribbon Controller Procedure:

1)  Turn on power while holding [OSC2 SYNC], [-12dB/-24dB] and [FILTER TYPE] to enter Test Mode. Wait until display 
    indicates "[1] MIDI Test."
2)  Press the [3] button to display "[3] Bend Mod."
3)  Push the bender lever left fully and gradually return it to the center. Press [LOWER].
4)  Push the bender lever right fully and gradually return it to the center. Press [UPPER].
5)  Push the bender lever to the MOD position and then gradually return it to the original position. Press [KEY MODE].
6)  When the '*' appears on the display, press [UP].
7)  From the previous display, press [4] to display "[4] Ribbon."
8)  While pressing down on the left edge of the ribbon controller, press [LOWER].
9)  While pressing down on the right edge of the ribbon controller, press [UPPER].
10) While pressing down on the center of the ribbon controller, press [KEY MODE].
11) When the '*' appears on the display, press [UP].
12) Turn off the power of the JP-8000.

Use the following procedure to reset the JP-8000 to factory settings:

1)  Turn on the power of the JP-8000. The unit may display "Memory Damaged."
2)  While holding [SHIFT], press [INIT/UTIL]. 
3)  Press [INIT/UTIL] repeatedly to select "INITIALIZE WRITE." 
4)  Use the [UP / DOWN] buttons to select "FACTORY PRESET," then press [WRITE].

After this procedure has been completed, the new version of software should be ready to go.

