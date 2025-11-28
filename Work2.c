#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define EPS 1e-9
#define MAX_NAME_LENGTH 50
#define MAX_PATH_LENGTH 256

typedef struct Date {
    int day, month, year;
} Date;

typedef struct Citizen {
    char surname[MAX_NAME_LENGTH];
    char name[MAX_NAME_LENGTH];
    char patronymic[MAX_NAME_LENGTH];
    Date birth_date;
    char gender;
    double income;
    struct Citizen* next;
} Citizen;

typedef enum { ADD, MODIFY, DELETE } OperationType;

typedef struct Operation {
    OperationType type;
    Citizen citizen;
    Citizen original_citizen;
    struct Operation* next;
} Operation;

typedef struct UndoStack {
    Operation* operations;
    int count;
    int total_modifications;
} UndoStack;

// Функции для работы с датами
int is_valid_date(int day, int month, int year) {
    if (year < 1900 || year > 2100) return 0;
    if (month < 1 || month > 12) return 0;

    int days_in_month;
    switch (month) {
    case 2:
        days_in_month = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 29 : 28;
        break;
    case 4: case 6: case 9: case 11:
        days_in_month = 30;
        break;
    default:
        days_in_month = 31;
    }

    return (day >= 1 && day <= days_in_month);
}

int compare_dates(const Date* a, const Date* b) {
    if (a->year != b->year) return a->year - b->year;
    if (a->month != b->month) return a->month - b->month;
    return a->day - b->day;
}

int calculate_age(const Date* birth_date) {
    return 2024 - birth_date->year;
}

int is_valid_name(const char* name, int allow_empty) {
    if (!name) return 0;
    if (!allow_empty && strlen(name) == 0) return 0;

    for (size_t i = 0; i < strlen(name); i++) {
        if (!isalpha(name[i])) return 0;
    }
    return 1;
}

int is_valid_gender(char gender) {
    return gender == 'M' || gender == 'W';
}

// Функции для работы со списком жителей
Citizen* create_citizen(const char* surname, const char* name, const char* patronymic,
    Date birth_date, char gender, double income) {
    Citizen* new_citizen = malloc(sizeof(Citizen));
    if (!new_citizen) return NULL;

    strcpy(new_citizen->surname, surname);
    strcpy(new_citizen->name, name);
    strcpy(new_citizen->patronymic, patronymic);
    new_citizen->birth_date = birth_date;
    new_citizen->gender = gender;
    new_citizen->income = income;
    new_citizen->next = NULL;

    return new_citizen;
}

void insert_sorted(Citizen** head, Citizen* new_citizen) {
    if (*head == NULL || compare_dates(&(*head)->birth_date, &new_citizen->birth_date) > 0) {
        new_citizen->next = *head;
        *head = new_citizen;
        return;
    }

    Citizen* current = *head;
    while (current->next != NULL &&
        compare_dates(&current->next->birth_date, &new_citizen->birth_date) <= 0) {
        current = current->next;
    }

    new_citizen->next = current->next;
    current->next = new_citizen;
}

