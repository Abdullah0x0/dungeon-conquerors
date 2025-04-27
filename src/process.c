#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/time.h>
#include <SDL2/SDL.h>
#include "../include/process.h"
#include "../include/game.h"
#include "../include/shared_memory.h"

// Array to store player process IDs
pid_t player_pids[MAX_PROCESSES];
pid_t enemy_pids[MAX_ENEMY_PROCESSES];
int num_processes = 0;
int num_enemy_processes = 0;

// Pipe file descriptors for IPC
int main_to_enemy_pipe[MAX_ENEMY_PROCESSES][2];  // Main process to enemy processes
int enemy_to_main_pipe[MAX_ENEMY_PROCESSES][2];  // Enemy processes to main process

// Set up IPC channels
bool setup_ipc_channels(void) {
    for (int i = 0; i < MAX_ENEMY_PROCESSES; i++) {
        // Initialize pipes with invalid values
        main_to_enemy_pipe[i][0] = -1;
        main_to_enemy_pipe[i][1] = -1;
        enemy_to_main_pipe[i][0] = -1;
        enemy_to_main_pipe[i][1] = -1;
    }
    
    return true;
}

// Create player processes
void create_player_processes(int count) {
    if (count > MAX_PROCESSES) {
        count = MAX_PROCESSES;
    }
    
    // Initialize game state player data
    for (int i = 0; i < count; i++) {
        lock_game_state();
        game_state->players[i].id = i;
        game_state->players[i].x = 4;
        game_state->players[i].y = 4;
        game_state->players[i].health = 100;
        game_state->players[i].score = 0;
        game_state->players[i].is_active = true;
        game_state->players[i].type = ENTITY_PLAYER;
        game_state->players[i].keys = 0;
        game_state->num_players = count;
        unlock_game_state();
        
        // Ensure starting area is clear
        lock_game_state();
        for (int y = 1; y < 6; y++) {
            for (int x = 1; x < 6; x++) {
                if (game_state->map.tiles[y][x] == TILE_WALL) {
                    // Clear any walls near the start
                    game_state->map.tiles[y][x] = TILE_EMPTY;
                }
            }
        }
        unlock_game_state();
    }
}

