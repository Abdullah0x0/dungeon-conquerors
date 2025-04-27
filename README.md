# Dungeon Conquerors

A multi-process, dungeon exploration game that demonstrates key operating system concepts through interactive gameplay.

## Demo Video
[Demo Video Link](https://youtu.be/2FhhNZob0Xk)

## Setup
### Compilation and Running
```bash
# Clone the repository
git clone [repository-url]
cd dungeon_conquerors

# Compile the game
make clean && make

# Run the game
./dungeon_conquerors
```

### Dependencies
- SDL2 library
- POSIX threads
- System V IPC
- POSIX semaphores

On Ubuntu/Debian, install dependencies with:
```bash
sudo apt-get install libsdl2-dev
```

## Game Overview

### Game Title: Dungeon Conquerors

### Game Summary
Dungeon Conquerors is a challenging top-down dungeon exploration game where the player must navigate a procedurally generated maze, collect keys, and find the exit while avoiding deadly enemies. Each enemy operates as an independent process with its own AI behavior, hunting the player throughout the dungeon. The game demonstrates multiple operating system concepts in action through its multi-process architecture and resource sharing mechanisms.

### Core Gameplay Loop
1. Explore the procedurally generated dungeon
2. Collect the keys hidden throughout the maze
3. Find and open doors for bonuses (health, score, extra keys)
4. Avoid or outmaneuver various enemy types
5. Reach the exit once all keys are collected
6. Compare your score with friends

## Gameplay Mechanics

### Controls
- **Arrow Keys**: Move the player (up, down, left, right)
- **ESC**: Quit the game
- **Any Key**: Dismiss the welcome screen

### Core Mechanics
- **Key Collection**: Gather 5 keys to unlock the exit
- **Door Interactions**: Opening doors provides random bonuses (health, score, extra keys)
- **Enemy Avoidance**: Different enemy types have unique movement patterns:
  - **Chasers**: Directly pursue the player
  - **Random**: Move randomly, but follow player when nearby
  - **Guards**: Patrol an area, chase when player enters their territory
  - **Smart**: Predict player movement and try to intercept

### Level Progression
- Single procedurally generated level with increasing difficulty as enemies approach
- Game timer tracks how long you take to escape

### Win/Loss Conditions
- **Win**: Collect all 5 keys and reach the exit
- **Loss**: Lose all health from enemy collisions

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
        write(main_to_enemy_pipe[enemy_id][1], message, sizeof(GameMessage));
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
    terminate_flag = 1;
}

// Set up signal handlers
signal(SIGINT, handle_signal);
signal(SIGTERM, handle_signal);
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
        // ... process results
    }
}
```

## Development and Design Challenges

Developing Dungeon Conquerors presented several interesting challenges:

1. **Process Coordination**: Ensuring multiple processes could work together seamlessly required careful synchronization
2. **Resource Management**: Properly acquiring and releasing system resources to prevent leaks or deadlocks
3. **Parallel AI**: Creating independent enemy behaviors that could run truly in parallel
4. **Inter-Process Communication**: Designing an efficient message passing system between processes
5. **Game Balance**: Creating challenging but fair gameplay with the constraints of a multi-process architecture

## Future Enhancements

Potential additions to enhance the game:
- Multiplayer support (multiple player processes)
- Additional enemy types with more complex behaviors
- More sophisticated level generation
- Power-ups and special abilities
- Boss battles

## Credits

Developed as a demonstration of operating system concepts in action, showing how fundamental OS principles can be applied in a practical, interactive application. 
