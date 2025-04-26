#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <stdbool.h>
#include <time.h>

// Game constants
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define TILE_SIZE 32
#define MAX_PLAYERS 4
#define MAX_ENEMIES 8
#define MAP_WIDTH 80
#define MAP_HEIGHT 80
#define MIN_PLAY_TIME_SEC 300 // 5 minutes minimum play time

// Game tile types
typedef enum {
    TILE_EMPTY = 0,
    TILE_WALL,
    TILE_DOOR,
    TILE_TREASURE,
    TILE_EXIT,
    TILE_KEY        // New tile type: key required to exit
} TileType;

// Player/enemy types
typedef enum {
    ENTITY_PLAYER = 0,
    ENTITY_ENEMY_CHASE,    // Chases the player
    ENTITY_ENEMY_RANDOM,   // Moves randomly
    ENTITY_ENEMY_GUARD,    // Guards treasures
    ENTITY_ENEMY_SMART     // Tries to block player's path to exit
} EntityType;

// Player structure
typedef struct {
    int id;
    int x;
    int y;
    int health;
    int score;
    bool is_active;
    EntityType type;       // What type of entity this is
    int keys;              // Number of keys collected
} Player;

// Game map structure
typedef struct {
    TileType tiles[MAP_HEIGHT][MAP_WIDTH];
    int width;
    int height;
} GameMap;

// Game state structure (to be stored in shared memory)
typedef struct {
    GameMap map;
    Player players[MAX_PLAYERS];  // Player[0] is the human player
    Player enemies[MAX_ENEMIES];  // AI-controlled enemies
    int num_players;
    int num_enemies;
    bool game_over;
    int winner_id;
    bool player_hit;       // Flag to indicate player was hit by enemy
    time_t start_time;     // Game start time
    time_t current_time;   // Current game time
    bool exit_enabled;     // Whether exit is enabled yet
    int keys_required;     // Number of keys needed to enable exit
    int keys_collected;    // Number of keys collected so far
} GameState;

// Message structure for IPC
typedef struct {
    int from_id;           // Sender ID
    int to_id;             // Recipient ID (-1 for broadcast)
    int x;                 // X position 
    int y;                 // Y position
    int message_type;      // Type of message
    int data;              // Additional data
} GameMessage;

// Message types
#define MSG_POSITION_UPDATE 1
#define MSG_ENEMY_MOVE 2
#define MSG_PLAYER_HIT 3
#define MSG_GAME_OVER 4
#define MSG_KEY_COLLECTED 5

// Function declarations
bool game_init(void);
void game_cleanup(void);
void render_game(SDL_Renderer *renderer, GameState *state, int player_id);
void update_player(GameState *state, int player_id, int dx, int dy);
bool is_valid_move(GameState *state, int player_id, int dx, int dy);
void process_player_input(SDL_Event *event, GameState *state, int player_id);
void enemy_ai_process(int enemy_id);
void check_player_enemy_collision(GameState *state);
void update_game_time(GameState *state);
bool check_exit_criteria(GameState *state);
void render_game_ui(SDL_Renderer *renderer, GameState *state);
void render_digit(SDL_Renderer *renderer, int x, int y, int digit);

#endif /* GAME_H */ 