#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define EPS 1e-9
#define MAX_NAME_LENGTH 50
#define MAX_UNDO_OPERATIONS 100

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
    Citizen original_citizen; // для MODIFY операций
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
    // Упрощенный расчет возраста (только по году)
    return 2024 - birth_date->year; // Текущий год можно сделать параметром
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

// Функции ввода/вывода
Citizen* read_citizens_from_file(const char* filename, UndoStack* undo_stack) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Cannot open file %s\n", filename);
        return NULL;
    }
    
    Citizen* head = NULL;
    char line[256];
    
    while (fgets(line, sizeof(line), file)) {
        char surname[MAX_NAME_LENGTH], name[MAX_NAME_LENGTH], patronymic[MAX_NAME_LENGTH];
        int day, month, year;
        char gender;
        double income;
        
        int parsed = sscanf(line, "%49s %49s %49s %d.%d.%d %c %lf",
                           surname, name, patronymic, &day, &month, &year, &gender, &income);
        
        if (parsed >= 7 && is_valid_name(surname, 0) && is_valid_name(name, 0) &&
            is_valid_name(patronymic, 1) && is_valid_date(day, month, year) &&
            is_valid_gender(gender) && income >= 0) {
            
            Date birth_date = {day, month, year};
            Citizen* new_citizen = create_citizen(surname, name, patronymic, 
                                                birth_date, gender, income);
            if (new_citizen) {
                insert_sorted(&head, new_citizen);
            }
        } else {
            printf("Warning: Invalid data format: %s", line);
        }
    }
    
    fclose(file);
    return head;
}

void write_citizens_to_file(Citizen* head, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        printf("Error: Cannot create file %s\n", filename);
        return;
    }
    
    Citizen* current = head;
    while (current != NULL) {
        fprintf(file, "%s %s %s %02d.%02d.%04d %c %.2f\n",
                current->surname, current->name, current->patronymic,
                current->birth_date.day, current->birth_date.month, current->birth_date.year,
                current->gender, current->income);
        current = current->next;
    }
    
    fclose(file);
    printf("Data written to %s\n", filename);
}

void print_citizens(Citizen* head) {
    printf("\n=== Citizens List (by age) ===\n");
    Citizen* current = head;
    int index = 1;
    
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
    
    printf("Surname: "); scanf("%49s", surname);
    printf("Name: "); scanf("%49s", name);
    printf("Patronymic: "); scanf("%49s", patronymic);
    printf("Birth date (dd mm yyyy): "); scanf("%d %d %d", &day, &month, &year);
    printf("Gender (M/W): "); scanf(" %c", &gender);
    printf("Monthly income: "); scanf("%lf", &income);
    
    if (!is_valid_name(surname, 0) || !is_valid_name(name, 0) || 
        !is_valid_name(patronymic, 1) || !is_valid_date(day, month, year) ||
        !is_valid_gender(gender) || income < 0) {
        printf("Error: Invalid input data\n");
        return;
    }
    
    if (find_citizen(*head, surname, name)) {
        printf("Error: Citizen with this surname and name already exists\n");
        return;
    }
    
    Date birth_date = {day, month, year};
    Citizen* new_citizen = create_citizen(surname, name, patronymic, birth_date, gender, income);
    if (new_citizen) {
        insert_sorted(head, new_citizen);
        push_operation(undo_stack, ADD, new_citizen, NULL);
        printf("Citizen added successfully\n");
    }
}

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
    
    Citizen original = *citizen; // сохраняем оригинальные данные
    
    int choice;
    do {
        printf("\nWhat to modify?\n");
        printf("1. Patronymic\n");
        printf("2. Birth date\n");
        printf("3. Gender\n");
        printf("4. Income\n");
        printf("0. Done\n");
        printf("Choice: "); scanf("%d", &choice);
        
        switch (choice) {
            case 1:
                printf("New patronymic: "); scanf("%49s", citizen->patronymic);
                if (!is_valid_name(citizen->patronymic, 1)) {
                    printf("Error: Invalid patronymic\n");
                    strcpy(citizen->patronymic, original.patronymic);
                }
                break;
            case 2:
                printf("New birth date (dd mm yyyy): "); 
                scanf("%d %d %d", &citizen->birth_date.day, &citizen->birth_date.month, &citizen->birth_date.year);
                if (!is_valid_date(citizen->birth_date.day, citizen->birth_date.month, citizen->birth_date.year)) {
                    printf("Error: Invalid date\n");
                    citizen->birth_date = original.birth_date;
                }
                break;
            case 3:
                printf("New gender (M/W): "); scanf(" %c", &citizen->gender);
                if (!is_valid_gender(citizen->gender)) {
                    printf("Error: Invalid gender\n");
                    citizen->gender = original.gender;
                }
                break;
            case 4:
                printf("New income: "); scanf("%lf", &citizen->income);
                if (citizen->income < 0) {
                    printf("Error: Invalid income\n");
                    citizen->income = original.income;
                }
                break;
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
    } else {
        printf("Citizen not found\n");
    }
}

void export_data_ui(Citizen* head) {
    char filename[100];
    
    printf("\n=== Export Data ===\n");
    printf("Enter output file path: ");
    scanf("%99s", filename);
    
    write_citizens_to_file(head, filename);
}

int main() {
    Citizen* citizens = NULL;
    UndoStack undo_stack;
    init_undo_stack(&undo_stack);
    
    char input_file[100];
    printf("Enter input file path: ");
    scanf("%99s", input_file);
    
    citizens = read_citizens_from_file(input_file, &undo_stack);
    if (!citizens) {
        printf("No data loaded. Starting with empty list.\n");
    } else {
        printf("Data loaded successfully from %s\n", input_file);
    }
    
    int choice;
    do {
        printf("\n=== Citizen Management System ===\n");
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
        scanf("%d", &choice);
        
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
                printf("Invalid choice\n");
        }
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
