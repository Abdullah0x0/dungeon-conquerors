# Dungeon Conquerors

A multi-process, dungeon exploration game that demonstrates key operating system concepts through interactive gameplay.

## Demo Video
[Demo Video Link] - Upload your demo to YouTube and paste the link here

## Game Overview

Dungeon Conquerors is a challenging top-down dungeon exploration game where the player must navigate a procedurally generated maze, collect keys, and find the exit while avoiding deadly enemies. Each enemy operates as an independent process with its own AI behavior, hunting the player throughout the dungeon. The game demonstrates multiple operating system concepts in action through its multi-process architecture and resource sharing mechanisms.

## Game Design

### Core Mechanics

1. **Movement**
   - Player moves on a grid-based system
   - Four-directional movement (up, down, left, right)
   - Collision detection with walls and enemies
   - Smooth animations for movement

2. **Level Progression**
   - Two levels with increasing difficulty
   - Level 1: Requires 5 keys
   - Level 2: Requires 7 keys
   - Procedurally generated maps
   - Exit only becomes accessible after collecting all required keys

3. **Resource Management**
   - Health system (starts at 100)
   - Score tracking
   - Key collection
   - Limited safe zones

### Game Elements

1. **Tiles**
   - Empty tiles (walkable)
   - Walls (block movement)
   - Doors (can be opened)
   - Treasure (grants score)
   - Keys (required for exit)
   - Exit (win condition)

2. **Enemies**
   - **Chaser (Purple)**
     - Fastest movement speed
     - Actively pursues player
     - Single large eye
     - Most aggressive behavior

   - **Random (Orange)**
     - Medium movement speed
     - Random movement pattern
     - Two small eyes
     - Becomes aggressive when player is close

   - **Guard (Cyan)**
     - Slower movement speed
     - Patrols specific areas
     - Horizontal bar eyes
     - Chases when player enters detection range

   - **Smart (Yellow)**
     - Fast movement speed
     - Predicts player movement
     - Eyes with red pupils
     - Tries to intercept player's path

3. **Player Abilities**
   - Movement in four directions
   - Key collection
   - Treasure collection
   - Temporary invulnerability after taking damage

### Game Flow

1. **Level Start**
   - Player spawns in safe zone
   - 3-second invulnerability period
   - Enemies spawn in different map regions
   - Keys and treasures are placed randomly

2. **Gameplay Loop**
   - Collect keys while avoiding enemies
   - Gather treasures for higher score
   - Navigate through doors and obstacles
   - Manage health and avoid enemy contact
   - Find and reach the exit

3. **Level Completion**
   - Collect all required keys
   - Exit becomes accessible
   - Reach exit to complete level
   - Progress to next level or win game

### Technical Features

1. **Graphics**
   - Modern color palette
   - Particle effects for various actions
   - Enemy animations and visual effects
   - UI elements with gradients and animations

2. **AI System**
   - Different enemy behaviors
   - Path prediction for smart enemies
   - Detection ranges and aggression levels
   - Enemy movement patterns

3. **Performance**
   - Efficient collision detection
   - Optimized rendering
   - Smooth animations
   - Responsive controls

## OS Concepts Used

### 1. Process Creation (fork)
**Corresponding Game Mechanics**: Each enemy operates as an independent process, allowing for true parallel AI behavior.

**Code Snippet**:
```c
// In process.c (create_enemy_processes function)
// Fork to create enemy process
pid_t pid = fork();

if (pid == -1) {
    perror("fork failed for enemy process");
    continue;
} else if (pid == 0) {
    // Child process - enemy AI
    enemy_process_main(i, game_state->enemies[i].type);
    exit(0);
} else {
    // Parent process
    enemy_pids[i] = pid;
    num_enemy_processes++;
}
```

### 2. Inter-Process Communication (Pipes)
**Corresponding Game Mechanics**: The main game process and enemy processes communicate through pipes to exchange information about positions, movements, and collisions.

**Code Snippet**:
```c
// In process.c (setup and use of pipes)
// Create pipes for IPC
if (pipe(main_to_enemy_pipe[i]) == -1 || pipe(enemy_to_main_pipe[i]) == -1) {
    perror("Failed to create pipes for enemy process");
    continue;
}

// Send a message to an enemy process
void send_message_to_enemy(int enemy_id, GameMessage *message) {
    if (enemy_id >= 0 && enemy_id < MAX_ENEMY_PROCESSES) {
        ssize_t bytes_written = write(main_to_enemy_pipe[enemy_id][1], message, sizeof(GameMessage));
        if (bytes_written != sizeof(GameMessage)) {
            fprintf(stderr, "Warning: Could not write full message to enemy process %d\n", enemy_id);
        }
    }
}
```

### 3. Shared Memory
**Corresponding Game Mechanics**: The game state (map, player positions, enemy positions, etc.) is shared between all processes using System V shared memory.

**Code Snippet**:
```c
// In shared_memory.c (init_shared_memory function)
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
```

### 4. Synchronization (Semaphores)
**Corresponding Game Mechanics**: POSIX semaphores ensure that when multiple processes try to modify the game state simultaneously, they do so in a safe, coordinated manner.

