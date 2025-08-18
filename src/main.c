#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "skard.h"

static char *read_file(const char *filename);

static void help(const char *prog_name);
static int repl(void);
static int file(const char *filename);
static int ast(const char *filename);

int main(int argc, char **argv)
{
    if (argc == 1) {
        return repl();
    }

    if (argc > 1) {
        const char *command = argv[1];
        size_t command_length = strlen(command);

        if (command_length == 4 && memcmp(command, "repl", 4) == 0) {
            return repl();
        }

        if (command_length == 3 && memcmp(command, "run", 3) == 0 && argc > 2) {
            return file(argv[2]);
        }

        if (command_length == 3 && memcmp(command, "ast", 3) == 0 && argc > 2) {
            return ast(argv[2]);
        }

        if (command_length == 4 && memcmp(command, "help", 4) == 0) {
            help(argv[0]);
            return EXIT_SUCCESS;
        }
    }

    if (argc >= 1) {
        help(argv[0]);
    } else {
        help(NULL);
    }

    return EXIT_FAILURE;
}

static void help(const char *prog_name)
{
    if (prog_name == NULL) {
        prog_name = "skard";
    }

    fprintf(stderr, "Skard\n");
    fprintf(stderr, "Usage: %s [command]\n", prog_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "Commands:\n");
    fprintf(stderr, "  %-15s %s\n", "repl", "Start an interactive session (default).");
    fprintf(stderr, "  %-15s %s\n", "run <file>", "Execute the specified file.");
    fprintf(stderr, "  %-15s %s\n", "ast <file>", "Generate and print the AST of the specified file.");
    fprintf(stderr, "  %-15s %s\n", "help", "Show this help message.");
}

static int repl(void)
{
    // TODO: Not yet implemented.
    return EXIT_FAILURE;
}

static int file(const char *filename)
{
    char *source = read_file(filename);
    if (source == NULL) {
        return EXIT_FAILURE;
    }

    struct sk_parser parser;
    sk_parser_init(&parser, source);

    struct sk_ast_node *ast = sk_parser_parse(&parser);

    struct sk_chunk chunk;
    sk_chunk_init(&chunk);

    struct sk_compiler compiler;
    sk_compiler_compile(&compiler, ast, &chunk);

    struct sk_vm vm;
    sk_vm_init(&vm);

    sk_vm_run(&vm, &chunk);

    sk_vm_free(&vm);

    sk_chunk_free(&chunk);

    free(source);
    return EXIT_SUCCESS;
}

static int ast(const char *filename)
{
    char *source = read_file(filename);
    if (source == NULL) {
        return EXIT_FAILURE;
    }

    struct sk_parser parser;
    sk_parser_init(&parser, source);

    struct sk_ast_node *node = sk_parser_parse(&parser);
    sk_ast_node_print(node);

    free(source);
    return EXIT_SUCCESS;
}

static char *read_file(const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file '%s'.", filename);
        return NULL;
    }

    fseek(file, 0L, SEEK_END);

    size_t size = ftell(file);
    rewind(file);

    char *buffer = malloc(size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read file '%s'.", filename);
        return NULL;
    }

    size_t read = fread(buffer, sizeof(char), size, file);
    if (read < size) {
        free(buffer);
        fprintf(stderr, "Could not read file '%s'.", filename);
        return NULL;
    }

    buffer[read] = '\0';

    fclose(file);
    return buffer;
}
