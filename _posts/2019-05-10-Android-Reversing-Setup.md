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

# Analysing APK-Files

In my mind a good point to start is using the MobFS - Framework, it comes 
with lot of Features, and since it provides a docker image, it is super easy
to start.

To do dynamic analysis as well, we have to install it on the Host.
Once the installation and static analysis has finshed (which may take a while)
we can easily inspect the results in the web interface.

Besides providing some really nice insights about the APK it also decompiles
the jar Files, means we don't have to install dex2jar on our own.


