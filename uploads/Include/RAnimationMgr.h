#ifndef _RAnimationMgr_h
#define _RAnimationMgr_h

#include "RAnimation.h"

_NAMESPACE_REALSPACE2_BEGIN





#define MAX_ANIMATION_NODE 1000



typedef RHashList<RAnimationFile*>				RAnimationFileHashList;
typedef RHashList<RAnimationFile*>::iterator	RAnimationFileHashList_Iter;

typedef RHashList<RAnimation*>					RAnimationHashList;
typedef RHashList<RAnimation*>::iterator		RAnimationHashList_Iter;



class RAnimationFileMgr
{
public:
	RAnimationFileMgr();
	~RAnimationFileMgr();

	static RAnimationFileMgr* GetInstance();
	
	void Destroy();

	RAnimationFile* Add(char* filename);
	RAnimationFile* Get(char* filename);

	RAnimationFileHashList m_list;
};

inline RAnimationFileMgr* RGetAnimationFileMgr() { return RAnimationFileMgr::GetInstance(); }


class RAnimationMgr {
public:
	RAnimationMgr();
	~RAnimationMgr(); 

	

	bool LoadAnimationFileList(char* filename) {
		return true;
	}

	inline RAnimation* Add(char* name,char* filename,int sID,int MotionTypeID = -1) {
		return AddAnimationFile(name,filename,sID,false,MotionTypeID);
	}

	inline RAnimation* AddGameLoad(char* name,char* filename,int sID,int MotionTypeID = -1) {
		return AddAnimationFile(name,filename,sID,true,MotionTypeID);
	}

	RAnimation* AddAnimationFile(char* name,char* filename,int sID,bool notload,int MotionTypeID = -1);

	void DelAll(); 

	RAnimation* GetAnimation(char* name,int MotionTypeID=-1);
	RAnimation* GetAnimation(int sID,int MotionTypeID=-1);
	RAnimation* GetAnimationListMap(char* name,int wtype);

	void ReloadAll();

	void MakeListMap(int size);

public:

	int	m_id_last;
	int	m_list_map_size;

	RAnimationHashList  m_list;
	RAnimationHashList* m_list_map;

	vector<RAnimation*> m_node_table;
};

_NAMESPACE_REALSPACE2_END

#endif