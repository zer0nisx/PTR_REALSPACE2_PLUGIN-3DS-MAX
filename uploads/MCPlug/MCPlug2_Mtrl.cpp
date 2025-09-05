#include "MCplug2.h"

BOOL MtlKeeper::AddMtl(Mtl* mtl)
{
	if (!mtl) return FALSE;

	int numMtls = mtlTab.Count();

	for (int i = 0; i < numMtls; i++) {
		if (mtlTab[i] == mtl) {
			return FALSE;
		}
	}

	mtlTab.Append(1, &mtl, 25);

	return TRUE;
}

int MtlKeeper::GetMtlID(Mtl* mtl)
{
	int numMtls = mtlTab.Count();

	for (int i = 0; i < numMtls; i++) {
		if (mtlTab[i] == mtl)
			return i;
	}

	return -1;
}

int MtlKeeper::Count()
{
	return mtlTab.Count();
}

Mtl* MtlKeeper::GetMtl(int id)
{
	return mtlTab[id];
}

void MCplug2::export_mtrl_list()
{
	int numMtls = mtlList.Count();

	for (int i = 0; i < numMtls; i++) {
		DumpMaterial(mtlList.GetMtl(i), i, -1);
	}
}

void MCplug2::DumpMaterial(Mtl* mtl, int mtlID, int subNo)
{
	mtrl_data* mtrl_node = new mtrl_data;
	memset(mtrl_node, 0, sizeof(mtrl_data));

	mtrl_node->m_mtrl_id = mtlID;
	mtrl_node->m_sub_mtrl_id = subNo;
	mtrl_node->m_power = 0.f;

	int i;
	TimeValue t = GetStaticFrame();

	if (!mtl) return;

	TSTR className;
	mtl->GetClassName(className);

	if (mtl->ClassID() == Class_ID(DMTL_CLASS_ID, 0)) {
		StdMat* std = (StdMat*)mtl;

		mtrl_node->m_ambient.r = std->GetAmbient(t).r;
		mtrl_node->m_ambient.g = std->GetAmbient(t).g;
		mtrl_node->m_ambient.b = std->GetAmbient(t).b;

		mtrl_node->m_diffuse.r = std->GetDiffuse(t).r;
		mtrl_node->m_diffuse.g = std->GetDiffuse(t).g;
		mtrl_node->m_diffuse.b = std->GetDiffuse(t).b;

		mtrl_node->m_specular.r = std->GetSpecular(t).r;
		mtrl_node->m_specular.g = std->GetSpecular(t).g;
		mtrl_node->m_specular.b = std->GetSpecular(t).b;

		mtrl_node->m_twosided = std->GetTwoSided() ? 1 : 0;
		mtrl_node->m_power = std->GetShinStr(t);

		mtrl_node->m_additive = (std->GetTransparencyType() == TRANSP_ADDITIVE) ? 1 : 0;

		if (strnicmp(mtl->GetName(), "at_", 3) == 0)
			mtrl_node->m_alphatest_ref = 100;
	}
	else {
		mtrl_node->m_ambient.r = mtl->GetAmbient().r;
		mtrl_node->m_ambient.g = mtl->GetAmbient().g;
		mtrl_node->m_ambient.b = mtl->GetAmbient().b;

		mtrl_node->m_diffuse.r = mtl->GetDiffuse().r;
		mtrl_node->m_diffuse.g = mtl->GetDiffuse().g;
		mtrl_node->m_diffuse.b = mtl->GetDiffuse().b;

		mtrl_node->m_specular.r = mtl->GetSpecular().r;
		mtrl_node->m_specular.g = mtl->GetSpecular().g;
		mtrl_node->m_specular.b = mtl->GetSpecular().b;
	}

	for (i = 0; i < mtl->NumSubTexmaps(); i++) {
		Texmap* subTex = mtl->GetSubTexmap(i);
		float amt = 1.0f;

		if (subTex) {
			if (mtl->ClassID() == Class_ID(DMTL_CLASS_ID, 0)) {
				if (!((StdMat*)mtl)->MapEnabled(i)) continue;
				amt = ((StdMat*)mtl)->GetTexmapAmt(i, 0);
			}

			DumpTexture(subTex, mtl->ClassID(), i, amt, mtrl_node);
		}
	}

	if (mtl->NumSubMtls() > 0) {
		mtrl_node->m_sub_mtrl_num = mtl->NumSubMtls();

		for (i = 0; i < mtl->NumSubMtls(); i++) {
			Mtl* subMtl = mtl->GetSubMtl(i);

			if (subMtl) DumpMaterial(subMtl, mtlID, i);
		}
	}

	m_mesh_list.add_mtrl(mtrl_node);
}

