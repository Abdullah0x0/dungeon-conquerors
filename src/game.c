#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <pthread.h>
#include <math.h>
#include "../include/game.h"
#include "../include/shared_memory.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Global variables
SDL_Texture* tile_textures[5] = {NULL};  // Textures for different tile types
SDL_Texture* player_textures[MAX_PLAYERS] = {NULL};  // Textures for players

// Thread for background events
pthread_t background_thread;
bool background_thread_running = false;

// Thread data structure
typedef struct {
    int interval_ms;  // Interval between events in milliseconds
} ThreadData;

// Colors for different tile types
SDL_Color tile_colors[6] = {
    {100, 100, 100, 255},  // TILE_EMPTY
    {50, 50, 50, 255},     // TILE_WALL
    {150, 100, 50, 255},   // TILE_DOOR
    {255, 215, 0, 255},    // TILE_TREASURE
    {0, 255, 0, 255},      // TILE_EXIT
    {0, 191, 255, 255}     // TILE_KEY (Bright blue)
};

// Colors for different players and enemies
SDL_Color player_colors[MAX_PLAYERS] = {
    {255, 0, 0, 255},    // Player 1 (Red)
    {0, 0, 255, 255},    // Player 2 (Blue)
    {0, 255, 0, 255},    // Player 3 (Green)
    {255, 255, 0, 255}   // Player 4 (Yellow)
};

// Colors for different enemy types
SDL_Color enemy_colors[4] = {
    {255, 0, 255, 255},  // Chaser (Purple)
    {255, 165, 0, 255},  // Random (Orange)
    {0, 255, 255, 255},  // Guard (Cyan)
    {255, 255, 0, 255}   // Smart (Yellow)
};

// Thread function for background events
void* background_event_thread(void* data) {
    ThreadData* thread_data = (ThreadData*)data;
    
    while (background_thread_running) {
        // Add random treasure periodically
        lock_game_state();
        
        if (game_state != NULL && !game_state->game_over) {
            // Update game time
            game_state->current_time = time(NULL);
            
            // Enable exit based on key collection only, no time requirement
            if (game_state->keys_collected >= game_state->keys_required) {
                game_state->exit_enabled = true;
            }
            
            // 3% chance to add a new treasure
            if (rand() % 100 < 3) {
                int x = rand() % (game_state->map.width - 2) + 1;
                int y = rand() % (game_state->map.height - 2) + 1;
                
                if (game_state->map.tiles[y][x] == TILE_EMPTY) {
                    game_state->map.tiles[y][x] = TILE_TREASURE;
                }
            }
            
            // Check for game end conditions
            bool all_inactive = true;
            int highest_score = -1;
            int winner_id = -1;
            
            for (int i = 0; i < game_state->num_players; i++) {
                if (game_state->players[i].is_active) {
                    all_inactive = false;
                }
                
                if (game_state->players[i].score > highest_score) {
                    highest_score = game_state->players[i].score;
                    winner_id = i;
                }
            }
            
            // If all players are inactive or someone reached the exit, end the game
            if (all_inactive) {
                game_state->game_over = true;
                game_state->winner_id = winner_id;
            }
        }
        
        unlock_game_state();
        
        // Sleep for the specified interval
        usleep(thread_data->interval_ms * 1000);
    }
    
    free(thread_data);
    return NULL;
}

// Initialize game resources
bool game_init(void) {
    // Seed random number generator
    srand(time(NULL));
    
    // Start background event thread
    background_thread_running = true;
    ThreadData* thread_data = malloc(sizeof(ThreadData));
    if (thread_data == NULL) {
        return false;
    }
    
    thread_data->interval_ms = 500;  // 500ms interval
    
    if (pthread_create(&background_thread, NULL, background_event_thread, thread_data) != 0) {
        free(thread_data);
        return false;
    }
    
    return true;
}

// Clean up game resources
void game_cleanup(void) {
    printf("Stopping background thread...\n");
    // Stop background thread
    if (background_thread_running) {
        background_thread_running = false;
        
        // Give the thread time to exit gracefully
        usleep(100000); // 100ms
        
        // Join the thread
        int join_result = pthread_join(background_thread, NULL);
        if (join_result != 0) {
            printf("Warning: Failed to join background thread: %d\n", join_result);
        } else {
            printf("Background thread joined successfully\n");
        }
    }
    
    // Free any other game resources if needed
    printf("Game cleanup completed\n");
}

