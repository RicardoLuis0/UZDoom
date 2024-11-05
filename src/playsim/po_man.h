#ifndef __PO_MAN_H
#define __PO_MAN_H

#include "tarray.h"
#include "r_defs.h"
#include "m_bbox.h"
#include "dthinker.h"

struct FPolyObj;

class DPolyAction : public DThinker
{
	DECLARE_CLASS(DPolyAction, DThinker)
	HAS_OBJECT_POINTERS
public:
	void Construct(FPolyObj *polyNum);
	void Serialize(FSerializer &arc);
	void OnDestroy() override;
	void Stop();
	double GetSpeed() const { return m_Speed; }

	void StopInterpolation();
protected:
	FPolyObj *m_PolyObj;
	double m_Speed;
	double m_Dist;
	TObjPtr<DInterpolation*> m_Interpolation;

	void SetInterpolation();
};

struct FPolyVertex
{
	DVector2 pos;

	FPolyVertex &operator=(vertex_t *v)
	{
		pos = v->fPos();
		return *this;
	}
};

struct FPolySeg
{
	FPolyVertex v1;
	FPolyVertex v2;
	side_t *wall;
};

//
// Linked lists of polyobjects
//
struct FPolyObj;
struct FPolyNode
{
	FPolyObj *poly;				// owning polyobject
	FPolyNode *pnext;			// next polyobj in list
	FPolyNode *pprev;			// previous polyobj

	subsector_t *subsector;		// containimg subsector
	FPolyNode *snext;			// next subsector

	TArray<FPolySeg> segs;		// segs for this node
	int state;
};

enum EPolyObjFlags
{
	POLYF_CARRYING = 0x00000001,
};

// ===== Polyobj data =====
struct FPolyObj
{
	FLevelLocals			*Level;
	TArray<side_t *>		Sidedefs;
	TArray<line_t *>		Linedefs;
	TArray<vertex_t *>		Vertices;
	TArray<FPolyVertex>		OriginalPts;
	TArray<FPolyVertex>		PrevPts;
	FPolyVertex				AnchorSpot;
	FPolyVertex				StartSpot;
	FPolyVertex				CenterSpot;
	FBoundingBox			Bounds;	// Bounds in map coordinates 
	subsector_t				*CenterSubsector;
	int						MirrorNum;

	DAngle		Angle;
	int			tag;			// reference tag assigned in HereticEd
	int			bbox[4];		// bounds in blockmap coordinates
	int			validcount;
	int			crush; 			// should the polyobj attempt to crush mobjs?
	bool		bHurtOnTouch;	// should the polyobj hurt anything it touches?
	bool		bBlocked;
	uint8_t		bHasPortals;	// 1 for any portal, 2 for a linked portal (2 must block rotations.)
	int			seqType;
	int			flags;
	double		Size;			// polyobj size (area of POLY_AREAUNIT == size of FRACUNIT)
	FPolyNode	*subsectorlinks;
	TObjPtr<DPolyAction*> specialdata;	// pointer to a thinker, if the poly is moving
	TObjPtr<DInterpolation*> interpolation;

	FPolyObj();
	DInterpolation *SetInterpolation();
	void StopInterpolation();

	int GetMirror();
	bool MovePolyobj (const DVector2 &pos, bool force = false);
	bool RotatePolyobj (DAngle angle, bool fromsave = false);
	void ClosestPoint(const DVector2 &fpos, DVector2 &out, side_t **side) const;
	void LinkPolyobj ();
	void RecalcActorFloorCeil(FBoundingBox bounds) const;
	void CreateSubsectorLinks();
	void ClearSubsectorLinks(bool absolute = false);
	void CalcCenter();
	void UpdateLinks();
	static void ClearAllSubsectorLinks();

	DVector2 RotOffset(DVector2 Offset);
	DVector2 UnRotOffset(DVector2 Offset);

