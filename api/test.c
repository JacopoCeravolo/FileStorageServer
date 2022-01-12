#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char const *argv[])
{
    char *a1 = malloc(50);
    char *a2 = malloc(200);

    strcpy(a1, "Prima stringa");

    char *a3 = malloc(strlen(a1) + 1);
    memcpy(a3, a1, strlen(a1) + 1);

    printf("A1: %s : %lu\n", a1, strlen(a1) + 1);
    printf("A3: %s : %lu\n", a3, strlen(a3) + 1);

    strcpy(a2, "Seconda string molto lunga più della prima lunga stringa non 200 bytes però");

    size_t old_s = strlen(a1) + 1;
    printf("Old size: %lu\n", old_s);
    size_t new_s = strlen(a1) + strlen(a2) + 2;
    a3 = realloc(a3, new_s);

    printf("A3: %s : %lu\n", a3, strlen(a3) + 1);

    memcpy(a3, a2, strlen(a2) + 1);

    printf("\nA1: %s : %lu\n", a1, strlen(a1) + 1);
    printf("A2: %s : %lu\n", a2, strlen(a2) + 1);
    printf("A3: %s : %lu\n", a3, strlen(a3) + 1);

    return 0;
}
