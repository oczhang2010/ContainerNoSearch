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
	int idx;                //序号
	int type;               //字符个数
	vector<Point> contour;  //轮廓点集合
	RotatedRect box;        //轮廓点的最小外接矩形
	float k;                //斜率
}
boxInfo;

typedef struct _ocrResult{
	int conf;                //评分
	string text;             //ocr结果文字列
}
ocrResult;

typedef map<string, ocrResult> map_ocrResult;

// line信息
typedef struct _lineInfo
{
	RotatedRect box;	      // 外框的最小外接矩形 
	float k;                  // 最小外接矩形的斜率k y=kx+b 
	float b;                  // 最小外接矩形的偏移b y=kx+b 
	int box_1of2_count = 0;   // 中心线上轮廓的数量
	int box_num_count = 0;    // 检出字符数量
	vector<boxInfo> box_info; //
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
void sortBoxInfo(vector<boxInfo>* pBoxinfo, int pos = 0, int orient = 0); //orient:0=横向(pos:0=中心 1=左边距 2=右边距) 1=竖向(pos:0=中心 1=上边距 2=下边距)
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



