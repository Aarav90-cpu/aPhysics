#!/usr/bin/env python3
# Copyright 2026 Aarav Ravindra Kharade
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Independent IEEE 754 float32 reference emulator for Step 2.1 cloth hashing."""

import struct


def f32(val: float) -> float:
    return struct.unpack("<f", struct.pack("<f", float(val)))[0]


def fnv1a64(data: bytes, state: int = 0xCBF29CE484222325) -> int:
    h = 0xCBF29CE484222325 if state == 0 else state
    prime = 0x100000001B3
    for b in data:
        h = ((h ^ b) * prime) & 0xFFFFFFFFFFFFFFFF
    return h


def pack_f32_list(vals: list[float]) -> bytes:
    return b"".join(struct.pack("<f", v) for v in vals)


def run_small_cloth_reference(steps: int) -> int:
    seed = 1337
    fixed_dt = f32(1.0 / 60.0)
    substeps = 8
    gx, gy, gz = f32(0.0), f32(-9.81), f32(0.0)

    # Small 3-vertex cloth: vertex 0 pinned, vertices 1 and 2 unpinned.
    cloth_id = 0
    vertex_count = 3
    mass = f32(1.5)
    damping = f32(2.0)
    thickness = f32(0.01)
    inv_mass = f32(f32(vertex_count) / mass)

    pos = [
        f32(0.0), f32(2.0), f32(0.0),
        f32(1.0), f32(2.0), f32(0.0),
        f32(0.5), f32(1.5), f32(0.25),
    ]
    prev = list(pos)
    vel = [f32(0.0)] * (vertex_count * 3)
    inv_masses = [f32(0.0), inv_mass, inv_mass]

    h_step = f32(fixed_dt / f32(substeps))
    damp_factor = f32(1.0 - f32(damping * h_step))
    if damp_factor < 0.0:
        damp_factor = f32(0.0)

    for _ in range(steps):
        for _ in range(substeps):
            for i in range(vertex_count):
                if inv_masses[i] > 0.0:
                    idx = i * 3
                    vx = f32(vel[idx + 0] + f32(gx * h_step))
                    vy = f32(vel[idx + 1] + f32(gy * h_step))
                    vz = f32(vel[idx + 2] + f32(gz * h_step))

                    px, py, pz = pos[idx + 0], pos[idx + 1], pos[idx + 2]
                    prev[idx + 0] = px
                    prev[idx + 1] = py
                    prev[idx + 2] = pz

                    pos[idx + 0] = f32(px + f32(vx * h_step))
                    pos[idx + 1] = f32(py + f32(vy * h_step))
                    pos[idx + 2] = f32(pz + f32(vz * h_step))

            for i in range(vertex_count):
                if inv_masses[i] > 0.0:
                    idx = i * 3
                    vx = f32(f32(pos[idx + 0] - prev[idx + 0]) / h_step)
                    vy = f32(f32(pos[idx + 1] - prev[idx + 1]) / h_step)
                    vz = f32(f32(pos[idx + 2] - prev[idx + 2]) / h_step)

                    vel[idx + 0] = f32(vx * damp_factor)
                    vel[idx + 1] = f32(vy * damp_factor)
                    vel[idx + 2] = f32(vz * damp_factor)

    world_header = struct.pack("<QQfIfff", seed, steps, fixed_dt, substeps, gx, gy, gz)
    cloth_header = struct.pack("<IIfff", cloth_id, vertex_count, mass, damping, thickness)
    h = fnv1a64(world_header)
    h = fnv1a64(cloth_header, h)
    h = fnv1a64(pack_f32_list(pos), h)
    h = fnv1a64(pack_f32_list(vel), h)
    h = fnv1a64(pack_f32_list(inv_masses), h)
    return h


if __name__ == "__main__":
    for s in (0, 5, 10):
        print(f"step={s}: 0x{run_small_cloth_reference(s):016x}ULL")
