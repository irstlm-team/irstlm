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
		m_wk_order=m_hk_order;
		m_k_order=1;
		m_hwk_ngt=new ngramtable((char*) hwk_modelfile.c_str(), m_hwk_order, NULL,NULL,NULL);
		m_hk_ngt=new ngramtable((char*) hk_modelfile.c_str(), m_hk_order, NULL,NULL,NULL);
		m_wk_ngt=m_hk_ngt; //just a link to m_hk_ngt
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
		//delete m_wk_ngt;  //it is just a link to m_hk_ngt
		delete m_k_ngt;
	}
	
	void ContextSimilarity::normalize_topic_scores(topic_map_t& map)
	{	
		UNUSED(map);
		/* normalization type 1
		 double max = -1000000.0;
		 double min =  1000000.0;
		 for (topic_map_t::iterator it=map.begin(); it != map.end(); ++it){
		 min = (map[it->first]<min)?map[it->first]:min;
		 max = (map[it->first]>max)?map[it->first]:max;
		 }
		 for (topic_map_t::iterator it=map.begin(); it != map.end(); ++it){
		 map[it->first] = (map[it->first]-min)/(max-min);
		 }
		 VERBOSE(2,"min:"<<min << " max:" << max << std::endl);
		 */
		/*
		 //normalization type 2
		 double norm =  0.0;
     for (topic_map_t::iterator it=map.begin(); it != map.end(); ++it){
		 norm += fabs(map[it->first]);
		 }
		 for (topic_map_t::iterator it=map.begin(); it != map.end(); ++it){
		 map[it->first] = map[it->first]/norm;
		 }
		 VERBOSE(2,"norm:" << norm << std::endl);
		 */
	}
	
	double ContextSimilarity::DeltaCrossEntropy(topic_map_t& topic_map, topic_map_t& tmp_map, double len)
	{
		double xDeltaEntropy = 0.0;
		for (topic_map_t::iterator it=tmp_map.begin(); it != tmp_map.end(); ++it){
			xDeltaEntropy += topic_map[it->first] * tmp_map[it->first];
			//			VERBOSE(2,"topic_map[it->first]:" << topic_map[it->first] << " tmp_map[it->first]:" << tmp_map[it->first] << " product:" << topic_map[it->first] * tmp_map[it->first] << std::endl);
		}
		//		VERBOSE(2," xDeltaEntropy:" << xDeltaEntropy << " len:" << len << " xDeltaEntropy/len:" << xDeltaEntropy/len << std::endl);
		return xDeltaEntropy/len;
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
			//			std::cout << it->first << topic_map_delimiter2 << exp(it->second * M_LN10);
		}
		
		std::cout << std::endl;
	}
	
	void ContextSimilarity::print_topic_scores(topic_map_t& map, topic_map_t& refmap, double len)
	{
		for (topic_map_t::iterator it=map.begin(); it != map.end(); ++it)
		{
			if (it!=map.begin()) { std::cout << topic_map_delimiter1; }
			std::cout << it->first << topic_map_delimiter2 << it->second;
		}
		std::cout << " DeltaCrossEntropy:" << DeltaCrossEntropy(refmap,map,len);
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
	
	void ContextSimilarity::create_ngram(const string_vec_t& text, ngram& ng)
	{
		//text is a vector of strings with w in the last position and the history in the previous positions
		//text must have at least one word
		//if text has two word, further computation will rely on normal counts, i.e. counts(h,w,k), counts(h,w), counts(w,k), counts(h,k), counts(k)
		//if text has only one word, further computation will rely on lower-order counts, i.e. (w,k), counts(w), counts(w), counts(k), counts()
		VERBOSE(2,"void ContextSimilarity::create_ngram" << std::endl);
		VERBOSE(2,"text.size:" << text.size() << std::endl);
		
		MY_ASSERT(text.size()>0);
		
		if (text.size()==1){
			//all further computation will rely on lower-order counts
			ng.pushw(text.at(text.size()-1));
		}else {
			ng.pushw(text.at(text.size()-2));
			ng.pushw(text.at(text.size()-1));
		}
		VERBOSE(2,"output of create_ngram ng:|" << ng << "| ng.size:" << ng.size << std::endl);
	}
	
	void ContextSimilarity::create_topic_ngram(const string_vec_t& text, const std::string& topic, ngram& ng)
	{
		//text is a vector of string with w in the last position and the history in the previous positions
		//text must have at least one word
		//topic is added in the most recent position of the ngram
		create_ngram(text, ng);
		add_topic(topic, ng);
		VERBOSE(2,"output of create_topic_ngram ng:|" << ng << "| ng.size:" << ng.size << std::endl);
	}
	
	void ContextSimilarity::add_topic(const std::string& topic, ngram& ng)
	{		
		ng.pushw(topic);
	}
	
	void ContextSimilarity::modify_topic(const std::string& topic, ngram& ng)
	{		
		*ng.wordp(1) = ng.dict->encode(topic.c_str());
	}
	
	void ContextSimilarity::get_counts(ngram& ng, ngramtable& ngt, double& c_xk, double& c_x)
	{			
		VERBOSE(2, "double ContextSimilarity::get_counts(ngram& ng, double& c_xk, double& c_x) with ng:|" << ng << "|" << std::endl);
		//counts taken from the tables are modified to avoid zero values for the probs
		//a constant epsilon (smmothing) is added
		//we also assume that c(x) = sum_k c(xk)
		
		//we assume that ng ends with a correct topic 
		//we assume that ng is compliant with ngt, and has the correct size
		
		c_xk = m_smoothing;
		c_x  = m_smoothing * m_topic_size;
		
		if (ngt.get(ng)) { c_xk += ng.freq; }
		if (ngt.get(ng,ng.size,ng.size-1)) { c_x += ng.freq; }
		
		VERBOSE(3, "c_xk:" << c_xk << " c_x:" << c_x << std::endl);
	}
	
	double ContextSimilarity::topic_score(string_vec_t text, const std::string& topic, ngramtable& ngt, ngramtable& ngt2)
	{
		ngram ng(ngt.getDict());
		
		create_topic_ngram(text, topic, ng);
		
		return topic_score(ng, ngt, ngt2);
	}
	
	double ContextSimilarity::topic_score(ngram& ng, ngramtable& ngt, ngramtable& ngt2){
#ifdef OPTION_1
		return topic_score_option1(ng, ngt, ngt2);
#elif OPTION_2
		return topic_score_option2(ng, ngt, ngt2);
#elif OPTION_3
		return topic_score_option3(ng, ngt, ngt2);
#else
		return topic_score_option0(ng, ngt, ngt2);
#endif
	}
	
	double ContextSimilarity::topic_score_option0(ngram& ng, ngramtable& ngt, ngramtable& ngt2)
	{
		UNUSED(ngt);
		UNUSED(ngt2);
		VERBOSE(2, "double ContextSimilarity::topic_score_option0(ngram& ng, ngramtable& ngt) with ng:|" << ng << "|" << std::endl);
		
		//option 0: uniform (not considering log function) 
		//P(k|hw) = 1/number_of_topics
		double log_pr = -log(m_topic_size)/M_LN10;
		
		VERBOSE(3, "option0: return: " << log_pr<< std::endl);	
		return log_pr;
	}
	
	double ContextSimilarity::topic_score_option1(ngram& ng, ngramtable& ngt, ngramtable& ngt2)
	{
		VERBOSE(2, "double ContextSimilarity::topic_score_option1(ngram& ng, ngramtable& ngt) with ng:|" << ng << "|" << std::endl);
		
		//ngt provides counts c(hwk) and c(hw) (or c(wk) and c(w))
		double c_xk, c_x;
		get_counts(ng, ngt, c_xk, c_x);
		
		//copy and transform codes
		// shift all terms, but the topic
		// ng2[3]=ng[4];
		// ng2[2]=ng[3];
		// ng2[1]=ng[1];
		ngram ng2(ngt2.getDict());
		ng2.trans(ng);
		ng2.shift();
		*ng2.wordp(1)=ng2.dict->encode(ng.dict->decode(*ng.wordp(1)));
		
		//ngt2 provides counts c(hk) and c(h) (or c(k) and c())
		double c_xk2, c_x2;
		get_counts(ng2, ngt2, c_xk2, c_x2);
		
		//option 1: (not considering log function) 
		//P(k|hw)/sum_v P(k|hv) ~approx~ P(k|hw)/P(k|h) ~approx~ num_pr/den_pr
		//num_pr = c'(hwk)/c'(hw)
		//den_pr = c'(hk)/c'(h)
		double den_log_pr = log10(c_xk2) - log10(c_x2);
		double num_log_pr = log10(c_xk) - log10(c_x);
		double log_pr = num_log_pr - den_log_pr;
		VERBOSE(3, "option1: num_log_pr:" << num_log_pr << " den_log_pr:" << den_log_pr << " return: " << log_pr << std::endl);
		return log_pr;
	}
	
	double ContextSimilarity::topic_score_option2(ngram& ng, ngramtable& ngt, ngramtable& ngt2)
	{
		UNUSED(ngt2);
		VERBOSE(2, "double ContextSimilarity::topic_score_option2(ngram& ng, ngramtable& ngt) with ng:|" << ng << "|" << std::endl);
		
		//ngt provides counts c(hwk) and c(hw) (or c(wk) and c(w))
		double c_xk, c_x;
		get_counts(ng, ngt, c_xk, c_x);
		
		//option 1: (not considering log function) 
		//P(k|hw)/sum_v P(k|hv) ~approx~ P(k|hw)/P(k|h) ~approx~ c'(hwk)/c'(hw)
		double log_pr = log10(c_xk) - log10(c_x);
		VERBOSE(3, "option2: log_pr:" << log_pr << " return: " << log_pr << std::endl);
		return log_pr;
	}
	
	double ContextSimilarity::topic_score_option3(ngram& ng, ngramtable& ngt, ngramtable& ngt2)
	{
		VERBOSE(2, "double ContextSimilarity::topic_score_option3(ngram& ng, ngramtable& ngt) with ng:|" << ng << "|" << std::endl);
		
		//ngt provides counts c(hwk) and c(hw) (or c(wk) and c(w))
		double c_xk, c_x;
		get_counts(ng, ngt, c_xk, c_x);
		
		//copy and transform codes
		// shift all terms, but the topic
		// ng2[3]=ng[4];
		// ng2[2]=ng[3];
		// ng2[1]=ng[1];
		ngram ng2(ngt2.getDict());
		ng2.trans(ng);
		ng2.shift();
		*ng2.wordp(1)=ng2.dict->encode(ng.dict->decode(*ng.wordp(1)));
		
		//ngt2 provides counts c(hk) and c(h) (or c(k) and c())
		double c_xk2, c_x2;
		get_counts(ng2, ngt2, c_xk2, c_x2);
		
		/*;
		 //approximation 3: (not considering log function) 
		 //P(k|hw)/sum_v P(k|hv) ~approx~ logistic_function(P(k|hw)/P(k|h))
		 // ~approx~ logistic_function(num_pr/den_pr)
		 // ~approx~ logistic_function(c'(hwk)/c'(hw)/c'(hk)/c'(h))
		 // ~approx~ logistic_function((c'(hwk)*c'(h))/(c'(hw)*c'(hk)))
		 
		 return logistic_function((c'(hwk)*c'(h))/(c'(hw)*c'(hk)),1.0,1.0)
		 */
		
		double log_pr = logistic_function((c_xk*c_x2)/(c_x*c_xk2),1.0,1.0);
		
		VERBOSE(3, "option3: return: " << log_pr << std::endl);
		return log_pr;
	}
	
	double ContextSimilarity::total_topic_score(string_vec_t text, const std::string& topic, ngramtable& ngt, ngramtable& ngt2, dictionary& dict)
	{
		ngram ng(ngt.getDict());
		create_topic_ngram(text, topic, ng);
		return total_topic_score(ng, ngt, ngt2, dict);
	}
	
	double ContextSimilarity::total_topic_score(ngram& ng, ngramtable& ngt, ngramtable& ngt2, dictionary& dict)
	{		
		double tot_pr = 0.0;
		double v_topic_pr;
		for (int v=0; v<dict.size(); ++v){
			//replace last word, which is in position 2, keeping topic in position 1 unchanged
			*ng.wordp(2) = ng.dict->encode(dict.decode(v));
			v_topic_pr = topic_score(ng, ngt, ngt2);
			tot_pr += pow(10.0,v_topic_pr); //v_pr is a lo10 prob
		}
		return log10(tot_pr);
	}
	
	double ContextSimilarity::total_topic_score(string_vec_t text, const std::string& topic, ngramtable& ngt, ngramtable& ngt2, dictionary& dict, lmContainer& lm, double weight)
	{
		ngram ng(ngt.getDict());
		create_topic_ngram(text, topic, ng);
		return total_topic_score(ng, ngt, ngt2, dict, lm, weight);
	}
	
	double ContextSimilarity::total_topic_score(ngram& ng, ngramtable& ngt, ngramtable& ngt2, dictionary& dict, lmContainer& lm, double weight)
	{		
		double tot_pr = 0.0;
		double v_pr, v_topic_pr, v_lm_pr;
		for (int v=0; v<dict.size(); ++v){
			//replace last word, which is in position 2, keeping topic in position 1 unchanged
			*ng.wordp(2) = ng.dict->encode(dict.decode(v));
			v_topic_pr = topic_score(ng, ngt, ngt2);
			v_lm_pr = lm.clprob(ng);
			v_pr = v_lm_pr + weight * v_topic_pr;
			tot_pr += pow(10.0,v_pr); //v_pr is a lo10 prob
		}
		return log10(tot_pr);
	}
	
	void ContextSimilarity::modify_context_map(string_vec_t& text, ngramtable& ngt, ngramtable& ngt2, dictionary& dict, topic_map_t& topic_weights, topic_map_t& mod_topic_weights)
	{
		ngram ng(ngt.getDict());
		create_topic_ngram(text, "dummy_topic", ng); //just for initialization
		
		modify_context_map(ng, ngt, ngt2, dict, topic_weights, mod_topic_weights);
	}
	
	void ContextSimilarity::modify_context_map(ngram& ng, ngramtable& ngt, ngramtable& ngt2, dictionary& dict, topic_map_t& topic_weights, topic_map_t& mod_topic_weights)
	{
		double global_score;
		double mod_topic_pr;
		for (topic_map_t::iterator it = topic_weights.begin(); it!= topic_weights.end(); ++it)
		{
			modify_topic(it->first, ng);
			global_score = total_topic_score(ng, ngt, ngt2, dict);
			global_score = pow(10.0,global_score);
			mod_topic_pr = it->second/global_score;
			mod_topic_weights.insert(make_pair(it->first,mod_topic_pr));
		}
	}
	
	void ContextSimilarity::modify_context_map(string_vec_t& text, ngramtable& ngt, ngramtable& ngt2, dictionary& dict, lmContainer& lm, double weight, topic_map_t& topic_weights, topic_map_t& mod_topic_weights)
	{
		ngram ng(ngt.getDict());
		create_topic_ngram(text, "dummy_topic", ng); //just for initialization
		
		modify_context_map(ng, ngt, ngt2, dict, lm, weight, topic_weights, mod_topic_weights);
	}
	
	void ContextSimilarity::modify_context_map(ngram& ng, ngramtable& ngt, ngramtable& ngt2, dictionary& dict, lmContainer& lm, double weight, topic_map_t& topic_weights, topic_map_t& mod_topic_weights)
	{
		double global_score;
		double mod_topic_pr;
		for (topic_map_t::iterator it = topic_weights.begin(); it!= topic_weights.end(); ++it)
		{
			modify_topic(it->first, ng);
			global_score = total_topic_score(ng, ngt, ngt2, dict, lm, weight);
			global_score = pow(10.0,global_score);
			mod_topic_pr = it->second/global_score;
			mod_topic_weights.insert(make_pair(it->first,mod_topic_pr));
		}
	}
	
	
	double ContextSimilarity::context_similarity(string_vec_t& text, topic_map_t& topic_weights)
	{
#ifdef SOLUTION_1
		return context_similarity_solution1(text, topic_weights);
#elif SOLUTION_2
		return context_similarity_solution2(text, topic_weights);
#else
		UNUSED(text);
		UNUSED(topic_weights);
		VERBOSE(3, "This solution type is not defined; forced to default solution 1" << std::endl);
		return context_similarity_solution1(text, topic_weights);
//		exit(IRSTLM_CMD_ERROR_GENERIC);
#endif
	}
	
	//return the log10 of the similarity score
	double ContextSimilarity::context_similarity_solution1(string_vec_t& text, topic_map_t& topic_weights)
	{
		VERBOSE(2, "double ContextSimilarity::context_similarity_solution1(string_vec_t& text, topic_map_t& topic_weights)" << std::endl);
		double ret_log10_pr = 0.0;
		
		if (!m_active){
			//similarity score is disable
			//return an uninforming score (log(1.0) = 0.0)
			ret_log10_pr = 0.0;
		}
		else if (topic_weights.size() == 0){
			//a-priori topic distribution is "empty", i.e. there is no score for any topic
			//return an uninforming score (log(1.0) = 0.0)
			ret_log10_pr = 0.0;
		}
		else{
			VERBOSE(3, "topic_weights.size():" << topic_weights.size() << std::endl);
				
			ngramtable* current_ngt;
			ngramtable* current_ngt2;
			
			if (text.size()==1){
				current_ngt = m_wk_ngt;
				current_ngt2 = m_k_ngt;
			}
			else{
				current_ngt = m_hwk_ngt;
				current_ngt2 = m_hk_ngt;
			}
			
			ngram ng(current_ngt->getDict());
			create_topic_ngram(text, "dummy_topic", ng); //just for initialization
			
			
			if (reliable(ng, current_ngt)){
				//this word sequence is reliable
				
				double ret_pr = 0.0;
				for (topic_map_t::iterator it = topic_weights.begin(); it!= topic_weights.end(); ++it)
				{
					ngram current_ng = ng;
					modify_topic(it->first, current_ng);
					
					double apriori_topic_score = it->second; //prob
					double current_topic_score = exp(topic_score(current_ng, *current_ngt, *current_ngt2) * M_LN10); //topic_score(...) returns  a log10; hence exp is applied to (score *  M_LN10)
					
					VERBOSE(3, "current_ng:|" << current_ng << "| topic:|" << it->first  << "| apriori_topic_score:" << apriori_topic_score << " topic_score:" << current_topic_score << " score_toadd:" << ret_pr << std::endl);
					ret_pr += apriori_topic_score * current_topic_score;
					VERBOSE(3, "CURRENT ret_pr:" << ret_pr << std::endl);
				}
				ret_log10_pr = log10(ret_pr);
			}
			else{
				//this word sequence is not reliable enough, because occurrences of ng are too little
				//return an uninforming score (log10(1/K) = -log10(K))
				ret_log10_pr = -log(m_topic_size)/M_LN10;
//				ret_log10_pr = 0.0;
				VERBOSE(3, "CURRENT ret_pr:" << pow(10.0,ret_log10_pr) << std::endl);
			}
			
		}
		VERBOSE(2, "ret_log10_pr:" << ret_log10_pr << std::endl);
		return ret_log10_pr;
	}
	
	//return the log10 of the similarity score
	double ContextSimilarity::context_similarity_solution2(string_vec_t& text, topic_map_t& topic_weights)
	{
		return context_similarity_solution1(text, topic_weights);
	}
	
	bool ContextSimilarity::reliable(ngram& ng, ngramtable* ngt)
	{
		VERBOSE(2, "ContextSimilarity::reliable(ngram& ng, ngramtable* ngt) ng:|" << ng << "| ng.size:" << ng.size<< "| thr:" << m_threshold_on_h << std::endl);	
		
		bool ret=false;
		
		if (ngt->get(ng,ng.size,ng.size-1) && (ng.freq > m_threshold_on_h)){
			ret=true;
		}else{
			ret=false;
		}
		VERBOSE(3, "ng:|" << ng << "| thr:" << m_threshold_on_h << " reliable:" << ret << std::endl);
		return ret;
	}
	
	
	//returns the scores for all topics in the topic models (without apriori topic prob)
	void ContextSimilarity::get_topic_scores(string_vec_t& text, topic_map_t& topic_map)
	{
		if (m_active){ //similarity score is disable
			ngramtable* current_ngt;
			ngramtable* current_ngt2;
			
			if (text.size()==1){
				current_ngt = m_wk_ngt;
				current_ngt2 = m_k_ngt;
			}
			else{
				current_ngt = m_hwk_ngt;
				current_ngt2 = m_hk_ngt;
			}
			
			ngram ng(current_ngt->getDict());
			create_topic_ngram(text, "dummy_topic", ng); //just for initialization
			
			get_topic_scores(ng, *current_ngt, *current_ngt2, topic_map);
		}
	}	
	
	
	//returns the scores for all topics in the topic models (without apriori topic prob)
	void ContextSimilarity::get_topic_scores(ngram& ng, ngramtable& ngt, ngramtable& ngt2, topic_map_t& topic_map)
	{		
		if (m_active){ //similarity score is disable
			for (int i=0; i<m_k_ngt->getDict()->size();++i)
			{
				std::string _topic = m_k_ngt->getDict()->decode(i);
				modify_topic(_topic, ng);
				topic_map[_topic] = pow(10.0,topic_score(ng, ngt, ngt2));
			}
		}
	}	
	
}//namespace irstlm
