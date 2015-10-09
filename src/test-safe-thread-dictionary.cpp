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

#define _WORD_PER_THREAD_ 100

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
	std::vector<std::string> *in;
	std::vector<int> *out;
	int start_pos;
	int end_pos;
};

/********************************/
using namespace std;
using namespace irstlm;


static void *encode_array_helper(void *argv){
	task_array t=*(task_array *)argv;
	
	IFVERBOSE(3){
	std::cout << "encode_array_helper()";
	std::cout << " ### (dictionary*) t.ctx):|" << (void*)((dictionary*) t.ctx) << "|";
	std::cout << " ### t.in:|" << (void*) t.in << "|";
	std::cout << " ### t.out:|" << (void*) t.out << "|";
	std::cout << " ### t.start_pos:|" << (int)  t.start_pos << "|";
	std::cout << " ### t.end_pos:|" << (int) t.end_pos << "|" << std::endl;
	}
	for (int i=t.start_pos; i<t.end_pos; ++i){
		(t.out)->at(i) = ((dictionary*) t.ctx)->encode((t.in)->at(i).c_str());
		((dictionary*) t.ctx)->incfreq((t.out)->at(i),1);
		VERBOSE(2, "encode_array_helper() in thread:|" << pthread_self()<< "| i:" << (int) i << " code:" << (int) (t.out)->at(i) << " freq:|" << (int) ((dictionary*) t.ctx)->freq((t.out)->at(i)) << "|" << std::endl);
	}
	
	return NULL;
};

void print_help(int TypeFlag=0){
  std::cerr << std::endl << "test-safe-thread-dictionary - test for multi-threading for dictionary" << std::endl;
  std::cerr << std::endl << "USAGE:"  << std::endl;
	std::cerr << "       test-safe-thread-dictionary" << std::endl;
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
		std::vector<int> code_vec;
		
		//step 1: reading input data
		std::string str;
		while(inptxt >> str) {
			word_vec.push_back(str);
		}
		
		//step 2: generating results with one single thread
		size_t word_vec_size = word_vec.size();
		
		code_vec.resize(word_vec_size);
		
		dictionary* dict = new dictionary((char*) NULL);
		dict->incflag(1);
		int c = -1000;
		for (size_t i=0 ; i<word_vec_size; ++i){
			c = dict->encode(word_vec.at(i).c_str());
			dict->incfreq(c,1);
			code_vec.at(i) = c;
		}
		dict->incflag(0);
		
		
		//step 3: creating threads and generating results with multi-threading
		threadpool thpool=thpool_init(threads);
		
		std::vector<int> thread_code_vec;
		thread_code_vec.resize(word_vec_size);
		
		dictionary* thread_dict = new dictionary((char*) NULL);
		thread_dict->incflag(1);
		int step=_WORD_PER_THREAD_;
		int numtasks=ceil((float)word_vec_size/step);
		VERBOSE(2, "word_vec_size:" << word_vec_size  << " threads:" << threads << " numtasks: " << numtasks << " step:" << step << std::endl);
		
		task_array *t=new task_array[numtasks];
		int thread_i=0;
		for (size_t j=0 ; j<word_vec_size ; j+=step){
			//prepare and assign tasks to threads
			
			t[thread_i].ctx=thread_dict;
			t[thread_i].in=&word_vec;
			t[thread_i].out=&thread_code_vec;
			t[thread_i].start_pos=j;
			t[thread_i].end_pos=(j+step<word_vec_size)?j+step:word_vec_size;
			thpool_add_work(thpool, &encode_array_helper, (void *)&t[thread_i]);
			
			VERBOSE(2, "creating thread_task .... dict:|" << (void*) dict << "| thread " << thread_i << " start_pos:" << t[thread_i].start_pos<< " end_pos:" << t[thread_i].end_pos << " in thread:|" << pthread_self()<< "|" << std::endl);
			
			++thread_i;
		}
		//join all threads
		thpool_wait(thpool);
		thread_dict->incflag(0);
		
		
		//step 4: checking correctness of results
		int errors=0;
		if (thread_dict->size() != dict->size()){
			std::cout << "thread_dict->size():" << thread_dict->size();
			std::cout << " vs dict->size():" << dict->size();
			std::cout << " ERROR" << std::endl;
		}
		
		for (size_t i=0 ; i<word_vec_size; ++i){
			int thread_c=thread_code_vec[i];
			int c=code_vec[i];
			if (thread_dict->freq(thread_c) != dict->freq(c)){
				std::cout << "word:|" << word_vec.at(i) << "|";
				std::cout << " thread_c=" << thread_c << " thread_dict->freq(thread_c)=" << thread_dict->freq(thread_c);
				std::cout << " vs c=" << c << " dict->freq(c)=" << dict->freq(c);
				std::cout << " ERROR" << std::endl;
				++errors;
			}
		}
		
  	std::cout << "There are " << (int)  errors << " errors in " << (int) word_vec_size << " word entries with " << (int) threads << " threads" << std::endl;
	}
}
