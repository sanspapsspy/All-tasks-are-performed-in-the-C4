#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define MAX_EXPR_LENGTH 256

// Структура для стека
typedef struct StackNode {
    char data[MAX_EXPR_LENGTH];
    struct StackNode* next;
} StackNode;

typedef struct {
    StackNode* top;
} Stack;

// Базовые операции со стеком
Stack* create_stack() {
    Stack* stack = malloc(sizeof(Stack));
    stack->top = NULL;
    return stack;
}

int is_empty(Stack* stack) {
    return stack->top == NULL;
}

void push(Stack* stack, const char* data) {
    StackNode* new_node = malloc(sizeof(StackNode));
    strcpy(new_node->data, data);
    new_node->next = stack->top;
    stack->top = new_node;
}

char* pop(Stack* stack) {
    if (is_empty(stack)) return NULL;

    StackNode* temp = stack->top;
    char* data = malloc(strlen(temp->data) + 1);
    strcpy(data, temp->data);

    stack->top = temp->next;
    free(temp);

    return data;
}

char* peek(Stack* stack) {
    if (is_empty(stack)) return NULL;
    return stack->top->data;
}

void free_stack(Stack* stack) {
    while (!is_empty(stack)) {
        char* data = pop(stack);
        free(data);
    }
    free(stack);
}

// Функции для работы с выражениями
int get_priority(char op) {
    switch (op) {
    case '+': case '-': return 1;
    case '*': case '/': case '%': return 2;
    case '^': return 3;
    default: return 0;
    }
}

int is_operator(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '^';
}

int check_balance(const char* expr) {
    int balance = 0;

    for (int i = 0; expr[i] != '\0'; i++) {
        if (expr[i] == '(') {
            balance++;
        }
        else if (expr[i] == ')') {
            balance--;
            if (balance < 0) return 0;
        }
    }

    return balance == 0;
}

// Преобразование в обратную польскую запись
int infix_to_postfix(const char* infix, char* postfix) {
    if (!check_balance(infix)) {
        return 0;
    }

    Stack* stack = create_stack();
    char output[MAX_EXPR_LENGTH * 2] = "";
    int output_len = 0;

    for (int i = 0; infix[i] != '\0'; i++) {
        if (isspace(infix[i])) continue;

        // Число
        if (isdigit(infix[i])) {
            while (isdigit(infix[i])) {
                output[output_len++] = infix[i++];
            }
            output[output_len++] = ' ';
            i--;
        }
        // Открывающая скобка
        else if (infix[i] == '(') {
            push(stack, "(");
        }
        // Закрывающая скобка
        else if (infix[i] == ')') {
            while (!is_empty(stack) && strcmp(peek(stack), "(") != 0) {
                char* op = pop(stack);
                strcat(output, op);
                strcat(output, " ");
                free(op);
            }
            if (is_empty(stack)) {
                free_stack(stack);
                return 0;
            }
            char* temp = pop(stack);
            free(temp);
        }
        // Оператор
        else if (is_operator(infix[i])) {
            char op_str[2] = { infix[i], '\0' };

            // Обработка унарного минуса
            if (infix[i] == '-' && (i == 0 || infix[i - 1] == '(' || is_operator(infix[i - 1]))) {
                push(stack, "~");
                continue;
            }

            while (!is_empty(stack) &&
                get_priority(peek(stack)[0]) >= get_priority(infix[i]) &&
                infix[i] != '^') {
                char* op = pop(stack);
                strcat(output, op);
                strcat(output, " ");
                free(op);
            }
            push(stack, op_str);
        }
    }

    // Выталкиваем оставшиеся операторы
    while (!is_empty(stack)) {
        char* op = pop(stack);
        if (strcmp(op, "(") == 0) {
            free(op);
            free_stack(stack);
            return 0;
        }
        strcat(output, op);
        strcat(output, " ");
        free(op);
    }

    strcpy(postfix, output);
    free_stack(stack);
    return 1;
}

