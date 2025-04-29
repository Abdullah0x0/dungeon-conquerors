#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <pthread.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include "../include/game.h"
#include "../include/shared_memory.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Function declaration from main.c
void draw_simple_text(SDL_Renderer* renderer, int x, int y, const char* text, int size);

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

// Modern color palette for tiles
SDL_Color tile_colors[6] = {
    {40, 44, 52, 255},    // TILE_EMPTY - Dark background
    {24, 26, 31, 255},    // TILE_WALL - Nearly black
    {121, 80, 32, 255},   // TILE_DOOR - Rich wooden brown
    {212, 175, 55, 255},  // TILE_TREASURE - Rich gold
    {46, 204, 113, 255},  // TILE_EXIT - Emerald green
    {52, 152, 219, 255}   // TILE_KEY - Bright blue
};

// Vibrant neon colors for players
SDL_Color player_colors[MAX_PLAYERS] = {
    {231, 76, 60, 255},   // Player 1 (Bright red)
    {41, 128, 185, 255},  // Player 2 (Bright blue)
    {46, 204, 113, 255},  // Player 3 (Bright green)
    {241, 196, 15, 255}   // Player 4 (Bright yellow)
};

// Modern colors for different enemy types
SDL_Color enemy_colors[4] = {
    {155, 89, 182, 255},  // Chaser (Amethyst purple)
    {230, 126, 34, 255},  // Random (Carrot orange)
    {26, 188, 156, 255},  // Guard (Turquoise)
    {241, 196, 15, 255}   // Smart (Sunflower yellow)
};

// Animation variables
float animation_time = 0.0f;
int particle_count = 0;
#define MAX_PARTICLES 100

// Particle structure for visual effects
typedef struct {
    float x, y;         // Position
    float vx, vy;       // Velocity
    float lifetime;     // Remaining life
    float max_lifetime; // Maximum lifetime
    SDL_Color color;    // Particle color
    int size;           // Particle size
    bool active;        // Is particle active
} Particle;

Particle particles[MAX_PARTICLES];

// Initialize a particle effect
void spawn_particle(float x, float y, SDL_Color color, int type) {
    if (particle_count >= MAX_PARTICLES) return;
    
    int idx = -1;
    // Find an inactive particle
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) {
            idx = i;
            break;
        }
    }
    
    if (idx == -1) return; // No free particles
    
    particles[idx].x = x;
    particles[idx].y = y;
    particles[idx].color = color;
    particles[idx].active = true;
    
    // Different particle behaviors based on type
    switch (type) {
        case 0: // Explosion
            particles[idx].vx = (rand() % 100 - 50) / 25.0f;
            particles[idx].vy = (rand() % 100 - 50) / 25.0f;
            particles[idx].size = 3 + (rand() % 3);
            particles[idx].max_lifetime = 0.5f + (rand() % 100) / 200.0f;
            break;
        case 1: // Trail
            particles[idx].vx = (rand() % 60 - 30) / 100.0f;
            particles[idx].vy = (rand() % 60 - 30) / 100.0f - 0.2f; // Upward bias
            particles[idx].size = 2 + (rand() % 2);
            particles[idx].max_lifetime = 0.3f + (rand() % 100) / 500.0f;
            break;
        case 2: // Sparkle
            particles[idx].vx = 0;
            particles[idx].vy = 0;
            particles[idx].size = 1 + (rand() % 2);
            particles[idx].max_lifetime = 0.2f + (rand() % 100) / 500.0f;
            break;
    }
    
    particles[idx].lifetime = particles[idx].max_lifetime;
    particle_count++;
}

// Update all particles
void update_particles(float dt) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;
        
        particles[i].lifetime -= dt;
        if (particles[i].lifetime <= 0) {
            particles[i].active = false;
            particle_count--;
            continue;
        }
        
        particles[i].x += particles[i].vx;
        particles[i].y += particles[i].vy;
        
        // Add slight gravity effect
        particles[i].vy += 0.02f;
    }
}

