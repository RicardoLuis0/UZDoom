/*
** r_interpolate.cpp
**
** Movement interpolation
**
**---------------------------------------------------------------------------
** Copyright 2008 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include "p_3dmidtex.h"
#include "stats.h"
#include "r_data/r_interpolate.h"
#include "p_local.h"
#include "po_man.h"
#include "serializer_doom.h"
#include "serialize_obj.h"
#include "g_levellocals.h"
#include "vm.h"

//==========================================================================
//
// Exports
//
//==========================================================================

DEFINE_FIELD(DInterpolation, Next);
DEFINE_FIELD(DInterpolation, Prev);
DEFINE_FIELD(DInterpolation, Level);
DEFINE_FIELD(DInterpolation, refcount);

void NativeUnlinkFromMap(DInterpolation * self)
{
	self->UnlinkFromMap();
}

void NativeUpdateInterpolation(DInterpolation * self)
{
	self->UpdateInterpolation();
}

void NativeRestore(DInterpolation * self)
{
	self->Restore();
}

void NativeInterpolate(DInterpolation * self, double smoothratio)
{
	self->Interpolate(smoothratio);
}

DEFINE_ACTION_FUNCTION_NATIVE(DInterpolation, UnlinkFromMap, NativeUnlinkFromMap)
{
	PARAM_SELF_PROLOGUE(DInterpolation);
	self->UnlinkFromMap();
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DInterpolation, UpdateInterpolation, NativeUpdateInterpolation)
{
	PARAM_SELF_PROLOGUE(DInterpolation);
	self->UpdateInterpolation();
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DInterpolation, Restore, NativeRestore)
{
	PARAM_SELF_PROLOGUE(DInterpolation);
	self->Restore();
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DInterpolation, Interpolate, NativeInterpolate)
{
	PARAM_SELF_PROLOGUE(DInterpolation);
	PARAM_FLOAT(smoothratio);
	self->Interpolate(smoothratio);
	return 0;
}

void DInterpolation::CallUnlinkFromMap()
{
	IFVIRTUAL(DInterpolation, UnlinkFromMap)
	{
		VMValue params[1] = { (DObject*)this };
		VMCall(func, params, 1, nullptr, 0);
	}
	else
	{
		UnlinkFromMap();
	}
}

void DInterpolation::CallUpdateInterpolation()
{
	IFVIRTUAL(DInterpolation, UpdateInterpolation)
	{
		VMValue params[1] = { (DObject*)this };
		VMCall(func, params, 1, nullptr, 0);
	}
	else
	{
		UpdateInterpolation();
	}
}

void DInterpolation::CallRestore()
{
	IFVIRTUAL(DInterpolation, Restore)
	{
		VMValue params[1] = { (DObject*)this };
		VMCall(func, params, 1, nullptr, 0);
	}
	else
	{
		Restore();
	}
}

void DInterpolation::CallInterpolate(double smoothratio)
{
	IFVIRTUAL(DInterpolation, Interpolate)
	{
		VMValue params[2] = { (DObject*)this, smoothratio };
		VMCall(func, params, 2, nullptr, 0);
	}
	else
	{
		Interpolate(smoothratio);
	}
}

//==========================================================================
//
// DSectorPlaneInterpolation
//
//==========================================================================

class DSectorPlaneInterpolation : public DInterpolation
{
	DECLARE_CLASS(DSectorPlaneInterpolation, DInterpolation)

public:

	sector_t *sector;
	double oldheight, oldtexz;
	double bakheight, baktexz;
	bool ceiling;
	TArray<DInterpolation *> attached;

	void Init(sector_t *sector, bool plane, bool attach);

	DSectorPlaneInterpolation() {}
	DSectorPlaneInterpolation(sector_t *sector, bool plane, bool attach) { Init(sector, plane, attach); }

	virtual void UnlinkFromMap() override;
	virtual void UpdateInterpolation() override;
	virtual void Restore() override;
	virtual void Interpolate(double smoothratio) override;

	virtual void Serialize(FSerializer &arc) override;
};

DEFINE_FIELD_NAMED(DSectorPlaneInterpolation, sector, _sector);
DEFINE_FIELD(DSectorPlaneInterpolation, oldheight);
DEFINE_FIELD(DSectorPlaneInterpolation, oldtexz);
DEFINE_FIELD(DSectorPlaneInterpolation, bakheight);
DEFINE_FIELD(DSectorPlaneInterpolation, baktexz);
DEFINE_FIELD(DSectorPlaneInterpolation, ceiling);
DEFINE_FIELD(DSectorPlaneInterpolation, attached);

void NativeInitSectorPlane(DSectorPlaneInterpolation * self, sector_t *sector, bool plane, bool attach)
{
	self->Init(sector, plane, attach);
}

DEFINE_ACTION_FUNCTION_NATIVE(DSectorPlaneInterpolation, Init, NativeInitSectorPlane)
{
	PARAM_SELF_PROLOGUE(DSectorPlaneInterpolation);
	PARAM_POINTER(sector, sector_t);
	PARAM_BOOL(plane);
	PARAM_BOOL(attach);
	self->Init(sector, plane, attach);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DSectorPlaneInterpolation, UnlinkFromMap, NativeUnlinkFromMap)
{
	PARAM_SELF_PROLOGUE(DSectorPlaneInterpolation);
	self->UnlinkFromMap();
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DSectorPlaneInterpolation, UpdateInterpolation, NativeUpdateInterpolation)
{
	PARAM_SELF_PROLOGUE(DSectorPlaneInterpolation);
	self->UpdateInterpolation();
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DSectorPlaneInterpolation, Restore, NativeRestore)
{
	PARAM_SELF_PROLOGUE(DSectorPlaneInterpolation);
	self->Restore();
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DSectorPlaneInterpolation, Interpolate, NativeInterpolate)
{
	PARAM_SELF_PROLOGUE(DSectorPlaneInterpolation);
	PARAM_FLOAT(smoothratio);
	self->Interpolate(smoothratio);
	return 0;
}

//==========================================================================
//
// DSectorScrollInterpolation
//
//==========================================================================

class DSectorScrollInterpolation : public DInterpolation
{
	DECLARE_CLASS(DSectorScrollInterpolation, DInterpolation)

public:
	sector_t *sector;
	double oldx, oldy;
	double bakx, baky;
	bool ceiling;

	void Init(sector_t *sector, bool plane);

	DSectorScrollInterpolation() {}
	DSectorScrollInterpolation(sector_t *sector, bool plane) { Init(sector, plane); }

	virtual void UnlinkFromMap() override;
	virtual void UpdateInterpolation() override;
	virtual void Restore() override;
	virtual void Interpolate(double smoothratio) override;

	virtual void Serialize(FSerializer &arc) override;
};

DEFINE_FIELD_NAMED(DSectorScrollInterpolation, sector, _sector);
DEFINE_FIELD(DSectorScrollInterpolation, oldx);
DEFINE_FIELD(DSectorScrollInterpolation, oldy);
DEFINE_FIELD(DSectorScrollInterpolation, bakx);
DEFINE_FIELD(DSectorScrollInterpolation, baky);
DEFINE_FIELD(DSectorScrollInterpolation, ceiling);

void NativeInitSectorScroll(DSectorScrollInterpolation * self, sector_t *sector, bool plane)
{
	self->Init(sector, plane);
}

DEFINE_ACTION_FUNCTION_NATIVE(DSectorScrollInterpolation, Init, NativeInitSectorScroll)
{
	PARAM_SELF_PROLOGUE(DSectorScrollInterpolation);
	PARAM_POINTER(sector, sector_t);
	PARAM_BOOL(plane);
	self->Init(sector, plane);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DSectorScrollInterpolation, UnlinkFromMap, NativeUnlinkFromMap)
{
	PARAM_SELF_PROLOGUE(DSectorScrollInterpolation);
	self->UnlinkFromMap();
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DSectorScrollInterpolation, UpdateInterpolation, NativeUpdateInterpolation)
{
	PARAM_SELF_PROLOGUE(DSectorScrollInterpolation);
	self->UpdateInterpolation();
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DSectorScrollInterpolation, Restore, NativeRestore)
{
	PARAM_SELF_PROLOGUE(DSectorScrollInterpolation);
	self->Restore();
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DSectorScrollInterpolation, Interpolate, NativeInterpolate)
{
	PARAM_SELF_PROLOGUE(DSectorScrollInterpolation);
	PARAM_FLOAT(smoothratio);
	self->Interpolate(smoothratio);
	return 0;
}

//==========================================================================
//
// DWallScrollInterpolation
//
//==========================================================================

class DWallScrollInterpolation : public DInterpolation
{
	DECLARE_CLASS(DWallScrollInterpolation, DInterpolation)
public:
	side_t *side;
	int part;
	double oldx, oldy;
	double bakx, baky;

	void Init(side_t *side, int part);

	DWallScrollInterpolation() {}
	DWallScrollInterpolation(side_t *side, int part) { Init(side, part); }

	virtual void UnlinkFromMap() override;
	virtual void UpdateInterpolation() override;
	virtual void Restore() override;
	virtual void Interpolate(double smoothratio) override;

	virtual void Serialize(FSerializer &arc) override;
};

DEFINE_FIELD_NAMED(DWallScrollInterpolation, side, _side);
DEFINE_FIELD(DWallScrollInterpolation, part);
DEFINE_FIELD(DWallScrollInterpolation, oldx);
DEFINE_FIELD(DWallScrollInterpolation, oldy);
DEFINE_FIELD(DWallScrollInterpolation, bakx);
DEFINE_FIELD(DWallScrollInterpolation, baky);

void NativeInitWallScroll(DWallScrollInterpolation * self, side_t *side, int part)
{
	self->Init(side, part);
}

DEFINE_ACTION_FUNCTION_NATIVE(DWallScrollInterpolation, Init, NativeInitWallScroll)
{
	PARAM_SELF_PROLOGUE(DWallScrollInterpolation);
	PARAM_POINTER(side, side_t);
	PARAM_INT(part);
	self->Init(side, part);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DWallScrollInterpolation, UnlinkFromMap, NativeUnlinkFromMap)
{
	PARAM_SELF_PROLOGUE(DWallScrollInterpolation);
	self->UnlinkFromMap();
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DWallScrollInterpolation, UpdateInterpolation, NativeUpdateInterpolation)
{
	PARAM_SELF_PROLOGUE(DWallScrollInterpolation);
	self->UpdateInterpolation();
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DWallScrollInterpolation, Restore, NativeRestore)
{
	PARAM_SELF_PROLOGUE(DWallScrollInterpolation);
	self->Restore();
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DWallScrollInterpolation, Interpolate, NativeInterpolate)
{
	PARAM_SELF_PROLOGUE(DWallScrollInterpolation);
	PARAM_FLOAT(smoothratio);
	self->Interpolate(smoothratio);
	return 0;
}

//==========================================================================
//
// DPolyobjInterpolation
//
//==========================================================================

class DPolyobjInterpolation : public DInterpolation
{
	DECLARE_CLASS(DPolyobjInterpolation, DInterpolation)
public:
	FPolyObj *poly;
	TArray<double> oldverts, bakverts;
	double oldcx, oldcy;
	double bakcx, bakcy;

	void Init(FPolyObj *poly);

	DPolyobjInterpolation() {}
	DPolyobjInterpolation(FPolyObj *poly) { Init(poly); }

	virtual void UnlinkFromMap() override;
	virtual void UpdateInterpolation() override;
	virtual void Restore() override;
	virtual void Interpolate(double smoothratio) override;

	virtual void Serialize(FSerializer &arc) override;
};

DEFINE_FIELD(DPolyobjInterpolation, poly);
DEFINE_FIELD(DPolyobjInterpolation, oldverts);
DEFINE_FIELD(DPolyobjInterpolation, bakverts);
DEFINE_FIELD(DPolyobjInterpolation, oldcx);
DEFINE_FIELD(DPolyobjInterpolation, oldcy);
DEFINE_FIELD(DPolyobjInterpolation, bakcx);
DEFINE_FIELD(DPolyobjInterpolation, bakcy);

void NativeInitPolyobj(DPolyobjInterpolation * self, FPolyObj *poly)
{
	self->Init(poly);
}

DEFINE_ACTION_FUNCTION_NATIVE(DPolyobjInterpolation, Init, NativeInitPolyobj)
{
	PARAM_SELF_PROLOGUE(DPolyobjInterpolation);
	PARAM_POINTER(poly, FPolyObj);
	self->Init(poly);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DPolyobjInterpolation, UnlinkFromMap, NativeUnlinkFromMap)
{
	PARAM_SELF_PROLOGUE(DPolyobjInterpolation);
	self->UnlinkFromMap();
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DPolyobjInterpolation, UpdateInterpolation, NativeUpdateInterpolation)
{
	PARAM_SELF_PROLOGUE(DPolyobjInterpolation);
	self->UpdateInterpolation();
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DPolyobjInterpolation, Restore, NativeRestore)
{
	PARAM_SELF_PROLOGUE(DPolyobjInterpolation);
	self->Restore();
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DPolyobjInterpolation, Interpolate, NativeInterpolate)
{
	PARAM_SELF_PROLOGUE(DPolyobjInterpolation);
	PARAM_FLOAT(smoothratio);
	self->Interpolate(smoothratio);
	return 0;
}

//==========================================================================
//
//
//
//==========================================================================

IMPLEMENT_CLASS(DInterpolation, true, true)

IMPLEMENT_POINTERS_START(DInterpolation)
	IMPLEMENT_POINTER(Next)
	IMPLEMENT_POINTER(Prev)
IMPLEMENT_POINTERS_END

IMPLEMENT_CLASS(DSectorPlaneInterpolation, false, false)
IMPLEMENT_CLASS(DSectorScrollInterpolation, false, false)
IMPLEMENT_CLASS(DWallScrollInterpolation, false, false)
IMPLEMENT_CLASS(DPolyobjInterpolation, false, false)

//==========================================================================
//
//
//
//==========================================================================

int FInterpolator::CountInterpolations ()
{
	int count = 0;
	for (DInterpolation *probe = Head; probe != nullptr; probe = probe->Next)
	{
		count++;
	}
	return count;
}

//==========================================================================
//
//
//
//==========================================================================

void FInterpolator::UpdateInterpolations()
{
	for (DInterpolation *probe = Head; probe != nullptr; probe = probe->Next)
	{
		probe->CallUpdateInterpolation();
	}
}

//==========================================================================
//
//
//
//==========================================================================

void FInterpolator::AddInterpolation(DInterpolation *interp)
{
	interp->Next = Head;
	if (Head != nullptr) Head->Prev = interp;
	interp->Prev = nullptr;
	Head = interp;
}

//==========================================================================
//
//
//
//==========================================================================

void FInterpolator::RemoveInterpolation(DInterpolation *interp)
{
	if (Head == interp)
	{
		Head = interp->Next;
		if (Head != nullptr) Head->Prev = nullptr;
	}
	else
	{
		if (interp->Prev != nullptr) interp->Prev->Next = interp->Next;
		if (interp->Next != nullptr) interp->Next->Prev = interp->Prev;
	}
	interp->Next = nullptr;
	interp->Prev = nullptr;
}

//==========================================================================
//
//
//
//==========================================================================

void FInterpolator::DoInterpolations(double smoothratio)
{
	if (smoothratio >= 1.)
	{
		didInterp = false;
		return;
	}

	didInterp = true;

	DInterpolation *probe = Head;
	while (probe != nullptr)
	{
		DInterpolation *next = probe->Next;
		probe->CallInterpolate(smoothratio);
		probe = next;
	}
}

//==========================================================================
//
//
//
//==========================================================================

void FInterpolator::RestoreInterpolations()
{
	if (didInterp)
	{
		didInterp = false;
		for (DInterpolation *probe = Head; probe != nullptr; probe = probe->Next)
		{
			probe->CallRestore();
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void FInterpolator::ClearInterpolations()
{
	DInterpolation *probe = Head;
	Head = nullptr;

	while (probe != nullptr)
	{
		DInterpolation *next = probe->Next;
		probe->Next = probe->Prev = nullptr;
		probe->CallUnlinkFromMap();
		probe->Destroy();
		probe = next;
	}

}

FSerializer &Serialize(FSerializer &arc, const char *key, FInterpolator &rs, FInterpolator *def)
{
	if (arc.BeginObject(key))
	{
		arc("head", rs.Head)
			.EndObject();
	}
	return arc;
}

//==========================================================================
//
//
//
//==========================================================================

int DInterpolation::AddRef()
{
	return ++refcount;
}

//==========================================================================
//
//
//
//==========================================================================

int DInterpolation::DelRef(bool force)
{
	if (refcount > 0) --refcount;
	if (force && refcount == 0)
	{
		UnlinkFromMap();
		Destroy();
	}
	return refcount;
}

//==========================================================================
//
//
//
//==========================================================================

void DInterpolation::UnlinkFromMap()
{
	if (Level)
		Level->interpolator.RemoveInterpolation(this);
	refcount = 0;
}

//==========================================================================
//
//
//
//==========================================================================

void DInterpolation::Serialize(FSerializer &arc)
{
	Super::Serialize(arc);
	arc("refcount", refcount)
		("next", Next)
		("prev", Prev)
		("level", Level);
}
//==========================================================================
//
//
//
//==========================================================================

void DSectorPlaneInterpolation::Init(sector_t *_sector, bool _plane, bool attach)
{
	Level = _sector->Level;
	sector = _sector;
	ceiling = _plane;
	UpdateInterpolation();

	if (attach)
	{
		P_Start3dMidtexInterpolations(attached, sector, ceiling);
		P_StartLinkedSectorInterpolations(attached, sector, ceiling);
	}
	sector->Level->interpolator.AddInterpolation(this);
}

//==========================================================================
//
//
//
//==========================================================================

void DSectorPlaneInterpolation::UnlinkFromMap()
{
	if (sector != nullptr)
	{
		if (ceiling)
		{
			sector->interpolations[sector_t::CeilingMove] = nullptr;
		}
		else
		{
			sector->interpolations[sector_t::FloorMove] = nullptr;
		}
		sector = nullptr;
	}
	for (unsigned i = 0; i < attached.Size(); i++)
	{
		attached[i]->DelRef();
	}
	attached.Reset();
	Super::UnlinkFromMap();
}

//==========================================================================
//
//
//
//==========================================================================

void DSectorPlaneInterpolation::UpdateInterpolation()
{
	if (!ceiling)
	{
		oldheight = sector->floorplane.fD();
		oldtexz = sector->GetPlaneTexZ(sector_t::floor);
	}
	else
	{
		oldheight = sector->ceilingplane.fD();
		oldtexz = sector->GetPlaneTexZ(sector_t::ceiling);
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DSectorPlaneInterpolation::Restore()
{
	if (!ceiling)
	{
		sector->floorplane.setD(bakheight);
		sector->SetPlaneTexZ(sector_t::floor, baktexz, true);
	}
	else
	{
		sector->ceilingplane.setD(bakheight);
		sector->SetPlaneTexZ(sector_t::ceiling, baktexz, true);
	}
	P_RecalculateAttached3DFloors(sector);
	sector->CheckPortalPlane(ceiling? sector_t::ceiling : sector_t::floor);
}

//==========================================================================
//
//
//
//==========================================================================

void DSectorPlaneInterpolation::Interpolate(double smoothratio)
{
	secplane_t *pplane;
	int pos;

	if (!ceiling)
	{
		pplane = &sector->floorplane;
		pos = sector_t::floor;
	}
	else
	{
		pplane = &sector->ceilingplane;
		pos = sector_t::ceiling;
	}

	bakheight = pplane->fD();
	baktexz = sector->GetPlaneTexZ(pos);

	if (refcount == 0 && oldheight == bakheight)
	{
		UnlinkFromMap();
		Destroy();
	}
	else
	{
		pplane->setD(oldheight + (bakheight - oldheight) * smoothratio);
		sector->SetPlaneTexZ(pos, oldtexz + (baktexz - oldtexz) * smoothratio, true);
		P_RecalculateAttached3DFloors(sector);
		sector->CheckPortalPlane(pos);
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DSectorPlaneInterpolation::Serialize(FSerializer &arc)
{
	Super::Serialize(arc);
	arc("sector", sector)
		("ceiling", ceiling)
		("oldheight", oldheight)
		("oldtexz", oldtexz)
		("attached", attached);
}
//==========================================================================
//
//
//
//==========================================================================

void DSectorScrollInterpolation::Init(sector_t *_sector, bool _plane)
{
	Level = _sector->Level;
	sector = _sector;
	ceiling = _plane;
	UpdateInterpolation ();
	sector->Level->interpolator.AddInterpolation(this);
}

//==========================================================================
//
//
//
//==========================================================================

void DSectorScrollInterpolation::UnlinkFromMap()
{
	if (sector != nullptr)
	{
		if (ceiling)
		{
			sector->interpolations[sector_t::CeilingScroll] = nullptr;
		}
		else
		{
			sector->interpolations[sector_t::FloorScroll] = nullptr;
		}
		sector = nullptr;
	}
	Super::UnlinkFromMap();
}

//==========================================================================
//
//
//
//==========================================================================

void DSectorScrollInterpolation::UpdateInterpolation()
{
	oldx = sector->GetXOffset(ceiling);
	oldy = sector->GetYOffset(ceiling, false);
}

//==========================================================================
//
//
//
//==========================================================================

void DSectorScrollInterpolation::Restore()
{
	sector->SetXOffset(ceiling, bakx);
	sector->SetYOffset(ceiling, baky);
}

//==========================================================================
//
//
//
//==========================================================================

void DSectorScrollInterpolation::Interpolate(double smoothratio)
{
	bakx = sector->GetXOffset(ceiling);
	baky = sector->GetYOffset(ceiling, false);

	if (refcount == 0 && oldx == bakx && oldy == baky)
	{
		UnlinkFromMap();
		Destroy();
	}
	else
	{
		sector->SetXOffset(ceiling, oldx + (bakx - oldx) * smoothratio);
		sector->SetYOffset(ceiling, oldy + (baky - oldy) * smoothratio);
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DSectorScrollInterpolation::Serialize(FSerializer &arc)
{
	Super::Serialize(arc);
	arc("sector", sector)
		("ceiling", ceiling)
		("oldx", oldx)
		("oldy", oldy);
}
//==========================================================================
//
//
//
//==========================================================================

void DWallScrollInterpolation::Init(side_t *_side, int _part)
{
	Level = _side->GetLevel();
	side = _side;
	part = _part;
	UpdateInterpolation ();
	side->GetLevel()->interpolator.AddInterpolation(this);
}

//==========================================================================
//
//
//
//==========================================================================

void DWallScrollInterpolation::UnlinkFromMap()
{
	if (side != nullptr)
	{
		side->textures[part].interpolation = nullptr;
		side = nullptr;
	}
	Super::UnlinkFromMap();
}

//==========================================================================
//
//
//
//==========================================================================

void DWallScrollInterpolation::UpdateInterpolation()
{
	oldx = side->GetTextureXOffset(part);
	oldy = side->GetTextureYOffset(part);
}

//==========================================================================
//
//
//
//==========================================================================

void DWallScrollInterpolation::Restore()
{
	side->SetTextureXOffset(part, bakx);
	side->SetTextureYOffset(part, baky);
}

//==========================================================================
//
//
//
//==========================================================================

void DWallScrollInterpolation::Interpolate(double smoothratio)
{
	bakx = side->GetTextureXOffset(part);
	baky = side->GetTextureYOffset(part);

	if (refcount == 0 && oldx == bakx && oldy == baky)
	{
		UnlinkFromMap();
		Destroy();
	}
	else
	{
		side->SetTextureXOffset(part, oldx + (bakx - oldx) * smoothratio);
		side->SetTextureYOffset(part, oldy + (baky - oldy) * smoothratio);
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DWallScrollInterpolation::Serialize(FSerializer &arc)
{
	Super::Serialize(arc);
	arc("side", side)
		("part", part)
		("oldx", oldx)
		("oldy", oldy);
}
//==========================================================================
//
//
//
//==========================================================================

void DPolyobjInterpolation::Init(FPolyObj *po)
{
	Level = po->Level;
	poly = po;
	oldverts.Resize(po->Vertices.Size() << 1);
	bakverts.Resize(po->Vertices.Size() << 1);
	UpdateInterpolation ();
	po->Level->interpolator.AddInterpolation(this);
}

//==========================================================================
//
//
//
//==========================================================================

void DPolyobjInterpolation::UnlinkFromMap()
{
	if (poly != nullptr)
	{
		poly->interpolation = nullptr;
	}
	Super::UnlinkFromMap();
}

//==========================================================================
//
//
//
//==========================================================================

void DPolyobjInterpolation::UpdateInterpolation()
{
	for(unsigned int i = 0; i < poly->Vertices.Size(); i++)
	{
		oldverts[i*2  ] = poly->Vertices[i]->fX();
		oldverts[i*2+1] = poly->Vertices[i]->fY();
	}
	oldcx = poly->CenterSpot.pos.X;
	oldcy = poly->CenterSpot.pos.Y;
}

//==========================================================================
//
//
//
//==========================================================================

void DPolyobjInterpolation::Restore()
{
	for(unsigned int i = 0; i < poly->Vertices.Size(); i++)
	{
		poly->Vertices[i]->set(bakverts[i*2  ], bakverts[i*2+1]);
	}
	poly->CenterSpot.pos.X = bakcx;
	poly->CenterSpot.pos.Y = bakcy;
	poly->ClearSubsectorLinks();
}

//==========================================================================
//
//
//
//==========================================================================

void DPolyobjInterpolation::Interpolate(double smoothratio)
{
	bool changed = false;
	for(unsigned int i = 0; i < poly->Vertices.Size(); i++)
	{
		bakverts[i*2  ] = poly->Vertices[i]->fX();
		bakverts[i*2+1] = poly->Vertices[i]->fY();

		if (bakverts[i * 2] != oldverts[i * 2] || bakverts[i * 2 + 1] != oldverts[i * 2 + 1])
		{
			changed = true;
			poly->Vertices[i]->set(
				oldverts[i * 2] + (bakverts[i * 2] - oldverts[i * 2]) * smoothratio,
				oldverts[i * 2 + 1] + (bakverts[i * 2 + 1] - oldverts[i * 2 + 1]) * smoothratio);
		}
	}
	if (refcount == 0 && !changed)
	{
		UnlinkFromMap();
		Destroy();
	}
	else
	{
		bakcx = poly->CenterSpot.pos.X;
		bakcy = poly->CenterSpot.pos.Y;
		poly->CenterSpot.pos.X = bakcx + (bakcx - oldcx) * smoothratio;
		poly->CenterSpot.pos.Y = bakcy + (bakcy - oldcy) * smoothratio;

		poly->ClearSubsectorLinks();
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DPolyobjInterpolation::Serialize(FSerializer &arc)
{
	Super::Serialize(arc);
	arc("poly", poly)
		("oldverts", oldverts)
		("oldcx", oldcx)
		("oldcy", oldcy);
	if (arc.isReading()) bakverts.Resize(oldverts.Size());
}

//==========================================================================
//
//
//
//==========================================================================

DInterpolation *side_t::SetInterpolation(int position)
{
	if (textures[position].interpolation == nullptr)
	{
		textures[position].interpolation = Create<DWallScrollInterpolation>(this, position);
	}
	textures[position].interpolation->AddRef();
	GC::WriteBarrier(textures[position].interpolation);
	return textures[position].interpolation;
}

//==========================================================================
//
//
//
//==========================================================================

void side_t::StopInterpolation(int position)
{
	if (textures[position].interpolation != nullptr)
	{
		textures[position].interpolation->DelRef();
	}
}

//==========================================================================
//
//
//
//==========================================================================

DInterpolation *sector_t::SetInterpolation(int position, bool attach)
{
	if (interpolations[position] == nullptr)
	{
		DInterpolation *interp;
		switch (position)
		{
		case sector_t::CeilingMove:
			interp = Create<DSectorPlaneInterpolation>(this, true, attach);
			break;

		case sector_t::FloorMove:
			interp = Create<DSectorPlaneInterpolation>(this, false, attach);
			break;

		case sector_t::CeilingScroll:
			interp = Create<DSectorScrollInterpolation>(this, true);
			break;

		case sector_t::FloorScroll:
			interp = Create<DSectorScrollInterpolation>(this, false);
			break;

		default:
			return nullptr;
		}
		interpolations[position] = interp;
	}
	interpolations[position]->AddRef();
	GC::WriteBarrier(interpolations[position]);
	return interpolations[position];
}

//==========================================================================
//
//
//
//==========================================================================

DInterpolation *FPolyObj::SetInterpolation()
{
	if (interpolation != nullptr)
	{
		interpolation->AddRef();
	}
	else
	{
		interpolation = Create<DPolyobjInterpolation>(this);
		interpolation->AddRef();
	}
	GC::WriteBarrier(interpolation);
	return interpolation;
}

//==========================================================================
//
//
//
//==========================================================================

void FPolyObj::StopInterpolation()
{
	if (interpolation != nullptr)
	{
		interpolation->DelRef();
	}
}


