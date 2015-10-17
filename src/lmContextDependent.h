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

#ifndef MF_LMCONTEXTDEPENDENT_H
#define MF_LMCONTEXTDEPENDENT_H

#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>
#include "util.h"
#include "dictionary.h"
#include "n_gram.h"
#include "lmContainer.h"
#include "context-similarity.h"

namespace irstlm {
	class PseudoTopicModel
	{
	public:
		PseudoTopicModel(){};
		~PseudoTopicModel(){};
		
		void load(const std::string &filename){
			UNUSED(filename);
		};
		
		double prob(string_vec_t& text, topic_map_t& topic_weights){
			UNUSED(text);
			UNUSED(topic_weights);
			return 1.0;
		}
	};
}

namespace irstlm {
	/*
	 Context-dependent LM
	 Wrapper LM which combines a standard ngram-based word-based LM
	 and a bigram-based topic model 
	 */
	
#define LMCONTEXTDEPENDENT_CONFIGURE_MAX_TOKEN 6
	
	static const std::string context_delimiter="___CONTEXT___";
	
	class lmContextDependent: public lmContainer
	{
	private:
		static const bool debug=true;
		int order;
		int dictionary_upperbound; //set by user
		double  logOOVpenalty; //penalty for OOV words (default 0)
		bool      isInverted;
		int memmap;  //level from which n-grams are accessed via mmap
		
		lmContainer* m_lm;
		bool m_isinverted;
		
		
		//flag for enabling/disabling normalization of the language model 
		// if disabled, score returns by the language model do not sum to 1.0
		bool m_normalization;
		
		ContextSimilarity* m_similaritymodel;   //to remove when TopicModel is ready
		double m_lm_weight;
		
		double m_similaritymodel_weight;
		
		float ngramcache_load_factor;
		float dictionary_load_factor;
		
		dictionary *dict; // dictionary for all interpolated LMs
		
	public:
		
		lmContextDependent(float nlf=0.0, float dlfi=0.0);
		virtual ~lmContextDependent();
		
		void load(const std::string &filename,int mmap=0);
		
		
		inline std::string getContextDelimiter() const{
			return context_delimiter;
		}
		
		void GetSentenceAndContext(std::string& sentence, std::string& context, std::string& line);
		
		virtual double clprob(int* ng, int ngsize, double* bow=NULL,int* bol=NULL,char** maxsuffptr=NULL,unsigned int* statesize=NULL,bool* extendible=NULL){
			VERBOSE(0, "virtual double clprob(int* ng, int ngsize, double* bow=NULL,int* bol=NULL,char** maxsuffptr=NULL,unsigned int* statesize=NULL,bool* extendible=NULL)" << std::endl << "This LM type (lmContextDependent) does not support this function" << std::endl);
			UNUSED(ng);
			UNUSED(ngsize);
			UNUSED(bow);
			UNUSED(bol);
			UNUSED(maxsuffptr);
			UNUSED(statesize);
			UNUSED(extendible);
			assert(false);
		};
		
		virtual double clprob(ngram ng,            double* bow=NULL,int* bol=NULL,char** maxsuffptr=NULL,unsigned int* statesize=NULL,bool* extendible=NULL){
			VERBOSE(0, "virtual double clprob(ngram ng,            double* bow=NULL,int* bol=NULL,char** maxsuffptr=NULL,unsigned int* statesize=NULL,bool* extendible=NULL)" << std::endl << "This LM type (lmContextDependent) does not support this function" << std::endl);
			UNUSED(ng);
			UNUSED(bow);
			UNUSED(bol);
			UNUSED(maxsuffptr);
			UNUSED(statesize);
			UNUSED(extendible);
			assert(false);
		};
		
		virtual double clprob(string_vec_t& text, double* bow=NULL,int* bol=NULL,char** maxsuffptr=NULL,unsigned int* statesize=NULL,bool* extendible=NULL){
			VERBOSE(0, "virtual double clprob(string_vec_t& text, int ngsize, double* bow=NULL,int* bol=NULL,char** maxsuffptr=NULL,unsigned int* statesize=NULL,bool* extendible=NULL)" << std::endl << "This LM type (lmContextDependent) does not support this function" << std::endl);
			UNUSED(text);
			UNUSED(bow);
			UNUSED(bol);
			UNUSED(maxsuffptr);
			UNUSED(statesize);
			UNUSED(extendible);
			assert(false);
		};
		
