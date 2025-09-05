#include "stdafx.h"
#include "RMtrl.h"

#include <string.h>
#include <stdio.h>

#include "RMtrl.h"
#include "assert.h"
#include <tchar.h>

#include "RBaseTexture.h"

#include "MDebug.h"

_USING_NAMESPACE_REALSPACE2

RMtrl::RMtrl()
{
	m_name[0] = 0;
	m_opa_name[0] = 0;

	m_id = -1;
	m_u_id = -1;
	m_mtrl_id = -1;
	m_sub_mtrl_id = -1;

	m_bDiffuseMap = false;
	m_bAlphaMap = false;
	m_bTwoSided = false;
	m_bAdditive = false;
	m_bAlphaTestMap = false;

	m_nAlphaTestValue = -1;

	m_ambient = 0xffffffff;
	m_diffuse = 0xffffffff;
	m_specular = 0xffffffff;

	m_power = 1.0f;

	m_bUse = true;
	m_pTexture = NULL;
	m_pAniTexture = NULL;

	m_pToonTexture = NULL;
	m_FilterType = D3DTEXF_LINEAR;
	m_ToonFilterType = D3DTEXF_POINT;
	m_AlphaRefValue = 0x05;
	m_TextureBlendMode = D3DTOP_BLENDTEXTUREALPHA;

	m_ToonTextureBlendMode = D3DTOP_MODULATE2X;

	m_bAniTex = false;
	m_nAniTexCnt = 0;
	m_nAniTexSpeed = 0;
	m_nAniTexGap = 0;
	m_backup_time = 0;

	m_name_ani_tex[0] = 0;
	m_name_ani_tex_ext[0] = 0;
	m_bObjectMtrl = false;

	m_dwTFactorColor = D3DCOLOR_COLORVALUE(0.0f, 1.0f, 0.0f, 0.0f);
}

RMtrl::~RMtrl()
{
	if (m_bAniTex) {
		if (m_pAniTexture) {
			for (int i = 0; i < m_nAniTexCnt; i++) {
				RDestroyBaseTexture(m_pAniTexture[i]);
			}
			delete[] m_pAniTexture;
		}
	}
	else {
		RDestroyBaseTexture(m_pTexture);
		m_pTexture = NULL;
	}
}

LPDIRECT3DTEXTURE9 RMtrl::GetTexture() {
	if (m_bAniTex) {
		DWORD this_time = timeGetTime();

		DWORD gap = (this_time - m_backup_time);

		if (gap > (DWORD)m_nAniTexSpeed) {
			gap %= m_nAniTexSpeed;
			m_backup_time = this_time;
		}

		int pos = gap / m_nAniTexGap;

		if ((pos < 0) || (pos > m_nAniTexCnt - 1))
			pos = 0;

		if (m_pAniTexture[pos]) {
			return 	m_pAniTexture[pos]->GetTexture();
		}

		return NULL;
	}
	else {
		if (!m_pTexture) return NULL;
		return m_pTexture->GetTexture();
	}
}

void RMtrl::SetTColor(DWORD color)
{
	m_dwTFactorColor = color;
}

DWORD RMtrl::GetTColor()
{
	return m_dwTFactorColor;
}

void RMtrl::CheckAniTexture()
{
	if (m_name[0]) {
		char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
		_splitpath(m_name, drive, dir, fname, ext);

		if (strnicmp(fname, "txa", 3) == 0) {
			static char texname[256];

			int imax = 0;
			int ispeed = 0;

			sscanf(fname, "txa %d %d %s", &imax, &ispeed, texname);

			m_pAniTexture = new RBaseTexture * [imax];

			for (int i = 0; i < imax; i++) {
				m_pAniTexture[i] = NULL;
			}

			int n = (int)strlen(texname);

			if (dir[0]) {
				strcpy(m_name_ani_tex, dir);
				strncat(m_name_ani_tex, texname, n - 2);
				int pos = strlen(dir) + n - 2;
				m_name_ani_tex[pos] = NULL;
			}
			else {
				strncpy(m_name_ani_tex, texname, n - 2);
				m_name_ani_tex[n - 2] = NULL;
			}

			strcpy(m_name_ani_tex_ext, ext);

			m_nAniTexSpeed = ispeed;
			m_nAniTexCnt = imax;
			m_nAniTexGap = ispeed / imax;

			m_bAniTex = true;
		}
	}
}

