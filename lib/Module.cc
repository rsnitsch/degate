/* -*-c++-*-
 
 This file is part of the IC reverse engineering tool degate.
 
 Copyright 2008, 2009 by Martin Schobert
 
 Degate is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 any later version.
 
 Degate is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with degate. If not, see <http://www.gnu.org/licenses/>.
 
*/

#include <degate.h>
#include <Module.h>
#include <boost/foreach.hpp>

using namespace degate;


Module::Module(std::string const& module_name, std::string const& _entity_name) :
  entity_name(_entity_name) {
  set_name(module_name);
}

Module::~Module() {}

void Module::set_entity_name(std::string const& name) {
  entity_name = name;
}

std::string Module::get_entity_name() const {
  return entity_name;
}

    
void Module::add_gate(Gate_shptr gate) throw(InvalidPointerException) {
  if(gate == NULL) 
    throw InvalidPointerException("Invalid pointer passed to add_gate().");
  
  gates.insert(gate);
  determine_module_ports();
}

bool Module::remove_gate(Gate_shptr gate) throw(InvalidPointerException) {
  if(gate == NULL) 
    throw InvalidPointerException("Invalid pointer passed to remove_gate().");
  
  
  gate_collection::const_iterator g_iter = gates.find(gate);
  if(g_iter != gates.end()) {
    gates.erase(g_iter);
    determine_module_ports();
    return true;
  }
  else {
    
    for(module_collection::iterator iter = modules.begin();
	iter != modules.end(); ++iter) {
      
      Module_shptr child = *iter;
      if((*iter)->remove_gate(gate) == true) return true;
    }
  }
  
  return false;
}


void Module::add_module(Module_shptr module) throw(InvalidPointerException) {
  if(module == NULL) 
    throw InvalidPointerException("Invalid pointer passed to add_modue().");
  
  modules.insert(module);
}



bool Module::remove_module(Module_shptr module) throw(InvalidPointerException) {

  if(module == NULL) 
    throw InvalidPointerException("Invalid pointer passed to remove_module().");
  
  for(module_collection::iterator iter = modules.begin();
      iter != modules.end(); ++iter) {
    
    Module_shptr child = *iter;
    
    if(child == module) {
      child->move_gates_recursive(this);
      modules.erase(iter);
      return true;
    }
    else if((*iter)->remove_module(module) == true)
      return true;
  }
  
  return false;
}

void Module::move_gates_recursive(Module * dst_mod) 
  throw(InvalidPointerException) {
  if(dst_mod == NULL)
    throw InvalidPointerException("Invalid pointer passed to move_all_child_gates_into_current_module().");

  for(gate_collection::iterator g_iter = gates_begin();
      g_iter != gates_end(); ++g_iter) {
    std::cout << "Add gate " << (*g_iter)->get_name() << " to module " << dst_mod->get_name() << std::endl;

    dst_mod->add_gate(*g_iter);
  }

  for(module_collection::iterator iter = modules.begin();
      iter != modules.end(); ++iter)
    (*iter)->move_gates_recursive(dst_mod);   
}

Module::module_collection::iterator Module::modules_begin() {
  return modules.begin();
}

Module::module_collection::iterator Module::modules_end() {
  return modules.end();
}

Module::gate_collection::iterator Module::gates_begin() {
  return gates.begin();
}

Module::gate_collection::iterator Module::gates_end() {
  return gates.end();
}

Module::port_collection::iterator Module::ports_begin() {
  return ports.begin();
}

Module::port_collection::iterator Module::ports_end() {
  return ports.end();
}


void Module::automove_gates() {
  /*
    from top to down in the module hierarchy:

    for each module: iterate over gates:

      iterate over gate ports:
      - port unconnected - ignore
      - port connected - check all related
      
   */
}

void Module::determine_module_ports() {

  /*
    data structure: known_nets, initially empty

    Iterate over gates:

    - Gate port unconnected -> no module port
    - Gate port connected to a net
      - net not in known_nets
        - iterate over net: 
	  - not exists: a gateport (!= current) belongs to a gate that is
	    a direct or indirect child of the current module
	    -> is a module port, add net to known_nets
	  - else -> no module port
      - net in known_nets -> no module port
   */

  //debug(TM, "in determine_module_ports()");
  std::set<Net_shptr> known_ports;

  for(gate_collection::iterator g_iter = gates_begin();
      g_iter != gates_end(); ++g_iter) {

    Gate_shptr gate = *g_iter;
    assert(gate != NULL);
    
    //debug(TM, "check gate");

    for(Gate::port_const_iterator p_iter = gate->ports_begin();
	p_iter != gate->ports_end(); ++p_iter) {
	
      GatePort_shptr gate_port = *p_iter;
      assert(gate_port != NULL);
      //debug(TM, "check gate port");

      Net_shptr net = gate_port->get_net();

      //debug(TM, "check net");

      //if(net == NULL) debug(TM, "is null");
      
      if((net != NULL) && known_ports.find(net) == known_ports.end()) {

	//debug(TM, "iterate over net with size %d", net->size());

	bool is_a_port = false;

	for(Net::connection_iterator c_iter = net->begin();
	    c_iter != net->end() && !is_a_port; ++c_iter) {
	  
	  object_id_t oid = *c_iter;

	  //debug(TM,"\tcheck object id: %d", oid);

	  if((oid != gate_port->get_object_id()) &&
	     !exists_gate_port_recursive(oid)) {
	    // is a module port
	    //debug(TM, "is a module port");
	    //std::cout << "This gate port is a module port: " << std::endl;
	    //gate_port->print();

	    GateTemplatePort_shptr tmpl_port = gate_port->get_template_port();
	    std::string mod_port_name;

	    if(tmpl_port == NULL)
	      mod_port_name = "unnamed";
	    else if(tmpl_port->is_tristate())
	      mod_port_name = "tristate";
	    else if(tmpl_port->is_inport())
	      mod_port_name = "inport";
	    else if(tmpl_port->is_outport())
	      mod_port_name = "outport";
	    else
	      mod_port_name = "undefined";

	    ports[mod_port_name].push_back(gate_port);
	    is_a_port = true;
	    known_ports.insert(net);
	  }
	}
      }
    }

  }
}


bool Module::exists_gate_port_recursive(object_id_t oid) const {
  assert(oid != 0);

  for(gate_collection::const_iterator g_iter = gates.begin();
      g_iter != gates.end(); ++g_iter) {

    Gate_shptr gate = *g_iter;

    for(Gate::port_const_iterator p_iter = gate->ports_begin();
	p_iter != gate->ports_end(); ++p_iter)
      if((*p_iter)->get_object_id() == oid) return true;
  }

  for(module_collection::const_iterator iter = modules.begin();
      iter != modules.end(); ++iter)
    if((*iter)->exists_gate_port_recursive(oid)) return true;
  
  return false;
}