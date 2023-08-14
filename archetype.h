///////////////////////////////////////////////////////////////////////////////
///                                                                         ///
///                                Interface                                ///
///                                                                         ///
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <stddef.h>
#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////
/// Types

typedef uint64_t ecs_id_t;
typedef struct ecs_t ecs_t;

///////////////////////////////////////////////////////////////////////////////
/// Functions

ecs_t      *ecs_create                      (int entity_count_hint);
void        ecs_delete                      (ecs_t *ecs);

#define     ecs_register(ecs, fn, ...)      _ecs_register((ecs), (fn), #__VA_ARGS__)
ecs_id_t   _ecs_register                    (ecs_t *ecs, void (*fn)(void *, ecs_id_t *, size_t), char const *components);

ecs_id_t    ecs_spawn                       (ecs_t *ecs);
void        ecs_despawn                     (ecs_t *ecs, ecs_id_t entity_id);

#define     ecs_set(ecs, entity_id, T, ...) _ecs_set((ecs), (entity_id), #T, sizeof (T), &(T)__VA_ARGS__)
#define     ecs_get(ecs, entity_id, T)      _ecs_get((ecs), (entity_id), #T)
#define     ecs_rem(ecs, entity_id, T)      _ecs_rem((ecs), (entity_id), #T)
void       _ecs_set                         (ecs_t *ecs, ecs_id_t entity_id, char const *component_name, size_t component_stride, void const *data);
void      *_ecs_get                         (ecs_t *ecs, ecs_id_t entity_id, char const *component_name);
void       _ecs_rem                         (ecs_t *ecs, ecs_id_t entity_id, char const *component_name);

void        ecs_run                         (ecs_t *ecs, ecs_id_t system_id);
#define     ecs_field(components, T)        _ecs_field((components), #T)
void      *_ecs_field                       (void const *components, char const *component_name);

///////////////////////////////////////////////////////////////////////////////
///                                                                         ///
///                             Implementation                              ///
///                                                                         ///
///////////////////////////////////////////////////////////////////////////////

#if defined(ECS_IMPL)

#include <stdlib.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////
/// Types

typedef struct {
    uint64_t key;
    uint8_t dist;
} _ecs_map_slot_t;

typedef struct {
    char *data;
    size_t cap;
    size_t len;
    size_t stride;
} _ecs_arr_t, _ecs_map_t;

typedef struct {
    _ecs_map_t edges;
    _ecs_map_t components;
    _ecs_arr_t entities;
    uint64_t id;
} _ecs_archetype_t;

typedef struct {
    uint64_t archetype_id;
    size_t row;
} _ecs_entity_t;

struct ecs_t {
    _ecs_map_t entities;
    _ecs_map_t systems;
    _ecs_map_t archetypes;
    _ecs_arr_t ids;
    uint32_t next_idx;
    uint64_t root_archetype_id;
};

///////////////////////////////////////////////////////////////////////////////
/// Hash

