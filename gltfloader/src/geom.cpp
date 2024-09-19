
#include <stdlib.h>
#include <string.h>
#include <vector>

// include the Defold SDK
#include <dmsdk/sdk.h>

#include "geom.h"

// List of all the objects bounding boxes (will put this in a lqdb for fast raycasting)
//   Initially use AABB (much faster) use OBox later + lqdb
static std::vector<OBB>     g_bounds;

Vec3 normalize(Vec3 a)
{
	if( a.x == 0.0f && a.y == 0.0f && a.z == 0.0f)
		return a;

    float vlen = sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
    float invlen = 1.0f / vlen;
	return Vec3( a.x * invlen, a.y * invlen, a.z * invlen);
}

float dot(const Vec3 &a, const Vec3 &b) {
    return (a.x * b.x + a.y * b.y + a.z * b.z);
}


// -- http://gamedev.stackexchange.com/a/18459
// -- ray.position  is a vec3
// -- ray.direction is a vec3
// -- aabb.min      is a vec3
// -- aabb.max      is a vec3
bool intersect( Ray ray, AABB aabb, float *distance)
{
    Vec3 dir     = normalize(ray.direction);
	Vec3 dirfrac = Vec3(
		1.0f / dir.x,
		1.0f / dir.y,
		1.0f / dir.z
	);

	float t1 = (aabb.min.x - ray.position.x) * dirfrac.x;
	float t2 = (aabb.max.x - ray.position.x) * dirfrac.x;
	float t3 = (aabb.min.y - ray.position.y) * dirfrac.y;
	float t4 = (aabb.max.y - ray.position.y) * dirfrac.y;
	float t5 = (aabb.min.z - ray.position.z) * dirfrac.z;
	float t6 = (aabb.max.z - ray.position.z) * dirfrac.z;

	float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
	float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

	// -- ray is intersecting AABB, but whole AABB is behind us
	if (tmax < 0.0f) return false;

	// -- ray does not intersect AABB
	if (tmin > tmax) return false;

	// -- Return collision po int and distance from ray origin
    float dx = ray.position.x + ray.direction.x * tmin;
    float dy = ray.position.y + ray.direction.y * tmin;
    float dz = ray.position.z + ray.direction.z * tmin;
	*distance = sqrt( dx * dx + dy * dy + dz * dz);
    return true;
}

bool intersectOBB( Ray ray, OBB obb, float *distance)
{
    const Vec3& O 		= ray.position; // Line origin    
    const Vec3& D 		= ray.direction; // Line direction    
    
    const Vec3& C  		= obb.center;   // Box center    
    const Vec3 bbmin 	= obb.axis[0];
    const Vec3 bbmax 	= obb.axis[1];
    const Vec3& e  		= obb.extents;  // Box extents    

	// Intersection method from Real-Time Rendering and Essential Mathematics for Games
	
	float tMin = 0.0f;
	float tMax = 100000.0f;

	Vec3 delta = Vec3(C.x - O.x, C.y - O.y, C.z - O.z);

	// Test intersection with the 2 planes perpendicular to the OBB's X axis
	{
		Vec3 xaxis(obb.mat[0][0], obb.mat[0][1], obb.mat[0][2]);
		float e = dot(xaxis, delta);
		float f = dot(ray.direction, xaxis);

		if ( fabs(f) > 0.001f ){ // Standard case

			float t1 = (e+bbmin.x)/f; // Intersection with the "left" plane
			float t2 = (e+bbmax.x)/f; // Intersection with the "right" plane
			// t1 and t2 now contain distances betwen ray origin and ray-plane intersections

			// We want t1 to represent the nearest intersection, 
			// so if it's not the case, invert t1 and t2
			if (t1>t2){
				float w=t1;t1=t2;t2=w; // swap t1 and t2
			}

			// tMax is the nearest "far" intersection (amongst the X,Y and Z planes pairs)
			if ( t2 < tMax )
				tMax = t2;
			// tMin is the farthest "near" intersection (amongst the X,Y and Z planes pairs)
			if ( t1 > tMin )
				tMin = t1;

			// And here's the trick :
			// If "far" is closer than "near", then there is NO intersection.
			// See the images in the tutorials for the visual explanation.
			if (tMax < tMin )
				return false;

		}else{ // Rare case : the ray is almost parallel to the planes, so they don't have any "intersection"
			if(-e+bbmin.x > 0.0f || -e+bbmax.x < 0.0f)
				return false;
		}
	}


	// Test intersection with the 2 planes perpendicular to the OBB's Y axis
	// Exactly the same thing than above.
	{
		Vec3 yaxis(obb.mat[1][0], obb.mat[1][1], obb.mat[1][2]);
		float e = dot(yaxis, delta);
		float f = dot(ray.direction, yaxis);

		if ( fabs(f) > 0.001f ){

			float t1 = (e+bbmin.y)/f;
			float t2 = (e+bbmax.y)/f;

			if (t1>t2){float w=t1;t1=t2;t2=w;}

			if ( t2 < tMax )
				tMax = t2;
			if ( t1 > tMin )
				tMin = t1;
			if (tMin > tMax)
				return false;

		}else{
			if(-e+bbmin.y > 0.0f || -e+bbmax.y < 0.0f)
				return false;
		}
	}


	// Test intersection with the 2 planes perpendicular to the OBB's Z axis
	// Exactly the same thing than above.
	{
		Vec3 zaxis(obb.mat[2][0], obb.mat[2][1], obb.mat[2][2]);
		float e = dot(zaxis, delta);
		float f = dot(ray.direction, zaxis);

		if ( fabs(f) > 0.001f ){

			float t1 = (e+bbmin.z)/f;
			float t2 = (e+bbmax.z)/f;

			if (t1>t2){float w=t1;t1=t2;t2=w;}

			if ( t2 < tMax )
				tMax = t2;
			if ( t1 > tMin )
				tMin = t1;
			if (tMin > tMax)
				return false;

		}else{
			if(-e+bbmin.z > 0.0f || -e+bbmax.z < 0.0f)
				return false;
		}
	}

	*distance = tMin;
	return true;
}

