#include "json_parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    char* data;
    size_t size;
    size_t capacity;
} Buffer;

static Buffer* buffer_create(void) {
    Buffer* buffer = malloc(sizeof(Buffer));
    if (!buffer) return NULL;
    
    buffer->data = malloc(JSON_BUFFER_SIZE);
    if (!buffer->data) {
        free(buffer);
        return NULL;
    }
    
    buffer->size = 0;
    buffer->capacity = JSON_BUFFER_SIZE;
    return buffer;
}

static void buffer_destroy(Buffer* buffer) {
    if (!buffer) return;
    free(buffer->data);
    free(buffer);
}

static bool buffer_append(Buffer* buffer, const char* str) {
    size_t len = strlen(str);
    size_t new_size = buffer->size + len;
    
    if (new_size > buffer->capacity) {
        size_t new_capacity = buffer->capacity * 2;
        while (new_size > new_capacity) {
            new_capacity *= 2;
        }
        
        char* new_data = realloc(buffer->data, new_capacity);
        if (!new_data) return false;
        
        buffer->data = new_data;
        buffer->capacity = new_capacity;
    }
    
    memcpy(buffer->data + buffer->size, str, len);
    buffer->size = new_size;
    return true;
}

static void add_token(Token** tokens, size_t* count, size_t* capacity,
                     TokenType type, const char* value, const char* style) {
    if (*count >= *capacity) {
        size_t new_capacity = *capacity * 2;
        Token* new_tokens = realloc(*tokens, new_capacity * sizeof(Token));
        if (!new_tokens) return;
        *tokens = new_tokens;
        *capacity = new_capacity;
    }
    
    Token* token = &(*tokens)[*count];
    token->type = type;
    token->value = json_escape_string(value);  // Use proper string escaping
    token->style = strdup(style);
    (*count)++;
}

static void tokenize_value(const TreeNode* node, Token** tokens, size_t* count, size_t* capacity) {
    if (!node || !tokens || !count || !capacity) return;
    
    char temp[32];
    
    switch (node->type) {
        case JSON_OBJECT:
            add_token(tokens, count, capacity, TOKEN_BRACE, "{", "brace");
            for (size_t i = 0; i < node->children_count; i++) {
                const TreeNode* child = node->children[i];
                if (i > 0) {
                    add_token(tokens, count, capacity, TOKEN_COMMA, ",", "operator");
                }
                add_token(tokens, count, capacity, TOKEN_STRING, child->name, "key");
                add_token(tokens, count, capacity, TOKEN_COLON, ":", "operator");
                tokenize_value(child, tokens, count, capacity);
            }
            add_token(tokens, count, capacity, TOKEN_BRACE, "}", "brace");
            break;
            
        case JSON_ARRAY:
            add_token(tokens, count, capacity, TOKEN_BRACKET, "[", "brace");
            for (size_t i = 0; i < node->children_count; i++) {
                if (i > 0) {
                    add_token(tokens, count, capacity, TOKEN_COMMA, ",", "operator");
                }
                tokenize_value(node->children[i], tokens, count, capacity);
            }
            add_token(tokens, count, capacity, TOKEN_BRACKET, "]", "brace");
            break;
            
        case JSON_STRING:
            snprintf(temp, sizeof(temp), "\"%s\"", node->value);
            add_token(tokens, count, capacity, TOKEN_STRING, temp, "string");
            break;
            
        case JSON_NUMBER:
            add_token(tokens, count, capacity, TOKEN_NUMBER, node->value, "number");
            break;
            
        case JSON_BOOL:
            add_token(tokens, count, capacity, TOKEN_BOOL, node->value, "boolean");
            break;
            
        case JSON_NULL:
            add_token(tokens, count, capacity, TOKEN_NULL, "null", "null");
            break;
    }
}

Token* json_tokenize(JsonParser* parser, size_t* token_count) {
    if (!parser || !token_count) return NULL;
    *token_count = 0;
    
    size_t capacity = JSON_INITIAL_CAPACITY;
    Token* tokens = malloc(capacity * sizeof(Token));
    if (!tokens) return NULL;
    
    TreeNode* root = json_parse_tree(parser);
    if (!root) {
        free(tokens);
        return NULL;
    }
    
    tokenize_value(root, &tokens, token_count, &capacity);
    tree_node_destroy(root);
    
    return tokens;
}