// Create enemy AI processes
void create_enemy_processes(int count) {
    if (count > MAX_ENEMY_PROCESSES) {
        count = MAX_ENEMY_PROCESSES;
    }
    
    // Create pipes for IPC between main and enemy processes
    for (int i = 0; i < count; i++) {
        if (pipe(main_to_enemy_pipe[i]) == -1 || pipe(enemy_to_main_pipe[i]) == -1) {
            perror("Failed to create pipes for enemy process");
            continue;
        }
        
        // Initialize enemy data in game state
        lock_game_state();
        // Position enemies in different parts of the map far from player
        game_state->enemies[i].id = i;
        
        // Define valid spawn locations based on corner regions, far from player's start position (2,2)
        struct {
            int min_x, max_x;
            int min_y, max_y;
        } spawnRegions[4] = {
            {MAP_WIDTH - 20, MAP_WIDTH - 5, 5, 15},                  // Top right
            {MAP_WIDTH - 20, MAP_WIDTH - 5, MAP_HEIGHT - 15, MAP_HEIGHT - 5}, // Bottom right
            {5, 15, MAP_HEIGHT - 15, MAP_HEIGHT - 5},                // Bottom left
            {MAP_WIDTH/2 + 20, MAP_WIDTH - 5, MAP_HEIGHT/2 - 5, MAP_HEIGHT/2 + 5}  // Middle right
        };
        
        // Select a spawn region for this enemy
        int regionIndex = i % 4;
        
        // Find a valid empty tile within the region
        int x = 0, y = 0;
        bool valid_position = false;
        int attempts = 0;
        const int MAX_ATTEMPTS = 50;
        
        while (!valid_position && attempts < MAX_ATTEMPTS) {
            x = spawnRegions[regionIndex].min_x + 
                rand() % (spawnRegions[regionIndex].max_x - spawnRegions[regionIndex].min_x);
            y = spawnRegions[regionIndex].min_y + 
                rand() % (spawnRegions[regionIndex].max_y - spawnRegions[regionIndex].min_y);
            
            // Check if the position is an empty tile
            if (game_state->map.tiles[y][x] == TILE_EMPTY) {
                // Clear any walls in adjacent tiles to ensure enemies can move
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        int nx = x + dx;
                        int ny = y + dy;
                        if (nx > 0 && nx < MAP_WIDTH-1 && ny > 0 && ny < MAP_HEIGHT-1) {
                            if (game_state->map.tiles[ny][nx] == TILE_WALL) {
                                game_state->map.tiles[ny][nx] = TILE_EMPTY;
                            }
                        }
                    }
                }
                valid_position = true;
            }
            attempts++;
        }
        
        // If no valid position found, use fallback positions
        if (!valid_position) {
            switch (i) {
                case 0: x = MAP_WIDTH - 10; y = MAP_HEIGHT - 10; break;
                case 1: x = MAP_WIDTH - 15; y = 15; break;
                case 2: x = MAP_WIDTH - 12; y = MAP_HEIGHT - 12; break;
                case 3: x = MAP_WIDTH - 20; y = MAP_HEIGHT / 2; break;
                case 4: x = 15; y = MAP_HEIGHT - 15; break;
                default: x = MAP_WIDTH - 8; y = MAP_HEIGHT - 8; break;
            }
            // Ensure the fallback position is walkable
            game_state->map.tiles[y][x] = TILE_EMPTY;
        }
        
        // Set enemy position and type
        game_state->enemies[i].x = x;
        game_state->enemies[i].y = y;
        
        // Assign enemy type
        switch (i % 5) {
            case 0: game_state->enemies[i].type = ENTITY_ENEMY_CHASE; break;
            case 1: game_state->enemies[i].type = ENTITY_ENEMY_RANDOM; break;
            case 2: game_state->enemies[i].type = ENTITY_ENEMY_GUARD; break;
            case 3: game_state->enemies[i].type = ENTITY_ENEMY_CHASE; break;
            case 4: game_state->enemies[i].type = ENTITY_ENEMY_SMART; break;
        }
        
        game_state->enemies[i].health = 100;
        game_state->enemies[i].is_active = true;
        game_state->num_enemies = count;
        unlock_game_state();
        
        // Fork to create enemy process
        pid_t pid = fork();
        
        if (pid == -1) {
            perror("fork failed for enemy process");
            continue;
        } else if (pid == 0) {
            // Child process - enemy AI
            
            // Close unused pipe ends
            close(main_to_enemy_pipe[i][1]); // Close write end of main-to-enemy pipe
            close(enemy_to_main_pipe[i][0]); // Close read end of enemy-to-main pipe
            
            // Run enemy process
            enemy_process_main(i, game_state->enemies[i].type);
            
            // Clean up and exit
            close(main_to_enemy_pipe[i][0]);
            close(enemy_to_main_pipe[i][1]);
            exit(0);
        } else {
            // Parent process
            enemy_pids[i] = pid;
            num_enemy_processes++;
            
            // Close unused pipe ends
            close(main_to_enemy_pipe[i][0]); // Close read end of main-to-enemy pipe
            close(enemy_to_main_pipe[i][1]); // Close write end of enemy-to-main pipe
        }
    }
}

// Wait for all player processes to finish
void wait_for_player_processes(void) {
    for (int i = 0; i < num_processes; i++) {
        if (player_pids[i] > 0) {
            int status;
            waitpid(player_pids[i], &status, 0);
        }
    }
}

// Wait for all enemy processes to finish
void wait_for_enemy_processes(void) {
    for (int i = 0; i < num_enemy_processes; i++) {
        if (enemy_pids[i] > 0) {
            int status;
            waitpid(enemy_pids[i], &status, 0);
        }
    }
}

