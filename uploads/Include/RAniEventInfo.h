#pragma once

#include "MXml.h"

#include "MDebug.h"

#include "MZFileSystem.h"
#include "fileinfo.h"

#include <list>
using namespace std;

#define SAFE_DELETE(p) { if(p) { delete (p); (p)=NULL; } }

class RAniEventInfo
{
private:
	char m_cFileName[256];
	int m_nBeginFrame;
	char m_cEventType[256];

public:
	RAniEventInfo() {}
	rvector m_vPos;

	char* GetFileName() { return m_cFileName; }
	int GetBeginFrame() { return m_nBeginFrame; }
	char* GetEventType() { return m_cEventType; }

	void SetFileName(char* filename) { strcpy(m_cFileName, filename); }
	void SetBeginFrame(int BeginFrame) { m_nBeginFrame = BeginFrame; }
	void SetEventType(char* EventType) { strcpy(m_cEventType, EventType); }
};

typedef list<RAniEventInfo*> AniNameEventSet;
typedef list<RAniEventInfo*>::iterator AniNameEventSetIter;

typedef struct _RAniNameEventSet
{
	char					m_cAnimationName[256];
	AniNameEventSet			m_AniNameEventSet;
	AniNameEventSetIter		m_AniNameEventSetIter;

	char* GetAnimationName() { return m_cAnimationName; }
	void SetAnimationName(char* AnimationName) { strcpy(m_cAnimationName, AnimationName); }
	void Destroy()
	{
		while (!(m_AniNameEventSet.empty()))
		{
			SAFE_DELETE(m_AniNameEventSet.back());
			m_AniNameEventSet.pop_back();
		}
	}
}RAniNameEventSet;

typedef list<RAniNameEventSet*> AniIDEventSet;
typedef list<RAniNameEventSet*>::iterator AniIDEventSetIter;

typedef struct _RAniIDEventSet
{
	int						m_nID;
	AniIDEventSet			m_AniIDEventSet;
	AniIDEventSetIter		m_AniIDEventSetIter;

	int GetID() { return m_nID; }
	void SetID(int id) { m_nID = id; }

	RAniNameEventSet* GetAniNameEventSet(char* AnimationName);

	void Destroy()
	{
		while (!(m_AniIDEventSet.empty()))
		{
			m_AniIDEventSet.back()->Destroy();
			SAFE_DELETE(m_AniIDEventSet.back());
			m_AniIDEventSet.pop_back();
		}
	}
}RAniIDEventSet;

typedef list<RAniIDEventSet*> AniEventMgr;
typedef list<RAniIDEventSet*>::iterator AniEventMgrIter;

class RAniEventMgr
{
public:
	AniEventMgr				m_AniEventMgr;
	AniEventMgrIter			m_AniEventMgrIter;

	bool ReadXml(MZFileSystem* pFileSystem, const char* szFileName);
	void ParseAniEvent(MXmlElement* PNode, RAniIDEventSet* pAnimIdEventSet);
	RAniIDEventSet* GetAniIDEventSet(int id);

	void Destroy()
	{
		while (!(m_AniEventMgr.empty()))
		{
			m_AniEventMgr.back()->Destroy();
			SAFE_DELETE(m_AniEventMgr.back());
			m_AniEventMgr.pop_back();
		}
	}
	RAniEventMgr() {}
	~RAniEventMgr() { Destroy(); }
};
