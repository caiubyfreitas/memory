#include <iostream>
#include <math.h>
#include <time.h>
#include "../external/raylib/raylib.h"

const u_int8_t MAX_ROWS  = 3;
const u_int8_t MAX_COLS  = 3;
constexpr u_int8_t MAX_SEQUENCE = MAX_COLS * MAX_ROWS;
const u_int8_t CELL_HEIGHT = 79;
const u_int8_t CELL_WIDTH = 84;
const u_int8_t CELL_SIZE = 100;
const u_int8_t CELL_PAD_H  = 25;
const u_int8_t CELL_PAD_V  = 10;
const u_int8_t CELL_PAD = 25;
const u_int8_t MAX_LEVEL = 10;
const u_int16_t SCR_WIDTH = 800;
const u_int16_t SCR_HEIGHT = 489;

enum class State { Idle, Title, IsRunning, IsAnimating, IsWaiting, IsOver } ;

State GameState;

typedef struct Cell
{
    u_int8_t id;
    bool selected;
    Rectangle hitbox;
    u_int8_t image_id;
    Texture2D image;
} Cell;

typedef struct Level
{
    u_int8_t difficulty;
    u_int8_t sequence[20];
} Level;

u_int8_t CurrentLevel;
Cell board[MAX_ROWS][MAX_COLS];
Level levels[MAX_LEVEL];

u_int16_t FrameCounter;

Texture2D Overlay, Background, ClickToPlay;
Texture2D TVStaticImage[2];
float AnimTVStaticInterval, AnimTVIdleInterval, AnimTVIdleElapsedTime, AnimTVStaticElapsedTime;
u_int8_t AnimTVStaticFrame;
bool AnimTVStaticActive;
Sound TVStaticSound, GameStartSound;
float GameStartDelay;
bool GameStartSoundOn;

void SetPiecePositions()
{
    // Generates a non-repeatable sequence of random numbers from 1 to 9. 
    u_int8_t positions[10] = {0};
    u_int8_t room = 0;
    SetRandomSeed(time(NULL));
    do
    {
        u_int8_t num = GetRandomValue(1, 9);
        if (positions[num] == 0)
        {
            positions[num] = room;
            room++;
        }
    }
    while(room < 10);
    // Assign each number representing an image to a position in the board
    u_int8_t k = 1;
    for (u_int8_t i=0; i<MAX_COLS; i++)
    {
        for (u_int8_t j=0; j<MAX_ROWS; j++)
        {
            board[i][j].image_id = positions[k++];
            //printf("image: %d \n", board[i][j].image_id);
        }
    }
}

void SetupBoard()
{
    u_int8_t counter = 0;
    float StartCornerX = 190;
    float StartCornerY = 135;
    for (u_int8_t i=0; i<MAX_COLS; i++)
    {
        for (u_int8_t j=0; j<MAX_ROWS; j++)
        {
            board[i][j] = 
            {
                counter,
                false,
                Rectangle
                {
                    StartCornerX + (i*CELL_WIDTH) + (i*CELL_PAD_H), 
                    StartCornerY + (j*CELL_HEIGHT) + (j*CELL_PAD_V), 
                    CELL_WIDTH, 
                    CELL_HEIGHT, 
                },
                0, // image id
                0
            };
            counter++;
        }
    }
}

void SetupDifficultyLevels()
{
    levels[0] = { 3, {0} };
    levels[1] = { 5, {0} };
    levels[2] = { 6, {0} };
    levels[3] = { 8, {0} };
    levels[4] = {10, {0} };
    u_int8_t seed = 0;
    for (u_int8_t i=5; i < MAX_LEVEL; i++)
    {   
        seed = (i % 5 == 0) ? seed + 1 : seed; // increases difficulty after passing 5 consecutive levels
        levels[i].difficulty = levels[i%5].difficulty + seed;
    }
}

void GenerateLevels()
{
    if (GameState == State::Idle)
    {
        SetRandomSeed(time(NULL));
        for (u_int8_t i=0; i < MAX_LEVEL; i++)
        {
            printf("lvl: %i -> ", i);
            for (u_int8_t j=0; j < levels[i].difficulty; j++)
            {
                levels[i].sequence[j] = GetRandomValue(1, MAX_SEQUENCE);
                printf("%d ", levels[i].sequence[j]);
            }
            printf("\n");
        }
        CurrentLevel = 0;
    }
}


void ShowHitBoxes()
{
    for (u_int8_t i=0; i<MAX_COLS; i++)
    {
        for (u_int8_t j=0; j<MAX_ROWS; j++)
        {
            DrawRectangleLines(board[i][j].hitbox.x, board[i][j].hitbox.y, board[i][j].hitbox.width, board[i][j].hitbox.height, RED);
        }
    }
}

