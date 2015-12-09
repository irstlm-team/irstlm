// $Id: lmInterpolation.h 3686 2010-10-15 11:55:32Z bertoldi $

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

#ifndef MF_LMINTERPOLATION_H
#define MF_LMINTERPOLATION_H

#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>
#include <string>
#include <math.h>
#include <vector>
#include "util.h"
#include "dictionary.h"
#include "n_gram.h"
#include "lmContainer.h"


namespace irstlm {
	/*
	 interpolation of several sub LMs
	 */
	
#define LMINTERPOLATION_MAX_TOKEN 3
	
	class lmInterpolation: public lmContainer
	{
		static const bool debug=true;
		size_t m_number_lm;
		int order;
		int dictionary_upperbound; //set by user
		double  logOOVpenalty; //penalty for OOV words (default 0)
		bool isInverted;
		bool m_map_flag; //flag for the presence of a map between name and lm
		int memmap;  //level from which n-grams are accessed via mmap
		
		std::vector<double> m_weight;
		std::vector<std::string> m_file;
		std::vector<bool> m_isinverted;
		std::vector<lmContainer*> m_lm;
		
		int maxlev; //maximun order of sub LMs;
		
		std::map< std::string, size_t > m_idx;
		std::map< size_t, std::string > m_name;
		
		float ngramcache_load_factor;
		float dictionary_load_factor;
		
		dictionary *dict; // dictionary for all interpolated LMs
		
		void set_weight(const topic_map_t& map, double_vec_t& weight);

	public:
		
		lmInterpolation(float nlf=0.0, float dlfi=0.0);
		virtual ~lmInterpolation() {}
		
		virtual void load(const std::string &filename,int mmap=0);
		lmContainer* load_lm(int i, int memmap, float nlf, float dlf);
		
		virtual double clprob(ngram ng, double* bow, int* bol, ngram_state_t* maxsuffidx, char** maxsuffptr, unsigned int* statesize, bool* extendible, double* lastbow);
		virtual double clprob(ngram ng, topic_map_t& lm_weights, double* bow=NULL, int* bol=NULL, ngram_state_t* maxsuffidx=NULL, char** maxsuffptr=NULL, unsigned int* statesize=NULL,bool* extendible=NULL, double* lastbow=NULL);
		
		virtual double clprob(string_vec_t& text, double* bow, int* bol, ngram_state_t* maxsuffidx, char** maxsuffptr, unsigned int* statesize, bool* extendible, double* lastbow)
		{
			VERBOSE(2,"lmInterpolation::clprob(string_vec_t& text, ...)" << std::endl);
			
			//create the actual ngram
			ngram ng(dict);
			ng.pushw(text);
			VERBOSE(3,"ng:|" << ng << "|" << std::endl);		
			for (size_t i=0; i<m_number_lm; i++) {
				VERBOSE(2,"this:|" << (void*) this << "| i:" << i << " m_weight[i]:" << m_weight[i] << endl);
			}
			MY_ASSERT (ng.size == (int) text.size());
			return clprob(ng, bow, bol, maxsuffidx, maxsuffptr, statesize, extendible, lastbow);
		}
		
		virtual double clprob(string_vec_t& text, topic_map_t& lm_weights, double* bow, int* bol, ngram_state_t* maxsuffidx, char** maxsuffptr, unsigned int* statesize, bool* extendible, double* lastbow)
		{
			VERBOSE(2,"lmInterpolation::clprob(string_vec_t& text, topic_map_t& lm_weights, ...)" << std::endl);
			
			//create the actual ngram
			ngram ng(dict);
			ng.pushw(text);
			VERBOSE(3,"ng:|" << ng << "|" << std::endl);		
			
			MY_ASSERT (ng.size == (int) text.size());
			return clprob(ng, lm_weights, bow, bol, maxsuffidx, maxsuffptr, statesize, extendible, lastbow);
		}
		
		virtual const char *cmaxsuffptr(ngram ong, unsigned int* size=NULL);
		virtual ngram_state_t cmaxsuffidx(ngram ong, unsigned int* size=NULL);
		
		int maxlevel() const {
			return maxlev;
		}
		
		virtual inline void setDict(dictionary* d) {
			if (dict) delete dict;
			dict=d;
		}
		
		virtual inline dictionary* getDict() const {
			return dict;
		}
		
		//set penalty for OOV words
		virtual inline double getlogOOVpenalty() const {
			return logOOVpenalty;
		}
		
		virtual double setlogOOVpenalty(int dub);
		
		double inline setlogOOVpenalty(double oovp) {
			return logOOVpenalty=oovp;
		}
		
		//set the inverted flag (used to set the inverted flag of each subLM, when loading)
		inline bool is_inverted(const bool flag) {
			return isInverted = flag;
		}
		
		//for an interpolation LM this variable does not make sense
		//for compatibility, we return true if all subLM return true
		inline bool is_inverted() {
			for (size_t i=0; i<m_number_lm; i++) {
				if (m_isinverted[i] == false) return false;
			}
			return true;
		}
		
		inline virtual void dictionary_incflag(const bool flag) {
			dict->incflag(flag);
		}
		
		inline virtual bool is_OOV(int code) { //returns true if the word is OOV for each subLM
			for (size_t i=0; i<m_number_lm; i++) {
				int _code=m_lm[i]->getDict()->encode(getDict()->decode(code));
				if (m_lm[i]->is_OOV(_code) == false) return false;
			}
			return true;
		}
		
		virtual int addWord(const char *w){
			for (size_t i=0; i<m_number_lm; i++) {
				m_lm[i]->getDict()->incflag(1);
				m_lm[i]->getDict()->encode(w);
				m_lm[i]->getDict()->incflag(0);
			}
			getDict()->incflag(1);
			int c=getDict()->encode(w);
			getDict()->incflag(0);
			return c;
		}
		
		virtual inline int get(ngram& ng) {
			return get(ng,ng.size,ng.size);
		}
		virtual int get(ngram& ng,int n,int lev);

		/* returns into the dictionary the successors of the given ngram */
		virtual void getSuccDict(ngram& ng,dictionary* d);
		
	};
}//namespace irstlm

#endif

