
#include "filesystem.h"
#include "cmdlib.h"
#include "model_iqm.h"
#include "texturemanager.h"
#include "modelrenderer.h"
#include "engineerrors.h"
#include "dobject.h"
#include "bonecomponents.h"
#include "i_time.h"

#include <bit>

IMPLEMENT_CLASS(DBoneComponents, false, false);


IQMModel::IQMModel()
{
}

IQMModel::~IQMModel()
{
}

struct iqm_header_t
{
	uint64_t magic1;
	uint64_t magic2;

	uint32_t version;
	uint32_t filesize;
	uint32_t flags;
	uint32_t num_text, ofs_text;
	uint32_t num_meshes, ofs_meshes;
	uint32_t num_vertexarrays, num_vertices, ofs_vertexarrays;
	uint32_t num_triangles, ofs_triangles, ofs_adjacency;
	uint32_t num_joints, ofs_joints;
	uint32_t num_poses, ofs_poses;
	uint32_t num_anims, ofs_anims;
	uint32_t num_frames, num_framechannels, ofs_frames, ofs_bounds;
	uint32_t num_comment, ofs_comment;
	uint32_t num_extensions, ofs_extensions;
};

static_assert(sizeof(iqm_header_t) == (4 * 32));

struct iqm_mesh_t
{
	uint32_t name;
	uint32_t material;
	uint32_t first_vertex, num_vertices;
	uint32_t first_triangle, num_triangles;
};

static_assert(sizeof(iqm_mesh_t) == (4 * 6));
//static_assert(sizeof(iqm_mesh_t) == sizeof(IQMMesh)); -- IQMMesh isn't initialized via memcpy, so this isn't required

struct iqm_vertex_array_t
{
	uint32_t type;
	uint32_t flags;
	uint32_t format;
	uint32_t size;
	uint32_t offset;
};

static_assert(sizeof(iqm_vertex_array_t) == (4 * 5));
static_assert(sizeof(iqm_vertex_array_t) == sizeof(IQMVertexArray));

struct iqm_triangle_t
{
	uint32_t vertex[3];
};

static_assert(sizeof(iqm_triangle_t) == (4 * 3));
static_assert(sizeof(iqm_triangle_t) == sizeof(IQMTriangle));

struct iqm_adjacency_t
{
	uint32_t triangle[3];
};

static_assert(sizeof(iqm_adjacency_t) == (4 * 3));
static_assert(sizeof(iqm_adjacency_t) == sizeof(IQMAdjacency));

//static_assert(sizeof(iqm_triangle_t) == sizeof(iqm_adjacency_t));

struct iqm_joint_t
{
	uint32_t name;
	int32_t parent;
	FVector3 translation;
	FVector4 rotation;
	FVector3 scale;
};

static_assert(sizeof(iqm_joint_t) == (4 * 12));
//static_assert(sizeof(iqm_joint_t) == sizeof(IQMJoint)); -- IQMJoint isn't initialized via memcpy, so this isn't required

struct iqm_pose_t
{
	int32_t parent;
	uint32_t channelmask;
	float channeloffset[10];
	float channelscale[10];
};

static_assert(sizeof(iqm_pose_t) == (4 * 22));
static_assert(sizeof(iqm_pose_t) == sizeof(IQMPose));

enum iqm_anim_flags_e
{
	IQM_LOOP = 1<<0
};

struct iqm_anim_t
{
	uint32_t name;
	uint32_t first_frame, num_frames;
	float framerate;
	uint32_t flags;
};

static_assert(sizeof(iqm_anim_t) == (4 * 5));
//static_assert(sizeof(iqm_anim_t) == sizeof(IQMAnim)); -- IQMAnim isn't initialized via memcpy, so this isn't required

struct iqm_bounds_t
{
	FVector3 bbmins, bbmaxs;
	float xyradius, radius;
};

static_assert(sizeof(iqm_bounds_t) == (4 * 8));
static_assert(sizeof(iqm_bounds_t) == sizeof(IQMBounds));