void RMtrl::Restore(LPDIRECT3DDEVICE9 dev, char* path)
{
	if (m_name[0] == 0) return;

	static char name[256];
	static char name2[256];

	int level = RTextureType_Etc;

	if (m_bObjectMtrl) {
		level = RTextureType_Object;
	}

	bool map_path = false;

	char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
	_splitpath(m_name, drive, dir, fname, ext);

	if (dir[0])
		map_path = true;

	if (!map_path && path && path[0]) {
		if (m_bAniTex) {
			strcpy(name, path);
			strcat(name, m_name);

			m_pAniTexture[0] = RCreateBaseTextureMg(name, level);

			if (m_pAniTexture[0] == NULL) {
				m_pAniTexture[0] = RCreateBaseTextureMg(m_name, level);
			}

			strcpy(name2, path);
			strcat(name2, m_name_ani_tex);

			for (int i = 1; i < m_nAniTexCnt; i++) {
				sprintf(name, "%s%02d%s", name2, i, m_name_ani_tex_ext);
				m_pAniTexture[i] = RCreateBaseTextureMg(name, level);

				if (m_pAniTexture[i] == NULL) {
					sprintf(name, "%s%2d.%s", m_name_ani_tex, i, m_name_ani_tex_ext);
					m_pAniTexture[i] = RCreateBaseTextureMg(name, level);
					if (m_pAniTexture[i] == NULL)
						int a = 0;
				}
			}
		}
		else {
			strcpy(name, path);
			strcat(name, m_name);

			m_pTexture = NULL;
			m_pTexture = RCreateBaseTextureMg(name, level);

			if (!m_pTexture)
				m_pTexture = RCreateBaseTextureMg(m_name, level);
		}
	}
	else {
		if (m_bAniTex) {
			m_pAniTexture[0] = RCreateBaseTextureMg(m_name, level);

			for (int i = 1; i < m_nAniTexCnt; i++) {
				sprintf(name, "%s%02d%s", m_name_ani_tex, i, m_name_ani_tex_ext);
				m_pAniTexture[i] = RCreateBaseTextureMg(name, level);
			}
		}
		else {
			m_pTexture = RCreateBaseTextureMg(m_name, level);
		}
	}
}

RMtrlMgr::RMtrlMgr()
{
	m_id_last = 0;

	m_node_table.reserve(MAX_MTRL_NODE);

	for (int i = 0; i < MAX_MTRL_NODE; i++)
		m_node_table[i] = NULL;

	m_bObjectMtrl = false;
}

RMtrlMgr::~RMtrlMgr()
{
	DelAll();
}

int RMtrlMgr::Add(char* name, int u_id)
{
	RMtrl* node;
	node = new RMtrl;

	node->m_id = m_id_last;
	node->m_u_id = u_id;
	node->m_bObjectMtrl = m_bObjectMtrl;

	strcpy(node->m_name, name);

	sprintf(node->m_mtrl_name, "%s%d", name, m_id_last);

	m_node_table.push_back(node);

	push_back(node);
	m_id_last++;
	return m_id_last - 1;
}

int RMtrlMgr::Add(RMtrl* tex)
{
	tex->m_id = m_id_last;
	tex->m_bObjectMtrl = m_bObjectMtrl;

	sprintf(tex->m_mtrl_name, "%s%d", tex->m_name, tex->m_id);

	m_node_table.push_back(tex);

	push_back(tex);
	m_id_last++;
	return m_id_last - 1;
}

int	RMtrlMgr::LoadList(char* fname)
{
	return 1;
}

