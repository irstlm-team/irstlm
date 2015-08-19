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
#include "ngramtable.h"
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
	ContextSimilarity::ContextSimilarity(const std::string &k_modelfile, const std::string &hk_modelfile, const std::string &hwk_modelfile)
	{
		m_hwk_order=3;
		m_hk_order=2;
		m_k_order=1;
		m_hwk_ngt=new ngramtable((char*) hwk_modelfile.c_str(), m_hwk_order, NULL,NULL,NULL);
		m_hk_ngt=new ngramtable((char*) hk_modelfile.c_str(), m_hk_order, NULL,NULL,NULL);
		m_k_ngt=new ngramtable((char*) k_modelfile.c_str(), m_k_order, NULL,NULL,NULL);
		
		m_smoothing = 0.001;
		m_threshold_on_h = 0;
		m_active=true;
		
		m_topic_size = m_k_ngt->getDict()->size();
		VERBOSE(1, "There are " << m_topic_size << " topics in the model" << std::endl);
		
#ifdef MY_ASSERT_FLAG
		VERBOSE(0, "MY_ASSERT is active" << std::endl);
#else
		VERBOSE(0, "MY_ASSERT is NOT active" << std::endl);
#endif
		
	}

	
	ContextSimilarity::~ContextSimilarity()
	{
		delete m_hwk_ngt;
		delete m_hk_ngt;
	}
	
	//return the log10 of the similarity score
	double ContextSimilarity::get_context_similarity(string_vec_t& text, topic_map_t& topic_weights)
	{
		VERBOSE(2, "double ContextSimilarity::get_context_similarity(string_vec_t& text, topic_map_t& topic_weights)" << std::endl);
		double ret_log10_pr;
		
		if (!m_active){ //similarity score is disable
			ret_log10_pr = 0.0;
		}else if (m_topic_size == 0){
			//a-priori topic distribution is "empty", i.e. there is no score for any topic
			//return an uninforming score (0.0)
			ret_log10_pr = 0.0;
		} else{
			VERBOSE(3, "topic_weights.size():" << topic_weights.size() << std::endl);
			ngram base_num_ng(m_hwk_ngt->getDict());
			ngram base_den_ng(m_hk_ngt->getDict());
			
			
			create_ngram(text, base_num_ng, base_den_ng);
			if (base_reliable(base_den_ng, 2, m_hk_ngt)){ //we do not know about the reliability of the denominator
				
				for (topic_map_t::iterator it = topic_weights.begin(); it!= topic_weights.end(); ++it)
				{
					ngram num_ng = base_num_ng;
					ngram den_ng = base_den_ng;
					add_topic(it->first, num_ng, den_ng);
					
					double apriori_topic_score = log10(it->second); //log10-prob
					double topic_score = get_topic_similarity(num_ng, den_ng); //log10-prob
					
					VERBOSE(3, "topic:|" << it->first  << "| apriori_topic_score:" << apriori_topic_score << " topic_score:" << topic_score << std::endl);
					if (it == topic_weights.begin()){
						ret_log10_pr = apriori_topic_score + topic_score;
					}else{
						ret_log10_pr = logsum(ret_log10_pr, apriori_topic_score + topic_score)/M_LN10;
					}
					VERBOSE(3, "CURRENT ret_log10_pr:" << ret_log10_pr << std::endl);
				}
			}else{
				//the similarity score is not reliable enough, because occurrences of base_den_ng are too little 
				//we also assume that also counts for base_num_ng are unreliable
				//return an uninforming score (0.0)
				ret_log10_pr = 0.0;
			}
		}
		
		VERBOSE(2, "ret_log10_pr:" << ret_log10_pr << std::endl);
		return ret_log10_pr;
	}

	//returns the scores for all topics in the topic models (without apriori topic prob)
	void ContextSimilarity::get_topic_scores(topic_map_t& topic_map, string_vec_t& text)
	{				
		ngram base_num_ng(m_hwk_ngt->getDict());
		ngram base_den_ng(m_hk_ngt->getDict());
		create_ngram(text, base_num_ng, base_den_ng);
		
		
		if (m_active){ //similarity score is disable
			for (int i=0; i<m_k_ngt->getDict()->size();++i)
			{
				ngram num_ng = base_num_ng;
				ngram den_ng = base_den_ng;
				std::string _topic = m_k_ngt->getDict()->decode(i);
				add_topic(_topic, num_ng, den_ng);
				topic_map[_topic] = get_topic_similarity(num_ng, den_ng);
			}
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
		//text must have at least one word
		//if text has two word, further computation will rely on normal counts, i.e. counts(h,w,k), counts(h,w), counts(h,k), counts(k)
		//if text has only one word, further computation will rely on lower-order counts, i.e. (w,k), counts(w), counts(k), counts()
		VERBOSE(2,"void ContextSimilarity::create_ngram" << std::endl);
		VERBOSE(2,"text.size:" << text.size() << std::endl);

		MY_ASSERT(text.size()>0);
		
		if (text.size()==1){
			//all further computation will rely on lower-order counts
			num_ng.pushw(text.at(text.size()-1));
		}else {
			num_ng.pushw(text.at(text.size()-2));
			num_ng.pushw(text.at(text.size()-1));
			den_ng.pushw(text.at(text.size()-2));
		}
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
		ngram num_ng(m_hwk_ngt->getDict());
		ngram den_ng(m_hk_ngt->getDict());
		
		create_topic_ngram(text, topic, num_ng, den_ng);
		
		return get_topic_similarity(num_ng, den_ng);
	}
	
	double ContextSimilarity::get_topic_similarity(ngram& num_ng, ngram& den_ng)
	{			
		VERBOSE(2, "double ContextSimilarity::get_topic_similarity(ngram& num_ng, ngram& den_ng) with  num_ng:|" << num_ng << "| den_ng:|" << den_ng << "|" << std::endl);
		
		double num_log_pr, den_log_pr;
		
		double c_hk=m_smoothing, c_h=m_smoothing * m_topic_size;
		double c_hwk=m_smoothing, c_hw=m_smoothing * m_topic_size;
		
		if (den_ng.size == m_hk_order){//we rely on counts(h,k) and counts(h)
			if (m_hk_ngt->get(den_ng)) {	c_hk += den_ng.freq; }
			if (m_hk_ngt->get(den_ng,den_ng.size,den_ng.size-1)) { c_h += den_ng.freq; }
		}else{//we actually rely on counts(k) and counts()
			c_hk += m_hk_ngt->getDict()->freq(*(den_ng.wordp(1)));
			c_h += m_k_ngt->getDict()->totfreq();
		}
		den_log_pr = log10(c_hk) - log10(c_h);
		VERBOSE(3, "c_hk:" << c_hk << " c_h:" << c_h << std::endl);
		
		if (num_ng.size == m_hwk_order){ //we rely on counts(h,w,k) and counts(h,w)
			if (reliable(num_ng, num_ng.size, m_hwk_ngt)){
				if (m_hwk_ngt->get(num_ng)) {	c_hwk += num_ng.freq; }
				if (m_hwk_ngt->get(num_ng,num_ng.size,num_ng.size-1)) { c_hw += num_ng.freq; }
			}else{
				c_hwk=1;
				c_hk=m_topic_size;
			}
		}else{ //hence num_ng.size=m_hwk_order-1, we actually rely on counts(h,k) and counts(h)
			if (reliable(num_ng, num_ng.size, m_hk_ngt)){
				if (m_hk_ngt->get(num_ng)) {	c_hwk += num_ng.freq; }
				if (m_hk_ngt->get(num_ng,num_ng.size,num_ng.size-1)) { c_hw += num_ng.freq; }
			}else{
				c_hwk=1;
				c_hk=m_topic_size;
			}
		}
		VERBOSE(3, "c_hwk:" << c_hwk << " c_hw:" << c_hw << std::endl);
		num_log_pr = log10(c_hwk) - log10(c_hw);
		
		VERBOSE(3, "num_log_pr:" << num_log_pr << " den_log_pr:" << den_log_pr << std::endl);
		return num_log_pr - den_log_pr;
	}
	
	bool ContextSimilarity::reliable(ngram& ng, int size, ngramtable* ngt)
	{
		VERBOSE(2, "ContextSimilarity::reliable(ngram& ng, int size, ngramtable* ngt) ng:|" << ng << "| thr:" << m_threshold_on_h << "| ng.size:" << ng.size << " size:" << size << std::endl);	
		
		bool ret=false;
		
		if (ng.size < size){
			//num_ng has size lower than expected (2)
			//in this case we will rely on counts(h, topic) instead of counts(h, w, topic)
			VERBOSE(3, "ng:|" << ng << "| has size (" << ng.size<< " ) lower than expected (" << size << ")" << std::endl);
			ret=true;
		}
		
		if (ngt->get(ng,size,size-1) && (ng.freq > m_threshold_on_h)){
			ret=true;
		}else{
			ret=false;
		}
		VERBOSE(3, "ng:|" << ng << "| thr:" << m_threshold_on_h << " reliable:" << ret << std::endl);
		return ret;
	}
	
	bool ContextSimilarity::base_reliable(ngram& ng, int size, ngramtable* ngt)
	{
		VERBOSE(2, "ContextSimilarity::base_reliable(ngram& ng, int size, ngramtable* ngt) ng:|" << ng << "| thr:" << m_threshold_on_h << "|" << std::endl);

		bool ret=false;
		
		if (ng.size < size){
			//den_ng has size lower than expected (1)
			//in this case we will rely on counts(topic) instead of counts(h, topic)
			VERBOSE(3, "ng:|" << ng << "| has size (" << ng.size<< " ) lower than expected (" << size << ")" << std::endl);
			ret=true;
		}
		else{
			ng.pushc(0);
			if (m_hk_ngt->get(ng,size,size-1) && (ng.freq > m_threshold_on_h)){
				ret=true;
			}else{
				ret=false;
			}
			ng.shift();
		}
		VERBOSE(3, "ng:|" << ng << "| thr:" << m_threshold_on_h << " reliable:" << ret << std::endl);
		return ret;
	}
	
}//namespace irstlm
