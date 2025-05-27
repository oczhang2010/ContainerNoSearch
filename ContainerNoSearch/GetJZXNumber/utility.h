#include "opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2\core\core.hpp"
#include "vector"
#include "map"

using namespace cv;
using namespace std;

typedef struct _targetInfo
{
	Mat mGray;
	RotatedRect box;
}
targetInfo;

typedef struct _boxInfo{
	int idx;                
	int type;               
	vector<Point> contour;  
	RotatedRect box;        
	float k;                
}
boxInfo;

typedef struct _ocrResult{
	int conf;                
	string text;             
}
ocrResult;

typedef map<string, ocrResult> map_ocrResult;

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

void CalBoxInfo(LineInfo* lineInfo);
void CalcRotatedRectPoints(RotatedRect* box, float* k);
void CalContourInfo(vector<vector<Point>> contours, LineInfo* lineInfo);
RotatedRect getRotatedRectFromPoints(vector<Point> pts, float* k);
void sortBoxInfo(vector<boxInfo>* pBoxinfo, int pos = 0, int orient = 0); 
void drawRotatedRect(Mat srcImg, RotatedRect box, int thickness = 1, CvScalar color = CV_RGB(255, 255, 255));
vector<Point2f> orderRotatedRectPoint(RotatedRect rect);
Mat CreateMat(Mat src, Rect rect, bool isReturnErr = false);
float getDistance(CvPoint p0, CvPoint p1);
Mat warpImage(Mat srcImage, Point2f* srcPts, Point2f* dstPts, int warpWidth, int warpHeight);
vector<boxInfo> breakContours(Mat inputImage,boxInfo boxinfo, int uWidth,int minHeight, int target = 1);
bool isBlack(Mat inputImg);
float averageDark(Mat inputImg);
Mat getTransform(RotatedRect rect);
Rect getRectFromBox(RotatedRect inbox);
string removeChar(string instr, string rmchar);
string reverseText(string instr);
Mat jointMat(Mat img1, Mat img2, int diff);
map_ocrResult mergeMap(map_ocrResult map1, map_ocrResult map2);
vector<ocrResult> sortOcrResult(map_ocrResult map, int charCount = 0);
RotatedRect breakBox(RotatedRect inBox, int orient, int charCnt, int idx, float angle, float k, int diffx = 0, int diffy = 0);
void mapAdd(map_ocrResult* map, int conf, string text);



