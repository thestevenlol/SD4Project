#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKENS 100
#define MAX_TOKEN_LENGTH 128

typedef enum {
    CMD_UNKNOWN,
    CMD_ADD,
    CMD_SUB,
    CMD_MUL,
    CMD_DIV,
    CMD_CONCAT,
    CMD_REVERSE,
    CMD_SORT,
    CMD_FILTER,
    CMD_MAP,
    CMD_TRANSFORM
} CommandType;

typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_ARRAY
} DataType;

typedef struct {
    DataType type;
    union {
        int int_val;
        float float_val;
        char* string_val;
        struct {
            void* items;
            int length;
            DataType item_type;
        } array_val;
    } data;
} Value;

typedef struct {
    CommandType type;
    int num_args;
    Value* args;
    Value result;
    bool has_error;
    char error_msg[256];
} Command;

// Forward declarations
char** tokenize_input(const char* input, int* token_count);
CommandType parse_command_type(const char* cmd_str);
Value parse_value(const char* token);
Command* parse_command(char** tokens, int token_count);
void execute_command(Command* cmd);
void free_command(Command* cmd);
void free_value(Value* val);
void print_value(Value* val);

int main(int argc, char* argv[]) {
    char input[MAX_INPUT_SIZE];
    
    if (argc > 1) {
        // Read from file
        FILE* file = fopen(argv[1], "r");
        if (!file) {
            perror("Failed to open file");
            return 1;
        }
        
        size_t bytes_read = fread(input, 1, MAX_INPUT_SIZE - 1, file);
        input[bytes_read] = '\0';
        fclose(file);
    } else {
        // Read from stdin
        if (!fgets(input, MAX_INPUT_SIZE, stdin)) {
            fprintf(stderr, "Failed to read input\n");
            return 1;
        }
    }
    
    int token_count = 0;
    char** tokens = tokenize_input(input, &token_count);
    
    if (tokens) {
        Command* cmd = parse_command(tokens, token_count);
        
        if (cmd) {
            execute_command(cmd);
            
            if (cmd->has_error) {
                fprintf(stderr, "Error: %s\n", cmd->error_msg);
            } else {
                printf("Result: ");
                print_value(&cmd->result);
                printf("\n");
            }
            
            free_command(cmd);
        }
        
        // Free tokens
        for (int i = 0; i < token_count; i++) {
            free(tokens[i]);
        }
        free(tokens);
    }
    
    return 0;
}

char** tokenize_input(const char* input, int* token_count) {
    char** tokens = (char**)malloc(MAX_TOKENS * sizeof(char*));
    if (!tokens) return NULL;
    
    *token_count = 0;
    const char* delimiters = " \t\n,;";
    char* input_copy = strdup(input);
    char* token = strtok(input_copy, delimiters);
    
    while (token && *token_count < MAX_TOKENS) {
        tokens[*token_count] = strdup(token);
        (*token_count)++;
        token = strtok(NULL, delimiters);
    }
    
    free(input_copy);
    return tokens;
}

CommandType parse_command_type(const char* cmd_str) {
    if (!cmd_str || strlen(cmd_str) == 0) return CMD_UNKNOWN;
    
    if (strcasecmp(cmd_str, "add") == 0) return CMD_ADD;
    if (strcasecmp(cmd_str, "sub") == 0) return CMD_SUB;
    if (strcasecmp(cmd_str, "mul") == 0) return CMD_MUL;
    if (strcasecmp(cmd_str, "div") == 0) return CMD_DIV;
    if (strcasecmp(cmd_str, "concat") == 0) return CMD_CONCAT;
    if (strcasecmp(cmd_str, "reverse") == 0) return CMD_REVERSE;
    if (strcasecmp(cmd_str, "sort") == 0) return CMD_SORT;
    if (strcasecmp(cmd_str, "filter") == 0) return CMD_FILTER;
    if (strcasecmp(cmd_str, "map") == 0) return CMD_MAP;
    if (strcasecmp(cmd_str, "transform") == 0) return CMD_TRANSFORM;
    
    return CMD_UNKNOWN;
}

