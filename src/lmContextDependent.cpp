// $Id: lmContextDependent.cpp 3686 2010-10-15 11:55:32Z bertoldi $

/******************************************************************************
 IrstLM: IRST Language Model Toolkit
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
#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include "lmContainer.h"
#include "lmContextDependent.h"
#include "util.h"

using namespace std;

inline void error(const char* message)
{
  std::cerr << message << "\n";
  throw std::runtime_error(message);
}

namespace irstlm {
	
	lmContextDependent::lmContextDependent(float nlf, float dlf)
	{
		ngramcache_load_factor = nlf;
		dictionary_load_factor = dlf;
		m_lm=NULL;
		m_similaritymodel=NULL;
		
		order=0;
		memmap=0;
		isInverted=false;
		
	}
	
	lmContextDependent::~lmContextDependent()
	{
		if (m_lm) delete m_lm;
		if (m_similaritymodel) delete m_similaritymodel;
	}
	
	void lmContextDependent::load(const std::string &filename,int mmap)
	{
		VERBOSE(2,"lmContextDependent::load(const std::string &filename,int memmap)" << std::endl);
		VERBOSE(2,"configuration file:|" << filename << "|" << std::endl);
		
		dictionary_upperbound=1000000;
		int memmap=mmap;
		
		//get info from the configuration file
		fstream inp(filename.c_str(),ios::in|ios::binary);
		VERBOSE(0, "filename:|" << filename << "|" << std::endl);
		
		char line[MAX_LINE];
		const char* words[LMCONTEXTDEPENDENT_CONFIGURE_MAX_TOKEN];
		int tokenN;
		inp.getline(line,MAX_LINE,'\n');
		tokenN = parseWords(line,words,LMCONTEXTDEPENDENT_CONFIGURE_MAX_TOKEN);
		
		if (tokenN != 1 || ((strcmp(words[0],"LMCONTEXTDEPENDENT") != 0) && (strcmp(words[0],"lmcontextdependent")!=0)))
			error((char*)"ERROR: wrong header format of configuration file\ncorrect format: LMCONTEXTDEPENDENT\nfilename_of_LM\nweight k_model hk_model hwk_model pruning_threshold");
		
		//reading ngram-based LM
		inp.getline(line,BUFSIZ,'\n');
		tokenN = parseWords(line,words,1);
		if(tokenN < 1 || tokenN > 1) {
			error((char*)"ERROR: wrong header format of configuration file\ncorrect format: LMCONTEXTDEPENDENT\nfilename_of_LM\nweight k_model hk_model hwk_model pruning_threshold");
		}
		
		VERBOSE(0, "model_w:|" << words[0] << "|" << std::endl);
		//checking the language model type
		m_lm=lmContainer::CreateLanguageModel(words[0],ngramcache_load_factor, dictionary_load_factor);
		
		m_lm->setMaxLoadedLevel(requiredMaxlev);
		
		m_lm->load(words[0], memmap);
		maxlev=m_lm->maxlevel();
		dict=m_lm->getDict();
		getDict()->genoovcode();
		
		m_lm->init_caches(m_lm->maxlevel());
		
		//reading topic model
		inp.getline(line,BUFSIZ,'\n');
		tokenN = parseWords(line,words,LMCONTEXTDEPENDENT_CONFIGURE_MAX_TOKEN);
		
		if(tokenN < 5 || tokenN > LMCONTEXTDEPENDENT_CONFIGURE_MAX_TOKEN) {
			error((char*)"ERROR: wrong header format of configuration file\ncorrect format: LMCONTEXTDEPENDENT\nfilename_of_LM\nweight k_model hk_model hwk_model pruning_threshold");
		}
		
		//loading topic model and initialization
		m_similaritymodel_weight = (float) atof(words[0]);
		std::string _k_ngt = words[1];
		std::string _hk_ngt = words[2];
		std::string _hwk_ngt = words[3];
		int _thr = atoi(words[4]);
		double _smoothing = 0.1;
		if (tokenN == 6){ _smoothing = atof(words[5]); }
		m_similaritymodel = new ContextSimilarity(_k_ngt, _hk_ngt, _hwk_ngt);
		m_similaritymodel->set_Threshold_on_H(_thr);
		m_similaritymodel->set_SmoothingValue(_smoothing);
		
		inp.close();
		
		VERBOSE(0, "model_k:|" << _k_ngt << "|" << std::endl);
		VERBOSE(0, "model_hk:|" << _hk_ngt << "|" << std::endl);
		VERBOSE(0, "model_hwk:|" << _hwk_ngt << "|" << std::endl);
		VERBOSE(0, "topic_threshold_on_h:|" << m_similaritymodel->get_Threshold_on_H() << "|" << std::endl);
		VERBOSE(0, "shift-beta smoothing on counts:|" << m_similaritymodel->get_SmoothingValue() << "|" << std::endl);
	}

	void lmContextDependent::GetSentenceAndContext(std::string& sentence, std::string& context, std::string& line)
	{
		VERBOSE(2,"lmContextDependent::GetSentenceAndContext" << std::endl);
		VERBOSE(2,"line:|" << line << "|" << std::endl);
		size_t pos = line.find(context_delimiter);	
		if (pos != std::string::npos){ // context_delimiter is found
			sentence = line.substr(0, pos);
			line.erase(0, pos + context_delimiter.length());
			
			//getting context string;
			context = line;
		}else{
			sentence = line;
			context = "";
		}	
		VERBOSE(2,"sentence:|" << sentence << "|" << std::endl);	
		VERBOSE(2,"context:|" << context << "|" << std::endl);	
	}

	double lmContextDependent::lprob(ngram ng, topic_map_t& topic_weights, double* bow,int* bol,char** maxsuffptr,unsigned int* statesize,bool* extendible)
	{
		VERBOSE(2,"lmContextDependent::lprob(ngram ng, topic_map_t& topic_weights, ...)" << std::endl);
		string_vec_t text;
		if (ng.size>1){
			text.push_back(ng.dict->decode(*ng.wordp(2)));
		}
		text.push_back(ng.dict->decode(*ng.wordp(1)));
		
		return lprob(ng, text, topic_weights, bow, bol, maxsuffptr, statesize, extendible);
	}
	
	double lmContextDependent::lprob(int* codes, int sz, topic_map_t& topic_weights, double* bow,int* bol,char** maxsuffptr,unsigned int* statesize,bool* extendible)
	{
		VERBOSE(3,"lmContextDependent::lprob(int* codes, int sz, topic_map_t& topic_weights, " << std::endl);
		//create the actual ngram
		ngram ong(dict);
		ong.pushc(codes,sz);
		MY_ASSERT (ong.size == sz);
		
		return lprob(ong, topic_weights, bow, bol, maxsuffptr, statesize, extendible);	
	}
	
	double lmContextDependent::lprob(string_vec_t& text, topic_map_t& topic_weights, double* bow,int* bol,char** maxsuffptr,unsigned int* statesize,bool* extendible)
	{
		VERBOSE(2,"lmContextDependent::lprob(string_vec_t& text, topic_map_t& topic_weights, ...)" << std::endl);
		
		//create the actual ngram
		ngram ng(dict);
		ng.pushw(text);
		VERBOSE(3,"ng:|" << ng << "|" << std::endl);		
		
		MY_ASSERT (ng.size == (int) text.size());
		return lprob(ng, text, topic_weights, bow, bol, maxsuffptr, statesize, extendible);
	}
	
	
	double lmContextDependent::lprob(ngram ng, lm_map_t& lm_weights, topic_map_t& topic_weights, double* bow,int* bol,char** maxsuffptr,unsigned int* statesize,bool* extendible)
	{
		VERBOSE(2,"lmContextDependent::lprob(ngram ng, lm_map_t& lm_weights, topic_map_t& topic_weights, ...)" << std::endl);
		string_vec_t text;
		if (ng.size>1){
			text.push_back(ng.dict->decode(*ng.wordp(2)));
		}
		text.push_back(ng.dict->decode(*ng.wordp(1)));
		
		return lprob(ng, text, lm_weights, topic_weights, bow, bol, maxsuffptr, statesize, extendible);
	}
	
	double lmContextDependent::lprob(int* codes, int sz, lm_map_t& lm_weights, topic_map_t& topic_weights, double* bow,int* bol,char** maxsuffptr,unsigned int* statesize,bool* extendible)
	{
		VERBOSE(3,"lmContextDependent::lprob(int* codes, int sz, lm_map_t& lm_weights, topic_map_t& topic_weights, " << std::endl);
		//create the actual ngram
		ngram ong(dict);
		ong.pushc(codes,sz);
		MY_ASSERT (ong.size == sz);
		
		return lprob(ong, lm_weights, topic_weights, bow, bol, maxsuffptr, statesize, extendible);	
	}
	
	double lmContextDependent::lprob(string_vec_t& text, lm_map_t& lm_weights, topic_map_t& topic_weights, double* bow,int* bol,char** maxsuffptr,unsigned int* statesize,bool* extendible)
	{
		VERBOSE(2,"lmContextDependent::lprob(string_vec_t& text, lm_map_t& lm_weights, topic_map_t& topic_weights, ...)" << std::endl);
		
		//create the actual ngram
		ngram ng(dict);
		ng.pushw(text);
		VERBOSE(3,"ng:|" << ng << "|" << std::endl);		
		
		MY_ASSERT (ng.size == (int) text.size());
		return lprob(ng, text, lm_weights, topic_weights, bow, bol, maxsuffptr, statesize, extendible);
	}
	
	double lmContextDependent::lprob(ngram& ng, string_vec_t& text, topic_map_t& topic_weights, double* bow,int* bol,char** maxsuffptr,unsigned int* statesize,bool* extendible)
	{
		VERBOSE(2,"lmContextDependent::lprob(ngram& ng, topic_map_t& topic_weights, ...)" << std::endl);
		double lm_logprob = m_lm->clprob(ng, bow, bol, maxsuffptr, statesize, extendible);
		double similarity_score = m_similaritymodel->context_similarity(text, topic_weights);
		double ret_logprob = lm_logprob + m_similaritymodel_weight * similarity_score;
		VERBOSE(2, "lm_log10_pr:" << lm_logprob << " similarity_score:" << similarity_score << " m_similaritymodel_weight:" << m_similaritymodel_weight << " ret_log10_pr:" << ret_logprob << std::endl);
		
		return ret_logprob;
	}
	
	double lmContextDependent::lprob(ngram& ng, string_vec_t& text, lm_map_t& lm_weights, topic_map_t& topic_weights, double* bow,int* bol,char** maxsuffptr,unsigned int* statesize,bool* extendible)
	{
		VERBOSE(2,"lmContextDependent::lprob(ngram& ng, lm_map_t& lm_weights, topic_map_t& topic_weights, ...)" << std::endl);
		double lm_logprob = m_lm->clprob(ng, lm_weights, bow, bol, maxsuffptr, statesize, extendible);
		double similarity_score = m_similaritymodel->context_similarity(text, topic_weights);
		double ret_logprob = lm_logprob + m_similaritymodel_weight * similarity_score;
		VERBOSE(2, "lm_log10_pr:" << lm_logprob << " similarity_score:" << similarity_score << " m_similaritymodel_weight:" << m_similaritymodel_weight << " ret_log10_pr:" << ret_logprob << std::endl);
		
		return ret_logprob;
	}
	
	
	double lmContextDependent::total_clprob(string_vec_t& text, topic_map_t& topic_weights)
	{		
		VERBOSE(2,"lmContextDependent::total_lprob(string_vec_t& text, topic_map_t& topic_weights)" << std::endl);
		double tot_pr = 0.0;
		double v_pr;
		for (int v=0; v<dict->size(); ++v){
			//replace last word, which is in position 2, keeping topic in position 1 unchanged
			text.at(text.size()-1) = dict->decode(v);
			v_pr = clprob(text, topic_weights);
			tot_pr += pow(10.0,v_pr); //v_pr is a lo10 prob
		}
		return log10(tot_pr);
	}
	
	double lmContextDependent::total_clprob(ngram& ng, topic_map_t& topic_weights)
	{		
		VERBOSE(2,"lmContextDependent::total_lprob(ngram& ng, topic_map_t& topic_weights)" << std::endl);
		double tot_pr = 0.0;
		double v_pr;
		double oovpenalty = getlogOOVpenalty();
		setlogOOVpenalty((double) 0);
		for (int v=0; v<dict->size(); ++v){
			//replace last word, which is in position 2, keeping topic in position 1 unchanged
			*ng.wordp(1) = ng.dict->encode(dict->decode(v));
			v_pr = clprob(ng, topic_weights);
			tot_pr += pow(10.0,v_pr); //v_pr is a lo10 prob
		}
		setlogOOVpenalty(oovpenalty);
		return log10(tot_pr);
	}
	
	double lmContextDependent::setlogOOVpenalty(int dub)
	{
		MY_ASSERT(dub > dict->size());
		m_lm->setlogOOVpenalty(dub);  //set OOV Penalty by means of DUB
		logOOVpenalty=log(m_lm->getlogOOVpenalty());
		return logOOVpenalty;
	}
}//namespace irstlm
