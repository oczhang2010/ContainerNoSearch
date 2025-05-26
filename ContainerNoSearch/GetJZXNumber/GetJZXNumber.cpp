// GetJZXNumber.cpp : 定义 DLL 应用程序的导出函数。
//
#include "ocrJzx.h"
#include "stdafx.h"
#include "stdio.h"
#include "io.h"
#include "direct.h"
#include "fstream"
#include "detectJzx.h"


extern "C" __declspec(dllexport) const int __stdcall Add(int a1,int a2)
{
	//char* retData;
	//sprintf(retData, "%.*s%.*s", sizeof(in1), in1, sizeof(in2), in2);;
	int aa = a1 + a2;
	return aa;
}
extern "C" __declspec(dllexport) const int __stdcall GetJZXNumber(char* filename)
{
	double t = (double)cvGetTickCount();

	//出力フォルダを作成する
	string filedir = filename;
	filedir = filedir.substr(0, filedir.length() - 4);
	if (_access((filedir).c_str(), 6) == -1)  
	{
		_mkdir((filedir).c_str());           
	}

	//写真を読込
	Mat imgOri = imread(filename);
	if (imgOri.empty()){
		return false;
	}

	//写真サイズを統一：heightが480画素不満の場合，height＝480画素へ拡大
	Mat img;
	if (imgOri.rows > 0 && imgOri.rows > 480){
		double r = 480.0 / imgOri.rows;
		double w = floor(imgOri.cols*r);
		double h = 480;
		cv::resize(imgOri, img, Size(w, h));
	}
	else{
		imgOri.copyTo(img);
	}
	bool found = false;
	string ret = "NO DATA";
	DetectJzx* detect = new DetectJzx();
	found = detect->searchNumber(img, 6);
	if (!found){
		return 0;
	}

	Mat outImg = detect->OutputImage(img);
	if (imgOri.rows > 0 && imgOri.rows > 480){
		cv::resize(outImg, outImg, Size(imgOri.cols, imgOri.rows));
	}
	imwrite(filedir + "\\Data.jpg", outImg);

	const char* tessdata = "";
	OcrJzx* ocrJzx = new OcrJzx();
	ocrJzx->ocrApi = new tesseract::TessBaseAPI();
	//ocrJzx->ocrApi->Init(tessdata, "eng", tesseract::OEM_DEFAULT);
	ocrJzx->ocrApi->Init(tessdata, "cont+eng", tesseract::OEM_DEFAULT);
	ocrJzx->findResult = detect->findResult;
	ocrJzx->isBlackHat = detect->findResult.isBlackHat;
	ocrJzx->filepath = filedir;
	//ocrJzx->mOcr = img;
	//ocrJzx->recognizeInfo();
	ocrJzx->readInfo();

	ofstream fstream;
	fstream.open(filedir + "\\Result.dat");

	ret = ocrJzx->ownerTxt + "," + ocrJzx->ownerTxt1;
	fstream << "Owner=" << ret << ";\n";

	ret = ocrJzx->numberTxt + "," + ocrJzx->numberTxt1;
	fstream << "Number=" << ret  << ";\n";

	ret = ocrJzx->checkDigitTxt + "," + ocrJzx->checkDigitTxt1;
	fstream << "Digit=" << ret << ";\n";

	double diff = ((double)cvGetTickCount() - t) / ((double)cvGetTickFrequency()*1000.);
	fstream << "TimeDiff=" << diff << ";\n";
	fstream.close();
	return found ? 1 : 0;
}




