# PTR RealSpace2 PBR Pipeline Documentation

## 🎯 Overview

This documentation covers the complete **PBR (Physically Based Rendering) pipeline** implementation for the PTR RealSpace2 engine and its 3DS Max plugin. The system enables export and import of advanced PBR materials with full support for modern rendering workflows.

## 📚 Documentation Structure

### Core Documentation
- **[System Architecture](./architecture.md)** - Overall system design and component relationships
- **[API Reference](./api-reference.md)** - Complete function and class documentation
- **[Implementation Guide](./implementation-guide.md)** - Step-by-step development guide
- **[Technical Specifications](./technical-specs.md)** - Detailed format specifications

### User Guides
- **[Artist Workflow](./artist-workflow.md)** - Guide for 3D artists using the pipeline
- **[Developer Integration](./developer-integration.md)** - Integration guide for developers
- **[Migration Guide](./migration-guide.md)** - Upgrading from legacy material system

### Technical References
- **[Material Format V9](./material-format-v9.md)** - Complete V9 format specification
- **[PBR Properties](./pbr-properties.md)** - PBR material properties reference
- **[Texture Mapping](./texture-mapping.md)** - UV and texture system documentation

## 🚀 Quick Start

### For Artists
1. Configure PBR materials in 3DS Max using standard naming conventions
2. Export using the MCPlug plugin with automatic PBR detection
3. Materials automatically load in RealSpace2 engine with full PBR support

### For Developers
1. Review the [System Architecture](./architecture.md) for overall understanding
2. Check [API Reference](./api-reference.md) for integration points
3. Follow [Implementation Guide](./implementation-guide.md) for custom implementations

## ✨ Key Features

### PBR Material Support
- **10 Texture Types**: Diffuse, Normal, Specular, Roughness, Metallic, Emissive, AO, Height, Reflection, Refraction
- **PBR Properties**: Roughness, Metallic, IOR, Emissive Intensity
- **8 UV Channels**: Independent tiling, offset, and rotation per channel

### Advanced Features
- **Automatic Detection**: Smart PBR material detection based on naming conventions
- **Backward Compatibility**: Full support for legacy V1-V8 material formats
- **Optimized Loading**: Efficient texture loading and management system

### Export/Import Pipeline
- **3DS Max Plugin**: Seamless export from 3DS Max with PBR detection
- **RealSpace2 Engine**: Native PBR material loading and rendering
- **Format Versioning**: Robust versioning system for future extensions

## 🔧 System Requirements

### 3DS Max Plugin
- 3DS Max 2018 or later
- Visual Studio 2017+ for building
- Windows 10/11

### RealSpace2 Engine
- DirectX 9.0c compatible graphics card
- OpenGL 3.3+ support (optional)
- 512MB+ VRAM recommended

## 📊 Implementation Status

| Component | Status | Coverage |
|-----------|--------|----------|
| Export Pipeline | ✅ Complete | 100% |
| Import Pipeline | ✅ Complete | 100% |
| PBR Properties | ✅ Complete | 100% |
| Texture Loading | ✅ Complete | 100% |
| UV Mapping | ✅ Complete | 100% |
| Backward Compatibility | ✅ Complete | 100% |

## 🛠️ Build Instructions

### Plugin Build
```bash
# Open Visual Studio solution
start uploads/MCPlug/MCplug2.sln

# Build configuration: Release x64
# Output: MCplug2.dlu (3DS Max plugin)
```

### Engine Integration
```cpp
// Include headers
#include "RMesh.h"
#include "RMtrlMgr_V9.h"

// Initialize V9 material system
RMtrlMgr_V9* pMtrlMgr = new RMtrlMgr_V9();
```

## 📞 Support

For technical support or questions:
- Review the documentation sections above
- Check the [Implementation Guide](./implementation-guide.md) for common issues
- Consult the [API Reference](./api-reference.md) for function details

## 📄 License

This documentation and implementation are part of the PTR RealSpace2 project.

---

**Last Updated**: December 2024
**Version**: 1.0
**Pipeline Status**: ✅ Production Ready
