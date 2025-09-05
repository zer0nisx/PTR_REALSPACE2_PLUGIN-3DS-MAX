#ifndef RFONT_H
#define RFONT_H

#pragma warning(disable: 4786)

#include "RTypes.h"
#include <d3dx9.h>
#include <map>
#include <list>
#include <string>
#include "Realspace2.h"
#include "MemPool.h"

using namespace std;

_NAMESPACE_REALSPACE2_BEGIN

struct RCHARINFO : public CMemPoolSm<RCHARINFO> {
	int nWidth;
	int nFontTextureID;
	int nFontTextureIndex;
};

typedef map<WORD, RCHARINFO*> RCHARINFOMAP;

struct RFONTTEXTURECELLINFO;

typedef list<RFONTTEXTURECELLINFO*> RFONTTEXTURECELLINFOLIST;

struct RFONTTEXTURECELLINFO {
	int nID;
	int nIndex;
	RFONTTEXTURECELLINFOLIST::iterator itr;
};

class RFontTexture {
	HDC		m_hDC;
	DWORD* m_pBitmapBits;
	HBITMAP m_hbmBitmap;

	HBITMAP m_hPrevBitmap;

	LPDIRECT3DTEXTURE9		m_pTexture;
	int m_nWidth;
	int	m_nHeight;
	int m_nX, m_nY;
	int m_nCell;
	int m_LastUsedID;

	int m_nCellSize;

	RFONTTEXTURECELLINFO* m_CellInfo;
	RFONTTEXTURECELLINFOLIST m_PriorityQueue;

	bool UploadTexture(RCHARINFO* pCharInfo, DWORD* pBitmapBits, int w, int h);

	bool InitCellInfo();
	void ReleaseCellInfo();

public:
	RFontTexture();
	~RFontTexture();

	bool Create();
	void Destroy();

	LPDIRECT3DTEXTURE9 GetTexture() { return m_pTexture; }

	int GetCharWidth(HFONT hFont, const TCHAR* szChar);
	bool MakeFontBitmap(HFONT hFont, RCHARINFO* pInfo, const TCHAR* szText, int nOutlineStyle, DWORD nColorArg1, DWORD nColorArg2);
	bool IsNeedUpdate(int nIndex, int nID);

	int GetWidth() { return m_nWidth; }
	int GetHeight() { return m_nHeight; }
	int GetCellCountX() { return m_nX; }
	int GetCellCountY() { return m_nY; }

	int GetCellSize() { return m_nCellSize; }
	void ChangeCellSize(int size);
};

class RFont {
	HFONT	m_hFont;
	int		m_nHeight;
	int		m_nOutlineStyle;

	bool	m_bAntiAlias;

	DWORD	m_ColorArg1;
	DWORD	m_ColorArg2;

	RCHARINFOMAP m_CharInfoMap;
	RFontTexture* m_pFontTexture;

	static	bool	m_bInFont;

public:
	RFont(void);
	virtual ~RFont(void);

	bool Create(const TCHAR* szFontName, int nHeight, bool bBold = false, bool bItalic = false, int nOutlineStyle = 0, int nCacheSize = -1, bool bAntiAlias = false, DWORD nColorArg1 = 0, DWORD nColorArg2 = 0);
	void Destroy(void);

	bool BeginFont();
	bool EndFont();

	void DrawText(float x, float y, const TCHAR* szText, DWORD dwColor = 0xFFFFFFFF, float fScale = 1.0f);

	int GetHeight(void) { return m_nHeight; }
	int GetTextWidth(const TCHAR* szText, int nSize = -1);
};

bool DumpFontTexture();
bool RFontCreate();
void RFontDestroy();

_NAMESPACE_REALSPACE2_END

bool CheckFont();
void SetUserDefineFont(char* FontName);

#endif
