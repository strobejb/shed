#ifndef SHED_CARDS_DLL_REPLACEMENT_H
#define SHED_CARDS_DLL_REPLACEMENT_H

#include <windows.h>

#define ectFACES  0
#define ectBACKS  1
#define ectINVERT 2

#define ecsCLUBS    0
#define ecsDIAMONDS 1
#define ecsHEARTS   2
#define ecsSPADES   3

#define ecbCROSSHATCH 53
#define ecbWEAVE1     54
#define ecbWEAVE2     55
#define ecbROBOT      56
#define ecbFLOWERS    57
#define ecbVINE1      58
#define ecbVINE2      59
#define ecbFISH1      60
#define ecbFISH2      61
#define ecbSHELLS     62
#define ecbCASTLE     63
#define ecbISLAND     64
#define ecbCARDHAND   65
#define ecbUNUSED     66
#define ecbTHE_X      67
#define ecbTHE_O      68

#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI cdtInit(int *width, int *height);
BOOL WINAPI cdtDraw(HDC hdc, int x, int y, int card, int type, DWORD color);
BOOL WINAPI cdtDrawExt(HDC hdc, int x, int y, int dx, int dy, int card, int type, DWORD color);
BOOL WINAPI cdtAnimate(HDC hdc, int cardback, int x, int y, int frame);
void WINAPI cdtTerm(void);

#ifdef __cplusplus
}
#endif

#endif
