
#ifndef _TINY_GLTF_DUMP_H_
#define _TINY_GLTF_DUMP_H_
//
// TODO(syoyo): Print extensions and extras for each glTF object.
//

#include <string>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream> 

std::string GetFilePathExtension(const std::string &FileName);
std::string PrintMode(int mode);
std::string PrintTarget(int target);
std::string PrintType(int ty);
std::string PrintComponentType(int ty);
std::string PrintWrapMode(int mode);
std::string PrintFilterMode(int mode);
std::string PrintIntArray(const std::vector<int> &arr);
std::string PrintFloatArray(const std::vector<double> &arr);
std::string Indent(const int indent);
std::string PrintParameterValue(const tinygltf::Parameter &param);
std::string PrintValue(const std::string &name, const tinygltf::Value &value, const int indent, const bool tag);

void DumpNode(const tinygltf::Node &node, int indent);
void DumpStringIntMap(const std::map<std::string, int> &m, int indent);
void DumpExtensions(const tinygltf::ExtensionMap &extension, const int indent);
void DumpPrimitive(const tinygltf::Primitive &primitive, int indent);
void DumpTextureInfo(const tinygltf::TextureInfo &texinfo, const int indent);
void DumpNormalTextureInfo(const tinygltf::NormalTextureInfo &texinfo, const int indent);
void DumpOcclusionTextureInfo(const tinygltf::OcclusionTextureInfo &texinfo, const int indent);
void DumpPbrMetallicRoughness(const tinygltf::PbrMetallicRoughness &pbr, const int indent);
void Dump(const tinygltf::Model &model);

#endif //_TINY_GLTF_DUMP_H_