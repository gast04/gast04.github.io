Goals before start: 
* do it on Windows
* make everything in an Emulator
(only Android so far)

# Downloading APK Files

My first Attempt was to setup a KOPLAYER instance and download the APK File
within the instance which seemed handy for me since I could start it using
the same instance.

The problem with this is that some APK files may not be downloadable since
the installation on tablets is not possible, to change the Model of KOPLAYER
in the Software Settings did not change that, unfortunately.
This is due to the restriction of some Applications that they don't run on
tablets or only in certain regions and so on...

To still get the app we can use the Chrome plugin "APK Downloader", here
we just paste in the URL from Google Play and download the APK file.

Now that we have the APK-File we can start analysing it :)


# Analysis without Reversing the APK

Before we get into reversing APK-Files we should make some basic analysis to 
get a better understanding of the app.

Starting with intercepting the network communication and choosing a good proxy.
I am really a fan of the BurpSuite proxy and I see no reason so far to change, but
how to intercept the traffic of an Android application?

Well, that took me a while to figure out a good and reasonable way. One option
are intercepting Apps like "Packet Capture", they make it quite easy to intercept 
traffic you install the App, install their Certificate and they open a local VPN
and will capture all the traffic of an App which you can choose. For a first
simple analysis thats nice, but for replaying messages and editing requests this 
is not enough, and the fact that it is an app and running in the emulator is quite
annoying as well.

So let's setup BurpSuite on the host and a systemwide proxy on the emulator, I 
did not really figure out how and if that works at all...
Koplayer is not designed for that, it's an emulator for speed and gaming not
for analysis and hacking.

But the AVD Manager from Android Studio is made for analysis and hacking, and using
the extended controls from the emulator it's easy to set a proxy server. **Important**
is to specify your **Host IP** and make BurpSuite listen an **all Interfaces** and 
not only localhost, since it's an emulator.
Done, check if the settings are working by clicking apply. To also intercept SSL 
traffic we have to install the PortSwigger Certificat from BurpSuite on the Emulator, 
therefore we just have to download it(accessing http://burp from the host), rename the
file to a \*.cer ending and drag & drop it in the emulator. Under 
`Settings->Security&Location->Advanced->Encryption&Credentials->Install from SD card` 
we can install it.

> It's important to take care which System Target you use for the AVD since a Google Play
> System Image will not allow you to get a root shell using `adb shell`, it has several
> restrictions and does not allow to run the adbd as root. 
> The Google APIs Image does allow you a root shell by the following commands: 
> > adb root
> > adb shell
> The difference between these two images is that the play Image has the Google Play
> Store already installed and you can download Apps, but since we analyse only a single
> specific App we don't need that anyway.

**my experience**

KOPLAYER: 
PRO: fast, easy to use, many root tools already installed
CON: I don't know how to install a proxy

AVD Emulator:
PRO: feature rich
CON: harder to use and to start with it


# Deep look into the APK

In my mind a good point to start is using the MobFS - Framework, it comes 
with lot of Features, and since it provides a docker image, it is super easy
to start.

To do dynamic analysis as well, we have to install it on the Host.
Once the installation and static analysis has finshed (which may take a while)
we can easily inspect the results in the web interface.

Besides providing some really nice insights about the APK it also decompiles
the jar Files, means we don't have to install dex2jar on our own.


# Frida (TODO)