constexpr uint64_t magic_to_bytes(const char * s)
{
	return
		(uint64_t(s[0]) <<  0)
		| (uint64_t(s[1]) <<  8)
		| (uint64_t(s[2]) << 16)
		| (uint64_t(s[3]) << 24)
		| (uint64_t(s[4]) << 32)
		| (uint64_t(s[5]) << 40)
		| (uint64_t(s[6]) << 48)
		| (uint64_t(s[7]) << 56);
}

constexpr uint64_t magic1 = magic_to_bytes("INTERQUA");
constexpr uint64_t magic2 = magic_to_bytes("KEMODEL\0");

inline unsigned mask_bits(unsigned m)
{ // count bits in the 10-bit iqm mask
	return  m&1
		+ ((m&2) >> 1)
		+ ((m&4) >> 2)
		+ ((m&8) >> 3)
		+ ((m&16) >> 4)
		+ ((m&32) >> 5)
		+ ((m&64) >> 6)
		+ ((m&128) >> 7)
		+ ((m&256) >> 8)
		+ ((m&512) >> 9);
}

uint64_t iqm_total_load_time_ns = 0;

bool IQMModel::Load(const char* path, int lumpnum, const char* buffer, int length)
{
	mLumpNum = lumpnum;

	uint64_t loadStartTime = I_nsTime();

	if(length < sizeof(iqm_header_t))
	{
		return false;
	}

	iqm_header_t h;

	memcpy(&h, buffer, sizeof(iqm_header_t));

	if (h.magic1 != magic1 || h.magic2 != magic2 || h.version != 2 || h.num_text == 0)
		return false;

	/*if (num_joints <= 0)
	{
		Printf("Invalid model: \"%s%s\", no joint data is present\n", path, fileSystem.GetLongName(mLumpNum).GetChars());
		return false;
	}*/

	const char * text = buffer + h.ofs_text;

	Meshes.Resize(h.num_meshes);
	Triangles.Resize(h.num_triangles);
	Adjacency.Resize(h.num_triangles);
	Joints.Resize(h.num_joints);
	Poses.Resize(h.num_poses);
	Anims.Resize(h.num_anims);
	Bounds.Resize(h.num_frames);
	VertexArrays.Resize(h.num_vertexarrays);
	NumVertices = h.num_vertices;

	{
		iqm_mesh_t * m = (iqm_mesh_t *) (buffer + h.ofs_meshes);
		for (uint32_t i = 0; i < h.num_meshes; i++, m++)
		{
			if(m->name >= h.num_text || m->material >= h.num_text)
			{
				return false;
			}
			Meshes[i].Name = FString(text + m->name);
			Meshes[i].Material = FString(text + m->material);
			Meshes[i].FirstVertex = m->first_vertex;
			Meshes[i].NumVertices = m->num_vertices;
			Meshes[i].FirstTriangle = m->first_triangle;
			Meshes[i].NumTriangles = m->num_triangles;
			Meshes[i].Skin = LoadSkin(path, Meshes[i].Material.GetChars());
		}
	}

	{

		size_t tri_adj_size = sizeof(iqm_triangle_t) * h.num_triangles;

		if((h.ofs_triangles + tri_adj_size) > length || (h.ofs_adjacency + tri_adj_size) > length)
		{
			return false;
		}

		memcpy(Triangles.Data(), buffer + h.ofs_triangles, tri_adj_size);
		memcpy(Adjacency.Data(), buffer + h.ofs_adjacency, tri_adj_size);
	}

	{
		iqm_joint_t * j = (iqm_joint_t *) (buffer + h.ofs_joints);
		for (uint32_t i = 0; i < h.num_joints; i++, j++)
		{
			if(j->name >= h.num_text)
			{
				return false;
			}
			Joints[i].Name = FString(text + j->name);
			Joints[i].Parent = j->parent;
			Joints[i].Translate = j->translation;
			Joints[i].Quaternion = j->rotation;
			Joints[i].Scale = j->scale;
			Joints[i].Quaternion.MakeUnit();
		}
	}
		
	{
		size_t pose_size = sizeof(iqm_pose_t) * h.num_poses;

		if((h.ofs_poses + pose_size) > length)
		{
			return false;
		}

		memcpy(Poses.Data(), buffer + h.ofs_poses, pose_size);
	}
		
	{
		iqm_anim_t * a = (iqm_anim_t *) (buffer + h.ofs_anims);
		for (uint32_t i = 0; i < h.num_anims; i++, a++)
		{
			if(a->name >= h.num_text)
			{
				return false;
			}
			Anims[i].Name = FString(text + a->name);
			Anims[i].FirstFrame = a->first_frame;
			Anims[i].NumFrames = a->num_frames;
			Anims[i].Framerate = a->framerate;
			Anims[i].Loop = a->flags & IQM_LOOP;
		}
	}

	{
		baseframe.Resize(h.num_joints);
		inversebaseframe.Resize(h.num_joints);

		for (uint32_t i = 0; i < h.num_joints; i++)
		{
			const IQMJoint& j = Joints[i];

			VSMatrix m, invm;
			m.loadIdentity();
			m.translate(j.Translate.X, j.Translate.Y, j.Translate.Z);
			m.multQuaternion(j.Quaternion);
			m.scale(j.Scale.X, j.Scale.Y, j.Scale.Z);
			m.inverseMatrix(invm);
			if (j.Parent >= 0)
			{
				baseframe[i] = baseframe[j.Parent];
				baseframe[i].multMatrix(m);
				inversebaseframe[i] = invm;
				inversebaseframe[i].multMatrix(inversebaseframe[j.Parent]);
			}
			else
			{
				baseframe[i] = m;
				inversebaseframe[i] = invm;
			}			
		}
	}

	{
		size_t bounds_size = sizeof(iqm_bounds_t) * h.num_anims;

		if((h.ofs_bounds + bounds_size) > length)
		{
			return false;
		}

		memcpy(Bounds.Data(), buffer + h.ofs_bounds, bounds_size);

		size_t va_size = sizeof(iqm_vertex_array_t) * h.num_vertexarrays;

		if((h.ofs_vertexarrays + va_size) > length)
		{
			return false;
		}

		memcpy(VertexArrays.Data(), buffer + h.ofs_vertexarrays, va_size);
	}

	TRSData.Resize(h.num_frames * h.num_poses);

	uint16_t * framedata = (uint16_t *) (buffer + h.ofs_frames);

	uint16_t * limit = (uint16_t *) (buffer + length);

	for (uint32_t i = 0; i < h.num_frames; i++)
	{
		for (uint32_t j = 0; j < h.num_poses; j++)
		{
			const IQMPose& p = Poses[j];

			FVector3 translate = *(FVector3*) &p.ChannelOffset[0];
			FVector4 quaternion = *(FVector4*) &p.ChannelOffset[3];
			FVector3 scale = *(FVector3*) &p.ChannelOffset[7];

			if(framedata + mask_bits(p.ChannelMask) > limit)
			{
				return false;
			}

			if (p.ChannelMask & 0x01) translate.X += *(framedata++) * p.ChannelScale[0];
			if (p.ChannelMask & 0x02) translate.Y += *(framedata++) * p.ChannelScale[1];
			if (p.ChannelMask & 0x04) translate.Z += *(framedata++) * p.ChannelScale[2];
				
			if (p.ChannelMask & 0x08) quaternion.X += *(framedata++) * p.ChannelScale[3];
			if (p.ChannelMask & 0x10) quaternion.Y += *(framedata++) * p.ChannelScale[4];
			if (p.ChannelMask & 0x20) quaternion.Z += *(framedata++) * p.ChannelScale[5];
			if (p.ChannelMask & 0x40) quaternion.W += *(framedata++) * p.ChannelScale[6];

			if (p.ChannelMask & 0x080) scale.X += *(framedata++) * p.ChannelScale[7];
			if (p.ChannelMask & 0x100) scale.Y += *(framedata++) * p.ChannelScale[8];
			if (p.ChannelMask & 0x200) scale.Z += *(framedata++) * p.ChannelScale[9];

			TRSData[i * h.num_poses + j].translation = translate;
			TRSData[i * h.num_poses + j].rotation = quaternion.Unit();
			TRSData[i * h.num_poses + j].scaling = scale;
		}
	}

	//If a model doesn't have an animation loaded, it will crash. We don't want that!
	if (h.num_frames <= 0)
	{
		h.num_frames = 1;
		TRSData.Resize(h.num_joints);

		for (uint32_t j = 0; j < h.num_joints; j++)
		{
			TRSData[j].translation = Joints[j].Translate;
			TRSData[j].rotation = Joints[j].Quaternion.Unit();
			TRSData[j].scaling = Joints[j].Scale;
		}
	}

	uint64_t endTime = I_nsTime();

	iqm_total_load_time_ns += endTime - loadStartTime;

	return true;
	
}

