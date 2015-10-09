// $Id: test-safe-thread-dictionary.cpp 3677 2010-10-13 09:06:51Z bertoldi $

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

#define _AUGMENT_STEP_ 70

#include <iostream>
#include <string>
#include <stdlib.h>
#include <vector>
#include "cmd.h"
#include "util.h"
#include "dictionary.h"


#include <pthread.h>
#include "thpool.h"

#include <math.h>  //for ceil function

#include <pthread.h>

struct task_array {      //basic task info to run task
	void *ctx;
	void *in;
};

/********************************/
using namespace std;
using namespace irstlm;


static void *augment_helper(void *argv){
	task_array t=*(task_array *)argv;
	
	IFVERBOSE(3){
		std::cout << "augment_helper()";
		std::cout << " ### (dictionary*) t.ctx):|" << (void*)((dictionary*) t.ctx) << "|";
		std::cout << " ### t.in:|" << (void*) t.in << "|";
	}
	
	((dictionary*) t.ctx)->augment(((dictionary*) t.in));
	
	size_t in_size = ((dictionary*) t.in)->size();
	for (size_t h=0;h<in_size;++h){
		size_t c = ((dictionary*) t.ctx)->encode(((dictionary*) t.in)->decode(h));
		((dictionary*) t.ctx)->incfreq(c, ((dictionary*) t.in)->freq(h));
	}
	VERBOSE(2, "augment_helper() in thread:|" << pthread_self()<< "|" << std::endl);
	
	return NULL;
};

