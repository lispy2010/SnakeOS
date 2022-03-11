#include <stddef.h>
#include <stdint.h>

#define LEFT 0
#define RIGHT 1
#define UP 2
#define DOWN 3

// snake type
typedef struct snake
{
    uint8_t x, y, direction, length;
    uint64_t score;
    uint8_t tail_x[300], tail_y[300];
} snake_t;

// apple type
typedef struct apple
{
    uint8_t x, y;
} apple_t;

// vga buffer
uint16_t *vga_buffer = (uint16_t *)0xB8000;

// vga width and height
const size_t VGA_WIDTH = 80;
const size_t VGA_HEIGHT = 25;

// snake
snake_t snake;

// apple
apple_t apple;

uint8_t game_over = 0;
uint8_t paused = 0;
uint8_t menu = 1;

// random seed
uint32_t random_seed = 0;

uint32_t get_time()
{
    uint32_t ret;
    asm volatile("rdtsc" : "=A"(ret));
    return ret;
}

void random_init(uint32_t seed)
{
    random_seed = seed;
}

uint32_t random()
{
    random_seed = random_seed * 1103515245 + 12345;
    return (uint32_t)(random_seed / 65536) % 32768;
}

void clrscr(uint8_t color)
{
    for (int x = 0; x < VGA_WIDTH; x++)
    {
        for (int y = 0; y < VGA_HEIGHT; y++)
        {
            vga_buffer[y * VGA_WIDTH + x] = (color << 8) | ' ';
        }
    }
}

void putchar(char ch, int x, int y, uint8_t color)
{
    vga_buffer[y * VGA_WIDTH + x] = (color << 8) | ch;
}

void print(const char *str, int x, int y, uint8_t color)
{
    for (int i = 0; str[i] != '\0'; i++)
    {
        putchar(str[i], x + i, y, color);
    }
}

