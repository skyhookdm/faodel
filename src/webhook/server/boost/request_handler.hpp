//
// request_handler.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2014 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef WEBHOOK_REQUEST_HANDLER_HPP
#define WEBHOOK_REQUEST_HANDLER_HPP

#include <string>
#include <map>
#include <functional>
#include <mutex>

#include "webhook/Server.hh"

namespace http {
namespace server {

struct reply;
struct request;

//typedef int (*CallbackType)(std::string &req_string, std::string *reply_string);

/// The common handler for all incoming requests.
class request_handler
{
public:
  request_handler(const request_handler&) = delete;
  request_handler& operator=(const request_handler&) = delete;

  /// Construct with a directory containing files to be served.
  explicit request_handler();

  /// Handle a request and produce a reply.
  void handle_request(const request& req, reply& rep);

  //CDU
  int registerHook(const std::string &name, webhook::cb_web_handler_t func);
  int updateHook(const std::string &name, webhook::cb_web_handler_t func);
  int deregisterHook(const std::string &name);
  
  std::string makeHTMLHeader(const std::string &name);
  
private:

  /// Perform URL-decoding on a string. Returns false if the encoding was
  /// invalid.
  static bool url_decode(const std::string& in, std::string& out);

  std::map<std::string, webhook::cb_web_handler_t> cbs;
  std::mutex cbs_mutex;

  std::pair<std::string,std::string> splitString(const std::string &item, char delim);
  std::map<std::string,std::string> parseArgString(const std::string &args);
  
  void dumpRegisteredHandles(std::stringstream &results);
  void dumpAbout(std::stringstream &results);

};

} // namespace server
} // namespace http

#endif // WEBHOOK_REQUEST_HANDLER_HPP
