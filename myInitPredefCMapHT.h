/*
   Copyright (C) 2026 Vincenzo Lo Cicero

   Author: Vincenzo Lo Cicero.
   e-mail: vincenzolocicero68@gmail.com

   This file is part of mypdfsearch.

   mypdfsearch is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   mypdfsearch is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with mypdfsearch.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef	MY_INIT__PREDEFINED_CMAP_HASHTABLE_INCLUDED
#define	MY_INIT__PREDEFINED_CMAP_HASHTABLE_INCLUDED

#ifdef __cplusplus
#  define BEGIN_C_DECLS extern "C" {
#  define END_C_DECLS   }
#else // !__cplusplus 
#  define BEGIN_C_DECLS
#  define END_C_DECLS
#endif // __cplusplus 

//#include "scanner.h"

BEGIN_C_DECLS

int InitCMapHT(Params *pParams);

int CMapHT_CNS_EUC_H(Params *pParams);
int CMapHT_UniCNS_UCS2_V(Params *pParams);
int CMapHT_UniCNS_UTF16_V(Params *pParams);
int CMapHT_UniJIS_UCS2_V(Params *pParams);
int CMapHT_90msp_RKSJ_H(Params *pParams);
int CMapHT_GB_EUC_V(Params *pParams);
int CMapHT_ETenms_B5_V(Params *pParams);
int CMapHT_H(Params *pParams);
int CMapHT_UniGB_UTF16_V(Params *pParams);
int CMapHT_KSC_EUC_H(Params *pParams);
int CMapHT_83pv_RKSJ_H(Params *pParams);
int CMapHT_UniGB_UCS2_H(Params *pParams);
int CMapHT_CNS_EUC_V(Params *pParams);
int CMapHT_GBKp_EUC_V(Params *pParams);
int CMapHT_90msp_RKSJ_V(Params *pParams);
int CMapHT_Add_RKSJ_V(Params *pParams);
int CMapHT_B5pc_H(Params *pParams);
int CMapHT_90ms_RKSJ_H(Params *pParams);
int CMapHT_KSCms_UHC_HW_H(Params *pParams);
int CMapHT_UniJIS_UTF16_V(Params *pParams);
int CMapHT_UniKS_UTF16_H(Params *pParams);
int CMapHT_90pv_RKSJ_H(Params *pParams);
int CMapHT_HKscs_B5_H(Params *pParams);
int CMapHT_GBpc_EUC_H(Params *pParams);
int CMapHT_UniKS_UCS2_V(Params *pParams);
int CMapHT_UniJIS_UTF16_H(Params *pParams);
int CMapHT_GBK2K_H(Params *pParams);
int CMapHT_ETenms_B5_H(Params *pParams);
int CMapHT_90ms_RKSJ_V(Params *pParams);
int CMapHT_UniGB_UCS2_V(Params *pParams);
int CMapHT_GBK_EUC_V(Params *pParams);
int CMapHT_UniCNS_UCS2_H(Params *pParams);
int CMapHT_UniKS_UTF16_V(Params *pParams);
int CMapHT_V(Params *pParams);
int CMapHT_GBK2K_V(Params *pParams);
int CMapHT_EUC_H(Params *pParams);
int CMapHT_B5pc_V(Params *pParams);
int CMapHT_Ext_RKSJ_H(Params *pParams);
int CMapHT_GB_EUC_H(Params *pParams);
int CMapHT_EUC_V(Params *pParams);
int CMapHT_HKscs_B5_V(Params *pParams);
int CMapHT_GBK_EUC_H(Params *pParams);
int CMapHT_GBpc_EUC_V(Params *pParams);
int CMapHT_UniJIS_UCS2_HW_H(Params *pParams);
int CMapHT_KSCms_UHC_HW_V(Params *pParams);
int CMapHT_ETen_B5_V(Params *pParams);
int CMapHT_UniKS_UCS2_H(Params *pParams);
int CMapHT_UniGB_UTF16_H(Params *pParams);
int CMapHT_Add_RKSJ_H(Params *pParams);
int CMapHT_UniJIS_UCS2_HW_V(Params *pParams);
int CMapHT_GBKp_EUC_H(Params *pParams);
int CMapHT_UniCNS_UTF16_H(Params *pParams);
int CMapHT_Ext_RKSJ_V(Params *pParams);
int CMapHT_KSC_EUC_V(Params *pParams);
int CMapHT_ETen_B5_H(Params *pParams);
int CMapHT_KSCpc_EUC_H(Params *pParams);

END_C_DECLS

#endif // MY_INIT__PREDEFINED_CMAP_HASHTABLE_INCLUDED