		virtual double clprob(int* ng, int ngsize, topic_map_t& topic_weights, double* bow=NULL,int* bol=NULL,char** maxsuffptr=NULL,unsigned int* statesize=NULL,bool* extendible=NULL){
			return lprob(ng, ngsize, topic_weights, bow, bol, maxsuffptr, statesize, extendible);
		};
		virtual double clprob(ngram ng, topic_map_t& topic_weights, double* bow=NULL,int* bol=NULL,char** maxsuffptr=NULL,unsigned int* statesize=NULL,bool* extendible=NULL){
			return lprob(ng, topic_weights, bow, bol, maxsuffptr, statesize, extendible);
		};
		virtual double clprob(string_vec_t& text, topic_map_t& topic_weights, double* bow=NULL,int* bol=NULL,char** maxsuffptr=NULL,unsigned int* statesize=NULL,bool* extendible=NULL){
			return lprob(text, topic_weights, bow, bol, maxsuffptr, statesize, extendible);
		};
		
		virtual double lprob(int* ng, int ngsize, topic_map_t& topic_weights, double* bow=NULL,int* bol=NULL,char** maxsuffptr=NULL,unsigned int* statesize=NULL,bool* extendible=NULL);
		virtual double lprob(ngram ng, topic_map_t& topic_weights, double* bow=NULL,int* bol=NULL,char** maxsuffptr=NULL,unsigned int* statesize=NULL,bool* extendible=NULL);
		virtual double lprob(string_vec_t& text, topic_map_t& topic_weights, double* bow=NULL,int* bol=NULL,char** maxsuffptr=NULL,unsigned int* statesize=NULL,bool* extendible=NULL);
		
		double lprob(ngram& ng, string_vec_t& text, topic_map_t& topic_weights, double* bow,int* bol,char** maxsuffptr,unsigned int* statesize,bool* extendible);
		double total_clprob(string_vec_t& text, topic_map_t& topic_weights);
		double total_clprob(ngram& ng, topic_map_t& topic_weights);		
		
		
		
		virtual inline int get(ngram& ng) {
			return m_lm->get(ng);
		}
		
		virtual int get(ngram& ng,int n,int lev){
			return m_lm->get(ng,n,lev);
		}
		
		virtual int succscan(ngram& h,ngram& ng,LMT_ACTION action,int lev){
			return m_lm->succscan(h,ng,action,lev);
		}
		
		int maxlevel() const {
			return maxlev;
		};
		
		virtual inline void setDict(dictionary* d) {
			if (dict) delete dict;
			dict=d;
		};
		
		virtual inline lmContainer* getWordLM() const {
			return m_lm;
		};
		
		virtual inline ContextSimilarity* getContextSimilarity() const {
			return m_similaritymodel;
		};
		
		virtual inline dictionary* getDict() const {
			return dict;
		};
		
		//set penalty for OOV words
		virtual inline double getlogOOVpenalty() const {
			return logOOVpenalty;
		}
		
		virtual double setlogOOVpenalty(int dub);
		
		double inline setlogOOVpenalty(double oovp) {
			return logOOVpenalty=oovp;
		}
		
		//set the inverted flag
		inline bool is_inverted(const bool flag) {
			return isInverted = flag;
		}
		
		//for an interpolation LM this variable does not make sense
		//for compatibility, we return true if all subLM return true
		inline bool is_inverted() {
			return m_isinverted;
		}
		
		inline virtual void dictionary_incflag(const bool flag) {
			dict->incflag(flag);
		};
		
		inline virtual bool is_OOV(int code) { //returns true if the word is OOV for each subLM
			return m_lm->is_OOV(code);
		}
		
		inline void set_Active(bool value){
			m_similaritymodel->set_Active(value);
		}
		
		bool is_Normalized(){
			return  m_normalization;
		}
		void set_Normalized(bool val){
			m_normalization = val;
		}
		
	};
}//namespace irstlm

#endif

