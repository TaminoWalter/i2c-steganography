#include <stdio.h>

// link other c files
#include "cli.c"
#include "image-bmp.c"
#include "image-png.c"

#ifdef _WIN32
#include <string.h>
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#else
#include <strings.h>
#endif

int asprintf(char** strp, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    // 1. Berechne die benötigte Länge (ohne zu schreiben)
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    if (len < 0) return -1;

    // 2. Speicher reservieren (+1 für den Null-Terminator)
    *strp = (char*)malloc(len + 1);
    if (!*strp) return -1;

    // 3. Tatsächlich in den neuen Speicher schreiben
    va_start(args, fmt);
    len = vsnprintf(*strp, len + 1, fmt, args);
    va_end(args);

    return len;
}

// Kleine Hilfsfunktion, um die Dateiendung zu finden
const char *getFileExtension(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1;
}

// Prüft, ob es ein PNG ist (Gross-/Kleinschreibung egal)
int isPng(const char *filename) {
    const char *ext = getFileExtension(filename);
    return (strcasecmp(ext, "png") == 0);
}

// Hilfsfunktion: Versucht eine Textdatei zu lesen
// Gibt NULL zurück, wenn die Datei nicht existiert.
char* readFileContent(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        return NULL; // Datei existiert nicht -> Es ist wohl ein normaler Text-String
    }

    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    rewind(f);

    // Speicher reservieren (+1 für das Null-Byte am Ende)
    char* buffer = malloc(length + 1);
    if (!buffer) {
        fclose(f);
        return NULL;
    }

    fread(buffer, 1, length, f);
    buffer[length] = '\0'; // String abschließen
    fclose(f);

    return buffer;
}

static int runEmbed(struct command *cmd) {
    char *inputFile = getArgument(cmd, "file");
    char *rawContentArg = getArgument(cmd, "content");

    char *messageToEmbed = NULL;
    int mustFreeMessage = 0; // Merker, ob wir den Speicher später freigeben müssen

    // 1. Versuch: Ist das Argument ein Dateipfad?
    char *fileContent = readFileContent(rawContentArg);

    if (fileContent != NULL) {
        // Ja, Datei gefunden! Inhalt nutzen.
        messageToEmbed = fileContent;
        mustFreeMessage = 1; // Wir haben malloc in readFileContent genutzt
        printf("Reading content from file: '%s' (%lu bytes)\n", rawContentArg, strlen(messageToEmbed));
    } else {
        // Nein, Datei nicht gefunden. Wir nutzen das Argument direkt als Text.
        messageToEmbed = rawContentArg;
        mustFreeMessage = 0; // Das ist nur ein Pointer auf argv, nicht freigeben!
        printf("Embedding raw text string.\n");
    }

    // 2. Output Dateiname bestimmen
    char *outputFile = getOption(cmd, "output");

    // Automatisch entscheiden: Ist der Input ein PNG?
    if (isPng(inputFile)) {
        if (outputFile == NULL) outputFile = "out.png";
        embedMessagePNG(inputFile, outputFile, messageToEmbed);
    } else {
        if (outputFile == NULL) outputFile = "out.bmp";
        embedMessage(inputFile, outputFile, messageToEmbed);
    }

    // 3. Wichtig: Speicher aufräumen, falls wir eine Datei gelesen haben
    if (mustFreeMessage) {
        free(messageToEmbed);
    }

    return 0;
}

void initEmbedCmd(struct command *parent, struct command *cmd) {
    static struct argument arguments[] = {
        {
            .name = "file",
            .description = "Input image filename (.png or .bmp supported)",
        },
        {
            .name = "content",
            .description = "Text to be hidden inside the image",
        },
    };

    static struct option options[] = {
        {
            .name = "output",
            .shorthand = 'o',
            .description = "Output filename",
        },
    };

    *cmd = (struct command){
        .name = "embed",
        .description = "The embed command is used to embed any hidden text or file onto an image using steganography procedures.",
        .shortDescription = "Hides some content inside an image (.png or .bmp supported)",
        .parent = parent,
        .arguments = arguments,
        .argumentCount = 2,
        .options = options,
        .optionCount = 1,
        .run = runEmbed,
    };

    char *fullName = fullCommandPath(cmd);
    char **examples = malloc(5 * sizeof(char *));

    asprintf(&examples[0], "%s sample.png \"My hidden message\"", fullName);
    asprintf(&examples[1], "%s sample.bmp \"My hidden message\"", fullName);
    asprintf(&examples[2], "%s sample.png topSecret.txt", fullName);
    asprintf(&examples[3], "%s sample.png \"Example text\" --output output.png", fullName);
    asprintf(&examples[4], "%s sample.bmp \"Example text\" -o output.bmp", fullName);

    free(fullName);

    cmd->examples = examples;
    cmd->exampleCount = 5;
}

