#pragma once

#include <cglm/cglm.h>

typedef struct slb_Camera
{
    vec3 position;
    vec3 front;
    vec3 up;
    vec3 right;

    float yaw;
    float pitch;
    float roll;
    float zoom;
    float FOV;

    vec3 worldUp;
} slb_Camera;

// Initializes the camera
slb_Camera slb_Camera_Create(vec3 position, vec3 up, float yaw,
                         float pitch, float FOV);

// Updates the right, up and front vectors
void slb_Camera_UpdateVectors(slb_Camera* camera);

// Gets the view matrix of the camera
void slb_Camera_GetViewMatrix(slb_Camera* camera, mat4 view);

void slb_Camera_CursorToWorld(slb_Camera* camera, int cursorX, int cursorY,
        int screenWidth, int screenHeight, mat4 projection, mat4 view,
        vec3 pos);
