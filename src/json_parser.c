#include "json_parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define INITIAL_CAPACITY 16
#define MAX(a, b) ((a) > (b) ? (a) : (b))

static void skip_whitespace(JsonParser* parser) {
    while (parser->pos < parser->input_len) {
        char c = parser->input[parser->pos];
        if (c == ' ' || c == '\t' || c == '\r') {
            parser->pos++;
            parser->column++;
        } else if (c == '\n') {
            parser->pos++;
            parser->line++;
            parser->column = 0;
        } else {
            break;
        }
    }
}

static void add_error(JsonParser* parser, const char* message) {
    if (parser->error_count >= parser->error_capacity) {
        size_t new_capacity = parser->error_capacity * 2;
        ValidationError* new_errors = realloc(parser->errors, new_capacity * sizeof(ValidationError));
        if (!new_errors) return;
        parser->errors = new_errors;
        parser->error_capacity = new_capacity;
    }
    
    ValidationError* error = &parser->errors[parser->error_count++];
    error->message = strdup(message);
    error->position.line = parser->line;
    error->position.column = parser->column;
}

static char* parse_string(JsonParser* parser) {
    if (parser->pos >= parser->input_len || parser->input[parser->pos] != '"') {
        add_error(parser, "Expected string");
        return NULL;
    }
    
    parser->pos++; // Skip opening quote
    parser->column++;
    
    size_t start = parser->pos;
    size_t len = 0;
    bool escaped = false;
    
    while (parser->pos < parser->input_len) {
        char c = parser->input[parser->pos];
        
        if (escaped) {
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else if (c == '"') {
            break;
        }
        
        parser->pos++;
        parser->column++;
        len++;
    }
    
    if (parser->pos >= parser->input_len) {
        add_error(parser, "Unterminated string");
        return NULL;
    }
    
    parser->pos++; // Skip closing quote
    parser->column++;
    
    char* str = malloc(len + 1);
    if (!str) return NULL;
    
    memcpy(str, &parser->input[start], len);
    str[len] = '\0';
    return str;
}

static TreeNode* parse_value(JsonParser* parser);

static TreeNode* parse_array(JsonParser* parser) {
    TreeNode* node = tree_node_create(NULL, NULL, JSON_ARRAY);
    if (!node) return NULL;
    
    parser->pos++; // Skip [
    parser->column++;
    skip_whitespace(parser);
    
    while (parser->pos < parser->input_len && parser->input[parser->pos] != ']') {
        TreeNode* value = parse_value(parser);
        if (!value) {
            tree_node_destroy(node);
            return NULL;
        }
        
        char index_str[32];
        snprintf(index_str, sizeof(index_str), "%zu", node->children_count);
        value->name = strdup(index_str);
        tree_node_add_child(node, value);
        
        skip_whitespace(parser);
        if (parser->pos < parser->input_len && parser->input[parser->pos] == ',') {
            parser->pos++;
            parser->column++;
            skip_whitespace(parser);
        }
    }
    
    if (parser->pos >= parser->input_len || parser->input[parser->pos] != ']') {
        add_error(parser, "Unterminated array");
        tree_node_destroy(node);
        return NULL;
    }
    
    parser->pos++; // Skip ]
    parser->column++;
    return node;
}

static TreeNode* parse_object(JsonParser* parser) {
    TreeNode* node = tree_node_create(NULL, NULL, JSON_OBJECT);
    if (!node) return NULL;
    
    parser->pos++; // Skip {
    parser->column++;
    skip_whitespace(parser);
    
    while (parser->pos < parser->input_len && parser->input[parser->pos] != '}') {
        char* key = parse_string(parser);
        if (!key) {
            tree_node_destroy(node);
            return NULL;
        }
        
        skip_whitespace(parser);
        if (parser->pos >= parser->input_len || parser->input[parser->pos] != ':') {
            free(key);
            add_error(parser, "Expected ':'");
            tree_node_destroy(node);
            return NULL;
        }
        
        parser->pos++; // Skip :
        parser->column++;
        skip_whitespace(parser);
        
        TreeNode* value = parse_value(parser);
        if (!value) {
            free(key);
            tree_node_destroy(node);
            return NULL;
        }
        
        value->name = key;
        tree_node_add_child(node, value);
        
        skip_whitespace(parser);
        if (parser->pos < parser->input_len && parser->input[parser->pos] == ',') {
            parser->pos++;
            parser->column++;
            skip_whitespace(parser);
        }
    }
    
    if (parser->pos >= parser->input_len || parser->input[parser->pos] != '}') {
        add_error(parser, "Unterminated object");
        tree_node_destroy(node);
        return NULL;
    }
    
    parser->pos++; // Skip }
    parser->column++;
    return node;
}

static TreeNode* parse_value(JsonParser* parser) {
    skip_whitespace(parser);
    
    if (parser->pos >= parser->input_len) {
        add_error(parser, "Unexpected end of input");
        return NULL;
    }
    
    char c = parser->input[parser->pos];
    switch (c) {
        case '"': {
            char* str = parse_string(parser);
            if (!str) return NULL;
            TreeNode* node = tree_node_create(NULL, str, JSON_STRING);
            free(str);
            return node;
        }
        case '{':
            return parse_object(parser);
        case '[':
            return parse_array(parser);
        case 't':
            if (parser->pos + 3 < parser->input_len &&
                strncmp(&parser->input[parser->pos], "true", 4) == 0) {
                parser->pos += 4;
                parser->column += 4;
                return tree_node_create(NULL, "true", JSON_BOOL);
            }
            add_error(parser, "Invalid true value");
            return NULL;
        case 'f':
            if (parser->pos + 4 < parser->input_len &&
                strncmp(&parser->input[parser->pos], "false", 5) == 0) {
                parser->pos += 5;
                parser->column += 5;
                return tree_node_create(NULL, "false", JSON_BOOL);
            }
            add_error(parser, "Invalid false value");
            return NULL;
        case 'n':
            if (parser->pos + 3 < parser->input_len &&
                strncmp(&parser->input[parser->pos], "null", 4) == 0) {
                parser->pos += 4;
                parser->column += 4;
                return tree_node_create(NULL, "null", JSON_NULL);
            }
            add_error(parser, "Invalid null value");
            return NULL;
        default:
            if (c == '-' || isdigit(c)) {
                // Simple number parsing for now
                size_t start = parser->pos;
                size_t len = 0;
                bool has_decimal = false;
                
                if (c == '-') {
                    parser->pos++;
                    parser->column++;
                    len++;
                }
                
                while (parser->pos < parser->input_len) {
                    c = parser->input[parser->pos];
                    if (c == '.' && !has_decimal) {
                        has_decimal = true;
                    } else if (!isdigit(c)) {
                        break;
                    }
                    parser->pos++;
                    parser->column++;
                    len++;
                }
                
                char* num_str = malloc(len + 1);
                if (!num_str) return NULL;
                memcpy(num_str, &parser->input[start], len);
                num_str[len] = '\0';
                
                TreeNode* node = tree_node_create(NULL, num_str, JSON_NUMBER);
                free(num_str);
                return node;
            }
            
            add_error(parser, "Invalid value");
            return NULL;
    }
}

JsonParser* json_parser_create(const char* input, size_t len) {
    JsonParser* parser = malloc(sizeof(JsonParser));
    if (!parser) return NULL;
    
    parser->input = malloc(len + 1);
    if (!parser->input) {
        free(parser);
        return NULL;
    }
    
    memcpy(parser->input, input, len);
    parser->input[len] = '\0';
    parser->input_len = len;
    parser->pos = 0;
    parser->line = 1;
    parser->column = 0;
    
    parser->error_capacity = INITIAL_CAPACITY;
    parser->errors = malloc(parser->error_capacity * sizeof(ValidationError));
    if (!parser->errors) {
        free(parser->input);
        free(parser);
        return NULL;
    }
    parser->error_count = 0;
    
    return parser;
}

void json_parser_destroy(JsonParser* parser) {
    if (!parser) return;
    
    for (size_t i = 0; i < parser->error_count; i++) {
        free(parser->errors[i].message);
    }
    
    free(parser->errors);
    free(parser->input);
    free(parser);
}

TreeNode* json_parse_tree(JsonParser* parser) {
    if (!parser) return NULL;
    parser->pos = 0;
    parser->line = 1;
    parser->column = 0;
    parser->error_count = 0;
    return parse_value(parser);
}

TreeNode* tree_node_create(const char* name, const char* value, JsonType type) {
    TreeNode* node = malloc(sizeof(TreeNode));
    if (!node) return NULL;
    
    node->name = name ? strdup(name) : NULL;
    node->value = value ? strdup(value) : NULL;
    node->type = type;
    node->children = NULL;
    node->children_count = 0;
    node->children_capacity = 0;
    node->parent = NULL;
    
    return node;
}

void tree_node_add_child(TreeNode* parent, TreeNode* child) {
    if (!parent || !child) return;
    
    if (parent->children_count >= parent->children_capacity) {
        size_t new_capacity = parent->children_capacity == 0 ? INITIAL_CAPACITY : parent->children_capacity * 2;
        TreeNode** new_children = realloc(parent->children, new_capacity * sizeof(TreeNode*));
        if (!new_children) return;
        
        parent->children = new_children;
        parent->children_capacity = new_capacity;
    }
    
    parent->children[parent->children_count++] = child;
    child->parent = parent;
}

void tree_node_destroy(TreeNode* node) {
    if (!node) return;
    
    for (size_t i = 0; i < node->children_count; i++) {
        tree_node_destroy(node->children[i]);
    }
    
    free(node->name);
    free(node->value);
    free(node->children);
    free(node);
}

void json_free(void* ptr) {
    free(ptr);
} 

