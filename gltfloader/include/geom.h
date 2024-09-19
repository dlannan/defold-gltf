#ifndef _GEOM_HEADER_
#define _GEOM_HEADER_

#include <string>

typedef struct Vec3 {
    float x, y, z;
    Vec3( float _x = 0.0f, float _y = 0.0f, float _z = 0.0f):x(_x),y(_y),z(_z) {};
} Vec3;

typedef struct Vec4 {
    float x, y, z, w;
    Vec4( float _x = 0.0f, float _y = 0.0f, float _z = 0.0f, float _w = 1.0f):x(_x),y(_y),z(_z),w(_w) {};
} Vec4;


typedef struct Ray {
    Vec3 position;
    Vec3 direction;
    Ray( Vec3 _pos, Vec3 _dir):position(_pos), direction(_dir) {}
} Ray;

typedef struct AABB {
    Vec3        min;
    Vec3        max;
	uint64_t    tag;
    dmVMath::Matrix4    mat;
	AABB( Vec3 _min, Vec3 _max, uint64_t _tag):min(_min), max(_max), tag(_tag){}
} AABB;

typedef struct OBB {
    Vec3        center;
    Vec3        axis[3];
    Vec3        extents;
	uint64_t    tag;
    dmVMath::Matrix4    mat;
} OBB;

Vec3 normalize(Vec3 a);

float dot(const Vec3 &a, const Vec3 &b);

bool intersect( Ray ray, AABB aabb, float *distance);
bool intersectOBB( Ray ray, OBB obb, float *distance);

int AddBoundingBox(lua_State *L);
OBB MultWorld( OBB obb );
int RaycastToBox( lua_State *L);
int UpdateOBB( lua_State *L );
int PerlinNoise( lua_State *L );

#endif // _GEOM_HEADER_
