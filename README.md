# ecs

```c
#include <stdio.h>

typedef struct { int x, y; } ivec2;

enum { POSITION, VELOCITY };

int main(void) {
    ecs_t *ecs = ecs_create(2, sizeof (ivec2), sizeof (ivec2));

    ecs_id_t e1 = ecs_spawn(ecs);
    ecs_id_t e2 = ecs_spawn(ecs);
    ecs_id_t e3 = ecs_spawn(ecs);

    ecs_set(ecs, e1, POSITION, &(ivec2){0, 0});
    ecs_set(ecs, e2, VELOCITY, &(ivec2){1, 1});
    ecs_set(ecs, e3, POSITION, &(ivec2){2, 2});
    ecs_set(ecs, e3, VELOCITY, &(ivec2){2, 2});

    for (ecs_view_t view = ecs_query(ecs, 1, POSITION); ecs_valid(&view); ecs_next(&view)) {
        ivec2 *p = ecs_column(&view, POSITION);
        printf("e: %llu, p: {%d, %d}\n", view.entity, p->x, p->y);
    }
    printf("\n");

    for (ecs_view_t view = ecs_query(ecs, 1, VELOCITY); ecs_valid(&view); ecs_next(&view)) {
        ivec2 *v = ecs_column(&view, VELOCITY);
        printf("e: %llu, v: {%d, %d}\n", view.entity, v->x, v->y);
    }
    printf("\n");

    for (ecs_view_t view = ecs_query(ecs, 2, POSITION, VELOCITY); ecs_valid(&view); ecs_next(&view)) {
        ivec2 *p = ecs_column(&view, POSITION);
        ivec2 *v = ecs_column(&view, VELOCITY);
        printf("e: %llu, p: {%d, %d}, v: {%d, %d}\n", view.entity, p->x, p->y, v->x, v->y);
    }
    printf("\n");

    ecs_delete(ecs);
}

// Output:
//  e: 0, p: {0, 0}
//  e: 2, p: {2, 2}
//
//  e: 1, v: {1, 1}
//  e: 2, v: {2, 2}
//
//  e: 2, p: {2, 2}, v: {2, 2}
```