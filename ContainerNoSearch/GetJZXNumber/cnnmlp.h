#ifndef LP_MLP_H_H
#define LP_MLP_H_H

//#include "util.h"
#include "numbernndoc.h"
//#include <wx/datetime.h>
//#include <ml.h>
//#include <wx/string.h>

typedef struct _MlpResult
{
	//wxString text;
	string text;
	double mse;
	bool isOk;
} MlpResult;

class CnnMlp
{
public:
	//CnnMlp(wxString xmlFile , int detectImagewidth = 28 , int detectImageHeight = 28);
	CnnMlp(string xmlFile, int detectImagewidth = 28, int detectImageHeight = 28);
	virtual ~CnnMlp();

	virtual MlpResult& Recognize(IplImage *src ,double thresh_value = 0.0f, double maxMse = 0.8);
	//void SetXmlFile(wxString xmlFile);
	void SetXmlFile(string xmlFile);
	MlpResult& GetResult();
	bool CheckXmlFileExists();

protected:
	//void SaveImage(IplImage *save_image , wxString fileName);
	void SaveImage(IplImage *save_image, string fileName);
	void TrimBinaryImage(IplImage *pBinImage);

	NumberCnnDoc* m_doc;
	//wxString m_xmlFile;
	string m_xmlFile;
	int m_detectImageWidth;
	int m_detectImageheight;
	MlpResult m_result;
};

#endif