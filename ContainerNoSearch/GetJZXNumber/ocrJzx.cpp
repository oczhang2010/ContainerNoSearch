#include "stdafx.h"
#include "detectJzx.h"
#include "map"

OcrJzx::OcrJzx(){}
OcrJzx::~OcrJzx(){}

void OcrJzx::readInfo(){
	Mat gray = findResult.mGray;
	//Mat threshImg;

	orient = findResult.lineInfoOwner.box.size.width > findResult.lineInfoOwner.box.size.height ? 0 : 1;

	isBlackHat = checkIsblack(gray, findResult.lineInfoOwner.box, 2, 2, true);

////-----------------------------------------------------
//	Mat tmpImg1 = getWarpImage(gray, findResult.lineInfoOwner.box, 2, 2);
//	int shift1 = 255 - averageDark(tmpImg1);
//	tmpImg1 = tmpImg1 + shift1;
//	cv::imwrite(filepath + "\\owner_v.jpg", tmpImg1);
////-----------------------------------------------------

	Mat ownerImage;
	map_ocrResult ownerMap;
	readDatafromLineInfo(gray, findResult.lineInfoOwner, orient, 0,4, &ownerMap, &ownerImage);
	cv::imwrite(filepath + "\\owner.jpg", ownerImage);
	vector<ocrResult> v1 = sortOcrResult(ownerMap, 4);
	ownerTxt = v1.size() > 0 ? v1[0].text : "";
	ownerTxt1 = v1.size() > 1 ? v1[1].text : "";

//
////-----------------------------------------------------
//	Mat tmpImg2 = getWarpImage(gray, findResult.lineInfoNo.box, 2, 2);
//	int shift2 = 255 - averageDark(tmpImg2);
//	tmpImg2 = tmpImg2 + shift2;
//	cv::imwrite(filepath + "\\number_v1.jpg", tmpImg2);
////-----------------------------------------------------

	Mat numberImage;
	map_ocrResult numberMap;
	int charNum = (findResult.lineInfoNo1.box_num_count > 0) ? 3 : 6;
	readDatafromLineInfo(gray, findResult.lineInfoNo, orient, 1, charNum, &numberMap, &numberImage);
	cv::imwrite(filepath + "\\number.jpg", numberImage);
	if (findResult.lineInfoNo1.box_num_count > 0){
		Mat numberImage1;
////-----------------------------------------------------
//		Mat tmpImg3 = getWarpImage(gray, findResult.lineInfoNo1.box, 2, 2);
//		int shift3 = 255 - averageDark(tmpImg3);
//		tmpImg3 = tmpImg3 + shift3;
//		cv::imwrite(filepath + "\\number_v2.jpg", tmpImg3);
////-----------------------------------------------------
		map_ocrResult numberMap1;
		readDatafromLineInfo(gray, findResult.lineInfoNo1, orient, 1,3, &numberMap1, &numberImage1);
		cv::imwrite(filepath + "\\number1.jpg", numberImage1);
		numberImage = jointMat(numberImage1, numberImage, 2);
		numberMap = mergeMap(numberMap1, numberMap);
	}
	vector<ocrResult> v2 = sortOcrResult(numberMap, 6);
	numberTxt = v2.size() > 0 ? v2[0].text : "";
	numberTxt1 = v2.size() > 1 ? v2[1].text : "";

	Mat checkdigitImage1, checkdigitImage2;
	readDataDigit(gray, 1, orient, &checkDigitTxt, &checkDigitTxt1, &checkdigitImage1, &checkdigitImage2);  //�ڶ�������chartype��0=��д��ĸ 1=����
	cv::imwrite(filepath + "\\checkditig.jpg", checkdigitImage1);

	Mat out;
	Mat allNumberImage1 = jointMat(numberImage, checkdigitImage1, 6);
	out = jointMat(ownerImage, allNumberImage1, 6);
	cv::imwrite(filepath + "\\number_all.jpg", allNumberImage1);
	cv::imwrite(filepath + "\\out.jpg", out);

	return;
}

