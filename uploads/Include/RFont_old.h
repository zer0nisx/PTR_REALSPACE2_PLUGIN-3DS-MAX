#ifndef RFONT_H
#define RFONT_H

#pragma warning(disable: 4786)

#include "RTypes.h"
#include <d3dx8.h>
#include <map>
#include <list>
#include <string>
#include "Realspace2.h"

using namespace std;




enum r_font_type {
	font_type_16 = 0,
	font_type_32,

	font_type_end,
};

class RBaseFontTextureInfo {
public:
	RBaseFontTextureInfo() {

		m_nSize = 0;
		m_nLastIndex = 0;
		m_nTableIndex = 0; 

		m_nMinIndexSize	= 0;
		m_nMaxIndexSize	= 0;

		m_fXLen = 0.f;
		m_fYLen = 0.f;
	}

	int m_nSize;
	int m_nLastIndex;

	float m_fXLen;
	float m_fYLen;

	int m_nTableIndex;

	int m_nMinIndexSize;
	int m_nMaxIndexSize;
};

#define FONT_TEXTURE_MAX 512



class RBaseFontTexture;

class RBaseFontTexture {
protected:
	RBaseFontTexture() {

		m_bLock = false;

		m_nTextureWidth	= 0;
		m_nTextureHeight= 0;

		m_pTexture = NULL;
		m_pInfo = NULL;

		Create(RealSpace2::RGetDevice(),FONT_TEXTURE_MAX,FONT_TEXTURE_MAX);
	}

public:

	~RBaseFontTexture() {
		Destroy();
	}

	static RBaseFontTexture* GetInstance() {
		static RBaseFontTexture pInst;
		return &pInst;
	}
	bool Create(LPDIRECT3DDEVICE8 dev,int w,int h);


	void Destroy() {
		Clear();
	}

	void Clear();

	long Lock() {

		HRESULT hr;

		if(m_pTexture==NULL) return -1;
		if(m_bLock) return 0;

		hr = m_pTexture->LockRect(0, &m_d3dlr, 0, 0);

		m_bLock = true;

		return hr;
	}

	long UnLock() {

		HRESULT hr;

		if(m_pTexture==NULL) return -1;
		if(m_bLock==false) return 0;

		hr = m_pTexture->UnlockRect(0);

		m_bLock = false;

		return hr;
	}

	bool GetInfo(int type);
	void GetIndexPos(int type,int index,int& x,int& y);
	BYTE* GetPosPtr(int type,int x,int y);
	BYTE* GetIndexPtr(int type,int index);
	void GetIndexUV(int type,int index,int w,int h,float*uv);

	int MakeTexture(int type);
	int MakeTexture(int type,DWORD* pBitmapBits,int w,int h,int nSamplingMultiplier,int debug_cnt);

	LPDIRECT3DTEXTURE8 GetTexture() {
		return m_pTexture;
	}

	

	bool isNeedUpdate(int type,int table,int index) {

		if( m_info[type].m_nTableIndex!=table ) {
			if(m_info[type].m_nTableIndex > table+1) {
				return true;
			}
			else {
				if( m_info[type].m_nLastIndex >= index ) {
					return true;
				}
			}
		}
		return false;
	}

public:

	bool m_bLock;

	D3DLOCKED_RECT		m_d3dlr;
	LPDIRECT3DTEXTURE8	m_pTexture;

	int	m_nTextureWidth;
	int m_nTextureHeight;		

	RBaseFontTextureInfo* m_pInfo;
	RBaseFontTextureInfo m_info[font_type_end];
};

#define GetBaseFontTexture() RBaseFontTexture::GetInstance()

_NAMESPACE_REALSPACE2_BEGIN

class RFont;

class RFontTexture{
public:
	LPDIRECT3DDEVICE8		m_pd3dDevice;		
	LPDIRECT3DTEXTURE8		m_pTexture;			

	int	m_nTextWidth, m_nTextHeight;			
	int	m_nTextureWidth, m_nTextureHeight;		
	int	m_nRealTextWidth,m_nRealTextHeight;

	RFont*	m_pFont;
	int		m_nIndex;
	int		m_nFontType;
	int		m_nTableIndex;

public:
	enum FTCOLORMODE {
		FTCOLORMODE_DIFFUSE,
		FTCOLORMODE_FIXED
	};


public:
	RFontTexture(void);
	virtual ~RFontTexture(void);

	bool Create(RFont* pFont , HFONT hFont, const TCHAR* szText, int nOutlineStyle, DWORD nColorArg1, DWORD nColorArg2, int nSamplingMultiplier);
	void Destroy(void);

	void BeginState(FTCOLORMODE nMode=FTCOLORMODE_DIFFUSE);
	void EndState(void);

	void DrawText(FTCOLORMODE nMode, float x, float y, DWORD dwColor,float fScale=1.0f);

	int GetTextWidth(void){ return m_nTextWidth; }
	int GetTextHeight(void){ return m_nTextHeight; }

	static DWORD			m_dwStateBlock;		
};

struct RFONTTEXTUREINFO;



typedef map<WORD,RFONTTEXTUREINFO*>		FONTTEXTUREMAP;
typedef map<WORD,int>					FONTWIDTHMAP;


typedef list<FONTTEXTUREMAP::iterator>	FONTTEXTUREPRIORITYQUE;

struct RFONTTEXTUREINFO{
	RFontTexture						FontTexture;
	FONTTEXTUREPRIORITYQUE::iterator	pqi;
};


class RFont {
public:
	LPDIRECT3DDEVICE8	m_pd3dDevice;		
	RBaseFontTexture*	m_pTex;

	static DWORD m_dwStateBlock;

	void BeginFontState();
	void EndFontState(void);

	void BeginState();
	void EndState(void);

	int m_nDebugFontCount;

	bool m_isBigFont;

protected:
	HFONT					m_hFont;	

	FONTTEXTUREMAP			m_FontMap;
	FONTWIDTHMAP			m_FontWidthMap;
	int						m_nCacheSize;
	FONTTEXTUREPRIORITYQUE	m_FontUsage;
	int						m_nHeight;
	int						m_nOutlineStyle;
	int						m_nSamplingMultiplier;

	DWORD					m_ColorArg1;
	DWORD					m_ColorArg2;

	bool					m_bFontBegin;

public:
	RFont(void);
	virtual ~RFont(void);

	bool Create(LPDIRECT3DDEVICE8 pd3dDevice, const TCHAR* szFontName, int nHeight, bool bBold=false, bool bItalic=false, int nOutlineStyle=0, int nCacheSize=-1, int nSamplingMultiplier=1, DWORD nColorArg1=0, DWORD nColorArg2=0);
	void Destroy(void);






	bool BeginFont();
	bool EndFont();

	void DrawText(float x, float y, const TCHAR* szText, DWORD dwColor=0xFFFFFFFF, float fScale=1.0f);

	int GetSamplingMultiplier(void){ return m_nSamplingMultiplier; }
	int GetHeight(void){ return m_nHeight; }
	int GetTextWidth(const TCHAR* szText, int nSize=-1);

protected:
	int GetCharWidth(const TCHAR* szChar);
};

void RFont_Render();

void Frame_Begin();
void Frame_End();

void ResetFont();

_NAMESPACE_REALSPACE2_END

bool CheckFont();
void SetUserDefineFont(char* FontName);

#endif
