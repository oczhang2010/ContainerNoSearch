#include "include\tesseract\baseapi.h"  
#include "opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2\core\core.hpp"
#include "vector"
#include "include\leptonica\allheaders.h"
#include "cnnmlp.h"
#include "numbercnnmlp.h"



using namespace cv;
using namespace std;


typedef struct _boxInfo{
	int idx;                
	int type;               
	vector<Point> contour;  
	RotatedRect box;        
	float k;                
}
boxInfo;

// line��Ϣ
typedef struct _lineInfo
{
	RotatedRect box;	       
	float k;                   
	float b;                   
	int box_1of2_count = 0;   
	int box_num_count = 0;    
	vector<boxInfo> box_info; 
}
LineInfo;

typedef struct _targetInfo
{
	RotatedRect box;
	CvRect rect;
	float k_value;
	bool isTryMore = false;
	int tryCount = 0;
}
targetInfo;

inline void ClearLineInfo(LineInfo* info)
{
	info = new LineInfo;
	if (info)
	{
		info->k = 0;
		info->b = 0.0;
		info->box_1of2_count = 0;
		info->box_num_count = 0;
		info->box_info.clear();
	}
	info = 0;
}
class Number
{
public:
	string filepath;
	int mImageWidth = 0;
	int mImageHeight = 0;
	int debug_level;
	int tryCount;
	int frameCount;
	Mat mGray;
	Mat mOcr;
	double mThresh;
	double mUheight;
	double mUwidth;
	RotatedRect mBox_u;;
	vector<targetInfo*> vec_targetinfo;
	LineInfo lineInfoOwner;
	LineInfo lineInfoNo;
	LineInfo lineInfoNo1;
	LineInfo lineInfoCheckDigit;
	RotatedRect box_owner;
	RotatedRect box_number;
	RotatedRect box_checkdigit;
	String ownerTxt;
	String numberTxt;
	String checkDigitTxt;
	tesseract::TessBaseAPI* ocrApi;
	NumberCnnMlp* numberMlp;
	/*

	*/
private:

public:
	Number();
	~Number();
	bool Number::searchNumber(Mat img,int method = 0);
	bool Number::findCharU(Mat img, double thresh);
	bool Number::isCharU(boxInfo boxinfo);
	bool Number::setTargetRegion(int type, Rect* rect, RotatedRect* box, float k, float b, int rows, int cols);
	Mat Number::prePare(Mat inputImg, bool doMORPH, bool isBlack, unsigned int val1_1, unsigned int val1_2, int method2, double* val2);
	int Number::findOwnerInfo(Rect rect, RotatedRect box, float k, int lastThresh = 0, int orient = 0);
	int Number::findNoInfo(Rect rect, RotatedRect box, float k, int lastThresh = 0, int orient = 0);
	int Number::findCheckDigitInfo(Rect rect, RotatedRect box, float k, int lastThresh = 0, int orient = 0);
	void Number::filterContour(vector<vector<Point>> pContours, LineInfo* lineInfo, int rows, int cols, int orient = 0); 
	void Number::calContourType(LineInfo* lineInfo, int target = 0, int orient = 0); 
	void Number::checkLine(LineInfo* lineInfo,int target=0, int orient = 0);   
	void Number::CalBoxInfo(LineInfo* lineInfo);
	void Number::CalcRotatedRectPoints(RotatedRect* box, float* k);
	vector<Point2f> Number::orderRotatedRectPoint(RotatedRect rect);
	void Number::CalContourInfo(vector<vector<Point>> contours, LineInfo* lineInfo);
	RotatedRect Number::getRotatedRectFromPoints(vector<Point> pts, float* k);
	Mat Number::CreateMat(Mat src, Rect rect, bool isReturnErr = false);
	void Number::sortBoxInfo(vector<boxInfo>* pBoxinfo, int pos = 0, int orient = 0); 
	void Number::sortLineInfo(LineInfo* line_Info, int pos = 0, int orient=0);        
	void Number::outputInfo(RotatedRect rect, string filename);
	void Number::writePicture();
	float Number::getDistance(CvPoint p0, CvPoint p1);
	Mat Number::warpImage(Mat srcImage, Point2f* srcPts, Point2f* dstPts, int warpWidth, int warpHeight);
	vector<boxInfo> Number::breakContours(boxInfo boxinfo, int minHeight, int target = 1);
	void Number::recognizeInfo();
	Mat Number::getWarpImage(Mat img,RotatedRect rect,int diffx = 0,int diffy = 0);
	String ocrProcess(Mat in, int idx, RotatedRect box,int type); 
	String readText(Mat img, int type);
	String readTextMlp(Mat img, int type);
	void Number::drawRotatedRect(Mat srcImg, RotatedRect box, int thickness = 1, CvScalar color = CV_RGB(255, 255, 255));
	void Number::threshRoi(Mat fromImg, Mat* toImg, RotatedRect box);

};



