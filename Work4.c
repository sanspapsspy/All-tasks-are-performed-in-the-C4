#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct Node {
    int value;
    struct Node* prev;
    struct Node* next;
} Node;

Node* create_circular_list(int n) {
    if (n <= 0) return NULL;

    Node* head = malloc(sizeof(Node));
    head->value = 1;
    head->prev = head;
    head->next = head;

    Node* current = head;

    for (int i = 2; i <= n; i++) {
        Node* new_node = malloc(sizeof(Node));
        new_node->value = i;

        new_node->prev = head->prev;
        new_node->next = head;
        head->prev->next = new_node;
        head->prev = new_node;
    }

    return head;
}

int is_prime(int n) {
    if (n < 2) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;

    for (int i = 3; i <= sqrt(n); i += 2) {
        if (n % i == 0) return 0;
    }
    return 1;
}

long long calculate_prime_product(int x) {
    long long product = 1;
    for (int i = 2; i < x; i++) {
        if (is_prime(i)) {
            product *= i;
        }
    }
    return product;
}

int josephus_simulation(int n, int k, int forward) {
    Node* current = create_circular_list(n);
    if (!current) return -1;

    printf("Elimination order: ");

    if (!forward) {
        current = current->prev;
    }

    while (current->next != current) {
        for (int i = 1; i < k; i++) {
            current = forward ? current->next : current->prev;
        }

        Node* to_remove = current;
        printf("%d ", to_remove->value);

        to_remove->prev->next = to_remove->next;
        to_remove->next->prev = to_remove->prev;

        current = forward ? to_remove->next : to_remove->prev;
        free(to_remove);
    }

    int last_remaining = current->value;
    free(current);

    printf("\nLast remaining: %d\n", last_remaining);
    return last_remaining;
}

int is_natural_number(const char* str) {
    if (!str || *str == '\0') return 0;

    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) return 0;
    }

    int num = atoi(str);
    return num > 0;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Usage: %s <N> <k> <-f/-b>\n", argv[0]);
        printf("N, k - natural numbers, k < N\n");
        printf("Flags: -f for forward, -b for backward\n");
        return 1;
    }

    if (!is_natural_number(argv[1]) || !is_natural_number(argv[2])) {
        fprintf(stderr, "Error: N and k must be natural numbers\n");
        return 1;
    }

    int N = atoi(argv[1]);
    int k = atoi(argv[2]);
    const char* flag = argv[3];

    if (k >= N) {
        fprintf(stderr, "Error: k must be less than N\n");
        return 1;
    }

    int forward;
    if (strcmp(flag, "-f") == 0) {
        forward = 1;
    }
    else if (strcmp(flag, "-b") == 0) {
        forward = 0;
    }
    else {
        fprintf(stderr, "Error: Invalid flag. Use -f or -b\n");
        return 1;
    }

    printf("Josephus problem: N=%d, k=%d, direction=%s\n",
        N, k, forward ? "forward" : "backward");

    int last_remaining = josephus_simulation(N, k, forward);

    if (last_remaining != -1) {
        long long prime_product = calculate_prime_product(last_remaining);
        printf("Product of primes less than %d: %lld\n", last_remaining, prime_product);
    }

    return 0;
}
