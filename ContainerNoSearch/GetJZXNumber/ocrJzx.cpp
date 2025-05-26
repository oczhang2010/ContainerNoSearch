#include "stdafx.h"
#include "detectJzx.h"
#include "map"

//构造函数
OcrJzx::OcrJzx(){}
OcrJzx::~OcrJzx(){}

//识别文字入口
void OcrJzx::readInfo(){
	Mat gray = findResult.mGray;
	//Mat threshImg;

	//判断文字排列是横向还是竖向
	orient = findResult.lineInfoOwner.box.size.width > findResult.lineInfoOwner.box.size.height ? 0 : 1;

	//判断是否为白底黑字,之后图像拗正函数getWarpImage中根据这个判断反转黑白
	isBlackHat = checkIsblack(gray, findResult.lineInfoOwner.box, 2, 2, true);

////-----------------------------------------------------
//	Mat tmpImg1 = getWarpImage(gray, findResult.lineInfoOwner.box, 2, 2);
//	int shift1 = 255 - averageDark(tmpImg1);
//	tmpImg1 = tmpImg1 + shift1;
//	cv::imwrite(filepath + "\\owner_v.jpg", tmpImg1);
////-----------------------------------------------------

	//识别货主代码
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

	//识别箱编码:识别下段
	Mat numberImage;
	map_ocrResult numberMap;
	int charNum = (findResult.lineInfoNo1.box_num_count > 0) ? 3 : 6;
	readDatafromLineInfo(gray, findResult.lineInfoNo, orient, 1, charNum, &numberMap, &numberImage);
	cv::imwrite(filepath + "\\number.jpg", numberImage);
	//识别箱编码:识别上段
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
		//横向合并上下段
		numberImage = jointMat(numberImage1, numberImage, 2);
		numberMap = mergeMap(numberMap1, numberMap);
	}
	vector<ocrResult> v2 = sortOcrResult(numberMap, 6);
	numberTxt = v2.size() > 0 ? v2[0].text : "";
	numberTxt1 = v2.size() > 1 ? v2[1].text : "";

	//识别校验位
	Mat checkdigitImage1, checkdigitImage2;
	readDataDigit(gray, 1, orient, &checkDigitTxt, &checkDigitTxt1, &checkdigitImage1, &checkdigitImage2);  //第二个参数chartype：0=大写字母 1=数字
	cv::imwrite(filepath + "\\checkditig.jpg", checkdigitImage1);

	//结合箱编号和校验位图像，并保存到文件
	Mat out;
	Mat allNumberImage1 = jointMat(numberImage, checkdigitImage1, 6);
	out = jointMat(ownerImage, allNumberImage1, 6);
	cv::imwrite(filepath + "\\number_all.jpg", allNumberImage1);
	//货主代码和箱号+校验位图片==>出力
	cv::imwrite(filepath + "\\out.jpg", out);

	return;
}

