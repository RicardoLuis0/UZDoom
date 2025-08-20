#pragma once

#include <type_traits>
#include <vector>
#include <map>
#include <cstddef>
#include "hwrenderer/data/buffers.h"
#include "tarray.h"
#include "zstring.h"

enum
{
	LIGHTBUF_BINDINGPOINT = 1,
	POSTPROCESS_BINDINGPOINT = 2,
	VIEWPOINT_BINDINGPOINT = 3,
	LIGHTNODES_BINDINGPOINT = 4,
	LIGHTLINES_BINDINGPOINT = 5,
	LIGHTLIST_BINDINGPOINT = 6,
	BONEBUF_BINDINGPOINT = 7
};

enum class UniformType
{
	Undefined,
	Int,
	UInt,
	Float,
	Vec2,
	Vec3,
	Vec4,
	IVec2,
	IVec3,
	IVec4,
	UVec2,
	UVec3,
	UVec4,
	Mat4
};

static inline const char *GetTypeStr(UniformType type)
{
	switch (type)
	{
	default:
	case UniformType::Int: return "int";
	case UniformType::UInt: return "uint";
	case UniformType::Float: return "float";
	case UniformType::Vec2: return "vec2";
	case UniformType::Vec3: return "vec3";
	case UniformType::Vec4: return "vec4";
	case UniformType::IVec2: return "ivec2";
	case UniformType::IVec3: return "ivec3";
	case UniformType::IVec4: return "ivec4";
	case UniformType::UVec2: return "uvec2";
	case UniformType::UVec3: return "uvec3";
	case UniformType::UVec4: return "uvec4";
	case UniformType::Mat4: return "mat4";
	}
}

struct UserUniformValue
{
	UniformType Type = UniformType::Undefined;
	double Values[4] = { 0.0, 0.0, 0.0, 0.0 };
};

struct UniformFieldDesc
{
	FString Name;
	UniformType Type;
	std::size_t Offset;
};

struct VaryingFieldDesc
{
	FString Name;
	FString Property;
	UniformType Type;
};

struct UniformField
{
	UniformType Type = UniformType::Undefined;
	void * Value = nullptr;
};

class UniformStructHolder
{
public:
	UniformStructHolder()
	{
	}

	UniformStructHolder(const UniformStructHolder &src)
	{
		*this = src;
	}

	~UniformStructHolder()
	{
		Clear();
	}

	UniformStructHolder &operator=(const UniformStructHolder &src)
	{
		Clear();

		if(src.alloc)
		{
			alloc = true;
			sz = src.sz;
			addr = new uint8_t[sz];
			memcpy((uint8_t*)addr, src.addr, sz);
		}
		else
		{
			sz = src.sz;
			addr = src.addr;
		}

		return *this;
	}

	void Clear()
	{
		if(alloc)
		{
			delete[] addr;
		}
		alloc = false;
		addr = nullptr;
		sz = 0;
	}

	template<typename T>
	void Set(const T *v)
	{
		Clear();

		sz = sizeof(T);
		addr = reinterpret_cast<const uint8_t*>(v);
	}

	template<typename T>
	void Set(const T &v, typename std::enable_if<!std::is_pointer_v<T>, bool>::type enabled = true)
	{
		static_assert(std::is_trivially_copyable_v<T>);

		alloc = true;
		sz = sizeof(T);
		addr = new uint8_t[sizeof(T)];
		memcpy((uint8_t*)addr, &v, sizeof(T));
	}

	bool alloc = false;
	size_t sz = 0;
	const uint8_t * addr = nullptr;
};

class UserUniforms
{
	void AddUniformField(size_t &offset, const FString &name, UniformType type, size_t fieldsize, size_t alignment = 0);
	void BuildStruct(const TMap<FString, UserUniformValue> &Uniforms);
public:
	UserUniforms() = default;
	UserUniforms(const TMap<FString, UserUniformValue> &Uniforms)
	{
		LoadUniforms(Uniforms);
	}

	~UserUniforms()
	{
		if(UniformStruct) delete[] UniformStruct;
	}

	//must only be called once
	void LoadUniforms(const TMap<FString, UserUniformValue> &Uniforms);
	void WriteUniforms(UniformStructHolder &Data) const;


	int UniformStructSize = 0;
	uint8_t * UniformStruct = nullptr;

	UniformField GetField(const FString &name);

	std::vector<UniformFieldDesc> Fields;
	TMap<FString, size_t> FieldNames;
};