#include"hazeremove.h"
#include"stdlib.h"
#include"math.h"
#define uiNR_OF_GREY (256)
#define eps 0.000001
#define bimax(a,b) (a>b)?a:b
#define bimin(a,b) (a<b)?a:b
#define trimin(a,b,c) (a<b)?((a<c)?a:c):(b<c)?b:c
#define trimax(a,b,c) (a>b)?((a>c)?a:c):(b>c)?b:c
//����һЩ����

static void ClipHistogram(unsigned long*, unsigned int, unsigned long);
static void MakeHistogram(unsigned char*, unsigned int, unsigned int, unsigned int,
	unsigned long*, unsigned int, unsigned char*, unsigned int);
static void MapHistogram(unsigned long*, unsigned char, unsigned char,
	unsigned int, unsigned long);
static void MakeLut(unsigned char*, unsigned char, unsigned char, unsigned int);
static void Interpolate(unsigned char*, int, unsigned long*, unsigned long*,
	unsigned long*, unsigned long*, unsigned int, unsigned int, unsigned char*);

const unsigned int uiMAX_REG_X = 16;      
const unsigned int uiMAX_REG_Y = 16;      
//������
int region_meanlight[256] = { 0 };
//����ƽ��������
unsigned int meanlight = 0;
//ƽ������
unsigned long heqlight = 100000000;
//CLAHE�Ҷ�����
const float alpha = 0.4;
const float vmax = 1 - exp(-alpha);
const int divtable[] = { 65535, 32768, 16384, 10922, 8192, 6553, 5461, 4681, 4096, 3640, 3276, 2978, 2730, 2520, 2340, 2184, 2048, 1927, 1820, 1724, 1638, 1560, 1489, 1424, 1365, 1310, 1260, 1213, 1170, 1129, 1092, 1057, 1024, 992, 963, 936, 910, 885, 862, 840, 819, 799, 780, 762, 744, 728, 712, 697, 682, 668, 655, 642, 630, 618, 606, 595, 585, 574, 564, 555, 546, 537, 528, 520, 512, 504, 496, 489, 481, 474, 468, 461, 455, 448, 442, 436, 431, 425, 420, 414, 409, 404, 399, 394, 390, 385, 381, 376, 372, 368, 364, 360, 356, 352, 348, 344, 341, 337, 334, 330, 327, 324, 321, 318, 315, 312, 309, 306, 303, 300, 297, 295, 292, 289, 287, 284, 282, 280, 277, 275, 273, 270, 268, 266, 264, 262, 260, 258, 256, 254, 252, 250, 248, 246, 244, 242, 240, 239, 237, 235, 234, 232, 230, 229, 227, 225, 224, 222, 221, 219, 218, 217, 215, 214, 212, 211, 210, 208, 207, 206, 204, 203, 202, 201, 199, 198, 197, 196, 195, 193, 192, 191, 190, 189, 188, 187, 186, 185, 184, 183, 182, 181, 180, 179, 178, 177, 176, 175, 174, 173, 172, 171, 170, 169, 168, 168, 167, 166, 165, 164, 163, 163, 162, 161, 160, 159, 159, 158, 157, 156, 156, 155, 154, 153, 153, 152, 151, 151, 150, 149, 148, 148, 147, 146, 146, 145, 144, 144, 143, 143, 142, 141, 141, 140, 140, 139, 138, 138, 137, 137, 136, 135, 135, 134, 134, 133, 133, 132, 132, 131, 131, 130, 130, 129, 129, 128, 128 };


