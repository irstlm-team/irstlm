// $Id: compile-lm.cpp 3677 2010-10-13 09:06:51Z bertoldi $

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
#include <sstream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include "cmd.h"
#include "util.h"
#include "math.h"
#include "lmContainer.h"

using namespace std;
using namespace irstlm;

/********************************/
void print_help(int TypeFlag=0){
  std::cerr << std::endl << "context-dependent-evaluation - compute ngram probabilities and text perplexity given a LM" << std::endl;
  std::cerr << std::endl << "USAGE:"  << std::endl;
	std::cerr << "       context-dependent-evaluation [options] lm=<input-file.lm>" << std::endl;
	std::cerr << std::endl << "DESCRIPTION:" << std::endl;
	std::cerr << "       context-dependent-evaluation uses the given LM to compute ngram probabilities and text perplexity of the input" << std::endl;
	std::cerr << "       The LM must be in a IRSTLM-compliant type" << std::endl;
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
  char *testfile=NULL;
  char *lmfile=NULL;
	
	bool sent_PP_flag = false;
	bool contextbasedscore = false;
	
	int debug = 0;
  int requiredMaxlev = 1000;
  int dub = 10000000;
  int randcalls = 0;
  float ngramcache_load_factor = 0.0;
  float dictionary_load_factor = 0.0;
	
  bool help=false;
	
	DeclareParams((char*)
								"lm", CMDSTRINGTYPE|CMDMSG, &lmfile, "LM to load",
								"test", CMDSTRINGTYPE|CMDMSG, &testfile, "computes scores of the specified text file",
                "randcalls", CMDINTTYPE|CMDMSG, &randcalls, "computes N random calls on the specified text file",
								"r", CMDINTTYPE|CMDMSG, &randcalls, "computes N random calls on the specified text file",
                "contextbasedscore", CMDBOOLTYPE|CMDMSG, &contextbasedscore, "computes context-dependent probabilities and pseudo-perplexity of the text from standard input",
                "cbs", CMDBOOLTYPE|CMDMSG, &contextbasedscore, "computes context-dependent probabilities and pseudo-perplexity of the text from standard input",
								"debug", CMDINTTYPE|CMDMSG, &debug, "verbose output for --eval option; default is 0",
								"d", CMDINTTYPE|CMDMSG, &debug, "verbose output for --eval option; default is 0",
                "level", CMDINTTYPE|CMDMSG, &requiredMaxlev, "maximum level to load from the LM; if value is larger than the actual LM order, the latter is taken",
								"l", CMDINTTYPE|CMDMSG, &requiredMaxlev, "maximum level to load from the LM; if value is larger than the actual LM order, the latter is taken",
                "dub", CMDINTTYPE|CMDMSG, &dub, "dictionary upperbound to compute OOV word penalty: default 10^7",
                "sentence", CMDBOOLTYPE|CMDMSG, &sent_PP_flag, "computes perplexity at sentence level (identified through the end symbol)",
                "dict_load_factor", CMDFLOATTYPE|CMDMSG, &dictionary_load_factor, "sets the load factor for ngram cache; it should be a positive real value; default is 0",
                "ngram_load_factor", CMDFLOATTYPE|CMDMSG, &ngramcache_load_factor, "sets the load factor for ngram cache; it should be a positive real value; default is false",
								
								"Help", CMDBOOLTYPE|CMDMSG, &help, "print this help",
								"h", CMDBOOLTYPE|CMDMSG, &help, "print this help",
								
                (char*)NULL
								);
	
	if (argc == 1){
		usage();
		exit_error(IRSTLM_NO_ERROR);
	}
	
	GetParams(&argc, &argv, (char*) NULL);
	
	if (help){
		usage();
		exit_error(IRSTLM_NO_ERROR);
	}	
	
	if (lmfile == NULL) {
		usage();
		exit_error(IRSTLM_ERROR_DATA,"Warning: Please specify a LM file to read from");
	}
	
	if (testfile == NULL) {
		usage();
		exit_error(IRSTLM_NO_ERROR);
	}
	
  if (lmfile!=NULL) std::cerr << "lmfile: " << lmfile << std::endl;
  if (testfile!=NULL) std::cerr << "testfile: " << testfile << std::endl;
  if (contextbasedscore==true) std::cerr << "contextbasedscore: " << contextbasedscore << std::endl;
  std::cerr << "loading up to the LM level " << requiredMaxlev << " (if any)" << std::endl;
  std::cerr << "dub: " << dub<< std::endl;
	
	
  //checking the language model type
  std::string infile(lmfile);
	
  lmContainer* lmt = lmContainer::CreateLanguageModel(infile,ngramcache_load_factor,dictionary_load_factor);
	
  lmt->setMaxLoadedLevel(requiredMaxlev);
	
  lmt->load(infile);

  if (dub) lmt->setlogOOVpenalty((int)dub);
	
  //use caches to save time (only if PS_CACHE_ENABLE is defined through compilation flags)
  lmt->init_caches(lmt->maxlevel());
	
  if (contextbasedscore == true) {
		
		if (lmt->getLanguageModelType() == _IRSTLM_LMINTERPOLATION) {
			debug = (debug>4)?4:debug;
			std::cerr << "Maximum debug value for this LM type: " << debug << std::endl;
		}
		if (lmt->getLanguageModelType() == _IRSTLM_LMMACRO) {
			debug = (debug>4)?4:debug;
			std::cerr << "Maximum debug value for this LM type: " << debug << std::endl;
		}
		if (lmt->getLanguageModelType() == _IRSTLM_LMCLASS) {
			debug = (debug>4)?4:debug;
			std::cerr << "Maximum debug value for this LM type: " << debug << std::endl;
		}
		std::cerr << "Start Eval" << std::endl;
		std::cerr << "OOV code: " << lmt->getDict()->oovcode() << std::endl;
		std::cout.setf(ios::fixed);
		std::cout.precision(2);
		
		int Nbo=0, Nw=0,Noov=0;
		double logPr=0,PP=0,PPwp=0,Pr;
		
		double bow;
		int bol=0;
		char *msp;
		unsigned int statesize;
		
		// variables for storing sentence-based Perplexity
		int sent_Nbo=0, sent_Nw=0,sent_Noov=0;
		double sent_logPr=0,sent_PP=0,sent_PPwp=0;
		
		
		ngram ng(lmt->getDict());
		ng.dict->incflag(1);
		int bos=ng.dict->encode(ng.dict->BoS());
		int eos=ng.dict->encode(ng.dict->EoS());
		ng.dict->incflag(0);
		
		
		const std::string context_delimiter="___CONTEXT___";
		const char topic_map_delimiter='=';
		
		string_vec_t topic_weight_vec;
		string_vec_t topic_weight;
		
		std::fstream inptxt(testfile,std::ios::in);
		
		// loop over input lines
		char line[MAX_LINE];
		while (inptxt.getline(line,MAX_LINE)) {
			
			std::string line_str = line;
			
			VERBOSE(0,"line_str:|" << line_str << "|" << std::endl);	
						
			//getting sentence string;
			std::string sentence;
			std::string context;
			
			size_t pos = line_str.find(context_delimiter);	
			if (pos != std::string::npos){ // context_delimiter is found
				sentence = line_str.substr(0, pos);
				std::cout << sentence << std::endl;
				line_str.erase(0, pos + context_delimiter.length());
				VERBOSE(0,"pos:|" << pos << "|" << std::endl);	
				VERBOSE(0,"sentence:|" << sentence << "|" << std::endl);	
				VERBOSE(0,"line_str:|" << line_str << "|" << std::endl);	
				
				//getting context string;
				context = line_str;
			}else{
				sentence = line_str;
				context = "";
			}
			VERBOSE(0,"context:|" << context << "|" << std::endl);	
			VERBOSE(0,"line_str:|" << line_str << "|" << std::endl);	
			//getting topic weights
			topic_map_t topic_weight_map;
			
			split(context, ' ', topic_weight_vec);
			for (string_vec_t::iterator it=topic_weight_vec.begin(); it!=topic_weight_vec.end(); ++it){
				split(*it, topic_map_delimiter, topic_weight);
				topic_weight_map[topic_weight.at(0)] = strtod (topic_weight.at(1).c_str(), NULL);
				topic_weight.clear();
			}
			topic_weight_vec.clear();
			
			lmt->dictionary_incflag(1);
			
			
			if(1){
				// computation using std::string
				// loop over ngrams of the sentence
				string_vec_t w_vec;
				split(sentence, ' ', w_vec);
				
				size_t last, first;
				size_t size=0;
				size_t order = lmt->maxlevel();
				for (size_t i=0; i<w_vec.size(); ++i){
					++size;
					size=(size<order)?size:order;
					last=i+1;
					// reset ngram at begin of sentence
					if (w_vec.at(i) == lmt->getDict()->BoS()) {
						size=0;
						continue;
					}
					first = last - size;
					
					VERBOSE(0,"prob for first:|" << first << "| and last:|" << last << "| size:" << size << std::endl);
					string_vec_t tmp_w_vec(w_vec.begin() + first, w_vec.begin() +last);
					
					for (string_vec_t::iterator it=tmp_w_vec.begin(); it!=tmp_w_vec.end(); ++it){
						
						VERBOSE(0,"*it:|" << *it << "|" << std::endl);	
					}
					if (ng.size>=1) {
						Pr=lmt->clprob(tmp_w_vec, topic_weight_map, &bow, &bol, &msp, &statesize);
						VERBOSE(0,"prob for first:|" << first << "| and last:|" << last << "| is Pr=" << Pr << std::endl);	
						logPr+=Pr;
						sent_logPr+=Pr;
						VERBOSE(0,"sent_logPr:|" << sent_logPr << " logPr:|" << logPr << std::endl);	
						
						if (debug==1) {
							std::cout << "first:|" << first << "| and last:| [" << size-bol << "]" << " " << std::endl;
						}
					}
				}
			}
			
			if(0){
			// computation using ngram object
			// loop over ngrams of the sentence
			std::istringstream ss(sentence); // Insert the string into a stream
			while (ss >> ng){
				//computing context-based prob for each ngram of the sentence
				VERBOSE(0,"working on ng:|" << ng << "| ng.size:" << ng.size << std::endl);	
				
				if (ng.size>lmt->maxlevel()) ng.size=lmt->maxlevel();	
				
				// reset ngram at begin of sentence
				if (*ng.wordp(1)==bos) {
					ng.size=1;
					continue;
				}
				
				if (ng.size>=1) {
					Pr=lmt->clprob(ng,topic_weight_map, &bow, &bol, &msp, &statesize);
					VERBOSE(0,"prob for ng:|" << ng << "| is Pr=" << Pr << std::endl);	
					logPr+=Pr;
					sent_logPr+=Pr;
					VERBOSE(0,"sent_logPr:|" << sent_logPr << " logPr:|" << logPr << std::endl);	
					
					if (debug==1) {
						std::cout << ng.dict->decode(*ng.wordp(1)) << " [" << ng.size-bol << "]" << " ";
						if (*ng.wordp(1)==eos) std::cout << std::endl;
					} else if (debug==2) {
						std::cout << ng << " [" << ng.size-bol << "-gram]" << " " << Pr;
						std::cout << std::endl;
						std::cout.flush();
					} else if (debug==3) {
						std::cout << ng << " [" << ng.size-bol << "-gram]" << " " << Pr << " bow:" << bow;
						std::cout << std::endl;
						std::cout.flush();
					} else if (debug==4) {
						std::cout << ng << " [" << ng.size-bol << "-gram: recombine:" << statesize << " state:" << (void*) msp << "] [" << ng.size+1-((bol==0)?(1):bol) << "-gram: bol:" << bol << "] " << Pr << " bow:" << bow;
						std::cout << std::endl;
						std::cout.flush();
					}
				}
				
				if (lmt->is_OOV(*ng.wordp(1))) {
					Noov++;
					sent_Noov++;
				}
				if (bol) {
					Nbo++;
					sent_Nbo++;
				}
				Nw++;
				sent_Nw++;
				if (sent_PP_flag && (*ng.wordp(1)==eos)) {
					sent_PP=exp((-sent_logPr * log(10.0)) /sent_Nw);
					sent_PPwp= sent_PP * (1 - 1/exp((sent_Noov *  lmt->getlogOOVpenalty()) * log(10.0) / sent_Nw));
					
					std::cout << "%% sent_Nw=" << sent_Nw
					<< " sent_PP=" << sent_PP
					<< " sent_PPwp=" << sent_PPwp
					<< " sent_Nbo=" << sent_Nbo
					<< " sent_Noov=" << sent_Noov
					<< " sent_OOV=" << (float)sent_Noov/sent_Nw * 100.0 << "%" << std::endl;
					std::cout.flush();
					//reset statistics for sentence based Perplexity
					sent_Nw=sent_Noov=sent_Nbo=0;
					sent_logPr=0.0;
				}
			}
			}
			if ((Nw % 100000)==0) {
				std::cerr << ".";
				lmt->check_caches_levels();
			}
			
			topic_weight_map.clear();
		}
		
		
		PP=exp((-logPr * log(10.0)) /Nw);
		
		PPwp= PP * (1 - 1/exp((Noov *  lmt->getlogOOVpenalty()) * log(10.0) / Nw));
		
		std::cout << "%% Nw=" << Nw
		<< " PP=" << PP
		<< " PPwp=" << PPwp
		<< " Nbo=" << Nbo
		<< " Noov=" << Noov
		<< " OOV=" << (float)Noov/Nw * 100.0 << "%";
		if (debug) std::cout << " logPr=" <<  logPr;
		std::cout << std::endl;
		std::cout.flush();
		
		if (debug>1) lmt->used_caches();
		
		if (debug>1) lmt->stat();
		
		delete lmt;
		return 0;
	}
}

