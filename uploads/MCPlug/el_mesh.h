#if !defined(AFX_EL_MESH_H__A5FA3593_78B0_4304_B36D_635328A82A04__INCLUDED_)
#define AFX_EL_MESH_H__A5FA3593_78B0_4304_B36D_635328A82A04__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif

#define DEL2(p) { if(p) { delete[] (p);   (p)=NULL; } }

#pragma warning (disable : 4530)

#define MAX_NAME_LEN		40
#define MAX_PATH_NAME_LEN	256
#define MAX_MTRL			400

#include <list>
#include <vector>

#include <D3DX9.h>

#include "RMeshUtil.h"

using namespace std;

enum AnimationType {
	RAniType_TransForm = 0,
	RAniType_Vertex,
	RAniType_Bone,
	RAniType_Tm,
};

class el_mesh;

class mesh_data
{
public:
	mesh_data();
	~mesh_data();

	int				m_id;
	int				m_u_id;

	el_mesh* List;

	char			m_Name[MAX_NAME_LEN];
	char			m_Parent[MAX_NAME_LEN];

	mesh_data* m_pParent;

	D3DXMATRIX		m_mat_base;
	D3DXMATRIX		m_mat_init;

	D3DXVECTOR3		m_ap_scale;

	D3DXVECTOR3		m_axis_scale;
	float			m_axis_scale_angle;

	D3DXVECTOR3		m_axis_rot;
	float			m_axis_rot_angle;

	D3DXMATRIX		m_etc_mat;

	bool			m_is_rot;
	bool			m_is_pos;

	bool			m_is_vertex_ani;

	int				m_point_num;
	int				m_face_num;

	D3DXVECTOR3* m_point_list;
	RFaceInfo* m_face_list;

	int				m_tex_point_num;
	D3DXVECTOR3* m_tex_point_list;

	int				m_physique_num;
	RPhysiqueInfo* m_physique;

	RFaceNormalInfo* m_face_normal_list;

	int				m_point_color_num;
	D3DXVECTOR3* m_point_color_list;

	D3DXVECTOR3** m_vertex_ani_list;
	DWORD* m_vertex_ani_frame;
	int				m_vertex_ani_num;

	int				m_mtrl_id;

	int				m_tex_face_num;

	bool			m_NormalFlag;

	RPosKey* m_pos_key;
	RQuatKey* m_quat_key;
	RTMKey* m_tm_key;
	RVisKey* m_vis_key;

	int				m_pos_key_num;
	int				m_quat_key_num;
	int				m_tm_key_num;
	int				m_vis_key_num;

	int				m_frame;
	int				m_max_frame;
	int				m_max_frame_tick;
};

typedef list<mesh_data*>		el_mesh_list;
typedef el_mesh_list::iterator	el_mesh_node;

class mtrl_data {
public:

	int					m_id;
	int					m_mtrl_id;
	int					m_sub_mtrl_id;

	char				m_tex_name[MAX_PATH_NAME_LEN];
	char				m_opa_name[MAX_PATH_NAME_LEN];

	D3DXCOLOR			m_ambient;
	D3DXCOLOR			m_diffuse;
	D3DXCOLOR			m_specular;

	float				m_power;
	int					m_twosided;
	int					m_sub_mtrl_num;
	int					m_additive;
	int					m_alphatest_ref;
	bool				m_bUse;
};

typedef list<mtrl_data*>		el_mtrl_list;
typedef el_mtrl_list::iterator	el_mtrl_node;

class mtrl_data_v9 {
public:
	// Basic material info (compatible with mtrl_data)
	int					m_id;
	int					m_mtrl_id;
	int					m_sub_mtrl_id;

	// Legacy texture paths (for backward compatibility)
	char				m_tex_name[MAX_PATH_NAME_LEN];		// Legacy diffuse
	char				m_opa_name[MAX_PATH_NAME_LEN];		// Legacy opacity

	// Extended PBR texture paths
	char				m_normal_map[MAX_PATH_NAME_LEN];
	char				m_specular_map[MAX_PATH_NAME_LEN];
	char				m_roughness_map[MAX_PATH_NAME_LEN];
	char				m_metallic_map[MAX_PATH_NAME_LEN];
	char				m_emissive_map[MAX_PATH_NAME_LEN];
	char				m_ao_map[MAX_PATH_NAME_LEN];		// Ambient Occlusion
	char				m_height_map[MAX_PATH_NAME_LEN];
	char				m_reflection_map[MAX_PATH_NAME_LEN];
	char				m_refraction_map[MAX_PATH_NAME_LEN];

