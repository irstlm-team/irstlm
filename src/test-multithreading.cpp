// $Id: test_multithreading.cpp 3677 2010-10-13 09:06:51Z bertoldi $

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

struct task {      //basic task info to run task
	void *ctx;
	std::vector<ngram> *in;
	std::vector<double> *out;
	int pos;
};



/********************************/
using namespace std;
using namespace irstlm;


static void *clprob_helper(void *argv){
	task t=*(task *)argv;
	(t.out)->at(t.pos) = ((lmContainer*) t.ctx)->clprob((t.in)->at(t.pos));
	/*
  std::cout << "clprob_helper()" << std::endl;
	std::cout << "### (lmContainer*) t.ctx):|" << (void*)((lmContainer*) t.ctx) << "|" << std::endl;
	std::cout << "### t.in:|" << (void*) t.in << "|" << std::endl;
	std::cout << "### t.out:|" << (void*) t.out << "|" << std::endl;
	std::cout << "### ng:|" << (t.in)->at(t.pos) << "|" << std::endl;
	std::cout << "### Pr:|" << (t.out)->at(t.pos) << "|" << std::endl;
*/

	return NULL;
};

void print_help(int TypeFlag=0){
  std::cerr << std::endl << "test_multithreading - test for multi-threading" << std::endl;
  std::cerr << std::endl << "USAGE:"  << std::endl;
	std::cerr << "       test_multithreading" << std::endl;
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
	
	std::cout << "argc:" << argc << std::endl;
/*
 if (argc > 0){
		usage();
		exit_error(IRSTLM_NO_ERROR);
	}
	*/
	
  GetParams(&argc, &argv, (char*) NULL);
	
	std::cout << "help:" << help << std::endl;
	std::cout << "lmfile:" << lmfile << std::endl;
	
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
		
		ng.dict->incflag(1);
		/*
		 int bos=ng.dict->encode(ng.dict->BoS());
		 int eos=ng.dict->encode(ng.dict->EoS());
		 */
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
/*
 for (size_t i=0 ; i<ngram_vec_size; ++i){
			std::cout << "prob_vec[" << i << "]=" << prob_vec[i] << " ng:|" << ngram_vec.at(i) << "|" << std::endl;
		}
*/		
		
		
    threadpool thpool=thpool_init(threads);
		
    int numtasks=ngram_vec_size;
    task *t=new task[numtasks];
		for (size_t i=0 ; i<ngram_vec_size; ++i){
			//prepare and assign tasks to threads
			t[i].ctx=lmt;
			t[i].in=&ngram_vec;
			t[i].out=&thread_prob_vec;
			t[i].pos=i;
//			std::cout << "creating thread_task .... lmt:" << (void*) lmt << " i:" << i << " ng:|" << ngram_vec.at(i) << "|" << std::endl;
			thpool_add_work(thpool, &clprob_helper, (void *)&t[i]);
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
		
  	std::cout << "There are " << errors << " errors" << std::endl;

	}
}