Value parse_value(const char* token) {
    Value val;
    
    // Check if it's a string (enclosed in quotes)
    if (token[0] == '"' && token[strlen(token) - 1] == '"') {
        val.type = TYPE_STRING;
        // Remove the quotes and copy the string
        val.data.string_val = (char*)malloc(strlen(token) - 1);
        strncpy(val.data.string_val, token + 1, strlen(token) - 2);
        val.data.string_val[strlen(token) - 2] = '\0';
        return val;
    }
    
    // Check if it's an array (enclosed in brackets)
    if (token[0] == '[' && token[strlen(token) - 1] == ']') {
        // Parsing arrays would be complex, so for simplicity we'll treat it as a string
        val.type = TYPE_STRING;
        val.data.string_val = strdup(token);
        return val;
    }
    
    // Check if it's a float (contains a decimal point)
    if (strchr(token, '.')) {
        val.type = TYPE_FLOAT;
        val.data.float_val = atof(token);
        return val;
    }
    
    // Otherwise, assume it's an integer
    val.type = TYPE_INT;
    val.data.int_val = atoi(token);
    return val;
}

Command* parse_command(char** tokens, int token_count) {
    if (token_count == 0) return NULL;
    
    Command* cmd = (Command*)malloc(sizeof(Command));
    if (!cmd) return NULL;
    
    memset(cmd, 0, sizeof(Command));
    cmd->type = parse_command_type(tokens[0]);
    
    if (cmd->type == CMD_UNKNOWN) {
        cmd->has_error = true;
        snprintf(cmd->error_msg, sizeof(cmd->error_msg), "Unknown command: %s", tokens[0]);
        return cmd;
    }
    
    // Parse arguments
    cmd->num_args = token_count - 1;
    if (cmd->num_args > 0) {
        cmd->args = (Value*)malloc(cmd->num_args * sizeof(Value));
        if (!cmd->args) {
            free(cmd);
            return NULL;
        }
        
        for (int i = 0; i < cmd->num_args; i++) {
            cmd->args[i] = parse_value(tokens[i + 1]);
        }
    }
    
    return cmd;
}

