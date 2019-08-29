
Since I started cracking some Games, I moved lot of things from linux to
Windows and I also installed Windows 10 now on my Notebook, to be not 
dependend anymore on my Desktop PC.   

On Linux I had my standard tools on analysing binaries and so on, using strace
radare2, the IDA Linux remote debugger, and so on...    

As soon as I started my first Game I noticed that I will need new tools, I 
decided to start to seriously use x64dbg now, since I met the creator ;)
and some other tools.

Cracking Games is not really the same as analysing binaries from CTFs, CTF 
binaries are usually quite small and only consist of one executable. Looking 
in a Games install directory, there are several executeables, so where to start?    

Tooling, is everything, know your tools, what can your tools do for you? and
after you cracked a game, what can you do for your tool?   



<br/>
# Challenges you Face during Game Cracking

Games are/contain:
* packed
* anti-debug code
* CD-protection
* anti-tamper protection

Well unpacking can be easy some times, finding and patching anti debug code
can be already quite a challenge, especially when you move from linux.

TIB
TEB
PEB
ProcessHeap
NTGlobalFlags
FindWindowA
TLS-Callback

<br/>
# Packer

<br/>
# Anti-Debug

<br/>
# Tracer


