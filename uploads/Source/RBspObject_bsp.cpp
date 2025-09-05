

#include <crtdbg.h>
#include <map>

#include "MXml.h"
#include "MZFileSystem.h"
#include "RBspObject.h"
#include "RMaterialList.h"
#include "RealSpace2.h"
#include "RBaseTexture.h"
#include "MDebug.h"
#include "RVersions.h"
#include "RMaterialList.h"
#include "RVisualMeshMgr.h"
#include "FileInfo.h"
#include "ROcclusionList.h"
#include "MProfiler.h"
#include "RShaderMgr.h"
#include "RLenzFlare.h"

using namespace std;

_NAMESPACE_REALSPACE2_BEGIN

#define TEST_NODE_POS rvector(0,0,200)

struct RWinding {
	int nCount;
	rvector *pVertices;

	RWinding();
	~RWinding() { delete []pVertices; }
	RWinding(int n) { _ASSERT(n>2 && n<100); nCount=n; pVertices=new rvector[n]; }
	RWinding(RWinding *source) { 
		nCount=source->nCount; 
		pVertices=new rvector[nCount]; 
		memcpy(pVertices,source->pVertices,sizeof(rvector)*nCount); }

	void Draw(DWORD color) {
		{
			struct LVERTEX {
				float x, y, z;		
				DWORD color;
			} ;

			RGetDevice()->SetTexture(0,NULL);
			RGetDevice()->SetRenderState( D3DRS_LIGHTING, FALSE );

			LVERTEX lines[1024];

			RGetDevice()->SetVertexShader( D3DFVF_XYZ | D3DFVF_DIFFUSE );

			for(int i=0;i<nCount+1;i++)
			{
				lines[i].x=pVertices[i%nCount].x;
				lines[i].y=pVertices[i%nCount].y;
				lines[i].z=pVertices[i%nCount].z;
				lines[i].color=color;
			}
			HRESULT hr=RGetDevice()->DrawPrimitiveUP(D3DPT_LINESTRIP,nCount,lines,sizeof(LVERTEX));
		}
	}

	void Dump()
	{
		static char buffer[256],buffer2[256];
		sprintf(buffer,"n = %d ",nCount);
		for(int i=0;i<nCount;i++)
		{
			sprintf(buffer2,"(%4.f,%4.f,%4.f) ",pVertices[i].x,pVertices[i].y,pVertices[i].z);
			strcat(buffer,buffer2);
		}
		OutputDebugString(buffer);
	}
};

#define	ON_EPSILON	0.1
#define MAX_WINDING_POINTS	1024

enum SIDE {
	SIDE_FRONT,
	SIDE_ON,
	SIDE_BACK
} ;


RWinding *ClipWinding(RWinding *pWinding, rplane &plane)
{
	SIDE sides[MAX_WINDING_POINTS];
	float dists[MAX_WINDING_POINTS];

	int nFront=0,nBack=0;
	for(int i=0;i<pWinding->nCount;i++)
	{
		float fDot=D3DXPlaneDotCoord(&plane,&pWinding->pVertices[i]);
		if(fDot>ON_EPSILON) {
			sides[i]=SIDE_FRONT;
			nFront++;
		} else 
			if(fDot<-ON_EPSILON) {
				sides[i]=SIDE_BACK;
				nBack++;
			}
			else
				sides[i]=SIDE_ON;
		dists[i]=fDot;
	}
	sides[i]=sides[0];
	dists[i]=dists[0];

	if(!nFront)	 
	{
		delete pWinding;
		return NULL;
	}

	if(!nBack)	
		return pWinding;

	int nPointCount=0;

	for(i=0;i<pWinding->nCount;i++)
	{
		if(sides[i]!=SIDE_BACK)
			nPointCount++;

		if( sides[i] != SIDE_ON &&
			sides[i+1] != SIDE_ON &&
			sides[i] != sides[i+1] )
		{
			nPointCount++;
		}
	}

	RWinding *neww= new RWinding(nPointCount);

	nPointCount=0;
	for (i=0;i<pWinding->nCount;i++)
	{
		rvector *p1 = &pWinding->pVertices[i];

		if( sides[i] != SIDE_BACK )
		{
			neww->pVertices[nPointCount]=*p1;
			nPointCount++;
		}

		if( sides[i] != SIDE_ON &&
			sides[i+1] != SIDE_ON &&
			sides[i] != sides[i+1] )
		{
			rvector *p2  = &pWinding->pVertices[(i+1)%pWinding->nCount];
			rvector *mid = &neww->pVertices[nPointCount];
			nPointCount++;

			float t = dists[i] / (dists[i]-dists[i+1]);
			*mid = *p1 + t * ( *p2 - *p1 );
		}
	}

	
	delete pWinding;
	return neww;
}


struct RNode;

struct RPortal {

	RPortal() { pWinding=NULL; pFrom=pTo=NULL; }
	~RPortal() { SAFE_DELETE(pWinding); }

	rplane		plane;
	RWinding	*pWinding;
	RNode		*pFrom,*pTo;
	int			nID;
};

struct lplane
{
	bool operator()(const rplane *a, const rplane *b) const
	{
		return 

		a->d < b->d ? true : 
		a->d > b->d ? false :

		a->a < b->a ? true :
		a->a > b->a ? false :

		a->b < b->b ? true :
		a->b > b->b ? false :

		a->c < b->c ? true :
		a->c > b->c ? false :

		false;
	}
};

typedef list<RPortal*> RPORTALLIST;

#define IS_EQ_PLANE(x,y) (IS_EQ(x.a,y.a) && IS_EQ(x.b,y.b) && IS_EQ(x.c,y.c) && IS_EQ(x.d,y.d))

int g_nPortalCount;	


class RPortalList : public RPORTALLIST {
public:
	iterator find(rplane &plane) {
		for(iterator i=begin();i!=end();i++)
		{
			if(IS_EQ_PLANE((*i)->plane,plane))
				return i;
		}
		return end();
	}

	void Split(rplane &plane) {
		for(iterator i=begin();i!=end();)
		{
			RPortal *pPortal=*i;
			if(!IS_EQ_PLANE(pPortal->plane,plane))
			{
				RPortal *pFront=new RPortal;
				pFront->nID=g_nPortalCount++;
				pFront->pFrom=pPortal->pFrom;
				pFront->pTo=pPortal->pTo;
				pFront->pWinding=new RWinding(pPortal->pWinding);
				pFront->pWinding=ClipWinding(pFront->pWinding,plane);
				if(pFront->pWinding)
				{
					pFront->plane=pPortal->plane;
					i=insert(i,pFront);
					i++;
				}else
					delete pFront;

				
				pPortal->pWinding=ClipWinding(pPortal->pWinding,-plane);
				if(!pPortal->pWinding)
				{
					delete(pPortal);
					i=erase(i);
					continue;
				}
			}
			i++;
		}
	}

};

struct RNode {
	RSBspNode *pNode;
	RPortalList portallist;
	RNode *Positive,*Negative;
	bool	bVisited;

	RNode() { Positive=Negative=NULL; }
	~RNode() { 
		SAFE_DELETE(Positive);
		SAFE_DELETE(Negative);
	}

	RNode *GetLeafNode(rvector &pos)
	{
		if(!pNode || (!pNode->Negative && !pNode->Positive)) {
			
			return this;
		}
		if(pNode->plane.a*pos.x+pNode->plane.b*pos.y+pNode->plane.c*pos.z+pNode->plane.d>0)
			return Positive ? Positive->GetLeafNode(pos) : NULL;
		else
			return Negative ? Negative->GetLeafNode(pos) : NULL;
	}


};

RWinding *g_testwinding=NULL;
RWinding *g_testwinding2=NULL;
RWinding *g_testwinding3=NULL;

RBspObject *g_pbsp=NULL;
RSBspNode *g_pRootNode=NULL;



RWinding *NewWindingFromBoundingBox(rboundingbox &bb,int i)
{
	int nAxis=i/2;
	float fCoordAxis= (i%2) ? bb.vmin[nAxis] : bb.vmax[nAxis];

	int nAxis1=(nAxis+1)%3,nAxis2=(nAxis+2)%3;

	RWinding *w= new RWinding(4);
	w->pVertices[0][nAxis]=fCoordAxis;
	w->pVertices[0][nAxis1]=bb.vmin[nAxis1];
	w->pVertices[0][nAxis2]=bb.vmin[nAxis2];

	w->pVertices[1][nAxis]=fCoordAxis;
	w->pVertices[1][nAxis1]=bb.vmin[nAxis1];
	w->pVertices[1][nAxis2]=bb.vmax[nAxis2];

	w->pVertices[2][nAxis]=fCoordAxis;
	w->pVertices[2][nAxis1]=bb.vmax[nAxis1];
	w->pVertices[2][nAxis2]=bb.vmax[nAxis2];

	w->pVertices[3][nAxis]=fCoordAxis;
	w->pVertices[3][nAxis1]=bb.vmax[nAxis1];
	w->pVertices[3][nAxis2]=bb.vmin[nAxis2];

	return w;
}

RWinding *NewLargeWinding(rplane &plane)
{
#define PORTALMAX 100000.f
#define PORTALMIN -100000.f

	RWinding *w=new RWinding(4);

	int nAxis;

	if(fabs(plane.a)>fabs(plane.b) && fabs(plane.a)>fabs(plane.c) )
		nAxis=0;
	else if(fabs(plane.b)>fabs(plane.c))
		nAxis=1;
	else
		nAxis=2;

	int nAxis1=(nAxis+1)%3,nAxis2=(nAxis+2)%3;

	float *fplane=(float*)plane;
	w->pVertices[0][nAxis1]=PORTALMIN;
	w->pVertices[0][nAxis2]=PORTALMIN;
	w->pVertices[0][nAxis]=-(fplane[nAxis1]*w->pVertices[0][nAxis1]+fplane[nAxis2]*w->pVertices[0][nAxis2]+plane.d)/fplane[nAxis];

	w->pVertices[1][nAxis1]=PORTALMAX;
	w->pVertices[1][nAxis2]=PORTALMIN;
	w->pVertices[1][nAxis]=-(fplane[nAxis1]*w->pVertices[1][nAxis1]+fplane[nAxis2]*w->pVertices[1][nAxis2]+plane.d)/fplane[nAxis];

	w->pVertices[2][nAxis1]=PORTALMAX;
	w->pVertices[2][nAxis2]=PORTALMAX;
	w->pVertices[2][nAxis]=-(fplane[nAxis1]*w->pVertices[2][nAxis1]+fplane[nAxis2]*w->pVertices[2][nAxis2]+plane.d)/fplane[nAxis];

	w->pVertices[3][nAxis1]=PORTALMIN;
	w->pVertices[3][nAxis2]=PORTALMAX;
	w->pVertices[3][nAxis]=-(fplane[nAxis1]*w->pVertices[3][nAxis1]+fplane[nAxis2]*w->pVertices[3][nAxis2]+plane.d)/fplane[nAxis];

	return w;

}


bool CheckWinding(RWinding *pWinding, rplane &plane)
{
	SIDE sides[MAX_WINDING_POINTS];
	float dists[MAX_WINDING_POINTS];

	int nFront=0,nBack=0;
	for(int i=0;i<pWinding->nCount;i++)
	{
		float fDot=D3DXPlaneDotCoord(&plane,&pWinding->pVertices[i]);
		if(fDot>ON_EPSILON) {
			sides[i]=SIDE_FRONT;
			nFront++;
		} else 
			if(fDot<-ON_EPSILON) {
				sides[i]=SIDE_BACK;
				nBack++;
			}
			else
				sides[i]=SIDE_ON;
		dists[i]=fDot;
	}
	sides[i]=sides[0];
	dists[i]=dists[0];

	if(!nBack)	
		return false;

	return true;
}

void GetPlane(rplane *plane,rboundingbox &bb,int i)
{
	int nAxis=i/2;
	int nAxis1=(nAxis+1)%3,nAxis2=(nAxis+2)%3;

	float *fplane=(float*)*plane;
	fplane[nAxis1]=fplane[nAxis2]=0;
	fplane[nAxis]=(i%2) ? 1 : -1;
	plane->d=(i%2) ?  -bb.vmin[nAxis] : bb.vmax[nAxis];
}



RSBspNode *GetAdjacencyNode(RSBspNode *pNode, int i)
{
	rvector pos=.5f*(pNode->bbTree.vmax+pNode->bbTree.vmin);
	int nAxis=i/2;

	pos[nAxis]=(i%2) ? pNode->bbTree.vmin[nAxis]-2.f : pNode->bbTree.vmax[nAxis]+2.f;
	return g_pRootNode->GetLeafNode(pos);
}

enum CLIP_SEPERATOR_DIR
{ 
	CLIP_SEPERATOR_FORWARD,
	CLIP_SEPERATOR_BACKWARD
};

RWinding *ClipToSeperators ( RWinding *source, RWinding *pass, RWinding *target, CLIP_SEPERATOR_DIR clipdir )
{
	long	cur_src_point;
	long	next_src_point;
	long	cur_pass_point;
	rvector	v1, v2;
	rplane	plane;

	long	i;
	float	d;
	float	length;
	long	front_count;

	if( ! source || ! pass ) return target;

	
	for (cur_src_point=0 ; cur_src_point<source->nCount; cur_src_point++)
	{
		
		next_src_point = (cur_src_point+1)%source->nCount;
		v1 = source->pVertices[next_src_point] - source->pVertices[cur_src_point];

		for (cur_pass_point=0 ; cur_pass_point<pass->nCount; cur_pass_point++)
		{
			
			v2 = pass->pVertices[cur_pass_point] - source->pVertices[cur_src_point];

			
			rvector normal;
			CrossProduct( &normal,  v1 ,v2  );
			length = DotProduct( normal, normal );			
			if (length < ON_EPSILON)
				continue;
			length = (float)sqrt(length);
			plane.a = normal.x/length;
			plane.b = normal.y/length;
			plane.c = normal.z/length;
			plane.d = -D3DXPlaneDotNormal(&plane,&pass->pVertices[cur_pass_point]);

			
			front_count = 0;
			for (i=0 ; i<source->nCount ; i++)
			{
				if (i == cur_src_point || i == next_src_point)
					continue;
				d = D3DXPlaneDotCoord(&plane,&source->pVertices[i]);
				if (d < -ON_EPSILON)
					break;
				if (d > ON_EPSILON)
				{
					front_count++;
					
					
					break;
				}
			}
			
			
			
			
			if (i == source->nCount)
				continue;

			
			if ( front_count )
				plane=-plane;

			
			
			
			front_count = 0;
			for (i=0 ; i<pass->nCount; i++)
			{
				if (i==cur_pass_point)
					continue;
				d = D3DXPlaneDotCoord(&plane,&pass->pVertices[i]);
				if (d < -ON_EPSILON)
					break;
				if (d > ON_EPSILON)
					front_count++;
			}
			
			if (i != pass->nCount)
				continue;
			
			if ( ! front_count )
				continue;

			
			
			
			if ( clipdir == CLIP_SEPERATOR_BACKWARD )
			{
				plane=-plane;
			}

			
			
			target = ClipWinding (target, plane);
			if (!target)
				return NULL;		
		}
	}

	return target;
}

list<RSBspNode *> g_nodelist;

RNode *testnode=NULL;
RNode *g_Tree;

void DrawTest(RNode *pNode)
{
	if(!pNode) return;

	RPORTALLIST::iterator i=pNode->portallist.begin();
	for(size_t n=0;n<pNode->portallist.size();n++)
	{
		RPortal *pPortal=*i;
		if(pPortal->pWinding)
			pPortal->pWinding->Draw(0xffff00ff);
		i++;
	}

	

	

}


void DrawDebug()
{


	

	

	if(g_testwinding)
		g_testwinding->Draw(0xff0000);
	if(g_testwinding2)
		g_testwinding2->Draw(0xffff00);
	if(g_testwinding3)
		g_testwinding3->Draw(0x00ffff);

}

struct RFindPVSParam {
	int nDirFrom;
	RNode	*pNode;
	RWinding *pSource;
	RWinding *pTarget;
	rplane targetplane;

	RFindPVSParam() {
		pSource=NULL;
		pTarget=NULL;
	}
	~RFindPVSParam() { 
		SAFE_DELETE(pSource);
		SAFE_DELETE(pTarget);
	}
};

int nCount=0;

void FindNodes(RNode *pNodeFrom,RFindPVSParam *pParam)
{
	nCount++;
	if(nCount>10) 
	{
		
	}

	RSBspNode *pNode=pParam->pNode->pNode;

	if(!pNode->bVisibletest)
	{
		pNode->bVisibletest=true;
		g_nodelist.push_back(pNode);
	}

	RPORTALLIST *portallist=&pParam->pNode->portallist;
	for(RPORTALLIST::iterator i=portallist->begin();i!=portallist->end();i++)
	{
		RPortal *pPortal=*i;
		_ASSERT(pPortal->pWinding->nCount>0 && pPortal->pWinding->nCount<100);

		if(pPortal->pTo==pNodeFrom) continue;

		RFindPVSParam *pp=new RFindPVSParam;
		pp->pNode=pPortal->pTo;
		pp->pSource=new RWinding(pParam->pSource);
		pp->pTarget=new RWinding(pPortal->pWinding);
		pp->targetplane=pPortal->plane;

		if(pParam->pTarget)
		{
			pp->pTarget=ClipWinding(pp->pTarget,-pParam->targetplane);
			if(!pp->pTarget) goto exit;
			pp->pTarget=ClipToSeperators(pp->pSource,pParam->pTarget,pp->pTarget,CLIP_SEPERATOR_FORWARD);
			if(!pp->pTarget) goto exit;
			pp->pTarget=ClipToSeperators(pParam->pTarget,pp->pSource,pp->pTarget,CLIP_SEPERATOR_BACKWARD);
			if(!pp->pTarget) goto exit;
			pp->pSource=ClipWinding(pp->pSource,pParam->targetplane);
			if(!pp->pSource) goto exit;
			pp->pSource=ClipToSeperators(pp->pTarget,pParam->pTarget,pp->pSource,CLIP_SEPERATOR_FORWARD);
			if(!pp->pSource) goto exit;
			pp->pSource=ClipToSeperators(pParam->pTarget,pp->pTarget,pp->pSource,CLIP_SEPERATOR_BACKWARD);
			if(!pp->pSource) goto exit;
		}

		FindNodes(pParam->pNode,pp);
exit:
		delete pp;
	}
}

