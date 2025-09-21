#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdbool.h>
#include <cglm/cglm.h>

typedef struct slb_Json_t*        slb_Json;
typedef struct slb_JsonIterator_t slb_JsonIterator;

slb_Json slb_Json_Create();
slb_Json slb_Json_CreateArray();
void   slb_Json_Destroy(slb_Json j);

void slb_Json_SaveBool(slb_Json j, const char* name, const bool val);
void slb_Json_SaveString(slb_Json j, const char* name, const char* val);
void slb_Json_SaveInt(slb_Json j, const char* name, const int val);
void slb_Json_SaveFloat(slb_Json j, const char* name, const float val);
void slb_Json_SaveDouble(slb_Json j, const char* name, const double val);

void slb_Json_SaveFloat2(slb_Json j, const char* name, const vec2 val);
void slb_Json_SaveFloat3(slb_Json j, const char* name, const vec3 val);
void slb_Json_SaveFloat4(slb_Json j, const char* name, const vec4 val);

void slb_Json_SaveMat4(slb_Json j, const char* name, const mat4 val);

void slb_Json_SaveFloatArray(slb_Json j, const char* name,
                           const float* val, size_t size);

size_t slb_Json_LoadFloatArray(slb_Json j, const char* name, float* val);
void   slb_Json_LoadBool(slb_Json j, const char* key, bool* val);
void   slb_Json_LoadString(slb_Json j, const char* key, char* val);
void   slb_Json_LoadInt(slb_Json j, const char* key, int* val);
void   slb_Json_LoadFloat(slb_Json j, const char* key, float* val);
void   slb_Json_LoadDouble(slb_Json j, const char* key, double* val);

void slb_Json_LoadFloat2(slb_Json j, const char* key, vec2 val);
void slb_Json_LoadFloat3(slb_Json j, const char* key, vec3 val);
void slb_Json_LoadFloat4(slb_Json j, const char* key, vec4 val);

void slb_Json_LoadFloat16(slb_Json j, const char* key, mat4 val);

void slb_Json_PushBack(slb_Json j, const slb_Json val);

typedef void (*slb_JsonIteratorFunc)(slb_Json j);

void slb_Json_Iterate(slb_Json j, slb_JsonIteratorFunc sys);

slb_Json slb_Json_GetArrayElement(slb_Json j, int index);
int    slb_Json_GetArraySize(slb_Json j);
bool   slb_Json_HasKey(slb_Json j, const char* key);

bool   slb_Json_SaveToFile(slb_Json j, const char* filename);
slb_Json slb_Json_LoadFromFile(const char* filename);

void slb_Json_SaveIntArray(slb_Json j, const char* name, 
        const int* val, size_t size);
size_t slb_Json_LoadIntArray(slb_Json j, const char* name, int* val);
size_t slb_Json_GetIntArraySize(slb_Json j, const char* name);
void slb_Json_PushBackInt(slb_Json j, const char* name, int val);
void slb_Json_CreateIntArray(slb_Json j, const char* name);

#ifdef __cplusplus
}

#    include <nlohmann/json.hpp>

nlohmann::json slb_Json_Getslb_Json(slb_Json j);

void slb_Json_Setslb_Json(const slb_Json j, const nlohmann::json& json);

#endif
