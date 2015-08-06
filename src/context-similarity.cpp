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
	ContextSimilarity::ContextSimilarity(const std::string &dictfile, const std::string &num_modelfile, const std::string &den_modelfile)
	{
		m_num_lm=lmContainer::CreateLanguageModel(num_modelfile);
		m_den_lm=lmContainer::CreateLanguageModel(num_modelfile);
		
		m_num_lm->load(num_modelfile);
		m_den_lm->load(den_modelfile);
		
		m_num_lm->getDict()->genoovcode();
		m_den_lm->getDict()->genoovcode();
		
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
	
	//return the log10 of the similarity score
	double ContextSimilarity::score(string_vec_t& text, topic_map_t& topic_weights)
	{
		VERBOSE(4, "double ContextSimilarity::score(string_vec_t& text, topic_map_t& topic_weights)" << std::endl);
		double ret_log10_pr;
		
		if (topic_weights.size() > 0){
			
			ngram base_num_ng(m_num_lm->getDict());
			ngram base_den_ng(m_den_lm->getDict());
			create_ngram(text, base_num_ng, base_den_ng);
			
			for (topic_map_t::iterator it = topic_weights.begin(); it!= topic_weights.end(); ++it)
			{
				ngram num_ng = base_num_ng;
				ngram den_ng = base_den_ng;
				add_topic(it->first, num_ng, den_ng);
				double apriori_topic_score = log10(it->second);
				double topic_score = get_topic_similarity(num_ng, den_ng); //log10-prob
				
				VERBOSE(3, "topic:|" << it->first  << "apriori_topic_score:" << apriori_topic_score << " topic_score:" << topic_score << std::endl);
				if (it == topic_weights.begin()){
					ret_log10_pr = apriori_topic_score + topic_score;
				}else{
					ret_log10_pr = logsum(ret_log10_pr, apriori_topic_score + topic_score)/M_LN10;
				}
				VERBOSE(4, "CURRENT ret_log10_pr:" << ret_log10_pr << std::endl);
			}
		}else{
			//a-priori topic distribution is "empty", i.e. there is nore score for any topic
			//return a "constant" lower-bound score,  SIMILARITY_LOWER_BOUND = log(0.0)
			ret_log10_pr = SIMILARITY_LOWER_BOUND;
		}
		
		VERBOSE(3, "ret_log10_pr:" << ret_log10_pr << std::endl);
		return ret_log10_pr;
	}
	
	//returns the scores for all topics in the topic models (without apriori topic prob)
	void ContextSimilarity::get_topic_scores(topic_map_t& topic_map, string_vec_t& text)
	{		
		ngram base_num_ng(m_num_lm->getDict());
		ngram base_den_ng(m_den_lm->getDict());
		create_ngram(text, base_num_ng, base_den_ng);
		
		for (topic_dict_t::iterator it=m_lm_topic_dict.begin(); it != m_lm_topic_dict.end(); ++it)
		{
			ngram num_ng = base_num_ng;
			ngram den_ng = base_den_ng;
			add_topic(*it, num_ng, den_ng);
			topic_map[*it] = get_topic_similarity(num_ng, den_ng);
		}
	}
	
	
	void ContextSimilarity::add_topic_scores(topic_map_t& topic_map, topic_map_t& tmp_map)
	{
		for (topic_map_t::iterator it=tmp_map.begin(); it != tmp_map.end(); ++it){
			topic_map[it->first] += tmp_map[it->first];
		}
	}
	
	//returns the scores for all topics in the topic models (without apriori topic prob)
	void ContextSimilarity::print_topic_scores(topic_map_t& map)
	{
		for (topic_map_t::iterator it=map.begin(); it != map.end(); ++it)
		{
			if (it!=map.begin()) { std::cout << topic_map_delimiter1; }
			std::cout << it->first << topic_map_delimiter2 << it->second;
		}
		std::cout << std::endl;
	}
	
	void ContextSimilarity::setContextMap(topic_map_t& topic_map, const std::string& context){
		
		string_vec_t topic_weight_vec;
		string_vec_t topic_weight;
		
		// context is supposed in this format
		// topic-name1,topic-value1:topic-name2,topic-value2:...:topic-nameN,topic-valueN
		
		//first-level split the context in a vector of 	topic-name1,topic-value1, using the first separator ':'
		split(context, topic_map_delimiter1, topic_weight_vec);
		for (string_vec_t::iterator it=topic_weight_vec.begin(); it!=topic_weight_vec.end(); ++it){
			//first-level split the context in a vector of 	topic-name1 and ,topic-value1, using the second separator ','
			split(*it, topic_map_delimiter2, topic_weight);
			topic_map[topic_weight.at(0)] = strtod (topic_weight.at(1).c_str(), NULL);
			topic_weight.clear();
		}
	}
	
	void ContextSimilarity::create_ngram(const string_vec_t& text, ngram& num_ng, ngram& den_ng)
	{
		//text is a vector of strings with w in the last position and the history in the previous positions
		//text must have at least two words
		VERBOSE(3,"void ContextSimilarity::create_ngram" << std::endl);

		//TO_CHECK: what happens when text has zero element
		//		if (text.size()==0)
		
		//TO_CHECK: what happens when text has just one element
		
		
		
		// lm model for the numerator is assumed to be a 3-gram lm, hence num_gr have only size 3 (two words and one topic); here we insert two words
		if (text.size()==1){
			num_ng.pushw(num_ng.dict->OOV());
		}else {
			num_ng.pushw(text.at(text.size()-2));
		}
		num_ng.pushw(text.at(text.size()-1));
		
		// lm model for the denominator is assumed to be a 2-gram lm, hence den_gr have only size 2 (one word and one topic); here we insert one word
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
		ngram num_ng(m_num_lm->getDict());
		ngram den_ng(m_den_lm->getDict());
		
		create_topic_ngram(text, topic, num_ng, den_ng);
		
		return get_topic_similarity(num_ng, den_ng);
	}
	
	double ContextSimilarity::get_topic_similarity(ngram& num_ng, ngram& den_ng)
	{	
		double num_pr=m_num_lm->clprob(num_ng);
		double den_pr=m_den_lm->clprob(den_ng);
	 VERBOSE(4, "num_ng:|" << num_ng << "| pr:" << num_pr << std::endl);
	 VERBOSE(4, "den_ng:|" << den_ng << "| pr:" << den_pr << std::endl);
		return num_pr - den_pr;
		//		return m_lm->clprob(num_ng) - m_lm->clprob(den_ng);
	}
	
}//namespace irstlm
