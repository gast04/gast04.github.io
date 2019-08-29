
OS: Ubuntu 18.04

As training for a Competition I made some Firmware, IoT Setup Preparations

<br/>
# Arduino (ATmega328P, ATmega2560)

Starting with reversing programs for Arduino Boards of course, it's probably the
simplest and lot of sources are available in the wild.

I Downloaded the latest software Package from their homepage and installed the 
toolchain, works out of the box.

Requirements I needed for Installation:
  * freeglut3-dev

In order to know where the compiled sketches are getting stored we have to 
enable the verbose mode in the Arduino IDE.
  > File->Preferences->Show verbose output

If we compile now a simple sketch we see where the .eep, .hex, .elf files are
getting stored, even more nice is we see which commands are used

Since it is using the binaries from the install directory we have all the 
(maybe long) paths in the output but we can trim that.

-> I will skip the verbose output here and only point to the interesting parts,
   it is also very nice that we the avr-gcc commands and so on, like that we
   don't have to google for the correct commandline arguments and so on



Compiling sketch...
avr-g++ -c -g -Os -std=gnu++11 -fpermissive -fno-exceptions -ffunction-sections -fdata-sections -fno-threadsafe-statics -Wno-error=narrowing -MMD -flto -mmcu=atmega2560 -DF_CPU=16000000L -DARDUINO=10809 -DARDUINO_AVR_MEGA2560 -DARDUINO_ARCH_AVR -I/avr/cores/arduino -I/avr/variants/mega /tmp/arduino_build_478369/sketch/MYPROG.ino.cpp -o /tmp/arduino_build_478369/sketch/MYPROG.ino.cpp.o

Here we can see the compilation and that the output is stored in the tmp directory


Compiling core...
Using precompiled core: /tmp/arduino_cache_682155/core/core_arduino_avr_mega_cpu_atmega2560_f09daf148f324672450511b9824ed27b.a
(I dont know what the core actually does, probably provides libraries)

Linking everything together...
avr-gcc -Os -g -flto -fuse-linker-plugin -Wl,--gc-sections,--relax -mmcu=atmega2560 -o /tmp/arduino_build_478369/MYPROG.ino.elf /tmp/arduino_build_478369/sketch/MYPROG.ino.cpp.o /arduino_cache_682155/core/core_arduino_avr_mega_cpu_atmega2560_f09daf148f324672450511b9824ed27b.a -L/tmp/arduino_build_478369 -lm

at this step we have our executeable

avr-objcopy -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 /tmp/arduino_build_478369/MultiSerial.ino.elf /tmp/arduino_build_478369/MYPROG.ino.eep

avr-objcopy -O ihex -R .eeprom /tmp/arduino_build_478369/MYPROG.ino.elf /tmp/arduino_build_478369/MYPROG.ino.hex

avr-size -A /tmp/arduino_build_478369/MYPROG.ino.elf


Interesting is, that after we have the .elf file we dont stop, we generate a 
.hex file, and if we hit the upload button instead of the compile button, we
get one more command which is uploading the .hex file to the arduino and 
not the .elf file.

> avrdude -C/avrdude.conf -v -patmega2560 -cwiring -P/dev/ttyACM0 -b115200 -D -Uflash:w:/tmp/arduino_build_478369/MYPROG.ino.hex:i


The First Question of course, what if we get either the .hex file or the .elf 
file, can we analyse it? 


# Analysis

The .elf file we can simply throw into IDA or radare2 and can analyse it, we 
have to know which kind of avr it is, to properly map the Memory sections, but
that should be quite simple.

Binary for the Analysis:

> file BlinkSample.ino.elf
> BlinkSample.ino.elf: ELF 32-bit LSB executable, Atmel AVR 8-bit, version 1 (SYSV), statically linked, with debug_info, not stripped

How do we know which kind of Atmel it is?

looking at the strings in the binary is working on non-stripped and on stripped
binaries as well.

> strings BlinkSapmle.ino.elf
> GCC: (GNU) 5.4.0
> ***atmega2560***
> .shstrtab
> .data
> .text
> .bss
> .comment
> .note.gnu.avr.deviceinfo

By knowing the architecture we can choose the correct IDA-Config

## using radare2 

Loading it without config into r2 already leads to nice and easy to understand
results:

* entry0 found
* main function
* library functions 

We look into the main, we know that using an embedded board we always have a 
main loop which never exits and a setup function which is called once, this
is how the main function is structured, first there is code which is only 
called once and after we have a loop which end with and *rjmp* building our
endless loop.

                                      
            f t                                                                          
            │ │                                                                          
            │ ╰╮                                                                         
   ╭────────╯  │                                                                         
   │           │                                                                         
  __x560__[ob] │                                                                         
   v           │                                                                         
   │           │                                                                         
   ╰──────╮    │                                                                         
          │ ╭──╯                                                                         
          │ │                                                                            
       __x586__[oc]                                                                      
        v                                                                                
        │                                                                                
        ╰─╮                                                                              
          │                                                                              
          │                                                                              
          │                                                                              
          │                                                                              
   ╭────────╮                                                                            
   │      │ │ ╭──╮                                                                       
   │      │ │ │  │                                                                       
   │   __x58a__[of]                                                                      
   │        f t  │                                                                       
   │        │ │  │                                                                       
   │        │ ╰──╯                                                                       
   │    ╭───╯                                                                            
   │   __x59a__[oh]                                                                      
   │    v                                                                                
   │    │                                                                                
   ╰────╯                                                                                
                                                                                         
                                                                                         
              
