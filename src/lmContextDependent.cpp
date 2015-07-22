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
		m_topicmodel=NULL;
		
		order=0;
		memmap=0;
		isInverted=false;
		
	}
	
	lmContextDependent::~lmContextDependent()
	{
		if (m_lm) delete m_lm;
		if (m_topicmodel) delete m_topicmodel;
	}
	
	void lmContextDependent::load(const std::string &filename,int mmap)
	{
		VERBOSE(2,"lmContextDependent::load(const std::string &filename,int memmap)" << std::endl);
		VERBOSE(2," filename:|" << filename << "|" << std::endl);
		
		dictionary_upperbound=1000000;
		int memmap=mmap;
		
		//get info from the configuration file
		fstream inp(filename.c_str(),ios::in|ios::binary);
		
		char line[MAX_LINE];
		const char* words[LMCONFIGURE_MAX_TOKEN];
		int tokenN;
		inp.getline(line,MAX_LINE,'\n');
		tokenN = parseWords(line,words,LMCONFIGURE_MAX_TOKEN);
		
		if (tokenN != 1 || ((strcmp(words[0],"LMCONTEXTDEPENDENT") != 0) && (strcmp(words[0],"lmcontextdependent")!=0)))
			error((char*)"ERROR: wrong header format of configuration file\ncorrect format: LMCONTEXTDEPENDENT\nweight_of_ngram_LM filename_of_LM\nweight_of_topic_model filename_of_topic_model");
		
		//reading ngram-based LM
		inp.getline(line,BUFSIZ,'\n');
		tokenN = parseWords(line,words,2);
		if(tokenN < 2 || tokenN > 2) {
			error((char*)"ERROR: wrong header format of configuration file\ncorrect format: LMCONTEXTDEPENDENT\nweight_of_ngram_LM filename_of_LM\nweight_of_topic_model filename_of_topic_model");
		}
		
		//loading ngram-based LM and initialization
		m_lm_weight = (float) atof(words[0]);
		
		//checking the language model type
		m_lm=lmContainer::CreateLanguageModel(words[1],ngramcache_load_factor,dictionary_load_factor);
		
		m_lm->setMaxLoadedLevel(requiredMaxlev);
		
		m_lm->load(words[1], memmap);
		maxlev=m_lm->maxlevel();
		dict=m_lm->getDict();
		getDict()->genoovcode();
		
		m_lm->init_caches(m_lm->maxlevel());		
		
		//reading topic model
		inp.getline(line,BUFSIZ,'\n');
		tokenN = parseWords(line,words,2);
		
		if(tokenN < 2 || tokenN > 2) {
			error((char*)"ERROR: wrong header format of configuration file\ncorrect format: LMCONTEXTDEPENDENT\nweight_of_ngram_LM filename_of_LM\nweight_of_topic_model filename_of_topic_model");
		}
		
		//loading topic model and initialization
		m_topicmodel_weight = (float) atof(words[0]);
		m_topicmodel = new  PseudoTopicModel();
		m_topicmodel->load(words[1]);
		
		inp.close();
	}
	
	double lmContextDependent::lprob(ngram ng, topic_map_t& topic_weights, double* bow,int* bol,char** maxsuffptr,unsigned int* statesize,bool* extendible)
	{
		std::vector<std::string> text;   // replace with the text passed as parameter
		double lm_prob = m_lm->clprob(ng, bow, bol, maxsuffptr, statesize, extendible);
		double topic_prob = m_topicmodel->prob(text, topic_weights);
		double ret_prob = m_lm_weight * lm_prob + m_topicmodel_weight * topic_prob;
		VERBOSE(0, "lm_prob:" << lm_prob << " m_lm_weight:" << m_lm_weight << " topic_prob:" << topic_prob << " m_topicmodel_weight:" << m_topicmodel_weight << " ret_prob:" << ret_prob << std::endl);
		
		return ret_prob;
	}
	
	double lmContextDependent::lprob(std::vector<std::string>& text, topic_map_t& topic_weights, double* bow,int* bol,char** maxsuffptr,unsigned int* statesize,bool* extendible)
	{
		VERBOSE(0,"lmContextDependent::lprob(int* codes, int sz, topic_map_t& topic_weights, " << std::endl);
		//create the actual ngram
		ngram ng(dict);
		ng.pushw(text);
		MY_ASSERT (ng.size == text.size());
		
		double lm_prob = m_lm->clprob(ng, bow, bol, maxsuffptr, statesize, extendible);
		double topic_prob = m_topicmodel->prob(text, topic_weights);
		
		double ret_prob = m_lm_weight * lm_prob + m_topicmodel_weight * topic_prob;
		VERBOSE(0, "lm_prob:" << lm_prob << " m_lm_weight:" << m_lm_weight << " topic_prob:" << topic_prob << " m_topicmodel_weight:" << m_topicmodel_weight << " ret_prob:" << ret_prob << std::endl);
		
		return ret_prob;	
	}
	
	double lmContextDependent::lprob(int* codes, int sz, topic_map_t& topic_weights, double* bow,int* bol,char** maxsuffptr,unsigned int* statesize,bool* extendible)
	{
		VERBOSE(0,"lmContextDependent::lprob(int* codes, int sz, topic_map_t& topic_weights, " << std::endl);
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