Citizen* find_citizen(Citizen* head, const char* surname, const char* name) {
    Citizen* current = head;
    while (current != NULL) {
        if (strcmp(current->surname, surname) == 0 && strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void delete_citizen(Citizen** head, const char* surname, const char* name) {
    if (*head == NULL) return;

    if (strcmp((*head)->surname, surname) == 0 && strcmp((*head)->name, name) == 0) {
        Citizen* temp = *head;
        *head = (*head)->next;
        free(temp);
        return;
    }

    Citizen* current = *head;
    while (current->next != NULL) {
        if (strcmp(current->next->surname, surname) == 0 &&
            strcmp(current->next->name, name) == 0) {
            Citizen* temp = current->next;
            current->next = current->next->next;
            free(temp);
            return;
        }
        current = current->next;
    }
}

// Функции для системы Undo
void init_undo_stack(UndoStack* stack) {
    stack->operations = NULL;
    stack->count = 0;
    stack->total_modifications = 0;
}

void push_operation(UndoStack* stack, OperationType type, const Citizen* citizen,
    const Citizen* original_citizen) {
    Operation* new_op = malloc(sizeof(Operation));
    if (!new_op) return;

    new_op->type = type;
    if (citizen) new_op->citizen = *citizen;
    if (original_citizen) new_op->original_citizen = *original_citizen;
    new_op->next = stack->operations;
    stack->operations = new_op;
    stack->count++;
    stack->total_modifications++;
}

int undo_last_operations(UndoStack* stack, Citizen** head) {
    if (stack->count == 0) {
        printf("No operations to undo\n");
        return 0;
    }

    int operations_to_undo = stack->total_modifications / 2;
    if (operations_to_undo == 0) operations_to_undo = 1;

    printf("Undoing last %d operations\n", operations_to_undo);

    int undone_count = 0;
    for (int i = 0; i < operations_to_undo && stack->operations != NULL; i++) {
        Operation* op = stack->operations;
        stack->operations = op->next;

        switch (op->type) {
        case ADD:
            delete_citizen(head, op->citizen.surname, op->citizen.name);
            printf("Undo: Added citizen %s %s\n", op->citizen.surname, op->citizen.name);
            break;

        case DELETE:
            insert_sorted(head, create_citizen(op->citizen.surname, op->citizen.name,
                op->citizen.patronymic, op->citizen.birth_date,
                op->citizen.gender, op->citizen.income));
            printf("Undo: Deleted citizen %s %s\n", op->citizen.surname, op->citizen.name);
            break;

        case MODIFY:
        {
            Citizen* citizen = find_citizen(*head, op->citizen.surname, op->citizen.name);
            if (citizen) {
                *citizen = op->original_citizen;
            }
        }
        printf("Undo: Modified citizen %s %s\n", op->citizen.surname, op->citizen.name);
        break;
        }

        free(op);
        stack->count--;
        undone_count++;
    }

    stack->total_modifications -= undone_count;
    return undone_count;
}

// Улучшенные функции работы с файлами
Citizen* read_citizens_from_file(const char* filename, UndoStack* undo_stack) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Cannot open file '%s'\n", filename);
        printf("Please check:\n");
        printf("1. Does the file exist?\n");
        printf("2. Is the file path correct?\n");
        printf("3. Do you have read permissions?\n");
        return NULL;
    }

    Citizen* head = NULL;
    char line[512];
    int line_num = 0;
    int success_count = 0;

    printf("Reading from file: %s\n", filename);

    while (fgets(line, sizeof(line), file)) {
        line_num++;

        // Пропускаем пустые строки
        if (strlen(line) <= 1) continue;

        char surname[MAX_NAME_LENGTH], name[MAX_NAME_LENGTH], patronymic[MAX_NAME_LENGTH];
        int day, month, year;
        char gender;
        double income;

        // Пробуем разные форматы ввода
        int parsed = sscanf(line, "%49s %49s %49s %d.%d.%d %c %lf",
            surname, name, patronymic, &day, &month, &year, &gender, &income);

        if (parsed < 7) {
            // Пробуем другой формат с пробелами вместо точек в дате
            parsed = sscanf(line, "%49s %49s %49s %d %d %d %c %lf",
                surname, name, patronymic, &day, &month, &year, &gender, &income);
        }

        if (parsed >= 7) {
            if (is_valid_name(surname, 0) && is_valid_name(name, 0) &&
                is_valid_name(patronymic, 1) && is_valid_date(day, month, year) &&
                is_valid_gender(gender) && income >= 0) {

                Date birth_date = { day, month, year };
                Citizen* new_citizen = create_citizen(surname, name, patronymic,
                    birth_date, gender, income);
                if (new_citizen) {
                    insert_sorted(&head, new_citizen);
                    success_count++;
                }
            }
            else {
                printf("Warning: Invalid data at line %d: %s", line_num, line);
            }
        }
        else {
            printf("Warning: Cannot parse line %d: %s", line_num, line);
        }
    }

    fclose(file);

    if (success_count > 0) {
        printf("Successfully loaded %d citizens from file\n", success_count);
    }
    else {
        printf("No valid citizen data found in file\n");
    }

    return head;
}

int write_citizens_to_file(Citizen* head, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        printf("Error: Cannot create file '%s'\n", filename);
        return 0;
    }

    Citizen* current = head;
    int count = 0;

    while (current != NULL) {
        fprintf(file, "%s %s %s %02d.%02d.%04d %c %.2f\n",
            current->surname, current->name, current->patronymic,
            current->birth_date.day, current->birth_date.month, current->birth_date.year,
            current->gender, current->income);
        current = current->next;
        count++;
    }

    fclose(file);
    printf("Successfully written %d citizens to '%s'\n", count, filename);
    return 1;
}

