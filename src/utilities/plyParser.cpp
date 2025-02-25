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


GaussianSplat gaussian_splat_from_ply_file(std::string filename)
{
    GaussianSplat splat;
    splat.filename = std::filesystem::path(filename).filename().string();
    splat.had_error = false;

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        splat.had_error = true;
        return splat;
    }

    /* Expect first line to be "ply" */
    std::vector<std::string> tokens = next_line_tokens(file);
    if (tokens[0] != "ply") {
        std::cerr << "Error: Unable to parse .ply file as it does not start with 'ply'" << std::endl;
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
            std::cerr << "Expected whitespace in line in header" << std::endl;
            splat.had_error = true;
            return splat;
        }
        if (tokens.size() != 3) { 
            std::cerr << "Error: expected each line in the header to have 3 words separated by whitespace." << std::endl;
            splat.had_error = true;
            return splat;
        }

        if (specifier == "format") {
            auto format = tokens[1];
            if (format != "binary_little_endian") {
                std::cerr << "Error: Only binary_little_endian .ply format, not " << format << std::endl;
                splat.had_error = true;
                return splat;
            }
        } else if (specifier == "element") {
            auto element_kind = tokens[1];
            if (element_kind == "vertex") {
                vertices = std::stoi(tokens[2]);
            } else {
                std::cerr << "Warning: Unrecognized element kind " << element_kind << std::endl;
            }
        } else if (specifier == "property") {
            auto datatype = tokens[1];
            auto property_name = tokens[2];
            /* We expect all properties to be of type float */
            if (datatype != "float") {
                std::cerr << "Error: Unrecognized property, ignoring " << property_name << std::endl;
                splat.had_error = true;
                return splat;
            } 

            auto expected_property_name = expected_properties[property_count];
            /* 
             * Probably a bad sign, but I know some software will set slighly different names for 
             * the normals (nx, ny, nz vs nnx, nny, nnz). 
             */
            if (expected_property_name != property_name) {
                std::cerr << "Warning: Expected property " << property_count << " to have name "
                    << expected_property_name << " but got " << property_name << std::endl;
            }
            property_count++;
        } 
    }

    /* Make sure we parsed a vertices count and that we have the expected number of properties */
    if (vertices == -1) {
        std::cerr << "Error: .ply does not contain a vertex number" << std::endl;
        splat.had_error = true;
        return splat;
    }
    if (property_count !=  EXPECTED_PROPERTIES_COUNT) {
        std::cerr << "Error: expecetd " << EXPECTED_PROPERTIES_COUNT << " but got " << property_count << std::endl;
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
            std::cerr << "Error: Failed to read vertex data at index " << i << std::endl;
            splat.had_error = true;
            return splat;
        }

        splat.ws_positions.push_back(glm::vec3(data[0], data[1], data[2]));
        splat.normals.push_back(glm::vec3(data[3], data[4], data[5]));
        splat.colors.push_back(glm::vec3(data[6], data[7], data[8]));

        SphericalHarmonics sh;
        for (int j = 0; j < SPHERICAL_HARMONICS_COEFFS_COUNT; j++) {
            sh.coeffs[j] = data[9 + j];
        }
        splat.shs.push_back(sh);

        splat.opacities.push_back(data[54]);
        splat.scales.push_back(glm::vec3(data[55], data[56], data[57]));
        splat.rotations.push_back(glm::vec4(data[58], data[59], data[60], data[61]));
    }

    file.close();
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
}
