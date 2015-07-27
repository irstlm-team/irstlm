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
#include <set>
#include "util.h"
#include "dictionary.h"
#include "n_gram.h"
#include "lmContainer.h"

class ngram;

namespace irstlm {
	#define topic_map_delimiter1 ':'
	#define topic_map_delimiter2 ','
	#define SIMILARITY_LOWER_BOUND -10000
	
	typedef std::map< std::string, float > topic_map_t;
	typedef std::set< std::string > topic_dict_t;
	

	class ContextSimilarity
	{
	private:
		lmContainer* m_num_lm; // P(topic | h' w)
		lmContainer* m_den_lm; // P(topic | h')
		topic_dict_t m_lm_topic_dict; //the dictionary of the topics seen in the language model
		topic_map_t topic_map; 
		
		void create_ngram(const string_vec_t& text, ngram& num_ng, ngram& den_ng);
		void add_topic(const std::string& topic, ngram& num_ng, ngram& den_ng);
		void create_topic_ngram(const string_vec_t& text, const std::string& topic, ngram& num_ng, ngram& den_ng);
		
		double get_topic_similarity(string_vec_t text, const std::string& topic);
		double get_topic_similarity(ngram& num_ng, ngram& den_ng);
		
	public:
		ContextSimilarity(const std::string &dictfile, const std::string &num_modelfile, const std::string &den_modelfile);
		~ContextSimilarity();
		
		void setContextMap(topic_map_t& topic_map, const std::string& context);
		void get_topic_scores(topic_map_t& map, string_vec_t& text);
		void add_topic_scores(topic_map_t& map, topic_map_t& tmp_map);
		void print_topic_scores(topic_map_t& map);
		
		double score(string_vec_t& text, topic_map_t& topic_weights);
	};
}


#endif

