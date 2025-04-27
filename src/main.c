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

// Define M_PI if not defined (for pulse calculations)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Global flag for handling signals
volatile sig_atomic_t terminate_flag = 0;

// Global flag to track if game over message was printed
static bool game_over_message_printed = false;

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
        case 'D':
            SDL_RenderDrawLine(renderer, x, y, x, y + size);
            SDL_RenderDrawLine(renderer, x, y, x + 2*size/3, y);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y, x + size, y + size/3);
            SDL_RenderDrawLine(renderer, x + size, y + size/3, x + size, y + 2*size/3);
            SDL_RenderDrawLine(renderer, x + size, y + 2*size/3, x + 2*size/3, y + size);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y + size, x, y + size);
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
        case 'G':
            SDL_RenderDrawLine(renderer, x + size, y, x + size/3, y);
            SDL_RenderDrawLine(renderer, x + size/3, y, x, y + size/3);
            SDL_RenderDrawLine(renderer, x, y + size/3, x, y + 2*size/3);
            SDL_RenderDrawLine(renderer, x, y + 2*size/3, x + size/3, y + size);
            SDL_RenderDrawLine(renderer, x + size/3, y + size, x + 2*size/3, y + size);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y + size, x + size, y + 2*size/3);
            SDL_RenderDrawLine(renderer, x + size, y + 2*size/3, x + size, y + size/2);
            SDL_RenderDrawLine(renderer, x + size, y + size/2, x + 2*size/3, y + size/2);
            break;
        case 'H':
            SDL_RenderDrawLine(renderer, x, y, x, y + size);
            SDL_RenderDrawLine(renderer, x + size, y, x + size, y + size);
            SDL_RenderDrawLine(renderer, x, y + size/2, x + size, y + size/2);
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
        case 'M':
            SDL_RenderDrawLine(renderer, x, y, x, y + size);
            SDL_RenderDrawLine(renderer, x, y, x + size/2, y + size/2);
            SDL_RenderDrawLine(renderer, x + size/2, y + size/2, x + size, y);
            SDL_RenderDrawLine(renderer, x + size, y, x + size, y + size);
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
        case 'Q':
            SDL_RenderDrawLine(renderer, x + size/3, y, x + 2*size/3, y);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y, x + size, y + size/3);
            SDL_RenderDrawLine(renderer, x + size, y + size/3, x + size, y + 2*size/3);
            SDL_RenderDrawLine(renderer, x + size, y + 2*size/3, x + 2*size/3, y + size);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y + size, x + size/3, y + size);
            SDL_RenderDrawLine(renderer, x + size/3, y + size, x, y + 2*size/3);
            SDL_RenderDrawLine(renderer, x, y + 2*size/3, x, y + size/3);
            SDL_RenderDrawLine(renderer, x, y + size/3, x + size/3, y);
            SDL_RenderDrawLine(renderer, x + size/2, y + 2*size/3, x + size, y + size);
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
        case 'U':
            SDL_RenderDrawLine(renderer, x, y, x, y + 2*size/3);
            SDL_RenderDrawLine(renderer, x, y + 2*size/3, x + size/3, y + size);
            SDL_RenderDrawLine(renderer, x + size/3, y + size, x + 2*size/3, y + size);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y + size, x + size, y + 2*size/3);
            SDL_RenderDrawLine(renderer, x + size, y + 2*size/3, x + size, y);
            break;
        case 'V':
            SDL_RenderDrawLine(renderer, x, y, x + size/2, y + size);
            SDL_RenderDrawLine(renderer, x + size/2, y + size, x + size, y);
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
        case ':':
            SDL_RenderDrawLine(renderer, x + size/2, y + size/4, x + size/2, y + size/3);
            SDL_RenderDrawLine(renderer, x + size/2, y + 2*size/3, x + size/2, y + 3*size/4);
            break;
        case ' ':
            // Nothing for space
            break;
        case '0':
            SDL_RenderDrawLine(renderer, x + size/3, y, x + 2*size/3, y);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y, x + size, y + size/3);
            SDL_RenderDrawLine(renderer, x + size, y + size/3, x + size, y + 2*size/3);
            SDL_RenderDrawLine(renderer, x + size, y + 2*size/3, x + 2*size/3, y + size);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y + size, x + size/3, y + size);
            SDL_RenderDrawLine(renderer, x + size/3, y + size, x, y + 2*size/3);
            SDL_RenderDrawLine(renderer, x, y + 2*size/3, x, y + size/3);
            SDL_RenderDrawLine(renderer, x, y + size/3, x + size/3, y);
            break;
        case '1':
            SDL_RenderDrawLine(renderer, x + size/3, y, x + size/2, y);
            SDL_RenderDrawLine(renderer, x + size/2, y, x + size/2, y + size);
            SDL_RenderDrawLine(renderer, x + size/3, y + size, x + 2*size/3, y + size);
            break;
        case '2':
            SDL_RenderDrawLine(renderer, x + size/4, y, x + 3*size/4, y);
            SDL_RenderDrawLine(renderer, x + 3*size/4, y, x + size, y + size/4);
            SDL_RenderDrawLine(renderer, x + size, y + size/4, x + 3*size/4, y + size/2);
            SDL_RenderDrawLine(renderer, x + 3*size/4, y + size/2, x + size/4, y + size/2);
            SDL_RenderDrawLine(renderer, x + size/4, y + size/2, x, y + 3*size/4);
            SDL_RenderDrawLine(renderer, x, y + 3*size/4, x, y + size);
            SDL_RenderDrawLine(renderer, x, y + size, x + size, y + size);
            break;
        case '3':
            SDL_RenderDrawLine(renderer, x, y, x + 2*size/3, y);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y, x + size, y + size/3);
            SDL_RenderDrawLine(renderer, x + size, y + size/3, x + 2*size/3, y + size/2);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y + size/2, x + size/2, y + size/2);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y + size/2, x + size, y + 2*size/3);
            SDL_RenderDrawLine(renderer, x + size, y + 2*size/3, x + 2*size/3, y + size);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y + size, x, y + size);
            break;
        case '4':
            SDL_RenderDrawLine(renderer, x + 2*size/3, y, x + 2*size/3, y + size);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y, x, y + 2*size/3);
            SDL_RenderDrawLine(renderer, x, y + 2*size/3, x + size, y + 2*size/3);
            break;
        case '5':
            SDL_RenderDrawLine(renderer, x, y, x + size, y);
            SDL_RenderDrawLine(renderer, x, y, x, y + size/2);
            SDL_RenderDrawLine(renderer, x, y + size/2, x + 2*size/3, y + size/2);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y + size/2, x + size, y + 3*size/4);
            SDL_RenderDrawLine(renderer, x + size, y + 3*size/4, x + 2*size/3, y + size);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y + size, x + size/3, y + size);
            break;
        case '6':
            SDL_RenderDrawLine(renderer, x + 2*size/3, y, x + size/3, y);
            SDL_RenderDrawLine(renderer, x + size/3, y, x, y + size/3);
            SDL_RenderDrawLine(renderer, x, y + size/3, x, y + 2*size/3);
            SDL_RenderDrawLine(renderer, x, y + 2*size/3, x + size/3, y + size);
            SDL_RenderDrawLine(renderer, x + size/3, y + size, x + 2*size/3, y + size);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y + size, x + size, y + 2*size/3);
            SDL_RenderDrawLine(renderer, x + size, y + 2*size/3, x + size/2, y + size/2);
            SDL_RenderDrawLine(renderer, x + size/2, y + size/2, x, y + size/2);
            break;
        case '7':
            SDL_RenderDrawLine(renderer, x, y, x + size, y);
            SDL_RenderDrawLine(renderer, x + size, y, x + size/3, y + size);
            break;
        case '8':
            SDL_RenderDrawLine(renderer, x + size/3, y, x + 2*size/3, y);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y, x + size, y + size/3);
            SDL_RenderDrawLine(renderer, x + size, y + size/3, x + 2*size/3, y + size/2);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y + size/2, x + size/3, y + size/2);
            SDL_RenderDrawLine(renderer, x + size/3, y + size/2, x, y + size/3);
            SDL_RenderDrawLine(renderer, x, y + size/3, x + size/3, y);
            SDL_RenderDrawLine(renderer, x + size/3, y + size/2, x, y + 2*size/3);
            SDL_RenderDrawLine(renderer, x, y + 2*size/3, x + size/3, y + size);
            SDL_RenderDrawLine(renderer, x + size/3, y + size, x + 2*size/3, y + size);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y + size, x + size, y + 2*size/3);
            SDL_RenderDrawLine(renderer, x + size, y + 2*size/3, x + 2*size/3, y + size/2);
            break;
        case '9':
            SDL_RenderDrawLine(renderer, x, y + 2*size/3, x + size/3, y + size);
            SDL_RenderDrawLine(renderer, x + size/3, y + size, x + 2*size/3, y + size);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y + size, x + size, y + 2*size/3);
            SDL_RenderDrawLine(renderer, x + size, y + 2*size/3, x + size, y + size/3);
            SDL_RenderDrawLine(renderer, x + size, y + size/3, x + 2*size/3, y);
            SDL_RenderDrawLine(renderer, x + 2*size/3, y, x + size/3, y);
            SDL_RenderDrawLine(renderer, x + size/3, y, x, y + size/3);
            SDL_RenderDrawLine(renderer, x, y + size/3, x + size/2, y + size/2);
            SDL_RenderDrawLine(renderer, x + size/2, y + size/2, x + size, y + size/2);
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
        "Dungeon Conquerors",
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
        
        // Check for messages from enemy processes - only if not showing welcome screen
        if (!showing_welcome) {
            for (int i = 0; i < game_state->num_enemies; i++) {
                if (receive_message_from_enemy(i, &message)) {
                    if (message.message_type == MSG_PLAYER_HIT) {
                        printf("Player hit by enemy %d!\n", i);
                        lock_game_state();
                        game_state->player_hit = true;
                        check_player_enemy_collision(game_state);
                        unlock_game_state();
                    }
                }
            }
        }
        
        // Render game
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        lock_game_state();
        
        if (showing_welcome) {
            // Draw welcome message with modern styling
            SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
            SDL_RenderClear(renderer);
            
            // Create pulsing animation for welcome screen
            float pulse = (sinf(welcome_timer * 0.05f) * 0.2f + 0.8f);
            
            // Draw stylish background gradient
            for (int y = 0; y < WINDOW_HEIGHT; y++) {
                float progress = (float)y / WINDOW_HEIGHT;
                SDL_SetRenderDrawColor(renderer, 
                                      (Uint8)(20 + 10 * progress), 
                                      (Uint8)(20 + 15 * progress), 
                                      (Uint8)(40 + 20 * progress), 
                                      255);
                SDL_RenderDrawLine(renderer, 0, y, WINDOW_WIDTH, y);
            }
            
            // Draw decorative elements (stars)
            for (int i = 0; i < 50; i++) {
                int x = (welcome_timer + i * 50) % WINDOW_WIDTH;
                int y = (i * 73) % WINDOW_HEIGHT;
                int size = (i % 3) + 1;
                float star_pulse = (sinf(welcome_timer * 0.1f + i * 0.2f) * 0.5f + 0.5f);
                
                SDL_SetRenderDrawColor(renderer, 
                                      (Uint8)(150 * star_pulse), 
                                      (Uint8)(150 * star_pulse), 
                                      (Uint8)(255 * star_pulse), 
                                      (Uint8)(150 * star_pulse));
                SDL_Rect star = {x, y, size, size};
                SDL_RenderFillRect(renderer, &star);
            }
            
            // Draw semi-transparent background panel for message
            SDL_SetRenderDrawColor(renderer, 20, 20, 60, 200);
            SDL_Rect msg_bg = {
                WINDOW_WIDTH / 6, 
                WINDOW_HEIGHT / 3, 
                WINDOW_WIDTH * 2 / 3, 
                WINDOW_HEIGHT / 3
            };
            SDL_RenderFillRect(renderer, &msg_bg);
            
            // Draw panel border with glow
            SDL_SetRenderDrawColor(renderer, 
                                  (Uint8)(100 * pulse), 
                                  (Uint8)(150 * pulse), 
                                  (Uint8)(255 * pulse), 
                                  255);
            for (int i = 0; i < 3; i++) {
                SDL_Rect border = {
                    msg_bg.x - i, 
                    msg_bg.y - i, 
                    msg_bg.w + i*2, 
                    msg_bg.h + i*2
                };
                SDL_RenderDrawRect(renderer, &border);
            }
            
            // Draw title with larger size
            draw_simple_text(renderer, WINDOW_WIDTH/2 - 120, WINDOW_HEIGHT/3 - 30, "DUNGEON CONQUERORS", 15);
            
            // Draw welcome messages with glowing effect
            SDL_SetRenderDrawColor(renderer, 
                                  (Uint8)(200 * pulse), 
                                  (Uint8)(220 * pulse), 
                                  (Uint8)(255 * pulse), 
                                  255);
            
            // Display correct number of keys based on level
            char keys_text[32];
            sprintf(keys_text, "COLLECT KEYS");
            draw_simple_text(renderer, WINDOW_WIDTH/6 + 30, WINDOW_HEIGHT/3 + 30, keys_text, 15);
            draw_simple_text(renderer, WINDOW_WIDTH/6 + 30, WINDOW_HEIGHT/3 + 60, "TO OPEN THE EXIT", 15);
            
            // Warning message with red color
            SDL_SetRenderDrawColor(renderer, 
                                  (Uint8)(255 * pulse), 
                                  (Uint8)(100 * pulse), 
                                  (Uint8)(100 * pulse), 
                                  255);
            draw_simple_text(renderer, WINDOW_WIDTH/6 + 30, WINDOW_HEIGHT/3 + 90, "BEWARE OF ENEMIES!", 15);
            
            // Flashing "Press any key" prompt
            if (welcome_timer % 40 < 30) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 100, 255);
                draw_simple_text(renderer, WINDOW_WIDTH/6 + 90, WINDOW_HEIGHT/3 + 130, "PRESS ANY KEY", 12);
            }
            
            // Draw decorative animated corner symbols
            int corner_size = 20;
            SDL_SetRenderDrawColor(renderer, 
                                  (Uint8)(150 * pulse), 
                                  (Uint8)(200 * pulse), 
                                  (Uint8)(255 * pulse), 
                                  255);
            
            // Top-left corner
            SDL_RenderDrawLine(renderer, msg_bg.x, msg_bg.y, msg_bg.x + corner_size, msg_bg.y);
            SDL_RenderDrawLine(renderer, msg_bg.x, msg_bg.y, msg_bg.x, msg_bg.y + corner_size);
            
            // Top-right corner
            SDL_RenderDrawLine(renderer, msg_bg.x + msg_bg.w, msg_bg.y, msg_bg.x + msg_bg.w - corner_size, msg_bg.y);
            SDL_RenderDrawLine(renderer, msg_bg.x + msg_bg.w, msg_bg.y, msg_bg.x + msg_bg.w, msg_bg.y + corner_size);
            
            // Bottom-left corner
            SDL_RenderDrawLine(renderer, msg_bg.x, msg_bg.y + msg_bg.h, msg_bg.x + corner_size, msg_bg.y + msg_bg.h);
            SDL_RenderDrawLine(renderer, msg_bg.x, msg_bg.y + msg_bg.h, msg_bg.x, msg_bg.y + msg_bg.h - corner_size);
            
            // Bottom-right corner
            SDL_RenderDrawLine(renderer, msg_bg.x + msg_bg.w, msg_bg.y + msg_bg.h, msg_bg.x + msg_bg.w - corner_size, msg_bg.y + msg_bg.h);
            SDL_RenderDrawLine(renderer, msg_bg.x + msg_bg.w, msg_bg.y + msg_bg.h, msg_bg.x + msg_bg.w, msg_bg.y + msg_bg.h - corner_size);
            
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
            // Only print game over message once
            if (!game_over_message_printed) {
                printf("Game over condition reached\n");
                game_over_message_printed = true;
            }
            
            // Calculate pulsing effect for game over screen
            float pulse_time = (float)(SDL_GetTicks() % 2000) / 2000.0f;
            float pulse = (sinf(pulse_time * 2.0f * M_PI) * 0.2f + 0.8f);
            
            // Add a slight darkening overlay for game over
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
            SDL_Rect overlay = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
            SDL_RenderFillRect(renderer, &overlay);
            
            // Display victory or defeat message with modern styling
            SDL_SetRenderDrawColor(renderer, 30, 30, 40, 230);
            SDL_Rect msg_bg = {
                WINDOW_WIDTH / 4, 
                WINDOW_HEIGHT / 2 - 70, 
                WINDOW_WIDTH / 2, 
                140
            };
            SDL_RenderFillRect(renderer, &msg_bg);
            
            // Draw multiple borders for a glowing effect
            if (game_state->winner_id == -2) {
                // Game Exited - blue glow
                for (int i = 0; i < 3; i++) {
                    SDL_SetRenderDrawColor(renderer, 
                                          (Uint8)(30 * pulse), 
                                          (Uint8)(100 * pulse), 
                                          (Uint8)(180 * pulse), 
                                          (Uint8)(255 - i * 40));
                    SDL_Rect border = {
                        msg_bg.x - i, 
                        msg_bg.y - i, 
                        msg_bg.w + i*2, 
                        msg_bg.h + i*2
                    };
                    SDL_RenderDrawRect(renderer, &border);
                }
                
                // Draw Game Exited message
                SDL_SetRenderDrawColor(renderer, 
                                      (Uint8)(100 * pulse), 
                                      (Uint8)(180 * pulse), 
                                      (Uint8)(255 * pulse), 
                                      255);
                draw_simple_text(renderer, WINDOW_WIDTH/2 - 90, WINDOW_HEIGHT/2 - 40, "GAME EXITED", 20);
                
                // Draw score
                char score_text[32];
                sprintf(score_text, "SCORE: %d", game_state->players[0].score);
                draw_simple_text(renderer, WINDOW_WIDTH/2 - 70, WINDOW_HEIGHT/2, score_text, 15);
                
                // Console message - print only once
                if (!game_over_message_printed) {
                    printf("\n*******************************\n");
                    printf("*   Game exited by player   *\n");
                    printf("*   Final Score: %d   *\n", game_state->players[0].score);
                    printf("*******************************\n\n");
                    game_over_message_printed = true;
                }
            }
            else if (game_state->winner_id == 0) {
                // Victory - green glow
                for (int i = 0; i < 3; i++) {
                    SDL_SetRenderDrawColor(renderer, 
                                          (Uint8)(0 * pulse), 
                                          (Uint8)(150 * pulse), 
                                          (Uint8)(50 * pulse), 
                                          (Uint8)(255 - i * 40));
                    SDL_Rect border = {
                        msg_bg.x - i, 
                        msg_bg.y - i, 
                        msg_bg.w + i*2, 
                        msg_bg.h + i*2
                    };
                    SDL_RenderDrawRect(renderer, &border);
                }
                
                // Draw victory message
                SDL_SetRenderDrawColor(renderer, 
                                      (Uint8)(100 * pulse), 
                                      (Uint8)(255 * pulse), 
                                      (Uint8)(100 * pulse), 
                                      255);
                draw_simple_text(renderer, WINDOW_WIDTH/2 - 80, WINDOW_HEIGHT/2 - 40, "VICTORY!", 20);
                
                // Draw score
                char score_text[32];
                sprintf(score_text, "SCORE: %d", game_state->players[0].score);
                draw_simple_text(renderer, WINDOW_WIDTH/2 - 70, WINDOW_HEIGHT/2, score_text, 15);
                
                // Draw checkmark symbol - moved down to avoid overlapping with text
                int center_x = WINDOW_WIDTH / 2;
                int center_y = WINDOW_HEIGHT / 2 + 45; // Moved down from +30 to +45
                int size = 30;
                
                // Left line of checkmark
                SDL_RenderDrawLine(renderer, 
                    center_x - size/2, center_y,
                    center_x - size/6, center_y + size/2);
                
                // Right line of checkmark
                SDL_RenderDrawLine(renderer, 
                    center_x - size/6, center_y + size/2,
                    center_x + size/2, center_y - size/2);
                
                // Make lines thicker
                for (int i = -2; i <= 2; i++) {
                    SDL_RenderDrawLine(renderer, 
                        center_x - size/2 + i, center_y,
                        center_x - size/6 + i, center_y + size/2);
                    
                    SDL_RenderDrawLine(renderer, 
                        center_x - size/6 + i, center_y + size/2,
                        center_x + size/2 + i, center_y - size/2);
                }
                
                // Console message - print only once
                if (!game_over_message_printed) {
                    printf("\n*******************************\n");
                    printf("*   VICTORY! You escaped the dungeon!   *\n");
                    printf("*   Final Score: %d   *\n", game_state->players[0].score);
                    printf("*******************************\n\n");
                    game_over_message_printed = true;
                }
            } else {
                // Defeat - red glow
                for (int i = 0; i < 3; i++) {
                    SDL_SetRenderDrawColor(renderer, 
                                          (Uint8)(180 * pulse), 
                                          (Uint8)(30 * pulse), 
                                          (Uint8)(30 * pulse), 
                                          (Uint8)(255 - i * 40));
                    SDL_Rect border = {
                        msg_bg.x - i, 
                        msg_bg.y - i, 
                        msg_bg.w + i*2, 
                        msg_bg.h + i*2
                    };
                    SDL_RenderDrawRect(renderer, &border);
                }
                
                // Draw defeat message
                SDL_SetRenderDrawColor(renderer, 
                                      (Uint8)(255 * pulse), 
                                      (Uint8)(100 * pulse), 
                                      (Uint8)(100 * pulse), 
                                      255);
                draw_simple_text(renderer, WINDOW_WIDTH/2 - 70, WINDOW_HEIGHT/2 - 40, "DEFEAT!", 20);
                
                // Draw X symbol - positioned to match the checkmark in victory screen
                int center_x = WINDOW_WIDTH / 2;
                int center_y = WINDOW_HEIGHT / 2 + 15; // Moved higher to be closer to the "DEFEAT!" text
                int size = 30;
                
                // X lines
                SDL_RenderDrawLine(renderer, 
                    center_x - size/2, center_y - size/2,
                    center_x + size/2, center_y + size/2);
                
                SDL_RenderDrawLine(renderer, 
                    center_x - size/2, center_y + size/2,
                    center_x + size/2, center_y - size/2);
                
                // Make lines thicker
                for (int i = -2; i <= 2; i++) {
                    SDL_RenderDrawLine(renderer, 
                        center_x - size/2 + i, center_y - size/2,
                        center_x + size/2 + i, center_y + size/2);
                    
                    SDL_RenderDrawLine(renderer, 
                        center_x - size/2 + i, center_y + size/2,
                        center_x + size/2 + i, center_y - size/2);
                }
                
                // Console message - print only once
                if (!game_over_message_printed) {
                    printf("\n*******************************\n");
                    printf("*   DEFEAT! The dungeon has claimed another victim!   *\n");
                    printf("*******************************\n\n");
                    game_over_message_printed = true;
                }
            }
            
            // Press any key to exit message
            if (SDL_GetTicks() % 1000 < 500) {
                SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
                draw_simple_text(renderer, WINDOW_WIDTH/2 - 100, WINDOW_HEIGHT/2 + 50, "PRESS ANY KEY TO EXIT", 10);
            }
            
            // Present the game over screen
            SDL_RenderPresent(renderer);
            
            // Wait for key press or timeout to exit
            static bool exit_wait_started = false;
            static Uint32 exit_start_time = 0;
            static const Uint32 EXIT_TIMEOUT = 5000; // 5 seconds timeout
            
            if (!exit_wait_started) {
                exit_start_time = SDL_GetTicks();
                exit_wait_started = true;
            }
            
            // Check for any key press to exit
            SDL_Event exit_event;
            while (SDL_PollEvent(&exit_event)) {
                if (exit_event.type == SDL_KEYDOWN || exit_event.type == SDL_QUIT) {
                    running = false;
                    break;
                }
            }
            
            // Check for timeout
            if (SDL_GetTicks() - exit_start_time > EXIT_TIMEOUT) {
                running = false;
            }
            
            // Skip the rest of the loop
            unlock_game_state();
            continue;
        }
        
        unlock_game_state();
        
        SDL_RenderPresent(renderer);
        
        // Cap frame rate to approximately 60 FPS
        SDL_Delay(16);
        
        // Check if termination was requested
        if (terminate_flag) {
            printf("Termination signal received\n");
            running = false;
            break;
        }
        
        // Update welcome timer if showing welcome screen
        if (showing_welcome) {
            welcome_timer++;
            if (welcome_timer >= WELCOME_DURATION) {
                showing_welcome = false;
                // Resume normal game time tracking
                lock_game_state();
                time_t paused_duration = game_state->current_time - welcome_start_time;
                game_state->start_time += paused_duration; // Adjust start time to account for pause
                unlock_game_state();
            }
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
    
    // Clean up in the correct order to prevent segmentation faults
    // First clean up game resources
    printf("Cleaning up game resources...\n");
    game_cleanup();
    
    // Then clean up IPC channels 
    printf("Cleaning up IPC channels...\n");
    cleanup_ipc_channels();
    
    // Finally clean up shared memory - this must be done last
    printf("Cleaning up shared memory...\n");
    cleanup_shared_memory();
    
    // Quit SDL after everything else is cleaned up
    printf("Quitting SDL...\n");
    SDL_Quit();
    
    printf("Game ended successfully\n");
    return 0;
} 