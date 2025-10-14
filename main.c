
#include <stdio.h>
#include "cli.c"

static int runEmbed(struct command *cmd) {
    printf("RUNNING EMBED\n");

    for (int i = 0; i < cmd->argumentCount; i++) {
        printf("%s: %s\n", cmd->arguments[i].name, cmd->arguments[i].value);
    }

    for (int i = 0; i < cmd->optionCount; i++) {
        printf("%s: %s\n", cmd->options[i].name, cmd->options[i].value);
    }

    return 0;
}

void initEmbedCmd(struct command *parent, struct command *cmd) {
    static struct argument arguments[] = {
        {
            .name = "file",
            .description = "Input image filename",
        },
        {
            .name = "content",
            .description = "Text or file to be hidden inside the image",
        },
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
        },
    };

    *cmd = (struct command){
        .name = "embed",
        .description = "The embed command is used to embed any hidden text or file onto an image using steganography procedures.",
        .shortDescription = "Hides some content inside an image",
        .parent = parent,
        .arguments = arguments,
        .argumentCount = 2,
        .options = options,
        .optionCount = 2,
        .run = runEmbed,
    };

    char *fullName = fullCommandPath(cmd);
    char **examples = malloc(5 * sizeof(char *));

    asprintf(&examples[0], "%s input.png \"My hidden message\"", fullName);
    asprintf(&examples[1], "%s input.png ./my_hidden_file.txt", fullName);
    asprintf(&examples[2], "%s input.png \"Example text\" --output output.png", fullName);
    asprintf(&examples[3], "%s input.png \"Example text\" --channel r", fullName);
    asprintf(&examples[4], "%s input.png \"Example text\" -o output.png -c rgb", fullName);

    free(fullName);

    cmd->examples = examples;
    cmd->exampleCount = 5;
}

void initExtractCmd(struct command *parent, struct command *cmd) {
    static struct argument arguments[] = {
        {
            .name = "file",
            .description = "Input image filename",
        },
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
        .name = "extract",
        .description = "The extract command is used to extract any hidden text or file from an image which was hidden using steganography procedures.",
        .shortDescription = "Extracts some hidden content from an image",
        .parent = parent,
        .arguments = arguments,
        .argumentCount = 1,
        .options = options,
        .optionCount = 2,
        .run = NULL,
    };
}

void initCapacityCmd(struct command *parent, struct command *cmd) {
    static struct argument arguments[] = {
        {
            .name = "file",
            .description = "Input image filename",
        },
    };

    static struct option options[] = {
        {
            .name = "channel",
            .shorthand = 'c',
            .description = "Color channel(s) to use for the steganography. (Allowed values: rgb)",
        }
    };

    *cmd = (struct command){
        .name = "capacity",
        .description = "The capacity command is used to evaluate the capacity of the content which can be hidden inside the provided image file.",
        .shortDescription = "Get capacity of a file",
        .parent = parent,
        .arguments = arguments,
        .argumentCount = 1,
        .options = options,
        .optionCount = 1,
        .run = NULL,
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

    static struct command subCommands[3];
    initEmbedCmd(cmd, &subCommands[0]);
    initExtractCmd(cmd, &subCommands[1]);
    initCapacityCmd(cmd, &subCommands[2]);

    cmd->subcommands = subCommands;
    cmd->subcommandCount = 3;
}

int main(int argc, char **argv) {
    struct command root;
    initRootCmd(&root);

    return executeCommand(root, argc, argv, 1);
}
