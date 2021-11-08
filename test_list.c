#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "list.h"

bool
cmp_int(void* x, void* y)
{
    int a = (int)x;
    int b = (int)y;
    return (a == b) ? true : false;
}

void
free_int(void* ptr)
{
    return;
}

void
print_int(void* x, FILE* f)
{
    int p = (int)x;
    fprintf(f, "%d ", p);
}

void
increment(void* x)
{
    int a = (int)x;
    a++;
}

int test_int()
{
    printf("***** TEST INT *****\n");
    FILE* f;
    list_t *l;
    int x, index;

    f = fopen("intlist_log.txt", "w+");

    printf("creating list\n");
    l =  list_create(cmp_int, free_int, print_int);

    printf("insert tail 4\n");
    x = 4;
    if (list_insert_tail(l, (void*)x) != 0) {
        printf("list_insert_tail failed: %s\n", strerror(errno));
    }

    printf("insert tail 5\n");
    x = 5;
    if (list_insert_tail(l, (void*)x) != 0) {
        printf("list_insert_tail failed: %s\n", strerror(errno));
    }

    printf("insert head 3\n");
    x = 3;
    if (list_insert_head(l, (void*)x) != 0) {
        printf("list_insert_head failed: %s\n", strerror(errno));
    }

    printf("insert head 2\n");
    x = 2;
    if (list_insert_head(l, (void*)x) != 0) {
        printf("list_insert_head failed: %s\n", strerror(errno));
    }

    printf("insert head 1\n");
    x = 1;
    if (list_insert_head(l, (void*)x) != 0) {
        printf("list_insert_head failed: %s\n", strerror(errno));
    }

    printf("remove head ");
    if (list_remove_head(l, (void*)&x) != 0) {
        printf("list_remove_head failed: %s\n", strerror(errno));
    } else {
        printf("%d\n", x);
    }

    printf("insert 1 at index 0\n");
    x = 1;
    index = 0;
    if (list_insert_at_index(l, (void*)x, index) != 0) {
        printf("list_insert_at_index failed: %s\n", strerror(errno));
    }

    printf("insert 0 at index 0\n");
    x = 0;
    index = 0;
    if (list_insert_at_index(l, (void*)x, index) != 0) {
        printf("list_insert_at_index failed: %s\n", strerror(errno));
    }

    printf("insert tail 7\n");
    x = 7;
    if (list_insert_tail(l, (void*)x) != 0) {
        printf("list_insert_tail failed: %s\n", strerror(errno));
    }
    
    printf("insert 6 at index 6\n");
    x = 6;
    index = 6;
    if (list_insert_at_index(l, (void*)x, index) != 0) {
        printf("list_insert_at_index failed: %s\n", strerror(errno));
    }

    printf("insert 6 at index 9, expecting error\n");
    x = 6;
    index = 9;
    if (list_insert_at_index(l, (void*)x, index) != 0) {
        printf("list_insert_at_index failed: %s\n", strerror(errno));
    }

    printf("getting index of 4: ");
    x = 4;
    index = list_index_of(l, (void*)x);
    if (index < 0) {
        printf("list_index_of failed: %s\n", strerror(errno));
    } else {
        printf("%d\n", index);
    }

    printf("getting index of 8, expecting error\n");
    x = 8;
    index = list_index_of(l, (void*)x);
    if (index < 0) {
        printf("list_index_of failed: %s\n", strerror(errno));
    } else {
        printf("%d\n", index);
    }


    printf("printing list\n");
    list_dump(l, f);

    printf("destroying list\n");
    if (list_destroy(l) != 0) {
        printf("list_destroy failed: %s\n", strerror(errno));
    }

    fclose(f);

    return 0;
}


typedef struct _obj {
    int id;
    char *s;
} obj;

bool
cmp_struct(void* x, void* y)
{
    obj *o1 = malloc(sizeof(obj));
    obj *o2 = malloc(sizeof(obj));
    o1 = *(obj**)x;
    o2 = *(obj**)y;
    if (o1->id == o2->id && strcmp(o1->s, o2->s) == 0) return true;
    else return false;

}

void
free_struct(void* x)
{
    obj *o = (obj*)x;
    if(o->s) free(o->s);
    //free(o);
}

void
print_struct(void* x, FILE* f)
{
    //obj *o = malloc(sizeof(obj));
    obj *o = (obj*)x;
    fprintf(f, "%d -> %s\n", (int)o->id, (char*)o->s);
    //free(o);
}

void
increment_struct(void* x)
{
    obj *o = (obj*)x;
    o->id++;
}

int test_struct()
{
    printf("***** TEST OBJECT *****\n");
    FILE* f;
    list_t *l1;
    int index;
    
    l1 = list_create(cmp_struct, free_struct, print_struct);
    f = fopen("objlist_log.txt", "w+");

    obj object0;
    obj object1;
    obj object2;
    obj object3;

    object0.id = 0;
    object1.id = 1;
    object2.id = 2;
    object3.id = 3;

    object0.s = malloc(20);
    object1.s = malloc(20);
    object2.s = malloc(20);
    object3.s = malloc(20);

    strcpy(object0.s, "Jacopo");
    strcpy(object1.s, "Ceravolo");
    strcpy(object2.s, "Via");
    strcpy(object3.s, "Berlinghieri");

    printf("insert tail object 0\n");
    if (list_insert_tail(l1, (void*)&object0) != 0) {
        printf("list_insert_tail failed: %s\n", strerror(errno));
    }

    printf("insert tail object 1\n");
    if (list_insert_tail(l1, (void*)&object1) != 0) {
        printf("list_insert_tail failed: %s\n", strerror(errno));
    }

    printf("insert tail object 2\n");
    if (list_insert_tail(l1, (void*)&object2) != 0) {
        printf("list_insert_tail failed: %s\n", strerror(errno));
    }

    printf("insert tail object 3\n");
    if (list_insert_tail(l1, (void*)&object3) != 0) {
        printf("list_insert_tail failed: %s\n", strerror(errno));
    }
    
    printf("mapping increment\n");
    list_map(l1, increment_struct);
    
    printf("printing list\n");
    list_dump(l1, f);

    printf("destroying list\n");
    if (list_destroy(l1) != 0) {
        printf("list_destroy failed: %s\n", strerror(errno));
    }

    fclose(f);

    return 0;
}

int main(int argc, char const *argv[])
{
    test_int();
    test_struct();
    return 0;
}
