#include "stdafx.h"
#include "detectJzx.h"

DetectJzx::DetectJzx(){}
DetectJzx::~DetectJzx(){}

bool DetectJzx::searchNumber(Mat src, int method){

	Mat tmp;
	Mat gray;			
	GaussianBlur(src, tmp, Size(3, 3), 0, 0.0, BORDER_DEFAULT);
	cvtColor(tmp, gray, CV_RGB2GRAY);

	double thres_val;
	FindContent* contents[6];
	contents[0] = new FindContent();
	contents[1] = new FindContent();
	contents[2] = new FindContent();
	contents[3] = new FindContent();
	contents[4] = new FindContent();
	contents[5] = new FindContent();

	#pragma omp parallel sections num_threads(3)
	{
		#pragma omp section
		{
			if (findCharU(prePare(gray, true, false, 20, 10, 0, &thres_val), 0, contents[0])){
				if (findResult.lineInfoOwner.box_num_count == 0){
					findResult = *contents[0];
				}
			}
		}
		#pragma omp section
		{
			if (findResult.lineInfoOwner.box_num_count == 0){
				if (findCharU(prePare(gray, true, true, 20, 10, 0, &thres_val), 0, contents[1])){
					if (findResult.lineInfoOwner.box_num_count == 0){
						findResult = *contents[1];
					}
				}
			}
		}
		#pragma omp section
		{
			if (findResult.lineInfoOwner.box_num_count == 0){
				if (findCharU(prePare(gray, false, false, 0, 0, 0, &thres_val), 0, contents[2])){
					if (findResult.lineInfoOwner.box_num_count == 0){
						findResult = *contents[2];
					}
				}
			}
		}
	}
	#pragma omp parallel sections num_threads(3)
	{
		#pragma omp section
		{
			if (findResult.lineInfoOwner.box_num_count == 0){
				if (findCharU(prePare(gray, true, false, 20, 10, 0, &thres_val), 1, contents[3])){
					if (findResult.lineInfoOwner.box_num_count == 0){
						findResult = *contents[3];
					}
				}
			}
		}
		#pragma omp section
		{
			if (findResult.lineInfoOwner.box_num_count == 0){
				if (findCharU(prePare(gray, true, true, 20, 10, 0, &thres_val), 1, contents[4])){
					if (findResult.lineInfoOwner.box_num_count == 0){
						findResult = *contents[4];
					}
				}
			}
		}
		#pragma omp section
		{
			if (findResult.lineInfoOwner.box_num_count == 0){
				if (findCharU(prePare(gray, false, false, 0, 0, 0, &thres_val), 1, contents[5])){
					if (findResult.lineInfoOwner.box_num_count == 0){
						findResult = *contents[5];
					}
				}
			}
		}
	}
	if (findResult.lineInfoOwner.box_num_count > 0){
		return true;
	}
	return false;
}

