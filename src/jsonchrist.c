#include "json_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

// ANSI color codes
#define COLOR_RESET   "\x1b[0m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_WHITE   "\x1b[37m"
#define COLOR_RED     "\x1b[31m"

// Global output file pointer
static FILE* output = NULL;

typedef struct {
    bool tree;
    bool pretty;
    bool compact;
    bool flatten;
    bool stream;
    bool validate;
    bool stats;
    bool highlight;
    bool edit;
    bool index;
    bool no_color;
    size_t indent;
    const char* input_file;
    const char* output_file;
} Options;

static void print_usage(const char* program) {
    fprintf(stderr, "Usage: %s [options] input.json\n", program);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --tree           Output hierarchical tree structure\n");
    fprintf(stderr, "  --pretty         Output formatted JSON\n");
    fprintf(stderr, "  --compact        Output compact JSON\n");
    fprintf(stderr, "  --flatten        Output flattened key-value pairs\n");
    fprintf(stderr, "  --stream         Output parsing events stream\n");
    fprintf(stderr, "  --validate       Validate JSON and show errors\n");
    fprintf(stderr, "  --stats          Output JSON statistics\n");
    fprintf(stderr, "  --highlight      Output syntax-highlighted JSON\n");
    fprintf(stderr, "  --edit           Output editable node structure\n");
    fprintf(stderr, "  --index          Output searchable index\n");
    fprintf(stderr, "  --no-color       Disable colored output\n");
    fprintf(stderr, "  --indent N       Set indentation level (default: 4)\n");
    fprintf(stderr, "  -o, --output FILE Write output to FILE\n");
    fprintf(stderr, "  -h, --help       Display this help message\n");
    fprintf(stderr, "\nExamples:\n");
    fprintf(stderr, "  %s --tree input.json\n", program);
    fprintf(stderr, "  %s --pretty --indent 2 input.json\n", program);
    fprintf(stderr, "  %s --validate --stats input.json\n", program);
}

static Options parse_options(int argc, char* argv[]) {
    Options opts = {
        .indent = 4,  // Default indentation
        .no_color = false
    };
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--tree") == 0) opts.tree = true;
        else if (strcmp(argv[i], "--pretty") == 0) opts.pretty = true;
        else if (strcmp(argv[i], "--compact") == 0) opts.compact = true;
        else if (strcmp(argv[i], "--flatten") == 0) opts.flatten = true;
        else if (strcmp(argv[i], "--stream") == 0) opts.stream = true;
        else if (strcmp(argv[i], "--validate") == 0) opts.validate = true;
        else if (strcmp(argv[i], "--stats") == 0) opts.stats = true;
        else if (strcmp(argv[i], "--highlight") == 0) opts.highlight = true;
        else if (strcmp(argv[i], "--edit") == 0) opts.edit = true;
        else if (strcmp(argv[i], "--index") == 0) opts.index = true;
        else if (strcmp(argv[i], "--no-color") == 0) opts.no_color = true;
        else if (strcmp(argv[i], "--indent") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "Error: --indent requires a number\n");
                exit(1);
            }
            opts.indent = atoi(argv[i]);
            if (opts.indent > 8) {
                fprintf(stderr, "Warning: Large indentation may cause wide output\n");
            }
        }
        else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "Error: -o/--output requires a filename\n");
                exit(1);
            }
            opts.output_file = argv[i];
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            exit(0);
        }
        else if (argv[i][0] != '-') {
            if (opts.input_file) {
                fprintf(stderr, "Error: Multiple input files specified\n");
                exit(1);
            }
            opts.input_file = argv[i];
        }
        else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            exit(1);
        }
    }
    
    if (!opts.input_file) {
        fprintf(stderr, "Error: No input file specified\n");
        exit(1);
    }
    
    // If no output format is specified, default to pretty print
    if (!opts.tree && !opts.pretty && !opts.compact && !opts.flatten &&
        !opts.stream && !opts.validate && !opts.stats && !opts.highlight &&
        !opts.edit && !opts.index) {
        opts.pretty = true;
    }
    
    return opts;
}

