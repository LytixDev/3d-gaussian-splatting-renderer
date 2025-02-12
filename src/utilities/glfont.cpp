#include <iostream>
#include "glfont.h"


Mesh generateTextGeometryBuffer(std::string text, float characterHeightOverWidth, float totalTextWidth) {

    float characterWidth = totalTextWidth / float(text.length());
    float characterHeight = characterHeightOverWidth * characterWidth;

    /* Each character represented as a quad made up of 2 triangles */
    unsigned int vertexCount = 4 * text.length();
    unsigned int indexCount = 6 * text.length();

    float charWidth  = 1.0 / 128.0; /* 128 ASCII characters in the texture */
    float charHeight = 1.0;

    Mesh mesh;

    mesh.vertices.resize(vertexCount);
    mesh.indices.resize(indexCount);
    mesh.textureCoordinates.resize(vertexCount);

    for(unsigned int i = 0; i < text.length(); i++)
    {
        float baseXCoordinate = float(i) * characterWidth;
        float charOffset = text[i] * charWidth;

        mesh.vertices.at(4 * i + 0) = {baseXCoordinate, 0, 0};
        mesh.vertices.at(4 * i + 1) = {baseXCoordinate + characterWidth, 0, 0};
        mesh.vertices.at(4 * i + 2) = {baseXCoordinate + characterWidth, characterHeight, 0};

        mesh.vertices.at(4 * i + 0) = {baseXCoordinate, 0, 0};
        mesh.vertices.at(4 * i + 2) = {baseXCoordinate + characterWidth, characterHeight, 0};
        mesh.vertices.at(4 * i + 3) = {baseXCoordinate, characterHeight, 0};


        mesh.indices.at(6 * i + 0) = 4 * i + 0;
        mesh.indices.at(6 * i + 1) = 4 * i + 1;
        mesh.indices.at(6 * i + 2) = 4 * i + 2;
        mesh.indices.at(6 * i + 3) = 4 * i + 0;
        mesh.indices.at(6 * i + 4) = 4 * i + 2;
        mesh.indices.at(6 * i + 5) = 4 * i + 3;

        /* vec2 in texture coordinate space */
        mesh.textureCoordinates.at(4 * i + 0) = {charOffset, 0};
        mesh.textureCoordinates.at(4 * i + 1) = {charOffset + charWidth, 0};
        mesh.textureCoordinates.at(4 * i + 2) = {charOffset + charWidth, charHeight};
        mesh.textureCoordinates.at(4 * i + 3) = {charOffset, charHeight};
    }

    return mesh;
}
