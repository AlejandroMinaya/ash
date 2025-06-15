#include <stdio.h>
#include <stdlib.h>

#include "../deps/SketchyBarHelper/sketchybar.h"

#define DEFAULT_LABEL_COLOR "0xFFFFFFFF"
#define SELECTED_LABEL_COLOR "0xFF00A3CB"
#define TOTAL_WORKSPACES 10
#define WORKSPACE_ID_SIZE 32

short workspaces_in_use = 0;


void paint_current_workspace(env env) {
    char* previous_workspace = env_get_value_for_key(env, "PREVIOUS_WORKSPACE");
    char* focused_workspace = env_get_value_for_key(env, "FOCUSED_WORKSPACE");

    if (previous_workspace == NULL || previous_workspace[0] == '\0') return;
    if (focused_workspace == NULL || focused_workspace[0] == '\0') return;

    short focused_workspace_update = 0b1 << atoi(focused_workspace);
    short previous_workspace_update = 0b1 << atoi(previous_workspace);

    char command[256];

    snprintf(command, 256, "--set space.%s label.color=" SELECTED_LABEL_COLOR " drawing=on", focused_workspace);
    sketchybar(command);

    snprintf(command, 256, "--set space.%s label.color=" DEFAULT_LABEL_COLOR, previous_workspace);
    sketchybar(command);
}


void main_handler(env env) {
    paint_current_workspace(env);
}

pid_t launch_workspace_watcher(short workspace_id) {
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
    printf("Hi, I'm process %i\n", workspace_id);
    pid_t aerospace_cmd_pid = fork();

    // Error with aerospace command
    if (aerospace_cmd_pid == -1) {
        perror("Failed to fork into aerospace workspace command");
        exit(-1);
    }

    // Still child process, i.e. pid
    if (aerospace_cmd_pid > 0) {
        wait(&aerospace_cmd_pid);
        exit(0);
    }

    char workspace[WORKSPACE_ID_SIZE];
    snprintf(workspace, sizeof(workspace_id), "%i", workspace_id);

    // Grandchild process, i.e. aerospace_cmd_pid
    execlp("aerospace", "aerospace", "list-windows", "--workspace", workspace, "--count", NULL);
    perror("Failed to launch aerospace command");

    exit(0);
}

void start_watching() {
    pid_t watchers_pids[TOTAL_WORKSPACES];
    for (short i = 1; i <= TOTAL_WORKSPACES; i++) {
        watchers_pids[(i-1)] = launch_workspace_watcher(i);
    }

    for (short i = 1; i <= TOTAL_WORKSPACES; i++) wait(NULL);
}

int main(int argc, char** argv) {
    // event_server_begin(main_handler, "ash");

    start_watching();
    return 0;
}
