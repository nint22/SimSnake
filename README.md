SimSnake
========

An evolutionary genetic-algorithm that attempts to solve the "Snake Game", where genes
are an instruction set with data. This simulation is hooked up to a real-world 32x32 LED
screen, which is used as a screen-saver / desktop toy.

Note that there are a few random implementations of snake AI code to get you started
with. Those script files are written in assembly-like syntax, but without goto / label
mechanisms. They're also used to "seed" the gene pool.

Breeding happens after a round of ranking. The top half of the gene pool breeds with their
next rank (i.e. rank 1 breeds with rank 2, etc.), replacing the bottom half genes. Gene
breeding occurs by picking a main parent, cloning that data, then randomly swapping a small
set of chunks between the copied gene and the second parent. A 0.01% mutation rate is
introduced, where 0.01% of the data gets randomized.

Building
========

You can either build the main.cpp file or ncurses.cpp file. The first compiles the
application where it prints out all data and states to the console, while the second
creates an ncurses window within your console and draws to it sans scrolling.

Either "#define __ConsoleBuild__" at the start of main.cpp, or "#define __NCursesBuild__"
at the start of ncurses.cpp By default, the ncurses version is built.

Todo
====

+ Change language to a higher-level language, like BASIC, and breed by copying functions, not just memory chunks
+ Print simulation growht data: how much better the genes get over time...
+ Add back Linux and Windows support (this is an XCode / OSX project)

License (MIT License)
=====================

Copyright (c) 2014 CoreS2 - Core Software Solutions. All rights reserved.
Contact author Jeremy Bridon at jbridon@cores2.com for comercial use.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