void IQMModel::LoadGeometry()
{
	FileData lumpdata = fileSystem.ReadFile(mLumpNum);
	char * data = (char *) lumpdata.GetMem();
	size_t len = lumpdata.GetSize();

	Vertices.Resize(NumVertices);
	for (IQMVertexArray& vertexArray : VertexArrays)
	{
		if (vertexArray.Type == IQM_POSITION)
		{
			float lu = 0.0f, lv = 0.0f, lindex = -1.0f;
			if (vertexArray.Format == IQM_FLOAT && vertexArray.Size == 3)
			{
				if((vertexArray.Offset + (vertexArray.Size * sizeof(float) * NumVertices)) > len)
				{
					I_FatalError("IQM_POSITION vertex array is bigger than the model file");
				}

				float * arr = (float *)(data + vertexArray.Offset);

				for (FModelVertex& v : Vertices)
				{
					v.x = *(arr++);
					v.z = *(arr++);
					v.y = *(arr++);

					v.lu = lu;
					v.lv = lv;
					v.lindex = lindex;
				}
			}
			else
			{
				I_FatalError("Unsupported IQM_POSITION vertex format");
			}
		}
		else if (vertexArray.Type == IQM_TEXCOORD)
		{
			if (vertexArray.Format == IQM_FLOAT && vertexArray.Size == 2)
			{
				if((vertexArray.Offset + (vertexArray.Size * sizeof(float) * NumVertices)) > len)
				{
					I_FatalError("IQM_TEXCOORD vertex array is bigger than the model file");
				}

				float * arr = (float *)(data + vertexArray.Offset);

				for (FModelVertex& v : Vertices)
				{
					v.u = *(arr++);
					v.v = *(arr++);
				}
			}
			else
			{
				I_FatalError("Unsupported IQM_TEXCOORD vertex format");
			}
		}
		else if (vertexArray.Type == IQM_NORMAL)
		{
			if (vertexArray.Format == IQM_FLOAT && vertexArray.Size == 3)
			{
				if((vertexArray.Offset + (vertexArray.Size * sizeof(float) * NumVertices)) > len)
				{
					I_FatalError("IQM_NORMAL vertex array is bigger than the model file");
				}

				float * arr = (float *)(data + vertexArray.Offset);

				for (FModelVertex& v : Vertices)
				{
					v.SetNormal(arr[0], arr[1], arr[2]);

					arr += 3;
				}
			}
			else
			{
				I_FatalError("Unsupported IQM_NORMAL vertex format");
			}
		}
		else if (vertexArray.Type == IQM_BLENDINDEXES)
		{
			if (vertexArray.Format == IQM_UBYTE && vertexArray.Size == 4)
			{
				if((vertexArray.Offset + (vertexArray.Size * sizeof(uint8_t) * NumVertices)) > len)
				{
					I_FatalError("IQM_BLENDINDEXES vertex array is bigger than the model file");
				}

				uint8_t * arr = (uint8_t *)(data + vertexArray.Offset);

				for (FModelVertex& v : Vertices)
				{
					v.SetBoneSelector(arr[0], arr[1], arr[2], arr[3]);

					arr += 4;
				}
			}
			else if (vertexArray.Format == IQM_INT && vertexArray.Size == 4)
			{
				if((vertexArray.Offset + (vertexArray.Size * sizeof(int32_t) * NumVertices)) > len)
				{
					I_FatalError("IQM_BLENDINDEXES vertex array is bigger than the model file");
				}

				int32_t * arr = (int32_t *)(data + vertexArray.Offset);

				for (FModelVertex& v : Vertices)
				{
					v.SetBoneSelector(arr[0], arr[1], arr[2], arr[3]);

					arr += 4;
				}
			}
			else
			{
				I_FatalError("Unsupported IQM_BLENDINDEXES vertex format");
			}
		}
		else if (vertexArray.Type == IQM_BLENDWEIGHTS)
		{
			if (vertexArray.Format == IQM_UBYTE && vertexArray.Size == 4)
			{
				if((vertexArray.Offset + (vertexArray.Size * sizeof(uint8_t) * NumVertices)) > len)
				{
					I_FatalError("IQM_BLENDWEIGHTS vertex array is bigger than the model file");
				}

				uint8_t * arr = (uint8_t *)(data + vertexArray.Offset);

				for (FModelVertex& v : Vertices)
				{
					v.SetBoneWeight(arr[0], arr[1], arr[2], arr[3]);

					arr += 4;
				}
			}
			else if (vertexArray.Format == IQM_FLOAT && vertexArray.Size == 4)
			{
				if((vertexArray.Offset + (vertexArray.Size * sizeof(float) * NumVertices)) > len)
				{
					I_FatalError("IQM_BLENDWEIGHTS vertex array is bigger than the model file");
				}

				float * arr = (float *)(data + vertexArray.Offset);

				for (FModelVertex& v : Vertices)
				{
					v.SetBoneWeight(
						(int)clamp(arr[0] * 255.0f, 0.0f, 255.0f),
						(int)clamp(arr[1] * 255.0f, 0.0f, 255.0f),
						(int)clamp(arr[2] * 255.0f, 0.0f, 255.0f),
						(int)clamp(arr[3] * 255.0f, 0.0f, 255.0f));

					arr += 4;
				}
			}
			else
			{
				I_FatalError("Unsupported IQM_BLENDWEIGHTS vertex format");
			}
		}
	}
}

