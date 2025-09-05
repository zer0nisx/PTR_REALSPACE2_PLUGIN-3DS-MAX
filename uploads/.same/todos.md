# 🚀 Plan de Implementación EXPORTER_MESH_VER9 - Materiales PBR

## ✅ Análisis Completado
- [x] Analizar sistema actual de materiales MCPlug
- [x] Entender estructura ex_hd_t y versioning
- [x] Revisar RMesh::ReadElu() y export_bin()
- [x] Identificar limitación: solo diffuse + opacity maps

## ✅ Tareas Completadas

### 1. Definir Nueva Versión
- [x] Agregar #define EXPORTER_MESH_VER9 0x00005008 en RMeshUtil.h
- [x] Crear nueva estructura mtrl_data_v9 extendida con 85+ campos PBR

### 2. Extender Plugin MCPlug
- [x] Crear estructura mtrl_data_v9 con soporte completo PBR
- [x] Agregar constantes para mapas PBR (normal, roughness, metallic, etc.)
- [x] Crear DumpTexture_V9() para exportar todos los mapas
- [x] Crear DumpMaterial_V9() con detección automática PBR
- [x] Actualizar export_bin() para escribir datos VER9

### 3. Actualizar Motor RealSpace2
- [x] Crear clase RMtrl_V9 extendida con 9 tipos de texturas
- [x] Implementar RMtrlMgr_V9 para gestión de materiales PBR
- [x] Modificar ReadElu() para cargar archivos VER9
- [x] Agregar soporte completo para múltiples texturas y UV channels

### 4. Funcionalidades Implementadas
- [x] Soporte para 10 tipos de texturas (diffuse, normal, specular, roughness, metallic, emissive, AO, height, reflection, refraction)
- [x] Propiedades PBR completas (roughness, metallic, IOR, emissive intensity)
- [x] 8 canales UV con transformaciones independientes (tiling, offset, rotation)
- [x] Detección automática de material PBR basada en nombres
- [x] Retrocompatibilidad total con formatos v1-v8

## 📋 Tareas Pendientes

### 1. Integración Completa
- [ ] Integrar mtrl_data_v9 en el pipeline completo del plugin
- [ ] Actualizar DumpMaterial para usar DumpMaterial_V9 cuando detecte PBR
- [ ] Implementar export_bin_v9() que use mtrl_data_v9 nativo

### 2. Carga de Texturas
- [ ] Implementar carga de texturas PBR en RMtrl_V9::Restore()
- [ ] Agregar detección automática de archivos PBR por sufijos (_normal, _roughness, etc.)
- [ ] Optimizar carga de múltiples texturas

### 3. Testing y Validación
- [ ] Crear archivos de prueba con materiales PBR
- [ ] Validar carga correcta de archivos v8 (retrocompatibilidad)
- [ ] Testear exportación e importación de archivos v9

## 🎯 Archivos a Modificar
- uploads/Include/RMeshUtil.h
- uploads/MCPlug/el_mesh.h
- uploads/MCPlug/MCPlug2_Mtrl.cpp
- uploads/MCPlug/el_mesh.cpp
- uploads/Include/RMtrl.h
- uploads/Source/RMtrl.cpp
- uploads/Source/RMesh_Load.cpp
