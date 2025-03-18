/*
 *  Copyright (C) 2025 Nicolai Brand
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

#include "plyParser.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iterator>
#include <filesystem>
#include <algorithm>


#define EXPECTED_PROPERTIES_COUNT 62

static const char *expected_properties[] = {
    "x",
    "y",
    "z",
    "nx",
    "ny",
    "nz",
    "f_dc_0",
    "f_dc_1",
    "f_dc_2",
    "f_rest_0",
    "f_rest_1",
    "f_rest_2",
    "f_rest_3",
    "f_rest_4",
    "f_rest_5",
    "f_rest_6",
    "f_rest_7",
    "f_rest_8",
    "f_rest_9",
    "f_rest_10",
    "f_rest_11",
    "f_rest_12",
    "f_rest_13",
    "f_rest_14",
    "f_rest_15",
    "f_rest_16",
    "f_rest_17",
    "f_rest_18",
    "f_rest_19",
    "f_rest_20",
    "f_rest_21",
    "f_rest_22",
    "f_rest_23",
    "f_rest_24",
    "f_rest_25",
    "f_rest_26",
    "f_rest_27",
    "f_rest_28",
    "f_rest_29",
    "f_rest_30",
    "f_rest_31",
    "f_rest_32",
    "f_rest_33",
    "f_rest_34",
    "f_rest_35",
    "f_rest_36",
    "f_rest_37",
    "f_rest_38",
    "f_rest_39",
    "f_rest_40",
    "f_rest_41",
    "f_rest_42",
    "f_rest_43",
    "f_rest_44",
    "opacity",
    "scale_0",
    "scale_1",
    "scale_2",
    "rot_0",
    "rot_1",
    "rot_2",
    "rot_3"
};

static std::vector<std::string> next_line_tokens(std::ifstream &file) {
    std::string line;
    if (std::getline(file, line)) {
        std::istringstream iss(line);
        return {std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};
    }
    return {"error"};
}


static glm::vec4 normalize_quaternion(glm::vec4 r) {
	float ss = r.x * r.x + r.y * r.y + r.z * r.z + r.w * r.w;
	float norm = std::sqrt(ss);
	return glm::vec4(r.x / norm, r.y / norm, r.z / norm, r.w / norm);
}


static float sigmoid(float opacity) {
	return 1.0 / (1.0 + std::exp(-opacity));
}

// Baed on https://github.com/graphdeco-inria/diff-gaussian-rasterization/blob/main/cuda_rasterizer/forward.cu
// Zero degre spherical harmonics

const float C0 = 0.28209479177387814f;

glm::vec3 zero_deg_sh(glm::vec3 color) {
	return 0.5f + C0 * color;
}


GaussianSplat gaussian_splat_from_file(std::string filename)
{
    auto start_time = std::chrono::high_resolution_clock::now();
    
    GaussianSplat splat;
    std::string file_extension = std::filesystem::path(filename).extension().string();
    std::transform(file_extension.begin(), file_extension.end(), file_extension.begin(), ::tolower);
    
    if (file_extension == ".ply") {
        splat = gaussian_splat_from_ply_file(filename);
        splat.from_ply = true;
    }  else if (file_extension == ".splat") {
        splat = gaussian_splat_from_splat_file(filename);
        splat.from_ply = false;
    } else {
        splat.had_error = true;
        splat.warning_and_error_messages.push_back("Error: Unsupported file format. Supported formats are .ply and .splat");
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    splat.load_time_in_ms = duration;
    
    return splat;
}

GaussianSplat gaussian_splat_from_ply_file(std::string filename)
{
    GaussianSplat splat;
    splat.filename = std::filesystem::path(filename).filename().string();
    splat.had_error = false;

    std::ifstream file(filename);
    if (!file.is_open()) {
        splat.warning_and_error_messages.push_back("Error: Could not open file " + filename);
        splat.had_error = true;
        return splat;
    }

    /* Expect first line to be "ply" */
    std::vector<std::string> tokens = next_line_tokens(file);
    if (tokens[0] != "ply") {
        splat.warning_and_error_messages.push_back("Error: Unable to parse .ply file as it does not start with 'ply'");
        splat.had_error = true;
        return splat;
    }

    /* Parse rest of header */
    int property_count = 0;
    int vertices = -1;
    while (true) {
        tokens = next_line_tokens(file);
        std::string specifier = tokens[0];
        if (specifier == "end_header") { break; }
        if (specifier == "error") {
            splat.warning_and_error_messages.push_back("Expected whitespace in line in header");
            splat.had_error = true;
            return splat;
        }
        if (tokens.size() != 3) { 
            splat.warning_and_error_messages.push_back("Error: expected each line in the header to have 3 words separated by whitespace.");
            splat.had_error = true;
            return splat;
        }

        if (specifier == "format") {
            auto format = tokens[1];
            if (format != "binary_little_endian") {
                splat.warning_and_error_messages.push_back("Error: Only binary_little_endian .ply format, not " + format);
                splat.had_error = true;
                return splat;
            }
        } else if (specifier == "element") {
            auto element_kind = tokens[1];
            if (element_kind == "vertex") {
                vertices = std::stoi(tokens[2]);
            } else {
                splat.warning_and_error_messages.push_back("Warning: Unrecognized element kind " + element_kind);
            }
        } else if (specifier == "property") {
            auto datatype = tokens[1];
            auto property_name = tokens[2];
            /* We expect all properties to be of type float */
            if (datatype != "float") {
                splat.warning_and_error_messages.push_back("Error: Unrecognized property, ignoring " + property_name);
                splat.had_error = true;
                return splat;
            } 

            auto expected_property_name = expected_properties[property_count];
            /* 
             * Probably a bad sign, but I know some software will set slighly different names for 
             * the normals (nx, ny, nz vs nnx, nny, nnz). 
             */
            if (expected_property_name != property_name) {
                std::stringstream ss;
                ss << "Warning: Expected property " << property_count << " to have name "
                   << expected_property_name << " but got " << property_name;
                splat.warning_and_error_messages.push_back(ss.str());
            }
            property_count++;
        } 
    }

    /* Make sure we parsed a vertices count and that we have the expected number of properties */
    if (vertices == -1) {
        splat.warning_and_error_messages.push_back("Error: .ply does not contain a vertex number");
        splat.had_error = true;
        return splat;
    }
    if (property_count !=  EXPECTED_PROPERTIES_COUNT) {
        std::stringstream ss;
        ss << "Error: expected " << EXPECTED_PROPERTIES_COUNT << " but got " << property_count;
        splat.warning_and_error_messages.push_back(ss.str());
    }

    splat.count = vertices;
    splat.ws_positions.reserve(vertices);
    splat.normals.reserve(vertices);
    splat.colors.reserve(vertices);
    splat.shs.reserve(vertices);
    splat.opacities.reserve(vertices);
    splat.scales.reserve(vertices);
    splat.rotations.reserve(vertices);

    float data[EXPECTED_PROPERTIES_COUNT];
    /* Parse binary data */
    for (int i = 0; i < vertices; i++) {
        file.read(reinterpret_cast<char*>(data), sizeof(float) * EXPECTED_PROPERTIES_COUNT);
        if (!file) {
            std::stringstream ss;
            ss << "Error: Failed to read vertex data at index " << i;
            splat.warning_and_error_messages.push_back(ss.str());
            splat.had_error = true;
            return splat;
        }

        /* 
         * If we don't take the -x and -y values, the scene will be upside down for these axes'.
         * Seems as though it stored in a left-handed system?
         */
        splat.ws_positions.push_back(glm::vec3(-data[0], -data[1], data[2]));
        splat.normals.push_back(glm::vec3(data[3], data[4], data[5]));
        splat.colors.push_back(zero_deg_sh(glm::vec3(data[6], data[7], data[8])));

        SphericalHarmonics sh;
        for (int j = 0; j < SPHERICAL_HARMONICS_COEFFS_COUNT; j++) {
            sh.coeffs[j] = data[9 + j];
        }
        splat.shs.push_back(sh);
        splat.opacities.push_back(sigmoid(data[54]));
        splat.scales.push_back(glm::exp(glm::vec3(data[55], data[56], data[57])));
        splat.rotations.push_back(normalize_quaternion(glm::vec4(data[58], data[59], data[60], data[61])));
    }

    file.close();

    // TODO: Optional print flag maybe
    for (auto message : splat.warning_and_error_messages) {
        std::cout << message << std::endl;
    }

    return splat;
}

