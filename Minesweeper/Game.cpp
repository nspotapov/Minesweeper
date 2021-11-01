#include "framework.h"
#include "Game.h"

#include <stdlib.h>
#include <stdio.h>

#define MINES_COUNT 24

#define COLOR_UNEXPLORED RGB(0, 128, 128)
#define COLOR_OPENED RGB(238, 238, 236)
#define COLOR_MINE_HIT RGB(247, 64, 64)

#define VIEW_CELL_UNEXPLORED 0
#define VIEW_CELL_OPENED 1
#define VIEW_CELL_MINE 2
#define VIEW_CELL_MINE_HIT 3
#define VIEW_CELL_FLAG 4

#define GRID_ROWS_COUNT 16
#define GRID_COLUMNS_COUNT 16

int rows_count = GRID_ROWS_COUNT;
int cols_count = GRID_COLUMNS_COUNT;

static int GAME_CELL_FREE = 0;
static int GAME_CELL_MINE = 1;

static int GAME_GRID_X = 5;
static int GAME_GRID_Y = 5;

int gameField[GRID_ROWS_COUNT][GRID_COLUMNS_COUNT] = { 0 };
int viewField[GRID_ROWS_COUNT][GRID_COLUMNS_COUNT] = { 0 };

int lightI = -1;
int lightJ = -1;

int isActual = 0;

int isFirstClick = 1;

int flagsCount = MINES_COUNT;

unsigned int timer = 0;

int inGame = 1;

int isWin = 0;

static HPEN hPenBlack1 = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
static HBRUSH hBrushColorUnexplored = CreateSolidBrush(COLOR_UNEXPLORED);
static HBRUSH hBrushColorOpened = CreateSolidBrush(COLOR_OPENED);
static HBRUSH hBrushColorMineBackground = CreateSolidBrush(RGB(96, 96, 96));
static HBRUSH hBrushColorMineHit = CreateSolidBrush(COLOR_MINE_HIT);
static HBRUSH hBrushColorWhite = CreateSolidBrush(RGB(255, 255, 255));
static HBRUSH hBrushColorRed = CreateSolidBrush(RGB(255, 0, 0));
static HPEN hPenBlack2 = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));

static int MIN_CELL_SIZE = 20;


void drawLine(HDC hdc, int x1, int y1, int x2, int y2)
{
    MoveToEx(hdc, x1, y1, NULL);
    LineTo(hdc, x2, y2);
}

int getMinesAroundCount(int i, int j)
{
    int variants[] = { -1, 0, 1 };
    int mines = 0;
    for (int m = 0; m < 3; m++)
    {
        for (int n = 0; n < 3; n++)
        {
            if (!(m == 1 && n == 1) &&
                i + variants[m] >= 0 && i + variants[m] < GRID_ROWS_COUNT &&
                j + variants[n] >= 0 && j + variants[n] < GRID_COLUMNS_COUNT)
            {
                mines += gameField[i + variants[m]][j + variants[n]];
            }
        }
    }
    return mines;
}

void drawMinesweeperFrame(HWND hWnd, HDC hdc, int x, int y)
{
    drawMinesweeperGrid(hdc, x, y);
}

void recursiveOpenCell(int i, int j)
{
    if (viewField[i][j] == VIEW_CELL_OPENED) return;
    if (viewField[i][j] == VIEW_CELL_FLAG) return;
    if (gameField[i][j] == GAME_CELL_MINE) return;

    viewField[i][j] = VIEW_CELL_OPENED;
    if (getMinesAroundCount(i, j) == 0)
    {
        int dx, dy;
        int v[3] = { -1, 0, 1 };
        for (int m = 0; m < 3; m++)
        {
            for (int n = 0; n < 3; n++)
            {
                dx = v[m];
                dy = v[n];

                if ((dx != 0) && (dy != 0)) continue;
                if (isCordsGood(i + dy, j + dx))
                    recursiveOpenCell(i + dy, j + dx);
            }
        }
    }
}

int isCordsGood(int i, int j)
{
    return (((i >= 0) && (i < GRID_ROWS_COUNT)) && ((j >= 0) && (j < GRID_COLUMNS_COUNT)));
}

void drawMinesweeperStatistics(HDC hdc, int x, int y)
{
    TCHAR str1[] = _T("��� ��������: ");
    TextOut(hdc, x, y, str1, _tcsclen(str1));

    TCHAR str2[] = _T("������� ������: ");
    TextOut(hdc, x, y + 25, str2, _tcsclen(str2));

    if (isWin)
    {
        TCHAR str3[] = _T("������!");
        TextOut(hdc, x + 300, y, str3, _tcsclen(str3));
    }

    char text[5];
    TCHAR textOut[5];

    sprintf_s(text, "%d", flagsCount);
    OemToChar(text, textOut);
    TextOut(hdc, x + 150, y, textOut, _tcsclen(textOut));

    char timerText[10];
    TCHAR timerTextOut[10];

    int temp = timer;

    int seconds = temp % 60;
    temp /= 60;
    int minutes = temp % 60;
    temp /= 60;
    int hours = temp;
    if (hours > 0)
    {
        sprintf_s(timerText, "%d:%d:%d", hours, minutes, seconds);
    }
    else
    {
        sprintf_s(timerText, "%d:%d", minutes, seconds);
    }

    OemToChar(timerText, timerTextOut);
    TextOut(hdc, x + 170, y + 25, timerTextOut, _tcsclen(timerTextOut));
}