void print_help(int TypeFlag=0){
  std::cerr << std::endl << "test-safe-thread-dictionary-augment - test for multi-threading for dictionary, specific for some functions (augment, sort)" << std::endl;
  std::cerr << std::endl << "USAGE:"  << std::endl;
	std::cerr << "       test-safe-thread-dictionary-augment" << std::endl;
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
	char *testfile=NULL;
	int threads=1;
	
  DeclareParams((char*)
								
								"Help", CMDBOOLTYPE|CMDMSG, &help, "print this help",
								"h", CMDBOOLTYPE|CMDMSG, &help, "print this help",
								
								"test", CMDSTRINGTYPE|CMDMSG, &testfile, "test file",
								"thread", CMDINTTYPE|CMDMSG, &threads, "threads",
								
								(char *)NULL
								);
	
  GetParams(&argc, &argv, (char*) NULL);
	
	VERBOSE(1, "threads:" << threads << " testfile:" << testfile << std::endl);
	
	if (threads == 0){
		threads = 1;
		VERBOSE(1, "Impossible request for 0 threads; number of threads forced to 1" << std::endl);
	}
	
	if (help){
		usage();
		exit_error(IRSTLM_NO_ERROR);
	}
	
	
  if (testfile != NULL) {
		
		std::fstream inptxt(testfile,std::ios::in);
		
		std::vector<std::string> word_vec;
		
		//step 1: reading input data
		std::string str;
		while(inptxt >> str) {
			word_vec.push_back(str);
		}
		
		//step 1a: preparing reference data
		size_t word_vec_size = word_vec.size();
		
		dictionary* base_dict = new dictionary((char*) NULL);
		base_dict->incflag(1);
		int c = -1000;
		for (size_t i=0 ; i<word_vec_size; ++i){
			c = base_dict->encode(word_vec.at(i).c_str());
			base_dict->incfreq(c,1);
		}
		base_dict->genoovcode();
		base_dict->incflag(0);
		
		
		size_t augment_step=_AUGMENT_STEP_;
		size_t numparts=ceil((float)word_vec_size/augment_step);
		VERBOSE(0,"word_vec_size:" <<word_vec_size << " augment_step:" << augment_step << " numparts:" << numparts  << std::endl);
		
		std::vector< dictionary* > partial_dict;
		partial_dict.resize(numparts);
		size_t partial_start[numparts];
		size_t partial_end[numparts];
		size_t h=0;
		for (size_t j=0;j<numparts;++j){
			partial_start[j] = h;
			h += augment_step+1;
			partial_end[j] = (h<word_vec_size)?h:word_vec_size;
			partial_dict[j] = new dictionary((char*) NULL);
			
			partial_dict[j]->incflag(1);
			int c = -1000;
			for (size_t i=partial_start[j] ; i<partial_end[j]; ++i){
				c = partial_dict[j]->encode(word_vec.at(i).c_str());
				partial_dict[j]->incfreq(c,1);
			}
			partial_dict[j]->genoovcode();
			partial_dict[j]->incflag(0);
		}
		
		//step 2: generating results with one single thread

		dictionary* dict = new dictionary((char*) NULL);
		dict->incflag(1);
		for (size_t j=0;j<numparts;++j){
			dict->augment(partial_dict[j]);
			for (int h=0;h<partial_dict[j]->size();++h){
				size_t c = dict->encode(partial_dict[j]->decode(h));
				dict->incfreq(c, partial_dict[j]->freq(h));
//				VERBOSE(2, "b1 j:" << j << " h:" << h << " partial_word:|" << partial_dict[j]->decode(h)<< "|"<< std::endl);
//				VERBOSE(2, "b2 j:" << j << " c:" << c << "| dict_word:|" << dict->decode(c)<< "|" << std::endl);
			}
//			for (int h=0;h<dict->size();++h){
//				VERBOSE(2, "b3 j:" << j << " h:" << h << "| dict_word:|" << dict->decode(h)<< "|" << std::endl);
//			}
			
//			VERBOSE(2, "a3 j:" << j << " partial_size: " <<  partial_dict[j]->size() << " dict_size: " <<  dict->size() << " base_dict_size: " <<  base_dict->size() << std::endl);
		}
		dict->incflag(0);
		

		//step 3: creating threads and generating results with multi-threading
		threadpool thpool=thpool_init(threads);
		
		dictionary* thread_dict = new dictionary((char*) NULL);
		thread_dict->incflag(1);
		
		size_t numtasks=numparts;
//		VERBOSE(2, "word_vec_size:" << word_vec_size  << " threads:" << threads << " numtasks:" << numtasks << " augment_step:" << augment_step << std::endl);
		
		task_array *t=new task_array[numtasks];
		int thread_i=0;
		for (size_t j=0 ; j<numparts ; ++j){
			//prepare and assign tasks to threads
			
			t[thread_i].ctx=thread_dict;
			t[thread_i].in=partial_dict.at(j);
			thpool_add_work(thpool, &augment_helper, (void *)&t[thread_i]);
			
			VERBOSE(2, "creating thread_task .... dict:|" << (void*) dict << "| thread " << thread_i << " partial_dict:|" << t[thread_i].in << "|" << std::endl);
			
			++thread_i;
		}
		//join all threads
		thpool_wait(thpool);
		thread_dict->incflag(0);
		
		
		//step 4: checking correctness of results
		if (thread_dict->size() != dict->size()){
			std::cout << "thread_dict->size():" << thread_dict->size();
			std::cout << " vs dict->size():" << dict->size();
			std::cout << " ERROR" << std::endl;
		}
		if (thread_dict->size() != base_dict->size()){
			std::cout << "thread_dict->size():" << thread_dict->size();
			std::cout << " vs base_dict->size():" << base_dict->size();
			std::cout << " ERROR" << std::endl;
		}
		if (dict->size() != base_dict->size()){
			std::cout << "dict->size():" << dict->size();
			std::cout << " vs base_dict->size():" << base_dict->size();
			std::cout << " ERROR" << std::endl;
		}
		
		int errors=0;
		for (size_t i=0 ; i<word_vec_size; ++i){
			int thread_c=thread_dict->encode(word_vec.at(i).c_str());
			int c=dict->encode(word_vec.at(i).c_str());
			int base_c=base_dict->encode(word_vec.at(i).c_str());
			if (thread_dict->freq(thread_c) != dict->freq(c)){
				std::cout << "word:|" << word_vec.at(i) << "|";
				std::cout << " thread_c=" << thread_c << " thread_dict->freq(thread_c)=" << thread_dict->freq(thread_c);
				std::cout << " vs c=" << c << " dict->freq(c)=" << dict->freq(c);
				std::cout << " ERROR" << std::endl;
				++errors;
			}
			if (thread_dict->freq(thread_c) != base_dict->freq(base_c)){
				std::cout << "word:|" << word_vec.at(i) << "|";
				std::cout << " thread_c=" << thread_c << " thread_dict->freq(thread_c)=" << thread_dict->freq(thread_c);
				std::cout << " vs base_c=" << base_c << " base_dict->freq(base_c)=" << base_dict->freq(base_c);
				std::cout << " ERROR" << std::endl;
				++errors;
			}
		}
		
  	std::cout << "There are " << (int)  errors << " errors in " << (int) word_vec_size << " word entries with " << (int) threads << " threads" << std::endl;
	}
}