static int AddBoundingBox(lua_State *L)
{
    OBB obb;
    
    DM_LUA_STACK_CHECK(L, 1);
    // Min
    dmVMath::Vector3    vmin = *dmScript::CheckVector3(L, 1);
    // Max
    dmVMath::Vector3    vmax = *dmScript::CheckVector3(L, 2);

    // Tag used when doing hits
    uint64_t tag = dmScript::CheckHash(L, 3);

    dmVMath::Vector3 center = (vmax - vmin) * 0.5 + vmin;
    dmVMath::Vector3 extents;
    extents[0] = (vmax[0] - vmin[0]) * 0.5;
    extents[1] = (vmax[1] - vmin[1]) * 0.5;
    extents[2] = (vmax[2] - vmin[2]) * 0.5;

    // obb.axis[0]     = Vec3(1,0,0);
    // obb.axis[1]     = Vec3(0,1,0);
    // obb.axis[2]     = Vec3(0,0,1);

    obb.axis[0]     = Vec3(vmin[0],vmin[1],vmin[2]);
    obb.axis[1]     = Vec3(vmax[0],vmax[1],vmax[2]);

    obb.center = Vec3(center[0], center[1], center[2]);
    obb.extents = Vec3(extents[0], extents[1], extents[2]);
    obb.tag = tag;
    g_bounds.push_back(obb);
    lua_pushnumber(L, g_bounds.size() - 1);
    return 1;
} 

// Recal OOBB - rotations and translations may change min and max values.
OBB MultWorld( OBB obb )
{
    OBB out;
    // for(i=0; i<3; ++i) {
    //     dmVMath::Vector4 res = obb.mat * dmVMath::Vector4(obb.axis[i].x, obb.axis[i].y, obb.axis[i].z);
    //     out.axis[i] = Vec3(res[0], res[1], res[2]);
    // }
    out.mat = obb.mat;
    out.axis[0] = obb.axis[0];
    out.axis[1] = obb.axis[1];
    // dmVMath::Vector4 res = obb.mat * dmVMath::Vector4(obb.center.x, obb.center.y, obb.center.z, 1.0);
    out.center = Vec3(obb.mat[3][0], obb.mat[3][1], obb.mat[3][2]);
    out.extents = obb.extents;
    out.tag = obb.tag;
    return out;
}