**Code Snippet**:
```c
// In shared_memory.c (using semaphores for synchronization)
// Create POSIX semaphore for synchronization
sem_unlink(SEM_NAME);
sem_id = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0666, 1);

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
```

### 5. Threading (POSIX Threads)
**Corresponding Game Mechanics**: A background thread runs parallel to the main game loop, adding random treasures, updating the game timer, and checking game end conditions.

**Code Snippet**:
```c
// In game.c (creating background thread)
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

// Clean up thread resources
void game_cleanup(void) {
    if (background_thread_running) {
        background_thread_running = false;
        usleep(100000); // 100ms
        pthread_join(background_thread, NULL);
    }
}
```

### 6. Signal Handling
**Corresponding Game Mechanics**: Proper handling of termination signals (Ctrl+C) ensures the game shuts down gracefully, cleaning up all resources.

**Code Snippet**:
```c
// In main.c (signal handling setup)
// Global flag for handling signals
volatile sig_atomic_t terminate_flag = 0;

// Signal handler
void handle_signal(int sig) {
    (void)sig; // Mark parameter as unused
    terminate_flag = 1;
}

// Set up signal handlers
signal(SIGINT, handle_signal);
signal(SIGTERM, handle_signal);

// Clean up processes on termination
void cleanup_ipc_channels(void) {
    // Terminate all enemy processes
    for (int i = 0; i < num_enemy_processes; i++) {
        if (enemy_pids[i] > 0) {
            kill(enemy_pids[i], SIGTERM);
        }
    }
}
```

### 7. Non-blocking I/O
**Corresponding Game Mechanics**: The game uses non-blocking I/O to check for messages from enemy processes without interrupting the main game loop flow.

**Code Snippet**:
```c
// In process.c (receive_message_from_enemy function)
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
```

### 8. Process Management
**Corresponding Game Mechanics**: The game manages multiple processes, including proper cleanup and resource management.

**Code Snippet**:
```c
// In process.c (process management)
// Wait for enemy processes to finish with timeout
int wait_count = 0;
while (wait_count < 10) {
    bool all_done = true;
    for (int i = 0; i < num_enemy_processes; i++) {
        if (enemy_pids[i] > 0) {
            int status;
            pid_t result = waitpid(enemy_pids[i], &status, WNOHANG);
            if (result == 0) {
                all_done = false;
            } else if (result > 0) {
                enemy_pids[i] = -1;
            }
        }
    }
    if (all_done) break;
    usleep(100000); // 100ms
    wait_count++;
}

// Force terminate any remaining processes
for (int i = 0; i < num_enemy_processes; i++) {
    if (enemy_pids[i] > 0) {
        kill(enemy_pids[i], SIGKILL);
    }
}
```

### 9. Resource Cleanup
**Corresponding Game Mechanics**: Proper cleanup of system resources (shared memory, semaphores, pipes) when the game ends.

**Code Snippet**:
```c
// In shared_memory.c (cleanup_shared_memory function)
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
}
```

## Building and Running

### Prerequisites
- SDL2 library
- POSIX threads
- System V IPC
- POSIX semaphores
- C compiler (GCC recommended)
- Make

On Ubuntu/Debian, install dependencies with:
```bash
sudo apt-get install libsdl2-dev
```

### Build Instructions
```bash
make clean
make
```

### Run the Game
```bash
./dungeon_conquerors
```

## Controls

- Arrow Keys: Move player
- ESC: Exit game
- Any key: Skip welcome screen

## Game Rules

1. **Health System**
   - Start with 100 health
   - Lose 5 health per enemy hit
   - 1-second invulnerability after taking damage
   - Game over when health reaches 0

2. **Scoring**
   - Collect treasures for points
   - Higher score for faster completion
   - Bonus points for collecting extra treasures

3. **Level Requirements**
   - Level 1: Collect 5 keys
   - Level 2: Collect 7 keys
   - Exit remains locked until all keys are collected

## Development

### Project Structure
```
dungeon_conquerors/
├── include/         # Header files
├── src/            # Source files
├── obj/            # Object files
└── Makefile        # Build configuration
```

### Key Files
- `game.h`: Game structures and constants
- `game.c`: Core game logic
- `process.c`: Process management and AI
- `main.c`: Main game loop and rendering

## Future Enhancements

1. **Potential Features**
   - Additional enemy types
   - Power-ups and special abilities
   - More levels
   - Save/load system
   - High score tracking
   - Sound system


## Development and Design Challenges

Developing Dungeon Conquerors presented several interesting challenges:

1. **Process Coordination**: Ensuring multiple processes could work together seamlessly required careful synchronization
2. **Resource Management**: Properly acquiring and releasing system resources to prevent leaks or deadlocks
3. **Parallel AI**: Creating independent enemy behaviors that could run truly in parallel
4. **Inter-Process Communication**: Designing an efficient message passing system between processes
5. **Game Balance**: Creating challenging but fair gameplay with the constraints of a multi-process architecture

## Credits

Developed as a demonstration of operating system concepts in action, showing how fundamental OS principles can be applied in a practical, interactive application.

## License

This project is licensed under the MIT License - see the LICENSE file for details. 