bool DetectJzx::findCharU(Mat img, double threshMethod, FindContent* content){
	Mat threImg;
	if (threshMethod == 0){ 
		cv::threshold(img, threImg, 0, 255, CV_THRESH_OTSU);
		//img.copyTo(threImg);
	}
	else{  
		double val = cv::threshold(img, threImg, 0, 255, CV_THRESH_OTSU);
		cv::threshold(img, threImg, val, 255, CV_THRESH_BINARY);
	}

	if (debug_level > 4){
		imshow("ImgOut1", threImg);
		waitKey(1);
	}
	vector<vector<Point>> contours_debug;
	vector<vector<Point>> contours;
	cv::findContours(threImg, contours, noArray(), RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	vector<vector<Point>>::iterator contour = contours.begin();
	int cnt = contours.size();
	int i = -1;
	while (i < (int)contours.size() - 1){
		i++;
		//if (i > 190){
		//	i = i;
		//}
		contour = contours.begin() + i;
		if (debug_level > 7) {
			Mat ImgTmp = Mat(img.rows, img.cols, CV_8UC1);
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
		//------------------------------------------------------------------------
		if ((box.size.width > 80 || box.size.height > 45)){
			continue;
		}
		if (box.size.height < 10 || __min(box.size.height, box.size.width)<3){
			continue;
		}
		if (__max(box.size.width, box.size.height) < 13){
			continue;
		}
		if (box.size.height < 50 && box.size.width > box.size.height * 1.1){
			vector<boxInfo> retboxs;
			boxInfo boxinfoTmp;
			boxinfoTmp.box = box;
			boxinfoTmp.contour = *contour;
			retboxs = breakContours(threImg, boxinfoTmp,35, 35, 1);
			if (retboxs.size() > 1 || (retboxs.size() == 1 && retboxs[0].box.size.width < box.size.width*0.9)){
				vector<vector<Point>> pts;
				for (int i = 0; i < retboxs.size(); i++){
					pts.push_back((retboxs.begin() + i)->contour);
				}
				contours.insert(contours.end(), pts.begin(), pts.end());
				continue;
			}
			else{
				boxInfo boxinfoTmp2;
				boxinfoTmp2.box = box;
				boxinfoTmp2.contour = *contour;
				retboxs = breakContours(threImg,boxinfoTmp2, 35,35, 2);
				if (retboxs.size() > 1 || (retboxs.size() == 1 && retboxs[0].box.size.width < box.size.width*0.9)){
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
							Mat ImgTmp = Mat(img.rows, img.cols, CV_8UC1);
							ImgTmp = 0 * ImgTmp;
							contours_debug.clear();
							contours_debug.push_back(pt);
							cv::drawContours(ImgTmp, contours_debug, -1, Scalar(255), CV_FILLED);
							cv::imshow("ImgOut3", ImgTmp);
							cvWaitKey(1);
							cvWaitKey(1);
						}
					}
					continue;
				}
			}
		}
		RotatedRect boxU;
		int type = 1;
		bool isU = false;
		boxInfo boxinfoU;
		float w = 0;
		float h = 0;


		if (box.size.height > 12 && box.size.height < 35
			&& box.size.width > box.size.height*0.65 && box.size.width < box.size.height*1.2){
			type = 2;
			boxU = RotatedRect(Point(box.center.x + box.size.width / 4, box.center.y), Size(box.size.width / 2, box.size.height), box.angle);
			boxinfoU.box = boxU;
			boxinfoU.contour = *contour;
			boxinfoU.k = box_k;
			isU = isCharU(img,boxinfoU,&w,&h);
			if (!isU){
				boxU = RotatedRect(Point(box.center.x - box.size.width / 4, box.center.y), Size(box.size.width / 2, box.size.height), box.angle);
				boxinfoU.box = boxU;
				boxinfoU.contour = *contour;
				boxinfoU.k = box_k;
				isU = isCharU(img, boxinfoU, &w, &h);
			}
		}
		else{
			type = 1;
			boxU = RotatedRect(Point(box.center.x, box.center.y), Size(box.size.width, box.size.height), box.angle);
			boxinfoU.box = boxU;
			boxinfoU.contour = *contour;
			boxinfoU.k = box_k;
			isU = isCharU(img, boxinfoU, &w, &h);
		}
		if (isU){
			content->mGray = img;
			content->mUheight = h;
			content->mUwidth = w;
			content->mBox_u = boxU;
			content->mBox_k = box_k;
			content->mBox_u_all = box;
			bool ret = content->search();
			if (ret){
				return true;
			}
		}
		//contour++;
	}
	return false;
}

bool DetectJzx::isCharU(Mat img,boxInfo boxinfo,float* w,float* h){
	RotatedRect box;
	box.center = boxinfo.box.center;
	box.size = boxinfo.box.size;
	box.angle = boxinfo.box.angle;
	Mat ori = Mat(img.rows, img.cols, CV_8UC1);
	ori = 0 * ori;
	vector<vector<Point>> c;
	c.push_back(boxinfo.contour);
	cv::drawContours(ori, c, -1, Scalar(255), CV_FILLED);

	//Rect rect = boundingRect(boxinfo.contour);
	Point2f pt2[4];
	box.points(pt2);
	Rect rect = boundingRect(vector < Point > { pt2[0], pt2[1], pt2[2], pt2[3] });
	Mat tmp = Mat(img.rows, img.cols, CV_8UC1);
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
		if (debug_level > 7) {
			cv::imshow("ImgOut2", tmp);
			cvWaitKey(1);
			cvWaitKey(1);
		}
		contours.clear();
		cv::findContours(tmp, contours, noArray(), RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
		if (contours.size() >2 && contours.size() < 5){
			for (int i = contours.size() - 1; i >= 0; i--){
				RotatedRect b = minAreaRect(*(contours.begin() + i));
				if (__max(b.size.width, b.size.height) < 4){
					contours.erase(contours.begin() + i);
				}
			}
		}
		if (contours.size() == 2){
			float k;
			//RotatedRect b1 = minAreaRect(*(contours.begin()));
			RotatedRect b1 = getRotatedRectFromPoints(*(contours.begin()), &k);
			//RotatedRect b2 = minAreaRect(*(contours.begin() + 1));
			RotatedRect b2 = getRotatedRectFromPoints(*(contours.begin()+1), &k);
			if (__max(b1.size.height, b1.size.width) > (box.size.height - diff)*0.8
				&& __min(b1.size.height, b1.size.width) < __max(box.size.height*0.2, 6)
				&& __max(b2.size.height, b2.size.width) >(box.size.height - diff)*0.8
				&& __min(b2.size.height, b2.size.width) < __max(box.size.height*0.2, 6)){
				*h = box.size.height;
				if (b2.center.x > b1.center.x){
					*w = (b2.center.x + b2.size.width / 2.0) - (b1.center.x - b1.size.width / 2.0); 
				}
				else{
				    *w = (b1.center.x + b1.size.width / 2.0) - (b2.center.x - b2.size.width / 2.0);  
				}
				return true;
			}
		}
	}
	return false;
}

