/*
   Copyright (C) 2019 Vincenzo Lo Cicero

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

#ifndef __MYPARSER__
#define __MYPARSER__

#include "mypdfsearch.h"
#include "scanner.h"

typedef enum tagPreParseStates
{
	S_PPError,
	S_PP0,
	S_PP1,
	S_PP2,
	S_PP3,
	S_PP4,
	S_PP5,
	S_PP6,
	S_PP7,
	S_PP8,
	S_PP9,
	S_PP10,
	S_PP11,
	S_PP12,
	S_PP13,
	S_PP14,
	S_PP15,
	S_PP16,
	S_PP17,
	S_PP18,
	S_PP19,
	S_PP20,
	S_PP21,
	S_PP22,
	S_PP23,
	S_PP24,
	S_PP25,
	S_PP26,
	S_PP27,
	S_PP28,
	S_PP29,
	S_PP30,
	S_PP31,
	S_PP32
} PreParseStates;

/*
PDF32000 Pag. 118:

• Translations shall be specified as [ 1 0 0 1 t x t y ], where t x and t y shall be the distances to translate the
  origin of the coordinate system in the horizontal and vertical dimensions, respectively.

• Scaling shall be obtained by [ s x 0 0 s y 0 0 ]. This scales the coordinates so that 1 unit in the horizontal
  and vertical dimensions of the new coordinate system is the same size as s x and s y units, respectively, in
  the previous coordinate system.

• Rotations shall be produced by [ cos q sin q -sin q cos q 0 0 ], which has the effect of rotating the
  coordinate system axes by an angle q counter clockwise.

• Skew shall be specified by [ 1 tan a tan b 1 0 0 ], which skews the x axis by an angle a and the y axis by an angle b.

-------------------------------------------------------------------------------------------------------------------------

The cm operator takes six arguments, representing a matrix to be composed with the
CTM. Here are the basic transforms:

• Translation by (dx, dy) is specified by 1, 0, 0, 1, dx, dy
• Scaling by (sx, sy) about (0, 0) is specified by sx, 0, 0, sy, 0, 0
• Rotating counterclockwise by x radians about (0, 0) is specified by cos x, sin x, -sin x, cos x, 0, 0
*/

/*
typedef struct tagTransMatrix
{
	double a; // Scaling sx
	double b; // Rotating counterclockwise by x radians about (b, 0)
	double c; // Rotating counterclockwise by x radians about (0, c)
	double d; // Scaling sy
	double e; // Translation dx
	double f; // Translation dy
} TransMatrix;

typedef struct tagGlyphsWidths
{
	int FirstChar;
	int LastChar;
	int WidthsArraySize;
	double *pWidths;
	double MissingWidth;
	double dFontSpaceWidth;
} GlyphsWidths;

typedef struct tagTextState
{
	double Tc;
	double Tw;
	double Th;
	double Tl;
	double Tfs;
	double Trise;
} TextState;
*/

void PrintTokenHex(const Token *pToken);

wchar_t ConvertCIDToUnicode(Params *pParams, int cid);

int IsUTF16Encoding(const char *szEncoding);

//static unsigned int DecodeUTF16BE(const unsigned char *pBuf, size_t bufLen, size_t *pConsumed);

int VlRbtCompareFuncCol(const void* pKey1, uint32_t keysize1, const void* pKey2, uint32_t keysize2);
int VlRbtCompareFuncRow(const void* pKey1, uint32_t keysize1, const void* pKey2, uint32_t keysize2);
int VlRbtCompareFuncOrd(const void* pKey1, uint32_t keysize1, const void* pKey2, uint32_t keysize2);
int VlRbtOnTraverseFunc(void* pCurrNode);
//int VlRbtOnTraverseFuncNew(void* pCurrNode);

void MultiplyTransMatrix(TransMatrix *pA, TransMatrix *pB, TransMatrix *pRes);

int getFontDataHT(Params *pParams, uint32_t objNum);
int insertFontDataHT(Params *pParams, uint32_t objNum);

