// $Id: lmInterpolation.cpp 3686 2010-10-15 11:55:32Z bertoldi $

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
#include "lmInterpolation.h"
#include "util.h"

using namespace std;

namespace irstlm {
	lmInterpolation::lmInterpolation(float nlf, float dlf)
	{
		ngramcache_load_factor = nlf;
		dictionary_load_factor = dlf;
		
		order=0;
		memmap=0;
		isInverted=false;
		m_map_flag=false;
	}
	
	void lmInterpolation::load(const std::string &filename,int mmap)
	{
		VERBOSE(2,"lmInterpolation::load(const std::string &filename,int memmap)" << std::endl);
		VERBOSE(2," filename:|" << filename << "|" << std::endl);
		
		std::stringstream ss_format;
		
		ss_format << "LMINTERPOLATION number_of_models\nweight_of_LM_1 filename_of_LM_1 [inverted]\nweight_of_LM_2 filename_of_LM_2 [inverted]\n...\n";
		ss_format << "or\nLMINTERPOLATION number_of_models MAP\nweight_of_LM_1 name_LM_1 filename_of_LM_1\nweight_of_LM_2 name_LM_2 filename_of_LM_2 [inverted]\n...\n";
		
		dictionary_upperbound=1000000;
		int memmap=mmap;
		
		
		dict=new dictionary((char *)NULL,1000000,dictionary_load_factor);
		
		//get info from the configuration file
		fstream inp(filename.c_str(),ios::in|ios::binary);
		
		char line[MAX_LINE];
		const char* words[LMINTERPOLATION_MAX_TOKEN];
		size_t tokenN;
		inp.getline(line,MAX_LINE,'\n');
		tokenN = parseWords(line,words,LMINTERPOLATION_MAX_TOKEN);
		bool error=false;
		
		if ((tokenN<2) || (tokenN>3)){
			error=true;     
		}else if ((strcmp(words[0],"LMINTERPOLATION") != 0) && (strcmp(words[0],"lminterpolation")!=0)) {
			error=true;
		}else if ((tokenN==3) && ((strcmp(words[2],"MAP") != 0) && (strcmp(words[2],"map") != 0))){
			error=true;
		}
		
		if (error){
			std::stringstream ss_msg;
			ss_msg << "ERROR: wrong header format of configuration file\ncorrect format:" << ss_format.str();
			exit_error(IRSTLM_ERROR_DATA,ss_msg.str());
		}
		
		size_t idx_weight, idx_file, idx_name, idx_inverted, idx_size;
		if (tokenN==2){
			m_map_flag=false;
			idx_weight=0;
			idx_file=1;
			idx_inverted=2;
			idx_size=3;
			m_isadaptive=false;
		}else{
			m_map_flag=true;
			idx_weight=0;
			idx_name=1;
			idx_file=2;
			idx_inverted=3;
			idx_size=4;
			m_isadaptive=true;
		}
		
		m_number_lm = atoi(words[1]);
		
		m_weight.resize(m_number_lm);
		m_file.resize(m_number_lm);
		m_isinverted.resize(m_number_lm);
		m_lm.resize(m_number_lm);
		
		VERBOSE(2,"lmInterpolation::load(const std::string &filename,int mmap) m_number_lm:"<< m_number_lm << std::endl);
		
		dict->incflag(1);
		for (size_t i=0; i<m_number_lm; i++) {
			inp.getline(line,BUFSIZ,'\n');
			tokenN = parseWords(line,words,3);
			
			if(tokenN < idx_file || tokenN > idx_size) {
				error = true;
			}
			if (error){
				std::stringstream ss_msg;
				ss_msg << "ERROR: wrong header format of configuration file\ncorrect format:" << ss_format.str();
				exit_error(IRSTLM_ERROR_DATA,ss_msg.str());
			}
			
			//check whether the (textual) LM has to be loaded as inverted
			m_isinverted[i] = false;
			if(tokenN == idx_size) {
				if (strcmp(words[idx_inverted],"inverted") == 0)
					m_isinverted[i] = true;
			}
			VERBOSE(2,"i:" << i << " m_isinverted[i]:" << m_isinverted[i] << endl);
			
			m_weight[i] = atof(words[idx_weight]);
			VERBOSE(2,"this:|" << (void*) this << "| i:" << i << " m_weight[i]:" << m_weight[i] << endl);
			if (m_map_flag){
				m_idx[words[idx_name]] = i;
				m_name[i] = words[idx_name];
				VERBOSE(2,"i:" << i << " m_idx[words[idx_name]]:|" << m_idx[words[idx_name]] << "| m_name[i]:|" << m_name[i] << "|" << endl);
				std::stringstream name;
				name << i;
				m_idx[name.str()] = i;
				m_name[i] = name.str();
				VERBOSE(2,"i:" << i << " name.str():|" << name.str() << "| m_name[i]:|" << m_name[i] << "|" << endl);
			}
			m_file[i] = words[idx_file];
			
			VERBOSE(2,"lmInterpolation::load(const std::string &filename,int mmap) i:" << i << " m_name:|"<< m_name[i] << "|" " m_file:|"<< m_file[i] << "| isadaptive:|" << m_isadaptive << "|" << std::endl);
			
			m_lm[i] = load_lm(i,memmap,ngramcache_load_factor,dictionary_load_factor);
			//set the actual value for inverted flag, which is known only after loading the lM
			m_isinverted[i] = m_lm[i]->is_inverted();
			
			
			dictionary *_dict=m_lm[i]->getDict();
			for (int j=0; j<_dict->size(); j++) {
				dict->encode(_dict->decode(j));
			}
		}
		dict->genoovcode();
		inp.close();
		
		int maxorder = 0;
		for (size_t i=0; i<m_number_lm; i++) {
			maxorder = (maxorder > m_lm[i]->maxlevel())?maxorder:m_lm[i]->maxlevel();
		}
		
		if (order == 0) {
			order = maxorder;
			VERBOSE(3, "order is not set; reset to the maximum order of LMs: " << order << std::endl);
		} else if (order > maxorder) {
			order = maxorder;
			VERBOSE(3, "order is too high; reset to the maximum order of LMs: " << order << std::endl);
		}
		maxlev=order;
	}
	
