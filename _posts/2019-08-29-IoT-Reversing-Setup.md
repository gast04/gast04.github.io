
OS: Ubuntu 18.04<br/>
As training for a Competition I made some Firmware, IoT Setup Preparations

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
> avr-g++ -c -g -Os -std=gnu++11 -fpermissive -fno-exceptions -ffunction-sections -fdata-sections -fno-threadsafe-statics -Wno-error=narrowing -MMD -flto -mmcu=atmega2560 -DF_CPU=16000000L -DARDUINO=10809 -DARDUINO_AVR_MEGA2560 -DARDUINO_ARCH_AVR -I/avr/cores/arduino -I/avr/variants/mega /tmp/arduino_build_478369/sketch/MYPROG.ino.cpp -o /tmp/arduino_build_478369/sketch/MYPROG.ino.cpp.o

Here we can see the compilation and that the output is stored in the tmp directory


Compiling core...
Using precompiled core: 
> /tmp/arduino_cache_682155/core/core_arduino_avr_mega_cpu_atmega2560_f09daf148f324672450511b9824ed27b.a

(I dont know what the core actually does, probably provides libraries)

Linking everything together...
> avr-gcc -Os -g -flto -fuse-linker-plugin -Wl,--gc-sections,--relax -mmcu=atmega2560 -o /tmp/arduino_build_478369/MYPROG.ino.elf /tmp/arduino_build_478369/sketch/MYPROG.ino.cpp.o /arduino_cache_682155/core/core_arduino_avr_mega_cpu_atmega2560_f09daf148f324672450511b9824ed27b.a -L/tmp/arduino_build_478369 -lm

at this step we have our executeable

> avr-objcopy -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 /tmp/arduino_build_478369/MultiSerial.ino.elf /tmp/arduino_build_478369/MYPROG.ino.eep

> avr-objcopy -O ihex -R .eeprom /tmp/arduino_build_478369/MYPROG.ino.elf /tmp/arduino_build_478369/MYPROG.ino.hex

> avr-size -A /tmp/arduino_build_478369/MYPROG.ino.elf


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

> strings BlinkSapmle.ino.stripped.elf<br/>
> GCC: (GNU) 5.4.0<br/>
> ***atmega2560***<br/>
> .shstrtab<br/>
> .data<br/>
> .text<br/>
> .bss<br/>
> .comment<br/>
> .note.gnu.avr.deviceinfo<br/>

By knowing the architecture we can choose the correct IDA-config

Note:<br/>
To use the AVR-Toolchain, we need to export the installtion Path, or we always
call it from the install directory:
> export PATH="$PATH:/PathTo/Arduino/Software/arduino-1.8.9/hardware/tools/avr/bin"  

___
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
endless loop. Looking at the main graph in r2 we get something like

``` 
[0x470]
(fcn) main
            f t 
            │ │           0x470: start of Setup 
   ╭────────╯ ╰╮
   │           │
  __x560__[ob] │
   v           │
   │           │
   ╰──────╮    │
          │ ╭──╯
          │ │
       __x586__[oc]
          v
          ╰─╮
            │
   ╭──────╮ │
   │      │ │
   │   __x58a__[of]        0x58A: start of MainLoop
   │      │ 
   ╰──────╯
```

Even if the binary is ***stripped*** the main function is easy to find, first we take look at the interrupt vector table which is located at the very beginning of the binary, the first entry in the table is the reset which resets the execution, and restarts the main function, so it points to to an interrupt handler which will call the main function.


Further Analysis of the main function:<br>
If it is not stripped it's easy (if you can read avr assembly), functions are named:

* sym.digitalWrite.constprop
* sym.delay.constprop
* ...

We still have the source code, so that makes it anyway easy, what is ***sym.digitalWrite.constprop*** doing, in a stripped binary we don't have names, (I don't know why it is called constprop, as well,
I assume because it only takes one parameter which says HIGH or LOW but not the Port, so probably it is
called constprop because it's always using the same Port and Bit, ***TODO:*** verify)


It is interesting what the compiler is doing here, adding more pins to the simple program changes the definition of 
the ***sym.digitalWrite.constprop***, now it takes two parameters instead of one, it takes the pin and the value (HIGH/LOW) and it doesn't have the suffix ***constprop*** anymore. 

```
0x000005a8  61e0  ldi r22, 0x01				; value
0x000005aa  8de0  ldi r24, 0x0d				; pin
0x000005ac  62de  rcall sym.digitalWrite
```

Taking a look into the ***digitalWrite*** funtion we can analyse further and see that the pin is getting hardcoded into
the digitalWrite function if we only use one, that's in the beginning of the function.


```
mov     r19, r24        ; r19 = value
ldi     r30, 0xD5		; r30 = hardcoded pin 

vs.

mov     r19, r22        ; r19 = value
ldi     r30, r24		; r30 = pin 
```