void drawMinesweeperGrid(HDC hdc, int x, int y)
{

    for (int i = 0; i < GRID_ROWS_COUNT; i++)
    {
        for (int j = 0; j < GRID_COLUMNS_COUNT; j++)
        {
            drawMinesweeperCell(hdc, x, y, i, j);
        }
    }

}

void drawMinesweeperCell(HDC hdc, int x, int y, int i, int j)
{
    int cellState = viewField[i][j];

    if (cellState == VIEW_CELL_UNEXPLORED) drawUnexplored(hdc, x, y, i, j);
    if (cellState == VIEW_CELL_OPENED) drawOpened(hdc, x, y, i, j);
    if (cellState == VIEW_CELL_MINE) drawMine(hdc, x, y, i, j);
    if (cellState == VIEW_CELL_MINE_HIT) drawMineHit(hdc, x, y, i, j);
    if (cellState == VIEW_CELL_FLAG) drawFlag(hdc, x, y, i, j);

    if (lightI == i && lightJ == j &&
        (viewField[i][j] == VIEW_CELL_UNEXPLORED ||
            viewField[i][j] == VIEW_CELL_FLAG)) lightCell(hdc, i, j);
}

void drawUnexplored(HDC hdc, int x, int y, int i, int j)
{
    SelectObject(hdc, hPenBlack1);

    SelectObject(hdc, hBrushColorUnexplored);
    Rectangle(hdc, x + j * MIN_CELL_SIZE, y + i * MIN_CELL_SIZE,
        x + (j + 1) * MIN_CELL_SIZE - 1, y + (i + 1) * MIN_CELL_SIZE - 1);
}

void drawOpened(HDC hdc, int x, int y, int i, int j)
{
    SelectObject(hdc, GetStockObject(NULL_PEN));

    SelectObject(hdc, hBrushColorOpened);
    Rectangle(hdc, x + j * MIN_CELL_SIZE, y + i * MIN_CELL_SIZE,
        x + (j + 1) * MIN_CELL_SIZE - 1, y + (i + 1) * MIN_CELL_SIZE - 1);


    int minesAroundCount = getMinesAroundCount(i, j);

    if (minesAroundCount == 0) return;

    char minesAroundText[2];

    sprintf_s(minesAroundText, "%d", minesAroundCount);
    TCHAR minesAround[2];
    OemToChar(minesAroundText, minesAround);
    TextOut(hdc, x + j * MIN_CELL_SIZE + 5, y + i * MIN_CELL_SIZE,
        minesAround, _tcsclen(minesAround));

}

void drawMine(HDC hdc, int x, int y, int i, int j)
{
    drawOpened(hdc, x, y, i, j);

    int sx, sy;
    sx = j * MIN_CELL_SIZE + x;
    sy = i * MIN_CELL_SIZE + y;

    SelectObject(hdc, hPenBlack1);


    SelectObject(hdc, hBrushColorMineBackground);

    drawLine(hdc, sx + 1, sy + 1, sx + 18, sy + 18);
    drawLine(hdc, sx + 1, sy + 18, sx + 18, sy + 1);
    drawLine(hdc, sx + 9, sy + 1, sx + 10, sy + 18);
    drawLine(hdc, sx + 1, sy + 10, sx + 18, sy + 9);

    Ellipse(hdc, sx + 4, sy + 4, sx + 15, sy + 15);
}

void drawMineHit(HDC hdc, int x, int y, int i, int j)
{
    SelectObject(hdc, GetStockObject(NULL_PEN));

    SelectObject(hdc, hBrushColorMineHit);

    Rectangle(hdc, x + j * MIN_CELL_SIZE, y + i * MIN_CELL_SIZE,
        x + (j + 1) * MIN_CELL_SIZE, y + (i + 1) * MIN_CELL_SIZE);


    int sx, sy;
    sx = j * MIN_CELL_SIZE + x;
    sy = i * MIN_CELL_SIZE + y;

    SelectObject(hdc, hPenBlack1);

    SelectObject(hdc, hBrushColorMineBackground);

    drawLine(hdc, sx + 1, sy + 1, sx + 18, sy + 18);
    drawLine(hdc, sx + 1, sy + 18, sx + 18, sy + 1);
    drawLine(hdc, sx + 9, sy + 1, sx + 10, sy + 18);
    drawLine(hdc, sx + 1, sy + 10, sx + 18, sy + 9);

    Ellipse(hdc, sx + 4, sy + 4, sx + 15, sy + 15);
}

