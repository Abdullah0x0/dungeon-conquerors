#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <SDL2/SDL.h>
#include "../include/game.h"
#include "../include/shared_memory.h"
#include "../include/process.h"

// Global flag for handling signals
volatile sig_atomic_t terminate_flag = 0;

// Signal handler
void handle_signal(int sig) {
    (void)sig; // Mark parameter as unused
    terminate_flag = 1;
}

// Draw a simple character using lines
void draw_simple_char(SDL_Renderer* renderer, int x, int y, char c, int size) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    
    switch(c) {
        case 'A':
            SDL_RenderDrawLine(renderer, x, y + size, x + size/2, y);
            SDL_RenderDrawLine(renderer, x + size/2, y, x + size, y + size);
            SDL_RenderDrawLine(renderer, x + size/4, y + size/2, x + 3*size/4, y + size/2);
            break;
        case 'B':
            SDL_RenderDrawLine(renderer, x, y, x, y + size);
            SDL_RenderDrawLine(renderer, x, y, x + 2*size/3, y);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y, x + size, y + size/4);
            SDL_RenderDrawLine(renderer, x + size, y + size/4, x + 2*size/3, y + size/2);
            SDL_RenderDrawLine(renderer, x, y + size/2, x + 2*size/3, y + size/2);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y + size/2, x + size, y + 3*size/4);
            SDL_RenderDrawLine(renderer, x + size, y + 3*size/4, x + 2*size/3, y + size);
            SDL_RenderDrawLine(renderer, x, y + size, x + 2*size/3, y + size);
            break;
        case 'C':
            SDL_RenderDrawLine(renderer, x + size, y, x + size/3, y);
            SDL_RenderDrawLine(renderer, x + size/3, y, x, y + size/3);
            SDL_RenderDrawLine(renderer, x, y + size/3, x, y + 2*size/3);
            SDL_RenderDrawLine(renderer, x, y + 2*size/3, x + size/3, y + size);
            SDL_RenderDrawLine(renderer, x + size/3, y + size, x + size, y + size);
            break;
        case 'E':
            SDL_RenderDrawLine(renderer, x, y, x, y + size);
            SDL_RenderDrawLine(renderer, x, y, x + size, y);
            SDL_RenderDrawLine(renderer, x, y + size/2, x + 2*size/3, y + size/2);
            SDL_RenderDrawLine(renderer, x, y + size, x + size, y + size);
            break;
        case 'F':
            SDL_RenderDrawLine(renderer, x, y, x, y + size);
            SDL_RenderDrawLine(renderer, x, y, x + size, y);
            SDL_RenderDrawLine(renderer, x, y + size/2, x + 2*size/3, y + size/2);
            break;
        case 'I':
            SDL_RenderDrawLine(renderer, x, y, x + size, y);
            SDL_RenderDrawLine(renderer, x + size/2, y, x + size/2, y + size);
            SDL_RenderDrawLine(renderer, x, y + size, x + size, y + size);
            break;
        case 'K':
            SDL_RenderDrawLine(renderer, x, y, x, y + size);
            SDL_RenderDrawLine(renderer, x, y + size/2, x + size, y);
            SDL_RenderDrawLine(renderer, x, y + size/2, x + size, y + size);
            break;
        case 'L':
            SDL_RenderDrawLine(renderer, x, y, x, y + size);
            SDL_RenderDrawLine(renderer, x, y + size, x + size, y + size);
            break;
        case 'N':
            SDL_RenderDrawLine(renderer, x, y, x, y + size);
            SDL_RenderDrawLine(renderer, x, y, x + size, y + size);
            SDL_RenderDrawLine(renderer, x + size, y, x + size, y + size);
            break;
        case 'O':
            SDL_RenderDrawLine(renderer, x + size/3, y, x + 2*size/3, y);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y, x + size, y + size/3);
            SDL_RenderDrawLine(renderer, x + size, y + size/3, x + size, y + 2*size/3);
            SDL_RenderDrawLine(renderer, x + size, y + 2*size/3, x + 2*size/3, y + size);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y + size, x + size/3, y + size);
            SDL_RenderDrawLine(renderer, x + size/3, y + size, x, y + 2*size/3);
            SDL_RenderDrawLine(renderer, x, y + 2*size/3, x, y + size/3);
            SDL_RenderDrawLine(renderer, x, y + size/3, x + size/3, y);
            break;
        case 'P':
            SDL_RenderDrawLine(renderer, x, y, x, y + size);
            SDL_RenderDrawLine(renderer, x, y, x + 2*size/3, y);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y, x + size, y + size/4);
            SDL_RenderDrawLine(renderer, x + size, y + size/4, x + 2*size/3, y + size/2);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y + size/2, x, y + size/2);
            break;
        case 'R':
            SDL_RenderDrawLine(renderer, x, y, x, y + size);
            SDL_RenderDrawLine(renderer, x, y, x + 2*size/3, y);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y, x + size, y + size/4);
            SDL_RenderDrawLine(renderer, x + size, y + size/4, x + 2*size/3, y + size/2);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y + size/2, x, y + size/2);
            SDL_RenderDrawLine(renderer, x + size/2, y + size/2, x + size, y + size);
            break;
        case 'S':
            SDL_RenderDrawLine(renderer, x + size, y, x + size/3, y);
            SDL_RenderDrawLine(renderer, x + size/3, y, x, y + size/3);
            SDL_RenderDrawLine(renderer, x, y + size/3, x + size/3, y + size/2);
            SDL_RenderDrawLine(renderer, x + size/3, y + size/2, x + 2*size/3, y + size/2);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y + size/2, x + size, y + 2*size/3);
            SDL_RenderDrawLine(renderer, x + size, y + 2*size/3, x + 2*size/3, y + size);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y + size, x, y + size);
            break;
        case 'T':
            SDL_RenderDrawLine(renderer, x, y, x + size, y);
            SDL_RenderDrawLine(renderer, x + size/2, y, x + size/2, y + size);
            break;
        case 'W':
            SDL_RenderDrawLine(renderer, x, y, x + size/4, y + size);
            SDL_RenderDrawLine(renderer, x + size/4, y + size, x + size/2, y + size/2);
            SDL_RenderDrawLine(renderer, x + size/2, y + size/2, x + 3*size/4, y + size);
            SDL_RenderDrawLine(renderer, x + 3*size/4, y + size, x + size, y);
            break;
        case 'Y':
            SDL_RenderDrawLine(renderer, x, y, x + size/2, y + size/2);
            SDL_RenderDrawLine(renderer, x + size, y, x + size/2, y + size/2);
            SDL_RenderDrawLine(renderer, x + size/2, y + size/2, x + size/2, y + size);
            break;
        case '!':
            SDL_RenderDrawLine(renderer, x + size/2, y, x + size/2, y + 2*size/3);
            SDL_RenderDrawLine(renderer, x + size/2, y + 4*size/5, x + size/2, y + size);
            break;
        case '.':
            SDL_RenderDrawLine(renderer, x + size/2, y + 4*size/5, x + size/2, y + size);
            break;
        case ' ':
            // Nothing for space
            break;
        case '5':
            SDL_RenderDrawLine(renderer, x, y, x + size, y);
            SDL_RenderDrawLine(renderer, x, y, x, y + size/2);
            SDL_RenderDrawLine(renderer, x, y + size/2, x + 2*size/3, y + size/2);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y + size/2, x + size, y + 3*size/4);
            SDL_RenderDrawLine(renderer, x + size, y + 3*size/4, x + 2*size/3, y + size);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y + size, x + size/3, y + size);
            break;
        default:
            // Default unknown character
            SDL_RenderDrawLine(renderer, x, y, x + size, y + size);
            SDL_RenderDrawLine(renderer, x, y + size, x + size, y);
            break;
    }
}

