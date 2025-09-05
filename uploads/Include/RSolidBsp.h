#ifndef _RSOLIDBSP_H
#define _RSOLIDBSP_H

#include "RTypes.h"
#include "RNameSpace.h"

_NAMESPACE_REALSPACE2_BEGIN

class RImpactPlanes : public list<rplane> {
public:
	bool Add(rplane& p);
};

enum RCOLLISIONMETHOD {
	RCW_SPHERE,
	RCW_CYLINDER
};

class RSolidBspNode
{
private:
	bool GetColPlanes_Recurse(int nDepth = 0);

	static RCOLLISIONMETHOD m_ColMethod;
	static float	m_fColRadius;
	static float	m_fColHeight;
	static rvector	m_ColOrigin;
	static rvector	m_ColTo;
	static RImpactPlanes* m_pOutList;
	static float	fImpactDist;
	static rvector	impact;
	static rplane	impactPlane;

public:

	static bool				m_bTracePath;

	rplane			m_Plane;
	RSolidBspNode* m_pPositive, * m_pNegative;
	bool			m_bSolid;

	RSolidBspNode();
	virtual ~RSolidBspNode();

	bool GetColPlanes_Cylinder(RImpactPlanes* pOutList, const rvector& origin, const rvector& to, float fRadius, float fHeight);
	bool GetColPlanes_Sphere(RImpactPlanes* pOutList, const rvector& origin, const rvector& to, float fRadius);
	rvector GetLastColPos() { return impact; }
	rplane GetLastColPlane() { return impactPlane; }

	static bool CheckWall(RSolidBspNode* pRootNode, rvector& origin, rvector& targetpos, float fRadius, float fHeight = 0.f, RCOLLISIONMETHOD method = RCW_CYLINDER, int nDepth = 0, rplane* pimpactplane = NULL);
	static bool CheckWall2(RSolidBspNode* pRootNode, RImpactPlanes& impactPlanes, rvector& origin, rvector& targetpos, float fRadius, float fHeight, RCOLLISIONMETHOD method);

#ifndef _PUBLISH
	int				nPolygon;
	rvector* pVertices;
	rvector* pNormals;

	rboundingbox m_bb;

	void DrawPolygon();
	void DrawPolygonWireframe();
	void DrawPolygonNormal();

	void DrawSolidPolygon();
	void DrawSolidPolygonWireframe();
	void DrawSolidPolygonNormal();

	void DrawPos(rvector& pos);
	void DrawPlaneVertices(rplane& plane);
	void ConstructBoundingBox();
#endif
};

_NAMESPACE_REALSPACE2_END

#endif