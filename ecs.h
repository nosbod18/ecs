#pragma once
#include <stddef.h>
#include <stdint.h>

typedef uint64_t ecs_id_t;

void        ecs_init                (void);
ecs_id_t    ecs_create              (void);
#define     ecs_set(T, entity, ...) _ecs_set(#T, (entity), sizeof (T), (__VA_ARGS_))
#define     ecs_get(T, entity)      _ecs_get(#T, (entity))
#define     ecs_rem(T, entity)      _ecs_rem(#T, (entity))
void        ecs_delete              (ecs_id_t id);
void        ecs_quit                (void);

void      *_ecs_set                 (char const *component, ecs_id_t entity, size_t stride, void const *data);
void      *_ecs_get                 (char const *component, ecs_id_t entity);
void      *_ecs_rem                 (char const *component, ecs_id_t entity);