void print_citizens(Citizen* head) {
    printf("\n=== Citizens List (by age) ===\n");
    Citizen* current = head;
    int index = 1;

    if (current == NULL) {
        printf("No citizens in the list\n");
        return;
    }

    while (current != NULL) {
        printf("%d. %s %s %s | Born: %02d.%02d.%04d | Gender: %c | Income: %.2f | Age: %d\n",
            index++, current->surname, current->name, current->patronymic,
            current->birth_date.day, current->birth_date.month, current->birth_date.year,
            current->gender, current->income, calculate_age(&current->birth_date));
        current = current->next;
    }
    printf("Total: %d citizens\n", index - 1);
}

// Функции для пользовательского интерфейса
void add_citizen_ui(Citizen** head, UndoStack* undo_stack) {
    char surname[MAX_NAME_LENGTH], name[MAX_NAME_LENGTH], patronymic[MAX_NAME_LENGTH];
    int day, month, year;
    char gender;
    double income;

    printf("\n=== Add New Citizen ===\n");

    printf("Surname: ");
    if (scanf("%49s", surname) != 1) {
        printf("Error reading surname\n");
        return;
    }

    printf("Name: ");
    if (scanf("%49s", name) != 1) {
        printf("Error reading name\n");
        return;
    }

    printf("Patronymic: ");
    if (scanf("%49s", patronymic) != 1) {
        printf("Error reading patronymic\n");
        return;
    }

    printf("Birth date (dd mm yyyy): ");
    if (scanf("%d %d %d", &day, &month, &year) != 3) {
        printf("Error reading birth date\n");
        return;
    }

    printf("Gender (M/W): ");
    if (scanf(" %c", &gender) != 1) {
        printf("Error reading gender\n");
        return;
    }

    printf("Monthly income: ");
    if (scanf("%lf", &income) != 1) {
        printf("Error reading income\n");
        return;
    }

    // Валидация данных
    if (!is_valid_name(surname, 0)) {
        printf("Error: Invalid surname (only letters allowed, cannot be empty)\n");
        return;
    }
    if (!is_valid_name(name, 0)) {
        printf("Error: Invalid name (only letters allowed, cannot be empty)\n");
        return;
    }
    if (!is_valid_name(patronymic, 1)) {
        printf("Error: Invalid patronymic (only letters allowed)\n");
        return;
    }
    if (!is_valid_date(day, month, year)) {
        printf("Error: Invalid date\n");
        return;
    }
    if (!is_valid_gender(gender)) {
        printf("Error: Invalid gender (must be M or W)\n");
        return;
    }
    if (income < 0) {
        printf("Error: Income cannot be negative\n");
        return;
    }

    // Проверка на дубликат
    if (find_citizen(*head, surname, name)) {
        printf("Error: Citizen with this surname and name already exists\n");
        return;
    }

    Date birth_date = { day, month, year };
    Citizen* new_citizen = create_citizen(surname, name, patronymic, birth_date, gender, income);
    if (new_citizen) {
        insert_sorted(head, new_citizen);
        push_operation(undo_stack, ADD, new_citizen, NULL);
        printf("Citizen added successfully!\n");
    }
    else {
        printf("Error: Memory allocation failed\n");
    }
}

// Остальные функции UI (modify_citizen_ui, delete_citizen_ui, search_citizen_ui, export_data_ui)
// остаются такими же как в предыдущей версии

