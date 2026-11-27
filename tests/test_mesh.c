/*
 * Copyright 2026 Aarav Ravindra Kharade
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "core/aph_arena.h"
#include "core/aph_log.h"
#include "core/aph_status.h"
#include "mesh/aph_mesh.h"
#include "tests/aph_test.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int aph_test_failures = 0;

static void silent_log(aph_log_level level, const char *module, const char *msg, void *user) {
    (void)level;
    (void)module;
    (void)msg;
    (void)user;
}

static void check_edge_exact(const aph_mesh_edge *e, uint32_t v0, uint32_t v1, uint32_t tri0,
                             uint32_t tri1, uint32_t opp0, uint32_t opp1) {
    APH_CHECK(e->v0 == v0);
    APH_CHECK(e->v1 == v1);
    APH_CHECK(e->tri0 == tri0);
    APH_CHECK(e->tri1 == tri1);
    APH_CHECK(e->opp0 == opp0);
    APH_CHECK(e->opp1 == opp1);
}

static void test_mesh_invalid_args_and_oom(void) {
    _Alignas(APH_ARENA_MAX_ALIGN) uint8_t buf[1024];
    aph_arena arena;
    APH_CHECK(aph_arena_init(&arena, buf, sizeof(buf)) == APH_OK);

    const uint32_t tri[3] = {0U, 1U, 2U};
    aph_mesh_edges out;
    out.edges = (aph_mesh_edge *)buf;
    out.edge_count = 77U;

    /* 1. NULL out pointer */
    APH_CHECK(aph_mesh_build_edges(tri, 1U, 3U, &arena, NULL) == APH_ERR_INVALID_ARGUMENT);

    /* 2. NULL or uninitialized arena */
    APH_CHECK(aph_mesh_build_edges(tri, 1U, 3U, NULL, &out) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(out.edges == NULL);
    APH_CHECK(out.edge_count == 0U);

    aph_arena uninit;
    memset(&uninit, 0, sizeof(uninit));
    APH_CHECK(aph_mesh_build_edges(tri, 1U, 3U, &uninit, &out) == APH_ERR_INVALID_ARGUMENT);

    /* 3. NULL indices / zero triangles */
    APH_CHECK(aph_mesh_build_edges(NULL, 1U, 3U, &arena, &out) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(aph_mesh_build_edges(tri, 0U, 3U, &arena, &out) == APH_ERR_INVALID_ARGUMENT);

    /* 4. Out-of-range vertex indices */
    const uint32_t oob_tri[3] = {0U, 1U, 3U};
    APH_CHECK(aph_mesh_build_edges(oob_tri, 1U, 3U, &arena, &out) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(aph_mesh_build_edges(tri, 1U, 0U, &arena, &out) == APH_ERR_INVALID_ARGUMENT);

    /* 5. Degenerate triangles (repeated vertex inside a triangle) */
    const uint32_t deg0[3] = {0U, 0U, 1U};
    const uint32_t deg1[3] = {0U, 1U, 0U};
    const uint32_t deg2[3] = {1U, 2U, 2U};
    APH_CHECK(aph_mesh_build_edges(deg0, 1U, 3U, &arena, &out) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(aph_mesh_build_edges(deg1, 1U, 3U, &arena, &out) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(aph_mesh_build_edges(deg2, 1U, 3U, &arena, &out) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(arena.used == 0U);

    /* 6. Duplicated triangles (same winding, flipped winding, rotated winding) */
    void *pre_ptr = NULL;
    APH_CHECK(aph_arena_alloc(&arena, 32U, 16U, &pre_ptr) == APH_OK);
    const size_t pre_mark = aph_arena_mark(&arena);

    const uint32_t dup_same[6] = {0U, 1U, 2U, 0U, 1U, 2U};
    const uint32_t dup_flip[6] = {0U, 1U, 2U, 2U, 1U, 0U};
    const uint32_t dup_rot[6] = {0U, 1U, 2U, 1U, 2U, 0U};
    APH_CHECK(aph_mesh_build_edges(dup_same, 2U, 3U, &arena, &out) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(arena.used == pre_mark);
    APH_CHECK(aph_mesh_build_edges(dup_flip, 2U, 3U, &arena, &out) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(arena.used == pre_mark);
    APH_CHECK(aph_mesh_build_edges(dup_rot, 2U, 3U, &arena, &out) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(arena.used == pre_mark);

    /* 7. Non-manifold edge shared by 3 triangles */
    const uint32_t three_tris[9] = {0U, 1U, 2U, 0U, 1U, 3U, 0U, 1U, 4U};
    APH_CHECK(aph_mesh_build_edges(three_tris, 3U, 5U, &arena, &out) == APH_ERR_UNSUPPORTED);
    APH_CHECK(arena.used == pre_mark);

    /* 8. Overflow tri_count check happens before reading indices or allocating */
    APH_CHECK(aph_mesh_build_edges(tri, UINT32_MAX / 3U + 1U, 3U, &arena, &out) ==
              APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(arena.used == pre_mark);
    APH_CHECK(aph_mesh_build_edges(tri, UINT32_MAX, 3U, &arena, &out) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(arena.used == pre_mark);

    /* 9. OOM rollback when arena is too small for 3 * tri_count records */
    _Alignas(APH_ARENA_MAX_ALIGN) uint8_t small_buf[128];
    aph_arena small_arena;
    APH_CHECK(aph_arena_init(&small_arena, small_buf, sizeof(small_buf)) == APH_OK);
    void *small_pre = NULL;
    APH_CHECK(aph_arena_alloc(&small_arena, 16U, 16U, &small_pre) == APH_OK);
    const size_t small_mark = aph_arena_mark(&small_arena);
    const uint32_t quad[6] = {0U, 1U, 2U, 0U, 2U, 3U};
    APH_CHECK(aph_mesh_build_edges(quad, 2U, 4U, &small_arena, &out) == APH_ERR_OUT_OF_MEMORY);
    APH_CHECK(small_arena.used == small_mark);
    APH_CHECK(out.edges == NULL);
    APH_CHECK(out.edge_count == 0U);

    printf("  [PASS] test_mesh_invalid_args_and_oom\n");
}

static void test_mesh_single_triangle(void) {
    _Alignas(APH_ARENA_MAX_ALIGN) uint8_t buf[1024];
    aph_arena arena;
    APH_CHECK(aph_arena_init(&arena, buf, sizeof(buf)) == APH_OK);

    const uint32_t tri[3] = {2U, 0U, 1U};
    aph_mesh_edges out;
    APH_CHECK(aph_mesh_build_edges(tri, 1U, 3U, &arena, &out) == APH_OK);
    APH_CHECK(out.edges != NULL);
    APH_CHECK(((uintptr_t)out.edges % APH_MESH_ARRAY_ALIGN) == 0U);
    APH_CHECK(out.edge_count == 3U);

    check_edge_exact(&out.edges[0], 0U, 1U, 0U, APH_MESH_INVALID_INDEX, 2U, APH_MESH_INVALID_INDEX);
    check_edge_exact(&out.edges[1], 0U, 2U, 0U, APH_MESH_INVALID_INDEX, 1U, APH_MESH_INVALID_INDEX);
    check_edge_exact(&out.edges[2], 1U, 2U, 0U, APH_MESH_INVALID_INDEX, 0U, APH_MESH_INVALID_INDEX);

    printf("  [PASS] test_mesh_single_triangle\n");
}

static void test_mesh_quad(void) {
    _Alignas(APH_ARENA_MAX_ALIGN) uint8_t buf[1024];
    aph_arena arena;
    APH_CHECK(aph_arena_init(&arena, buf, sizeof(buf)) == APH_OK);

    const uint32_t quad[6] = {0U, 1U, 2U, 0U, 2U, 3U};
    aph_mesh_edges out;
    APH_CHECK(aph_mesh_build_edges(quad, 2U, 4U, &arena, &out) == APH_OK);
    APH_CHECK(out.edge_count == 5U);

    check_edge_exact(&out.edges[0], 0U, 1U, 0U, APH_MESH_INVALID_INDEX, 2U, APH_MESH_INVALID_INDEX);
    check_edge_exact(&out.edges[1], 0U, 2U, 0U, 1U, 1U, 3U);
    check_edge_exact(&out.edges[2], 0U, 3U, 1U, APH_MESH_INVALID_INDEX, 2U, APH_MESH_INVALID_INDEX);
    check_edge_exact(&out.edges[3], 1U, 2U, 0U, APH_MESH_INVALID_INDEX, 0U, APH_MESH_INVALID_INDEX);
    check_edge_exact(&out.edges[4], 2U, 3U, 1U, APH_MESH_INVALID_INDEX, 0U, APH_MESH_INVALID_INDEX);

    printf("  [PASS] test_mesh_quad\n");
}

static uint32_t build_grid_indices(uint32_t rows, uint32_t cols, uint32_t *indices) {
    uint32_t t = 0U;
    for (uint32_t r = 0U; r < rows - 1U; ++r) {
        for (uint32_t c = 0U; c < cols - 1U; ++c) {
            const uint32_t v00 = r * cols + c;
            const uint32_t v01 = v00 + 1U;
            const uint32_t v10 = (r + 1U) * cols + c;
            const uint32_t v11 = v10 + 1U;

            indices[t * 3U + 0U] = v00;
            indices[t * 3U + 1U] = v10;
            indices[t * 3U + 2U] = v11;
            t++;

            indices[t * 3U + 0U] = v00;
            indices[t * 3U + 1U] = v11;
            indices[t * 3U + 2U] = v01;
            t++;
        }
    }
    return t;
}

static void test_mesh_grid_topology(void) {
    const uint32_t rows = 4U;
    const uint32_t cols = 5U;
    const uint32_t vertex_count = rows * cols;
    uint32_t indices[24 * 3];
    const uint32_t tri_count = build_grid_indices(rows, cols, indices);
    APH_CHECK(tri_count == 24U);

    _Alignas(APH_ARENA_MAX_ALIGN) uint8_t buf[4096];
    aph_arena arena;
    APH_CHECK(aph_arena_init(&arena, buf, sizeof(buf)) == APH_OK);

    aph_mesh_edges out;
    APH_CHECK(aph_mesh_build_edges(indices, tri_count, vertex_count, &arena, &out) == APH_OK);

    const uint32_t expected_edges =
        rows * (cols - 1U) + (rows - 1U) * cols + (rows - 1U) * (cols - 1U);
    const uint32_t expected_boundary = 2U * (rows - 1U) + 2U * (cols - 1U);
    APH_CHECK(expected_edges == 43U);
    APH_CHECK(expected_boundary == 14U);
    APH_CHECK(out.edge_count == expected_edges);

    uint32_t boundary_count = 0U;
    uint32_t interior_count = 0U;
    uint32_t tri_ref_counts[24];
    memset(tri_ref_counts, 0, sizeof(tri_ref_counts));

    for (uint32_t i = 0U; i < out.edge_count; ++i) {
        const aph_mesh_edge *e = &out.edges[i];
        APH_CHECK(e->v0 < e->v1);
        APH_CHECK(e->v1 < vertex_count);
        APH_CHECK(e->tri0 < tri_count);
        APH_CHECK(e->opp0 < vertex_count);
        APH_CHECK(e->opp0 != e->v0 && e->opp0 != e->v1);
        tri_ref_counts[e->tri0]++;

        if (i > 0U) {
            const aph_mesh_edge *prev = &out.edges[i - 1U];
            APH_CHECK(prev->v0 < e->v0 || (prev->v0 == e->v0 && prev->v1 < e->v1));
        }

        if (e->tri1 == APH_MESH_INVALID_INDEX) {
            APH_CHECK(e->opp1 == APH_MESH_INVALID_INDEX);
            boundary_count++;
        } else {
            APH_CHECK(e->tri1 < tri_count);
            APH_CHECK(e->tri0 < e->tri1);
            APH_CHECK(e->opp1 < vertex_count);
            APH_CHECK(e->opp1 != e->v0 && e->opp1 != e->v1);
            APH_CHECK(e->opp0 != e->opp1);
            tri_ref_counts[e->tri1]++;
            interior_count++;
        }
    }

    APH_CHECK(boundary_count == expected_boundary);
    APH_CHECK(interior_count == expected_edges - expected_boundary);
    APH_CHECK(boundary_count + 2U * interior_count == 3U * tri_count);

    for (uint32_t t = 0U; t < tri_count; ++t) {
        APH_CHECK(tri_ref_counts[t] == 3U);
    }

    printf("  [PASS] test_mesh_grid_topology\n");
}

static void test_mesh_order_and_winding_invariance(void) {
    uint32_t base_indices[24 * 3];
    const uint32_t tri_count = build_grid_indices(4U, 5U, base_indices);

    uint32_t perm_indices[24 * 3];
    for (uint32_t i = 0U; i < tri_count; ++i) {
        /* Deterministic bijective permutation via mod 24 stride 7 (gcd(7, 24) == 1) */
        const uint32_t src = (i * 7U + 5U) % tri_count;
        const uint32_t a = base_indices[src * 3U + 0U];
        const uint32_t b = base_indices[src * 3U + 1U];
        const uint32_t c = base_indices[src * 3U + 2U];

        if ((i % 2U) == 0U) {
            /* Rotate vertex order */
            perm_indices[i * 3U + 0U] = b;
            perm_indices[i * 3U + 1U] = c;
            perm_indices[i * 3U + 2U] = a;
        } else {
            /* Reverse winding */
            perm_indices[i * 3U + 0U] = c;
            perm_indices[i * 3U + 1U] = b;
            perm_indices[i * 3U + 2U] = a;
        }
    }

    _Alignas(APH_ARENA_MAX_ALIGN) uint8_t buf_a[4096];
    _Alignas(APH_ARENA_MAX_ALIGN) uint8_t buf_b[4096];
    aph_arena arena_a;
    aph_arena arena_b;
    APH_CHECK(aph_arena_init(&arena_a, buf_a, sizeof(buf_a)) == APH_OK);
    APH_CHECK(aph_arena_init(&arena_b, buf_b, sizeof(buf_b)) == APH_OK);

    aph_mesh_edges edges_a;
    aph_mesh_edges edges_b;
    APH_CHECK(aph_mesh_build_edges(base_indices, tri_count, 20U, &arena_a, &edges_a) == APH_OK);
    APH_CHECK(aph_mesh_build_edges(perm_indices, tri_count, 20U, &arena_b, &edges_b) == APH_OK);

    APH_CHECK(edges_a.edge_count == edges_b.edge_count);
    for (uint32_t i = 0U; i < edges_a.edge_count; ++i) {
        APH_CHECK(edges_a.edges[i].v0 == edges_b.edges[i].v0);
        APH_CHECK(edges_a.edges[i].v1 == edges_b.edges[i].v1);
        APH_CHECK((edges_a.edges[i].tri1 == APH_MESH_INVALID_INDEX) ==
                  (edges_b.edges[i].tri1 == APH_MESH_INVALID_INDEX));
    }

    printf("  [PASS] test_mesh_order_and_winding_invariance\n");
}

static void test_mesh_arena_trim_and_zero_fill(void) {
    _Alignas(APH_ARENA_MAX_ALIGN) uint8_t buf[1024];
    memset(buf, 0xAA, sizeof(buf));

    aph_arena arena;
    APH_CHECK(aph_arena_init(&arena, buf, sizeof(buf)) == APH_OK);

    /* Unaligned prefix so block_offset tests alignment accounting too */
    void *prefix = NULL;
    APH_CHECK(aph_arena_alloc(&arena, 7U, 1U, &prefix) == APH_OK);
    APH_CHECK(arena.used == 7U);

    const uint32_t quad[6] = {0U, 1U, 2U, 0U, 2U, 3U};
    aph_mesh_edges out;
    APH_CHECK(aph_mesh_build_edges(quad, 2U, 4U, &arena, &out) == APH_OK);
    APH_CHECK(out.edge_count == 5U);

    /* Aligned block_offset for 16-byte alignment after 7 bytes is 16 */
    const size_t block_offset = (size_t)((const uint8_t *)out.edges - arena.base);
    APH_CHECK(block_offset == 16U);
    APH_CHECK(arena.used == block_offset + (size_t)out.edge_count * sizeof(aph_mesh_edge));

    /* Trimmed 6th record tail was dirtied during edge extraction; next alloc must zero-fill it */
    void *next = NULL;
    APH_CHECK(aph_arena_alloc(&arena, sizeof(aph_mesh_edge), 1U, &next) == APH_OK);
    const uint8_t *bytes = (const uint8_t *)next;
    for (size_t i = 0U; i < sizeof(aph_mesh_edge); ++i) {
        APH_CHECK(bytes[i] == 0U);
    }

    printf("  [PASS] test_mesh_arena_trim_and_zero_fill\n");
}

int main(void) {
    aph_set_log_callback(silent_log, NULL);

    test_mesh_invalid_args_and_oom();

    aph_set_log_callback(NULL, NULL);

    test_mesh_single_triangle();
    test_mesh_quad();
    test_mesh_grid_topology();
    test_mesh_order_and_winding_invariance();
    test_mesh_arena_trim_and_zero_fill();

    if (aph_test_failures == 0) {
        printf("mesh_test: all checks passed.\n");
    } else {
        fprintf(stderr, "mesh_test: %d check(s) failed.\n", aph_test_failures);
    }

    return aph_test_failures == 0 ? 0 : 1;
}
