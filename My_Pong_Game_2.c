#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h> // for sin() in pulsing colors

// ----------------------------
// Custom types for clarity
// ----------------------------
typedef char i8;
typedef unsigned char u8;
typedef int i32;
typedef float f32;
typedef unsigned char b8;

// ----------------------------
// Constants
// ----------------------------
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 800
#define PADDLE_WIDTH 20
#define PADDLE_HEIGHT 100
#define BALL_SIZE 20
#define PADDLE_SPEED 10
#define NAME_LEN 32
#define MAX_LEVELS 5

// ----------------------------
// Game states
// ----------------------------
typedef enum GameState { MENU, NAME_INPUT, LEVEL_SELECT, PLAYING, PAUSED, GAME_OVER } GameState;

// ----------------------------
// Structures
// ----------------------------
typedef struct Paddle { f32 x, y; int upKey, downKey; } Paddle;
typedef struct Ball { f32 x, y, speedX, speedY; } Ball;
typedef struct Level { i32 targetScore; f32 ballSpeed; } Level;

typedef struct Game {
    Paddle player1, player2;
    Ball ball;
    i32 score1, score2;
    f32 aiSpeed;
    GameState state;
    i32 currentLevel;
    i32 mode; // 0=PvAI,1=PvP,2=Creative
    char player1Name[NAME_LEN];
    char player2Name[NAME_LEN];
} Game;

// ----------------------------
// Level settings
// ----------------------------
Level levels[MAX_LEVELS] = {
    {3, 5.0f}, {5, 6.0f}, {7, 7.0f}, {9, 8.0f}, {11, 9.0f}
};

// Colors for each level
Color levelBg[MAX_LEVELS]     = { DARKGRAY, BLACK, DARKBLUE, DARKPURPLE, DARKGREEN };
Color levelBall[MAX_LEVELS]   = { WHITE, YELLOW, ORANGE, RED, SKYBLUE };
Color levelPaddle[MAX_LEVELS] = { WHITE, GREEN, BLUE, PURPLE, ORANGE };
Color levelText[MAX_LEVELS]   = { WHITE, YELLOW, ORANGE, RED, SKYBLUE };

// ----------------------------
// Function prototypes
// ----------------------------
void InitGame(Game *g, i32 mode);
void MovePaddle(Paddle *p);
void AIPaddle(Paddle *p, Ball b, f32 aiSpeed);
b8 CheckPaddleCollision(Ball *b,Paddle p);
void ResetBall(Ball *b,f32 speed);
void InputName(char *name,i32 maxLen,const char *prompt,Font font);
void DrawGame(Game g, Font font);
void UpdateGame(Game *g);
void DrawGameOver(Game *g, Font font);
void DrawPause(Game g, Font font);
void DrawLevelSelect(Game *g, Font font);

// ----------------------------
// Pulse color helper
// ----------------------------
u8 pulseColor(f32 t, u8 base){
    return (u8)(base + 50 * sin(t*5));
}

// ----------------------------
// Initialize game
// ----------------------------
void InitGame(Game *g, i32 mode){
    g->player1.x = 0; 
    g->player1.y = SCREEN_HEIGHT/2 - PADDLE_HEIGHT/2;
    g->player2.x = SCREEN_WIDTH - PADDLE_WIDTH; 
    g->player2.y = SCREEN_HEIGHT/2 - PADDLE_HEIGHT/2;

    g->player1.upKey = KEY_W;
    g->player1.downKey = KEY_S;
    g->player2.upKey = KEY_UP;
    g->player2.downKey = KEY_DOWN;

    g->score1 = g->score2 = 0;
    g->aiSpeed = PADDLE_SPEED*0.8f;
    g->state = NAME_INPUT;
    g->currentLevel = 0;
    g->mode = mode;

    g->ball.x = SCREEN_WIDTH/2 - BALL_SIZE/2;
    g->ball.y = SCREEN_HEIGHT/2 - BALL_SIZE/2;
    g->ball.speedX = levels[0].ballSpeed * ((rand()%2)?1:-1);
    g->ball.speedY = levels[0].ballSpeed * ((rand()%2)?1:-1);

    strcpy(g->player1Name,"");
    strcpy(g->player2Name,"");
}

// ----------------------------
// Move player paddle with assigned keys
// ----------------------------
void MovePaddle(Paddle *p){
    if(IsKeyDown(p->upKey) && p->y > 0) p->y -= PADDLE_SPEED;
    if(IsKeyDown(p->downKey) && p->y < SCREEN_HEIGHT - PADDLE_HEIGHT) p->y += PADDLE_SPEED;
}

// ----------------------------
// Simple AI movement
// ----------------------------
void AIPaddle(Paddle *p, Ball b, f32 aiSpeed){
    f32 center = p->y + PADDLE_HEIGHT/2;
    if(center < b.y + BALL_SIZE/2 - 15 && p->y < SCREEN_HEIGHT-PADDLE_HEIGHT) p->y += aiSpeed * ((rand()%2)?1:0.7f); // slight imperfection
    if(center > b.y + BALL_SIZE/2 + 15 && p->y > 0) p->y -= aiSpeed * ((rand()%2)?1:0.7f);
}