	inline DVector2 CalcLocalOffset(DVector2 pos)
	{
		return AnchorSpot.pos + RotOffset(pos - StartSpot.pos);
	}

	inline DVector2 UnRotAnchorOffset(DVector2 pos)
	{
		return StartSpot.pos + UnRotOffset(pos - AnchorSpot.pos);
	}

	inline void ToLocalOffset(DVector2 &pos)
	{
		pos = AnchorSpot.pos + RotOffset(pos - StartSpot.pos);
	}

	template<typename... T>
	inline void ToLocalOffsets(T&... pos)
	{
		(ToLocalOffset(pos), ...);
	}

	template<typename T, typename... V>
	static inline void ComplexToLocalOffsets(T &ref, V&... pos)
	{
		if constexpr(std::is_pointer_v<T>)
		{
			if(ref && ref->IsComplexPolyObj())
			{
				ref->GetPolyObj()->ToLocalOffsets(pos...);
			}
		}
		else
		{
			if(ref.IsComplexPolyObj())
			{
				ref.GetPolyObj()->ToLocalOffsets(pos...);
			}
		}
	}

	template<typename T>
	static inline DVector2 ComplexCalcLocalOffset(T &ref, DVector2 pos)
	{
		if constexpr(std::is_pointer_v<T>)
		{
			if(ref && ref->IsComplexPolyObj())
			{
				return ref->GetPolyObj()->CalcLocalOffset(pos);
			}
		}
		else
		{
			if(ref.IsComplexPolyObj())
			{
				return ref.GetPolyObj()->CalcLocalOffset(pos);
			}
		}
		return pos;
	}

	template<typename T>
	static inline DVector2 ComplexUnRotAnchorOffset(T &ref, DVector2 pos)
	{
		if constexpr(std::is_pointer_v<T>)
		{
			if(ref && ref->IsComplexPolyObj())
			{
				return ref->GetPolyObj()->UnRotAnchorOffset(pos);
			}
		}
		else
		{
			if(ref.IsComplexPolyObj())
			{
				return ref.GetPolyObj()->UnRotAnchorOffset(pos);
			}
		}
		return pos;
	}

	template<typename T>
	static inline DVector2 ComplexRotOffset(T &ref, DVector2 pos)
	{
		if constexpr(std::is_pointer_v<T>)
		{
			if(ref && ref->IsComplexPolyObj())
			{
				return ref->GetPolyObj()->RotOffset(pos);
			}
		}
		else
		{
			if(ref.IsComplexPolyObj())
			{
				return ref.GetPolyObj()->RotOffset(pos);
			}
		}
		return pos;
	}

private:

	void ThrustMobj (AActor *actor, side_t *side);
	void UpdateBBox ();
	void DoMovePolyobj (const DVector2 &pos);
	void UnLinkPolyobj ();
	bool CheckMobjBlocking (side_t *sd);

};

struct polyblock_t
{
	FPolyObj *polyobj;
	struct polyblock_t *prev;
	struct polyblock_t *next;
};


void PO_LinkToSubsectors(FLevelLocals *Level);


// ===== PO_MAN =====

typedef enum
{
	PODOOR_NONE,
	PODOOR_SLIDE,
	PODOOR_SWING,
} podoortype_t;

bool EV_RotatePoly (FLevelLocals *Level, line_t *line, int polyNum, int speed, int byteAngle, int direction, bool overRide);
bool EV_MovePoly (FLevelLocals *Level, line_t *line, int polyNum, double speed, DAngle angle, double dist, bool overRide);
bool EV_MovePolyTo (FLevelLocals *Level, line_t *line, int polyNum, double speed, const DVector2 &pos, bool overRide);
bool EV_OpenPolyDoor (FLevelLocals *Level, line_t *line, int polyNum, double speed, DAngle angle, int delay, double distance, podoortype_t type);
bool EV_StopPoly (FLevelLocals *Level, int polyNum);

bool PO_Busy (FLevelLocals *Level, int polyobj);


#endif