void MakeVisTest(RBspObject *pbsp)
{
	g_pRootNode=pbsp->GetOcRootNode();

	RNode *pNode=g_Tree->GetLeafNode(TEST_NODE_POS);

	RPORTALLIST *portallist=&pNode->portallist;
	for(RPORTALLIST::iterator i=portallist->begin();i!=portallist->end();i++)
	{
		RPortal *pPortal=*i;

		RFindPVSParam *pp=new RFindPVSParam;
		pp->pNode=pPortal->pTo;
		pp->pSource=new RWinding(pPortal->pWinding);
		pp->pTarget=NULL;

		pp->pSource->Dump();
		FindNodes(pNode,pp);
		delete pp;
	}
}

void ClipPortal(RNode *rn,RPortalList *pportallist)
{
	RSBspNode *pNode=rn->pNode;

	
	_ASSERT(!rn->Positive && !rn->Negative);

	

	if(pportallist->empty()) return;

	if(pNode)
	{
		for(int j=0;j<pNode->nPolygon;j++)
		{
			RPOLYGONINFO *pInfo=&pNode->pInfo[j];

			for(RPORTALLIST::iterator i=pportallist->begin();i!=pportallist->end();)
			{
				RPortal *pPortal=*i;
				_ASSERT(pPortal->pWinding->nCount>0 && pPortal->pWinding->nCount<100);

				if(IS_EQ_PLANE(pPortal->plane,pInfo->plane))
				{
					bool bSkipSplit=false;
					rplane planes[3];

					for(int k=0;k<3;k++)
					{
						rvector apoint=*pNode->pVertices[j*3+k].Coord();
						rvector edge=apoint-*pNode->pVertices[j*3+(k+1)%3].Coord();
						rvector normal;
						CrossProduct(&normal,edge,rvector(pInfo->plane.a,pInfo->plane.b,pInfo->plane.c));
						Normalize(normal);

						D3DXPlaneFromPointNormal(&planes[k],&apoint,&normal);
						if(!CheckWinding(pPortal->pWinding,planes[k]))
						{
							bSkipSplit=true;
							break;
						}
					}
					if(bSkipSplit) 
					{
						i++;
						continue;	
					}

					for(k=0;k<3;k++)
					{
						rplane *edgeplane=&planes[k];
						RWinding *pPortalWinding;
						if(k==2)
						{
							pPortalWinding=ClipWinding(pPortal->pWinding,*edgeplane);
							pPortal->pWinding=NULL;
						}
						else
						{
							pPortalWinding=new RWinding(pPortal->pWinding);
							pPortalWinding=ClipWinding(pPortalWinding,*edgeplane);
							pPortal->pWinding=ClipWinding(pPortal->pWinding,-*edgeplane);
						}

						if(pPortalWinding)
						{
							RPortal *pp=new RPortal;
							pp->pWinding=pPortalWinding;
							_ASSERT(pp->pWinding->nCount>0 && pp->pWinding->nCount<100);
							pp->plane=pPortal->plane;
							pp->pFrom=pPortal->pFrom;
							pp->pTo=pPortal->pTo;

							

							i=pportallist->insert(i,pp);
							i++;
						}
						if(!pPortal->pWinding) break;
					}

					delete pPortal;
					i=pportallist->erase(i);
				}else
					i++;
			}
		}
	}

	rn->portallist.insert(rn->portallist.end(),pportallist->begin(),pportallist->end());
	while(!pportallist->empty())
	{
		pportallist->erase(pportallist->begin());
	}

}

void PushPortal(RNode *rn, RPortal *pPortal)
{
	RSBspNode *pNode=rn->pNode;

	
	if(!rn->Positive && !rn->Negative)
	{
		return;
	}

	if(pNode->plane==pPortal->plane)		
	{
		if(rn->Positive && rn->Negative)
		{
			RPortal *pBack=new RPortal;
			pBack->nID=pPortal->nID;		
			pBack->pWinding=new RWinding(pPortal->pWinding);
			pBack->plane=-pPortal->plane;


			
			pPortal->pFrom=rn->Positive;
			PushPortal(rn->Positive,pPortal);

			
			pBack->pFrom=rn->Negative;
			PushPortal(rn->Negative,pBack);

			pBack->pTo=pPortal->pFrom;
			pPortal->pTo=pBack->pFrom;

			if(pPortal->pFrom!=pPortal->pTo){
				RPortalList portallist;
				portallist.push_back(pPortal);
				ClipPortal(pPortal->pFrom,&portallist);

				portallist.push_back(pBack);
				ClipPortal(pBack->pFrom,&portallist);
			}
		}else
		{
			_ASSERT(FALSE);
		}
	}else
	{
		int np=0,nz=0,nn=0;
		for(int i=0;i<pPortal->pWinding->nCount;i++)	
		{
			float fSide;
			fSide=D3DXPlaneDotCoord(&pNode->plane,&pPortal->pWinding->pVertices[i]);

			
			if(fSide<-ON_EPSILON)
			{
				nn++;
			}else
				if(fSide>ON_EPSILON)
				{
					np++;
				}else
					nz++;
		}

		_ASSERT(np*nn==0);

		if(np)
		{
			if(pPortal->pTo==rn) pPortal->pTo=rn->Positive;
			if(pPortal->pFrom==rn) pPortal->pFrom=rn->Positive;
			PushPortal(rn->Positive,pPortal);
		}

		if(nn)
		{
			if(pPortal->pTo==rn) pPortal->pTo=rn->Negative;
			if(pPortal->pFrom==rn) pPortal->pFrom=rn->Negative;
			PushPortal(rn->Negative,pPortal);
		}
	}
}

void SplitPortal(RNode *rn,RPortal *pSourcePortal,RPortalList *portallist)
{
	if(!pSourcePortal->pWinding) {
		delete pSourcePortal;	
		return;
	}

	if(!rn->Positive && !rn->Negative) {
		delete pSourcePortal;	
		return;
	}

	RSBspNode *pNode=rn->pNode;

	RPortal *pFront=new RPortal;
	pFront->plane=pSourcePortal->plane;
	pFront->pWinding=new RWinding(pSourcePortal->pWinding);

	if(pNode->plane!=pSourcePortal->plane) {
		pFront->pWinding=ClipWinding(pFront->pWinding,pNode->plane);
		
		pSourcePortal->pWinding=ClipWinding(pSourcePortal->pWinding,-pNode->plane);

		portallist->Split(pNode->plane);
	}

	if(rn->Positive)
		SplitPortal(rn->Positive,pFront,portallist);
	if(rn->Negative)
		SplitPortal(rn->Negative,pSourcePortal,portallist);
}




void GeneratePortals(RNode *rn)
{
	if(!rn->Positive && !rn->Negative) return;

	RSBspNode *pNode=rn->pNode;
	if(!pNode->Positive && !pNode->Negative) return;


	{
		
		RPortal *pPortal=new RPortal;
		pPortal->nID=g_nPortalCount++;
		pPortal->pWinding=NewLargeWinding(pNode->plane);
		pPortal->plane=pNode->plane;

		for(int k=0; k<6; k++)
		{
			rplane plane;
			GetPlane(&plane,g_pRootNode->bbTree,k);
			pPortal->pWinding=ClipWinding(pPortal->pWinding,plane);
			if(!pPortal->pWinding) break;
		}
		if(pPortal->pWinding)
		{
			pPortal->plane=pNode->plane;

			RPortalList portallist;
			portallist.push_back(pPortal);

			static int nCount=0;
			mlog("push portal %d %d \n",nCount++,g_nPortalCount);

			RPortal *pSourcePortal=new RPortal;
			pSourcePortal->plane=pPortal->plane;
			pSourcePortal->pWinding=new RWinding(pPortal->pWinding);
			SplitPortal(g_Tree,pSourcePortal,&portallist);
			
			RPortalList::iterator i;
			for(i=portallist.begin();i!=portallist.end();i++)
			{
				PushPortal(g_Tree,*i);
			}

		}else
			delete pPortal;
	}

	if(rn->Positive)
		GeneratePortals(rn->Positive);
	if(rn->Negative)
		GeneratePortals(rn->Negative);
}

RNode *MakeTree(RSBspNode *pNode)
{
	if(!pNode) return NULL;

	RNode *pnode=new RNode;
	pnode->pNode=pNode;

	pnode->Negative=MakeTree(pNode->Negative);
	pnode->Positive=MakeTree(pNode->Positive);

	if(pnode->Negative!=NULL && pnode->Positive==NULL )
	{
		pnode->Positive=new RNode;
		pnode->Positive->pNode=NULL;
	}


	if(pnode->Negative==NULL && pnode->Positive!=NULL )
	{
		pnode->Negative=new RNode;
		pnode->Negative->pNode=NULL;
	}

	return pnode;
}

void ClearCheckEmpty(RNode *rn)
{
	if(NULL == rn) return;

	rn->bVisited=false;

	ClearCheckEmpty(rn->Positive);
	ClearCheckEmpty(rn->Negative);
}

int g_nEmptyCount=0;

void CheckEmpty(RNode *rn)
{
	if(rn->bVisited) return;
	rn->bVisited=true;

	if(rn->pNode)
	{
		rn->pNode->bSolid=false;
		g_nEmptyCount++;
	}

	for(RPortalList::iterator i=rn->portallist.begin();i!=rn->portallist.end();i++)
	{
		RPortal *pPortal=*i;

		_ASSERT(pPortal->pFrom==rn);
		
		CheckEmpty(pPortal->pTo);
	}
}

void RBspObject::test_MakePortals()
{
	g_pRootNode=GetRootNode();
	g_Tree=MakeTree(GetRootNode());
	
}

#define MAX_LIGHTMAP_SIZE		1024
#define MAX_LEVEL_COUNT			10			



#define DEFAULT_BUFFER_SIZE	1000



#define INDEXBUFFERSIZE	3000


int nsplitcount=0,nleafcount=0;
int g_nPoly,g_nCall;
int g_nPickCheckPolygon,g_nRealPickCheckPolygon;


int				g_nCreatingPosition;
BSPVERTEX		*g_pLPVertices;
RSBspNode		*g_pLPNode;
RPOLYGONINFO	*g_pLPInfo;

rvector			*g_pLPColVertices;
RColBspNode		*g_pLPColNode;


#define BSP_FVF	(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX2)

void DrawBoundingBox(rboundingbox *bb,DWORD color)
{
	int i,j;

	int ind[8][3]= { {0,0,0},{1,0,0},{1,1,0},{0,1,0}, {0,0,1},{1,0,1},{1,1,1},{0,1,1} };
	int lines[12][2] = { {0,1},{1,5},{5,4},{4,0},{5,6},{1,2},{0,3},{4,7},{7,6},{6,2},{2,3},{3,7} };

	for(i=0;i<12;i++)
	{

		rvector a,b;
		for(j=0;j<3;j++)
		{
			a[j]=ind[lines[i][0]][j] ? bb->vmax[j] : bb->vmin[j];
			b[j]=ind[lines[i][1]][j] ? bb->vmax[j] : bb->vmin[j];
		}

		RDrawLine(a,b,color);
	}
}

static bool m_bisDrawLightMap;





RSBspNode::RSBspNode()
{
	Positive=Negative=NULL;
	nPolygon=0;
	pVertices=NULL;
	pInfo=NULL;

	nFrameCount=-1;
	pVerticeOffsets=NULL;
	pVerticeCounts=NULL;

	bSolid=TRUE;
}

RSBspNode::~RSBspNode()
{


	SAFE_DELETE(pVerticeOffsets);
	SAFE_DELETE(pVerticeCounts);
}

void RSBspNode::DrawBoundingBox(DWORD color)
{
	RGetDevice()->SetVertexShader( D3DFVF_XYZ | D3DFVF_DIFFUSE );
	RealSpace2::DrawBoundingBox(&bbTree,color);

	if(Negative) Negative->DrawBoundingBox(color);
	if(Positive) Positive->DrawBoundingBox(color);
}

void RSBspNode::DrawWireFrame(int nFace,DWORD color)
{
	for(int i=0;i<3;i++)
	{
		RDrawLine(*pVertices[nFace*3+i%3].Coord(),*pVertices[nFace*3+(i+1)%3].Coord(),color);
	}
}

RSBspNode* RSBspNode::GetLeafNode(rvector &pos)
{
	if(nPolygon) return this;
	if(plane.a*pos.x+plane.b*pos.y+plane.c*pos.z+plane.d>0)
		return Positive->GetLeafNode(pos);
	else
		return Negative->GetLeafNode(pos);
}

RColBspNode::RColBspNode()
{
	Positive=NULL;
	Negative=NULL;
#ifndef _PUBLISH
	pVertices=NULL;
	pNormals=NULL;
#endif
}

RColBspNode::~RColBspNode()
{
#ifndef _PUBLISH
	SAFE_DELETE(pNormals);
#endif
}

#ifndef _PUBLISH
void RColBspNode::DrawPolygon()
{
	

	if(nPolygon && !bSolid)
		RGetDevice()->DrawPrimitiveUP(D3DPT_TRIANGLELIST,nPolygon,pVertices,sizeof(rvector));

	if(Positive)
		Positive->DrawPolygon();

	if(Negative)
		Negative->DrawPolygon();

}

void RColBspNode::DrawPolygonWireframe()
{
	

	if(nPolygon && !bSolid)
	{
		rvector v[4];
		for(int i=0;i<nPolygon;i++)
		{
			memcpy(v,pVertices[i*3],3*sizeof(rvector));
			v[3]=v[0];

			RGetDevice()->DrawPrimitiveUP(D3DPT_LINESTRIP,3,&v,sizeof(rvector));
		}
	}

	if(Positive)
		Positive->DrawPolygonWireframe();

	if(Negative)
		Negative->DrawPolygonWireframe();

}

void RColBspNode::DrawPolygonNormal()
{
	

	if(nPolygon && !bSolid)
	{
		rvector v[2];
		for(int i=0;i<nPolygon;i++)
		{
			rvector center=1.f/3.f*(pVertices[i*3]+pVertices[i*3+1]+pVertices[i*3+2]);

			v[0]=center;
			v[1]=center+pNormals[i]*50.f;
			RGetDevice()->DrawPrimitiveUP(D3DPT_LINESTRIP,1,&v,sizeof(rvector));
		}
	}

	if(Positive)
		Positive->DrawPolygonNormal();

	if(Negative)
		Negative->DrawPolygonNormal();

}

void RColBspNode::DrawSolidPolygon()
{


	if(nPolygon && bSolid)
		RGetDevice()->DrawPrimitiveUP(D3DPT_TRIANGLELIST,nPolygon,pVertices,sizeof(rvector));

	if(Positive)
		Positive->DrawSolidPolygon();

	if(Negative)
		Negative->DrawSolidPolygon();
		 
}

void RColBspNode::DrawSolidPolygonWireframe()
{


	if(nPolygon && bSolid)
	{
		rvector v[4];
		for(int i=0;i<nPolygon;i++)
		{
			memcpy(v,pVertices[i*3],3*sizeof(rvector));
			v[3]=v[0];

			RGetDevice()->DrawPrimitiveUP(D3DPT_LINESTRIP,3,&v,sizeof(rvector));
		}
	}

	if(Positive)
		Positive->DrawSolidPolygonWireframe();

	if(Negative)
		Negative->DrawSolidPolygonWireframe();

}

void RColBspNode::DrawSolidPolygonNormal()
{
	

	if(nPolygon && bSolid)
	{
		rvector v[2];
		for(int i=0;i<nPolygon;i++)
		{
			rvector center=1.f/3.f*(pVertices[i*3]+pVertices[i*3+1]+pVertices[i*3+2]);

			v[0]=center;
			v[1]=center+pNormals[i]*50.f;
			RGetDevice()->DrawPrimitiveUP(D3DPT_LINESTRIP,1,&v,sizeof(rvector));
		}
	}

	if(Positive)
		Positive->DrawSolidPolygonNormal();

	if(Negative)
		Negative->DrawSolidPolygonNormal();

}

void RColBspNode::DrawPos(rvector &pos)
{
	if(nPolygon) {
		if((timeGetTime()/500) %2 == 0) {
			RGetDevice()->SetVertexShader( D3DFVF_XYZ );
			RGetDevice()->SetRenderState(D3DRS_ZENABLE, FALSE );
			RGetDevice()->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
			RGetDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
			RGetDevice()->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
			RGetDevice()->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
			RGetDevice()->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1 );
			RGetDevice()->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
			RGetDevice()->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
			RGetDevice()->SetRenderState(D3DRS_TEXTUREFACTOR ,   0xff00ffff);
			RGetDevice()->DrawPrimitiveUP(D3DPT_TRIANGLELIST,nPolygon,pVertices,sizeof(rvector));
			
			float fSize=10.f;
			RGetDevice()->SetRenderState(D3DRS_POINTSIZE,   *(DWORD*)&fSize);
			RGetDevice()->DrawPrimitiveUP(D3DPT_POINTLIST,nPolygon*3,pVertices,sizeof(rvector));
			

		}
		return;
	}

	if(D3DXPlaneDotCoord(&plane,&pos)<0) {
		if(Negative)
			Negative->DrawPos(pos);
	}else {
		if(Positive)
			Positive->DrawPos(pos);
	}
}

void RColBspNode::ConstructBoundingBox()
{
	if(!Positive && !Negative) return;

	if(nPolygon)
	{
		int i,j;
		m_bb.vmin=pVertices[0];
		m_bb.vmax=pVertices[0];
		for(i=1;i<nPolygon*3;i++){
			for(j=0;j<3;j++)
			{
				m_bb.vmin[j]=min(m_bb.vmin[j],pVertices[i][j]);
				m_bb.vmax[j]=max(m_bb.vmax[j],pVertices[i][j]);
			}
		}
		return;
	}

	if(Positive)
		Positive->ConstructBoundingBox();

	if(Negative)
		Negative->ConstructBoundingBox();

	if(Positive)
	{
		m_bb=Positive->m_bb;
		if(Negative)
			MergeBoundingBox(&m_bb,&Negative->m_bb);
	}else
	{
		if(Negative)
			m_bb=Negative->m_bb;
	}
}
#endif




RBspLightmapManager::RBspLightmapManager()
{
	m_nSize=MAX_LIGHTMAP_SIZE;
	m_pData=new DWORD[MAX_LIGHTMAP_SIZE*MAX_LIGHTMAP_SIZE];
	m_pFreeList=new RFREEBLOCKLIST[MAX_LEVEL_COUNT+1];

	POINT p={0,0};
	m_pFreeList[MAX_LEVEL_COUNT].push_back(p);
}

RBspLightmapManager::~RBspLightmapManager()
{
	Destroy();
}

void RBspLightmapManager::Destroy()
{
	SAFE_DELETE(m_pData);
	if(m_pFreeList){
		delete []m_pFreeList;
		m_pFreeList=NULL;
	}
}