void drawFlag(HDC hdc, int x, int y, int i, int j)
{
    int sx, sy;
    sx = j * MIN_CELL_SIZE + x;
    sy = i * MIN_CELL_SIZE + y;

    drawUnexplored(hdc, x, y, i, j);
    SelectObject(hdc, GetStockObject(NULL_PEN));

    SelectObject(hdc, hBrushColorWhite);

    Rectangle(hdc, sx + 2, sy + 1, sx + 5, sy + 18);
    Rectangle(hdc, sx + 2, sy + 2, sx + 8, sy + 10);
    Rectangle(hdc, sx + 2, sy + 3, sx + 12, sy + 9);
    Rectangle(hdc, sx + 2, sy + 4, sx + 15, sy + 8);
    Rectangle(hdc, sx + 2, sy + 5, sx + 16, sy + 7);


    SelectObject(hdc, hBrushColorRed);

    drawLine(hdc, sx + 3, sy + 2, sx + 3, sy + 17);
    drawLine(hdc, sx + 4, sy + 2, sx + 4, sy + 17);

    Rectangle(hdc, sx + 3, sy + 3, sx + 7, sy + 9);
    Rectangle(hdc, sx + 3, sy + 4, sx + 11, sy + 8);
    Rectangle(hdc, sx + 3, sy + 5, sx + 14, sy + 7);
    Rectangle(hdc, sx + 3, sy + 6, sx + 15, sy + 6);


}

void lightCell(HDC hdc, int i, int j)
{
    SelectObject(hdc, hPenBlack2);

    SelectObject(hdc, GetStockObject(NULL_BRUSH));

    Rectangle(hdc, GAME_GRID_X + j * MIN_CELL_SIZE,
        GAME_GRID_Y + i * MIN_CELL_SIZE,
        GAME_GRID_X + (j + 1) * MIN_CELL_SIZE,
        GAME_GRID_Y + (i + 1) * MIN_CELL_SIZE);
}

void fillMines(int i, int j)
{
    int mines = MINES_COUNT;
    while (mines > 0)
    {
        int ri = rand() % GRID_ROWS_COUNT;
        int rj = rand() % GRID_COLUMNS_COUNT;

        if (ri == i && rj == j) continue;
        if (gameField[ri][rj] == GAME_CELL_MINE) continue;

        gameField[ri][rj] = GAME_CELL_MINE;
        mines--;
    }
}

void openCell(int i, int j)
{
    if (!inGame) return;

    if (viewField[i][j] == VIEW_CELL_FLAG) return;
    if (viewField[i][j] != VIEW_CELL_UNEXPLORED) return;

    if (gameField[i][j] == GAME_CELL_FREE)
        recursiveOpenCell(i, j);
    else
    {
        viewField[i][j] = VIEW_CELL_MINE_HIT;
        showAllMines();
        inGame = 0;
    }
}

void markedCell(int i, int j)
{
    if (viewField[i][j] == VIEW_CELL_UNEXPLORED && flagsCount > 0)
    {
        viewField[i][j] = VIEW_CELL_FLAG;
        flagsCount--;
    }
    else if (viewField[i][j] == VIEW_CELL_FLAG)
    {
        viewField[i][j] = VIEW_CELL_UNEXPLORED;
        flagsCount++;
    }

}

void showAllMines()
{
    for (int i = 0; i < GRID_ROWS_COUNT; i++)
    {
        for (int j = 0; j < GRID_COLUMNS_COUNT; j++)
        {
            if (gameField[i][j] == GAME_CELL_MINE && viewField[i][j] != VIEW_CELL_MINE_HIT)
                viewField[i][j] = VIEW_CELL_MINE;
        }
    }
}

int checkWin()
{
    if (isFirstClick)
        return 0;
    for (int i = 0; i < GRID_ROWS_COUNT; i++)
    {
        for (int j = 0; j < GRID_COLUMNS_COUNT; j++)
        {
            if (gameField[i][j] == GAME_CELL_MINE && viewField[i][j] != VIEW_CELL_FLAG)
            {
                return 0;
            }
        }
    }
    return 1;
}

void restartGame()
{
    gameField[GRID_ROWS_COUNT][GRID_COLUMNS_COUNT] = { 0 };
    viewField[GRID_ROWS_COUNT][GRID_COLUMNS_COUNT] = { 0 };

    lightI = -1;
    lightJ = -1;

    isActual = 0;

    isFirstClick = 1;

    flagsCount = MINES_COUNT;

    timer = 0;

    inGame = 1;

    isWin = 0;

    clearField();
}

void clearField()
{
    for (int i = 0; i < GRID_ROWS_COUNT; i++)
    {
        for (int j = 0; j < GRID_COLUMNS_COUNT; j++)
        {
            gameField[i][j] = GAME_CELL_FREE;
            viewField[i][j] = VIEW_CELL_UNEXPLORED;
        }
    }
}