// Draw a simple text string
void draw_simple_text(SDL_Renderer* renderer, int x, int y, const char* text, int size) {
    int spacing = size + size/3;
    int cx = x;
    
    for (int i = 0; text[i] != '\0'; i++) {
        draw_simple_char(renderer, cx, y, text[i], size);
        cx += spacing;
    }
}

int main(int argc, char* argv[]) {
    // Mark parameters as unused to suppress warnings
    (void)argc;
    (void)argv;
    
    printf("Dungeon Conquerors\n");
    
    // Set up signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        printf("Error initializing SDL: %s\n", SDL_GetError());
        return 1;
    }
    
    printf("SDL initialized successfully\n");
    
    // Initialize game
    if (!game_init()) {
        printf("Failed to initialize game\n");
        SDL_Quit();
        return 1;
    }
    
    printf("Game initialized successfully\n");
    
    // Initialize shared memory for game state
    if (!init_shared_memory()) {
        printf("Failed to initialize shared memory\n");
        game_cleanup();
        SDL_Quit();
        return 1;
    }
    
    printf("Shared memory initialized successfully\n");
    
    // Set up IPC channels
    if (!setup_ipc_channels()) {
        printf("Failed to set up IPC channels\n");
        cleanup_shared_memory();
        game_cleanup();
        SDL_Quit();
        return 1;
    }
    
    printf("IPC channels initialized\n");
    
    // Initialize player data
    create_player_processes(1); // Create just the player data, not separate processes
    printf("Player initialized\n");
    
    // Create enemy AI processes
    create_enemy_processes(5); // Create 5 enemy processes
    printf("Enemy processes created\n");
    
    // Initialize SDL in the main process
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    
    printf("Creating window...\n");
    
    // Set display environment variable for WSL compatibility
    putenv("DISPLAY=:0");
    
    window = SDL_CreateWindow(
        "Dungeon Conquerors - Single Player with Enemies",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    
    if (window == NULL) {
        printf("Error creating window: %s\n", SDL_GetError());
        cleanup_ipc_channels();
        cleanup_shared_memory();
        game_cleanup();
        SDL_Quit();
        return 1;
    }
    
    printf("Window created successfully\n");
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Error creating renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        cleanup_ipc_channels();
        cleanup_shared_memory();
        game_cleanup();
        SDL_Quit();
        return 1;
    }
    
    printf("Renderer created successfully\n");
    
    // Game loop
    bool running = true;
    SDL_Event event;
    GameMessage message;
    
    printf("Starting game loop\n");
    
    // Show welcome message
    bool showing_welcome = true;
    int welcome_timer = 0;
    const int WELCOME_DURATION = 180; // Show for about 3 seconds (60 FPS * 3)
    
    // Pause enemies until welcome screen is dismissed
    lock_game_state();
    // Store the current time to calculate paused duration later
    time_t welcome_start_time = game_state->current_time;
    unlock_game_state();
    
    while (running && !terminate_flag) {
        // Process events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                printf("Quit event received\n");
                running = false;
            } else if (showing_welcome && event.type == SDL_KEYDOWN) {
                // Any key press skips the welcome message
                showing_welcome = false;
                
                // Resume normal game time tracking
                lock_game_state();
                time_t paused_duration = game_state->current_time - welcome_start_time;
                game_state->start_time += paused_duration; // Adjust start time to account for pause
                unlock_game_state();
            } else if (!showing_welcome) {
                // Only process game input after welcome message is gone
                // Process player input (locking is done inside this function)
                process_player_input(&event, game_state, 0);
                
                // Broadcast player position to enemy processes
                lock_game_state();
                int player_x = game_state->players[0].x;
                int player_y = game_state->players[0].y;
                unlock_game_state();
                
                broadcast_player_position(player_x, player_y);
            }
        }
        
        // Check for messages from enemy processes
        for (int i = 0; i < game_state->num_enemies; i++) {
            if (receive_message_from_enemy(i, &message)) {
                if (message.message_type == MSG_PLAYER_HIT && !showing_welcome) {
                    printf("Player hit by enemy %d!\n", i);
                    lock_game_state();
                    game_state->player_hit = true;
                    check_player_enemy_collision(game_state);
                    unlock_game_state();
                }
            }
        }
        
        // Render game
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        lock_game_state();
        
        if (showing_welcome) {
            // Draw welcome message instead of game
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            
            // Draw semi-transparent background for message
            SDL_SetRenderDrawColor(renderer, 0, 0, 100, 200);
            SDL_Rect msg_bg = {WINDOW_WIDTH / 6, WINDOW_HEIGHT / 3, WINDOW_WIDTH * 2 / 3, WINDOW_HEIGHT / 3};
            SDL_RenderFillRect(renderer, &msg_bg);
            
            // Draw message border
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_Rect border = {msg_bg.x - 2, msg_bg.y - 2, msg_bg.w + 4, msg_bg.h + 4};
            SDL_RenderDrawRect(renderer, &border);
            
            // Draw welcome messages using our simple text drawing
            draw_simple_text(renderer, WINDOW_WIDTH/6 + 20, WINDOW_HEIGHT/3 + 20, "COLLECT 5 KEYS", 15);
            draw_simple_text(renderer, WINDOW_WIDTH/6 + 20, WINDOW_HEIGHT/3 + 50, "TO OPEN THE EXIT", 15);
            draw_simple_text(renderer, WINDOW_WIDTH/6 + 20, WINDOW_HEIGHT/3 + 80, "BEWARE OF ENEMIES!", 15);
            draw_simple_text(renderer, WINDOW_WIDTH/6 + 80, WINDOW_HEIGHT/3 + 120, "PRESS ANY KEY", 10);
            
            // Increment timer and check if welcome should end
            welcome_timer++;
            if (welcome_timer >= WELCOME_DURATION) {
                showing_welcome = false;
            }
        } else {
            // Regular game rendering
            render_game(renderer, game_state, 0);
        }
        
        // Check if game is over
        if (game_state->game_over) {
            printf("Game over condition reached\n");
            
            // Display victory or defeat message
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
            SDL_Rect msg_bg = {WINDOW_WIDTH / 4, WINDOW_HEIGHT / 2 - 50, WINDOW_WIDTH / 2, 100};
            SDL_RenderFillRect(renderer, &msg_bg);
            
            // Instead of text, draw colored shapes to indicate victory/defeat
            if (game_state->winner_id == 0) {
                // Victory - Draw a green checkmark
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
                
                // Draw V shape for victory
                int center_x = WINDOW_WIDTH / 2;
                int center_y = WINDOW_HEIGHT / 2;
                int size = 30;
                
                // Left line of V
                SDL_RenderDrawLine(renderer, 
                    center_x - size, center_y - size,
                    center_x, center_y + size);
                
                // Right line of V
                SDL_RenderDrawLine(renderer, 
                    center_x, center_y + size,
                    center_x + size, center_y - size);
                
                // Make lines thicker
                for (int i = -2; i <= 2; i++) {
                    SDL_RenderDrawLine(renderer, 
                        center_x - size + i, center_y - size,
                        center_x + i, center_y + size);
                    
                    SDL_RenderDrawLine(renderer, 
                        center_x + i, center_y + size,
                        center_x + size + i, center_y - size);
                }
                
                // Console message
                printf("\n*******************************\n");
                printf("*   VICTORY! You escaped the dungeon!   *\n");
                printf("*   Final Score: %d   *\n", game_state->players[0].score);
                printf("*******************************\n\n");
            } else {
                // Defeat - Draw a red X
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                
                int center_x = WINDOW_WIDTH / 2;
                int center_y = WINDOW_HEIGHT / 2;
                int size = 30;
                
                // Draw X shape for defeat
                SDL_RenderDrawLine(renderer, 
                    center_x - size, center_y - size,
                    center_x + size, center_y + size);
                
                SDL_RenderDrawLine(renderer, 
                    center_x + size, center_y - size,
                    center_x - size, center_y + size);
                
                // Make lines thicker
                for (int i = -2; i <= 2; i++) {
                    SDL_RenderDrawLine(renderer, 
                        center_x - size + i, center_y - size,
                        center_x + size + i, center_y + size);
                    
                    SDL_RenderDrawLine(renderer, 
                        center_x + size + i, center_y - size,
                        center_x - size + i, center_y + size);
                }
                
                // Console message
                printf("\n*******************************\n");
                printf("*   DEFEAT! You were killed by enemies!   *\n");
                printf("*   Final Score: %d   *\n", game_state->players[0].score);
                printf("*******************************\n\n");
            }
            
            SDL_RenderPresent(renderer);
            SDL_Delay(3000); // Show message for 3 seconds
            
            running = false;
        }
        
        unlock_game_state();
        
        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
        
        // Check if termination was requested
        if (terminate_flag) {
            printf("Termination signal received\n");
            running = false;
            break;
        }
    }
    
    printf("Game loop ended\n");
    
    // Send game over message to all enemy processes
    GameMessage game_over_msg;
    game_over_msg.from_id = 0;
    game_over_msg.to_id = -1;
    game_over_msg.message_type = MSG_GAME_OVER;
    game_over_msg.x = 0;
    game_over_msg.y = 0;
    game_over_msg.data = 0;
    
    for (int i = 0; i < game_state->num_enemies; i++) {
        send_message_to_enemy(i, &game_over_msg);
    }
    
    // Wait for enemy processes to finish - with a timeout
    printf("Waiting for enemy processes to finish...\n");
    
    // Instead of waiting indefinitely, add a timeout
    int wait_count = 0;
    while (wait_count < 10) {
        bool all_done = true;
        for (int i = 0; i < num_enemy_processes; i++) {
            if (enemy_pids[i] > 0) {
                int status;
                pid_t result = waitpid(enemy_pids[i], &status, WNOHANG);
                if (result == 0) {
                    // Process still running
                    all_done = false;
                } else if (result > 0) {
                    // Process finished
                    enemy_pids[i] = -1;
                }
            }
        }
        
        if (all_done) break;
        
        // Wait a bit and try again
        usleep(100000); // 100ms
        wait_count++;
    }
    
    // Force terminate any remaining enemy processes
    for (int i = 0; i < num_enemy_processes; i++) {
        if (enemy_pids[i] > 0) {
            printf("Forcibly terminating enemy process %d\n", i);
            kill(enemy_pids[i], SIGKILL);
            enemy_pids[i] = -1;
        }
    }
    
    // Ensure all IPC resources are properly closed
    printf("Cleaning up IPC resources...\n");
    cleanup_ipc_channels();
    
    // Cleanup SDL resources first
    printf("Cleaning up SDL resources...\n");
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }
    
    // Make sure any pending signals are handled before cleanup
    // to avoid interrupting cleanup process
    usleep(100000); // 100ms pause
    
    // Send game over message to all enemy processes again to ensure they terminate
    printf("Sending final termination signals to enemies...\n");
    for (int i = 0; i < num_enemy_processes; i++) {
        if (enemy_pids[i] > 0) {
            kill(enemy_pids[i], SIGTERM);
        }
    }
    
    // Small delay to allow processes to terminate
    usleep(500000); // 500ms
    
    // Now cleanup game resources before IPC and shared memory
    printf("Cleaning up game resources...\n");
    game_cleanup();
    
    // Clean up IPC channels (this already terminates enemy processes)
    printf("Cleaning up IPC channels...\n");
    cleanup_ipc_channels();
    
    // Detach from shared memory - check first if still valid
    printf("Cleaning up shared memory...\n");
    cleanup_shared_memory();
    
    // Quit SDL last
    printf("Quitting SDL...\n");
    SDL_Quit();
    
    printf("Game ended successfully\n");
    return 0;
} 