The rest of the function, does nothing special, saving SREG, disable Interrupts writing clearing or setting the pin,
restoring SREG and return. We can also look up the source in ***wiring_digital.c***.

#### what if we only have the .hex file?

The .hex file is the acutal code, before it gets loaded on the microcontroller, one line looks like following, if we open the original .elf file in radare and compare it we can see a pattern.


```
:1000000007C1000033C1000031C100002FC1000052

can be split into

:10 0000 00 07C1000033C1000031C100002FC10000 52
 |    |   |               |                   |
Idk   |  Idk          Code bytes          Checksum
   Offset
```

With this knowledge we can write a python script and convert the .hex file back into a binary

```python
import binascii

hexfile = open("code.hex")
hexcontent = hexfile.readlines()

binaryfile = open("generated.elf", "wb")

for line in hexcont:
    binaryfile.write(binascii.unhexlify(line[9:-3]))

binaryfile.close()
```

We can analyse the generated binary using r2. We notice some difference because all the elf-header is missing and so on, 
for example the Linux `file` command only detects it as data and r2 doesn't know where to load it, but analysing it using `aaa` detects the entry and which is calling the main function, so same procedure as before.

Analysing AVR-code can be hard, it's a different ISA and if you are not familar with it, it can get hard and will require lot instruciton googling...

___
## Dynamic Analysis


[simavr](https://github.com/buserror/simavr) is the simulator of choice, it works out of the box and is easy to use, since the ATmega2560 and ATmega328P are preconfigured.

>simavr -m atmega2560 program.elf

If we want to debug the program we can append `-g` to wait for a gdb connection.

>simavr -m atmega2560 -g program.elf

Here we have to use `avr-gdb` from the avr-toolchain, or, and thats my favourite, we use r2 to debug the program :)

> r2 -a avr -d gdb://127.0.0.1:1234 

We just have to specify the architecture and we are good to go, the next awesome thing it can do is, it can simulate
a .hex file, which is a really cool feature.

>  simavr -m atmega2560 -f 16MHz -g program.hex

Depending on controller and it's frequency we can again simply debug it with r2, if we really wanna use `avr-gdb` 
we would have to use the following gdb commands.

> avr-gdb   
> (gdb) target remote:1234
 
 
Running the Code is nice, but like that we cannot give input, unless we modify registers by our own, the simavr 
repository contains also a functional emulator, in the examples folder we find a simduino folder which is an 
arduino emulator, be careful it ***emulates the ATmega328p*** and ***not ATmega2560***, I didn't check how much work it
is to change this to another architecture I just recompiled my program for the ATmega328p.

If you run an ATmega2560 code using the emulator, it will SEGFAULT.

Ok, so now it's getting a bit hacky, I am not sure if r2 is really showing the memory and so on correct, I did not find
a way to look at SRAM values, which is quite annoying. The debugging works fine and using `picocom` works also fine
to give SerialInput. 

To make comparision I tried the same using gdb, to use a python plugin for the `avr-gdb` we first have to compile gdb 
to support avr and python. 

* Download from  [https://ftp.gnu.org/gnu/gdb/](https://ftp.gnu.org/gnu/gdb/)
* ./configure --with-python --target=avr
* make && make install

I had to install `texinfo` as well to make it work.

I really cannot say that this is nice to use, the `gef` and `peda` do not support AVR, the gdbinit file which 
made it finally working was:
[https://github.com/cyrus-and/gdb-dashboard/blob/master/.gdbinit](https://github.com/cyrus-and/gdb-dashboard/blob/master/.gdbinit)

Using this I was able to step through the binary with gdb setting breakpoints was still a challenge because I 
always got a leading 0x80.... which is not the correct address... I could solve this by using `b *($pc+0xOffset)` instead of `b *<addr>`.

GDB-Commands, I always forget...

| Command | Action |
| ------------|:-------------|
| ni | step over |
| si | step into |
| x/b \<addr> | show 1 bytes of addr |
| x/h \<addr> | show 2 bytes of addr |
| x/6i $pc-3  | show 6 dissasembled instructions |
| i r         | info registers |
| bp *\<addr> | create breakpoint at addr |
| i b         | list breakpoints |
| d \<num>    | delete breakpoint num |


<br>

Performing now debugging, is still not a easy task, even we have now all the tools needed and a working
debugging setup, if we for exmaple don't know the instruction which is reading the input from the UART, or
if we don't know that we have to search for `ld r24, x` we will not succeed, and we will not be able to name functions and break down the program into smaller parts. Therfore now some AVR basics:


We can find the Pin/Port and so on numbers in ***iomxx0_1.h*** (TODO: find a better listing), during debugging our
assembly has no labels, so we don't know if it is UBRRnH or RXENn or something else... 

Knowing this, and the rest will be hopefully reading simple code :)




## IDA
todo

