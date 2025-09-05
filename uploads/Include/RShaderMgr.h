#ifndef _RSHADERMGR_H
#define _RSHADERMGR_H

#include "windows.h"
#include "vector"
#include "map"
#include "d3dx9.h"

using namespace std;

typedef vector<LPDIRECT3DVERTEXSHADER9>				stdVecShader;
typedef vector<LPDIRECT3DVERTEXDECLARATION9>		stdVecShaderDecl;
typedef map<string, LPDIRECT3DVERTEXSHADER9>			stdMapShader;
typedef map<string, LPDIRECT3DVERTEXDECLARATION9>	stdMapShaderDecl;

#define MAX_LIGHT 2
#define UPDATE_LIGHT0 0x1
#define UPDATE_LIGHT1 0x2
#define UPDATE_MATERIAL 0x4

enum ESHADER
{
	SHADER_SKIN,
	SHADER_SKIN_SPEC,
};

class RMtrl;

class RShaderMgr
{
public:
	RShaderMgr();
	~RShaderMgr();

private:
	stdVecShader m_ShaderVec;

	stdVecShaderDecl m_ShaderDeclVec;

	bool	mbNeedUpdate[3];
	bool	mbLight[2];

public:
	static RShaderMgr msInstance;

	static bool mbUsingShader;

private:
	void init();

public:
	bool addVertexShaderFromFile(char* fileName_);
	bool addPixelShader(char* fileName_);
	bool addVertexShader(const char* shader_);
	bool addVertexShaderFromResource(char* Resource_);

	LPDIRECT3DVERTEXSHADER9 getShader(int i_);
	LPDIRECT3DVERTEXDECLARATION9 getShaderDecl(int i);

	void setMtrl(D3DXCOLOR& rColor_, float fVisAlpha_);
	void setMtrl(RMtrl* pMtrl_, float fVisAlpha_);
	void setLight(int iLignt_, D3DLIGHT9* pLight_);
	void setAmbient(DWORD value_);
	void LightEnable(int iLignt_, bool bEnable_);

	void Update();

	bool Initialize();
	void Release();

	void SetDisable();
	bool SetEnable();

	static RMtrl* mpMtrl;
	static D3DLIGHT9 mLight[MAX_LIGHT];

public:
	static RShaderMgr* getShaderMgr();
};

#ifndef GETSHADER
#define GETSHADER
RShaderMgr* RGetShaderMgr();
#endif

#endif