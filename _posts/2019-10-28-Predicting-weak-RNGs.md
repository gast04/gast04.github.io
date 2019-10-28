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
# Neural Network approach One (FAIL)

In my mind that seemed to be a task for LSTM, since those are the network which you use if you have to 
predict the next element of a sequence. To keep it short, this was my first approach I fought a dragon 
and used always the shield and observed if the dragon is hitting or missing me, I could distinguish this
by the messages of the game. I observed 2.000 sequences each of the length 201 and after I trained a 
LSTM network with the 200 length  sequences and used the 201th element as label.

Unfortunatelly this did not work at all, I had a hard time training this network, I will not go into much 
further details since already during training I could not make the error close to zero, which it has to be
since the outcome is either 0 or 1, so if it is not zero or close to zero at least the uncertainty would
be too much to kill 20 dragons.

I could kill 2 dragons, but I can also kill 2 dragons by always attacking all the time.

My explanations for this is, LSTM and Recurrent Neural Networks need a repeating sequence, if the 
200 Sequence input shows no repetition, it will not be able to learn.

I tested this and generated simple sequences like `10101...`, `11011011...`, `00010001...` and so on, 
the same network performed perfectly, so I guess that my assumption might be true.

___
# Neural Network approach Two

I used a simple network, after all neural networks are pattern detector so I tried the following, I created a 
model with the structure `200->200->100->1`. 200 inputs fully connected leading to one output, if there
is a pattern in the RNG this will hopefully detect it. 

I was amazed as I got immediatelly a zero error at the training set. I thought about overfitting so I 
increased the training data to 10.000 sequences and labels, still I got an error of Zero, this doesn't 
look like overfitting for me anymore.

I tested it in fighting dragons, the weird things was, still at max two dead dragons...

That took me a while, until I noticed that it is also random if the Hero is hitting or not, this
means that I have to predict the next to elements of a given sequence. I generated new sequences and labels
and was hoping that I still get a zero error, and I really did. I used the following pattern for
my attack scheme now. The Hero is attacking first so the first element of the prediction revers to the
hero and the second element to the dragon. We get the following table:

| Hero pred.| Dragon pred. | Description |  
|-----------|-----------|-------------|
| 0         | 0         | Hero and Dragon will miss | 
| 0         | 1         | Hero will miss, Dragon will hit | 
| 1         | 0         | Hero will hit, Dragon will miss | 
| 1         | 1         | Hero will hit, Dragon will hit  | 

We wanna avoid every hit from the dragon so we use in all cases the shield except for the case
when the Hero will hit and the Dragon will miss. Means only in one out of four we will attack 
and make damage.

I implemented it and could kill a dragon without getting any damage. (Verbose output of my Bot, cleaned output)

DH - Dragon Health
HH - Hero Health
DA - Dragon Attack
HA - Hero Attack
```
BLACK DRAGON arrived
DH: -1, HH: -1  | DA: hit, HA: shield
DH: 94%, HH: -1 | DA: missed, HA: hit
DH: 94%, HH: -1 | DA: hit, HA: shield
DH: 81%, HH: -1 | DA: missed, HA: hit
DH: 74%, HH: -1 | DA: missed, HA: hit
DH: 66%, HH: -1 | DA: missed, HA: hit
DH: 66%, HH: -1 | DA: missed, HA: shield
DH: 52%, HH: -1 | DA: missed, HA: hit
DH: 48%, HH: -1 | DA: missed, HA: hit
DH: 48%, HH: -1 | DA: hit, HA: shield
DH: 48%, HH: -1 | DA: missed, HA: shield
DH: 42%, HH: -1 | DA: missed, HA: hit
DH: 42%, HH: -1 | DA: hit, HA: shield
DH: 42%, HH: -1 | DA: missed, HA: shield
DH: 26%, HH: -1 | DA: missed, HA: hit
DH: 26%, HH: -1 | DA: missed, HA: shield
DH: 26%, HH: -1 | DA: missed, HA: shield
DH: 26%, HH: -1 | DA: missed, HA: shield
DH: 26%, HH: -1 | DA: hit, HA: shield
DH: 26%, HH: -1 | DA: hit, HA: shield
DH: 26%, HH: -1 | DA: missed, HA: shield
DH: 26%, HH: -1 | DA: missed, HA: shield
DH: 26%, HH: -1 | DA: hit, HA: shield
DH: 26%, HH: -1 | DA: hit, HA: shield
DH: 26%, HH: -1 | DA: missed, HA: shield
DH: 26%, HH: -1 | DA: hit, HA: shield
DH: 26%, HH: -1 | DA: hit, HA: shield
DH: 26%, HH: -1 | DA: hit, HA: shield
DH: 26%, HH: -1 | DA: hit, HA: shield
DH: 26%, HH: -1 | DA: missed, HA: shield
DH: 26%, HH: -1 | DA: missed, HA: shield
DH: 26%, HH: -1 | DA: hit, HA: shield
DH: 26%, HH: -1 | DA: missed, HA: shield
DH: 26%, HH: -1 | DA: hit, HA: shield
DH: 26%, HH: -1 | DA: missed, HA: shield
DH: 11%, HH: -1 | DA: missed, HA: hit
DH: 11%, HH: -1 | DA: missed, HA: shield
Dragon killed
```

As it shows whenever the dragon was supposed to hit, the hero is using his shield, the hero is only
attacking when the dragon will miss his attack. This was working for all 20 Dragons giving me all
drinks needed to get the flag from valis and still with 100% health left.

```
YOU ENTER THE TAVERN AND APPROACH VALIS.

- FLA... THE FLAG... I'VE DONE EVERYTHING... PLEASE...
- EH.  FINE.  HERE IT IS: flag{the-flag-goes-here}
```

___
# Code
The organizers published the challenge [code](https://github.com/koriakin/cpuadventure).

I cloned the repository and added my bot and all my code, there is optimization possible but it works ;) 
everything can be found on my [github](https://github.com/gast04/CTF-Writeups/tree/master/DragonTeaser).


***NOTE:***
I finished/solved this after the CTF event but it was still really funny to solve it.