// Clean up IPC channels
void cleanup_ipc_channels(void) {
    // Close all pipe ends
    for (int i = 0; i < MAX_ENEMY_PROCESSES; i++) {
        if (main_to_enemy_pipe[i][0] >= 0) close(main_to_enemy_pipe[i][0]);
        if (main_to_enemy_pipe[i][1] >= 0) close(main_to_enemy_pipe[i][1]);
        if (enemy_to_main_pipe[i][0] >= 0) close(enemy_to_main_pipe[i][0]);
        if (enemy_to_main_pipe[i][1] >= 0) close(enemy_to_main_pipe[i][1]);
    }
    
    // Terminate all enemy processes
    for (int i = 0; i < num_enemy_processes; i++) {
        if (enemy_pids[i] > 0) {
            kill(enemy_pids[i], SIGTERM);
        }
    }
}

// Send a message to an enemy process
void send_message_to_enemy(int enemy_id, GameMessage *message) {
    if (enemy_id >= 0 && enemy_id < MAX_ENEMY_PROCESSES) {
        ssize_t bytes_written = write(main_to_enemy_pipe[enemy_id][1], message, sizeof(GameMessage));
        if (bytes_written != sizeof(GameMessage)) {
            // Handle error - could not write full message
            fprintf(stderr, "Warning: Could not write full message to enemy process %d\n", enemy_id);
        }
    }
}

// Receive a message from an enemy process (non-blocking)
bool receive_message_from_enemy(int enemy_id, GameMessage *message) {
    if (enemy_id >= 0 && enemy_id < MAX_ENEMY_PROCESSES) {
        fd_set readfds;
        struct timeval tv;
        
        FD_ZERO(&readfds);
        FD_SET(enemy_to_main_pipe[enemy_id][0], &readfds);
        
        // Set timeout to 0 for non-blocking
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        
        int retval = select(enemy_to_main_pipe[enemy_id][0] + 1, &readfds, NULL, NULL, &tv);
        
        if (retval > 0) {
            if (read(enemy_to_main_pipe[enemy_id][0], message, sizeof(GameMessage)) == sizeof(GameMessage)) {
                return true;
            }
        }
    }
    
    return false;
}

// Send a message from an enemy process to the main process
void send_message_to_main(int enemy_id, GameMessage *message) {
    if (enemy_id >= 0 && enemy_id < MAX_ENEMY_PROCESSES) {
        ssize_t bytes_written = write(enemy_to_main_pipe[enemy_id][1], message, sizeof(GameMessage));
        if (bytes_written != sizeof(GameMessage)) {
            // Handle error - could not write full message
            fprintf(stderr, "Warning: Could not write full message to main process from enemy %d\n", enemy_id);
        }
    }
}

// Receive a message from the main process (non-blocking)
bool receive_message_from_main(int enemy_id, GameMessage *message) {
    if (enemy_id >= 0 && enemy_id < MAX_ENEMY_PROCESSES) {
        fd_set readfds;
        struct timeval tv;
        
        FD_ZERO(&readfds);
        FD_SET(main_to_enemy_pipe[enemy_id][0], &readfds);
        
        // Set timeout to 0 for non-blocking
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        
        int retval = select(main_to_enemy_pipe[enemy_id][0] + 1, &readfds, NULL, NULL, &tv);
        
        if (retval > 0) {
            if (read(main_to_enemy_pipe[enemy_id][0], message, sizeof(GameMessage)) == sizeof(GameMessage)) {
                return true;
            }
        }
    }
    
    return false;
}

// Broadcast player position to all enemy processes
void broadcast_player_position(int x, int y) {
    GameMessage message;
    message.from_id = 0; // From player
    message.to_id = -1;  // Broadcast
    message.message_type = MSG_POSITION_UPDATE;
    message.x = x;
    message.y = y;
    message.data = 0;
    
    for (int i = 0; i < num_enemy_processes; i++) {
        send_message_to_enemy(i, &message);
    }
}