int CLAHE1(unsigned char* pImage, unsigned int uiXRes, unsigned int uiYRes,
	unsigned char Min, unsigned char Max, unsigned int uiNrX, unsigned int uiNrY,
	unsigned int uiNrBins, float fCliplimit)
	/*   pImage - ����ͼ��ָ��
	*   uiXRes - ��
	*   uiYRes - ��
	*   Min - ��ͻҶȼ�0
	*   Max - ��߻Ҷȼ�255
	*   uiNrX - ��������� �㷨����padding
	*   uiNrY - ��������� �㷨����padding����Ҫ����Ϊ��������
	*   uiNrBins - ֱ��ͼ����/256Ĭ�ϣ���Ȼ���Ū128�ٶȸ���,����Ч��Ҳ�ܺõġ�
	*   float fCliplimit - ���򻯵Ľض����ޣ�1��Ϊ�޶Աȶ����Ƶ�����Ӧֱ��ͼ���⣬һ�����0.01����
	*/
{
	heqlight = 0;
	unsigned int uiX, uiY;          /* ��������� */
	unsigned int uiXSize, uiYSize, uiSubX, uiSubY; /* �����С �������� */
	unsigned int uiXL, uiXR, uiYU, uiYB;  /* ��ֵ */
	unsigned long ulClipLimit, ulNrPixels;/* �ü��ޣ���������� */
	unsigned char* pImPointer;           /* ͼ������ */
	unsigned char aLUT[uiNR_OF_GREY];        /* ���ұ��� */
	unsigned long* pulHist, *pulMapArray; /* ֱ��ͼ��ӳ���ָ��*/
	unsigned long* pulLU, *pulLB, *pulRU, *pulRB; /* ��ֵָ�� */

	if (uiNrX > uiMAX_REG_X) return -1;       /* ����� */
	if (uiNrY > uiMAX_REG_Y) return -2;       /* ����� */
	if (uiXRes % uiNrX) return -3;      /* ����Ϊ������ */
	if (uiYRes & uiNrY) return -4;      /* ���� */
	if (Max >= uiNR_OF_GREY) return -5;       /* ���Ҷ�̫�� */
	if (Min >= Max) return -6;          /* ��С�Ҷ����� */
	if (uiNrX < 2 || uiNrY < 2) return -7;/* ������Ҫ4������ */
	if (fCliplimit == 1.0) return 0;      /* ����ԭͼ���� */
	if (uiNrBins == 0) uiNrBins = 128;      /* Ĭ��128 */

	pulMapArray = (unsigned long *)malloc(sizeof(unsigned long)*uiNrX*uiNrY*uiNrBins); /* �����������ڴ� ֱ��ͼ */
	if (pulMapArray == 0) return -8;      /* �ڴ治�㣿��ô���� */

	uiXSize = uiXRes / uiNrX; uiYSize = uiYRes / uiNrY;  /* �����С���� */
	ulNrPixels = (unsigned long)uiXSize * (unsigned long)uiYSize;

	if (fCliplimit > 0.0) {          /* Calculate actual cliplimit     */
		ulClipLimit = (unsigned long)(fCliplimit * (uiXSize * uiYSize)+uiXSize * uiYSize / uiNrBins);
		ulClipLimit = (ulClipLimit < 1UL) ? 1UL : ulClipLimit;
	}
	else ulClipLimit = 1UL << 14;          /* ��ֱ��ͼ���� */
	MakeLut(aLUT, Min, Max, uiNrBins);      /* ����LUT */
	/* ����ÿ�������ӳ��� */
	for (uiY = 0, pImPointer = pImage; uiY < uiNrY; uiY++) {
		for (uiX = 0; uiX < uiNrX; uiX++, pImPointer += uiXSize*3) {
			pulHist = &pulMapArray[uiNrBins * (uiY * uiNrX + uiX)];//��������
			MakeHistogram(pImPointer, uiXRes, uiXSize, uiYSize, pulHist, uiNrBins, aLUT, uiX + uiY*uiNrX);
			ClipHistogram(pulHist, uiNrBins, ulClipLimit);
			MapHistogram(pulHist, Min, Max, uiNrBins, ulNrPixels);
		}
		pImPointer += (uiYSize - 1) * uiXRes*3;          /* ����һ�� ����RGB���Գ���3 */
	}

	/* ��ֵ */
	for (pImPointer = pImage, uiY = 0; uiY <= uiNrY; uiY++) {
		if (uiY == 0) {                      /* �������һ�� */
			uiSubY = uiYSize >> 1;  uiYU = 0; uiYB = 0;
		}
		else {
			if (uiY == uiNrY) {                  /* �������һ�� */
				uiSubY = uiYSize >> 1;    uiYU = uiNrY - 1;     uiYB = uiYU;
			}
			else {                      /* Ĭ�� */
				uiSubY = uiYSize; uiYU = uiY - 1; uiYB = uiYU + 1;
			}
		}
		for (uiX = 0; uiX <= uiNrX; uiX++) {
			if (uiX == 0) {                  /* ��������� */
				uiSubX = uiXSize >> 1; uiXL = 0; uiXR = 0;
			}
			else {
				if (uiX == uiNrX) {              /* ��������� */
					uiSubX = uiXSize >> 1;  uiXL = uiNrX - 1; uiXR = uiXL;
				}
				else {                      /* Ĭ�� */
					uiSubX = uiXSize; uiXL = uiX - 1; uiXR = uiXL + 1;
				}
			}

			pulLU = &pulMapArray[uiNrBins * (uiYU * uiNrX + uiXL)];
			pulRU = &pulMapArray[uiNrBins * (uiYU * uiNrX + uiXR)];
			pulLB = &pulMapArray[uiNrBins * (uiYB * uiNrX + uiXL)];
			pulRB = &pulMapArray[uiNrBins * (uiYB * uiNrX + uiXR)];
			Interpolate(pImPointer, uiXRes, pulLU, pulRU, pulLB, pulRB, uiSubX, uiSubY, aLUT);
			pImPointer += uiSubX*3;              /* �����µĿ� */
		}
		pImPointer += (uiSubY - 1) * uiXRes*3;
	}
	free(pulMapArray);                      /* ֱ��ͼ�ڴ��ͷ� */
	return 0;                          /* ��� */
}
void ClipHistogram(unsigned long* pulHistogram, unsigned int
	uiNrGreylevels, unsigned long ulClipLimit)
	/* 
	*����ֱ��ͼ
	*/
{
	unsigned long* pulBinPointer, *pulEndPointer, *pulHisto;
	unsigned long ulNrExcess, ulUpper, ulBinIncr, ulStepSize, i;
	long lBinExcess;

	ulNrExcess = 0;  pulBinPointer = pulHistogram;
	for (i = 0; i < uiNrGreylevels; i++) { /* ���㳬Խ������ */
		lBinExcess = (long)pulBinPointer[i] - (long)ulClipLimit;
		if (lBinExcess > 0) ulNrExcess += lBinExcess;      /* ���������� */
	};

	/* �ط��� */
	ulBinIncr = ulNrExcess / uiNrGreylevels;          /* ƽ����Խ�� */
	ulUpper = ulClipLimit - ulBinIncr;     /* ����ı�ƽ�� */

	for (i = 0; i < uiNrGreylevels; i++) {
		if (pulHistogram[i] > ulClipLimit) pulHistogram[i] = ulClipLimit; /*���� */
		else {
			if (pulHistogram[i] > ulUpper) {        /* �ߵ����ӵ� */
				ulNrExcess -= pulHistogram[i] - ulUpper; pulHistogram[i] = ulClipLimit;
			}
			else {                    /* �͵����ӵ� */
				ulNrExcess -= ulBinIncr; pulHistogram[i] += ulBinIncr;
			}
		}
	}

	while (ulNrExcess) {   /* �ط���  */
		pulEndPointer = &pulHistogram[uiNrGreylevels]; pulHisto = pulHistogram;

		while (ulNrExcess && pulHisto < pulEndPointer) {
			ulStepSize = uiNrGreylevels / ulNrExcess;
			if (ulStepSize < 1) ulStepSize = 1;          /* ��С����1 */
			for (pulBinPointer = pulHisto; pulBinPointer < pulEndPointer && ulNrExcess;
				pulBinPointer += ulStepSize) {
				if (*pulBinPointer < ulClipLimit) {
					(*pulBinPointer)++;     ulNrExcess--;      /* ��excess */
				}
			}
			pulHisto++;          /* �ط��� */
		}
	}





}
void MakeHistogram(unsigned char* pImage, unsigned int uiXRes,
	unsigned int uiSizeX, unsigned int uiSizeY,
	unsigned long* pulHistogram,
	unsigned int uiNrGreylevels, unsigned char* pLookupTable,
	unsigned int countforrg)
	/* 
	*����ֱ��ͼ����
	*/
{
	unsigned char* pImagePointer;

	unsigned char* ptempPointer = pImage;
	unsigned int i;
	long lightsum = 0;
	for (i = 0; i < uiNrGreylevels; i++) pulHistogram[i] = 0L; /* ���ֱ��ͼ */

	for (i = 0; i < uiSizeY; i++) {
		pImagePointer = &pImage[3*uiSizeX];
		while (pImage < pImagePointer) pulHistogram[pLookupTable[*pImage++]]++;

		pImagePointer += 3*uiXRes;
		pImage = &pImagePointer[-3*uiSizeX];
	}


	for (i = 0; i < uiSizeY; i++) {
		pImagePointer = &ptempPointer[3 * uiSizeX];
		while (ptempPointer < pImagePointer) {
			lightsum += (*ptempPointer * 77 + *(ptempPointer + 1) * 151 + *(ptempPointer + 2) * 28) /256;
			ptempPointer += 3;
		}

		pImagePointer += 3 * uiXRes;
		ptempPointer = &pImagePointer[-3 * uiSizeX];
	}
	region_meanlight[countforrg] = lightsum / (uiSizeX*uiSizeY);
	



	for (i = 0; i < uiNrGreylevels; i++) {
		pulHistogram[i] = pulHistogram[i] / 3+1;
		
	}
		

}