void Initialize()
{
    SetupBoard();
    SetPiecePositions();
    SetupDifficultyLevels();
    GenerateLevels();
    InitWindow(SCR_WIDTH, SCR_HEIGHT, "MEMORY GAME");
    GameState = State::Idle;
    CurrentLevel = 1;
    Overlay = LoadTexture("resources/overlay.png");
    Background = LoadTexture("resources/background.png");
    ClickToPlay = LoadTexture("resources/clicktoplay.png");
    // Load images for pieces
    for (u_int8_t i=0; i<MAX_COLS; i++)
    {
        for (u_int8_t j=0; j<MAX_ROWS; j++)
        {
            std::string c = "resources/" + std::to_string(board[i][j].image_id) + ".png";
            printf("%s\n", c.c_str());
            board[i][j].image = LoadTexture(c.c_str());
        }
    }
    TVStaticImage[0] = LoadTextureFromImage(GenImageWhiteNoise(550, 400, 0.49f));
    TVStaticImage[1] = LoadTextureFromImage(GenImageWhiteNoise(550, 400, 0.5f));
    AnimTVStaticInterval = 0.10f;
    AnimTVStaticElapsedTime = 0.0f;
    AnimTVIdleElapsedTime = 0.0f;
    AnimTVIdleInterval = 1.5f;
    AnimTVStaticFrame = 0;
    AnimTVStaticActive = true;
    InitAudioDevice();
    TVStaticSound = LoadSound("resources/whitenoise.wav");
    GameStartSound = LoadSound("resources/GameStart.wav");
    PlaySound(TVStaticSound);
    GameStartSoundOn = false;
    GameStartDelay = 0;
}

void DrawBoard()
{
    for (u_int8_t i=0; i < MAX_ROWS; i++)
    {
        for (u_int8_t j=0; j < MAX_COLS; j++)
        {
            DrawRectangle(board[i][j].hitbox.x,board[i][j].hitbox.y,board[i][j].hitbox.width,board[i][j].hitbox.height, WHITE);
        }
    }
}

void Update()
{
    if (GameState == State::Idle)
    {
        AnimTVStaticElapsedTime += GetFrameTime();
        if (AnimTVStaticElapsedTime > AnimTVStaticInterval)
        {
            AnimTVIdleElapsedTime += AnimTVStaticElapsedTime;
            if (AnimTVIdleElapsedTime > AnimTVIdleInterval)
            {
                AnimTVIdleElapsedTime = 0;
                AnimTVStaticActive = false;
                StopSound(TVStaticSound);
            }
            else 
            {
                AnimTVStaticElapsedTime = 0;
                (AnimTVStaticFrame > 0) ? AnimTVStaticFrame = 0 : AnimTVStaticFrame++;  
            }
        }
    }
    if (GameState == State::Title)
    {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            PlaySound(GameStartSound);
            GameStartSoundOn = true;
        }
    }
    FrameCounter++;
}

void Draw()
{
    BeginDrawing();
    {
        ClearBackground(BLACK);
        DrawTexture(Overlay, 0, 0, WHITE);
        if (GameState == State::Idle)
        {
            if (AnimTVStaticActive)
            {
                DrawTexture(TVStaticImage[AnimTVStaticFrame], 50, 50, WHITE);
            }
            else
            {
                GameState = State::Title;
            }
        }
        if (GameState == State::Title)
        {
            DrawTexture(Background, 50, 50, WHITE);
            if (GameStartSoundOn)
            {
                GameStartDelay += GetFrameTime();
                if (GameStartDelay > 3)
                {
                    GameStartDelay = 0;
                    GameStartSoundOn = false;
                    GameState = State::IsRunning;
                }
                else 
                {
                    if ((FrameCounter/10)%2)
                    {
                        DrawTexture(ClickToPlay, 220, 240, BLACK);
                    }
                }
            }
            else 
            {
                DrawTexture(ClickToPlay, 220, 240, BLACK);
            }
        }
        if (GameState == State::IsRunning)
        {
            DrawTexture(Background, 50, 50, WHITE);
            DrawFPS(95,70);
            ShowHitBoxes();
            //DrawTexture(board[0][0].image, 100, 200, WHITE);
            for (u_int8_t i=0; i < MAX_ROWS; i++)
            {
                for (u_int8_t j=0; j < MAX_COLS; j++)
                {
                    DrawTexture(board[i][j].image, board[i][j].hitbox.x, board[i][j].hitbox.y, WHITE);
                    //printf("%f %f\n", board[i][j].hitbox.x, board[i][j].hitbox.y);
                }
            }
        }
        if (GameState == State::IsOver)
        {
            
        }        
    }
    EndDrawing();
}


void Terminate()
{
    UnloadTexture(Overlay);
    UnloadTexture(TVStaticImage[0]);
    UnloadTexture(TVStaticImage[1]);
    UnloadTexture(Background);
    UnloadTexture(ClickToPlay);
    UnloadSound(TVStaticSound);
    UnloadSound(GameStartSound);
    CloseAudioDevice();
    CloseWindow();
}

int main()
{
    Initialize();
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        Update();
        Draw();
    }
    Terminate();
    return 0;
}