const cm = new CModule(`
void waitOnDbg(void) {
  volatile int loopy = 1;
  while(loopy) {
    volatile int a = 0x1337;
    volatile int b = a + 0x1330;
  };
}
`);

const waitOnDbg = new NativeFunction(cm.waitOnDbg, 'void', []);


Java.perform(function() {
  console.log("\n[*]Setting native hooks:\n");

  const open_libc = Module.findExportByName("libc.so", "open");
  Interceptor.attach(open_libc, {
    onEnter: function(args) {
      
      var arg0 = args[0].readCString();
      console.log("libc: open: " + arg0);

      if (arg0 == "<path>/base.apk") {
        args[0] == "/data/local/tmp/base.apk"
        console.log("--> /data/local/tmp/base.apk");
      }
    }
  });

  const opendir_libc = Module.findExportByName("libc.so", "opendir");
  Interceptor.attach(opendir_libc, {
    onEnter: function(args) {
      console.log("libc: opendir: " + args[0].readCString());
    }
  });

  const openat_libc = Module.findExportByName("libc.so", "openat");
  Interceptor.attach(openat_libc, {
    onEnter: function(args) {
      console.log("libc: openat: " + args[0].readCString());
    }
  });

  const stat_libc = Module.findExportByName("libc.so", "stat");
  Interceptor.attach(stat_libc, {
    onEnter: function(args) {
      console.log("libc: stat: " + args[0].readCString());
    }
  });

  const open_dlopen = Module.findExportByName("libc.so", "dlopen");
  Interceptor.attach(open_dlopen, {
    onEnter: function(args) {
      console.log("libc: dlopen: " + args[0].readCString());
    }
  });

  const open_dlsym = Module.findExportByName("libc.so", "dlsym");
  Interceptor.attach(open_dlsym, {
    onEnter: function(args) {
      console.log("libc: dlsym: " + args[1].readCString());
    }
  });
});

console.log("waitOnDbg, attach debugger to continue\n");
setImmediate(() => {
  waitOnDbg();
});

// frida -U -f <package-id> --no-pause -l script.js