// Render all active particles
void render_particles(SDL_Renderer* renderer, int start_x, int start_y) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;
        
        // Alpha fades out as particle ages
        float alpha_factor = particles[i].lifetime / particles[i].max_lifetime;
        
        SDL_SetRenderDrawColor(
            renderer,
            particles[i].color.r,
            particles[i].color.g,
            particles[i].color.b,
            (Uint8)(255 * alpha_factor)
        );
        
        // Convert particle world position to screen position
        int screen_x = (particles[i].x - start_x) * TILE_SIZE;
        int screen_y = (particles[i].y - start_y) * TILE_SIZE;
        
        // Draw the particle as a small rectangle
        SDL_Rect particle_rect = {
            (int)screen_x,
            (int)screen_y,
            particles[i].size,
            particles[i].size
        };
        
        SDL_RenderFillRect(renderer, &particle_rect);
    }
}

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
    
    // Update animation time based on current game time
    float current_time = (float)(state->current_time - state->start_time);
    animation_time = current_time;
    
    // Update particles
    update_particles(1.0f / 60.0f); // Assuming 60 FPS
    
    // Draw a dark background gradient
    for (int y = 0; y < WINDOW_HEIGHT; y++) {
        // Subtle gradient effect
        int gradient = 20 + (y * 10 / WINDOW_HEIGHT);
        SDL_SetRenderDrawColor(renderer, gradient/2, gradient/2, gradient, 255);
        SDL_RenderDrawLine(renderer, 0, y, WINDOW_WIDTH, y);
    }
    
    // Render map tiles with modern effects
    for (int y = 0; y < visible_height; y++) {
        for (int x = 0; x < visible_width; x++) {
            int map_x = start_x + x;
            int map_y = start_y + y;
            
            if (map_x >= 0 && map_x < state->map.width && map_y >= 0 && map_y < state->map.height) {
                TileType tile = state->map.tiles[map_y][map_x];
                
                // Create a tile position for special effects
                int tile_x = x * TILE_SIZE;
                int tile_y = y * TILE_SIZE;
                
                // If it's an exit and not enabled, render it differently
                if (tile == TILE_EXIT && !state->exit_enabled) {
                    // Render as inactive exit (darker green with pulsing effect)
                    float pulse = (sinf(animation_time * 2.0f) * 0.3f + 0.7f);
                    SDL_SetRenderDrawColor(renderer, 
                                          (Uint8)(30 * pulse), 
                                          (Uint8)(100 * pulse), 
                                          (Uint8)(50 * pulse), 
                                          255);
                } else if (tile == TILE_EXIT && state->exit_enabled) {
                    // Active exit gets a pulsing effect
                    float pulse = (sinf(animation_time * 4.0f) * 0.3f + 0.7f);
                    SDL_SetRenderDrawColor(renderer, 
                                          (Uint8)(46 * pulse), 
                                          (Uint8)(204 * pulse), 
                                          (Uint8)(113 * pulse), 
                                          255);
                    
                    // Add sparkle particles occasionally to the exit
                    if (rand() % 10 == 0) {
                        float px = map_x + (rand() % 100) / 100.0f;
                        float py = map_y + (rand() % 100) / 100.0f;
                        SDL_Color spark_color = {200, 255, 200, 255};
                        spawn_particle(px, py, spark_color, 2);
                    }
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
                
                // Draw tile with modern appearance
                SDL_Rect tile_rect = {
                    tile_x,
                    tile_y,
                    TILE_SIZE,
                    TILE_SIZE
                };
                
                SDL_RenderFillRect(renderer, &tile_rect);
                
                // Add detail to the tile based on type
                if (tile == TILE_WALL) {
                    // Draw subtle grid lines for walls to create a brick effect
                    SDL_SetRenderDrawColor(renderer, 40, 42, 54, 255);
                    
                    // Horizontal lines
                    if (map_y % 2 == 0) {
                        SDL_RenderDrawLine(renderer, 
                                          tile_x, tile_y + TILE_SIZE/2, 
                                          tile_x + TILE_SIZE, tile_y + TILE_SIZE/2);
                    } else {
                        SDL_RenderDrawLine(renderer, 
                                          tile_x + TILE_SIZE/2, tile_y, 
                                          tile_x + TILE_SIZE/2, tile_y + TILE_SIZE);
                    }
                } 
                else if (tile == TILE_KEY) {
                    // Draw key with glowing effect
                    float glow = (sinf(animation_time * 3.0f) * 0.3f + 0.7f);
                    SDL_SetRenderDrawColor(renderer, 
                                          (Uint8)(255 * glow), 
                                          (Uint8)(255 * glow), 
                                          (Uint8)(100 + 155 * glow), 
                                          255);
                    
                    int cx = tile_x + TILE_SIZE / 2;
                    int cy = tile_y + TILE_SIZE / 2;
                    int radius = TILE_SIZE / 3;
                    
                    // Draw key head (circle with more points for smoothness)
                    for (int i = 0; i < 16; i++) {
                        float angle = i * M_PI / 8;
                        SDL_RenderDrawLine(renderer, 
                            cx, cy,
                            cx + radius * cosf(angle),
                            cy + radius * sinf(angle));
                    }
                    
                    // Draw key stem
                    SDL_RenderDrawLine(renderer, cx, cy + radius/2, cx, cy + radius * 1.5);
                    SDL_RenderDrawLine(renderer, cx-2, cy + radius * 1.5, cx+2, cy + radius * 1.5);
                    
                    // Occasionally add sparkle particles
                    if (rand() % 20 == 0) {
                        float px = map_x + 0.5f;
                        float py = map_y + 0.5f;
                        SDL_Color spark_color = {255, 255, 150, 255};
                        spawn_particle(px, py, spark_color, 2);
                    }
                }
                else if (tile == TILE_DOOR) {
                    // Draw door frame with a more detailed appearance
                    float glow = (sinf(animation_time * 2.0f + map_x * 0.1f + map_y * 0.1f) * 0.2f + 0.8f);
                    SDL_SetRenderDrawColor(renderer, 
                                          (Uint8)(150 * glow), 
                                          (Uint8)(80 * glow), 
                                          (Uint8)(40 * glow), 
                                          255);
                    
                    // Draw door outline
                    SDL_Rect door_frame = {
                        tile_x + 2,
                        tile_y + 2,
                        TILE_SIZE - 4,
                        TILE_SIZE - 4
                    };
                    SDL_RenderDrawRect(renderer, &door_frame);
                    
                    // Draw door handle
                    SDL_Rect door_handle = {
                        tile_x + TILE_SIZE * 3/4 - 2,
                        tile_y + TILE_SIZE/2 - 2,
                        4,
                        4
                    };
                    SDL_RenderFillRect(renderer, &door_handle);
                    
                    // Draw door panels
                    SDL_RenderDrawLine(renderer, 
                                      tile_x + TILE_SIZE/2, tile_y + 4,
                                      tile_x + TILE_SIZE/2, tile_y + TILE_SIZE - 4);
                }
                else if (tile == TILE_TREASURE) {
                    // Draw treasure chest with a pulsing golden glow
                    float glow = (sinf(animation_time * 2.5f) * 0.3f + 0.7f);
                    
                    // Chest base
                    SDL_SetRenderDrawColor(renderer, 
                                          (Uint8)(150 * glow), 
                                          (Uint8)(100 * glow), 
                                          (Uint8)(50 * glow), 
                                          255);
                    
                    SDL_Rect chest_base = {
                        tile_x + 2,
                        tile_y + TILE_SIZE/2,
                        TILE_SIZE - 4,
                        TILE_SIZE/2 - 2
                    };
                    SDL_RenderFillRect(renderer, &chest_base);
                    
                    // Chest top
                    SDL_SetRenderDrawColor(renderer, 
                                          (Uint8)(200 * glow), 
                                          (Uint8)(150 * glow), 
                                          (Uint8)(50 * glow), 
                                          255);
                    
                    SDL_Rect chest_top = {
                        tile_x + 2,
                        tile_y + 2,
                        TILE_SIZE - 4,
                        TILE_SIZE/2 - 2
                    };
                    SDL_RenderFillRect(renderer, &chest_top);
                    
                    // Lock
                    SDL_SetRenderDrawColor(renderer, 
                                          (Uint8)(220 * glow), 
                                          (Uint8)(180 * glow), 
                                          (Uint8)(40 * glow), 
                                          255);
                    
                    SDL_Rect lock = {
                        tile_x + TILE_SIZE/2 - 2,
                        tile_y + TILE_SIZE/2 - 2,
                        4,
                        4
                    };
                    SDL_RenderFillRect(renderer, &lock);
                    
                    // Add occasional sparkle
                    if (rand() % 30 == 0) {
                        float px = map_x + 0.5f;
                        float py = map_y + 0.3f;
                        SDL_Color spark_color = {255, 215, 0, 255};
                        spawn_particle(px, py, spark_color, 2);
                    }
                }
            }
        }
    }
    
    // Render particles behind players and enemies
    render_particles(renderer, start_x, start_y);
    
    // Render players with modern effects
    for (int i = 0; i < state->num_players; i++) {
        Player* p = &state->players[i];
        
        if (p->is_active) {
            int screen_x = (p->x - start_x) * TILE_SIZE;
            int screen_y = (p->y - start_y) * TILE_SIZE;
            
            // Check if player is in visible area
            if (screen_x >= 0 && screen_x < WINDOW_WIDTH && screen_y >= 0 && screen_y < WINDOW_HEIGHT) {
                // Pulsing effect for player
                float pulse = (sinf(animation_time * 3.0f) * 0.15f + 0.85f);
                
                // Set color based on player ID with pulsing
                SDL_SetRenderDrawColor(
                    renderer,
                    (Uint8)(player_colors[i].r * pulse),
                    (Uint8)(player_colors[i].g * pulse),
                    (Uint8)(player_colors[i].b * pulse),
                    player_colors[i].a
                );
                
                // Draw player base (circle)
                int cx = screen_x + TILE_SIZE / 2;
                int cy = screen_y + TILE_SIZE / 2;
                int radius = TILE_SIZE / 3;
                
                // Draw filled circle by rendering lines from center
                for (int angle = 0; angle < 360; angle += 10) {
                    float rad_angle = angle * M_PI / 180.0f;
                    SDL_RenderDrawLine(renderer, 
                        cx, cy,
                        cx + radius * cosf(rad_angle),
                        cy + radius * sinf(rad_angle));
                }
                
                // Draw player inner circle (highlight)
                SDL_SetRenderDrawColor(
                    renderer,
                    (Uint8)fmin(255, player_colors[i].r * 1.3f),
                    (Uint8)fmin(255, player_colors[i].g * 1.3f),
                    (Uint8)fmin(255, player_colors[i].b * 1.3f),
                    player_colors[i].a
                );
                
                radius = TILE_SIZE / 6;
                for (int angle = 0; angle < 360; angle += 15) {
                    float rad_angle = angle * M_PI / 180.0f;
                    SDL_RenderDrawLine(renderer, 
                        cx, cy,
                        cx + radius * cosf(rad_angle),
                        cy + radius * sinf(rad_angle));
                }
                
                // Draw player movement trail (particles)
                if (rand() % 5 == 0) {
                    float px = p->x + (rand() % 80 - 40) / 100.0f;
                    float py = p->y + (rand() % 80 - 40) / 100.0f;
                    SDL_Color trail_color = player_colors[i];
                    trail_color.a = 150;  // Semi-transparent
                    spawn_particle(px, py, trail_color, 1);
                }
            }
        }
    }
    
    // Render enemies with modern effects
    for (int i = 0; i < state->num_enemies; i++) {
        Player* enemy = &state->enemies[i];
        
        if (enemy->is_active) {
            int screen_x = (enemy->x - start_x) * TILE_SIZE;
            int screen_y = (enemy->y - start_y) * TILE_SIZE;
            
            // Check if enemy is in visible area
            if (screen_x >= 0 && screen_x < WINDOW_WIDTH && screen_y >= 0 && screen_y < WINDOW_HEIGHT) {
                // Set color based on enemy type with a pulsing effect
                SDL_Color color = enemy_colors[0]; // Default color
                float pulse_speed = 2.0f;
                
                switch(enemy->type) {
                    case ENTITY_ENEMY_CHASE:
                        color = enemy_colors[0]; // Purple for chaser
                        pulse_speed = 4.0f;      // Faster pulse for chasers
                        break;
                    case ENTITY_ENEMY_RANDOM:
                        color = enemy_colors[1]; // Orange for random
                        pulse_speed = 1.5f;
                        break;
                    case ENTITY_ENEMY_GUARD:
                        color = enemy_colors[2]; // Cyan for guard
                        pulse_speed = 1.0f;
                        break;
                    case ENTITY_ENEMY_SMART:
                        color = enemy_colors[3]; // Yellow for smart
                        pulse_speed = 3.0f;
                        break;
                    default:
                        break;
                }
                
                // Calculate pulsing effect
                float pulse = (sinf(animation_time * pulse_speed) * 0.2f + 0.8f);
                
                SDL_SetRenderDrawColor(
                    renderer,
                    (Uint8)(color.r * pulse),
                    (Uint8)(color.g * pulse),
                    (Uint8)(color.b * pulse),
                    color.a
                );
                
                // Draw enemy base triangle with floating animation
                float hover_offset = sinf(animation_time * 2.0f + i * 0.5f) * 2.0f;
                
                SDL_Point triangle[3] = {
                    {screen_x + TILE_SIZE / 2, (int)(screen_y + TILE_SIZE / 4 + hover_offset)},           // Top
                    {screen_x + TILE_SIZE / 4, (int)(screen_y + TILE_SIZE * 3 / 4 + hover_offset)},       // Bottom left
                    {screen_x + TILE_SIZE * 3 / 4, (int)(screen_y + TILE_SIZE * 3 / 4 + hover_offset)}    // Bottom right
                };
                
                // Fill the triangle
                for (int y = triangle[0].y; y <= triangle[1].y; y++) {
                    // Calculate x range for each scan line
                    float progress = (float)(y - triangle[0].y) / (triangle[1].y - triangle[0].y);
                    int x_start = triangle[0].x - progress * (triangle[0].x - triangle[1].x);
                    int x_end = triangle[0].x + progress * (triangle[2].x - triangle[0].x);
                    SDL_RenderDrawLine(renderer, x_start, y, x_end, y);
                }
                
                // Draw triangle outline
                SDL_RenderDrawLine(renderer, triangle[0].x, triangle[0].y, triangle[1].x, triangle[1].y);
                SDL_RenderDrawLine(renderer, triangle[1].x, triangle[1].y, triangle[2].x, triangle[2].y);
                SDL_RenderDrawLine(renderer, triangle[2].x, triangle[2].y, triangle[0].x, triangle[0].y);
                
                // Draw eye (based on enemy type)
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White eyes
                
                // Different eye patterns for different enemy types
                switch(enemy->type) {
                    case ENTITY_ENEMY_CHASE:
                        // One big central eye
                        {
                            SDL_Rect enemy_eye = {
                    screen_x + TILE_SIZE * 3 / 8,
                                (int)(screen_y + TILE_SIZE * 3 / 8 + hover_offset),
                    TILE_SIZE / 4,
                    TILE_SIZE / 4
                };
                            SDL_RenderFillRect(renderer, &enemy_eye);
                        }
                        break;
                    case ENTITY_ENEMY_RANDOM:
                        // Two small eyes
                        {
                            SDL_Rect eye_left = {
                                screen_x + TILE_SIZE * 5 / 16,
                                (int)(screen_y + TILE_SIZE * 3 / 8 + hover_offset),
                                TILE_SIZE / 8,
                                TILE_SIZE / 8
                            };
                            SDL_Rect eye_right = {
                                screen_x + TILE_SIZE * 9 / 16,
                                (int)(screen_y + TILE_SIZE * 3 / 8 + hover_offset),
                                TILE_SIZE / 8,
                                TILE_SIZE / 8
                            };
                            SDL_RenderFillRect(renderer, &eye_left);
                            SDL_RenderFillRect(renderer, &eye_right);
                        }
                        break;
                    case ENTITY_ENEMY_GUARD:
                        // Horizontal bar eyes
                        {
                            SDL_Rect enemy_bar_eye = {
                                screen_x + TILE_SIZE * 3 / 8,
                                (int)(screen_y + TILE_SIZE * 3 / 8 + hover_offset),
                                TILE_SIZE / 4,
                                TILE_SIZE / 8
                            };
                            SDL_RenderFillRect(renderer, &enemy_bar_eye);
                        }
                        break;
                    case ENTITY_ENEMY_SMART:
                        // Smart enemy has a red pupil in the eye
                        {
                            SDL_Rect smart_eye = {
                                screen_x + TILE_SIZE * 3 / 8,
                                (int)(screen_y + TILE_SIZE * 3 / 8 + hover_offset),
                                TILE_SIZE / 4,
                                TILE_SIZE / 4
                            };
                            SDL_RenderFillRect(renderer, &smart_eye);
                            
                            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red pupil
                            SDL_Rect pupil = {
                                screen_x + TILE_SIZE / 2 - TILE_SIZE / 16,
                                (int)(screen_y + TILE_SIZE / 2 - TILE_SIZE / 16 + hover_offset),
                                TILE_SIZE / 8,
                                TILE_SIZE / 8
                            };
                            SDL_RenderFillRect(renderer, &pupil);
                        }
                        break;
                    default:
                        // Default eye
                        {
                            SDL_Rect default_eye = {
                                screen_x + TILE_SIZE * 3 / 8,
                                (int)(screen_y + TILE_SIZE * 3 / 8 + hover_offset),
                                TILE_SIZE / 4,
                                TILE_SIZE / 4
                            };
                            SDL_RenderFillRect(renderer, &default_eye);
                        }
                        break;
                }
                
                // Add occasional enemy trail particles
                if (rand() % 15 == 0) {
                    float px = enemy->x;
                    float py = enemy->y;
                    spawn_particle(px, py, color, 1);
                }
            }
        }
    }
    
    // Render game UI (timer, score, health, keys)
    render_game_ui(renderer, state);
    
    // Render game over message if applicable
    if (state->game_over) {
        char game_over_msg[64];
        sprintf(game_over_msg, "Game Over! Winner: Player %d", state->winner_id + 1);
        
        // Create a pulsing overlay
        float pulse = (sinf(animation_time * 2.0f) * 0.1f + 0.9f);
        
        // Draw game over background with pulse effect
        if (state->winner_id >= 0) {
            // Victory
            SDL_SetRenderDrawColor(renderer, (Uint8)(0 * pulse), (Uint8)(100 * pulse), (Uint8)(0 * pulse), 200);
        } else {
            // Defeat
            SDL_SetRenderDrawColor(renderer, (Uint8)(100 * pulse), (Uint8)(0 * pulse), (Uint8)(0 * pulse), 200);
        }
        
        SDL_Rect msg_rect = {
            WINDOW_WIDTH / 4,
            WINDOW_HEIGHT / 2 - 50,
            WINDOW_WIDTH / 2,
            100
        };
        SDL_RenderFillRect(renderer, &msg_rect);
        
        // Draw border
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_Rect border = {msg_rect.x - 2, msg_rect.y - 2, msg_rect.w + 4, msg_rect.h + 4};
        SDL_RenderDrawRect(renderer, &border);
        
        // Generate lots of particles for game over
        if (particle_count < MAX_PARTICLES / 2) {
            for (int i = 0; i < 5; i++) {
                float px = rand() % WINDOW_WIDTH / TILE_SIZE + start_x;
                float py = rand() % WINDOW_HEIGHT / TILE_SIZE + start_y;
                
                SDL_Color particle_color;
                if (state->winner_id >= 0) {
                    // Victory particles
                    particle_color.r = 100 + (rand() % 155);
                    particle_color.g = 200 + (rand() % 55);
                    particle_color.b = 100 + (rand() % 155);
                } else {
                    // Defeat particles
                    particle_color.r = 200 + (rand() % 55);
                    particle_color.g = 50 + (rand() % 100);
                    particle_color.b = 50 + (rand() % 100);
                }
                particle_color.a = 255;
                
                spawn_particle(px, py, particle_color, 0);
            }
        }
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
    
    // Check for enemy collision
    for (int i = 0; i < state->num_enemies; i++) {
        if (state->enemies[i].is_active && 
            state->enemies[i].x == new_x && 
            state->enemies[i].y == new_y) {
            return false;
        }
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
                // Random bonus: health or score (removed key bonus)
                int bonus_type = rand() % 2;
                
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
                }
                
                state->map.tiles[new_y][new_x] = TILE_EMPTY;
                break;
                
            case TILE_EXIT:
                // Reached exit, check if we need to advance to the next level
                if (state->exit_enabled) {
                    if (state->current_level < MAX_LEVEL) {
                        // Advance to the next level
                        state->current_level++;
                        printf("Level %d completed! Advancing to level %d!\n", 
                               state->current_level - 1, state->current_level);
                        
                        // Reset key collection for the new level
                        state->keys_collected = 0;
                        state->exit_enabled = false;
                        
                        // Restore player's health
                        player->health = 100;
                        player->keys = 0;
                        
                        // Generate the next level
                        generate_level(state, state->current_level);
                        
                        // Reset player position to starting point
                        player->x = 2;
                        player->y = 2;
                        
                        // Don't allow immediate move
                        unlock_game_state();
                        return;
                    } else {
                        // Final level completed, end game
                        state->game_over = true;
                        state->winner_id = player_id;
                        printf("Exit reached! Game won!\n");
                    }
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
                // Mark player as inactive and set game exit status
                lock_game_state();
                state->players[player_id].is_active = false;
                state->game_over = true;
                state->winner_id = -2;  // Special code to indicate game exited (not victory or defeat)
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
    // Draw modern UI background with gradient
    for (int y = WINDOW_HEIGHT - 40; y < WINDOW_HEIGHT; y++) {
        float fade = (float)(y - (WINDOW_HEIGHT - 40)) / 40.0f;
        SDL_SetRenderDrawColor(renderer, 
                              (Uint8)(20 + fade * 20), 
                              (Uint8)(20 + fade * 20), 
                              (Uint8)(30 + fade * 20), 
                              (Uint8)(200 + fade * 55));
        SDL_RenderDrawLine(renderer, 0, y, WINDOW_WIDTH, y);
    }
    
    // Draw UI frame
    SDL_SetRenderDrawColor(renderer, 60, 60, 80, 255);
    SDL_Rect frame = {
        4,
        WINDOW_HEIGHT - 36,
        WINDOW_WIDTH - 8,
        32
    };
    SDL_RenderDrawRect(renderer, &frame);
    
    // Draw subtle UI separator lines
    SDL_SetRenderDrawColor(renderer, 100, 100, 120, 100);
    SDL_RenderDrawLine(renderer, WINDOW_WIDTH/3, WINDOW_HEIGHT - 36, WINDOW_WIDTH/3, WINDOW_HEIGHT - 4);
    SDL_RenderDrawLine(renderer, WINDOW_WIDTH*2/3, WINDOW_HEIGHT - 36, WINDOW_WIDTH*2/3, WINDOW_HEIGHT - 4);
    
    // Render score with gradient
    int score_width = state->players[0].score;
    if (score_width > 0) {
        // Cap score display width
        if (score_width > WINDOW_WIDTH/3 - 20) {
            score_width = WINDOW_WIDTH/3 - 20;
        }
        
        // Draw score label
        SDL_SetRenderDrawColor(renderer, 180, 180, 200, 255);
        draw_simple_text(renderer, 15, WINDOW_HEIGHT - 34, "SCORE", 8);
        
        // Draw score bar background
        SDL_SetRenderDrawColor(renderer, 40, 40, 50, 200);
        SDL_Rect score_bg = {
            15,
            WINDOW_HEIGHT - 20,
            WINDOW_WIDTH/3 - 25,
            14
        };
        SDL_RenderFillRect(renderer, &score_bg);
        
        // Draw score gradient bar
        for (int x = 0; x < score_width; x++) {
            float progress = (float)x / (WINDOW_WIDTH/3 - 25);
            SDL_SetRenderDrawColor(renderer, 
                                  (Uint8)(255 * (1.0f - progress)), 
                                  (Uint8)(215 * progress + 180 * (1.0f - progress)), 
                                  (Uint8)(0 * progress + 40 * (1.0f - progress)), 
                                  255);
            SDL_RenderDrawLine(renderer, 
                              15 + x, 
                              WINDOW_HEIGHT - 20, 
                              15 + x, 
                              WINDOW_HEIGHT - 7);
        }
    }
    
    // Render keys collected with animations
    // Draw key label
    SDL_SetRenderDrawColor(renderer, 180, 180, 200, 255);
    draw_simple_text(renderer, WINDOW_WIDTH/3 + 15, WINDOW_HEIGHT - 34, "KEYS", 8);
    
    // Determine number of key slots based on level
    int num_slots = state->keys_required; 
    
    // Adjust spacing if we have more than 5 slots
    int key_spacing = 25; // Default spacing
    if (num_slots > 5) {
        key_spacing = (WINDOW_WIDTH/3 - 35) / num_slots; // Adjust spacing to fit all keys
    }
    
    // Draw key slots (empty)
    for (int i = 0; i < num_slots; i++) {
        SDL_SetRenderDrawColor(renderer, 40, 40, 50, 200);
        SDL_Rect key_slot = {
            WINDOW_WIDTH/3 + 15 + (i * key_spacing),
            WINDOW_HEIGHT - 20,
            20,
            14
        };
        SDL_RenderFillRect(renderer, &key_slot);
        
        // Draw slot border
        SDL_SetRenderDrawColor(renderer, 80, 80, 100, 200);
        SDL_RenderDrawRect(renderer, &key_slot);
    }
    
    // Draw collected keys with animations
    for (int i = 0; i < state->players[0].keys; i++) {
        // Don't draw more keys than slots available
        if (i >= num_slots) break;
        
        float pulse = (sinf(animation_time * 3.0f + i * 0.5f) * 0.2f + 0.8f);
        SDL_SetRenderDrawColor(renderer, 
                              (Uint8)(52 * pulse), 
                              (Uint8)(152 * pulse), 
                              (Uint8)(219 * pulse), 
                              255);
        
        SDL_Rect key_rect = {
            WINDOW_WIDTH/3 + 17 + (i * key_spacing),
            WINDOW_HEIGHT - 18,
            16,
            10
        };
        SDL_RenderFillRect(renderer, &key_rect);
        
        // Draw key detail
        SDL_SetRenderDrawColor(renderer, 200, 200, 255, 255);
        SDL_Rect key_detail = {
            WINDOW_WIDTH/3 + 25 + (i * key_spacing),
            WINDOW_HEIGHT - 15,
            4,
            4
        };
        SDL_RenderFillRect(renderer, &key_detail);
    }
    
    // Render health with modern style
    // Draw health label
    SDL_SetRenderDrawColor(renderer, 180, 180, 200, 255);
    draw_simple_text(renderer, WINDOW_WIDTH*2/3 + 15, WINDOW_HEIGHT - 34, "HEALTH", 8);
    
    // Draw health bar background
    SDL_SetRenderDrawColor(renderer, 40, 40, 50, 200);
    SDL_Rect health_bg = {
        WINDOW_WIDTH*2/3 + 15,
        WINDOW_HEIGHT - 20,
        WINDOW_WIDTH/3 - 30,
        14
    };
    SDL_RenderFillRect(renderer, &health_bg);
    
    // Calculate width based on health
    int health_width = (WINDOW_WIDTH/3 - 30) * state->players[0].health / 100;
    
    // Draw health gradient from red to green based on amount
    float health_percent = state->players[0].health / 100.0f;
    for (int x = 0; x < health_width; x++) {
        float progress = (float)x / (WINDOW_WIDTH/3 - 30);
        SDL_SetRenderDrawColor(renderer, 
                              (Uint8)(255 * (1.0f - progress * health_percent)), 
                              (Uint8)(80 * (1.0f - health_percent) + 220 * health_percent * progress), 
                              (Uint8)(80 * (1.0f - health_percent)), 
                              255);
        SDL_RenderDrawLine(renderer, 
                          WINDOW_WIDTH*2/3 + 15 + x, 
                          WINDOW_HEIGHT - 20, 
                          WINDOW_WIDTH*2/3 + 15 + x, 
                          WINDOW_HEIGHT - 7);
    }
    
    // Add health bar segments for more modern look
    for (int i = 1; i < 10; i++) {
        int x = WINDOW_WIDTH*2/3 + 15 + (WINDOW_WIDTH/3 - 30) * i / 10;
        SDL_SetRenderDrawColor(renderer, 60, 60, 70, 150);
        SDL_RenderDrawLine(renderer, x, WINDOW_HEIGHT - 20, x, WINDOW_HEIGHT - 7);
    }
    
    // Low health warning (pulsing) when below 30%
    if (state->players[0].health < 30) {
        float warning_pulse = (sinf(animation_time * 5.0f) * 0.5f + 0.5f);
        SDL_SetRenderDrawColor(renderer, 
                              255, 
                              (Uint8)(50 * warning_pulse), 
                              (Uint8)(50 * warning_pulse), 
                              (Uint8)(100 * warning_pulse + 50));
        SDL_Rect warning = {
            WINDOW_WIDTH*2/3 + 15,
            WINDOW_HEIGHT - 20,
            WINDOW_WIDTH/3 - 30,
            14
        };
        SDL_RenderDrawRect(renderer, &warning);
    }
    
    // Render time elapsed as a stylish digital clock at top of screen
    time_t elapsed = state->current_time - state->start_time;
    int minutes = elapsed / 60;
    int seconds = elapsed % 60;
    
    // Draw timer background with gradient
    for (int y = 0; y < 25; y++) {
        float fade = (float)y / 25.0f;
        SDL_SetRenderDrawColor(renderer, 
                              (Uint8)(20 + fade * 20), 
                              (Uint8)(20 + fade * 20), 
                              (Uint8)(30 + fade * 20), 
                              (Uint8)(200 - fade * 100));
        SDL_RenderDrawLine(renderer, WINDOW_WIDTH/2 - 60, y, WINDOW_WIDTH/2 + 60, y);
    }
    
    // Draw timer border
    SDL_SetRenderDrawColor(renderer, 100, 100, 120, 150);
    SDL_Rect timer_border = {
        WINDOW_WIDTH/2 - 52, 
        3, 
        104, 
        22
    };
    SDL_RenderDrawRect(renderer, &timer_border);
    
    // Draw digital clock style time using 7-segment display style digits
    float glow = (sinf(animation_time * 1.0f) * 0.2f + 0.8f);
    SDL_SetRenderDrawColor(renderer, 
                          (Uint8)(180 * glow), 
                          (Uint8)(220 * glow), 
                          (Uint8)(255 * glow), 
                          255);
    
    // Render minutes tens digit
    render_digit(renderer, WINDOW_WIDTH/2 - 40, 7, minutes / 10);
    
    // Render minutes ones digit
    render_digit(renderer, WINDOW_WIDTH/2 - 25, 7, minutes % 10);
    
    // Render colon between minutes and seconds
    SDL_Rect colon_top = {WINDOW_WIDTH/2 - 10, 10, 2, 2};
    SDL_Rect colon_bottom = {WINDOW_WIDTH/2 - 10, 16, 2, 2};
    SDL_RenderFillRect(renderer, &colon_top);
    SDL_RenderFillRect(renderer, &colon_bottom);
    
    // Render seconds tens digit
    render_digit(renderer, WINDOW_WIDTH/2, 7, seconds / 10);
    
    // Render seconds ones digit
    render_digit(renderer, WINDOW_WIDTH/2 + 15, 7, seconds % 10);
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

// Generate a level with difficulty based on level number
void generate_level(GameState *state, int level) {
    if (state == NULL) {
        printf("Error: NULL state passed to generate_level\n");
        return;
    }
    
    printf("Generating level %d...\n", level);
    
    // Validate level number
    if (level < 1 || level > MAX_LEVEL) {
        printf("Error: Invalid level number %d (max: %d)\n", level, MAX_LEVEL);
        return;
    }
    
    // Reset the map
    memset(state->map.tiles, 0, sizeof(state->map.tiles));
    state->map.width = MAP_WIDTH;
    state->map.height = MAP_HEIGHT;
    
    // Initialize map with walls around the edges
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (x == 0 || y == 0 || x == MAP_WIDTH-1 || y == MAP_HEIGHT-1) {
                state->map.tiles[y][x] = TILE_WALL;
            } else {
                state->map.tiles[y][x] = TILE_EMPTY;
            }
        }
    }
    
    // Create a more complex maze using a cellular automaton approach
    // First, randomly place walls - denser for higher levels
    int wall_chance = 30 + (level - 1) * 5; // 30% for level 1, 35% for level 2, etc.
    wall_chance = wall_chance > 50 ? 50 : wall_chance; // Cap at 50%
    
    for (int y = 1; y < MAP_HEIGHT-1; y++) {
        for (int x = 1; x < MAP_WIDTH-1; x++) {
            // Keep starting area clear (top-left corner)
            if (x < 5 && y < 5) continue;
            
            // Place walls based on level difficulty
            if (rand() % 100 < wall_chance) {
                state->map.tiles[y][x] = TILE_WALL;
            }
        }
    }
    
    // Smooth the maze using a cellular automaton approach
    for (int iteration = 0; iteration < 3; iteration++) {
        // Create a temporary copy of the map
        TileType temp_map[MAP_HEIGHT][MAP_WIDTH];
        memcpy(temp_map, state->map.tiles, sizeof(temp_map));
        
        for (int y = 1; y < MAP_HEIGHT-1; y++) {
            for (int x = 1; x < MAP_WIDTH-1; x++) {
                // Count walls in 3x3 neighborhood
                int wall_count = 0;
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (state->map.tiles[y+dy][x+dx] == TILE_WALL) {
                            wall_count++;
                        }
                    }
                }
                
                // Apply cellular automaton rules
                if (wall_count >= 5) {
                    temp_map[y][x] = TILE_WALL;
                } else if (wall_count <= 2) {
                    temp_map[y][x] = TILE_EMPTY;
                }
                // Otherwise, keep the current state
            }
        }
        
        // Update the map with the smoothed version
        memcpy(state->map.tiles, temp_map, sizeof(temp_map));
    }
    
    // Ensure the player's starting position is clear
    for (int y = 1; y < 8; y++) {
        for (int x = 1; x < 8; x++) {
            state->map.tiles[y][x] = TILE_EMPTY;
        }
    }
    
    // Also create clear paths outward from the starting area in four directions
    // Path to the right
    for (int x = 8; x < 15; x++) {
        state->map.tiles[4][x] = TILE_EMPTY;
        state->map.tiles[5][x] = TILE_EMPTY;
    }
    
    // Path downward
    for (int y = 8; y < 15; y++) {
        state->map.tiles[y][4] = TILE_EMPTY;
        state->map.tiles[y][5] = TILE_EMPTY;
    }
    
    // Place a door at the end of each starting path
    state->map.tiles[4][14] = TILE_DOOR;
    state->map.tiles[14][4] = TILE_DOOR;
    
    // Add doors to create sections - reduced to just 2 doors
    for (int i = 0; i < 2; i++) {
        int x = 15 + rand() % (MAP_WIDTH - 25);
        int y = 15 + rand() % (MAP_HEIGHT - 25);
        state->map.tiles[y][x] = TILE_DOOR;
    }
    
    // Add treasures - minimal quantity
    int num_treasures = MAP_WIDTH * MAP_HEIGHT / 400 + (level - 1); // Reduced from /200 to /400, and from *2 to *1
    for (int i = 0; i < num_treasures; i++) {
        int x = rand() % (MAP_WIDTH - 2) + 1;
        int y = rand() % (MAP_HEIGHT - 2) + 1;
        if (state->map.tiles[y][x] == TILE_EMPTY) {
            state->map.tiles[y][x] = TILE_TREASURE;
        }
    }
    
    // Add exit in the far corner
    state->map.tiles[MAP_HEIGHT-2][MAP_WIDTH-2] = TILE_EMPTY; // Clear any walls near exit
    state->map.tiles[MAP_HEIGHT-3][MAP_WIDTH-2] = TILE_EMPTY;
    state->map.tiles[MAP_HEIGHT-2][MAP_WIDTH-3] = TILE_EMPTY;
    state->map.tiles[MAP_HEIGHT-3][MAP_WIDTH-3] = TILE_EMPTY;
    state->map.tiles[MAP_HEIGHT-2][MAP_WIDTH-2] = TILE_EXIT;
    
    // Create a path from exit toward the center of the map
    int path_x = MAP_WIDTH - 2;
    int path_y = MAP_HEIGHT - 2;
    int target_x = MAP_WIDTH / 2;
    int target_y = MAP_HEIGHT / 2;
    
    // Clear a path from exit toward center (using simple pathfinding)
    while (path_x > target_x || path_y > target_y) {
        // Move toward center, one step at a time
        if (path_x > target_x) {
            path_x--;
            state->map.tiles[path_y][path_x] = TILE_EMPTY;
        }
        if (path_y > target_y) {
            path_y--;
            state->map.tiles[path_y][path_x] = TILE_EMPTY;
        }
        
        // Also clear adjacent tiles to make the path wider and more visible
        if (path_x + 1 < MAP_WIDTH) 
            state->map.tiles[path_y][path_x + 1] = TILE_EMPTY;
        if (path_y + 1 < MAP_HEIGHT) 
            state->map.tiles[path_y + 1][path_x] = TILE_EMPTY;
    }
    
    // Reset level complete flag
    state->level_complete = false;
    
    // Increase enemy speed and aggression based on level
    for (int i = 0; i < state->num_enemies; i++) {
        Player* enemy = &state->enemies[i];
        // Store speed and aggression in the score field temporarily
        enemy->score = (int)((1.5f + (level * 0.2f)) * 100); // Store speed * 100
        enemy->health = (int)((0.8f + (level * 0.1f)) * 100); // Store aggression * 100
        enemy->keys = 10 + (level * 2); // Store detection range
    }
    
    // Set keys required based on level
    if (level == 1) {
        state->keys_required = 5;  // Level 1: 5 keys
    } else if (level == 2) {
        state->keys_required = 7;  // Level 2: 7 keys
    }
    state->keys_collected = 0;
    state->exit_enabled = false;
    
    // Add exactly the required number of keys
    for (int i = 0; i < state->keys_required; i++) {
        int x, y;
        do {
            // Place keys farther from the starting point
            x = 20 + rand() % (MAP_WIDTH - 25);
            y = 20 + rand() % (MAP_HEIGHT - 25);
        } while (state->map.tiles[y][x] != TILE_EMPTY);
        
        state->map.tiles[y][x] = TILE_KEY;
        
        // Create a path from this key to either the starting area or the map center
        int path_x = x;
        int path_y = y;
        int target_x, target_y;
        
        // Alternate between creating paths to start and center to create connections
        if (i % 2 == 0) {
            // Path to starting area
            target_x = 3;
            target_y = 3;
        } else {
            // Path to center of map
            target_x = MAP_WIDTH / 2;
            target_y = MAP_HEIGHT / 2;
        }
        
        // Create path from key to target
        while ((abs(path_x - target_x) > 3) || (abs(path_y - target_y) > 3)) {
            // Choose direction based on which axis has greater distance
            if (abs(path_x - target_x) > abs(path_y - target_y)) {
                // Move horizontally
                path_x += (path_x < target_x) ? 1 : -1;
            } else {
                // Move vertically
                path_y += (path_y < target_y) ? 1 : -1;
            }
            
            // Clear current tile if it's a wall
            if (state->map.tiles[path_y][path_x] == TILE_WALL) {
                state->map.tiles[path_y][path_x] = TILE_EMPTY;
            }
            
            // Occasionally place a door (5% chance)
            if (rand() % 100 < 5 && state->map.tiles[path_y][path_x] == TILE_EMPTY) {
                state->map.tiles[path_y][path_x] = TILE_DOOR;
            }
        }
    }
    
    printf("Level %d generated: %d keys required\n", level, state->keys_required);
} 