static int runExtract(struct command *cmd) {
    char *inputFile = getArgument(cmd, "file");

    //Output Option abrufen
    char *outputFile = getOption(cmd, "output");

    if (isPng(inputFile)) {
        extractMessagePNG(inputFile, outputFile);
    } else {
        extractMessage(inputFile, outputFile);
    }

    return 0;
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
            .description = "Output filename (should be .txt)",
        }
    };

    *cmd = (struct command){
        .name = "extract",
        .description = "The extract command is used to extract any hidden text or file from an image which was hidden using steganography procedures.",
        .shortDescription = "Extracts some hidden content from an image (.png or .bmp supported)",
        .parent = parent,
        .arguments = arguments,
        .argumentCount = 1,
        .options = options,
        .optionCount = 1,
        .run = runExtract,
    };

    char *fullName = fullCommandPath(cmd);
    char **examples = malloc(4 * sizeof(char *));

    asprintf(&examples[0], "%s out.png", fullName);
    asprintf(&examples[1], "%s out.bmp", fullName);
    asprintf(&examples[2], "%s out.png -o exfiltratedData.txt", fullName);
    asprintf(&examples[3], "%s out.png --output exfiltratedData.txt", fullName);

    free(fullName);

    cmd->examples = examples;
    cmd->exampleCount = 4;
}

static int runCapacity(struct command *cmd) {
    char *inputFile = getArgument(cmd, "file");
    long capacity = 0;

    // 1. Kapazität ermitteln
    if (isPng(inputFile)) {
        capacity = getPngCapacity(inputFile);
    } else {
        capacity = getBmpCapacity(inputFile);
    }

    // 2. Fehlerprüfung
    if (capacity < 0) {
        printf("Error: Could not read image file or format invalid.\n");
        return 1;
    }
    if (capacity == 0) {
        printf("Image is too small to hold any data.\n");
        return 0;
    }

    // 3. Konstanten für die Schätzung
    // Durchschnittswerte:
    // - Ein Wort: ca. 5 Buchstaben + 1 Leerzeichen = 6 Bytes
    // - Eine A4 Seite (einfacher Zeilenabstand): ca. 3000 Zeichen
    // - Die Bibel (als Referenz für riesige Mengen): ca. 4.000.000 Zeichen
    const long BYTES_PER_WORD = 6;
    const long BYTES_PER_PAGE = 3000;
    const long BYTES_PER_BIBLE = 4000000;

    // 4. Ausgabe formatieren
    printf("---------------------------------------------\n");
    printf("CAPACITY ANALYSIS: %s\n", inputFile);
    printf("---------------------------------------------\n");

    // Rohe Bytes / MB Anzeige
    if (capacity > 1024 * 1024) {
        printf("Max. hidden data   : %.2f MB (%ld bytes)\n", (double)capacity / (1024 * 1024), capacity);
    } else {
        printf("Max. hidden data   : %.2f KB (%ld bytes)\n", (double)capacity / 1024, capacity);
    }

    printf("\nEstimated Text Content:\n");

    // Zeichen
    printf("  - %-10ld Characters (ASCII)\n", capacity);

    // Wörter
    printf("  - %-10ld Words (approx.)\n", capacity / BYTES_PER_WORD);

    // Seiten
    long pages = capacity / BYTES_PER_PAGE;
    if (pages > 0) {
        printf("  - %-10ld A4 Pages (full text)\n", pages);
    } else {
        printf("  - < 1        A4 Page\n");
    }

    // Fun Fact bei grossen Bildern
    if (capacity > BYTES_PER_BIBLE) {
        printf("\nWow! You could hide the entire Bible %.1fx times in this image.\n",
               (double)capacity / BYTES_PER_BIBLE);
    }

    printf("---------------------------------------------\n");

    return 0;
}

void initCapacityCmd(struct command *parent, struct command *cmd) {
    static struct argument arguments[] = {
        {
            .name = "file",
            .description = "Input image filename",
        },
    };

    *cmd = (struct command){
        .name = "capacity",
        .description = "The capacity command evaluates how many bytes can be hidden inside the provided image.",
        .shortDescription = "Check max message size for an image",
        .parent = parent,
        .arguments = arguments,
        .argumentCount = 1,
        .options = NULL,
        .optionCount = 0,
        .run = runCapacity,
    };
}

void initRootCmd(struct command *cmd) {
    *cmd = (struct command){
        .name = "stego",
        .description = "stego CLI v1.1.0 - Supports PNG and BMP\n\n"
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
