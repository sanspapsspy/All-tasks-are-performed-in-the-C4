#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define EPS 1e-9
#define MAX_NAME_LENGTH 50
#define INITIAL_CAPACITY 10

typedef struct {
    unsigned int id;
    char name[MAX_NAME_LENGTH];
    char surname[MAX_NAME_LENGTH];
    double salary;
} Employee;

// Функция сравнения для qsort
int compare_employees_asc(const void* a, const void* b) {
    const Employee* emp1 = (const Employee*)a;
    const Employee* emp2 = (const Employee*)b;
    
    // Сравнение по зарплате
    if (fabs(emp1->salary - emp2->salary) > EPS) {
        return (emp1->salary > emp2->salary) ? 1 : -1;
    }
    
    // Сравнение по фамилии
    int surname_cmp = strcmp(emp1->surname, emp2->surname);
    if (surname_cmp != 0) return surname_cmp;
    
    // Сравнение по имени
    int name_cmp = strcmp(emp1->name, emp2->name);
    if (name_cmp != 0) return name_cmp;
    
    // Сравнение по ID
    return (emp1->id > emp2->id) ? 1 : -1;
}

int compare_employees_desc(const void* a, const void* b) {
    return -compare_employees_asc(a, b);
}

// Валидация строки (только латинские буквы)
int is_valid_name(const char* name) {
    if (name == NULL || strlen(name) == 0) return 0;
    
    for (size_t i = 0; i < strlen(name); i++) {
        if (!isalpha(name[i])) return 0;
    }
    return 1;
}

// Чтение сотрудников из файла
Employee* read_employees(const char* filename, int* count) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file %s\n", filename);
        return NULL;
    }
    
    Employee* employees = malloc(INITIAL_CAPACITY * sizeof(Employee));
    if (!employees) {
        fclose(file);
        return NULL;
    }
    
    int capacity = INITIAL_CAPACITY;
    *count = 0;
    char line[256];
    
    while (fgets(line, sizeof(line), file)) {
        // Проверяем необходимость увеличения массива
        if (*count >= capacity) {
            capacity *= 2;
            Employee* temp = realloc(employees, capacity * sizeof(Employee));
            if (!temp) {
                fprintf(stderr, "Memory allocation error\n");
                fclose(file);
                free(employees);
                return NULL;
            }
            employees = temp;
        }
        
        // Парсинг строки
        Employee emp;
        int parsed = sscanf(line, "%u %49s %49s %lf", 
                           &emp.id, emp.name, emp.surname, &emp.salary);
        
        if (parsed == 4 && is_valid_name(emp.name) && is_valid_name(emp.surname) && emp.salary >= 0) {
            employees[(*count)++] = emp;
        } else {
            fprintf(stderr, "Warning: Invalid data format in line: %s", line);
        }
    }
    
    fclose(file);
    return employees;
}

// Запись сотрудников в файл
int write_employees(const char* filename, Employee* employees, int count) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Error: Cannot create file %s\n", filename);
        return 0;
    }
    
    for (int i = 0; i < count; i++) {
        fprintf(file, "%u %s %s %.2f\n", 
                employees[i].id, 
                employees[i].name, 
                employees[i].surname, 
                employees[i].salary);
    }
    
    fclose(file);
    return 1;
}

// Основная функция
int main(int argc, char* argv[]) {
    // Валидация аргументов командной строки
    if (argc != 4) {
        printf("Usage: %s <input_file> <-a/-d> <output_file>\n", argv[0]);
        printf("Flags: -a for ascending, -d for descending\n");
        return 1;
    }
    
    const char* input_file = argv[1];
    const char* flag = argv[2];
    const char* output_file = argv[3];
    
    // Валидация флага
    if (strcmp(flag, "-a") != 0 && strcmp(flag, "-d") != 0 &&
        strcmp(flag, "/a") != 0 && strcmp(flag, "/d") != 0) {
        fprintf(stderr, "Error: Invalid flag. Use -a/-d or /a//d\n");
        return 1;
    }
    
    int is_ascending = (strcmp(flag, "-a") == 0 || strcmp(flag, "/a") == 0);
    
    // Чтение данных
    int employee_count = 0;
    Employee* employees = read_employees(input_file, &employee_count);
    
    if (!employees || employee_count == 0) {
        fprintf(stderr, "Error: No valid employees data found\n");
        free(employees);
        return 1;
    }
    
    printf("Read %d employees from %s\n", employee_count, input_file);
    
    // Сортировка
    if (is_ascending) {
        qsort(employees, employee_count, sizeof(Employee), compare_employees_asc);
        printf("Sorted in ascending order\n");
    } else {
        qsort(employees, employee_count, sizeof(Employee), compare_employees_desc);
        printf("Sorted in descending order\n");
    }
    
    // Запись результатов
    if (write_employees(output_file, employees, employee_count)) {
        printf("Results written to %s\n", output_file);
    } else {
        fprintf(stderr, "Error writing to output file\n");
        free(employees);
        return 1;
    }
    
    // Освобождение памяти
    free(employees);
    
    return 0;
}
