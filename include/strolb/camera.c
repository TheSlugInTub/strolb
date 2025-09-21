#include <strolb/camera.h>

slb_Camera slb_Camera_Create(vec3 position, vec3 up, float yaw,
                             float pitch, float FOV)
{
    slb_Camera camera;
    camera.pitch = pitch;
    camera.yaw = yaw;
    camera.roll = 0.0f;
    camera.FOV = FOV;
    camera.zoom = 0.0f;
    camera.position[0] = position[0];
    camera.position[1] = position[1];
    camera.position[2] = position[2];
    camera.up[0] = up[0];
    camera.up[1] = up[1];
    camera.up[2] = up[2];
    camera.worldUp[0] = 0.0f;
    camera.worldUp[1] = 0.0f;
    camera.worldUp[2] = 1.0f;
    camera.front[0] = 0.0f;
    camera.front[1] = -2.0f;
    camera.front[2] = 0.0f;
    return camera;
}

void slb_Camera_UpdateVectors(slb_Camera* camera)
{
    // Corrected front vector calculation for Z-up system
    camera->front[0] =
        cos(glm_rad(camera->yaw)) * cos(glm_rad(camera->pitch));
    camera->front[1] =
        sin(glm_rad(camera->yaw)) * cos(glm_rad(camera->pitch));
    camera->front[2] = sin(glm_rad(camera->pitch));
    glm_normalize(camera->front);

    // Calculate right vector using worldUp (Z-axis)
    glm_cross(camera->front, camera->worldUp, camera->right);
    glm_normalize(camera->right);

    // Calculate real up vector using right and front
    glm_cross(camera->right, camera->front, camera->up);
    glm_normalize(camera->up);

    // Handle roll if needed
    if (camera->roll != 0.0f)
    {
        vec3 originalRight;
        glm_vec3_copy(camera->right, originalRight);

        float cosRoll = cos(camera->roll);
        float sinRoll = sin(camera->roll);

        // Rotate right vector
        camera->right[0] =
            cosRoll * originalRight[0] - sinRoll * camera->up[0];
        camera->right[1] =
            cosRoll * originalRight[1] - sinRoll * camera->up[1];
        camera->right[2] =
            cosRoll * originalRight[2] - sinRoll * camera->up[2];
        glm_normalize(camera->right);

        // Recalculate up vector
        glm_cross(camera->right, camera->front, camera->up);
        glm_normalize(camera->up);
    }
}

void slb_Camera_GetViewMatrix(slb_Camera* camera, mat4 view)
{
    glm_mat4_identity(view);

    vec3 center;
    glm_vec3_add(camera->position, camera->front, center);

    glm_lookat(camera->position, center, camera->up, view);
}

void slb_Camera_CursorToWorld(slb_Camera* camera, int cursorX,
                              int cursorY, int screenWidth,
                              int screenHeight, mat4 projection,
                              mat4 view, vec3 pos)
{
    // Step 1: Convert screen coordinates to normalized device coordinates (NDC)
    // Screen space: (0,0) top-left to (width,height) bottom-right
    // NDC space: (-1,-1) bottom-left to (1,1) top-right
    float ndc_x = (2.0f * cursorX) / screenWidth - 1.0f;
    float ndc_y = (2.0f * cursorY) / screenHeight - 1.0f;

    // Step 2: Create ray in clip space (NDC with depth)
    // We use two points: near plane (z=-1) and far plane (z=1)
    vec4 ray_clip_near = {ndc_x, ndc_y, -1.0f, 1.0f};
    vec4 ray_clip_far = {ndc_x, ndc_y, 1.0f, 1.0f};
    
    // Step 3: Convert from clip space to world space
    mat4 inverse_proj, inverse_view, inverse_vp;
    
    // Calculate inverse projection matrix
    glm_mat4_inv(projection, inverse_proj);
    
    // Calculate inverse view matrix
    glm_mat4_inv(view, inverse_view);
    
    // Transform from clip space to eye space (apply inverse projection)
    vec4 ray_eye_near, ray_eye_far;
    glm_mat4_mulv(inverse_proj, ray_clip_near, ray_eye_near);
    glm_mat4_mulv(inverse_proj, ray_clip_far, ray_eye_far);
    
    // Perspective divide
    if (ray_eye_near[3] != 0.0f) {
        ray_eye_near[0] /= ray_eye_near[3];
        ray_eye_near[1] /= ray_eye_near[3];
        ray_eye_near[2] /= ray_eye_near[3];
        ray_eye_near[3] = 1.0f;
    }
    
    if (ray_eye_far[3] != 0.0f) {
        ray_eye_far[0] /= ray_eye_far[3];
        ray_eye_far[1] /= ray_eye_far[3];
        ray_eye_far[2] /= ray_eye_far[3];
        ray_eye_far[3] = 1.0f;
    }
    
    // Transform from eye space to world space (apply inverse view)
    vec4 ray_world_near, ray_world_far;
    glm_mat4_mulv(inverse_view, ray_eye_near, ray_world_near);
    glm_mat4_mulv(inverse_view, ray_eye_far, ray_world_far);
    
    // Step 4: Create ray direction
    vec3 ray_origin = {ray_world_near[0], ray_world_near[1], ray_world_near[2]};
    vec3 ray_end = {ray_world_far[0], ray_world_far[1], ray_world_far[2]};
    vec3 ray_direction;
    
    glm_vec3_sub(ray_end, ray_origin, ray_direction);
    glm_vec3_normalize(ray_direction);
    
    // Step 5: Ray-plane intersection
    // Plane: Y = 0 (normal vector is (0, 1, 0))
    // Ray: P = ray_origin + t * ray_direction
    // Intersection: ray_origin.y + t * ray_direction.y = 0
    // Solve for t: t = -ray_origin.y / ray_direction.y
    
    if (fabsf(ray_direction[1]) < 1e-6f) {
        // Ray is parallel to the plane, no intersection
        return;
    }
    
    float t = -ray_origin[1] / ray_direction[1];
    
    if (t < 0.0f) {
        // Intersection is behind the ray origin
        return;
    }
    
    // Calculate intersection point
    pos[0] = ray_origin[0] + t * ray_direction[0];
    pos[1] = 0.0f; // On the Y=0 plane
    pos[2] = ray_origin[2] + t * ray_direction[2];
    
    return;
}