// ===== EXPORTER_MESH_VER9 - Enhanced Material Dumping =====
void MCplug2::DumpMaterial_V9(Mtl* mtl, int mtlID, int subNo)
{
	mtrl_data_v9* mtrl_node = new mtrl_data_v9;

	mtrl_node->m_mtrl_id = mtlID;
	mtrl_node->m_sub_mtrl_id = subNo;

	int i;
	TimeValue t = GetStaticFrame();

	if (!mtl) return;

	TSTR className;
	mtl->GetClassName(className);

	if (mtl->ClassID() == Class_ID(DMTL_CLASS_ID, 0)) {
		StdMat* std = (StdMat*)mtl;

		// Basic material properties
		mtrl_node->m_ambient.r = std->GetAmbient(t).r;
		mtrl_node->m_ambient.g = std->GetAmbient(t).g;
		mtrl_node->m_ambient.b = std->GetAmbient(t).b;

		mtrl_node->m_diffuse.r = std->GetDiffuse(t).r;
		mtrl_node->m_diffuse.g = std->GetDiffuse(t).g;
		mtrl_node->m_diffuse.b = std->GetDiffuse(t).b;

		mtrl_node->m_specular.r = std->GetSpecular(t).r;
		mtrl_node->m_specular.g = std->GetSpecular(t).g;
		mtrl_node->m_specular.b = std->GetSpecular(t).b;

		// Legacy properties
		mtrl_node->m_twosided = std->GetTwoSided() ? 1 : 0;
		mtrl_node->m_power = std->GetShinStr(t) * 100.f;
		mtrl_node->m_additive = (std->GetTransparencyType() == TRANSP_ADDITIVE) ? 1 : 0;

		// PBR properties (convert from legacy if no specific PBR values)
		mtrl_node->m_roughness = 1.0f - (std->GetShininess(t) / 100.0f); // Convert shininess to roughness
		mtrl_node->m_metallic = 0.0f; // Default non-metallic
		mtrl_node->m_opacity = 1.0f - std->GetXParency(t);

		// Emissive from self-illumination
		float selfIllum = std->GetSelfIllum(t);
		mtrl_node->m_emissive.r = mtrl_node->m_diffuse.r * selfIllum;
		mtrl_node->m_emissive.g = mtrl_node->m_diffuse.g * selfIllum;
		mtrl_node->m_emissive.b = mtrl_node->m_diffuse.b * selfIllum;
		mtrl_node->m_emissive_intensity = selfIllum;

		// Alpha test detection
		if (strnicmp(mtl->GetName(), "at_", 3) == 0)
			mtrl_node->m_alphatest_ref = 100;

		// Material naming convention detection for PBR hints
		const char* matName = mtl->GetName();
		if (strstr(matName, "_pbr") || strstr(matName, "_PBR")) {
			mtrl_node->m_material_type = 1; // PBR
		}
		else if (strstr(matName, "_unlit")) {
			mtrl_node->m_material_type = 2; // Unlit
		}
		else {
			mtrl_node->m_material_type = 0; // Legacy
		}
	}
	else {
		// Non-standard material
		mtrl_node->m_ambient.r = mtl->GetAmbient().r;
		mtrl_node->m_ambient.g = mtl->GetAmbient().g;
		mtrl_node->m_ambient.b = mtl->GetAmbient().b;

		mtrl_node->m_diffuse.r = mtl->GetDiffuse().r;
		mtrl_node->m_diffuse.g = mtl->GetDiffuse().g;
		mtrl_node->m_diffuse.b = mtl->GetDiffuse().b;

		mtrl_node->m_specular.r = mtl->GetSpecular().r;
		mtrl_node->m_specular.g = mtl->GetSpecular().g;
		mtrl_node->m_specular.b = mtl->GetSpecular().b;

		// Default PBR values for non-standard materials
		mtrl_node->m_material_type = 0; // Legacy
	}

	// Process all texture maps
	for (i = 0; i < mtl->NumSubTexmaps(); i++) {
		Texmap* subTex = mtl->GetSubTexmap(i);
		float amt = 1.0f;

		if (subTex) {
			if (mtl->ClassID() == Class_ID(DMTL_CLASS_ID, 0)) {
				if (!((StdMat*)mtl)->MapEnabled(i)) continue;
				amt = ((StdMat*)mtl)->GetTexmapAmt(i, 0);
			}

			DumpTexture_V9(subTex, mtl->ClassID(), i, amt, mtrl_node);
		}
	}

	// Process sub-materials
	if (mtl->NumSubMtls() > 0) {
		mtrl_node->m_sub_mtrl_num = mtl->NumSubMtls();

		for (i = 0; i < mtl->NumSubMtls(); i++) {
			Mtl* subMtl = mtl->GetSubMtl(i);

			if (subMtl) DumpMaterial_V9(subMtl, mtlID, i);
		}
	}

	// For now, add to the regular list (will need el_mesh extension)
	// TODO: Update el_mesh to support v9 materials
	//m_mesh_list.add_mtrl_v9(mtrl_node);

	// Temporary: Convert back to legacy format for compatibility
	mtrl_data* legacy_mtrl = new mtrl_data;
	memset(legacy_mtrl, 0, sizeof(mtrl_data));

	legacy_mtrl->m_id = mtrl_node->m_id;
	legacy_mtrl->m_mtrl_id = mtrl_node->m_mtrl_id;
	legacy_mtrl->m_sub_mtrl_id = mtrl_node->m_sub_mtrl_id;

	strcpy(legacy_mtrl->m_tex_name, mtrl_node->m_tex_name);
	strcpy(legacy_mtrl->m_opa_name, mtrl_node->m_opa_name);

	legacy_mtrl->m_ambient = mtrl_node->m_ambient;
	legacy_mtrl->m_diffuse = mtrl_node->m_diffuse;
	legacy_mtrl->m_specular = mtrl_node->m_specular;
	legacy_mtrl->m_power = mtrl_node->m_power;
	legacy_mtrl->m_twosided = mtrl_node->m_twosided;
	legacy_mtrl->m_sub_mtrl_num = mtrl_node->m_sub_mtrl_num;
	legacy_mtrl->m_additive = mtrl_node->m_additive;
	legacy_mtrl->m_alphatest_ref = mtrl_node->m_alphatest_ref;
	legacy_mtrl->m_bUse = mtrl_node->m_bUse;

	m_mesh_list.add_mtrl(legacy_mtrl);

	// Store v9 data for later export (will implement el_mesh v9 support)
	// TODO: Implement proper v9 storage
	delete mtrl_node; // Temporary cleanup
}