// Render the game state
void render_game(SDL_Renderer* renderer, GameState* state, int player_id) {
    if (!state || !renderer) {
        return;
    }
    
    Player* player = &state->players[player_id];
    
    // Center view on player
    int center_x = player->x;
    int center_y = player->y;
    
    // Visible area
    int visible_width = WINDOW_WIDTH / TILE_SIZE;
    int visible_height = (WINDOW_HEIGHT - 40) / TILE_SIZE; // Account for UI space
    
    int start_x = center_x - visible_width / 2;
    int start_y = center_y - visible_height / 2;
    
    // Ensure start coordinates are within bounds
    if (start_x < 0) start_x = 0;
    if (start_y < 0) start_y = 0;
    if (start_x + visible_width >= state->map.width) start_x = state->map.width - visible_width;
    if (start_y + visible_height >= state->map.height) start_y = state->map.height - visible_height;
    
    // Render map tiles
    for (int y = 0; y < visible_height; y++) {
        for (int x = 0; x < visible_width; x++) {
            int map_x = start_x + x;
            int map_y = start_y + y;
            
            if (map_x >= 0 && map_x < state->map.width && map_y >= 0 && map_y < state->map.height) {
                TileType tile = state->map.tiles[map_y][map_x];
                
                // If it's an exit and not enabled, render it differently
                if (tile == TILE_EXIT && !state->exit_enabled) {
                    // Render as inactive exit (darker green)
                    SDL_SetRenderDrawColor(renderer, 0, 100, 0, 255);
                } else {
                    // Set color based on tile type
                    SDL_SetRenderDrawColor(
                        renderer,
                        tile_colors[tile].r,
                        tile_colors[tile].g,
                        tile_colors[tile].b,
                        tile_colors[tile].a
                    );
                }
                
                // Draw tile
                SDL_Rect tile_rect = {
                    x * TILE_SIZE,
                    y * TILE_SIZE,
                    TILE_SIZE,
                    TILE_SIZE
                };
                
                SDL_RenderFillRect(renderer, &tile_rect);
                
                // For keys and doors, add a distinctive pattern
                if (tile == TILE_KEY) {
                    // Draw key shape
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    int cx = x * TILE_SIZE + TILE_SIZE / 2;
                    int cy = y * TILE_SIZE + TILE_SIZE / 2;
                    int radius = TILE_SIZE / 4;
                    
                    // Draw key head (circle)
                    for (int i = 0; i < 8; i++) {
                        SDL_RenderDrawPoint(renderer, 
                            cx + radius * cos(i * M_PI / 4),
                            cy + radius * sin(i * M_PI / 4));
                    }
                    
                    // Draw key stem
                    SDL_RenderDrawLine(renderer, cx, cy + radius, cx, cy + radius * 2);
                }
                else if (tile == TILE_DOOR) {
                    // Draw door frame
                    SDL_SetRenderDrawColor(renderer, 100, 50, 0, 255);
                    SDL_Rect door_frame = {
                        x * TILE_SIZE + 2,
                        y * TILE_SIZE + 2,
                        TILE_SIZE - 4,
                        TILE_SIZE - 4
                    };
                    SDL_RenderDrawRect(renderer, &door_frame);
                }
            }
        }
    }
    
    // Render players
    for (int i = 0; i < state->num_players; i++) {
        Player* p = &state->players[i];
        
        if (p->is_active) {
            int screen_x = (p->x - start_x) * TILE_SIZE;
            int screen_y = (p->y - start_y) * TILE_SIZE;
            
            // Check if player is in visible area
            if (screen_x >= 0 && screen_x < WINDOW_WIDTH && screen_y >= 0 && screen_y < WINDOW_HEIGHT) {
                // Set color based on player ID
                SDL_SetRenderDrawColor(
                    renderer,
                    player_colors[i].r,
                    player_colors[i].g,
                    player_colors[i].b,
                    player_colors[i].a
                );
                
                // Draw player
                SDL_Rect player_rect = {
                    screen_x + TILE_SIZE / 4,
                    screen_y + TILE_SIZE / 4,
                    TILE_SIZE / 2,
                    TILE_SIZE / 2
                };
                
                SDL_RenderFillRect(renderer, &player_rect);
            }
        }
    }
    
    // Render enemies
    for (int i = 0; i < state->num_enemies; i++) {
        Player* enemy = &state->enemies[i];
        
        if (enemy->is_active) {
            int screen_x = (enemy->x - start_x) * TILE_SIZE;
            int screen_y = (enemy->y - start_y) * TILE_SIZE;
            
            // Check if enemy is in visible area
            if (screen_x >= 0 && screen_x < WINDOW_WIDTH && screen_y >= 0 && screen_y < WINDOW_HEIGHT) {
                // Set color based on enemy type
                SDL_Color color = enemy_colors[0]; // Default color
                
                switch(enemy->type) {
                    case ENTITY_ENEMY_CHASE:
                        color = enemy_colors[0]; // Purple for chaser
                        break;
                    case ENTITY_ENEMY_RANDOM:
                        color = enemy_colors[1]; // Orange for random
                        break;
                    case ENTITY_ENEMY_GUARD:
                        color = enemy_colors[2]; // Cyan for guard
                        break;
                    case ENTITY_ENEMY_SMART:
                        color = enemy_colors[3]; // Yellow for smart
                        break;
                    default:
                        break;
                }
                
                SDL_SetRenderDrawColor(
                    renderer,
                    color.r,
                    color.g,
                    color.b,
                    color.a
                );
                
                // Draw enemy - using triangles to distinguish from players
                // First, create a triangle outline
                SDL_Point triangle[3] = {
                    {screen_x + TILE_SIZE / 2, screen_y + TILE_SIZE / 4},           // Top
                    {screen_x + TILE_SIZE / 4, screen_y + TILE_SIZE * 3 / 4},       // Bottom left
                    {screen_x + TILE_SIZE * 3 / 4, screen_y + TILE_SIZE * 3 / 4}    // Bottom right
                };
                
                // Draw triangle lines
                SDL_RenderDrawLine(renderer, triangle[0].x, triangle[0].y, triangle[1].x, triangle[1].y);
                SDL_RenderDrawLine(renderer, triangle[1].x, triangle[1].y, triangle[2].x, triangle[2].y);
                SDL_RenderDrawLine(renderer, triangle[2].x, triangle[2].y, triangle[0].x, triangle[0].y);
                
                // Fill smaller rect in middle of triangle to make it more visible
                SDL_Rect enemy_center = {
                    screen_x + TILE_SIZE * 3 / 8,
                    screen_y + TILE_SIZE * 3 / 8,
                    TILE_SIZE / 4,
                    TILE_SIZE / 4
                };
                
                SDL_RenderFillRect(renderer, &enemy_center);
            }
        }
    }
    
    // Render game UI (timer, score, health, keys)
    render_game_ui(renderer, state);
    
    // Render game over message if applicable
    if (state->game_over) {
        char game_over_msg[64];
        sprintf(game_over_msg, "Game Over! Winner: Player %d", state->winner_id + 1);
        
        // Draw game over background
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
        SDL_Rect msg_rect = {
            WINDOW_WIDTH / 4,
            WINDOW_HEIGHT / 2 - 15,
            WINDOW_WIDTH / 2,
            30
        };
        SDL_RenderFillRect(renderer, &msg_rect);
    }
}

