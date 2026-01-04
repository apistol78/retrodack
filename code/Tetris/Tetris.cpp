// #include <conio.h>
// #include <stdbool.h>
// #include <stdint.h>
#include <stdio.h>
// #include <stdlib.h>
#include <string.h>
// #include <time.h>
// #include <windows.h>
#include <math.h>

#include "Runtime/Runtime.h"

#define A_WIDTH  10  // Arena width
#define A_HEIGHT 20  // Arena height
#define T_WIDTH  4   // Tetromino width
#define T_HEIGHT 4   // Tetromino height

const int tetrominoes[7][16] =
{
	{0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},  // I
	{0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0},  // O
	{0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0},  // S
	{0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0},  // Z
	{0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},  // T
	{0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},  // L
	{0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0}   // J
};

int arena[A_HEIGHT][A_WIDTH];

uint32_t score = 0;
bool gameOver = false;
int currTetrominoIdx;
int currRotation = 0;
int currX = A_WIDTH / 2;
int currY = 0;

void newTetromino();
bool validPos(int tetromino, int rotation, int posX, int posY);
int rotate(int x, int y, int rotation);
void processInputs();
bool moveDown();
void addToArena();
void checkLines();
void drawArena();

int tetrisMain()
{
	memset(arena, 0, sizeof(arena[0][0]) * A_HEIGHT * A_WIDTH);
	newTetromino();

	const int targetFrameTime = 350;
	uint32_t lastTime = rt_timer_get_ms();

	while (!gameOver)
	{
		uint32_t now = rt_timer_get_ms();
		uint32_t elapsed = (now - lastTime);
		processInputs();

		if (elapsed >= targetFrameTime)
		{
			if (!moveDown())
			{
				addToArena();
				checkLines();
				newTetromino();
			}
			lastTime = now;
		}

		drawArena();
		rt_kernel_sleep(10);
	}

	printf("Game over!\nScore: %d\n", score);
	return 0;
}

void newTetromino()
{
	currTetrominoIdx = rand() % 7;
	currRotation = 0;
	currX = (A_WIDTH / 2) - (T_WIDTH / 2);
	currY = 0;
	gameOver = !validPos(currTetrominoIdx, currRotation, currX, currY);
}

bool validPos(int tetromino, int rotation, int posX, int posY)
{
	for (int x = 0; x < T_WIDTH; x++)
	{
		for (int y = 0; y < T_HEIGHT; y++)
		{
			int index = rotate(x, y, rotation);
			if (1 != tetrominoes[tetromino][index])
				continue;

			int arenaX = x + posX;
			int arenaY = y + posY;
			if (0 > arenaX || A_WIDTH <= arenaX || A_HEIGHT <= arenaY)
				return false;

			int arenaXY = arena[arenaY][arenaX];
			if (arenaY >= 0 && arenaXY != 0)
				return false;
		}
	}
	return true;
}

int rotate(int x, int y, int rotation)
{
	switch (rotation % 4)
	{
	case 0:
		return x + y * T_WIDTH;
	case 1:
		return 12 + y - (x * T_WIDTH);
	case 2:
		return 15 - (y * T_WIDTH) - x;
	case 3:
		return 3 - y + (x * T_WIDTH);
	default:
		return 0;
	}
}

void processInputs()
{
	const uint32_t st = rt_input_get_state();

	static bool rotated = false;
	if (st & (RT_INPUT_BUTTON_A | RT_INPUT_DPAD_N))
	{
		if (!rotated)
		{
			int nextRotation = (currRotation + 1) % 4;
			if (validPos(currTetrominoIdx, nextRotation, currX, currY))
				currRotation = nextRotation;
		}
		rotated = true;
	}
	else
		rotated = false;

	if (st & RT_INPUT_DPAD_W)
	{
		if (validPos(currTetrominoIdx, currRotation, currX - 1, currY))
			currX--;
	}
	if (st & RT_INPUT_DPAD_E)
	{
		if (validPos(currTetrominoIdx, currRotation, currX + 1, currY))
			currX++;
	}
	if (st & RT_INPUT_DPAD_S)
	{
		if (validPos(currTetrominoIdx, currRotation, currX, currY + 1))
			currY++;
	}
}

bool moveDown()
{
	if (validPos(currTetrominoIdx, currRotation, currX, currY + 1))
	{
		currY++;
		return true;
	}
	return false;
}

void addToArena()
{
	for (int y = 0; y < T_HEIGHT; y++)
	{
		for (int x = 0; x < T_WIDTH; x++)
		{
			int index = rotate(x, y, currRotation);
			if (tetrominoes[currTetrominoIdx][index] != 1)
				continue;

			int arenaX = currX + x;
			int arenaY = currY + y;
			bool xInRange = (0 <= arenaX) && (arenaX < A_WIDTH);
			bool yInRange = (0 <= arenaY) && (arenaY < A_HEIGHT);
			if (xInRange && yInRange)
				arena[arenaY][arenaX] = (currTetrominoIdx + 1);
		}
	}
}

void checkLines()
{
	int clearedLines = 0;

	for (int y = A_HEIGHT - 1; y >= 0; y--)
	{
		bool lineFull = true;
		for (int x = 0; x < A_WIDTH; x++)
		{
			if (arena[y][x] == 0)
			{
				lineFull = false;
				break;
			}
		}

		if (!lineFull)
			continue;

		clearedLines++;
		for (int yy = y; yy > 0; yy--)
		{
			for (int xx = 0; xx < A_WIDTH; xx++)
				arena[yy][xx] = arena[yy - 1][xx];
		}

		for (int xx = 0; xx < A_WIDTH; xx++)
			arena[0][xx] = 0;

		y++;
	}

	if (0 < clearedLines)
		score += 100 * clearedLines;
}

void drawArena()
{
	rt_gfx_context_t cx;
	cx.width = 360;
	cx.height = 360;
	cx.pixels = rt_video_get_secondary_target();

	rt_video_clear(0);
	rt_video_wait();

	const int32_t ox = (360 - A_WIDTH * 16) / 2;
	const int32_t oy = (360 - A_HEIGHT * 16) / 2;

	rt_gfx_fill_rect(&cx, ox, oy, A_WIDTH * 16, A_HEIGHT * 16, 8);

	for (int y = 0; y < A_HEIGHT; y++)
	{
		for (int x = 0; x < A_WIDTH; x++)
		{
			int rotatedPos = rotate(x - currX, y - currY, currRotation);
			bool validX = x >= currX && x < currX + T_WIDTH;
			bool validY = y >= currY && y < currY + T_HEIGHT;
			bool xyFilled = 1 == tetrominoes[currTetrominoIdx][rotatedPos];

			if (arena[y][x] != 0)
				rt_gfx_fill_rect(&cx, ox + x * 16 + 1, oy + y * 16 + 1, 14, 14, arena[y][x]);
			if (validX && validY && xyFilled)
				rt_gfx_fill_rect(&cx, ox + x * 16 + 1, oy + y * 16 + 1, 14, 14, currTetrominoIdx + 1);
		}
	}

	rt_video_present(0);

	// printf("%s\n\nScore: %d\n\n", buffer, score);
}