void OcrJzx::readDatafromLineInfo(Mat inputImg, LineInfo lineInfo, int orient, int chartype, int charNumber, map_ocrResult* map, Mat* out){
	int dataLength = charNumber;
	tesseract::PageSegMode mode = (orient == 0) ? tesseract::PSM_SINGLE_BLOCK : tesseract::PSM_SINGLE_BLOCK_VERT_TEXT;

	Mat out1, out2;
	int conf_sum = 0;
	string retStr = "";

	//1.不分割直接做ocr
	map->clear();
	readDatafromBox(inputImg, lineInfo.box, chartype, charNumber, map, &out1, 2, 2, mode);

	//2.分割字符后ocr(无粘连时加间隔3，有粘连时加大间隔到5)
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
			//无粘连时直接连接
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

			//有粘连时分割后连接(粘连字符间加大间隔)
			for (int j = 0; j < boxinfo.type; j++){
				RotatedRect b;
				if (orient == 0){
					//横排时
					b = breakBox(boxinfo.box, orient, boxinfo.type, j, lineInfo.box.angle, boxinfo.k, 0, 2);
				}
				else{
					//竖排排时，考虑到角度倾斜性，左右幅度加大一些
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
	//	//个别字符ocr结果插入返回集合中
	//	mapAdd(map, (int)(conf_sum / lineInfo.box_info.size()), retStr);
	//}

	//map_ocrResult::iterator it;
	//for (it = map1.begin(); it != map1.end(); it++){
	//	mapAdd(map, (it->second).conf, (it->second).text);
	//}
	
	//分割粘连后重新ocr
	//readDatafromImg(out2, chartype, map, 2, 2, tesseract::PSM_SINGLE_BLOCK);
	
	//conf2 = 0;
	//text2 = readTextfromImg(out2, chartype, &conf2, 20, 20, false);

	////单个字符ocr结果好的话，采用单个字符ocr结果
	//conf3 = conf3 / dataLength;
	//if (conf3 > conf2){
	//	text2 = text3;
	//	conf2 = conf3;
	//}

	////比较直接ocr和切分字符后ocr分数，采用得分高结果
	//if (conf1 < conf2){
	//	*txt1 = text2;
	//	*txt2 = text1;
	//	*out = out2;
	//}
	//else{
	//	*txt1 = text1;
	//	*txt2 = text2;
	//	*out = (orient == 0) ? out1 : out2; //竖排时不能直立输出，只能采用分割后横向排列输入
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
		*out = (c1 > max && c1 < 999) ? ocrRoi : *out; //返回用：最大得分对应图像

		//特别处理：4位货主代码时，第四位是U后面有一个I的话，可以判断这个竖线是外框线可以去除
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
			*out = (c2 > max  && c2 < 999) ? ocrRoi : *out; //返回用：最大得分对应图像
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
	float diff = 0; //图片ocd时，实现已做过灰度去除
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
	//对象box
	RotatedRect checkbox = findResult.lineInfoCheckDigit.box;

	//校验位四周有框线，需要调整宽度：宽度过窄时增加宽度（如checkdigit=1时，最小外接矩形会全黑，需要增加宽度）
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
	Mat checkOut1 = getWarpImage(inputImg, checkbox, diffx, diffy, false);  //《--最后后一个参数true=要二值化
	int shift = 255 - averageDark(checkOut1);
	checkOut1 = checkOut1 + shift;
	*out1 = checkOut1;
	readDatafromImg(checkOut1, 1,0, &map1, 20, 20, tesseract::PSM_SINGLE_BLOCK);
	readDatafromImg(checkOut1, 1,0, &map1, 20, 20, tesseract::PSM_SINGLE_CHAR);

	////第二次尝试：二值化后寻找，注意：二值化图像不需再用对比度穷尽法，直接去ocr即可！
	//int checkConf2 = 0;
	//Mat checkOut2;
	//cv::threshold(checkOut1, checkOut2, 0, 255, CV_THRESH_OTSU); 
	//string checkText2 = readText1(checkOut2, chartype, &checkConf2, tesseract::PSM_SINGLE_BLOCK);//
	//if (checkConf2 == 0){
	//	checkText2 = readText1(checkOut2, chartype, &checkConf2,tesseract::PSM_SINGLE_CHAR);//
	//}
	////第三次尝试：反转黑白后再试一次
	//int checkConf3 = 0;
	//Mat checkOut3;
	//checkOut2.copyTo(checkOut3);
	//checkOut3 = 255 - checkOut3;  //二值化图像反转黑白后再试一次
	////string checkText3 = readTextfromImg(checkOut3, chartype, &checkConf3, 20, 20, tesseract::PSM_SINGLE_CHAR);
	//string checkText3 = readTextfromImg(checkOut3, chartype, &checkConf3, 20, 20, tesseract::PSM_SINGLE_BLOCK);

	////比较二次二值化尝试结果
	//if (checkConf3 > checkConf2){
	//	checkText2 = checkText3;
	//	checkConf2 = checkConf3;
	//	*out2 = checkOut3;
	//}
	//else{
	//	*out2 = checkOut2;
	//}
	//注意:sort第二个参数=最大位数要设0（不限返回text位数）：两边框有可能变成1返回
	//vector<ocrResult> v1 = sortOcrResult(map1,0);  
	//string checkText1 = v1.size() > 0 ? v1[0].text : "";
	//int checkConf1 = v1.size() > 0 ? v1[0].conf : 0;
	//if (checkConf1 > checkConf2){
	//	*text1 = checkText1;
	//	*text2 = checkText2.length()>0 ? checkText2 : checkText1;
	//}
	//else{
	//	*text1 = checkText2;
	//	*text2 = checkText1.length()>0 ? checkText1 : checkText2;
	//}
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

	//if (mode == tesseract::PSM_SINGLE_BLOCK_VERT_TEXT && retTxt.length()>0){
	//	tesseract::Orientation orieentation;
	//	tesseract::WritingDirection direction;
	//	tesseract::TextlineOrder order;
	//	float deskew_angle;
	//	int *b1 = 0;
	//	bool *b2 = false;
	//	ocrApi->GetBlockTextOrientations(&b1, &b2);
	//	if (*b1 > 0){
	//		retTxt = reverseText(retTxt);
	//	}
	//	//tesseract::PageIterator* it = ocrApi->AnalyseLayout();
	//	//it->Orientation(&orieentation, &direction, &order, &deskew_angle);
	//	//if (order == tesseract::TEXTLINE_ORDER_RIGHT_TO_LEFT || order == tesseract::TEXTLINE_ORDER_TOP_TO_BOTTOM){
	//	//	retTxt = reverseText(retTxt);
	//	//}
	//}

	return retTxt;

	//vector<int> c;
	//vector<string> t;
	//char* resultTxt = ocrApi->GetUTF8Text();
	//int* cfs = ocrApi->AllWordConfidences();
	//string ret = "";
	//while (*cfs != '\0'){
	//	string text = "";
	//	while (*resultTxt != '\n'){
	//		text = text + *resultTxt;
	//		ret = ret + *resultTxt;
	//		resultTxt++;
	//	}
	//	resultTxt++;
	//	t.push_back(text);
	//	c.push_back(*cfs);
	//	cfs++;
	//}

	//ocrApi->SetPageSegMode(tesseract::PSM_AUTO);
	//ocrApi->SetImage(img.data, img.cols, img.rows, 1, img.step);
	//String ocrResult0 = ocrApi->GetUTF8Text();
	//int conf0 = ocrApi->MeanTextConf();

	//ocrApi->SetPageSegMode(tesseract::PSM_SINGLE_WORD);
	//ocrApi->SetImage(img.data, img.cols, img.rows, 1, img.step);
	//String ocrResult1 = ocrApi->GetUTF8Text();
	//int conf1 = ocrApi->MeanTextConf();

	//ocrApi->SetPageSegMode(tesseract::PSM_SINGLE_CHAR);
	//ocrApi->SetImage(img.data, img.cols, img.rows, 1, img.step);
	//String ocrResult2 = ocrApi->GetUTF8Text();
	//int conf2 = ocrApi->MeanTextConf();

	//ocrApi->Clear();

	//int retConf = (conf1 > conf0) ? conf1 : conf0;
	//string retTxt = (conf1 > conf0) ? ocrResult1 : ocrResult0;
	//retConf = (conf2 > retConf) ? conf2 : retConf;
	//retTxt = (conf2 > retConf) ? ocrResult2 : retTxt;
	//*conf = retConf;
	////fprintf(stdout, "confidence: %d, text: %s", retConf, retTxt);
	//return retTxt.substr(0, retTxt.length() - 2);;

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

//void OcrJzx::recognizeInfo(){
//	Mat gray = findResult.mGray;
//	//Mat threshImg;
//
//	//判断文字排列是横向还是竖向
//	orient = findResult.lineInfoOwner.box.size.width > findResult.lineInfoOwner.box.size.height ? 0 : 1;
//
//	//判断是否为白底黑字,之后getWarpImage中根据这个判断反转黑白
//	isBlackHat = checkIsblack(gray, findResult.lineInfoOwner.box, 2, 2, true);
//
//	//识别货主代码
//	Mat ownerImage;
//	//doOcr(threshImg, findResult.lineInfoOwner, orient, 0, &ownerImage, &ownerTxt, &ownerTxt1);
//	readTextfromLineInfo(gray, findResult.lineInfoOwner, orient, 0, &ownerImage, &ownerTxt, &ownerTxt1);
//	cv::imwrite(filepath + "\\owner.jpg", ownerImage);
//
//	//识别箱编码
//	//识别下段
//	Mat numberImage;
//	readTextfromLineInfo(gray, findResult.lineInfoNo, orient, 1, &numberImage, &numberTxt, &numberTxt1);
//	cv::imwrite(filepath + "\\number.jpg", numberImage);
//	//识别上段
//	if (findResult.lineInfoNo1.box_num_count > 0){
//		Mat numberImage1;
//		string txt1 = "";
//		string txt2 = "";
//		readTextfromLineInfo(gray, findResult.lineInfoNo1, orient, 1, &numberImage1, &txt1, &txt2);
//		numberTxt = txt1 + numberTxt;
//		numberTxt1 = txt2 + numberTxt1;
//		cv::imwrite(filepath + "\\number1.jpg", numberImage1);
//		//横向合并上下段
//		numberImage = jointMat(numberImage, numberImage1, 2);
//	}
//
//	//识别校验位
//	Mat checkdigitImage1, checkdigitImage2;
//	readCheckdigit(gray, 1, &checkDigitTxt, &checkDigitTxt1, &checkdigitImage1, &checkdigitImage2);  //第二个参数chartype：0=大写字母 1=数字
//	cv::imwrite(filepath + "\\checkditig.jpg", checkdigitImage1);
//
//	//结合箱编号和校验位图像，并再次识别
//	Mat allNumberImage1 = jointMat(numberImage, checkdigitImage1, 6);
//	int digitconf1 = 0;
//	string allnumber1 = readTextfromImg(allNumberImage1, 1, &digitconf1, 20, 20, false);
//	Mat allNumberImage2 = jointMat(numberImage, checkdigitImage2, 6);
//	threshold(allNumberImage2, allNumberImage2, 0, 255, CV_THRESH_OTSU);
//	int digitconf2 = 0;
//	string allnumber2 = readTextfromImg(allNumberImage1, 1, &digitconf2, 20, 20, false);
//
//	Mat out;
//	if (digitconf1 > digitconf2){
//		numberTxt2 = allnumber1;
//		out = jointMat(ownerImage, allNumberImage1, 6);
//		cv::imwrite(filepath + "\\number_all.jpg", allNumberImage1);
//	}
//	else{
//		numberTxt2 = allnumber2;
//		out = jointMat(ownerImage, allNumberImage1, 6);
//		cv::imwrite(filepath + "\\number_all.jpg", allNumberImage2);
//	}
//	//货主代码和箱号+校验位图片==>出力
//	cv::imwrite(filepath + "\\out.jpg", out);
//
//	return;
//}
//
//void OcrJzx::readTextfromLineInfo(Mat inputImg, LineInfo lineInfo, int orient, int chartype, Mat* out, string* txt1, string* txt2){
//	int dataLength = lineInfo.box_num_count;
//	Mat out1, out2;
//	int conf1, conf2, conf3;
//	string text1, text2, text3;
//
//	//1.不分割直接做ocr
//	conf1 = 0;
//	readTextfromBox(inputImg, lineInfo.box, 4, 2, chartype, &conf1, &text1, &out1, false);
//	text1 = (orient == 0) ? text1 : removeChar(text1, "\n");
//
//	//2.分割字符后ocr(无粘连时加间隔3，有粘连时加大间隔到5)
//	conf2 = 0;
//	text2 = "";
//	conf3 = 0;
//	text3 = "";
//	for (int i = 0; i < lineInfo.box_info.size(); i++)
//	{
//		boxInfo boxinfo = lineInfo.box_info[i];
//		if (boxinfo.type < 1){
//			dataLength--;
//			continue;
//		}
//		if (boxinfo.type == 1){
//			//无粘连时直接连接
//			RotatedRect box = boxinfo.box;
//			box.angle = lineInfo.box.angle;
//			Mat tmpImg = getWarpImage(inputImg,box,2,2);
//			out2 = jointMat(out2, tmpImg, 3);
//			//单个字符ocr
//			int tmpConf = 0;
//			text3 = text3 + readTextfromImg(tmpImg, chartype, &tmpConf, 20, 20, true, tesseract::PSM_SINGLE_CHAR);
//			conf3 = conf3 + tmpConf;
//		}
//		else{
//			//有粘连时分割后连接(粘连字符间加大间隔)
//			for (int j = 0; j < boxinfo.type; j++){
//				RotatedRect b;
//				if (orient == 0){
//					//横排时
//					b = breakBox(boxinfo.box, orient, boxinfo.type, j, lineInfo.box.angle, boxinfo.k, 0, 2);
//				}
//				else{
//					//竖排排时，考虑到角度倾斜性，左右幅度加大一些
//					b = breakBox(boxinfo.box, orient, boxinfo.type, j, lineInfo.box.angle, boxinfo.k, 6, 0);
//				}
//				Mat tmpImg = getWarpImage(inputImg, b);
//				out2 = jointMat(out2, tmpImg, (j>0 && orient == 0) ? 5 : 3);
//				int tmpConf = 0;
//				text3 = text3 + readTextfromImg(tmpImg, chartype, &tmpConf, 20, 20, true, tesseract::PSM_SINGLE_CHAR);
//				conf3 = conf3 + tmpConf;
//
//			}
//		}
//	}
//	//分割粘连后重新ocr
//	conf2 = 0;
//	text2 = readTextfromImg(out2, chartype, &conf2, 20, 20, false);
//
//	//单个字符ocr结果好的话，采用单个字符ocr结果
//	conf3 = conf3 / dataLength;
//	if (conf3 > conf2){
//		text2 = text3;
//		conf2 = conf3;
//	}
//
//	//比较直接ocr和切分字符后ocr分数，采用得分高结果
//	if (conf1 < conf2){
//		*txt1 = text2;
//		*txt2 = text1;
//		*out = out2;
//	}
//	else{
//		*txt1 = text1;
//		*txt2 = text2;
//		*out = (orient == 0) ? out1 : out2; //竖排时不能直立输出，只能采用分割后横向排列输入
//	}
//	return;
//
//}
//
//void OcrJzx::readCheckdigit(Mat inputImg, int chartype, string* text1, string* text2, Mat* out1,Mat* out2){
//	//对象box
//	RotatedRect checkbox = findResult.lineInfoCheckDigit.box;
//
//	//校验位四周有框线，需要调整宽度：宽度过窄时增加宽度（如checkdigit=1时，最小外接矩形会全黑，需要增加宽度）
//	int diffx = -4;
//	if (checkbox.size.width < findResult.mUwidth * 0.6){
//		diffx = 4;
//	}
//	else if (checkbox.size.width < findResult.mUwidth * 1.1){
//		diffx = 2;
//	}
//	int diffy = -2;
//	if (checkbox.size.height > findResult.mUheight * 1.1){
//		diffy = -4;
//	}
//	int checkConf1 = 0;
//	string checkText1 = "";
//	Mat checkOut1;
//	readTextfromBox(inputImg, checkbox, diffx, diffy, chartype, &checkConf1, &checkText1, &checkOut1, false);
//	int checkConf2 = 0;
//	string checkText2 = "";
//	Mat checkOut2;
//	readTextfromBox(inputImg, checkbox, diffx, diffy, chartype, &checkConf2, &checkText2, &checkOut2, true);
//
//	//反转再试一次
//	Mat checkOut3 = getWarpImage(inputImg, checkbox, diffx, diffy, true);  //《--左后一个参数true=要二值化
//	if (!isBlackHat){    //注意-->这里是反转：和正常是相反的！！
//		checkOut3 = 255 - checkOut3;
//	}
//	int checkConf3 = 0;
//	string checkText3 = readTextfromImg(checkOut3,chartype,&checkConf3);
//	if (checkConf3 > checkConf2){
//		checkText2 = checkText3;
//		checkOut2 = checkOut3;
//	}
//
//	if (checkConf1 > checkConf2){
//		*text1 = checkText1;
//		*text2 = checkText2;
//		*out1 = checkOut1;
//		*out2 = checkOut2;
//	}
//	else{
//		*text1 = checkText2;
//		*text2 = checkText1;
//		*out1 = checkOut2;
//		*out2 = checkOut1;
//	}
//	return;
//}
//
//string OcrJzx::readTextfromImg(Mat ocrImg, int chartype, int* conf, int diffx, int diffy,bool isThresh,tesseract::PageSegMode mode){
//	Mat ocr = Mat(ocrImg.rows + diffx, ocrImg.cols + diffy, CV_8UC1, Scalar(255));
//	Mat ocrRoi = Mat(ocr, Rect(diffx/2, diffy/2, ocrImg.cols, ocrImg.rows));
//	ocrImg.copyTo(ocrRoi);
//	//convertScaleAbs(ocr, ocr, 1.2, -5.0);
//	if (isThresh){
//		threshold(ocr, ocr, 0, 255, CV_THRESH_OTSU);
//	}
//	string text = readText1(ocr, chartype, conf, mode);
//	//imshow("tmp", ocr);
//	//waitKey(1);
//	//waitKey(1);
//	//destroyWindow("tmp");
//	return text;
//}
//
//void OcrJzx::readTextfromBox(Mat inputImg, RotatedRect box,int diffx, int diffy,int chartype, int* conf, string* text, Mat* out,bool isThresh){
//	Mat charImg = getWarpImage(inputImg, box,diffx,diffy);
//	Mat ocrImg = Mat(charImg.rows + 20, charImg.cols + 20, CV_8UC1, Scalar(255));
//	Mat ocrRoi = Mat(ocrImg, Rect(10, 10, charImg.cols, charImg.rows));
//	charImg.copyTo(ocrRoi);
//	convertScaleAbs(ocrRoi, ocrRoi, 1.2, -5.0);
//	if (isThresh){
//		threshold(ocrImg, ocrImg, 0, 255, CV_THRESH_OTSU);
//	}
//	int conf1 = 0;
//	*text = readText1(ocrImg, chartype, &conf1);
//	*conf = conf1;
//	*out = ocrImg;
//	return;
//}
//
//
//String OcrJzx::readText(Mat img, int chartype){
//	char* whitelist_all = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
//	char* whitelist_number = "0123456789";
//	char* whitelist_char = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
//	ocrApi->SetPageSegMode(tesseract::PSM_SINGLE_BLOCK);
//	ocrApi->SetImage(img.data, img.cols, img.rows, 1, img.step);
//	if (chartype == 0){
//		ocrApi->SetVariable("tessedit_char_whitelist", whitelist_char);
//	}
//	else if (chartype == 1){
//		ocrApi->SetVariable("tessedit_char_whitelist", whitelist_number);
//	}
//	else{
//		ocrApi->SetVariable("tessedit_char_whitelist", whitelist_all);
//	}
//	String ocrResult = ocrApi->GetUTF8Text();
//	ocrResult = ocrResult.substr(0, ocrResult.length() - 2);
//	int conf = ocrApi->MeanTextConf();
//	//fprintf(stdout, "confidence: %d, text: %s", conf, ocrResult);
//	return (ocrResult == "?") ? "?" : ocrResult;
//}
//
//void OcrJzx::doOcr(Mat inputImg, LineInfo lineInfo, int orient, int chartype, Mat* out, string* txt1, string* txt2){
//	float uWidth = findResult.mUwidth * 2;
//	float uHeight = findResult.mUheight * 2;
//	int dataLength = lineInfo.box_num_count;
//	Mat outImg;
//
//	//原图做二值化
//	Mat threshImg;
//	threshold(inputImg, threshImg, 0, 255, CV_THRESH_OTSU);
//
//	float diff;
//	//准备输出字符图像：竖排时需要特殊出力
//	if (orient == 0){
//		//原图上找出字符部分并拗正
//		Mat warp = getWarpImage(inputImg, lineInfo.box, 4, 2);
//		//准备返回出力识别图像
//		outImg = Mat(warp.rows + 20, warp.cols + 20, CV_8UC1, Scalar(255));
//		Mat roi = CreateMat(outImg, Rect(10, 10, warp.cols, warp.rows));
//		warp.copyTo(roi);
//		int conf1 = 0;
//		string text1 = readText1(outImg, chartype, &conf1);
//		//cv::imshow("temp0", outImg);
//		//cv::waitKey(1);
//		//cv::destroyWindow("temp0");
//
//		//横向排列时：分割字符后个别ocr
//		float boxheight = lineInfo.box.size.height * 2;  //注意：拗正时是放大2倍的，所以此处box高度要乘以2！！！
//		Mat ocr = Mat((int)__max(uHeight, boxheight) + 20, (int)(uWidth + 20) * (dataLength + 1) + 10, CV_8UC1, Scalar(255));
//		diff = 10.0;
//		int conf2 = 0;
//		string text2 = "";
//		int conf3 = 0;
//		string text3 = "";
//		for (int i = 0; i < lineInfo.box_info.size(); i++)
//		{
//			boxInfo boxinfo = lineInfo.box_info[i];
//			if (boxinfo.type < 1){
//				continue;
//			}
//			//有你粘连时切分开来再做ocr
//			for (int j = 0; j < boxinfo.type; j++){
//				float newX = boxinfo.box.center.x - boxinfo.box.size.width / 2 + boxinfo.box.size.width / boxinfo.type *(j + 0.5);
//				float newY = boxinfo.box.center.y;
//				float w = boxinfo.box.size.width / boxinfo.type + 1; // ((boxinfo.type > 1) ? -2 : 0);
//				float h = boxinfo.box.size.height + 2;
//				RotatedRect b = RotatedRect(Point(newX, newY), Size(w, h), lineInfo.box.angle);
//				Mat charImg = getWarpImage(inputImg, b);
//				Mat ocrRoi = CreateMat(ocr, Rect(diff + ((j>0) ? 6 : 4), 10, charImg.cols, charImg.rows));
//				diff = diff + charImg.cols + ((j>0) ? 6 : 4);
//				charImg.copyTo(ocrRoi);
//
//				Mat charOcr = Mat(charImg.rows + 20, charImg.cols + 20, CV_8UC1, Scalar(255));
//				Mat oritmp = CreateMat(charOcr, Rect(10, 10, charImg.cols, charImg.rows));
//				charImg.copyTo(oritmp);
//				int score = 0;
//				text3 = text3 + readText2(charOcr, chartype, &score);
//				conf3 = conf3 + score;
//
//				//cv::imshow("tmp", ocr);
//				//cv::waitKey(1);
//				//cv::destroyWindow("tmp");
//			}
//		}
//		//分割粘连后重新ocr
//		text2 = readText1(ocr, chartype, &conf2);
//		Mat outRoi = CreateMat(ocr, Rect(1, 1, diff + 8, ocr.rows));
//
//		conf3 = conf3 / dataLength;
//		if (conf3 > conf2){
//			text2 = text3;
//		}
//
//		//如果粘连部分切开后得分高：使用切开后ocr所得文字
//		if (conf1 < conf2){
//			*txt1 = text2;
//			*txt2 = text1;
//			*out = outRoi;
//		}
//		else{
//			*txt1 = text1;
//			*txt2 = text2;
//			*out = outImg;
//		}
//		//cv::imshow("temp0", outRoi);
//		//cv::waitKey(1);
//		//cv::destroyWindow("temp0");
//		//cv::imshow("temp1", outImg);
//		//cv::waitKey(1);
//		//cv::destroyWindow("temp0");
//	}
//	else
//	{
//		//竖向排列时：分割字符后横向排列：准备输出用Mat
//		outImg = Mat((int)uHeight + 20, (int)(uWidth + 20) * (dataLength + 1) + 20, CV_8UC1, Scalar(255));
//		diff = 10.0;
//		for (int i = 0; i < lineInfo.box_info.size(); i++)
//		{
//			boxInfo boxinfo = lineInfo.box_info[i];
//			for (int j = 0; j < boxinfo.type; j++){
//				float newX = boxinfo.box.center.x;
//				float newY = boxinfo.box.center.y - boxinfo.box.size.height / 2 + boxinfo.box.size.height / boxinfo.type *(j + 0.5);;
//				if (boxinfo.box.angle != 0){
//					newX = (-boxinfo.k * newY + (boxinfo.box.center.x + boxinfo.box.center.y * boxinfo.k));
//				}
//				float w = boxinfo.box.size.width + 6;
//				float h = boxinfo.box.size.height / boxinfo.type;
//				RotatedRect b = RotatedRect(Point(newX, newY), Size(w, h), lineInfo.box.angle);
//				Mat charImg = getWarpImage(inputImg, b);
//				Mat ocrRoi = CreateMat(outImg, Rect(diff + 2, 8, charImg.cols, charImg.rows));
//				diff = diff + charImg.cols + 2;
//				charImg.copyTo(ocrRoi);
//				//cv::imshow("tmp", outImg);
//				//cv::waitKey(1);
//				//cv::waitKey(1);
//			}
//		}
//		Mat outRoi = CreateMat(outImg, Rect(1, 1, diff + 8, outImg.rows));
//		*out = outRoi;
//		*txt1 = readText(outImg, chartype);
//		*txt2 = *txt1;
//		//cv::imshow("temp", outImg);
//		//cv::waitKey(1);
//		//cv::destroyWindow("temp");
//	}
//	return;
//
//}
//
//void OcrJzx::recognizeCheckdigit(Mat inputImg, Mat numImage, Mat* allnumberImage, string* txt1, string* txt2){
//	//ocr:校验码位
//	boxInfo chkDigitInfo = findResult.lineInfoCheckDigit.box_info[0];
//	Mat threshOcr;
//	threshold(inputImg, threshOcr, 0, 255, CV_THRESH_OTSU);
//
//	//调整宽度：宽度过窄时增加宽度（如checkdigit=1时，最小外接矩形会全黑，需要增加宽度）
//	int diffx = -4;
//	if (chkDigitInfo.box.size.width < findResult.mUwidth * 0.6){
//		diffx = 4;
//	}
//	else if (chkDigitInfo.box.size.width < findResult.mUwidth * 1.1){
//		diffx = 2;
//	}
//	//原图上找出箱编号或下排编号部分并拗正
//	Mat checkImg1 = getWarpImage(inputImg, chkDigitInfo.box, diffx, -2);
//
//	//准备识别用图像(四周加10像素空白！)
//	Mat ocr1 = Mat(checkImg1.rows + 20, checkImg1.cols + 20, CV_8UC1, Scalar(255));
//	Mat checkRoi1 = CreateMat(ocr1, Rect(10, 10, checkImg1.cols, checkImg1.rows));
//	checkImg1.copyTo(checkRoi1);
//
//	//文字识别
//	int conf1, conf2;
//	string text1 = readText1(ocr1, 1, &conf1);
//
//	//二值化+反转后再试一次
//	Mat checkImg2 = getWarpImage(inputImg, chkDigitInfo.box, diffx, -2, true);  //《--左后一个参数true=要二值化
//	if (!isBlackHat){    //注意-->这里是反转：和正常是相反的！！
//		checkImg2 = 255 - checkImg2;
//	}
//	//ocr用图像（四周加10像素
//	Mat ocr2 = Mat(checkImg2.rows + 20, checkImg2.cols + 20, CV_8UC1, Scalar(255));
//	Mat checkRoi2 = CreateMat(ocr2, Rect(10, 10, checkImg2.cols, checkImg2.rows));
//	//复制数据
//	checkImg2.copyTo(checkRoi2);
//	string text2 = readText1(ocr2, 1, &conf2);
//	Mat out;
//	if (conf1 > conf2){
//		out = ocr1;
//		*txt1 = text1;
//		*txt2 = text2;
//	}
//	else
//	{
//		out = ocr2;
//		*txt1 = text2;
//		*txt2 = text1;
//	}
//	//识别用图片出力
//	cv::imwrite(filepath + "\\checkdigit.jpg", out);
//
//	//拼接编号+校验位图像
//	float h = __max(out.rows, numImage.rows);
//	Mat outNum = Mat(h + 4, numImage.cols + out.cols + 20, CV_8UC1, Scalar(255));
//	Mat roi1 = CreateMat(outNum, Rect(2, 2, numImage.cols, numImage.rows));
//	Mat roi2 = CreateMat(outNum, Rect(numImage.cols + 4, 2, out.cols, out.rows));
//	numImage.copyTo(roi1);
//	out.copyTo(roi2);
//
//	//整体识别：编号+校验位
//	numberTxt2 = readText(outNum, 1);
//	//cv::imwrite(filepath + "\\number2.jpg", outNum);
//	*allnumberImage = outNum;
//	return;
//
//}
//
//
//string OcrJzx::doOcr(Mat inputImg, LineInfo lineInfo, int orient, int targetCharNum, int chartype, Mat* out){
//	float uWidth = findResult.mUwidth * 2;
//	float uHeight = findResult.mUheight * 2;
//	int dataLength = lineInfo.box_num_count;
//	string retstr = "";
//	Mat outImg;
//
//	//原图做二值化
//	Mat threshImg;
//	threshold(inputImg, threshImg, 0, 255, CV_THRESH_OTSU);
//
//	//原图上找出货主代码部分并拗正
//	isBlackHat = false;
//	Mat warp = getWarpImage(threshImg, lineInfo.box);
//	Mat warp1 = getWarpImage(inputImg, lineInfo.box);
//
//	//先判断是否为白底黑字,根据判断反转黑白
//	isBlackHat = isBlack(warp);
//	if (isBlackHat){
//		warp = 255 - warp;
//	}
//	//imshow("temp0", warp);
//	//waitKey(1);
//	//cv::destroyWindow("temp0");
//
//
//	//准备输出字符图像：竖排时需要特殊出力
//	if (orient == 0){
//		//准备返回debug用识别图像
//		outImg = Mat(warp.rows + 20, warp.cols + 20, CV_8UC1, Scalar(255));
//		Mat roi = CreateMat(outImg, Rect(10, 10, warp.cols, warp.rows));
//		warp.copyTo(roi);
//		*out = outImg;
//		//imshow("temp0", outImg);
//		//waitKey(1);
//		//cv::destroyWindow("temp0");
//
//		//横向排列时：分割字符后个别ocr
//		Mat ocr = Mat((int)uHeight + 20, (int)(uWidth + 20) * (dataLength + 1) + 10, CV_8UC1, Scalar(255));
//		int k = 0;
//		for (int i = 0; i < lineInfo.box_info.size(); i++)
//		{
//			boxInfo boxinfo = lineInfo.box_info[i];
//			if (boxinfo.type < 1){
//				continue;
//			}
//			int conf10 = 0;
//			string text10 = readTextfromBox(inputImg, boxinfo.box, uWidth*boxinfo.type, uHeight, 2, 2, chartype, &conf10);
//			int conf11 = 0;
//			string text11 = readTextfromBox(threshImg, boxinfo.box, uWidth*boxinfo.type, uHeight, 2, 2, chartype, &conf11);
//			int conf1 = conf10>conf11 ? conf10 : conf11;
//			string text1 = conf10>conf11 ? text10 : text11;
//
//			int conf2 = 0;
//			string text2 = "";
//			if (boxinfo.type > 1){
//				//有你粘连时切分开来分别ocr
//				for (int j = 0; j < boxinfo.type; j++){
//					Mat ocrImg = Mat(uHeight + 20, uWidth + 20, CV_8UC1, Scalar(255));
//					float newX = boxinfo.box.center.x - boxinfo.box.size.width / 2 + boxinfo.box.size.width / boxinfo.type *(j + 0.5);
//					float newY = boxinfo.box.center.y;
//					float w = boxinfo.box.size.width / boxinfo.type - 1;
//					float h = boxinfo.box.size.height / boxinfo.type + 2;
//					RotatedRect b = RotatedRect(Point(newX, newY), Size(w, h), boxinfo.box.angle);
//					int conf20 = 0;
//					string text20 = readTextfromBox(inputImg, b, uWidth, uHeight, 2, 0, chartype, &conf20);
//					int conf21 = 0;
//					string text21 = readTextfromBox(threshImg, b, uWidth, uHeight, 2, 0, chartype, &conf21);
//					conf2 = conf2 + (conf20 > conf21 ? conf20 : conf21);
//					text2 = text2 + (conf20 > conf21 ? text20 : text21);
//				}
//			}
//			//如果粘连部分切开后得分高：使用切开后ocr所得文字
//			conf2 = conf2 / boxinfo.type;
//			if (conf1 < conf2){
//				retstr = retstr + text2;
//			}
//			else{
//				retstr = retstr + text1;
//			}
//		}
//	}
//	else
//	{
//		//竖向排列时：分割字符后横向排列：准备输出用Mat
//		outImg = Mat((int)uHeight + 20, (int)(uWidth + 20) * (dataLength + 1) + 20, CV_8UC1, Scalar(255));
//		int k = 0;
//		for (int i = 0; i < lineInfo.box_info.size(); i++)
//		{
//			boxInfo boxinfo = lineInfo.box_info[i];
//			for (int j = 0; j < boxinfo.type; j++){
//				float newX = boxinfo.box.center.x;
//				float newY = boxinfo.box.center.y - boxinfo.box.size.height / 2 + boxinfo.box.size.height / boxinfo.type *(j + 0.5);;
//				float w = boxinfo.box.size.width + 6;
//				float h = boxinfo.box.size.height / boxinfo.type + 2;
//				RotatedRect b = RotatedRect(Point(newX, newY), Size(w, h), boxinfo.box.angle);
//				Mat charImg = getWarpImage(inputImg, b);
//				if (!isBlackHat){
//					charImg = 255 - charImg;
//				}
//				Mat ocrRoi = CreateMat(outImg, Rect((uWidth + 20)*k + 10, 10, charImg.cols, charImg.rows));
//				charImg.copyTo(ocrRoi);
//				imshow("tmp", outImg);
//				waitKey(1);
//				k++;
//			}
//		}
//		*out = outImg;
//		imshow("temp", outImg);
//		waitKey(1);
//		cv::destroyWindow("temp");
//
//		retstr = readText(outImg, chartype);
//	}
//	return retstr;
//
//}
//string OcrJzx::ocrProcess(Mat in, int idx, RotatedRect box, int type, string filename){
//	Rect rect = Rect((int)((findResult.mUwidth * 2 + 20)*idx + findResult.mUwidth - in.cols / 2 + 10), (int)(findResult.mUheight - in.rows / 2 + 10), in.cols, in.rows);
//	Rect rectOcr = Rect((int)((findResult.mUwidth * 2 + 20)*idx) + 1, 1, (int)(findResult.mUwidth * 2 + 20) - 2, (int)(findResult.mUheight * 2 + 20) - 2);
//	Mat ocrBaseRoi = CreateMat(mOcr, rect);
//	in.copyTo(ocrBaseRoi);
//	Mat ocrRoi = CreateMat(mOcr, rectOcr);
//	//imshow("tmp", mOcr);
//	//waitKey(100);
//	//imshow("tmp2", in);
//	//waitKey(200);
//
//	string ocrTxt = "";
//	if (type == 0){
//		//imshow("tmp", ocrRoi);
//		//waitKey(1);
//		ocrTxt = readText(ocrRoi, type);
//	}
//	else{
//		//imshow("tmp2", mlpRoi);
//		//waitKey(1);
//		//ocrTxt = readTextMlp(mlpRoi, type);
//		ocrTxt = readText(ocrRoi, type);
//	}
//	cv::imwrite(filepath + "\\" + filename + ".jpg", ocrRoi);
//	return ocrTxt;
//}
//
//String OcrJzx::readText(Mat img, int type){
//	char* whitelist_all = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
//	char* whitelist_number = "0123456789";
//	char* whitelist_char = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
//	//imshow("tmp", img);
//	//waitKey(1);
//	ocrApi->SetPageSegMode(tesseract::PSM_SINGLE_BLOCK);
//	ocrApi->SetImage(img.data, img.cols, img.rows, 1, img.step);
//	if (type == 0){
//		ocrApi->SetVariable("tessedit_char_whitelist", whitelist_char);
//	}
//	else if (type == 1){
//		ocrApi->SetVariable("tessedit_char_whitelist", whitelist_number);
//	}
//	else{
//		ocrApi->SetVariable("tessedit_char_whitelist", whitelist_all);
//	}
//	String ocrResult = ocrApi->GetUTF8Text();
//	ocrResult = ocrResult.substr(0, ocrResult.length() - 2);
//	//ocrResult = ocrResult.replace(("\n", "");
//	int conf = ocrApi->MeanTextConf();
//	//fprintf(stdout, "confidence: %d, text: %s", conf, ocrResult);
//	return (ocrResult == "?") ? "?" : ocrResult;
//
//}
//
//
//
//void OcrJzx::recognizeOwner(Mat threshImg){
//	//orc:货主代码
//
//	ownerTxt = "";
//	//货主代码整体识别
//	Mat owner = getWarpImage(threshImg, findResult.lineInfoOwner.box, 4, 2);
//	isBlackHat = isBlack(owner);
//	if (!isBlackHat){
//		owner = 255 - owner;
//	}
//	mOwnerOcr = Mat(owner.rows + 20, owner.cols + 20, CV_8UC1);
//	mOwnerOcr = 255 - 0 * mOwnerOcr;
//	Mat ownerRoi = CreateMat(mOwnerOcr, Rect(10, 10, owner.cols, owner.rows));
//	owner.copyTo(ownerRoi);
//	ownerTxt = readText(mOwnerOcr, 0);
//	cv::imwrite(filepath + "\\owner.jpg", mOwnerOcr);
//
//	//去除所有\n
//	string::size_type pos = 0;
//	while ((pos = ownerTxt.find_first_of("\n", pos)) != string::npos)
//	{
//		ownerTxt.replace(pos, 1, "");
//		pos++;
//	}
//
//	//orc:货主代码分割+个体识别
//	int idx = 0;
//	int k = 0;
//	for (int i = 0; i< findResult.lineInfoOwner.box_info.size(); i++)
//	{
//		boxInfo boxinfo = findResult.lineInfoOwner.box_info[i];
//		for (int j = 0; j < boxinfo.type; j++){
//			double newX, newY;
//			float w, h;
//			if (orient == 0){
//				newX = boxinfo.box.center.x - boxinfo.box.size.width / 2 + boxinfo.box.size.width / boxinfo.type *(j + 0.5);
//				newY = boxinfo.box.center.y;
//				w = boxinfo.box.size.width / boxinfo.type;
//				h = boxinfo.box.size.height;
//				if (boxinfo.type <= 1){
//					w = w + 2;
//					h = h + 2;
//				}
//				else{
//					w = w - 1;
//					h = h - 1;
//				}
//			}
//			else{
//				newX = boxinfo.box.center.x;
//				newY = boxinfo.box.center.y - boxinfo.box.size.height / 2 + boxinfo.box.size.height / boxinfo.type *(j + 0.5);
//				w = boxinfo.box.size.width;
//				h = boxinfo.box.size.height / boxinfo.type;
//				w = w + 6;
//				h = h - 2;
//			}
//			RotatedRect b = RotatedRect(Point(newX, newY), Size(w, h), boxinfo.box.angle);
//			Mat charImg = getWarpImage(threshImg, b);
//			if (!isBlackHat){
//				charImg = 255 - charImg;
//			}
//			k++;
//			string txt = ocrProcess(charImg, idx, b, 0, "owner_" + to_string(k));
//			ownerTxt1 = ownerTxt1 + txt;
//			idx = idx + 1;
//		}
//	}
//}
//void OcrJzx::recognizeNumber(Mat threshImg){
//	numberTxt1 = "";
//	int idx = 4;
//	int k = 0;
//	//orc:箱号编码有双层时上层
//	if (findResult.lineInfoNo1.box_num_count > 0){
//		//sortLineInfo(&lineInfoNo1, 0, orient);
//		for (int i = 0; i < findResult.lineInfoNo1.box_info.size(); i++)
//		{
//			boxInfo boxinfo = findResult.lineInfoNo1.box_info[i];
//			for (int j = 0; j < boxinfo.type; j++){
//				double newX, newY;
//				float w, h;
//				if (orient == 0){
//					newX = boxinfo.box.center.x - boxinfo.box.size.width / 2 + boxinfo.box.size.width / boxinfo.type *(j + 0.5);
//					newY = boxinfo.box.center.y;
//					w = boxinfo.box.size.width / boxinfo.type;
//					h = boxinfo.box.size.height;
//					if (boxinfo.type <= 1){
//						w = w + 2;
//						h = h + 2;
//					}
//					else{
//						w = w - 1;
//						h = h - 1;
//					}
//				}
//				else{
//					newX = boxinfo.box.center.x;
//					newY = boxinfo.box.center.y - boxinfo.box.size.height / 2 + boxinfo.box.size.height / boxinfo.type *(j + 0.5);
//					w = boxinfo.box.size.width;
//					h = boxinfo.box.size.height / boxinfo.type;
//					w = w + 4;
//					h = h - 2;
//				}
//				RotatedRect b = RotatedRect(Point(newX, newY), Size(w, h), boxinfo.box.angle);
//				Mat charImg = getWarpImage(threshImg, b);
//				if (!isBlackHat){
//					charImg = 255 - charImg;
//				}
//				k++;
//				string txt = ocrProcess(charImg, idx, b, 1, "number_" + to_string(k));
//				numberTxt1 = numberTxt1 + txt;
//				idx = idx + 1;
//			}
//		}
//		if (orient == 0){
//			Mat imgNo1 = getWarpImage(threshImg, findResult.lineInfoNo1.box, 4, 2);
//			if (!isBlackHat){
//				imgNo1 = 255 - imgNo1;
//			}
//
//			mNumber1Ocr = Mat(imgNo1.rows + 20, imgNo1.cols + 20, CV_8UC1);
//			mNumber1Ocr = 255 - 0 * mNumber1Ocr;
//			Mat no1Roi = CreateMat(mNumber1Ocr, Rect(10, 10, imgNo1.cols, imgNo1.rows));
//			imgNo1.copyTo(no1Roi);
//			string txt = readText(mNumber1Ocr, 1);
//			numberTxt = txt;
//			cv::imwrite(filepath + "\\number11.jpg", mNumber1Ocr);
//
//		}
//	}
//	//ocr:箱号编码
//	for (int i = 0; i < findResult.lineInfoNo.box_info.size(); i++)
//	{
//		boxInfo boxinfo = findResult.lineInfoNo.box_info[i];
//		for (int j = 0; j < boxinfo.type; j++){
//			double newX, newY;
//			float w, h;
//			if (orient == 0){
//				newX = boxinfo.box.center.x - boxinfo.box.size.width / 2 + boxinfo.box.size.width / boxinfo.type *(j + 0.5);
//				newY = boxinfo.box.center.y;
//				w = boxinfo.box.size.width / boxinfo.type;
//				h = boxinfo.box.size.height;
//				if (boxinfo.type <= 1){
//					w = w + 1;
//					h = h + 1;
//				}
//				else{
//					w = w - 1;
//					h = h - 1;
//				}
//			}
//			else{
//				newX = boxinfo.box.center.x;
//				newY = boxinfo.box.center.y - boxinfo.box.size.height / 2 + boxinfo.box.size.height / boxinfo.type *(j + 0.5);
//				w = boxinfo.box.size.width;
//				h = boxinfo.box.size.height / boxinfo.type;
//				w = w + 8;
//				h = h;
//			}
//			RotatedRect b = RotatedRect(Point(newX, newY), Size(w, h), boxinfo.box.angle);
//			Mat charImg = getWarpImage(threshImg, b);
//			if (!isBlackHat){
//				charImg = 255 - charImg;
//			}
//			k++;
//			string txt = ocrProcess(charImg, idx, b, 1, "number_" + to_string(k));
//			numberTxt1 = numberTxt1 + txt;
//			idx = idx + 1;
//		}
//	}
//
//	if (orient == 0){
//		Mat imgNo = getWarpImage(threshImg, findResult.lineInfoNo.box, 4, 2);
//		if (!isBlackHat){
//			imgNo = 255 - imgNo;
//		}
//		mNumberOcr = Mat(imgNo.rows + 20, imgNo.cols + 20, CV_8UC1);
//		mNumberOcr = 255 - 0 * mNumberOcr;
//
//		Mat noRoi = CreateMat(mNumberOcr, Rect(10, 10, imgNo.cols, imgNo.rows));
//		imgNo.copyTo(noRoi);
//		string txt = readText(mNumberOcr, 1);
//		cv::imwrite(filepath + "\\number.jpg", mNumberOcr);
//		numberTxt = numberTxt + txt;
//	}
//	else{
//		numberTxt = numberTxt1;
//	}
//	//imshow("tmp", mNumberOcr);
//	//waitKey(1);
//}
//
//void OcrJzx::recognizeCheckdigit(Mat threshImg){
//	//ocr:校验码位
//	int idx = 10;
//	boxInfo chkDigitInfo = findResult.lineInfoCheckDigit.box_info[0];
//
//	//调整宽度：宽度过窄时增加宽度（如checkdigit=1时，最小外接矩形会全黑，需要增加宽度）
//	int diff = 0;
//	if (chkDigitInfo.box.size.width < findResult.mUwidth * 0.6){
//		diff = 4;
//	}
//
//	//正则变换图形
//	Mat checkImg = getWarpImage(threshImg, chkDigitInfo.box, diff, 0);
//	if (!isBlackHat){
//		checkImg = 255 - checkImg;
//	}
//
//	//出力到mOcr+识别单个文字
//	checkDigitTxt = ocrProcess(checkImg, idx, chkDigitInfo.box, 1, "checkdigit");
//
//	//拼接编号+校验位图像
//	numberTxt2 = "";
//	if (orient == 0){
//		Mat roi0, roi1, roi2;
//		if (findResult.lineInfoNo1.box_num_count>0){
//			mNumCheckdigitOcr = Mat(mNumberOcr.rows + 4, mNumber1Ocr.cols + mNumberOcr.cols + checkImg.cols + 20, CV_8UC1);
//			mNumCheckdigitOcr = 255 - 0 * mNumCheckdigitOcr;
//			roi0 = CreateMat(mNumCheckdigitOcr, Rect(2, 2, mNumber1Ocr.cols, mNumber1Ocr.rows));
//			roi1 = CreateMat(mNumCheckdigitOcr, Rect(mNumber1Ocr.cols + 1, 2, mNumberOcr.cols, mNumberOcr.rows));
//			roi2 = CreateMat(mNumCheckdigitOcr, Rect(mNumber1Ocr.cols + mNumberOcr.cols + 10, 10, checkImg.cols, checkImg.rows));
//			mNumber1Ocr.copyTo(roi0);
//			mNumberOcr.copyTo(roi1);
//			checkImg.copyTo(roi2);
//		}
//		else{
//			mNumCheckdigitOcr = Mat(mNumberOcr.rows + 4, mNumberOcr.cols + checkImg.cols + 20, CV_8UC1);
//			mNumCheckdigitOcr = 255 - 0 * mNumCheckdigitOcr;
//			roi1 = CreateMat(mNumCheckdigitOcr, Rect(2, 2, mNumberOcr.cols, mNumberOcr.rows));
//			roi2 = CreateMat(mNumCheckdigitOcr, Rect(mNumberOcr.cols + 10, 10, checkImg.cols, checkImg.rows));
//			mNumberOcr.copyTo(roi1);
//			checkImg.copyTo(roi2);
//		}
//		//整体识别：编号+校验位
//		numberTxt2 = readText(mNumCheckdigitOcr, 1);
//		cv::imwrite(filepath + "\\number2.jpg", mNumCheckdigitOcr);
//
//		//imshow("tmp2", mNumCheckdigitOcr);
//		//waitKey(1);
//	}
//}
//
//
//orc:识别和出力：货主代码
//void OcrJzx::recognizeOwner(Mat threshImg){
//	float uWidth = findResult.mUwidth * 2;
//	float uHeight = findResult.mUheight * 2;
//
//	Mat gray = getWarpImage(threshImg, findResult.lineInfoOwner.box, 4, 2, false);
//	Mat imgthresh = getWarpImage(threshImg, findResult.lineInfoOwner.box, 4, 2, true);
//	Mat imgthreshRev;
//	imgthreshRev = 255 - imgthresh;
//
//	ownerTxt = "";
//	//原图上找出货主代码部分并拗正
//	Mat owner = getWarpImage(threshImg, findResult.lineInfoOwner.box, 4, 2);
//
//	//先判断是否为白底黑字
//	isBlackHat = !isBlack(owner);
//
//	//根据判断反转黑白
//	if (!isBlackHat){
//		owner = 255 - owner;
//	}
//
//	//准备识别用图像(四周夹10像素空白！)
//	mOwnerOcr = Mat(owner.rows + 20, owner.cols + 20, CV_8UC1, Scalar(255)); //  threshImg.channels(), Scalar(255, 255, 255)); // CV_8UC1);
//	//mOwnerOcr = 255 - 0 * mOwnerOcr;
//
//	Mat ownerRoi = CreateMat(mOwnerOcr, Rect(10, 10, owner.cols, owner.rows));
//	owner.copyTo(ownerRoi);
//
//	//文字识别
//	vector<int> conf;
//	vector<string> txt;
//	ownerTxt1 = readText1(mOwnerOcr, 0, &txt, &conf);
//
//	//准备输出字符图像：竖排时需要特殊出力
//	if (orient == 0){
//		ownerTxt = ownerTxt1;
//	}
//	else
//	{
//		//竖向排列时：检出文字列中去除所有换行符\n
//		string::size_type pos = 0;
//		while ((pos = ownerTxt1.find_first_of("\n", pos)) != string::npos)
//		{
//			ownerTxt1.replace(pos, 1, "");
//			pos++;
//		}
//
//		//竖向排列时：分割字符后横向排列：准备输出用Mat
//		//orc:货主代码分割+个体识别
//		mOwnerOcr = Mat((int)uHeight + 20, (int)(uWidth + 20) * 4 + 10, CV_8UC1, Scalar(255));
//		//mOwnerOcr = 255 - 0 * mOwnerOcr;
//		int k = 0;
//		for (int i = 0; i < findResult.lineInfoOwner.box_info.size(); i++)
//		{
//			boxInfo boxinfo = findResult.lineInfoOwner.box_info[i];
//			for (int j = 0; j < boxinfo.type; j++){
//				float newX = boxinfo.box.center.x;
//				float newY = boxinfo.box.center.y - boxinfo.box.size.height / 2 + boxinfo.box.size.height / boxinfo.type *(j + 0.5);;
//				float w = boxinfo.box.size.width + 6;
//				float h = boxinfo.box.size.height / boxinfo.type - 2;
//				RotatedRect b = RotatedRect(Point(newX, newY), Size(w, h), boxinfo.box.angle);
//				Mat charImg = getWarpImage(threshImg, b);
//				//imshow("tmp3", charImg);
//				//waitKey(1);
//				if (!isBlackHat){
//					charImg = 255 - charImg;
//				}
//				Mat ocrRoi = CreateMat(mOwnerOcr, Rect((uWidth + 20)*k + 10, 10, charImg.cols, charImg.rows));
//				charImg.copyTo(ocrRoi);
//				//imshow("tmp4", mOwnerOcr);
//				//waitKey(1);				
//				k++;
//			}
//		}
//		ownerTxt = readText(mOwnerOcr, 0);
//	}
//	cv::imwrite(filepath + "\\owner.jpg", mOwnerOcr);
//
//}
//
////ocr：识别和出力：箱编号
//void OcrJzx::recognizeNumber(Mat threshImg){
//	float uWidth = findResult.mUwidth * 2;
//	float uHeight = findResult.mUheight * 2;
//	numberTxt = "";
//	//orc:箱号编码有双层时上层
//	if (findResult.lineInfoNo1.box_num_count > 0){
//		if (orient == 0){
//			//横向排列时
//			//首先高度补正（超过U字符高度时，可能发生粘连，减小高度）
//			if (findResult.lineInfoNo1.box.size.height > uHeight - 1){
//				findResult.lineInfoNo1.box.size.height = uHeight - 2;
//			}
//			//原图上找出上排箱编号部分并拗正
//			Mat num1 = getWarpImage(threshImg, findResult.lineInfoNo1.box, 4, 2);
//			//根据需要反转黑白
//			if (!isBlackHat){
//				num1 = 255 - num1;
//			}
//
//			//准备识别用图像(四周夹10像素空白！)
//			mNumber1Ocr = Mat(num1.rows + 20, num1.cols + 20, CV_8UC1, Scalar(255)); //CV_8UC1);
//			//mNumber1Ocr = 255 - 0 * mNumber1Ocr;
//			Mat num1Roi = CreateMat(mNumber1Ocr, Rect(10, 10, num1.cols, num1.rows));
//			num1.copyTo(num1Roi);
//		}
//		else
//		{
//			//竖向排列时：
//			//orc:上层编号分割+个体识别
//			mNumber1Ocr = Mat((int)uHeight + 20, (int)(uWidth + 20) * 3 + 10, CV_8UC1, Scalar(255)); // CV_8UC1);
//			//mNumber1Ocr = 255 - 0 * mNumber1Ocr;
//			int k = 0;
//			for (int i = 0; i < findResult.lineInfoNo1.box_info.size(); i++)
//			{
//				boxInfo boxinfo = findResult.lineInfoNo1.box_info[i];
//				for (int j = 0; j < boxinfo.type; j++){
//					float newX = boxinfo.box.center.x;
//					float newY = boxinfo.box.center.y - boxinfo.box.size.height / 2 + boxinfo.box.size.height / boxinfo.type *(j + 0.5);;
//					float w = boxinfo.box.size.width + 8;
//					float h = boxinfo.box.size.height / boxinfo.type;
//					RotatedRect b = RotatedRect(Point(newX, newY), Size(w, h), findResult.lineInfoNo1.box.angle);
//					Mat charImg = getWarpImage(threshImg, b);
//					if (!isBlackHat){
//						charImg = 255 - charImg;
//					}
//					Mat ocrRoi = CreateMat(mNumber1Ocr, Rect((uWidth + 20) * k + 10, 10, charImg.cols, charImg.rows));
//					charImg.copyTo(ocrRoi);
//					//imshow("tmp", mNumber1Ocr);
//					//waitKey(1);
//					k++;
//				}
//			}
//		}
//		//文字识别
//		numberTxt = readText(mNumber1Ocr, 1);
//		//识别用图片出力
//		cv::imwrite(filepath + "\\number1.jpg", mNumber1Ocr);
//	}
//
//	//ocr:箱号编码或下排编码出力
//	if (orient == 0){
//		//横向排列时
//		//首先高度补正（超过U字符高度时，可能发生粘连，减小高度）
//		if (findResult.lineInfoNo.box.size.height > uHeight - 1){
//			findResult.lineInfoNo.box.size.height = uHeight - 2;
//		}
//		//原图上找出箱编号或下排编号部分并拗正
//		Mat num = getWarpImage(threshImg, findResult.lineInfoNo.box, 4, 2);
//		//根据需要反转黑白
//		if (!isBlackHat){
//			num = 255 - num;
//		}
//
//		//准备识别用图像(四周加10像素空白！)
//		mNumberOcr = Mat(num.rows + 20, num.cols + 20, CV_8UC1, Scalar(255)); // CV_8UC1);
//		//mNumberOcr = 255 - 0 * mNumberOcr;
//		Mat numRoi = CreateMat(mNumberOcr, Rect(10, 10, num.cols, num.rows));
//		num.copyTo(numRoi);
//	}
//	else
//	{
//		//竖向排列时：
//		//orc:货主代码分割+个体识别
//		int count = findResult.lineInfoNo1.box_num_count > 0 ? 3 : 6;
//		mNumberOcr = Mat((int)uHeight + 20, (int)(uWidth + 20)* count + 10, CV_8UC1);
//		//mNumberOcr = 255 - 0 * mNumberOcr;
//		int k = 0;
//		for (int i = 0; i < findResult.lineInfoNo.box_info.size(); i++)
//		{
//			boxInfo boxinfo = findResult.lineInfoNo.box_info[i];
//			for (int j = 0; j < boxinfo.type; j++){
//				float newX = boxinfo.box.center.x;
//				float newY = boxinfo.box.center.y - boxinfo.box.size.height / 2 + boxinfo.box.size.height / boxinfo.type *(j + 0.5);;
//				float w = boxinfo.box.size.width + 8;
//				float h = boxinfo.box.size.height / boxinfo.type;
//				RotatedRect b = RotatedRect(Point(newX, newY), Size(w, h), findResult.lineInfoNo1.box.angle);
//				Mat charImg = getWarpImage(threshImg, b);
//				if (!isBlackHat){
//					charImg = 255 - charImg;
//				}
//				//Mat ocrRoi = CreateMat(mNumberOcr, Rect((uWidth+20) * k + 10, 10, charImg.cols, charImg.rows));
//				Mat ocrRoi = CreateMat(mNumberOcr, Rect((uWidth + 20) * k + 1, 1, charImg.cols, charImg.rows));
//				charImg.copyTo(ocrRoi);
//				//imshow("tmp", mNumberOcr);
//				//waitKey(1);
//				k++;
//			}
//		}
//	}
//	//文字识别
//	numberTxt = numberTxt + readText(mNumberOcr, 1);
//	//识别用图片出力
//	cv::imwrite(filepath + "\\number.jpg", mNumberOcr);
//	//imshow("tmp", mNumberOcr);
//	//waitKey(1);
//}