int	RMtrlMgr::SaveList(char* name)
{
	return 1;
}

void RMtrlMgr::Del(RMtrl* tex)
{
	iterator node;

	for (node = begin(); node != end();) {
		if ((*node) == tex) {
			RDestroyBaseTexture((*node)->m_pTexture);
			(*node)->m_pTexture = NULL;
			delete (*node);
			node = erase(node);
		}
		else ++node;
	}
}

void RMtrlMgr::Del(int id)
{
	iterator node;

	for (node = begin(); node != end();) {
		if ((*node)->m_id == id) {
			RDestroyBaseTexture((*node)->m_pTexture);
			(*node)->m_pTexture = NULL;
			delete (*node);
			node = erase(node);
		}
		else ++node;
	}
}

void RMtrlMgr::DelAll()
{
	if (size() == 0) return;

	iterator node;
	RMtrl* pMtrl;

	for (node = begin(); node != end(); ) {
		pMtrl = *node;
		delete pMtrl;
		pMtrl = NULL;

		node = erase(node);
	}

	m_node_table.clear();

	m_id_last = 0;
}

void RMtrlMgr::Restore(LPDIRECT3DDEVICE9 dev, char* path)
{
	if (size() == 0) return;

	iterator node;

	RMtrl* mtrl;

	for (node = begin(); node != end(); ++node) {
		mtrl = (*node);

		if (!mtrl) continue;

		mtrl->Restore(dev, path);
	}
}

void RMtrlMgr::ClearUsedCheck()
{
	iterator node;

	for (node = begin(); node != end(); ++node) {
		(*node)->m_bUse = false;
	}
}

void RMtrlMgr::ClearUsedMtrl()
{
	if (size() == 0) return;

	iterator node;
	RMtrl* pMtrl;

	for (node = begin(); node != end(); ) {
		pMtrl = (*node);

		if (pMtrl->m_bUse == false) {
			delete pMtrl;
			pMtrl = NULL;
			node = erase(node);
		}
		else {
			++node;
		}
	}

	int _size = size();
}

#include "MProfiler.h"

RMtrl* RMtrlMgr::Get_s(int mtrl_id, int sub_id)
{
	if (size() == 0)
		return NULL;

	iterator node;

	RMtrl* pMtrl = NULL;

	for (node = begin(); node != end(); ++node) {
		pMtrl = (*node);

		if (pMtrl) {
			if (pMtrl->m_mtrl_id == mtrl_id) {
				if (pMtrl->m_sub_mtrl_id == sub_id) {
					return pMtrl;
				}
			}
		}
	}

	return NULL;
}

LPDIRECT3DTEXTURE9 RMtrlMgr::Get(int id)
{
	iterator node;

	for (node = begin(); node != end(); ++node) {
		if ((*node)->m_id == id) {
			return (*node)->GetTexture();
		}
	}
	return NULL;
}

LPDIRECT3DTEXTURE9 RMtrlMgr::Get(int id, int sub_id)
{
	iterator node;

	for (node = begin(); node != end(); ++node) {
		if ((*node)->m_mtrl_id == id && (*node)->m_sub_mtrl_id == sub_id) {
			return (*node)->GetTexture();
		}
	}
	return NULL;
}

LPDIRECT3DTEXTURE9 RMtrlMgr::GetUser(int u_id)
{
	iterator node;

	for (node = begin(); node != end(); ++node) {
		if ((*node)->m_u_id == u_id) {
			return (*node)->GetTexture();
		}
	}
	return NULL;
}

LPDIRECT3DTEXTURE9 RMtrlMgr::Get(char* name)
{
	iterator node;

	for (node = begin(); node != end(); ++node) {
		if (!strcmp((*node)->m_name, name)) {
			return (*node)->GetTexture();
		}
	}
	return NULL;
}

RMtrl* RMtrlMgr::GetMtrl(char* name)
{
	iterator node;

	for (node = begin(); node != end(); ++node) {
		if (!strcmp((*node)->m_name, name)) {
			return (*node);
		}
	}
	return NULL;
}

