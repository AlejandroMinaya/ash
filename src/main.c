#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "../deps/SketchyBarHelper/sketchybar.h"

#define DEFAULT_LABEL_COLOR "0xFFFFFFFF"
#define SELECTED_LABEL_COLOR "0xFF00A3CB"
#define TOTAL_WORKSPACES 10
#define WORKSPACE_ID_SIZE 32
#define WINDOWS_COUNT_SIZE 8
#define SKETCHYBAR_COMMAND_SIZE 256

void paint_current_workspace(env env) {
    char* previous_workspace = env_get_value_for_key(env, "PREVIOUS_WORKSPACE");
    char* focused_workspace = env_get_value_for_key(env, "FOCUSED_WORKSPACE");

    if (previous_workspace == NULL || previous_workspace[0] == '\0') return;
    if (focused_workspace == NULL || focused_workspace[0] == '\0') return;

    char* response;
    char command[SKETCHYBAR_COMMAND_SIZE];

    snprintf(command, SKETCHYBAR_COMMAND_SIZE, "--set space.%s label.color=" SELECTED_LABEL_COLOR " drawing=on", focused_workspace);
    sketchybar(command);

    snprintf(command, SKETCHYBAR_COMMAND_SIZE, "--set space.%s label.color=" DEFAULT_LABEL_COLOR, previous_workspace);
    sketchybar(command);
}

void get_aerospace_workspace_count(int workspace_id, int aerospace_fd[]) {
    close(aerospace_fd[0]);

    dup2(aerospace_fd[1], STDOUT_FILENO);
    close(aerospace_fd[1]);

    char workspace[WORKSPACE_ID_SIZE];
    snprintf(workspace, WORKSPACE_ID_SIZE, "%i", workspace_id);
    execlp("aerospace", "aerospace", "list-windows", "--workspace", workspace, "--count", NULL);

    perror("Failed to launch aerospace command");
}


int read_aerospace_window_count(int read_fd) {
    char workspace_windows_str[WINDOWS_COUNT_SIZE];

    ssize_t bytes_read = read(read_fd, &workspace_windows_str, WINDOWS_COUNT_SIZE - 1);
    if (bytes_read > 0) {
        workspace_windows_str[bytes_read] = '\0';
    } else {
        workspace_windows_str[0] = '0';
        workspace_windows_str[1] = '\0';
    }
    return atoi(workspace_windows_str);
}


void toggle_workspace_indicator(int workspace_id, int aerospace_fd[]) {
    close(aerospace_fd[1]);

    int windows_count = read_aerospace_window_count(aerospace_fd[0]);
    close(aerospace_fd[0]);

    char command[SKETCHYBAR_COMMAND_SIZE];
    if (windows_count > 0)  snprintf(command, SKETCHYBAR_COMMAND_SIZE, "--set space.%i drawing=on", workspace_id);
    else snprintf(command, SKETCHYBAR_COMMAND_SIZE, "--set space.%i drawing=off", workspace_id);
    sketchybar(command);
}

int update_workspace(short workspace_id) {
    // Child process...
    int aerospace_fd[2];
    // Error creating pipe
    if (pipe(aerospace_fd) == -1) {
        perror("Couldn't create aerospace pipe");
        return -2;
    }

    pid_t aerospace_cmd_pid = fork();
    // Error with fork
    if (aerospace_cmd_pid == -1) {
        perror("Failed to fork into aerospace workspace command");
        exit(-1);
    }

    // aerospace subprocess
    if (aerospace_cmd_pid == 0) {
        get_aerospace_workspace_count(workspace_id, aerospace_fd);
        exit(0);
    }

    waitpid(aerospace_cmd_pid, NULL, 0);
    toggle_workspace_indicator(workspace_id, aerospace_fd);
    return 0;
}

void update_all_workspaces() {
    for (short i = 1; i <= TOTAL_WORKSPACES; i++)  update_workspace(i);
}

void main_handler(env env) {
    update_all_workspaces();
    paint_current_workspace(env);
}

int main(int argc, char** argv) {
    event_server_begin(main_handler, "ash");
    return 0;
}
