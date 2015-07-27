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
#include "lmContextDependent.h"

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
	bool topicscore = false;
	
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
                "topicscore", CMDBOOLTYPE|CMDMSG, &topicscore, "computes the topic scores of the text from standard input",
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
  if (topicscore==true) std::cerr << "topicscore: " << topicscore << std::endl;
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
	
	if (topicscore == true) {
		
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
		
		std::cerr << "Start Topic Score generation " << std::endl;
		std::cerr << "OOV code: " << lmt->getDict()->oovcode() << std::endl;
		std::cout.setf(ios::fixed);
		std::cout.precision(2);
		
		std::fstream inptxt(testfile,std::ios::in);
		
		// loop over input lines
		char line[MAX_LINE];
		while (inptxt.getline(line,MAX_LINE)) {
			
			std::string line_str = line;
			
			VERBOSE(0,"line_str:|" << line_str << "|" << std::endl);	
			
			//getting sentence string;
			std::string sentence;
			std::string context;
			
			
			((lmContextDependent*) lmt)->GetSentenceAndContext(sentence,context,line_str);
			VERBOSE(0,"sentence:|" << sentence << "|" << std::endl);	
			VERBOSE(0,"context:|" << context << "|" << std::endl);	
			VERBOSE(0,"line_str:|" << line_str << "|" << std::endl);
				
			//getting apriori topic weights
			topic_map_t apriori_topic_map;
			((lmContextDependent*) lmt)->getContextSimilarity()->setContextMap(apriori_topic_map,context);
			
			if(1){
				// computation using std::string
				// loop over ngrams of the sentence
				string_vec_t word_vec;
				split(sentence, ' ', word_vec);
				
				//first points to the last recent term to take into account
				//last points to the position after the most recent term to take into account
				//last could point outside the vector of string; do NOT use word_vec.at(last)
				size_t last, first;
				size_t size=0;
				size_t order = lmt->maxlevel();
				
				
				
				topic_map_t sentence_topic_map;
				VERBOSE(0,"word_vec.size():|" << word_vec.size() << "|" << std::endl);	
				for (size_t i=0; i<word_vec.size(); ++i){
					++size;
					size=(size<order)?size:order;
					last=i+1;
					// reset ngram at begin of sentence
					if (word_vec.at(i) == lmt->getDict()->BoS()) {
						size=0;
						continue;
					}
					first = last - size;
					
					VERBOSE(0,"topic scores for first:|" << first << "| last:|" << last << "| size:" << size << std::endl);
					string_vec_t tmp_word_vec(word_vec.begin() + first, word_vec.begin() +last);
					
					if (size>=1) {
						VERBOSE(2,"computing topic_scores for first:|" << first << "| and last:|" << last << "|" << std::endl);	
						
						topic_map_t tmp_topic_map;
						((lmContextDependent*) lmt)->getContextSimilarity()->get_topic_scores(tmp_topic_map, tmp_word_vec);
						
						VERBOSE(2,std::cout << "first:" << first << " last:" << last << ((lmContextDependent*) lmt)->getContextDelimiter());
						if (debug > 0){
							((lmContextDependent*) lmt)->getContextSimilarity()->print_topic_scores(tmp_topic_map);
						}
						((lmContextDependent*) lmt)->getContextSimilarity()->add_topic_scores(sentence_topic_map, tmp_topic_map);
						tmp_topic_map.clear();
					}
				}
				std::cout << sentence << ((lmContextDependent*) lmt)->getContextDelimiter();
				((lmContextDependent*) lmt)->getContextSimilarity()->print_topic_scores(sentence_topic_map);
			}
			
			apriori_topic_map.clear();
		}
		
		
		delete lmt;
		return 0;
	}
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
		std::cerr << "Start ContextBased Evaluation" << std::endl;
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
		
		std::fstream inptxt(testfile,std::ios::in);
		
		// loop over input lines
		char line[MAX_LINE];
		while (inptxt.getline(line,MAX_LINE)) {
			
			std::string line_str = line;
			
			VERBOSE(0,"line_str:|" << line_str << "|" << std::endl);	
						
			//getting sentence string;
			std::string sentence;
			std::string context;
			
			((lmContextDependent*) lmt)->GetSentenceAndContext(sentence,context,line_str);
			VERBOSE(0,"sentence:|" << sentence << "|" << std::endl);	
			VERBOSE(0,"context:|" << context << "|" << std::endl);	
			VERBOSE(0,"line_str:|" << line_str << "|" << std::endl);
			
			//getting apriori topic weights
			topic_map_t apriori_topic_map;
			((lmContextDependent*) lmt)->getContextSimilarity()->setContextMap(apriori_topic_map,context); 
			
			
			if(1){
				// computation using std::string
				// loop over ngrams of the sentence
				string_vec_t word_vec;
				split(sentence, ' ', word_vec);
				
				//first points to the last recent term to take into account
				//last points to the position after the most recent term to take into account
				//last could point outside the vector of string; do NOT use word_vec.at(last)
				size_t last, first;
				size_t size=0;
				size_t order = lmt->maxlevel();
				
				VERBOSE(0,"word_vec.size():|" << word_vec.size() << "|" << std::endl);	
				for (size_t i=0; i<word_vec.size(); ++i){
					++size;
					size=(size<order)?size:order;
					last=i+1;
					// reset ngram at begin of sentence
					if (word_vec.at(i) == lmt->getDict()->BoS()) {
						size=0;
						continue;
					}
					first = last - size;
					
					VERBOSE(0,"prob for first:|" << first << "| last:|" << last << "| size:" << size << std::endl);
					string_vec_t tmp_word_vec(word_vec.begin() + first, word_vec.begin() +last);
					
					if (size>=1) {
						VERBOSE(0,"computing prob for first:|" << first << "| and last:|" << last << "|" << std::endl);	
						Pr=lmt->clprob(tmp_word_vec, apriori_topic_map, &bow, &bol, &msp, &statesize);
						VERBOSE(0," --> prob for first:|" << first << "| and last:|" << last << "| is Pr=" << Pr << std::endl);	
						logPr+=Pr;
						sent_logPr+=Pr;
						VERBOSE(0,"sent_logPr:|" << sent_logPr << " logPr:|" << logPr << std::endl);	
						
						if (debug==1) {
							std::cout << "first:|" << first << "| and last:| [" << size-bol << "]" << " " << std::endl;
						}
					}
				}
			}
			
			if ((Nw % 100000)==0) {
				std::cerr << ".";
				lmt->check_caches_levels();
			}
			
			apriori_topic_map.clear();
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