static void format_value(const TreeNode* node, Buffer* buffer, size_t indent, size_t level) {
    if (!node || !buffer) return;
    
    // Add indentation
    for (size_t i = 0; i < level * indent; i++) {
        buffer_append(buffer, " ");
    }
    
    switch (node->type) {
        case JSON_NULL:
            buffer_append(buffer, "null");
            break;
            
        case JSON_BOOL:
            buffer_append(buffer, node->value);
            break;
            
        case JSON_NUMBER:
            buffer_append(buffer, node->value);
            break;
            
        case JSON_STRING:
            buffer_append(buffer, "\"");
            buffer_append(buffer, node->value);
            buffer_append(buffer, "\"");
            break;
            
        case JSON_ARRAY:
            buffer_append(buffer, "[\n");
            for (size_t i = 0; i < node->children_count; i++) {
                format_value(node->children[i], buffer, indent, level + 1);
                if (i < node->children_count - 1) {
                    buffer_append(buffer, ",");
                }
                buffer_append(buffer, "\n");
            }
            for (size_t i = 0; i < level * indent; i++) {
                buffer_append(buffer, " ");
            }
            buffer_append(buffer, "]");
            break;
            
        case JSON_OBJECT:
            buffer_append(buffer, "{\n");
            for (size_t i = 0; i < node->children_count; i++) {
                const TreeNode* child = node->children[i];
                for (size_t j = 0; j < (level + 1) * indent; j++) {
                    buffer_append(buffer, " ");
                }
                buffer_append(buffer, "\"");
                buffer_append(buffer, child->name);
                buffer_append(buffer, "\": ");
                format_value(child, buffer, indent, level + 1);
                if (i < node->children_count - 1) {
                    buffer_append(buffer, ",");
                }
                buffer_append(buffer, "\n");
            }
            for (size_t i = 0; i < level * indent; i++) {
                buffer_append(buffer, " ");
            }
            buffer_append(buffer, "}");
            break;
    }
}

char* json_format(JsonParser* parser, size_t indent) {
    if (!parser) return NULL;
    
    TreeNode* root = json_parse_tree(parser);
    if (!root) return NULL;
    
    Buffer* buffer = buffer_create();
    if (!buffer) {
        tree_node_destroy(root);
        return NULL;
    }
    
    format_value(root, buffer, indent, 0);
    buffer_append(buffer, "\n");
    
    char* result = strdup(buffer->data);
    
    buffer_destroy(buffer);
    tree_node_destroy(root);
    
    return result;
}

char* json_compact(JsonParser* parser) {
    return json_format(parser, 0);
}

static void print_tree_node(const TreeNode* node, const char* prefix, bool is_root, bool is_last, FILE* output) {
    char new_prefix[1024];
    char indent[1024] = "";

    if (!is_root) {
        snprintf(indent, sizeof(indent), "%s%s", prefix, is_last ? "└── " : "├── ");
    }

    switch (node->type) {
        case JSON_NULL:
            fprintf(output, "%snull\n", indent);
            break;
        case JSON_BOOL:
            fprintf(output, "%s%s\n", indent, node->value);
            break;
        case JSON_NUMBER:
            fprintf(output, "%s%s\n", indent, node->value);
            break;
        case JSON_STRING:
            fprintf(output, "%s\"%s\"\n", indent, node->value);
            break;
        case JSON_ARRAY:
            if (!is_root) fprintf(output, "%sArray\n", indent);
            for (size_t i = 0; i < node->children_count; i++) {
                snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix, is_last ? "    " : "│   ");
                print_tree_node(node->children[i], new_prefix, false, i == node->children_count - 1, output);
            }
            break;
        case JSON_OBJECT:
            if (!is_root) fprintf(output, "%sObject\n", indent);
            for (size_t i = 0; i < node->children_count; i++) {
                const TreeNode* child = node->children[i];
                snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix, is_last ? "    " : "│   ");
                
                if (child->name) {
                    fprintf(output, "%s%s%s\n", new_prefix, 
                           i == node->children_count - 1 ? "└── " : "├── ", 
                           child->name);
                }
                
                char next_prefix[1024];
                snprintf(next_prefix, sizeof(next_prefix), "%s%s", new_prefix, 
                        i == node->children_count - 1 ? "    " : "│   ");
                print_tree_node(child, next_prefix, false, true, output);
            }
            break;
    }
}

void json_print_tree(const TreeNode* root, FILE* output) {
    if (!root) return;
    print_tree_node(root, "", true, true, output);
} 

