# API Reference

## 📚 Overview

Complete API reference for the PTR RealSpace2 PBR Pipeline, covering export plugin interfaces, engine classes, and material management systems.

## 🔧 Export Plugin API (MCPlug2)

### Core Export Interface

#### `class MCPlug2 : public SceneExport`

Main plugin class handling 3DS Max scene export.

```cpp
class MCPlug2 : public SceneExport {
public:
    // SceneExport interface implementation
    int ExtCount() override;
    const TCHAR* Ext(int n) override;
    const TCHAR* LongDesc() override;
    const TCHAR* ShortDesc() override;
    const TCHAR* AuthorName() override;
    const TCHAR* CopyrightMessage() override;
    const TCHAR* OtherMessage1() override;
    const TCHAR* OtherMessage2() override;
    unsigned int Version() override;
    void ShowAbout(HWND hWnd) override;

    // Export functionality
    int DoExport(const TCHAR* name, ExpInterface* ei, Interface* i,
                 BOOL suppressPrompts = FALSE, DWORD options = 0) override;
};
```

### PBR Detection and Export

#### `ShouldUsePBRPipeline()`

Analyzes a 3DS Max material to determine if PBR pipeline should be used.

```cpp
bool ShouldUsePBRPipeline(Mtl* material);
```

**Parameters:**
- `material` - Pointer to 3DS Max material to analyze

**Returns:**
- `true` if material has PBR characteristics
- `false` if material should use legacy pipeline

**Implementation Logic:**
1. Checks for Physical Material or PBR material types
2. Analyzes texture slot usage (normal maps, roughness, etc.)
3. Examines material naming conventions
4. Detects advanced material properties

**Example:**
```cpp
Mtl* sceneMaterial = GetMaterialFromScene();
if (ShouldUsePBRPipeline(sceneMaterial)) {
    // Use V9 PBR export pipeline
    DumpMaterial_V9(sceneMaterial, &mtrlData);
} else {
    // Use legacy export pipeline
    DumpMaterial(sceneMaterial, &legacyData);
}
```

#### `DumpMaterial_V9()`

Extracts PBR material data from 3DS Max material into V9 structure.

```cpp
void DumpMaterial_V9(Mtl* mtl, mtrl_data_v9* data);
```

**Parameters:**
- `mtl` - Source 3DS Max material
- `data` - Target V9 material data structure

**Functionality:**
- Extracts all 10 texture types (diffuse, normal, roughness, etc.)
- Copies PBR properties (roughness, metallic, IOR)
- Processes UV transformations for 8 channels
- Sets texture activation flags

**Example:**
```cpp
mtrl_data_v9 materialData;
ZeroMemory(&materialData, sizeof(mtrl_data_v9));
DumpMaterial_V9(maxMaterial, &materialData);
```

#### `DumpTexture_V9()`

Extracts texture information for V9 format export.

```cpp
void DumpTexture_V9(Texmap* texmap, char* filename, int channel, mtrl_data_v9* data);
```

**Parameters:**
- `texmap` - 3DS Max texture map
- `filename` - Output filename buffer
- `channel` - UV channel index (0-7)
- `data` - Material data structure to update

### Binary Export Functions

#### `export_bin_v9()`

Writes V9 format binary data to file.

```cpp
void export_bin_v9(FILE* file);
```

**Parameters:**
- `file` - Target file handle for binary writing

**Output Format:**
1. V9 header with `EXPORTER_MESH_VER9` identifier
2. Material count
3. Array of `mtrl_data_v9` structures
4. Mesh data with extended UV channels

#### `export_bin()`

Legacy export function with automatic V9 detection.

```cpp
void export_bin(FILE* file);
```

**Parameters:**
- `file` - Target file handle

**Logic:**
1. Analyzes scene materials for PBR content
2. Automatically calls `export_bin_v9()` if PBR detected
3. Falls back to legacy export for non-PBR materials

## 🎮 Engine API (RealSpace2)

### Material Classes

#### `class RMtrl_V9 : public RMtrl`

Extended material class supporting PBR textures and properties.

