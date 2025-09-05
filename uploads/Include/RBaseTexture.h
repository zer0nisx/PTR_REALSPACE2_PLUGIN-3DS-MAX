#ifndef _RBASETEXTURE_H
#define _RBASETEXTURE_H

#include <D3DX9.h>

#include <string>
#include <map>
#include <algorithm>

#include "RNameSpace.h"

class MZFileSystem;

using namespace std;

_NAMESPACE_REALSPACE2_BEGIN

class RBaseTexture
{
public:
	RBaseTexture();
	virtual ~RBaseTexture();

	void Destroy();

	void OnInvalidate();
	bool OnRestore(bool bManaged = false);

	int GetWidth() { return m_Info.Width; }
	int GetHeight() { return m_Info.Height; }
	int GetDepth() { return m_Info.Depth; }
	int GetMipLevels() { return m_Info.MipLevels; }

	int GetTexLevel() { return m_nTexLevel; }
	void SetTexLevel(int level) { m_nTexLevel = level; }

	int GetTexType() { return m_nTexType; }
	void SetTexType(int type) { m_nTexType = type; }

	D3DFORMAT GetFormat() { return m_Info.Format; }

	LPDIRECT3DTEXTURE9	GetTexture();

private:

	bool SubCreateTexture();

public:
	bool	m_bManaged;
	DWORD	m_dwLastUseTime;
	char* m_pTextureFileBuffer;
	int		m_nFileSize;
	char	m_szTextureName[256];
	int		m_nRefCount;
	bool	m_bUseMipmap;
	bool	m_bUseFileSystem;

	int		m_nTexLevel;
	DWORD	m_nTexType;

	DWORD	m_dwColorkey;

	D3DXIMAGE_INFO m_Info;
	LPDIRECT3DTEXTURE9 m_pTex;
};

class RTextureManager : public map<string, RBaseTexture*>
{
public:
	virtual ~RTextureManager();

	void Destroy();

	RBaseTexture* CreateBaseTextureSub(bool mg, const char* filename, int texlevel, bool bUseMipmap = false, bool bUseFileSystem = true, DWORD colorkey = 0);
	RBaseTexture* CreateBaseTexture(const char* filename, int texlevel, bool bUseMipmap = false, bool bUseFileSystem = true, DWORD colorkey = 0);
	RBaseTexture* CreateBaseTextureMg(const char* filename, int texlevel, bool bUseMipmap = false, bool bUseFileSystem = true, DWORD colorkey = 0);

	void DestroyBaseTexture(RBaseTexture*);
	void DestroyBaseTexture(char* szName);

	void OnInvalidate();
	void OnRestore();
	void OnChangeTextureLevel(DWORD flag);

	int UpdateTexture(DWORD max_life_time = 5000);
	int CalcUsedSize();
	int CalcAllUsedSize();
	int PrintUsedTexture();
	int CalcUsedCount();
};

void RBaseTexture_Create();
void RBaseTexture_Destory();

void RBaseTexture_Invalidate();
void RBaseTexture_Restore();

RTextureManager* RGetTextureManager();

#define RTextureType_Etc		0
#define RTextureType_Map		1<<1
#define RTextureType_Object		1<<2
#define RTextureType_All		1<<3

void SetObjectTextureLevel(int nLevel);
void SetMapTextureLevel(int nLevel);
void SetTextureFormat(int nLevel);

int GetObjectTextureLevel();
int GetMapTextureLevel();
int GetTextureFormat();

RBaseTexture* RCreateBaseTexture(const char* filename, DWORD nTexType = RTextureType_Etc, bool bUseMipmap = false, bool bUseFileSystem = true, DWORD colorkey = 0);
RBaseTexture* RCreateBaseTextureMg(const char* filename, DWORD nTexType = RTextureType_Etc, bool bUseMipmap = false, bool bUseFileSystem = true, DWORD colorkey = 0);

void			RDestroyBaseTexture(RBaseTexture*);

void			RChangeBaseTextureLevel(DWORD flag);

_NAMESPACE_REALSPACE2_END

#endif
