#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <stdbool.h>
#include <sys/types.h>
#include <semaphore.h>
#include "game.h"

// Shared memory key - will be generated at runtime using ftok
#define SHM_PATH "/etc/passwd"  // Use a file that's guaranteed to exist
#define SHM_ID 'D'

// Semaphore name
#define SEM_NAME "/dungeon_conquerors_sem"

// Shared memory and semaphore handles
extern int shm_id;
extern sem_t* sem_id;
extern GameState* game_state;

// Function declarations
bool init_shared_memory(void);
void cleanup_shared_memory(void);
void lock_game_state(void);
void unlock_game_state(void);
GameState* get_game_state(void);

#endif /* SHARED_MEMORY_H */ 