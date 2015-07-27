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
#include <sstream>
#include <stdexcept>
#include <string>
#include "lmContainer.h"
#include "context-similarity.h"
#include "util.h"
#include "mfstream.h"

using namespace std;

inline void error(const char* message)
{
  std::cerr << message << "\n";
  throw std::runtime_error(message);
}

namespace irstlm {
	ContextSimilarity::ContextSimilarity(const std::string &dictfile, const std::string &modelfile)
	{
		m_lm=lmContainer::CreateLanguageModel(modelfile);
		
		m_lm->load(modelfile);
		
		m_lm->getDict()->genoovcode();
		
		//loading form file		
		std::string str;
		
		mfstream inp(dictfile.c_str(),ios::in);
		
		if (!inp) {
			std::stringstream ss_msg;
			ss_msg << "cannot open " << dictfile << "\n";
			exit_error(IRSTLM_ERROR_IO, ss_msg.str());
		}
		VERBOSE(0, "Loading the list of topic" << std::endl);
		
		while (inp >> str)
		{
			m_lm_topic_dict.insert(str);
		}
		VERBOSE(0, "There are " << m_lm_topic_dict.size() << " topic" << std::endl);
	}

	
	ContextSimilarity::~ContextSimilarity()
	{}
	
	double ContextSimilarity::score(string_vec_t& text, topic_map_t& topic_weights)
	{
		VERBOSE(4, "double ContextSimilarity::score(string_vec_t& text, topic_map_t& topic_weights)" << std::endl);
		if (topic_weights.size() == 0){
			//a-priori topic distribution is "empty", i.e. there is nore score for any topic
			//return a "constant" lower-bound score,  SIMILARITY_LOWER_BOUND = log(0.0)
			return SIMILARITY_LOWER_BOUND;
		}
		
		ngram base_num_ng(m_lm->getDict());
		ngram base_den_ng(m_lm->getDict());
		create_ngram(text, base_num_ng, base_den_ng);
		
		double ret_logprob = 0.0;
		double add_logprob;
		topic_map_t::iterator it = topic_weights.begin();
		do
		{
			ngram num_ng = base_num_ng;
			ngram den_ng = base_den_ng;
			add_topic(it->first, num_ng, den_ng);
			
			VERBOSE(0, "topic:|" << it->first << " log(p(topic):" << log(it->second) << std::endl);
			double topic_score = get_topic_similarity(num_ng, den_ng);
			add_logprob = log(it->second) + topic_score;
			VERBOSE(0, "topic_score:" << topic_score << std::endl);
			VERBOSE(0, "add_logprob:" << add_logprob << std::endl);
			ret_logprob = logsum(ret_logprob, add_logprob);
			++it;
		}while (it!= topic_weights.end());
		
		
		VERBOSE(0, "ret_logprob:" << ret_logprob << std::endl);
		return ret_logprob;
	}
	
	
	topic_map_t ContextSimilarity::get_topic_scores(string_vec_t& text)
	{
		topic_map_t topic_map;
		
		ngram base_num_ng(m_lm->getDict());
		ngram base_den_ng(m_lm->getDict());
		create_ngram(text, base_num_ng, base_den_ng);
		
		for (topic_dict_t::iterator it=m_lm_topic_dict.begin(); it != m_lm_topic_dict.end(); ++it)
		{
			ngram num_ng = base_num_ng;
			ngram den_ng = base_den_ng;
			add_topic(*it, num_ng, den_ng);
			topic_map[*it] = get_topic_similarity(num_ng, den_ng);
		}
		return topic_map;
	}
	
	void ContextSimilarity::create_ngram(const string_vec_t& text, ngram& num_ng, ngram& den_ng)
	{
		//text is  a vector of string with w in the last position and the history in the previous positions
		//text must have at least two words
		VERBOSE(3,"void ContextSimilarity::create_ngram" << std::endl);

		//TO_CHECK: what happens when text has zero element
		//		if (text.size()==0)
		
		//TO_CHECK: what happens when text has just one element
		if (text.size()==1){
			num_ng.pushw(num_ng.dict->OOV());
		}else {
			num_ng.pushw(text.at(text.size()-2));
		}
		num_ng.pushw(text.at(text.size()-1));
		
		den_ng.pushw(den_ng.dict->OOV());		//or den_ng.pushc(m_lm->getDict()->getoovcode());
		den_ng.pushw(text.at(text.size()-1));
	}
	
	void ContextSimilarity::create_topic_ngram(const string_vec_t& text, const std::string& topic, ngram& num_ng, ngram& den_ng)
	{
		//text is  a vector of string with w in the last position and the history in the previous positions
		//text must have at least two words
		create_ngram(text, num_ng, den_ng);
		add_topic(topic, num_ng, den_ng);
	}
	
	void ContextSimilarity::add_topic(const std::string& topic, ngram& num_ng, ngram& den_ng)
	{		
		num_ng.pushw(topic);
		den_ng.pushw(topic);
	}
	
	double ContextSimilarity::get_topic_similarity(string_vec_t text, const std::string& topic)
	{
		ngram num_ng(m_lm->getDict());
		ngram den_ng(m_lm->getDict());
		
		create_topic_ngram(text, topic, num_ng, den_ng);
		
		return get_topic_similarity(num_ng, den_ng);
	}
	
	double ContextSimilarity::get_topic_similarity(ngram& num_ng, ngram& den_ng)
	{	
		double num_pr=m_lm->clprob(num_ng);
		double den_pr=m_lm->clprob(den_ng);
	 VERBOSE(0, "num_ng:|" << num_ng << "| pr:" << num_pr << std::endl);
	 VERBOSE(0, "den_ng:|" << den_ng << "| pr:" << den_pr << std::endl);
		return m_lm->clprob(num_ng) - m_lm->clprob(den_ng);
	}
	
}//namespace irstlm
