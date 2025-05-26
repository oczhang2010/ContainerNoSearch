#include "include\tesseract\baseapi.h"  //注意这行要放在前面一下，不然tesscallback.h中会出错！
//tesseract编译参考https://blog.csdn.net/zhulingling329/article/details/53909637
#include "include\leptonica\allheaders.h"
#include "findContent.h"

class OcrJzx
{
public:
	int debug_level;
	Mat mOcr;
	//RotatedRect box_owner;
	//RotatedRect box_number;
	//RotatedRect box_checkdigit;
	String ownerTxt;
	String numberTxt;
	String checkDigitTxt;
	tesseract::TessBaseAPI* ocrApi;
	FindContent findResult;
	/*

	*/
private:
	//私有方法
public:
	OcrJzx();
	~OcrJzx();
	void OcrJzx::recognizeInfo(string outPath);
	Mat OcrJzx::getWarpImage(Mat img, RotatedRect rect, int diffx = 0, int diffy = 0);
	//String OcrJzx::ocrProcess(Mat in, int idx, RotatedRect box, int type); //type: 0=字母 1=数字
	String OcrJzx::readText(Mat img, int type);
	String OcrJzx::readString(Mat img, RotatedRect inBox, int charNum);
};