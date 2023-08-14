# ecs
A collection of various entity component system implementations

## Archetype
```c
#define ECS_IMPL
#include "ecs.h"
#include <stdio.h>

typedef struct {
    float x, y;
} pos_t, vel_t;

void pos_system(ecs_view_t *view) {
    for (size_t i = 0; i < view->count; i++)
        printf("e: %llu, p: {%d, %d}\n", view->entities[i], p[i].x, p[i].y);
    printf("\n");
}

void vel_system(ecs_view_t *view) {
    for (size_t i = 0; i < view->count; i++)
        printf("e: %llu, v: {%d, %d}\n", view->entities[i], v[i].x, v[i].y);
    printf("\n");
}

void posvel_system(ecs_view_t *view) {
    for (size_t i = 0; i < view->count; i++)
        printf("e: %llu, p: {%d, %d}, v: {%d, %d}\n", view->entities[i], p[i].x, p[i].y, v[i].x, v[i].y);
    printf("\n");
}

int main(void) {
    ecs_t *ecs = ecs_create(4);
    ecs_id_t const pos = ecs_register(ecs, pos_system, pos_t);
    ecs_id_t const vel = ecs_register(ecs, vel_system, vel_t);
    ecs_id_t const posvel = ecs_register(ecs, posvel_system, pos_t, vel_t);

    ecs_id_t e0 = ecs_spawn(ecs);
    ecs_id_t e1 = ecs_spawn(ecs);
    ecs_id_t e2 = ecs_spawn(ecs);

    ecs_set(ecs, e0, pos_t, {0, 0});
    ecs_set(ecs, e1, vel_t, {1, 1});
    ecs_set(ecs, e2, pos_t, {2, 2});
    ecs_set(ecs, e2, vel_t, {2, 2});

    ecs_run(ecs, pos);
    ecs_run(ecs, vel);
    ecs_run(ecs, posvel);

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

## Sparse Set
```c
#define ECS_IMPL
#include "ecs/sparse_set.h"
#include <stdio.h>

typedef struct { int x, y; } ivec2;

enum { POSITION, VELOCITY };

int main(void) {
    ecs_t *ecs = ecs_create(2, sizeof (ivec2), sizeof (ivec2));

    ecs_id_t e0 = ecs_spawn(ecs);
    ecs_id_t e1 = ecs_spawn(ecs);
    ecs_id_t e2 = ecs_spawn(ecs);

    ecs_set(ecs, e0, POSITION, &(ivec2){0, 0});
    ecs_set(ecs, e1, VELOCITY, &(ivec2){1, 1});
    ecs_set(ecs, e2, POSITION, &(ivec2){2, 2});
    ecs_set(ecs, e2, VELOCITY, &(ivec2){2, 2});

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

## License
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
