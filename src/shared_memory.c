#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>           /* For O_* constants */
#include <semaphore.h>
#include <errno.h>
#include "../include/shared_memory.h"
#include "../include/game.h"

// Shared memory and semaphore handles
int shm_id = -1;
sem_t* sem_id = NULL;
GameState* game_state = NULL;

// Initialize shared memory for game state
bool init_shared_memory(void) {
    // Generate a key using ftok
    key_t key = ftok(SHM_PATH, SHM_ID);
    if (key == -1) {
        perror("ftok failed");
        return false;
    }
    
    // Create shared memory segment
    shm_id = shmget(key, sizeof(GameState), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget failed");
        return false;
    }

    // Attach to shared memory segment
    game_state = (GameState*)shmat(shm_id, NULL, 0);
    if (game_state == (void*)-1) {
        perror("shmat failed");
        return false;
    }

    // Initialize game state in shared memory
    memset(game_state, 0, sizeof(GameState));
    game_state->map.width = MAP_WIDTH;
    game_state->map.height = MAP_HEIGHT;
    game_state->game_over = false;
    
    // Initialize timer
    game_state->start_time = time(NULL);
    game_state->current_time = game_state->start_time;
    game_state->exit_enabled = false;
    game_state->keys_required = 5;  // Player needs to collect 5 keys
    game_state->keys_collected = 0;
    
    // Initialize map with walls around the edges
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (x == 0 || y == 0 || x == MAP_WIDTH-1 || y == MAP_HEIGHT-1) {
                game_state->map.tiles[y][x] = TILE_WALL;
            } else {
                game_state->map.tiles[y][x] = TILE_EMPTY;
            }
        }
    }
    
    // Create a more complex maze using a cellular automaton approach
    // First, randomly place walls
    for (int y = 1; y < MAP_HEIGHT-1; y++) {
        for (int x = 1; x < MAP_WIDTH-1; x++) {
            // Keep starting area clear (top-left corner)
            if (x < 5 && y < 5) continue;
            
            // 30% chance of a wall
            if (rand() % 100 < 30) {
                game_state->map.tiles[y][x] = TILE_WALL;
            }
        }
    }
    
    // Smooth the maze using a cellular automaton approach
    for (int iteration = 0; iteration < 3; iteration++) {
        // Create a temporary copy of the map
        TileType temp_map[MAP_HEIGHT][MAP_WIDTH];
        memcpy(temp_map, game_state->map.tiles, sizeof(temp_map));
        
        for (int y = 1; y < MAP_HEIGHT-1; y++) {
            for (int x = 1; x < MAP_WIDTH-1; x++) {
                // Count walls in 3x3 neighborhood
                int wall_count = 0;
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (game_state->map.tiles[y+dy][x+dx] == TILE_WALL) {
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
        memcpy(game_state->map.tiles, temp_map, sizeof(temp_map));
    }
    
    // Ensure the player's starting position is clear
    for (int y = 1; y < 4; y++) {
        for (int x = 1; x < 4; x++) {
            game_state->map.tiles[y][x] = TILE_EMPTY;
        }
    }
    
    // Add doors to create sections
    for (int i = 0; i < 15; i++) {
        int x = 10 + rand() % (MAP_WIDTH - 20);
        int y = 10 + rand() % (MAP_HEIGHT - 20);
        game_state->map.tiles[y][x] = TILE_DOOR;
    }
    
    // Add keys in different areas of the map (far from start)
    for (int i = 0; i < game_state->keys_required; i++) {
        int x, y;
        do {
            // Place keys farther from the starting point
            x = 20 + rand() % (MAP_WIDTH - 25);
            y = 20 + rand() % (MAP_HEIGHT - 25);
        } while (game_state->map.tiles[y][x] != TILE_EMPTY);
        
        game_state->map.tiles[y][x] = TILE_KEY;
    }
    
    // Add treasures - more of them for higher scores
    for (int i = 0; i < MAP_WIDTH * MAP_HEIGHT / 50; i++) {
        int x = rand() % (MAP_WIDTH - 2) + 1;
        int y = rand() % (MAP_HEIGHT - 2) + 1;
        if (game_state->map.tiles[y][x] == TILE_EMPTY) {
            game_state->map.tiles[y][x] = TILE_TREASURE;
        }
    }
    
    // Add exit in the far corner - initially inaccessible until keys are collected
    game_state->map.tiles[MAP_HEIGHT-2][MAP_WIDTH-2] = TILE_EMPTY; // Clear any walls near exit
    game_state->map.tiles[MAP_HEIGHT-3][MAP_WIDTH-2] = TILE_EMPTY;
    game_state->map.tiles[MAP_HEIGHT-2][MAP_WIDTH-3] = TILE_EMPTY;
    game_state->map.tiles[MAP_HEIGHT-2][MAP_WIDTH-2] = TILE_EXIT;
    
    // Create POSIX semaphore for synchronization
    // First unlink any existing semaphore with the same name
    sem_unlink(SEM_NAME);
    
    sem_id = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0666, 1);
    if (sem_id == SEM_FAILED) {
        perror("sem_open failed");
        shmdt(game_state);
        shmctl(shm_id, IPC_RMID, NULL);
        return false;
    }
    
    return true;
}

// Clean up shared memory resources
void cleanup_shared_memory(void) {
    // Close semaphore first
    if (sem_id != NULL && sem_id != SEM_FAILED) {
        sem_close(sem_id);
        sem_unlink(SEM_NAME);
        sem_id = NULL;
    }
    
    // Then detach from shared memory
    if (game_state != NULL && game_state != (void*)-1) {
        shmdt(game_state);
        game_state = NULL;
    }
    
    // Finally remove the shared memory segment
    if (shm_id != -1) {
        shmctl(shm_id, IPC_RMID, NULL);
        shm_id = -1;
    }
    
    printf("Shared memory cleanup completed\n");
}

// Lock game state for exclusive access
void lock_game_state(void) {
    if (sem_id != NULL && sem_id != SEM_FAILED) {
        if (sem_wait(sem_id) == -1) {
            perror("sem_wait (lock) failed");
        }
    }
}

// Unlock game state
void unlock_game_state(void) {
    if (sem_id != NULL && sem_id != SEM_FAILED) {
        if (sem_post(sem_id) == -1) {
            perror("sem_post (unlock) failed");
        }
    }
}

// Get a pointer to the game state
GameState* get_game_state(void) {
    return game_state;
} 