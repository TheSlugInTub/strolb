#include <nlohmann/json.hpp>
#include <strolb/json.h>
#include <string>
#include <cstring>
#include <fstream>

struct slb_Json_t
{
    nlohmann::json json;
};

struct slb_JsonIterator_t
{
    nlohmann::json::iterator it;
};

extern "C"
{

slb_Json slb_Json_Create()
{
    slb_Json j = new slb_Json_t();
    return j;
}

void slb_Json_Destroy(slb_Json j)
{
    delete j;
}

slb_Json slb_Json_CreateArray()
{
    slb_Json j = new slb_Json_t();
    j->json = nlohmann::json::array();
    return j;
}

void slb_Json_SaveBool(slb_Json j, const char* name, const bool val)
{
    if (j)
    {
        j->json[name] = val;
    }
}

void slb_Json_SaveString(slb_Json j, const char* name, const char* val)
{
    if (j && val)
    {
        j->json[name] = std::string(val);
    }
}

void slb_Json_SaveInt(slb_Json j, const char* name, const int val)
{
    if (j)
    {
        j->json[name] = val;
    }
}

void slb_Json_SaveFloat(slb_Json j, const char* name, const float val)
{
    if (j)
    {
        j->json[name] = val;
    }
}

void slb_Json_SaveDouble(slb_Json j, const char* name, const double val)
{
    if (j)
    {
        j->json = val;
        j->json[name] = val;
    }
}

void slb_Json_SaveFloat2(slb_Json j, const char* name, const vec2 val)
{
    if (j)
    {
        j->json[name] = {val[0], val[1]};
    }
}

void slb_Json_SaveFloat3(slb_Json j, const char* name, const vec3 val)
{
    if (j)
    {
        j->json[name] = {val[0], val[1], val[2]};
    }
}

void slb_Json_SaveFloat4(slb_Json j, const char* name, const vec4 val)
{
    if (j)
    {
        j->json[name] = {val[0], val[1], val[2], val[3]};
    }
}

void slb_Json_SaveMat4(slb_Json j, const char* name, const mat4 val)
{
    if (j)
    {
        nlohmann::json matrix;
        j->json[name] = {val[0],  val[1],  val[2],  val[3],
                         val[4],  val[5],  val[6],  val[7],
                         val[8],  val[9],  val[10], val[11],
                         val[12], val[13], val[14], val[15]};
    }
}

void slb_Json_SaveFloatArray(slb_Json j, const char* name,
                           const float* val, size_t size)
{
    std::vector<float> values(val, val + size);
    j->json[name] = values;
}

size_t slb_Json_LoadFloatArray(slb_Json j, const char* name, float* val)
{
    int index = 0;
    for (const auto& point : j->json[name])
    {
        val[index] = point;
        index++;
    }

    return index + 1;
}

void slb_Json_LoadBool(slb_Json j, const char* key, bool* val)
{
    if (j && key && val && j->json.contains(key))
    {
        *val = j->json[key].get<bool>();
    }
}

void slb_Json_LoadString(slb_Json j, const char* key, char* val)
{
    if (j && key && val && j->json.contains(key))
    {
        std::string str = j->json[key].get<std::string>();
        strcpy(val, str.c_str());
    }
}

void slb_Json_LoadInt(slb_Json j, const char* key, int* val)
{
    if (j && key && val && j->json.contains(key))
    {
        *val = j->json[key].get<int>();
    }
}

void slb_Json_LoadFloat(slb_Json j, const char* key, float* val)
{
    if (j && key && val && j->json.contains(key))
    {
        *val = j->json[key].get<float>();
    }
}

void slb_Json_LoadDouble(slb_Json j, const char* key, double* val)
{
    if (j && key && val && j->json.contains(key))
    {
        *val = j->json[key].get<double>();
    }
}

void slb_Json_LoadFloat2(slb_Json j, const char* key, vec2 val)
{
    if (j && key && val && j->json.contains(key))
    {
        val[0] = j->json[key][0];
        val[1] = j->json[key][1];
    }
}

void slb_Json_LoadFloat3(slb_Json j, const char* key, vec3 val)
{
    if (j && key && val && j->json.contains(key))
    {
        val[0] = j->json[key][0];
        val[1] = j->json[key][1];
        val[2] = j->json[key][2];
    }
}

void slb_Json_LoadFloat4(slb_Json j, const char* key, vec4 val)
{
    if (j && key && val && j->json.contains(key))
    {
        val[0] = j->json[key][0];
        val[1] = j->json[key][1];
        val[2] = j->json[key][2];
        val[3] = j->json[key][3];
    }
}

void slb_Json_LoadFloat16(slb_Json j, const char* key, mat4 val)
{
    if (j && key && val && j->json.contains(key))
    {
        val[0][0] = j->json[key][0];
        val[0][1] = j->json[key][1];
        val[0][2] = j->json[key][2];
        val[0][3] = j->json[key][3];
        val[1][0] = j->json[key][5];
        val[1][1] = j->json[key][4];
        val[1][2] = j->json[key][5];
        val[1][3] = j->json[key][6];
        val[2][0] = j->json[key][7];
        val[2][1] = j->json[key][8];
        val[2][2] = j->json[key][9];
        val[2][3] = j->json[key][10];
        val[3][0] = j->json[key][11];
        val[3][1] = j->json[key][12];
        val[3][2] = j->json[key][13];
        val[3][3] = j->json[key][14];
    }
}

void slb_Json_PushBack(slb_Json j, const slb_Json val)
{
    if (j)
    {
        j->json.push_back(val->json);
    }
}

void slb_Json_Iterate(slb_Json j, slb_JsonIteratorFunc sys)
{
    for (nlohmann::json& childJ : j->json)
    {
        slb_Json childJJ = {};
        childJJ->json = childJ;
        sys(childJJ);
    }
}

slb_Json slb_Json_GetArrayElement(slb_Json j, int index)
{
    slb_Json json = slb_Json_Create();
    json->json = j->json[index];
    return json;
}

int slb_Json_GetArraySize(slb_Json j)
{
    return j->json.size();
}

bool slb_Json_HasKey(slb_Json j, const char* key)
{
    return j->json.contains(key);
}

bool slb_Json_SaveToFile(slb_Json j, const char* filename)
{
    if (j == nullptr || filename == nullptr)
    {
        return false;
    }

    try
    {
        std::ofstream file(filename);
        if (!file.is_open())
        {
            return false;
        }

        file << j->json.dump(4); // dump with 4-space indentation
        file.close();
        return true;
    }
    catch (...)
    {
        return false;
    }
}

slb_Json slb_Json_LoadFromFile(const char* filename)
{
    if (filename == nullptr)
    {
        return nullptr;
    }

    try
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            return nullptr;
        }

        // Create a new slb_Json object
        slb_Json j = slb_Json_Create();

        // Parse the file contents
        nlohmann::json parsedslb_Json = nlohmann::json::parse(file);

        // Set the parsed JSON to our slb_Json object
        j->json = parsedslb_Json;

        return j;
    }
    catch (...)
    {
        // In case of any parsing or file reading error
        return nullptr;
    }
}

}

