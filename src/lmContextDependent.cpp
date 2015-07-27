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
		VERBOSE(2," filename:|" << filename << "|" << std::endl);
		
		dictionary_upperbound=1000000;
		int memmap=mmap;
		
		//get info from the configuration file
		fstream inp(filename.c_str(),ios::in|ios::binary);
		VERBOSE(0, "filename:|" << filename << "|" << std::endl);
		
		char line[MAX_LINE];
		const char* words[LMCONFIGURE_MAX_TOKEN];
		int tokenN;
		inp.getline(line,MAX_LINE,'\n');
		tokenN = parseWords(line,words,LMCONFIGURE_MAX_TOKEN);
		
		if (tokenN != 1 || ((strcmp(words[0],"LMCONTEXTDEPENDENT") != 0) && (strcmp(words[0],"lmcontextdependent")!=0)))
			error((char*)"ERROR: wrong header format of configuration file\ncorrect format: LMCONTEXTDEPENDENT\nfilename_of_LM\nweight topic_dict topic_model");
		
		//reading ngram-based LM
		inp.getline(line,BUFSIZ,'\n');
		tokenN = parseWords(line,words,1);
		if(tokenN < 1 || tokenN > 1) {
			error((char*)"ERROR: wrong header format of configuration file\ncorrect format: LMCONTEXTDEPENDENT\nfilename_of_LM\nweight topic_dict topic_model");
		}
		
		VERBOSE(0, "modelfile:|" << words[0] << "|" << std::endl);
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
		tokenN = parseWords(line,words,3);
		
		if(tokenN < 3 || tokenN > 3) {
			error((char*)"ERROR: wrong header format of configuration file\ncorrect format: LMCONTEXTDEPENDENT\nfilename_of_LM\nweight topic_dict topic_model");
		}
		
		//loading topic model and initialization
		m_similaritymodel_weight = (float) atof(words[0]);
		m_similaritymodel = new ContextSimilarity(words[1], words[2]);
		
		inp.close();
		
		VERBOSE(0, "topicdict:|" << words[1] << "|" << std::endl);
		VERBOSE(0, "topicmodel:|" << words[2] << "|" << std::endl);
	}
	
	double lmContextDependent::lprob(ngram ng, topic_map_t& topic_weights, double* bow,int* bol,char** maxsuffptr,unsigned int* statesize,bool* extendible)
	{
		string_vec_t text;   // replace with the text passed as parameter
		double lm_logprob = m_lm->clprob(ng, bow, bol, maxsuffptr, statesize, extendible);
		double similarity_score = m_similaritymodel->score(text, topic_weights);
		double ret_logprob = lm_logprob + m_similaritymodel_weight * similarity_score;
		VERBOSE(0, "lm_logprob:" << lm_logprob << " similarity_score:" << similarity_score << " m_similaritymodel_weight:" << m_similaritymodel_weight << " ret_logprob:" << ret_logprob << std::endl);
		
		return ret_logprob;
	}
	
	double lmContextDependent::lprob(string_vec_t& text, topic_map_t& topic_weights, double* bow,int* bol,char** maxsuffptr,unsigned int* statesize,bool* extendible)
	{
		VERBOSE(0,"lmContextDependent::lprob(string_vec_t& text, topic_map_t& topic_weights, " << std::endl);
		//create the actual ngram
		ngram ng(dict);
		ng.pushw(text);
		VERBOSE(0,"ng:|" << ng << "|" << std::endl);		
		
		MY_ASSERT (ng.size == (int) text.size());
		double lm_logprob = m_lm->clprob(ng, bow, bol, maxsuffptr, statesize, extendible);
		double similarity_score = m_similaritymodel->score(text, topic_weights);
		double ret_logprob = lm_logprob + m_similaritymodel_weight * similarity_score;
		VERBOSE(0, "lm_logprob:" << lm_logprob << " similarity_score:" << similarity_score << " m_similaritymodel_weight:" << m_similaritymodel_weight << " ret_logprob:" << ret_logprob << std::endl);
		
		return ret_logprob;
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
	
	double lmContextDependent::setlogOOVpenalty(int dub)
	{
		MY_ASSERT(dub > dict->size());
		m_lm->setlogOOVpenalty(dub);  //set OOV Penalty by means of DUB
		double OOVpenalty = m_lm->getlogOOVpenalty();  //get OOV Penalty
		logOOVpenalty=log(OOVpenalty);
		return logOOVpenalty;
	}
}//namespace irstlm
