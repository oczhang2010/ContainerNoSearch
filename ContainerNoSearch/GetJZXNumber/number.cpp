#include "stdafx.h"
#include "number.h"

Number::Number()
{
}

Number::~Number()
{
}

bool Number::searchNumber(Mat src, int method){

	Mat tmp;
	Mat gray;			
	GaussianBlur(src, tmp, Size(3, 3), 0, 0.0, BORDER_DEFAULT);
	cvtColor(tmp, gray, CV_RGB2GRAY);

	double thres_val;

	if (method == 0 || method == 1){
		if (findCharU(prePare(gray, true, false, 20, 10, 0, &thres_val), 0)){
			return true;
		}
	}
	if (method == 0 || method == 2){
		if (findCharU(prePare(gray, true, false, 20, 10, 0, &thres_val), 1)){
			return true;
		}
	}
	if (method == 0 || method == 3){
		if (findCharU(prePare(gray, true, true, 20, 10, 0, &thres_val), 0)){
			return true;
		}
	}
	if (method == 0 || method == 4){
		if (findCharU(prePare(gray, true, true, 20, 10, 0, &thres_val), 1)){
			return true;
		}
	}
	if (method == 0 || method == 5){
		if (findCharU(prePare(gray, false, false, 0, 0, 0, &thres_val), 0)){
			return true;
		}
	}
	if (method == 0 || method == 6){
		if (findCharU(prePare(gray, false, false, 0, 0, 0, &thres_val), 1)){
			return true;
		}
	}
	return false;
}

bool Number::findCharU(Mat img, double thresh){

	Mat threImg;
	if (thresh == 0){
		cv::threshold(mGray, threImg, 0, 255, CV_THRESH_OTSU);
		//img.copyTo(threImg);
	}
	else{
		cv::threshold(mGray, threImg, mThresh, 255, CV_THRESH_BINARY);
	}

	if (debug_level > 4){
		imshow("ImgOut1", mGray);
		waitKey(1);
	}
	vector<vector<Point>> contours_debug;
	vector<vector<Point>> contours;
	cv::findContours(threImg, contours, noArray(), RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	vector<vector<Point>>::iterator contour = contours.begin();
	int cnt = contours.size();
	while (contour != contours.end()) {

		if (debug_level > 7) {
			Mat ImgTmp = Mat(mGray.rows, mGray.cols, CV_8UC1);
			ImgTmp = 0 * ImgTmp;
			contours_debug.clear();
			contours_debug.push_back(*contour);
			cv::drawContours(ImgTmp, contours_debug, -1, Scalar(255), CV_FILLED);
			cv::imshow("ImgOut4", ImgTmp);
			cvWaitKey(1);
			cvWaitKey(1);
		}
		float box_k;
		RotatedRect box = getRotatedRectFromPoints(*contour, &box_k); // minAreaRect(*contour);
		Point2f pt[4];
		box.points(pt);

		Rect rect = boundingRect(vector < Point > { pt[0], pt[1], pt[2], pt[3] });
		if ((rect.height > rect.width && box.size.height < box.size.width)
			|| (rect.height < rect.width && box.size.height > box.size.width)){
			box = RotatedRect(box.center, Size(box.size.height, box.size.width), box.angle - 90);
		}

		if ((box.size.width > 80 || box.size.height > 45)){
			contour = contours.erase(contour);
			continue;
		}
		if (box.size.height < 13 || __min(box.size.height, box.size.width)<3){
			contour = contours.erase(contour);
			continue;
		}
		if (__max(box.size.width, box.size.height) < 13){
			contour = contours.erase(contour);
			continue;
		}
		if (box.size.height < 50 && box.size.width > box.size.height * 1.1){
			vector<boxInfo> retboxs;
			boxInfo boxinfoTmp;
			boxinfoTmp.box = box;
			boxinfoTmp.contour = *contour;
			retboxs = breakContours(boxinfoTmp, 35, 1);
			if (retboxs.size() > 1 || (retboxs.size() == 1 && retboxs[0].box.size.width < box.size.width*0.9)){
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
				boxInfo boxinfoTmp2;
				boxinfoTmp2.box = box;
				boxinfoTmp2.contour = *contour;
				retboxs = breakContours(boxinfoTmp2, 35, 2);
				if (retboxs.size() > 1 || (retboxs.size() == 1 && retboxs[0].box.size.width < box.size.width*0.9)){
					int cur = contour - contours.begin();
					sortBoxInfo(&retboxs, 1);
					double fr = -1;
					double to = -1;
					for (int i = retboxs.size() - 1; i >= 0; i--){
						if (fr>0 && to>0 && (retboxs.begin() + i)->box.size.height < box.size.height * 0.8){
							break;
						}
						Point2f pt[4];
						(retboxs.begin() + i)->box.points(pt);
						if (fr < 0 || fr > __min(__min(pt[0].x, pt[1].x), __min(pt[2].x, pt[3].x))){
							fr = __min(__min(pt[0].x, pt[1].x), __min(pt[2].x, pt[3].x));
						}
						if (to < 0 || to < __max(__max(pt[0].x, pt[1].x), __max(pt[2].x, pt[3].x))){
							to = __max(__max(pt[0].x, pt[1].x), __max(pt[2].x, pt[3].x));
						}
					}
					if (to - fr < box.size.width*0.7){
						vector<Point> pt;
						vector<vector<Point>> pts;
						for (int j = 0; j < boxinfoTmp2.contour.size(); j++){
							if (boxinfoTmp2.contour[j].x >= fr && boxinfoTmp2.contour[j].x <= to){
								pt.push_back(boxinfoTmp2.contour[j]);
							}
						}
						pts.push_back(pt);
						contours.insert(contours.end(), pts.begin(), pts.end());
						if (debug_level > 8) {
							Mat ImgTmp = Mat(mGray.rows, mGray.cols, CV_8UC1);
							ImgTmp = 0 * ImgTmp;
							contours_debug.clear();
							contours_debug.push_back(pt);
							cv::drawContours(ImgTmp, contours_debug, -1, Scalar(255), CV_FILLED);
							cv::imshow("ImgOut3", ImgTmp);
							cvWaitKey(1);
							cvWaitKey(1);
						}
					}
					contour = contours.begin() + cur;
					contour = contours.erase(contour);
					continue;
				}
			}
		}
		RotatedRect boxU;
		int type = 1;
		bool isU = false;
		boxInfo boxinfoU;
		
		if (box.size.height > 13 && box.size.height < 35
			&& box.size.width > box.size.height*0.65 && box.size.width < box.size.height*1.2){
			type = 2;
			boxU = RotatedRect(Point(box.center.x + box.size.width / 4, box.center.y), Size(box.size.width / 2, box.size.height), box.angle);
			boxinfoU.box = boxU;
			boxinfoU.contour = *contour;
			boxinfoU.k = box_k;
			isU = isCharU(boxinfoU);
			if (!isU){
				boxU = RotatedRect(Point(box.center.x - box.size.width / 4, box.center.y), Size(box.size.width / 2, box.size.height), box.angle);
				boxinfoU.box = boxU;
				boxinfoU.contour = *contour;
				boxinfoU.k = box_k;
				isU = isCharU(boxinfoU);
			}
		}
		else{
			type = 1;
			boxU = RotatedRect(Point(box.center.x, box.center.y), Size(box.size.width, box.size.height), box.angle);
			boxinfoU.box = boxU;
			boxinfoU.contour = *contour;
			boxinfoU.k = box_k;
			isU = isCharU(boxinfoU);
		}
		if (isU){

			mBox_u = boxU;
			mUheight = boxU.size.height;
			mUwidth = boxU.size.width;

			RotatedRect box2 = boxU;
			Rect rect2;
			float k = box_k;
			float b = 0;
			if (!setTargetRegion(11, &rect2, &box2, k, b, img.rows, img.cols)){
				contour++;
				continue;
			}

			if (rect2.x + rect2.width>img.cols - 5 || rect2.x< 5
				|| rect2.y + rect2.height>img.rows - 3 || rect2.y < 3){
				contour++;
				continue;
			}

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
					contour++;
					continue;
				}
				ret = findNoInfo(rect2, box2, k);
				if (ret == 2){
					box2 = lineInfoNo.box;
					if (!setTargetRegion(23, &rect2, &box2, lineInfoNo.k, lineInfoNo.b, img.rows, img.cols)){
						contour++;
						continue;
					}
					if (findCheckDigitInfo(rect2, box2, k) == 3){
						return true;
					}
				}
				box2 = lineInfoOwner.box;
				if (!setTargetRegion(14, &rect2, &box2, lineInfoNo.k, lineInfoNo.b, img.rows, img.cols)){
					contour++;
					continue;
				}
				ret = findNoInfo(rect2, box2, k);
				if (ret == 2){
					box2 = lineInfoNo.box;
					if (!setTargetRegion(23, &rect2, &box2, lineInfoNo.k, lineInfoNo.b, img.rows, img.cols)){
						contour++;
						continue;
					}
					if (findCheckDigitInfo(rect2, box2, k) == 3){
						return true;
					}
				}
				else if (ret == 20){
					box2 = lineInfoNo1.box;
					if (!setTargetRegion(14, &rect2, &box2, lineInfoNo.k, lineInfoNo.b, img.rows, img.cols)){
						contour++;
						continue;
					}
					if (findNoInfo(rect2, box2, k) == 2){
						box2 = lineInfoNo.box;
						if (!setTargetRegion(23, &rect2, &box2, lineInfoNo.k, lineInfoNo.b, img.rows, img.cols)){
							contour++;
							continue;
						}
						if (findCheckDigitInfo(rect2, box2, k) == 3){
							return true;
						}
					}
				}
			}
			box2 = boxU;
			if (!setTargetRegion(15, &rect2, &box2, k, b, img.rows, img.cols)){
				contour++;
				continue;
			}
			ret = findOwnerInfo(rect2, box2, k,0,1);
			if (ret == 1){
				box2 = lineInfoOwner.box;
				if (!setTargetRegion(16, &rect2, &box2, lineInfoOwner.k, lineInfoOwner.b, img.rows, img.cols)){
					contour++;
					continue;
				}
				ret = findNoInfo(rect2, box2, k,0,1);
				if (ret == 2){
					box2 = lineInfoNo.box;
					if (!setTargetRegion(17, &rect2, &box2, lineInfoNo.k, lineInfoNo.b, img.rows, img.cols)){
						contour++;
						continue;
					}
					if (findCheckDigitInfo(rect2, box2, k,0,1) == 3){
						return true;
					}

				}
			}

		}

		contour++;
	}
	return false;
}

