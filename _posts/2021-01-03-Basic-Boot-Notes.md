___
# Intro

It's work in progress... just some notes I forgot durin working on a OS

Bootloader must be of size 512 bytes, and end with the bytes 0xAA55 which 
gives 510 bytes for the actual code. Otherwise it will not be detected as
bootloader as this is the bootloader signature.


# Compile and startup commands

```
# compiler it using nasm
nasm -f bin -o bloader bloader.asm

# starting with qemu
qemu-system-i386 -s -S -boot c osSmall
#   -s short for "-gdb tcp::1234"
#   -S freeze CPU at startup (use 'c' to start execution)
#   -boot c for booting from hard disk (but we dont get the far here)
#         just dont use 'n' for PXE boot

# connect debugger
r2 -c "db 0x7c00" -b 16 -d gdb://127.0.0.1:1234
#   add breakpoint at 0x7c00 as this is where the bootloader gets loaded

```


# Ressources

* http://www.osdever.net/tutorials/view/hello-world-boot-loader
* http://mikeos.sourceforge.net/write-your-own-os.html
* http://x86asm.net/articles/introduction-to-uefi/index.html
* http://x86asm.net/articles/uefi-programming-first-steps/


<br/>

___
# UEFI Protocols

Not all protocols are defined in the UEFI specification, EDK II for example
provides many more for easier usage, however when you use them, be aware
that they might not be available on an UEFI compliant system.




