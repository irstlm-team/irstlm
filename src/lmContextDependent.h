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
#include <string>
#include <math.h>
#include "util.h"
#include "dictionary.h"
#include "n_gram.h"
#include "lmContainer.h"

namespace irstlm {
	/*
	 Context-dependent LM
	 Wrapper LM which combines a standard ngram-based word-based LM
	 and a bigram-based topic model 
	 */
	
#define LMCONFIGURE_MAX_TOKEN 3
	
	class lmContextDependent: public lmContainer
	{
		static const bool debug=true;
		int order;
		int dictionary_upperbound; //set by user
		double  logOOVpenalty; //penalty for OOV words (default 0)
		bool      isInverted;
		int memmap;  //level from which n-grams are accessed via mmap
		
		lmContainer* m_lm;
		std::string m_lm_file;
		bool m_isinverted;
		
		//  TopicModel* m_topicmodel;
		lmContainer* m_topicmodel;   //to remove when TopicModel is ready
		double m_lm_weight;
		
		double m_topicmodel_weight;
		std::string m_topicmodel_file;
		
		float ngramcache_load_factor;
		float dictionary_load_factor;
		
		dictionary *dict; // dictionary for all interpolated LMs
		
	public:
		
		lmContextDependent(float nlf=0.0, float dlfi=0.0);
		virtual ~lmContextDependent();
		
		void load(const std::string &filename,int mmap=0);
		
		virtual double clprob(ngram ng,            double* bow=NULL,int* bol=NULL,char** maxsuffptr=NULL,unsigned int* statesize=NULL,bool* extendible=NULL){
			VERBOSE(0, "virtual double clprob(ngram ng,            double* bow=NULL,int* bol=NULL,char** maxsuffptr=NULL,unsigned int* statesize=NULL,bool* extendible=NULL)" << std::endl << "This LM type (lmContextDependent) does not support this function" << std::endl);
			VERBOSE(0, "This LM type (lmContextDependent) does not support this function");
			UNUSED(ng);
			UNUSED(bow);
			UNUSED(bol);
			UNUSED(maxsuffptr);
			UNUSED(statesize);
			UNUSED(extendible);
			assert(false);
		};
		
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
		
		virtual double clprob(int* ng, int ngsize, topic_map_t& topic_weights, double* bow=NULL,int* bol=NULL,char** maxsuffptr=NULL,unsigned int* statesize=NULL,bool* extendible=NULL){
			return lprob(ng, ngsize, topic_weights, bow, bol, maxsuffptr, statesize, extendible);
		};
		virtual double clprob(ngram ng, topic_map_t& topic_weights, double* bow=NULL,int* bol=NULL,char** maxsuffptr=NULL,unsigned int* statesize=NULL,bool* extendible=NULL){
			return lprob(ng, topic_weights, bow, bol, maxsuffptr, statesize, extendible);
		};
		virtual double lprob(int* ng, int ngsize, topic_map_t& topic_weights, double* bow=NULL,int* bol=NULL,char** maxsuffptr=NULL,unsigned int* statesize=NULL,bool* extendible=NULL);
		virtual double lprob(ngram ng, topic_map_t& topic_weights, double* bow=NULL,int* bol=NULL,char** maxsuffptr=NULL,unsigned int* statesize=NULL,bool* extendible=NULL);
		
		int maxlevel() const {
			return maxlev;
		};
		
		virtual inline void setDict(dictionary* d) {
			if (dict) delete dict;
			dict=d;
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
	};
}//namespace irstlm

#endif