static uint64_t _ecs_str_hash(char const *str, uint32_t seed) {
#define	_ecs_rotl32(x, r) ((x << r) | (x >> (32 - r)))
#define _ecs_fmix32(h) h^=h>>16; h*=0x85ebca6b; h^=h>>13; h*=0xc2b2ae35; h^=h>>16;
    const uint8_t * data = (const uint8_t*)str;
    const size_t len = strlen(str);
    const int nblocks = len / 16;
    uint32_t h1 = seed;
    uint32_t h2 = seed;
    uint32_t h3 = seed;
    uint32_t h4 = seed;
    uint32_t c1 = 0x239b961b;
    uint32_t c2 = 0xab0e9789;
    uint32_t c3 = 0x38b34ae5;
    uint32_t c4 = 0xa1e38b93;
    const uint32_t * blocks = (const uint32_t *)(data + nblocks*16);
    for (int i = -nblocks; i; i++) {
        uint32_t k1 = blocks[i*4+0];
        uint32_t k2 = blocks[i*4+1];
        uint32_t k3 = blocks[i*4+2];
        uint32_t k4 = blocks[i*4+3];
        k1 *= c1; k1  = _ecs_rotl32(k1,15); k1 *= c2; h1 ^= k1;
        h1 = _ecs_rotl32(h1,19); h1 += h2; h1 = h1*5+0x561ccd1b;
        k2 *= c2; k2  = _ecs_rotl32(k2,16); k2 *= c3; h2 ^= k2;
        h2 = _ecs_rotl32(h2,17); h2 += h3; h2 = h2*5+0x0bcaa747;
        k3 *= c3; k3  = _ecs_rotl32(k3,17); k3 *= c4; h3 ^= k3;
        h3 = _ecs_rotl32(h3,15); h3 += h4; h3 = h3*5+0x96cd1c35;
        k4 *= c4; k4  = _ecs_rotl32(k4,18); k4 *= c1; h4 ^= k4;
        h4 = _ecs_rotl32(h4,13); h4 += h1; h4 = h4*5+0x32ac3b17;
    }
    const uint8_t * tail = (const uint8_t*)(data + nblocks*16);
    uint32_t k1 = 0;
    uint32_t k2 = 0;
    uint32_t k3 = 0;
    uint32_t k4 = 0;
    switch(len & 15) {
    case 15: k4 ^= tail[14] << 16; /* fall through */
    case 14: k4 ^= tail[13] << 8; /* fall through */
    case 13: k4 ^= tail[12] << 0; k4 *= c4; k4  = _ecs_rotl32(k4,18); k4 *= c1; h4 ^= k4; /* fall through */
    case 12: k3 ^= tail[11] << 24; /* fall through */
    case 11: k3 ^= tail[10] << 16; /* fall through */
    case 10: k3 ^= tail[ 9] << 8; /* fall through */
    case  9: k3 ^= tail[ 8] << 0; k3 *= c3; k3  = _ecs_rotl32(k3,17); k3 *= c4; h3 ^= k3; /* fall through */
    case  8: k2 ^= tail[ 7] << 24; /* fall through */
    case  7: k2 ^= tail[ 6] << 16; /* fall through */
    case  6: k2 ^= tail[ 5] << 8; /* fall through */
    case  5: k2 ^= tail[ 4] << 0; k2 *= c2; k2  = _ecs_rotl32(k2,16); k2 *= c3; h2 ^= k2; /* fall through */
    case  4: k1 ^= tail[ 3] << 24; /* fall through */
    case  3: k1 ^= tail[ 2] << 16; /* fall through */
    case  2: k1 ^= tail[ 1] << 8; /* fall through */
    case  1: k1 ^= tail[ 0] << 0; k1 *= c1; k1  = _ecs_rotl32(k1,15); k1 *= c2; h1 ^= k1; /* fall through */
    };
    h1 ^= len; h2 ^= len; h3 ^= len; h4 ^= len;
    h1 += h2; h1 += h3; h1 += h4;
    h2 += h1; h3 += h1; h4 += h1;
    _ecs_fmix32(h1); _ecs_fmix32(h2); _ecs_fmix32(h3); _ecs_fmix32(h4);
    h1 += h2; h1 += h3; h1 += h4;
    h2 += h1; h3 += h1; h4 += h1;
    return (((uint64_t)h2)<<32)|h1;
    #undef _ecs_rotl32
    #undef _ecs_fmix32
}

static uint64_t _ecs_u64_hash(uint64_t key) {
    #define _ecs_xorshift(x, i) ((x) ^ ((x) >> (i)))
    static uint64_t const P = 0x5555555555555555ULL;
    static uint64_t const C = 17316035218449499591ULL;
    return C * _ecs_xorshift(P * _ecs_xorshift(key, 32), 32);
    #undef _ecs_xorshift
}

///////////////////////////////////////////////////////////////////////////////
/// Array

#define _ecs_arr_foreach(x, m, ...) do {\
    for (size_t _i##__LINE__ = 0; _i##__LINE__ < (m).len; _i##__LINE__++) {\
        x = (void *)&(m).data[_i##__LINE__ * (m).stride];\
        __VA_ARGS__;\
    }\
} while (0)

static _ecs_arr_t _ecs_arr_make(size_t stride, size_t cap) {
    if (cap < 8) cap = 8;
    return (_ecs_arr_t){
        .data = calloc(stride, cap),
        .cap = cap,
        .stride = stride,
    };
}

static void _ecs_arr_reserve(_ecs_arr_t *a, size_t cap) {
    if (cap < a->cap) return;

    while (a->cap <= cap)
        a->cap *= 1.5f;

    a->data = realloc(a->data, a->stride * a->cap);
}

static void _ecs_arr_set(_ecs_arr_t *a, size_t i, void const *val) {
    if (val)
        memcpy(&a->data[i * a->stride], val, a->stride);
}

