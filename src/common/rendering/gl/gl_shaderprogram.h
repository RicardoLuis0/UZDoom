
#pragma once

#include "gl_system.h"
#include "gl_shader.h"
#include "hwrenderer/postprocessing/hw_postprocess.h"
#include "hwrenderer/data/shaderuniforms.h"

class UniformBlockDecl
{
public:
	static FString Create(const char *name, const std::vector<UniformFieldDesc> &fields, int bindingpoint)
	{
		FString decl;
		FString layout;
		if (bindingpoint == -1)
		{
			layout = "push_constant";
		}
		else if (screen->glslversion < 4.20)
		{
			layout = "std140";
		}
		else
		{
			layout.Format("std140, binding = %d", bindingpoint);
		}
		decl.Format("layout(%s) uniform %s\n{\n", layout.GetChars(), name);
		for (size_t i = 0; i < fields.size(); i++)
		{
			decl.AppendFormat("\t%s %s;\n", GetTypeStr(fields[i].Type), fields[i].Name);
		}
		decl += "};\n";

		return decl;
	}
};

template<typename T, int bindingpoint>
class ShaderUniforms
{
public:
	ShaderUniforms()
	{
		memset(&Values, 0, sizeof(Values));
	}

	~ShaderUniforms()
	{
		if (mBuffer != nullptr)
			delete mBuffer;
	}

	int BindingPoint() const
	{
		return bindingpoint;
	}

	FString CreateDeclaration(const char *name, const std::vector<UniformFieldDesc> &fields)
	{
		mFields = fields;
		return UniformBlockDecl::Create(name, fields, bindingpoint);
	}

	void Init()
	{
		if (mBuffer == nullptr)
			mBuffer = screen->CreateDataBuffer(bindingpoint, false, false);
	}

	void SetData()
	{
		if (mBuffer != nullptr)
			mBuffer->SetData(sizeof(T), &Values, BufferUsageType::Static);
	}

	IDataBuffer* GetBuffer() const
	{
		// OpenGL needs to mess around with this in ways that should not be part of the interface.
		return mBuffer;
	}

	T *operator->() { return &Values; }
	const T *operator->() const { return &Values; }

	T Values;

private:
	ShaderUniforms(const ShaderUniforms &) = delete;
	ShaderUniforms &operator=(const ShaderUniforms &) = delete;

	IDataBuffer *mBuffer = nullptr;
	std::vector<UniformFieldDesc> mFields;
};

namespace OpenGLRenderer
{

class FShaderProgram : public PPShaderBackend
{
public:
	FShaderProgram();
	~FShaderProgram();

	enum ShaderType
	{
		Vertex,
		Fragment,
		NumShaderTypes
	};

	void Compile(ShaderType type, const char *lumpName, const char *defines, int maxGlslVersion);
	void Compile(ShaderType type, const char *name, const FString &code, const char *defines, int maxGlslVersion);
	void Link(const char *name);
	void SetUniformBufferLocation(int index, const char *name);

	void Bind();

	GLuint Handle() { return mProgram; }
	//explicit operator bool() const { return mProgram != 0; }

	std::unique_ptr<IDataBuffer> Uniforms;

private:
	FShaderProgram(const FShaderProgram &) = delete;
	FShaderProgram &operator=(const FShaderProgram &) = delete;

	void CompileShader(ShaderType type);
	FString PatchShader(ShaderType type, const FString &code, const char *defines, int maxGlslVersion);

	void CreateShader(ShaderType type);
	FString GetShaderInfoLog(GLuint handle);
	FString GetProgramInfoLog(GLuint handle);

	GLuint mProgram = 0;
	GLuint mShaders[NumShaderTypes];
	FString mShaderSources[NumShaderTypes];
	FString mShaderNames[NumShaderTypes];
	TArray<std::pair<FString, int>> samplerstobind;
};

class FPresentShaderBase
{
public:
	virtual ~FPresentShaderBase() {}
	virtual void Bind() = 0;

	ShaderUniforms<PresentUniforms, POSTPROCESS_BINDINGPOINT> Uniforms;

protected:
	virtual void Init(const char * vtx_shader_name, const char * program_name);
	std::unique_ptr<FShaderProgram> mShader;
};

class FPresentShader : public FPresentShaderBase
{
public:
	void Bind() override;

};

class FPresent3DCheckerShader : public FPresentShaderBase
{
public:
	void Bind() override;
};

class FPresent3DColumnShader : public FPresentShaderBase
{
public:
	void Bind() override;
};

class FPresent3DRowShader : public FPresentShaderBase
{
public:
	void Bind() override;
};

class FShadowMapShader
{
public:
	void Bind();

	ShaderUniforms<ShadowMapUniforms, POSTPROCESS_BINDINGPOINT> Uniforms;

private:
	std::unique_ptr<FShaderProgram> mShader;
};

}