// Main function for enemy process
void enemy_process_main(int enemy_id, EntityType enemy_type) {
    printf("Enemy %d process started (type: %d)\n", enemy_id, enemy_type);
    
    bool running = true;
    GameMessage message;
    int player_x = -1, player_y = -1;
    int move_counter = 0;
    time_t start_time = time(NULL);
    bool can_track_player = false;
    
    // Main enemy loop
    while (running) {
        // Check for messages from main process
        if (receive_message_from_main(enemy_id, &message)) {
            if (message.message_type == MSG_POSITION_UPDATE) {
                // Only update known player position if tracking is enabled
                if (can_track_player) {
                    player_x = message.x;
                    player_y = message.y;
                }
            } else if (message.message_type == MSG_GAME_OVER) {
                // Game over, exit the loop
                running = false;
            }
        }
        
        // Don't track the player for the first 5 seconds to give player time to move
        time_t current_time = time(NULL);
        if (!can_track_player && (current_time - start_time) >= 5) {
            can_track_player = true;
        }
        
        // Don't move every cycle - only every few cycles based on enemy type
        move_counter++;
        int move_frequency;
        
        switch (enemy_type) {
            case ENTITY_ENEMY_CHASE:
                move_frequency = 8; 
                break;
            case ENTITY_ENEMY_RANDOM:
                move_frequency = 10;
                break;
            case ENTITY_ENEMY_GUARD:
                move_frequency = 12;
                break;
            case ENTITY_ENEMY_SMART:
                move_frequency = 7;
                break;
            default:
                move_frequency = 10;
        }
        
        // Increase move frequency at the beginning to slow down enemies
        if (!can_track_player) {
            move_frequency *= 2;
        }
        
        if (move_counter >= move_frequency) {
            move_counter = 0;
            
            // Determine next move based on enemy type
            int dx = 0, dy = 0;
            
            lock_game_state();
            int enemy_x = game_state->enemies[enemy_id].x;
            int enemy_y = game_state->enemies[enemy_id].y;
            bool is_active = game_state->enemies[enemy_id].is_active;
            unlock_game_state();
            
            if (!is_active) {
                running = false;
                break;
            }
            
            // All enemies now have some ability to track the player
            if (player_x >= 0 && player_y >= 0) {
                // Calculate distance to player
                int dx_to_player = player_x - enemy_x;
                int dy_to_player = player_y - enemy_y;
                int dist_squared = dx_to_player*dx_to_player + dy_to_player*dy_to_player;
                
                switch (enemy_type) {
                    case ENTITY_ENEMY_CHASE:
                        // Always chase player if position is known
                        // Smart chase - prioritize the larger distance dimension
                        if (abs(dx_to_player) > abs(dy_to_player)) {
                            dx = (dx_to_player > 0) ? 1 : -1;
                        } else {
                            dy = (dy_to_player > 0) ? 1 : -1;
                        }
                        break;
                        
                    case ENTITY_ENEMY_RANDOM:
                        // Now follows player if close enough (aggressive random)
                        if (dist_squared < 100) { // Within range of ~10 tiles
                            // Simple chase for random when close
                            if (rand() % 2 == 0) {
                                dx = (dx_to_player > 0) ? 1 : -1;
                            } else {
                                dy = (dy_to_player > 0) ? 1 : -1;
                            }
                        } else {
                            // Random movement if player is far
                            int dir = rand() % 4;
                            if (dir == 0) dx = 1;
                            else if (dir == 1) dx = -1;
                            else if (dir == 2) dy = 1;
                            else dy = -1;
                        }
                        break;
                        
                    case ENTITY_ENEMY_GUARD:
                        // Now actively guards area but moves toward player if detected
                        if (dist_squared < 64) { // Within range of ~8 tiles
                            // Chase player if detected in guarded area
                            if (move_counter % 2 == 0) {
                                dx = (dx_to_player > 0) ? 1 : -1;
                            } else {
                                dy = (dy_to_player > 0) ? 1 : -1;
                            }
                        } else {
                            // Guard behavior - patrol in a small area
                            if (move_counter % 2 == 0) {
                                // Move horizontally in a pattern
                                if ((move_counter / 2) % 2 == 0) dx = 1;
                                else dx = -1;
                            } else {
                                // Move vertically in a pattern
                                if ((move_counter / 2) % 2 == 0) dy = 1;
                                else dy = -1;
                            }
                        }
                        break;
                        
                    case ENTITY_ENEMY_SMART:
                        // Smart enemy tries to predict and intercept player's path
                        // First, determine if player is moving primarily horizontally or vertically
                        // We'll use the last known positions to estimate this
                        static int last_player_x = -1;
                        static int last_player_y = -1;
                        
                        if (last_player_x != -1 && last_player_y != -1) {
                            int player_dx = player_x - last_player_x;
                            int player_dy = player_y - last_player_y;
                            
                            // Try to intercept player by predicting where they're going
                            if (abs(player_dx) > abs(player_dy) && player_dx != 0) {
                                // Player moving horizontally
                                // Move to intercept a few tiles ahead in x direction
                                int intercept_x = player_x + player_dx * 3;
                                int dx_to_intercept = intercept_x - enemy_x;
                                
                                if (dx_to_intercept != 0) {
                                    dx = (dx_to_intercept > 0) ? 1 : -1;
                                } else {
                                    // Already at intercept x, move in y direction toward player
                                    dy = (dy_to_player > 0) ? 1 : -1;
                                }
                            } else if (player_dy != 0) {
                                // Player moving vertically
                                // Move to intercept a few tiles ahead in y direction
                                int intercept_y = player_y + player_dy * 3;
                                int dy_to_intercept = intercept_y - enemy_y;
                                
                                if (dy_to_intercept != 0) {
                                    dy = (dy_to_intercept > 0) ? 1 : -1;
                                } else {
                                    // Already at intercept y, move in x direction toward player
                                    dx = (dx_to_player > 0) ? 1 : -1;
                                }
                            } else {
                                // Player not moving or first time seeing player
                                // Use standard chase logic but faster
                                if (rand() % 3 == 0) {
                                    // Sometimes move diagonally for smarter movement
                                    dx = (dx_to_player > 0) ? 1 : -1;
                                    dy = (dy_to_player > 0) ? 1 : -1;
                                } else {
                                    // Prioritize larger distance axis
                                    if (abs(dx_to_player) > abs(dy_to_player)) {
                                        dx = (dx_to_player > 0) ? 1 : -1;
                                    } else {
                                        dy = (dy_to_player > 0) ? 1 : -1;
                                    }
                                }
                            }
                        } else {
                            // First time seeing player, use standard chase
                            if (abs(dx_to_player) > abs(dy_to_player)) {
                                dx = (dx_to_player > 0) ? 1 : -1;
                            } else {
                                dy = (dy_to_player > 0) ? 1 : -1;
                            }
                        }
                        
                        // Update last known player position
                        last_player_x = player_x;
                        last_player_y = player_y;
                        break;
                        
                    default:
                        // Default chase behavior for unknown types
                        if (enemy_x < player_x) dx = 1;
                        else if (enemy_x > player_x) dx = -1;
                        else if (enemy_y < player_y) dy = 1;
                        else if (enemy_y > player_y) dy = -1;
                        break;
                }
            } else {
                // No player position known, use random movement for all types
                int dir = rand() % 4;
                if (dir == 0) dx = 1;
                else if (dir == 1) dx = -1;
                else if (dir == 2) dy = 1;
                else dy = -1;
            }
            
            // Check if the move is valid
            lock_game_state();
            int new_x = enemy_x + dx;
            int new_y = enemy_y + dy;
            
            // Boundary check
            if (new_x > 0 && new_x < game_state->map.width - 1 && 
                new_y > 0 && new_y < game_state->map.height - 1) {
                
                // Check if the tile is walkable
                if (game_state->map.tiles[new_y][new_x] != TILE_WALL) {
                    // Update enemy position
                    game_state->enemies[enemy_id].x = new_x;
                    game_state->enemies[enemy_id].y = new_y;
                    
                    // Check for collision with player
                    for (int i = 0; i < game_state->num_players; i++) {
                        if (game_state->players[i].is_active &&
                            game_state->players[i].x == new_x &&
                            game_state->players[i].y == new_y) {
                            
                            // Hit player - send message to main process
                            game_state->player_hit = true;
                            
                            GameMessage hit_message;
                            hit_message.from_id = enemy_id;
                            hit_message.to_id = i;
                            hit_message.message_type = MSG_PLAYER_HIT;
                            hit_message.x = new_x;
                            hit_message.y = new_y;
                            hit_message.data = 5; // Reduced damage amount from 10 to 5
                            
                            unlock_game_state();
                            send_message_to_main(enemy_id, &hit_message);
                            lock_game_state();
                        }
                    }
                }
            }
            unlock_game_state();
            
            // Send movement message to main process
            GameMessage move_message;
            move_message.from_id = enemy_id;
            move_message.to_id = 0;
            move_message.message_type = MSG_ENEMY_MOVE;
            move_message.x = new_x;
            move_message.y = new_y;
            move_message.data = 0;
            
            send_message_to_main(enemy_id, &move_message);
        }
        
        // Sleep to avoid using 100% CPU
        usleep(50000); // 50ms
    }
    
    printf("Enemy %d process ended\n", enemy_id);
}