float RBspLightmapManager::CalcUnused() 
{
	float fUnused=0.f;

	for(int i=0;i<=MAX_LEVEL_COUNT;i++) {
		float fThisLevelSize=pow(0.25,(MAX_LEVEL_COUNT-i));
		fUnused+=(float)m_pFreeList[i].size()*fThisLevelSize;
	}

	return fUnused;
}

bool RBspLightmapManager::GetFreeRect(int nLevel,POINT *pt)
{
	if(nLevel>MAX_LEVEL_COUNT) return false;

	if(!m_pFreeList[nLevel].size())		
	{
		POINT point;
		if(!GetFreeRect(nLevel+1,&point))	
			return false;

		int nSize=1<<nLevel;

		POINT newpoint;						

		newpoint.x=point.x+nSize;newpoint.y=point.y;
		m_pFreeList[nLevel].push_back(newpoint);

		newpoint.x=point.x;newpoint.y=point.y+nSize;
		m_pFreeList[nLevel].push_back(newpoint);

		newpoint.x=point.x+nSize;newpoint.y=point.y+nSize;
		m_pFreeList[nLevel].push_back(newpoint);

		*pt=point;

	}else
	{
		*pt=*m_pFreeList[nLevel].begin();
		m_pFreeList[nLevel].erase(m_pFreeList[nLevel].begin());
	}

	return true;
}

bool RBspLightmapManager::Add(DWORD *data,int nSize,POINT *retpoint)
{
	int nLevel=0,nTemp=1;
	while(nSize>nTemp)
	{
		nTemp=nTemp<<1;
		nLevel++;
	}
	_ASSERT(nSize==nTemp);

	POINT pt;
	if(!GetFreeRect(nLevel,&pt))		
		return false;

	for(int y=0;y<nSize;y++)
	{
		for(int x=0;x<nSize;x++)
		{
			m_pData[(y+pt.y)*GetSize()+(x+pt.x)]=data[y*nSize+x];
		}
	}
	*retpoint=pt;
	return true;
}

void RBspLightmapManager::Save(const char *filename)
{
	RSaveAsBmp(GetSize(),GetSize(),m_pData,filename);
}

RMapObjectList::~RMapObjectList()
{
	for(iterator i=begin();i!=end();i++)
	{
		ROBJECTINFO *info=*i;
		delete info->pVisualMesh;
		delete info;
	}
}




RBspObject::RBspObject()
{
	m_pBspRoot=NULL;
	m_pOcRoot=NULL;
	m_ppLightmapTextures=NULL;
	m_pMaterials=NULL;
	g_pLPVertices=NULL;
	m_pVertexBuffer=NULL;
	m_pIndexBuffer=NULL;
	m_pConvexVertices=NULL;
	m_pConvexPolygons=NULL;

	m_pBspVertices=NULL;
	m_pOcVertices=NULL;
	m_pBspInfo=NULL;
	m_pOcInfo=NULL;
	m_pOcclusion=NULL;

	m_bWireframe=false;
	m_bShowLightmap=false;
	m_AmbientLight=rvector(0,0,0);

	m_MeshList.SetMtrlAutoLoad(true);
	m_MeshList.SetMapObject(true);

	m_nMaterial=0;
	m_nLightmap=0;
	m_nOcclusion=0;

	m_pColVertices=NULL;
	m_pColRoot=NULL;
}

void RBspObject::ClearLightmaps()
{
	while(m_LightmapList.size())
	{
		delete *m_LightmapList.begin();
		m_LightmapList.erase(m_LightmapList.begin());
	}

	if(m_ppLightmapTextures)
	{
		for(int i=0;i<m_nLightmap;i++)
			SAFE_RELEASE(m_ppLightmapTextures[i]);
		SAFE_DELETE(m_ppLightmapTextures);
	}

	m_nLightmap = 0;
}

void RBspObject::LightMapOnOff(bool bDraw)
{
	if(m_bisDrawLightMap == bDraw)
		return;

	m_bisDrawLightMap = bDraw;

	if(bDraw) {	
		OpenLightmap();
	} else {	
		ClearLightmaps();
		Sort_Nodes(m_pOcRoot);
	}
}

void RBspObject::SetDrawLightMap(bool b) { 
	m_bisDrawLightMap = b; 
}

RBspObject::~RBspObject()
{
	ClearLightmaps();

	SAFE_RELEASE(m_pVertexBuffer);
#ifdef USE_INDEX_BUFFER
	OnInvalidate();
#endif
	SAFE_DELETE(m_pConvexVertices);
	SAFE_DELETE(m_pConvexPolygons);


	SAFE_DELETE(m_pColVertices);
	if(m_pColRoot)
	{
		delete []m_pColRoot;
		m_pColRoot=NULL;
	}


	SAFE_DELETE(m_pBspInfo);
	SAFE_DELETE(m_pBspVertices);
	if(m_pBspRoot)
	{
		delete []m_pBspRoot;
		m_pBspRoot=NULL;
	}

	SAFE_DELETE(m_pOcInfo);
	SAFE_DELETE(m_pOcVertices);
	if(m_pOcRoot)
	{
		delete []m_pOcRoot;
		m_pOcRoot=NULL;
	}

	if(m_nMaterial)
	{
		for(int i=0;i<m_nMaterial;i++)
		{
			RDestroyBaseTexture(m_pMaterials[i].texture);
			m_pMaterials[i].texture=NULL;
		}
		if(m_pMaterials)
		{
			delete []m_pMaterials;
			m_pMaterials=NULL;
		}
	}

	if(m_nOcclusion)
	{
		if(m_pOcclusion)
		{
			delete []m_pOcclusion;
			m_pOcclusion=NULL;
		}
	}

}

void RBspObject::DrawNormal(int nIndex,float fSize)
{
	int j;
	RCONVEXPOLYGONINFO *pInfo=&m_pConvexPolygons[nIndex];

	RGetDevice()->SetVertexShader( D3DFVF_XYZ | D3DFVF_DIFFUSE );

	for(j=0;j<pInfo->nVertices;j++)
	{
		
		RDrawLine(pInfo->pVertices[j],pInfo->pVertices[(j+1)%pInfo->nVertices],0xff808080);
		
		
		RDrawLine(pInfo->pVertices[j],pInfo->pVertices[j]+fSize*pInfo->pNormals[j],0xff00ff);
	}

	
}

void RBspObject::SetDiffuseMap(int nMaterial)
{
	LPDIRECT3DDEVICE8 pd3dDevice=RGetDevice();

	RBaseTexture *pTex=m_pMaterials[nMaterial].texture;
	if(pTex)
	{
		pd3dDevice->SetTexture(0,pTex->GetTexture());
		pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	}
	else
	{
		DWORD dwDiffuse=VECTOR2RGB24(m_pMaterials[nMaterial].Diffuse);
		pd3dDevice->SetRenderState(D3DRS_TEXTUREFACTOR , dwDiffuse);
		pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TFACTOR );
	}
}

void RBspObject::Flush()
{
#ifdef USE_INDEX_BUFFER
	g_nCall++;
	m_pIndexBuffer->Unlock();
	
	RGetDevice()->SetIndices(m_pIndexBuffer,0);
	HRESULT hr=RGetDevice()->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,m_nPolygon*3,0,m_nIndexPosition/3);
	m_nIndexPosition=0;
	m_pIndexBuffer->Lock(0,0,(BYTE**)&m_pIndices,D3DLOCK_DISCARD);
#endif
}

void RBspObject::Draw(RSBspNode *pNode,int nMaterial)
{
	if(pNode->nFrameCount!=g_nFrameCount) return;

	if(pNode->nPolygon)
	{
		int nCount=pNode->pVerticeCounts[nMaterial];
		if(nCount)
		{
			g_nPoly+=nCount;
#ifdef USE_INDEX_BUFFER
			int indexbase=pNode->nPosition+3*pNode->pVerticeOffsets[nMaterial];
			for(int i=0;i<pNode->pVerticeCounts[nMaterial]*3;i++)
			{
				int index=indexbase+i;
				m_pIndices[m_nIndexPosition++]=index;
				if(m_nIndexPosition==INDEXBUFFERSIZE)
					Flush();
			}
#else
			g_nCall++;
			HRESULT hr;
			hr=RGetDevice()->DrawPrimitive(D3DPT_TRIANGLELIST,pNode->nPosition+3*pNode->pVerticeOffsets[nMaterial],nCount);
			_ASSERT(hr==D3D_OK);
#endif
		}
	}else
	{
		if(pNode->Negative) Draw(pNode->Negative,nMaterial);
		if(pNode->Positive) Draw(pNode->Positive,nMaterial);
	}
}

rplane RBViewFrustum[6];

int g_nChosenNodeCount;

void RBspObject::Draw()
{
	g_nPoly=0;
	g_nCall=0;

	if(!m_pVertexBuffer)
		return;

	LPDIRECT3DDEVICE8 pd3dDevice=RGetDevice();
	pd3dDevice->SetVertexShader( BSP_FVF );

	RGetDevice()->SetStreamSource(0,m_pVertexBuffer,sizeof(BSPVERTEX) );

	if(m_bWireframe)
	{
		pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
		pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
		pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
		pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
	}
	else
	{
		if(m_bShowLightmap)
		{
			
			pd3dDevice->SetTextureStageState( 1, D3DTSS_MAGFILTER , D3DTEXF_POINT );
			pd3dDevice->SetTextureStageState( 1, D3DTSS_MINFILTER , D3DTEXF_POINT );

			pd3dDevice->SetRenderState(D3DRS_TEXTUREFACTOR , 0xffffffff);
			pd3dDevice->SetTextureStageState( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
			pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_MODULATE4X );
			pd3dDevice->SetTextureStageState( 1, D3DTSS_COLORARG2, D3DTA_TFACTOR );

		}else
		{
			pd3dDevice->SetTextureStageState( 1, D3DTSS_MAGFILTER , D3DTEXF_LINEAR );
			pd3dDevice->SetTextureStageState( 1, D3DTSS_MINFILTER , D3DTEXF_LINEAR );

			pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
			pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
			pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2 );
			pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );

			if(!m_nLightmap)
			{
				pd3dDevice->SetTextureStageState( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
				pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
				pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
				pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1 );
			}else
			{
				pd3dDevice->SetTextureStageState( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
				pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_MODULATE4X );
				pd3dDevice->SetTextureStageState( 1, D3DTSS_COLORARG2, D3DTA_CURRENT );
				pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
				pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1 );
			}
		}
	}

	pd3dDevice->SetTextureStageState( 2, D3DTSS_COLOROP,   D3DTOP_DISABLE );
	pd3dDevice->SetTextureStageState( 2, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );

	DWORD dwNumPasses;
	HRESULT hr=pd3dDevice->ValidateDevice( &dwNumPasses );
	_ASSERT(hr==D3D_OK);

	pd3dDevice->SetTexture(0,NULL);
	pd3dDevice->SetTexture(1,NULL);
    RGetDevice()->SetRenderState( D3DRS_LIGHTING, FALSE );


	rmatrix mat;
	RGetDevice()->GetTransform(D3DTS_WORLD, &mat);

	for(int i=0;i<m_nOcclusion;i++)
	{
		ROcclusion *poc=&m_pOcclusion[i];

		bool bPositive=D3DXPlaneDotCoord(&poc->plane,&RCameraPosition)>0;
		D3DXPlaneTransform(poc->pPlanes,poc->pPlanes,&mat);

		poc->pPlanes[0] = bPositive ? poc->plane : -poc->plane;
		for(int j=0;j<poc->nCount;j++)
		{
			if(bPositive)
				D3DXPlaneFromPoints(poc->pPlanes+j+1,&poc->pVertices[j],&poc->pVertices[(j+1)%poc->nCount],&RCameraPosition);
			else
				D3DXPlaneFromPoints(poc->pPlanes+j+1,&poc->pVertices[(j+1)%poc->nCount],&poc->pVertices[j],&RCameraPosition);
			D3DXPlaneTransform(poc->pPlanes+j+1,poc->pPlanes+j+1,&mat);
		}
	}


	

	
	

	rmatrix invtrmat;
	D3DXMatrixTranspose(&invtrmat, &mat);

	for(i=0;i<6;i++)
	{
		D3DXPlaneTransform(RBViewFrustum+i,RGetViewFrustum()+i,&invtrmat);
	}

	g_nChosenNodeCount=0;
	_BP("Choose Nodes..");

	ChooseNodes(m_pBspRoot);
	_EP("Choose Nodes..");
	
	RGetDevice()->SetRenderState(D3DRS_ALPHABLENDENABLE, false );
	RGetDevice()->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
	RGetDevice()->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
	RSetWBuffer(true);
	RGetDevice()->SetRenderState(D3DRS_ZWRITEENABLE, true );

	int nCount=m_nMaterial*max(1,m_nLightmap);

	RGetDevice()->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW );

#ifdef USE_INDEX_BUFFER
	m_nIndexPosition=0;
	m_pIndexBuffer->Lock(0,0,(BYTE**)&m_pIndices,D3DLOCK_DISCARD);
#endif

	for(i=0;i<nCount;i++)
	{
		if(m_ppLightmapTextures)
			RGetDevice()->SetTexture(1,m_ppLightmapTextures[i / m_nMaterial]);

		if((m_pMaterials[i % m_nMaterial].dwFlags & (RM_FLAG_ADDITIVE|RM_FLAG_USEOPACITY) ) == 0 )
		{
			SetDiffuseMap(i % m_nMaterial);

			Draw(m_pBspRoot,i);
		}

		if(m_nIndexPosition)
			Flush();
	}

	

	RGetDevice()->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_ALWAYS );
	RGetDevice()->SetRenderState(D3DRS_ALPHATESTENABLE,false);
	RGetDevice()->SetTextureStageState( 0, D3DTSS_ALPHAOP , D3DTOP_DISABLE );
	RGetDevice()->SetTextureStageState( 1, D3DTSS_ALPHAOP , D3DTOP_DISABLE );


	
	RGetDevice()->SetRenderState(D3DRS_ALPHABLENDENABLE, false );
	RGetDevice()->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
	RGetDevice()->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
	RGetDevice()->SetRenderState(D3DRS_ZWRITEENABLE, true );

	pd3dDevice->SetTexture(0,NULL);
	pd3dDevice->SetTexture(1,NULL);
	pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
	pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2 );
	pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
	pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
	pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
	pd3dDevice->SetVertexShader(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1 );

	DrawObjects();


	RGetDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
	RGetDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	RGetDevice()->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
	RGetDevice()->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
}



void RBspObject::SetObjectLight(rvector vPos)
{
	float fIntensityFirst = FLT_MIN;

	float fdistance = 0.f;
	float fIntensity = 0.f;

	RLIGHT *first = NULL;
	RLIGHT *plight = NULL;

	RLightList *pllist = GetObjectLightList();

	D3DLIGHT8 light;

	ZeroMemory( &light, sizeof(D3DLIGHT8) );

	light.Type       = D3DLIGHT_POINT;

	light.Attenuation0 = 0.f;
	light.Attenuation1 = 0.0010f;
	light.Attenuation2 = 0.f;

	for(RLightList::iterator i=pllist->begin();i!=pllist->end();i++) {

		plight = *i;

		fdistance = Magnitude(plight->Position - vPos);
		fIntensity = (fdistance - plight->fAttnStart) / (plight->fAttnEnd - plight->fAttnStart);

		fIntensity = min(max(1.0f-fIntensity,0),1);
		fIntensity *= plight->fIntensity;

		fIntensity = min(max(fIntensity,0),1);

		if(fIntensityFirst < fIntensity) {
			fIntensityFirst=fIntensity;
			first=plight;
		}
	}

	if(first) {

		light.Position = first->Position;









		light.Ambient.r = min(first->Color.x*first->fIntensity * 0.25,1.f);
		light.Ambient.g = min(first->Color.y*first->fIntensity * 0.25,1.f);
		light.Ambient.b = min(first->Color.z*first->fIntensity * 0.25,1.f);

		light.Diffuse.r  = min(first->Color.x*first->fIntensity * 0.25,1.f);
		light.Diffuse.g  = min(first->Color.y*first->fIntensity * 0.25,1.f);
		light.Diffuse.b  = min(first->Color.z*first->fIntensity * 0.25,1.f);

		light.Specular.r = 1.f;
		light.Specular.g = 1.f;
		light.Specular.b = 1.f;

		light.Range       = first->fAttnEnd*1.0f;

		RGetDevice()->SetLight( 0, &light );
		RGetDevice()->LightEnable( 0, TRUE );

		RShaderMgr::getShaderMgr()->setLight( 0, &light );
	}
}

void RBspObject::SetCharactorLight(rvector pos)
{

}


void RBspObject::DrawObjects()
{
	RGetDevice()->SetRenderState(D3DRS_CULLMODE ,D3DCULL_NONE);
	RGetDevice()->SetRenderState(D3DRS_LIGHTING ,TRUE);



	D3DXMATRIX world;
	RGetDevice()->GetTransform(D3DTS_WORLD, &world);

	rvector v_add = rvector(world._41,world._42,world._43);



	for(list<ROBJECTINFO*>::iterator i=m_ObjectList.begin();i!=m_ObjectList.end();i++)
	{
		ROBJECTINFO *pInfo=*i;

		if(pInfo->pLight)
		{
			D3DLIGHT8 light;
			ZeroMemory( &light, sizeof(D3DLIGHT8) );

			light.Type       = D3DLIGHT_POINT;

			light.Attenuation0 = 0.f;

			light.Attenuation1 = 0.0001f;
			light.Attenuation2 = 0.f;

			light.Position = pInfo->pLight->Position + v_add;

			rvector lightmapcolor(1,1,1);

















			light.Diffuse.r  = pInfo->pLight->Color.x*pInfo->pLight->fIntensity;
			light.Diffuse.g  = pInfo->pLight->Color.y*pInfo->pLight->fIntensity;
			light.Diffuse.b  = pInfo->pLight->Color.z*pInfo->pLight->fIntensity;


			light.Range       = pInfo->pLight->fAttnEnd;

			RGetDevice()->SetLight( 0, &light );
			RGetDevice()->LightEnable( 0, TRUE );
			RGetDevice()->LightEnable( 1, FALSE );
		}

		pInfo->pVisualMesh->SetWorldMatrix(world);
		pInfo->pVisualMesh->Frame();
		pInfo->pVisualMesh->Render();


	}

}