static void print_path_value(const TreeNode* node, char* path, size_t path_len) {
    switch (node->type) {
        case JSON_NULL:
            fprintf(output, "%s: null\n", path);
            break;
        case JSON_BOOL:
        case JSON_NUMBER:
            fprintf(output, "%s: %s\n", path, node->value);
            break;
        case JSON_STRING:
            fprintf(output, "%s: \"%s\"\n", path, node->value);
            break;
        case JSON_ARRAY:
        case JSON_OBJECT:
            for (size_t i = 0; i < node->children_count; i++) {
                const TreeNode* child = node->children[i];
                size_t new_len = path_len;
                char new_path[JSON_PATH_MAX_LENGTH];
                strncpy(new_path, path, JSON_PATH_MAX_LENGTH);
                
                if (node->type == JSON_ARRAY) {
                    new_len += snprintf(new_path + path_len, JSON_PATH_MAX_LENGTH - path_len,
                                      "[%zu]", i);
                } else {
                    new_len += snprintf(new_path + path_len, JSON_PATH_MAX_LENGTH - path_len,
                                      ".%s", child->name);
                }
                
                print_path_value(child, new_path, new_len);
            }
            break;
    }
}

static void print_stream_events(const TreeNode* node) {
    switch (node->type) {
        case JSON_OBJECT:
            fprintf(output, "START_OBJECT\n");
            for (size_t i = 0; i < node->children_count; i++) {
                const TreeNode* child = node->children[i];
                fprintf(output, "FIELD_NAME: \"%s\"\n", child->name);
                print_stream_events(child);
            }
            fprintf(output, "END_OBJECT\n");
            break;
            
        case JSON_ARRAY:
            fprintf(output, "START_ARRAY\n");
            for (size_t i = 0; i < node->children_count; i++) {
                print_stream_events(node->children[i]);
            }
            fprintf(output, "END_ARRAY\n");
            break;
            
        case JSON_STRING:
            fprintf(output, "VALUE_STRING: \"%s\"\n", node->value);
            break;
            
        case JSON_NUMBER:
            fprintf(output, "VALUE_NUMBER: %s\n", node->value);
            break;
            
        case JSON_BOOL:
            fprintf(output, "VALUE_BOOLEAN: %s\n", node->value);
            break;
            
        case JSON_NULL:
            fprintf(output, "VALUE_NULL\n");
            break;
    }
}

static void print_validation_result(const JsonParser* parser) {
    if (parser->error_count == 0) {
        fprintf(output, "Valid JSON.\n");
    } else {
        fprintf(output, "{\n    \"valid\": false,\n    \"errors\": [\n");
        for (size_t i = 0; i < parser->error_count; i++) {
            const ValidationError* error = &parser->errors[i];
            fprintf(output, "        {\n");
            fprintf(output, "            \"message\": \"%s\",\n", error->message);
            fprintf(output, "            \"position\": { \"line\": %zu, \"column\": %zu }\n",
                   error->position.line, error->position.column);
            fprintf(output, "        }%s\n", i < parser->error_count - 1 ? "," : "");
        }
        fprintf(output, "    ]\n}\n");
    }
}

static void print_stats(const JsonStats* stats) {
    fprintf(output, "Total Keys: %zu\n", stats->total_keys);
    fprintf(output, "Total Values: %zu\n", stats->total_values);
    fprintf(output, "Depth: %zu\n", stats->depth);
    fprintf(output, "Types:\n");
    fprintf(output, "    - Strings: %zu\n", stats->types.string_count);
    fprintf(output, "    - Numbers: %zu\n", stats->types.number_count);
    fprintf(output, "    - Booleans: %zu\n", stats->types.bool_count);
    fprintf(output, "    - Nulls: %zu\n", stats->types.null_count);
    fprintf(output, "    - Arrays: %zu\n", stats->types.array_count);
    fprintf(output, "    - Objects: %zu\n", stats->types.object_count);
}

