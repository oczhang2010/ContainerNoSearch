#include "stdafx.h"
#include "detectJzx.h"

//构造函数
FindContent::FindContent(){}
FindContent::~FindContent(){}

//搜寻箱号详细入口
bool FindContent::search(){
	//mUheight = boxU.size.height;
	//mUwidth = boxU.size.width;
	//正常检出区域设定
	RotatedRect box2 = mBox_u;
	Rect rect2;
	float k = mBox_k;
	Mat img = mGray;
	float b = 0;
	if (!setTargetRegion(11, &rect2, &box2, k, b, img.rows, img.cols)){
		return false;
	}

	//箱号不会太靠近边缘，去掉太靠近边缘的区域：
	if (rect2.x + rect2.width>img.cols - 5 || rect2.x< 5
		|| rect2.y + rect2.height>img.rows - 3 || rect2.y < 3){
		return false;
	}

	//搜索箱号信息
	lineInfoOwner.box_num_count = 0;
	lineInfoNo.box_num_count = 0;
	lineInfoNo1.box_num_count = 0;
	lineInfoCheckDigit.box_num_count = 0;
	lineInfoOwner.box_info.clear();
	lineInfoNo.box_info.clear();
	lineInfoNo1.box_info.clear();
	lineInfoCheckDigit.box_info.clear();

	int ret = findOwnerInfo(rect2, box2, k);
	if (ret == 1){
		box2 = lineInfoOwner.box;
		if (!setTargetRegion(12, &rect2, &box2, lineInfoOwner.k, lineInfoOwner.b, img.rows, img.cols)){
			return false;
		}
		//寻找箱编号部分-6位数字
		//1.0 第一次尝试：在货主代码右边检测箱编号
		ret = findNoInfo(rect2, box2, k);
		if (ret == 2){
			//在货主代码右边检测到时
			box2 = lineInfoNo.box;
			if (!setTargetRegion(23, &rect2, &box2, lineInfoNo.k, lineInfoNo.b, img.rows, img.cols)){
				return false;
			}
			//在箱编号右方检测校验码部分，找到时完成
			if (findCheckDigitInfo(rect2, box2, k) == 3){
				return true;
			}
		}
		//2.0 第二次尝试：在货主代码下一行检测箱编号
		box2 = lineInfoOwner.box;
		if (!setTargetRegion(14, &rect2, &box2, lineInfoNo.k, lineInfoNo.b, img.rows, img.cols)){
			return false;
		}
		ret = findNoInfo(rect2, box2, k);
		if (ret == 2){
			//找到全部6位数字时
			box2 = lineInfoNo.box;
			if (!setTargetRegion(23, &rect2, &box2, lineInfoNo.k, lineInfoNo.b, img.rows, img.cols)){
				return false;
			}
			//在箱编号右方检测校验码部分，找到时完成
			if (findCheckDigitInfo(rect2, box2, k) == 3){
				return true;
			}
		}
		else if (ret == 20){
			//只找到部分：3位数字时：
			box2 = lineInfoNo1.box;
			if (!setTargetRegion(14, &rect2, &box2, lineInfoNo.k, lineInfoNo.b, img.rows, img.cols)){
				return false;
			}
			//继续尝试向下一行再寻找剩下3位数字
			if (findNoInfo(rect2, box2, k) == 2){
				//找到剩下3位数字时
				box2 = lineInfoNo.box;
				if (!setTargetRegion(23, &rect2, &box2, lineInfoNo.k, lineInfoNo.b, img.rows, img.cols)){
					return false;
				}
				//在找到后3为箱编号的右方检测校验码部分，找到时完成
				if (findCheckDigitInfo(rect2, box2, k) == 3){
					return true;
				}
			}
		}
	}
	//横向货主代码找不到时，向上设定4个U字符高度候选区域
	box2 = mBox_u;
	if (!setTargetRegion(15, &rect2, &box2, k, b, img.rows, img.cols)){
		return false;
	}
	//尝试竖向向上方向寻找货主代码
	ret = findOwnerInfo(rect2, box2, k, 0, 1);
	if (ret == 1){
		//找到货主代码时
		box2 = lineInfoOwner.box;
		if (!setTargetRegion(16, &rect2, &box2, lineInfoOwner.k, lineInfoOwner.b, img.rows, img.cols)){
			return false;
		}
		ret = findNoInfo(rect2, box2, k, 0, 1);
		if (ret == 2){
			//在货主代码下方检测到时
			box2 = lineInfoNo.box;
			if (!setTargetRegion(17, &rect2, &box2, lineInfoNo.k, lineInfoNo.b, img.rows, img.cols)){
				return false;
			}
			//在箱编号右方检测校验码部分，找到时完成
			if (findCheckDigitInfo(rect2, box2, k, 0, 1) == 3){
				return true;
			}

		}
	}
	return false;
}

