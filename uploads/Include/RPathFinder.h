


#ifndef RPATHFINDER_H
#define RPATHFINDER_H

#include <list>
using namespace std;

#include "RNameSpace.h"
#include "RTypes.h"
#include "CMPtrList.h"
#include "MPolygonMapModel.h"
#include "RPath.h"

_NAMESPACE_REALSPACE2_BEGIN

class RBspObject;
class RSBspNode;

class RVECTORLIST : public list<rvector*>{
public:
	virtual ~RVECTORLIST(void){
		Clear();
	}

	void Clear() {
		while(empty()==false){
			delete *begin();
			erase(begin());
		}
	}
};


class MRPathNode : public MNodeModel{
	RPathList*	m_pPathList;
	RPathNode*	m_pPathNode;
public:
	int			m_nShortestPathID;		

	static rvector		m_StartPos;		
	static rvector		m_EndPos;		
	static MRPathNode*	m_pStartNode;	
	static MRPathNode*	m_pEndNode;		
public:
	MRPathNode(RPathList* pPathList, RPathNode* pPathNode);
	virtual ~MRPathNode(void);

	RPathNode* GetRPathNode(void){ return m_pPathNode; }
	int GetRPathNodeIndex(void){ return m_pPathNode->m_nIndex; }

	
	virtual int GetSuccessorCount(void){
		return (int)m_pPathNode->m_Neighborhoods.size();
	}
	virtual MNodeModel* GetSuccessor(int i){
		_ASSERT(i>=0 && i<(int)m_pPathNode->m_Neighborhoods.size());
		return (MNodeModel*)((*m_pPathList)[m_pPathNode->m_Neighborhoods[i]->nIndex])->m_pUserData;
	}

	
	static bool GetSuccessorPortal(rvector v[2], MRPathNode* pParent, MRPathNode* pSuccessor);

	
	virtual float GetSuccessorCost(MNodeModel* pSuccessor);
	
	virtual float GetSuccessorCostFromStart(MNodeModel* pSuccessor);
	
	virtual float GetHeuristicCost(MNodeModel* pNode);
};


struct RPATHRESULT{
	rvector		Pos;
	MRPathNode*	pPathNode;

	RPATHRESULT(void){}
	RPATHRESULT(rvector& Pos, MRPathNode* pPathNode){
		RPATHRESULT::Pos = Pos;
		RPATHRESULT::pPathNode = pPathNode;
	}
};

class RPathResultList : public list<RPATHRESULT*>{
public:
	virtual ~RPathResultList(void){
		Clear();
	}

	void Clear() {
		while(empty()==false){
			delete *begin();
			erase(begin());
		}
	}
};


class RPathFinder{
	RBspObject*	m_pBSPObject;	

	RPathNode*	m_pStartNode;	
	RPathNode*	m_pEndNode;		
	rvector		m_StartPos;		
	rvector		m_EndPos;		
	MAStar		m_PolygonPathFinder;		
	MPolygonMapModel	m_PolygonMapModel;	
	float		m_fEnlarge;
protected:

	bool FindVisibilityPath(RVECTORLIST* pVisibilityPathList, rvector& StartPos, rvector& EndPos, CMPtrList<MNodeModel>* pPolygonShortestPath, MPolygonMapModel* pPolygonMapModel=NULL);
	bool FindVisibilityPath(RPathResultList* pVisibilityPathList, rvector& StartPos, rvector& EndPos, CMPtrList<MNodeModel>* pPolygonShortestPath, MPolygonMapModel* pPolygonMapModel=NULL);
public:
	RPathFinder(void);
	virtual ~RPathFinder(void);

	void Create(RBspObject* pBSPObject);
	void Destroy(void);

	
	
	
	bool SetStartPos(int sx, int sy);
	bool SetStartPos(rvector &position);

	
	void SetStartPos(RPathNode *pStartNode,rvector StartPos);

	
	
	
	bool SetEndPos(int sx, int sy);
	bool SetEndPos(rvector &position);

	
	
	
	bool FindPath(RVECTORLIST* pVisibilityPathList);
	bool FindPath(RPathResultList* pVisibilityPathList);

	
	
	bool FindStraightPath(RVECTORLIST* pVisibilityPathList);

	
	void Enlarge(float fMargin);

	RPathNode* GetStartNode(void){ return m_pStartNode; }
	RPathNode* GetEndNode(void){ return m_pEndNode; }
	MAStar* GetPolygonPathFinder(void){ return &m_PolygonPathFinder; }
	MPolygonMapModel* GetPolygonMapModel(void){ return &m_PolygonMapModel; }
};





bool IsPosInNode(rvector& Pos, RPathNode* pNode);

int IsNodeConnection(RPathNode* pNode1, RPathNode* pNode2, RPathList* pPathList);

bool IsPathAcrossPortal(rvector& Pos, rvector& NextPos, RPathNode* pNode, RPathNode* pNextNode, int nNodeNeighborhoodID, RPathList* pPathList);

bool IsPathInNode(rvector& Pos, rvector& NextPos, RPathNode* pNode, RPathNode* pNextNode, int nNodeNeighborhoodID1, int nNodeNeighborhoodID2, RPathList* pPathList);


_NAMESPACE_REALSPACE2_END

#endif