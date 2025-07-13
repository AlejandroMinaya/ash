#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "../deps/SketchyBarHelper/sketchybar.h"

#define DEFAULT_LABEL_COLOR "0xFFFFFFFF"
#define SELECTED_LABEL_COLOR "0xFF00A3CB"
#define TOTAL_WORKSPACES 4
#define WORKSPACE_ID_SIZE 32
#define WINDOWS_COUNT_SIZE 8
#define SKETCHYBAR_COMMAND_SIZE 256

short workspaces_in_use = 0;


void paint_current_workspace(env env) {
    char* previous_workspace = env_get_value_for_key(env, "PREVIOUS_WORKSPACE");
    char* focused_workspace = env_get_value_for_key(env, "FOCUSED_WORKSPACE");

    if (previous_workspace == NULL || previous_workspace[0] == '\0') return;
    if (focused_workspace == NULL || focused_workspace[0] == '\0') return;

    char* response;
    char command[SKETCHYBAR_COMMAND_SIZE];

    snprintf(command, SKETCHYBAR_COMMAND_SIZE, "--set space.%s label.color=" SELECTED_LABEL_COLOR " drawing=on", focused_workspace);
    response = sketchybar(command);
    printf("[SKETCHYBAR]: %s\n", response);

    snprintf(command, SKETCHYBAR_COMMAND_SIZE, "--set space.%s label.color=" DEFAULT_LABEL_COLOR, previous_workspace);
    response = sketchybar(command);
    printf("[SKETCHYBAR]: %s\n", response);
}

void get_aerospace_workspace_count(int workspace_id, int aerospace_fd[]) {
    close(aerospace_fd[0]);

    dup2(aerospace_fd[1], STDOUT_FILENO);
    close(aerospace_fd[1]);

    char workspace[WORKSPACE_ID_SIZE];
    snprintf(workspace, WORKSPACE_ID_SIZE, "%i", workspace_id);

    // Grandchild process, i.e. aerospace_cmd_pid
     execlp("aerospace", "aerospace", "list-windows", "--workspace", workspace, "--count", NULL);
    perror("Failed to launch aerospace command");
}


void toggle_workspace_indicator(int workspace_id, int aerospace_fd[], pid_t subprocess_pid) {
    close(aerospace_fd[1]);

    printf("[WORKSPACE %i]: Waiting for aerospace...\n", workspace_id);
    waitpid(subprocess_pid, NULL, 0);
    printf("\n[WORKSPACE %i]: Done waiting!\n", workspace_id);

    char workspace_windows_str[WINDOWS_COUNT_SIZE];

    ssize_t bytes_read = read(aerospace_fd[0], &workspace_windows_str, WINDOWS_COUNT_SIZE - 1);
    if (bytes_read > 0) {
        workspace_windows_str[bytes_read] = '\0';
    } else {
        workspace_windows_str[0] = '0';
        workspace_windows_str[1] = '\0';
    }

    close(aerospace_fd[0]);

    char command[SKETCHYBAR_COMMAND_SIZE];
    if (atoi(workspace_windows_str) > 0) {
        snprintf(command, SKETCHYBAR_COMMAND_SIZE, "--set space.%i drawing=on", workspace_id);
        printf("[WORKSPACE %i]: Drawing ON (%s)\n", workspace_id, command);
    } else {
        snprintf(command, SKETCHYBAR_COMMAND_SIZE, "--set space.%i drawing=off", workspace_id);
        printf("[WORKSPACE %i]: Drawing OFF (%s)\n", workspace_id, command);
    }
    char* response = sketchybar(command);
    printf("[SKETCHYBAR]: %s\n", response);

    printf("[WORKSPACE %i]: Bye-bye\n\n", workspace_id);
}


pid_t update_workspace__old(short workspace_id) {
    pid_t pid = fork();

    // Error with fork
    if (pid == -1) {
        perror("Failed to create watcher");
        exit(-1);
    }

    // Parent process...
    if (pid > 0) return pid;

    // Child process...
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

    // aerospace subprocess
    if (aerospace_cmd_pid == 0) {
        get_aerospace_workspace_count(workspace_id, aerospace_fd);
        exit(0);
    }

    waitpid(aerospace_cmd_pid, NULL, 0);
    toggle_workspace_indicator(workspace_id, aerospace_fd, aerospace_cmd_pid);
    exit(0);
}

typedef struct  {
    short workspace_id;
} update_workspace_args;

void *update_workspace(void *_args) {
    update_workspace_args *args = _args;
    // Child process...
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

    // aerospace subprocess
    if (aerospace_cmd_pid == 0) {
        get_aerospace_workspace_count(args->workspace_id, aerospace_fd);
        exit(0);
    }

    waitpid(aerospace_cmd_pid, NULL, 0);
    toggle_workspace_indicator(args->workspace_id, aerospace_fd, aerospace_cmd_pid);
    return 0;
}
void update_all_workspaces__old() {
    pid_t watchers_pids[TOTAL_WORKSPACES];

    for (short i = 1; i <= TOTAL_WORKSPACES; i++) {
        watchers_pids[(i-1)] = update_workspace__old(i);
    }

    for (short i = 1; i <= TOTAL_WORKSPACES; i++) wait(NULL);
}
void update_all_workspaces() {
    pthread_t watchers_pids[TOTAL_WORKSPACES];

    for (short i = 1; i <= TOTAL_WORKSPACES; i++) {
        update_workspace_args *args = malloc(sizeof *args);
        args->workspace_id = i;
        if (pthread_create(watchers_pids + i-1, NULL, update_workspace, args)) free(args);
    }

    for (short i = 1; i <= TOTAL_WORKSPACES; i++) wait(NULL);
}


void main_handler(env env) {
    paint_current_workspace(env);
    update_all_workspaces();
}

int main(int argc, char** argv) {
    event_server_begin(main_handler, "ash");

    return 0;
}