// ----------------------------
// Check ball/paddle collision
// ----------------------------
b8 CheckPaddleCollision(Ball *b,Paddle p){
    return (b->x < p.x+PADDLE_WIDTH && b->x+BALL_SIZE > p.x &&
            b->y+BALL_SIZE > p.y && b->y < p.y+PADDLE_HEIGHT);
}

// ----------------------------
// Reset ball to center
// ----------------------------
void ResetBall(Ball *b,f32 speed){
    b->x = SCREEN_WIDTH/2 - BALL_SIZE/2;
    b->y = SCREEN_HEIGHT/2 - BALL_SIZE/2;
    b->speedX = speed * ((rand()%2)?1:-1);
    b->speedY = speed * ((rand()%2)?1:-1);
}

// ----------------------------
// Input player name
// ----------------------------
void InputName(char *name,i32 maxLen,const char *prompt,Font font){
    i32 len = strlen(name);
    b8 finished = 0;
    while(!finished){
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTextEx(font,prompt,(Vector2){SCREEN_WIDTH/2-150, SCREEN_HEIGHT/2-50},24,1,WHITE);
        DrawTextEx(font,name,(Vector2){SCREEN_WIDTH/2-100, SCREEN_HEIGHT/2},24,1,YELLOW);
        EndDrawing();

        int key = GetCharPressed();
        while(key > 0){
            if((key>=32 && key<=125) && len<maxLen-1){ name[len]=(char)key; len++; name[len]='\0'; }
            key = GetCharPressed();
        }

        if(IsKeyPressed(KEY_BACKSPACE) && len>0){ len--; name[len]='\0'; }
        if(IsKeyPressed(KEY_ENTER) && len>0) finished = 1;
        if(WindowShouldClose()) exit(0);
    }
}

// ----------------------------
// Draw level select menu
// ----------------------------
void DrawLevelSelect(Game *g, Font font){
    BeginDrawing();
    ClearBackground(BLACK);
    DrawTextEx(font,"Select Level",(Vector2){SCREEN_WIDTH/2-80,100},28,1,WHITE);

    for(int i=0;i<MAX_LEVELS;i++){
        DrawTextEx(font,TextFormat("%d: Level %d (Target %d)",i+1,i+1,levels[i].targetScore),
                   (Vector2){SCREEN_WIDTH/2-100,180+i*35},20,1,YELLOW);
    }
    EndDrawing();

    for(int i=0;i<MAX_LEVELS;i++){
        if(IsKeyPressed(KEY_ONE+i)){ g->currentLevel=i; g->state=PLAYING; ResetBall(&g->ball,levels[g->currentLevel].ballSpeed); g->score1=g->score2=0; }
    }
}

// ----------------------------
// Draw pause screen
// ----------------------------
void DrawPause(Game g, Font font){
    BeginDrawing();
    ClearBackground(BLACK);
    DrawTextEx(font,"PAUSED",(Vector2){SCREEN_WIDTH/2-50,SCREEN_HEIGHT/2-60},28,1,WHITE);
    DrawTextEx(font,"Press ENTER to continue",(Vector2){SCREEN_WIDTH/2-100,SCREEN_HEIGHT/2-30},20,1,YELLOW);
    DrawTextEx(font,"Press Q to quit to menu",(Vector2){SCREEN_WIDTH/2-100,SCREEN_HEIGHT/2},20,1,YELLOW);
    EndDrawing();
}

// ----------------------------
// Draw playing field
// ----------------------------
void DrawGame(Game g, Font font){
    f32 t = GetTime();

    Color bg = levelBg[g.currentLevel];
    Color ballC = (Color){ pulseColor(t,levelBall[g.currentLevel].r),
                           pulseColor(t,levelBall[g.currentLevel].g),
                           pulseColor(t,levelBall[g.currentLevel].b), 255 };
    Color padC = (Color){ pulseColor(t,levelPaddle[g.currentLevel].r),
                          pulseColor(t,levelPaddle[g.currentLevel].g),
                          pulseColor(t,levelPaddle[g.currentLevel].b),255};
    Color textC = levelText[g.currentLevel];

    BeginDrawing();
    ClearBackground(bg);

    // Middle dotted line
    for(int y=0;y<SCREEN_HEIGHT;y+=20) DrawRectangle(SCREEN_WIDTH/2-2,y,4,10,WHITE);

    // Draw paddles
    DrawRectangle((int)g.player1.x,(int)g.player1.y,PADDLE_WIDTH,PADDLE_HEIGHT,padC);
    DrawRectangle((int)g.player2.x,(int)g.player2.y,PADDLE_WIDTH,PADDLE_HEIGHT,padC);

    // Draw ball
    DrawRectangle((int)g.ball.x,(int)g.ball.y,BALL_SIZE,BALL_SIZE,ballC);

    // Scores
    DrawTextEx(font,TextFormat("%d",g.score1),(Vector2){SCREEN_WIDTH/4,20},28,1,textC);
    DrawTextEx(font,TextFormat("%d",g.score2),(Vector2){SCREEN_WIDTH*3/4,20},28,1,textC);
    EndDrawing();
}

