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

#include "hw_bonebuffer.h"
#include "hw_dynlightdata.h"
#include "shaderuniforms.h"
#include "v_video.h"

int FBoneBuffer::UploadBones(const TArray<VSMatrix>& bones)
{
	Map();
	int totalsize = bones.Size();
	if (totalsize > (int)mMaxUploadSize)
	{
		totalsize = mMaxUploadSize;
	}

	uint8_t *mBufferPointer = (uint8_t*)mBuffer->Memory();
	assert(mBufferPointer != nullptr);
	if (mBufferPointer == nullptr) return -1;
	if (totalsize <= 0) return -1;	// there are no bones

	unsigned int thisindex = mIndex.fetch_add(totalsize);

	if (thisindex + totalsize <= mBufferSize)
	{
		memcpy(mBufferPointer + thisindex * BONEBUFFER_ELEMENT_SIZE, bones.Data(), totalsize * BONEBUFFER_ELEMENT_SIZE);
		Unmap();
		return thisindex;
	}
	else
	{
		Unmap();
		return -1;	// Buffer is full. Since it is being used live at the point of the upload we cannot do much here but to abort.
	}
}