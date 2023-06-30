#pragma once
#include <stddef.h>
#include <stdint.h>

typedef uint64_t ecs_id_t;

typedef struct ecs_world_t ecs_world_t;

typedef struct {
    void *pool, *pools[16];
    int index_to_com[16];
    int count;
    size_t entity_index;
    uint64_t entity;
} ecs_view_t;

ecs_world_t    *ecs_new     (int component_count, ...);
ecs_id_t        ecs_spawn   (ecs_world_t *world);
void            ecs_set     (ecs_world_t *world, ecs_id_t entity, int component, void const *data);
void           *ecs_get     (ecs_world_t *world, ecs_id_t entity, int component);
void            ecs_rem     (ecs_world_t *world, ecs_id_t entity, int component);
void            ecs_despawn (ecs_world_t *world, ecs_id_t entity);
void            ecs_free    (ecs_world_t *world);
