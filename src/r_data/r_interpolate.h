#ifndef R_INTERPOLATE_H
#define R_INTERPOLATE_H

#include "dobject.h"

struct FLevelLocals;
//==========================================================================
//
//
//
//==========================================================================

class DInterpolation : public DObject
{
	DECLARE_ABSTRACT_CLASS(DInterpolation, DObject)
	HAS_OBJECT_POINTERS
public:
	TObjPtr<DInterpolation*> Next;
	TObjPtr<DInterpolation*> Prev;
	FLevelLocals *Level;
	int refcount = 0;

	DInterpolation(FLevelLocals *l = nullptr) : Level(l) {}

	int AddRef();
	int DelRef(bool force = false);

	virtual void UnlinkFromMap();
	virtual void UpdateInterpolation() = 0;
	virtual void Restore() = 0;
	virtual void Interpolate(double smoothratio) = 0;

	virtual void Serialize(FSerializer &arc) override;

	void CallUnlinkFromMap();
	void CallUpdateInterpolation();
	void CallRestore();
	void CallInterpolate(double smoothratio);
};

//==========================================================================
//
//
//
//==========================================================================

struct FInterpolator
{
	TObjPtr<DInterpolation*> Head = MakeObjPtr<DInterpolation*>(nullptr);
	bool didInterp = false;
	int count = 0;

	int CountInterpolations ();

public:
	void UpdateInterpolations();
	void AddInterpolation(DInterpolation *);
	void RemoveInterpolation(DInterpolation *);
	void DoInterpolations(double smoothratio);
	void RestoreInterpolations();
	void ClearInterpolations();
};


#endif

