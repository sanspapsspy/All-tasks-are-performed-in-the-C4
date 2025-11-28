#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE 256

typedef struct Term {
    int coef;
    int exp;
    struct Term* next;
} Term;

typedef struct Polynomial {
    Term* head;
} Polynomial;

Polynomial accumulator = { NULL };

// Базовые функции
void add_term(Polynomial* poly, int coef, int exp) {
    if (coef == 0) return;

    Term* new_term = malloc(sizeof(Term));
    new_term->coef = coef;
    new_term->exp = exp;
    new_term->next = NULL;

    if (!poly->head || exp > poly->head->exp) {
        new_term->next = poly->head;
        poly->head = new_term;
        return;
    }

    Term* current = poly->head;
    while (current->next && current->next->exp > exp) {
        current = current->next;
    }

    if (current->exp == exp) {
        current->coef += coef;
        free(new_term);
        if (current->coef == 0) {
            // Упрощенное удаление нулевого члена
            if (current == poly->head) {
                poly->head = current->next;
                free(current);
            }
        }
    }
    else {
        new_term->next = current->next;
        current->next = new_term;
    }
}

Polynomial* parse_poly(const char* str) {
    Polynomial* poly = malloc(sizeof(Polynomial));
    poly->head = NULL;

    const char* p = str;
    while (*p) {
        while (*p && isspace(*p)) p++;
        if (!*p) break;

        int sign = 1, coef = 1, exp = 0;

        if (*p == '+') { sign = 1; p++; }
        else if (*p == '-') { sign = -1; p++; }

        if (isdigit(*p)) {
            coef = 0;
            while (isdigit(*p)) {
                coef = coef * 10 + (*p - '0');
                p++;
            }
        }

        coef *= sign;

        if (*p == 'x') {
            p++;
            exp = 1;
            if (*p == '^') {
                p++;
                exp = 0;
                while (isdigit(*p)) {
                    exp = exp * 10 + (*p - '0');
                    p++;
                }
            }
        }

        add_term(poly, coef, exp);
        while (*p && *p != '+' && *p != '-') p++;
    }

    return poly;
}

void print_poly(const Polynomial* poly) {
    if (!poly || !poly->head) {
        printf("0");
        return;
    }

    Term* current = poly->head;
    int first = 1;

    while (current) {
        int coef = current->coef;
        int exp = current->exp;

        if (!first) printf(" %c ", coef >= 0 ? '+' : '-');
        else if (coef < 0) printf("-");

        coef = abs(coef);

        if (exp == 0) printf("%d", coef);
        else if (exp == 1) printf(coef == 1 ? "x" : "%dx", coef);
        else printf(coef == 1 ? "x^%d" : "%dx^%d", coef, exp);

        first = 0;
        current = current->next;
    }
}

void free_poly(Polynomial* poly) {
    if (!poly) return;
    Term* current = poly->head;
    while (current) {
        Term* next = current->next;
        free(current);
        current = next;
    }
    free(poly);
}

// Операции
Polynomial* poly_add(const Polynomial* a, const Polynomial* b) {
    Polynomial* result = malloc(sizeof(Polynomial));
    result->head = NULL;

    Term* current = a->head;
    while (current) {
        add_term(result, current->coef, current->exp);
        current = current->next;
    }

    current = b->head;
    while (current) {
        add_term(result, current->coef, current->exp);
        current = current->next;
    }

    return result;
}

Polynomial* poly_mult(const Polynomial* a, const Polynomial* b) {
    Polynomial* result = malloc(sizeof(Polynomial));
    result->head = NULL;

    Term* ta = a->head;
    while (ta) {
        Term* tb = b->head;
        while (tb) {
            add_term(result, ta->coef * tb->coef, ta->exp + tb->exp);
            tb = tb->next;
        }
        ta = ta->next;
    }

    return result;
}

int poly_eval(const Polynomial* poly, int x) {
    int result = 0;
    Term* current = poly->head;
    while (current) {
        int term = current->coef;
        for (int i = 0; i < current->exp; i++) term *= x;
        result += term;
        current = current->next;
    }
    return result;
}

Polynomial* poly_diff(const Polynomial* poly) {
    Polynomial* result = malloc(sizeof(Polynomial));
    result->head = NULL;

    Term* current = poly->head;
    while (current) {
        if (current->exp > 0) {
            add_term(result, current->coef * current->exp, current->exp - 1);
        }
        current = current->next;
    }

    return result;
}

