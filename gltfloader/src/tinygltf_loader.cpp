//
// TODO(syoyo): Print extensions and extras for each glTF object.
//
#define TINYGLTF_IMPLEMENTATION
// #define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"
#include "tinygltf_dump.h"

// include the Defold SDK
#include <dmsdk/sdk.h>
#include <dmsdk/gamesys/components/comp_collection_proxy.h>
#include <dmsdk/gamesys/components/comp_factory.h>

typedef struct DefoldModel
{
    tinygltf::Model model;

} DefoldModel;

static std::vector<tinygltf::Model>     g_models;

static dmResource::HFactory             m_Factory;
static dmConfigFile::HConfig            m_ConfigFile;

static dmGameObject::HCollection        m_MainCollection;
static dmGameObject::HCollection        m_LevelCollection;

static dmGameSystem::HFactoryWorld      m_FactoryWorld;
dmGameSystem::HFactoryComponent         m_MeshFactory;

dmArray<dmGameObject::HInstance>        m_Meshes;

void InitMeshBuilding(dmResource::HFactory _Factory, dmConfigFile::HConfig _ConfigFile)
{
    m_Factory = _Factory;
    m_ConfigFile = _ConfigFile;

    const char* path = dmConfigFile::GetString(m_ConfigFile, "bootstrap.main_collection", 0);
    dmResource::Result res = dmResource::Get(m_Factory, path, (void **)&m_MainCollection);
    if (dmResource::RESULT_OK != res)
    {
        dmLogError("Failed to get level collection '%s'", path);
        return;
    }
    assert(m_MainCollection != 0);

    dmhash_t go_name = dmHashString64("/factories");
    dmGameObject::HInstance go = dmGameObject::GetInstanceFromIdentifier(m_MainCollection, go_name);
    if (go == 0)
    {
        dmLogError("Main collection does not have a game object named %s", dmHashReverseSafe64(go_name));
        return;
    }

    uint32_t component_type_index;
    dmGameObject::Result r = dmGameObject::GetComponent(go, dmHashString64("meshfactory"), &component_type_index, (dmGameObject::HComponent *)&m_MeshFactory, (dmGameObject::HComponentWorld *)&m_FactoryWorld);
    assert(dmGameObject::RESULT_OK == r);
}

void DestroyMeshBuilding()
{
    if (m_MainCollection)
    {
        dmResource::Release(m_Factory, m_MainCollection);
        m_MainCollection = 0;
    }    
}

static dmGameObject::HInstance SpawnMesh(dmGameSystem::HFactoryComponent factory)
{
    if (m_Meshes.Full())
    {
        dmLogWarning("Stars buffer is full, skipping spawn of new star.");
        return 0;
    }

    uint32_t index = dmGameObject::AcquireInstanceIndex(m_MainCollection);
    if (index == dmGameObject::INVALID_INSTANCE_POOL_INDEX)
    {
        dmLogError("Gameobject buffer is full. See `collection.max_instances` in game.project");
        return 0;
    }

    dmhash_t starid = dmGameObject::ConstructInstanceId(index);

    float y = 0.0f;
    dmVMath::Point3 position(0.0f, y, 0.1f);
    dmVMath::Quat rotation(0,0,0,1);
    dmVMath::Vector3 scale(2.0f,2.0f,2.0f);

    dmGameObject::HPropertyContainer properties = 0;

    dmGameObject::HInstance instance = dmGameSystem::CompFactorySpawn(m_FactoryWorld, factory, m_MainCollection,
                                                            index, starid, position, rotation, scale, properties);

    m_Meshes.Push(instance);
    return instance;
}

int GenerateGltfMesh(const tinygltf::Model &model)
{

    return 0;
}

int load_gltf(const char *gltf_filename, bool dump)
{
    // Store original JSON string for `extras` and `extensions`
    bool store_original_json_for_extras_and_extensions = false;
    // if (argc > 2) {
    //   store_original_json_for_extras_and_extensions = true;
    // }

    tinygltf::Model model;
    tinygltf::TinyGLTF gltf_ctx;
    std::string err;
    std::string warn;
    std::string input_filename(gltf_filename);
    std::string ext = tinygltf::GetFilePathExtension(input_filename);

    gltf_ctx.SetStoreOriginalJSONForExtrasAndExtensions(
        store_original_json_for_extras_and_extensions);

    bool ret = false;
    if (ext.compare("glb") == 0)
    {
        std::cout << "Reading binary glTF" << std::endl;
        // assume binary glTF.
        ret = gltf_ctx.LoadBinaryFromFile(&model, &err, &warn,
                                          input_filename.c_str());
    }
    else
    {
        std::cout << "Reading ASCII glTF" << std::endl;
        // assume ascii glTF.
        ret = gltf_ctx.LoadASCIIFromFile(&model, &err, &warn, input_filename.c_str());
    }

    if (!warn.empty())
    {
        printf("Warn: %s\n", warn.c_str());
    }

    if (!err.empty())
    {
        printf("Err: %s\n", err.c_str());
    }

    if (!ret)
    {
        printf("Failed to parse glTF\n");
        return -1;
    }

    if (dump)
        Dump(model);

    int modelid = g_models.size();
    g_models.push_back(model);

    return modelid;
}
