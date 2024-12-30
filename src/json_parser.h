#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// Configuration constants
#define JSON_INITIAL_CAPACITY 16
#define JSON_BUFFER_SIZE 1024
#define JSON_PATH_MAX_LENGTH 256
#define JSON_MAX_DEPTH 1000

// JSON value types
typedef enum {
    JSON_NULL,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} JsonType;

// Token types for syntax highlighting
typedef enum {
    TOKEN_BRACE,
    TOKEN_BRACKET,
    TOKEN_COLON,
    TOKEN_COMMA,
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_BOOL,
    TOKEN_NULL
} TokenType;

// Tree node structure for hierarchical view
typedef struct TreeNode {
    char* name;
    char* value;
    JsonType type;
    struct TreeNode** children;
    size_t children_count;
    size_t children_capacity;
    struct TreeNode* parent;
} TreeNode;

// Token structure for syntax highlighting
typedef struct {
    TokenType type;
    char* value;
    char* style;
} Token;

// Statistics structure
typedef struct {
    size_t total_keys;
    size_t total_values;
    size_t depth;
    struct {
        size_t string_count;
        size_t number_count;
        size_t bool_count;
        size_t null_count;
        size_t array_count;
        size_t object_count;
    } types;
} JsonStats;

// Validation error structure
typedef struct {
    char* message;
    struct {
        size_t line;
        size_t column;
    } position;
} ValidationError;

// Main parser context
typedef struct {
    char* input;
    size_t input_len;
    size_t pos;
    size_t line;
    size_t column;
    ValidationError* errors;
    size_t error_count;
    size_t error_capacity;
} JsonParser;

// Core parsing functions
JsonParser* json_parser_create(const char* input, size_t len);
void json_parser_destroy(JsonParser* parser);
TreeNode* json_parse_tree(JsonParser* parser);
char* json_format(JsonParser* parser, size_t indent);
char* json_compact(JsonParser* parser);
Token* json_tokenize(JsonParser* parser, size_t* token_count);
JsonStats json_stats(JsonParser* parser);
bool json_validate(JsonParser* parser);

// Tree node operations
TreeNode* tree_node_create(const char* name, const char* value, JsonType type);
void tree_node_add_child(TreeNode* parent, TreeNode* child);
void tree_node_destroy(TreeNode* node);

// Utility functions
char* json_escape_string(const char* str);
char* json_unescape_string(const char* str);
void json_free(void* ptr);

// Add the function declaration
void json_print_tree(const TreeNode* root, FILE* output);

#endif // JSON_PARSER_H 

