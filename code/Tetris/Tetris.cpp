#include <stdio.h>
#include <string.h>
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
int nextTetrominoIdx;
int currTetrominoIdx;
int currRotation = 0;
int currX = A_WIDTH / 2;
int currY = 0;

// Graphics
rt_gfx_image_t* img_background;
rt_gfx_image_t* img_foreground;

void newTetromino();
bool validPos(int tetromino, int rotation, int posX, int posY);
int rotate(int x, int y, int rotation);
bool processInputs();
bool moveDown();
void addToArena();
void checkLines();
void drawArena();

int tetrisMain()
{
	img_background = rt_gfx_load_image("background.pcx");
	img_foreground = rt_gfx_load_image("foreground.pcx");

	for (int32_t i = 0; i < 256; ++i)
	{
		rt_video_set_palette(i, img_foreground->palette->colors[i].dw);
	}

	for (;;)
	{
		score = 0;
		gameOver = false;
		nextTetrominoIdx = rand() % 7;
		currRotation = 0;
		currX = A_WIDTH / 2;
		currY = 0;

		memset(arena, 0, sizeof(arena[0][0]) * A_HEIGHT * A_WIDTH);
		newTetromino();

		const int targetFrameTime = 350;
		const int targetFrameTimeFast = 50;

		uint32_t lastTime = rt_timer_get_ms();

		while (!gameOver)
		{
			uint32_t now = rt_timer_get_ms();
			uint32_t elapsed = (now - lastTime);
			const bool speedUp = processInputs();

			if (elapsed >= (speedUp ? targetFrameTimeFast : targetFrameTime))
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
		}

		printf("Game over!\nScore: %d\n", score);
	}
	return 0;
}

void newTetromino()
{
	currTetrominoIdx = nextTetrominoIdx;
	nextTetrominoIdx = rand() % 7;
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

bool processInputs()
{
	const uint32_t st = rt_input_get_state();

	static bool rotated = false;
	if (st & RT_INPUT_BUTTON_A)
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

	static uint32_t lastMove = 0;
	const bool haveMove = ((st & (RT_INPUT_DPAD_W | RT_INPUT_DPAD_E)) != 0);
	if (haveMove)
	{
		uint32_t tm = rt_timer_get_ms();
		if ((tm - lastMove) > 200)
		{
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
			lastMove = tm;
		}
	}
	else
		lastMove = 0;

	const bool speedUp = ((st & RT_INPUT_DPAD_S) != 0);
	return speedUp;
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

	// Scrolling background.
	static int x = 0;
	rt_gfx_blit_image(&cx, img_background, x - 360, 0);
	rt_gfx_blit_image(&cx, img_background, x, 0);
	x = (x + 1) % 360;

	const int32_t basePaletteIndex = 128;

	const int32_t ox = (360 - A_WIDTH * 16) / 2;
	const int32_t oy = 28; // (360 - A_HEIGHT * 16) / 2;

	// Background of "next".
	rt_gfx_fill_rect(&cx, 10, oy, T_WIDTH * 16, T_HEIGHT * 16, 0);

	// Play area background.
	int cl = 0;
	for (int x = 0; x < A_WIDTH * 16; x += 16)
	{
		rt_gfx_fill_rect(&cx, ox + x, oy, 16, A_HEIGHT * 16, cl);
		cl = 1 - cl;
	}

	// Tiles.
	for (int y = 0; y < A_HEIGHT; y++)
	{
		for (int x = 0; x < A_WIDTH; x++)
		{
			int rotatedPos = rotate(x - currX, y - currY, currRotation);
			bool validX = x >= currX && x < currX + T_WIDTH;
			bool validY = y >= currY && y < currY + T_HEIGHT;
			bool xyFilled = 1 == tetrominoes[currTetrominoIdx][rotatedPos];

			if (arena[y][x] != 0)
			{
				rt_gfx_fill_rect(&cx, ox + x * 16 + 1, oy + y * 16 + 1, 14, 14, (arena[y][x] - 1) + basePaletteIndex);
				rt_gfx_draw_rect(&cx, ox + x * 16, oy + y * 16, 16, 16, basePaletteIndex + 9);
			}
			if (validX && validY && xyFilled)
			{
				rt_gfx_fill_rect(&cx, ox + x * 16 + 1, oy + y * 16 + 1, 14, 14, currTetrominoIdx + basePaletteIndex);
				rt_gfx_draw_rect(&cx, ox + x * 16, oy + y * 16, 16, 16, basePaletteIndex + 9);
			}
		}
	}

	// Next tetromino.
	for (int y = 0; y < T_HEIGHT; ++y)
	{
		for (int32_t x = 0; x < T_WIDTH; ++x)
		{
			int pos = x + y * T_WIDTH;
			bool xyFilled = 1 == tetrominoes[nextTetrominoIdx][pos];

			if (xyFilled)
				rt_gfx_fill_rect(&cx, 10 + x * 16 + 1, oy + y * 16 + 1, 14, 14, nextTetrominoIdx + basePaletteIndex);
		}
	}

	// Score
	const int sox = (360 - 8 * 24) / 2;
	int denom = 1;
	for (int i = 0; i < 8; ++i)
	{
		int d = (score / denom) % 10;
		denom *= 10;

		rt_gfx_blit_image_region(
			&cx,
			img_foreground,
			d * 24,
			0,
			24,
			24,
			sox + (8 - 1 - i) * 24,
			0
		);
	}

	rt_video_present(1);
}
