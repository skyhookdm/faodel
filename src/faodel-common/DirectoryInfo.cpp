// Copyright 2018 National Technology & Engineering Solutions of Sandia, 
// LLC (NTESS). Under the terms of Contract DE-NA0003525 with NTESS,  
// the U.S. Government retains certain rights in this software. 

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include "faodel-common/StringHelpers.hh"
#include "faodel-common/DirectoryInfo.hh"

using namespace std;
using namespace faodel;

namespace faodel {


/**
 * @brief Configuration sometimes holds all info about a dirinfo in a url. Unpack it and turn into a direntry
 * @param new_url In addition to normal resources, contains "num=" (childredn) and "ag0=0x123" for child 0's
 */
DirectoryInfo::DirectoryInfo(faodel::ResourceURL new_url) {
  info = ExpandPunycode(new_url.GetOption("info"));
  new_url.RemoveOption("info");

  string s = new_url.GetOption("num");
  if(s!="") {
    new_url.RemoveOption("num");
    int64_t num_children;
    int rc = faodel::StringToInt64(&num_children, s);
    if(rc==0) {
      for(int64_t i=0; i<num_children; i++) {
        string name="ag"+std::to_string(i);
        string s_node=new_url.GetOption(name);
        if(s_node!=""){
          children.push_back( NameAndNode(name, nodeid_t(s_node)));
          new_url.RemoveOption(name);
        }
      }
    }
  }
  url=new_url; //Stripped of all imported details
}

/**
 * @brief True when DirectoryInfo does not contain any fields that are set (eg when "no info available" is returned)
 * @retval TRUE When url is empty, info is "", and no children
 * @retval FALSE When any field is set to a value
 */
bool DirectoryInfo::IsEmpty() const {
  return (url.IsEmpty()) && (info.empty()) && (children.size()==0);
}

bool DirectoryInfo::GetChildReferenceNode(const std::string &child_name, nodeid_t *reference_node) const {

  for(auto &name_node : children){
    if(name_node.name == child_name){
      if(reference_node) *reference_node = name_node.node;
      return true;
    }
  }
  if(reference_node) *reference_node = NODE_UNSPECIFIED;
  return false;
}
bool DirectoryInfo::GetChildNameByReferenceNode(faodel::nodeid_t reference_node, string *child_name) const {
  for(auto &name_node : children){
    if(name_node.node == reference_node){
      if(child_name) *child_name = name_node.name;
      return true;
    }
  }
  if(child_name) *child_name = "";
  return false;
}
/**
 * @brief Have a node join the member of a directory under a particular name
 * @param[in] node The node associated with the directory
 * @param[in] reference_name The name to be used with the node
 * @retval TRUE Node was added to the list
 * @retval FALSE Node w/ Name was already in use in the list and therefore not added
 */
bool DirectoryInfo::Join(nodeid_t node, const std::string &reference_name){

  string new_name;
  if(!reference_name.empty()){
    for(auto &child : children) {
      if(child.name == reference_name) return false; //already a named item, bail. TODO: More dynamic handling
    }
    new_name=reference_name;

  } else {
    //No name given. Generate one.
    int i=children.size(); //Jump ahead to make a better guess
    bool bad_name;
    do {
      //Make a new guess for a name
      bad_name=false;
      stringstream ss;
      ss<<"ag"<<hex<<i++;
      new_name=ss.str();

      //See if any other nodes are using this name
      for(size_t j=0; (!bad_name) && (j<children.size()); j++){
        if(children[j].name == new_name) {
          bad_name=true;
          break;
        }
      }
    } while(bad_name);
  }
  children.emplace_back(new_name, node);
  return true;
}

/**
 * @brief Remove an entry from directory based on its url. Uses URL's name then nodeid to locate entry
 * @param child_url - URL to the child resource that will be removed
 * @retval TRUE Entry was found and removed
 * @retval FALSE Entry was not found
 */
bool DirectoryInfo::Leave(const faodel::ResourceURL &child_url) {
  bool found = LeaveByName(child_url.name);
  return found || LeaveByNode(child_url.reference_node);
}

/**
 * @brief Remove an entry from a directory based on its node id (note: only removes first match)
 * @param node -  Node to remove
 * @retval TRUE Entry was found and removed
 * @retval FALSE Entry was not found
 */
bool DirectoryInfo::LeaveByNode(nodeid_t node){
  if (node==faodel::NODE_UNSPECIFIED) return false;
  for(size_t i=0; i<children.size(); i++){
    if(children[i].node == node){
      children.erase(children.begin()+(int)i);
      return true;
    }
  }
  return false;
}

/**
 * @brief Remove an entry from a directory based on its name (note: empty name is ignored)
 * @param name - Name of entry to remove
 * @retval TRUE Entry was found and removed
 * @retval FALSE Entry was empty or not found
 */
bool DirectoryInfo::LeaveByName(const std::string &name){
  if(name.empty()) return false; //Can't remove empty name
  for(size_t i=0; i<children.size(); i++){
    if(children[i].name == name){
      children.erase(children.begin()+i);
      return true;
    }
  }
  return false;
}

/**
 * @brief Determine if a node is in this entry's list of children
 * @param[in] node The node to look for
 * @retval TRUE Node was found
 * @retval FALSE Node was not found
 */
bool DirectoryInfo::ContainsNode(faodel::nodeid_t node) const {
  for(size_t i=0; i<children.size(); i++) {
    if(children[i].node == node) return true;
  }
  return false;
}

//Provide concise one-liner for the dir.
//NOTE: This is NOT for serialization
std::string DirectoryInfo::to_string() const {
  stringstream ss;
  ss<<url.GetFullURL()
    <<"&info="<<info
    <<"&num="<<children.size();
  for(auto &name_node : children){
    ss<<"&"<<name_node.name
      <<"="<<name_node.node.GetHex();
  }
  return ss.str();
}
void DirectoryInfo::webhookInfo(faodel::ReplyStream &rs){

  rs.mkSection("DirectoryInfo: "+url.GetBucketPathName());

  rs.tableBegin("Info");
  rs.tableTop({"Parameter","Setting"});
  rs.tableRow({"Path/Name:", url.GetPathName()});
  rs.tableRow({"Type:", url.resource_type});
  rs.tableRow({"Info:",info});
  rs.tableRow({"Reference Node:",url.reference_node.GetHtmlLink()});
  rs.tableRow({"Children:", std::to_string(children.size())});
  rs.tableRow({"URL:", url.GetFullURL()});
  rs.tableEnd();

  rs.tableBegin("Children");
  rs.tableTop({"NodeName","ReferenceNode"});
  for(auto &name_node : children){
    rs.tableRow({name_node.name,
          name_node.node.GetHtmlLink()});
  }
  rs.tableEnd();

}


void DirectoryInfo::sstr(std::stringstream &ss, int depth, int indent) const {
  ss<<std::string(indent,' ')
    <<"DirectoryInfo:\t"  << url.GetFullURL() //full_name
    <<" Info: '"     << info
    <<"' NumChildren: " << children.size() <<std::endl;

  if(depth>0){
    for(auto &name_and_node : children){
      ss<<std::string(indent+2,' ')<<name_and_node.name<<" "<<name_and_node.node.GetHex()<<endl;
    }
  }
}

} // namespace opbox