RMtrl* RMtrlMgr::GetToolMtrl(char* name)
{
	iterator node;

	for (node = begin(); node != end(); ++node) {
		if (!strcmp((*node)->m_mtrl_name, name)) {
			return (*node);
		}
	}
	return NULL;
}

int	RMtrlMgr::GetNum()
{
	return size();
}

// ===== EXPORTER_MESH_VER9 - PBR Material Implementation =====

RMtrl_V9::RMtrl_V9() : RMtrl()
{
	// Initialize extended textures
	m_pNormalTexture = NULL;
	m_pSpecularTexture = NULL;
	m_pRoughnessTexture = NULL;
	m_pMetallicTexture = NULL;
	m_pEmissiveTexture = NULL;
	m_pAOTexture = NULL;
	m_pHeightTexture = NULL;
	m_pReflectionTexture = NULL;
	m_pRefractionTexture = NULL;

	// Initialize PBR properties
	m_roughness = 0.5f;
	m_metallic = 0.0f;
	m_ior = 1.5f;
	m_emissive_intensity = 1.0f;
	m_emissive = D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f);
	m_material_type = 1; // PBR by default

	// Initialize UV channels
	m_uv_diffuse = 0;
	m_uv_normal = 0;
	m_uv_specular = 0;
	m_uv_roughness = 0;
	m_uv_metallic = 0;
	m_uv_emissive = 0;
	m_uv_ao = 0;
	m_uv_height = 0;

	// Initialize UV transformations
	for (int i = 0; i < 8; i++) {
		m_uv_tiling[i] = D3DXVECTOR2(1.0f, 1.0f);
		m_uv_offset[i] = D3DXVECTOR2(0.0f, 0.0f);
		m_uv_rotation[i] = 0.0f;
	}

	// Initialize texture flags
	m_bHasNormalMap = false;
	m_bHasSpecularMap = false;
	m_bHasRoughnessMap = false;
	m_bHasMetallicMap = false;
	m_bHasEmissiveMap = false;
	m_bHasAOMap = false;
	m_bHasHeightMap = false;
}

RMtrl_V9::~RMtrl_V9()
{
	// Cleanup extended textures
	RDestroyBaseTexture(m_pNormalTexture);
	RDestroyBaseTexture(m_pSpecularTexture);
	RDestroyBaseTexture(m_pRoughnessTexture);
	RDestroyBaseTexture(m_pMetallicTexture);
	RDestroyBaseTexture(m_pEmissiveTexture);
	RDestroyBaseTexture(m_pAOTexture);
	RDestroyBaseTexture(m_pHeightTexture);
	RDestroyBaseTexture(m_pReflectionTexture);
	RDestroyBaseTexture(m_pRefractionTexture);
}

void RMtrl_V9::Restore(LPDIRECT3DDEVICE9 dev, char* path)
{
	// Call base class restore first
	RMtrl::Restore(dev, path);

	// Restore extended textures
	// TODO: Implement texture loading based on file paths stored in material data
}

LPDIRECT3DTEXTURE9 RMtrl_V9::GetTexture(int textureType)
{
	switch (textureType) {
	case 0: // Diffuse
		return RMtrl::GetTexture();
	case 1: // Normal
		return m_pNormalTexture ? m_pNormalTexture->GetTexture() : NULL;
	case 2: // Specular
		return m_pSpecularTexture ? m_pSpecularTexture->GetTexture() : NULL;
	case 3: // Roughness
		return m_pRoughnessTexture ? m_pRoughnessTexture->GetTexture() : NULL;
	case 4: // Metallic
		return m_pMetallicTexture ? m_pMetallicTexture->GetTexture() : NULL;
	case 5: // Emissive
		return m_pEmissiveTexture ? m_pEmissiveTexture->GetTexture() : NULL;
	case 6: // AO
		return m_pAOTexture ? m_pAOTexture->GetTexture() : NULL;
	case 7: // Height
		return m_pHeightTexture ? m_pHeightTexture->GetTexture() : NULL;
	case 8: // Reflection
		return m_pReflectionTexture ? m_pReflectionTexture->GetTexture() : NULL;
	case 9: // Refraction
		return m_pRefractionTexture ? m_pRefractionTexture->GetTexture() : NULL;
	default:
		return NULL;
	}
}

