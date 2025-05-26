
class FindContent
{
	public:

		int debug_level=0;
		Mat mGray;
		float mUheight;
		float mUwidth;
		RotatedRect mBox_u;
		RotatedRect mBox_u_all; //和U粘连在一起轮廓
		float mBox_k;
		LineInfo lineInfoOwner;
		LineInfo lineInfoNo;
		LineInfo lineInfoNo1;
		LineInfo lineInfoCheckDigit;
		RotatedRect box_owner;
		RotatedRect box_number;
		RotatedRect box_checkdigit;
		bool isBlackHat = false;
	private:
		//私有方法
	public:
		FindContent();
		~FindContent();
		bool FindContent::search();
		bool FindContent::setTargetRegion(int type, Rect* rect, RotatedRect* box, float k, float b, int rows, int cols);
		int FindContent::findOwnerInfo(Rect rect, RotatedRect box, float k, int lastThresh = 0, int orient = 0);
		int FindContent::findNoInfo(Rect rect, RotatedRect box, float k, int lastThresh = 0, int orient = 0);
		int FindContent::findCheckDigitInfo(Rect rect, RotatedRect box, float k, int lastThresh = 0, int orient = 0);
		void FindContent::filterContour(vector<vector<Point>> pContours, LineInfo* lineInfo, int rows, int cols, int orient = 0); //orient:0=横向 1=竖向
		void FindContent::calContourType(LineInfo* lineInfo, int target = 0, int orient = 0); //target:0=all,1=owner orient:0=横向 1=竖向
		void FindContent::rejustOwnerContourType(LineInfo* lineInfo, int orient = 0); //target:0=all,1=owner orient:0=横向 1=竖向
		void FindContent::checkLine(LineInfo* lineInfo, int target = 0, int orient = 0);  //target:0=all,1=owner  orient:0=横向 1=竖向 
		void FindContent::sortLineInfo(LineInfo* line_Info, int pos = 0, int orient = 0);        //orient:0=横向(pos:0=中心 1=左边距 2=右边距) 1=竖向(pos:0=中心 1=上边距 2=下边距)
};

class DetectJzx
{
public:
	int debug_level = 0;
	FindContent findResult;
private:
	//私有方法
public:
	DetectJzx();
	~DetectJzx();
	bool DetectJzx::searchNumber(Mat img, int method = 0);
	bool DetectJzx::findCharU(Mat img, double threshMethod, FindContent* content);
	bool DetectJzx::isCharU(Mat img, boxInfo boxinfo,float* w,float* h);
	Mat DetectJzx::prePare(Mat inputImg, bool doMORPH, bool isBlack, unsigned int val1_1, unsigned int val1_2, int method2, double* val2);
	Mat DetectJzx::OutputImage(Mat img);
};

class OcrJzx
{
	public:
		int debug_level = 0;
		string filepath;
		int orient = 0;
		//Mat mOcr;
		//Mat mOwnerOcr;
		//Mat mNumberOcr;
		//Mat mNumber1Ocr;
		//Mat mCheckdigitOcr;
		//Mat mNumCheckdigitOcr;
		String ownerTxt;
		String numberTxt;
		String checkDigitTxt;
		String ownerTxt1;
		String numberTxt1;
		String checkDigitTxt1;
		string numberTxt2;
		bool isBlackHat = false;
		tesseract::TessBaseAPI* ocrApi;
		FindContent findResult;
	private:
		//私有方法
	public:
		OcrJzx();
		~OcrJzx();
		void OcrJzx::readInfo();
		void OcrJzx::readDatafromLineInfo(Mat imputImg, LineInfo lineInfo, int orient, int chartype, int charNumber, map_ocrResult* map, Mat* out);
		void OcrJzx::readDatafromBox(Mat inputImg, RotatedRect box, int chartype, int charNumber, map_ocrResult* result, Mat* out, int diffx = 20, int diffy = 20, tesseract::PageSegMode mode = tesseract::PSM_SINGLE_BLOCK);
		void OcrJzx::readDatafromImg(Mat ocrImg, int chartype, int charNumber, map_ocrResult* result, int diffx = 20, int diffy = 20, tesseract::PageSegMode mode = tesseract::PSM_SINGLE_BLOCK);
		void OcrJzx::readDataDigit(Mat inputImg, int chartype, int orient, string* text1, string* text2, Mat* out1, Mat* out2);
		String readText1(Mat img, int chartype, int* conf, tesseract::PageSegMode mode = tesseract::PSM_SINGLE_BLOCK);
		string OcrJzx::readTextfromImg(Mat ocrImg, int chartype, int charNumber, int* conf, int diffx = 20, int diffy = 20, tesseract::PageSegMode mode = tesseract::PSM_SINGLE_BLOCK);
		Mat OcrJzx::getWarpImage(Mat img, RotatedRect rect, int diffx = 0, int diffy = 0, bool isThresh = false);
		bool OcrJzx::checkIsblack(Mat img, RotatedRect box, int diffx, int diffy, bool isThresh);

		//void OcrJzx::recognizeInfo();
		//void OcrJzx::readTextfromLineInfo(Mat imputImg, LineInfo lineInfo, int orient, int chartype, Mat* out, string* txt1, string* txt2);
		//void OcrJzx::readTextfromBox(Mat inputImg, RotatedRect box, int diffx, int diffy, int chartype, int* conf, string* text, Mat* out, bool isThresh = false);
		//string OcrJzx::readTextfromImg(Mat ocrImg, int chartype, int* conf, int diffx = 20, int diffy = 20, bool isThresh = false, tesseract::PageSegMode mode = tesseract::PSM_SINGLE_BLOCK);
		//void OcrJzx::readCheckdigit(Mat inputImg, int chartype, string* text1, string* text2, Mat* out1,Mat* out2);
		//void OcrJzx::recognizeOwner(Mat threshImg);
		//void OcrJzx::recognizeNumber(Mat threshImg);
		//String readText(Mat img, int chartype);
		//void OcrJzx::doOcr(Mat imputImg, LineInfo lineInfo, int orient, int chartype, Mat* out, string* txt1, string* txt2);
		//void OcrJzx::recognizeCheckdigit(Mat threshImg, Mat numImage, Mat* allNumberImage, string* txt1, string* txt2);
		//String readText2(Mat img, int chartype, int* conf);
		//String ocrProcess(Mat in, int idx, RotatedRect box, int type, string filename); //type: 0=字母 1=数字 filename(输出文件用)

};