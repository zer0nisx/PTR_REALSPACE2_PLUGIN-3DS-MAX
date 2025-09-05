#ifndef _RBOUNDARY_H
#define _RBOUNDARY_H

#include "RTypes.h"
#include "vector"

enum CDInfoType
{
	CDINFO_CLOTH
};

typedef struct
{
	rvector* v;
	rvector* n;
	rvector* pos;
} sClothCDInfo, psClothCDInfo;

union CDInfo
{
	sClothCDInfo clothCD;
};

class RBoundary;

typedef std::vector<RBoundary*> COList;
typedef std::vector<RBoundary*>::iterator iterCOList;

class RBoundary
{
public:

public:

	virtual bool isCollide(CDInfo* data_, CDInfoType cdType_) { return false; };

	virtual bool create() { return true; };

	virtual void draw() {};

	virtual void setSphere(rvector centre_, float radius_) {};
	virtual void setTopCentre(const rvector& in_) {};
	virtual void setBottomCentre(const rvector& in_) {};

public:
	RBoundary(void);
	~RBoundary(void);
};

#endif