static void *_ecs_arr_get(_ecs_arr_t const *a, size_t i) {
    return &a->data[i * a->stride];
}

#define _ecs_arr_get_as(a, i, T)\
    (*(T *)_ecs_arr_get((a), (i)))

static size_t _ecs_arr_push(_ecs_arr_t *a, void const *val) {
    if (!val) return SIZE_MAX;

    _ecs_arr_reserve(a, a->len + 1);
    _ecs_arr_set(a, a->len, val);
    return a->len++;
}

static void *_ecs_arr_pop(_ecs_arr_t *a) {
    return a->len ? &a->data[--a->len * a->stride] : NULL;
}

#define _ecs_arr_pop_as(a, T)\
    (*(T *)_ecs_arr_pop((a)))

static void _ecs_arr_free(_ecs_arr_t *a) {
    free(a->data);
    a->data = NULL;
    a->cap = a->len = 0;
}

#define _ecs_map_foreach(k, v, m, ...) do {\
    for (size_t _i##__LINE__ = 0; _i##__LINE__ < (m).cap; _i##__LINE__++) {\
        _ecs_map_slot_t *slot = _ecs_arr_get(&(m), _i##__LINE__);\
        if (slot->dist == 0) continue;\
        k = slot->key;\
        v = (void *)(slot + 1);\
        __VA_ARGS__;\
    }\
} while (0)

