// $Id: util.h 363 2010-02-22 15:02:45Z mfederico $

#ifndef IRSTLM_UTIL_H
#define IRSTLM_UTIL_H


#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <math.h>

using namespace std;

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

//random values between -1 and +1
#define MY_RAND (((float)random()/RAND_MAX)* 2.0 - 1.0)

#define UNUSED(x) { (void) x; }

#define LMTMAXLEV  20
#define MAX_LINE  100000

//0.000001 = 10^(-6)
//0.000000000001 = 10^(-12)
//1.000001 = 1+10^(-6)
//1.000000000001 = 1+10^(-12)
//0.999999 = 1-10^(-6)
//0.999999999999 = 1-10^(-12)
#define LOWER_SINGLE_PRECISION_OF_0 -0.000001
#define UPPER_SINGLE_PRECISION_OF_0 0.000001
#define LOWER_DOUBLE_PRECISION_OF_0 -0.000000000001
#define UPPER_DOUBLE_PRECISION_OF_0 0.000000000001
#define UPPER_SINGLE_PRECISION_OF_1 1.000001
#define LOWER_SINGLE_PRECISION_OF_1 0.999999
#define UPPER_DOUBLE_PRECISION_OF_1 1.000000000001
#define LOWER_DOUBLE_PRECISION_OF_1 0.999999999999

#define	IRSTLM_NO_ERROR		0
#define	IRSTLM_ERROR_GENERIC	1
#define	IRSTLM_ERROR_IO		2
#define	IRSTLM_ERROR_MEMORY	3
#define	IRSTLM_ERROR_DATA	4
#define	IRSTLM_ERROR_MODEL	5

#define BUCKET 10000
#define SSEED 50

typedef std::vector< std::string > string_vec_t;
typedef std::vector< double > double_vec_t;
typedef std::vector< float > float_vec_t;

class ngram;
class mfstream;

std::string gettempfolder();
std::string createtempName();
void createtempfile(mfstream  &fileStream, std::string &filePath, std::ios_base::openmode flags);

void removefile(const std::string &filePath);

void *MMap(int	fd, int	access, off_t	offset, size_t	len, off_t	*gap);
int Munmap(void	*p,size_t	len,int	sync);


// A couple of utilities to measure access time
void ResetUserTime();
void PrintUserTime(const std::string &message);
double GetUserTime();

void ShowProgress(long long current,long long total);

int parseWords(char *, const char **, int);
int parseline(istream& inp, int Order,ngram& ng,float& prob,float& bow);

//split a string into a vector of string according to one specified delimiter (char)
string_vec_t &split(const std::string &s, char delim, string_vec_t &elems);

void exit_error(int err, const std::string &msg="");

namespace irstlm{


	void* reallocf(void *ptr, size_t size);

}

//extern int tracelevel;
extern const int tracelevel;

#define IRSTLM_TRACE_ERR(str) do { std::cerr << str; } while (false)
//#define IRSTLM_TRACE_ERR(str) { std::cerr << str; }

#define VERBOSE(level,str) { if (tracelevel > level) { IRSTLM_TRACE_ERR("DEBUG_LEVEL:" << level << "/" << tracelevel << " "); IRSTLM_TRACE_ERR(str); } }
#define IFVERBOSE(level) if (tracelevel > level)


void MY_ASSERT(bool x);

float logsum(float a,float b);
float log10sum(float a,float b);
double logsum(double a,double b);
double log10sum(double a,double b);

double logistic_function(double x, double max=1.0, double steep=1.0);

#endif

