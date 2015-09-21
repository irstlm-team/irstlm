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

typedef std::pair<double,int> double_and_int_pair;

struct cmp_double_and_int_pair {
	//order first by the first field (double), and in case of equality by the second field (int)
	bool operator()(const double_and_int_pair& a, const double_and_int_pair& b) const {
		if (a.first < b.first){
			return true;
		}else if (a.first > b.first){
			return false;
		}else{
			if (a.second<b.second){
				return true;
			}else{
				return false;
			}
		}
	}
};

typedef std::map<int, double_and_int_pair> int_to_double_and_int_map;
//typedef std::map<double_and_int_pair,int,cmp_double_and_int_pair> double_and_int_to_int_map;
typedef std::map<double_and_int_pair,double_and_int_pair,cmp_double_and_int_pair> double_and_int_to_double_and_int_map;

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
	
	bool sent_flag = false;
	bool contextbasedscore = false;
	bool topicscore = false;
	bool rankscore = false;
	bool context_model_active = true;
	bool context_model_normalization = false;
  char *lexiconfile=NULL;
	
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
								"lexicon", CMDSTRINGTYPE|CMDMSG, &lexiconfile, "lexicon file contains associated words (required by rankscore)",
                "randcalls", CMDINTTYPE|CMDMSG, &randcalls, "computes N random calls on the specified text file",
								"r", CMDINTTYPE|CMDMSG, &randcalls, "computes N random calls on the specified text file",
                "contextbasedscore", CMDBOOLTYPE|CMDMSG, &contextbasedscore, "computes context-dependent probabilities and pseudo-perplexity of the text from standard input",
                "topicscore", CMDBOOLTYPE|CMDMSG, &topicscore, "computes the topic scores of the text from standard input",
                "rankscore", CMDBOOLTYPE|CMDMSG, &rankscore, "computes the avergae rank position of the text from standard input",
								"debug", CMDINTTYPE|CMDMSG, &debug, "verbose output for --eval option; default is 0",
								"d", CMDINTTYPE|CMDMSG, &debug, "verbose output for --eval option; default is 0",
                "level", CMDINTTYPE|CMDMSG, &requiredMaxlev, "maximum level to load from the LM; if value is larger than the actual LM order, the latter is taken",
								"l", CMDINTTYPE|CMDMSG, &requiredMaxlev, "maximum level to load from the LM; if value is larger than the actual LM order, the latter is taken",
                "dub", CMDINTTYPE|CMDMSG, &dub, "dictionary upperbound to compute OOV word penalty: default 10^7",
                "sentence", CMDBOOLTYPE|CMDMSG, &sent_flag, "computes perplexity at sentence level (identified through the end symbol)",
                "dict_load_factor", CMDFLOATTYPE|CMDMSG, &dictionary_load_factor, "sets the load factor for ngram cache; it should be a positive real value; default is 0",
                "ngram_load_factor", CMDFLOATTYPE|CMDMSG, &ngramcache_load_factor, "sets the load factor for ngram cache; it should be a positive real value; default is false",
                "context_model_active", CMDBOOLTYPE|CMDMSG, &context_model_active, "enable/disable context-dependent model (default is true)",
                "context_model_normalization", CMDBOOLTYPE|CMDMSG, &context_model_normalization, "enable/disable normalization of context-dependent model (default is false)",
								
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
  if (rankscore==true){
		std::cerr << "rankscore: " << rankscore << std::endl;
		
		if (lexiconfile == NULL) {
			usage();
			exit_error(IRSTLM_ERROR_DATA,"Warning: Please specify a lexicon file to read from");
		}
		std::cerr << "lexicon: " << lexiconfile << std::endl;
	}
  std::cerr << "loading up to the LM level " << requiredMaxlev << " (if any)" << std::endl;
  std::cerr << "dub: " << dub<< std::endl;
	
	
  //checking the language model type
  std::string infile(lmfile);
	
  lmContainer* lmt = lmContainer::CreateLanguageModel(infile,ngramcache_load_factor,dictionary_load_factor);
	
  lmt->setMaxLoadedLevel(requiredMaxlev);
	
  lmt->load(infile);
	((lmContextDependent*) lmt)->set_Active(context_model_active);
	((lmContextDependent*) lmt)->set_Normalized(context_model_normalization);
	
  if (dub) lmt->setlogOOVpenalty((int)dub);
	
  //use caches to save time (only if PS_CACHE_ENABLE is defined through compilation flags)
  lmt->init_caches(lmt->maxlevel());
	
	//read lexicon form file
	std::multimap< std::string, std::string > lexicon;
	if (lexiconfile == NULL) {
		fstream inp(lexiconfile,ios::in|ios::binary);
		std::string w1, w2;
		while (inp >> w1 >> w2){
			lexicon.insert(make_pair(w1,w2));
		}
	}
	
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
			
			VERBOSE(2,"input_line:|" << line_str << "|" << std::endl);
			
			//getting sentence string;
			std::string sentence;
			std::string context;
			
			((lmContextDependent*) lmt)->GetSentenceAndContext(sentence,context,line_str);
			
			//getting apriori topic weights
			topic_map_t apriori_topic_map;
			((lmContextDependent*) lmt)->getContextSimilarity()->setContextMap(apriori_topic_map,context);
			
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
				
				string_vec_t tmp_word_vec(word_vec.begin() + first, word_vec.begin() +last);
				
				if (size>=1) {
					VERBOSE(2,"computing topic_scores for first:|" << first << "| and last:|" << last << "|" << std::endl);	
					
					topic_map_t tmp_topic_map;
					((lmContextDependent*) lmt)->getContextSimilarity()->get_topic_scores(tmp_word_vec, tmp_topic_map);
					IFVERBOSE(2){
						VERBOSE(2,"before normalization word-based topic-distribution:");
						((lmContextDependent*) lmt)->getContextSimilarity()->print_topic_scores(tmp_topic_map);
					}
					((lmContextDependent*) lmt)->getContextSimilarity()->normalize_topic_scores(tmp_topic_map);
					IFVERBOSE(2){
						VERBOSE(2,"after normalization word-based topic-distribution:");
						((lmContextDependent*) lmt)->getContextSimilarity()->print_topic_scores(tmp_topic_map);
					}
					VERBOSE(2,"first:" << first << " last:" << last <<  " tmp_topic_map.size:" << tmp_topic_map.size() << std::endl);
					
					((lmContextDependent*) lmt)->getContextSimilarity()->add_topic_scores(sentence_topic_map, tmp_topic_map);
					IFVERBOSE(2){
						//						VERBOSE(2,"word-based topic-distribution:");
						//						((lmContextDependent*) lmt)->getContextSimilarity()->print_topic_scores(tmp_topic_map);
						VERBOSE(2,"word-based topic-distribution:");
						((lmContextDependent*) lmt)->getContextSimilarity()->print_topic_scores(tmp_topic_map,apriori_topic_map,1);
					}
					tmp_topic_map.clear();
					//					IFVERBOSE(2){
					//						VERBOSE(2,"sentence-based topic-distribution:");
					//						((lmContextDependent*) lmt)->getContextSimilarity()->print_topic_scores(sentence_topic_map);
					//						VERBOSE(2,"sentence-based topic-distribution:");
					//						((lmContextDependent*) lmt)->getContextSimilarity()->print_topic_scores(sentence_topic_map,apriori_topic_map);
					//					}
				}
			}
			IFVERBOSE(2){
				//						VERBOSE(2,"sentence-based topic-distribution:");
				//						((lmContextDependent*) lmt)->getContextSimilarity()->print_topic_scores(sentence_topic_map);
				VERBOSE(2,"sentence-based topic-distribution:");
				((lmContextDependent*) lmt)->getContextSimilarity()->print_topic_scores(sentence_topic_map,apriori_topic_map,last);
			}
			std::cout << sentence << ((lmContextDependent*) lmt)->getContextDelimiter();
			((lmContextDependent*) lmt)->getContextSimilarity()->print_topic_scores(sentence_topic_map);
			//			((lmContextDependent*) lmt)->getContextSimilarity()->print_topic_scores(sentence_topic_map,apriori_topic_map);			
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
		
		int Nw=0,Noov=0;
		double logPr=0,PP=0,PPwp=0,Pr;
		double norm_logPr=0,norm_PP=0,norm_PPwp=0,norm_Pr;
		double model_logPr=0,model_PP=0,model_PPwp=0,model_Pr;
		double model_norm_logPr=0,model_norm_PP=0,model_norm_PPwp=0,model_norm_Pr;
		
		double bow;
		int bol=0;
		char *msp;
		unsigned int statesize;
		
		// variables for storing sentence-based Perplexity
		int sent_Nw=0,sent_Noov=0;
		double sent_logPr=0,sent_PP=0,sent_PPwp=0;	
		double sent_norm_logPr=0,sent_norm_PP=0,sent_norm_PPwp=0;		
		double sent_model_logPr=0,sent_model_PP=0,sent_model_PPwp=0;		
		double sent_model_norm_logPr=0,sent_model_norm_PP=0,sent_model_norm_PPwp=0;		
		
		double oovpenalty = lmt->getlogOOVpenalty();
		double norm_oovpenalty = oovpenalty;
		
		VERBOSE(1,"oovpenalty:" << oovpenalty  << std::endl);	
		
		std::fstream inptxt(testfile,std::ios::in);
		
		// loop over input lines
		char line[MAX_LINE];
		while (inptxt.getline(line,MAX_LINE)) {
			
			std::string line_str = line;
			
			VERBOSE(1,"input_line:|" << line_str << "|" << std::endl);	
			
			//getting sentence string;
			std::string sentence;
			std::string context;
			
			((lmContextDependent*) lmt)->GetSentenceAndContext(sentence,context,line_str);
			
			//getting apriori topic weights
			topic_map_t apriori_topic_map;
			((lmContextDependent*) lmt)->getContextSimilarity()->setContextMap(apriori_topic_map,context); 
			
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
			
			for (size_t i=0; i< word_vec.size(); ++i){
				++size;
				size=(size<order)?size:order;
				last=i+1;
				
				// reset ngram at begin of sentence
				if (word_vec.at(i) == lmt->getDict()->BoS()) {
					size=0;
					continue;
				}
				first = last - size;
				
				string_vec_t tmp_word_vec(word_vec.begin() + first, word_vec.begin() +last);
				
				if (size>=1) {
					VERBOSE(2,"computing prob for first:|" << first << "| and last:|" << last << "|" << std::endl);
					
					VERBOSE(2,"word_vec.at(i):|" << word_vec.at(i) << "| lmt->getDict()->OOV():|" << lmt->getDict()->OOV() << "|" << std::endl);
					if (lmt->getDict()->encode(word_vec.at(i).c_str()) == lmt->getDict()->oovcode()) {
						Noov++;
						sent_Noov++;
					}
					Nw++;
					sent_Nw++;
					
					if ((Nw % 100000)==0) {
						std::cerr << ".";
						lmt->check_caches_levels();
					}
					
					
					VERBOSE(2,"tmp_word_vec.size:|" << tmp_word_vec.size() << "|" << std::endl);	
					VERBOSE(2,"dict.size:|" << lmt->getDict()->size() << "|" << std::endl);	
					
					double current_pr = lmt->clprob(tmp_word_vec, apriori_topic_map, &bow, &bol, &msp, &statesize);
/*
					double tot_pr = 0.0;
					if (context_model_normalization){
						tot_pr = ((lmContextDependent*) lmt)->total_clprob(tmp_word_vec, apriori_topic_map);
					}
*/
					
//					string_vec_t::iterator it=tmp_word_vec.end()-1;
					int current_pos = tmp_word_vec.size()-1;
					std::string current_word = tmp_word_vec.at(current_pos);
					
					int_to_double_and_int_map code_to_prob_and_code_map;
					double_and_int_to_double_and_int_map prob_and_code_to_prob_and_rank_map;
					
					//computation of the oracle probability. i.e. the maximum prob
					double best_pr = -1000000.0;
					int best_code = lmt->getlogOOVpenalty();
					
					
					
					/*
					 //loop over all words in the LM
					 dictionary* current_dict = lmt->getDict();
					 */
					
					//loop over a set of selected alternative words
					//populate the dictionary with all words associated with the current word
					dictionary* current_dict = new dictionary((char *)NULL,1000000);
					current_dict->incflag(1);
					
					std::pair <std::multimap< std::string, std::string>::iterator, std::multimap< std::string, std::string>::iterator> ret = lexicon.equal_range(current_word);
					for (std::multimap<std::string, std::string>::const_iterator it=ret.first; it!=ret.second; ++it)
					{
						if (current_word != (it->second).c_str()){
							//exclude the current word from the selected alternative words
							current_dict->encode((it->second).c_str());
						}
					}
					current_dict->incflag(0);
					
					double tot_pr = 0.0;
					for (int j=0; j<current_dict->size(); ++j)
					{
						//loop over all words in the LM
					  tmp_word_vec.at(current_pos) = current_dict->decode(j);
						IFVERBOSE(3){
							std::cout << "tmp_word_vec j:" << j;
							for (string_vec_t::const_iterator it2=tmp_word_vec.begin(); it2!=tmp_word_vec.end(); ++it2) {
								std::cout << " |" << (*it2) << "|";
							}
							std::cout << std::endl;
						}				
						
						double pr=lmt->clprob(tmp_word_vec, apriori_topic_map, &bow, &bol, &msp, &statesize);
						if (best_pr < pr){
							best_pr = pr;
							best_code = j;
							VERBOSE(3,"current_best:" << best_code << " current_word:|" << lmt->getDict()->decode(best_code) << "| best_prob:" << pow(10.0,best_pr) << " norm_best_prob:" << pow(10.0,best_pr - tot_pr) << std::endl);
						}
						tot_pr += pow(10.0,best_pr);
					}
					model_Pr = best_pr;
					VERBOSE(2,"model_best_code:" << best_code << " model_best_word:|" << lmt->getDict()->decode(best_code) << "| model_best_prob:" << pow(10.0,best_pr) << " tot_pr:" << tot_pr << std::endl);
					IFVERBOSE(3){
						for (int_to_double_and_int_map::const_iterator it3=code_to_prob_and_code_map.begin(); it3!=code_to_prob_and_code_map.end(); ++it3)
						{
							VERBOSE(3,"it3: word:" << it3->first << " pr:" << (it3->second).first << " word:" << (it3->second).second << std::endl);
						}
						
						for (double_and_int_to_double_and_int_map::const_iterator it3=prob_and_code_to_prob_and_rank_map.begin(); it3!=prob_and_code_to_prob_and_rank_map.end(); ++it3)
						{
							VERBOSE(3,"it3: pr:" << (it3->first).first << " second:" << (it3->first).second << " norm_pr:" << (it3->second).first << " rank:" << (it3->second).second << std::endl);
						}
					}

					norm_oovpenalty = oovpenalty;
					VERBOSE(2,"tot_pr:" << tot_pr << " oovpenalty:" << oovpenalty << " norm_oovpenalty:" << norm_oovpenalty << std::endl);	


					norm_Pr = current_pr - tot_pr;
					model_norm_Pr = model_Pr - tot_pr;
					VERBOSE(1,"Pr:" << Pr << " norm_Pr:" << norm_Pr << " model_Pr:" << model_Pr << " model_norm_Pr:" << model_norm_Pr << " current_code:" << lmt->getDict()->encode(word_vec.at(i).c_str()) << " current_word:|" << word_vec.at(i) << "| model_best_code:" << best_code << " model_best_word:|" << lmt->getDict()->decode(best_code) << "|" << std::endl);

					model_norm_logPr+=model_norm_Pr;
					sent_model_norm_logPr+=model_norm_Pr;
					norm_logPr+=norm_Pr;
					sent_norm_logPr+=norm_Pr;
					VERBOSE(2,"sent_model_norm_logPr:" << sent_model_norm_logPr << " model_norm_logPr:" << model_norm_logPr << std::endl);	
					VERBOSE(2,"sent_norm_logPr:" << sent_norm_logPr << " norm_logPr:" << norm_logPr << std::endl);	
					
					model_logPr+=model_Pr;
					sent_model_logPr+=model_Pr;
					logPr+=current_pr;
					sent_logPr+=Pr;
					VERBOSE(2,"sent_model_logPr:" << sent_model_logPr << " model_logPr:" << model_logPr << std::endl);	
					VERBOSE(2,"sent_logPr:" << sent_logPr << " logPr:" << logPr << std::endl);
					 
					
				}
			}
			
			if (sent_flag) {
				sent_model_norm_PP = exp((-sent_model_norm_logPr * M_LN10) / sent_Nw);
				sent_model_norm_PPwp = sent_model_norm_PP * (1 - 1/exp(sent_Noov *  norm_oovpenalty * M_LN10 / sent_Nw));
				sent_norm_PP = exp((-sent_norm_logPr * M_LN10) / sent_Nw);
				sent_norm_PPwp = sent_norm_PP * (1 - 1/exp(sent_Noov * norm_oovpenalty * M_LN10 / sent_Nw));
				
				
				std::cout << "%% sent_Nw=" << sent_Nw
				<< " sent_norm_logPr=" << sent_norm_logPr
				<< " sent_norm_PP=" << sent_norm_PP
				<< " sent_norm_PPwp=" << sent_norm_PPwp
				<< " sent_norm_PP_noOOV=" << (sent_norm_PP-sent_norm_PPwp)
				<< " sent_Noov=" << sent_Noov
				<< " sent_OOVrate=" << (float)sent_Noov/sent_Nw * 100.0 << "%" << std::endl;
				std::cout << "%% sent_Nw=" << sent_Nw
				<< " sent_model_norm_logPr=" << sent_model_norm_logPr
				<< " sent_model_norm_PP=" << sent_model_norm_PP
				<< " sent_model_norm_PPwp=" << sent_model_norm_PPwp
				<< " sent_model_norm_PP_noOOV=" << (sent_model_norm_PP-sent_model_norm_PPwp)
				<< " sent_Noov=" << sent_Noov
				<< " sent_OOVrate=" << (float)sent_Noov/sent_Nw * 100.0 << "%" << std::endl;
				
				sent_model_PP = exp((-sent_model_logPr * M_LN10) / sent_Nw);
				sent_model_PPwp = sent_model_PP * (1 - 1/exp(sent_Noov * oovpenalty * M_LN10 / sent_Nw));
				sent_PP = exp((-sent_logPr * M_LN10) / sent_Nw);
				sent_PPwp = sent_PP * (1 - 1/exp(sent_Noov * oovpenalty * M_LN10 / sent_Nw));
				std::cout << "%% sent_Nw=" << sent_Nw
				<< " sent_logPr=" << sent_logPr
				<< " sent_PP=" << sent_PP
				<< " sent_PPwp=" << sent_PPwp
				<< " sent_PP_noOOV=" << (sent_PP-sent_PPwp)
				<< " sent_Noov=" << sent_Noov
				<< " sent_OOVrate=" << (float)sent_Noov/sent_Nw * 100.0 << "%" << std::endl;
			
				std::cout << "%% sent_Nw=" << sent_Nw
				<< " sent_model_logPr=" << sent_model_logPr
				<< " sent_model_PP=" << sent_model_PP
				<< " sent_model_PPwp=" << sent_model_PPwp
				<< " sent_model_PP_noOOV=" << (sent_model_PP-sent_model_PPwp)
				<< " sent_Noov=" << sent_Noov
				<< " sent_OOVrate=" << (float)sent_Noov/sent_Nw * 100.0 << "%" << std::endl;
				std::cout.flush();
				//reset statistics for sentence based Perplexity
				sent_Noov = 0;
				sent_Nw = 0;
				sent_model_norm_logPr = 0.0;
				sent_model_logPr = 0.0;
				sent_norm_logPr = 0.0;
				sent_logPr = 0.0;
			}
			
			apriori_topic_map.clear();
		}
		
		
		model_norm_PP = exp((-model_norm_logPr * M_LN10) / Nw);
		model_norm_PPwp = model_norm_PP * (1 - 1/exp(Noov *  norm_oovpenalty * M_LN10 / Nw));
		model_PP = exp((-model_logPr * M_LN10) / Nw);
		model_PPwp = model_PP * (1 - 1/exp(Noov * oovpenalty * M_LN10 / Nw));
		norm_PP = exp((-norm_logPr * M_LN10) / Nw);
		norm_PPwp = norm_PP * (1 - 1/exp(Noov *  norm_oovpenalty * M_LN10 / Nw));
		PP = exp((-logPr * M_LN10) / Nw);
		PPwp = PP * (1 - 1/exp(Noov * oovpenalty * M_LN10 / Nw));
		
		std::cout << "%% Nw=" << Nw
		<< " model_logPr=" << model_logPr
		<< " model_PP=" << model_PP
		<< " model_PPwp=" << model_PPwp
		<< " model_PP_noOOV=" << (model_PP-model_PPwp)
		<< " Noov=" << Noov
		<< " OOVrate=" << (float)Noov/Nw * 100.0 << "%";
		std::cout << std::endl;
		std::cout.flush();
		std::cout << "%% Nw=" << Nw
		<< " model_norm_logPr=" << model_norm_logPr
		<< " model_norm_PP=" << model_norm_PP
		<< " model_norm_PPwp=" << model_norm_PPwp
		<< " model_norm_PP_noOOV=" << (model_norm_PP-model_norm_PPwp)
		<< " Noov=" << Noov
		<< " OOVrate=" << (float)Noov/Nw * 100.0 << "%";
		std::cout << std::endl;
		std::cout.flush();
		std::cout << "%% Nw=" << Nw
		<< " logPr=" << logPr
		<< " PP=" << PP
		<< " PPwp=" << PPwp
		<< " PP_noOOV=" << (PP-PPwp)
		<< " Noov=" << Noov
		<< " OOVrate=" << (float)Noov/Nw * 100.0 << "%";
		std::cout << std::endl;
		std::cout.flush();
		std::cout << "%% Nw=" << Nw
		<< " norm_logPr=" << norm_logPr
		<< " norm_PP=" << norm_PP
		<< " norm_PPwp=" << norm_PPwp
		<< " norm_PP_noOOV=" << (norm_PP-norm_PPwp)
		<< " Noov=" << Noov
		<< " OOVrate=" << (float)Noov/Nw * 100.0 << "%";
		std::cout << std::endl;
		std::cout.flush();
		
		if (debug>1) lmt->used_caches();
		
		if (debug>1) lmt->stat();
		
		delete lmt;
		return 0;
	}
  if (rankscore == true) {
		
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
		std::cerr << "Start RankBased Evaluation" << std::endl;
		std::cerr << "OOV code: " << lmt->getDict()->oovcode() << std::endl;
		std::cout.setf(ios::fixed);
		std::cout.precision(2);
		
		int Nw=0,Noov=0;
		double avgRank;
		int tot_rank = 0;
		int max_rank = 0;
		
		/*
		//collect total occurrences of current word in the following intervals
		// [firs position], [<=1%], [<=2%], [<=5%], [<=10%]
		int Rank_histogram[5];
		int Rank_limit[5];
		*/
		
		/*
		int max_rank = lmt->getDict()->size();
		double ratio = 0.001;
		
		double Rank_perc[5];
		Rank_perc[0] = 0; Rank_limit[0] = 1;
		Rank_perc[1] =  1 * ratio; Rank_limit[1] = Rank_perc[1] * max_rank;
		Rank_perc[2] =  2 * ratio; Rank_limit[2] = Rank_perc[2] * max_rank;
		Rank_perc[3] =  5 * ratio; Rank_limit[3] = Rank_perc[3] * max_rank;
		Rank_perc[4] = 10 * ratio; Rank_limit[4] = Rank_perc[4] * max_rank;
		
		VERBOSE(1, "Rank thresholds: Rank_[bst]=1" << 
						" Rank_[1]=" << Rank_perc[1]*100 <<"%<=" << Rank_limit[1] << 
						" Rank_[2]=" << Rank_perc[2]*100 <<"%<=" << Rank_limit[2] << 
						" Rank_[3]=" << Rank_perc[3]*100 <<"%<=" << Rank_limit[3] << 
						" Rank_[4]=" << Rank_perc[4]*100 <<"%<=" << Rank_limit[4] << 
						std::endl);
		*/
		
		/*
		Rank_limit[0] = 1;
		Rank_limit[1] =  10;
		Rank_limit[2] =  20;
		Rank_limit[3] =  50;
		Rank_limit[4] = 100;
		
		VERBOSE(1, "Rank thresholds: Rank_[bst]=1" << 
						" Rank_[1]=" << Rank_limit[1] << 
						" Rank_[2]=" << Rank_limit[2] << 
						" Rank_[3]=" << Rank_limit[3] << 
						" Rank_[4]=" << Rank_limit[4] << 
						std::endl);
		
		Rank_histogram[0] = 0;
		Rank_histogram[1] = 0;
		Rank_histogram[2] = 0;
		Rank_histogram[3] = 0;
		Rank_histogram[4] = 0;
		*/
		
		double bow;
		int bol=0;
		char *msp;
		unsigned int statesize;
		
		// variables for storing sentence-based Rank Statistics
		int sent_Nw=0,sent_Noov=0;
		double sent_avgRank;
		int sent_tot_rank = 0;
		int sent_id = 0;
		
		std::fstream inptxt(testfile,std::ios::in);
		
		// loop over input lines
		char line[MAX_LINE];
		while (inptxt.getline(line,MAX_LINE)) {
			
			std::string line_str = line;
			
			VERBOSE(1,"input_line:|" << line_str << "|" << std::endl);	
			
			//getting sentence string;
			std::string sentence;
			std::string context;
			
			((lmContextDependent*) lmt)->GetSentenceAndContext(sentence,context,line_str);
			
			//getting apriori topic weights
			topic_map_t apriori_topic_map;
			((lmContextDependent*) lmt)->getContextSimilarity()->setContextMap(apriori_topic_map,context); 
			
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

			std::stringstream rank_outstr;
			
			for (size_t word_pos=0; word_pos<word_vec.size(); ++word_pos){
				++size;
				size=(size<order)?size:order;
				last=word_pos+1;
				
				// reset ngram at begin of sentence
				if (word_vec.at(word_pos) == lmt->getDict()->BoS()) {
					size=0;
					continue;
				}
				first = last - size;
				
				string_vec_t tmp_word_vec(word_vec.begin() + first, word_vec.begin() +last);
				
				if (size>=1) {
					
					VERBOSE(2,"computing rank for first:|" << first << "| and last:|" << last << "|" << std::endl);	
					
					VERBOSE(2,"word_vec.at(word_pos):|" << word_vec.at(word_pos) << "| lmt->getDict()->OOV():|" << lmt->getDict()->OOV() << "|" << std::endl);
					
					if (lmt->getDict()->encode(word_vec.at(word_pos).c_str()) == lmt->getDict()->oovcode()) {
						Noov++;
						sent_Noov++;
					}
					Nw++;
					sent_Nw++;
					
					if ((Nw % 100000)==0) {
						std::cerr << ".";
						lmt->check_caches_levels();
					}
					
					VERBOSE(2,"tmp_word_vec.size:|" << tmp_word_vec.size() << "|" << std::endl);	
					VERBOSE(2,"dict.size:|" << lmt->getDict()->size() << "|" << std::endl);	
					string_vec_t::iterator it=tmp_word_vec.end()-1;
					
					int current_pos = tmp_word_vec.size()-1;
					std::string current_word = tmp_word_vec.at(current_pos);
					
					double current_pr=lmt->clprob(tmp_word_vec, apriori_topic_map, &bow, &bol, &msp, &statesize);
					double tot_pr = 0.0;
					
					/*
					if (context_model_normalization){
						tot_pr = ((lmContextDependent*) lmt)->total_clprob(tmp_word_vec, apriori_topic_map);
						current_pr = current_pr - tot_pr;
					}
*/
					
					/*
					 //loop over all words in the LM
					 dictionary* current_dict = lmt->getDict();
					*/
					
					//loop over a set of selected alternative words
					//populate the dictionary with all words associated with the current word
					dictionary* current_dict = new dictionary((char *)NULL,1000000);
					current_dict->incflag(1);
					
					std::pair <std::multimap< std::string, std::string>::iterator, std::multimap< std::string, std::string>::iterator> ret = lexicon.equal_range(current_word);
					for (std::multimap<std::string, std::string>::const_iterator it=ret.first; it!=ret.second; ++it)
					{
						if (current_word != (it->second).c_str()){
							//exclude the current word from the selected alternative words
							current_dict->encode((it->second).c_str());
						}
					}
					current_dict->incflag(0);
					
					max_rank = current_dict->size()+1; //we add 1, to count for the current word as well, which is not included in the selected alternative words
					int current_rank = 1;
					for (int i=0; i<current_dict->size(); i++)
					{
					  tmp_word_vec.at(current_pos) = current_dict->decode(i);
						IFVERBOSE(3){
							std::cout << "tmp_word_vec i:" << i;
							for (string_vec_t::const_iterator it2=tmp_word_vec.begin(); it2!=tmp_word_vec.end(); ++it2) {
								std::cout << " |" << (*it2) << "|";
							}
							std::cout << std::endl;
						}
						double pr=lmt->clprob(tmp_word_vec, apriori_topic_map, &bow, &bol, &msp, &statesize);
						if (context_model_normalization){
							pr = pr - tot_pr;
						}
						if (pr > current_pr){
							++current_rank;	
						}
						
						VERBOSE(3," current_pos:" << current_pos << " word:|" << tmp_word_vec.at(current_pos) << "| current_pr:" << current_pr << " pr:" << pr << " current_rank:" << current_rank <<std::endl);
					}
					delete current_dict;
					/* loop over the whole dictionary
					int current_rank = 1;
					//computation of the ranking of the current word (among all LM words)
					for (int i=0; i<lmt->getDict()->size(); i++)
					{
						//loop over all words in the LM
					  tmp_word_vec.at(current_pos) = lmt->getDict()->decode(i);
						//					  *it = lmt->getDict()->decode(i);
						IFVERBOSE(3){
							std::cout << "tmp_word_vec i:" << i;
							for (string_vec_t::const_iterator it2=tmp_word_vec.begin(); it2!=tmp_word_vec.end(); ++it2) {
								std::cout << " |" << (*it2) << "|";
							}
							std::cout << std::endl;
						}
						double pr=lmt->clprob(tmp_word_vec, apriori_topic_map, &bow, &bol, &msp, &statesize);
						if (context_model_normalization){
							pr = pr - tot_pr;
						}
						if (pr > current_pr){
							++current_rank;	
						}
					}
					 */
					
					
					/*
					int_to_double_and_int_map code_to_prob_and_code_map;
					double_and_int_to_double_and_int_map prob_and_code_to_prob_and_rank_map;
					
					//computation of the ranking
					int default_rank=-1;
					for (int i=0; i<lmt->getDict()->size(); i++)
					{
						//loop over all words in the LM
					  tmp_word_vec.at(current_pos) = lmt->getDict()->decode(i);
						//					  *it = lmt->getDict()->decode(i);
						IFVERBOSE(3){
							std::cout << "tmp_word_vec i:" << i;
							for (string_vec_t::const_iterator it2=tmp_word_vec.begin(); it2!=tmp_word_vec.end(); ++it2) {
								std::cout << " |" << (*it2) << "|";
							}
							std::cout << std::endl;
						}
						double pr=lmt->clprob(tmp_word_vec, apriori_topic_map, &bow, &bol, &msp, &statesize);
						if (context_model_normalization){
							pr = pr - tot_pr;
						}
						code_to_prob_and_code_map.insert(make_pair(i,make_pair(pr,i)));
						prob_and_code_to_prob_and_rank_map.insert(make_pair(make_pair(pr,i),make_pair(pr,default_rank)));
						VERBOSE(3," i:" << i << " word_prob:" << pr << std::endl);
					}
					IFVERBOSE(3){
						
						for (int_to_double_and_int_map::const_iterator it3=code_to_prob_and_code_map.begin(); it3!=code_to_prob_and_code_map.end();++it3)
						{
							VERBOSE(3,"it3: word:" << it3->first << " pr:" << (it3->second).first << " word:" << (it3->second).second << std::endl);
						}
						
						for (double_and_int_to_double_and_int_map::const_iterator it3=prob_and_code_to_prob_and_rank_map.begin(); it3!=prob_and_code_to_prob_and_rank_map.end();++it3)
						{
							VERBOSE(3,"it3: pr:" << (it3->first).first << " second:" << (it3->first).second << " norm_pr:" << (it3->second).first << " rank:" << (it3->second).second << std::endl);
						}
					}					
					//set rank of word according to their prob
					//note that prob are already sorted in the map in ascending order wrt to prob (and secondarily code)
					int rank=0;
					for (double_and_int_to_double_and_int_map::reverse_iterator it3=prob_and_code_to_prob_and_rank_map.rbegin(); it3!=prob_and_code_to_prob_and_rank_map.rend();++it3)
					{
						(it3->second).second = rank;
						++rank;
						IFVERBOSE(3){
							if (rank < 10){
								VERBOSE(3,"it3: pr:" << (it3->first).first << " code:" << (it3->first).second << " rank:" << (it3->second).second << std::endl);
							}
						}
					 }


					IFVERBOSE(3){
						int i_tmp=0;
						for (double_and_int_to_double_and_int_map::const_reverse_iterator it3=prob_and_code_to_prob_and_rank_map.rbegin(); it3!=prob_and_code_to_prob_and_rank_map.rend();++it3)
						{
							VERBOSE(3,"it3: pr:" << (it3->first).first << " code:" << (it3->first).second <<" rank:" << (it3->second).second << std::endl);
							if (++i_tmp==10) break;
						}
					}
					double_and_int_pair current_prob_and_code = (code_to_prob_and_code_map.find(current_code))->second;
					int current_rank = ((prob_and_code_to_prob_and_rank_map.find(current_prob_and_code))->second).second;
					VERBOSE(1," current_word:" << current_word << " current_code:" << current_code << " current_rank:" << current_rank << std::endl);
					 
					 */
					
					sent_tot_rank += current_rank;
					tot_rank += current_rank;
					/*
					if (current_rank <= Rank_limit[0]){
						++Rank_histogram[0];
						VERBOSE(1,"HERE 0 current_rank:" << current_rank << " Rank_limit[0]:" << Rank_limit[0] << std::endl);
					}
					if (current_rank <= Rank_limit[1]){
						++Rank_histogram[1]; ++Rank_histogram[2]; ++Rank_histogram[3]; ++Rank_histogram[4];
						VERBOSE(1,"HERE 1 current_rank:" << current_rank << " Rank_limit[1]:" << Rank_limit[1] << std::endl);
					}else if (current_rank <= Rank_limit[2]){
						++Rank_histogram[2]; ++Rank_histogram[3]; ++Rank_histogram[4];
						VERBOSE(1,"HERE 2 current_rank:" << current_rank << " Rank_limit[2]:" << Rank_limit[2] << std::endl);
					}else if (current_rank <= Rank_limit[3]){
						++Rank_histogram[3]; ++Rank_histogram[4];
						VERBOSE(1,"HERE 3 current_rank:" << current_rank << " Rank_limit[3]:" << Rank_limit[3] << std::endl);
					}else if (current_rank <= Rank_limit[4]){
						++Rank_histogram[4];
						VERBOSE(1,"HERE 4 current_rank:" << current_rank << " Rank_limit[4]:" << Rank_limit[4] << std::endl);
					}
					*/
					if (debug>1){
						//output format:
						//word_pos:current_rank:max_rank
						rank_outstr << " " << word_pos << ":" << current_rank << ":" << max_rank;
					}
				}
			}
			
			if (sent_flag) {
				if (debug>1){
					VERBOSE(1," sent_tot_rank:" << sent_tot_rank << " sent_Nw:" << sent_Nw << std::endl);
					//output format: a blank-separated list of triplets
					//current_pos:current_rank:max_rank
					std::cout << "sent_id=" << sent_id << " ranking= " << rank_outstr.str() << std::endl;
				}
				
				sent_avgRank = ((double) sent_tot_rank)  / sent_Nw;
				
				std::cout << "%% sent_Nw=" << sent_Nw
				<< " sent_avgRank=" << sent_avgRank
				<< " sent_Noov=" << sent_Noov
				<< " sent_OOVrate=" << (float)sent_Noov/sent_Nw * 100.0 << "%";
				std::cout << std::endl;
				std::cout.flush();
				
				//reset statistics for sentence based avg Ranking
				sent_Nw = 0;
				sent_Noov = 0;
				sent_tot_rank = 0;
				++sent_id;
			}
		}
		
		avgRank = ((double) tot_rank) / Nw;
		
		std::cout << "%% Nw=" << Nw
		<< " avgRank=" << avgRank
		<< " Noov=" << Noov
		<< " OOVrate=" << (float)Noov/Nw * 100.0 << "%";
		/*
		std::cout << " Rank_[bst]=" << Rank_histogram[0];
		std::cout << " Rank_[1]=" << Rank_histogram[1];
		std::cout << " Rank_[2]=" << Rank_histogram[2];
		std::cout << " Rank_[3]=" << Rank_histogram[3];
		std::cout << " Rank_[4]=" << Rank_histogram[4];
		std::cout << " Rank_[bst]=" << (float)Rank_histogram[0]/Nw * 100.0 << "%";
		std::cout << " Rank_[1]=" << (float)Rank_histogram[1]/Nw * 100.0 << "%";
		std::cout << " Rank_[2]=" << (float)Rank_histogram[2]/Nw * 100.0 << "%";
		std::cout << " Rank_[3]=" << (float)Rank_histogram[3]/Nw * 100.0 << "%";
		std::cout << " Rank_[4]=" << (float)Rank_histogram[4]/Nw * 100.0 << "%";
		 */
		std::cout << std::endl;
		std::cout.flush();
		
		if (debug>1) lmt->used_caches();
		
		if (debug>1) lmt->stat();
		
		delete lmt;
		return 0;
	}
}

