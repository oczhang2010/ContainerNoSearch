#include "stdafx.h"
#include "detectJzx.h"

FindContent::FindContent(){}
FindContent::~FindContent(){}

bool FindContent::search(){
	//mUheight = boxU.size.height;
	//mUwidth = boxU.size.width;
	RotatedRect box2 = mBox_u;
	Rect rect2;
	float k = mBox_k;
	Mat img = mGray;
	float b = 0;
	if (!setTargetRegion(11, &rect2, &box2, k, b, img.rows, img.cols)){
		return false;
	}

	if (rect2.x + rect2.width>img.cols - 5 || rect2.x< 5
		|| rect2.y + rect2.height>img.rows - 3 || rect2.y < 3){
		return false;
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
			return false;
		}
		ret = findNoInfo(rect2, box2, k);
		if (ret == 2){
			box2 = lineInfoNo.box;
			if (!setTargetRegion(23, &rect2, &box2, lineInfoNo.k, lineInfoNo.b, img.rows, img.cols)){
				return false;
			}
			if (findCheckDigitInfo(rect2, box2, k) == 3){
				return true;
			}
		}
		box2 = lineInfoOwner.box;
		if (!setTargetRegion(14, &rect2, &box2, lineInfoNo.k, lineInfoNo.b, img.rows, img.cols)){
			return false;
		}
		ret = findNoInfo(rect2, box2, k);
		if (ret == 2){
			box2 = lineInfoNo.box;
			if (!setTargetRegion(23, &rect2, &box2, lineInfoNo.k, lineInfoNo.b, img.rows, img.cols)){
				return false;
			}
			if (findCheckDigitInfo(rect2, box2, k) == 3){
				return true;
			}
		}
		else if (ret == 20){
			box2 = lineInfoNo1.box;
			if (!setTargetRegion(14, &rect2, &box2, lineInfoNo.k, lineInfoNo.b, img.rows, img.cols)){
				return false;
			}
			if (findNoInfo(rect2, box2, k) == 2){
				box2 = lineInfoNo.box;
				if (!setTargetRegion(23, &rect2, &box2, lineInfoNo.k, lineInfoNo.b, img.rows, img.cols)){
					return false;
				}
				if (findCheckDigitInfo(rect2, box2, k) == 3){
					return true;
				}
			}
		}
	}
	box2 = mBox_u;
	if (!setTargetRegion(15, &rect2, &box2, k, b, img.rows, img.cols)){
		return false;
	}
	ret = findOwnerInfo(rect2, box2, k, 0, 1);
	if (ret == 1){
		box2 = lineInfoOwner.box;
		if (!setTargetRegion(16, &rect2, &box2, lineInfoOwner.k, lineInfoOwner.b, img.rows, img.cols)){
			return false;
		}
		ret = findNoInfo(rect2, box2, k, 0, 1);
		if (ret == 2){
			box2 = lineInfoNo.box;
			if (!setTargetRegion(17, &rect2, &box2, lineInfoNo.k, lineInfoNo.b, img.rows, img.cols)){
				return false;
			}
			if (findCheckDigitInfo(rect2, box2, k, 0, 1) == 3){
				return true;
			}

		}
	}
	return false;
}

