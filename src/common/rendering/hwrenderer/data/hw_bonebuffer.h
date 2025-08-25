#pragma once

#include "tarray.h"
#include "hwrenderer/data/buffers.h"
#include "common/utility/matrix.h"
#include "shaderuniforms.h"
#include <atomic>
#include <mutex>

class FRenderState;

static const int BONEBUFFER_ELEMENT_SIZE = (16*sizeof(float));
static const int BONEBUFFER_MAX_ELEMENTS = 80000;

struct FBoneBuffer : FBufferManager
{
	FBoneBuffer(int pipelineNbr = 1) : FBufferManager(BONEBUFFER_MAX_ELEMENTS, BONEBUFFER_ELEMENT_SIZE, BONEBUF_BINDINGPOINT, pipelineNbr) {}
	int UploadBones(const TArray<VSMatrix> &bones);
};