void RBspObject::DrawLight(D3DLIGHT8 *pLight)
{
	LPDIRECT3DDEVICE8 pd3dDevice=RGetDevice();
	if(!m_pVertexBuffer)
		return;

	int nChosen=ChooseNodes(m_pOcRoot,rvector(pLight->Position),pLight->Range);


	pd3dDevice->SetVertexShader( BSP_FVF );

	RGetDevice()->SetStreamSource(0,m_pVertexBuffer,sizeof(BSPVERTEX) );

	

	pd3dDevice->SetTexture(1,NULL);
	pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
	pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
	pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );

	RGetDevice()->SetRenderState( D3DRS_AMBIENT, 0 );
	pd3dDevice->SetLight( 0, pLight );
	pd3dDevice->LightEnable( 0, TRUE );
	pd3dDevice->LightEnable( 1, FALSE );
	pd3dDevice->SetRenderState( D3DRS_LIGHTING, TRUE );
	

	pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
	pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
	pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, false);

	pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW );

	for(int i=0;i<m_nMaterial;i++)
	{
		pd3dDevice->SetTexture(0,m_pMaterials[i].texture->GetTexture());
		if((m_pMaterials[i].dwFlags & RM_FLAG_ADDITIVE) == 0 )
			Draw(m_pOcRoot,i);
	}

	pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false );
	pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
	pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
	pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, true );
	pd3dDevice->SetTexture(0,NULL);
	pd3dDevice->SetTexture(1,NULL);
	pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
	pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2 );
	pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
	pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
	pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
	pd3dDevice->SetVertexShader(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1 );
}



void RBspObject::DrawBoundingBox()
{
	RGetDevice()->SetTexture(0,NULL);
	RGetDevice()->SetTexture(1,NULL);
	RGetDevice()->SetVertexShader( D3DFVF_XYZ | D3DFVF_DIFFUSE );
	m_pOcRoot->DrawBoundingBox(0xffffff);
}

void RBspObject::DrawOcclusions()
{
	RGetDevice()->SetTexture(0,NULL);
	RGetDevice()->SetTexture(1,NULL);
	RGetDevice()->SetRenderState(D3DRS_ZENABLE, false );
	RGetDevice()->SetVertexShader( D3DFVF_XYZ | D3DFVF_DIFFUSE );

	for(int i=0;i<m_nOcclusion;i++)
	{
		ROcclusion *poc=&m_pOcclusion[i];
		
		for(int j=0;j<poc->nCount;j++)
		{
			RDrawLine(poc->pVertices[j],poc->pVertices[(j+1)%poc->nCount],0xffff00ff);
		}
	}

	RSetWBuffer(true);
}

#ifndef _PUBLISH
void RBspObject::DrawColNodePolygon(rvector &pos)
{ 
	m_pColRoot->DrawPos(pos); 
}

void RBspObject::DrawCollision_Polygon()
{
	LPDIRECT3DDEVICE8 pd3dDevice=RGetDevice();

	pd3dDevice->SetVertexShader( D3DFVF_XYZ );
	pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW );
	pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

	pd3dDevice->SetRenderState(D3DRS_TEXTUREFACTOR , 0x40808080);
	pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
	pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
	pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
	pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1 );
	pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
	pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );

	m_pColRoot->DrawPolygon();

	RSetWBuffer(true);
	pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, true);
	pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

	pd3dDevice->SetRenderState(D3DRS_TEXTUREFACTOR , 0x40ffffff);
	m_pColRoot->DrawPolygonWireframe();

	pd3dDevice->SetRenderState(D3DRS_TEXTUREFACTOR , 0x40ff00ff);
	m_pColRoot->DrawPolygonNormal();

	pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );

}

void RBspObject::DrawCollision_Solid()
{
	LPDIRECT3DDEVICE8 pd3dDevice=RGetDevice();

	pd3dDevice->SetVertexShader( D3DFVF_XYZ );
	pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW );
	pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

	pd3dDevice->SetRenderState(D3DRS_TEXTUREFACTOR , 0x40808080);
	pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
	pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
	pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
	pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1 );
	pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
	pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );

	m_pColRoot->DrawSolidPolygon();

	RSetWBuffer(true);
	pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, true);
	pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

	pd3dDevice->SetRenderState(D3DRS_TEXTUREFACTOR , 0x40ffffff);
	m_pColRoot->DrawSolidPolygonWireframe();

	pd3dDevice->SetRenderState(D3DRS_TEXTUREFACTOR , 0x40ff00ff);
	m_pColRoot->DrawSolidPolygonNormal();

	pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );

}
#else
void RBspObject::DrawColNodePolygon(rvector &pos)	{}
void RBspObject::DrawCollision_Polygon()			{}
void RBspObject::DrawCollision_Solid()				{}
#endif

void RBspObject::ChooseNodes(RSBspNode *bspNode)
{
	if(isInViewFrustum(&bspNode->bbTree,RBViewFrustum))
	{
		if(bspNode->nPolygon)
		{
			for(int i=0;i<m_nOcclusion;i++)
			{
				bool bVisible=false;

				ROcclusion *poc=&m_pOcclusion[i];

				for(int j=0;j<poc->nCount+1;j++)
				{
					if(isInPlane(&bspNode->bbTree,&poc->pPlanes[j]))
					{
						bVisible=true;
						break;
					}
				}

				
				if(!bVisible) 
					return;
			}
		}

		g_nChosenNodeCount++;
		bspNode->nFrameCount=g_nFrameCount;
		if(bspNode->Negative)
			ChooseNodes(bspNode->Negative);
		if(bspNode->Positive)
			ChooseNodes(bspNode->Positive);
	}
}

int RBspObject::ChooseNodes(RSBspNode *bspNode,rvector &center,float fRadius)
{
	if (bspNode == NULL) return 0;
	if (!bspNode->nPolygon)
	{
		if(IsInSphere(bspNode->bbTree,center,fRadius))
		{
			bspNode->nFrameCount=g_nFrameCount;
			
			int nNegative=ChooseNodes(bspNode->Negative,center,fRadius);
			int nPositive=ChooseNodes(bspNode->Positive,center,fRadius);

			return 1+nNegative+nPositive;
		}
	}
	return 0;
}




bool RBspObject::ReadString(MZFile *pfile,char *buffer,int nBufferSize)
{
	int nCount=0;
	do{
		pfile->Read(buffer,1);
		nCount++;
		buffer++;
		if(nCount>=nBufferSize)
			return false;
	}while((*(buffer-1))!=0);
	return true;
}

bool bPathOnlyLoad = false;

void DeleteVoidNodes(RSBspNode *pNode)
{
	if(pNode->Positive)
		DeleteVoidNodes(pNode->Positive);
	if(pNode->Negative)
		DeleteVoidNodes(pNode->Negative);

	if(pNode->Positive && !pNode->Positive->nPolygon && !pNode->Positive->Positive && !pNode->Positive->Negative)
	{
		SAFE_DELETE(pNode->Positive);
	}
	if(pNode->Negative && !pNode->Negative->nPolygon && !pNode->Negative->Positive && !pNode->Negative->Negative)
	{
		SAFE_DELETE(pNode->Negative);
	}
}

void RecalcBoundingBox(RSBspNode *pNode)
{
	if(pNode->nPolygon)
	{
		rboundingbox *bb=&pNode->bbTree;
		bb->vmin.x=bb->vmin.y=bb->vmin.z=FLT_MAX;
		bb->vmax.x=bb->vmax.y=bb->vmax.z=-FLT_MAX;
		for(int i=0;i<pNode->nPolygon*3;i++)
		{
			for(int j=0;j<3;j++)
			{
				bb->vmin[j]=min(bb->vmin[j],(*pNode->pVertices[i].Coord())[j]);
				bb->vmax[j]=max(bb->vmax[j],(*pNode->pVertices[i].Coord())[j]);
			}
		}
	}
	else
	{
		if(pNode->Positive)
		{
			RecalcBoundingBox(pNode->Positive);
			memcpy(&pNode->bbTree,&pNode->Positive->bbTree,sizeof(rboundingbox));
		}
		if(pNode->Negative)
		{
			RecalcBoundingBox(pNode->Negative);
			memcpy(&pNode->bbTree,&pNode->Negative->bbTree,sizeof(rboundingbox));
		}
		if(pNode->Positive) MergeBoundingBox(&pNode->bbTree,&pNode->Positive->bbTree);
		if(pNode->Negative) MergeBoundingBox(&pNode->bbTree,&pNode->Negative->bbTree);
	}
}

bool RBspObject::Open(const char *filename,ROpenFlag nOpenFlag)
{
	m_filename=filename;

	char xmlname[_MAX_PATH];
	sprintf(xmlname,"%s.xml",filename);
	if(!OpenDescription(xmlname))
		MLog("Error while loading %s\n",xmlname);

	if(!OpenRs(filename))
	{
		MLog("Error while loading %s\n",filename);
		return false;
	}

	if(nOpenFlag!=ROF_EXCEPTBSP)
	{
		char bspname[_MAX_PATH];
		sprintf(bspname,"%s.bsp",filename);
		if(!OpenBsp(bspname))
			MLog("Error while loading %s\n",bspname);
	}

	if(nOpenFlag==ROF_ALL && !OpenLightmap())
	{
		MLog("Error while loading lightmap\n");

		

		Sort_Nodes(m_pBspRoot);
	}

	

	CreatePolygonTable(m_pBspRoot);

	if(nOpenFlag==ROF_ALL && !CreateVertexBuffer())
		MLog("Error while Creating VB\n");


	char colfilename[_MAX_PATH];
	sprintf(colfilename,"%s.col",filename);
	if(nOpenFlag==ROF_ALL && !OpenCol(colfilename))
		MLog("Error while loading %s\n",colfilename);

	OnRestore();

	if(m_bisDrawLightMap==false) {
		m_bisDrawLightMap = true;
		LightMapOnOff(false);
	}

	return true;
}

void RBspObject::OptimizeBoundingBox()
{
	DeleteVoidNodes(m_pOcRoot);
	RecalcBoundingBox(m_pOcRoot);
}

bool RBspObject::CreateIndexBuffer()
{
	RGetDevice()->CreateIndexBuffer(sizeof(WORD)*INDEXBUFFERSIZE,D3DUSAGE_DYNAMIC,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&m_pIndexBuffer);
	m_nIndexPosition=0;
	return true;
}

void RBspObject::OnInvalidate()
{
#ifdef USE_INDEX_BUFFER
	SAFE_RELEASE(m_pIndexBuffer);
#endif

	if(m_ppLightmapTextures)
	{
		for(int i=0;i<m_nLightmap;i++)
		{
			SAFE_RELEASE(m_ppLightmapTextures[i]);
		}
		delete m_ppLightmapTextures;
		m_nLightmap=0;
	}
}

void RBspObject::OnRestore()
{
#ifdef USE_INDEX_BUFFER
	CreateIndexBuffer();
#endif
	if(m_nLightmap==0)
		OpenLightmap();
}

bool RBspObject::Open_MaterialList(MXmlElement *pElement)
{
	RMaterialList ml;
	ml.Open(pElement);
	

	
	
	m_nMaterial=ml.size();
	m_nMaterial=m_nMaterial+1;

	m_pMaterials=new RBSPMATERIAL[m_nMaterial];

	m_pMaterials[0].texture=NULL;	
	m_pMaterials[0].Diffuse=rvector(1,1,1);
	m_pMaterials[0].dwFlags=0;

	RMaterialList::iterator itor;
	itor=ml.begin();

	for(int i=1;i<m_nMaterial;i++)
	{
		RMATERIAL *mat=*itor;

		m_pMaterials[i].dwFlags=mat->dwFlags;
		m_pMaterials[i].Diffuse=mat->Diffuse;
		m_pMaterials[i].Specular=mat->Specular;
		m_pMaterials[i].Ambient=mat->Ambient;
		m_pMaterials[i].Name=mat->Name;
		m_pMaterials[i].DiffuseMap=mat->DiffuseMap;


		string DiffuseMapName=m_filename;
		string::size_type nPos=DiffuseMapName.find_last_of("\\"),nothing=-1;
		if(nPos==nothing)
			nPos=DiffuseMapName.find_last_of("/");

		if(nPos==nothing)
			DiffuseMapName="";
		else
			DiffuseMapName=DiffuseMapName.substr(0,nPos)+"/";

		DiffuseMapName+=mat->DiffuseMap;
		if(DiffuseMapName.length())
		{


			m_pMaterials[i].texture= RCreateBaseTexture(DiffuseMapName.c_str(),RTextureType_Map,true);
		}

		itor++;
	}

	return true;
}


bool RBspObject::Open_LightList(MXmlElement *pElement)
{
	RLightList llist;
	llist.Open(pElement);

	for(RLightList::iterator i=llist.begin();i!=llist.end();i++)
	{
		RLIGHT *plight=*i;
		if(strnicmp(plight->Name.c_str(),RTOK_MAX_OBJLIGHT,strlen(RTOK_MAX_OBJLIGHT))==0)
			m_StaticObjectLightList.push_back(plight);
		else
			m_StaticMapLightList.push_back(plight);
	}
	llist.erase(llist.begin(),llist.end());

	return true;
}

bool RBspObject::Open_OcclusionList(MXmlElement *pElement)
{
	

	ROcclusionList oclist;
	oclist.Open(pElement);

	m_nOcclusion=oclist.size();

	if( m_nOcclusion ) {
		m_pOcclusion=new ROcclusion[m_nOcclusion];
	}

	int nIndex=0;
	for(ROcclusionList::iterator i=oclist.begin();i!=oclist.end();i++)
	{
		ROcclusion *poc=*i;
		m_pOcclusion[nIndex].Name=poc->Name;
		m_pOcclusion[nIndex].nCount=poc->nCount;
		m_pOcclusion[nIndex].pVertices=poc->pVertices;
		m_pOcclusion[nIndex].CalcPlane();
		m_pOcclusion[nIndex].pPlanes=new rplane[poc->nCount+1];
		poc->pVertices=NULL;

		nIndex++;
	}

	return true;
}

bool RBspObject::Open_ObjectList(MXmlElement *pElement)
{
	int i;

	MXmlElement	aObjectNode,aChild;
	int nCount = pElement->GetChildNodeCount();





	char szTagName[256],szContents[256];
	for (i = 0; i < nCount; i++)
	{
		aObjectNode=pElement->GetChildNode(i);
		aObjectNode.GetTagName(szTagName);

		if(stricmp(szTagName,RTOK_OBJECT)==0)
		{
			ROBJECTINFO *pInfo=new ROBJECTINFO;
			aObjectNode.GetAttribute(szContents,RTOK_NAME);
			pInfo->name=szContents;

			char fname[_MAX_PATH];
            GetPurePath(fname,m_descfilename.c_str());
			strcat(fname,szContents);

			

			m_MeshList.SetMtrlAutoLoad(true);
			m_MeshList.SetMapObject(true);

			pInfo->nMeshID = m_MeshList.Add(fname);
			RMesh *pmesh=m_MeshList.GetFast(pInfo->nMeshID);

			if(pmesh)
			{
				strcat(fname,".ani");

				m_AniList.Add(fname,fname,i);
				RAnimation* pAni=m_AniList.GetAnimation(fname);

				pmesh->SetAnimation(pAni);

				pInfo->pVisualMesh=new RVisualMesh;
				pInfo->pVisualMesh->Create(pmesh);
				pInfo->pVisualMesh->SetAnimation(pAni);

				pInfo->pLight=NULL;

				m_ObjectList.push_back(pInfo);
			}
			else
				delete pInfo;
		}

	}


	

	for(list<ROBJECTINFO*>::iterator i=m_ObjectList.begin();i!=m_ObjectList.end();i++)
	{
		ROBJECTINFO *pInfo=*i;

		
		float fIntensityFirst=FLT_MIN;

		pInfo->pVisualMesh->CalcBox();
		rvector center = (pInfo->pVisualMesh->m_vBMax+pInfo->pVisualMesh->m_vBMin)*.5f;


		RLightList *pllist=GetObjectLightList();

		for(RLightList::iterator i=pllist->begin();i!=pllist->end();i++)
		{
			RLIGHT *plight=*i;
			float fdistance=Magnitude(plight->Position-center);
			float fIntensity=(fdistance-plight->fAttnStart)/(plight->fAttnEnd-plight->fAttnStart);
			fIntensity=min(max(1.0f-fIntensity,0),1);
			fIntensity*=plight->fIntensity;
			fIntensity=min(max(fIntensity,0),1);

			if(fIntensityFirst<fIntensity)
			{
				fIntensityFirst=fIntensity;
				pInfo->pLight=plight;
			}
		}
	}

	return true;
}

bool RBspObject::Open_LenzFalreList( MXmlElement* pElement )
{
	int i;

	MXmlElement	aObjectNode,aChild;
	int nCount		= pElement->GetChildNodeCount();

	char szTagName[256],szContents[256];
	rvector pos;

	for (i = 0; i < nCount; i++)
	{
		aObjectNode=pElement->GetChildNode(i);
		aObjectNode.GetTagName(szTagName);

		if( szTagName[0] == '#' )
		{
			continue;
		}

		aObjectNode.GetAttribute( szContents, "name" );
		if( stricmp( szContents, "sun_dummy" ) == 0 )
		{
			int nTemp = aObjectNode.GetChildNodeCount();
			for( int j = 0 ; j < nTemp; ++j )
			{
				aChild = aObjectNode.GetChildNode( j );
				aChild.GetTagName( szTagName );
				if( !stricmp( szTagName, RTOK_POSITION))
				{
					aChild.GetContents( szContents );
					int nSuccess = sscanf( szContents, "%f %f %f", &pos.x, &pos.y, &pos.z );
					_ASSERT( nSuccess == 3 );
					if( !RGetLenzFlare()->SetLight( pos ))
					{
						mlog( "Fail to Set LenzFlare Position...\n" );
					}
				}
			}
		}
	}

	return true;
}

bool RBspObject::OpenDescription(const char *filename)
{

	MZFile mzf;
	if(!mzf.Open(filename,g_pFileSystem))
		return false;

	m_descfilename=filename;

	char *buffer;
	buffer=new char[mzf.GetLength()+1];
	mzf.Read(buffer,mzf.GetLength());
	buffer[mzf.GetLength()]=0;
	
	MXmlDocument aXml;
	aXml.Create();
	if(!aXml.LoadFromMemory(buffer))
	{
		delete buffer;
		return false;
	}

	int iCount, i;
	MXmlElement		aParent, aChild;
	aParent = aXml.GetDocumentElement();
	iCount = aParent.GetChildNodeCount();

	char szTagName[256];
	for (i = 0; i < iCount; i++)
	{
		aChild = aParent.GetChildNode(i);
		aChild.GetTagName(szTagName);
		if(stricmp(szTagName,RTOK_MATERIALLIST)==0)
			Open_MaterialList(&aChild); else
		if(stricmp(szTagName,RTOK_LIGHTLIST)==0)
			Open_LightList(&aChild); else
		if(stricmp(szTagName,RTOK_OBJECTLIST)==0)
			Open_ObjectList(&aChild); else
		if(stricmp(szTagName,RTOK_OCCLUSIONLIST)==0)
			Open_OcclusionList(&aChild);
		if(stricmp(szTagName,RTOK_DUMMYLIST)==0)
			Open_LenzFalreList(&aChild);
	}

	delete buffer;
	mzf.Close();

	return true;
}



