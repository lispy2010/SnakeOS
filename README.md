
# SnakeOS

SnakeOS is an operating system that, well, only runs snake game.
In this game, you use WASD keys to move the snake, and you hahe to avoid touching your tail or walls.
Eating apples increases the score and makes the snake longer.

# Warning

This game may not work properly on some computers. I tested it using VirtualBox on Ubuntu 20.04, and the game had incorrect colors. But with QEMU it worked fine.

# Dependencies

To build this game, you need [GCC Cross Compiler](https://wiki.osdev.org/GCC_Cross-Compiler) installed. To create ISO, you will also need xorriso, mtools and grub-common packages installed.

# Build process

To build, just run build.sh file from the command line:
`$ ./build.sh`

# Credits

Some code (more specifically, tail logic) is taken from a YouTuber NVitanovic.
Made by lispy2010 and GitHub Copilot with <3
