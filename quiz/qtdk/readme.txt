Welcome
-------

Welcome to the QuizTron48k development kit!

This kit can be used to create new QuizTron48k tape files with your
own question sets or whatever.

Use at your own risk. Copyright (c)2016 Jari Komppa, but 
released under the unlicense, see http://unlicense.org 
(practically public domain).

Toss me a note somehow if you use this for something.


Contents
--------

Files you should have:

bg.scr                 - The game's background picture.
crt0.ihx               - Game code, compiled to intel hex.
entertainment1.scr     - Example loading screen
entertainment1.txt     - Example data file
quiztronamatic.exe     - Tool that processes the game data
mackarel.exe           - Tool that produces .tap from the above
m.bat                  - Batch file to compile the example
readme.txt             - You're likely reading this.


Data file
---------

The example data file should be pretty self-descriptive, but here's some rules.
All lines that start with a # are considered comments, and empty lines are ignored.
If a line has a space, it's not empty, so be careful.

The file starts with one or more lines of intro text. This is your chance to
make Quiztron be as passive aggressive as you wish. (Do try to keep it family 
friendly, unless you're going totally overboard, but that should be made obvious).

The intro text should end with a line that starts with a / character. That line is
also ignored in the output, but used to figure out where the intro lines end.

After that you should have groups of five lines; one line question, four answers. 
The first answer should always be the correct one, the game randomizes the answer
order at runtime.

And that's it.

There should be at least one question, at least one intro line, but no more than 255
of either, or things will blow up. You've been warned.


Building
--------

To build the game, first run your data through quiztronamatic.exe, like:

    quiztronamatic mydata.txt mydata.txt.packed
    |              |          |
    +--------------|----------|--- Quiztron processing tool
                   +----------|--- Your question data file
                              +--- Resulting processed file

You may want to note what the tool says, as it outputs all sorts of neat diagnostic
messages, like when word wrapping goes awry or it detects some other kind of
problem.

Most of the processing time goes to compression. It's not because the compression
is super great, but because it's super unoptimized. Sorry about that.

After you're done, you need to prepare "low block" for mackarel by appending the
game background image to your new data file, like so:

    copy /b bg.scr+mydata.txt.packed lowblock.dat 
    |    |  |      |                 |
    +----|--|------|-----------------|--- Copy command
         +--|------|-----------------|--- Do a binary copy instead of text.
            +------|-----------------|--- Background art file
                   +-----------------|--- Your processed data file
                                     +--- Resulting filename

After this you just need to feed all sorts of things to mackarel to generate the
tap file.

    mackarel crt0.ihx out.tap appname load.scr -nosprestore -noei -lowblock lowblock.dat 0x5b00
    |        |        |       |       |        |            |     |         |            |
    +--------|--------|-------|-------|--------|------------|-----|---------|------------|--- Mackarel tool
             +--------|-------|-------|--------|------------|-----|---------|------------|--- Game code in intel hex
                      +-------|-------|--------|------------|-----|---------|------------|--- Output tap file name
                              +-------|--------|------------|-----|---------|------------|--- App name inside tap
                                      +--------|------------|-----|---------|------------|--- Loading screen (optional)
                                               +------------|-----|---------|------------|--- Don't bother restoring SP
                                                            +-----|---------|------------|--- Don't bother enabling interrupts
                                                                  +---------|------------|--- Include a low block data file
                                                                            +------------|--- Data file created above
                                                                                         +--- Address to load to.
                                                                                         
Mackarel will also output some stuff, but the thing you should pay attention 
to is the last memory map.

    On boot: rrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr
             sssssssssssssssssssssssssssLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
             LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL..
             ................CCCCCCCCCCCCCCCCCCCCCCCCCCCCCC..................
         
The L:s are your question data. In the map above we can see that we have plenty
of space for additional stuff, but if you add another couple hundred lines of
intro text, that space will probably not be enough.

Just something to keep in mind.

And that's that. 

Cheers!

http://iki.fi/sol

   