bool RBspObject::OpenRs(const char *filename)
{
	MZFile file;
	if(!file.Open(filename,g_pFileSystem)) 
		return false;

	RHEADER header;
	file.Read(&header,sizeof(RHEADER));
	if(header.dwID!=RS_ID || header.dwVersion!=RS_VERSION)
	{
		file.Close();
		return false;
	}

	
	int nMaterial;
	file.Read(&nMaterial,sizeof(int));

	if(m_nMaterial-1!=nMaterial)
		return false;

	for(int i=1;i<m_nMaterial;i++)
	{
		char buf[256];
		if(!ReadString(&file,buf,sizeof(buf)))
			return false;
	}

	Open_ConvexPolygons(&file);

	file.Read(&m_nBspNodeCount,sizeof(int));
	file.Read(&m_nBspPolygon,sizeof(int));

	file.Read(&m_nNodeCount,sizeof(int));
	file.Read(&m_nPolygon,sizeof(int));
	m_pOcRoot=new RSBspNode[m_nNodeCount];
	m_pOcInfo=new RPOLYGONINFO[m_nPolygon];
	m_pOcVertices=new BSPVERTEX[m_nPolygon*3];

	g_pLPNode=m_pOcRoot;
	g_pLPInfo=m_pOcInfo;
	g_nCreatingPosition=0;
	g_pLPVertices=m_pOcVertices;

	Open_Nodes(m_pOcRoot,&file);

	return true;
}

bool RBspObject::OpenBsp(const char *filename)
{
	MZFile file;
	if(!file.Open(filename,g_pFileSystem)) 
		return false;

	RHEADER header;
	file.Read(&header,sizeof(RHEADER));
	if(header.dwID!=RBSP_ID || header.dwVersion!=RBSP_VERSION)
	{
		file.Close();
		return false;
	}

	int nBspNodeCount,nBspPolygon;
	
	file.Read(&nBspNodeCount,sizeof(int));
	file.Read(&nBspPolygon,sizeof(int));

	if(m_nBspNodeCount!=nBspNodeCount || m_nBspPolygon!=nBspPolygon ) 
	{
		file.Close();
		return false;
	}

	m_pBspRoot=new RSBspNode[m_nBspNodeCount];
	m_pBspInfo=new RPOLYGONINFO[m_nBspPolygon];
	m_pBspVertices=new BSPVERTEX[m_nBspPolygon*3];

	g_pLPNode=m_pBspRoot;
	g_pLPInfo=m_pBspInfo;
	g_nCreatingPosition=0;
	g_pLPVertices=m_pBspVertices;

	Open_Nodes(m_pBspRoot,&file);
	_ASSERT(m_pBspRoot+m_nBspNodeCount>g_pLPNode);
	
	file.Close();
	return true;
}

bool RBspObject::Open_ColNodes(RColBspNode *pNode,MZFile *pfile)
{
	pfile->Read(&pNode->plane,sizeof(rplane));

	pfile->Read(&pNode->bSolid,sizeof(bool));

	bool flag;
	pfile->Read(&flag,sizeof(bool));
	if(flag)
	{
		g_pLPColNode++;
		pNode->Positive=g_pLPColNode;
		Open_ColNodes(pNode->Positive,pfile);
	}
	pfile->Read(&flag,sizeof(bool));
	if(flag)
	{
		g_pLPColNode++;
		pNode->Negative=g_pLPColNode;
		Open_ColNodes(pNode->Negative,pfile);
	}

	int nPolygon;
	pfile->Read(&nPolygon,sizeof(int));

#ifndef _PUBLISH
	pNode->nPolygon=nPolygon;
	if(pNode->nPolygon)
	{
		pNode->pVertices=g_pLPColVertices;
		pNode->pNormals=new rvector[pNode->nPolygon];
		g_pLPColVertices+=pNode->nPolygon*3;
		for(int i=0;i<pNode->nPolygon;i++)
		{
			pfile->Read(pNode->pVertices+i*3,sizeof(rvector)*3);
			pfile->Read(pNode->pNormals+i,sizeof(rvector));
		}
	}
#else
	
	rvector temp;
	for(int i=0;i<nPolygon*4;i++)
		pfile->Read(&temp,sizeof(rvector));

#endif

	return true;

}

bool RBspObject::OpenCol(const char *filename)
{
	MZFile file;
	if(!file.Open(filename,g_pFileSystem)) 
		return false;

	RHEADER header;
	file.Read(&header,sizeof(RHEADER));
	if(header.dwID!=R_COL_ID || header.dwVersion!=R_COL_VERSION)
	{
		file.Close();
		return false;
	}

	int nBspNodeCount,nBspPolygon;
	
	file.Read(&nBspNodeCount,sizeof(int));
	file.Read(&nBspPolygon,sizeof(int));

	m_pColRoot=new RColBspNode[nBspNodeCount];
	m_pColVertices=new rvector[nBspPolygon*3];

	g_pLPColNode=m_pColRoot;
	g_pLPColVertices=m_pColVertices;
	g_nCreatingPosition=0;

	Open_ColNodes(m_pColRoot,&file);
	_ASSERT(m_pColRoot+nBspNodeCount>g_pLPColNode);

	file.Close();
#ifndef _PUBLISH	
	m_pColRoot->ConstructBoundingBox();
#endif
	return true;

}


bool SaveMemoryBmp(int x,int y,void *data,void **retmemory,int *nsize);

bool RBspObject::OpenLightmap()
{
	char lightmapinfofilename[_MAX_PATH];
	sprintf(lightmapinfofilename,"%s.lm",m_filename.c_str());

	int i;


	MZFile file;
	if(!file.Open(lightmapinfofilename,g_pFileSystem)) return false;

	RHEADER header;
	file.Read(&header,sizeof(RHEADER));
	if(header.dwID!=R_LM_ID || header.dwVersion!=R_LM_VERSION)
	{
		file.Close();
		return false;
	}

	int nSourcePolygon,nNodeCount;
	
	file.Read(&nSourcePolygon,sizeof(int));
	file.Read(&nNodeCount,sizeof(int));

	
	if(nSourcePolygon!=m_nConvexPolygon || m_nNodeCount!=nNodeCount)
	{
		file.Close();
		return false;
	}

	file.Read(&m_nLightmap,sizeof(int));
	m_ppLightmapTextures=new LPDIRECT3DTEXTURE8[m_nLightmap];
	for(i=0;i<m_nLightmap;i++)
	{
		int nSize;
		void *memory;
		file.Read(&nSize,sizeof(int));

		float fUnused;
		file.Read(&fUnused,sizeof(float));
		memory=new BYTE[nSize*nSize*4];
		file.Read(memory,nSize*nSize*4);

		RBspLightmapManager *plm=new RBspLightmapManager;
		plm->m_fUnused=fUnused;
		plm->SetData((DWORD*)memory);
		plm->SetSize(nSize);
		m_LightmapList.push_back(plm);


		void *bmpmemory;
		int nBmpSize;
		bool bSaved=SaveMemoryBmp(plm->GetSize(),plm->GetSize(),plm->GetData(),&bmpmemory,&nBmpSize);
		_ASSERT(bSaved);

		m_ppLightmapTextures[i]=NULL;

		D3DXCreateTextureFromFileInMemoryEx(
			RGetDevice(),bmpmemory,nBmpSize,
			D3DX_DEFAULT, D3DX_DEFAULT, 
			D3DX_DEFAULT,
			0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, 
			D3DX_FILTER_TRIANGLE|D3DX_FILTER_MIRROR, 
			D3DX_FILTER_TRIANGLE|D3DX_FILTER_MIRROR, 
			0, NULL, NULL, &m_ppLightmapTextures[i]);
		delete bmpmemory;
	}

	

	{
		int *pOrder=new int[m_nPolygon];
		file.Read(pOrder,sizeof(int)*m_nPolygon);

		

		RPOLYGONINFO *pbaseinfo=new RPOLYGONINFO[m_nPolygon];
		BSPVERTEX	*pbasever=new BSPVERTEX[m_nPolygon*3];

		memcpy(pbaseinfo,m_pOcInfo,sizeof(RPOLYGONINFO)*m_nPolygon);
		memcpy(pbasever,m_pOcVertices,sizeof(BSPVERTEX)*m_nPolygon*3);

		for(i=0;i<m_nPolygon;i++)
		{
			memcpy(m_pOcInfo+i,pbaseinfo+pOrder[i],sizeof(RPOLYGONINFO));
			memcpy(m_pOcVertices+i*3,pbasever+pOrder[i]*3,sizeof(BSPVERTEX)*3);
		}

		delete pbaseinfo;
		delete pbasever;
		delete pOrder;
	}

	
	if(!m_pBspRoot)
	{
		int *plightmapnumber=new int[m_nBspPolygon];
		file.Read(plightmapnumber,sizeof(int)*m_nBspPolygon);
	}else
		for(i=0;i<m_nBspPolygon;i++)
			file.Read(&(m_pBspInfo+i)->nLightmapTexture,sizeof(int));
	for(i=0;i<m_nPolygon;i++)
		file.Read(&(m_pOcInfo+i)->nLightmapTexture,sizeof(int));

	
	if(!m_pBspRoot)
	{
		float *puvs=new float[m_nBspPolygon*3*2];
		file.Read(puvs,sizeof(float)*m_nBspPolygon*3*2);
	}else
	{
		for(i=0;i<m_nBspPolygon*3;i++)
			file.Read(&(m_pBspVertices+i)->tu2,sizeof(float)*2);
	}
	for(i=0;i<m_nPolygon*3;i++)
		file.Read(&(m_pOcVertices+i)->tu2,sizeof(float)*2);

	file.Close();
	
	return true;
}

bool RBspObject::Open_ConvexPolygons(MZFile *pfile)
{
	int nConvexVertices;

	pfile->Read(&m_nConvexPolygon,sizeof(int));
	pfile->Read(&nConvexVertices,sizeof(int));


	m_pConvexPolygons=new RCONVEXPOLYGONINFO[m_nConvexPolygon];
	m_pConvexVertices=new rvector[nConvexVertices];
	m_pConvexNormals=new  rvector[nConvexVertices];


	rvector *pLoadingVertex=m_pConvexVertices;
	rvector *pLoadingNormal=m_pConvexNormals;


	for(int i=0;i<m_nConvexPolygon;i++)
	{
		pfile->Read(&m_pConvexPolygons[i].nMaterial,sizeof(int));
		
		m_pConvexPolygons[i].nMaterial+=2;
		pfile->Read(&m_pConvexPolygons[i].dwFlags,sizeof(DWORD));
		pfile->Read(&m_pConvexPolygons[i].plane,sizeof(rplane));
		pfile->Read(&m_pConvexPolygons[i].fArea,sizeof(float));
		pfile->Read(&m_pConvexPolygons[i].nVertices,sizeof(int));

		m_pConvexPolygons[i].pVertices=pLoadingVertex;
		for(int j=0;j<m_pConvexPolygons[i].nVertices;j++)
		{
			pfile->Read(pLoadingVertex,sizeof(rvector));
			pLoadingVertex++;
		}
		m_pConvexPolygons[i].pNormals=pLoadingNormal;
		for(int j=0;j<m_pConvexPolygons[i].nVertices;j++)
		{
			pfile->Read(pLoadingNormal,sizeof(rvector));
			pLoadingNormal++;
		}

		
	}
	return true;
}


void RBspObject::CreatePolygonTable(RSBspNode *pNode)
{
	if(pNode->Positive)
		CreatePolygonTable(pNode->Positive);

	if(pNode->Negative)
		CreatePolygonTable(pNode->Negative);

	if(pNode->nPolygon)
	{
		int nCount=m_nMaterial*max(1,m_nLightmap);

		SAFE_DELETE(pNode->pVerticeOffsets);
		SAFE_DELETE(pNode->pVerticeCounts);

		pNode->pVerticeOffsets=new int[nCount];
		pNode->pVerticeCounts=new int[nCount];

		for(int j=0;j<nCount;j++)
		{
			pNode->pVerticeOffsets[j]=0;
			pNode->pVerticeCounts[j]=0;
		}

		int lastmatind=0,lastmat=pNode->pInfo[0].nMaterial+pNode->pInfo[0].nLightmapTexture*m_nMaterial;

		for(j=1;j<pNode->nPolygon;j++)
		{
			int nMatIndex=pNode->pInfo[j].nMaterial+pNode->pInfo[j].nLightmapTexture*m_nMaterial;
			if(nMatIndex!=lastmat)
			{
				if(lastmat!=-1)
				{
					pNode->pVerticeOffsets[lastmat]=lastmatind;
					pNode->pVerticeCounts[lastmat]=j-lastmatind;
				}
				lastmat=nMatIndex;
				lastmatind=j;
			}
		}

		if(lastmat!=-1)
		{
			pNode->pVerticeOffsets[lastmat]=lastmatind;
			pNode->pVerticeCounts[lastmat]=pNode->nPolygon-lastmatind;
		}
	}
}

void RBspObject::Sort_Nodes(RSBspNode *pNode)
{
	if(pNode->Positive)
		Sort_Nodes(pNode->Positive);
	
	if(pNode->Negative)
		Sort_Nodes(pNode->Negative);

	if(pNode->nPolygon)
	{
		
		
		for(int j=0;j<pNode->nPolygon-1;j++)
		{
			for(int k=j+1;k<pNode->nPolygon;k++)
			{
				RPOLYGONINFO *pj=pNode->pInfo+j,*pk=pNode->pInfo+k;

				if(pj->nLightmapTexture > pk->nLightmapTexture ||
					( pj->nLightmapTexture == pk->nLightmapTexture  && pNode->pInfo[j].nMaterial > pNode->pInfo[k].nMaterial ) )
				{
					BSPVERTEX temp[3];
					memcpy(temp,pNode->pVertices+j*3,sizeof(BSPVERTEX)*3);
					memcpy(pNode->pVertices+j*3,pNode->pVertices+k*3,sizeof(BSPVERTEX)*3);
					memcpy(pNode->pVertices+k*3,temp,sizeof(BSPVERTEX)*3);

					RPOLYGONINFO ttemp;
					memcpy(&ttemp,pNode->pInfo+j,sizeof(ttemp));
					memcpy(pNode->pInfo+j,pNode->pInfo+k,sizeof(ttemp));
					memcpy(pNode->pInfo+k,&ttemp,sizeof(ttemp));
				}
			}
		}
	}
}

bool RBspObject::Open_Nodes(RSBspNode *pNode,MZFile *pfile)
{
	pfile->Read(&pNode->bbTree,sizeof(rboundingbox));
	pfile->Read(&pNode->plane,sizeof(rplane));

	bool flag;
	pfile->Read(&flag,sizeof(bool));
	if(flag)
	{
		g_pLPNode++;
		pNode->Positive=g_pLPNode;
		Open_Nodes(pNode->Positive,pfile);
	}
	pfile->Read(&flag,sizeof(bool));
	if(flag)
	{
		g_pLPNode++;
		pNode->Negative=g_pLPNode;
		Open_Nodes(pNode->Negative,pfile);
	}

	pfile->Read(&pNode->nPolygon,sizeof(int));

	if(pNode->nPolygon)
	{
		pNode->pVertices=g_pLPVertices;g_pLPVertices+=pNode->nPolygon*3;
		pNode->pInfo=g_pLPInfo;g_pLPInfo+=pNode->nPolygon;

		BSPVERTEX *pVertex=pNode->pVertices;
		RPOLYGONINFO *pInfo=pNode->pInfo;

		for(int i=0;i<pNode->nPolygon;i++)
		{
			int mat;

			rvector c1,c2,c3,nor;

			pfile->Read(&mat,sizeof(int));
			pfile->Read(&pInfo->nConvexPolygon,sizeof(int));
			pfile->Read(&pInfo->dwFlags,sizeof(DWORD));
			pfile->Read(pVertex,sizeof(BSPVERTEX));c1=*pVertex->Coord();pVertex++;
			pfile->Read(pVertex,sizeof(BSPVERTEX));c2=*pVertex->Coord();pVertex++;
			pfile->Read(pVertex,sizeof(BSPVERTEX));c3=*pVertex->Coord();pVertex++;

			CrossProduct(&nor,c2-c1,c1-c3);
			Normalize(nor);
			pInfo->plane.a=nor.x;
			pInfo->plane.b=nor.y;
			pInfo->plane.c=nor.z;
			pInfo->plane.d=-DotProduct(nor,c1);

			if((pInfo->dwFlags & RM_FLAG_HIDE)!=0)
				pInfo->nMaterial=-1;
			else
			{
				pInfo->nMaterial=mat+1;		
				pInfo->dwFlags|=m_pMaterials[pInfo->nMaterial].dwFlags;
			}
			_ASSERT(pInfo->nMaterial<m_nMaterial);
			pInfo->nPolygonID=g_nCreatingPosition;
			pInfo->nLightmapTexture=0;
			
			pInfo++;
			g_nCreatingPosition++;
		}	
	}

	return true;
}

bool RBspObject::CreateVertexBuffer()
{


	g_nCreatingPosition=0;

	HRESULT hr=RGetDevice()->CreateVertexBuffer( sizeof(BSPVERTEX)*m_nBspPolygon*3, 0 , BSP_FVF, D3DPOOL_MANAGED, &m_pVertexBuffer );
	_ASSERT(hr==D3D_OK);

	m_pVertexBuffer->Lock(0,0,(BYTE**)&g_pLPVertices,0);

	CreateVertexBuffer(m_pBspRoot);

	m_pVertexBuffer->Unlock();

	return true;
}

bool RBspObject::CreateVertexBuffer(RSBspNode *pNode)
{
	if(pNode->nPolygon)
	{
		memcpy(g_pLPVertices+g_nCreatingPosition,pNode->pVertices,sizeof(BSPVERTEX)*pNode->nPolygon*3);
		pNode->nPosition=g_nCreatingPosition;
		g_nCreatingPosition+=pNode->nPolygon*3;
	}
	else
	{
	if(pNode->Positive)
		CreateVertexBuffer(pNode->Positive);
	if(pNode->Negative)
		CreateVertexBuffer(pNode->Negative);
	}
	return true;
}




