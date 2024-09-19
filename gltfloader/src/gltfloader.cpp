// myextension.cpp
// Extension lib defines
#define LIB_NAME "gltfloader"
#define MODULE_NAME "gltfloader"

#include <stdlib.h>
#include <string.h>
#include <vector>

// include the Defold SDK
#include <dmsdk/sdk.h>

#include "geom.h"
#include "tiny_gltf.h"

extern int load_gltf(const char *gltf_filename, bool dump);
extern void InitMeshBuilding(dmResource::HFactory _Factory, dmConfigFile::HConfig _ConfigFile);
extern void DestroyMeshBuilding();


static void GetTableNumbersInt( lua_State * L, int tblidx, int *data )
{
    // Iterate indices and set float buffer with correct lookups
    lua_pushnil(L);
    size_t idx = 0;
    // Build a number array matching the buffer. They are all assumed to be type float (for the time being)
    while( lua_next( L, tblidx ) != 0) {
        data[idx++] = (int)lua_tonumber( L, -1 );
        lua_pop( L, 1 );
    }
}

static void GetTableNumbersFloat( lua_State * L, int tblidx, float *data )
{
    // Iterate indices and set float buffer with correct lookups
    lua_pushnil(L);
    size_t idx = 0;
    // Build a number array matching the buffer. They are all assumed to be type float (for the time being)
    while( lua_next( L, tblidx ) != 0) {
        data[idx++] = lua_tonumber( L, -1 );
        lua_pop( L, 1 );
    }
}

static int SetDataShortsToTable(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    unsigned int offset = luaL_checknumber(L, 1);
    unsigned int length = luaL_checknumber(L, 2);
    const unsigned char *data = (unsigned char *)luaL_checkstring(L, 3);
    luaL_checktype(L, 4, LUA_TTABLE);

    assert(length > 0);
    
    // Now we have the data, cast it to the union and write back out.
    int idx = 1;
    unsigned short *ptr = (unsigned short *)&data[offset];
    for( unsigned int i=0; i<length; i+=sizeof(unsigned short), ptr++)
    {
        unsigned int val = ( unsigned int )*ptr;
        //printf("%d\n", val);
        lua_pushnumber(L, val);  /* value */
        lua_rawseti(L, 4, idx++);  /* set table at key `i' */
    }

    return 0;
}

static int SetDataBytesToTable(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    unsigned int offset = luaL_checknumber(L, 1);
    unsigned int length = luaL_checknumber(L, 2);
    const unsigned char *data = (unsigned char *)luaL_checkstring(L, 3);
    luaL_checktype(L, 4, LUA_TTABLE);

    assert(length > 0);

    // Now we have the data, cast it to the union and write back out.
    int idx = 1;
    unsigned char *ptr = (unsigned char *)&data[offset];
    for( unsigned int i=0; i<length; i+=sizeof(unsigned char), ptr++)
    {
        unsigned int val = ( unsigned int )*ptr;
        //printf("%d\n", val);
        lua_pushnumber(L, val);  /* value */
        lua_rawseti(L, 4, idx++);  /* set table at key `i' */
    }

    return 0;
}

static int SetDataIntsToTable(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    unsigned int offset = luaL_checknumber(L, 1);
    unsigned int length = luaL_checknumber(L, 2);
    const unsigned char *data = (unsigned char *)luaL_checkstring(L, 3);
    luaL_checktype(L, 4, LUA_TTABLE);

    assert(length > 0);
    
    // Now we have the data, cast it to the union and write back out.
    int idx = 1;
    unsigned int *ptr = (unsigned int *)&data[offset];
    for( unsigned int i=0; i<length; i+=sizeof(unsigned int), ptr++)
    {
        unsigned int val = *ptr;
        //printf("%d\n", val);
        lua_pushnumber(L, val);  /* value */
        lua_rawseti(L, 4, idx++);  /* set table at key `i' */
    }

    return 0;
}

static int SetDataFloatsToTable(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    unsigned int offset = luaL_checknumber(L, 1);
    unsigned int length = luaL_checknumber(L, 2);
    const char *data = luaL_checkstring(L, 3);
    luaL_checktype(L, 4, LUA_TTABLE);
    
    assert(length > 0);

    // Now we have the data, cast it to the union and write back out.
    unsigned int idx = 1;
    float *ptr = (float *)&data[offset];
    for( unsigned int i=0; i<length; i+= sizeof(float), ptr++)
    {
        float val = *ptr;
        lua_pushnumber(L, val);  /* value */
        lua_rawseti(L, 4, idx++);  /* set table at key `i' */
    }

    return 0;
}