// Вычисление выражения в RPN
int evaluate_postfix(const char* postfix, long long* result) {
    Stack* stack = create_stack();
    char token[50];
    const char* ptr = postfix;

    while (*ptr != '\0') {
        while (*ptr && isspace(*ptr)) ptr++;
        if (!*ptr) break;

        int i = 0;
        while (*ptr && !isspace(*ptr)) {
            token[i++] = *ptr++;
        }
        token[i] = '\0';

        if (strlen(token) == 0) continue;

        // Число
        if (isdigit(token[0]) || (token[0] == '-' && isdigit(token[1]))) {
            push(stack, token);
        }
        // Унарный минус
        else if (strcmp(token, "~") == 0) {
            if (is_empty(stack)) {
                free_stack(stack);
                return 0;
            }
            char* num_str = pop(stack);
            long long num = atoll(num_str);
            char result_str[50];
            sprintf(result_str, "%lld", -num);
            push(stack, result_str);
            free(num_str);
        }
        // Бинарный оператор
        else if (is_operator(token[0])) {
            if (is_empty(stack)) {
                free_stack(stack);
                return 0;
            }
            char* right_str = pop(stack);
            if (is_empty(stack)) {
                free(right_str);
                free_stack(stack);
                return 0;
            }
            char* left_str = pop(stack);

            long long left = atoll(left_str);
            long long right = atoll(right_str);
            long long res;

            switch (token[0]) {
            case '+': res = left + right; break;
            case '-': res = left - right; break;
            case '*': res = left * right; break;
            case '/':
                if (right == 0) {
                    free(left_str);
                    free(right_str);
                    free_stack(stack);
                    return 0;
                }
                res = left / right;
                break;
            case '%':
                if (right == 0) {
                    free(left_str);
                    free(right_str);
                    free_stack(stack);
                    return 0;
                }
                res = left % right;
                break;
            case '^':
                if (right < 0) {
                    free(left_str);
                    free(right_str);
                    free_stack(stack);
                    return 0;
                }
                res = 1;
                for (int i = 0; i < right; i++) res *= left;
                break;
            default:
                free(left_str);
                free(right_str);
                free_stack(stack);
                return 0;
            }

            char result_str[50];
            sprintf(result_str, "%lld", res);
            push(stack, result_str);

            free(left_str);
            free(right_str);
        }
    }

    if (is_empty(stack)) {
        free_stack(stack);
        return 0;
    }

    char* result_str = pop(stack);
    *result = atoll(result_str);
    free(result_str);

    if (!is_empty(stack)) {
        free_stack(stack);
        return 0;
    }

    free_stack(stack);
    return 1;
}

// Создание демонстрационного файла
void create_demo_file() {
    FILE* file = fopen("demo_expressions.txt", "w");
    if (file) {
        fprintf(file, "2 + 3 * 4\n");
        fprintf(file, "(2 + 3) * 4\n");
        fprintf(file, "10 / (5 - 5)\n");
        fprintf(file, "2 ^ 3 + 1\n");
        fprintf(file, "((2 + 3) * (4 - 1)\n");
        fprintf(file, "15 %% 4\n");
        fprintf(file, "2 ^ 3 ^ 2\n");
        fprintf(file, "10 / 0\n");
        fprintf(file, "-5 + 8\n");
        fprintf(file, "2 * (3 + 4)) - 1\n");
        fclose(file);
        printf("Demo file 'demo_expressions.txt' created!\n");
    }
}

