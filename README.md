# EE 5450 HW1: Threaded Morse Code

In this assignment, you will use each LED on its own to display a word in Morse code.

Here is how Morse code works: https://en.wikipedia.org/wiki/Morse_code

Basically, you have a fixed-length long "dash" and a fixed-length short "dot" for how long
an LED would be on for.  The space between parts of the same letter is one unit.

You only need to be responsible for letters, do not worry about incorporating numbers (unless you want to).

Pick four words, and make a thread for each word.

You can structure in a couple different ways, this is what I recommend:
1. Create a morse code function that will return you an array of on-times and off-times based on the letter char given.  The maximum number of dots and dashes for letters is four.
2. Create entry points for each word, and call your function as many times as you have letters for each word.
3. Setup all the GPIOs and create the threads.