static void print_highlighted_value(const TreeNode* node, int indent) {
    for (int i = 0; i < indent; i++) fprintf(output, " ");
    
    switch (node->type) {
        case JSON_NULL:
            fprintf(output, "%s%s%s", COLOR_BLUE, "null", COLOR_RESET);
            break;
        case JSON_BOOL:
            fprintf(output, "%s%s%s", COLOR_BLUE, node->value, COLOR_RESET);
            break;
        case JSON_NUMBER:
            fprintf(output, "%s%s%s", COLOR_BLUE, node->value, COLOR_RESET);
            break;
        case JSON_STRING:
            fprintf(output, "%s\"%s\"%s", COLOR_YELLOW, node->value, COLOR_RESET);
            break;
        case JSON_ARRAY:
            fprintf(output, "%s[%s\n", COLOR_WHITE, COLOR_RESET);
            for (size_t i = 0; i < node->children_count; i++) {
                print_highlighted_value(node->children[i], indent + 4);
                if (i < node->children_count - 1) {
                    fprintf(output, "%s,%s\n", COLOR_WHITE, COLOR_RESET);
                } else {
                    fprintf(output, "\n");
                }
            }
            for (int i = 0; i < indent; i++) fprintf(output, " ");
            fprintf(output, "%s]%s", COLOR_WHITE, COLOR_RESET);
            break;
        case JSON_OBJECT:
            fprintf(output, "%s{%s\n", COLOR_WHITE, COLOR_RESET);
            for (size_t i = 0; i < node->children_count; i++) {
                const TreeNode* child = node->children[i];
                for (int j = 0; j < indent + 4; j++) fprintf(output, " ");
                fprintf(output, "%s\"%s\"%s%s: %s", COLOR_GREEN, child->name,
                       COLOR_RESET, COLOR_WHITE, COLOR_RESET);
                print_highlighted_value(child, 0);
                if (i < node->children_count - 1) {
                    fprintf(output, "%s,%s\n", COLOR_WHITE, COLOR_RESET);
                } else {
                    fprintf(output, "\n");
                }
            }
            for (int i = 0; i < indent; i++) fprintf(output, " ");
            fprintf(output, "%s}%s", COLOR_WHITE, COLOR_RESET);
            break;
    }
}

static void print_editable_node(const TreeNode* node) {
    fprintf(output, "EditableNode {\n");
    if (node->name) {
        fprintf(output, "    \"key\": \"%s\",\n", node->name);
    }
    fprintf(output, "    \"type\": \"%s\",\n",
           node->type == JSON_NULL ? "NULL" :
           node->type == JSON_BOOL ? "BOOL" :
           node->type == JSON_NUMBER ? "NUMBER" :
           node->type == JSON_STRING ? "STRING" :
           node->type == JSON_ARRAY ? "ARRAY" : "OBJECT");
    
    if (node->value) {
        fprintf(output, "    \"value\": \"%s\",\n", node->value);
    }
    
    fprintf(output, "    \"children\": [");
    if (node->children_count > 0) {
        fprintf(output, "\n");
        for (size_t i = 0; i < node->children_count; i++) {
            print_editable_node(node->children[i]);
            if (i < node->children_count - 1) fprintf(output, ",");
            fprintf(output, "\n");
        }
        fprintf(output, "    ");
    }
    fprintf(output, "]\n}");
}

