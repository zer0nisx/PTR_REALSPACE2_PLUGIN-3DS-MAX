#if !defined(AFX_RMTRL_H__8A7FB4A5_9CA2_409D_99FE_AD7B159F607A__INCLUDED_)
#define AFX_RMTRL_H__8A7FB4A5_9CA2_409D_99FE_AD7B159F607A__INCLUDED_

#include <list>
#include <vector>
#include <string>
#include <map>

#include <D3DX9.h>

#include "RBaseTexture.h"

#define SAFE_REL(p)      { if(p) { (p)->Release(); (p)=NULL; } }

_USING_NAMESPACE_REALSPACE2

class RMtrl
{
public:
	RMtrl();
	~RMtrl();

	void CheckAniTexture();

	LPDIRECT3DTEXTURE9 GetTexture();

	void Restore(LPDIRECT3DDEVICE9 dev, char* path = NULL);

	void	SetTColor(DWORD color);
	DWORD	GetTColor();

public:

	RBaseTexture* m_pTexture;

	RBaseTexture* m_pToonTexture;

	D3DTEXTUREFILTERTYPE m_FilterType;
	D3DTEXTUREFILTERTYPE m_ToonFilterType;

	DWORD				m_AlphaRefValue;
	D3DTEXTUREOP		m_TextureBlendMode;
	D3DTEXTUREOP		m_ToonTextureBlendMode;

	D3DXCOLOR	m_ambient;
	D3DXCOLOR	m_diffuse;
	D3DXCOLOR	m_specular;

	float	m_power;

	char	m_mtrl_name[255];

	char	m_name[255];
	char	m_opa_name[255];

	char	m_name_ani_tex[255];
	char	m_name_ani_tex_ext[20];

	int		m_id;
	int		m_u_id;
	int		m_mtrl_id;
	int		m_sub_mtrl_id;

	int		m_sub_mtrl_num;

	bool	m_bDiffuseMap;
	bool	m_bTwoSided;
	bool	m_bAlphaMap;
	bool	m_bAlphaTestMap;
	bool	m_bAdditive;

	int		m_nAlphaTestValue;

	bool	m_bUse;

	bool	m_bAniTex;
	int 	m_nAniTexCnt;
	int 	m_nAniTexSpeed;
	int 	m_nAniTexGap;
	DWORD	m_backup_time;

	bool	m_bObjectMtrl;

	DWORD	m_dwTFactorColor;

	RBaseTexture** m_pAniTexture;
};

using namespace std;

#define MAX_MTRL_NODE 100

class RMtrlMgr :public list<RMtrl*>
{
public:
	RMtrlMgr();
	virtual ~RMtrlMgr();

	int		Add(char* name, int u_id = -1);
	int		Add(RMtrl* tex);

	void	Del(int id);

	int		LoadList(char* name);
	int		SaveList(char* name);

	void	DelAll();
	void	Restore(LPDIRECT3DDEVICE9 dev, char* path = NULL);

	void	CheckUsed(bool b);
	void	ClearUsedCheck();
	void	ClearUsedMtrl();

	void	SetObjectTexture(bool bObject) { m_bObjectMtrl = bObject; }

	RMtrl* Get_s(int mtrl_id, int sub_id);

	LPDIRECT3DTEXTURE9 Get(int id);
	LPDIRECT3DTEXTURE9 Get(int id, int sub_id);
	LPDIRECT3DTEXTURE9 GetUser(int id);
	LPDIRECT3DTEXTURE9 Get(char* name);

	RMtrl* GetMtrl(char* name);
	RMtrl* GetToolMtrl(char* name);

	void	Del(RMtrl* tex);
	int		GetNum();

	vector<RMtrl*>	m_node_table;

	bool	m_bObjectMtrl;
	int		m_id_last;
};

// ===== EXPORTER_MESH_VER9 - PBR Material Class =====
class RMtrl_V9 : public RMtrl
{
public:
	RMtrl_V9();
	virtual ~RMtrl_V9();

	// Extended texture support
	RBaseTexture* m_pNormalTexture;
	RBaseTexture* m_pSpecularTexture;
	RBaseTexture* m_pRoughnessTexture;
	RBaseTexture* m_pMetallicTexture;
	RBaseTexture* m_pEmissiveTexture;
	RBaseTexture* m_pAOTexture;
	RBaseTexture* m_pHeightTexture;
	RBaseTexture* m_pReflectionTexture;
	RBaseTexture* m_pRefractionTexture;

	// PBR properties
	float		m_roughness;			// 0.0-1.0
	float		m_metallic;				// 0.0-1.0
	float		m_ior;					// Index of refraction
	float		m_emissive_intensity;	// Emissive strength multiplier
	D3DXCOLOR	m_emissive;				// Emissive color

	// Material type
	int			m_material_type;		// 0=Legacy, 1=PBR, 2=Unlit

	// UV channel assignments
	int			m_uv_diffuse;
	int			m_uv_normal;
	int			m_uv_specular;
	int			m_uv_roughness;
	int			m_uv_metallic;
	int			m_uv_emissive;
	int			m_uv_ao;
	int			m_uv_height;

	// UV transformations per channel
	D3DXVECTOR2	m_uv_tiling[8];
	D3DXVECTOR2	m_uv_offset[8];
	float		m_uv_rotation[8];

	// Texture flags
	bool		m_bHasNormalMap;
	bool		m_bHasSpecularMap;
	bool		m_bHasRoughnessMap;
	bool		m_bHasMetallicMap;
	bool		m_bHasEmissiveMap;
	bool		m_bHasAOMap;
	bool		m_bHasHeightMap;

	// Extended methods
	virtual void Restore(LPDIRECT3DDEVICE9 dev, char* path = NULL);
	virtual LPDIRECT3DTEXTURE9 GetTexture(int textureType = 0); // 0=diffuse, 1=normal, etc.

	void SetPBRProperties(float roughness, float metallic, float ior = 1.5f);
	void SetEmissive(const D3DXCOLOR& color, float intensity = 1.0f);
	void SetUVChannel(int textureType, int channel);
	void SetUVTransform(int channel, const D3DXVECTOR2& tiling, const D3DXVECTOR2& offset, float rotation = 0.0f);

	// Shader mode selection
	enum MaterialShaderMode {
		SHADER_LEGACY = 0,
		SHADER_PBR = 1,
		SHADER_UNLIT = 2,
		SHADER_CUSTOM = 3
	};

	MaterialShaderMode GetShaderMode() const;
	bool IsPBRMaterial() const { return m_material_type == 1; }
	bool IsUnlitMaterial() const { return m_material_type == 2; }
};

// Extended material manager for v9 materials
class RMtrlMgr_V9 : public RMtrlMgr
{
public:
	RMtrlMgr_V9();
	virtual ~RMtrlMgr_V9();

	// Extended methods for v9 materials
	int AddV9(RMtrl_V9* mtrl);
	RMtrl_V9* GetMtrlV9(char* name);
	RMtrl_V9* GetV9(int mtrl_id, int sub_id);

	// Load v9 format files
	int LoadListV9(char* name);
	int SaveListV9(char* name);

	void RestoreV9(LPDIRECT3DDEVICE9 dev, char* path = NULL);

	// Version detection
	bool IsV9Format(char* filename);

	vector<RMtrl_V9*> m_v9_materials;
};

#endif