// ----------------------------
// Update game logic
// ----------------------------
void UpdateGame(Game *g){
    if(g->state!=PLAYING) return;

    MovePaddle(&g->player1);
    if(g->mode==1) MovePaddle(&g->player2);
    else if(g->mode==0 || g->mode==2) AIPaddle(&g->player2,g->ball,g->aiSpeed);

    g->ball.x += g->ball.speedX;
    g->ball.y += g->ball.speedY;

    if(g->ball.y <= 0 || g->ball.y >= SCREEN_HEIGHT-BALL_SIZE) g->ball.speedY*=-1;
    if(CheckPaddleCollision(&g->ball,g->player1) || CheckPaddleCollision(&g->ball,g->player2)) g->ball.speedX*=-1;

    if(g->ball.x<0){ g->score2++; ResetBall(&g->ball,levels[g->currentLevel].ballSpeed); }
    if(g->ball.x>SCREEN_WIDTH){ g->score1++; ResetBall(&g->ball,levels[g->currentLevel].ballSpeed); }

    if(g->ball.speedX>0 && g->mode!=1) g->aiSpeed+=0.01f;

    if(g->score1>=levels[g->currentLevel].targetScore || g->score2>=levels[g->currentLevel].targetScore)
        g->state = GAME_OVER;

    if(IsKeyPressed(KEY_P)) g->state = PAUSED;
}

// ----------------------------
// Draw game over menu
// ----------------------------
void DrawGameOver(Game *g, Font font){
    BeginDrawing();
    ClearBackground(BLACK);
    DrawTextEx(font,"GAME OVER",(Vector2){SCREEN_WIDTH/2-80,SCREEN_HEIGHT/2-60},28,1,WHITE);
    DrawTextEx(font,"Press 1: Retry, 2: Menu",(Vector2){SCREEN_WIDTH/2-100,SCREEN_HEIGHT/2},20,1,YELLOW);
    EndDrawing();

    if(IsKeyPressed(KEY_ONE)){ g->state = LEVEL_SELECT; }
    if(IsKeyPressed(KEY_TWO)){ g->state = MENU; }
}

// ----------------------------
// Main
// ----------------------------
int main(){
    srand(time(NULL));
    InitWindow(SCREEN_WIDTH,SCREEN_HEIGHT,"Pong Bros");
    SetTargetFPS(60);

    // Load font
    char fontPath[256];
    sprintf(fontPath,"%s/fonts/PressStart2P.ttf",getenv("PWD"));
    Font arcadeFont = LoadFont(fontPath);
    if(arcadeFont.texture.id==0){ printf("Font not found!\n"); return -1; }

    Game pong;
    pong.state = MENU;

    while(!WindowShouldClose()){
        switch(pong.state){
            case MENU:
                BeginDrawing();
                ClearBackground(BLACK);
                DrawTextEx(arcadeFont,"PONG BROS",(Vector2){SCREEN_WIDTH/2-100,100},28,1,WHITE);
                DrawTextEx(arcadeFont,"1: Player vs AI",(Vector2){SCREEN_WIDTH/2-100,200},24,1,YELLOW);
                DrawTextEx(arcadeFont,"2: Player vs Player",(Vector2){SCREEN_WIDTH/2-100,250},24,1,YELLOW);
                DrawTextEx(arcadeFont,"3: Creative Mode",(Vector2){SCREEN_WIDTH/2-100,300},24,1,YELLOW);
                EndDrawing();

                if(IsKeyPressed(KEY_ONE)){ InitGame(&pong,0); InputName(pong.player1Name,NAME_LEN,"Enter Player 1 Name:",arcadeFont); pong.state=LEVEL_SELECT; }
                if(IsKeyPressed(KEY_TWO)){ InitGame(&pong,1); InputName(pong.player1Name,NAME_LEN,"Player 1 Name:",arcadeFont); InputName(pong.player2Name,NAME_LEN,"Player 2 Name:",arcadeFont); pong.state=LEVEL_SELECT; }
                if(IsKeyPressed(KEY_THREE)){ InitGame(&pong,2); InputName(pong.player1Name,NAME_LEN,"Enter Player Name:",arcadeFont); pong.state=LEVEL_SELECT; }
                break;

            case LEVEL_SELECT: DrawLevelSelect(&pong, arcadeFont); break;
            case PLAYING: UpdateGame(&pong); DrawGame(pong,arcadeFont); break;
            case PAUSED:
                DrawPause(pong,arcadeFont);
                if(IsKeyPressed(KEY_ENTER)) pong.state = PLAYING;
                if(IsKeyPressed(KEY_Q)) pong.state = MENU;
                break;
            case GAME_OVER: DrawGameOver(&pong,arcadeFont); break;
            default: break;
        }
    }

    UnloadFont(arcadeFont);
    CloseWindow();
    return 0;
}
