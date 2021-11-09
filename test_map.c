#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "utilities.h"
#include "hash_map.h"



int main() {

    FILE *f;
    hash_map_t *hmap = hash_map_create(10, NULL, string_compare, NULL, print_string_int);

    f = fopen("map_log.txt", "w+");
    static char keys[11][32] = { "key1", "key2", "key3", "key4", "key5", "key6", "key7", "key8", "key9", "key10", "key11"};

    int value = 101;

    for (size_t i = 0; i < 11; i++)
    {
        if (hash_map_insert(hmap, (void*)keys[i], (void*)value) != 0) {
            printf("hash_map_insert failed: %s\n", strerror(errno));
        }
    }


    hash_map_dump(hmap, f);
    hash_map_destroy(hmap);
    fclose(f);

}