TCHAR* MCplug2::GetMapID(Class_ID cid, int subNo)
{
	static TCHAR buf[50];

	if (cid == Class_ID(0, 0)) {
		strcpy(buf, ID_ENVMAP);
	}
	else if (cid == Class_ID(DMTL_CLASS_ID, 0)) {
		switch (subNo) {
		case ID_AM: strcpy(buf, ID_MAP_AMBIENT);		break;
		case ID_DI: strcpy(buf, ID_MAP_DIFFUSE);		break;
		case ID_SP: strcpy(buf, ID_MAP_SPECULAR);		break;
		case ID_SH: strcpy(buf, ID_MAP_SHINE);			break;
		case ID_SS: strcpy(buf, ID_MAP_SHINESTRENGTH);	break;
		case ID_SI: strcpy(buf, ID_MAP_SELFILLUM);		break;
		case ID_OP: strcpy(buf, ID_MAP_OPACITY);		break;
		case ID_FI: strcpy(buf, ID_MAP_FILTERCOLOR);	break;
		case ID_BU: strcpy(buf, ID_MAP_BUMP);			break;
		case ID_RL: strcpy(buf, ID_MAP_REFLECT);		break;
		case ID_RR: strcpy(buf, ID_MAP_REFRACT);		break;
		// ===== EXPORTER_MESH_VER9 - PBR Maps =====
		case ID_NORMAL_MAP: strcpy(buf, ID_MAP_NORMAL);		break;
		case ID_ROUGHNESS_MAP: strcpy(buf, ID_MAP_ROUGHNESS);	break;
		case ID_METALLIC_MAP: strcpy(buf, ID_MAP_METALLIC);	break;
		case ID_EMISSIVE_MAP: strcpy(buf, ID_MAP_EMISSIVE);	break;
		case ID_AO_MAP: strcpy(buf, ID_MAP_AO);				break;
		case ID_HEIGHT_MAP: strcpy(buf, ID_MAP_HEIGHT);		break;
		}
	}
	else
		strcpy(buf, ID_MAP_GENERIC);

	return buf;
}