#define _ecs_map_foreachv(v, m, ...)\
    _ecs_map_foreach(uint64_t _k##__LINE__, v, m, __VA_ARGS__)

static _ecs_map_t _ecs_map_make(size_t stride, size_t cap) {
    size_t ncap = 8;
    while (ncap < cap)
        ncap *= 2;

    stride += sizeof (_ecs_map_slot_t);
    _ecs_map_t m = _ecs_arr_make(stride, ncap + 2);
    m.cap -= 2;

    return m;
}

static void _ecs_map_swap(_ecs_map_t *m, _ecs_map_slot_t *a, _ecs_map_slot_t *b) {
    void *t = _ecs_arr_get(m, m->cap + 1);
    memcpy(t, a, m->stride);
    memcpy(a, b, m->stride);
    memcpy(b, t, m->stride);
}

static void _ecs_map_resize(_ecs_map_t *m, size_t cap) {
    _ecs_map_t m2 = _ecs_map_make(m->stride - sizeof (_ecs_map_slot_t), cap);

    for (size_t i = 0; i < m->cap; i++) {
        _ecs_map_slot_t *item = _ecs_arr_get(m, i);
        if (item->dist == 0) continue;

        for (size_t j = item->key & (m2.cap - 1); ; item->dist++, j = (j + 1) & (m2.cap - 1)) {
            _ecs_map_slot_t *slot = _ecs_arr_get(&m2, j);
            if (slot->dist == 0) {
                m2.len++;
                memcpy(slot, item, m2.stride);
                break;
            }
            if (slot->dist < item->dist)
                _ecs_map_swap(m, slot, item);
        }
    }

    free(m->data);
    *m = m2;
}

static void _ecs_map_set(_ecs_map_t *m, uint64_t key, void const *val) {
    if (m->len * 4 == m->cap * 3)
        _ecs_map_resize(m, m->cap * 2);

    _ecs_map_slot_t *item = _ecs_arr_get(m, m->cap);
    item->key = key;
    item->dist = 1;
    memcpy(item + 1, val, m->stride - sizeof (_ecs_map_slot_t));

    for (size_t i = item->key & (m->cap - 1); ; item->dist++, i = (i + 1) & (m->cap - 1)) {
        _ecs_map_slot_t *slot = _ecs_arr_get(m, i);
        if (slot->dist == 0 || slot->key == item->key) {
            m->len += (slot->dist == 0);
            memcpy(slot, item, m->stride);
            break;
        }
        if (slot->dist < item->dist)
            _ecs_map_swap(m, slot, item);
    }
}

static void *_ecs_map_get(_ecs_map_t const *m, uint64_t key) {
    for (size_t i = key & (m->cap - 1); ; i = (i + 1) & (m->cap - 1)) {
        _ecs_map_slot_t *slot = _ecs_arr_get(m, i);
        if (slot->dist == 0)
            return NULL;
        if (slot->key == key)
            return slot + 1;
    }
}

#define _ecs_map_get_as(m, key, T)\
    (*(T *)_ecs_map_get((m), (key)))

static void _ecs_map_rem(_ecs_map_t *m, uint64_t key) {
    for (size_t i = key & (m->cap - 1); ; i = (i + 1) & (m->cap - 1)) {
        _ecs_map_slot_t *slot = _ecs_arr_get(m, i);
        if (slot->dist == 0)
            break;
        if (slot->key == key) {
            slot->dist = 0;
            for (;;) {
                _ecs_map_slot_t *curr = slot;
                i = (i + 1) & (m->cap - 1);
                slot = _ecs_arr_get(m, i);
                if (slot->dist <= 1) {
                    curr->dist = 0;
                    break;
                }
                memcpy(curr, slot, m->stride);
                curr->dist--;
            }
            m->len--;
            if (m->len * 10 == m->cap)
                _ecs_map_resize(m, m->cap / 2);
            break;
        }
    }
}

static void _ecs_map_free(_ecs_map_t *m) {
    _ecs_arr_free(m);
}

///////////////////////////////////////////////////////////////////////////////
/// Archetype

static _ecs_archetype_t _ecs_archetype_make(uint64_t id) {
    return (_ecs_archetype_t){
        .components = _ecs_map_make(sizeof (_ecs_arr_t), 0),
        .entities   = _ecs_arr_make(sizeof (ecs_id_t), 0),
        .edges      = _ecs_map_make(sizeof (uint64_t), 0),
        .id         = id
    };
}

// Initializes `curr` with the correct component arrays.
// Works going forward (e.g. [A, B] -> [A, B, C]) and backward (e.g. [A, B, C] -> [A, B])
static void _ecs_archetype_qualify(_ecs_archetype_t *curr, _ecs_archetype_t *next, uint64_t component_id, size_t component_stride, int set) {
    _ecs_map_foreach(uint64_t key, uint64_t *val, curr->edges, {
        _ecs_map_set(&next->edges, key, &(uint64_t){next->id ^ key});

        _ecs_arr_t *curr_arr = _ecs_map_get(&curr->components, key);
        if (!curr_arr || (!set && key == component_id)) continue;

        _ecs_arr_t next_arr = _ecs_arr_make(curr_arr->stride, 0);
        _ecs_map_set(&next->components, key, &next_arr);
    });

    if (set) {
        _ecs_map_set(&curr->edges, component_id, &next->id);
        _ecs_map_set(&next->edges, component_id, &curr->id);

        _ecs_arr_t arr = _ecs_arr_make(component_stride, 0);
        _ecs_map_set(&next->components, component_id, &arr);
    }
}

// Gets or creates a new archetype by either combining or removing `curr_archetype_id` and `component_id`
static uint64_t _ecs_archetype_obtain(uint64_t curr_archetype_id, uint64_t component_id, size_t component_stride, _ecs_map_t *archetypes, int set) {
    // This will find the next archetype when adding or removing a component
    //  - If adding a component, curr_archetype_id will not have component_id "in" it yet, so they will combine
    //  - If removing a component, curr_archetype_id will have component_id "in" it, so it will "take" component_id out
    // Since XOR is transitive, no archetype will be duplicated even if the same components were added in different orders
    uint64_t next_archetype_id = curr_archetype_id ^ component_id;

    _ecs_archetype_t *get = _ecs_map_get(archetypes, next_archetype_id);
    if (get) return get->id;

    _ecs_archetype_t *curr = _ecs_map_get(archetypes, curr_archetype_id);
    _ecs_archetype_t next = _ecs_archetype_make(next_archetype_id);

    _ecs_archetype_qualify(curr, &next, component_id, component_stride, set);

    _ecs_map_set(archetypes, next.id, &next);
    return next.id;
}

// Moves an entity and its components between archetypes
static size_t _ecs_archetype_transfer(uint64_t curr_archetype_id, uint64_t next_archetype_id, size_t curr_row, _ecs_map_t *archetypes) {
    _ecs_archetype_t *curr = _ecs_map_get(archetypes, curr_archetype_id);
    _ecs_archetype_t *next = _ecs_map_get(archetypes, next_archetype_id);

    // Swap and pop the entity
    size_t next_row = _ecs_arr_push(&next->entities, _ecs_arr_get(&curr->entities, curr_row));
    _ecs_arr_set(&curr->entities, curr_row, _ecs_arr_pop(&curr->entities));

    // Swap and pop the entity's components, taking care if next has fewer component types than curr
    _ecs_map_foreach(uint64_t key, _ecs_arr_t *curr_arr, curr->components, {
        _ecs_arr_t *next_arr = _ecs_map_get(&next->components, key);
        if (next_arr) {
            _ecs_arr_reserve(next_arr, next_row);
            _ecs_arr_set(next_arr, next_row, _ecs_arr_get(curr_arr, curr_row));
            next_arr->len++;
        }
        _ecs_arr_set(curr_arr, curr_row, _ecs_arr_pop(curr_arr));
    });

    return next_row;
}

static void _ecs_archetype_free(_ecs_archetype_t *archetype) {
    _ecs_map_foreachv(_ecs_arr_t *arr, archetype->components, {
        _ecs_arr_free(arr);
    });

    _ecs_map_free(&archetype->components);
    _ecs_arr_free(&archetype->entities);
    _ecs_map_free(&archetype->edges);
    *archetype = (_ecs_archetype_t){0};
}

#define _ecs_id_idx(x)          ((x) & 0xffffffff)
#define _ecs_id_ver(x)          (((x) >> 32) & 0xffffffff)
#define _ecs_id_make(ver, idx)  ((((ecs_id_t)(idx)) << 32) | ((uint32_t)(ver)))

///////////////////////////////////////////////////////////////////////////////
/// ECS

ecs_t *ecs_create(int entity_count_hint) {
    ecs_t *ecs = malloc(sizeof *ecs);
    if (!ecs) return NULL;

    ecs->archetypes         = _ecs_map_make(sizeof (_ecs_archetype_t), 0);
    ecs->systems            = _ecs_map_make(sizeof (void (*)()), 0);
    ecs->entities           = _ecs_map_make(sizeof (_ecs_entity_t), entity_count_hint);
    ecs->ids                = _ecs_arr_make(sizeof (ecs_id_t), entity_count_hint);
    ecs->next_idx           = UINT32_MAX;
    ecs->root_archetype_id  = 0;

    _ecs_archetype_t root = _ecs_archetype_make(ecs->root_archetype_id);
    _ecs_map_set(&ecs->archetypes, ecs->root_archetype_id, &root);

    return ecs;
}

void ecs_delete(ecs_t *ecs) {
    _ecs_map_foreachv(_ecs_archetype_t *archetype, ecs->archetypes, {
        _ecs_archetype_free(archetype);
    });

    _ecs_map_free(&ecs->archetypes);
    _ecs_map_free(&ecs->systems);
    _ecs_map_free(&ecs->entities);
    _ecs_arr_free(&ecs->ids);
    free(ecs);
}

ecs_id_t _ecs_register(ecs_t *ecs, void (*fn)(void *, ecs_id_t *, size_t), char const *components) {
    char *dup = strdup(components);
    ecs_id_t system_id = ecs->root_archetype_id;
    for (char const *tok = strtok(dup, ", "); tok; tok = strtok(NULL, ", "))
        system_id ^= _ecs_str_hash(tok, 0); // Combine the component hashes so system_id would be equal to the corresponding archetype_id
    free(dup);

    _ecs_map_set(&ecs->systems, system_id, &fn);
    return system_id;
}

ecs_id_t ecs_spawn(ecs_t *ecs) {
    ecs_id_t entity_id = 0;

    // Creates or recycles an entity. See https://skypjack.github.io/2019-05-06-ecs-baf-part-3/
    if (ecs->next_idx < UINT32_MAX) {
        ecs_id_t tmp = _ecs_arr_get_as(&ecs->ids, ecs->next_idx, ecs_id_t);
        uint32_t idx = ecs->next_idx;
        ecs->next_idx = _ecs_id_idx(tmp);
        entity_id = _ecs_id_make(_ecs_id_ver(tmp), idx);
        _ecs_arr_set(&ecs->ids, idx, &entity_id);
    } else {
        entity_id = _ecs_arr_push(&ecs->ids, &ecs->ids.len);
    }

    _ecs_archetype_t *root = _ecs_map_get(&ecs->archetypes, ecs->root_archetype_id);
    size_t row = _ecs_arr_push(&root->entities, &entity_id);
    _ecs_map_set(&ecs->entities, _ecs_u64_hash(entity_id), &(_ecs_entity_t){.archetype_id = ecs->root_archetype_id, .row = row});

    return entity_id;
}

void ecs_despawn(ecs_t *ecs, ecs_id_t entity_id) {
    uint64_t hash = _ecs_u64_hash(entity_id);

    _ecs_entity_t *entity = _ecs_map_get(&ecs->entities, hash);
    if (!entity) return;

    // Transfer the entity to the root component to remove the components then pop the most recently added entity from root (which will be this entity)
    _ecs_archetype_transfer(entity->archetype_id, ecs->root_archetype_id, entity->row, &ecs->archetypes);
    _ecs_archetype_t *root = _ecs_map_get(&ecs->archetypes, ecs->root_archetype_id);
    _ecs_arr_pop(&root->entities);

    _ecs_map_rem(&ecs->entities, hash);

    // Increment this index's version
    _ecs_arr_set(&ecs->ids, _ecs_id_idx(entity_id), &(ecs_id_t){_ecs_id_make(_ecs_id_ver(entity_id) + 1, _ecs_id_idx(entity_id))});
    ecs->next_idx = _ecs_id_idx(entity_id);
}

void _ecs_set(ecs_t *ecs, ecs_id_t entity_id, char const *component_name, size_t component_stride, void const *data) {
    _ecs_entity_t *entity = _ecs_map_get(&ecs->entities, _ecs_u64_hash(entity_id));
    if (!entity) return;

    uint64_t component_id = _ecs_str_hash(component_name, 0);
    uint64_t curr_id = entity->archetype_id;

    entity->archetype_id = _ecs_archetype_obtain(curr_id, component_id, component_stride, &ecs->archetypes, 1);
    entity->row = _ecs_archetype_transfer(curr_id, entity->archetype_id, entity->row, &ecs->archetypes);

    _ecs_archetype_t *archetype = _ecs_map_get(&ecs->archetypes, entity->archetype_id);
    _ecs_arr_t *arr = _ecs_map_get(&archetype->components, component_id);

    _ecs_arr_reserve(arr, entity->row);
    _ecs_arr_set(arr, entity->row, data);
    arr->len++;
}

void *_ecs_get(ecs_t *ecs, ecs_id_t entity_id, char const *component_name) {
    _ecs_entity_t *entity = _ecs_map_get(&ecs->entities, _ecs_u64_hash(entity_id));
    if (!entity) return NULL;

    uint64_t component_id = _ecs_str_hash(component_name, 0);
    _ecs_archetype_t *archetype = _ecs_map_get(&ecs->archetypes, entity->archetype_id);
    _ecs_arr_t *arr = _ecs_map_get(&archetype->components, component_id);
    return _ecs_arr_get(arr, entity->row);
}

void _ecs_rem(ecs_t *ecs, ecs_id_t entity_id, char const *component_name) {
    _ecs_entity_t *entity = _ecs_map_get(&ecs->entities, _ecs_u64_hash(entity_id));
    if (!entity) return;

    uint64_t component_id = _ecs_str_hash(component_name, 0);
    uint64_t curr_id = entity->archetype_id;

    entity->archetype_id = _ecs_archetype_obtain(curr_id, component_id, 0, &ecs->archetypes, 0);
    entity->row = _ecs_archetype_transfer(curr_id, entity->archetype_id, entity->row, &ecs->archetypes);
}

static void _ecs_run(ecs_t *ecs, _ecs_archetype_t *archetype, void (*fn)(void *, ecs_id_t *, size_t), int num_component_types) {
    if (!archetype || archetype->components.len < num_component_types) return;

    if (archetype->entities.len)
        fn(&archetype->components, (ecs_id_t *)archetype->entities.data, archetype->entities.len);

    _ecs_map_foreachv(uint64_t *edge, archetype->edges, {
        _ecs_run(ecs, _ecs_map_get(&ecs->archetypes, *edge), fn, archetype->components.len);
    });
}

void ecs_run(ecs_t *ecs, ecs_id_t system_id) {
    void (**fn)(void *, ecs_id_t *, size_t) = _ecs_map_get(&ecs->systems, system_id);
    if (!fn) return;

    _ecs_run(ecs, _ecs_map_get(&ecs->archetypes, system_id), *fn, 0);
}

void *_ecs_field(void const *components, char const *component_name) {
    _ecs_arr_t *arr = _ecs_map_get(components, _ecs_str_hash(component_name, 0));
    return arr ? arr->data : NULL;
}

#endif // ECS_IMPL