static int SetDataIndexFloatsToTable(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    const char *data = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);
    luaL_checktype(L, 3, LUA_TTABLE);       // This is the indices tables. 
    unsigned int elements = luaL_checknumber(L, 4); // How many floats per index

    size_t indiceslen = lua_objlen(L, 3);
    int * idata = (int *)calloc(indiceslen, sizeof(int));    
    GetTableNumbersInt(L, 3, idata);

    // Now we have the data, cast it to the union and write back out.
    unsigned int idx = 1;
    float *ptr = (float *)&data[0];
    for( unsigned int i=0; i<indiceslen; i++)
    {
        for( int j=0; j< elements; j++)
        {
            float val = ptr[idata[i] * elements + j];
            lua_pushnumber(L, val);  /* value */
            lua_rawseti(L, 2, idx++);  /* set table at key `i' */
        }
    }

    free(idata);
    return 0;
}

static int SetBufferBytesFromTable(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    dmScript::LuaHBuffer *buffer = dmScript::CheckBuffer(L, 1);
    const char *streamname = luaL_checkstring(L, 2);
    luaL_checktype(L, 3, LUA_TTABLE);
    luaL_checktype(L, 4, LUA_TTABLE);

    float* bytes = 0x0;
    uint32_t count = 0;
    uint32_t components = 0;
    uint32_t stride = 0;
    dmBuffer::Result r = dmBuffer::GetStream(buffer->m_Buffer, dmHashString64(streamname), (void**)&bytes, &count, &components, &stride);

    if(components == 0 || count == 0) return 0;

    // This is very rudimentary.. will make nice later (maybe)    
    size_t indiceslen = lua_objlen(L, 3);
    int * idata = (int *)calloc(indiceslen, sizeof(int));    
    GetTableNumbersInt(L, 3, idata);

    size_t floatslen = lua_objlen(L, 4);
    float *floatdata = (float *)calloc(floatslen, sizeof(float));    
    GetTableNumbersFloat(L, 4, floatdata);

    if (r == dmBuffer::RESULT_OK) {
        for (int i = 0; i < count; ++i)
        {
            for (int c = 0; c < components; ++c)
            {
                bytes[c] = floatdata[idata[i] * components + c];
            }
            bytes += stride;
        }
    } else {
        // handle error
    }
    
    free(floatdata);
    free(idata);
    r = dmBuffer::ValidateBuffer(buffer->m_Buffer);
    return 0;
}

static int SetBufferBytes(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    dmScript::LuaHBuffer *buffer = dmScript::CheckBuffer(L, 1);
    const char *streamname = luaL_checkstring(L, 2);
    const uint8_t *bufferstring = (const uint8_t *)luaL_checkstring(L, 3);
    
    uint8_t* bytes = 0x0;
    uint32_t size = 0;
    uint32_t count = 0;
    uint32_t components = 0;
    uint32_t stride = 0;
    dmBuffer::Result r = dmBuffer::GetStream(buffer->m_Buffer, dmHashString64(streamname), (void**)&bytes, &count, &components, &stride);

    size_t idx = 0;
    if (r == dmBuffer::RESULT_OK) {
        for (uint32_t i = 0; i < count; ++i)
        {
            for (uint32_t c = 0; c < components; ++c)
            {
                bytes[c] = bufferstring[idx++];
            }
            bytes += stride;
        }
    } else {
        // handle error
    }
        
    r = dmBuffer::ValidateBuffer(buffer->m_Buffer);
    return 0;
}

static int BuildIndicesToTable(lua_State *L)
{
    DM_LUA_STACK_CHECK(L, 0);
    int start = luaL_checknumber(L, 1);
    int length = luaL_checknumber(L, 2);
    int type = luaL_checknumber(L, 3);
    luaL_checktype(L, 4, LUA_TTABLE);
    
    assert(length > 0);
    // Only support two types atm. Triangle list and Triangle strip.
    // type = 1 is triangle list
    // type = 2 is triangle strip

    if(type == 1) 
    {
        unsigned int idx = 1;
        for( int i=0; i<length; i++)
        {
            lua_pushnumber(L, i);  /* value */
            lua_rawseti(L, 4, idx++);  /* set table at key `i' */
        }
    }

    if(type == 2)
    {
        unsigned int idx = 1;
        for( int i=1; i<length-1; i++)
        {
            lua_pushnumber(L, i-1);  /* value */
            lua_rawseti(L, 4, idx++);  /* set table at key `i' */
            lua_pushnumber(L, i);  /* value */
            lua_rawseti(L, 4, idx++);  /* set table at key `i' */
            lua_pushnumber(L, i+1);  /* value */
            lua_rawseti(L, 4, idx++);  /* set table at key `i' */
        }
    }

    return 0;
}