// Check if a move is valid
bool is_valid_move(GameState* state, int player_id, int dx, int dy) {
    Player* player = &state->players[player_id];
    int new_x = player->x + dx;
    int new_y = player->y + dy;
    
    // Check bounds
    if (new_x < 0 || new_x >= state->map.width || new_y < 0 || new_y >= state->map.height) {
        return false;
    }
    
    // Check for walls
    if (state->map.tiles[new_y][new_x] == TILE_WALL) {
        return false;
    }
    
    return true;
}

// Update player position
void update_player(GameState* state, int player_id, int dx, int dy) {
    lock_game_state();
    
    Player* player = &state->players[player_id];
    
    if (player->is_active && is_valid_move(state, player_id, dx, dy)) {
        int new_x = player->x + dx;
        int new_y = player->y + dy;
        
        // Handle tile interactions
        TileType tile = state->map.tiles[new_y][new_x];
        
        switch (tile) {
            case TILE_TREASURE:
                // Collect treasure
                player->score += 10;
                state->map.tiles[new_y][new_x] = TILE_EMPTY;
                break;
                
            case TILE_KEY:
                // Collect key
                player->keys++;
                state->keys_collected++;
                printf("Key collected! (%d/%d)\n", state->keys_collected, state->keys_required);
                state->map.tiles[new_y][new_x] = TILE_EMPTY;
                
                // Check if all keys collected
                if (state->keys_collected >= state->keys_required) {
                    printf("All keys collected! Find the exit!\n");
                    state->exit_enabled = true;
                    printf("Exit is now enabled!\n");
                }
                break;
                
            case TILE_DOOR:
                // Doors now give bonuses instead of requiring keys
                // Random bonus: health, score, or extra key
                int bonus_type = rand() % 3;
                
                switch (bonus_type) {
                    case 0: // Health bonus
                        player->health += 10;
                        if (player->health > 100) player->health = 100;
                        printf("Door opened! Health bonus received!\n");
                        break;
                    case 1: // Score bonus
                        player->score += 20;
                        printf("Door opened! Score bonus received!\n");
                        break;
                    case 2: // Extra key
                        player->keys++;
                        printf("Door opened! Extra key received!\n");
                        break;
                }
                
                state->map.tiles[new_y][new_x] = TILE_EMPTY;
                break;
                
            case TILE_EXIT:
                // Reached exit, end game if enabled
                if (state->exit_enabled) {
                    state->game_over = true;
                    state->winner_id = player_id;
                    printf("Exit reached! Game won!\n");
                } else {
                    // Show message about needing keys
                    printf("Exit is locked! Collect all %d keys first. (%d/%d collected)\n", 
                           state->keys_required, state->keys_collected, state->keys_required);
                    // Don't allow move onto inactive exit
                    unlock_game_state();
                    return;
                }
                break;
                
            default:
                // Empty tile or other, just move
                break;
        }
        
        // Update position
        player->x = new_x;
        player->y = new_y;
    }
    
    unlock_game_state();
}

