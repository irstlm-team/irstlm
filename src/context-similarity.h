// $Id: lmContextDependent.h 3686 2010-10-15 11:55:32Z bertoldi $

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

#ifndef MF_CONTEXTSIMILARITY_H
#define MF_CONTEXTSIMILARITY_H

#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>
#include <string>
#include <math.h>
#include "cmd.h"
#include "util.h"
#include "dictionary.h"
#include "n_gram.h"
#include "ngramtable.h"
#include "lmContainer.h"

class ngram;

namespace irstlm {
#define topic_map_delimiter1 ':'
#define topic_map_delimiter2 ','
#define SIMILARITY_LOWER_BOUND -10000
	
	typedef std::map< std::string, float > topic_map_t;
	
	class ContextSimilarity
	{
	private:
		ngramtable* m_hwk_ngt; // counts(h, w, topic)
		ngramtable* m_hk_ngt; // counts(h, topic)
		ngramtable* m_wk_ngt; // counts(w, topic)
		ngramtable* m_k_ngt; // counts(topic)
		int m_k_order; //order of m_k_ngt
		int m_hk_order; //order of m_hk_ngt
		int m_wk_order; //order of m_wk_ngt
		int m_hwk_order; //order of m_hwk_ngt
	
		int m_topic_size; //number of topics in the model
		
		topic_map_t topic_map; 
		int m_threshold_on_h; //frequency threshold on h to allow computation of similairty scores
		double m_smoothing; //smoothing value to sum to the counts to avoid zero-prob; implements a sort of shift-beta smoothing

		//flag for enabling/disabling context_similarity scores
		// if disabled, context_similarity is 0.0 and topic_scores distribution is empty
		bool m_active;
		
		void create_ngram(const string_vec_t& text, ngram& ng);
		void add_topic(const std::string& topic, ngram& ng);
		void modify_topic(const std::string& topic, ngram& ng);
		void create_topic_ngram(const string_vec_t& text, const std::string& topic, ngram& ng);
	
		void get_counts(ngram& ng, ngramtable& ngt, double& c_xk, double& c_x);
		
		double topic_score(string_vec_t text, const std::string& topic, ngramtable& ngt, ngramtable& ngt2);
		double topic_score(ngram& ng, ngramtable& ngt, ngramtable& ngt2);
		double topic_score_option0(ngram& ng, ngramtable& ngt, ngramtable& ngt2);
		double topic_score_option1(ngram& ng, ngramtable& ngt, ngramtable& ngt2);
		double topic_score_option2(ngram& ng, ngramtable& ngt, ngramtable& ngt2);
		double topic_score_option3(ngram& ng, ngramtable& ngt, ngramtable& ngt2);
		
		double total_topic_score(string_vec_t text, const std::string& topic, ngramtable& ngt, ngramtable& ngt2, dictionary& dict);
		double total_topic_score(string_vec_t text, const std::string& topic, ngramtable& ngt, ngramtable& ngt2, dictionary& dict, lmContainer& lm, double weight);
		double total_topic_score(ngram& ng, ngramtable& ngt, ngramtable& ngt2, dictionary& dict);
		double total_topic_score(ngram& ng, ngramtable& ngt, ngramtable& ngt2, dictionary& dict, lmContainer& lm, double weight);
		
		
		
		void modify_context_map(string_vec_t& text, ngramtable& ngt, ngramtable& ngt2, dictionary& dict, topic_map_t& topic_weights, topic_map_t& mod_topic_weights);
		void modify_context_map(ngram& ng, ngramtable& ngt, ngramtable& ngt2, dictionary& dict, topic_map_t& topic_weights, topic_map_t& mod_topic_weights);
		void modify_context_map(string_vec_t& text, ngramtable& ngt, ngramtable& ngt2, dictionary& dict, lmContainer& lm, double weight, topic_map_t& topic_weights, topic_map_t& mod_topic_weights);
		void modify_context_map(ngram& ng, ngramtable& ngt, ngramtable& ngt2, dictionary& dict, lmContainer& lm, double weight, topic_map_t& topic_weights, topic_map_t& mod_topic_weights);
		
		double context_similarity_solution1(string_vec_t& text, topic_map_t& topic_weights);
		double context_similarity_solution2(string_vec_t& text, topic_map_t& topic_weights);
		
		bool reliable(ngram& ng, ngramtable* ngt);
		
	public:
		ContextSimilarity(const std::string &dictfile, const std::string &num_modelfile, const std::string &den_modelfile);
		~ContextSimilarity();
		
		void setContextMap(topic_map_t& topic_map, const std::string& context);
		
		void get_topic_scores(string_vec_t& text, topic_map_t& topic_map);
		void get_topic_scores(ngram& ng, ngramtable& ngt, ngramtable& ngt2, topic_map_t& topic_map);
		
		void add_topic_scores(topic_map_t& map, topic_map_t& tmp_map);
		void print_topic_scores(topic_map_t& map);
		void print_topic_scores(topic_map_t& map, topic_map_t& refmap, double len);
		double DeltaCrossEntropy(topic_map_t& topic_map, topic_map_t& tmp_map, double len);

		void normalize_topic_scores(topic_map_t& map);
		
		double context_similarity(string_vec_t& text, topic_map_t& topic_weights);
		
		int get_Threshold_on_H(){
			return  m_threshold_on_h;
		}
		void set_Threshold_on_H(int val){
			m_threshold_on_h = val;
		}
		double get_SmoothingValue(){
			return  m_smoothing;
		}
		void set_SmoothingValue(double val){
			m_smoothing = val;
		}
		bool is_Active(){
			return  m_active;
		}
		void set_Active(bool val){
			m_active = val;
		}
		
	};
}


#endif

