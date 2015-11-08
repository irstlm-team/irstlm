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

void usage(const char *msg = 0){
  if (msg) {
    std::cerr << msg << std::endl;
  }
	if (!msg){
		print_help();
	}
}

void load_lexicon(const char* lexfile, std::multimap< std::string, std::string >& lexicon){
	if (lexfile!= NULL) {
		fstream inp(lexfile,ios::in|ios::binary);
		std::string w1, w2;
		while (inp >> w1 >> w2){
			lexicon.insert(make_pair(w1,w2));
		}
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
	
	bool add_lexicon_words = false;
	bool add_lm_words = false;
	bool add_sentence_words = false;
	bool add_full_dictionary = false;
	int successor_limit=100;
	
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
                "rankscore", CMDBOOLTYPE|CMDMSG, &rankscore, "computes the average rank position of the text from standard input",
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
                "add_lexicon_words", CMDBOOLTYPE|CMDMSG, &add_lexicon_words, "enable/disable addition of the words in the lexicon into the alternatives (default is false)",
                "add_lm_words", CMDBOOLTYPE|CMDMSG, &add_lm_words, "enable/disable addition of the unigram/bigrmam successors into the alternatives (default is false)",
                "add_sentence_words", CMDBOOLTYPE|CMDMSG, &add_sentence_words, "enable/disable addition of the words of the current sentence into the alternatives (default is false)",
                "add_full_dictionary", CMDBOOLTYPE|CMDMSG, &add_full_dictionary, "enable/disable addition of all words of the LM dictionary into the alternatives (default is false)",
								"successor_limit", CMDINTTYPE|CMDMSG, &successor_limit, "threshold to decide whether adding the unigram/bigram successors into the alternatives (default is 100)",
								
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
	
	if (lmfile!=NULL) VERBOSE(1, "lmfile: " << lmfile << std::endl);
  if (testfile!=NULL) VERBOSE(1, "testfile: " << testfile << std::endl);
	if (lexiconfile != NULL){
		VERBOSE(1, "lexicon: " << lexiconfile << std::endl);
	}
  VERBOSE(1, "contextbasedscore: " << contextbasedscore << std::endl);
  VERBOSE(1, "topicscore: " << topicscore << std::endl);
  VERBOSE(1, "rankscore: " << rankscore << std::endl);
	
	VERBOSE(1,"add_lexicon_words: " << add_lexicon_words << std::endl);
	VERBOSE(1,"add_lm_words: " << add_lm_words << " successor_limit:" << successor_limit<< std::endl);
	VERBOSE(1,"add_sentence_words: " << add_sentence_words << std::endl);
	
  std::cerr << "loading up to the LM level " << requiredMaxlev << " (if any)" << std::endl;
  std::cerr << "dub: " << dub<< std::endl;
	
	
	if (topicscore == true) {
		VERBOSE(0, "NOT SUPPORTED" << std::endl);
		return 0;
	}
	
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
	if (add_lexicon_words){
		if (lexiconfile != NULL) {
			load_lexicon(lexiconfile, lexicon);
		}else{
			VERBOSE(1, "You did not set any lexicon, but you activated parameter \"--add_lexicon_words\". This is formally correct; maybe you want to pass the lexicon through the input; Please check whether your setting is correct." << std::endl);			
		}
	}else{
		VERBOSE(1, "You set a lexicon, but you did not activate parameter \"--add_lexicon_words\". Hence, words in he lexicon are not used as alternatives" << std::endl);
	}
	/*
	if (std::string lexiconfile!= NULL) {
		fstream inp(lexiconfile,ios::in|ios::binary);
		std::string w1, w2;
		while (inp >> w1 >> w2){
			lexicon.insert(make_pair(w1,w2));
		}
		add_lexicon_words=true;
	}
	*/
	
	if (topicscore == true) {
		if (lmt->getLanguageModelType() != _IRSTLM_LMCONTEXTDEPENDENT) {
			exit_error(IRSTLM_ERROR_DATA, "This type of score is not available for the LM loaded");
		}
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
			std::string tmp_sentence;
			std::string sentence;
			std::string context;
			std::string sentence_lexiconfile;
			
			//remove lexicon string from the input, even if it is not used at all for this type of score
			((lmContextDependent*) lmt)->GetSentenceAndLexicon(tmp_sentence,sentence_lexiconfile,line_str);
			bool withContext = lmt->GetSentenceAndContext(sentence,context,tmp_sentence);
			
			//getting apriori topic weights
			topic_map_t apriori_topic_map;
			if (withContext){
				lmt->setContextMap(apriori_topic_map,context);
			}
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
					size=1;
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
						VERBOSE(2,"word-based topic-distribution:");
						((lmContextDependent*) lmt)->getContextSimilarity()->print_topic_scores(tmp_topic_map,apriori_topic_map,1);
					}
					tmp_topic_map.clear();
				}
			}
			IFVERBOSE(2){
				VERBOSE(2,"sentence-based topic-distribution:");
				((lmContextDependent*) lmt)->getContextSimilarity()->print_topic_scores(sentence_topic_map,apriori_topic_map,last);
			}
			std::cout << sentence << ((lmContextDependent*) lmt)->getContextDelimiter();
			((lmContextDependent*) lmt)->getContextSimilarity()->print_topic_scores(sentence_topic_map);	
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
		double logPr=0,PP=0,PPwp=0,current_Pr;
		double norm_logPr=0,norm_PP=0,norm_PPwp=0,norm_Pr;
		double model_logPr=0,model_PP=0,model_PPwp=0,model_Pr;
		double model_norm_logPr=0,model_norm_PP=0,model_norm_PPwp=0,model_norm_Pr;
		int current_dict_alternatives = 0;
		
		double bow;
		int bol=0;
		char *msp;
		ngram_state_t msidx;
		unsigned int statesize;
		
		// variables for storing sentence-based Perplexity
		int sent_Nw=0,sent_Noov=0;
		double sent_logPr=0,sent_PP=0,sent_PPwp=0;	
		double sent_norm_logPr=0,sent_norm_PP=0,sent_norm_PPwp=0;		
		double sent_model_logPr=0,sent_model_PP=0,sent_model_PPwp=0;		
		double sent_model_norm_logPr=0,sent_model_norm_PP=0,sent_model_norm_PPwp=0;		
		int sent_current_dict_alternatives = 0;
		
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
			std::string tmp_sentence;
			std::string sentence;
			std::string context;
			std::string sentence_lexiconfile;
			
			bool withLexicon = ((lmContextDependent*) lmt)->GetSentenceAndLexicon(tmp_sentence,sentence_lexiconfile,line_str);
			bool withContext = lmt->GetSentenceAndContext(sentence,context,tmp_sentence);
			
			//getting apriori topic weights
			topic_map_t apriori_topic_map;
			if (withContext){
				((lmContextDependent*) lmt)->setContextMap(apriori_topic_map,context);
			}
			// computation using std::string
			// loop over ngrams of the sentence
			string_vec_t word_vec;
			split(sentence, ' ', word_vec);
			
			//first points to the last recent term to take into account
			//last points to the position after the most recent term to take into account
			//last could point outside the vector of string; do NOT use word_vec.at(last)
			size_t last, first;
			size_t order = lmt->maxlevel();
			
			//start the computation from the second word because the first is the BoS symbol,but including BoS in the ngrams
			size_t size=0;
			for (size_t i=0; i< word_vec.size(); ++i){
				++size;
				size=(size<order)?size:order;
				last=i+1;
				
				// reset ngram at begin of sentence
				if (word_vec.at(i) == lmt->getDict()->BoS()) {
					size=1;
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
						
					if (withContext){
						current_Pr = lmt->clprob(tmp_word_vec, apriori_topic_map, &bow, &bol, &msidx, &msp, &statesize);
					}else{
						current_Pr = lmt->clprob(tmp_word_vec, &bow, &bol, &msidx, &msp, &statesize);
					}
					/*
					 double tot_pr = 0.0;
					 if (context_model_normalization){
					 tot_pr = ((lmContextDependent*) lmt)->total_clprob(tmp_word_vec, apriori_topic_map);
					 }
					 */
					
					//					string_vec_t::iterator it=tmp_word_vec.end()-1;
					int current_pos = tmp_word_vec.size()-1;
					std::string current_word = tmp_word_vec.at(current_pos);
					
					//loop over a set of selected alternative words
					//populate the dictionary with all words associated with the current word
					
					dictionary* current_dict;
					if (add_full_dictionary){
						//loop over all words in the LM
						current_dict = lmt->getDict();
					}else{
						current_dict = new dictionary((char *)NULL,1000000);
					}
					current_dict->incflag(1);
					
					current_dict->encode(current_word.c_str());
					
					VERBOSE(2,"after current word current_dict->size:" << current_dict->size() << std::endl);
					
					//add words from the lexicon
					if (add_lexicon_words){
						
						if (withLexicon){
							lexicon.clear();
							load_lexicon(sentence_lexiconfile.c_str(), lexicon);
						}
												
						std::pair <std::multimap< std::string, std::string>::iterator, std::multimap< std::string, std::string>::iterator> ret = lexicon.equal_range(current_word);
						for (std::multimap<std::string, std::string>::const_iterator it=ret.first; it!=ret.second; ++it)
						{
							current_dict->encode((it->second).c_str());
							/*
							 //exclude the current word from the selected alternative words
							 if (current_word != (it->second).c_str()){
							 current_dict->encode((it->second).c_str());
							 }
							 */
						}
					}
					VERBOSE(2,"after add_lexicon_words current_dict->size:" << current_dict->size() << std::endl);
					
					if (add_lm_words){
						bool succ_flag=false;
						ngram hg(lmt->getDict());
						
						if (size==1) {
							hg.pushw(lmt->getDict()->BoS());
							hg.pushc(0);
							
							lmt->get(hg,hg.size,hg.size-1);
							VERBOSE(1,"add_lm_words hg:|" << hg << "| hg.size:" << hg.size << " hg.succ:" << hg.succ << std::endl);
							if (hg.succ < successor_limit){
								succ_flag=true;
							}else{
								VERBOSE(3,"successors are not added into the alternatives because they are too many" << std::endl);	
							}
						}else if (size>=2) {
							hg.pushw(tmp_word_vec.at(tmp_word_vec.size()-2));
							hg.pushc(0);
							
							lmt->get(hg,hg.size,hg.size-1);
							VERBOSE(1,"add_lm_words hg:|" << hg << "| hg.size:" << hg.size << " hg.succ:" << hg.succ << std::endl);
							if (hg.succ < successor_limit){
								succ_flag=true;
							}else{
								VERBOSE(3,"successors are not added into the alternatives because they are too many" << std::endl);	
							}
							
							if (!succ_flag && size>=3){
								hg.size=0;
								hg.pushw(tmp_word_vec.at(tmp_word_vec.size()-3));
								hg.pushw(tmp_word_vec.at(tmp_word_vec.size()-2));
								hg.pushc(0);
								
								lmt->get(hg,hg.size,hg.size-1);
								VERBOSE(1,"add_lm_words hg:|" << hg << "| hg.size:" << hg.size << " hg.succ:" << hg.succ << std::endl);
								
								if (hg.succ < successor_limit){
									succ_flag=true;
								}else{
									VERBOSE(3,"successors are not added into the alternatives because they are too many" << std::endl);	
								}
							}
						}
						
						
						if (succ_flag){
							ngram ng=hg;
							lmt->succscan(hg,ng,LMT_INIT,ng.size);	
							while(lmt->succscan(hg,ng,LMT_CONT,ng.size)) {
								current_dict->encode(ng.dict->decode(*ng.wordp(1)));
							}
						}
						
					}
					VERBOSE(2,"after add_lm_words current_dict->size:" << current_dict->size() << std::endl);
					
					if (add_sentence_words){
						for (string_vec_t::const_iterator it=word_vec.begin(); it!=word_vec.end(); ++it)
						{
							current_dict->encode(it->c_str());
						}
					}
					current_dict->incflag(0);
					VERBOSE(2,"after add_sentence_words current_dict->size:" << current_dict->size() << std::endl);
					
					sent_current_dict_alternatives += current_dict->size();
					current_dict_alternatives += current_dict->size();
					
					VERBOSE(2,"current_dict->size:" << current_dict->size() << std::endl);
					for (int h=0;h<current_dict->size();++h){
						VERBOSE(3,"h:" << h << " w:|" << current_dict->decode(h) << "|" << std::endl);
					}
					
					//the first word in current_dict is always the current_word; hence we can skip it during the scan 
					//variables for the computation of the oracle probability, i.e. the maximum prob
					//double best_pr = -1000000.0;
					//int best_code = lmt->getlogOOVpenalty();
					double best_pr = current_Pr;
					int best_code = 0;
					//variables for the computation of the mass probability related to the current word, i.e. the sum of the probs for all words associated with the current word
					double current_tot_pr = pow(10.0,current_Pr);
					//					for (int j=0; j<current_dict->size(); ++j){
					for (int j=1; j<current_dict->size(); ++j)
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
						
						double pr;
						if (withContext){
							pr=lmt->clprob(tmp_word_vec, apriori_topic_map, &bow, &bol, &msidx, &msp, &statesize, NULL, NULL);
						}else{
							pr=lmt->clprob(tmp_word_vec, &bow, &bol, &msidx, &msp, &statesize, NULL, NULL);
						}
						current_tot_pr += pow(10.0,pr);
						if (best_pr < pr){
							best_pr = pr;
							best_code = j;
							VERBOSE(3,"current_best:" << best_code << " current_word:|" << current_dict->decode(best_code) << "| best_prob:" << pow(10.0,best_pr) << " norm_best_prob:" << pow(10.0,best_pr - current_tot_pr) << std::endl);
						}
						VERBOSE(3,"current_Pr:" << current_Pr << " current_word:" << current_word << "| ===> code:" << j << " word:|" << tmp_word_vec.at(current_pos) << "| pr:" << pr << " versus best_code:" << best_code << " best_word:|" << current_dict->decode(best_code) << "| best_pr:" << best_pr << std::endl);
					}
					
					current_tot_pr=log10(current_tot_pr);
					
					model_Pr = best_pr;
					VERBOSE(2,"model_best_code:" << best_code << " model_best_word:|" << current_dict->decode(best_code) << "| model_best_prob:" << pow(10.0,best_pr) << " current_tot_pr:" << current_tot_pr << std::endl);
					
					norm_oovpenalty = oovpenalty;
					VERBOSE(2,"current_tot_pr:" << current_tot_pr << " oovpenalty:" << oovpenalty << " norm_oovpenalty:" << norm_oovpenalty << std::endl);	
					
					norm_Pr = current_Pr - current_tot_pr;
					model_norm_Pr = model_Pr - current_tot_pr;
					VERBOSE(1,"current_Pr:" << current_Pr << " norm_Pr:" << norm_Pr << " model_Pr:" << model_Pr << " model_norm_Pr:" << model_norm_Pr << " current_code:" << lmt->getDict()->encode(word_vec.at(i).c_str()) << " current_word:|" << word_vec.at(i) << "| model_best_code:" << best_code << " model_best_word:|" << current_dict->decode(best_code) << "|" << std::endl);
					
					model_norm_logPr+=model_norm_Pr;
					sent_model_norm_logPr+=model_norm_Pr;
					norm_logPr+=norm_Pr;
					sent_norm_logPr+=norm_Pr;
					VERBOSE(2,"sent_model_norm_logPr:" << sent_model_norm_logPr << " model_norm_logPr:" << model_norm_logPr << std::endl);	
					VERBOSE(2,"sent_norm_logPr:" << sent_norm_logPr << " norm_logPr:" << norm_logPr << std::endl);	
					
					model_logPr+=model_Pr;
					sent_model_logPr+=model_Pr;
					logPr+=current_Pr;
					sent_logPr+=current_Pr;
					VERBOSE(2,"sent_model_logPr:" << sent_model_logPr << " model_logPr:" << model_logPr << std::endl);	
					VERBOSE(2,"sent_logPr:" << sent_logPr << " current_Pr:" << current_Pr << std::endl);
					delete current_dict;
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
				<< " sent_OOVrate=" << (float)sent_Noov/sent_Nw * 100.0 << "%" 
				<< " sent_avg_alternatives=" << (float) sent_current_dict_alternatives/sent_Nw
				<< std::endl;
				
				std::cout << "%% sent_Nw=" << sent_Nw
				<< " sent_model_norm_logPr=" << sent_model_norm_logPr
				<< " sent_model_norm_PP=" << sent_model_norm_PP
				<< " sent_model_norm_PPwp=" << sent_model_norm_PPwp
				<< " sent_model_norm_PP_noOOV=" << (sent_model_norm_PP-sent_model_norm_PPwp)
				<< " sent_Noov=" << sent_Noov
				<< " sent_OOVrate=" << (float)sent_Noov/sent_Nw * 100.0 << "%" 
				<< " sent_avg_alternatives=" << (float) sent_current_dict_alternatives/sent_Nw
				<< std::endl;
				
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
				<< " sent_OOVrate=" << (float)sent_Noov/sent_Nw * 100.0 << "%" 
				<< " sent_avg_alternatives=" << (float) sent_current_dict_alternatives/sent_Nw
				<< std::endl;
			
				std::cout << "%% sent_Nw=" << sent_Nw
				<< " sent_model_logPr=" << sent_model_logPr
				<< " sent_model_PP=" << sent_model_PP
				<< " sent_model_PPwp=" << sent_model_PPwp
				<< " sent_model_PP_noOOV=" << (sent_model_PP-sent_model_PPwp)
				<< " sent_Noov=" << sent_Noov
				<< " sent_OOVrate=" << (float)sent_Noov/sent_Nw * 100.0 << "%" 
				<< " sent_avg_alternatives=" << (float) sent_current_dict_alternatives/sent_Nw
				<< std::endl;
				std::cout.flush();
				//reset statistics for sentence based Perplexity
				sent_Noov = 0;
				sent_Nw = 0;
				sent_model_norm_logPr = 0.0;
				sent_model_logPr = 0.0;
				sent_norm_logPr = 0.0;
				sent_logPr = 0.0;
				sent_current_dict_alternatives = 0;
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
		<< " OOVrate=" << (float)Noov/Nw * 100.0 << "%" 
		<< " avg_alternatives=" << (float) current_dict_alternatives/Nw
		<< std::endl;
		std::cout.flush();
		
		std::cout << "%% Nw=" << Nw
		<< " model_norm_logPr=" << model_norm_logPr
		<< " model_norm_PP=" << model_norm_PP
		<< " model_norm_PPwp=" << model_norm_PPwp
		<< " model_norm_PP_noOOV=" << (model_norm_PP-model_norm_PPwp)
		<< " Noov=" << Noov
		<< " OOVrate=" << (float)Noov/Nw * 100.0 << "%"
		<< " avg_alternatives=" << (float) current_dict_alternatives/Nw
		<< std::endl;
		std::cout.flush();
		
		std::cout << "%% Nw=" << Nw
		<< " logPr=" << logPr
		<< " PP=" << PP
		<< " PPwp=" << PPwp
		<< " PP_noOOV=" << (PP-PPwp)
		<< " Noov=" << Noov
		<< " OOVrate=" << (float)Noov/Nw * 100.0 << "%"
		<< " avg_alternatives=" << (float) current_dict_alternatives/Nw
		<< std::endl;
		std::cout.flush();
		
		std::cout << "%% Nw=" << Nw
		<< " norm_logPr=" << norm_logPr
		<< " norm_PP=" << norm_PP
		<< " norm_PPwp=" << norm_PPwp
		<< " norm_PP_noOOV=" << (norm_PP-norm_PPwp)
		<< " Noov=" << Noov
		<< " OOVrate=" << (float)Noov/Nw * 100.0 << "%"
		<< " avg_alternatives=" << (float) current_dict_alternatives/Nw
		<< std::endl;
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
		int current_dict_alternatives = 0;
		
		double bow;
		int bol=0;
		char *msp;
		ngram_state_t msidx;
		unsigned int statesize;
		
		// variables for storing sentence-based Rank Statistics
		int sent_Nw=0,sent_Noov=0;
		double sent_avgRank;
		int sent_tot_rank = 0;
		int sent_id = 0;	
		int sent_current_dict_alternatives = 0;
		
		std::fstream inptxt(testfile,std::ios::in);
		
		// loop over input lines
		char line[MAX_LINE];
		while (inptxt.getline(line,MAX_LINE)) {
			
			std::string line_str = line;
			
			VERBOSE(1,"input_line:|" << line_str << "|" << std::endl);	
			
			//getting sentence string;
			std::string tmp_sentence;
			std::string sentence;
			std::string context;
			std::string sentence_lexiconfile;
			
			bool withLexicon = ((lmContextDependent*) lmt)->GetSentenceAndLexicon(tmp_sentence,sentence_lexiconfile,line_str);
			bool withContext = lmt->GetSentenceAndContext(sentence,context,tmp_sentence);
			
			//getting apriori topic weights
			topic_map_t apriori_topic_map;
			if (withContext){
				((lmContextDependent*) lmt)->setContextMap(apriori_topic_map,context);
			}
			
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
					size=1;
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
					
					double current_Pr;
					if (withContext){
						current_Pr=lmt->clprob(tmp_word_vec, apriori_topic_map, &bow, &bol, &msidx, &msp, &statesize);				
					}else{
						current_Pr=lmt->clprob(tmp_word_vec, &bow, &bol, &msidx, &msp, &statesize);				
					}
					
					//loop over a set of selected alternative words
					//populate the dictionary with all words associated with the current word
					
					dictionary* current_dict;
					if (add_full_dictionary){
						//loop over all words in the LM
						current_dict = lmt->getDict();
					}else{
						current_dict = new dictionary((char *)NULL,1000000);
					}
					current_dict->incflag(1);
					
					current_dict->encode(current_word.c_str());
					
					VERBOSE(2,"after current word current_dict->size:" << current_dict->size() << std::endl);
					
					//add words from the lexicon
					if (add_lexicon_words){
						
						if (withLexicon){
							lexicon.clear();
							load_lexicon(sentence_lexiconfile.c_str(), lexicon);
						}
						
						std::pair <std::multimap< std::string, std::string>::iterator, std::multimap< std::string, std::string>::iterator> ret = lexicon.equal_range(current_word);
						for (std::multimap<std::string, std::string>::const_iterator it=ret.first; it!=ret.second; ++it)
						{
							current_dict->encode((it->second).c_str());
							/*
							 //exclude the current word from the selected alternative words
							 if (current_word != (it->second).c_str()){
							 current_dict->encode((it->second).c_str());
							 }
							 */
						}
					}
					VERBOSE(2,"after add_lexicon_words current_dict->size:" << current_dict->size() << std::endl);
					
					if (add_lm_words){
						bool succ_flag=false;
						ngram hg(lmt->getDict());
						
						if (size==1) {
							hg.pushw(lmt->getDict()->BoS());
							hg.pushc(0);
							
							lmt->get(hg,hg.size,hg.size-1);
							VERBOSE(1,"add_lm_words hg:|" << hg << "| hg.size:" << hg.size << " hg.succ:" << hg.succ << std::endl);
							if (hg.succ < successor_limit){
								succ_flag=true;
							}else{
								VERBOSE(3,"successors are not added into the alternatives because they are too many" << std::endl);	
							}
						}else if (size>=2) {
							hg.pushw(tmp_word_vec.at(tmp_word_vec.size()-2));
							hg.pushc(0);
							
							lmt->get(hg,hg.size,hg.size-1);
							VERBOSE(1,"add_lm_words hg:|" << hg << "| hg.size:" << hg.size << " hg.succ:" << hg.succ << std::endl);
							if (hg.succ < successor_limit){
								succ_flag=true;
							}else{
								VERBOSE(3,"successors are not added into the alternatives because they are too many" << std::endl);	
							}
							
							if (!succ_flag && size>=3){
								hg.size=0;
								hg.pushw(tmp_word_vec.at(tmp_word_vec.size()-3));
								hg.pushw(tmp_word_vec.at(tmp_word_vec.size()-2));
								hg.pushc(0);
								
								lmt->get(hg,hg.size,hg.size-1);
								VERBOSE(1,"add_lm_words hg:|" << hg << "| hg.size:" << hg.size << " hg.succ:" << hg.succ << std::endl);
								
								if (hg.succ < successor_limit){
									succ_flag=true;
								}else{
									VERBOSE(3,"successors are not added into the alternatives because they are too many" << std::endl);	
								}
							}
						}
						
						
						if (succ_flag){
							ngram ng=hg;
							lmt->succscan(hg,ng,LMT_INIT,ng.size);	
							while(lmt->succscan(hg,ng,LMT_CONT,ng.size)) {
								current_dict->encode(ng.dict->decode(*ng.wordp(1)));
							}
						}
						
					}
					
					VERBOSE(2,"after add_lm_words current_dict->size:" << current_dict->size() << std::endl);
					
					if (add_sentence_words){
						for (string_vec_t::const_iterator it=word_vec.begin(); it!=word_vec.end(); ++it)
						{
							current_dict->encode(it->c_str());
						}
					}
					current_dict->incflag(0);
					VERBOSE(2,"after add_sentence_words current_dict->size:" << current_dict->size() << std::endl);
					
					sent_current_dict_alternatives += current_dict->size();
					current_dict_alternatives += current_dict->size();
					
					VERBOSE(2,"current_dict->size:" << current_dict->size() << std::endl);
					for (int h=0;h<current_dict->size();++h){
						VERBOSE(2,"h:" << h << " w:|" << current_dict->decode(h) << "|" << std::endl);
					}

				  //the first word in current_dict is always the current_word; hence we can skip it during the scan 
					//variables for the computation of the ranking
					max_rank = current_dict->size(); //the current word is  included in the selected alternative words
					int current_rank = 1;
					//					for (int j=0; j<current_dict->size(); ++j){
					for (int j=1; j<current_dict->size(); j++)
					{
					  tmp_word_vec.at(current_pos) = current_dict->decode(j);
						IFVERBOSE(3){
							std::cout << "tmp_word_vec j:" << j;
							for (string_vec_t::const_iterator it2=tmp_word_vec.begin(); it2!=tmp_word_vec.end(); ++it2) {
								std::cout << " |" << (*it2) << "|";
							}
							std::cout << std::endl;
						}
						
						double pr;
						if (withContext){
							pr=lmt->clprob(tmp_word_vec, apriori_topic_map, &bow, &bol, &msidx, &msp, &statesize);
						}else{
							pr=lmt->clprob(tmp_word_vec, &bow, &bol, &msidx, &msp, &statesize);
						}
						if (pr > current_Pr){
							++current_rank;	
						}
						
						VERBOSE(3," current_pos:" << current_pos << " word:|" << tmp_word_vec.at(current_pos) << "| current_Pr:" << current_Pr << " pr:" << pr << " current_rank:" << current_rank <<std::endl);
					}
					
					sent_tot_rank += current_rank;
					tot_rank += current_rank;
					
					if (debug>1){
						//output format:
						//word_pos:current_rank:max_rank
						rank_outstr << " " << word_pos << ":" << current_rank << ":" << max_rank;
					}
					delete current_dict;
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
				<< " sent_OOVrate=" << (float)sent_Noov/sent_Nw * 100.0 << "%"
				<< " sent_avg_alternatives=" << (float) sent_current_dict_alternatives/sent_Nw
				<< std::endl;
				std::cout.flush();
				
				//reset statistics for sentence based avg Ranking
				sent_Nw = 0;
				sent_Noov = 0;
				sent_tot_rank = 0;
				++sent_id;
				sent_current_dict_alternatives = 0;
			}
		}
		
		avgRank = ((double) tot_rank) / Nw;
		
		std::cout << "%% Nw=" << Nw
		<< " avgRank=" << avgRank
		<< " Noov=" << Noov
		<< " OOVrate=" << (float)Noov/Nw * 100.0 << "%"
		<< " avg_alternatives=" << (float) current_dict_alternatives/Nw
		<< std::endl;
		std::cout.flush();
		
		if (debug>1) lmt->used_caches();
		
		if (debug>1) lmt->stat();
		
		delete lmt;
		return 0;
	}
}

