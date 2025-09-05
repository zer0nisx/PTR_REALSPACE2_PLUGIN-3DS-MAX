#ifndef _RPATH_H
#define _RPATH_H

#include <list>
#include <vector>

using namespace std;

#include "RTypes.h"
#include "RNameSpace.h"

class MZFileSystem;

_NAMESPACE_REALSPACE2_BEGIN

class RPathNode;

class RPath {
public:
	int		nEdge;				
	int		nIndex;				
};

typedef vector<rvector*>	RVERTEXLIST;
typedef vector<RPath*>		RPATHLIST;

class RPathNode {
public:
	RPathNode(void);
	~RPathNode(void);

	rplane plane;
	RVERTEXLIST vertices;
	RPATHLIST	m_Neighborhoods;

	int	m_nIndex;				

	int	m_nSourceID;			
	int	m_nGroupID;				

	void*	m_pUserData;

	void DrawWireFrame(DWORD color);
	void DeletePath(int nIndex);			
};

class RPathList : public vector<RPathNode*> {
public:
	virtual ~RPathList();

	void DeleteAll();

	bool Save(const char *filename,int nSourcePolygon);
	bool Open(const char *filename,int nSourcePolygon,MZFileSystem *pfs=NULL);

	bool ConstructNeighborhood();
	bool ConstructGroupIDs();
	bool EliminateInvalid();

	int GetGroupCount() { return m_nGroup; }

protected:
	int	m_nGroup;

	void MarkGroupID(RPathNode *pNode,int nGroupID);			
};

_NAMESPACE_REALSPACE2_END

#endif