void IQMModel::UnloadGeometry()
{
	Vertices.Reset();
}

int IQMModel::FindFrame(const char* name, bool nodefault)
{
	// [MK] allow looking up frames by animation name plus offset (using a colon as separator)
	const char* colon = strrchr(name,':');
	size_t nlen = (colon==nullptr)?strlen(name):(colon-name);
	for (unsigned i = 0; i < Anims.Size(); i++)
	{
		if (!strnicmp(name, Anims[i].Name.GetChars(), nlen))
		{
			// if no offset is given, return the first frame
			if (colon == nullptr) return Anims[i].FirstFrame;
			unsigned offset = atoi(colon+1);
			if (offset >= Anims[i].NumFrames) return FErr_NotFound;
			return Anims[i].FirstFrame+offset;
		}
	}
	return FErr_NotFound;
}

void IQMModel::RenderFrame(FModelRenderer* renderer, FGameTexture* skin, int frame1, int frame2, double inter, int translation, const FTextureID* surfaceskinids, const TArray<VSMatrix>& boneData, int boneStartPosition)
{
	renderer->SetupFrame(this, 0, 0, NumVertices, boneData, boneStartPosition);

	FGameTexture* lastSkin = nullptr;
	for (unsigned i = 0; i < Meshes.Size(); i++)
	{
		FGameTexture* meshSkin = skin;

		if (!meshSkin)
		{
			if (surfaceskinids && surfaceskinids[i].isValid())
			{
				meshSkin = TexMan.GetGameTexture(surfaceskinids[i], true);
			}
			else if (Meshes[i].Skin.isValid())
			{
				meshSkin = TexMan.GetGameTexture(Meshes[i].Skin, true);
			}	
			else
			{
				continue;
			}
		}

		if (meshSkin->isValid())
		{
			if (meshSkin != lastSkin)
			{
				renderer->SetMaterial(meshSkin, false, translation);
				lastSkin = meshSkin;
			}
			renderer->DrawElements(Meshes[i].NumTriangles * 3, Meshes[i].FirstTriangle * 3 * sizeof(unsigned int));
		}
	}
}

