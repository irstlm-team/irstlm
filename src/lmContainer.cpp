// $Id: lmContainer.cpp 3686 2010-10-15 11:55:32Z bertoldi $

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
#include <sstream>
#include "util.h"
#include "lmContainer.h"
#include "lmtable.h"
#include "lmmacro.h"
#include "lmclass.h"
#include "lmInterpolation.h"
#include "lmContextDependent.h"

using namespace std;

namespace irstlm {
	
#ifdef PS_CACHE_ENABLE
#if PS_CACHE_ENABLE==0
#undef PS_CACHE_ENABLE
#endif
#endif
	
#ifdef LMT_CACHE_ENABLE
#if LMT_CACHE_ENABLE==0
#undef LMT_CACHE_ENABLE
#endif
#endif
	
#if PS_CACHE_ENABLE
	bool lmContainer::ps_cache_enabled=true;
#else
	bool lmContainer::ps_cache_enabled=false;
#endif
	
#if LMT_CACHE_ENABLE
	bool lmContainer::lmt_cache_enabled=true;
#else
	bool lmContainer::lmt_cache_enabled=false;
#endif
	
	inline void error(const char* message)
	{
		std::cerr << message << "\n";
		throw std::runtime_error(message);
	}
	
	lmContainer::lmContainer()
	{
		requiredMaxlev=IRSTLM_REQUIREDMAXLEV_DEFAULT;
		lmtype=_IRSTLM_LMUNKNOWN;
		maxlev=0;
	}
	
	int lmContainer::getLanguageModelType(std::string filename)
	{
		fstream inp(filename.c_str(),ios::in|ios::binary);
		
		if (!inp.good()) {
			std::stringstream ss_msg;
			ss_msg << "Failed to open " << filename;
			exit_error(IRSTLM_ERROR_IO, ss_msg.str());
		}
		//give a look at the header to get informed about the language model type
		std::string header;
		inp >> header;
		inp.close();
		
		VERBOSE(1,"LM header:|" << header << "|" << std::endl);
		
		int type=_IRSTLM_LMUNKNOWN;
		VERBOSE(1,"type: " << type << std::endl);
		if (header == "lmminterpolation" || header == "LMINTERPOLATION") {
			type = _IRSTLM_LMINTERPOLATION;
		} else if (header == "lmcontextdependent" || header == "LMCONTEXTDEPENDENT") {
			type = _IRSTLM_LMCONTEXTDEPENDENT;
		} else if (header == "lmmacro" || header == "LMMACRO") {
			type = _IRSTLM_LMMACRO;
		} else if (header == "lmclass" || header == "LMCLASS") {
			type = _IRSTLM_LMCLASS;
		} else {
			type = _IRSTLM_LMTABLE;
		}
		VERBOSE(1,"type: " << type << std::endl);
		
		return type;
	};
	
	lmContainer* lmContainer::CreateLanguageModel(const std::string infile, float nlf, float dlf)
	{
		int type = lmContainer::getLanguageModelType(infile);
		
		VERBOSE(1,"lmContainer* lmContainer::CreateLanguageModel(...) Language Model Type of " << infile << " is " << type << std::endl);
		
		return lmContainer::CreateLanguageModel(type, nlf, dlf);
	}
	
	lmContainer* lmContainer::CreateLanguageModel(int type, float nlf, float dlf)
	{
		VERBOSE(1,"Language Model Type is " << type << std::endl);
		
		lmContainer* lm=NULL;
		
		switch (type) {
				
			case _IRSTLM_LMTABLE:
				VERBOSE(1,"_IRSTLM_LMTABLE" << std::endl);
				lm = new lmtable(nlf, dlf);
				break;
				
			case _IRSTLM_LMMACRO:
				VERBOSE(1,"_IRSTLM_LMMACRO" << std::endl);
				lm = new lmmacro(nlf, dlf);
				break;
				
			case _IRSTLM_LMCLASS:
				VERBOSE(1,"_IRSTLM_LMCLASS" << std::endl);
				lm = new lmclass(nlf, dlf);
				break;
				
			case _IRSTLM_LMINTERPOLATION:
				VERBOSE(1,"_IRSTLM_LMINTERPOLATION" << std::endl);
				lm = new lmInterpolation(nlf, dlf);
				break;

			case _IRSTLM_LMCONTEXTDEPENDENT:
				VERBOSE(1,"_IRSTLM_LMCONTEXTDEPENDENT" << std::endl);
				lm = new lmContextDependent(nlf, dlf);
				break;
				
			default:
				VERBOSE(1,"UNKNOWN" << std::endl);
				exit_error(IRSTLM_ERROR_DATA, "This language model type is unknown!");
		}
		VERBOSE(1,"lmContainer* lmContainer::CreateLanguageModel(int type, float nlf, float dlf) lm:|" << (void*) lm << "|" << std::endl);
		
		lm->setLanguageModelType(type);
		
		VERBOSE(1,"lmContainer* lmContainer::CreateLanguageModel(int type, float nlf, float dlf) lm->getLanguageModelType:|" << lm->getLanguageModelType() << "|" << std::endl)
		return lm;
	}
	