GaussianSplat gaussian_splat_from_splat_file(std::string filename)
{
    // The .splat file format is "Reverse engineered" from:
    // https://github.com/antimatter15/splat/blob/main/convert.py
    GaussianSplat splat;
    splat.filename = std::filesystem::path(filename).filename().string();
    splat.had_error = false;

    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        splat.warning_and_error_messages.push_back("Error: Could not open file " + filename);
        splat.had_error = true;
        return splat;
    }

    // Get file size to determine number of vertices
    file.seekg(0, std::ios::end);
    std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Each vertex in the .splat format consists of:
    // - 3 floats for position
    // - 3 floats for scales
    // - 4 bytes for colors
    // - 4 bytes for rotation
    const size_t bytes_per_vertex = 12 + 12 + 4 + 4;
    
    if (file_size % bytes_per_vertex != 0) {
        splat.warning_and_error_messages.push_back("Error: File size is not a multiple of vertex size");
        splat.had_error = true;
        return splat;
    }

    size_t vertex_count = file_size / bytes_per_vertex;
    splat.count = vertex_count;

    // Reserve space
    splat.ws_positions.reserve(vertex_count);
    //splat.normals.reserve(vertex_count);
    splat.colors.reserve(vertex_count);
    //splat.shs.reserve(vertex_count);
    splat.opacities.reserve(vertex_count);
    splat.scales.reserve(vertex_count);
    splat.rotations.reserve(vertex_count);

    float position[3];
    float scales[3];
    uint8_t color_data[4];
    uint8_t rotation_data[4];
    for (size_t i = 0; i < vertex_count; i++) {
        file.read(reinterpret_cast<char*>(position), sizeof(float) * 3);
        // NOTE: Same as .ply files, coordinate system seems to be left handed
        splat.ws_positions.push_back(glm::vec3(-position[0], -position[1], position[2]));

        // Read scales
        file.read(reinterpret_cast<char*>(scales), sizeof(float) * 3);
        splat.scales.push_back(glm::vec3(scales[0], scales[1], scales[2]));
        //splat.scales.push_back(glm::vec3(scales[0], scales[1], 1e-06));

        // Read color. Normalize it between 0-1
        file.read(reinterpret_cast<char*>(color_data), sizeof(uint8_t) * 4);
        splat.colors.push_back(glm::vec3(
            color_data[0] / 255.0f,
            color_data[1] / 255.0f,
            color_data[2] / 255.0f
        ));
        // Opacity. Already calculated :: 1 / (1 + e^(-opacity)). Normalize it as well.
        splat.opacities.push_back(color_data[3] / 255.0f);

        // Read rotation. Normalize each component between -1 to 1.
        file.read(reinterpret_cast<char*>(rotation_data), sizeof(uint8_t) * 4);
        glm::vec4 rot(
            //rotation_data[0],
            //rotation_data[1],
            //rotation_data[2],
            //rotation_data[3]
            float(rotation_data[0]) - 128.0f / 128.0f,
            float(rotation_data[1]) - 128.0f / 128.0f,
            float(rotation_data[2]) - 128.0f / 128.0f,
            float(rotation_data[3]) - 128.0f / 128.0f
            // float(rotation_data[0])  * 100000.0f,
            // float(rotation_data[1]) * 100000.0f,
            // float(rotation_data[2]) * 100000.0f,
            // float(rotation_data[3]) * 100000.0f
        );
        
        //splat.rotations.push_back(normalize_quaternion(rot));
        splat.rotations.push_back(normalize_quaternion(rot));

        if (!file) {
            std::stringstream ss;
            ss << "Error: Failed to read vertex data at index " << i;
            splat.warning_and_error_messages.push_back(ss.str());
            splat.had_error = true;
            return splat;
        }
    }

    file.close();

    for (auto message : splat.warning_and_error_messages) {
        std::cout << message << std::endl;
    }

    return splat;
}


void gaussian_splat_print(GaussianSplat &splat)
{
    std::cout << "Gaussian Splat Information for " << splat.filename << "\n";
    std::cout << "  Had Error: " << (splat.had_error ? "Yes" : "No") << "\n";
    std::cout << "  Number of Vertices: " << splat.count << "\n";

    // for (const auto& pos : splat.ws_positions) {
    //     std::cout << "Position: (" << pos.x << ", " << pos.y << ", " << pos.z << ")\n";
    // }
    
    // for (const auto& color : splat.colors) {
    //     std::cout << "Color: (" << color.r << ", " << color.g << ", " << color.b << ")\n";
    // }
    
    for (const auto& scales : splat.scales) {
        std::cout << "Scales: (" << scales.x << ", " << scales.y << ", " << scales.z << ")\n";
    }
}

