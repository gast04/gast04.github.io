
___
# Intro

In my [last notes](https://gast04.github.io/2019/05/10/Android-Reversing-Setup.html) about Android 
I wrote some basic setups, since I am doing some
BugBounties now I have a few more notes to take to avoid looking them up on the
internet again.

As for all Security related tasks it takes time to build a good toolset or build
own tools and the same goes for android app testing. This decision is left to the tester.

<br/>

___
# Make an App debugable

We can check looking into the `AndroidManifest.xml` file it the app is debugable.
If it is, we find the tag `android:debuggable="true"` in the `application` part.
If this is not found we can simply add it. After we pack it again with apktool 
sign it and install it on the device. We can verify that it worked using drozer.

```
dz> run app.package.attacksurface com.example.app
Attack Surface:
  1 activities exported
  0 broadcast receivers exported
  0 content providers exported
  0 services exported
    is debuggable
```

To debug the app in an emulator now we have to go into `Settings->System->Developer Options` and 
select a debug app, and we also have to set the `Wait for debugger` flag. The path depends on the 
Android version but it's always called Developer Options.

If we start the app now it will wait for a debugger.

<br/>

___
# Drozer

I didn't played around a lot with it so far, as there are many tools available but 
[drozer](https://github.com/FSecureLABS/drozer) was mentioned in some blogs I was reading so I 
gave it a try. To install it we have to install the 
[drozer-APK](https://github.com/mwrlabs/drozer/releases/download/2.3.4/drozer-agent-2.3.4.apk) on
the emulator/device. On our host we install drozer by `pip install drozer`. After we can connect from 
the host system to the agent by `drozer console connect`. There are many modules which can be used 
the most interesting I found so far is the `attacksurface`. This returns a nice list of exposed attack vectors.

<br/>

___
# Connect Debugger to waiting App

Here I did not really find a method which satisfied my needs completely, I tried `jdb`
and other stuff but everything seemed unecessary complex... My final approach is now to 
use [smalidea](https://github.com/JesusFreke/smalidea), this seems the best and easiest 
way to me. Simply install the plugin and set breakpoints directly in the smali code after
loaded via `Profile and Debug APK`. Make sure the Apk is debugable, just hit the 
`Attach Debugger to Android Process`-Button and have fun.

<br/>

___
# Debug using gdbserver

Debugging native applications can be done by using gdbserver on the android phone.

```
gdbserver tcp:<localport> --attach <pid>
```

Before we can connect to it from our host system we have to forward the localport from 
android to a remote(device) port.

```
adb forward tcp:<remoteport> tcp:<localport>
```

Now we have everything setup to connect `gdb` or `radare2`. 

```
r2 -d gdb://127.0.0.1:<remoteport>

gdb -q
# target remote localhost:<remoteport>
```

Debugging using radare did not work as expected, so I am mainly using gdb.
`show shared` to get the entry address of the custom library. I had an IDA instance
open of the same library to calculate the offsets, maybe there is a better way
but it works good enough for me. The pointer

```
...
0xe3de4440  0xe3deb994  Yes (*)     target:/vendor/lib/egl/libGLESv1_CM_emulation.so
0xe3da6dd0  0xe3db16f4  Yes (*)     target:/vendor/lib/egl/libGLESv2_emulation.so
0xe3d4d640  0xe3d66825  Yes (*)     target:/system/lib/libcompiler_rt.so
0xe3d0c850  0xe3d0d059  Yes (*)     target:/system/lib/libwebviewchromium_loader.so
0xd6ad3000  0xd6ad3000  Yes (*)     target:/data/app/com.example.nativedebugging-4Fbxl5HAxcExc7XuzHmEIw==/oat/x86/base.odex
0xd698ec40  0xd69ae1b0  Yes (*)     target:/data/app/com.example.nativedebugging-4Fbxl5HAxcExc7XuzHmEIw==/lib/x86/libnative-lib.so
0xd62cb9f0  0xd62cff45  Yes (*)     target:/vendor/lib/hw/gralloc.ranchu.so
...
```

To set library hooks important for us is the libnative-lib.so, this is the library used by the app, we 
can see this because of the path, the other shared libraries are from the system. The start address
is from the entry function. To calculate an offset to a Java exportet function we would need 
` loaded_addr - entry_offset + function_offset`. We can set a breakpoint on all exported function and 
after single step through it using gdb.

<br/>

___
# Notes

For creating native applications we have to add the NDK path to the `local.properties` build file from
Android Studio. `ndk.dir=/PathToNDK/ndk/21.0.6113669`. 
Without, it will not build and show an error that it could not find the NDK and please install it even
it is already installed using the SDK Manager.
