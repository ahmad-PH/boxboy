
# BoxBoy - UT AP Spring 95

This is an implementation of a minimal subset of [BoxBoy](https://en.wikipedia.org/wiki/BoxBoy!), a puzzle platformer video game. Implementing this subset of the game was the 3rd Assignment of my the AP course that I took in [University of Tehran](https://ut.ac.ir/en), during my 2nd semester. 

Since I love creating games, I put a good amount of work into this assignment, and I think the final result is cute, and worth checking out :).

I will soon be adding a video of the game, so that you can quickly get a feel of what the game looks like.

Please do note, that we were restricted to write all our code in a single file, and NOT to use inheritance or classes at all, since the corresponding material (OO design, and multifile coding) wasn't yet taught by the course instructor. Had we not been restricted, I would've have definitely used both these features. So, please do not judge my OO design skills based on the source code :D.

## Getting Started

To play the game, either clone the repository or simply download its ZIP and then extract it. Then, enter the extracted directory, and run the following:
```
./boxboy.out < <map-address>
```
and replace `<map-address>` with the map you want to run. you can find a couple of maps in the `maps` folder.

## Controls

Here is a list of things you can do, and associated controls:
- move the character: arrow keys
- jump: `space`
- create boxes: hold down `x`, and then use the arrow keys
- throw the created boxes: `c` (to throw them to right) or `z` (to throw them to left)
- teleport to the location of the last created box: press `space` while the boxes are still attached to you, but you are in a position where you can't jump. (for example, you're stuck mid-air).

## Author

* **Ahmad Pourihosseini** -  [ahmad-PH](https://github.com/ahmad-PH)