Mat DetectJzx::prePare(Mat inputImg, bool doMORPH, bool isBlack, unsigned int val1_1, unsigned int val1_2, int method2, double* val2){
	Mat retImg;
	if (doMORPH && !isBlack){
		Mat element = getStructuringElement(MORPH_RECT, Size(val1_1, val1_2));
		morphologyEx(inputImg, retImg, MORPH_BLACKHAT, element);   //blackhat
	}
	else if (doMORPH && isBlack){
		Mat element = getStructuringElement(MORPH_RECT, Size(val1_1, val1_2));
		morphologyEx(inputImg, retImg, MORPH_TOPHAT, element);    //tophat
	}
	else{
		inputImg.copyTo(retImg);
	}
	////imshow("temp", retImg);
	////waitKey(1);
	//convertScaleAbs(retImg, retImg,2.0,-10.0);
	////imshow("temp2", retImg);
	////waitKey(0);

	return retImg;
}

Mat DetectJzx::OutputImage(Mat inputImg){
	Mat img ;
	inputImg.copyTo(img);
	CvScalar colorR = CV_RGB(255, 0, 0);
	CvScalar colorG = CV_RGB(0, 255, 0);
	CvScalar colorB = CV_RGB(0, 0, 255);
	drawRotatedRect(img, findResult.lineInfoOwner.box, 1, colorR);
	if (findResult.lineInfoNo1.box_num_count > 0){
		drawRotatedRect(img, findResult.lineInfoNo1.box, 1, colorG);
	}
	drawRotatedRect(img, findResult.lineInfoNo.box, 1, colorG);
	drawRotatedRect(img, findResult.lineInfoCheckDigit.box, 1, colorB);
	//imshow("tmp", img);
	//waitKey(1);
	return img;
}

//Mat DetectJzx::prePare(Mat inputImg, bool doMORPH, bool isBlack, unsigned int val1_1, unsigned int val1_2, int method2, double* val2){
//	Mat grayTmp;
//	double thres_sv = *val2;
//	if (doMORPH && !isBlack){
//		Mat element = getStructuringElement(MORPH_RECT, Size(val1_1, val1_2));
//		morphologyEx(inputImg, grayTmp, MORPH_BLACKHAT, element);   //blackhat
//	}
//	else if (doMORPH && isBlack){
//		Mat element = getStructuringElement(MORPH_RECT, Size(val1_1, val1_2));
//		morphologyEx(inputImg, grayTmp, MORPH_TOPHAT, element);    //tophat
//	}
//	else{
//		grayTmp = inputImg;
//	}
//	grayTmp.copyTo(mGray);
//	Mat retImg = Mat(inputImg.rows, inputImg.cols, CV_8UC1);
//	Mat mtmp = Mat(inputImg.rows, inputImg.cols, CV_8UC1);
//	Sobel(grayTmp, mtmp, CV_16S, 1, 0, 3, 1, 1, BORDER_DEFAULT);
//	convertScaleAbs(mtmp, mtmp);
//	if (method2 == 0){
//		*val2 = round(threshold(mtmp, retImg, 0, 255, CV_THRESH_OTSU));
//		mThresh = *val2;
//		thres_sv = *val2;
//	}
//	else if (method2 == 1){
//		threshold(mtmp, retImg, *val2, 255, CV_THRESH_BINARY);
//		thres_sv = *val2;
//		if (countNonZero(retImg) == 0 || countNonZero(255 - retImg) == 0){
//			*val2 = 0;
//			mThresh = *val2;
//			thres_sv = 0;
//		}
//	}
//	else{
//		retImg = 0 * mtmp;
//		thres_sv = 0;
//		*val2 = 0;
//		mThresh = *val2;
//	}
//	return retImg;
//}

