___
# Intro

Have you ever tried reverse engineering the native part of an android
application and noticed that the toolset is just horrible? for example:

* long loading times
* debugger crashes
* unreliable results

Working now one year as mobile tester, I wanna share my thought of what
I think is the most convienent way of debugging native android code.
(maybe IOS comes in the next blog)

The thing is, just dont debug, seriously avoid when possible!

<br/>

___
# Debugging with IDA

No doubt, IDA is a great tool, however once you feed it a binary/library which
is over ~100Mb in size everything will simply take ages. The initial analysis
starting a debugger and debugging itself...

The initial analysis can be speed up by a factor of 10x by simply closing all
windows which IDA opens per default. This is cause for example the function
window is sorted, if IDA detects a new function and inserts it the whole list
gets sorted again. If the binary/library contains now 100k functions (which is
easily possible analyzing an Unreal game) this sorting may take a while...

Furthermore attaching to an android application IDA always fetches all libraries
from the device/emulator, this can take up to 40 seconds. This is highly
impractical if you have to restart the debugging session several times, which
also can happen easily, cause IDA is not rock solid code and might encounter
crashes during remote debugging.

### Commands for connecting IDA

```bash
# to assure you catch everything, start app in debug mode
adb shell am start -D -n <package-id>/<package-id>.<activity>
# this will pause the application from the java side so no native code
# is executed yet (needs debuggable flag set to true)

# push ida debugserver (depend on architecture) to phone/emulator
# execute and forward port
adb push android_{server, server64, x64_server, x86_server} /data/local/tmp/
adb shell chmod +x /data/local/tmp/android_{...}
adb shell /data/local/tmp/android_{...} &
adb forward tcp:23948 tcp:23946

# now inside IDA we can connect to port 23948
# Select correct debugger
#   Remote Linux for x86 emulator
#   Remote ARM Linux/Android debugger for android device

# A really nice feature in IDA is the Suspend on library load/unload option
# available at Debugger -> Debugger options...
# as the names says it breaks on library load/unload

# now setting all the breakpoints and hitting continue in IDA, all whats
# left todo is to continue the JavaVM

# forward jdwp port and contine execution
adb forward tcp:33333 jdwp:<target-pid>
jdb -connect com.sun.jdi.SocketAttach:hostname=localhost,port=33333
```

All these steps are already pain, and attaching IDA and continuing the
JavaVM has to be done everytime on an app restart...

<br/>

___
# What are the options?

Instead of IDA we could use radare2, I use it a lot at work but to be honest
only for native linux stuff. From the android perspective it doesnt have the
problem of the slow startup times as it doesnt fetch all libraries. I havent
really used it a lot as I simply encoutered to many issues, and wrong register
values was the worst of it.
In my mind its not fair to judge an open source tool so I will leave it like
that, I could make a PR but yea...

The other option is the gdb shipped with the android ndk's and these work just
like magic, simply because you can easily configure them.

<br/>

___
# Using GDB

The whole gdb interface might be overwhelming at the beginning but it simply
works and thats the key point here and the only thing which is interesting.

A really nice feature is that we can specify the `solib-search-path`, this
sets where gdb is looking for libraries, we can simply point it to the
decompiled apk's lib folder and only those libs will be loaded. This gives
instant startup time.

Further more we can specify the `sysroot` directory, this sets where android
system libraries are loaded from, if we dont wanna debug android libraries
we can simply set it to an empty directory of our host and no libraries and
symbols will be loaded, this again speeds up the whole process.

However we want the `linker` library to be loaded with symbols cause it gives
us the possibility to set a breakpoint on the symbol `__dl_notify_gdb_of_load`.
This is hit whenever a library is loaded. So the same as the IDA feature.

To achive a linker load with symbols the directory where `sysroot` points to
needs to have the same tree structure as on the device/emulator:
```
android_libs
└── system
    └── bin
        └── linker
```
The `sysroot` points to `android_libs` in my case.

Having the symbols of the `linker` loaded we can set a breakpoint on
`__dl_notify_gdb_of_load` and get notified whenever a library is loaded,
and set our breakpoints their.

Always executing this is quite some work especially we still need to tell
gdb that we want to remote debug and so on. So to avoid this we can simply
write all commands in the `.gdbinit` file, this will be executed at gdb
startup.

```GDB
# .gdbinit

# depends if emulator or device
#source /home/niku/git-repos/peda/peda.py
source /home/niku/git-repos/peda-arm/peda-arm.py

# the page continue reading feature, bad for python scripting,
# prevents fetching all output of command
set pagination off

set auto-solib-add on
set solib-search-path /home/niku/Project/RevTarget/decapk/lib/x86
set sysroot /home/niku/Project/RevTarget/android_libs/

target remote localhost:12345

# load symbols of linker
sharedlibrary linker

# break on library load
break* __dl_notify_gdb_of_load
  commands

  # execute python script on breakpoint hit
  source /home/niku/Project/RevTarget/bp_handle.py

  continue
end

# might not be wanted
continue
```