// Process player input
void process_player_input(SDL_Event* event, GameState* state, int player_id) {
    if (!event || !state) {
        return;
    }
    
    // Handle keyboard input
    if (event->type == SDL_KEYDOWN) {
        switch (event->key.keysym.sym) {
            case SDLK_UP:
                update_player(state, player_id, 0, -1);
                break;
            case SDLK_DOWN:
                update_player(state, player_id, 0, 1);
                break;
            case SDLK_LEFT:
                update_player(state, player_id, -1, 0);
                break;
            case SDLK_RIGHT:
                update_player(state, player_id, 1, 0);
                break;
            case SDLK_ESCAPE:
                // Mark player as inactive
                lock_game_state();
                state->players[player_id].is_active = false;
                unlock_game_state();
                break;
        }
    }
}

// Update game time and check exit criteria
void update_game_time(GameState* state) {
    lock_game_state();
    
    if (state != NULL) {
        // Update current time
        state->current_time = time(NULL);
        
        // Check if all keys collected to enable exit
        if (state->keys_collected >= state->keys_required) {
            state->exit_enabled = true;
        }
    }
    
    unlock_game_state();
}

// Render the UI elements
void render_game_ui(SDL_Renderer* renderer, GameState* state) {
    // Draw background bar
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_Rect info_rect = {
        0,
        WINDOW_HEIGHT - 30,
        WINDOW_WIDTH,
        30
    };
    SDL_RenderFillRect(renderer, &info_rect);
    
    // Render score
    int score_width = state->players[0].score;
    if (score_width > 0) {
        // Cap score display width
        if (score_width > WINDOW_WIDTH - 250) {
            score_width = WINDOW_WIDTH - 250;
        }
        
        SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255); // Gold color
        SDL_Rect score_rect = {
            10,
            WINDOW_HEIGHT - 25,
            score_width,
            20
        };
        SDL_RenderFillRect(renderer, &score_rect);
    }
    
    // Render keys collected (blue squares) above the health bar
    for (int i = 0; i < state->players[0].keys; i++) {
        SDL_SetRenderDrawColor(renderer, 0, 191, 255, 255); // Key color
        SDL_Rect key_rect = {
            WINDOW_WIDTH - 110 + (i * 22),
            WINDOW_HEIGHT - 45,
            20,
            15
        };
        SDL_RenderFillRect(renderer, &key_rect);
    }
    
    // Render health
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_Rect health_rect = {
        WINDOW_WIDTH - 110,
        WINDOW_HEIGHT - 25,
        state->players[0].health,
        20
    };
    SDL_RenderFillRect(renderer, &health_rect);
    
    // Render time elapsed as numeric display at top of screen
    time_t elapsed = state->current_time - state->start_time;
    int minutes = elapsed / 60;
    int seconds = elapsed % 60;
    
    // Draw timer background
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_Rect timer_bg = {
        WINDOW_WIDTH / 2 - 50, 
        5, 
        100, 
        20
    };
    SDL_RenderFillRect(renderer, &timer_bg);
    
    // Draw digits for the timer (since we don't have text rendering)
    // We'll use small rectangles to create digital number display
    
    // Draw the minutes (tens digit)
    int mins_tens = minutes / 10;
    render_digit(renderer, WINDOW_WIDTH / 2 - 40, 8, mins_tens);
    
    // Draw the minutes (ones digit)
    int mins_ones = minutes % 10;
    render_digit(renderer, WINDOW_WIDTH / 2 - 25, 8, mins_ones);
    
    // Draw colon between minutes and seconds
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect colon_top = {WINDOW_WIDTH / 2 - 12, 10, 2, 2};
    SDL_Rect colon_bottom = {WINDOW_WIDTH / 2 - 12, 16, 2, 2};
    SDL_RenderFillRect(renderer, &colon_top);
    SDL_RenderFillRect(renderer, &colon_bottom);
    
    // Draw the seconds (tens digit)
    int secs_tens = seconds / 10;
    render_digit(renderer, WINDOW_WIDTH / 2 - 5, 8, secs_tens);
    
    // Draw the seconds (ones digit)
    int secs_ones = seconds % 10;
    render_digit(renderer, WINDOW_WIDTH / 2 + 10, 8, secs_ones);
}