// Сумматор
void set_accumulator(Polynomial* poly) {
    free_poly(&accumulator);
    accumulator.head = NULL;
    if (poly) {
        Term* current = poly->head;
        while (current) {
            add_term(&accumulator, current->coef, current->exp);
            current = current->next;
        }
    }
}

Polynomial* get_accumulator() {
    Polynomial* copy = malloc(sizeof(Polynomial));
    copy->head = NULL;
    Term* current = accumulator.head;
    while (current) {
        add_term(copy, current->coef, current->exp);
        current = current->next;
    }
    return copy;
}

// Обработка команд
void process_command(const char* line) {
    char buf[MAX_LINE];
    strcpy(buf, line);

    char* nl = strchr(buf, '\n');
    if (nl) *nl = 0;
    char* sc = strchr(buf, ';');
    if (sc) *sc = 0;

    if (buf[0] == '%' || buf[0] == '[') return;

    char* start = buf;
    while (*start && isspace(*start)) start++;
    if (!*start) return;

    printf("> %s\n", start);

    if (strncmp(start, "Add", 3) == 0) {
        char* p = strchr(start, '(');
        if (!p) { printf("Error\n"); return; }

        char p1[100], p2[100];
        int count = sscanf(p, "(%99[^,],%99[^)])", p1, p2);

        Polynomial* a, * b;
        if (count == 2) {
            a = parse_poly(p1);
            b = parse_poly(p2);
        }
        else if (count == 1) {
            a = get_accumulator();
            b = parse_poly(p1);
        }
        else { printf("Error\n"); return; }

        Polynomial* res = poly_add(a, b);
        set_accumulator(res);
        printf("Result: "); print_poly(res); printf("\n");

        free_poly(a);
        if (count == 2) free_poly(b);
        free_poly(res);

    }
    else if (strncmp(start, "Mult", 4) == 0) {
        char* p = strchr(start, '(');
        if (!p) { printf("Error\n"); return; }

        char p1[100], p2[100];
        int count = sscanf(p, "(%99[^,],%99[^)])", p1, p2);

        Polynomial* a, * b;
        if (count == 2) {
            a = parse_poly(p1);
            b = parse_poly(p2);
        }
        else if (count == 1) {
            a = get_accumulator();
            b = parse_poly(p1);
        }
        else { printf("Error\n"); return; }

        Polynomial* res = poly_mult(a, b);
        set_accumulator(res);
        printf("Result: "); print_poly(res); printf("\n");

        free_poly(a);
        if (count == 2) free_poly(b);
        free_poly(res);

    }
    else if (strncmp(start, "Eval", 4) == 0) {
        char* p = strchr(start, '(');
        if (!p) { printf("Error\n"); return; }

        int x;
        if (sscanf(p, "(%d)", &x) == 1) {
            Polynomial* a = get_accumulator();
            int val = poly_eval(a, x);
            printf("P(%d) = %d\n", x, val);
            free_poly(a);
        }
        else printf("Error\n");

    }
    else if (strncmp(start, "Diff", 4) == 0) {
        Polynomial* a = get_accumulator();
        Polynomial* res = poly_diff(a);
        set_accumulator(res);
        printf("Result: "); print_poly(res); printf("\n");
        free_poly(a);
        free_poly(res);

    }
    else {
        printf("Unknown command\n");
    }
}

// Создание тестового файла
void create_demo_file() {
    FILE* f = fopen("poly_demo.txt", "w");
    if (f) {
        fprintf(f, "Add(2x^2-x+2,-x^2+3x-1);\n");
        fprintf(f, "Mult(x+1,x-1);\n");
        fprintf(f, "Eval(2);\n");
        fprintf(f, "Diff();\n");
        fclose(f);
        printf("Demo file 'poly_demo.txt' created!\n");
    }
}

int main() {
    printf("=== Simple Polynomial Calculator ===\n");

    // Инициализация сумматора
    add_term(&accumulator, 0, 0);

    // Создаем демо файл
    create_demo_file();

    // Читаем и выполняем команды
    FILE* f = fopen("poly_demo.txt", "r");
    if (!f) {
        printf("Cannot open file\n");
        return 1;
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        process_command(line);
    }

    fclose(f);
    free_poly(&accumulator);

    printf("\nPress any key to exit...");
    getchar();
    return 0;
}