void IQMModel::BuildVertexBuffer(FModelRenderer* renderer)
{
	if (!GetVertexBuffer(renderer->GetType()))
	{
		LoadGeometry();

		auto vbuf = renderer->CreateVertexBuffer(true, true);
		SetVertexBuffer(renderer->GetType(), vbuf);

		FModelVertex* vertptr = vbuf->LockVertexBuffer(Vertices.Size());
		memcpy(vertptr, Vertices.Data(), Vertices.Size() * sizeof(FModelVertex));
		vbuf->UnlockVertexBuffer();

		unsigned int* indxptr = vbuf->LockIndexBuffer(Triangles.Size() * 3);
		memcpy(indxptr, Triangles.Data(), Triangles.Size() * sizeof(unsigned int) * 3);
		vbuf->UnlockIndexBuffer();

		UnloadGeometry();
	}
}

void IQMModel::AddSkins(uint8_t* hitlist, const FTextureID* surfaceskinids)
{
	for (unsigned i = 0; i < Meshes.Size(); i++)
	{
		if (surfaceskinids && surfaceskinids[i].isValid())
			hitlist[surfaceskinids[i].GetIndex()] |= FTextureManager::HIT_Flat;
	}
}

const TArray<TRS>* IQMModel::AttachAnimationData()
{
	return &TRSData;
}