// Check player-enemy collision and apply damage
void check_player_enemy_collision(GameState *state) {
    if (!state) return;
    
    static time_t last_hit_time = 0;
    time_t game_time = state->current_time - state->start_time;
    
    // Check if player is in starting safe zone (near starting position)
    bool in_safe_zone = false;
    const int SAFE_ZONE_RADIUS = 5;
    const int STARTING_X = 4;
    const int STARTING_Y = 4;
    
    if (abs(state->players[0].x - STARTING_X) <= SAFE_ZONE_RADIUS && 
        abs(state->players[0].y - STARTING_Y) <= SAFE_ZONE_RADIUS) {
        in_safe_zone = true;
    }
    
    // Safe zone immunity: no damage in starting area and within first 3 seconds
    if (game_time < 3 || in_safe_zone) {
        state->player_hit = false;
        return;
    }
    
    // Invulnerability after each hit to avoid rapid damage
    if ((game_time - last_hit_time) < 1) {
        // Clear the hit flag during invulnerability period
        state->player_hit = false;
        return;
    }
    
    if (state->player_hit) {
        // Player was hit - reduce health
        state->players[0].health -= 5; // Damage amount
        
        // Reset the hit flag
        state->player_hit = false;
        
        // Record when the hit occurred for invulnerability period
        last_hit_time = game_time;
        
        // Check if player died
        if (state->players[0].health <= 0) {
            state->players[0].health = 0;
            state->players[0].is_active = false;
            state->game_over = true;
            state->winner_id = -1; // No winner, player lost
        }
    }
}

