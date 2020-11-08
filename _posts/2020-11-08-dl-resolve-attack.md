
___
# Intro

During the last CTF I played I was working on a really nice challenge, which 
I think is worth a writeup. The challenge was called crySYS and all the materials
can be find on my [github](https://github.com/gast04/CTF-Writeups/tree/master/crySYS).

The Challenge itself contained a simple bufferoverflow and the goal was to get
a shell on the server. Even the source code was given: 

```C
#include <stdio.h>
#include <unistd.h>

//gcc -o challenge -no-pie -fno-stack-protector challenges.c
//LD_PRELOAD=./libc-2.27.so ./ld-2.27.so ./challenge

int not_vulnerable() {
  char buf[80];
  return read(0, buf, 0x1000);
}

int main() {
  not_vulnerable();
  return 0;
}
```

Furthermore, we see the compile command and we know there is no stack canary
which means the binary is exploitable.

The Rest of this blog I will describe my two attempts of how I solved it.
<br/>

___
# Background Information

You might think, why we just do `return2libc`? and I thought that in the first
moment as well but for a `return2libc` attack you always need a leak, like in
this example the resolved address of `read`, then calculate the base address
of the libc and then calculate the address of `system` or `exevp` or wathever.

For a `return2libc` attack we would need an address leak, but in this special 
case there is no function resolved which can leak anything. Only `read` is 
resolved. We could of course setup a write system call using a `ROPchain` but
this binary is so small that there are simply not enough gadgets.

So the only attack I could come up, was `return2dlresolve`. There are already
some good resources available on line on that attack:

* https://gist.github.com/ricardo2197/8c7f6f5b8950ed6771c1cd3a116f7e62
* https://www.rootnetsec.com/ropemporium-ret2csu/
* https://1ce0ear.github.io/2017/10/20/return-to-dl/
* https://ddaa.tw/hitcon_pwn_200_blinkroot.html
<br/>

___
# Tooling

For the exploit I use `python` with `pwntools`. I have a patched version
of `pwntools` which spawn `radare2` instead of `gdb` by calling 
`gdb.attach(p, r2cmd="db {}".format(hex(offset)))` and I can pass it a
r2 startup command. (I have plans on doing a PR on pwntools about this patch.)

For the binary analysis I am using IDA 7.5, its just easy to read out 
all the offsets from it, but as this binary is so small every other 
tool will be fine too.
<br/>

___
# return-to-dl-resolve ?

This technique is pretty nice as it independent of the used libc version and
it doesnt need a leak, all it requires is a `ROPchain` and a memory location
where we can write to.

To exploit it we are going to resolve the `execvp` function, we achieve this
by creating all structs needed by the dynamic linker to resolve this function,
and after it resolved it will immediatelly call it.

We need:
* forge structs: JMPREL-entry, SYMTAB-entry, STRTAB-entry
  ( we dont need everything excatly recreated just the bytes we need)
* write to a known address (we use bss segment)
* setup arguments for execvp call
* call dlresolve
* Hopefully having a shell at this point :)
<br/>

___
# First Attempt, simple ROP

To trigger the BO and call read again to read the forged structs we use:

```Python
chain_read  = b""
chain_read += b"A"*80                   # overflow padding
chain_read += b"B"*8                    # fake rbp
chain_read += p64(POP_RSI_POP_R15_RET)  # set read buffer
chain_read += p64(C_AREA)               # ptr to controlable buffer
chain_read += b"C"*8                    # dummy r15 data
chain_read += p64(MOV_EDI_CALL_READ)    # call read
chain_read += b"D"*16                   # note end of chain
```

`C_AREA` is the address of the `bss` segment which is `0x601030`, even IDA shows
that this segment is only 8byte in size, as memory is handled in pages, we have
a whole page for our structs, which is more than enough.

The two `ROPgadgets` simply setting up the arguments, as it is a 64bit binary
the three arguments for read are in the `rdi`,`rsi` and `rdx` registers.

This attempt can be found in `exp_version_1.py`. During debugging I noticed that
`read` call is this gadget:

```
.text:0000000000400500 E8 EB FE FF FF    call    _read
.text:0000000000400505 C9                leave
.text:0000000000400506 C3                retn
```

This contains a problem, the `leave` instruction restores the `rsp` from the
`rbp` and we overwrite the `rbp`. Meaning we control where the `ROPchain`
continues. This is kind of bad, we dont want this power. Because I found no
way to set the rbp to a reasonable address on the stack, this means we loose the
rest of our `ROPchain`. Of course what we can do is, set the `rsp` to the `bss`
segment and write the rest of our `ROPchain` to the `bss` segment. But I 
considered this as unecessary work, so I used a different approach `ret2csu`.
<br/>

___
# Second Attempt, ret2csu

TODO

