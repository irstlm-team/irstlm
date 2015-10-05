// $Id: test-safe-thread-probability.cpp 3677 2010-10-13 09:06:51Z bertoldi $

/******************************************************************************
 IrstLM: IRST Language Model Toolkit, compile LM
 Copyright (C) 2006 Marcello Federico, ITC-irst Trento, Italy
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 
 ******************************************************************************/

#define _NGRAM_PER_THREAD_ 100

#include <iostream>
#include <string>
#include <stdlib.h>
#include <vector>
#include "cmd.h"
#include "util.h"
#include "n_gram.h"
#include "mdiadapt.h"
#include "lmContainer.h"


#include <pthread.h>
#include "thpool.h"

#include <math.h>  //for ceil function

struct task_array {      //basic task info to run task
	void *ctx;
	std::vector<ngram> *in;
	std::vector<double> *out;
	int start_pos;
	int end_pos;
};

/********************************/
using namespace std;
using namespace irstlm;

static void *clprob_array_helper(void *argv){
	task_array t=*(task_array *)argv;
	
	IFVERBOSE(3){
	std::cout << "clprob_helper()";
	std::cout << " ### (lmContainer*) t.ctx):|" << (void*)((lmContainer*) t.ctx) << "|";
	std::cout << " ### t.in:|" << (void*) t.in << "|";
	std::cout << " ### t.out:|" << (void*) t.out << "|";
	std::cout << " ### t.start_pos:|" << (int)  t.start_pos << "|";
	std::cout << " ### t.end_pos:|" << (int) t.end_pos << "|" << std::endl;
	}
	for (int i=t.start_pos; i<t.end_pos; ++i){
//		std::cout << "clprob_helper() i:|" << i << "|" << std::endl;
		(t.out)->at(i) = ((lmContainer*) t.ctx)->clprob((t.in)->at(i));
	}
	
	return NULL;
};

void print_help(int TypeFlag=0){
  std::cerr << std::endl << "test-safe-thread-probability - test for multi-threading for probability queries" << std::endl;
  std::cerr << std::endl << "USAGE:"  << std::endl;
	std::cerr << "       test-safe-thread-probability" << std::endl;
	std::cerr << std::endl << "DESCRIPTION:" << std::endl;
	std::cerr << std::endl << "OPTIONS:" << std::endl;
	
	FullPrintParams(TypeFlag, 0, 1, stderr);
}

void usage(const char *msg = 0)
{
  if (msg) {
    std::cerr << msg << std::endl;
  }
	if (!msg){
		print_help();
	}
}

int main(int argc, char **argv)
{	
	bool help=false;
	char *lmfile=NULL;
	char *testfile=NULL;
	int threads=1;
	
  DeclareParams((char*)
								
								"Help", CMDBOOLTYPE|CMDMSG, &help, "print this help",
								"h", CMDBOOLTYPE|CMDMSG, &help, "print this help",
								
								"lm", CMDSTRINGTYPE|CMDMSG, &lmfile, "config file",
								"test", CMDSTRINGTYPE|CMDMSG, &testfile, "test file",
								"thread", CMDINTTYPE|CMDMSG, &threads, "threads",
								
								(char *)NULL
								);
	
  GetParams(&argc, &argv, (char*) NULL);
	
	VERBOSE(1, "threads:" << threads << std::endl);
	VERBOSE(1, "lmfile:" << lmfile << std::endl);
	VERBOSE(1, "testfile:" << testfile << std::endl);
	
	if (threads == 0){
		threads = 1;
		VERBOSE(1, "Impossible request for 0 threads; number of threads forced to 1" << std::endl);
	}
	
	if (help){
		usage();
		exit_error(IRSTLM_NO_ERROR);
	}
	
	if (!lmfile) {
		exit_error(IRSTLM_ERROR_DATA,"The LM file (-lm) is not specified");
	}
	
  lmContainer* lmt = lmContainer::CreateLanguageModel(lmfile);
	
  lmt->load(lmfile);
	
  if (testfile != NULL) {
		
		std::fstream inptxt(testfile,std::ios::in);
		
		std::vector<ngram> ngram_vec;
		std::vector<double> prob_vec;
		std::vector<double> thread_prob_vec;
		
		
		ngram ng(lmt->getDict());
		ng.dict->incflag(0);
		
		while(inptxt >> ng) {
			if (ng.size>=1) {
				ngram_vec.push_back(ng);
			}
		}
		
		size_t ngram_vec_size = ngram_vec.size();
		
		prob_vec.resize(ngram_vec_size);
		thread_prob_vec.resize(ngram_vec_size);
		
		double Pr = -1000;
		for (size_t i=0 ; i<ngram_vec_size; ++i){
			Pr = lmt->clprob(ngram_vec.at(i));
			prob_vec.at(i) = Pr;
		}
		IFVERBOSE(1){
			for (size_t i=0 ; i<ngram_vec_size; ++i){
				std::cout << "prob_vec[" << i << "]=" << prob_vec[i] << " ng:|" << ngram_vec.at(i) << "|" << std::endl;
			}
		}
		
		threadpool thpool=thpool_init(threads);
		
		int step=_NGRAM_PER_THREAD_;
		int numtasks=ceil((float)ngram_vec_size/step);
		VERBOSE(2, "ngram_vec_size:" << ngram_vec_size  << " threads:" << threads << " numtasks: " << numtasks << " step:" << step << std::endl);
		
		task_array *t=new task_array[numtasks];
		int thread_i=0;
		for (size_t j=0 ; j<ngram_vec_size ; j+=step){
			//prepare and assign tasks to threads
			
			t[thread_i].ctx=lmt;
			t[thread_i].in=&ngram_vec;
			t[thread_i].out=&thread_prob_vec;
			t[thread_i].start_pos=j;
			t[thread_i].end_pos=(j+step<ngram_vec_size)?j+step:ngram_vec_size;
			thpool_add_work(thpool, &clprob_array_helper, (void *)&t[thread_i]);
			
			VERBOSE(2, "creating thread_task .... lmt:" << (void*) lmt << " thread " << thread_i << " start_pos:" << t[thread_i].start_pos<< " end_pos:" << t[thread_i].end_pos << std::endl);
			
			++thread_i;
		}
		//join all threads
		thpool_wait(thpool);
		
		
		
		int errors=0;
		for (size_t i=0 ; i<ngram_vec_size; ++i){
			if (thread_prob_vec[i] != prob_vec[i]){
				std::cout << "thread_prob_vec[" << i << "]=" << thread_prob_vec[i];
				std::cout << " ERROR" << std::endl;
				++errors;
			}
		}
		
  	std::cout << "There are " << errors << " errors in " << (int) ngram_vec_size << " ngram prob queries with " << (int) threads << " threads" << std::endl;
	}
}
