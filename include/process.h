#ifndef PROCESS_H
#define PROCESS_H

#include <stdbool.h>
#include <sys/types.h>
#include "game.h"

// Maximum number of player processes
#define MAX_PROCESSES 4
#define MAX_ENEMY_PROCESSES 5

// Array to store process IDs
extern pid_t player_pids[MAX_PROCESSES];
extern pid_t enemy_pids[MAX_ENEMY_PROCESSES];
extern int num_processes;
extern int num_enemy_processes;

// Pipe file descriptors for IPC
extern int main_to_enemy_pipe[MAX_ENEMY_PROCESSES][2];  // Main process to enemy processes
extern int enemy_to_main_pipe[MAX_ENEMY_PROCESSES][2];  // Enemy processes to main process

// Function declarations
void create_player_processes(int count);
void create_enemy_processes(int count);
void wait_for_player_processes(void);
void wait_for_enemy_processes(void);
void player_process_main(int player_id);
bool setup_ipc_channels(void);
void cleanup_ipc_channels(void);

// Enemy AI functions
void enemy_process_main(int enemy_id, EntityType enemy_type);
void send_message_to_enemy(int enemy_id, GameMessage *message);
bool receive_message_from_enemy(int enemy_id, GameMessage *message);
void send_message_to_main(int enemy_id, GameMessage *message);
bool receive_message_from_main(int enemy_id, GameMessage *message);
void broadcast_player_position(int x, int y);

#endif /* PROCESS_H */ 