	lmContainer* lmInterpolation::load_lm(int i,int memmap, float nlf, float dlf)
	{
		//checking the language model type
		lmContainer* lmt=lmContainer::CreateLanguageModel(m_file[i],nlf,dlf);
		
		//let know that table has inverted n-grams
		lmt->is_inverted(m_isinverted[i]);  //set inverted flag for each LM
		
		lmt->setMaxLoadedLevel(requiredMaxlev);
		
		lmt->load(m_file[i], memmap);
		
		lmt->init_caches(lmt->maxlevel());
		return lmt;
	}
	
	//return log10 prob of an ngram
	double lmInterpolation::clprob(ngram ng, topic_map_t& lm_weights, double* bow, int* bol, ngram_state_t* maxsuffidx, char** maxsuffptr, unsigned int* statesize, bool* extendible, double* lastbow)
	{
		VERBOSE(1,"double lmInterpolation::clprob(ngram ng, topic_map_t& lm_weights,...)"  << std::endl);
		
		double pr=0.0;
		double _logpr;
		
		char* _maxsuffptr=NULL,*actualmaxsuffptr=NULL;
		ngram_state_t _maxsuffidx=0,actualmaxsuffidx=0;
		unsigned int _statesize=0,actualstatesize=0;
		int _bol=0,actualbol=MAX_NGRAM;
		double _bow=0.0,actualbow=0.0; 
		double _lastbow=0.0,actuallastbow=0.0; 
		bool _extendible=false,actualextendible=false;
		
		double_vec_t weight(m_number_lm);
		set_weight(lm_weights,weight);
		
		for (size_t i=0; i<m_number_lm; i++) {
			if (weight[i]>0.0){
				ngram _ng(m_lm[i]->getDict());
				_ng.trans(ng);
				_logpr=m_lm[i]->clprob(_ng,&_bow,&_bol,&_maxsuffptr,&_statesize,&_extendible,&_lastbow);
				
				IFVERBOSE(3){
					//cerr.precision(10);
					VERBOSE(3," LM " << i << " weight:" << weight[i] << std::endl);
					VERBOSE(3," LM " << i << " log10 logpr:" << _logpr<< std::endl);
					VERBOSE(3," LM " << i << " pr:" << pow(10.0,_logpr) << std::endl);
					VERBOSE(3," _statesize:" << _statesize << std::endl);
					VERBOSE(3," _bow:" << _bow << std::endl);
					VERBOSE(3," _bol:" << _bol << std::endl);
					VERBOSE(3," _lastbow:" << _lastbow << std::endl);
				}
				
				/*
				 //TO CHECK the following claims
				 //What is the statesize of a LM interpolation? The largest _statesize among the submodels
				 //What is the maxsuffptr of a LM interpolation? The _maxsuffptr of the submodel with the largest _statesize
				 //What is the bol of a LM interpolation? The smallest _bol among the submodels
				 //What is the bow of a LM interpolation? The weighted sum of the bow of the submodels
				 //What is the prob of a LM interpolation? The weighted sum of the prob of the submodels
				 //What is the extendible flag of a LM interpolation? true if the extendible flag is one for any LM
				 //What is the lastbow of a LM interpolation? The weighted sum of the lastbow of the submodels
				 */
				
				pr+=weight[i]*pow(10.0,_logpr);
				actualbow+=weight[i]*pow(10.0,_bow);
				
				if(_statesize > actualstatesize || i == 0) {
					actualmaxsuffptr = _maxsuffptr;
					actualmaxsuffidx = _maxsuffidx;
					actualstatesize = _statesize;
				}
				if (_bol < actualbol) {
					actualbol=_bol; //backoff limit of LM[i]
				}
				if (_extendible) {
					actualextendible=true; //set extendible flag to true if the ngram is extendible for any LM
				}
				if (_lastbow < actuallastbow) {
					actuallastbow=_lastbow; //backoff limit of LM[i]
				}
			}
			else{
				VERBOSE(3," LM " << i << " weight is zero" << std::endl);
			}
		}
		if (bol) *bol=actualbol;
		if (bow) *bow=log(actualbow);
		if (maxsuffptr) *maxsuffptr=actualmaxsuffptr;
		if (maxsuffidx) *maxsuffidx=actualmaxsuffidx;
		if (statesize) *statesize=actualstatesize;
		if (extendible) *extendible=actualextendible;
		if (lastbow) *bol=actuallastbow;
		
		if (statesize) VERBOSE(3, " statesize:" << *statesize << std::endl);
		if (bow) VERBOSE(3, " bow:" << *bow << std::endl);
		if (bol) VERBOSE(3, " bol:" << *bol << std::endl);
		if (lastbow) VERBOSE(3, " lastbow:" << *lastbow << std::endl);
		
		return log10(pr);
	}
	
