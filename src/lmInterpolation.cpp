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
#include <stdexcept>
#include <string>
#include "lmContainer.h"
#include "lmInterpolation.h"
#include "util.h"

using namespace std;
	
inline void error(const char* message)
{
  std::cerr << message << "\n";
  throw std::runtime_error(message);
}

namespace irstlm {
lmInterpolation::lmInterpolation(float nlf, float dlf)
{
  ngramcache_load_factor = nlf;
  dictionary_load_factor = dlf;
	
  order=0;
  memmap=0;
  isInverted=false;
}

void lmInterpolation::load(const std::string &filename,int mmap)
{
  VERBOSE(2,"lmInterpolation::load(const std::string &filename,int memmap)" << std::endl);
  VERBOSE(2," filename:|" << filename << "|" << std::endl);
	
	
  dictionary_upperbound=1000000;
  int memmap=mmap;
	
	
  dict=new dictionary((char *)NULL,1000000,dictionary_load_factor);
	
  //get info from the configuration file
  fstream inp(filename.c_str(),ios::in|ios::binary);
	
  char line[MAX_LINE];
  const char* words[LMINTERPOLATION_MAX_TOKEN];
  int tokenN;
  inp.getline(line,MAX_LINE,'\n');
  tokenN = parseWords(line,words,LMINTERPOLATION_MAX_TOKEN);
	
  if (tokenN != 2 || ((strcmp(words[0],"LMINTERPOLATION") != 0) && (strcmp(words[0],"lminterpolation")!=0)))
    error((char*)"ERROR: wrong header format of configuration file\ncorrect format: LMINTERPOLATION number_of_models\nweight_of_LM_1 filename_of_LM_1\nweight_of_LM_2 filename_of_LM_2");
	
  m_number_lm = atoi(words[1]);
	
  m_weight.resize(m_number_lm);
  m_file.resize(m_number_lm);
  m_isinverted.resize(m_number_lm);
  m_lm.resize(m_number_lm);
	
  VERBOSE(2,"lmInterpolation::load(const std::string &filename,int mmap) m_number_lm:"<< m_number_lm << std::endl;);
	
  dict->incflag(1);
  for (int i=0; i<m_number_lm; i++) {
    inp.getline(line,BUFSIZ,'\n');
    tokenN = parseWords(line,words,3);
		
    if(tokenN < 2 || tokenN >3) {
      error((char*)"ERROR: wrong header format of configuration file\ncorrect format: LMINTERPOLATION number_of_models\nweight_of_LM_1 filename_of_LM_1\nweight_of_LM_2 filename_of_LM_2");
    }
		
		//check whether the (textual) LM has to be loaded as inverted
    m_isinverted[i] = false;
    if(tokenN == 3) {
      if (strcmp(words[2],"inverted") == 0)
        m_isinverted[i] = true;
    }
    VERBOSE(2,"i:" << i << " m_isinverted[i]:" << m_isinverted[i] << endl);
		
    m_weight[i] = (float) atof(words[0]);
    m_file[i] = words[1];
    VERBOSE(2,"lmInterpolation::load(const std::string &filename,int mmap) m_file:"<< words[1] << std::endl;);
		
    m_lm[i] = load_lm(i,memmap,ngramcache_load_factor,dictionary_load_factor);
		//set the actual value for inverted flag, which is known only after loading the lM
    m_isinverted[i] = m_lm[i]->is_inverted();
		
    dictionary *_dict=m_lm[i]->getDict();
    for (int j=0; j<_dict->size(); j++) {
      dict->encode(_dict->decode(j));
    }
  }
  getDict()->genoovcode();
	
  getDict()->incflag(1);
  inp.close();
	
  int maxorder = 0;
  for (int i=0; i<m_number_lm; i++) {
    maxorder = (maxorder > m_lm[i]->maxlevel())?maxorder:m_lm[i]->maxlevel();
  }
	
  if (order == 0) {
    order = maxorder;
    std::cerr << "order is not set; reset to the maximum order of LMs: " << order << std::endl;
  } else if (order > maxorder) {
    order = maxorder;
    std::cerr << "order is too high; reset to the maximum order of LMs: " << order << std::endl;
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


double lmInterpolation::clprob(ngram ng, double* bow,int* bol,char** maxsuffptr,unsigned int* statesize,bool* extendible)
{
	
  double pr=0.0;
  double _logpr;
	
  char* _maxsuffptr=NULL,*actualmaxsuffptr=NULL;
  unsigned int _statesize=0,actualstatesize=0;
  int _bol=0,actualbol=MAX_NGRAM;
  double _bow=0.0,actualbow=0.0; 
	bool _extendible=false;
  bool actualextendible=false;
	
  for (size_t i=0; i<m_lm.size(); i++) {
		
    ngram _ng(m_lm[i]->getDict());
    _ng.trans(ng);
    _logpr=m_lm[i]->clprob(_ng,&_bow,&_bol,&_maxsuffptr,&_statesize,&_extendible);
		
    /*
		 cerr.precision(10);
		 std::cerr << " LM " << i << " weight:" << m_weight[i] << std::endl;
		 std::cerr << " LM " << i << " log10 logpr:" << _logpr<< std::endl;
		 std::cerr << " LM " << i << " pr:" << pow(10.0,_logpr) << std::endl;
		 std::cerr << " _statesize:" << _statesize << std::endl;
		 std::cerr << " _bow:" << _bow << std::endl;
		 std::cerr << " _bol:" << _bol << std::endl;
		 */
		
    //TO CHECK the following claims
    //What is the statesize of a LM interpolation? The largest _statesize among the submodels
    //What is the maxsuffptr of a LM interpolation? The _maxsuffptr of the submodel with the largest _statesize
    //What is the bol of a LM interpolation? The smallest _bol among the submodels
    //What is the bow of a LM interpolation? The weighted sum of the bow of the submodels
    //What is the prob of a LM interpolation? The weighted sum of the prob of the submodels
    //What is the extendible flag of a LM interpolation? true if the extendible flag is one for any LM
		
    pr+=m_weight[i]*pow(10.0,_logpr);
    actualbow+=m_weight[i]*pow(10.0,_bow);
		
    if(_statesize > actualstatesize || i == 0) {
      actualmaxsuffptr = _maxsuffptr;
      actualstatesize = _statesize;
    }
    if (_bol < actualbol) {
      actualbol=_bol; //backoff limit of LM[i]
    }
    if (_extendible) {
      actualextendible=true; //set extendible flag to true if the ngram is extendible for any LM
    }
  }
  if (bol) *bol=actualbol;
  if (bow) *bow=log(actualbow);
  if (maxsuffptr) *maxsuffptr=actualmaxsuffptr;
  if (statesize) *statesize=actualstatesize;
  if (extendible) {
    *extendible=actualextendible;
		//    delete _extendible;
  }
	
  /*
	 if (statesize) std::cerr << " statesize:" << *statesize << std::endl;
	 if (bow) std::cerr << " bow:" << *bow << std::endl;
	 if (bol) std::cerr << " bol:" << *bol << std::endl;
	 */
  return log(pr)/M_LN10;
}

double lmInterpolation::clprob(int* codes, int sz, double* bow,int* bol,char** maxsuffptr,unsigned int* statesize,bool* extendible)
{
	
  //create the actual ngram
  ngram ong(dict);
  ong.pushc(codes,sz);
  MY_ASSERT (ong.size == sz);
	
  return clprob(ong, bow, bol, maxsuffptr, statesize, extendible);
}

double lmInterpolation::setlogOOVpenalty(int dub)
{
  MY_ASSERT(dub > dict->size());
  double _logpr;
  double OOVpenalty=0.0;
  for (int i=0; i<m_number_lm; i++) {
    m_lm[i]->setlogOOVpenalty(dub);  //set OOV Penalty for each LM
    _logpr=m_lm[i]->getlogOOVpenalty();
    OOVpenalty+=m_weight[i]*exp(_logpr);
  }
  logOOVpenalty=log(OOVpenalty);
  return logOOVpenalty;
}
}//namespace irstlm