void MapHistogram(unsigned long* pulHistogram, unsigned char Min, unsigned char Max,
	unsigned int uiNrGreylevels, unsigned long ulNrOfPixels)
	/* 
	* �µĲ��ұ� ���������ֲ�����ǿ�Աȹ������
	*/
{
	unsigned long uMax = unsigned long(Max);
	unsigned int i;  unsigned long ulSum = 0;
	const float fScale = ((float)(Max - Min)) / ulNrOfPixels;
	const unsigned long ulMin = (unsigned long)Min;
	unsigned int valspeed = Max - Min;
	unsigned long HistSum[256] = { 0 };
	float val[256] = { 0.0 };
	float temp[256] = { 0.0 };

	unsigned long zzz = 0;
	for (int ii = 0; ii < 255; ++ii){
		zzz = pulHistogram[ii];
	}

	int ta = 0;
	float tb = 0.0;
	for (i = 0; i < uiNrGreylevels; i++) {
		unsigned long tempcount = pulHistogram[i];
		HistSum[i] = 0;
		HistSum[i] += (i == 0) ? pulHistogram[i] : HistSum[i - 1] + pulHistogram[i];
		ta = HistSum[i];
		val[i] = float(vmax)*float(HistSum[i]) / float(ulNrOfPixels);
		tb = float(vmax)*float(HistSum[i]);
		if (val[i] >= 1.0)
			val[i] = 1.0 - eps;
		temp[i] = -1 / alpha*log(1 - val[i]);
		
		pulHistogram[i] = (unsigned long)(temp[i]*float(valspeed));
		
		if (pulHistogram[i] > uMax) pulHistogram[i] = uMax;
		
	}






}