void el_cut_path(char* name)
{
	char r_name[256];

	for (int i = strlen(name) - 1; i >= 0; i--) {
		if (name[i] == '\\') {
			break;
		}
	}

	int st = i + 1;

	for (i = st; i < (int)strlen(name); i++) {
		r_name[i - st] = name[i];
	}

	r_name[i - st] = NULL;
	strcpy(name, r_name);
}

void MCplug2::DumpTexture(Texmap* tex, Class_ID cid, int subNo, float amt, mtrl_data* mtrl_node)
{
	if (tex == NULL) return;

	TSTR className;
	tex->GetClassName(className);

	if (tex->ClassID() == Class_ID(BMTEX_CLASS_ID, 0x00)) {
		TSTR mapName = ((BitmapTex*)tex)->GetMapName();
		TSTR str = FixupName(mapName);

		el_cut_path((char*)str);

		if (strcmp(GetMapID(cid, subNo), ID_MAP_DIFFUSE) == 0) {
			strcpy(mtrl_node->m_tex_name, str);
		}

		if (strcmp(GetMapID(cid, subNo), ID_MAP_OPACITY) == 0) {
			strcpy(mtrl_node->m_opa_name, str);
		}
	}
}

void MCplug2::DumpTexture_V9(Texmap* tex, Class_ID cid, int subNo, float amt, mtrl_data_v9* mtrl_node)
{
	if (tex == NULL) return;

	TSTR className;
	tex->GetClassName(className);

	if (tex->ClassID() == Class_ID(BMTEX_CLASS_ID, 0x00)) {
		TSTR mapName = ((BitmapTex*)tex)->GetMapName();
		TSTR str = FixupName(mapName);

		el_cut_path((char*)str);

		// Get the map type and assign to appropriate slot
		const char* mapType = GetMapID(cid, subNo);

		if (strcmp(mapType, ID_MAP_DIFFUSE) == 0) {
			strcpy(mtrl_node->m_tex_name, str);
			mtrl_node->m_uv_diffuse = 0; // Default UV channel
		}
		else if (strcmp(mapType, ID_MAP_OPACITY) == 0) {
			strcpy(mtrl_node->m_opa_name, str);
		}
		else if (strcmp(mapType, ID_MAP_NORMAL) == 0) {
			strcpy(mtrl_node->m_normal_map, str);
			mtrl_node->m_bHasNormalMap = true;
			mtrl_node->m_uv_normal = 0;
		}
		else if (strcmp(mapType, ID_MAP_SPECULAR) == 0) {
			strcpy(mtrl_node->m_specular_map, str);
			mtrl_node->m_bHasSpecularMap = true;
			mtrl_node->m_uv_specular = 0;
		}
		else if (strcmp(mapType, ID_MAP_ROUGHNESS) == 0) {
			strcpy(mtrl_node->m_roughness_map, str);
			mtrl_node->m_bHasRoughnessMap = true;
			mtrl_node->m_uv_roughness = 0;
		}
		else if (strcmp(mapType, ID_MAP_METALLIC) == 0) {
			strcpy(mtrl_node->m_metallic_map, str);
			mtrl_node->m_bHasMetallicMap = true;
			mtrl_node->m_uv_metallic = 0;
		}
		else if (strcmp(mapType, ID_MAP_EMISSIVE) == 0 || strcmp(mapType, ID_MAP_SELFILLUM) == 0) {
			strcpy(mtrl_node->m_emissive_map, str);
			mtrl_node->m_bHasEmissiveMap = true;
			mtrl_node->m_uv_emissive = 0;
		}
		else if (strcmp(mapType, ID_MAP_AO) == 0 || strcmp(mapType, ID_MAP_AMBIENT) == 0) {
			strcpy(mtrl_node->m_ao_map, str);
			mtrl_node->m_bHasAOMap = true;
			mtrl_node->m_uv_ao = 0;
		}
		else if (strcmp(mapType, ID_MAP_HEIGHT) == 0 || strcmp(mapType, ID_MAP_BUMP) == 0) {
			strcpy(mtrl_node->m_height_map, str);
			mtrl_node->m_bHasHeightMap = true;
			mtrl_node->m_uv_height = 0;
		}
		else if (strcmp(mapType, ID_MAP_REFLECT) == 0) {
			strcpy(mtrl_node->m_reflection_map, str);
		}
		else if (strcmp(mapType, ID_MAP_REFRACT) == 0) {
			strcpy(mtrl_node->m_refraction_map, str);
		}

		// Extract UV tiling and offset information if available
		if (tex->ClassID() == Class_ID(BMTEX_CLASS_ID, 0x00)) {
			BitmapTex* bmTex = (BitmapTex*)tex;
			StdUVGen* uvGen = bmTex->GetUVGen();
			if (uvGen) {
				TimeValue t = GetStaticFrame();

				// Get UV channel (default to 0 if not specified)
				int uvChannel = uvGen->GetMapChannel();
				if (uvChannel < 0 || uvChannel > 7) uvChannel = 0;

				// Get tiling
				mtrl_node->m_uv_tiling[uvChannel].x = uvGen->GetUScl(t);
				mtrl_node->m_uv_tiling[uvChannel].y = uvGen->GetVScl(t);

				// Get offset
				mtrl_node->m_uv_offset[uvChannel].x = uvGen->GetUOffs(t);
				mtrl_node->m_uv_offset[uvChannel].y = uvGen->GetVOffs(t);

				// Get rotation
				mtrl_node->m_uv_rotation[uvChannel] = uvGen->GetWAng(t);
			}
		}
	}
}

BOOL MCplug2::CheckForAndExportFaceMap(Mtl* mtl, Mesh* mesh)
{
	if (!mtl || !mesh) {
		return FALSE;
	}

	ULONG matreq = mtl->Requirements(-1);

	if (!(matreq & MTLREQ_FACEMAP)) {
		return FALSE;
	}

	for (int i = 0; i < mesh->getNumFaces(); i++) {
		Point3 tv[3];
		Face* f = &mesh->faces[i];
		make_face_uv(f, tv);
	}

	return TRUE;
}