static void build_index(const TreeNode* node, const char* path) {
    char new_path[JSON_PATH_MAX_LENGTH];
    
    if (node->name) {
        snprintf(new_path, sizeof(new_path), "%s.%s", path, node->name);
    } else {
        strncpy(new_path, path, sizeof(new_path));
    }
    
    switch (node->type) {
        case JSON_STRING:
        case JSON_NUMBER:
        case JSON_BOOL:
        case JSON_NULL:
            fprintf(output, "\"%s\" => [%s]\n", node->value, new_path);
            break;
            
        case JSON_ARRAY:
        case JSON_OBJECT:
            for (size_t i = 0; i < node->children_count; i++) {
                build_index(node->children[i], new_path);
            }
            break;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    Options opts = parse_options(argc, argv);
    
    // Redirect output if needed
    output = stdout;
    if (opts.output_file) {
        output = fopen(opts.output_file, "w");
        if (!output) {
            fprintf(stderr, "Error: Cannot open output file '%s'\n", opts.output_file);
            return 1;
        }
    }
    
    // Read input file
    FILE* fp = fopen(opts.input_file, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", opts.input_file);
        if (output != stdout) fclose(output);
        return 1;
    }
    
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char* input = malloc(size + 1);
    if (!input) {
        fprintf(stderr, "Error: Out of memory\n");
        fclose(fp);
        if (output != stdout) fclose(output);
        return 1;
    }
    
    if (fread(input, 1, size, fp) != size) {
        fprintf(stderr, "Error: Failed to read file\n");
        free(input);
        fclose(fp);
        if (output != stdout) fclose(output);
        return 1;
    }
    input[size] = '\0';
    fclose(fp);
    
    // Parse JSON
    JsonParser* parser = json_parser_create(input, size);
    if (!parser) {
        fprintf(stderr, "Error: Failed to create parser\n");
        free(input);
        if (output != stdout) fclose(output);
        return 1;
    }
    
    TreeNode* root = json_parse_tree(parser);
    if (!root && !opts.validate) {
        fprintf(stderr, "Error: Failed to parse JSON\n");
        json_parser_destroy(parser);
        free(input);
        if (output != stdout) fclose(output);
        return 1;
    }
    
    // Process each requested output format
    if (opts.tree && root) {
        fprintf(output, "\nTree Structure:\n");
        json_print_tree(root, output);
    }
    
    if (opts.pretty && root) {
        fprintf(output, "\nFormatted JSON:\n");
        char* formatted = json_format(parser, opts.indent);
        if (formatted) {
            fprintf(output, "%s", formatted);
            json_free(formatted);
        }
    }
    
    if (opts.compact && root) {
        fprintf(output, "\nCompact JSON:\n");
        char* compact = json_compact(parser);
        if (compact) {
            fprintf(output, "%s\n", compact);
            json_free(compact);
        }
    }
    
    if (opts.flatten && root) {
        fprintf(output, "\nFlattened Key-Value Pairs:\n");
        char path[JSON_PATH_MAX_LENGTH] = "$";
        print_path_value(root, path, 1);
    }
    
    if (opts.stream && root) {
        fprintf(output, "\nParsing Events Stream:\n");
        print_stream_events(root);
    }
    
    if (opts.validate) {
        fprintf(output, "\nValidation Result:\n");
        print_validation_result(parser);
    }
    
    if (opts.stats && root) {
        fprintf(output, "\nJSON Statistics:\n");
        JsonStats stats = json_stats(parser);
        print_stats(&stats);
    }
    
    if (opts.highlight && root) {
        fprintf(output, "\nSyntax Highlighted JSON:\n");
        if (!opts.no_color && isatty(fileno(output))) {
            print_highlighted_value(root, 0);
        } else {
            // Fallback to pretty print if no color support
            char* formatted = json_format(parser, opts.indent);
            if (formatted) {
                fprintf(output, "%s", formatted);
                json_free(formatted);
            }
        }
        fprintf(output, "\n");
    }
    
    if (opts.edit && root) {
        fprintf(output, "\nEditable Node Structure:\n");
        print_editable_node(root);
        fprintf(output, "\n");
    }
    
    if (opts.index && root) {
        fprintf(output, "\nSearchable Index:\n");
        build_index(root, "$");
    }
    
    // Cleanup
    if (root) tree_node_destroy(root);
    json_parser_destroy(parser);
    free(input);
    if (output != stdout) fclose(output);
    
    return 0;
} 