int FindContent::findOwnerInfo(Rect rect, RotatedRect boxinfo, float k, int lastThresh, int orient)
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

	filterContour(contours, lineInfo, mGray.rows, mGray.cols, orient);

	calContourType(lineInfo, 1, orient);

	checkLine(lineInfo, 1, orient);

	//rejustOwnerContourType(lineInfo, orient);
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
		imshow("ImgOut4", tmpImg);
		cvWaitKey(1);
	}

	if (__min(lineInfo->box.size.width, lineInfo->box.size.height) > mUwidth* 0.8
		&& lineInfo->box_num_count >= 4 && lineInfo->box_info.size() >= 2 && lineInfo->box_info.size() <= 5
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
					if (abs(b1.center.y - b2.center.y) < mUheight*0.5){
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

int FindContent::findNoInfo(Rect rect, RotatedRect boxinfo, float k, int lastThresh, int orient)
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

	filterContour(contours, lineInfo, mGray.rows, mGray.cols, orient);

	calContourType(lineInfo, 0, orient);

	checkLine(lineInfo, 0, orient); 

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
		imshow("ImgOut4", tmpImg);
		cvWaitKey(1);
	}

	if (__min(lineInfo->box.size.width, lineInfo->box.size.height) > mUwidth*.08
		&& lineInfo->box_num_count > 5
		&& (orient == 1 || abs(lineInfoOwner.box.size.height - (lineInfo->box_info.begin())->box.size.height) < lineInfoOwner.box.size.height*0.2)        
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
	else if (orient == 0 && __min(lineInfo->box.size.width, lineInfo->box.size.height) > 13
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
		lineInfoNo = *lineInfo;
		return 2;
	}

	return -1;

}

int FindContent::findCheckDigitInfo(Rect rect, RotatedRect boxinfo, float k, int lastThresh, int orient)
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
		//drawRotatedRect(tmpImg, lineInfo->box, 1, Scalar(255));
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

bool FindContent::setTargetRegion(int type, Rect* rect, RotatedRect* box, float k, float b, int rows, int cols){
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
		
		w = box->size.width * (4 + (0.8+0.6) * 3)+10; //
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
		newCenter.y = box->center.y + box->size.height / 2 + mUheight * 4 + 4;
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
	newRect.height = newRect.height + 8;

	*box = newBox;
	*rect = newRect;
	return true;
}

void FindContent::filterContour(vector<vector<Point>> pContours, LineInfo* lineInfo, int rows, int cols, int orient){
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

		if ((orient == 0 && (box.size.width > 80 || box.size.height > 45)) ||
			(orient == 1 && (box.size.width > 45 || box.size.height > 10 * mUheight))){
			contour = contours.erase(contour);
			continue;
		}
		if (box.size.height < __min(mUheight>0 ? mUheight : 999, 13)){
			contour = contours.erase(contour);
			continue;
		}
		if (box.size.height > 1.1*mUheight && box.size.width < mUwidth *0.3){
			contour = contours.erase(contour);
			continue;
		}
		if (__max(box.size.width, box.size.height) < __min(mUheight > 0 ? mUheight : 999, 13)){
			contour = contours.erase(contour);
			continue;
		}
		int w = box.size.width;
		int h = box.size.height;
		if (!(box.size.width == mBox_u_all.size.width && box.size.width == mBox_u_all.size.width
			&& box.center.x == mBox_u_all.center.x && box.center.y == mBox_u_all.center.y)) { 
			if (orient == 0 && box.size.height < 50	&& box.size.width > mUwidth * 2.2){
				vector<boxInfo> retboxs;
				boxInfo boxinfoTmp;
				boxinfoTmp.box = box;
				boxinfoTmp.contour = *contour;
				retboxs = breakContours(mGray, boxinfoTmp, mUheight, 35, 1);
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
					retboxs = breakContours(mGray, boxinfoTmp2, mUheight, 35, 2);
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

void FindContent::calContourType(LineInfo* lineInfo, int target, int orient){
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

void FindContent::checkLine(LineInfo* lineInfo, int target, int orient){ //target:0=all,1=owner orient��1=���� 2=���� 
	vector<boxInfo> bottom;
	if (orient == 0){
		sortLineInfo(lineInfo, 2, 0);

		while (lineInfo->box_info.size()>1 && lineInfoNo.box_num_count == 0){
			if (target == 1 && lineInfo->box_info.begin()->box.center.x - lineInfo->box_info.begin()->box.size.width / 2 > mBox_u.center.x){
				lineInfo->box_1of2_count--;
				if (lineInfo->box_info.begin()->type > 0){
					lineInfo->box_num_count--;
				}
				lineInfo->box_info.erase(lineInfo->box_info.begin());
				continue;
			}
			else if (target == 1 && lineInfo->box_info.begin()->box.center.x + lineInfo->box_info.begin()->box.size.width / 2 > mBox_u.center.x + mUwidth / 2 + 2){
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
		for (int i = lineInfo->box_info.size() - 1; i >= 0; i--){
			if (abs((lineInfo->box_info.begin() + i)->box.center.x - mBox_u.center.x) > mUwidth * 1.3){
				lineInfo->box_1of2_count--;
				if ((lineInfo->box_info.begin() + i)->type > 0){
					lineInfo->box_num_count = lineInfo->box_num_count - (lineInfo->box_info.begin() + i)->type;
				}
				lineInfo->box_info.erase(lineInfo->box_info.begin() + i);
				continue;
			}
			else if (__max((lineInfo->box_info.begin() + i)->box.size.height, (lineInfo->box_info.begin() + i)->box.size.width) < __min(12, mUheight *0.6)){
				lineInfo->box_1of2_count--;
				if ((lineInfo->box_info.begin() + i)->type > 0){
					lineInfo->box_info.erase(lineInfo->box_info.begin() + i);
				}
				lineInfo->box_info.erase(lineInfo->box_info.begin() + i);
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

void FindContent::sortLineInfo(LineInfo* lineInfo, int pos, int orient){
	vector<boxInfo> boxs = lineInfo->box_info;
	sortBoxInfo(&boxs, pos, orient);
	lineInfo->box_info = boxs;
}