bool			g_bPickFound;
rvector			g_PickOrigin;
rvector			g_PickDir;
RBSPPICKINFO	*g_pPickOut;

rvector			*g_pColPos;

rplane			g_PickPlane;
float			g_fPickDist;
DWORD			g_dwPassFlag;




bool RBspObject::Pick(rvector &pos,rvector &dir,RBSPPICKINFO *pOut,DWORD dwPassFlag)
{
	if(!m_pBspRoot) return false;

	g_PickOrigin=pos;
	g_PickDir=dir;
	Normalize(g_PickDir);

	g_bPickFound=false;
	g_pPickOut=pOut;
	D3DXPlaneFromPointNormal(&g_PickPlane,&g_PickOrigin,&g_PickDir);
	g_fPickDist=FLT_MAX;

	g_dwPassFlag=dwPassFlag;

	Pick(m_pBspRoot);

	if(g_bPickFound)
		return true;

	return false;

	

	
}

bool RBspObject::PickOcTree(rvector &pos,rvector &dir,RBSPPICKINFO *pOut,DWORD dwPassFlag)
{
	if(!m_pOcRoot) return false;

	g_PickOrigin=pos;
	g_PickDir=dir;
	Normalize(g_PickDir);

	g_bPickFound=false;
	g_pPickOut=pOut;
	D3DXPlaneFromPointNormal(&g_PickPlane,&g_PickOrigin,&g_PickDir);
	g_fPickDist=FLT_MAX;

	g_dwPassFlag=dwPassFlag;

	Pick(m_pOcRoot);

	if(g_bPickFound)
		return true;

	return false;
}

bool RBspObject::Pick(RSBspNode *pNode)
{
	if(!pNode) return false;
	float fDistToBB=GetDistance(pNode->bbTree,g_PickOrigin);
	if (fDistToBB>g_fPickDist) return false;

	if(pNode->nPolygon){
		for(int i=0;i<pNode->nPolygon;i++)
		{
			g_nPickCheckPolygon++;

			if( (pNode->pInfo[i].dwFlags & g_dwPassFlag) != 0 ) continue;

			float fDist;
			float fDistToPlane=D3DXPlaneDotCoord(&pNode->pInfo[i].plane,&g_PickOrigin);
			if((((pNode->pInfo[i].dwFlags & RM_FLAG_TWOSIDED) == 0 ) &&  fDistToPlane<0) || (fDistToPlane>g_fPickDist)) continue;

			BSPVERTEX *pVer=pNode->pVertices+i*3;

			g_nRealPickCheckPolygon++;
			if(IsIntersect(g_PickOrigin,g_PickDir,*pVer->Coord(),*(pVer+2)->Coord(),*(pVer+1)->Coord(),&fDist))
			{
				if(fDist<g_fPickDist)
				{
					g_bPickFound=true;
					g_fPickDist=fDist;
					g_pPickOut->PickPos=g_PickOrigin+fDist*g_PickDir;
					g_pPickOut->pNode=pNode;
					g_pPickOut->nIndex=i;
					g_pPickOut->pInfo=&pNode->pInfo[i];
				}
			}
		}
	}
	else
	if(isLineIntersectBoundingBox(g_PickOrigin,g_PickDir,pNode->bbTree))
	{
		if(D3DXPlaneDotCoord(&pNode->plane,&g_PickOrigin)>0)
			if(Pick(pNode->Positive)) return true;
			else
				return Pick(pNode->Negative);
		else
			if(Pick(pNode->Negative)) return true;
			else
				return Pick(pNode->Positive);
	}

	return false;
}

bool RBspObject::PickShadow(rvector &pos,rvector &dir,RBSPPICKINFO *pOut)
{
	if(!m_pBspRoot) return false;

	g_PickOrigin=pos;
	g_PickDir=dir;
	Normalize(g_PickDir);

	g_bPickFound=false;
	g_pPickOut=pOut;
	D3DXPlaneFromPointNormal(&g_PickPlane,&g_PickOrigin,&g_PickDir);
	g_fPickDist=FLT_MAX;

	PickShadow(m_pBspRoot);

	if(g_bPickFound)
		return true;

	return false;
}



bool RBspObject::PickShadow(RSBspNode *pNode)
{
	if(!pNode) return false;

	float fDistToBB=GetDistance(pNode->bbTree,g_PickOrigin);
	if (fDistToBB>g_fPickDist) return false;
	if(pNode->nPolygon){
		for(int i=0;i<pNode->nPolygon;i++)
		{
			float fDist;
			BSPVERTEX *pVer=pNode->pVertices+i*3;
			if( (pNode->pInfo[i].dwFlags & (RM_FLAG_ADDITIVE | RM_FLAG_USEOPACITY | RM_FLAG_HIDE)) == 0 &&
				(pNode->pInfo[i].dwFlags & RM_FLAG_CASTSHADOW) != 0 &&
				(D3DXPlaneDotCoord(&pNode->pInfo[i].plane,&g_PickOrigin)>=0) &&
				IsIntersect(g_PickOrigin,g_PickDir,*pVer->Coord(),*(pVer+2)->Coord(),*(pVer+1)->Coord(),&fDist))
			{
				rvector pickto=g_PickOrigin+g_PickDir;

				rvector pos;
				D3DXPlaneIntersectLine(&pos,&pNode->pInfo[i].plane,&g_PickOrigin,&pickto);

				if(D3DXPlaneDotCoord(&g_PickPlane,&pos)>=0)
				{
					float fDist=Magnitude(pos-g_PickOrigin);
					if(fDist<g_fPickDist)
					{
						g_bPickFound=true;
						g_fPickDist=fDist;
						g_pPickOut->PickPos=pos;
						g_pPickOut->pNode=pNode;
						g_pPickOut->nIndex=i;
						g_pPickOut->pInfo=&pNode->pInfo[i];
					}
				}
			}


		}
	}
	else
		if(isLineIntersectBoundingBox(g_PickOrigin,g_PickDir,pNode->bbTree))
		{
			if(D3DXPlaneDotCoord(&pNode->plane,&g_PickOrigin)>0)
				if(PickShadow(pNode->Positive)) return true;
				else
					return PickShadow(pNode->Negative);
			else
				if(PickShadow(pNode->Negative)) return true;
				else
					return PickShadow(pNode->Positive);
		}

		return false;
}



void RBspObject::GetNormal(RCONVEXPOLYGONINFO *poly,rvector &position,rvector *normal,int au,int av)
{

	int nSelPolygon=-1;
	float fMinDist=FLT_MAX;

	if(poly->nVertices==3)
		nSelPolygon=0;
	else
	{
		
		rvector pnormal(poly->plane.a,poly->plane.b,poly->plane.c);

		for(int i=0;i<poly->nVertices-2;i++)
		{
			float t;
			
			rvector *a=&poly->pVertices[0];
			rvector *b=&poly->pVertices[i+1];
			rvector *c=&poly->pVertices[i+2];


			if(IsIntersect(position+pnormal,-pnormal,*a,*b,*c,&t))
			{
				nSelPolygon=i;
				break;
			}else
			{
				float dist=GetDistance(position,*a,*b);
				if(dist<fMinDist) {fMinDist=dist;nSelPolygon=i;}
				dist=GetDistance(position,*b,*c);
				if(dist<fMinDist) {fMinDist=dist;nSelPolygon=i;}
				dist=GetDistance(position,*c,*a);
				if(dist<fMinDist) {fMinDist=dist;nSelPolygon=i;}
			}
		}
	}

	rvector *v0=&poly->pVertices[0];
	rvector *v1=&poly->pVertices[nSelPolygon+1];
	rvector *v2=&poly->pVertices[nSelPolygon+2];

	rvector *n0=&poly->pNormals[0];
	rvector *n1=&poly->pNormals[nSelPolygon+1];
	rvector *n2=&poly->pNormals[nSelPolygon+2];
	

	rvector a,b,x,tem;

	a=*v1-*v0;
	b=*v2-*v1;
	x=position-*v0;

	float f=b[au]*x[av]-b[av]*x[au];

	if(IS_ZERO(f))
	{
		*normal=*n0;
		return;
	}
	float t=(a[av]*x[au]-a[au]*x[av])/f;

	tem=InterpolatedVector(*n1,*n2,t);


	rvector inter=a+t*b;

	int axisfors;
	if(fabs(inter.x)>fabs(inter.y) && fabs(inter.x)>fabs(inter.z)) axisfors=0;
	else
		if(fabs(inter.y)>fabs(inter.z)) axisfors=1;
		else axisfors=2;

	float s=x[axisfors]/inter[axisfors];





	*normal=InterpolatedVector(*n0,tem,s);
}


bool RBspObject::GenerateLightmap(const char *filename,int nMaxlightmapsize,int nMinLightmapSize,int nSuperSample,float fToler,RGENERATELIGHTMAPCALLBACK pProgressFn)
{
	bool bReturnValue=true;

	ClearLightmaps();

	int i,j,k,l;

	float fMaximumArea=0;

	
	for(i=0;i<m_nConvexPolygon;i++)
	{
		fMaximumArea=max(fMaximumArea,m_pConvexPolygons[i].fArea);
	}

	int nConstCount=0;
	int nLight;
	RLIGHT **pplight;
	pplight=new RLIGHT*[m_StaticMapLightList.size()];
	rvector *lightmap=new rvector[nMaxlightmapsize*nMaxlightmapsize];
	DWORD	*lightmapdata=new DWORD[nMaxlightmapsize*nMaxlightmapsize];
	bool *isshadow=new bool[(nMaxlightmapsize+1)*(nMaxlightmapsize+1)];
	int	*pSourceLightmap=new int[m_nConvexPolygon];
	map<DWORD,int> ConstmapTable;

	vector<RLIGHTMAPTEXTURE*> sourcelightmaplist;

	RHEADER header(R_LM_ID,R_LM_VERSION);

	for(i=0;i<m_nConvexPolygon;i++)
	{
		RCONVEXPOLYGONINFO *poly=m_pConvexPolygons+i;

		
		if(pProgressFn)
		{
			bool bContinue=pProgressFn((float)i/(float)m_nConvexPolygon);
			if(!bContinue)
			{
				bReturnValue=false;
				goto clearandexit;
			}
		}

		rboundingbox bbox;	

		bbox.vmin=bbox.vmax=poly->pVertices[0];
		for(j=1;j<poly->nVertices;j++)
		{
			for(k=0;k<3;k++)
			{
				bbox.vmin[k]=min(bbox.vmin[k],poly->pVertices[j][k]);
				bbox.vmax[k]=max(bbox.vmax[k],poly->pVertices[j][k]);
			}
		}

		int lightmapsize; 

		
		{
			
			lightmapsize=nMaxlightmapsize;

			float targetarea=fMaximumArea/4.f;
			while(poly->fArea<targetarea && lightmapsize>nMinLightmapSize)
			{
				targetarea/=4.f;
				lightmapsize/=2;
			}
			
			rvector diff=float(lightmapsize)/float(lightmapsize-1)*(bbox.vmax-bbox.vmin);

			
			for(k=0;k<3;k++)
			{
				bbox.vmin[k]-=.5f/float(lightmapsize)*diff[k];
				bbox.vmax[k]+=.5f/float(lightmapsize)*diff[k];
			}

			rvector pnormal=rvector(poly->plane.a,poly->plane.b,poly->plane.c);
	
			RBSPMATERIAL *pMaterial = &m_pMaterials[m_pConvexPolygons[i].nMaterial];

			rvector ambient=pMaterial->Ambient;

			
			nLight=0;

			list<RLIGHT*>::iterator light=m_StaticMapLightList.begin();
			while(light!=m_StaticMapLightList.end()){
				
				if(	DotProduct((*light)->Position-*poly->pVertices,pnormal)>0 &&			
					GetDistance((*light)->Position,poly->plane)<(*light)->fAttnEnd)			
				{
					pplight[nLight++]=*light;
				}
				light++;
			}
			

			int au,av,ax; 

			if(fabs(poly->plane.a)>fabs(poly->plane.b) && fabs(poly->plane.a)>fabs(poly->plane.c) )
				ax=0;   
			else if(fabs(poly->plane.b)>fabs(poly->plane.c))
				ax=1;	
			else
				ax=2;	

			au=(ax+1)%3;
			av=(ax+2)%3;

			for(j=0;j<lightmapsize;j++)			
			{
				for(k=0;k<lightmapsize;k++)		
				{

					lightmap[j*lightmapsize+k]=m_AmbientLight;
						
				}
			}

			for(l=0;l<nLight;l++)
			{
				RLIGHT *plight=pplight[l];

				
				for(j=0;j<lightmapsize+1;j++)			
				{
					for(k=0;k<lightmapsize+1;k++)		
					{
						isshadow[k*(lightmapsize+1)+j]=false;
						if((plight->dwFlags & RM_FLAG_CASTSHADOW)==0 ||
							(poly->dwFlags & RM_FLAG_RECEIVESHADOW)==0) continue;
						_ASSERT(plight->dwFlags ==16);

						rvector position;
						position[au]=bbox.vmin[au]+((float)k/(float)lightmapsize)*diff[au];
						position[av]=bbox.vmin[av]+((float)j/(float)lightmapsize)*diff[av];
						
						position[ax]=(-poly->plane.d-pnormal[au]*position[au]-pnormal[av]*position[av])/pnormal[ax];

						float fDistanceToPolygon=Magnitude(position-plight->Position);

						RBSPPICKINFO bpi;
						if(PickShadow(plight->Position,position-plight->Position,&bpi)) 
						{
							float fDistanceToPickPos=Magnitude(bpi.PickPos-plight->Position);

							if(
								fDistanceToPolygon>fDistanceToPickPos+fToler)
								isshadow[k*(lightmapsize+1)+j]=true;

						}

						{
							for(RMapObjectList::iterator i=m_ObjectList.begin();i!=m_ObjectList.end();i++)
							{
								ROBJECTINFO *poi=*i;
								float t;

								
								rmatrix inv;	
								float det;
								D3DXMatrixInverse(&inv,&det,&poi->pVisualMesh->m_WorldMat);
								rvector origin=plight->Position*inv;
								rvector target=position*inv;

								rvector dir=target-origin;
								rvector dirorigin=position-plight->Position;

								rvector vOut;

								BOOL bBBTest=D3DXBoxBoundProbe(&poi->pVisualMesh->m_vBMin,&poi->pVisualMesh->m_vBMax,&origin,&dir);
								if( 
									bBBTest &&
									
									poi->pVisualMesh->Pick(plight->Position,dirorigin,&vOut,&t))
								{
									rvector PickPos=plight->Position+vOut*t;
									
									
									
										isshadow[k*(lightmapsize+1)+j]=true;
								}
							}
						}
					}
				}


				for(j=0;j<lightmapsize;j++)			
				{
					for(k=0;k<lightmapsize;k++)		
					{
						rvector color=rvector(0,0,0);

						
						
						
						

						int nShadowCount=0;

						for(int m=0;m<4;m++)
						{
							if(isshadow[(k+m%2)*(lightmapsize+1)+j+m/2])
								nShadowCount++;
						}


						if(nShadowCount<4)
						{
							if(nShadowCount>0)		
							{
								int m,n;
								rvector tempcolor=rvector(0,0,0);

								
								
								for(m=0;m<nSuperSample;m++)
								{
									for(n=0;n<nSuperSample;n++)
									{
										rvector position;
										position[au]=bbox.vmin[au]+(((float)k+((float)n+.5f)/(float)nSuperSample)/(float)lightmapsize)*diff[au];
										position[av]=bbox.vmin[av]+(((float)j+((float)m+.5f)/(float)nSuperSample)/(float)lightmapsize)*diff[av];
										
										position[ax]=(-poly->plane.d-pnormal[au]*position[au]-pnormal[av]*position[av])/pnormal[ax];

										RBSPPICKINFO bpi;
										if(PickShadow(plight->Position,position-plight->Position,&bpi) &&
											Magnitude(bpi.PickPos-position)<fToler) 
										{
											rvector dpos=plight->Position-position;
											float fdistance=Magnitude(dpos);
											float fIntensity=(fdistance-plight->fAttnStart)/(plight->fAttnEnd-plight->fAttnStart);
											fIntensity=min(max(1.0f-fIntensity,0),1);
											Normalize(dpos);

											rvector normal;
											GetNormal(poly,position,&normal,au,av);

											float fDot;
											fDot=DotProduct(dpos,normal);
											fDot=max(0,fDot);

											tempcolor+=fIntensity*plight->fIntensity*fDot*plight->Color;
										}
									}
								}
								tempcolor*=1.f/(nSuperSample*nSuperSample);
								
								color+=tempcolor;
							}
							else					
							{
								rvector position;
								position[au]=bbox.vmin[au]+(((float)k+.5f)/(float)lightmapsize)*diff[au];
								position[av]=bbox.vmin[av]+(((float)j+.5f)/(float)lightmapsize)*diff[av];
								
								position[ax]=(-poly->plane.d-pnormal[au]*position[au]-pnormal[av]*position[av])/pnormal[ax];

								rvector dpos=plight->Position-position;
								float fdistance=Magnitude(dpos);
								float fIntensity=(fdistance-plight->fAttnStart)/(plight->fAttnEnd-plight->fAttnStart);
								fIntensity=min(max(1.0f-fIntensity,0),1);
								Normalize(dpos);
								
								rvector normal;
								GetNormal(poly,position,&normal,au,av);
								
								float fDot;
								fDot=DotProduct(dpos,normal);
								fDot=max(0,fDot);

								color+=fIntensity*plight->fIntensity*fDot*plight->Color;
							}
						}

						lightmap[j*lightmapsize+k]+=color;
					}
				}
			}

			

			for(j=0;j<lightmapsize*lightmapsize;j++)
			{
				rvector color=lightmap[j];

				color*=0.25f;
				color.x=min(color.x,1);
				color.y=min(color.y,1);
				color.z=min(color.z,1);
				lightmap[j]=color;
				lightmapdata[j]=((DWORD)(color.x*255))<<16 | ((DWORD)(color.y*255))<<8 | ((DWORD)(color.z*255));
			}

			
			
			
			
			
			

			for(j=1;j<lightmapsize-1;j++)
			{
				for(k=1;k<lightmapsize-1;k++)
				{
					rvector color=lightmap[j*lightmapsize+k]*4;
					color+=(lightmap[(j-1)*lightmapsize+k]+lightmap[j*lightmapsize+k-1]+
							lightmap[(j+1)*lightmapsize+k]+lightmap[j*lightmapsize+k+1])*2;
					color+=	lightmap[(j-1)*lightmapsize+(k-1)]+lightmap[(j-1)*lightmapsize+k+1]+
							lightmap[(j+1)*lightmapsize+(k-1)]+lightmap[(j+1)*lightmapsize+k+1];
					color/=16.f;

					lightmapdata[j*lightmapsize+k]=((DWORD)(color.x*255))<<16 | ((DWORD)(color.y*255))<<8 | ((DWORD)(color.z*255));
				}
			}
		}

		bool bConstmap=true;
		for(j=0;j<lightmapsize*lightmapsize;j++)
		{
			if(lightmapdata[j]!=lightmapdata[0])
			{
				bConstmap=false;
				nConstCount++;
				break;
			}
		}

		bool bNeedInsert=true;
		if(bConstmap)
		{
			
			lightmapsize=2;

			map<DWORD,int>::iterator it=ConstmapTable.find(lightmapdata[0]);
			if(it!= ConstmapTable.end())
			{
				pSourceLightmap[i]=(*it).second;
				bNeedInsert=false;
			}
		}

		if(bNeedInsert)
		{
			int nLightmap=sourcelightmaplist.size();

			pSourceLightmap[i]=nLightmap;
			if(bConstmap)
				ConstmapTable.insert(map<DWORD,int>::value_type(lightmapdata[0],nLightmap));

#ifdef GENERATE_TEMP_FILES   
			
			if(i<100)	
			{
				char lightmapfilename[256];
				sprintf(lightmapfilename,"%s%d.bmp",m_filename.c_str(),nLightmap);
				RSaveAsBmp(lightmapsize,lightmapsize,lightmapdata,lightmapfilename);
			}
#endif			

			RLIGHTMAPTEXTURE *pnew=new RLIGHTMAPTEXTURE;
			pnew->bLoaded=false;
			pnew->nSize=lightmapsize;
			pnew->data=new DWORD[lightmapsize*lightmapsize];
			memcpy(pnew->data,lightmapdata,lightmapsize*lightmapsize*sizeof(DWORD));
			sourcelightmaplist.push_back(pnew);
		}
	}

	
	CalcLightmapUV(m_pBspRoot,pSourceLightmap,&sourcelightmaplist);
	CalcLightmapUV(m_pOcRoot,pSourceLightmap,&sourcelightmaplist);


	

	FILE *file=fopen(filename,"wb+");

	fwrite(&header,sizeof(RHEADER),1,file);

	
	fwrite(&m_nConvexPolygon,sizeof(int),1,file);
	fwrite(&m_nNodeCount,sizeof(int),1,file);

	
	m_nLightmap=m_LightmapList.size();
	fwrite(&m_nLightmap,sizeof(int),1,file);
	for(size_t i=0;i<m_LightmapList.size();i++)
	{
		

		

		RBspLightmapManager *plm=m_LightmapList[i];		
		
		int nSize=plm->GetSize();
		fwrite(&nSize,sizeof(int),1,file);
		float fUnused=plm->CalcUnused();
		fwrite(&fUnused,sizeof(float),1,file);
		fwrite(plm->GetData(),plm->GetSize()*plm->GetSize(),sizeof(DWORD),file);
	}


	Sort_Nodes(m_pOcRoot);

	
	for(int i=0;i<m_nPolygon;i++)
		fwrite(&(m_pOcInfo+i)->nPolygonID,sizeof(int),1,file);

	
	for(int i=0;i<m_nBspPolygon;i++)
		fwrite(&(m_pBspInfo+i)->nLightmapTexture,sizeof(int),1,file);
	for(int i=0;i<m_nPolygon;i++)
		fwrite(&(m_pOcInfo+i)->nLightmapTexture,sizeof(int),1,file);

	
	for(int i=0;i<m_nBspPolygon*3;i++)
		fwrite(&(m_pBspVertices+i)->tu2,sizeof(float),2,file);
	for(int i=0;i<m_nPolygon*3;i++)
		fwrite(&(m_pOcVertices+i)->tu2,sizeof(float),2,file);

	fclose(file);

clearandexit:

	delete []pplight;
	delete []lightmap;
	delete []lightmapdata;
	delete []isshadow;

	delete pSourceLightmap;
	while(sourcelightmaplist.size())
	{
		delete (*sourcelightmaplist.begin())->data;
		delete *sourcelightmaplist.begin();
		sourcelightmaplist.erase(sourcelightmaplist.begin());
	}

	return bReturnValue;
}

