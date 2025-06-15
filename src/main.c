#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../deps/SketchyBarHelper/sketchybar.h"

#define DEFAULT_LABEL_COLOR "0xFFFFFFFF"
#define SELECTED_LABEL_COLOR "0xFF00A3CB"
#define TOTAL_WORKSPACES 10
#define WORKSPACE_ID_SIZE 32
#define WINDOWS_COUNT_SIZE 2 // Assuming max 100 windows per workspace, log10(100) = 2
#define SKETCHYBAR_COMMAND_SIZE 256

short workspaces_in_use = 0;


void paint_current_workspace(env env) {
    char* previous_workspace = env_get_value_for_key(env, "PREVIOUS_WORKSPACE");
    char* focused_workspace = env_get_value_for_key(env, "FOCUSED_WORKSPACE");

    if (previous_workspace == NULL || previous_workspace[0] == '\0') return;
    if (focused_workspace == NULL || focused_workspace[0] == '\0') return;

    short focused_workspace_update = 0b1 << atoi(focused_workspace);
    short previous_workspace_update = 0b1 << atoi(previous_workspace);

    char command[SKETCHYBAR_COMMAND_SIZE];

    snprintf(command, SKETCHYBAR_COMMAND_SIZE, "--set space.%s label.color=" SELECTED_LABEL_COLOR " drawing=on", focused_workspace);
    sketchybar(command);

    snprintf(command, SKETCHYBAR_COMMAND_SIZE, "--set space.%s label.color=" DEFAULT_LABEL_COLOR, previous_workspace);
    sketchybar(command);
}

pid_t update_workspace(short workspace_id) {
    pid_t pid = fork();

    // Error with fork
    if (pid == -1) {
        perror("Failed to create watcher");
        exit(-1);
    }

    // Parent process
    if (pid > 0) {
        return pid;
    }

    // Child process
    int aerospace_fd[2];
    // Error creating pipe
    if (pipe(aerospace_fd) == -1) {
        perror("Couldn't create aerospace pipe");
        exit(-2);
    }

    pid_t aerospace_cmd_pid = fork();
    // Error with fork
    if (aerospace_cmd_pid == -1) {
        perror("Failed to fork into aerospace workspace command");
        exit(-1);
    }

    // Still child process, i.e. pid
    if (aerospace_cmd_pid > 0) {
        close(aerospace_fd[1]);
        wait(&aerospace_cmd_pid);

        char workspace_windows_str[WINDOWS_COUNT_SIZE];
        read(aerospace_fd[0], &workspace_windows_str, WINDOWS_COUNT_SIZE);
        close(aerospace_fd[0]);

        char command[SKETCHYBAR_COMMAND_SIZE];
        if (atoi(workspace_windows_str) > 0) {
            snprintf(command, SKETCHYBAR_COMMAND_SIZE, "--set space.%i drawing=on", workspace_id);
        } else {
            snprintf(command, SKETCHYBAR_COMMAND_SIZE, "--set space.%i drawing=off", workspace_id);
        }
        sketchybar(command);

        exit(0);
    }

    close(aerospace_fd[0]);
    dup2(aerospace_fd[1], STDOUT_FILENO);
    close(aerospace_fd[1]);

    char workspace[WORKSPACE_ID_SIZE];
    snprintf(workspace, sizeof(workspace_id), "%i", workspace_id);

    // Grandchild process, i.e. aerospace_cmd_pid
    execlp("aerospace", "aerospace", "list-windows", "--workspace", workspace, "--count", NULL);
    perror("Failed to launch aerospace command");

    exit(0);
}

void update_all_workspaces() {
    pid_t watchers_pids[TOTAL_WORKSPACES];
    for (short i = 1; i <= TOTAL_WORKSPACES; i++) {
        watchers_pids[(i-1)] = update_workspace(i);
    }

    for (short i = 1; i <= TOTAL_WORKSPACES; i++) wait(NULL);
}


void main_handler(env env) {
    paint_current_workspace(env);
    update_all_workspaces();
}


void setup_workspaces() {
    char command[SKETCHYBAR_COMMAND_SIZE];
    for(short i = 1; i < TOTAL_WORKSPACES; i++) {
        snprintf(command, SKETCHYBAR_COMMAND_SIZE, "--add item space.%i left --set space.%i drawing=off label=\"%i\" click_script=\"aerospace workspace %i\"", i, i, i, i);
        sketchybar(command);
    }
    printf("[DONE] Setup workspaces\n");
}
int main(int argc, char** argv) {
    // setup_workspaces();

    event_server_begin(main_handler, "ash");

    return 0;
}