void MakeLut(unsigned char * pLUT, unsigned char Min, unsigned char Max, unsigned int uiNrBins)
/* 
* [0,uiNrBins-1]. ��ô�����LUT
*/
{
	int i;
	const unsigned char BinSize = (unsigned char)(1 + (Max - Min) / uiNrBins);
	
	for (i = Min; i <= Max; i++)  { 
		pLUT[i] = (i - Min) / BinSize;
		
	}
}

void Interpolate(unsigned char * pImage, int uiXRes, unsigned long * pulMapLU,
	unsigned long * pulMapRU, unsigned long * pulMapLB, unsigned long * pulMapRB,
	unsigned int uiXSize, unsigned int uiYSize, unsigned char * pLUT)
	/* pImage      �������ͼ��ָ��
	* uiXRes      - x�ֱ���
	* pulMap*     - ӳ���
	* uiXSize     - �ӿ�x�ֱ���
	* uiYSize     - �ӿ�y�ֱ���
	* pLUT           - �Ҷ�ͼ���ұ�
	*˫���Բ�ֵ���� ��Matlab��U UL L ������ֵ��΢��ͬ
	*/
{
	const unsigned int uiIncr = (uiXRes - uiXSize)*3; /* ָ��������*/
	unsigned char GreyValue; unsigned int uiNum = uiXSize*uiYSize; /* ��һ��ϵ�� */
	unsigned char tempr, tempg, tempb = 0;
	unsigned int uiXCoef, uiYCoef, uiXInvCoef, uiYInvCoef, uiShift = 0;

	if (uiNum & (uiNum - 1))   /* ��2��������� */
		for (uiYCoef = 0, uiYInvCoef = uiYSize; uiYCoef < uiYSize;
			uiYCoef++, uiYInvCoef--, pImage += uiIncr) {
		for (uiXCoef = 0, uiXInvCoef = uiXSize; uiXCoef < uiXSize;
			uiXCoef++, uiXInvCoef--) {
			GreyValue = pLUT[*pImage];          
			*pImage = (unsigned char)((uiYInvCoef * (uiXInvCoef*pulMapLU[GreyValue]
				+ uiXCoef * pulMapRU[GreyValue])
				+ uiYCoef * (uiXInvCoef * pulMapLB[GreyValue]
				+ uiXCoef * pulMapRB[GreyValue])) / uiNum);
			tempr = *pImage;
			++pImage;
			GreyValue = pLUT[*pImage];
			*pImage = (unsigned char)((uiYInvCoef * (uiXInvCoef*pulMapLU[GreyValue]
				+ uiXCoef * pulMapRU[GreyValue])
				+ uiYCoef * (uiXInvCoef * pulMapLB[GreyValue]
				+ uiXCoef * pulMapRB[GreyValue])) / uiNum);
			tempg = *pImage;
			++pImage;
			GreyValue = pLUT[*pImage];
			*pImage = (unsigned char)((uiYInvCoef * (uiXInvCoef*pulMapLU[GreyValue]
				+ uiXCoef * pulMapRU[GreyValue])
				+ uiYCoef * (uiXInvCoef * pulMapLB[GreyValue]
				+ uiXCoef * pulMapRB[GreyValue])) / uiNum);
			tempb = *pImage;
			++pImage;
			heqlight += (tempr * 77 + tempg * 151 + tempb * 28) >> 8;
		}
	}
	else {               /* ��λ������� */
		while (uiNum >>= 1) uiShift++;           
		for (uiYCoef = 0, uiYInvCoef = uiYSize; uiYCoef < uiYSize;
			uiYCoef++, uiYInvCoef--, pImage += uiIncr) {
			for (uiXCoef = 0, uiXInvCoef = uiXSize; uiXCoef < uiXSize;
				uiXCoef++, uiXInvCoef--) {
				GreyValue = pLUT[*pImage];      /* �õ�ֱ��ͼ�� */
				*pImage = (unsigned char)((uiYInvCoef* (uiXInvCoef * pulMapLU[GreyValue]
					+ uiXCoef * pulMapRU[GreyValue])
					+ uiYCoef * (uiXInvCoef * pulMapLB[GreyValue]
					+ uiXCoef * pulMapRB[GreyValue])) >> uiShift);
				tempr = *pImage;
				++pImage;
				GreyValue = pLUT[*pImage];
				*pImage = (unsigned char)((uiYInvCoef* (uiXInvCoef * pulMapLU[GreyValue]
					+ uiXCoef * pulMapRU[GreyValue])
					+ uiYCoef * (uiXInvCoef * pulMapLB[GreyValue]
					+ uiXCoef * pulMapRB[GreyValue])) >> uiShift);
				tempg = *pImage;
				++pImage;
				GreyValue = pLUT[*pImage];
				*pImage = (unsigned char)((uiYInvCoef* (uiXInvCoef * pulMapLU[GreyValue]
					+ uiXCoef * pulMapRU[GreyValue])
					+ uiYCoef * (uiXInvCoef * pulMapLB[GreyValue]
					+ uiXCoef * pulMapRB[GreyValue])) >> uiShift);
				tempb = *pImage;
				++pImage;
				heqlight += (tempr * 77 + tempg * 151 + tempb * 28) >> 8;
			}
		}
	}
};