void OcrJzx::readDatafromLineInfo(Mat inputImg, LineInfo lineInfo, int orient, int chartype, int charNumber, map_ocrResult* map, Mat* out){
	int dataLength = charNumber;
	tesseract::PageSegMode mode = (orient == 0) ? tesseract::PSM_SINGLE_BLOCK : tesseract::PSM_SINGLE_BLOCK_VERT_TEXT;

	Mat out1, out2;
	int conf_sum = 0;
	string retStr = "";

	map->clear();
	readDatafromBox(inputImg, lineInfo.box, chartype, charNumber, map, &out1, 2, 2, mode);

	map_ocrResult map1 = map_ocrResult();
	for (int i = 0; i < lineInfo.box_info.size(); i++)
	{
		boxInfo boxinfo = lineInfo.box_info[i];
		if (boxinfo.type < 1){
			dataLength--;
			continue;
		}
		RotatedRect box = boxinfo.box;
		box.angle = lineInfo.box.angle;
		Mat tmpImg = getWarpImage(inputImg, box, 2, 2);
		int shift = 255 - averageDark(tmpImg);
		tmpImg = tmpImg + shift;
		//map_ocrResult mapTmp;
		//readDatafromImg(tmpImg, chartype, &mapTmp, 20, 20, mode);
		//map1 = mergeMap(map1, mapTmp);
		if (boxinfo.type == 1){
			out2 = jointMat(out2, tmpImg, 3);
			//int conf_val = 0;
			//retStr = retStr + readTextfromImg(tmpImg, chartype, &conf_val, 20, 20, mode);
			//conf_sum = conf_sum + conf_val;
		}
		else{
			//int conf1 = 0;
			//string txt1 = "";
			////int conf2 = 0;
			////string txt2 = "";
			//txt1 = readTextfromImg(tmpImg, chartype, &conf1, 20, 20, mode);

			for (int j = 0; j < boxinfo.type; j++){
				RotatedRect b;
				if (orient == 0){
					b = breakBox(boxinfo.box, orient, boxinfo.type, j, lineInfo.box.angle, boxinfo.k, 0, 2);
				}
				else{
					b = breakBox(boxinfo.box, orient, boxinfo.type, j, lineInfo.box.angle, boxinfo.k, 6, 0);
				}
				Mat tmpImg2 = getWarpImage(inputImg, b);
				shift = 255 - averageDark(tmpImg2);
				tmpImg2 = tmpImg2 + shift;
				out2 = jointMat(out2, tmpImg2, (j>0 && orient == 0) ? 5 : 3);
				//int confTmp = 0;
				//txt2 = txt2 + readTextfromImg(tmpImg2, chartype, &confTmp, 20, 20, mode);
				//conf2 = conf2 + confTmp;
			}
			//retStr = retStr + txt1;
			//conf_sum = conf_sum + conf1;

			//if (conf1 > (int)(conf2 / boxinfo.type)){
			//	retStr = retStr + txt1;
			//	conf_sum = conf_sum + (int)(conf1 * boxinfo.type);
			//}
			//else{
			//	retStr = retStr + txt2;
			//	conf_sum = conf_sum + conf2;
			//}
		}
	}
	//if (conf_sum > 0 && lineInfo.box_info.size() > 0){
	//	mapAdd(map, (int)(conf_sum / lineInfo.box_info.size()), retStr);
	//}

	//map_ocrResult::iterator it;
	//for (it = map1.begin(); it != map1.end(); it++){
	//	mapAdd(map, (it->second).conf, (it->second).text);
	//}
	
	//readDatafromImg(out2, chartype, map, 2, 2, tesseract::PSM_SINGLE_BLOCK);
	
	//conf2 = 0;
	//text2 = readTextfromImg(out2, chartype, &conf2, 20, 20, false);

	//conf3 = conf3 / dataLength;
	//if (conf3 > conf2){
	//	text2 = text3;
	//	conf2 = conf3;
	//}

	//if (conf1 < conf2){
	//	*txt1 = text2;
	//	*txt2 = text1;
	//	*out = out2;
	//}
	//else{
	//	*txt1 = text1;
	//	*txt2 = text2;
	//	*out = (orient == 0) ? out1 : out2; 
	//}
	//*out = out1;
	*out = out2;

	return;

}

