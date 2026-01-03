
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct argument {
    char *name;
    char *description;
    char *value;
};

struct option {
    char *name;
    char shorthand;
    char *description;
    char *value;
};

struct command {
    char *name;
    char *description;
    char *shortDescription;

    char **examples;
    int exampleCount;

    struct command *parent;

    struct command *subcommands;
    int subcommandCount;

    struct argument *arguments;
    int argumentCount;

    struct option *options;
    int optionCount;

    int(*run)(struct command *cmd);
};

void printHelp(struct command *cmd);

int executeCommand(struct command cmd, int argc, char **argv, const int offsetI) {
    int foundArguments = 0;

    // check and parse all arguments provided in the app call
    for (int i = offsetI; i < argc; i++) {
        char *arg = argv[i];

        // check subcommands
        for (int j = 0; j < cmd.subcommandCount; j++) {
            if (strcmp(cmd.subcommands[j].name, arg) == 0) {
                // subcommand found - continue by parsing the subcommand
                return executeCommand(cmd.subcommands[j], argc, argv, offsetI + 1);
            }
        }

        // check options
        if (strlen(arg) >= 2 && arg[0] == '-') {
            // show help page
            if (arg[1] == 'h' || strcmp(arg, "--help") == 0) {
                printHelp(&cmd);
                return 0;
            }

            // verify value is provided
            if (i + 1 >= argc) {
                printf("missing value for option %s\n", arg);
                return -1;
            }

            // get the value for the option (the argument provided next, after the option's name)
            // TODO allow using --option=value?
            char *value = argv[i + 1];

            bool found = false;

            for (int j = 0; j < cmd.optionCount; j++) {
                struct option *opt = &cmd.options[j];

                found = (arg[1] == opt->shorthand) ||
                    (strlen(arg) >= 3 && arg[1] == '-' && strcmp(&arg[2], opt->name) == 0);

                // apply the option's value and stop searching
                if (found) {
                    opt->value = value;
                    i++;
                    break;
                }
            }

            if (found) {
                continue;
            }

            // the option was not found on the command
            printf("invalid option %s\n", arg);
            return -1;
        }

        // verify invalid subcommand (if sub commands are present one must be selected)
        if (cmd.subcommandCount > 0) {
            printf("invalid command \"%s\"\n", arg);
            return -1;
        }

        // treat as an argument
        if (foundArguments >= cmd.argumentCount) {
            printf("too many arguments\n");
            return -1;
        }

        cmd.arguments[foundArguments].value = arg;
        foundArguments++;
    }

    // not all required arguments were passed
    if (foundArguments < cmd.argumentCount) {
        printf("missing arguments\n");
        return -1;
    }

    // run the parsed command
    if (cmd.run) {
        return cmd.run(&cmd);
    }

    // show help when no argument is passed
    printHelp(&cmd);
    return 0;
}

char *getArgument(struct command *cmd, char *name) {
    for (int i = 0; i < cmd->argumentCount; i++) {
        if (strcmp(cmd->arguments[i].name, name) == 0) {
            return cmd->arguments[i].value;
        }
    }
    return NULL;
}

int getArgumentInt(struct command *cmd, char *name) {
    char *value = getArgument(cmd, name);
    if (value == NULL) {
        return 0;
    }
    return (int)strtol(value, NULL, 10);
}

char *getOption(struct command *cmd, char *name) {
    for (int i = 0; i < cmd->optionCount; i++) {
        if (strcmp(cmd->options[i].name, name) == 0) {
            return cmd->options[i].value;
        }
    }
    return NULL;
}

int getOptionInt(struct command *cmd, char *name) {
    char *value = getOption(cmd, name);
    if (value == NULL) {
        return 0;
    }
    return (int)strtol(value, NULL, 10);
}

char* fullCommandPath(struct command* cmd) {
    if (cmd->parent == NULL) {
        return _strdup(cmd->name);
    }

    char* parentName = fullCommandPath(cmd->parent);

    // 1. Länge berechnen und in Variable speichern
    size_t needed = strlen(parentName) + strlen(cmd->name) + 2; // +1 Leerzeichen, +1 Nullterminator
    char* fullName = malloc(needed);

    if (fullName == NULL) {
        free(parentName);
        return NULL;
    }

    // 2. Nutze 'needed' statt 'sizeof(fullName)'
    int result = snprintf(fullName, needed, "%s %s", parentName, cmd->name);

    if (result < 0 || result >= needed) {
        // Fehlerbehandlung oder Abschneidung erkannt
    }

    free(parentName);

    return fullName;
}

void printHelp(struct command *cmd) {
    printf("%s\n", cmd->description);
    printf("\n");

    // Compute usage string
    char *usage = fullCommandPath(cmd);

    if (cmd->subcommandCount > 0) {
        // first options then subcommands
        if (cmd->optionCount > 0) {
            asprintf(&usage, "%s [options]", usage);
        }

        asprintf(&usage, "%s [command]", usage);
    } else {
        // first arguments then options
        for (int i = 0; i < cmd->argumentCount; i++) {
            asprintf(&usage, "%s <%s>", usage, cmd->arguments[i].name);
        }

        if (cmd->optionCount > 0) {
            asprintf(&usage, "%s [options]", usage);
        }
    }

    printf("Usage:\n");
    printf("\t%s\n", usage);
    free(usage);

    if (cmd->exampleCount > 0) {
        printf("\n");
        printf("Examples:\n");
        for (int i = 0; i < cmd->exampleCount; i++) {
            printf("\t%s\n", cmd->examples[i]);
        }
    }

    if (cmd->subcommandCount > 0) {
        printf("\n");
        printf("Available Commands:\n");

        int maxLen = 0;

        for (int i = 0; i < cmd->subcommandCount; i++) {
            int len = (int)strlen(cmd->subcommands[i].name);
            if (len > maxLen) {
                maxLen = len;
            }
        }

        for (int i = 0; i < cmd->subcommandCount; i++) {
            printf(
                "\t%-*s  %s\n",
                maxLen,
                cmd->subcommands[i].name,
                cmd->subcommands[i].shortDescription ? cmd->subcommands[i].shortDescription : ""
            );
        }
    }

    if (cmd->argumentCount > 0) {
        printf("\n");
        printf("Arguments:\n");

        int maxLen = 0;

        for (int i = 0; i < cmd->argumentCount; i++) {
            int len = (int)strlen(cmd->arguments[i].name);
            if (len > maxLen) {
                maxLen = len;
            }
        }

        for (int i = 0; i < cmd->argumentCount; i++) {
            printf(
                "\t%-*s    %s\n",
                maxLen,
                cmd->arguments[i].name,
                cmd->arguments[i].description ? cmd->arguments[i].description : ""
            );
        }
    }

    printf("\n");
    printf("Options:\n");

    // minimum of 4 required for the standard help option
    int maxLen = 4;

    for (int i = 0; i < cmd->optionCount; i++) {
        int len = (int)strlen(cmd->options[i].name);
        if (len > maxLen) {
            maxLen = len;
        }
    }

    for (int i = 0; i < cmd->optionCount; i++) {
        printf(
            "\t-%c, --%-*s  %s\n",
            cmd->options[i].shorthand,
            maxLen,
            cmd->options[i].name,
            cmd->options[i].description ? cmd->options[i].description : ""
        );
    }

    printf(
        "\t-h, --%-*s  Show this help dialog\n",
        maxLen,
        "help"
    );

    if (cmd->subcommandCount > 0) {
        printf("\n");
        printf("Use \"%s [command] --help\" for more information about a command.", cmd->name);
    }
}