// Main function for player process
void player_process_main(int player_id) {
    printf("Player %d process started\n", player_id);
    
    // Set specific environment variables for this process
    char display_env[64]; // Increased buffer size to avoid overflow
    sprintf(display_env, "SDL_VIDEO_WINDOW_POS=%d,%d", 50 + 100 * player_id, 50 + 50 * player_id);
    putenv(display_env);
    
    // In a WSL environment, make sure we're using the right display
    putenv("DISPLAY=:0");
    
    // Add a small delay to stagger window creation between processes
    usleep(500000 * player_id); // 500ms delay per player ID
    
    // Initialize SDL in the player process
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    
    char window_title[64]; // Increased buffer size from 32 to 64
    sprintf(window_title, "Dungeon Conquerors - Player %d", player_id + 1);
    
    window = SDL_CreateWindow(
        window_title,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    
    if (window == NULL) {
        printf("Error creating window: %s\n", SDL_GetError());
        return;
    }
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Error creating renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        return;
    }
    
    // Player game loop
    bool running = true;
    SDL_Event event;
    
    while (running) {
        // Process events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else {
                // Process player input (locking is done inside this function)
                process_player_input(&event, game_state, player_id);
            }
        }
        
        // Render game
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        lock_game_state();
        render_game(renderer, game_state, player_id);
        
        // Check if game is over
        if (game_state->game_over) {
            running = false;
        }
        
        unlock_game_state();
        
        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }
    
    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    
    printf("Player %d process ended\n", player_id);
} 