void RBspObject::CalcLightmapUV(RSBspNode *pNode,int *pSourceLightmap,vector<RLIGHTMAPTEXTURE*> *pSourceLightmaps)
{
	if(pNode->nPolygon)
	{
		int i,j,k;
		for(i=0;i<pNode->nPolygon;i++)
		{
			int is=pNode->pInfo[i].nConvexPolygon;
			int nSI=pSourceLightmap[is];	

			RCONVEXPOLYGONINFO *poly=m_pConvexPolygons+is;

			rboundingbox bbox;	

			bbox.vmin=bbox.vmax=poly->pVertices[0];
			for(j=1;j<poly->nVertices;j++)
			{
				for(k=0;k<3;k++)
				{
					bbox.vmin[k]=min(bbox.vmin[k],poly->pVertices[j][k]);
					bbox.vmax[k]=max(bbox.vmax[k],poly->pVertices[j][k]);
				}
			}

			RLIGHTMAPTEXTURE* pDestLightmap=pSourceLightmaps->at(nSI);

			int lightmapsize=pDestLightmap->nSize;

			rvector diff=float(lightmapsize)/float(lightmapsize-1)*(bbox.vmax-bbox.vmin);

			
			for(k=0;k<3;k++)
			{
				bbox.vmin[k]-=.5f/float(lightmapsize)*diff[k];
				bbox.vmax[k]+=.5f/float(lightmapsize)*diff[k];
			}

			int au,av,ax; 

			if(fabs(poly->plane.a)>fabs(poly->plane.b) && fabs(poly->plane.a)>fabs(poly->plane.c) )
				ax=0;   
			else if(fabs(poly->plane.b)>fabs(poly->plane.c))
				ax=1;	
			else
				ax=2;	

			au=(ax+1)%3;
			av=(ax+2)%3;

			
			for(j=0;j<3;j++)
			{
				pNode->pVertices[i*3+j].tu2=((*pNode->pVertices[i*3+j].Coord())[au]-bbox.vmin[au])/diff[au];
				pNode->pVertices[i*3+j].tv2=((*pNode->pVertices[i*3+j].Coord())[av]-bbox.vmin[av])/diff[av];
			}

			RBspLightmapManager *pCurrentLightmap=m_LightmapList.size() ? m_LightmapList[m_LightmapList.size()-1] : NULL;

			if(!pDestLightmap->bLoaded)
			{
				POINT pt;

				while(!pCurrentLightmap || 
					!pCurrentLightmap->Add(pDestLightmap->data,pDestLightmap->nSize,&pt))
				{
					pCurrentLightmap=new RBspLightmapManager;
					m_LightmapList.push_back(pCurrentLightmap);
				}
				pDestLightmap->bLoaded=true;
				pDestLightmap->position=pt;
				pDestLightmap->nLightmapIndex=m_LightmapList.size()-1;
			}

			pNode->pInfo[i].nLightmapTexture=pDestLightmap->nLightmapIndex;

			
			float fScaleFactor=(float)pDestLightmap->nSize/(float)pCurrentLightmap->GetSize();
			for(j=0;j<3;j++)
			{
				pNode->pVertices[i*3+j].tu2 = 
					pNode->pVertices[i*3+j].tu2*fScaleFactor+(float)pDestLightmap->position.x/(float)pCurrentLightmap->GetSize();
				pNode->pVertices[i*3+j].tv2 = 
					pNode->pVertices[i*3+j].tv2*fScaleFactor+(float)pDestLightmap->position.y/(float)pCurrentLightmap->GetSize();
			}
		}
	}

	if(pNode->Positive) CalcLightmapUV(pNode->Positive,pSourceLightmap,pSourceLightmaps);
	if(pNode->Negative) CalcLightmapUV(pNode->Negative,pSourceLightmap,pSourceLightmaps);
}

void RBspObject::GetUV(rvector &Pos,RSBspNode *pNode,int nIndex,float *uv)
{
	rplane plane=m_pConvexPolygons[pNode->pInfo[nIndex].nConvexPolygon].plane;

	int au,av,ax; 

	if(fabs(plane.a)>fabs(plane.b) && fabs(plane.a)>fabs(plane.c) )
		ax=0;   
	else if(fabs(plane.b)>fabs(plane.c))
		ax=1;	
	else
		ax=2;	

	au=(ax+1)%3;
	av=(ax+2)%3;

	rvector *v0=pNode->pVertices[nIndex*3+0].Coord();
	rvector *v1=pNode->pVertices[nIndex*3+1].Coord();
	rvector *v2=pNode->pVertices[nIndex*3+2].Coord();

	float *uv0=&pNode->pVertices[nIndex*3+0].tu2;
	float *uv1=&pNode->pVertices[nIndex*3+1].tu2;
	float *uv2=&pNode->pVertices[nIndex*3+2].tu2;

	rvector a,b,x;
	float tem[2];

	a=*v1-*v0;
	b=*v2-*v1;
	x=Pos-*v0;

	float f=b[au]*x[av]-b[av]*x[au];
	
	if(IS_ZERO(f))
	{
		uv[0]=uv0[0];
		uv[1]=uv0[1];
		return;
	}
	float t=(a[av]*x[au]-a[au]*x[av])/f;

	tem[0]=(1-t)*uv1[0]+t*uv2[0];
	tem[1]=(1-t)*uv1[1]+t*uv2[1];

	rvector inter=a+t*b;

	int axisfors;
	if(fabs(inter.x)>fabs(inter.y) && fabs(inter.x)>fabs(inter.z)) axisfors=0;
	else
		if(fabs(inter.y)>fabs(inter.z)) axisfors=1;
		else axisfors=2;

	float s=x[axisfors]/inter[axisfors];

	uv[0]=(1-s)*uv0[0]+s*tem[0];
	uv[1]=(1-s)*uv0[1]+s*tem[1];


}

DWORD RBspObject::GetLightmap(rvector &Pos,RSBspNode *pNode,int nIndex)
{

	if(!m_LightmapList.size())
		return 0xffffffff;

	float uv[2];
	GetUV(Pos,pNode,nIndex,uv);
	
	RBspLightmapManager *pCurrentLightmap=m_LightmapList[pNode->pInfo[nIndex].nLightmapTexture];

	int nSize=pCurrentLightmap->GetSize();

	DWORD *pdata=pCurrentLightmap->GetData();

	uv[0]*=(float)nSize;
	uv[1]*=(float)nSize;

	

	
	int iuv[2];
	iuv[0]=int(uv[0]);
	iuv[1]=int(uv[1]);

	float s=uv[0]-(float)iuv[0];
	float t=uv[1]-(float)iuv[1];

	_ASSERT(s>=0 && s<1.f);

	rvector color0,color1,tempcolor,tempcolor2;
	color0=DWORD2VECTOR(pdata[iuv[1]*nSize+iuv[0]]);
	color1=DWORD2VECTOR(pdata[(iuv[1]+1)*nSize+iuv[0]]);
	tempcolor=(1-t)*color0+t*color1;

	color0=DWORD2VECTOR(pdata[iuv[1]*nSize+iuv[0]+1]);
	color1=DWORD2VECTOR(pdata[(iuv[1]+1)*nSize+iuv[0]+1]);
	tempcolor2=(1-t)*color0+t*color1;

	rvector color=(1-s)*tempcolor+s*tempcolor2;

	return VECTOR2RGB24(color);
}

rvector RBspObject::GetDimension()
{
	if(!m_pOcRoot)
		return rvector(0,0,0);

	return m_pOcRoot->bbTree.vmax-m_pOcRoot->bbTree.vmin;
}



rvector checkwallorigin;
rvector checkwalldir;

float	impactdist;
rvector impact;
rplane	impactplane;
rplane	lastplane;

int impactcount;
RColBspNode *lastcolnode=NULL;

class RImpactPlanes : public multimap<float,rplane> {
public:
	void Add(float fDist,rplane &plane) { 
		for(iterator i=begin();i!=end();i++)
		{
			rplane p=i->second;
			if((plane.a*p.a+plane.b*p.b+plane.c*p.c)>0.99f && fabs(p.d-plane.d)<0.01f)
				return;
		}
		insert(RImpactPlanes::value_type(fDist,plane));
	}

} impactplanes;



int g_nDepth=0;
int g_path[256];
int g_nPathDepth;
int g_impactpath[256];
bool g_blatimpact;


bool clipline(rplane &plane,rvector &v0,rvector &v1)
{
	float dotv0=D3DXPlaneDotCoord(&plane,&v0);
	float dotv1=D3DXPlaneDotCoord(&plane,&v1);

	if(dotv0<0 && dotv1<0) return false;

	rvector intersect;
	rvector *p=D3DXPlaneIntersectLine(&intersect,&plane,&v0,&v1);
	if(!p) return true;

	if(dotv0<0) v0=intersect;
	if(dotv1<0) v1=intersect;
	return true;


}



bool GetIntersectionOfTwoPlanes(rvector *pOutDir,rvector *pOutAPoint,rplane &plane1,rplane &plane2)
{
	rvector n1=rvector(plane1.a,plane1.b,plane1.c);
	rvector n2=rvector(plane2.a,plane2.b,plane2.c);
	
	rvector dir;
	CrossProduct(&dir,n1,n2);

	if(IS_ZERO(DotProduct(dir,dir))) return false;	

	float determinant=DotProduct(n1,n1)*DotProduct(n2,n2)-DotProduct(n1,n2)*DotProduct(n1,n2);
	float c1=(plane1.d*DotProduct(n2,n2)-plane2.d*DotProduct(n1,n2))/determinant;
	float c2=(plane2.d*DotProduct(n1,n1)-plane1.d*DotProduct(n1,n2))/determinant;

	*pOutAPoint=c1*n1+c2*n2;
	*pOutDir=dir;

	return true;
}

void RBspObject::DrawSolidNode()
{
	if(!lastcolnode) return;
#ifndef _PUBLISH
	RGetDevice()->SetVertexShader( D3DFVF_XYZ );

	LPDIRECT3DDEVICE8 pd3dDevice=RGetDevice();
	pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW );

	pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

	pd3dDevice->SetRenderState(D3DRS_TEXTUREFACTOR , 0x40808080);
	pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
	pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
	pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
	pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1 );
	pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
	pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );

	lastcolnode->DrawSolidPolygon();

	RSetWBuffer(true);
	pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, true);
	pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

	pd3dDevice->SetRenderState(D3DRS_TEXTUREFACTOR , 0x40ffffff);
	lastcolnode->DrawSolidPolygonWireframe();

	pd3dDevice->SetRenderState(D3DRS_TEXTUREFACTOR , 0x40ff00ff);
	lastcolnode->DrawSolidPolygonNormal();

	pd3dDevice->SetRenderState(D3DRS_TEXTUREFACTOR , 0x40ff0000);
	{
		

		RColBspNode *pNode=m_pColRoot;
		for(int i=0;i<g_nPathDepth;i++)
		{
			rplane plane=pNode->plane;
			if(g_impactpath[i]==0)
				pNode=pNode->Negative;
			else
				pNode=pNode->Positive;

			
			bool bExist=false;
			rvector pos;
			for(int j=0;j<lastcolnode->nPolygon*3;j++)
			{
				if(fabs(D3DXPlaneDotCoord(&plane,&lastcolnode->pVertices[j]))<0.01f) {
					if(!bExist) {
						pos=lastcolnode->pVertices[j];
						bExist=true;
					}
					else
					{
						if(!IS_EQ3(pos,lastcolnode->pVertices[j])) {
							bExist=false;
							break;
						}
					}
				}
			}

			if(bExist)
			{
				rvector v[2];
				v[0]=pos;
				v[1]=pos+rvector(plane.a,plane.b,plane.c)*50.f;
				RGetDevice()->DrawPrimitiveUP(D3DPT_LINESTRIP,1,&v,sizeof(rvector));
			}
		}
	}
	pd3dDevice->SetRenderState(D3DRS_TEXTUREFACTOR , 0x40ffffff);

#endif
	
}

#define TOLERENCE 0.001f
#define SIGN(x) ( (x)<-TOLERENCE ? -1 : (x)>TOLERENCE ? 1 : 0 )