	//return log10 prob of an ngram
	double lmInterpolation::clprob(ngram ng, double* bow,int* bol, ngram_state_t* maxsuffidx, char** maxsuffptr, unsigned int* statesize, bool* extendible, double* lastbow)
	{
		VERBOSE(1,"double lmInterpolation::clprob(ngram ng, ...)"  << std::endl);
		
		double pr=0.0;
		double _logpr;
		
		char* _maxsuffptr=NULL,*actualmaxsuffptr=NULL;
		ngram_state_t _maxsuffidx=0,actualmaxsuffidx=0;
		unsigned int _statesize=0,actualstatesize=0;
		int _bol=0,actualbol=MAX_NGRAM;
		double _bow=0.0,actualbow=0.0; 
		double _lastbow=0.0,actuallastbow=0.0; 
		bool _extendible=false,actualextendible=false;
		
		for (size_t i=0; i<m_number_lm; i++) {
			VERBOSE(2,"this:|" << (void*) this << "| i:" << i << " m_weight[i]:" << m_weight[i] << endl);
		}
		for (size_t i=0; i<m_number_lm; i++) {
			
			if (m_weight[i]>0.0){
				ngram _ng(m_lm[i]->getDict());
				_ng.trans(ng);			
				_logpr=m_lm[i]->clprob(_ng,&_bow,&_bol,&_maxsuffidx,&_maxsuffptr,&_statesize,&_extendible,&_lastbow);
				
				IFVERBOSE(3){
					//cerr.precision(10);
					VERBOSE(3," LM " << i << " weight:" << m_weight[i] << std::endl);
					VERBOSE(3," LM " << i << " log10 logpr:" << _logpr<< std::endl);
					VERBOSE(3," LM " << i << " pr:" << pow(10.0,_logpr) << std::endl);
					VERBOSE(3," LM " << i << " msp:" << (void*) _maxsuffptr << std::endl);
					VERBOSE(3," LM " << i << " msidx:" << _maxsuffidx << std::endl);
					VERBOSE(3," LM " << i << " statesize:" << _statesize << std::endl);
					VERBOSE(3," LM " << i << " bow:" << _bow << std::endl);
					VERBOSE(3," LM " << i << " bol:" << _bol << std::endl);
					VERBOSE(3," LM " << i << " lastbow:" << _lastbow << std::endl);
				}
				
				/*
				 //TO CHECK the following claims
				 //What is the statesize of a LM interpolation? The largest _statesize among the submodels
				 //What is the maxsuffptr of a LM interpolation? The _maxsuffptr of the submodel with the largest _statesize
				 //What is the bol of a LM interpolation? The smallest _bol among the submodels
				 //What is the bow of a LM interpolation? The weighted sum of the bow of the submodels
				 //What is the prob of a LM interpolation? The weighted sum of the prob of the submodels
				 //What is the extendible flag of a LM interpolation? true if the extendible flag is one for any LM
				 //What is the lastbow of a LM interpolation? The weighted sum of the lastbow of the submodels
				 */
				
				pr+=m_weight[i]*pow(10.0,_logpr);
				actualbow+=m_weight[i]*pow(10.0,_bow);
				
				if(_statesize > actualstatesize || i == 0) {
					actualmaxsuffptr = _maxsuffptr;
					actualmaxsuffidx = _maxsuffidx;
					actualstatesize = _statesize;
				}
				if (_bol < actualbol) {
					actualbol=_bol; //backoff limit of LM[i]
				}
				if (_extendible) {
					actualextendible=true; //set extendible flag to true if the ngram is extendible for any LM
				}
				if (_lastbow < actuallastbow) {
					actuallastbow=_lastbow; //backoff limit of LM[i]
				}
			}
		}
		if (bol) *bol=actualbol;
		if (bow) *bow=log(actualbow);
		if (maxsuffptr) *maxsuffptr=actualmaxsuffptr;
		if (maxsuffidx) *maxsuffidx=actualmaxsuffidx;
		if (statesize) *statesize=actualstatesize;
		if (extendible) *extendible=actualextendible;
		if (lastbow) *bol=actuallastbow;
		
		if (maxsuffptr) VERBOSE(3, " msp:" << (void*) *maxsuffptr << std::endl);
		if (maxsuffidx) VERBOSE(3, " msidx:" << *maxsuffidx << std::endl);
		if (statesize) VERBOSE(3, " statesize:" << *statesize << std::endl);
		if (bow) VERBOSE(3, " bow:" << *bow << std::endl);
		if (bol) VERBOSE(3, " bol:" << *bol << std::endl);
		if (lastbow) VERBOSE(3, " lastbow:" << *lastbow << std::endl);
		
		return log10(pr);
	}
	