	bool lmContainer::filter(const string sfilter, lmContainer*& sublmC, const string skeepunigrams)
	{
		if (lmtype == _IRSTLM_LMTABLE) {
			sublmC = lmContainer::CreateLanguageModel(lmtype,((lmtable*) this)->GetNgramcacheLoadFactor(),((lmtable*) this)->GetDictionaryLoadFactor());
			
			//let know that table has inverted n-grams
			sublmC->is_inverted(is_inverted());
			sublmC->setMaxLoadedLevel(getMaxLoadedLevel());
			sublmC->maxlevel(maxlevel());
			
			bool res=((lmtable*) this)->filter(sfilter, (lmtable*) sublmC, skeepunigrams);
			
			return res;
		}
		return false;
	};
	
	bool lmContainer::GetSentenceAndLexicon(std::string& sentence, std::string& lexiconfile, std::string& line)
	{
		VERBOSE(2,"bool lmContextDependent::GetSentenceAndLexicon" << std::endl);
		VERBOSE(2,"line:|" << line << "|" << std::endl);
		bool ret;
		size_t pos = line.find(lexicon_delimiter);	
		if (pos != std::string::npos){ // lexicon_delimiter is found
			sentence = line.substr(0, pos);
			line.erase(0, pos + lexicon_delimiter.length());
			
			//getting context string;
			lexiconfile = line;
			ret=true;
		}else{
			sentence = line;
			lexiconfile = "";
			ret=false;
		}	
		VERBOSE(2,"sentence:|" << sentence << "|" << std::endl);	
		VERBOSE(2,"lexicon:|" << lexiconfile << "|" << std::endl);
		return ret;
	}
	bool lmContainer::GetSentenceAndContext(std::string& sentence, std::string& context, std::string& line)
	{
		VERBOSE(2,"bool lmContextDependent::GetSentenceAndContext" << std::endl);
		VERBOSE(2,"line:|" << line << "|" << std::endl);
		bool ret;
		size_t pos = line.find(context_delimiter);
		if (pos != std::string::npos){ // context_delimiter is found
			sentence = line.substr(0, pos);
			line.erase(0, pos + context_delimiter.length());
			
			//getting context string;
			context = line;
			ret=true;
		}else{
			sentence = line;
			context = "";
			ret=false;
		}
		VERBOSE(2,"sentence:|" << sentence << "|" << std::endl);
		VERBOSE(2,"context:|" << context << "|" << std::endl);
		return ret;
	}
	
	void lmContainer::setContextMap(topic_map_t& topic_map, const std::string& context){
		
		string_vec_t topic_weight_vec;
		string_vec_t topic_weight;
		
		// context is supposed in this format
		// topic-name1,topic-value1:topic-name2,topic-value2:...:topic-nameN,topic-valueN
		
		//first-level split the context in a vector of  topic-name1,topic-value1, using the first separator ':'
		split(context, topic_map_delimiter1, topic_weight_vec);
		for (string_vec_t::iterator it=topic_weight_vec.begin(); it!=topic_weight_vec.end(); ++it){
			//first-level split the context in a vector of  topic-name1 and ,topic-value1, using the second separator ','
			split(*it, topic_map_delimiter2, topic_weight);
			topic_map[topic_weight.at(0)] = strtod (topic_weight.at(1).c_str(), NULL);
			topic_weight.clear();
		}
	}
	
	
}//namespace irstlm