Executing a python script whenever the breakpoint triggers gives even more
flexibility in handling it because of the gdb python interface. A simple
script which only prints the name of the loaded function can look like this:

```Python
import gdb

# get inferior
inf = gdb.inferiors()[0]

# get name of newly loaded library
mem_loc = gdb.parse_and_eval("*(*((int*)($esp+4))+4)")
mem = inf.read_memory(mem_loc, 256)

# returns type buffer on python2, and memoryview on python3 
# yea thats improvable...
lib_name = ""
for i, b in enumerate(mem):
  if b == '\x00':
    lib_name = mem[:i]
    break

# get load address
mem_loc = gdb.parse_and_eval("*((int*)($esp+4))")
mem = inf.read_memory(mem_loc, 4)
load_addr = ord(mem[0]) | ord(mem[1])<<8 | ord(mem[2])<<16 | ord(mem[3])<<24

print("{} : {}".format(hex(load_addr), lib_name))
```

The argument to the function is the `link_map` struct the implementation can
be found here (linker_gdb_support.cpp)[https://android.googlesource.com/platform/bionic/+/master/linker/linker_gdb_support.cpp#76].
As we break on function entry we have to parse the argument based on the esp
register. The `link_map` struct is defined as:

```
struct link_map {
  void *l_addr;
  char *l_name;
  void *l_ld;
  struct link_map *l_next;
  struct link_map *l_prev;
}
```
Which is a double linked list of all loaded libraries, we also could review
all loaded libraries including their base address, this gives quite some
options.


The script above gives an output like (a bit cleaned):
```
gdb-peda$ c
Continuing.
0xc5bcd000 : /apex/com.android.art/lib/libopenjdkjvmti.so

Thread 8 "ADB-JDWP Connec" hit Breakpoint 1, 0xf490b080 in __dl_notify_gdb_of_load ()
0xc5b63000 : /apex/com.android.art/lib/libart-dexlayout.so

Thread 8 "ADB-JDWP Connec" hit Breakpoint 1, 0xf490b080 in __dl_notify_gdb_of_load ()
0xc5acd000 : /apex/com.android.art/lib/libjdwp.so

Thread 8 "ADB-JDWP Connec" hit Breakpoint 1, 0xf490b080 in __dl_notify_gdb_of_load ()
0xc5aa4000 : /apex/com.android.art/lib/libnpt.so

Thread 8 "ADB-JDWP Connec" hit Breakpoint 1, 0xf490b080 in __dl_notify_gdb_of_load ()
0xc58a9000 : /apex/com.android.art/lib/libdt_fd_forward.so

Thread 1 "package-id" hit Breakpoint 1, 0xf490b080 in __dl_notify_gdb_of_load ()
0xc4e0e000 : /data/app/~~ZYp7VFbicCbnp0yd43UBvg==/packade.id-A4sqyuy_v2PHNUAH_Cp1mA==/oat/x86/base.odex

Thread 20 "RenderThread" hit Breakpoint 1, 0xf490b080 in __dl_notify_gdb_of_load ()
0xc4ad1000 : /vendor/lib/egl/libEGL_emulation.so

Thread 20 "RenderThread" hit Breakpoint 1, 0xf490b080 in __dl_notify_gdb_of_load ()
0xc4a82000 : /vendor/lib/egl/libGLESv1_CM_emulation.so

Thread 20 "RenderThread" hit Breakpoint 1, 0xf490b080 in __dl_notify_gdb_of_load ()
0xc4a52000 : /vendor/lib/egl/libGLESv2_emulation.so

Thread 1 "package-id" hit Breakpoint 1, 0xf490b080 in __dl_notify_gdb_of_load ()
0xc4a26000 : /data/app/~~ZYp7VFbicCbnp0yd43UBvg==/packade.id-A4sqyuy_v2PHNUAH_Cp1mA==/lib/x86/libnative-lib.so
```

Using this basic setup it provides very flexibile opportunities for scripting
and automated debugging using the gdb python interface [1].

For example detecting a library load, setting automatic breakpoints, dumping
memory, continue single stepping in gdb, and much more...
And the best things of all, its stable! I never had any crashes with it and
everything happens basically instant without noticeable delays. Only the
jdb connection commands are annoying but these can be easily scripted ;)


<br/>

___
# Summary

My suggestions is still, to simply dont debug. Before I debug an application
I try all other options, especially frida, android profiler, frida stalker
whatever I find.

But if there is no other option and if the application is small, I use IDA,
but at a certain library size I switch to GDB and only use IDA for the static
part.

Guess the only way to make it even more easier is to modify the android
operating system in providing more log information. (I am working on that
but this will take while)

___
# Ressources

* [1] https://sourceware.org/gdb/onlinedocs/gdb/Python.html#Python


# zu guter letzt ;)
If you have any question or like this kind of content dont hesitate to
reach out to me at niku.systems at gmail.com. Also let me know if you are
interested in my tools at `http://niku.systems/#tools`(still in slow develop)
or wanna hire me for a project.
