/**
 * Orthanc - A Lightweight, RESTful DICOM Store
 * Copyright (C) 2012-2015 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * In addition, as a special exception, the copyright holders of this
 * program give permission to link the code of its release with the
 * OpenSSL project's "OpenSSL" library (or with modified versions of it
 * that use the same license as the "OpenSSL" library), and distribute
 * the linked executables. You must obey the GNU General Public License
 * in all respects for all of the code used other than "OpenSSL". If you
 * modify file(s) with this exception, you may extend this exception to
 * your version of the file(s), but you are not obligated to do so. If
 * you do not wish to do so, delete this exception statement from your
 * version. If you delete this exception statement from all source files
 * in the program, then also delete it here.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#include "../../OrthancServer/PrecompiledHeadersServer.h"
#include "PluginsErrorDictionary.h"

#if ORTHANC_PLUGINS_ENABLED != 1
#error The plugin support is disabled
#endif



#include "PluginsEnumerations.h"
#include "PluginsManager.h"

#include <memory>


namespace Orthanc
{
  PluginsErrorDictionary::PluginsErrorDictionary() : 
    pos_(ErrorCode_START_PLUGINS)
  {
  }


  PluginsErrorDictionary::~PluginsErrorDictionary()
  {
    for (Errors::iterator it = errors_.begin(); it != errors_.end(); ++it)
    {
      delete it->second;
    }
  }


  OrthancPluginErrorCode PluginsErrorDictionary::Register(const std::string& pluginName,
                                                          int32_t  pluginCode,
                                                          uint16_t httpStatus,
                                                          const char* description)
  {
    std::auto_ptr<Error> error(new Error);

    error->pluginName_ = pluginName;
    error->pluginCode_ = pluginCode;
    error->description_ = description;
    error->httpStatus_ = static_cast<HttpStatus>(httpStatus);

    OrthancPluginErrorCode code;

    {
      boost::mutex::scoped_lock lock(mutex_);
      errors_[pos_] = error.release();
      code = static_cast<OrthancPluginErrorCode>(pos_);
      pos_ += 1;
    }

    return code;
  }


  bool  PluginsErrorDictionary::Format(Json::Value& message,  /* out */
                                       HttpStatus& httpStatus,  /* out */
                                       const OrthancException& exception)
  {
    if (exception.GetErrorCode() >= ErrorCode_START_PLUGINS)
    {
      boost::mutex::scoped_lock lock(mutex_);
      Errors::const_iterator error = errors_.find(static_cast<int32_t>(exception.GetErrorCode()));
      
      if (error != errors_.end())
      {
        httpStatus = error->second->httpStatus_;
        message["PluginName"] = error->second->pluginName_;
        message["PluginCode"] = error->second->pluginCode_;
        message["Message"] = error->second->description_;

        return true;
      }
    }

    return false;
  }
}