	const char *lmInterpolation::cmaxsuffptr(ngram ng, unsigned int* statesize)
	{
		
		char *maxsuffptr=NULL;
		unsigned int _statesize=0,actualstatesize=0;
		
		for (size_t i=0; i<m_number_lm; i++) {
			
			if (m_weight[i]>0.0){
				ngram _ng(m_lm[i]->getDict());
				_ng.trans(ng);
				
				const char* _maxsuffptr = m_lm[i]->cmaxsuffptr(_ng,&_statesize);
				
				IFVERBOSE(3){
					//cerr.precision(10);
					VERBOSE(3," LM " << i << " weight:" << m_weight[i] << std::endl);
					VERBOSE(3," _statesize:" << _statesize << std::endl);
				}
				
				/*
				 //TO CHECK the following claims
				 //What is the statesize of a LM interpolation? The largest _statesize among the submodels
				 //What is the maxsuffptr of a LM interpolation? The _maxsuffptr of the submodel with the largest _statesize
				 */
				
				if(_statesize > actualstatesize || i == 0) {
					maxsuffptr = (char*) _maxsuffptr;
					actualstatesize = _statesize;
				}
			}
		}
		if (statesize) *statesize=actualstatesize;
		
		if (statesize) VERBOSE(3, " statesize:" << *statesize << std::endl);
		
		return maxsuffptr;
	}
	
