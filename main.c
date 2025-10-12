
#include <stdio.h>
#include "cli.c"

static int runWrite(struct command *cmd) {
    printf("RUNNING WRITE\n");

    for (int i = 0; i < cmd->argumentCount; i++) {
        printf("%s: %s\n", cmd->arguments[i].name, cmd->arguments[i].value);
    }

    for (int i = 0; i < cmd->optionCount; i++) {
        printf("%s: %s\n", cmd->options[i].name, cmd->options[i].value);
    }

    return 0;
}

void initWriteCmd(struct command *parent, struct command *cmd) {
    static struct argument arguments[] = {
        {
            .name = "file",
            .description = "Input image filename",
        },
        {
            .name = "content",
            .description = "Text to be hidden inside the image",
        }
    };

    static struct option options[] = {
        {
            .name = "output",
            .shorthand = 'o',
            .description = "Output filename",
        },
        {
            .name = "channel",
            .shorthand = 'c',
            .description = "Color channel(s) to use for the steganography. (Allowed values: rgb)",
        }
    };

    *cmd = (struct command){
        .name = "write",
        .description = "The write command is used to write any hidden text onto an image using steganography procedures.",
        .shortDescription = "Hides text on an image",
        .parent = parent,
        .arguments = arguments,
        .argumentCount = 2,
        .options = options,
        .optionCount = 2,
        .run = runWrite,
    };
}

void initRootCmd(struct command *cmd) {
    *cmd = (struct command){
        .name = "stego",
        .description = "stego CLI v1.0.0\n\n"
            "This steganography tool allows you to hide text on images or to read hidden text from images.\n\n"
        "Made by:\n"
        "Anujan Sivakurunathan\n"
        "Kevin Krummenacher\n"
        "Tamino Walter",
    };

    static struct command subCommands[1];
    initWriteCmd(cmd, &subCommands[0]);

    cmd->subcommands = subCommands;
    cmd->subcommandCount = 1;
}

int main(int argc, char **argv) {
    struct command root;
    initRootCmd(&root);

    return executeCommand(root, argc, argv, 1);
}