void OcrJzx::readDatafromBox(Mat inputImg, RotatedRect box, int chartype, int charNumber,map_ocrResult* mapRet, Mat* out, int diffx, int diffy, tesseract::PageSegMode mode){
	Mat charImg = getWarpImage(inputImg, box, diffx, diffy);
	Mat ocr = Mat(charImg.rows + 20, charImg.cols + 20, CV_8UC1, Scalar(255));
	Mat ocrRoi = Mat(ocr, Rect(10, 10, charImg.cols, charImg.rows));
	//charImg.copyTo(ocrRoi);
	int max = -1;
	*out = charImg;
	map_ocrResult ocrMap;
	float step = 0.05;
	float v1 = 1 - step;
	float v2 = 1;
	float diff = 255 - averageDark(charImg);
	while ((v1 < 1.35 || ocrMap.size() == 0) && v1<1.5){
		int c1 = 999;
		v1 = v1 + step;
		convertScaleAbs(charImg, ocrRoi, v1, diff);
		string txt1 = readText1(ocr, chartype, &c1, mode);
		if (debug_level > 5){
			imshow("ocrImage", ocr);
			waitKey(1);
			destroyWindow("ocrImage");
			imwrite(filepath + "\\data1.jpg", ocr);
		}
		max = (c1 > max && c1 < 999) ? c1 : max;
		*out = (c1 > max && c1 < 999) ? ocrRoi : *out; 

		if (charNumber == 4 && txt1.length() == 5 && txt1.substr(3, 1) == "U" && txt1.substr(4, 1) == "I"){
			txt1 = txt1.substr(0, 4);
		}
		if (c1 > 30 && c1 < 999 && txt1.length() == charNumber){
			mapAdd(&ocrMap, c1, txt1);
		}
		v2 = v2 - step;
		if (v2 > 0.85){
			convertScaleAbs(charImg, ocrRoi, v2, diff);
			int c2 = 999;
			string txt2 = readText1(ocr, chartype, &c2, mode);
			if (debug_level > 5){
				imshow("ocrImage", ocr);
				waitKey(1);
				destroyWindow("ocrImage");
				imwrite(filepath + "\\data-2.jpg", ocr);
			}
			max = (c2 > max && c2 < 999) ? c2 : max;
			*out = (c2 > max  && c2 < 999) ? ocrRoi : *out; 
			if (charNumber == 4 && txt1.length() == 5 && txt1.substr(3, 1) == "U" && txt1.substr(4, 1) == "I"){
				txt1 = txt1.substr(0, 4);
			}
			if (c2 > 30 && c2 < 999 && txt2.length() == charNumber){
				mapAdd(&ocrMap, c2, txt2);
			}
		}
	}
	*mapRet = ocrMap;
	return;
}

void OcrJzx::readDatafromImg(Mat ocrImg, int chartype, int charNumber, map_ocrResult* mapRet, int diffx, int diffy, tesseract::PageSegMode mode){
	Mat ocr = Mat(ocrImg.rows + diffx, ocrImg.cols + diffy, CV_8UC1, Scalar(255));
	Mat ocrRoi = Mat(ocr, Rect(diffx / 2, diffy / 2, ocrImg.cols, ocrImg.rows));
	//ocrImg.copyTo(ocrRoi);

	int max = -1;
	map_ocrResult ocrMap = *mapRet;
	float step = 0.05;
	float v1 = 0.9;
	float v2 = 1;
	//float diff = 255 - averageDark(ocrImg);
	float diff = 0; 
	while ((v1 < 1.25 || ocrMap.size() == 0) && v1<1.5){
		v1 = v1 + step;
		convertScaleAbs(ocrImg, ocrRoi, v1, diff);
		int c1 = 999;
		string txt1 = readText1(ocr, chartype, &c1, mode);
		if (debug_level > 5){
			imshow("ocrImage", ocr);
			waitKey(1);
			destroyWindow("ocrImage");
			imwrite(filepath + "\\data-2.jpg", ocr);
		}
		max = (c1 > max) ? c1 : max;
		if (c1 > 30 && c1 < 999 && (charNumber==0 ||txt1.length() == charNumber)){
			mapAdd(&ocrMap, c1, txt1);
		}

		//v2 = v2 - step;
		//if (v2 > 0.9){
		//	convertScaleAbs(ocrImg, ocrRoi, v2, diff);
		//	int c2 = 999;
		//	string txt2 = readText1(ocr, chartype, &c1, mode);
		//	if (debug_level > 5){
		//		imshow("ocrImage", ocr);
		//		waitKey(1);
		//		destroyWindow("ocrImage");
		//	}
		//	max = (c2 > max) ? c2 : max;
		//	if (c2 > 30 & c2 < 999){
		//		mapAdd(&ocrMap, c2, txt2);
		//	}
		//}
	}
	*mapRet = ocrMap;
	return;

}

