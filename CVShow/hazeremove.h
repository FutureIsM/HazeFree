#ifndef HazeREMOVE_H
#define HazeREMOVE_H
//�㷨��Ҫһ��uchar��������ͼ����Ϊ���룬RGB���У�����μ�����ʹ��
int AHR(unsigned char* pImage, unsigned int uiXRes, unsigned int uiYRes, unsigned char Min,
	unsigned char Max, unsigned int uiNrX, unsigned int uiNrY,
	unsigned int uiNrBins, float fCliplimit);
	/*	�ں�ȥ���㷨
	*	pImage - ����ͼ��ָ��
	*   uiXRes - ��
	*   uiYRes - ��
	*   Min - ��ͻҶȼ�0
	*   Max - ��߻Ҷȼ�255
	*   uiNrX - ��������� �㷨����padding
	*   uiNrY - ��������� �㷨����padding����Ҫ����Ϊ��������
	*   uiNrBins - ֱ��ͼ����/256Ĭ�ϣ���Ȼ���Ū128�ٶȸ���,����Ч��Ҳ�ܺõġ�
	*   fCliplimit - ���򻯵Ľض����ޣ�1��Ϊ�޶Աȶ����Ƶ�����Ӧֱ��ͼ���⣬һ�����0.01����
	*/
int CLAHE1(unsigned char* pImage,  unsigned int uiXRes, unsigned int uiYRes, unsigned char Min,
	unsigned char Max, unsigned int uiNrX, unsigned int uiNrY,
	unsigned int uiNrBins, float fCliplimit);
	/*CLAHEȥ���㷨*/
int DPHR(unsigned char* pImage, unsigned int uiXRes, unsigned int uiYRes);
	/*��ͨ��ȥ���㷨*/

#endif