bool RBspObject::CheckWall(rvector &origin,rvector &targetpos,float fRadius,float fHeight,RCHECKWALLFLAG method,int nDepth,rplane *pimpactplane)
{
	if(nDepth==0)
	{
		

		g_blatimpact=false;

		checkwalldir=targetpos-origin;
		Normalize(checkwalldir);
	}

	if(nDepth>=2) {
		targetpos=origin;
		return false;
	}

	
	

	checkwallorigin=origin;

	impactdist=FLT_MAX;
	impact=rvector(0,0,0);

	bool bIntersectThis;

	impactcount=0;
	impactplanes.erase(impactplanes.begin(),impactplanes.end());

	if(method==RCW_SPHERE)
		bIntersectThis=CheckWall_Sphere(m_pColRoot,origin,targetpos,fRadius);
	else
		bIntersectThis=CheckWall_Cylinder(m_pColRoot,origin,targetpos,fRadius,fHeight);

	RImpactPlanes::iterator i;


#ifdef TRACE_PATH	
	if(impactplanes.size() || nDepth>0)
	{
		mlog("\n");
		for(int i=0;i<nDepth;i++) mlog("    ");

		rvector dif=targetpos-origin;
		mlog("d%d from ( %3.5f %3.5f %3.5f ) by ( %3.3f %3.3f %3.3f ) ",nDepth
			,origin.x,origin.y,origin.z,dif.x,dif.y,dif.z);
	}
#endif

	
	for(i=impactplanes.begin();i!=impactplanes.end();)
	{
		rvector godir=targetpos-origin;
		rplane plane=i->second;
		if(D3DXPlaneDotNormal(&plane,&godir)>TOLERENCE)
		{
#ifdef TRACE_PATH
			rplane p=i->second;
			mlog(" del_back{d = %3.3f [%3.3f %3.3f %3.3f %3.3f]}",i->first,p.a,p.b,p.c,p.d);
#endif
			i=impactplanes.erase(i)	;
		}else
			i++;
	}


#ifdef TRACE_PATH	
	if(impactplanes.size() || nDepth>0)
	{
		mlog(" :::: %d planes ",impactplanes.size());
		for(RImpactPlanes::iterator i=impactplanes.begin();i!=impactplanes.end();i++)
		{
			rplane p=i->second;
			mlog(" d = %3.3f [%3.3f %3.3f %3.3f %3.3f] ",i->first,p.a,p.b,p.c,p.d);
		}
	}
#endif

	if(impactplanes.size())
	{

		bool bFound=false;
		rplane simulplanes[100];	
		float fDist=0;				

		

		for(i=impactplanes.begin();i!=impactplanes.end();i++) 
		{
			rplane plane=i->second;

			float fDistOrigin=D3DXPlaneDotCoord(&plane,&origin);
			float fDistTarget=D3DXPlaneDotCoord(&plane,&targetpos);

			if(fabs(fDistOrigin-fDistTarget)<TOLERENCE) 
				continue;

			bFound=true;
			simulplanes[0]=plane;
			fDist=i->first;
			break;
		}

		if(!bFound) {
			return false;
		}

		

		rvector currentorigin;

		rvector *p=D3DXPlaneIntersectLine(&currentorigin,&simulplanes[0],&origin,&targetpos);
		_ASSERT(p!=NULL);

		if(nDepth==0 && pimpactplane)
			*pimpactplane=simulplanes[0];

		

		int nSimulCount=0;

		for(i=impactplanes.begin();i!=impactplanes.end();i++) 
		{
			if(fabs(fDist-i->first)<0.01f) { 
				simulplanes[nSimulCount++]=i->second;
			}
		}

#ifdef TRACE_PATH
		mlog("%d simul ",nSimulCount);
#endif

		
		

		bool b1Case=false;
		{
			float fBestCase=0.f;

			for(int i=0;i<nSimulCount;i++)
			{
				rplane plane=simulplanes[i];

				

				
				rvector Ni=rvector(plane.a,plane.b,plane.c);

				
				rvector newtargetpos =  targetpos  + Ni * -D3DXPlaneDotCoord(&plane,&targetpos);

				bool bOk=true;
				for(int j=0;j<nSimulCount;j++)
				{
					if(D3DXPlaneDotCoord(&simulplanes[j],&newtargetpos)<-TOLERENCE){ 
						bOk=false;
						break;
					}
				}

				if(bOk)
				{
					b1Case=true;
#ifdef TRACE_PATH
					mlog("1 case ( %3.3f %3.3f %3.3f %3.3f plane ) ",plane.a,plane.b,plane.c,plane.d);
#endif
					float fDot=DotProduct(checkwalldir,newtargetpos-origin);
					if(fDot>fBestCase)
					{
						fBestCase=fDot;
						targetpos=newtargetpos;
					}

					

				}
			}
		}

		if(!b1Case)
		{
			
			

			bool b2Case=false;

			rvector dif=targetpos-currentorigin;
			for(int i=0;i<nSimulCount;i++)
			{
				for(int j=i+1;j<nSimulCount;j++)
				{
					rvector dir;
					CrossProduct(&dir,rvector(simulplanes[i].a,simulplanes[i].b,simulplanes[i].c),
						rvector(simulplanes[j].a,simulplanes[j].b,simulplanes[j].c));
					if(Magnitude(dir)<TOLERENCE) continue;
					
					Normalize(dir);

					rvector prjdif=dir*DotProduct(dir,dif);

					
					rvector newtargetpos=currentorigin+prjdif;

					bool bOk=true;
					for(int k=0;k<nSimulCount;k++)
					{
						if(D3DXPlaneDotCoord(&simulplanes[k],&newtargetpos)<-TOLERENCE){ 
							bOk=false;
							break;
						}
					}

					if(bOk)
					{
						b2Case=true;
#ifdef TRACE_PATH
						mlog("2 case ");
#endif
						targetpos=newtargetpos;
						break;
					}

					if(b2Case) break;
				}
				if(b2Case) break;
			}
		}

		rvector newdir=targetpos-currentorigin;
		if(DotProduct(newdir,checkwalldir)<-0.01) {
#ifdef TRACE_PATH
			mlog(" -> over dir.");
#endif
			targetpos=currentorigin;
			return false;
		}

		CheckWall(currentorigin,targetpos,fRadius,fHeight,method,nDepth+1);
		return true;
	}

	

	return false;
}

bool RBspObject::CheckSolid(rvector &pos,float fRadius,float fHeight,RCHECKWALLFLAG method)
{
	checkwallorigin=pos;

	impactdist=FLT_MAX;
	impact=rvector(0,0,0);

	if(method==RCW_SPHERE)
		return CheckWall_Sphere(m_pColRoot,pos,pos,fRadius);
	else
		return CheckWall_Cylinder(m_pColRoot,pos,pos,fRadius,fHeight);

}




bool checkplane(int side,rplane &plane,rvector &v0,rvector &v1,rvector *w0,rvector *w1)
{
	float dotv0=D3DXPlaneDotCoord(&plane,&v0);
	float dotv1=D3DXPlaneDotCoord(&plane,&v1);

	int signv0=SIGN(dotv0),signv1=SIGN(dotv1);

	if(signv0==side) {
		*w0=v0;

		if(signv1==side)
			*w1=v1;
		else
		{
			rvector intersect;
			if(D3DXPlaneIntersectLine(&intersect,&plane,&v0,&v1))
				*w1=intersect;
			else
				*w1=v1;
		}
		return true;
	}

	if(signv1==side) {
		*w1=v1;

		if(signv0==side)
			*w0=v0;
		else
		{
			rvector intersect;
			if(D3DXPlaneIntersectLine(&intersect,&plane,&v0,&v1))
				*w0=intersect;
			else
				*w0=v0;
		}
		return true;
	}

	return false;
}


bool rayunder(rplane &plane,rvector &v0,rvector &v1,rvector *w0,rvector *w1)
{
	return checkplane(-1,plane,v0,v1,w0,w1);
}

bool rayover(rplane &plane,rvector &v0,rvector &v1,rvector *w0,rvector *w1)
{
	return checkplane(1,plane,v0,v1,w0,w1);
}

bool RBspObject::CheckWall_Cylinder(RColBspNode *pNode,rvector v0,rvector v1,float fRadius,float fHeight,rplane v0plane)
{
	bool bHit=false;

	if(!pNode) return false;

	if(!pNode->Positive && !pNode->Negative) {	
		if(pNode->bSolid) {
			float fDist=Magnitude(checkwallorigin-v0);
			if(fDist<impactdist)
			{
				impactdist=fDist;
				impact=v0;
				impactplane=v0plane;

				g_blatimpact=true;
				g_nPathDepth=g_nDepth;	
				memcpy(g_impactpath,g_path,sizeof(g_path));

				lastcolnode=pNode;
			}
			impactplanes.Add(fDist,v0plane);

			
			if(v0plane.a==0 && v0plane.b==0 && v0plane.c==0 && v0plane.d==0 ) { 

			}

			return true;
		}
		return false;
	}

	rvector w0,w1;
	rvector dif=v1-v0;
	if(D3DXPlaneDotNormal(&pNode->plane,&dif) > 0 )
	{
		rvector rimpoint=rvector(-pNode->plane.a,-pNode->plane.b,0);
		if(IS_ZERO(rimpoint.x) && IS_ZERO(rimpoint.y))
			rimpoint.x=1.f;
		Normalize(rimpoint);
		rimpoint= rimpoint*fRadius;
		rimpoint.z +=  (pNode->plane.c < 0 ) ? fHeight : -fHeight;

		rplane shiftplane=pNode->plane;

		float fShift=D3DXPlaneDotNormal(&pNode->plane,&rimpoint);
		shiftplane.d=pNode->plane.d+fShift;

		if(rayunder(shiftplane,v0,v1,&w0,&w1))
		{
			bool bcurrentplane= (w0.x==v0.x && w0.y==v0.y && w0.z==v0.z) && fabs(D3DXPlaneDotCoord(&shiftplane,&w0))>0.01f;

			g_path[g_nDepth++]=0;
			bHit=CheckWall_Cylinder(pNode->Negative,w0,w1,fRadius,fHeight,bcurrentplane ? v0plane : shiftplane );
			g_nDepth--;
		}

		shiftplane.d=pNode->plane.d-fShift;

		if(rayover(shiftplane,v0,v1,&w0,&w1))
		{
			bool bcurrentplane= (w0.x==v0.x && w0.y==v0.y && w0.z==v0.z) && fabs(D3DXPlaneDotCoord(&shiftplane,&w0))>0.01f;

			g_path[g_nDepth++]=1;
			bHit |= CheckWall_Cylinder(pNode->Positive,w0,w1,fRadius,fHeight,bcurrentplane ? v0plane : -shiftplane );
			g_nDepth--;
		}

		return bHit;
	}else
	{
		rvector rimpoint=rvector(-pNode->plane.a,-pNode->plane.b,0);
		if(IS_ZERO(rimpoint.x) && IS_ZERO(rimpoint.y))
			rimpoint.x=1.f;
		Normalize(rimpoint);
		rimpoint= rimpoint*fRadius;
		rimpoint.z +=  (pNode->plane.c < 0 ) ? fHeight : -fHeight;

		rplane shiftplane=pNode->plane;

		float fShift=D3DXPlaneDotNormal(&pNode->plane,&rimpoint);
		shiftplane.d=pNode->plane.d-fShift;

		if(rayover(shiftplane,v0,v1,&w0,&w1))
		{
			bool bcurrentplane= (w0.x==v0.x && w0.y==v0.y && w0.z==v0.z) && fabs(D3DXPlaneDotCoord(&shiftplane,&w0))>0.01f;

			g_path[g_nDepth++]=1;
			bHit=CheckWall_Cylinder(pNode->Positive,w0,w1,fRadius,fHeight,bcurrentplane ? v0plane : -shiftplane );
			g_nDepth--;
		}

		shiftplane.d=pNode->plane.d+fShift;

		if(rayunder(shiftplane,v0,v1,&w0,&w1))
		{
			bool bcurrentplane= (w0.x==v0.x && w0.y==v0.y && w0.z==v0.z) && fabs(D3DXPlaneDotCoord(&shiftplane,&w0))>0.01f;

			g_path[g_nDepth++]=0;
			bHit |= CheckWall_Cylinder(pNode->Negative,w0,w1,fRadius,fHeight,bcurrentplane ? v0plane : shiftplane );
			g_nDepth--;
		}

		return bHit;
	}
}

bool RBspObject::CheckWall_Sphere(RColBspNode *pNode,rvector v0,rvector v1,float fRadius,rplane v0plane)
{
	bool bHit=false;

	if(!pNode) return false;
	
	if(!pNode->Positive && !pNode->Negative) {	
		if(pNode->bSolid) {
			float fDist=Magnitude(checkwallorigin-v0);
			if(fDist<impactdist)
			{
				impactdist=fDist;
				impact=v0;
				impactplane=v0plane;

				g_blatimpact=true;
				g_nPathDepth=g_nDepth;	
				memcpy(g_impactpath,g_path,sizeof(g_path));

				lastcolnode=pNode;
			}
			impactplanes.Add(fDist,v0plane);

			
			if(v0plane.a==0 && v0plane.b==0 && v0plane.c==0 && v0plane.d==0 ) { 

			}

			return true;
		}
		return false;
	}

	rvector w0,w1;
	rvector dif=v1-v0;
	if(D3DXPlaneDotNormal(&pNode->plane,&dif) > 0 )
	{
		
		rplane shiftplane=pNode->plane;
		shiftplane.d=pNode->plane.d-fRadius; 

		if(rayunder(shiftplane,v0,v1,&w0,&w1))
		{
			bool bcurrentplane= (w0.x==v0.x && w0.y==v0.y && w0.z==v0.z) && fabs(D3DXPlaneDotCoord(&shiftplane,&w0))>0.01f;

			g_path[g_nDepth++]=0;
			bHit=CheckWall_Sphere(pNode->Negative,w0,w1,fRadius, bcurrentplane ? v0plane : shiftplane );
			g_nDepth--;
			
			
		}

		shiftplane.d=pNode->plane.d+fRadius; 
		if(rayover(shiftplane,v0,v1,&w0,&w1))
		{
			bool bcurrentplane= (w0.x==v0.x && w0.y==v0.y && w0.z==v0.z) && fabs(D3DXPlaneDotCoord(&shiftplane,&w0))>0.01f;
			
			g_path[g_nDepth++]=1;
			bHit |= CheckWall_Sphere(pNode->Positive,w0,w1,fRadius,bcurrentplane ? v0plane : -shiftplane );
			g_nDepth--;
		}

		return bHit;
	}else
	{
		
		rplane shiftplane=pNode->plane;
		shiftplane.d=pNode->plane.d+fRadius;

		if(rayover(shiftplane,v0,v1,&w0,&w1))
		{
			bool bcurrentplane= (w0.x==v0.x && w0.y==v0.y && w0.z==v0.z) && fabs(D3DXPlaneDotCoord(&shiftplane,&w0))>0.01f;

			g_path[g_nDepth++]=1;
			bHit=CheckWall_Sphere(pNode->Positive,w0,w1,fRadius,bcurrentplane ? v0plane : -shiftplane );
			g_nDepth--;
			
			
			
		}

		shiftplane.d=pNode->plane.d-fRadius;
		if(rayunder(shiftplane,v0,v1,&w0,&w1))
		{
			bool bcurrentplane= (w0.x==v0.x && w0.y==v0.y && w0.z==v0.z) && fabs(D3DXPlaneDotCoord(&shiftplane,&w0))>0.01f;

			g_path[g_nDepth++]=0;
			bHit |= CheckWall_Sphere(pNode->Negative,w0,w1,fRadius,bcurrentplane ? v0plane : shiftplane );
			g_nDepth--;
		}

		return bHit;
	}
}

rvector RBspObject::GetFloor(rvector &origin,float fRadius,float fHeight,rplane *pimpactplane)
{
	g_blatimpact=false;

	checkwalldir=rvector(0,0,-1);

	impact=rvector(0,0,0);
	impactdist=FLT_MAX;

	rvector targetpos=origin+rvector(0,0,-10000);
	bool bIntersect=CheckWall_Cylinder(m_pColRoot,origin,targetpos,fRadius,fHeight);

	if(!bIntersect)
		return targetpos;

	rvector floor=impact;
	floor.z-=fHeight;
	if(pimpactplane)
		*pimpactplane=impactplane;

	return floor;
}









RBSPMATERIAL *RBspObject::GetMaterial(int nIndex)
{
	_ASSERT(nIndex<m_nMaterial);
	return &m_pMaterials[nIndex];
}

bool RBspObject::IsVisible(rboundingbox &bb)		
{
	for(int i=0;i<m_nOcclusion;i++)
	{
		bool bVisible=false;

		ROcclusion *poc=&m_pOcclusion[i];

		for(int j=0;j<poc->nCount+1;j++)
		{
			if(isInPlane(&bb,&poc->pPlanes[j]))
			{
				bVisible=true;
				break;
			}
		}

		
		if(!bVisible) 
			return false;
	}
	return true;
}

bool RBspObject::GetShadowPosition( rvector& pos_, rvector& dir_, rvector* outNormal_, rvector* outPos_ )
{
	RBSPPICKINFO pick_info;
	if(!Pick( pos_, dir_, &pick_info ))
		return false;
	*outPos_ = pick_info.PickPos;
	*outNormal_ = rvector(pick_info.pInfo[pick_info.nIndex].plane.a,
		pick_info.pInfo[pick_info.nIndex].plane.b, pick_info.pInfo[pick_info.nIndex].plane.c);
	
	return true;
}


bool RBspObject::PickCol(RColBspNode *pNode,rvector v0,rvector v1) 
{
	bool bHit=false;

	if(!pNode) return false;

	if(!pNode->Positive && !pNode->Negative) {	
		if(pNode->bSolid) {
			g_bPickFound=true;
			g_fPickDist=Magnitude(g_PickOrigin-v0);
			g_pPickOut->PickPos=v0;
			g_pPickOut->pNode=NULL;
			g_pPickOut->nIndex=-1;
			g_pPickOut->pInfo=NULL;
			return true;
		}
		return false;
	}

	rvector w0,w1;
	rvector dif=v1-v0;
	if(D3DXPlaneDotNormal(&pNode->plane,&dif) > 0 )
	{
		if(rayunder(pNode->plane,v0,v1,&w0,&w1))
		{
			bHit=PickCol(pNode->Negative,w0,w1);
			if(bHit) v1=impact;
		}

		if(rayover(pNode->plane,v0,v1,&w0,&w1))
		{
			bHit |= PickCol(pNode->Positive,w0,w1);
		}

		return bHit;
	}else
	{
		if(rayover(pNode->plane,v0,v1,&w0,&w1))
		{
			bHit=PickCol(pNode->Positive,w0,w1);
			if(bHit) v1=impact;		

		}

		if(rayunder(pNode->plane,v0,v1,&w0,&w1))
		{
			bHit |= PickCol(pNode->Negative,w0,w1);
		}

		return bHit;
	}
}


bool RBspObject::CheckWall_Corn(rvector *pOut,rvector &center,rvector &pole,float fRadius)
{
	float fMinLength=0.f;

	rvector dir=pole-center;
	float fLength=Magnitude(dir);
	Normalize(dir);

	impactcount=0;
	impactplanes.erase(impactplanes.begin(),impactplanes.end());

	bool bIntersectThis=CheckWall_Sphere(m_pColRoot,center,pole,fRadius);

	for(RImpactPlanes::iterator i=impactplanes.begin();i!=impactplanes.end();i++)
	{
		rplane plane=i->second;

		if(D3DXPlaneDotNormal(&plane,&dir)<0.01){
			rplane shiftplane=plane;
			shiftplane.d+=fRadius;
			
			
			rvector impactpos;
			D3DXPlaneIntersectLine(&impactpos,&shiftplane,&center,&pole);
			impactpos-=fRadius*rvector(plane.a,plane.b,plane.c);

			
			rplane ortplane;
			D3DXPlaneFromPointNormal(&ortplane,&impactpos,&dir);
			
			
			rvector ortcenter;
			D3DXPlaneIntersectLine(&ortcenter,&ortplane,&center,&pole);

			
			float fortlength=max(DotProduct(pole-ortcenter,dir),0);
			float fortradius=fRadius*fortlength/fLength;

			float fDistToCorn=fortradius-Magnitude(impactpos-ortcenter);

			
			if(fDistToCorn<fMinLength)
			{
				fMinLength=fDistToCorn;
			}
		}
	}

	*pOut=pole+dir*fMinLength;

	return true;
}

_NAMESPACE_REALSPACE2_END