static int LoadGltf(lua_State *L)
{
    const char * input_filename = luaL_checkstring(L, 1);
    bool dumpfile = false;
    int n = lua_gettop(L);
    if(n > 1) dumpfile = (luaL_checknumber(L, 2) == 1)?true:false;
    int ret = load_gltf(input_filename, dumpfile);

    lua_pushnumber(L, ret);
    return 1;
}

// Functions exposed to Lua
static const luaL_reg Module_methods[] =
{
    {"setbufferbytes", SetBufferBytes},
    {"setbufferbytesfromtable", SetBufferBytesFromTable},

    {"setdataindexfloatstotable", SetDataIndexFloatsToTable},
    {"setdatafloatstotable", SetDataFloatsToTable},
    {"setdataintstotable", SetDataIntsToTable},
    {"setdatashortstotable", SetDataShortsToTable},
    {"setdatabytestotable", SetDataBytesToTable},

    {"buildindicestotable", BuildIndicesToTable},

    {"addboundingbox", AddBoundingBox},
    {"raycasttobox", RaycastToBox},
    {"updateobb", UpdateOBB},

    {"loadgltf", LoadGltf},

    {"perlinnoise", PerlinNoise},    
    {0, 0}
};

static void LuaInit(lua_State* L)
{
    int top = lua_gettop(L);

    // Register lua names
    luaL_register(L, MODULE_NAME, Module_methods);

    lua_pop(L, 1);
    assert(top == lua_gettop(L));
}

dmExtension::Result AppInitializegltfloader(dmExtension::AppParams* params)
{
    dmLogInfo("AppInitializegltfloader\n");
    return dmExtension::RESULT_OK;
}

dmExtension::Result Initializegltfloader(dmExtension::Params* params)
{
    // Init Lua
    LuaInit(params->m_L);
    dmLogInfo("Registered %s Extension\n", MODULE_NAME);
    return dmExtension::RESULT_OK;
}

dmExtension::Result AppFinalizegltfloader(dmExtension::AppParams* params)
{
    dmLogInfo("AppFinalizegltfloader\n");
    return dmExtension::RESULT_OK;
}

dmExtension::Result Finalizegltfloader(dmExtension::Params* params)
{
    dmLogInfo("Finalizegltfloader\n");
    return dmExtension::RESULT_OK;
}

dmExtension::Result OnUpdategltfloader(dmExtension::Params* params)
{
    // dmLogInfo("OnUpdategltfloader\n");
    return dmExtension::RESULT_OK;
}

void OnEventgltfloader(dmExtension::Params* params, const dmExtension::Event* event)
{
    switch(event->m_Event)
    {
        case dmExtension::EVENT_ID_ACTIVATEAPP:
            dmLogInfo("OnEventgltfloader - EVENT_ID_ACTIVATEAPP\n");
            break;
        case dmExtension::EVENT_ID_DEACTIVATEAPP:
            dmLogInfo("OnEventgltfloader - EVENT_ID_DEACTIVATEAPP\n");
            break;
        case dmExtension::EVENT_ID_ICONIFYAPP:
            dmLogInfo("OnEventgltfloader - EVENT_ID_ICONIFYAPP\n");
            break;
        case dmExtension::EVENT_ID_DEICONIFYAPP:
            dmLogInfo("OnEventgltfloader - EVENT_ID_DEICONIFYAPP\n");
            break;
        case dmExtension::EVENT_ID_ENGINE_INITIALIZED:
            InitMeshBuilding(params->m_ResourceFactory, params->m_ConfigFile);
            break;
        case dmExtension::EVENT_ID_ENGINE_DELETE:
            DestroyMeshBuilding();
        default:
            dmLogWarning("OnEventgltfloader - Unknown event id %d\n", event->m_Event);
            break;
    }
}

// Defold SDK uses a macro for setting up extension entry points:
//
// DM_DECLARE_EXTENSION(symbol, name, app_init, app_final, init, update, on_event, final)

// gltfloader is the C++ symbol that holds all relevant extension data.
// It must match the name field in the `ext.manifest`
DM_DECLARE_EXTENSION(gltfloader, LIB_NAME, AppInitializegltfloader, AppFinalizegltfloader, Initializegltfloader, OnUpdategltfloader, OnEventgltfloader, Finalizegltfloader)

