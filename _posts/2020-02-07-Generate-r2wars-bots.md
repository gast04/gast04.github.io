
I organized an r2wars competition, and that brought me on an idea. It should be possible to generate a bot using machine learning by a genetic algorithm. So here is a short documentation about what I tried and 
how it worked out.

[Sourcecode](https://github.com/gast04/AI-Stuff/tree/master/BotGeneration)

___
# What is r2wars?
Write a bot in an assembler language. This bot is copied in 1024 bytes together with another bot (opponent), the bot who survives longer wins. Usual strategies are, overwrite as much of 1024bytes with invalid instructions to crash the other bot. Don't overwrite yourself, of course. Also, copying yourself to another memory (inside the 1024 bytes) can be a strategy and many more.

The assembly code is emulated by [ESIL](https://radare.gitbooks.io/radare2book/disassembling/esil.html)
and compiled with [rasm2](https://r2wiki.readthedocs.io/en/latest/tools/rasm2/).

___
# Idea

The code must be compilable with rasm2. To avoid compile errors, I predefined instructions. This limits the possible generated bots but speeds up generation. The emulation happens with ESIL using [r2pipe](https://github.com/radareorg/radare2-r2pipe). Emulation was done without an opponent, only executed instructions and writes mater.

My first steps: 
* generate x random bots which have y instructions
* emulate bots and measure how many instructions get executed and how many bytes have been overwritten
* fitness of a bot = (executed_instructions + 2*write_damage)
* apply evolution steps: merge bots, replace instructions, cut bots and extend

I started with following instructions: 
```Python
opcodes = [("mov",2), ("add",2), ("sub",2), ("jmp",1), 
 ("push",1), ("pushad",1), ("inc", 1), ("dec",1)]
```

The numbers next to instructions defines how many arguments it takes. (I know pushad does not take arguments but it
was easier to encode it like this.) Writes to memory like "mov [eax], 0x1234" are not supported for the moment. So 
other bots can be only killed by moving the stack pointer and performing push instructions.

The initial population bots had a size of 5 instructions. They grow in the mutation steps. The amount of executed instructions is limited to 500 in order to deal with loops. My goal with these instructions was to evolve a bot something like:
```
mov eax, 0x321
mov esp, 0x123
label1:
 push eax
jmp label1
inc edx
```

That seemed possible to me and not too complicated. But it turned out the configuration is tricky.

___
# Approach Zero

This first approach has several drawbacks, the random bot generation worked. But the problem was the design of the fitness function and the mutation steps.

I ended up getting bots like:
```
inc eax
label1:
add edi, 0x3ba
jmp label1
inc edx
```

An endless loop doing nothing. The executed instruction value in the fitness function was way too dominant in all my bots. Rarely there was a bot doing any damage at all even after several (~40) generations.

Once a bot found an endless loop, it stayed in it, and not mutation could improve it.

Mutation steps:
* combine bots, split two bots at random and swap there parts (turned out to be awful)
* choose an instruction and replace it with another one (worked well)
* cut a bot and add instructions randomly (turned out to be awful)

The combine bots mutation was just way too aggressive and just broke the evolution of bots. The same goes for cutting bots. Replacing an arbitrary instruction by a random one worked quite well and achieved some improvement.


___
# Approach One

I fixed the previous mistakes. The bot size was always 20 instructions in the beginning. The cutting of the bots was removed. Splitting and combining stayed in the mutations process. I adopted the fitness function to ignore the executed instructions and only count the damage. This makes sense since more damage requires more instructions to execute.

I also added a few more instructions: 
```Python
opcodes = [("mov",2), ("add",2), ("sub",2), ("jmp",1), 
 ("push",1), ("pushad",1), ("inc", 1), ("dec",1), 
 ("cmp", 2), ("je", 1), ("jne", 1)]
```

Hyperparameters of the Training:
```
initial population size: 20
bot start size: 20
generations: 50
winners to keep: 5
combinations: 3
mutation probability: 0.1
```

This setup generated one very interessting bot, which caused 1764 damage in the emulation after
500 executed instructions. The generated bot:
```
mov esp, 0xb9
add eax, 0x2b24cfd
pushad esp
jne esp
jne edx
inc ebx
mov esp, 0x312
add edi, 0xdb2fdff
add esp, 0x399dae9
pushad edx
sub esi, 0xa7239ea7
pushad eax
pushad edx
cmp esi, 0xf19e88fc
mov ebx, 0x5899022d
jne label18
dec esp
add ecx, 0x39c
label18:
pushad edi
cmp ecx, 0x1fc
```

This is pretty impressive in my mind. I run this bot against the bots from the competition: 
(GA-bot.x86-32)
```
1 dux-bot2.x86-32 13.00 (13-0-1 with 26 overall).
2 f0rki.arm-32 12.00 (12-0-2 with 25 overall).
3 kamikaze.x86-32 11.00 (11-0-3 with 23 overall).
4 nikubotV4.x86-32 10.00 (10-0-4 with 21 overall).
5 salt.x86-32 9.00 (9-0-5 with 20 overall).
6 f0rki.arm-64 9.00 (9-0-5 with 19 overall).
7 yrlfV2.x86-64 7.00 (7-0-7 with 14 overall).
8 GA-bot.x86-32 6.50 (6-1-7 with 14 overall).
9 mick.arm-32 6.00 (6-0-8 with 16 overall).
10 T2.x86-64 6.00 (6-0-8 with 12 overall).
11 clony.x86-32 4.00 (4-0-10 with 10 overall).
12 noclue.x86-32 3.50 (3-1-10 with 8 overall).
13 kw-0.x86-32 3.50 (3-1-10 with 6 overall).
14 searchbot.x86-32 2.50 (1-3-10 with 6 overall).
15 yrlfV3.x86-32 2.00 (2-0-12 with 9 overall).
```

Not bad 8/15. It even won 6 battles.


___
# Approach Two

I removed the crossing between bots, and only applied random instruction change in the mutation phase. The final bot I got had a damage score of 2549. Even stronger than the previous one. In the competition it placed slightly worse than the previous bot.

___
# Conclusion

Adding more instructions will certainly increase the deadlyness of the bot. 
But these instructions have to be encoded in a way to allow simple handling. Also 
more generations will for sure increase the damage of the bot, thats a time factor.
Many more optimizations can be applied. 

I wanted to test if the overall idea works and I can conclude it works :)