	// Material properties
	D3DXCOLOR			m_ambient;
	D3DXCOLOR			m_diffuse;
	D3DXCOLOR			m_specular;
	D3DXCOLOR			m_emissive;

	// PBR properties
	float				m_power;				// Legacy shininess
	float				m_roughness;			// PBR roughness (0.0-1.0)
	float				m_metallic;				// PBR metallic (0.0-1.0)
	float				m_opacity;				// Alpha value
	float				m_ior;					// Index of refraction
	float				m_emissive_intensity;	// Emissive strength

	// Surface properties
	int					m_twosided;
	int					m_sub_mtrl_num;
	int					m_additive;
	int					m_alphatest_ref;
	int					m_material_type;		// 0=Legacy, 1=PBR, 2=Unlit, etc.

	// UV channel assignments
	int					m_uv_diffuse;			// UV channel for diffuse (0-7)
	int					m_uv_normal;			// UV channel for normal map
	int					m_uv_specular;			// UV channel for specular
	int					m_uv_roughness;			// UV channel for roughness
	int					m_uv_metallic;			// UV channel for metallic
	int					m_uv_emissive;			// UV channel for emissive
	int					m_uv_ao;				// UV channel for AO
	int					m_uv_height;			// UV channel for height

	// Texture scaling and offset per UV channel
	D3DXVECTOR2			m_uv_tiling[8];			// Tiling for each UV channel
	D3DXVECTOR2			m_uv_offset[8];			// Offset for each UV channel
	float				m_uv_rotation[8];		// Rotation for each UV channel

	// Flags
	bool				m_bUse;
	bool				m_bHasNormalMap;
	bool				m_bHasSpecularMap;
	bool				m_bHasRoughnessMap;
	bool				m_bHasMetallicMap;
	bool				m_bHasEmissiveMap;
	bool				m_bHasAOMap;
	bool				m_bHasHeightMap;

	// Constructor
	mtrl_data_v9() {
		memset(this, 0, sizeof(mtrl_data_v9));

		// Set default values
		m_roughness = 0.5f;
		m_metallic = 0.0f;
		m_opacity = 1.0f;
		m_ior = 1.5f;
		m_emissive_intensity = 1.0f;
		m_material_type = 1; // PBR by default

		// Initialize UV channels to 0
		for (int i = 0; i < 8; i++) {
			m_uv_tiling[i] = D3DXVECTOR2(1.0f, 1.0f);
			m_uv_offset[i] = D3DXVECTOR2(0.0f, 0.0f);
			m_uv_rotation[i] = 0.0f;
		}

		m_bUse = true;
	}
};

typedef list<mtrl_data_v9*>		el_mtrl_list_v9;
typedef el_mtrl_list_v9::iterator	el_mtrl_node_v9;

class el_mesh
{
public:
	el_mesh();
	virtual ~el_mesh();

	int	 add_mesh(mesh_data* data);
	int	 add_mtrl(mtrl_data* data);
	int	 add_mtrl_v9(mtrl_data_v9* data);

	void del_mesh_list();

	bool export_text(char* filename);
	bool export_bin(char* filename);
	bool export_bin_v9(char* filename);
	bool export_ani(char* filename, int mode);
	bool export_etc(char* filename);

	int  ani_node_cnt();

	void ClearVoidMtrl();
	void ClearUsedMtrlCheck();
	void ClearUsedMtrl();

	mtrl_data* GetMtrl(int id, int sid);
	mtrl_data_v9* GetMtrl_V9(int id, int sid);

public:
	int				m_max_frame;

	el_mesh_list	m_list;
	vector<mesh_data*> m_data;
	int				m_data_num;

	el_mtrl_list	m_mtrl_list;
	vector<mtrl_data*> m_mtrl_data;
	int				m_mtrl_num;

	// V9 support
	el_mtrl_list_v9	m_mtrl_list_v9;
	vector<mtrl_data_v9*> m_mtrl_data_v9;
	int				m_mtrl_v9_num;
	bool			m_has_pbr_materials;
};

#endif