// Простая демонстрация
void demo_mode() {
    printf("=== DEMO MODE ===\n");
    printf("Creating demo file and processing...\n\n");

    create_demo_file();

    FILE* file = fopen("demo_expressions.txt", "r");
    if (!file) {
        printf("Error: Cannot open demo file\n");
        return;
    }

    char line[MAX_EXPR_LENGTH];
    int line_num = 0;

    printf("Processing expressions:\n");
    printf("=======================\n\n");

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';
        line_num++;

        printf("Expression %d: %s\n", line_num, line);

        // Проверка баланса скобок
        if (!check_balance(line)) {
            printf("  ERROR: Unbalanced parentheses\n\n");
            continue;
        }

        // Преобразование в RPN
        char postfix[MAX_EXPR_LENGTH * 2];
        if (!infix_to_postfix(line, postfix)) {
            printf("  ERROR: Cannot convert to postfix\n\n");
            continue;
        }

        printf("  RPN: %s\n", postfix);

        // Вычисление
        long long result;
        if (evaluate_postfix(postfix, &result)) {
            printf("  Result: %lld\n\n", result);
        }
        else {
            printf("  ERROR: Cannot evaluate expression\n\n");
        }
    }

    fclose(file);
    printf("Demo completed! Processed %d expressions.\n", line_num);
}

// Обработка файла
void process_file_mode(const char* filename) {
    printf("=== PROCESSING FILE: %s ===\n\n", filename);

    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Cannot open file %s\n", filename);
        return;
    }

    char line[MAX_EXPR_LENGTH];
    int line_num = 0;
    int success_count = 0;
    int error_count = 0;

    // Файл для ошибок
    FILE* error_file = fopen("errors.txt", "w");
    fprintf(error_file, "Error report for: %s\n", filename);
    fprintf(error_file, "=============================\n\n");

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';
        line_num++;

        printf("Line %d: %s\n", line_num, line);

        // Проверка баланса скобок
        if (!check_balance(line)) {
            printf("  ERROR: Unbalanced parentheses\n\n");
            fprintf(error_file, "Line %d: %s\n", line_num, line);
            fprintf(error_file, "Error: Unbalanced parentheses\n\n");
            error_count++;
            continue;
        }

        // Преобразование в RPN
        char postfix[MAX_EXPR_LENGTH * 2];
        if (!infix_to_postfix(line, postfix)) {
            printf("  ERROR: Cannot convert to postfix\n\n");
            fprintf(error_file, "Line %d: %s\n", line_num, line);
            fprintf(error_file, "Error: Cannot convert to postfix\n\n");
            error_count++;
            continue;
        }

        printf("  RPN: %s\n", postfix);

        // Вычисление
        long long result;
        if (evaluate_postfix(postfix, &result)) {
            printf("  Result: %lld\n\n", result);
            success_count++;
        }
        else {
            printf("  ERROR: Cannot evaluate expression\n\n");
            fprintf(error_file, "Line %d: %s\n", line_num, line);
            fprintf(error_file, "Error: Cannot evaluate expression\n\n");
            error_count++;
        }
    }

    fclose(file);
    fclose(error_file);

    printf("=== SUMMARY ===\n");
    printf("Total expressions: %d\n", line_num);
    printf("Successfully evaluated: %d\n", success_count);
    printf("Errors: %d\n", error_count);

    if (error_count > 0) {
        printf("Error report saved to: errors.txt\n");
    }
}

int main(int argc, char* argv[]) {
    printf("=========================================\n");
    printf("   ARITHMETIC EXPRESSION CALCULATOR\n");
    printf("=========================================\n\n");

    if (argc == 1) {
        // Демо режим
        demo_mode();
    }
    else if (argc == 2) {
        // Обработка одного файла
        process_file_mode(argv[1]);
    }
    else {
        printf("Usage:\n");
        printf("  %s                    - run demo mode\n", argv[0]);
        printf("  %s <filename>         - process specific file\n", argv[0]);
        printf("\nExample:\n");
        printf("  %s\n", argv[0]);
        printf("  %s my_expressions.txt\n", argv[0]);
    }

    printf("\nPress Enter to exit...");
    getchar();

    return 0;
}


////
//2 + 3 * 4
//(2 + 3) * 4
//10 / (5 - 5)
//2 ^ 3 + 1
//((2 + 3) * (4 - 1)
//    15 % 4
//    2 ^ 3 ^ 2
//    10 / 0
//    - 5 + 8
//    2 * (3 + 4)) - 1
////