int DPHR(unsigned char* pImage,unsigned int imageX, unsigned int imageY){
	unsigned int numpixel = imageX*imageY;
	int pI = 0;
	unsigned long sum = 0;
	unsigned long sumgraylight = 0;
	unsigned char *darkchannel;
	unsigned char maxdark = 0;
	unsigned char maxlight = 0;
	darkchannel = (unsigned char *)malloc(sizeof(unsigned char)*numpixel);/*���㰵ͨ��ͼ��*/
	for (pI = 0; pI < numpixel; ++pI){
		darkchannel[pI] = trimin(pImage[pI + pI + pI], pImage[pI + pI + pI + 1], pImage[pI + pI + pI + 2]);
		maxlight = bimax(maxlight, trimax( pImage[pI + pI + pI], pImage[pI + pI + pI + 1], pImage[pI + pI + pI + 2]));
		sumgraylight += (pImage[pI + pI + pI] * 77 + pImage[pI + pI + pI + 1] * 151 + pImage[pI + pI + pI + 2] * 28) >> 8;
		sum += darkchannel[pI];
		maxdark = bimax(darkchannel[pI], maxdark);

	}
	meanlight = sumgraylight / numpixel;
	unsigned char meandark = sum*13 / (numpixel*10);
	unsigned char meanfactor = bimin(230, meandark);
	unsigned int tempavg[8] = { 100 };
	unsigned char Atmospherlight = (maxdark + maxdark) >> 1;/*���������*/
	unsigned short divfactor = divtable[Atmospherlight];
	unsigned char avgdark = 0;
	unsigned char Ltrans = 0;
	unsigned char hftemp[3] = {0};
	unsigned short tempfact=0;
	unsigned long sumlight=0;/*����ͼ��*/
	for (pI = 0; pI < numpixel; ++pI){
		tempavg[0] = tempavg[1];
		tempavg[1] = tempavg[2];
		tempavg[2] = tempavg[3];
		tempavg[3] = tempavg[4];
		tempavg[4] = tempavg[5];
		tempavg[5] = tempavg[6];
		tempavg[6] = tempavg[7];
		tempavg[7] = darkchannel[pI];
		avgdark = (tempavg[0] +
			tempavg[1] +
			tempavg[2] +
			tempavg[3] +
			tempavg[4] +
			tempavg[5] +
			tempavg[6] +
			tempavg[7])>>3;
		Ltrans = bimin((meanfactor*avgdark) >> 8, darkchannel[pI]);
		
	//	tempfact = divtable[(65536 - ((Ltrans*divtable[Atmospherlight]) >> 7))>>8];

		float tempr = float(pImage[pI + pI + pI] - Ltrans) / (1.0 - float(Ltrans) / float(Atmospherlight));
		hftemp[0] = unsigned char(tempr>255.0 ? 255.0 : tempr);
		float tempg = float(pImage[pI + pI + pI+1] - Ltrans) / (1.0 - float(Ltrans) / float(Atmospherlight));

		hftemp[1] = unsigned char(tempg>255.0 ? 255.0 : tempg);
		float tempb = float(pImage[pI + pI + pI+2] - Ltrans) / (1.0 - float(Ltrans) / float(Atmospherlight));

		hftemp[2] = unsigned char(tempb>255.0 ? 255.0 : tempb);


		/*
		*ʵ��ʵ���в���Ч����ʾhftemp[0] = unsigned char(float(pImage[pI + pI + pI] - Ltrans) ֱ�Ӽ�ȥȻ������Խ��Ӱ�첻��
		*���Կ��԰�������ķ���ȥ��Ч��һ���õ�,���ҿ���ʡ�����ټ������
		*�������ж�����Ϊû�б��ʹ�����Ȼ���ﲻ�ᱥ��,�����Ǹ�����Ļ��㱨��
		*/

		//hftemp[0] = unsigned char(pImage[pI + pI + pI] - Ltrans) ;
		//hftemp[1] = unsigned char(pImage[pI + pI + pI + 1] - Ltrans) ;
		//hftemp[2] = unsigned char(pImage[pI + pI + pI + 2] - Ltrans) ;


		//float tempr = float(pImage[pI + pI + pI] - Ltrans);
		//hftemp[0] = unsigned char(tempr>255.0 ? 255.0 : tempr);
		//float tempg = float(pImage[pI + pI + pI + 1] - Ltrans);

		//hftemp[1] = unsigned char(tempg>255.0 ? 255.0 : tempg);
		//float tempb = float(pImage[pI + pI + pI + 2] - Ltrans) ;

		//hftemp[2] = unsigned char(tempb>255.0 ? 255.0 : tempb);


		sumlight += (hftemp[0] * 77 + hftemp[1] * 151 + hftemp[2] * 28) >> 8;
	}
	float relightfactor = float(bimin(heqlight / numpixel, 160)) / (float(sumlight) / float(numpixel));
		/*����У��*/

	unsigned short relight = relightfactor * 256;
	for (pI = 0; pI < numpixel; ++pI){
		tempavg[0] = tempavg[1];
		tempavg[1] = tempavg[2];
		tempavg[2] = tempavg[3];
		tempavg[3] = tempavg[4];
		tempavg[4] = tempavg[5];
		tempavg[5] = tempavg[6];
		tempavg[6] = tempavg[7];
		tempavg[7] = darkchannel[pI];
		avgdark = (tempavg[0] +
			tempavg[1] +
			tempavg[2] +
			tempavg[3] +
			tempavg[4] +
			tempavg[5] +
			tempavg[6] +
			tempavg[7]) >> 3;
		Ltrans = bimin((meanfactor*avgdark) >> 8, darkchannel[pI]);

//		tempfact = divtable[(65536 - ((Ltrans*divtable[Atmospherlight]) >> 7))>>8];

		float tempr = relightfactor* float(pImage[pI + pI + pI] - Ltrans) / (1.0 - float(Ltrans) / float(Atmospherlight));
		pImage[pI + pI + pI] = unsigned char(tempr>255.0 ? 255.0 : tempr);
		float tempg = relightfactor*float(pImage[pI + pI + pI + 1] - Ltrans) / (1.0 - float(Ltrans) / float(Atmospherlight));
		pImage[pI + pI + pI+1] = unsigned char(tempg>255.0 ? 255.0 : tempg);
		float tempb = relightfactor*float(pImage[pI + pI + pI + 2] - Ltrans) / (1.0 - float(Ltrans) / float(Atmospherlight));
		pImage[pI + pI + pI+2] = unsigned char(tempb>255.0 ? 255.0 : tempb);


		//float tempr = relightfactor* float(pImage[pI + pI + pI] - Ltrans) ;
		//pImage[pI + pI + pI] = unsigned char(tempr>255.0 ? 255.0 : tempr);
		//float tempg = relightfactor*float(pImage[pI + pI + pI + 1] - Ltrans) ;
		//pImage[pI + pI + pI + 1] = unsigned char(tempg>255.0 ? 255.0 : tempg);
		//float tempb = relightfactor*float(pImage[pI + pI + pI + 2] - Ltrans);
		//pImage[pI + pI + pI + 2] = unsigned char(tempb>255.0 ? 255.0 : tempb);



		
	}
	free(darkchannel);
	return 0;
};

