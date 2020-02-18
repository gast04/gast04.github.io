
Design decisions: 
* setup on Windows10
* everything was done via an Emulator<br/>

<br/>

___
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

<br/>

___
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
> > \>\_ adb root<br/>
> > \>\_ adb shell

> The difference between these two images is that the play Image has the Google Play
> Store already installed and you can download Apps, but since we analyse only a single
> specific App we don't need that anyway.

**my experience**

***KOPLAYER:***<br/>
PRO: fast, easy to use, many root tools already installed<br/>
CON: I don't know how to install a proxy<br/>
(koplayer adb is by default listening on port 6555, `adb connect 127.0.0.1:6555`)

***AVD Emulator:***<br/>
PRO: feature rich<br/>
CON: harder to use and to start with it

`Failed to attach: remote_write PTRACE_POKEDATA head failed: 5`   
Even Koplayer is faster, I am going with the AVD Emulator, it just has better
features, and we can disable SELinux at startup to avoid the error message
above, which occurs by Koplayer, when I wanna attach to an running app. We can 
start the emulator using:    
`PathTo/emulator.exe -avd AVDRoot -writable-system -selinux permissive`

<br/>

___
# Deep look into the APK

In my mind a good point to start is using the MobFS - Framework, it comes 
with lot of Features, and since it provides a docker image, it is super easy
to start.

***Note to MobFS:***<br/>
What confused by the first usage was, that downloading the Decompiled Java Source, I got an
empty folder, but actually there was just a hidden `.MobFS` folder in it, which contained 
the sources in a bunch of subfolders.

To do dynamic analysis as well, we have to install it on the Host.
Once the installation and static analysis has finshed (which may take a while)
we can easily inspect the results in the web interface.

Besides providing some really nice insights about the APK it also decompiles
the jar Files, means we don't have to install dex2jar on our own.

To check if the App allows debugging we can use `jdb`(java debugger tool), first we need the
PID of the app we want to debug we can get that by `adb jdwp` or using
`frida-ps -U`. We have to add port forwarding now and can later connect 
to the forwarded port. 

`adb forward tcp:55555 jdwp:<pid>`   
`jdb -connect com.sun.jdi.SocketAttach:hostname=localhost,port=55555`   

If this does not work, debugging is disabled, this can be also checked via
the Manifest file by searching for 'android:debuggable' which should be set
to false for all released applications.

<br/>

___
# Frida

To use Frida it is necessary to install latest python 3.x, if you install the wrong
python version, frida will complain, I had installed python 3.6 instead of the needed
3.7 and that was already not working. But there is a small workaround for this, we have
to patch the necessary python version into frida, since it is hardcoded. Therefore
open the file `_frida.cp36-win_amd64.pyd` and replace "python37.dll" with "python36.dll", 
after this I got no more errors and frida was working fine.

The Error I got was similar to:
> \*\*\*
> Failed to load the Frida native extension: DLL load failed: The specified module could not be found.
> Please ensure that the extension was compiled for Python 3.x.
> \*\*\*

Ressourece: [Frida-Hacking](https://awakened1712.github.io/hacking/hacking-frida/)   

frida-ps -U      show processes   
frida -U \<pid\>   attach to running process   
frida -D \<id\>    execute on specific device

<br/>

___
# Some Commands / Notes

`adb -s 7f1c864e shell`    
`adb -s emulator-5554 push pathto\fridaserver /data`   
`adb pull /data/data/app/file .`

make fridaserver executeable and start
for frida server download [use](https://github.com/frida/frida/releases)
the emulator is x86 so choose this server version as well!

Add "%LOCALAPPDATA%\Android\sdk\platform-tools" to Environment Variables to make it
availbe in cmd, without starting by path (same for emulator, if wanted)

***NOTE:***<br/>
Frida needs time to support new Android Versions, so don't use the always the newest, 
I noted that 10.2019 when I tried with API 29 but it was not support yet, you will get an 
error like: 
```
generic_x86:/data/local/tmp # ./frida-server-android 
  Aborted
```

An instant abort will happen, and if you look into `adb logcat` it will tell you that it
could not find the Android linker, as shown below.

```
xx-xx xx:xx:xx.xxx  8193  8193 F Frida   : Unable to locate the Android linker; please file a bug
```

**Further Usefull Ressources**
[OWASP-Mobile-Security-Guide](https://www.owasp.org/index.php/OWASP_Mobile_Security_Testing_Guide)

<br/>

___
# Patching APK-Files

(This was done on a Ubuntu 18-04)<br/>

Patching APK files is not as straigthforward as patching Linux/Windows binaries. I really like `apktool` 
here to do this for me. We first unpack/unzip the APK File.

`java -jar apktool_2.4.0.jar d App.apk`

After we cann edit the files in unpacked, probably we wanna change some code, so we will edit 
the smali files. We can pack the File again by using.

`java -jar apktool_2.4.0.jar b AppUnpackFolder/ `

The patched APK file can be found in the dist-directory of the unpacked folder. We can try installing 
the APK file now but it will not work...

```
adb install App.apk
  adb: failed to install App.apk: Failure [INSTALL_PARSE_FAILED_NO_CERTIFICATES: 
  Failed to collect certificates from /data/app/vmdl207535598.tmp/base.apk: Attempt to 
  get length of null array]
```

What we missed is that we first need to sign the APK. This can be done creating a Keystore and 
a Key and after we use this to sign the app. I used the following commands for that, maybe there
is a better way, but it did the Job:

```
keytool -genkey -alias mydomain -keyalg RSA -keystore KeyStore.jks -keysize 2048
keytool -certreq -alias mydomain -keystore KeyStore.jks -file mydomain.csr
jarsigner -verbose -keystore KeyStore.jks App.apk mydomain
```

Signed, perfect. We can try installing it again:

___
# Install/Uninstall APK-Files

```
adb install App.apk
  adb: failed to install App.apk: Failure [INSTALL_FAILED_UPDATE_INCOMPATIBLE: 
  Package com.crack.me signatures do not match the previously installed version; 
  ignoring!]
```

Hmm, ok let's delete the last installed Version. Delete it by holding it in the
emulator and dragging it to the bin or by `adb uninstall com.app.me`, here the
package name has to match otherwise it will not find it.

```
adb install App.apk
  adb: failed to install picsart.apk: Failure [INSTALL_FAILED_NO_MATCHING_ABIS: 
  Failed to extract native libraries, res=-113]
```

This error happens when we try to install an APK which uses native libraries
and it does provide the one for my architecture. For example, Android phones use
ARM if I run it on my emulator which needs a Intel x86-64 compiled library it
will not work if it is not provided.


***Tipp:***<br/>
Install the APK-file manually using `adb install app.apk` it gives you better 
error messages, than installing it by drag&drop.

The emulator was started using.
`./emulator -avd Pixel_API_25 -netdelay none -netspeed full -writable-system -selinux permissive`
