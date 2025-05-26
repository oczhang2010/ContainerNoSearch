#include "stdafx.h"
#include "utility.h"

vector<Point2f> orderRotatedRectPoint(RotatedRect rect){
	try{
		Point2f pt[4];
		rect.points(pt);
		if (rect.center.x < 0 || rect.center.y < 0){
			vector<Point2f> pts;
			return pts;
		}
		//按x排序，分类为左面2点和右面2点
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
		//点排序：左下为原点，顺时针排列
		// 左面2点中y坐标大的排在第一点(左下),y坐标小的排在第二点(左上)
		if (pt[max_x_idx[0]].y < pt[max_x_idx[1]].y){
			p[0] = pt[max_x_idx[1]];
			p[1] = pt[max_x_idx[0]];
		}
		else{
			p[0] = pt[max_x_idx[0]];
			p[1] = pt[max_x_idx[1]];
		}
		//  右面2点中y坐标小的排在第三点(右上),x坐标大的排在第四点(右下)
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
			//y方向高度 > x方向宽度，并且右边2点y轴坐标高于左边两点y坐标：短边是宽度长边是高度
			pts.push_back(p[2]);
			pts.push_back(p[3]);
			pts.push_back(p[0]);
			pts.push_back(p[1]);
		}
		else if ((__max(p[2].y, p[3].y) - __min(p[1].y, p[2].y))>w && p[0].y < p[2].y && p[0].y < p[3].y){
			//y方向高度 > x方向宽度，并且右边2点y轴坐标低于左边两点y坐标：短边是宽度长边是高度
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

void drawRotatedRect(Mat srcImg, RotatedRect box, int thickness, CvScalar color){
	if (box.size.width > 0){
		Point2f pt[4];
		box.points(pt);
		line(srcImg, pt[0], pt[1], color, thickness);
		line(srcImg, pt[1], pt[2], color, thickness);
		line(srcImg, pt[2], pt[3], color, thickness);
		line(srcImg, pt[3], pt[0], color, thickness);
	}
}

vector<boxInfo> breakContours(Mat inputImage, boxInfo boxinfo, int Uwidth,int minHeight, int target){
	vector<boxInfo> ret;

	RotatedRect box;
	box.center = boxinfo.box.center;
	box.size = boxinfo.box.size;
	box.angle = boxinfo.box.angle;

	//Rect rect = boxinfo.box.boundingRect();
	Rect rect = boundingRect(boxinfo.contour);
	int save_height = rect.height;
	Mat imgTmp = Mat(inputImage.rows, inputImage.cols, CV_8UC1);
	Mat roi, roi_gray;
	int diff = 0;
	int step = Uwidth<16 ? 2 : 4;
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
			//切割到小于一半是退出
			diff = 999;
			continue;
		}
		vector<vector<Point>> v = { boxinfo.contour };
		imgTmp = 0 * imgTmp;
		drawContours(imgTmp, v, -1, Scalar(255), CV_FILLED);
		roi = CreateMat(imgTmp, rect);
		Mat tmp = Mat(inputImage.rows, inputImage.cols, CV_8UC1);
		tmp = 0 * tmp;
		Mat roiTmp = CreateMat(tmp, rect);
		roi.copyTo(roiTmp);
		//if (debug_level > 9){
		//	imshow("Img", tmp);
		//	cvWaitKey(1);
		//	cvWaitKey(1);
		//}

		vector<vector<Point>> contours;
		cv::findContours(tmp, contours, noArray(), RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
		if (contours.size() == 0){
			break;
		}
		float k;
		//RotatedRect b = minAreaRect(*(contours.begin()));
		RotatedRect b = getRotatedRectFromPoints(*(contours.begin()), &k);
		if (contours.size() > 1){
			float box_start = -1;
			float box_end = 999;
			float box_width = 0;
			while (contours.size() > 0){
				//b = minAreaRect(*(contours.begin()));
				b = getRotatedRectFromPoints(*(contours.begin()), &k);
				CalcRotatedRectPoints(&b, &k);
				if (b.size.height <= __max(boxinfo.box.size.height * 0.2, 3) || (b.size.width <= __max(boxinfo.box.size.height * 0.2, 3) && b.size.height <= 7)){
					//分割后多出来的短线段不要
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
				diff = 9999;  //diff设成不符合循环条件，退出循环
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
		//去掉2边长出来边框时，返回新boxinfo
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
		//不能分割时，返回原来boxinfo
		ret.push_back(boxinfo);
	}
	return ret;
}

void sortBoxInfo(vector<boxInfo>* pBoxinfos, int pos, int orient){
	vector<boxInfo> boxs = *pBoxinfos;
	vector<boxInfo> box_infos;
	if (orient == 0){
		//按x轴中心点从大到小排序
		//pos : 0=按中心x排序 1=按左边距排序 2=按右边距排序
		//去掉既存排序结果
		for (int i = 0; i < boxs.size(); i++){
			boxs[i].idx = -1;
		}
		//排序
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
		//按y轴中心点从大到小排序
		//pos : 0=按中心y排序 1=按上边距排序 2=按下边距距排序
		//去掉既存排序结果
		for (int i = 0; i < boxs.size(); i++){
			boxs[i].idx = -1;
		}
		//排序
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

void CalContourInfo(vector<vector<Point>> contours, LineInfo* lineInfo){
	//重新计算外接矩形和斜率
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

void CalcRotatedRectPoints(RotatedRect* box, float* k)
{
	vector<Point2f> pt;
	pt = orderRotatedRectPoint(*box);
	if (pt.size() == 0){
		return;
	}

	//排序后返回的点顺是：左上为第一点，顺时针
	//宽度是第一点和第二点之间距离，高度是第一点和第三点间距离
	float w = sqrt(pow(pt[1].x - pt[0].x, 2) + pow((pt[1].y - pt[0].y), 2));
	float h = sqrt(pow(pt[3].x - pt[0].x, 2) + pow((pt[3].y - pt[0].y), 2));
	float angle = 0.0;
	float kValue = 0.0;
	if (pt[0].x == pt[1].x)
	{
		kValue = 99999;        /* k = 无穷大*/
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

	//如果高宽倒置时：互换高宽
	if (box->size.width > 12 && box->size.height > 5){
		Point2f ptTmp[4];
		box->points(ptTmp);
		Rect rect = boundingRect(vector < Point > { ptTmp[0], ptTmp[1], ptTmp[2], ptTmp[3] });
		if ((rect.height > rect.width && box->size.height < box->size.width)
			|| (rect.height < rect.width && box->size.height > box->size.width)){
			*box = RotatedRect(box->center, Size(box->size.height, box->size.width), box->angle - 90);
		}
	}
	*k = kValue;
}

Mat warpImage(Mat srcImage, Point2f* srcPts, Point2f* dstPts, int warpWidth, int warpHeight)
{
	Mat retImage;
	Mat m = getPerspectiveTransform(srcPts, dstPts);
	warpPerspective(srcImage, retImage, m, Size(warpWidth, warpHeight), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar());
	return retImage;
}

Mat CreateMat(Mat src, Rect rect, bool isReturnErr){
	try{
		if (src.rows < 4 || src.cols<4) {
			return Mat(1, 1, CV_8UC1);
		}
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

void CalBoxInfo(LineInfo* lineInfo){
	vector<boxInfo> boxinfos = lineInfo->box_info;
	//重新计算外接矩形和斜率
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

RotatedRect getRotatedRectFromPoints(vector<Point> pts, float* k){
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

Mat getTransform(RotatedRect rect){
	vector<Point2f> pt = orderRotatedRectPoint(rect);

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
	Mat m = getPerspectiveTransform(srcPts, dstPts);
	return m;
}

bool isBlack(Mat inputImg){
	int sum1 = 0;
	int sum2 = 0;
	for (int i = 0; i < inputImg.rows; i++){
		int k = 0;
		uchar a = inputImg.at<uchar>(i, k);
		if (inputImg.at<uchar>(i, 0) > 0){
			sum1++;
		}
		else{
			sum2++;
		}
		if (inputImg.at<uchar>(i, inputImg.cols - 1) > 0){
			sum1++;
		}
		else{
			sum2++;
		}
	}
	for (int j = 1; j < inputImg.cols - 1; j++){
		if (inputImg.at<uchar>(0, j) > 0){
			sum1++;
		}
		else{
			sum2++;
		}
		if (inputImg.at<uchar>(inputImg.rows - 1, j) > 0){
			sum1++;
		}
		else{
			sum2++;
		}
	}
	if (sum1 > sum2){
		return false;
	}
	else{
		return true;
	}

}

float averageDark(Mat inputImg){
	if (inputImg.rows == 0 || inputImg.cols == 0){
		return 0.0;
	}
	int count = 0;
	int sum = 0;
	for (int i = 0; i < inputImg.rows; i++){
		sum = sum + (int)(inputImg.at<uchar>(i, 0)) + (int)(inputImg.at<uchar>(i, inputImg.cols - 1));
		count = count + 2;
	}
	for (int j = 1; j < inputImg.cols - 1; j++){
		sum = sum + (int)(inputImg.at<uchar>(0, j)) + (int)(inputImg.at<uchar>(inputImg.rows - 1, j));
		count = count + 2;
	}
	return (float)(sum / count);
}


Rect getRectFromBox(RotatedRect inbox){
	Point2f ptTmp[4];
	inbox.points(ptTmp);
	Rect rect = boundingRect(vector < Point > { ptTmp[0], ptTmp[1], ptTmp[2], ptTmp[3] });
	return rect;
}

string removeChar(string instr, string rmchar){
	string workstr = instr;
	string::size_type pos = 0;
	while ((pos = workstr.find_first_of(rmchar, pos)) != string::npos)
	{
		workstr.replace(pos, 1, "");
		//pos++;
	}
	return workstr;

}

string reverseText(string instr){
	string ret = "";
	for (int i = 0; i < instr.length(); i++){
		ret = ret + instr.substr(i, 1);
	}
	return ret;
}

Mat jointMat(Mat img1, Mat img2, int diff){
	//连接2个Mat，间距设为diff
	if (img1.rows == 0){
		return img2;
	}
	if (img2.rows == 0){
		return img1;
	}
	float hh = __max(img1.rows, img2.rows);
	Mat out = Mat(hh, img1.cols + img2.cols + diff, CV_8UC1, Scalar(255));
	float top1 = (hh - img1.rows) / 2;
	Mat roi1 = Mat(out, Rect(0, top1, img1.cols, img1.rows));
	float top2 = (hh - img2.rows) / 2;
	Mat roi2 = Mat(out, Rect(img1.cols + diff, top2, img2.cols, img2.rows));
	img1.copyTo(roi1);
	img2.copyTo(roi2);
	return out;
}

map_ocrResult mergeMap(map_ocrResult map1, map_ocrResult map2){
	if (map1.size() == 0){
		return map2;
	}
	map_ocrResult ret = map_ocrResult();
	map_ocrResult::iterator it1, it2;
	for (it1 = map1.begin(); it1 != map1.end(); it1++){
		for (it2 = map2.begin(); it2 != map2.end(); it2++){
			ocrResult data = ocrResult();
			data.conf = ((it1->second).conf + (it2->second).conf) / 2;
			data.text = (it1->second).text + (it2->second).text;
			ret.insert((map<string, ocrResult>::value_type(data.text, data)));
		}
	}
	return ret;
}

vector<ocrResult> sortOcrResult(map_ocrResult map, int charCount){
	vector<ocrResult> ret = vector<ocrResult>();
	map_ocrResult::iterator it;
	for (it = map.begin(); it != map.end(); it++){
		if (charCount>0 && it->second.text.length() != charCount){
			continue;
		}
		bool insertFlg = false;
		vector<ocrResult>::iterator c = ret.begin();
		while (c != ret.end()){
			if (c->conf <= it->second.conf){
				ret.insert(c, it->second);
				insertFlg = true;
				break;
			}
			c++;
		}
		if (insertFlg == false){
			ret.push_back(it->second);
			continue;
		}
	}
	return ret;
}

RotatedRect breakBox(RotatedRect inBox, int orient, int charCnt, int idx, float angle, float k, int diffx, int diffy){
	//等分box后，获取第idx个子box
	if (orient == 0){
		float newX = inBox.center.x - inBox.size.width / 2 + inBox.size.width / charCnt *(idx + 0.5);
		float newY = inBox.center.y;
		float w = inBox.size.width / charCnt + diffx;
		float h = inBox.size.height + diffy;
		return RotatedRect(Point(newX, newY), Size(w, h), angle);
	}
	else{
		float newX = inBox.center.x;
		float newY = inBox.center.y - inBox.size.height / 2 + inBox.size.height / charCnt *(idx + 0.5);;
		if (angle != 0){
			newX = (-k * newY + (inBox.center.x + inBox.center.y * k));
		}
		float w = inBox.size.width + diffx;
		float h = inBox.size.height / charCnt + diffy;
		return RotatedRect(Point(newX, newY), Size(w, h), angle);
	}


}

void mapAdd(map_ocrResult* inMap, int conf, string text){
	map_ocrResult dic = *inMap;
	ocrResult data = ocrResult();
	data.conf = conf;
	data.text = text;
	map_ocrResult::iterator it = dic.find(text);
	if (it == dic.end()){
		dic.insert(map<string, ocrResult>::value_type(text, data)); //新text：插入
	}
	else{
		dic[text].conf = (conf > dic[text].conf) ? conf : dic[text].conf; //更新分数
	}
	*inMap = dic;
	return;
}


