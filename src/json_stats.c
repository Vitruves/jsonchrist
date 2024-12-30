#include "json_parser.h"
#include <stdlib.h>
#include <string.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static void collect_stats(TreeNode* node, JsonStats* stats, size_t depth) {
    if (!node || !stats) return;
    
    // Update depth
    stats->depth = MAX(stats->depth, depth);
    
    // Count values by type
    switch (node->type) {
        case JSON_STRING:
            stats->types.string_count++;
            break;
        case JSON_NUMBER:
            stats->types.number_count++;
            break;
        case JSON_BOOL:
            stats->types.bool_count++;
            break;
        case JSON_NULL:
            stats->types.null_count++;
            break;
        case JSON_ARRAY:
            stats->types.array_count++;
            break;
        case JSON_OBJECT:
            stats->types.object_count++;
            break;
    }
    
    // Count total values
    stats->total_values++;
    
    // Count keys (only for named nodes)
    if (node->name) {
        stats->total_keys++;
    }
    
    // Recursively process children
    for (size_t i = 0; i < node->children_count; i++) {
        collect_stats(node->children[i], stats, depth + 1);
    }
}

JsonStats json_stats(JsonParser* parser) {
    JsonStats stats = {0}; // Initialize all fields to 0
    
    if (!parser) return stats;
    
    TreeNode* root = json_parse_tree(parser);
    if (!root) return stats;
    
    collect_stats(root, &stats, 0);
    tree_node_destroy(root);
    
    return stats;
}

bool json_validate(JsonParser* parser) {
    if (!parser) return false;
    
    // Reset parser state
    parser->pos = 0;
    parser->line = 1;
    parser->column = 0;
    parser->error_count = 0;
    
    // Try to parse the tree
    TreeNode* root = json_parse_tree(parser);
    bool is_valid = (root != NULL && parser->error_count == 0);
    
    if (root) {
        tree_node_destroy(root);
    }
    
    return is_valid;
}

char* json_escape_string(const char* str) {
    if (!str) return NULL;
    
    size_t len = strlen(str);
    size_t escaped_len = len;
    
    // First pass: count needed space
    for (size_t i = 0; i < len; i++) {
        char c = str[i];
        if (c == '"' || c == '\\' || c == '\b' || c == '\f' ||
            c == '\n' || c == '\r' || c == '\t') {
            escaped_len++;
        }
    }
    
    char* result = malloc(escaped_len + 1);
    if (!result) return NULL;
    
    // Second pass: copy with escaping
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        char c = str[i];
        switch (c) {
            case '"':
                result[j++] = '\\';
                result[j++] = '"';
                break;
            case '\\':
                result[j++] = '\\';
                result[j++] = '\\';
                break;
            case '\b':
                result[j++] = '\\';
                result[j++] = 'b';
                break;
            case '\f':
                result[j++] = '\\';
                result[j++] = 'f';
                break;
            case '\n':
                result[j++] = '\\';
                result[j++] = 'n';
                break;
            case '\r':
                result[j++] = '\\';
                result[j++] = 'r';
                break;
            case '\t':
                result[j++] = '\\';
                result[j++] = 't';
                break;
            default:
                result[j++] = c;
                break;
        }
    }
    result[j] = '\0';
    
    return result;
}

char* json_unescape_string(const char* str) {
    if (!str) return NULL;
    
    size_t len = strlen(str);
    char* result = malloc(len + 1);
    if (!result) return NULL;
    
    size_t j = 0;
    bool escaped = false;
    
    for (size_t i = 0; i < len; i++) {
        char c = str[i];
        
        if (escaped) {
            switch (c) {
                case '"':
                case '\\':
                case '/':
                    result[j++] = c;
                    break;
                case 'b':
                    result[j++] = '\b';
                    break;
                case 'f':
                    result[j++] = '\f';
                    break;
                case 'n':
                    result[j++] = '\n';
                    break;
                case 'r':
                    result[j++] = '\r';
                    break;
                case 't':
                    result[j++] = '\t';
                    break;
                default:
                    // Invalid escape sequence, copy as is
                    result[j++] = '\\';
                    result[j++] = c;
                    break;
            }
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else {
            result[j++] = c;
        }
    }
    
    // Handle trailing backslash
    if (escaped) {
        result[j++] = '\\';
    }
    
    result[j] = '\0';
    return result;
} 