void itoa(int num, char *str, int base)
{
    int i = 0;
    int isNegative = 0;

    // Handle 0 explicitely, otherwise empty string is printed for 0
    if (num == 0)
    {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    // In standard itoa(), negative numbers are handled only with
    // base 10. Otherwise numbers are considered unsigned.
    if (num < 0 && base == 10)
    {
        isNegative = 1;
        num = -num;
    }

    // Process individual digits
    while (num != 0)
    {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }

    // If number is negative, append '-'
    if (isNegative)
        str[i++] = '-';

    str[i] = '\0'; // Append string terminator

    // Reverse the string
    int start = 0;
    int end = i - 1;
    while (start < end)
    {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

// kbhit
int kbhit()
{
    if (inb(0x64) & 1)
    {
        if (inb(0x60) != 0)
        {
            return inb(0x60);
        }
    }
    return 0;
}

void apple_random()
{
    apple.x = random() % (VGA_WIDTH - 2) + 1;
    apple.y = random() % (VGA_HEIGHT - 2) + 1;
}

void reset()
{
    snake.x = VGA_WIDTH / 2;
    snake.y = VGA_HEIGHT / 2;
    snake.direction = random() % 4;
    snake.score = 0;
    snake.length = 1;

    // fill tail with -1
    for (int i = 0; i < 300; i++)
    {
        snake.tail_x[i] = -1;
        snake.tail_y[i] = -1;
    }

    apple_random();

    paused = 0;
}

void kernel_main()
{
    random_init(get_time());

    reset();

    apple_random();

    int clear_timer = 1;
    int move_timer = 1;

    while (1)
    {
        if (!menu)
        {
            clear_timer--;
            if (clear_timer < 0 && !game_over)
            {
                clrscr(paused ? 0x22 : 0xAA);
                clear_timer = 20000;

                // make gray border
                for (int x = 0; x < VGA_WIDTH; x++)
                {
                    for (int y = 0; y < VGA_HEIGHT; y++)
                    {
                        if (x == 0 || x == VGA_WIDTH - 1 || y == 0 || y == VGA_HEIGHT - 1)
                        {
                            putchar(' ', x, y, 0x77);
                        }
                    }
                }

                if (paused)
                {
                    print("Paused", VGA_WIDTH / 2 - 5, VGA_HEIGHT / 2 - 1, 0x2F);
                    print("Press Space to resume", VGA_WIDTH / 2 - 12, VGA_HEIGHT / 2 + 1, 0x2F);
                }

                // draw score
                print("Score:", 1, 0, 0x70);
                // score_string has max digits of uint64_t
                char score_string[20];
                itoa(snake.score, score_string, 10);
                print(score_string, 8, 0, 0x70);
            }

            move_timer -= paused ? 0 : 1;
            if (move_timer == 0 && !game_over && !paused)
            {
                move_timer = 50000;
                switch (snake.direction)
                {
                case UP:
                    snake.y--;
                    break;
                case DOWN:
                    snake.y++;
                    break;
                case LEFT:
                    snake.x--;
                    break;
                case RIGHT:
                    snake.x++;
                    break;
                }

                // tail update
                // code taken from nvitanovic (youtube)
                uint8_t prev_x = snake.tail_x[0];
                uint8_t prev_y = snake.tail_y[0];
                uint8_t prev_x_2, prev_y_2;
                snake.tail_x[0] = snake.x;
                snake.tail_y[0] = snake.y;
                for (int i = 1; i < snake.length; i++)
                {
                    if (snake.tail_x[i] == -1 || snake.tail_y[i] == -1)
                    {
                        continue;
                    }
                    prev_x_2 = snake.tail_x[i];
                    prev_y_2 = snake.tail_y[i];
                    snake.tail_x[i] = prev_x;
                    snake.tail_y[i] = prev_y;
                    prev_x = prev_x_2;
                    prev_y = prev_y_2;
                }

                // if touching tail (except for first one, which is the head)
                // game over
                for (int i = 1; i < snake.length; i++)
                {
                    if (snake.tail_x[i] == -1 || snake.tail_y[i] == -1)
                    {
                        continue;
                    }
                    if (snake.x == snake.tail_x[i] && snake.y == snake.tail_y[i])
                    {
                        game_over = 1;
                    }
                }

                // if touching border die
                if (snake.x == 0 || snake.x == VGA_WIDTH - 1 || snake.y == 0 || snake.y == VGA_HEIGHT - 1)
                {
                    game_over = 1;
                }
            }

            if (snake.x == apple.x && snake.y == apple.y)
            {
                snake.score++;
                apple_random();
                snake.length++;
            }

            if (!game_over && !paused)
            {
                // draw tail
                for (int i = 1; i < snake.length; i++)
                {
                    if (snake.tail_x[i] == -1 || snake.tail_y[i] == -1)
                    {
                        continue;
                    }
                    putchar(' ', snake.tail_x[i], snake.tail_y[i], 0x11);
                }

                // draw the snake
                putchar(' ', snake.x, snake.y, 0x99);

                // draw apple
                putchar(' ', apple.x, apple.y, 0xCC);
            }

            if (game_over)
            {
                clrscr(0x00);

                print("Game over!", VGA_WIDTH / 2 - 8, VGA_HEIGHT / 2 - 1, 0x0F);
                print("Your score:", VGA_WIDTH / 2 - 9, VGA_HEIGHT / 2, 0x0F);

                char score_string[20];
                itoa(snake.score, score_string, 10);
                print(score_string, VGA_WIDTH / 2 + 3, VGA_HEIGHT / 2, 0x0F);

                // get how many digits the score has (no strlen)
                int score_digits = 0;
                while (score_string[score_digits] != '\0')
                {
                    score_digits++;
                }

                print("Press any key to retry", VGA_WIDTH / 2 - 14, VGA_HEIGHT / 2 + 2, 0x0F);
                while (!kbhit())
                {}

                game_over = 0;
                reset();
            }

            int key = kbhit();
            if (key)
            {
                switch (key)
                {
                case 0x11:
                    snake.direction = UP;
                    break;
                case 0x1F:
                    snake.direction = DOWN;
                    break;
                case 0x1E:
                    snake.direction = LEFT;
                    break;
                case 0x20:
                    snake.direction = RIGHT;
                    break;
                case 0x39:
                    paused = !paused;
                    break;
                }
            }
        }
        else
        {
            clrscr(0x22);
            print("SnakeOS", VGA_WIDTH / 2 - 6, VGA_HEIGHT / 2 - 8, 0x2F);
            print("Use WASD keys to control the snake", VGA_WIDTH / 2 - 19, VGA_HEIGHT / 2 - 5, 0x2F);
            print("Avoid borders and yourself", VGA_WIDTH / 2 - 14, VGA_HEIGHT / 2 - 4, 0x2F);
            print("Eat apples to grow", VGA_WIDTH / 2 - 13, VGA_HEIGHT / 2 - 3, 0x2F);
            print("Press any key to start!", VGA_WIDTH / 2 - 14, VGA_HEIGHT / 2 - 2, 0x2F);
            print("Made by lispy2010", 0, VGA_HEIGHT - 1, 0x2F);
            print("                                     ", VGA_WIDTH / 2 - 21, VGA_HEIGHT / 2 - 9, 0x77);
            putchar(' ', VGA_WIDTH / 2 - 21, VGA_HEIGHT / 2 - 8, 0x77);
            putchar(' ', VGA_WIDTH / 2 - 21, VGA_HEIGHT / 2 - 7, 0x77);
            putchar(' ', VGA_WIDTH / 2 - 21, VGA_HEIGHT / 2 - 6, 0x77);
            putchar(' ', VGA_WIDTH / 2 - 21, VGA_HEIGHT / 2 - 5, 0x77);
            putchar(' ', VGA_WIDTH / 2 - 21, VGA_HEIGHT / 2 - 4, 0x77);
            putchar(' ', VGA_WIDTH / 2 - 21, VGA_HEIGHT / 2 - 3, 0x77);
            putchar(' ', VGA_WIDTH / 2 - 21, VGA_HEIGHT / 2 - 2, 0x77);
            print("                                     ", VGA_WIDTH / 2 - 21, VGA_HEIGHT / 2 - 1, 0x77);
            putchar(' ', VGA_WIDTH / 2 + 15, VGA_HEIGHT / 2 - 8, 0x77);
            putchar(' ', VGA_WIDTH / 2 + 15, VGA_HEIGHT / 2 - 7, 0x77);
            putchar(' ', VGA_WIDTH / 2 + 15, VGA_HEIGHT / 2 - 6, 0x77);
            putchar(' ', VGA_WIDTH / 2 + 15, VGA_HEIGHT / 2 - 5, 0x77);
            putchar(' ', VGA_WIDTH / 2 + 15, VGA_HEIGHT / 2 - 4, 0x77);
            putchar(' ', VGA_WIDTH / 2 + 15, VGA_HEIGHT / 2 - 3, 0x77);
            putchar(' ', VGA_WIDTH / 2 + 15, VGA_HEIGHT / 2 - 2, 0x77);
            while (!kbhit())
            {
            }
            menu = 0;
        }
    }
}