```cpp
class RMtrl_V9 : public RMtrl {
private:
    // PBR Texture Resources
    RBaseTexture* m_pNormalTexture;      // Normal mapping
    RBaseTexture* m_pRoughnessTexture;   // Surface roughness
    RBaseTexture* m_pMetallicTexture;    // Metallic properties
    RBaseTexture* m_pEmissiveTexture;    // Emissive lighting
    RBaseTexture* m_pAOTexture;          // Ambient occlusion
    RBaseTexture* m_pHeightTexture;      // Height/displacement
    RBaseTexture* m_pReflectionTexture;  // Reflection mapping
    RBaseTexture* m_pRefractionTexture;  // Refraction mapping
    RBaseTexture* m_pSpecularTexture;    // Specular highlights

    // PBR Material Properties
    float m_roughness;                   // Surface roughness (0.0-1.0)
    float m_metallic;                    // Metallic factor (0.0-1.0)
    float m_ior;                         // Index of refraction
    float m_emissive_intensity;          // Emissive light intensity
    D3DXCOLOR m_emissive;               // Emissive color

    // UV Transformation System
    D3DXVECTOR2 m_uv_tiling[8];         // UV tiling per channel
    D3DXVECTOR2 m_uv_offset[8];         // UV offset per channel
    float m_uv_rotation[8];              // UV rotation per channel

    // Texture Management
    DWORD m_texture_flags;               // Active texture bitmask

public:
    // Constructor/Destructor
    RMtrl_V9();
    virtual ~RMtrl_V9();

    // Material Initialization
    void InitializeFromData(const mtrl_data_v9* data);

    // Texture Loading (overridden from RMtrl)
    virtual void Restore(LPDIRECT3DDEVICE9 dev, char* path) override;
    virtual void Invalidate() override;

    // PBR Property Accessors
    float GetRoughness() const { return m_roughness; }
    float GetMetallic() const { return m_metallic; }
    float GetIOR() const { return m_ior; }
    float GetEmissiveIntensity() const { return m_emissive_intensity; }
    const D3DXCOLOR& GetEmissiveColor() const { return m_emissive; }

    // Texture Accessors
    RBaseTexture* GetNormalTexture() const { return m_pNormalTexture; }
    RBaseTexture* GetRoughnessTexture() const { return m_pRoughnessTexture; }
    RBaseTexture* GetMetallicTexture() const { return m_pMetallicTexture; }
    // ... additional texture getters

    // UV Transformation Accessors
    const D3DXVECTOR2& GetUVTiling(int channel) const;
    const D3DXVECTOR2& GetUVOffset(int channel) const;
    float GetUVRotation(int channel) const;

    // Utility Functions
    bool HasTexture(PBRTextureType type) const;
    void SetTextureFlag(PBRTextureType type, bool active);
    bool IsTextureActive(PBRTextureType type) const;
};
```

**Key Methods:**

##### `InitializeFromData()`
```cpp
void RMtrl_V9::InitializeFromData(const mtrl_data_v9* data);
```
Initializes material from loaded V9 binary data.

##### `Restore()`
```cpp
void RMtrl_V9::Restore(LPDIRECT3DDEVICE9 dev, char* path) override;
```
Loads all PBR textures from disk with automatic filename detection.

**Implementation:**
1. Calls base class `RMtrl::Restore()` for legacy textures
2. Loads additional PBR textures based on naming conventions
3. Applies UV transformations to loaded textures
4. Updates texture activation flags

### Material Management

#### `class RMtrlMgr_V9 : public RMtrlMgr`

Manager class for V9 PBR materials with enhanced functionality.

```cpp
class RMtrlMgr_V9 : public RMtrlMgr {
private:
    vector<RMtrl_V9*> m_v9_materials;    // V9 material collection
    bool m_bV9Mode;                      // V9 mode flag

public:
    // Constructor/Destructor
    RMtrlMgr_V9();
    virtual ~RMtrlMgr_V9();

    // V9 Material Management
    void Add(RMtrl_V9* pMaterial);
    RMtrl_V9* Get(int index);
    RMtrl_V9* Get(const char* name);
    int GetCount() const;

    // File I/O Operations
    void LoadListV9(char* filename);
    void SaveListV9(char* filename);
    bool IsV9Format(char* filename);

    // Restoration and Management
    void RestoreV9(LPDIRECT3DDEVICE9 dev);
    void InvalidateV9();

    // Format Detection
    static bool DetectV9Format(const char* filename);

    // Legacy Compatibility
    void ConvertToLegacy(RMtrlMgr* legacyMgr);

    // Memory Management
    void ClearV9Materials();
    void OptimizeMemory();
};
```

**Key Methods:**

##### `LoadListV9()`
```cpp
void RMtrlMgr_V9::LoadListV9(char* filename);
```
Loads V9 material list from binary file.

##### `IsV9Format()`
```cpp
bool RMtrlMgr_V9::IsV9Format(char* filename);
```
Detects if file contains V9 format materials.

##### `RestoreV9()`
```cpp
void RMtrlMgr_V9::RestoreV9(LPDIRECT3DDEVICE9 dev);
```
Restores all V9 materials with their PBR textures.

### Mesh Loading Integration

#### `class RMesh` Extensions

Additional methods added to support V9 material loading.

```cpp
class RMesh {
private:
    RMtrlMgr_V9* m_pMtrlMgr_V9;         // V9 material manager
    bool m_bUseV9Materials;              // V9 mode flag

public:
    // V9 Material Support
    void SetV9MaterialManager(RMtrlMgr_V9* pMgr);
    RMtrlMgr_V9* GetV9MaterialManager() const;

    // Enhanced Loading
    void LoadMaterialsV9(MZFile* pfile);
    bool DetectMaterialVersion(MZFile* pfile);

    // Compatibility
    void MigrateToV9();
    void FallbackToLegacy();
};
```

##### `LoadMaterialsV9()`
```cpp
void RMesh::LoadMaterialsV9(MZFile* pfile);
```
Loads V9 materials from binary mesh file.