void RMtrl_V9::SetPBRProperties(float roughness, float metallic, float ior)
{
	m_roughness = max(0.0f, min(1.0f, roughness));
	m_metallic = max(0.0f, min(1.0f, metallic));
	m_ior = max(1.0f, ior);
	m_material_type = 1; // Set to PBR
}

void RMtrl_V9::SetEmissive(const D3DXCOLOR& color, float intensity)
{
	m_emissive = color;
	m_emissive_intensity = max(0.0f, intensity);
}

void RMtrl_V9::SetUVChannel(int textureType, int channel)
{
	if (channel < 0 || channel > 7) return;

	switch (textureType) {
	case 0: m_uv_diffuse = channel; break;
	case 1: m_uv_normal = channel; break;
	case 2: m_uv_specular = channel; break;
	case 3: m_uv_roughness = channel; break;
	case 4: m_uv_metallic = channel; break;
	case 5: m_uv_emissive = channel; break;
	case 6: m_uv_ao = channel; break;
	case 7: m_uv_height = channel; break;
	}
}

void RMtrl_V9::SetUVTransform(int channel, const D3DXVECTOR2& tiling, const D3DXVECTOR2& offset, float rotation)
{
	if (channel < 0 || channel > 7) return;

	m_uv_tiling[channel] = tiling;
	m_uv_offset[channel] = offset;
	m_uv_rotation[channel] = rotation;
}

RMtrl_V9::MaterialShaderMode RMtrl_V9::GetShaderMode() const
{
	return (MaterialShaderMode)m_material_type;
}

// ===== RMtrlMgr_V9 Implementation =====

RMtrlMgr_V9::RMtrlMgr_V9() : RMtrlMgr()
{
}

RMtrlMgr_V9::~RMtrlMgr_V9()
{
	// Cleanup v9 materials
	for (auto& mtrl : m_v9_materials) {
		delete mtrl;
	}
	m_v9_materials.clear();
}

int RMtrlMgr_V9::AddV9(RMtrl_V9* mtrl)
{
	if (!mtrl) return -1;

	m_v9_materials.push_back(mtrl);

	// Also add to base manager for compatibility
	return Add(mtrl);
}

RMtrl_V9* RMtrlMgr_V9::GetMtrlV9(char* name)
{
	for (auto& mtrl : m_v9_materials) {
		if (strcmp(mtrl->m_name, name) == 0) {
			return mtrl;
		}
	}
	return NULL;
}

RMtrl_V9* RMtrlMgr_V9::GetV9(int mtrl_id, int sub_id)
{
	for (auto& mtrl : m_v9_materials) {
		if (mtrl->m_mtrl_id == mtrl_id && mtrl->m_sub_mtrl_id == sub_id) {
			return mtrl;
		}
	}
	return NULL;
}

int RMtrlMgr_V9::LoadListV9(char* name)
{
	// TODO: Implement v9 format loading
	// Will read the new binary format with extended material data
	return 0;
}

int RMtrlMgr_V9::SaveListV9(char* name)
{
	// TODO: Implement v9 format saving
	return 0;
}

void RMtrlMgr_V9::RestoreV9(LPDIRECT3DDEVICE9 dev, char* path)
{
	// Restore base materials first
	Restore(dev, path);

	// Restore v9 specific materials
	for (auto& mtrl : m_v9_materials) {
		mtrl->Restore(dev, path);
	}
}

bool RMtrlMgr_V9::IsV9Format(char* filename)
{
	// TODO: Implement version detection
	// Check file header for EXPORTER_MESH_VER9
	return false;
}
