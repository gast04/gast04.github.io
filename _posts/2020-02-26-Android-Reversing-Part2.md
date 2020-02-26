
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

If we start the app now it will wait for an debugger.

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

TODO


<br/>


___
# TODO rest...
