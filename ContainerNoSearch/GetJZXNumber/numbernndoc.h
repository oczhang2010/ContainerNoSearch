#ifndef C_MLP_NUMBER_CNN_DOC_H_H
#define C_MLP_NUMBER_CNN_DOC_H_H

#include "NeuralNetwork.h"
#include "error.h"
//#include <wx/string.h>

#define NUMBER_CNN_IMAGE_WIDTH 28
#define NUMBER_CNN_IMAGE_HEIGHT 28
#define NUMBER_CNN_MLP_VER_COUNT 10

#define PLACE_CNN_IMAGE_WIDTH 60
#define PLACE_CNN_IMAGE_HEIGHT 60
#define PLACE_CNN_IMAGE_SIZE 3721
#define GAUSSIAN_FIELD_SIZE 21

class NumberCnnDoc
{
public:
	//NumberCnnDoc(wxString xmlFile,int classCount);
	NumberCnnDoc(string xmlFile, int classCount);
	~NumberCnnDoc();
	// thread entry point
	//wxString Recognize(IplImage* image , bool pDistort = true);
	string Recognize(IplImage* image, bool pDistort = true);
	std::vector<int> RecognizeKana(IplImage* image, double maxMse);
	//wxString RecognizePlace(IplImage* image , bool pDistort = false);
	string RecognizePlace(IplImage* image, bool pDistort = false);
	std::vector<int> NumberCnnDoc::RecognizeBigKana(IplImage* image, double maxMse);
	double GetMse();

protected:
	void ApplyDistortionMap( double* inputVector );
	void GenerateDistortionMap( double severityFactor = 1.0 );
	void CalculateNeuralNet(double *inputVector, int count, 
								   double* outputVector /* =NULL */, int oCount /* =0 */,
								   std::vector< std::vector< double > >* pNeuronOutputs /* =NULL */,
								   BOOL bDistort /* =FALSE */);
	inline double& At( double* p, int row, int col )  // zero-based indices, starting at bottom-left
		{ int location = row * m_cCols + col;
		  Assert( location>=0 && location<m_cCount && row<m_cRows && row>=0 && col<m_cCols && col>=0 , "At");
		  return p[ location ];
		}

	NeuralNetwork m_NN;

	int m_classCount;
	double m_mse;
	int m_cBackprops;
	int m_nAfterEveryNBackprops;
	double m_dEtaDecay;
	double m_dMinimumEta;
	bool  m_bNeedHessian;
	int m_cCols;
	int m_cRows;
	int m_cCount;
	double* m_DispH;  // horiz distortion map array
	double* m_DispV;  // vert distortion map array
	double m_GaussianKernel[ GAUSSIAN_FIELD_SIZE ] [ GAUSSIAN_FIELD_SIZE ];
	std::vector< VectorDoubles > m_NeuronOutputs;
};

#endif