static int RaycastToBox( lua_State *L)
{
    DM_LUA_STACK_CHECK(L, 3);
    // Origin
    float x1 = luaL_checknumber(L, 1);
    float y1 = luaL_checknumber(L, 2);
    float z1 = luaL_checknumber(L, 3);
    // Dir
    float x2 = luaL_checknumber(L, 4);
    float y2 = luaL_checknumber(L, 5);
    float z2 = luaL_checknumber(L, 6);

    struct Ray ray(Vec3(x1, y1, z1), Vec3(x2, y2, z2));

    // Go through boxes checking hits. Closest hit wins!
    float distance = FLT_MAX;
    float closest = FLT_MAX;
    int hitbox = -1;
    float hitpoint[3];
    for(int i=0; i<g_bounds.size(); ++i)
    {
        OBB testbox = MultWorld(g_bounds[i]);
        if( intersectOBB(ray, testbox, &distance ) ) {
            if(distance < closest) {
                closest = distance;
                hitbox = i;
            }
        }
    }
    
    if(closest == FLT_MAX) 
    {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
    }
    else 
    {
        lua_pushnumber(L, closest);
        dmScript::PushHash(L, g_bounds[hitbox].tag);
        hitpoint[0] = ray.position.x + ray.direction.x * closest;
        hitpoint[1] = ray.position.y + ray.direction.y * closest;
        hitpoint[2] = ray.position.z + ray.direction.z * closest;
        dmScript::PushVector3( L, dmVMath::Vector3( hitpoint[0], hitpoint[1], hitpoint[2]) );
    }
    return 3;
}

static int UpdateOBB( lua_State *L )
{
    DM_LUA_STACK_CHECK(L, 0);
    int     index = luaL_checknumber(L, 1);
    dmVMath::Matrix4 world    = *dmScript::CheckMatrix4(L, 2);

    // Need to do some index checking here.
    g_bounds[index].mat = world;
    return 0;
}


static int SEED = 0;

static int hash[] = {208,34,231,213,32,248,233,56,161,78,24,140,71,48,140,254,245,255,247,247,40,
    185,248,251,245,28,124,204,204,76,36,1,107,28,234,163,202,224,245,128,167,204,
    9,92,217,54,239,174,173,102,193,189,190,121,100,108,167,44,43,77,180,204,8,81,
    70,223,11,38,24,254,210,210,177,32,81,195,243,125,8,169,112,32,97,53,195,13,
    203,9,47,104,125,117,114,124,165,203,181,235,193,206,70,180,174,0,167,181,41,
    164,30,116,127,198,245,146,87,224,149,206,57,4,192,210,65,210,129,240,178,105,
    228,108,245,148,140,40,35,195,38,58,65,207,215,253,65,85,208,76,62,3,237,55,89,
    232,50,217,64,244,157,199,121,252,90,17,212,203,149,152,140,187,234,177,73,174,
    193,100,192,143,97,53,145,135,19,103,13,90,135,151,199,91,239,247,33,39,145,
    101,120,99,3,186,86,99,41,237,203,111,79,220,135,158,42,30,154,120,67,87,167,
    135,176,183,191,253,115,184,21,233,58,129,233,142,39,128,211,118,137,139,255,
    114,20,218,113,154,27,127,246,250,1,8,198,250,209,92,222,173,21,88,102,219
};

int noise2(int x, int y)
{
    int tmp = hash[(y + SEED) % 256];
    return hash[(tmp + x) % 256];
}

float lin_inter(float x, float y, float s)
{
    return x + s * (y-x);
}

float smooth_inter(float x, float y, float s)
{
    return lin_inter(x, y, s * s * (3-2*s));
}

float noise2d(float x, float y)
{
    int x_int = x;
    int y_int = y;
    float x_frac = x - x_int;
    float y_frac = y - y_int;
    int s = noise2(x_int, y_int);
    int t = noise2(x_int+1, y_int);
    int u = noise2(x_int, y_int+1);
    int v = noise2(x_int+1, y_int+1);
    float low = smooth_inter(s, t, x_frac);
    float high = smooth_inter(u, v, x_frac);
    return smooth_inter(low, high, y_frac);
}

float perlin2d(float x, float y, float freq, int depth)
{
    float xa = x*freq;
    float ya = y*freq;
    float amp = 1.0;
    float fin = 0;
    float div = 0.0;

    int i;
    for(i=0; i<depth; i++)
    {
        div += 256 * amp;
        fin += noise2d(xa, ya) * amp;
        amp /= 2;
        xa *= 2;
        ya *= 2;
    }

    return fin/div;
}

static int PerlinNoise( lua_State *L )
{
    DM_LUA_STACK_CHECK(L, 1);
    float x = luaL_checknumber(L, 1);
    float y = luaL_checknumber(L, 2);
    float freq = luaL_checknumber(L, 3);
    int depth = luaL_checknumber(L, 4);
    
    lua_pushnumber(L, perlin2d( x, y, freq, depth ));
    return 1;
}
