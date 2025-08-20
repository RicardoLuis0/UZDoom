#pragma once

#include "zstring.h"
#include "tarray.h"

#include "shaderuniforms.h"

enum class PostProcessUniformType
{
	Undefined,
	Int,
	Float,
	Vec2,
	Vec3,
	Vec4
};

struct PostProcessUniformValue
{
	PostProcessUniformType Type = PostProcessUniformType::Undefined;
	double Values[4] = { 0.0, 0.0, 0.0, 0.0 };
};

struct PostProcessShader
{
	FString Target;
	FString ShaderLumpName;
	int ShaderVersion = 0;

	FString Name;
	bool Enabled = false;

	UserUniforms Uniforms;
	TMap<FString, FString> Textures;
};

extern TArray<PostProcessShader> PostProcessShaders;

