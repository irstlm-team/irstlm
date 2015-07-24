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
	
	
	typedef std::map< std::string, float > topic_map_t;
	typedef std::set< std::string > topic_dict_t;
	
	#define SIMILARITY_LOWER_BOUND -10000
	class ContextSimilarity
	{
	private:
		lmContainer* m_lm; // P(topic | h' w)
		topic_dict_t* m_lm_topic_dict; //the dictionary of the topics seen in the language model
		topic_map_t* topic_map; 
		
		void create_ngram(const string_vec_t& text, ngram& num_ng, ngram& den_ng);
		void add_topic(const std::string& topic, ngram& num_ng, ngram& den_ng);
		void create_topic_ngram(const string_vec_t& text, const std::string& topic, ngram& num_ng, ngram& den_ng);
		
		double get_topic_similarity(string_vec_t text, const std::string& topic);
		double get_topic_similarity(ngram& num_ng, ngram& den_ng);
		
	public:
		ContextSimilarity(const std::string &filename);
		~ContextSimilarity();

		topic_map_t get_topic_scores(string_vec_t& text);
		
		double score(string_vec_t& text, topic_map_t& topic_weights);
	};
}


#endif

