___
# Intro

Lately I had to modify ELF-headers, during that I noticed that I lack some
basic knowledge about how that all works together. So I thought that's a good
opportunity to get a better understanding. There is stuff I never really
understood and never needed to know, for example, whats the difference between
segments and sections?

NOTE: all images are taken from [010editor](https://www.sweetscape.com/010editor/)
using the ELF-plugin.

Furthermore, its not a full guide, just the stuff I would otherwise forget
if I dont have it somewhere to look up.

<br/>

___
# ELF Header

The ELF file header itself is fairly simple, it contains stuff like `start
address` if its `dynamic` or `statically` linked, the offset to
Program Header Table (PHT) and Section Header Table (SHT).
Also the amount of PHT and SHT entries along with their sizes which makes
parsing very easy.

The static or dynamic information can be found in the `e_type` header entry,
where `ET_EXEC` stands for statically linked and `ET_DYN` for dynamically.

The picture below shows a hello world sample, and the stripped version on the
rigth.

![ELF Header](https://raw.githubusercontent.com/gast04/gast04.github.io/master/img/elf_header.png)

Taking a look ahead, we can already see that the stripped version does not
contain a symbol table, but still a dynamic symbol table.

# Program Header Table

From [1]
```
The ELF object file format uses program headers, which are read by the system
loader and describe how the program should be loaded into memory. These program
headers must be set correctly in order to run the program on a native ELF system.
```

Ok easy, so it defines where parts of the binary are loaded into memory.
For a simple hello world binary this looks like:

![PHT Sample](https://raw.githubusercontent.com/gast04/gast04.github.io/master/img/pht_sample.png)

What we directly notice is that there is only one program header entry which
has `RX` permission, and it has no virtual address or file offset, why is
that the case? this seems odd.

If the virtual address is zero then the addresses specified in the sections are
used. From those sections which use data from the same file offset as the
segment.

The type of the program header entry can be one of the following:

| Type           | Description |
| -------------- | ----------- |
| PT_NULL (0)    | Indicates an unused program header. |
| PT_LOAD (1)    | Indicates that this program header describes a segment to be loaded from the file. |
| PT_DYNAMIC (2) | Indicates a segment where dynamic linking information can be found. |
| PT_INTERP (3)  | Indicates a segment where the name of the program interpreter may be found. |
| PT_NOTE (4)    | Indicates a segment holding note information. |
| PT_SHLIB (5)   | A reserved program header type, defined but not specified by the ELF ABI. |
| PT_PHDR (6)    | Indicates a segment where the program headers may be found. |


# Trial and Error

Reading through ELF documentations I thought about what parts can I
modify, for example, how can I add an extra code segment?

I can not simply add data at the end of the file and rewrite the comment
section, to load my data as RX. Cause as we learned segments are used at
runtime, and I cant add data to the text segment cause the data is in the
middle of the binary, so I would need to shift a lot of stuff. Thats error
prone.


### Add a new segment

The PT_NOTE program header entry is not really needed, it just contains some
information about the binary. I changed its values to create a new RX area.

The steps to achieve this:

* change p_offset_FROM_FILE_BEGIN

This defines where the data starts which will be loaded, I simply append
data at the end of the binary and set the offset to it. 0x1000 padding was
applied just to get a nice address.

* change p_vaddr_VIRTUAL_ADDRESS

This defines where the data starting at `p_offset_FROM_FILE_BEGIN` will be
loaded in memory.

* change p_vaddr_PHYSICAL_ADDRESS

This only applies for systems in which physical addressing is relevant, so
we dont have to care.

* change p_filesz_SEGMENT_FILE_LENGTH and p_memsz_SEGMENT_RAM_LENGTH

This specifies how much we load into memory.

* change p_type

Specifies the type of the segment, we want it `PT_LOAD`. This assures the
loader will load the content.

* change p_flags

Specifies the execution flags, as we add code we need `PF_Read_Exec`.


These steps are enough to create an extra RX segment. Which gets loaded at
runtime. No other changes are needed. However out curiosity we could
add a section to the newly created segment.

### Add a section to the segment

The link between section and segment happens via the virtual address,
this is also what you see via `readelf -l <binary>`. (we will see a bit later)

I decided to overwrite the comment section, to make this work it is only needed
to adjust the addresses for virtual space, file offset and size.

Properly applying the changes, gives the readelf output:
(I simply appended a bunch of A's at the end of the binary, and the segment
has the size 0x64, the `.comment` section got renamed to `cc`)

```
>> readelf -l sample.mod

Elf file type is DYN (Shared object file)
Entry point 0x530
There are 9 program headers, starting at offset 64

Program Headers:
  Type           Offset             VirtAddr           PhysAddr
                 FileSiz            MemSiz              Flags  Align
  PHDR           0x0000000000000040 0x0000000000000040 0x0000000000000040
                 0x00000000000001f8 0x00000000000001f8  R      0x8
  INTERP         0x0000000000000238 0x0000000000000238 0x0000000000000238
                 0x000000000000001c 0x000000000000001c  R      0x1
      [Requesting program interpreter: /lib64/ld-linux-x86-64.so.2]
  LOAD           0x0000000000000000 0x0000000000000000 0x0000000000000000
                 0x0000000000000838 0x0000000000000838  R E    0x200000
  LOAD           0x0000000000000db8 0x0000000000200db8 0x0000000000200db8
                 0x0000000000000258 0x0000000000000260  RW     0x200000
  DYNAMIC        0x0000000000000dc8 0x0000000000200dc8 0x0000000000200dc8
                 0x00000000000001f0 0x00000000000001f0  RW     0x8
  LOAD           0x0000000000002100 0x0000000000210000 0x0000000000210000   <--- modified segment (file off, virt off)
                 0x0000000000000064 0x0000000000000064  R E    0x8
  GNU_EH_FRAME   0x00000000000006f0 0x00000000000006f0 0x00000000000006f0
                 0x000000000000003c 0x000000000000003c  R      0x4
  GNU_STACK      0x0000000000000000 0x0000000000000000 0x0000000000000000
                 0x0000000000000000 0x0000000000000000  RW     0x10
  GNU_RELRO      0x0000000000000db8 0x0000000000200db8 0x0000000000200db8
                 0x0000000000000248 0x0000000000000248  R      0x1

 Section to Segment mapping:
  Segment Sections...
   00     
   01     .interp 
   02     .interp .note.ABI-tag .note.gnu.build-id .gnu.hash .dynsym .dynstr 
          .gnu.version .gnu.version_r .rela.dyn .rela.plt .init .plt .plt.got 
          .text .fini .rodata .eh_frame_hdr .eh_frame 
   03     .init_array .fini_array .dynamic .got .data .bss 
   04     .dynamic 
   05     cc                  <--- modified section, alone in Segment 5
   06     .eh_frame_hdr 
   07     
   08     .init_array .fini_array .dynamic .got
```

Also the sections and offsets in r2 look good:

```
[0x00000530]> iS
[Sections]

nth paddr        size vaddr       vsize perm name
―――――――――――――――――――――――――――――――――――――――――――――――――
...
12  0x00000500   0x20 0x00000500   0x20 -r-x .plt
13  0x00000520    0x8 0x00000520    0x8 -r-x .plt.got
14  0x00000530  0x1a2 0x00000530  0x1a2 -r-x .text
...
23  0x00001000   0x10 0x00201000   0x10 -rw- .data
24  0x00001010    0x0 0x00201010    0x8 -rw- .bss
25  0x00002100   0x64 0x00210000   0x64 -r-x cc
26  0x00001040  0x5e8 0x00000000  0x5e8 ---- .symtab
...

[0x00000530]> px 10 @ 0x210000
  - offset -   0 1  2 3  4 5  6 7  8 9  A B  C D  E F  0123456789ABCDEF
  0x00210000  4141 4141 4141 4141 4141                 AAAAAAAAAA
```

Thats pretty nice.

<br/>

___
# Segments for runtime, Sections for linking

Reading the headline above slowly, I am asking the question, if sections are not
needed for runtime can we remove that information from a compiled binary?

The answer is yes we can. Taking a small hello world binary all we have to do
is setting `e_shnum_NUMBER_OF_SECTION_HEADER_ENTRIES = 0` after its
save to delete the table, which is stored at the end of the binary.

Comparing the modified binary with the original one:

![Alt text](Images/no_sections.png)

This shrinks the file size a bit and also has some nice "anti" reverse
engineering features. While the program executes normally, gdb is not able
to start it, and throws:

```
>> gdb -q main
"/home/niku/MyTests/elfmod/main": not in executable format: file format not recognized
(gdb)
```

Furthermore sections are also one the first things I am used to look when I
reverse engineer something, but opening in r2 shows only:

```
>>  r2 -c iS -q main
[Sections]

nth paddr       size vaddr       vsize perm name
――――――――――――――――――――――――――――――――――――――――――――――――
0   0x000004d0  0x18 0x000004d0   0x18 ---- .rela.plt
1   0x00000410  0xc0 0x00000410   0xc0 ---- .rel.plt
2   0x00000fb8  0x18 0x00200fb8   0x18 ---- .got.plt
```

This is nice in my mind, but of course this will not stop any reverse engineer
it might cause a smile though.

One more note on segments and sections, as segments are for runtime, as the
example above shows, also the execution permission from the segments are taken
during runtime. This means the text section can be specified as read only
however if the corresponding segment has rx, the text section will also have
rx at runtime.

<br/>

___
# Ressources

* [1] https://ftp.gnu.org/old-gnu/Manuals/ld-2.9.1/html_node/ld_23.html
* [2] https://www.intezer.com/blog/research/executable-linkable-format-101-part1-sections-segments/

<br/>

___
### zu guter letzt ;)

If you have any question or like this kind of content dont hesitate to
reach out to me at niku.systems at gmail.com. Also let me know if you are
interested in my tools at `http://niku.systems/#tools`(still in slow develop)
or wanna hire me for a project.
