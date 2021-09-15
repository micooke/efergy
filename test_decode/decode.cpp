#include <iostream>
#include <cmath>

#define _DEBUG 2 // 2 for non-Arduino debugging
#define EFERGY_SAMPLE_DATA
#include "../efergy_structs.h"

int main()
{
	efergy::S0.print();
	efergy::S1.print();
	efergy::S2.print();
	return 0;
}