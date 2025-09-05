#ifndef _RCLOTH_H
#define _RCLOTH_H

#include "vector"
#include "RMesh.h"
#include "RBoundary.h"

typedef struct constraint {
	int refA;
	int refB;
	float restLength;
}
sConstraint;

struct rvector;
struct RVertex;

#define	CLOTH_VALET_ONLY    0x00
#define CLOTH_HOLD			0x01
#define CLOTH_FORCE			0x02
#define CLOTH_COLLISION		0x04

class RCloth
{
public:
	RCloth(void);
	virtual ~RCloth(void);

protected:

	virtual void accumulateForces();

	virtual void varlet();

	virtual void satisfyConstraints();

public:

	virtual void update();
	virtual void render();

	virtual void UpdateNormal();

	void addCollisionObject(RBoundary* co_);

	void	SetAccelationRatio(float t_) { m_AccelationRatio = min(max(t_, 0), 1); }
	float	GetAccelationRatio() const { return m_AccelationRatio; }

	void	SetTimeStep(float t_) { m_fTimeStep = t_; }
	float	GetTimeStep() const { return m_fTimeStep; }

	void	SetNumIteration(int n) { m_nCntIter = n; }
	int		GetNumIteration() const { return m_nCntIter; }

protected:

	int		m_nCntP;
	int		m_nCntC;

	int		m_nCntIter;
	float	m_fTimeStep;
	float	m_AccelationRatio;

	rvector* m_pX;
	rvector* m_pOldX;
	rvector* m_pForce;
	int* m_pHolds;
	float* m_pWeights;
	rvector* m_pNormal;

	sConstraint* m_pConst;

	COList		mCOList;

	int			m_nNumVertices;
	RVertex* m_pVertices;

	LPDIRECT3DVERTEXBUFFER9 m_pVertexBuffer;
	LPDIRECT3DINDEXBUFFER9	m_pIndexBuffer;
};

enum WIND_TYPE
{
	NO_WIND = 0,
	RANDOM_WIND,
	CALM_WIND,
	LIGHT_AIR_WIND,
	SLIGHT_BREEZE_WIND,
	GENTLE_BREEZE_WIND,
	MODERATE_BREEZE_WIND,
	FRESH_BREEZE_WIND,
	STRONG_BREEZE_WIND,
	NEAR_GALE_WIND,
	GALE_WIND,
	STRONG_GALE_WIND,
	STROM_WIND,
	VIOLENT_STROM_WIND,
	HURRICANE_WIND,
	NUM_WIND_TYPE,
};

class RWindGenerator
{
public:
	RWindGenerator() :m_WindDirection(0, 0, 0), m_WindPower(0.f), m_Time(0), m_WindType(RANDOM_WIND), m_ResultWind(0, 0, 0), m_bFlag(false), m_fTemp1(0), m_fTemp2(0) {}
	virtual ~RWindGenerator() {}

public:

	rvector	GetWind() { return m_ResultWind; }

	void		Update(DWORD time);

	void		SetWindDirection(rvector& inDir) { m_WindDirection = inDir; }
	void		SetWindPower(float inPower) { m_WindPower = inPower; }
	void		SetDelayTime(DWORD inDelay) { m_DelayTime = inDelay; }
	void		SetWindType(WIND_TYPE type) { m_WindType = type; }

protected:

	rvector		m_WindDirection;
	float		m_WindPower;
	DWORD		m_Time;
	rvector		m_ResultWind;
	WIND_TYPE	m_WindType;
	bool		m_bFlag;
	float		m_fTemp1, m_fTemp2;
	DWORD		m_DelayTime;
};

#endif