	ngram_state_t lmInterpolation::cmaxsuffidx(ngram ng, unsigned int* statesize)
	{
		ngram_state_t maxsuffidx=0;
		unsigned int _statesize=0,actualstatesize=0;
		
		for (size_t i=0; i<m_number_lm; i++) {
			
			if (m_weight[i]>0.0){
				ngram _ng(m_lm[i]->getDict());
				_ng.trans(ng);
				
				ngram_state_t _maxsuffidx = m_lm[i]->cmaxsuffidx(_ng,&_statesize);
				
				IFVERBOSE(3){
					//cerr.precision(10);
					VERBOSE(3," LM " << i << " weight:" << m_weight[i] << std::endl);
					VERBOSE(3," _statesize:" << _statesize << std::endl);
				}
				
				/*
				 //TO CHECK the following claims
				 //What is the statesize of a LM interpolation? The largest _statesize among the submodels
				 //What is the maxsuffptr of a LM interpolation? The _maxsuffptr of the submodel with the largest _statesize
				 */
				
				if(_statesize > actualstatesize || i == 0) {
					maxsuffidx = _maxsuffidx;
					actualstatesize = _statesize;
				}
			}
		}
		
	  if (statesize) *statesize=actualstatesize;
		
		if (statesize) VERBOSE(3, " statesize:" << *statesize << std::endl);
		
		return maxsuffidx;
	}
	
	double lmInterpolation::setlogOOVpenalty(int dub)
	{
		MY_ASSERT(dub > dict->size());
		double _logpr;
		double OOVpenalty=0.0;
		for (size_t i=0; i<m_number_lm; i++) {
			if (m_weight[i]>0.0){
				m_lm[i]->setlogOOVpenalty(dub);  //set OOV Penalty for each LM
				_logpr=m_lm[i]->getlogOOVpenalty(); // logOOV penalty is in log10
				//    OOVpenalty+=m_weight[i]*exp(_logpr);
				OOVpenalty+=m_weight[i]*exp(_logpr*M_LN10);  // logOOV penalty is in log10
			}
		}
		//  logOOVpenalty=log(OOVpenalty);
		logOOVpenalty=log10(OOVpenalty);
		return logOOVpenalty;
	}
	
	
	void lmInterpolation::set_weight(const topic_map_t& map, double_vec_t& weight){
		VERBOSE(4,"void lmInterpolation::set_weight" << std::endl);
		VERBOSE(4,"map.size:" << map.size() << std::endl);
		for (topic_map_t::const_iterator it=map.begin(); it!=map.end();++it){
			if (m_idx.find(it->first) == m_idx.end()){
				exit_error(IRSTLM_ERROR_DATA, "void lmInterpolation::set_weight(const topic_map_t& map, double_vec_t& weight) ERROR: you are setting the weight of a LM which is not included in the interpolated LM");
			}
			weight[m_idx[it->first]] = it->second;                  
			VERBOSE(4,"it->first:|" << it->first << "| it->second:|" << it->second << "| m_idx[it->first]:|" << m_idx[it->first] << "| weight[m_idx[it->first]]:|" <<weight[m_idx[it->first]] << "|" << std::endl);
		}
	}
	int lmInterpolation::get(ngram& ng,int n,int lev)
	{
		/*The function get for the lmInterpolation  LM type is not well defined
		 The chosen implementation is the following:
		 - for each submodel with weight larger than 0.0,
		 -- an ngram is created with the submodel dictionary using the main ngram (of lmInterpolation)
		 -- the submodel-specific ngram is searched in the corresponding submodel
		 - the main ngram is considered found if any submodel-specific ngram is found
		 - the amount of successors of the main ngram are set to the maximum among the amount of successors of the submodel-specifc ngrams
		 Note: an other option could be that of setting he amount of successors of the main ngram to the sum of the amount of successors of the submodel-specifc ngram; but we did not implemented it.
		 */
		int ret = 0;
		int succ = 0;
		for (size_t i=0; i<m_number_lm; i++) {
			if (m_weight[i]>0.0){
				ngram _ng(m_lm[i]->getDict());
				_ng.trans(ng);
				if (m_lm[i]->get(_ng, n, lev)){
					ret = 1;
					succ = (_ng.succ>succ)?_ng.succ:succ;
				}
			}
		}
		ng.succ=succ;
		return ret;
	}
	
	/* returns into the dictionary the successors of the given ngram;
	 it collects the successors from all submodels with weights larger than 0.0
	 */
	void lmInterpolation::getSuccDict(ngram& ng,dictionary* d){
		for (size_t i=0; i<m_number_lm; i++) {
			if (m_weight[i]>0.0){
				ngram _ng(m_lm[i]->getDict());
				_ng.trans(ng);
				m_lm[i]->getSuccDict(_ng,d);
			}			
		}
	}
	
}//namespace irstlm