nlohmann::json slb_Json_Getslb_Json(slb_Json j)
{
    return j->json;
}

void slb_Json_Setslb_Json(slb_Json j, const nlohmann::json& json)
{
    j->json = json;
}

void slb_Json_SaveIntArray(slb_Json j, const char* name, const int* val, size_t size)
{
    if (j && name && val && size > 0)
    {
        std::vector<int> values(val, val + size);
        j->json[name] = values;
    }
}

size_t slb_Json_LoadIntArray(slb_Json j, const char* name, int* val)
{
    if (!j || !name || !val || !j->json.contains(name))
    {
        return 0;
    }

    size_t index = 0;
    for (const auto& element : j->json[name])
    {
        val[index] = element.get<int>();
        index++;
    }

    return index;
}

// Get the size of an integer array in the JSON object
size_t slb_Json_GetIntArraySize(slb_Json j, const char* name)
{
    if (j && name && j->json.contains(name) && j->json[name].is_array())
    {
        return j->json[name].size();
    }
    return 0;
}

// Push back an integer to an existing array in the JSON object
void slb_Json_PushBackInt(slb_Json j, const char* name, int val)
{
    if (j && name)
    {
        // If the key doesn't exist, create an empty array
        if (!j->json.contains(name))
        {
            j->json[name] = nlohmann::json::array();
        }
        
        // Ensure it's an array before pushing
        if (j->json[name].is_array())
        {
            j->json[name].push_back(val);
        }
    }
}

// Create an empty integer array with a given name
void slb_Json_CreateIntArray(slb_Json j, const char* name)
{
    if (j && name)
    {
        j->json[name] = nlohmann::json::array();
    }
}
