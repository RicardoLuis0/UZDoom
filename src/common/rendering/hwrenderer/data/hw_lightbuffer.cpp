// 
//---------------------------------------------------------------------------
//
// Copyright(C) 2014-2016 Christoph Oelckers
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
//--------------------------------------------------------------------------
//
/*
** gl_lightbuffer.cpp
** Buffer data maintenance for dynamic lights
**
**/

#include "hw_lightbuffer.h"
#include "hw_dynlightdata.h"
#include "shaderuniforms.h"
#include "v_video.h"

FBufferManager::FBufferManager(unsigned bufferSize, unsigned elementSize, unsigned bindingPoint, int pipelineNbr) : mBufferSize(bufferSize), mElementSize(elementSize), mByteSize(bufferSize * elementSize), mBindingPoint(bindingPoint), mPipelineNbr(pipelineNbr > 0 ? pipelineNbr : 1)
{
	mBufferSSBO = screen->useSSBO();

	if(mBufferSSBO)
	{
		mBlockAlign = 0;
		mBlockSize = mBufferSize;
		mMaxUploadSize = mBlockSize;
	}
	else
	{
		mBlockSize = screen->maxuniformblock / mElementSize;
		mBlockAlign = (screen->uniformblockalignment <= mElementSize) ? 1 : (screen->uniformblockalignment / mElementSize);
		mMaxUploadSize = (mBlockSize - mBlockAlign);
	}

	for (int n = 0; n < mPipelineNbr; n++)
	{
		mBufferPipeline[n] = screen->CreateDataBuffer(mBindingPoint, mBufferSSBO, false);
		mBufferPipeline[n]->SetData(mByteSize, nullptr, BufferUsageType::Persistent);
	}

	mIndex = 0;
	mPipelinePos = 0;
	mBuffer = mBufferPipeline[0];
}

int FLightBuffer::UploadLights(FDynLightData &data)
{
	// All meaasurements here are in vec4's.
	int size0 = data.arrays[0].Size()/4;
	int size1 = data.arrays[1].Size()/4;
	int size2 = data.arrays[2].Size()/4;
	int totalsize = size0 + size1 + size2 + 1;

	if (totalsize > (int)mMaxUploadSize)
	{
		int diff = totalsize - (int)mMaxUploadSize;

		size2 -= diff;
		if (size2 < 0)
		{
			size1 += size2;
			size2 = 0;
		}
		if (size1 < 0)
		{
			size0 += size1;
			size1 = 0;
		}
		totalsize = size0 + size1 + size2 + 1;
	}

	float *mBufferPointer = (float*)mBuffer->Memory();
	assert(mBufferPointer != nullptr);
	if (mBufferPointer == nullptr) return -1;
	if (totalsize <= 1) return -1;	// there are no lights

	unsigned thisindex = mIndex.fetch_add(totalsize);
	float parmcnt[] = { 0, float(size0), float(size0 + size1), float(size0 + size1 + size2) };

	if (thisindex + totalsize <= mBufferSize)
	{
		float *copyptr = mBufferPointer + thisindex*4;

		memcpy(&copyptr[0], parmcnt, LIGHTBUFFER_ELEMENT_SIZE);
		memcpy(&copyptr[4], &data.arrays[0][0], size0 * LIGHTBUFFER_ELEMENT_SIZE);
		memcpy(&copyptr[4 + 4*size0], &data.arrays[1][0], size1 * LIGHTBUFFER_ELEMENT_SIZE);
		memcpy(&copyptr[4 + 4*(size0 + size1)], &data.arrays[2][0], size2 * LIGHTBUFFER_ELEMENT_SIZE);
		return thisindex;
	}
	else
	{
		return -1;	// Buffer is full. Since it is being used live at the point of the upload we cannot do much here but to abort.
	}
}