void execute_command(Command* cmd) {
    if (!cmd) return;
    
    // Initialize result as integer zero
    cmd->result.type = TYPE_INT;
    cmd->result.data.int_val = 0;
    
    switch (cmd->type) {
        case CMD_ADD:
            if (cmd->num_args < 2) {
                cmd->has_error = true;
                strcpy(cmd->error_msg, "ADD requires at least two arguments");
                return;
            }
            
            if (cmd->args[0].type == TYPE_INT) {
                int result = cmd->args[0].data.int_val;
                for (int i = 1; i < cmd->num_args; i++) {
                    if (cmd->args[i].type != TYPE_INT) {
                        cmd->has_error = true;
                        strcpy(cmd->error_msg, "Type mismatch in ADD operation");
                        return;
                    }
                    result += cmd->args[i].data.int_val;
                }
                cmd->result.type = TYPE_INT;
                cmd->result.data.int_val = result;
            } else if (cmd->args[0].type == TYPE_FLOAT) {
                float result = cmd->args[0].data.float_val;
                for (int i = 1; i < cmd->num_args; i++) {
                    if (cmd->args[i].type == TYPE_INT) {
                        result += cmd->args[i].data.int_val;
                    } else if (cmd->args[i].type == TYPE_FLOAT) {
                        result += cmd->args[i].data.float_val;
                    } else {
                        cmd->has_error = true;
                        strcpy(cmd->error_msg, "Type mismatch in ADD operation");
                        return;
                    }
                }
                cmd->result.type = TYPE_FLOAT;
                cmd->result.data.float_val = result;
            } else if (cmd->args[0].type == TYPE_STRING) {
                // String concatenation
                size_t total_len = strlen(cmd->args[0].data.string_val) + 1;  // +1 for null terminator
                
                // Calculate total length needed
                for (int i = 1; i < cmd->num_args; i++) {
                    if (cmd->args[i].type != TYPE_STRING) {
                        cmd->has_error = true;
                        strcpy(cmd->error_msg, "Type mismatch in string ADD operation");
                        return;
                    }
                    total_len += strlen(cmd->args[i].data.string_val);
                }
                
                char* result = (char*)malloc(total_len);
                if (!result) {
                    cmd->has_error = true;
                    strcpy(cmd->error_msg, "Memory allocation failed");
                    return;
                }
                
                strcpy(result, cmd->args[0].data.string_val);
                for (int i = 1; i < cmd->num_args; i++) {
                    strcat(result, cmd->args[i].data.string_val);
                }
                
                cmd->result.type = TYPE_STRING;
                cmd->result.data.string_val = result;
            }
            break;
            
        case CMD_SUB:
            // Similar implementation to ADD but for subtraction
            if (cmd->num_args < 2) {
                cmd->has_error = true;
                strcpy(cmd->error_msg, "SUB requires at least two arguments");
                return;
            }
            
            if (cmd->args[0].type == TYPE_INT) {
                int result = cmd->args[0].data.int_val;
                for (int i = 1; i < cmd->num_args; i++) {
                    if (cmd->args[i].type != TYPE_INT) {
                        cmd->has_error = true;
                        strcpy(cmd->error_msg, "Type mismatch in SUB operation");
                        return;
                    }
                    result -= cmd->args[i].data.int_val;
                }
                cmd->result.type = TYPE_INT;
                cmd->result.data.int_val = result;
            } else if (cmd->args[0].type == TYPE_FLOAT) {
                float result = cmd->args[0].data.float_val;
                for (int i = 1; i < cmd->num_args; i++) {
                    if (cmd->args[i].type == TYPE_INT) {
                        result -= cmd->args[i].data.int_val;
                    } else if (cmd->args[i].type == TYPE_FLOAT) {
                        result -= cmd->args[i].data.float_val;
                    } else {
                        cmd->has_error = true;
                        strcpy(cmd->error_msg, "Type mismatch in SUB operation");
                        return;
                    }
                }
                cmd->result.type = TYPE_FLOAT;
                cmd->result.data.float_val = result;
            }
            break;
            
        case CMD_MUL:
            // Implementation for multiplication
            if (cmd->num_args < 2) {
                cmd->has_error = true;
                strcpy(cmd->error_msg, "MUL requires at least two arguments");
                return;
            }
            
            if (cmd->args[0].type == TYPE_INT) {
                int result = cmd->args[0].data.int_val;
                for (int i = 1; i < cmd->num_args; i++) {
                    if (cmd->args[i].type != TYPE_INT) {
                        cmd->has_error = true;
                        strcpy(cmd->error_msg, "Type mismatch in MUL operation");
                        return;
                    }
                    result *= cmd->args[i].data.int_val;
                }
                cmd->result.type = TYPE_INT;
                cmd->result.data.int_val = result;
            } else if (cmd->args[0].type == TYPE_FLOAT) {
                float result = cmd->args[0].data.float_val;
                for (int i = 1; i < cmd->num_args; i++) {
                    if (cmd->args[i].type == TYPE_INT) {
                        result *= cmd->args[i].data.int_val;
                    } else if (cmd->args[i].type == TYPE_FLOAT) {
                        result *= cmd->args[i].data.float_val;
                    } else {
                        cmd->has_error = true;
                        strcpy(cmd->error_msg, "Type mismatch in MUL operation");
                        return;
                    }
                }
                cmd->result.type = TYPE_FLOAT;
                cmd->result.data.float_val = result;
            }
            break;
            
        case CMD_DIV:
            // Implementation for division with division by zero check
            if (cmd->num_args < 2) {
                cmd->has_error = true;
                strcpy(cmd->error_msg, "DIV requires at least two arguments");
                return;
            }
            
            if (cmd->args[0].type == TYPE_INT) {
                int result = cmd->args[0].data.int_val;
                for (int i = 1; i < cmd->num_args; i++) {
                    if (cmd->args[i].type != TYPE_INT) {
                        cmd->has_error = true;
                        strcpy(cmd->error_msg, "Type mismatch in DIV operation");
                        return;
                    }
                    if (cmd->args[i].data.int_val == 0) {
                        cmd->has_error = true;
                        strcpy(cmd->error_msg, "Division by zero");
                        return;
                    }
                    result /= cmd->args[i].data.int_val;
                }
                cmd->result.type = TYPE_INT;
                cmd->result.data.int_val = result;
            } else if (cmd->args[0].type == TYPE_FLOAT) {
                float result = cmd->args[0].data.float_val;
                for (int i = 1; i < cmd->num_args; i++) {
                    float divisor = (cmd->args[i].type == TYPE_INT) ? 
                                    (float)cmd->args[i].data.int_val : 
                                    cmd->args[i].data.float_val;
                    
                    if (fabsf(divisor) < 0.000001f) {
                        cmd->has_error = true;
                        strcpy(cmd->error_msg, "Division by zero or near-zero value");
                        return;
                    }
                    result /= divisor;
                }
                cmd->result.type = TYPE_FLOAT;
                cmd->result.data.float_val = result;
            }
            break;
        
        case CMD_REVERSE:
            if (cmd->num_args != 1) {
                cmd->has_error = true;
                strcpy(cmd->error_msg, "REVERSE requires exactly one argument");
                return;
            }
            
            if (cmd->args[0].type == TYPE_STRING) {
                char* str = cmd->args[0].data.string_val;
                int len = strlen(str);
                char* reversed = (char*)malloc(len + 1);
                
                if (!reversed) {
                    cmd->has_error = true;
                    strcpy(cmd->error_msg, "Memory allocation failed");
                    return;
                }
                
                for (int i = 0; i < len; i++) {
                    reversed[i] = str[len - 1 - i];
                }
                reversed[len] = '\0';
                
                cmd->result.type = TYPE_STRING;
                cmd->result.data.string_val = reversed;
            } else {
                cmd->has_error = true;
                strcpy(cmd->error_msg, "REVERSE operation requires string argument");
            }
            break;
            
        // Other cases would follow with similar complex logic
            
        default:
            cmd->has_error = true;
            strcpy(cmd->error_msg, "Command not implemented yet");
            break;
    }
}

