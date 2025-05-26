#ifndef C_MLP_NUMBER_CNN_H_H
#define C_MLP_NUMBER_CNN_H_H

#include "cnnmlp.h"
//#include <wx/string.h>

class NumberCnnMlp : public CnnMlp
{
public:
	NumberCnnMlp(string xmlFile = "data/number.dat", int detectImagewidth = 28 , int detectImageHeight = 28);
	~NumberCnnMlp() {};
};

#endif