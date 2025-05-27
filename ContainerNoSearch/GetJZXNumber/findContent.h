#include "utility.h"
class FindContent
{
public:

	int debug_level;
	Mat mGray;
	double mThresh;
	double mUheight;
	double mUwidth;
	RotatedRect mBox_u;;
	LineInfo lineInfoOwner;
	LineInfo lineInfoNo;
	LineInfo lineInfoNo1;
	LineInfo lineInfoCheckDigit;
	RotatedRect box_owner;
	RotatedRect box_number;
	RotatedRect box_checkdigit;
private:
public:
	FindContent();
	~FindContent();
	bool FindContent::search(Mat img,RotatedRect boxU,float box_k);
	bool FindContent::setTargetRegion(int type, Rect* rect, RotatedRect* box, float k, float b, int rows, int cols);
	int FindContent::findOwnerInfo(Rect rect, RotatedRect box, float k, int lastThresh = 0, int orient = 0);
	int FindContent::findNoInfo(Rect rect, RotatedRect box, float k, int lastThresh = 0, int orient = 0);
	int FindContent::findCheckDigitInfo(Rect rect, RotatedRect box, float k, int lastThresh = 0, int orient = 0);
	void FindContent::filterContour(vector<vector<Point>> pContours, LineInfo* lineInfo, int rows, int cols, int orient = 0); 
	void FindContent::calContourType(LineInfo* lineInfo, int target = 0, int orient = 0); 
	void FindContent::checkLine(LineInfo* lineInfo, int target = 0, int orient = 0);  
	void FindContent::sortLineInfo(LineInfo* line_Info, int pos = 0, int orient = 0);        
};