void modify_citizen_ui(Citizen* head, UndoStack* undo_stack) {
    char surname[MAX_NAME_LENGTH], name[MAX_NAME_LENGTH];

    printf("\n=== Modify Citizen ===\n");
    printf("Enter surname: "); scanf("%49s", surname);
    printf("Enter name: "); scanf("%49s", name);

    Citizen* citizen = find_citizen(head, surname, name);
    if (!citizen) {
        printf("Error: Citizen not found\n");
        return;
    }

    printf("Current data: %s %s %s | %02d.%02d.%04d | %c | %.2f\n",
        citizen->surname, citizen->name, citizen->patronymic,
        citizen->birth_date.day, citizen->birth_date.month, citizen->birth_date.year,
        citizen->gender, citizen->income);

    Citizen original = *citizen;

    int choice;
    do {
        printf("\nWhat to modify?\n");
        printf("1. Patronymic\n");
        printf("2. Birth date\n");
        printf("3. Gender\n");
        printf("4. Income\n");
        printf("0. Done\n");
        printf("Choice: ");
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input\n");
            while (getchar() != '\n'); // очистка буфера
            continue;
        }

        switch (choice) {
        case 1:
            printf("New patronymic: ");
            scanf("%49s", citizen->patronymic);
            if (!is_valid_name(citizen->patronymic, 1)) {
                printf("Error: Invalid patronymic\n");
                strcpy(citizen->patronymic, original.patronymic);
            }
            break;
        case 2:
            printf("New birth date (dd mm yyyy): ");
            if (scanf("%d %d %d", &citizen->birth_date.day, &citizen->birth_date.month, &citizen->birth_date.year) != 3) {
                printf("Error reading date\n");
                citizen->birth_date = original.birth_date;
            }
            else if (!is_valid_date(citizen->birth_date.day, citizen->birth_date.month, citizen->birth_date.year)) {
                printf("Error: Invalid date\n");
                citizen->birth_date = original.birth_date;
            }
            break;
        case 3:
            printf("New gender (M/W): ");
            scanf(" %c", &citizen->gender);
            if (!is_valid_gender(citizen->gender)) {
                printf("Error: Invalid gender\n");
                citizen->gender = original.gender;
            }
            break;
        case 4:
            printf("New income: ");
            if (scanf("%lf", &citizen->income) != 1) {
                printf("Error reading income\n");
                citizen->income = original.income;
            }
            else if (citizen->income < 0) {
                printf("Error: Invalid income\n");
                citizen->income = original.income;
            }
            break;
        case 0:
            break;
        default:
            printf("Invalid choice\n");
        }
    } while (choice != 0);

    push_operation(undo_stack, MODIFY, citizen, &original);
    printf("Citizen modified successfully\n");
}

void delete_citizen_ui(Citizen** head, UndoStack* undo_stack) {
    char surname[MAX_NAME_LENGTH], name[MAX_NAME_LENGTH];

    printf("\n=== Delete Citizen ===\n");
    printf("Enter surname: "); scanf("%49s", surname);
    printf("Enter name: "); scanf("%49s", name);

    Citizen* citizen = find_citizen(*head, surname, name);
    if (!citizen) {
        printf("Error: Citizen not found\n");
        return;
    }

    push_operation(undo_stack, DELETE, citizen, NULL);
    delete_citizen(head, surname, name);
    printf("Citizen deleted successfully\n");
}

void search_citizen_ui(Citizen* head) {
    char surname[MAX_NAME_LENGTH], name[MAX_NAME_LENGTH];

    printf("\n=== Search Citizen ===\n");
    printf("Enter surname: "); scanf("%49s", surname);
    printf("Enter name: "); scanf("%49s", name);

    Citizen* citizen = find_citizen(head, surname, name);
    if (citizen) {
        printf("Found: %s %s %s | Born: %02d.%02d.%04d | Gender: %c | Income: %.2f | Age: %d\n",
            citizen->surname, citizen->name, citizen->patronymic,
            citizen->birth_date.day, citizen->birth_date.month, citizen->birth_date.year,
            citizen->gender, citizen->income, calculate_age(&citizen->birth_date));
    }
    else {
        printf("Citizen not found\n");
    }
}

