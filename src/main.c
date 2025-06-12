#include <stdio.h>
#include <stdlib.h>

#include "../deps/SketchyBarHelper/sketchybar.h"

#define DEFAULT_LABEL_COLOR "0xFFFFFFFF"
#define SELECTED_LABEL_COLOR "0xFF00A3CB"

short workspaces_in_use = 0;

void handler(env env) {
    char* previous_workspace = env_get_value_for_key(env, "PREVIOUS_WORKSPACE");
    char* focused_workspace = env_get_value_for_key(env, "FOCUSED_WORKSPACE");

    if (previous_workspace == NULL || previous_workspace[0] == '\0') return;
    if (focused_workspace == NULL || focused_workspace[0] == '\0') return;

    short focused_workspace_update = 0b1 << atoi(focused_workspace);
    short previous_workspace_update = 0b1 << atoi(previous_workspace);

    char command[256];

    snprintf(command, 256, "--set space.%s label.color=" SELECTED_LABEL_COLOR " drawing=on", focused_workspace);
    sketchybar(command);

    snprintf(command, 256, "--set space.%s label.color=" DEFAULT_LABEL_COLOR " drawing=on", previous_workspace);
    sketchybar(command);

    workspaces_in_use &= ~previous_workspace_update;
    workspaces_in_use |= focused_workspace_update;

    for(short i = 0; i < 0x1; i++) {
        if ((workspaces_in_use >> i) & 0b1) {
            snprintf(command, 256, "--set space.%s drawing=on", previous_workspace);
        }
        else {
            snprintf(command, 256, "--set space.%s drawing=off", previous_workspace);
        }
        sketchybar(command);
    }

}

int main(int argc, char** argv) {
    if (argc == 1) {
        return -1;
    }

    event_server_begin(handler, "ash");
    return 0;
}