//搜寻4位货主代码
int FindContent::findOwnerInfo(Rect rect, RotatedRect boxinfo, float k, int lastThresh, int orient)
{
	bool found = false;

	//建立工作Mat，截取灰度图并二值化
	Mat tmpImg = Mat(mGray.rows, mGray.cols, CV_8UC1);
	tmpImg = 0 * tmpImg;
	Mat roi_ori = CreateMat(mGray, rect);
	Mat roi = CreateMat(tmpImg, rect);
	if (roi_ori.rows == 1 && roi_ori.cols == 1){
		return found;  //rect区域超出画面范围时，CreateMat返回Mat(1,1,CV_8UC1):终止寻找返回false
	}
	cv::threshold(roi_ori, roi, 0, 255, CV_THRESH_OTSU);

	if (debug_level > 4){
		imshow("ImgOut3", tmpImg);
		cvWaitKey(1);
	}

	//中心线y=kx+b,斜率k,偏移b：k = tan(boxinfo.angle*3.1415/180)
	float b = boxinfo.center.y - k * boxinfo.center.x;

	//出力数据初始化
	LineInfo *lineInfo = new LineInfo;
	if (boxinfo.size.width < boxinfo.size.height){
		CalcRotatedRectPoints(&boxinfo, &k);
		b = boxinfo.center.y - k * boxinfo.center.x;
	}
	lineInfo->k = k;
	lineInfo->b = b;
	lineInfo->box = boxinfo;

	//寻找对象区域中轮廓
	vector<vector<Point>> contours;
	cv::findContours(tmpImg, contours, noArray(), RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	//重新计算外框box
	CalContourInfo(contours, lineInfo);

	filterContour(contours, lineInfo, mGray.rows, mGray.cols, orient);

	//判断每个轮廓type：1=字符(全高) 2=其他
	calContourType(lineInfo, 1, orient);

	//过滤不要两边不要的轮廓
	checkLine(lineInfo, 1, orient);

	////调整type
	//rejustOwnerContourType(lineInfo, orient);

	////重新计算外框box
	CalBoxInfo(lineInfo);

	//画debug用图形Start：
	if (debug_level > 4){
		tmpImg = 0 * tmpImg;
		//描绘已找到文字轮廓
		if (lineInfo->box_info.size() > 0){
			vector<vector<Point>> cs;
			for (int i = 0; i < lineInfo->box_info.size(); i++){
				cs.push_back(lineInfo->box_info[i].contour);
			}
			drawContours(tmpImg, cs, -1, Scalar(255), CV_FILLED);
		}
		imshow("ImgOut4", tmpImg);
		cvWaitKey(1);
	}

	if (__min(lineInfo->box.size.width, lineInfo->box.size.height) > mUwidth* 0.8
		&& lineInfo->box_num_count >= 4 && lineInfo->box_info.size() >= 2 && lineInfo->box_info.size() <= 5
		&& lineInfoOwner.box_num_count == 0 && lineInfoNo.box_num_count == 0 && lineInfoCheckDigit.box_num_count == 0){
		if (lineInfo->box_num_count > 4){
			if (orient == 0){
				for (int i = 0; i < 4; i++){
					//计算第i个字符的开始，结束x位置（全部4个字符:）
					double fr = lineInfo->box.center.x - lineInfo->box.size.width / 2 + lineInfo->box.size.width / 4 * i;
					double to = lineInfo->box.center.x - lineInfo->box.size.width / 2 + lineInfo->box.size.width / 4 * (i + 1);
					for (int j = lineInfo->box_info.size() - 1; j > 0; j--){
						RotatedRect b1 = (lineInfo->box_info.begin() + j - 1)->box;
						RotatedRect b2 = (lineInfo->box_info.begin() + j)->box;
						if (b1.center.x <= to && b1.center.x >= fr && b2.center.x <= to && b2.center.x >= fr){
							//2个轮廓都在一个字符范围内：第j个轮廓插入第j-1个轮廓后
							(lineInfo->box_info.begin() + j - 1)->contour.insert(
								(lineInfo->box_info.begin() + j - 1)->contour.end(), (lineInfo->box_info.begin() + j)->contour.begin(), (lineInfo->box_info.begin() + j)->contour.end());
							(lineInfo->box_info.begin() + j - 1)->box = minAreaRect((lineInfo->box_info.begin() + j - 1)->contour);
							lineInfo->box_1of2_count--;
							if ((lineInfo->box_info.begin() + j)->type > 0){
								lineInfo->box_num_count = lineInfo->box_num_count - (lineInfo->box_info.begin() + j)->type;
							}
							lineInfo->box_info.erase(lineInfo->box_info.begin() + j);
							continue;
						}
					}
				}
			}
			else{
				for (int j = lineInfo->box_info.size() - 1; j > 0; j--){
					RotatedRect b1 = (lineInfo->box_info.begin() + j - 1)->box;
					RotatedRect b2 = (lineInfo->box_info.begin() + j)->box;
					if (abs(b1.center.y - b2.center.y) < mUheight*0.5){
						//竖向时：2个轮廓都在同一个高度：第j个轮廓插入第j-1个轮廓后
						(lineInfo->box_info.begin() + j - 1)->contour.insert(
							(lineInfo->box_info.begin() + j - 1)->contour.end(), (lineInfo->box_info.begin() + j)->contour.begin(), (lineInfo->box_info.begin() + j)->contour.end());
						(lineInfo->box_info.begin() + j - 1)->box = minAreaRect((lineInfo->box_info.begin() + j - 1)->contour);
						lineInfo->box_1of2_count--;
						if ((lineInfo->box_info.begin() + j)->type > 0){
							lineInfo->box_num_count = lineInfo->box_num_count - (lineInfo->box_info.begin() + j)->type;
						}
						lineInfo->box_info.erase(lineInfo->box_info.begin() + j);
						continue;
					}
				}

			}
		}

		lineInfoOwner = *lineInfo;
		return 1;
	}
	return -1;

}

//搜寻6位箱编号
int FindContent::findNoInfo(Rect rect, RotatedRect boxinfo, float k, int lastThresh, int orient)
{
	bool found = false;

	//建立工作Mat，截取灰度图并二值化
	Mat tmpImg = Mat(mGray.rows, mGray.cols, CV_8UC1);
	tmpImg = 0 * tmpImg;
	Mat roi_ori = CreateMat(mGray, rect);
	Mat roi = CreateMat(tmpImg, rect);
	if (roi_ori.rows == 1 && roi_ori.cols == 1){
		return found; //rect区域超出画面范围时，CreateMat返回Mat(1,1,CV_8UC1):终止寻找返回false
	}
	cv::threshold(roi_ori, roi, 0, 255, CV_THRESH_OTSU);

	if (debug_level > 4){
		imshow("ImgOut3", tmpImg);
		cvWaitKey(1);
	}

	//中心线y=kx+b,斜率k,偏移b：k = tan(boxinfo.angle*3.1415/180)
	float b = boxinfo.center.y - k * boxinfo.center.x;

	//出力数据初始化
	LineInfo *lineInfo = new LineInfo;
	if (boxinfo.size.width < boxinfo.size.height){
		CalcRotatedRectPoints(&boxinfo, &k);
		b = boxinfo.center.y - k * boxinfo.center.x;
	}
	lineInfo->k = k;
	lineInfo->b = b;
	lineInfo->box = boxinfo;

	//寻找对象区域中轮廓
	vector<vector<Point>> contours;
	cv::findContours(tmpImg, contours, noArray(), RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	//重新计算外框box
	CalContourInfo(contours, lineInfo);

	filterContour(contours, lineInfo, mGray.rows, mGray.cols, orient);

	//判断每个轮廓type：
	calContourType(lineInfo, 0, orient);

	//过滤不要两边不要的轮廓
	checkLine(lineInfo, 0, orient); //第二个参数 1=ower 0=owner之外 第三个参数 0=横向 1=竖向

	////调整type
	//rejustOwnerContourType(lineInfo, orient);

	////重新计算外框box
	CalBoxInfo(lineInfo);

	//画debug用图形Start：
	if (debug_level > 4){
		tmpImg = 0 * tmpImg;
		//描绘已找到文字轮廓
		if (lineInfo->box_info.size() > 0){
			vector<vector<Point>> cs;
			for (int i = 0; i < lineInfo->box_info.size(); i++){
				cs.push_back(lineInfo->box_info[i].contour);
			}
			drawContours(tmpImg, cs, -1, Scalar(255), CV_FILLED);
		}
		imshow("ImgOut4", tmpImg);
		cvWaitKey(1);
	}

	if (__min(lineInfo->box.size.width, lineInfo->box.size.height) > mUwidth*.08
		&& lineInfo->box_num_count > 5
		&& (orient == 1 || abs(lineInfoOwner.box.size.height - (lineInfo->box_info.begin())->box.size.height) < lineInfoOwner.box.size.height*0.2)        //横向是两个区域高度要基本一致
		&& lineInfoOwner.box_num_count > 0 && lineInfoNo.box_num_count == 0 && lineInfoCheckDigit.box_num_count == 0){
		if (orient == 0){
			//字距间隔宽度：标准0.5字符高，box宽高比大时设为0.9
			double fact = 0.5;
			if (lineInfo->box.size.height * 0.9 < lineInfo->box.size.width / 6){
				fact = 0.9;
			}
			int cnt = (lineInfo->box_info.begin())->type;
			for (int i = 1; i < lineInfo->box_info.size(); i++)
			{
				RotatedRect b2 = (lineInfo->box_info.begin() + i)->box;
				RotatedRect b1 = (lineInfo->box_info.begin() + i - 1)->box;
				if ((lineInfo->box_info.begin() + i)->type > 0){
					cnt = cnt + (lineInfo->box_info.begin() + i)->type;
				}
				if (cnt == 5){
					//第五个字符有时候会稍微空开些,2倍fact
					if (b2.center.x - b2.size.width / 2 - b1.center.x - b1.size.width / 2 > b1.size.height * fact * 2){
						return -1;
					}
				}
				else{
					if (cnt <= 6 && b2.center.x - b2.size.width / 2 - b1.center.x - b1.size.width / 2 > b1.size.height * fact){
						return -1;
					}
				}
				if (cnt == 6){
					for (int j = lineInfo->box_info.size() - 1; j > i; j--){
						lineInfo->box_1of2_count--;
						if ((lineInfo->box_info.begin() + j)->type > 0){
							lineInfo->box_num_count = lineInfo->box_num_count - (lineInfo->box_info.begin() + j)->type;
						}
						lineInfo->box_info.erase(lineInfo->box_info.begin() + j);
					}
					break;
				}

			}
			//重新计算外框
			CalBoxInfo(lineInfo);
		}
		else{
			int cnt = (lineInfo->box_info.begin())->type;
			for (int i = 1; i < lineInfo->box_info.size(); i++)
			{
				if ((lineInfo->box_info.begin() + i)->type > 0){
					cnt = cnt + (lineInfo->box_info.begin() + i)->type;
				}
				if (cnt == 6){
					for (int j = lineInfo->box_info.size() - 1; j > i; j--){
						lineInfo->box_1of2_count--;
						if ((lineInfo->box_info.begin() + j)->type > 0){
							lineInfo->box_num_count = lineInfo->box_num_count - (lineInfo->box_info.begin() + j)->type;
						}
						lineInfo->box_info.erase(lineInfo->box_info.begin() + j);
					}
					break;
				}

			}
			//重新计算外框
			CalBoxInfo(lineInfo);

		}
		//判定找到区域
		lineInfoNo = *lineInfo;
		return 2;
	}
	else if (orient == 0 && __min(lineInfo->box.size.width, lineInfo->box.size.height) > 13
		&& __min(lineInfo->box_num_count, lineInfo->box_info.size()) >= 3
		&& lineInfo->box.center.y - lineInfo->box.size.height / 2 > lineInfoOwner.box.center.y + lineInfoOwner.box.size.height / 2 //必须是上下排列的
		&& abs(lineInfoOwner.box.size.height - (lineInfo->box_info.begin())->box.size.height) < lineInfoOwner.box.size.height*0.2        //两个区域高度要基本一致
		&& lineInfoOwner.box_num_count > 0 && lineInfoNo1.box_num_count == 0 && lineInfoCheckDigit.box_num_count == 0){
		double fact = 0.5;
		if (lineInfo->box.size.height * 0.9 < lineInfo->box.size.width / 3){
			fact = 0.9;
		}
		for (int i = 1; i < lineInfo->box_info.size(); i++)
		{
			RotatedRect b2 = (lineInfo->box_info.begin() + i)->box;
			RotatedRect b1 = (lineInfo->box_info.begin() + i - 1)->box;
			if (b2.center.x - b2.size.width / 2 - b1.center.x - b1.size.width / 2 > b1.size.height * fact){
				return -1;
			}
		}
		//判定找到区域
		lineInfoNo1 = *lineInfo;
		return 20;
	}
	else if (orient == 0 && __min(lineInfo->box.size.width, lineInfo->box.size.height) > 13
		&& lineInfo->box_num_count >= 3
		&& lineInfoOwner.box_num_count > 0 && lineInfoNo1.box_num_count > 0 && lineInfoCheckDigit.box_num_count == 0){
		double fact = 0.5;
		int cnt = 0;
		for (int i = 1; i < lineInfo->box_info.size(); i++)
		{
			RotatedRect b2 = (lineInfo->box_info.begin() + i)->box;
			RotatedRect b1 = (lineInfo->box_info.begin() + i - 1)->box;
			if ((lineInfo->box_info.begin() + i)->type > 0){
				cnt = cnt + (lineInfo->box_info.begin() + i)->type;
			}
			if (b2.center.x - b2.size.width / 2 - b1.center.x - b1.size.width / 2 > b1.size.height * fact){
				return -1;
			}
			if (cnt == 3){
				for (int j = lineInfo->box_info.size() - 1; j > i; j--){
					lineInfo->box_1of2_count--;
					if ((lineInfo->box_info.begin() + j)->type > 0){
						lineInfo->box_num_count = lineInfo->box_num_count - (lineInfo->box_info.begin() + j)->type;
					}
					lineInfo->box_info.erase(lineInfo->box_info.begin() + j);
				}
				break;
			}
		}
		//判定找到区域
		lineInfoNo = *lineInfo;
		return 2;
	}

	return -1;

}

//搜寻1位校验位
int FindContent::findCheckDigitInfo(Rect rect, RotatedRect boxinfo, float k, int lastThresh, int orient)
{
	bool found = false;

	//建立工作Mat，截取灰度图并二值化
	Mat tmpImg = Mat(mGray.rows, mGray.cols, CV_8UC1);
	Mat gray;
	cv::threshold(mGray, gray, 0, 255, CV_THRESH_OTSU);
	tmpImg = 0 * tmpImg;
	Mat roi_ori = CreateMat(gray, rect);
	Mat roi = CreateMat(tmpImg, rect);
	if (roi_ori.rows == 1 && roi_ori.cols == 1){
		return found; //rect区域超出画面范围时，CreateMat返回Mat(1,1,CV_8UC1):终止寻找返回false
	}
	roi_ori.copyTo(roi);
	//cv::threshold(roi_ori, roi, 0, 255, CV_THRESH_OTSU);

	if (debug_level > 4){
		imshow("ImgOut3", tmpImg);
		cvWaitKey(1);
	}

	//中心线y=kx+b,斜率k,偏移b：k = tan(boxinfo.angle*3.1415/180)
	float b = boxinfo.center.y - k * boxinfo.center.x;

	//出力数据初始化
	LineInfo *lineInfo = new LineInfo;
	if (boxinfo.size.width < boxinfo.size.height){
		CalcRotatedRectPoints(&boxinfo, &k);
		b = boxinfo.center.y - k * boxinfo.center.x;
	}
	lineInfo->k = k;
	lineInfo->b = b;
	lineInfo->box = boxinfo;

	//寻找对象区域中轮廓
	vector<vector<Point>> contours;
	cv::findContours(tmpImg, contours, noArray(), RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	//重新计算外框box
	CalContourInfo(contours, lineInfo);

	filterContour(contours, lineInfo, mGray.rows, mGray.cols);

	//判断每个轮廓type：1=字符(全高) 2=其他
	calContourType(lineInfo);

	//过滤不要两边不要的轮廓
	checkLine(lineInfo);

	////重新计算外框box
	CalBoxInfo(lineInfo);

	//画debug用图形Start：
	if (debug_level > 4){
		tmpImg = 0 * tmpImg;
		//描绘已找到文字轮廓
		if (lineInfo->box_info.size() > 0){
			vector<vector<Point>> cs;
			for (int i = 0; i < lineInfo->box_info.size(); i++){
				cs.push_back(lineInfo->box_info[i].contour);
			}
			drawContours(tmpImg, cs, -1, Scalar(255), CV_FILLED);
		}
		//描绘全部文字轮廓的最小外接矩形
		//drawRotatedRect(tmpImg, lineInfo->box, 1, Scalar(255));
		imshow("ImgOut4", tmpImg);
		cvWaitKey(1);
	}

	if (lineInfoOwner.box_num_count > 0 && lineInfoNo.box_num_count > 0)
	{
		//横向时：取左边第一个轮廓(太细轮廓不要)
		if (orient == 0){
			if (lineInfo->box_info.size() > 1)
			{
				if ((lineInfo->box_info.begin())->box.size.width < 3)
				{
					lineInfo->box_info.erase(lineInfo->box_info.begin());
				}
			}
			while (lineInfo->box_info.size() > 1)
			{
				lineInfo->box_info.erase(lineInfo->box_info.end() - 1);
			}
		}
		else{
			//竖向时，找U字符中间线上轮廓
			if (lineInfo->box_info.size() > 1)
			{
				if ((lineInfo->box_info.begin())->box.size.width < 3)
				{
					lineInfo->box_info.erase(lineInfo->box_info.begin());
				}
			}
			while (lineInfo->box_info.size() > 1)
			{
				lineInfo->box_info.erase(lineInfo->box_info.end() - 1);
			}

		}
		lineInfo->box_num_count = 1;
		CalBoxInfo(lineInfo);
		lineInfoCheckDigit = *lineInfo;
		return 3;
	}
	return -1;

}

//设定搜索范围：根据找到内容推算下一步位置
bool FindContent::setTargetRegion(int type, Rect* rect, RotatedRect* box, float k, float b, int rows, int cols){
	Point newCenter;
	RotatedRect newBox;
	float new_k;
	double w, h;
	if (type == 12){
		//type=12:站在Owner中心点、向右推算出箱编号可能区域
		//       ==》向右方平移2.5倍宽度+间隔(1个高度？)
		newCenter.x = box->center.x + box->size.width * 1.8 + box->size.height * 1;
		newCenter.y = newCenter.x * k + b;
		w = box->size.width * 2.5;
		h = box->size.height;
	}
	else if (type == 21){
		//type=2:站在箱编号中心点、向左推算出Owner可能区域
		//      ==》向左平移1.5倍宽度+间隔(1个高度？)
		newCenter.x = box->center.x - box->size.width * 1.5 - box->size.height * 1;
		newCenter.y = newCenter.x * k + b;
		w = box->size.width*1.0;
		h = box->size.height;
	}
	else if (type == 14){
		//type=14:站在Owner中心点、向下推算出箱编号可能区域
		//       ==》向下方平移1倍宽度+间隔(0.2个高度？)
		newCenter.y = box->center.y + box->size.height * 1 + box->size.height * 0.2;
		if (k >= 0.1 && (newCenter.y - b) / k > 0){
			newCenter.x = (newCenter.y - b) / k;
		}
		else{
			newCenter.x = box->center.x;
		}
		w = box->size.width * 2.5;
		h = box->size.height;

	}
	else if (type == 23){
		//type=23:站在箱编号全车牌中心点、推算出校验位所在的可能区域
		//      ==》右边界+间隔（5个高度？)
		newCenter.x = box->center.x + box->size.width / 2 + box->size.height * 2 + 8;
		newCenter.y = newCenter.x * k + b;
		w = box->size.height * 4;
		h = box->size.height - 4;
	}
	else if (type == 11){
		//type=11:站在Owner最右边U字符中心点、向左推算Owner所在可能区域
		//       ==》向左方平移1.5+间隔(0.8个U字符宽度？)
		newCenter.x = box->center.x - (box->size.width * 1 + box->size.width * 0.8 * 1.5); //假设字符间隔0.8个U字符宽度
		newCenter.y = box->center.y;
		//w = box->size.width * (4 + 0.8 * 3 + 1.2); //假设字符间隔位U字符宽度的0.8倍+1个字符误差
		w = box->size.width * (4 + (0.8+0.6) * 3)+10; //
		h = box->size.height;
	}
	else if (type == 15){
		//type=12:站在U中心点、向上推算Owner所在可能区域
		//       ==》向上方平移2.5个U字符宽度？
		newCenter.x = box->center.x;
		newCenter.y = box->center.y + box->size.height / 2 - mUheight * 2.5;
		w = box->size.width * 2;
		h = mUheight * 5;
	}
	else if (type == 16){
		//type=12:站在Owner中心点、向下推算箱编号所在可能区域
		//       ==》向左方平移5个U字符宽度？
		newCenter.x = box->center.x;
		newCenter.y = box->center.y + box->size.height / 2 + mUheight * 4 + 4;
		w = box->size.width * 2;
		h = mUheight * 8 - 8;
	}
	else if (type == 17){
		//type=13:站在竖向6位数字编号中心点、向左推算CheckDigit所在可能区域
		//       ==》向下左方移动2个u字符高度
		newCenter.x = box->center.x;
		newCenter.y = box->center.y + +box->size.height / 2 + mUheight * 2 + 4;
		w = box->size.width * 2;
		h = mUheight * 4 - 8;
	}
	else{
		return false;
	}

	if (newCenter.x <= 0 || newCenter.y <= 0){
		return false;
	}
	newBox = RotatedRect(newCenter, Size(w, h), box->angle);
	CalcRotatedRectPoints(&newBox, &new_k);

	//重新计算外接Rect
	Point2f pt[4];
	newBox.points(pt);
	vector<Point> pts = { pt[0], pt[1], pt[2], pt[3] };
	Rect newRect = boundingRect(pts);

	//搜索区域稍微放大一些
	newRect.x = (newRect.x < 8) ? newRect.x : newRect.x - 8;
	newRect.y = (newRect.y < 4) ? newRect.y : newRect.y - 4;
	newRect.width = newRect.width + 16;
	newRect.height = newRect.height + 8;

	//设定返回值
	*box = newBox;
	*rect = newRect;
	return true;
}

//过滤轮廓
void FindContent::filterContour(vector<vector<Point>> pContours, LineInfo* lineInfo, int rows, int cols, int orient){
	//过滤掉太大太小或高宽比不正确的轮廓
	vector<vector<Point>> contours_debug;
	vector<Point> allPoints;
	vector<boxInfo> ret_boxinfos;

	float k = lineInfo->k;
	float b = lineInfo->b;
	RotatedRect boxinfo = lineInfo->box;
	lineInfo->box_1of2_count = 0;
	vector<vector<Point>> contours = pContours;
	vector<vector<Point>>::iterator contour = contours.begin();
	int cnt = contours.size();
	while (contour != contours.end()) {
		if (debug_level > 7) {
			Mat ImgTmp = Mat(rows, cols, CV_8UC1);
			ImgTmp = 0 * ImgTmp;
			contours_debug.push_back(*contour);
			drawContours(ImgTmp, contours_debug, -1, Scalar(255));
			imshow("ImgOut2", ImgTmp);
			cvWaitKey(1);
			cvWaitKey(1);
		}
		//获取最小包围矩形（有倾斜角度！）
		float box_k;
		RotatedRect box = getRotatedRectFromPoints(*contour, &box_k); // minAreaRect(*contour);

		if ((orient == 0 && (box.size.width > 80 || box.size.height > 45)) ||
			(orient == 1 && (box.size.width > 45 || box.size.height > 10 * mUheight))){
			//发现有超长或超宽的轮廓：不可能是字符：清楚当前轮廓
			contour = contours.erase(contour);
			continue;
		}
		if (box.size.height < __min(mUheight>0 ? mUheight : 999, 13)){
			//高度太小的轮廓不可能是字符：删除
			contour = contours.erase(contour);
			continue;
		}
		if (box.size.height > 1.1*mUheight && box.size.width < mUwidth *0.3){
			//高度太大但宽度很小的轮廓不可能是字符：删除
			contour = contours.erase(contour);
			continue;
		}
		if (__max(box.size.width, box.size.height) < __min(mUheight > 0 ? mUheight : 999, 13)){
			//很小的轮廓：删除
			contour = contours.erase(contour);
			continue;
		}
		int w = box.size.width;
		int h = box.size.height;
		//先判断是否是U字符，含U字符的轮廓无需在做分割了
		if (!(box.size.width == mBox_u_all.size.width && box.size.width == mBox_u_all.size.width
			&& box.center.x == mBox_u_all.center.x && box.center.y == mBox_u_all.center.y)) { 
			if (orient == 0 && box.size.height < 50	&& box.size.width > mUwidth * 2.2){
				vector<boxInfo> retboxs;
				//太宽的轮廓分割后插入最后
				//先尝试削除上部区域粘连：参数=1上部削除
				boxInfo boxinfoTmp;
				boxinfoTmp.box = box;
				boxinfoTmp.contour = *contour;
				retboxs = breakContours(mGray, boxinfoTmp, mUheight, 35, 1);
				if (retboxs.size() > 1 || (retboxs.size() == 1 && retboxs[0].box.size.width < box.size.width*0.9)){
					//分割后box变多了，或者宽度缩小了：表明分割有效，删除当前轮廓，新找到轮廓插入队列后续处理
					int cur = contour - contours.begin();
					vector<vector<Point>> pts;
					for (int i = 0; i < retboxs.size(); i++){
						pts.push_back((retboxs.begin() + i)->contour);
					}
					contours.insert(contours.end(), pts.begin(), pts.end());
					contour = contours.begin() + cur;
					contour = contours.erase(contour);
					continue;
				}
				else{
					//分割无效时再次尝试削除另一端粘连：
					boxInfo boxinfoTmp2;
					boxinfoTmp2.box = box;
					boxinfoTmp2.contour = *contour;
					retboxs = breakContours(mGray, boxinfoTmp2, mUheight, 35, 2);
					if (retboxs.size() > 1 || (retboxs.size() == 1 && retboxs[0].box.size.width < box.size.width*0.9)){
						//分割后box变多了，或者宽度缩小了：表明分割有效，删除当前轮廓，新找到轮廓插入队列后续处理
						int cur = contour - contours.begin();
						vector<vector<Point>> pts;
						for (int i = 0; i < retboxs.size(); i++){
							pts.push_back((retboxs.begin() + i)->contour);
						}
						contours.insert(contours.end(), pts.begin(), pts.end());
						contour = contours.begin() + cur;
						contour = contours.erase(contour);
						continue;
					}
				}
			}
		}

		boxInfo boxinfo2;
		boxinfo2.idx = -1;
		boxinfo2.box = box;
		boxinfo2.contour = *contour;
		boxinfo2.type = 0;
		boxinfo2.k = box_k;
		ret_boxinfos.push_back(boxinfo2);
		allPoints.insert(allPoints.end(), contour->begin(), contour->end());
		contour++;
	}

	//重新计算外接矩形和斜率
	RotatedRect newbox;
	if (allPoints.size()>0){
		newbox = getRotatedRectFromPoints(allPoints, &k);
		b = newbox.center.y - k * newbox.center.x;
	}
	else{
		newbox = RotatedRect(Point(0, 0), Size(0, 0), 0.0);
	}
	lineInfo->box = newbox;
	lineInfo->k = k;
	lineInfo->b = b;
	lineInfo->box_info = ret_boxinfos;
	return;
}

//估算轮廓中含字符数量
void FindContent::calContourType(LineInfo* lineInfo, int target, int orient){
	//计数初始化
	lineInfo->box_num_count = 0;

	vector<boxInfo> boxinfos = lineInfo->box_info;
	RotatedRect newbox = lineInfo->box;
	//判断每个轮廓=?个宽度
	vector<boxInfo>::iterator c = boxinfos.begin();
	while (c != boxinfos.end()) {
		if (orient == 0){
			int boxtype = 0;
			RotatedRect box = c->box;
			//if (box.size.height > 12 && box.size.height < 35 && (box.size.width < mUwidth * 1.1 || box.size.width < box.size.height*0.65)){
			//	lineInfo->box_num_count++;
			//	c->type = 1;
			//}
			//else if (box.size.height > 12 && box.size.height < 35 && (box.size.width < mUwidth * 2 * 1.2 || box.size.width < box.size.height*1.2)){
			//	lineInfo->box_num_count++;
			//	lineInfo->box_num_count++;
			//	c->type = 2;
			//}
			//else{
			//	c->type = 0;
			//}
			if (box.size.height > 12 && box.size.height < 35 && box.size.width < mUwidth * 1.4){
				lineInfo->box_num_count++;
				c->type = 1;
			}
			else if (box.size.height > 12 && box.size.height < 35 && box.size.width >= mUwidth * 1.4 && box.size.width < mUwidth * 2 * 1.3){
					
				//fprintf(stdout, "Type=%d,boxWidth=%6.3f,boxHeight=%6.3f,mUwidth=%6.3f,mUheight=%6.3f",
				//		             c->type, box.size.width,box.size.height,mUwidth,mUheight);
				lineInfo->box_num_count++;
				lineInfo->box_num_count++;
				c->type = 2;
			}
			else if (box.size.height > 12 && box.size.height < 35 && box.size.width >= mUwidth * 2 * 1.3 && box.size.width < mUwidth * 3 * 1.3){
				lineInfo->box_num_count++;
				lineInfo->box_num_count++;
				lineInfo->box_num_count++;
				c->type = 3;
			}
			else{
				c->type = 0;
			}
		}
		else{
			int boxtype = 0;
			if (target == 1){
				for (int i = 1; i<5; i++){
					if (c->box.size.height > mUheight * 0.8 * i
						&& c->box.size.height < mUheight * 1.2 * i){
						boxtype = i;
						break;
					}
				}
			}
			else{
				for (int i = 1; i<7; i++){
					if (c->box.size.height > mUheight * 0.7 * i
						&& c->box.size.height < mUheight * 1.1 * i){
						boxtype = i;
						break;
					}
				}
			}
			lineInfo->box_num_count = lineInfo->box_num_count + boxtype;
			c->type = boxtype;
		}
		c++;
	}
	lineInfo->box_info = boxinfos;
	lineInfo->box_1of2_count = boxinfos.size();
}

//去除多余轮廓：两边零碎线条或细小噪点
void FindContent::checkLine(LineInfo* lineInfo, int target, int orient){ //target:0=all,1=owner orient：1=横向 2=竖向 
	vector<boxInfo> bottom;
	if (orient == 0){
		//横向区域时
		//box排序（x坐标从大到小:按x轴中心点排序）
		sortLineInfo(lineInfo, 2, 0);

		//去掉最靠右边的不要的轮廓（此时x坐标从大到小排序！checkDigit时不需要去除）
		while (lineInfo->box_info.size()>1 && lineInfoNo.box_num_count == 0){
			if (target == 1 && lineInfo->box_info.begin()->box.center.x - lineInfo->box_info.begin()->box.size.width / 2 > mBox_u.center.x){
				//中心点在U右边的轮廓都不要！
				lineInfo->box_1of2_count--;
				if (lineInfo->box_info.begin()->type > 0){
					lineInfo->box_num_count--;
				}
				lineInfo->box_info.erase(lineInfo->box_info.begin());
				continue;
			}
			else if (target == 1 && lineInfo->box_info.begin()->box.center.x + lineInfo->box_info.begin()->box.size.width / 2 > mBox_u.center.x + mUwidth / 2 + 2){
				//右边界超出U的右边界的轮廓都不要！
				lineInfo->box_1of2_count--;
				if (lineInfo->box_info.begin()->type > 0){
					lineInfo->box_num_count--;
				}
				lineInfo->box_info.erase(lineInfo->box_info.begin());
				continue;
			}
			else if (__max(lineInfo->box_info.begin()->box.size.height, lineInfo->box_info.begin()->box.size.width) < __min(12, lineInfo->box.size.height *0.7)){
				//右边只能是字符，高度较小或宽度太小的话直接过滤掉即可
				lineInfo->box_1of2_count--;
				if (lineInfo->box_info.begin()->type > 0){
					lineInfo->box_num_count--;
				}
				lineInfo->box_info.erase(lineInfo->box_info.begin());
				continue;
			}
			else if (lineInfo->box_info.size()>1
				&& (lineInfo->box_info.begin())->box.size.height > (lineInfo->box_info.begin() + 1)->box.size.height + 4){
				//发现最右边轮廓高度比较大：判断是边框删除最右边轮廓
				lineInfo->box_1of2_count--;
				if (lineInfo->box_info.begin()->type > 0){
					lineInfo->box_num_count--;
				}
				lineInfo->box_info.erase(lineInfo->box_info.begin());
				continue;
			}
			else{
				break;
			}
		}
		//重新排序(x坐标从小到大)
		for (int i = lineInfo->box_info.size() - 1; i >= 0; i--){
			bottom.push_back(lineInfo->box_info[i]);
		}
		//去掉最靠左边的轮廓（checkDigit时不需要）
		while (bottom.size() > 0 && lineInfoNo.box_num_count == 0){
			if (__max(bottom.begin()->box.size.height, bottom.begin()->box.size.width) < __min(15, lineInfo->box.size.height *0.7)){
				//左边只能是字符，高度较小或宽度太小的话直接过滤掉即可
				lineInfo->box_1of2_count--;
				if (bottom.begin()->type > 0){
					lineInfo->box_num_count--;
				}
				bottom.erase(bottom.begin());
				continue;
			}
			else if (bottom.size() > 2 && (lineInfoOwner.box_num_count == 0 || lineInfoNo.box_num_count == 0) && (bottom.begin())->box.size.width < (bottom.begin())->box.size.height * 0.2
				&& ((bottom.begin() + 1)->box.center.x - (bottom.begin())->box.center.x >(bottom.begin() + 2)->box.center.x - (bottom.begin() + 1)->box.center.x + 5
				|| (bottom.begin() + 1)->box.center.x - (bottom.begin())->box.center.x > ((bottom.begin() + 2)->box.center.x - (bottom.begin() + 1)->box.center.x) * 1.2
				|| (bottom.begin() + 1)->box.center.x - (bottom.begin() + 1)->box.size.width / 2 - (bottom.begin())->box.center.x - (bottom.begin())->box.size.width / 2 > ((bottom.begin() + 2)->box.center.x - (bottom.begin() + 1)->box.center.x))){
				//发现最左边2个轮廓间隔较大,并且宽度较大：删除最左边轮廓
				//寻找货主或编号时需要去除，但checkDigit时需要直接取最左位，无需去除左边间隔远轮廓
				lineInfo->box_1of2_count--;
				if (bottom.begin()->type > 0){
					lineInfo->box_num_count--;
				}
				bottom.erase(bottom.begin());
				continue;
			}
			else if (bottom.size()>1
				&& (bottom.begin())->box.size.height > (bottom.begin() + 1)->box.size.height + 4){
				//发现最左边轮廓高度比较大：判断是边框删除最右边轮廓
				lineInfo->box_1of2_count--;
				if (bottom.begin()->type > 0){
					lineInfo->box_num_count--;
				}
				bottom.erase(bottom.begin());
				continue;
			}
			else{
				break;
			}
		}
	}
	else{
		//竖向区域时
		//box排序（y坐标从大到小:按y轴中心点排序）
		sortLineInfo(lineInfo, 0, 1);

		//去掉最靠下边的不要的轮廓（此时y坐标从大到小排序！checkDigit时不需要去除）
		while (lineInfo->box_info.size()>1 && lineInfoNo.box_num_count == 0){
			if (target == 1 && lineInfo->box_info.begin()->box.center.y - lineInfo->box_info.begin()->box.size.height / 2 > mBox_u.center.y){
				//中心点在U下边的轮廓都不要！
				lineInfo->box_1of2_count--;
				if (lineInfo->box_info.begin()->type > 0){
					lineInfo->box_num_count--;
				}
				lineInfo->box_info.erase(lineInfo->box_info.begin());
				continue;
			}
			else{
				break;
			}
		}
		for (int i = lineInfo->box_info.size() - 1; i >= 0; i--){
			if (abs((lineInfo->box_info.begin() + i)->box.center.x - mBox_u.center.x) > mUwidth * 1.3){
				//发现最轮廓中心线偏移离开u中心点：判断是边框删除轮廓
				lineInfo->box_1of2_count--;
				if ((lineInfo->box_info.begin() + i)->type > 0){
					lineInfo->box_num_count = lineInfo->box_num_count - (lineInfo->box_info.begin() + i)->type;
				}
				lineInfo->box_info.erase(lineInfo->box_info.begin() + i);
				continue;
			}
			else if (__max((lineInfo->box_info.begin() + i)->box.size.height, (lineInfo->box_info.begin() + i)->box.size.width) < __min(12, mUheight *0.6)){
				//右边只能是字符，高度较小或宽度太小的话直接过滤掉即可
				lineInfo->box_1of2_count--;
				if ((lineInfo->box_info.begin() + i)->type > 0){
					lineInfo->box_info.erase(lineInfo->box_info.begin() + i);
				}
				lineInfo->box_info.erase(lineInfo->box_info.begin() + i);
				continue;
			}
		}
		//重新排序(y坐标从小到大)
		for (int i = lineInfo->box_info.size() - 1; i >= 0; i--){
			bottom.push_back(lineInfo->box_info[i]);
		}
		//去掉最靠上边的轮廓（checkDigit时不需要）
		while (bottom.size() > 0 && lineInfoNo.box_num_count == 0){
			if (bottom.begin()->box.size.height < __min(13, mUheight *0.6)){
				//上边只能是字符，高度或宽度较小的话直接过滤掉即可
				lineInfo->box_1of2_count--;
				if (bottom.begin()->type > 0){
					lineInfo->box_num_count--;
				}
				bottom.erase(bottom.begin());
				continue;
			}
			else{
				break;
			}
		}

	}
	lineInfo->box_info = bottom;
	vector<Point> ptAll;
	float new_k;
	for (int i = 0; i < lineInfo->box_info.size(); i++){
		ptAll.insert(ptAll.begin(), lineInfo->box_info[i].contour.begin(), lineInfo->box_info[i].contour.end());
	}
	RotatedRect newbox;
	newbox = getRotatedRectFromPoints(ptAll, &new_k);
	lineInfo->box = newbox;
	lineInfo->k = new_k;
}

//轮廓排序
void FindContent::sortLineInfo(LineInfo* lineInfo, int pos, int orient){
	vector<boxInfo> boxs = lineInfo->box_info;
	sortBoxInfo(&boxs, pos, orient);
	lineInfo->box_info = boxs;
}


//void FindContent::rejustOwnerContourType(LineInfo* lineInfo, int orient){
//	vector<boxInfo> infos = lineInfo->box_info;
//	if (orient == 0){
//		//此时box已按x轴从小到大排好序了
//		for (int i = infos.size() - 2; i >=0; i--){
//			if (infos[i].type == 2){
//				//计算两个相邻轮廓间距 diff=第i+1个轮廓左边距 - 第i个轮廓右边距
//				float diff = infos[i + 1].box.center.x - infos[i + 1].box.size.width / 2 - (infos[i].box.center.x + infos[i].box.size.width / 2);
//				float diff2 = infos[i + 1].box.center.x - infos[i].box.center.x;
//				if (infos[i].box.size.width < mUwidth / 2 + diff > mUwidth / 2){
//					//真正粘连时，最小距离 1+' '+1（1只有半个字符宽），宽度小于最小距离时：判定为超宽字符，type=1
//					lineInfo->box_num_count = lineInfo->box_num_count - (lineInfo->box_info.begin() + i)->type + 1;
//					(lineInfo->box_info.begin()+i)->type = 1;
//				}
//				else if (diff2 < diff /2 + mUwidth + diff + mUwidth /2 ){
//					//真正粘连应该计算为: 轮廓距1/2 + 一个U宽度+轮廓间距 + 半个U宽度
//					lineInfo->box_num_count = lineInfo->box_num_count - (lineInfo->box_info.begin() + i)->type + 1;
//					(lineInfo->box_info.begin() + i)->type = 1;
//				}
//			}
//		}
//	}
//	else
//	{
//		//竖向时：字符高度都差不多，只会是粘连：无需调整type
//	}
//}


