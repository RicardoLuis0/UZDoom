#ifndef __GL_LIGHTBUFFER_H
#define __GL_LIGHTBUFFER_H

#include "tarray.h"
#include "hw_dynlightdata.h"
#include "hwrenderer/data/buffers.h"
#include "shaderuniforms.h"
#include <atomic>
#include <mutex>

class FRenderState;

static const int LIGHTBUFFER_ELEMENTS_PER_LIGHT = 4;			// each light needs 4 vec4's.
static const int LIGHTBUFFER_ELEMENT_SIZE = (4*sizeof(float));
static const int LIGHTBUFFER_MAX_LIGHTS = 80000;

struct FLightBuffer : FBufferManager
{
	FLightBuffer(int pipelineNbr = 1) : FBufferManager(LIGHTBUFFER_MAX_LIGHTS * LIGHTBUFFER_ELEMENTS_PER_LIGHT, LIGHTBUFFER_ELEMENT_SIZE, LIGHTBUF_BINDINGPOINT, pipelineNbr) {}
	int UploadLights(FDynLightData &data);
};


#endif