int AHR(unsigned char* pImage,  unsigned int uiXRes, unsigned int uiYRes, unsigned char Min,
	unsigned char Max, unsigned int uiNrX, unsigned int uiNrY,
	unsigned int uiNrBins, float fCliplimit){
	unsigned char *climg;
	unsigned char *dpimg;
	climg = (unsigned char *)malloc(sizeof(unsigned char)*uiXRes*uiYRes*3);
	dpimg = (unsigned char *)malloc(sizeof(unsigned char)*uiXRes*uiYRes * 3);
	int pt = 0;
	for (pt = 0; pt < uiXRes*uiYRes * 3; ++pt)
	{
		climg[pt] = pImage[pt];
		dpimg[pt] = pImage[pt];
	}
	CLAHE1(climg, uiXRes, uiYRes, Min, Max, uiNrX, uiNrY, uiNrBins, fCliplimit);
	DPHR(dpimg, uiXRes, uiYRes);
	int fac = 0;
	int avglight = 0, stdl = 0;;
	for (fac = 0; fac < uiNrX*uiNrY; ++fac){
		avglight += region_meanlight[fac];
	}

	avglight = avglight / (uiNrX*uiNrY);
	for (fac = 0; fac < uiNrX*uiNrY; ++fac){
		stdl += (region_meanlight[fac] - avglight)* (region_meanlight[fac] - avglight);
	}
	stdl = stdl / (uiNrX*uiNrY);
	float stdli = sqrt(float(stdl));
	stdli += 9.5*float(heqlight*heqlight) / float(uiXRes* uiYRes*uiXRes* uiYRes) / 6400.0;
	if (stdli > 70.0)
		stdli = 70.0;
	if (stdli < 5.0)
		stdli = 5.0;
	float facA, facB;
	facA = stdli*stdli / 4900 * 4.0 + 2.0;
	facB = 6.0-stdli*stdli / 4900 * 4.0;
	facA = facA * 1024.0;
	facB = facB * 1024.0;

	int intfacA = facA;
	int intfacB = facB;

	for (pt = 0; pt < uiXRes*uiYRes * 3; ++pt)
	{
		pImage[pt] = (climg[pt]*intfacA+dpimg[pt]*intfacB)>>13;
	}
	free(climg);
	free(dpimg);
	return 0;
};