bool Number::isCharU(boxInfo boxinfo){
	RotatedRect box;
	box.center = boxinfo.box.center;
	box.size = boxinfo.box.size;
	box.angle = boxinfo.box.angle;
	Mat ori = Mat(mGray.rows, mGray.cols, CV_8UC1);
	ori = 0 * ori;
	vector<vector<Point>> c;
	c.push_back(boxinfo.contour);
	cv::drawContours(ori, c, -1, Scalar(255), CV_FILLED);

	if (debug_level > 7) {
		cv::imshow("ImgOut3", ori);
		cvWaitKey(1);
		cvWaitKey(1);
	}

	//Rect rect = boundingRect(boxinfo.contour);
	Point2f pt2[4];
	box.points(pt2);
	Rect rect = boundingRect(vector < Point > { pt2[0], pt2[1], pt2[2], pt2[3] });
	Mat tmp = Mat(mGray.rows, mGray.cols, CV_8UC1);
	Mat roi_ori, roiTmp;
	vector<vector<Point>> contours;
	int diff = 0;
	int step = 2;
	while (diff < 100){
		diff = diff + step;
		rect.height = rect.height - step;
		if (diff >= box.size.height * 1 / 2){
			diff = 999;
			continue;
		}

		roi_ori = CreateMat(ori, rect);
		roiTmp = CreateMat(tmp, rect);
		tmp = 0 * tmp;
		roi_ori.copyTo(roiTmp);
		contours.clear();
		cv::findContours(tmp, contours, noArray(), RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
		if (contours.size() == 2){
			RotatedRect b1 = minAreaRect(*(contours.begin()));
			RotatedRect b2 = minAreaRect(*(contours.begin() + 1));
			if (__max(b1.size.height, b1.size.width) > (box.size.height - diff)*0.8
				&& __min(b1.size.height, b1.size.width) < __max(box.size.height*0.2, 6)
				&& __max(b2.size.height, b2.size.width) >(box.size.height - diff)*0.8
				&& __min(b2.size.height, b2.size.width) < __max(box.size.height*0.2, 6)){
				return true;
			}
		}
	}
	return false;
}

Mat Number::prePare(Mat inputImg, bool doMORPH, bool isBlack, unsigned int val1_1, unsigned int val1_2, int method2, double* val2){
	Mat grayTmp;
	double thres_sv = *val2;
	if (doMORPH && !isBlack){
		Mat element = getStructuringElement(MORPH_RECT, Size(val1_1, val1_2));
		morphologyEx(inputImg, grayTmp, MORPH_BLACKHAT, element);   //blackhat
	}
	else if (doMORPH && isBlack){
		Mat element = getStructuringElement(MORPH_RECT, Size(val1_1, val1_2));
		morphologyEx(inputImg, grayTmp, MORPH_TOPHAT, element);    //tophat
	}
	else{
		grayTmp = inputImg;
	}
	grayTmp.copyTo(mGray);
	Mat retImg = Mat(inputImg.rows, inputImg.cols, CV_8UC1);
	Mat mtmp = Mat(inputImg.rows, inputImg.cols, CV_8UC1);
	Sobel(grayTmp, mtmp, CV_16S, 1, 0, 3, 1, 1, BORDER_DEFAULT);
	convertScaleAbs(mtmp, mtmp);
	if (method2 == 0){
		*val2 = round(threshold(mtmp, retImg, 0, 255, CV_THRESH_OTSU));
		thres_sv = *val2;
		mThresh = *val2;
	}
	else if (method2 == 1){
		threshold(mtmp, retImg, *val2, 255, CV_THRESH_BINARY);
		mThresh = *val2;
		thres_sv = *val2;
		if (countNonZero(retImg) == 0 || countNonZero(255 - retImg) == 0){
			*val2 = 0;
			mThresh = *val2;
			thres_sv = 0;
		}
	}
	else{
		retImg = 0 * mtmp;
		thres_sv = 0;
		*val2 = 0;
		mThresh = *val2;
	}
	return retImg;
}

int Number::findOwnerInfo(Rect rect, RotatedRect boxinfo, float k, int lastThresh, int orient)
{
	bool found = false;

	Mat tmpImg = Mat(mGray.rows, mGray.cols, CV_8UC1);
	tmpImg = 0 * tmpImg;
	Mat roi_ori = CreateMat(mGray, rect);
	Mat roi = CreateMat(tmpImg, rect);
	if (roi_ori.rows == 1 && roi_ori.cols == 1){
		return found;  
	}
	cv::threshold(roi_ori, roi, 0, 255, CV_THRESH_OTSU);

	if (debug_level > 4){
		imshow("ImgOut3", tmpImg);
		cvWaitKey(1);
	}

	float b = boxinfo.center.y - k * boxinfo.center.x;

	LineInfo *lineInfo = new LineInfo;
	if (boxinfo.size.width < boxinfo.size.height){
		CalcRotatedRectPoints(&boxinfo, &k);
		b = boxinfo.center.y - k * boxinfo.center.x;
	}
	lineInfo->k = k;
	lineInfo->b = b;
	lineInfo->box = boxinfo;

	vector<vector<Point>> contours;
	cv::findContours(tmpImg, contours, noArray(), RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	CalContourInfo(contours, lineInfo);

	filterContour(contours, lineInfo, mGray.rows, mGray.cols,orient);

	calContourType(lineInfo,1,orient);

	checkLine(lineInfo,1,orient);

	CalBoxInfo(lineInfo);

	if (debug_level > 4){
		tmpImg = 0 * tmpImg;
		if (lineInfo->box_info.size() > 0){
			vector<vector<Point>> cs;
			for (int i = 0; i < lineInfo->box_info.size(); i++){
				cs.push_back(lineInfo->box_info[i].contour);
			}
			drawContours(tmpImg, cs, -1, Scalar(255), CV_FILLED);
		}
		drawRotatedRect(tmpImg, lineInfo->box, 1, Scalar(255));
		imshow("ImgOut4", tmpImg);
		cvWaitKey(1);
	}

	if (__min(lineInfo->box.size.width, lineInfo->box.size.height) > mUwidth* 0.8
		&& lineInfo->box_num_count >= 4 && lineInfo->box_info.size() > 2 && lineInfo->box_info.size() <= 5
		&& lineInfoOwner.box_num_count == 0 && lineInfoNo.box_num_count == 0 && lineInfoCheckDigit.box_num_count == 0){
		if (lineInfo->box_num_count > 4){
			if (orient == 0){
				for (int i = 0; i < 4; i++){
					double fr = lineInfo->box.center.x - lineInfo->box.size.width / 2 + lineInfo->box.size.width / 4 * i;
					double to = lineInfo->box.center.x - lineInfo->box.size.width / 2 + lineInfo->box.size.width / 4 * (i + 1);
					for (int j = lineInfo->box_info.size() - 1; j > 0; j--){
						RotatedRect b1 = (lineInfo->box_info.begin() + j - 1)->box;
						RotatedRect b2 = (lineInfo->box_info.begin() + j)->box;
						if (b1.center.x <= to && b1.center.x >= fr && b2.center.x <= to && b2.center.x >= fr){
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
					if (abs(b1.center.y  - b2.center.y) < mUheight*0.5){
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

int Number::findNoInfo(Rect rect, RotatedRect boxinfo, float k, int lastThresh, int orient)
{
	bool found = false;

	Mat tmpImg = Mat(mGray.rows, mGray.cols, CV_8UC1);
	tmpImg = 0 * tmpImg;
	Mat roi_ori = CreateMat(mGray, rect);
	Mat roi = CreateMat(tmpImg, rect);
	if (roi_ori.rows == 1 && roi_ori.cols == 1){
		return found; 
	}
	cv::threshold(roi_ori, roi, 0, 255, CV_THRESH_OTSU);

	if (debug_level > 4){
		imshow("ImgOut3", tmpImg);
		cvWaitKey(1);
	}

	float b = boxinfo.center.y - k * boxinfo.center.x;

	LineInfo *lineInfo = new LineInfo;
	if (boxinfo.size.width < boxinfo.size.height){
		CalcRotatedRectPoints(&boxinfo, &k);
		b = boxinfo.center.y - k * boxinfo.center.x;
	}
	lineInfo->k = k;
	lineInfo->b = b;
	lineInfo->box = boxinfo;

	vector<vector<Point>> contours;
	cv::findContours(tmpImg, contours, noArray(), RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	CalContourInfo(contours, lineInfo);

	filterContour(contours, lineInfo, mGray.rows, mGray.cols,orient);

	calContourType(lineInfo, 0,orient);

	checkLine(lineInfo,0,orient); 

	CalBoxInfo(lineInfo);

	if (debug_level > 4){
		tmpImg = 0 * tmpImg;

		if (lineInfo->box_info.size() > 0){
			vector<vector<Point>> cs;
			for (int i = 0; i < lineInfo->box_info.size(); i++){
				cs.push_back(lineInfo->box_info[i].contour);
			}
			drawContours(tmpImg, cs, -1, Scalar(255), CV_FILLED);
		}
		drawRotatedRect(tmpImg, lineInfo->box, 1, Scalar(255));
		imshow("ImgOut4", tmpImg);
		cvWaitKey(1);
	}

	if (__min(lineInfo->box.size.width, lineInfo->box.size.height) > mUwidth*.08
		&& lineInfo->box_num_count > 5
		&& (orient==1||abs(lineInfoOwner.box.size.height - (lineInfo->box_info.begin())->box.size.height) < lineInfoOwner.box.size.height*0.2)       
		&& lineInfoOwner.box_num_count > 0 && lineInfoNo.box_num_count == 0 && lineInfoCheckDigit.box_num_count == 0){
		if (orient == 0){
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
			CalBoxInfo(lineInfo);

		}
		lineInfoNo = *lineInfo;
		return 2;
	}
	else if (orient==0 && __min(lineInfo->box.size.width, lineInfo->box.size.height) > 13
		&& __min(lineInfo->box_num_count, lineInfo->box_info.size()) >= 3
		&& lineInfo->box.center.y - lineInfo->box.size.height / 2 > lineInfoOwner.box.center.y + lineInfoOwner.box.size.height / 2 
		&& abs(lineInfoOwner.box.size.height - (lineInfo->box_info.begin())->box.size.height) < lineInfoOwner.box.size.height*0.2        
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
		lineInfoNo1 = *lineInfo;
		return 20;
	}
	else if (orient==0 && __min(lineInfo->box.size.width, lineInfo->box.size.height) > 13
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

		lineInfoNo = *lineInfo;
		return 2;
	}

	return -1;

}

int Number::findCheckDigitInfo(Rect rect, RotatedRect boxinfo, float k, int lastThresh, int orient)
{
	bool found = false;

	Mat tmpImg = Mat(mGray.rows, mGray.cols, CV_8UC1);
	Mat gray;
	cv::threshold(mGray, gray, 0, 255, CV_THRESH_OTSU);
	tmpImg = 0 * tmpImg;
	Mat roi_ori = CreateMat(gray, rect);
	Mat roi = CreateMat(tmpImg, rect);
	if (roi_ori.rows == 1 && roi_ori.cols == 1){
		return found; 
	}
	roi_ori.copyTo(roi);
	//cv::threshold(roi_ori, roi, 0, 255, CV_THRESH_OTSU);

	if (debug_level > 4){
		imshow("ImgOut3", tmpImg);
		cvWaitKey(1);
	}

	float b = boxinfo.center.y - k * boxinfo.center.x;

	LineInfo *lineInfo = new LineInfo;
	if (boxinfo.size.width < boxinfo.size.height){
		CalcRotatedRectPoints(&boxinfo, &k);
		b = boxinfo.center.y - k * boxinfo.center.x;
	}
	lineInfo->k = k;
	lineInfo->b = b;
	lineInfo->box = boxinfo;

	vector<vector<Point>> contours;
	cv::findContours(tmpImg, contours, noArray(), RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	CalContourInfo(contours, lineInfo);

	filterContour(contours, lineInfo, mGray.rows, mGray.cols);

	calContourType(lineInfo);

	checkLine(lineInfo);

	CalBoxInfo(lineInfo);

	if (debug_level > 4){
		tmpImg = 0 * tmpImg;
		if (lineInfo->box_info.size() > 0){
			vector<vector<Point>> cs;
			for (int i = 0; i < lineInfo->box_info.size(); i++){
				cs.push_back(lineInfo->box_info[i].contour);
			}
			drawContours(tmpImg, cs, -1, Scalar(255), CV_FILLED);
		}
		drawRotatedRect(tmpImg, lineInfo->box, 1, Scalar(255));
		imshow("ImgOut4", tmpImg);
		cvWaitKey(1);
	}

	if (lineInfoOwner.box_num_count > 0 && lineInfoNo.box_num_count > 0)
	{
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

bool Number::setTargetRegion(int type, Rect* rect, RotatedRect* box, float k, float b, int rows, int cols){
	Point newCenter;
	RotatedRect newBox;
	float new_k;
	double w, h;
	if (type == 12){
		newCenter.x = box->center.x + box->size.width * 1.8 + box->size.height * 1;
		newCenter.y = newCenter.x * k + b;
		w = box->size.width * 2.5;
		h = box->size.height;
	}
	else if (type == 21){
		newCenter.x = box->center.x - box->size.width * 1.5 - box->size.height * 1;
		newCenter.y = newCenter.x * k + b;
		w = box->size.width*1.0;
		h = box->size.height;
	}
	else if (type == 14){
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
		newCenter.x = box->center.x + box->size.width / 2 + box->size.height * 2 + 8;
		newCenter.y = newCenter.x * k + b;
		w = box->size.height * 4;
		h = box->size.height - 4;
	}
	else if (type == 11){
		newCenter.x = box->center.x - (box->size.width * 1 + box->size.width * 0.8 * 1.5); 
		newCenter.y = box->center.y;
		w = box->size.width * (4 + 0.8 * 3 + 1.2); 
		h = box->size.height;
	}
	else if (type == 15){
		newCenter.x = box->center.x;
		newCenter.y = box->center.y + box->size.height / 2 - mUheight * 2.5;
		w = box->size.width * 2;
		h = mUheight * 5;
	}
	else if (type == 16){
		newCenter.x = box->center.x;
		newCenter.y = box->center.y + box->size.height /2 + mUheight * 4 + 4;
		w = box->size.width * 2;
		h = mUheight * 8 - 8;
	}
	else if (type == 17){
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

	Point2f pt[4];
	newBox.points(pt);
	vector<Point> pts = { pt[0], pt[1], pt[2], pt[3] };
	Rect newRect = boundingRect(pts);

	newRect.x = (newRect.x < 8) ? newRect.x : newRect.x - 8;
	newRect.y = (newRect.y < 4) ? newRect.y : newRect.y - 4;
	newRect.width = newRect.width + 16;
	newRect.height = newRect.height + 8 ;

	*box = newBox;
	*rect = newRect;
	return true;
}

void Number::checkLine(LineInfo* lineInfo,int target, int orient){ 
	vector<boxInfo> bottom;
	if (orient == 0){  
		sortLineInfo(lineInfo, 0, 0);

		while (lineInfo->box_info.size()>1 && lineInfoNo.box_num_count == 0){
			if (target == 1 && lineInfo->box_info.begin()->box.center.x - lineInfo->box_info.begin()->box.size.width / 2 > mBox_u.center.x){
				lineInfo->box_1of2_count--;
				if (lineInfo->box_info.begin()->type > 0){
					lineInfo->box_num_count--;
				}
				lineInfo->box_info.erase(lineInfo->box_info.begin());
				continue;
			}
			else if (__max(lineInfo->box_info.begin()->box.size.height, lineInfo->box_info.begin()->box.size.width) < __min(12, lineInfo->box.size.height *0.7)){
				lineInfo->box_1of2_count--;
				if (lineInfo->box_info.begin()->type > 0){
					lineInfo->box_num_count--;
				}
				lineInfo->box_info.erase(lineInfo->box_info.begin());
				continue;
			}
			else if (lineInfo->box_info.size()>1
				&& (lineInfo->box_info.begin())->box.size.height > (lineInfo->box_info.begin() + 1)->box.size.height + 4){
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
			bottom.push_back(lineInfo->box_info[i]);
		}
		while (bottom.size() > 0 && lineInfoNo.box_num_count == 0){
			if (__max(bottom.begin()->box.size.height, bottom.begin()->box.size.width) < __min(15, lineInfo->box.size.height *0.7)){
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
				lineInfo->box_1of2_count--;
				if (bottom.begin()->type > 0){
					lineInfo->box_num_count--;
				}
				bottom.erase(bottom.begin());
				continue;
			}
			else if (bottom.size()>1
				&& (bottom.begin())->box.size.height > (bottom.begin() + 1)->box.size.height + 4){

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
		sortLineInfo(lineInfo, 0, 1);
		while (lineInfo->box_info.size()>1 && lineInfoNo.box_num_count == 0){
			if (target == 1 && lineInfo->box_info.begin()->box.center.y - lineInfo->box_info.begin()->box.size.height / 2 > mBox_u.center.y){
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
		for(int i = lineInfo->box_info.size()-1; i>=0;i--){
			if (abs((lineInfo->box_info.begin()+i)->box.center.x - mBox_u.center.x) > mUwidth * 1.1){
				lineInfo->box_1of2_count--;
				if ((lineInfo->box_info.begin()+i)->type > 0){
					lineInfo->box_num_count = lineInfo->box_num_count - (lineInfo->box_info.begin()+i)->type;
				}
				lineInfo->box_info.erase(lineInfo->box_info.begin()+i);
				continue;
			}
			else if (__max((lineInfo->box_info.begin()+i)->box.size.height, (lineInfo->box_info.begin()+i)->box.size.width) < __min(12, mUheight *0.6)){
				lineInfo->box_1of2_count--;
				if ((lineInfo->box_info.begin()+i)->type > 0){
					lineInfo->box_info.erase(lineInfo->box_info.begin() + i);
				}
				lineInfo->box_info.erase(lineInfo->box_info.begin()+i);
				continue;
			}
		}
		for (int i = lineInfo->box_info.size() - 1; i >= 0; i--){
			bottom.push_back(lineInfo->box_info[i]);
		}
		while (bottom.size() > 0 && lineInfoNo.box_num_count == 0){
			if (bottom.begin()->box.size.height < __min(13, mUheight *0.6)){
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

void Number::sortBoxInfo(vector<boxInfo>* pBoxinfos, int pos,int orient){
	vector<boxInfo> boxs = *pBoxinfos;
	vector<boxInfo> box_infos;
	if (orient == 0){
		for (int i = 0; i < boxs.size(); i++){
			boxs[i].idx = -1;
		}
		int cur_idx = 0;
		while (true){
			int max_idx = -1;
			int max_x = -1;
			for (int i = 0; i < boxs.size(); i++){
				if (boxs[i].idx < 0){
					int w = boxs[i].box.size.width / 2;
					int diff = (pos == 0) ? 0 : (pos == 1) ? (-1)*w : w;
					if (boxs[i].box.center.x + diff> max_x){
						max_x = boxs[i].box.center.x + diff;
						max_idx = i;
					}
				}
			}
			if (max_idx >= 0){
				boxs[max_idx].idx = cur_idx;
				box_infos.push_back(boxs[max_idx]);
				cur_idx++;
			}
			else{
				break;
			}
		}
	}
	else{
		for (int i = 0; i < boxs.size(); i++){
			boxs[i].idx = -1;
		}
		int cur_idx = 0;
		while (true){
			int max_idx = -1;
			int max_y = -1;
			for (int i = 0; i < boxs.size(); i++){
				if (boxs[i].idx < 0){
					int h = boxs[i].box.size.height / 2;
					int diff = (pos == 0) ? 0 : (pos == 1) ? (-1)*h : h;
					if (boxs[i].box.center.y + diff> max_y){
						max_y = boxs[i].box.center.y + diff;
						max_idx = i;
					}
				}
			}
			if (max_idx >= 0){
				boxs[max_idx].idx = cur_idx;
				box_infos.push_back(boxs[max_idx]);
				cur_idx++;
			}
			else{
				break;
			}
		}
	}
	*pBoxinfos = box_infos;
}

void Number::sortLineInfo(LineInfo* lineInfo, int pos,int orient){
	vector<boxInfo> boxs = lineInfo->box_info;
	sortBoxInfo(&boxs, pos,orient);
	lineInfo->box_info = boxs;
}

void Number::filterContour(vector<vector<Point>> pContours, LineInfo* lineInfo, int rows, int cols,int orient){
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
		float box_k;
		RotatedRect box = getRotatedRectFromPoints(*contour, &box_k); // minAreaRect(*contour);
		Point2f pt[4];
		box.points(pt);
		Rect rect = boundingRect(vector<Point>{ pt[0], pt[1], pt[2], pt[3] });
		if ((rect.height > rect.width && box.size.height < box.size.width)
			|| (rect.height < rect.width && box.size.height > box.size.width)){
			box = RotatedRect(box.center, Size(box.size.height, box.size.width), box.angle - 90);
		}

		if ((orient==0 && (box.size.width > 80 || box.size.height > 45)) ||
			(orient==1 && (box.size.width > 45 || box.size.height > 10*mUheight))){
			contour = contours.erase(contour);
			continue;
		}
		if (box.size.height < __min(mUheight>0?mUheight:999, 13)){
			contour = contours.erase(contour);
			continue;
		}
		if (box.size.height > 1.1*mUheight && box.size.width < mUwidth *0.3){
			contour = contours.erase(contour);
			continue;
		}		
		//if (box.size.height < 20 && box.size.width > box.size.height * 0.65){
		//	contour = contours.erase(contour);
		//	continue;
		//}
		if (__max(box.size.width, box.size.height) <  __min(mUheight>0 ? mUheight : 999, 13)){
			contour = contours.erase(contour);
			continue;
		}
		int w = box.size.width;
		int h = box.size.height;
		if (orient==0 && box.size.height < 50
			&& box.size.width > box.size.height * 1.1){
			vector<boxInfo> retboxs;
			boxInfo boxinfoTmp;
			boxinfoTmp.box = box;
			boxinfoTmp.contour = *contour;
			retboxs = breakContours(boxinfoTmp, 35, 1);
			if (retboxs.size() > 1 || (retboxs.size() == 1 && retboxs[0].box.size.width < box.size.width*0.9)){
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
				boxInfo boxinfoTmp2;
				boxinfoTmp2.box = box;
				boxinfoTmp2.contour = *contour;
				retboxs = breakContours(boxinfoTmp2, 35, 2);
				if (retboxs.size() > 1 || (retboxs.size() == 1 && retboxs[0].box.size.width < box.size.width*0.9)){
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

vector<boxInfo> Number::breakContours(boxInfo boxinfo, int minHeight, int target){
	vector<boxInfo> ret;

	RotatedRect box;
	box.center = boxinfo.box.center;
	box.size = boxinfo.box.size;
	box.angle = boxinfo.box.angle;

	//Rect rect = boxinfo.box.boundingRect();
	Rect rect = boundingRect(boxinfo.contour);
	int save_height = rect.height;
	Mat imgTmp = Mat(mGray.rows, mGray.cols, CV_8UC1);
	Mat roi, roi_gray;
	int diff = 0;
	int step = mUheight<16?4:2;
	boxInfo box_save;
	box_save.box = RotatedRect(Point(0, 0), Size(0, 0), 0.0);
	while (diff < __min(rect.height * 0.3 + 2, minHeight*1.5*0.25)){
		diff = diff + step;
		if (target == 1){
			rect.y = rect.y + step;
			//rect.height = rect.height - step;
		}
		else{
			rect.height = rect.height - step;
		}
		if (diff >= save_height * 1 / 3){
			diff = 999;
			continue;
		}
		vector<vector<Point>> v = { boxinfo.contour };
		imgTmp = 0 * imgTmp;
		drawContours(imgTmp, v, -1, Scalar(255), CV_FILLED);
		roi = CreateMat(imgTmp, rect);
		Mat tmp = Mat(mGray.rows, mGray.cols, CV_8UC1);
		tmp = 0 * tmp;
		Mat roiTmp = CreateMat(tmp, rect);
		roi.copyTo(roiTmp);
		if (debug_level > 9){
			imshow("Img", tmp);
			cvWaitKey(1);
			cvWaitKey(1);
		}

		vector<vector<Point>> contours;
		cv::findContours(tmp, contours, noArray(), RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
		if (contours.size() == 0){
			break;
		}
		RotatedRect b = minAreaRect(*(contours.begin()));
		float k;
		CalcRotatedRectPoints(&b, &k);
		if (contours.size() > 1){
			float box_start = -1;
			float box_end = 999;
			float box_width = 0;
			while (contours.size() > 0){
				b = minAreaRect(*(contours.begin()));
				CalcRotatedRectPoints(&b, &k);
				if (b.size.height <= __max(boxinfo.box.size.height * 0.2, 3) || (b.size.width <= __max(boxinfo.box.size.height * 0.2, 3) && b.size.height <= 7)){
					contours.erase((contours.begin()));
					continue;
				}
				boxInfo retbox;
				retbox.box = b;
				//retbox.box.size.height = boxinfo.box.size.height;
				retbox.contour = *(contours.begin());
				retbox.k = k;
				if (target == 1 && retbox.box.size.height<12){
					retbox.box.center.y = retbox.box.center.y - diff / 2;
					retbox.box.size.height = retbox.box.size.height + diff / 2;
					Point2f pt[4];
					retbox.box.points(pt);
					vector<Point> pt2 = { Point(pt[0].x, pt[0].y), Point(pt[1].x, pt[1].y), Point(pt[2].x, pt[2].y), Point(pt[3].x, pt[3].y) };
					retbox.contour.insert(retbox.contour.end(), pt2.begin(), pt2.end());
				}
				else if (target == 2 && retbox.box.size.height<25 && retbox.box.size.height>7){
					retbox.box.center.y = retbox.box.center.y + diff / 2;
					retbox.box.size.height = retbox.box.size.height + diff / 2;
					Point2f pt[4];
					retbox.box.points(pt);
					vector<Point> pt2 = { Point(pt[0].x, pt[0].y), Point(pt[1].x, pt[1].y), Point(pt[2].x, pt[2].y), Point(pt[3].x, pt[3].y) };
					retbox.contour.insert(retbox.contour.end(), pt2.begin(), pt2.end());
				}
				else{
					retbox.box.size.height = retbox.box.size.height - diff / 2;
				}
				if (retbox.box.size.width > retbox.box.size.height * 0.8){
					retbox.type = 5;
				}
				else{
					retbox.type = 1;
				}
				box_width = box_width + retbox.box.size.width;
				if (box_start == -1 || box_start > retbox.box.center.x - retbox.box.size.width / 2){
					box_start = retbox.box.center.x - retbox.box.size.width / 2;
				}
				if (box_end == 999 || box_end < retbox.box.center.x + retbox.box.size.width / 2){
					box_end = retbox.box.center.x + retbox.box.size.width / 2;
				}
				ret.push_back(retbox);
				contours.erase((contours.begin()));
			}
			if (ret.size() == 0){
				diff = 9999;  
			}
			if (ret.size() == 1){
				if (box_save.box.size.width == 0 && box_width >(box_end - box_start) * 0.8 && (box_end - box_start > boxinfo.box.size.width*0.9 || boxinfo.box.size.width - (box_end - box_start) <= 4)){
					ret.clear();
				}
				else if (box_save.box.size.width > 0 && box_width > (box_end - box_start) * 0.8 && (box_end - box_start > box_save.box.size.width*0.9 || boxinfo.box.size.width - (box_end - box_start) <= 4)){
					ret.clear();
				}
				else {
					box_save = ret[0];
					ret.clear();
				}
			}
			else{
				return ret;
			}
		}
		else if (box_save.box.size.width == 0 && b.size.width <= boxinfo.box.size.width*0.9 && boxinfo.box.size.width - b.size.width> 3){
			boxInfo retbox;
			b.size.height = b.size.height + diff / 2;
			retbox.box = b;
			//retbox.box.size.height = boxinfo.box.size.height;
			retbox.contour = *(contours.begin());
			retbox.k = k;
			retbox.type = 5;
			box_save = retbox;
		}
		else if (box_save.box.size.width > 0 && b.size.width <= box_save.box.size.width*0.9 && box_save.box.size.width - b.size.width > 3){
			boxInfo retbox;
			b.size.height = b.size.height + diff / 2;
			retbox.box = b;
			//retbox.box.size.height = boxinfo.box.size.height;
			retbox.contour = *(contours.begin());
			retbox.k = k;
			retbox.type = 5;
			box_save = retbox;
		}
		//else{
		//	box_save = boxinfo;
		//	break;
		//}
	}
	if (box_save.box.size.width >0){
		if (target == 1 && box_save.box.size.height < 13){
			Point2f pt[4];
			box_save.box.points(pt);
			vector<Point> pt2 = { Point(pt[0].x, pt[0].y), Point(pt[1].x, pt[1].y), Point(pt[2].x, pt[2].y), Point(pt[3].x, pt[3].y) };
			box_save.contour.insert(box_save.contour.end(), pt2.begin(), pt2.end());
		}
		//else if (target == 2 && box_save.box.size.height < 25){
		//	Point2f pt[4];
		//	box_save.box.points(pt);
		//	vector<Point> pt2 = { Point(pt[0].x, pt[0].y), Point(pt[1].x, pt[1].y), Point(pt[2].x, pt[2].y), Point(pt[3].x, pt[3].y) };
		//	box_save.contour.insert(box_save.contour.end(), pt2.begin(), pt2.end());
		//}
		ret.push_back(box_save);
	}
	else{
		ret.push_back(boxinfo);
	}
	return ret;
}

void Number::CalContourInfo(vector<vector<Point>> contours, LineInfo* lineInfo){
	RotatedRect newbox;
	float k, b;
	vector<Point> allPoints;
	for (int i = 0; i < contours.size(); i++){
		allPoints.insert(allPoints.end(), contours[i].begin(), contours[i].end());
	}
	if (allPoints.size()>0){
		newbox = getRotatedRectFromPoints(allPoints, &k);
		b = newbox.center.y - k * newbox.center.x;
		Point2f pt[4];
		newbox.points(pt);
		Rect rect = boundingRect(vector<Point>{ pt[0], pt[1], pt[2], pt[3] });
		if ((rect.height > rect.width && newbox.size.height < newbox.size.width)
			|| (rect.height < rect.width && newbox.size.height > newbox.size.width)){
			newbox = RotatedRect(newbox.center, Size(newbox.size.height, newbox.size.width), newbox.angle - 90);
		}
	}
	else{
		newbox = RotatedRect(Point(0, 0), Size(0, 0), 0.0);
		k = 0;
		b = 0;
	}
	lineInfo->k = k;
	lineInfo->b = b;
	lineInfo->box = newbox;
}

void Number::CalcRotatedRectPoints(RotatedRect* box, float* k)
{
	vector<Point2f> pt;
	pt = orderRotatedRectPoint(*box);
	if (pt.size() == 0){
		return;
	}

	float w = sqrt(pow(pt[1].x - pt[0].x, 2) + pow((pt[1].y - pt[0].y), 2));
	float h = sqrt(pow(pt[3].x - pt[0].x, 2) + pow((pt[3].y - pt[0].y), 2));
	float angle = 0.0;
	float kValue = 0.0;
	if (pt[0].x == pt[1].x)
	{
		kValue = 99999;        
		angle = 90;
	}
	else
	{
		kValue = (double)(pt[1].y - pt[0].y);
		kValue = kValue / (double)(pt[1].x - pt[0].x);
		angle = atan(kValue) / 3.1415 * 180;
		//angle = fmod(180.0 - angle, 180);
	}
	box->size = Size(w, h);
	box->angle = angle;
	//RotatedRect ret_box = RotatedRect(box->center, Size(w, h), angle);
	//*box = ret_box;
	*k = kValue;
}

vector<Point2f> Number::orderRotatedRectPoint(RotatedRect rect){
	try{
		Point2f pt[4];
		rect.points(pt);
		if (rect.center.x < 0 || rect.center.y < 0){
			vector<Point2f> pts;
			return pts;
		}
		int max_x_idx[4];
		float max_x = -99.0;
		for (int i = 0; i < 4; i++){
			if (pt[i].x > max_x){
				max_x = pt[i].x;
				max_x_idx[3] = i;
			}
		}
		max_x = -99;
		for (int i = 0; i < 4; i++){
			if (i != max_x_idx[3] && pt[i].x > max_x){
				max_x = pt[i].x;
				max_x_idx[2] = i;
			}
		}
		max_x = -99.0;
		for (int i = 0; i < 4; i++){
			if (i != max_x_idx[3] && i != max_x_idx[2]){
				max_x_idx[1] = i;
				break;
			}
		}
		for (int i = 0; i < 4; i++){
			if (i != max_x_idx[3] && i != max_x_idx[2] && i != max_x_idx[1]){
				max_x_idx[0] = i;
				break;
			}
		}
		Point2f p[4];
		if (pt[max_x_idx[0]].y < pt[max_x_idx[1]].y){
			p[0] = pt[max_x_idx[1]];
			p[1] = pt[max_x_idx[0]];
		}
		else{
			p[0] = pt[max_x_idx[0]];
			p[1] = pt[max_x_idx[1]];
		}
		if (pt[max_x_idx[2]].y < pt[max_x_idx[3]].y){
			p[2] = pt[max_x_idx[2]];
			p[3] = pt[max_x_idx[3]];
		}
		else{
			p[2] = pt[max_x_idx[3]];
			p[3] = pt[max_x_idx[2]];
		}

		vector<Point2f> pts;
		float w = __max(p[2].x, p[3].x) - __min(p[0].x, p[1].x);
		if ((__max(p[2].y, p[3].y) - __min(p[1].y, p[2].y))>w && p[1].y > p[2].y && p[1].y > p[3].y){
			pts.push_back(p[2]);
			pts.push_back(p[3]);
			pts.push_back(p[0]);
			pts.push_back(p[1]);
		}
		else if ((__max(p[2].y, p[3].y) - __min(p[1].y, p[2].y))>w && p[0].y < p[2].y && p[0].y < p[3].y){
			//pts.push_back(p[0]);
			//pts.push_back(p[1]);
			//pts.push_back(p[2]);
			//pts.push_back(p[3]);
			pts.push_back(p[1]);
			pts.push_back(p[2]);
			pts.push_back(p[3]);
			pts.push_back(p[0]);
		}
		else{
			pts.push_back(p[1]);
			pts.push_back(p[2]);
			pts.push_back(p[3]);
			pts.push_back(p[0]);
		}
		return pts;
	}
	catch (cv::Exception exp){
		vector<Point2f> pts;
		return pts;
	}
}

void Number::drawRotatedRect(Mat srcImg, RotatedRect box, int thickness, CvScalar color){
	if (box.size.width > 0){
		Point2f pt[4];
		box.points(pt);
		line(srcImg, pt[0], pt[1], color, thickness);
		line(srcImg, pt[1], pt[2], color, thickness);
		line(srcImg, pt[2], pt[3], color, thickness);
		line(srcImg, pt[3], pt[0], color, thickness);
	}
}

void Number::calContourType(LineInfo* lineInfo,int target,int orient){
	lineInfo->box_num_count = 0;

	vector<boxInfo> boxinfos = lineInfo->box_info;
	RotatedRect newbox = lineInfo->box;
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
			if (box.size.height > 12 && box.size.height < 35 && box.size.width < mUwidth * 1.6){
				lineInfo->box_num_count++;
				c->type = 1;
			}
			else if (box.size.height > 12 && box.size.height < 35 && box.size.width < mUwidth * 2 * 1.3){
				lineInfo->box_num_count++;
				lineInfo->box_num_count++;
				c->type = 2;
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
			lineInfo->box_num_count = lineInfo->box_num_count +boxtype;
			c->type = boxtype;
		}
		c++;
	}
	lineInfo->box_info = boxinfos;
	lineInfo->box_1of2_count = boxinfos.size();
}

void Number::CalBoxInfo(LineInfo* lineInfo){
	vector<boxInfo> boxinfos = lineInfo->box_info;
	RotatedRect newbox;
	float k, b;
	vector<Point> allPoints;
	for (int i = 0; i < boxinfos.size(); i++){
		allPoints.insert(allPoints.end(), boxinfos[i].contour.begin(), boxinfos[i].contour.end());
	}
	if (allPoints.size()>0){
		newbox = getRotatedRectFromPoints(allPoints, &k);
		b = newbox.center.y - k * newbox.center.x;
		Point2f pt[4];
		newbox.points(pt);
		Rect rect = boundingRect(vector<Point>{ pt[0], pt[1], pt[2], pt[3] });
		if ((rect.height > rect.width && newbox.size.height < newbox.size.width)
			|| (rect.height < rect.width && newbox.size.height > newbox.size.width)){
			newbox = RotatedRect(newbox.center, Size(newbox.size.height, newbox.size.width), newbox.angle - 90);
		}
	}
	else{
		newbox = RotatedRect(Point(0, 0), Size(0, 0), 0.0);
		k = 0;
		b = 0;
	}
	lineInfo->box = newbox;
	lineInfo->k = k;
	lineInfo->b = b;
}

RotatedRect Number::getRotatedRectFromPoints(vector<Point> pts, float* k){
	RotatedRect rect_out;
	if (pts.size()>0){
		RotatedRect box = minAreaRect(pts);
		CalcRotatedRectPoints(&box, k);
		rect_out = box;
	}
	else{
		rect_out.center = Point(0, 0);
		rect_out.size = Size(0, 0);
		rect_out.angle = 0;
	}
	return rect_out;
}

Mat Number::warpImage(Mat srcImage, Point2f* srcPts, Point2f* dstPts, int warpWidth, int warpHeight)
{
	Mat retImage;
	Mat m = getPerspectiveTransform(srcPts, dstPts);
	warpPerspective(srcImage, retImage, m, Size(warpWidth, warpHeight), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar());
	return retImage;
}

Mat Number::CreateMat(Mat src, Rect rect, bool isReturnErr){
	try{
		if (rect.x < 2) rect.x = 1;
		if (rect.y < 2) rect.y = 1;
		if (rect.x > src.cols - 2) rect.x = src.cols - 2;
		if (rect.y > src.rows - 2) rect.y = src.rows - 2;
		if (rect.x + rect.width > src.cols - 1) rect.width = src.cols - rect.x - 1;
		if (rect.y + rect.height > src.rows - 1) rect.height = src.rows - rect.y - 1;
		return Mat(src, rect);
	}
	catch (cv::Exception exp){
		return Mat(1, 1, CV_8UC1);
	}
}

void Number::outputInfo(RotatedRect rect, string filename){
	vector<Point2f> pt = orderRotatedRectPoint(rect);

	//float h = getDistance(pt[0], pt[3]) * 2;
	//float w = getDistance(pt[0], pt[1]) * 2;
	float h = sqrtf(powf((pt[0].x - pt[3].x), 2) + powf((pt[0].y - pt[3].y), 2)) * 2;
	float w = sqrtf(powf((pt[0].x - pt[1].x), 2) + powf((pt[0].y - pt[1].y), 2)) * 2;

	Point2f srcPts[4], dstPts[4];
	dstPts[0].x = 0;
	dstPts[0].y = 0;
	dstPts[1].x = (float)(w);
	dstPts[1].y = 0;
	dstPts[2].x = (float)(w);
	dstPts[2].y = (float)(h);
	dstPts[3].x = 0;
	dstPts[3].y = (float)(h);
	for (int i = 0; i < 4; i++){
		srcPts[i].x = pt[i].x;
		srcPts[i].y = pt[i].y;
	}
	Mat out = warpImage(mGray, srcPts, dstPts, w, h);
	cv::threshold(out, out, 0, 255, CV_THRESH_OTSU);
	out = 255 - out;
	imwrite(filepath + filename, out);
}

Mat Number::getWarpImage(Mat img,RotatedRect rect,int diffx,int diffy){
	if (rect.size.width < 3 && diffx < 0){
		diffx = 6 - rect.size.width;
	}
	rect.size.width = rect.size.width + diffx;
	rect.size.height = rect.size.height + diffy;

	//Point2f ptTmp[4], pt[4];
	//rect.points(ptTmp);
	//pt[0] = ptTmp[1];
	//pt[1] = ptTmp[2];
	//pt[2] = ptTmp[3];
	//pt[3] = ptTmp[0];
	vector<Point2f> pt = orderRotatedRectPoint(rect);

	//float h = getDistance(pt[0], pt[3]) * 2;
	//float w = getDistance(pt[0], pt[1]) * 2;
	//float h = sqrtf(powf((pt[0].x - pt[3].x), 2) + powf((pt[0].y - pt[3].y), 2));
	//float w = sqrtf(powf((pt[0].x - pt[1].x), 2) + powf((pt[0].y - pt[1].y), 2));
	float h = sqrtf(powf((pt[0].x - pt[3].x), 2) + powf((pt[0].y - pt[3].y), 2)) * 2;
	float w = sqrtf(powf((pt[0].x - pt[1].x), 2) + powf((pt[0].y - pt[1].y), 2)) * 2;

	Point2f srcPts[4], dstPts[4];
	dstPts[0].x = 0;
	dstPts[0].y = 0;
	dstPts[1].x = (float)(w);
	dstPts[1].y = 0;
	dstPts[2].x = (float)(w);
	dstPts[2].y = (float)(h);
	dstPts[3].x = 0;
	dstPts[3].y = (float)(h);
	for (int i = 0; i < 4; i++){
		srcPts[i].x = pt[i].x;
		srcPts[i].y = pt[i].y;
	}
	Mat out = warpImage(img, srcPts, dstPts, w, h);
	cv::threshold(out, out, 0, 255, CV_THRESH_OTSU);
	////cv::threshold(out, out, mThresh, 255, CV_THRESH_BINARY);
	out = 255 - out;
	return out;
}

void Number::threshRoi(Mat fromImg, Mat* toImg, RotatedRect box){

	Point2f pt[4];
	box.points(pt);
	Rect rect = boundingRect(vector<Point>{ pt[0], pt[1], pt[2], pt[3] });
	Mat roi = CreateMat(*toImg, rect);
	Mat roi_ori = CreateMat(mGray, rect);
	threshold(roi_ori, roi, 0, 255, CV_THRESH_OTSU);

}

void Number::recognizeInfo(){
	//numberMlp = new NumberCnnMlp();

	//Mat threshImg = Mat(mGray.rows, mGray.cols, CV_8UC1);
	//threshImg = 0 * threshImg;

	//threshRoi(mGray, &threshImg, lineInfoOwner.box);
	//if (lineInfoNo1.box_num_count > 0){
	//	threshRoi(mGray, &threshImg, lineInfoNo1.box);
	//}
	//threshRoi(mGray, &threshImg, lineInfoNo.box);
	//threshRoi(mGray, &threshImg, lineInfoCheckDigit.box);

	//Mat threshImg = Mat(mGray.rows,mGray.cols,CV_8UC1);
	//cv::threshold(mGray, threshImg, 0, 255, CV_THRESH_OTSU);


	Mat threshImg = mGray;
	mOcr = Mat(mUheight*2 + 20, (mUwidth*2 + 20) * 12, CV_8UC1);
	//mOcr = Mat(mUheight  + 20, (mUwidth  + 20) * 12, CV_8UC1);
	mOcr = 255 - 0 * mOcr;
	ownerTxt = "";
	int idx = 0;
	int orient = lineInfoOwner.box.size.width > lineInfoOwner.box.size.height ? 0 : 1;

	for (int i = 0; i< lineInfoOwner.box_info.size(); i++)
	{
		boxInfo boxinfo = lineInfoOwner.box_info[i];
		//Mat charImg = getWarpImage(mGray, boxinfo.box);
		//ownerTxt = ownerTxt + ocrProcess(charImg, idx, boxinfo.box, 0);
		//idx = idx + (boxinfo.type == 0) ? 1 : boxinfo.type;

		for (int j = 0; j < boxinfo.type; j++){
			double newX, newY;
			float w, h;
			if (orient == 0){
				newX = boxinfo.box.center.x - boxinfo.box.size.width / 2 + boxinfo.box.size.width / boxinfo.type *(j + 0.5);
				newY = boxinfo.box.center.y;
				w = boxinfo.box.size.width / boxinfo.type;
				h = boxinfo.box.size.height;
				if (boxinfo.type <= 1){
					w = w + 2;
					h = h + 2;
				}
				else{
					w = w - 1;
					h = h - 1;
				}
			}
			else{
				newX = boxinfo.box.center.x;
				newY = boxinfo.box.center.y - boxinfo.box.size.height / 2 + boxinfo.box.size.height / boxinfo.type *(j + 0.5);
				w = boxinfo.box.size.width;
				h = boxinfo.box.size.height / boxinfo.type;
				w = w + 6;
				h = h - 2;
			}
			RotatedRect b = RotatedRect(Point(newX, newY), Size(w, h), boxinfo.box.angle);
			Mat charImg = getWarpImage(threshImg, b);
			string txt = ocrProcess(charImg, idx, b, 1);
			//numberTxt = numberTxt + txt;
			idx = idx + 1;
		}
	}

	Mat owner = getWarpImage(threshImg, lineInfoOwner.box, 4, 2);
	Mat ownerOcr = Mat(owner.rows + 20, owner.cols + 20, CV_8UC1);
	Mat ownerRoi = CreateMat(ownerOcr, Rect(10, 10, owner.cols, owner.rows));
	owner.copyTo(ownerRoi);
	ownerTxt = readText(ownerOcr, 0);


	numberTxt = "";
	if (lineInfoNo1.box_num_count > 0){
		//sortLineInfo(&lineInfoNo1, 0, orient);
		for (int i = 0; i < lineInfoNo1.box_info.size(); i++)
		{
			boxInfo boxinfo = lineInfoNo1.box_info[i];
			//Mat charImg = getWarpImage(mGray, boxinfo.box);
			//numberTxt = numberTxt + ocrProcess(charImg, idx, boxinfo.box, 1);
			//idx = idx + (boxinfo.type == 0) ? 1 : boxinfo.type;

			for (int j = 0; j < boxinfo.type; j++){
				double newX, newY;
				float w, h;
				if (orient == 0){
					newX = boxinfo.box.center.x - boxinfo.box.size.width / 2 + boxinfo.box.size.width / boxinfo.type *(j + 0.5);
					newY = boxinfo.box.center.y;
					w = boxinfo.box.size.width / boxinfo.type;
					h = boxinfo.box.size.height;
					if (boxinfo.type <= 1){
						w = w + 4;
						h = h + 4;
					}
					else{
						w = w - 1;
						h = h - 1;
					}
				}
				else{
					newX = boxinfo.box.center.x;
					newY = boxinfo.box.center.y - boxinfo.box.size.height / 2 + boxinfo.box.size.height / boxinfo.type *(j + 0.5);
					w = boxinfo.box.size.width;
					h = boxinfo.box.size.height / boxinfo.type;
					w = w + 8;
					h = h - 2;
				}
				RotatedRect b = RotatedRect(Point(newX, newY), Size(w, h), boxinfo.box.angle);
				Mat charImg = getWarpImage(threshImg, b);
				string txt = ocrProcess(charImg, idx, b, 1);
				if (orient == 1){
					numberTxt = numberTxt + txt;
				}
				idx = idx + 1;
			}
		}
		if (orient == 0){
			Mat imgNo1 = getWarpImage(mGray, lineInfoNo1.box, 4, 2);
			Mat no1Ocr = Mat(imgNo1.rows + 20, imgNo1.cols + 20, CV_8UC1);
			Mat no1Roi = CreateMat(no1Ocr, Rect(10, 10, imgNo1.cols, imgNo1.rows));
			imgNo1.copyTo(no1Roi);
			ownerTxt = readText(no1Ocr, 1);
		}
	}
	//sortLineInfo(&lineInfoNo, 0, orient);
	for (int i = 0; i < lineInfoNo.box_info.size(); i++)
	{
		boxInfo boxinfo = lineInfoNo.box_info[i];
		//Mat charImg = getWarpImage(mGray, boxinfo.box);
		//numberTxt = numberTxt + ocrProcess(charImg, idx, boxinfo.box, 1);
		//idx = idx + (boxinfo.type == 0) ? 1 : boxinfo.type;

		for (int j = 0; j < boxinfo.type; j++){
			double newX, newY;
			float w, h;
			if (orient == 0){
				newX = boxinfo.box.center.x - boxinfo.box.size.width / 2 + boxinfo.box.size.width / boxinfo.type *(j + 0.5);
				newY = boxinfo.box.center.y;
				w = boxinfo.box.size.width / boxinfo.type;
				h = boxinfo.box.size.height;
				if (boxinfo.type <= 1){
					w = w + 1;
					h = h + 1;
				}
				else{
					w = w - 1;
					h = h - 1;
				}
			}
			else{
				newX = boxinfo.box.center.x;
				newY = boxinfo.box.center.y - boxinfo.box.size.height / 2 + boxinfo.box.size.height / boxinfo.type *(j + 0.5);
				w = boxinfo.box.size.width;
				h = boxinfo.box.size.height / boxinfo.type;
				w = w + 8;
				h = h - 2;
			}
			RotatedRect b = RotatedRect(Point(newX, newY), Size(w, h), boxinfo.box.angle);
			Mat charImg = getWarpImage(threshImg, b);
			string txt = ocrProcess(charImg, idx, b, 1);
			if (orient == 1){
				numberTxt = numberTxt + txt;
			}
			idx = idx + 1;
		}
	}
	if (orient == 0){
		Mat imgNo = getWarpImage(threshImg, lineInfoNo.box, 4, 2);
		Mat noOcr = Mat(imgNo.rows + 20, imgNo.cols + 20, CV_8UC1);
		Mat noRoi = CreateMat(noOcr, Rect(10, 10, imgNo.cols, imgNo.rows));
		imgNo.copyTo(noRoi);
		numberTxt = numberTxt+readText(noOcr, 1);
	}

	boxInfo boxinfo = lineInfoCheckDigit.box_info[0];
	int diff = -4;
	if (boxinfo.box.size.width < mUwidth * 0.8){
		diff = 6;
	}
	Mat checkImg = getWarpImage(threshImg, boxinfo.box, diff, -2);
	Mat checkOcr = Mat(checkImg.rows + 20, checkImg.cols + 20, CV_8UC1);
	Mat checkRoi = CreateMat(checkOcr, Rect(10, 10, checkImg.cols, checkImg.rows));
	checkImg.copyTo(checkRoi);
	string txt = ocrProcess(checkImg, idx, boxinfo.box, 1);
	checkDigitTxt = readText(checkOcr, 1);
	
	cv::imwrite(filepath + "\\out.jpg", mOcr);
	//delete numberMlp;

	return;

}

string Number::ocrProcess(Mat in, int idx,RotatedRect box,int type){
	Rect rect = Rect((int)((mUwidth*2 + 20)*idx + mUwidth - in.cols/2 + 10), (int)(mUheight - in.rows/2 + 10), in.cols, in.rows);
	Rect rectOcr = Rect((int)((mUwidth*2 + 20)*idx) + 1, 1, (int)(mUwidth*2 + 20) - 2, (int)(mUheight*2 + 20) - 2);
	Rect rectMlp = Rect((int)((mUwidth*2 + 20)*idx + mUwidth / 2 - in.cols / 2 + 10), (int)(mUheight - in.rows / 2 + 10), in.cols+5, in.rows+5);
	//Rect rect = Rect((int)((mUwidth + 20)*idx + mUwidth/2 - in.cols / 2 + 10), (int)(mUheight/2 - in.rows / 2 + 10), in.cols, in.rows);
	//Rect rectOcr = Rect((int)((mUwidth + 20)*idx) + 1, 1, (int)(mUwidth + 20) - 2, (int)(mUheight + 20) - 2);
	//Rect rectMlp = Rect((int)((mUwidth + 20)*idx + mUwidth / 2 - in.cols / 2 + 10), (int)(mUheight / 2 - in.rows / 2 + 10), in.cols+5, in.rows+5);
	Mat ocrBaseRoi = CreateMat(mOcr, rect);
	in.copyTo(ocrBaseRoi);
	Mat ocrRoi = CreateMat(mOcr, rectOcr);
	Mat mlpRoi = CreateMat(mOcr, rectMlp);
	//imshow("tmp", mOcr);
	//waitKey(100);
	//imshow("tmp2", in);
	//waitKey(200);

	string ocrTxt = "";
	if (type == 0){
		//imshow("tmp", ocrRoi);
		//waitKey(1);
		//ocrTxt = readText(ocrRoi, type);
	}
	else{
		//imshow("tmp2", mlpRoi);
		//waitKey(1);
		//ocrTxt = readTextMlp(mlpRoi, type);
		ocrTxt = readText(ocrRoi, type);
	}
	return ocrTxt;
}

void Number::writePicture(){

	recognizeInfo();
	return;
	//outputInfo(lineInfoOwner.box, "\\Owner.jpg");
	//for (int i = 0; i<lineInfoOwner.box_info.size(); i++)
	//{
	//	outputInfo(lineInfoOwner.box_info[i].box, "\\Owner_" + to_string(i) + ".jpg");
	//}
	//if (lineInfoNo1.box_num_count == 0){
	//	outputInfo(lineInfoNo.box, "\\Number.jpg");
	//	for (int i = 0; i<lineInfoNo.box_info.size(); i++)
	//	{
	//		outputInfo(lineInfoNo.box_info[i].box, "\\Nubmer_" + to_string(i) + ".jpg");
	//	}
	//}
	//else{
	//	outputInfo(lineInfoNo1.box, "\\Number1.jpg");
	//	outputInfo(lineInfoNo.box, "\\Number2.jpg");
	//	for (int i = 0; i<lineInfoNo1.box_info.size(); i++)
	//	{
	//		outputInfo(lineInfoNo.box_info[i].box, "\\Nubmer_" + to_string(i) + ".jpg");
	//	}
	//	for (int i = 0; i<lineInfoNo.box_info.size(); i++)
	//	{
	//		outputInfo(lineInfoNo.box_info[i].box, "\\Nubmer_" + to_string(i + lineInfoNo1.box_info.size()) + ".jpg");
	//	}
	//
	//}
	//outputInfo(lineInfoCheckDigit.box, "\\CheckDigit.jpg");
}

String Number::readText(Mat img, int type){
	//const char* tessdata = "C:\\Users\\oczha\\Documents\\Visual Studio 2013\\Projects\\jzx\\Debug\\";  
	//Pix *image = pixRead(file);
	//tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
	//api->Init(NULL, "eng");
	//api->Init(tessdata, "eng");
	//api->SetVariable("tessedit_char_whitelist", "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ ");
	//api->SetImage(image);
	//Boxa* boxes = api->GetComponentImages(tesseract::RIL_TEXTLINE, true, NULL, NULL);
	//printf("Found %d textline image components.\n", boxes->n);
	//for (int i = 0; i < boxes->n; i++){
	//	BOX* box = boxaGetBox(boxes, i, L_CLONE);
	//	api->SetRectangle(box->x, box->y, box->w, box->h);
	//	char* ocrResult = api->GetUTF8Text();
	//	int conf = api->MeanTextConf();
	//	fprintf(stdout, "Box[%d]: x=%d, y=%d, w=%d, h=%d, confidence: %d, text: %s",
	//		i, box->x, box->y, box->w, box->h, conf, ocrResult);
	//}

	char* whitelist_all = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
	char* whitelist_number = "0123456789 ";
	char* whitelist_char = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
	//imshow("tmp", img);
	//waitKey(1);
	ocrApi->SetPageSegMode(tesseract::PSM_SINGLE_BLOCK);
	ocrApi->SetImage(img.data, img.cols, img.rows, 1, img.step);
	if (type == 0){
		ocrApi->SetVariable("tessedit_char_whitelist", whitelist_char);
	}
	else if (type == 1){
		ocrApi->SetVariable("tessedit_char_whitelist", whitelist_number);
	}
	else{
		ocrApi->SetVariable("tessedit_char_whitelist", whitelist_all);
	}
	String ocrResult = ocrApi->GetUTF8Text();
	ocrResult = ocrResult.substr(0, ocrResult.length() - 2);
	int conf = ocrApi->MeanTextConf();
	//fprintf(stdout, "confidence: %d, text: %s", conf, ocrResult);
	return (ocrResult == "?")?"?":ocrResult;

	////ocr.SetVariable("tessedit_char_whitelist", "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ ");
	//ocrApi->SetImage(img.data, img.cols, img.rows, 1, img.step);
	//Boxa* boxes = ocrApi->GetComponentImages(tesseract::RIL_TEXTLINE, true, NULL, NULL);
	//printf("Found %d textline image components.\n", boxes->n);
	//for (int i = 0; i < boxes->n; i++){
	//	BOX* box = boxaGetBox(boxes, i, L_CLONE);
	//	ocrApi->SetRectangle(box->x, box->y, box->w, box->h);
	//	char* ocrResult = ocrApi->GetUTF8Text();
	//	int conf = ocrApi->MeanTextConf();
	//	fprintf(stdout, "Box[%d]: x=%d, y=%d, w=%d, h=%d, confidence: %d, text: %s",
	//		i, box->x, box->y, box->w, box->h, conf, ocrResult);
	//}

}

//String Number::readTextMlp(Mat img, int type){
//	double mse = 0;
//	String ret = "?";
//
//	MlpResult& mlp = numberMlp->Recognize(&IplImage(img));
//	if (mlp.isOk) {
//		ret = (mlp.text == "") ? "?" : mlp.text;
//		mse = mlp.mse;
//	}
//	return ret;
//}