void setPredefFontsWidthsArray(Params *pParams);
void set_Helvetica(Params *pParams);
void set_Helvetica_Bold(Params *pParams);
void set_Helvetica_BoldOblique(Params *pParams);
void set_Helvetica_Oblique(Params *pParams);
void set_Symbol(Params *pParams);
void set_Times_Bold(Params *pParams);
void set_Times_BoldItalic(Params *pParams);
void set_Times_Italic(Params *pParams);
void set_Times_Roman(Params *pParams);
void set_ZapfDingbats(Params *pParams);


unsigned char * getDecodedStream(Params *pParams, unsigned long int *pDecodedStreamSize, MyContent_t *pContent);

int OnTraverseTST(const wchar_t* key, void* data, uint32_t dataSize);

int InsertWordIntoTst(Params *pParams);

int InitializeCharSetHashTable(Params *pParams);
int InitializeUnicodeArray(Params *pParams);
void InitializeCharSetArrays(Params *pParams);
void MakeDifferencesArrayCodes(Params *pParams);
void MakeMacExpertArrayCodes(Params *pParams);

int myPrintLastBlock(Params *pParams);

void PrintToken(Token *pToken, char cCarattereIniziale, char cCarattereFinale, int bPrintACapo);

int PrintThisObject(Params *pParams, int objNum, int bDecodeStream, int nPageNumber, FILE* fpErrors);

int PrintThisObjectFontFile(Params *pParams, int objNum, FILE* fpOutput);
int LoadFirstBlock(Params *pParams, int objNum, const char *pszFunctionName);

void myTreeTraversePostOrderLeafOnly(Tree *head, Params *pParams);

int ParseInfoObject(Params *pParams, int objNum);
int ParseEncryptObject(Params *pParams, int objNum);
int ParseObject(Params *pParams, int objNum);
int ParseNextObject(Params *pParams, int objNum);
int CheckObjectType(Params *pParams, int objNum);
int getObjStmDataFromDecodedStream(Params *pParams);
int ParseStmObj(Params *pParams, int objNum);
int ParseTrailerXRefStreamObject(Params *pParams);
int ParseStreamObject(Params *pParams, int objNum);
int ParseStreamXObject(Params *pParams, int objNum);
int ParseCMapStream(Params *pParams, int objNum, unsigned char *pszDecodedStream, unsigned long int DecodedStreamSize);
int ParseToUnicodeStream(Params *pParams, int objNum, unsigned char *pszDecodedStream, unsigned long int DecodedStreamSize);
int ParseCMapObject(Params *pParams, int objNum);
int ParseGsObject(Params *pParams, int objNum);
int ParseFontWidthsArray(Params *pParams, int objNum);
int ParseFontDescriptorObject(Params *pParams, int objNum);
int ParseFontObject(Params *pParams, int objNum);
int ParseCIDFontObject(Params *pParams, int objNum);
int ParseEncodingObject(Params *pParams, int objNum);
int ParseDictionaryObject(Params *pParams, int objNum);
int ParseLengthObject(Params *pParams, int objNum);
int ParseIntegerObject(Params *pParams, int objNum);

int PushXObjDecodedContent(Params *pParams, int nPageNumber, int nXObjNumber);
double getCurrCharWidth(Params *pParams, wchar_t c, wchar_t cEncoding);

//int ManageShowTextOperator(Params *pParams, const char *szOpName, wchar_t *pszString, size_t lenString, wchar_t *pszEncodingString);
int ManageShowTextOperator(Params *pParams, const char *szOpName,
                           wchar_t *pszString, size_t lenString,
                           wchar_t *pszEncodingString,
                           const unsigned char *pRawBytes,
                           size_t rawBytesLen);

int ManageDecodedContentText(Params *pParams, int nPageNumber);
int ManageContent(Params *pParams, int nPageNumber);
int ManageExtractedText(Params *pParams, int bPrint, int bSearch);

int getObjsOffsets(Params *pParams, char *pszFileName); // preparse file

int Parse(Params *pParams, FilesList* myFilesList);

int match(Params *pParams, TokenTypeEnum ExpectedToken, char *pszFunctionName);
int matchLengthObj(Params *pParams, TokenTypeEnum ExpectedToken, char *pszFunctionName);


