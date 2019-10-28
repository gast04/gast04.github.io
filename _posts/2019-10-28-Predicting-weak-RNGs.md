A while ago I played the [Teaser Dragon CTF 2019](https://ctftime.org/event/851) and they had a really nice
Challenge in it called `CPU Adventure`, the description was as follows:

> My grandfather used to design computers back in the 60s. While cleaning out his attic, I found a strange  machine. Next to the machine, I found a deck of punched cards labeled “Dragon Adventure Game”. After some time, I managed to hook it up to modern hardware, but the game is too hard and I cannot get to the end without cheating. Can you help me?

> I’m attaching a transcription of the punched cards used by the machine. The machine proudly claims to have 4 general purpose registers, 1kiB of data memory, and 32kiB of instruction memory.

> To play the game, connect to the server as follows:
socat tcp4-connect:cpuadventure.hackable.software:1234 fd:0,rawer

> Hint: this is a custom CPU, don’t bother googling it.

The provided program was the following one: (trimmed)

```
1100111111010000001111001100100011100000110011010000000000001100100111010100000011010011111000011111110011001110000000110111100000101000111100001000110111100010010001100...
```


I was working on another Challenge and only looking briefly on it, since it seemed to be quite hard to 
me to black box reverse engineer a unkown custom CPU, but I found time to play the game during the CTF.
It was really nice made, it is a small 7x7 square which is the game field and the challenge was to basically
kill all the Dragons in the game and give a CPU-character called Valis all the drinks which the killed 
dragons are droping. The problem was that you have to kill the dragons without cheating, otherwise you will
never get the flag.

___
# Game Introduction

The start of the game looks like the following, valis is the character we have to give the drinks.
```
> ./emu game.bin flag.txt
THERE IS A TAVERN HERE.  INSIDE THE TAVERN, YOU SEE VALIS.

SELECT AN OPTION:
- GO (N)ORTH
- GO (E)AST
- (T)ALK TO VALIS
- (D)RINK
- SHOW (I)NVENTORY

YOUR CHOICE: 
```
Let's look for Dragons, this might look like as follows.
```
THERE IS A BEIGE DRAGON HERE.  HE APPEARS TO BE GUARDING SOME KIND OF A BOTTLE.

SELECT AN OPTION:
- GO (S)OUTH
- GO (N)ORTH
- GO (E)AST
- GO (W)EST
- (F)IGHT THE BEIGE DRAGON
- (D)RINK
- SHOW (I)NVENTORY

YOUR CHOICE: 

```

We decide to figth the dragon.
```
YOU ATTACK THE BEIGE DRAGON.

- (A)TTACK
- USE (S)HIELD
- (C)HEAT

YOUR CHOICE:
```

We can attack or use the shield. Attacking might hit the dragon or we miss, at the same time the dragon attacks
us and migth hit or miss. The shield is blocking all attacks from the dragon. Cheating is giving us 1000% of 
health, but this is not allowed, otherwise we will never get the flag. 

An attack migth look like
```
YOU HIT THE BEIGE DRAGON.
ENEMY HEALTH: 92%
THE BEIGE DRAGON HITS YOU.
CURRENT HEALTH: 93%
```
or like this
```
YOU HIT THE BEIGE DRAGON.
ENEMY HEALTH: 83%
THE BEIGE DRAGON ATTACKS YOU, BUT MISSES.
```

We can see that this is randomized. As I finished the game I had to kill 20 dragons, so playing it by hand
will for sure kill your hero before it is killing all the dragons. So writing a simple bot will not solve
the task. We have now two choices either we reverse engineer the program or we are able to somehow predict the
random number generator.

I am a fan of reverse engineering, but this was new to me and I was looking for the correct punch card format
but I gave up that part quite soon. I was more interested if it is possible to build a Neural Network which 
is able to predict the next action of the Dragon, it was a custom CPU so it migth be possible that the 
number generator is weak and a Neural Net is able to do that.

___
# Neural Network approach

todo


