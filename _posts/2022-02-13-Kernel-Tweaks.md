___
# Intro

Things I often forget, what do I need to make Kernel changes effective. This
is just a small summary of steps for myself, not a full guide.

<br/>

___
# The stuff

Modifications where done on the android kernel (5.4.61-android11), the downloading
and building and running is straight forward. Why I did this is to help me out
with some analysis I am doing on apps, and compiling the kernel and then run
the emulator on an ARM based mac makes things just easier.

So I added a syscall, which gives me more control, I know there are kernel
modules and the kernel has stuff like BPF and tracing but still. I just like
to do my own stuff. So adding a syscall is easy:

Files to modify:
* arch/arm/tools/syscall.tbl 
* (arch/x86/entry/syscalls/syscall_32.tbl) if you want x86 emulator
* kernel/sys.c

If you wanna have your new code in a extra directory, which makes sense than
you need a few more changes. (New directory is called Extension):
* Kconfig (root dir)
* Makefile (root dir)

New Files:
* Extension/ext.c
* include/Extension/ext.h
* Extension/Kconfig
* Extension/Makefile


The changes after all are pretty minor, here is the full git diff (shortened):
```
--- a/Kconfig
+++ b/Kconfig

 source "Documentation/Kconfig"
+
+source "Extension/Kconfig"

--- a/Makefile
+++ b/Makefile
 
-core-y         += kernel/ certs/ mm/ fs/ ipc/ security/ crypto/ block/
+core-y         += kernel/ certs/ mm/ fs/ ipc/ security/ crypto/ block/ Extension/

--- a/arch/x86/entry/syscalls/syscall_32.tbl
+++ b/arch/x86/entry/syscalls/syscall_32.tbl

 435    i386    clone3                  sys_clone3                      __ia32_sys_clone3
+436    i386    csys                    sys_csys                        __ia32_sys_csys

--- a/kernel/sys.c
+++ b/kernel/sys.c

+#include <Extension/code.h>
 
@@ -944,6 +946,19 @@ SYSCALL_DEFINE0(getegid)
 
+SYSCALL_DEFINE1(csys, char*, msg)
+{
+   // syscall implementation goes here
+   return 0;
+}
+

>> cat Extension/Kconfig
config EXTENSION
  bool "My super duper feature"
  default y
	help
    Extension for everything

>> cat Extension/Makefile
obj-y	+= code.o
```

And thats it, the code inside `code.c` and `code.h` is omitted its just some
functions which can be used inside the syscall and some globals to store data
between calls and so on.

Note that adding it to the `core-y` line is needed otherwise it will not be linked
to the core. Also the `y`'s defines core, an `m` for example would stand
for module.

# Emulator

For running it in the emulator I use (but thats just what I use):
```
./emulator -verbose -avd <avd_device> -kernel /pathTo/bzImage -show-kernel
  -no-sim -wipe-data -no-audio -cores 2 -ranchu -no-boot-anim
  -port 4321 -no-window | tee emu_out
```

Dont forget to wipe data before running a new kernel otherwise it will reuse
the snapshot.

