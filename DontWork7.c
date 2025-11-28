#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_EXPR_LENGTH 256
#define MAX_VARS 10

// Структура для таблицы истинности
typedef struct {
    char expression[MAX_EXPR_LENGTH];
    char variables[MAX_VARS];
    int var_count;
    int** truth_table;
} TruthTable;

// Функции для работы с переменными
void find_variables(const char* expr, char* vars, int* var_count) {
    *var_count = 0;
    for (int i = 0; expr[i] != '\0'; i++) {
        if (isalpha(expr[i]) && expr[i] != 'v') { // 'v' может быть частью ->, поэтому исключаем
            int found = 0;
            for (int j = 0; j < *var_count; j++) {
                if (vars[j] == expr[i]) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                vars[(*var_count)++] = expr[i];
            }
        }
    }
}

// Генерация всех комбинаций значений переменных
void generate_truth_combinations(int** combinations, int var_count) {
    int rows = 1 << var_count;

    for (int i = 0; i < rows; i++) {
        combinations[i] = malloc(var_count * sizeof(int));
        for (int j = 0; j < var_count; j++) {
            combinations[i][j] = (i >> (var_count - 1 - j)) & 1;
        }
    }
}

// Простая функция вычисления булевых выражений
int evaluate_simple_expression(const char* expr, int* var_values, char* vars, int var_count) {
    // Создаем копию выражения для работы
    char work_expr[MAX_EXPR_LENGTH];
    strcpy(work_expr, expr);

    // Заменяем переменные на их значения
    for (int i = 0; i < var_count; i++) {
        char var_str[2] = { vars[i], '\0' };
        char value_str[2] = { var_values[i] + '0', '\0' };

        char* pos = strstr(work_expr, var_str);
        while (pos != NULL) {
            // Проверяем что это отдельная переменная, а не часть другого слова
            if ((pos == work_expr || !isalpha(*(pos - 1))) &&
                (!isalpha(*(pos + 1)) || *(pos + 1) == '\0')) {
                memcpy(pos, value_str, 1);
            }
            pos = strstr(pos + 1, var_str);
        }
    }

    // Упрощенный калькулятор для базовых операций
    // Для демонстрации обработаем основные случаи

    // a & b
    if (strstr(work_expr, "&") != NULL) {
        return work_expr[0] == '1' && work_expr[4] == '1';
    }
    // a | b
    else if (strstr(work_expr, "|") != NULL) {
        return work_expr[0] == '1' || work_expr[4] == '1';
    }
    // a -> b (импликация)
    else if (strstr(work_expr, "->") != NULL) {
        return work_expr[0] == '0' || work_expr[5] == '1';
    }
    // ~a (отрицание)
    else if (strstr(work_expr, "~") != NULL) {
        return work_expr[1] == '0';
    }
    // a = b (эквиваленция)
    else if (strstr(work_expr, "=") != NULL && strstr(work_expr, "->") == NULL) {
        return work_expr[0] == work_expr[4];
    }
    // a <> b (XOR)
    else if (strstr(work_expr, "<>") != NULL) {
        return work_expr[0] != work_expr[5];
    }
    // a +> b (коимпликация)
    else if (strstr(work_expr, "+>") != NULL) {
        return work_expr[0] == '1' && work_expr[5] == '0';
    }
    // a ! b (штрих Шеффера)
    else if (strstr(work_expr, "!") != NULL) {
        return !(work_expr[0] == '1' && work_expr[4] == '1');
    }
    // a ? b (стрелка Пирса)
    else if (strstr(work_expr, "?") != NULL) {
        return !(work_expr[0] == '1' || work_expr[4] == '1');
    }

    // Если ничего не распознано, возвращаем значение первой переменной
    return work_expr[0] == '1';
}

// Построение таблицы истинности (упрощенная версия)
TruthTable* build_simple_truth_table(const char* expr) {
    printf("Building truth table for: %s\n", expr);

    TruthTable* table = malloc(sizeof(TruthTable));
    strcpy(table->expression, expr);

    // Находим переменные
    find_variables(expr, table->variables, &table->var_count);

    printf("Found %d variables: ", table->var_count);
    for (int i = 0; i < table->var_count; i++) {
        printf("%c ", table->variables[i]);
    }
    printf("\n");

    if (table->var_count == 0) {
        printf("No variables found, using simple evaluation\n");
        table->truth_table = NULL;
        return table;
    }

    int rows = 1 << table->var_count;
    printf("Generating %d rows for truth table\n", rows);

    table->truth_table = malloc(rows * sizeof(int*));
    int** combinations = malloc(rows * sizeof(int*));

    generate_truth_combinations(combinations, table->var_count);

    for (int i = 0; i < rows; i++) {
        table->truth_table[i] = malloc((table->var_count + 1) * sizeof(int));

        // Копируем значения переменных
        for (int j = 0; j < table->var_count; j++) {
            table->truth_table[i][j] = combinations[i][j];
        }

        // Вычисляем значение выражения
        table->truth_table[i][table->var_count] = evaluate_simple_expression(
            expr, combinations[i], table->variables, table->var_count);

        printf("Row %d: ", i);
        for (int j = 0; j < table->var_count; j++) {
            printf("%c=%d ", table->variables[j], combinations[i][j]);
        }
        printf("-> %d\n", table->truth_table[i][table->var_count]);
    }

    // Очистка
    for (int i = 0; i < rows; i++) {
        free(combinations[i]);
    }
    free(combinations);

    return table;
}

