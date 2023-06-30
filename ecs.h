///////////////////////////////////////////////////////////////////////////////
///                                                                         ///
///                                Interface                                ///
///                                                                         ///
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <stddef.h>
#include <stdint.h>

typedef uint64_t ecs_id_t;

typedef struct ecs_t ecs_t;

typedef struct {
    void *pool, *pools[16];
    int index_to_com[16];
    int count;
    size_t entity_index;
    uint64_t entity;
} ecs_view_t;

ecs_t      *ecs_create  (int count, ...);
void        ecs_delete  (ecs_t *ecs);
void        ecs_set     (ecs_t *ecs, ecs_id_t entity, int component, void const *data);
void       *ecs_get     (ecs_t const *ecs, ecs_id_t entity, int component);
void        ecs_rem     (ecs_t *ecs, ecs_id_t entity, int component);

ecs_id_t    ecs_spawn   (ecs_t *ecs);
void        ecs_despawn (ecs_t *ecs, ecs_id_t entity);

ecs_view_t  ecs_query   (ecs_t *ecs, int count, ...);
int         ecs_valid   (ecs_view_t const *view);
void        ecs_next    (ecs_view_t *view);
void       *ecs_column  (ecs_view_t const *view, int component);

///////////////////////////////////////////////////////////////////////////////
///                                                                         ///
///                             Implementation                              ///
///                                                                         ///
///////////////////////////////////////////////////////////////////////////////

#if defined(ECS_IMPL)

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////
/// Buffer / Array

#define _ecs_buf_free(b)            ((b) ? free(_ecs_buf__raw(b)), 0 : 0)
#define _ecs_buf_reserve(b, n, sz)  (!(b) || _ecs_buf__len(b) + ((n) * (sz)) > _ecs_buf__cap(b) ? b = _ecs_buf_grow((b), _ecs_buf_len(b) + ((n) * (sz))), 0 : 0)
#define _ecs_buf_len(b)             ((b) ? _ecs_buf__len(b) : 0)
#define _ecs_buf_get(b, i, sz)      ((void *)&((char *)(b))[(i) * (sz)])
#define _ecs_buf_set(b, i, x, sz)   (memcpy(&((char *)(b))[(i) * (sz)], (x), (sz)))
#define _ecs_buf_addn(b, n, sz)     (_ecs_buf_reserve(b, n, sz), _ecs_buf__len(b) += (n) * (sz), &((char *)(b))[_ecs_buf__len(b) - ((n) * (sz))])
#define _ecs_buf_push(b, x, sz)     (memcpy(_ecs_buf_addn(b, 1, sz), (x), (sz)))
#define _ecs_buf_pop(b, sz)         ((void *)&((char *)(b))[_ecs_buf__len(b) -= (sz)])
#define _ecs_buf__raw(b)            ((size_t *)(b) - 2)
#define _ecs_buf__cap(b)            (_ecs_buf__raw(b)[0])
#define _ecs_buf__len(b)            (_ecs_buf__raw(b)[1])

static void *_ecs_buf_grow(void *b, size_t n) {
    size_t m = b ? _ecs_buf__len(b) : 16;
    while (m < n) m *= 2;

    size_t *c = realloc(b ? _ecs_buf__raw(b) : b, (sizeof *c * 2) + m);
    c += 2;

    if (!b) _ecs_buf__len(c) = 0;
    _ecs_buf__cap(c) = m;

    return c;
}

#define _ecs_arr_free(a)     (_ecs_buf_free(a))
#define _ecs_arr_len(a)      (_ecs_buf_len(a) / sizeof *(a))
#define _ecs_arr_addn(a, n)  (_ecs_buf_addn(a, n, sizeof *(a)))
#define _ecs_arr_push(a, x)  (_ecs_buf_push(a, x, sizeof *(a)))
#define _ecs_arr_pop(a)      ((a)[_ecs_buf__len(a) -= sizeof *(a)])

///////////////////////////////////////////////////////////////////////////////
/// Pool

typedef struct {
    size_t *sparse;
    ecs_id_t *dense;
    void *data;
    size_t stride;
} _ecs_pool_t;

#define _ecs_lo32(x)    ((x) & 0xffffffff)
#define _ecs_hi32(x)    (_ecs_lo32((x) >> 32))
#define _ecs_mk64(h, l) (((uint64_t)(h) << 32) | _ecs_lo32(l))

static _ecs_pool_t _ecs_pool_make(size_t stride) {
    return (_ecs_pool_t){.stride = stride};
}

static int _ecs_pool_has(_ecs_pool_t const *p, ecs_id_t e) {
    size_t idx = p->sparse[_ecs_lo32(e)];
    return idx < _ecs_arr_len(p->dense) && p->dense[idx] == e;
}

static void *_ecs_pool_get(_ecs_pool_t const *p, ecs_id_t e) {
    return _ecs_pool_has(p, e) ? _ecs_buf_get(p->data, p->sparse[_ecs_lo32(e)], p->stride) : NULL;
}

static void _ecs_pool_set(_ecs_pool_t *p, ecs_id_t e, void const *data) {
    _ecs_buf_reserve(p->sparse, _ecs_lo32(e), sizeof *p->sparse);
    p->sparse[_ecs_lo32(e)] = _ecs_arr_len(p->dense);
    _ecs_buf__len(p->sparse)++;

    _ecs_arr_push(p->dense, &e);
    _ecs_buf_push(p->data, data, p->stride);
}

