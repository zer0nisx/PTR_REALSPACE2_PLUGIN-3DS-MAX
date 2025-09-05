#pragma once

struct RHEADER {
	DWORD dwID;
	DWORD dwVersion;

	RHEADER() {}
	RHEADER(DWORD id, DWORD version) { dwID = id; dwVersion = version; }
};

#define RS_ID			0x12345678
#define RS_VERSION		7

#define RBSP_ID			0x35849298
#define RBSP_VERSION	2

#define R_PAT_ID		0x09784348
#define R_PAT_VERSION	0

#define R_LM_ID			0x30671804
#define R_LM_VERSION	3

#define R_COL_ID		0x5050178f
#define R_COL_VERSION	0

#define R_NAV_ID		0x8888888f
#define R_NAV_VERSION	2