// void Panic();


int pdf(Params *pParams);

int obj(Params *pParams);
int objbody(Params *pParams);
int objitem(Params *pParams);
int array(Params *pParams);
int dictionary(Params *pParams);

int xrefstream_obj(Params *pParams);
int xrefstream_objbody(Params *pParams);
int xrefstream_streamdictitems(Params *pParams);
int xrefstream_keyvalue(Params *pParams);
int xrefstream_keyarray(Params *pParams);
int xrefstream_keydict(Params *pParams);
int xrefstream_keyvalueinternal(Params *pParams);
int xrefstream_keyarrayinternal(Params *pParams);

int stmobj(Params *pParams);
int stmobjbody(Params *pParams);
int stmobjstreamdictitems(Params *pParams);
int stmobjkeyvalue(Params *pParams);
int stmobjkeyarray(Params *pParams);
int stmobjkeydict(Params *pParams);
int stmobjkeyvalueinternal(Params *pParams);
int stmobjkeyarrayinternal(Params *pParams);

int prepagetree(Params *pParams);
int pagetree(Params *pParams);
int pagetreebody(Params *pParams);
int pagetreeitems(Params *pParams);
int pagetreeobj(Params *pParams);
int pagetreearrayobjs(Params *pParams);
int pagetreedictobjs(Params *pParams);

int contentobj(Params *pParams);
int contentobjbody(Params *pParams);
int streamdictitems(Params *pParams);
int contentkeyvalue(Params *pParams);
int contentkeyarray(Params *pParams);
int contentkeydict(Params *pParams);
int contentkeyvalueinternal(Params *pParams);
int contentkeyarrayinternal(Params *pParams);

int lengthobj(Params *pParams);
int integer_obj(Params *pParams);

int resourcesdictionary(Params *pParams);
int resourcesdictionarybody(Params *pParams);
int resourcesdictionaryitems(Params *pParams);
int resourcesarrayobjs(Params *pParams);
int resourcesdictobjs(Params *pParams);

int contentxobj(Params *pParams);
int contentxobjbody(Params *pParams);
int xobjstreamdictitems(Params *pParams);
int xobjcontentkeyvalue(Params *pParams);
int xobjcontentkeyarray(Params *pParams);
int xobjcontentkeydict(Params *pParams);

int gsobj(Params *pParams);
int gsobjbody(Params *pParams);

int widthsarrayobj(Params *pParams);
int widthsarrayobjbody(Params *pParams);


int encryptobj(Params *pParams);
int encryptobjbody(Params *pParams);
int encryptobjdictitems(Params *pParams);
int encryptobjkeyvalue(Params *pParams);
int encryptobjkeydict(Params *pParams);


int fontdescriptorobj(Params *pParams);
int fontdescriptorobjbody(Params *pParams);
int fontdescriptorobjbodydictitems(Params *pParams);
int fontdescriptorobjkeyvalue(Params *pParams);
int fontdescriptorobjkeyarray(Params *pParams);
int fontdescriptorobjkeydict(Params *pParams);

int contentfontobj(Params *pParams);
int contentfontobjbody(Params *pParams);
int fontobjstreamdictitems(Params *pParams);
int fontobjcontentkeyvalue(Params *pParams);
int fontdirectencodingobjarray(Params *pParams);
int fontobjcontentkeyarray(Params *pParams);
int fontobjcontentkeydict(Params *pParams);

int cidfontobj(Params *pParams);
int cidfontobjbody(Params *pParams);
int cidfontobjdictitems(Params *pParams);
int cidfontobjkeyvalue(Params *pParams);
int cidfontobjkeyarray(Params *pParams);
int cidfontobjkeydict(Params *pParams);

int encodingobj(Params *pParams);
int encodingobjbody(Params *pParams);
int encodingobjdictitems(Params *pParams);
int encodingobjdictitemskeyvalues(Params *pParams);
int encodingobjarray(Params *pParams);

int cot(Params *pParams);
int cot_dictbody(Params *pParams);
int cot_dictbodyitems(Params *pParams);

#endif /* __MYPARSER__ */