void export_data_ui(Citizen* head) {
    char filename[MAX_PATH_LENGTH];

    printf("\n=== Export Data ===\n");
    printf("Enter output file path: ");
    scanf("%255s", filename);

    write_citizens_to_file(head, filename);
}

// Функция для создания демо-файла
void create_demo_file() {
    FILE* file = fopen("citizens.txt", "w");
    if (file) {
        fprintf(file, "Smith John Michael 15.05.1980 M 2500.50\n");
        fprintf(file, "Johnson Mary 22.11.1992 W 1800.00\n");
        fprintf(file, "Williams Robert James 03.03.1975 M 3200.75\n");
        fprintf(file, "Brown Lisa 18.09.1988 W 2100.25\n");
        fprintf(file, "Davis David 29.12.1995 M 1900.00\n");
        fclose(file);
        printf("Demo file 'citizens.txt' created successfully!\n");
    }
    else {
        printf("Error: Cannot create demo file\n");
    }
}

int main() {
    Citizen* citizens = NULL;
    UndoStack undo_stack;
    init_undo_stack(&undo_stack);

    printf("=== Citizen Management System ===\n\n");

    // Запрос файла с проверкой существования
    char input_file[MAX_PATH_LENGTH];
    printf("Enter input file path (or press Enter for 'citizens.txt'): ");

    if (fgets(input_file, sizeof(input_file), stdin) == NULL) {
        printf("Error reading input\n");
        return 1;
    }

    // Убираем символ новой строки
    input_file[strcspn(input_file, "\n")] = 0;

    // Если пользователь не ввел ничего, используем citizens.txt
    if (strlen(input_file) == 0) {
        strcpy(input_file, "citizens.txt");
    }

    // Проверяем существование файла
    FILE* test_file = fopen(input_file, "r");
    if (!test_file) {
        printf("File '%s' not found.\n", input_file);
        printf("Would you like to create a demo file? (y/n): ");
        char choice;
        scanf(" %c", &choice);
        if (choice == 'y' || choice == 'Y') {
            create_demo_file();
            citizens = read_citizens_from_file("citizens.txt", &undo_stack);
        }
        else {
            printf("Starting with empty list.\n");
        }
    }
    else {
        fclose(test_file);
        citizens = read_citizens_from_file(input_file, &undo_stack);
    }

    // Очистка буфера после scanf
    while (getchar() != '\n');

    int choice;
    do {
        printf("\n=== Main Menu ===\n");
        printf("1. Show all citizens\n");
        printf("2. Add citizen\n");
        printf("3. Modify citizen\n");
        printf("4. Delete citizen\n");
        printf("5. Search citizen\n");
        printf("6. Export data to file\n");
        printf("7. Undo last operations\n");
        printf("8. Show undo stack info\n");
        printf("0. Exit\n");
        printf("Choice: ");

        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            while (getchar() != '\n'); // очистка буфера
            continue;
        }

        switch (choice) {
        case 1:
            print_citizens(citizens);
            break;
        case 2:
            add_citizen_ui(&citizens, &undo_stack);
            break;
        case 3:
            modify_citizen_ui(citizens, &undo_stack);
            break;
        case 4:
            delete_citizen_ui(&citizens, &undo_stack);
            break;
        case 5:
            search_citizen_ui(citizens);
            break;
        case 6:
            export_data_ui(citizens);
            break;
        case 7:
            undo_last_operations(&undo_stack, &citizens);
            break;
        case 8:
            printf("Undo stack: %d operations, total modifications: %d\n",
                undo_stack.count, undo_stack.total_modifications);
            break;
        case 0:
            printf("Exiting...\n");
            break;
        default:
            printf("Invalid choice. Please try again.\n");
        }

        // Очистка буфера после каждого выбора
        while (getchar() != '\n');

    } while (choice != 0);

    // Очистка памяти
    Citizen* current = citizens;
    while (current != NULL) {
        Citizen* next = current->next;
        free(current);
        current = next;
    }

    Operation* op = undo_stack.operations;
    while (op != NULL) {
        Operation* next = op->next;
        free(op);
        op = next;
    }

    return 0;
}

//(bash)gcc -o citizen_manager citizen_manager.c