string OcrJzx::readTextfromImg(Mat ocrImg, int chartype, int charNumber, int* conf, int diffx, int diffy, tesseract::PageSegMode mode){
	//Mat ocr = Mat(ocrImg.rows + diffx, ocrImg.cols + diffy, CV_8UC1, Scalar(255));
	//Mat ocrRoi = Mat(ocr, Rect(diffx / 2, diffy / 2, ocrImg.cols, ocrImg.rows));
	//ocrImg.copyTo(ocrRoi);
	//return readText1(ocr, chartype, conf, mode);
	map_ocrResult map;
	readDatafromImg(ocrImg, chartype, charNumber, &map, diffx, diffy, mode);
	vector<ocrResult> vector = sortOcrResult(map);
	*conf = (vector.size() > 0) ? vector[0].conf:0;
	return (vector.size() > 0) ? vector[0].text : "";
}

void OcrJzx::readDataDigit(Mat inputImg, int chartype, int orient,string* text1, string* text2, Mat* out1, Mat* out2){
	RotatedRect checkbox = findResult.lineInfoCheckDigit.box;

	int diffx;
	if (orient == 0){
		diffx = -4;
		if (checkbox.size.width < findResult.mUwidth * 0.6){
			diffx = 6;
		}
		else if (checkbox.size.width < findResult.mUwidth * 1.1){
			diffx = 2;
		}
	}
	else{
		diffx = 2;
		if (checkbox.size.width < findResult.mUwidth * 0.6){
			diffx = 6;
		}
	}
	int diffy = -1;
	if (checkbox.size.height > findResult.mUheight * 1.1){
		diffy = -2;
	}
	map_ocrResult map1;
	Mat checkOut1 = getWarpImage(inputImg, checkbox, diffx, diffy, false);  
	int shift = 255 - averageDark(checkOut1);
	checkOut1 = checkOut1 + shift;
	*out1 = checkOut1;
	readDatafromImg(checkOut1, 1,0, &map1, 20, 20, tesseract::PSM_SINGLE_BLOCK);
	readDatafromImg(checkOut1, 1,0, &map1, 20, 20, tesseract::PSM_SINGLE_CHAR);


	vector<ocrResult> v1 = sortOcrResult(map1, 0);
	*text1 = v1.size() > 0 ? v1[0].text : "";
	*text2 = v1.size() > 1 ? v1[1].text : "";
	return;
}

String OcrJzx::readText1(Mat img, int chartype, int* conf, tesseract::PageSegMode mode){
	char* whitelist_all = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char* whitelist_number = "0123456789";
	char* whitelist_char = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	ocrApi->SetPageSegMode(mode); // tesseract::PSM_SINGLE_BLOCK);tesseract::PSM_SINGLE_CHAR
	ocrApi->SetImage(img.data, img.cols, img.rows, 1, img.step);
	if (chartype == 0){
		ocrApi->SetVariable("tessedit_char_whitelist", whitelist_char);
	}
	else if (chartype == 1){
		ocrApi->SetVariable("tessedit_char_whitelist", whitelist_number);
	}
	else{
		ocrApi->SetVariable("tessedit_char_whitelist", whitelist_all);
	}
	ocrApi->Recognize(0);
	
	String retTxt = ocrApi->GetUTF8Text();
	retTxt = removeChar(retTxt, "\n");
	retTxt = removeChar(retTxt, " ");
	*conf = ocrApi->MeanTextConf();
	ocrApi->Clear();
	return retTxt;
}

Mat OcrJzx::getWarpImage(Mat img, RotatedRect rect, int diffx, int diffy, bool isThresh){
	if (rect.size.width < 3 && diffx < 0){
		diffx = 6 - rect.size.width;
	}
	rect.size.width = rect.size.width + diffx;
	rect.size.height = rect.size.height + diffy;
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
	Mat out = warpImage(img, srcPts, dstPts, w, h);
	if (isThresh){
		cv::threshold(out, out, 0, 255, CV_THRESH_OTSU);
		////cv::threshold(out, out, mThresh, 255, CV_THRESH_BINARY);
	}
	if (isBlackHat){
		out = 255 - out;
	}
	//out = 255 - out;
	return out;
}

bool OcrJzx::checkIsblack(Mat img, RotatedRect box, int diffx, int diffy, bool isThresh){
	Mat warp = getWarpImage(img, box, diffx, diffy, isThresh);
	return isBlack(warp);
}

