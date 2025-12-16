#pragma once

#include "c_cvars.h"
#include "v_palette.h"

#include "r_utility.h"

struct Colormap;

EXTERN_CVAR(Int, r_extralight)

template<bool addExtra = true>
inline int hw_ClampLight(int lightlevel) // TODO/tidy: rename + move out of hwrenderer namespace
{
	if constexpr(addExtra)
	{
		//max is needed for negative r_extralight values
		return max(int((clamp(lightlevel, 0, 255) / 255.0) * (255.0 - r_extralight)) + r_extralight, 0);
	}
	else
	{
		return clamp(lightlevel, 0, 255);
	}
}

EXTERN_CVAR(Int, gl_weaponlight);

inline	int getExtraLight()
{
	return r_viewpoint.extralight * gl_weaponlight;
}