const TArray<VSMatrix> IQMModel::CalculateBones(int frame1, int frame2, double inter, const TArray<TRS>* animationData, DBoneComponents* boneComponentData, int index)
{
	const TArray<TRS>& animationFrames = animationData ? *animationData : TRSData;
	if (Joints.Size() > 0)
	{
		int numbones = Joints.SSize();

		if (boneComponentData->trscomponents[index].SSize() != numbones)
			boneComponentData->trscomponents[index].Resize(numbones);
		if (boneComponentData->trsmatrix[index].SSize() != numbones)
			boneComponentData->trsmatrix[index].Resize(numbones);

		frame1 = clamp(frame1, 0, (animationFrames.SSize() - 1) / numbones);
		frame2 = clamp(frame2, 0, (animationFrames.SSize() - 1) / numbones);

		int offset1 = frame1 * numbones;
		int offset2 = frame2 * numbones;
		float t = (float)inter;
		float invt = 1.0f - t;

		float swapYZ[16] = { 0.0f };
		swapYZ[0 + 0 * 4] = 1.0f;
		swapYZ[1 + 2 * 4] = 1.0f;
		swapYZ[2 + 1 * 4] = 1.0f;
		swapYZ[3 + 3 * 4] = 1.0f;

		TArray<VSMatrix> bones(numbones, true);
		TArray<bool> modifiedBone(numbones, true);
		for (int i = 0; i < numbones; i++)
		{
			TRS bone;
			TRS from = animationFrames[offset1 + i];
			TRS to = animationFrames[offset2 + i];

			bone.translation = from.translation * invt + to.translation * t;
			bone.rotation = from.rotation * invt;
			if ((bone.rotation | to.rotation * t) < 0)
			{
				bone.rotation.X *= -1; bone.rotation.Y *= -1; bone.rotation.Z *= -1; bone.rotation.W *= -1;
			}
			bone.rotation += to.rotation * t;
			bone.rotation.MakeUnit();
			bone.scaling = from.scaling * invt + to.scaling * t;

			if (Joints[i].Parent >= 0 && modifiedBone[Joints[i].Parent])
			{
				boneComponentData->trscomponents[index][i] = bone;
				modifiedBone[i] = true;
			}
			else if (boneComponentData->trscomponents[index][i].Equals(bone))
			{
				bones[i] = boneComponentData->trsmatrix[index][i];
				modifiedBone[i] = false;
				continue;
			}
			else
			{
				boneComponentData->trscomponents[index][i] = bone;
				modifiedBone[i] = true;
			}

			VSMatrix m;
			m.loadIdentity();
			m.translate(bone.translation.X, bone.translation.Y, bone.translation.Z);
			m.multQuaternion(bone.rotation);
			m.scale(bone.scaling.X, bone.scaling.Y, bone.scaling.Z);

			VSMatrix& result = bones[i];
			if (Joints[i].Parent >= 0)
			{
				result = bones[Joints[i].Parent];
				result.multMatrix(swapYZ);
				result.multMatrix(baseframe[Joints[i].Parent]);
				result.multMatrix(m);
				result.multMatrix(inversebaseframe[i]);
			}
			else
			{
				result.loadMatrix(swapYZ);
				result.multMatrix(m);
				result.multMatrix(inversebaseframe[i]);
			}
			result.multMatrix(swapYZ);
		}

		boneComponentData->trsmatrix[index] = bones;

		return bones;
	}
	return {};
}