void free_value(Value* val) {
    if (!val) return;
    
    if (val->type == TYPE_STRING && val->data.string_val) {
        free(val->data.string_val);
        val->data.string_val = NULL;
    } else if (val->type == TYPE_ARRAY && val->data.array_val.items) {
        // Free array elements
        if (val->data.array_val.item_type == TYPE_STRING) {
            char** items = (char**)val->data.array_val.items;
            for (int i = 0; i < val->data.array_val.length; i++) {
                if (items[i]) free(items[i]);
            }
        }
        free(val->data.array_val.items);
    }
}

void free_command(Command* cmd) {
    if (!cmd) return;
    
    if (cmd->args) {
        for (int i = 0; i < cmd->num_args; i++) {
            free_value(&cmd->args[i]);
        }
        free(cmd->args);
    }
    
    free_value(&cmd->result);
    free(cmd);
}

void print_value(Value* val) {
    if (!val) return;
    
    switch (val->type) {
        case TYPE_INT:
            printf("%d", val->data.int_val);
            break;
        case TYPE_FLOAT:
            printf("%.6f", val->data.float_val);
            break;
        case TYPE_STRING:
            printf("\"%s\"", val->data.string_val ? val->data.string_val : "");
            break;
        case TYPE_ARRAY:
            printf("[array of %d items]", val->data.array_val.length);
            break;
    }
}