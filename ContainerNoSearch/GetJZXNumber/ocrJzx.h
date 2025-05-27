#include "include\tesseract\baseapi.h" 
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

public:
	OcrJzx();
	~OcrJzx();
	void OcrJzx::recognizeInfo(string outPath);
	Mat OcrJzx::getWarpImage(Mat img, RotatedRect rect, int diffx = 0, int diffy = 0);
	//String OcrJzx::ocrProcess(Mat in, int idx, RotatedRect box, int type); 
	String OcrJzx::readText(Mat img, int type);
	String OcrJzx::readString(Mat img, RotatedRect inBox, int charNum);
};