// Helper function to render a digit using small rectangles (7-segment display style)
void render_digit(SDL_Renderer* renderer, int x, int y, int digit) {
    // Set color for digits
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    
    // Segment dimensions
    int width = 10;
    int height = 2;
    
    // Segments for 7-segment display (a-g)
    SDL_Rect segments[7] = {
        {x, y, width, height},                // a (top)
        {x + width, y, height, width/2},      // b (top right)
        {x + width, y + width/2, height, width/2}, // c (bottom right)
        {x, y + width, width, height},        // d (bottom)
        {x, y + width/2, height, width/2},    // e (bottom left)
        {x, y, height, width/2},              // f (top left)
        {x, y + width/2 - height/2, width, height}  // g (middle)
    };
    
    // Define which segments are on for each digit (0-9)
    bool digit_patterns[10][7] = {
        {true, true, true, true, true, true, false},      // 0
        {false, true, true, false, false, false, false},  // 1
        {true, true, false, true, true, false, true},     // 2
        {true, true, true, true, false, false, true},     // 3
        {false, true, true, false, false, true, true},    // 4
        {true, false, true, true, false, true, true},     // 5
        {true, false, true, true, true, true, true},      // 6
        {true, true, true, false, false, false, false},   // 7
        {true, true, true, true, true, true, true},       // 8
        {true, true, true, true, false, true, true}       // 9
    };
    
    // Make sure digit is within range
    if (digit < 0) digit = 0;
    if (digit > 9) digit = 9;
    
    // Draw the segments for this digit
    for (int i = 0; i < 7; i++) {
        if (digit_patterns[digit][i]) {
            SDL_RenderFillRect(renderer, &segments[i]);
        }
    }
} 