static void _ecs_pool_rem(_ecs_pool_t *p, ecs_id_t e) {
    size_t pos = p->sparse[_ecs_lo32(e)];
    ecs_id_t rem = _ecs_arr_pop(p->dense);

    p->sparse[_ecs_lo32(rem)] = pos;
    _ecs_buf__len(p->sparse)--;

    p->dense[pos] = rem;
    _ecs_buf_set(p->data, pos, _ecs_buf_pop(p->data, p->stride), p->stride);
}

static void _ecs_pool_free(_ecs_pool_t *p) {
    _ecs_arr_free(p->sparse);
    _ecs_arr_free(p->dense);
    _ecs_buf_free(p->data);
}

///////////////////////////////////////////////////////////////////////////////
/// World

struct ecs_t {
    _ecs_pool_t *pools;
    ecs_id_t *entities;
    uint32_t next_idx;
};

ecs_t *ecs_create(int count, ...) {
    ecs_t *ecs = calloc(1, sizeof *ecs);
    if (!ecs) return NULL;

    ecs->next_idx = UINT32_MAX;

    va_list ap;
    va_start(ap, count);

    for (int i = 0; i < count; i++) {
        _ecs_pool_t p = _ecs_pool_make(va_arg(ap, size_t));
        _ecs_arr_push(ecs->pools, &p);
    }

    va_end(ap);
    return ecs;
}

void ecs_delete(ecs_t *ecs) {
    for (size_t i = 0; i < _ecs_arr_len(ecs->pools); i++)
        _ecs_pool_free(&ecs->pools[i]);

    _ecs_arr_free(ecs->pools);
    _ecs_arr_free(ecs->entities);
    free(ecs);
}

void ecs_set(ecs_t *ecs, ecs_id_t e, int c, void const *data) {
    _ecs_pool_set(&ecs->pools[c], e, data);
}

void *ecs_get(ecs_t const *ecs, ecs_id_t e, int c) {
    return _ecs_pool_get(&ecs->pools[c], e);
}

void ecs_rem(ecs_t *ecs, ecs_id_t e, int c) {
    _ecs_pool_rem(&ecs->pools[c], e);
}

///////////////////////////////////////////////////////////////////////////////
/// Entity

ecs_id_t ecs_spawn(ecs_t *ecs) {
    ecs_id_t e = 0;

    if (ecs->next_idx < UINT32_MAX) {
        ecs_id_t cur = ecs->entities[ecs->next_idx];
        uint32_t idx = ecs->next_idx;
        ecs->next_idx = _ecs_lo32(cur);
        e = _ecs_mk64(_ecs_hi32(cur), idx);
        ecs->entities[idx] = e;
    } else {
        e = _ecs_arr_len(ecs->entities);
        _ecs_arr_push(ecs->entities, &e);
    }

    return e;
}

void ecs_despawn(ecs_t *ecs, ecs_id_t e) {
    ecs->entities[_ecs_lo32(e)] = _ecs_mk64(_ecs_lo32(e) + 1, _ecs_lo32(e));
    ecs->next_idx = _ecs_lo32(e);
}

///////////////////////////////////////////////////////////////////////////////
/// View

static int _ecs_view_has(ecs_view_t const *v, ecs_id_t e) {
    for (int i = 0; i < v->count; i++)
        if (!_ecs_pool_has(v->pools[i], e))
            return 0;
    return 1;
}

ecs_view_t ecs_query(ecs_t *ecs, int count, ...) {
    ecs_view_t v = { .count = count };

    va_list ap;
    va_start(ap, count);

    for (int i = 0; i < count; i++) {
        int c = va_arg(ap, int);
        v.pools[i] = &ecs->pools[c];
        if (!v.pool || _ecs_arr_len(((_ecs_pool_t *)v.pools[i])->dense) < _ecs_arr_len(((_ecs_pool_t *)v.pool)->dense))
            v.pool = v.pools[i];
        v.index_to_com[i] = c;
    }

    va_end(ap);

    ecs_id_t *dense = ((_ecs_pool_t *)v.pool)->dense;
    if (v.pool && _ecs_arr_len(dense) > 0) {
        v.entity = dense[v.entity_index];
        if (!_ecs_view_has(&v, v.entity))
            ecs_next(&v);
    }

    return v;
}

int ecs_valid(ecs_view_t const *v) {
    return v->entity != UINT64_MAX;
}

void ecs_next(ecs_view_t *v) {
    ecs_id_t *dense = ((_ecs_pool_t *)v->pool)->dense;
    do {
        v->entity = (v->entity_index < _ecs_arr_len(dense)) ? dense[++v->entity_index] : UINT64_MAX;
    } while (v->entity != UINT64_MAX && !_ecs_view_has(v, v->entity));
}

void *ecs_column(ecs_view_t const *v, int c) {
    for (int i = 0; i < v->count; i++)
        if (v->index_to_com[i] == c)
            return _ecs_pool_get(v->pools[i], v->entity);
    return NULL;
}

#endif // ECS_IMPL