**Implementation:**
1. Reads material count from file
2. Iterates through materials, reading `mtrl_data_v9` structures
3. Creates `RMtrl_V9` instances for each material
4. Adds materials to V9 manager
5. Sets up material references in mesh

## 📋 Data Structures

### `mtrl_data_v9`

Complete V9 material data structure.

```cpp
struct mtrl_data_v9 {
    // Legacy Compatibility (first 4 fields)
    char  diffuse_map[256];              // Diffuse texture filename
    char  opacity_map[256];              // Opacity texture filename
    DWORD diffuse_color;                 // Diffuse color (ARGB)
    DWORD opacity;                       // Opacity value (0-255)

    // Extended PBR Texture Maps
    char  normal_map[256];               // Normal mapping texture
    char  roughness_map[256];            // Roughness texture
    char  metallic_map[256];             // Metallic texture
    char  emissive_map[256];             // Emissive texture
    char  ao_map[256];                   // Ambient occlusion texture
    char  height_map[256];               // Height/displacement texture
    char  reflection_map[256];           // Reflection texture
    char  refraction_map[256];           // Refraction texture
    char  specular_map[256];             // Specular highlight texture

    // PBR Material Properties
    float roughness;                     // Surface roughness (0.0-1.0)
    float metallic;                      // Metallic factor (0.0-1.0)
    float ior;                          // Index of refraction (1.0-3.0)
    float emissive_intensity;            // Emissive light intensity
    DWORD emissive_color;               // Emissive color (RGB)

    // UV Transformation System (8 channels)
    float uv_tiling_u[8];               // U-axis tiling per channel
    float uv_tiling_v[8];               // V-axis tiling per channel
    float uv_offset_u[8];               // U-axis offset per channel
    float uv_offset_v[8];               // V-axis offset per channel
    float uv_rotation[8];               // Rotation per channel (radians)

    // Texture Management
    DWORD texture_flags;                 // Bitmask for active textures

    // Future Extensions (reserved)
    DWORD reserved[16];                  // Reserved for future use
};
```

### `PBRTextureType` Enumeration

```cpp
enum PBRTextureType {
    PBR_DIFFUSE     = 0x001,    // Diffuse/albedo texture
    PBR_NORMAL      = 0x002,    // Normal mapping
    PBR_ROUGHNESS   = 0x004,    // Surface roughness
    PBR_METALLIC    = 0x008,    // Metallic properties
    PBR_EMISSIVE    = 0x010,    // Emissive lighting
    PBR_AO          = 0x020,    // Ambient occlusion
    PBR_HEIGHT      = 0x040,    // Height/displacement
    PBR_REFLECTION  = 0x080,    // Reflection mapping
    PBR_REFRACTION  = 0x100,    // Refraction mapping
    PBR_SPECULAR    = 0x200,    // Specular highlights
    PBR_OPACITY     = 0x400     // Opacity/transparency
};
```

## ⚙️ Constants and Defines

### Version Identifiers

```cpp
#define EXPORTER_MESH_VER9    0x00005008    // V9 format identifier
#define MATERIAL_V9_SIGNATURE 0x4D54524C    // "MTRL" signature
#define MAX_UV_CHANNELS       8             // Maximum UV channels
#define MAX_TEXTURE_FILENAME  256           // Maximum filename length
```

### Default Values

```cpp
// Default PBR Properties
const float DEFAULT_ROUGHNESS = 0.5f;
const float DEFAULT_METALLIC = 0.0f;
const float DEFAULT_IOR = 1.5f;
const float DEFAULT_EMISSIVE_INTENSITY = 0.0f;

// Default UV Transformations
const D3DXVECTOR2 DEFAULT_UV_TILING(1.0f, 1.0f);
const D3DXVECTOR2 DEFAULT_UV_OFFSET(0.0f, 0.0f);
const float DEFAULT_UV_ROTATION = 0.0f;
```

## 🔍 Usage Examples

### Export Plugin Usage

```cpp
// Check if material should use PBR pipeline
Mtl* material = GetMaterialFromNode(node);
if (ShouldUsePBRPipeline(material)) {
    // Extract PBR data
    mtrl_data_v9 mtrlData;
    DumpMaterial_V9(material, &mtrlData);

    // Write to file
    FILE* file = fopen("mesh.elu", "wb");
    export_bin_v9(file);
    fclose(file);
}
```

### Engine Loading Usage

```cpp
// Initialize V9 material system
RMtrlMgr_V9* pMtrlMgr = new RMtrlMgr_V9();
mesh->SetV9MaterialManager(pMtrlMgr);

// Load mesh with V9 materials
mesh->Open("mesh.elu", pDevice);

// Access PBR properties
RMtrl_V9* material = pMtrlMgr->Get(0);
float roughness = material->GetRoughness();
RBaseTexture* normalTex = material->GetNormalTexture();
```

---

This API reference provides complete documentation for integrating and extending the PBR material system in both export and engine environments.
