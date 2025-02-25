#ifndef CAMERA_HPP
#define CAMERA_HPP
#pragma once

// System headers
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <iostream>

namespace Gloom
{
    class Camera
    {
    public:
        Camera(glm::vec3 position         = glm::vec3(0.0f, 0.0f, 2.0f),
               GLfloat   movementSpeed    = 5.0f,
               GLfloat   mouseSensitivity = 0.1f)
        {
            cPosition         = position;
            cMovementSpeed    = movementSpeed;
            cMouseSensitivity = mouseSensitivity;
            
            updateCameraVectors();
        }

        // Public member functions

        /* Getter for the view matrix */
        glm::mat4 getViewMatrix() {
            return glm::lookAt(cPosition, cPosition + cFront, cUp);
        }

        /* Getter for the camera position */
        glm::vec3 getPosition() const { return cPosition; }

        /* Handle keyboard inputs from a callback mechanism */
        void handleKeyboardInputs(int key, int action)
        {
            if (key >= 0 && key < 512) {
                if (action == GLFW_PRESS) {
                    keysInUse[key] = true;
                }
                else if (action == GLFW_RELEASE) {
                    keysInUse[key] = false;
                }
            }
        }

        /* Handle mouse button inputs from a callback mechanism */
        void handleMouseButtonInputs(int button, int action)
        {
            if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
                isMousePressed = true;
            }
            else {
                isMousePressed = false;
                resetMouse = true;
            }
        }

        /* Handle cursor position from a callback mechanism */
        void handleCursorPosInput(double xpos, double ypos)
        {
            if (!isMousePressed) return;

            if (resetMouse) {
                lastX = xpos;
                lastY = ypos;
                resetMouse = false;
                return;
            }

            float xoffset = xpos - lastX;
            float yoffset = lastY - ypos; // Reversed since y-coordinates range from bottom to top
            lastX = xpos;
            lastY = ypos;

            xoffset *= cMouseSensitivity;
            yoffset *= cMouseSensitivity;

            yaw += xoffset;
            pitch += yoffset;

            // Constrain pitch
            if (pitch > 89.0f) pitch = 89.0f;
            if (pitch < -89.0f) pitch = -89.0f;

            updateCameraVectors();
        }

        /* Update the camera position and view matrix
           `deltaTime` is the time between the current and last frame */
        void updateCamera(GLfloat deltaTime)
        {
            GLfloat velocity = cMovementSpeed * deltaTime;

            if (keysInUse[GLFW_KEY_W])
                cPosition += cFront * velocity;
            if (keysInUse[GLFW_KEY_S])
                cPosition -= cFront * velocity;
            if (keysInUse[GLFW_KEY_A])
                cPosition -= cRight * velocity;
            if (keysInUse[GLFW_KEY_D])
                cPosition += cRight * velocity;
            if (keysInUse[GLFW_KEY_SPACE])
                cPosition += glm::vec3(0.0f, 1.0f, 0.0f) * velocity;
            if (keysInUse[GLFW_KEY_LEFT_SHIFT])
                cPosition -= glm::vec3(0.0f, 1.0f, 0.0f) * velocity;
            if (keysInUse[GLFW_KEY_E])
                cPosition += cUp * velocity;
            if (keysInUse[GLFW_KEY_Q])
                cPosition -= cUp * velocity;
        }

    private:
        // Disable copying and assignment
        Camera(Camera const &) = delete;
        Camera & operator =(Camera const &) = delete;

        // Private member function

        void updateCameraVectors() {
            glm::vec3 front;
            front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            front.y = sin(glm::radians(pitch));
            front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            
            cFront = glm::normalize(front);
            cRight = glm::normalize(glm::cross(cFront, glm::vec3(0.0f, 1.0f, 0.0f)));
            cUp = glm::normalize(glm::cross(cRight, cFront));
        }

        // Private member variables

        // Camera Attributes
        glm::vec3 cPosition;
        glm::vec3 cFront = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 cUp = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 cRight;

        // Euler Angles
        float yaw = -90.0f;
        float pitch = 0.0f;

        // Camera options
        GLfloat cMovementSpeed;
        GLfloat cMouseSensitivity;

        // Mouse tracking
        float lastX = 0.0f;
        float lastY = 0.0f;
        
        // State
        GLboolean resetMouse = true;
        GLboolean isMousePressed = false;
        GLboolean keysInUse[512] = {false};
    };
}

#endif