// Генерация имени файла
void generate_filename(const char* input_file, char* output_file) {
    const char* base_name = strrchr(input_file, '/');
    if (!base_name) base_name = strrchr(input_file, '\\');
    if (!base_name) base_name = input_file;
    else base_name++;

    // Простая случайная часть
    char random_part[10];
    srand(time(NULL));
    for (int i = 0; i < 6; i++) {
        random_part[i] = 'a' + rand() % 26;
    }
    random_part[6] = '\0';

    sprintf(output_file, "table_%s.txt", random_part);
}

// Запись таблицы в файл
void write_truth_table_to_file(const char* filename, TruthTable* table) {
    char output_file[100];
    generate_filename(filename, output_file);

    FILE* file = fopen(output_file, "w");
    if (!file) {
        printf("ERROR: Cannot create file %s\n", output_file);
        return;
    }

    fprintf(file, "TRUTH TABLE\n");
    fprintf(file, "Expression: %s\n", table->expression);
    fprintf(file, "Variables: ");
    for (int i = 0; i < table->var_count; i++) {
        fprintf(file, "%c", table->variables[i]);
        if (i < table->var_count - 1) fprintf(file, ", ");
    }
    fprintf(file, "\n\n");

    // Заголовок
    for (int i = 0; i < table->var_count; i++) {
        fprintf(file, " %c |", table->variables[i]);
    }
    fprintf(file, " Result\n");

    // Разделитель
    for (int i = 0; i < table->var_count; i++) {
        fprintf(file, "---|");
    }
    fprintf(file, "-------\n");

    // Данные
    int rows = 1 << table->var_count;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < table->var_count; j++) {
            fprintf(file, " %d |", table->truth_table[i][j]);
        }
        fprintf(file, "   %d\n", table->truth_table[i][table->var_count]);
    }

    fclose(file);
    printf("SUCCESS: Truth table saved to %s\n", output_file);
}

// Обработка файла
void process_bool_file_simple(const char* filename) {
    printf("\n=========================================\n");
    printf("PROCESSING FILE: %s\n", filename);
    printf("=========================================\n\n");

    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("ERROR: Cannot open file %s\n", filename);
        return;
    }

    char line[MAX_EXPR_LENGTH];
    int expr_count = 0;

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';

        // Пропускаем пустые строки и комментарии
        if (strlen(line) == 0 || line[0] == '%' || line[0] == '#') continue;

        expr_count++;
        printf("\n--- Expression %d ---\n", expr_count);

        TruthTable* table = build_simple_truth_table(line);
        write_truth_table_to_file(filename, table);

        // Очистка
        if (table->truth_table) {
            int rows = 1 << table->var_count;
            for (int i = 0; i < rows; i++) {
                free(table->truth_table[i]);
            }
            free(table->truth_table);
        }
        free(table);
    }

    fclose(file);
    printf("\n=========================================\n");
    printf("COMPLETED: Processed %d expressions from %s\n", expr_count, filename);
    printf("=========================================\n");
}

// Создание простого демо-файла
void create_simple_demo_file() {
    FILE* file = fopen("simple_bool.txt", "w");
    if (file) {
        fprintf(file, "# Simple Boolean Expressions Demo\n");
        fprintf(file, "a & b\n");
        fprintf(file, "a | b\n");
        fprintf(file, "a -> b\n");
        fprintf(file, "~a\n");
        fprintf(file, "a = b\n");
        fclose(file);
        printf("Created demo file: simple_bool.txt\n");
    }
}

int main(int argc, char* argv[]) {
    printf("=========================================\n");
    printf("   SIMPLE BOOLEAN TRUTH TABLE BUILDER\n");
    printf("=========================================\n\n");

    int table_mode = 0;
    char* filename = NULL;

    // Анализ аргументов
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--table") == 0) {
            table_mode = 1;
        }
        else if (filename == NULL) {
            filename = argv[i];
        }
    }

    if (table_mode) {
        if (filename == NULL) {
            printf("No file specified. Creating demo file...\n");
            create_simple_demo_file();
            printf("\nRunning demo...\n");
            process_bool_file_simple("simple_bool.txt");
        }
        else {
            process_bool_file_simple(filename);
        }
    }
    else {
        printf("Usage:\n");
        printf("  %s --table                    - run demo\n", argv[0]);
        printf("  %s --table <filename>         - process specific file\n", argv[0]);
        printf("\nExample:\n");
        printf("  %s --table\n", argv[0]);
        printf("  %s --table my_formulas.txt\n", argv[0]);
    }

    printf("\nPress Enter to exit...");
    getchar();

    return 0;
}
