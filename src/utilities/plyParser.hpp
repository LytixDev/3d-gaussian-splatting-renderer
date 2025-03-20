
/*
 *  Copyright (C) 2025 Nicolai Brand (https://lytix.dev)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once 

#include <vector>
#include <string>
#include <glm/glm.hpp>

#define SPHERICAL_HARMONICS_COEFFS_COUNT 45

typedef struct {
    float coeffs[SPHERICAL_HARMONICS_COEFFS_COUNT];
} SphericalHarmonics;

typedef struct gaussian_splat_t {
    std::string filename;
    bool had_error;
    bool from_ply; // if false then from splat
    std::vector<std::string> warning_and_error_messages;
    double load_time_in_ms = -1;

    /* Number of "vertices" */
    size_t count;
    /* In world-space coordinates */
    std::vector<glm::vec3> ws_positions; // x, y, z
    std::vector<glm::vec3> normals; // nx, ny, nz
    std::vector<glm::vec3> colors; // f_dc_0, f_dc_1, f_dc_2
    std::vector<SphericalHarmonics> shs; // f_rest_0 .. f_rest_44
    // Between 0 and 1, mapped by the sigmoid function
    std::vector<float> opacities; // opacity
    // Raised to e
    std::vector<glm::vec3> scales; // scale_0, scale_1, scale_2
    /* Quaternion with magnitude of 1 */
    std::vector<glm::vec4> rotations; // rot_0 .. rot_3
} GaussianSplat;

GaussianSplat gaussian_splat_from_file(std::string filename);
GaussianSplat gaussian_splat_from_ply_file(std::string filename);
GaussianSplat gaussian_splat_from_splat_file(std::string filename);

void gaussian_splat_print(GaussianSplat &splat);

