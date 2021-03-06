/* -------------------------------------------------------------------------- */
/* Copyright 2002-2010, OpenNebula Project Leads (OpenNebula.org)             */
/*                                                                            */
/* Licensed under the Apache License, Version 2.0 (the "License"); you may    */
/* not use this file except in compliance with the License. You may obtain    */
/* a copy of the License at                                                   */
/*                                                                            */
/* http://www.apache.org/licenses/LICENSE-2.0                                 */
/*                                                                            */
/* Unless required by applicable law or agreed to in writing, software        */
/* distributed under the License is distributed on an "AS IS" BASIS,          */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   */
/* See the License for the specific language governing permissions and        */
/* limitations under the License.                                             */
/* -------------------------------------------------------------------------- */

#include "Nebula.h"
#include "VirtualMachinePool.h"
#include "VirtualMachineHook.h"
#include <sstream>

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

VirtualMachinePool::VirtualMachinePool(SqliteDB *                db,
                                       vector<const Attribute *> hook_mads)
    : PoolSQL(db,VirtualMachine::table)
{
    const VectorAttribute * vattr;

    string name;
    string on;
    string cmd;
    string arg;
    string rmt;
    bool   remote;

    bool state_hook = false;

    for (unsigned int i = 0 ; i < hook_mads.size() ; i++ )
    {
        vattr = static_cast<const VectorAttribute *>(hook_mads[i]);

        name = vattr->vector_value("NAME");
        on   = vattr->vector_value("ON");
        cmd  = vattr->vector_value("COMMAND");
        arg  = vattr->vector_value("ARGUMENTS");
        rmt  = vattr->vector_value("REMOTE");

        transform (on.begin(),on.end(),on.begin(),(int(*)(int))toupper);

        if ( on.empty() || cmd.empty() )
        {
            ostringstream oss;

            oss << "Empty ON or COMMAND attribute in VM_HOOK. Hook "
                << "not registered!";
            Nebula::log("VM",Log::WARNING,oss);

            continue;
        }

        if ( name.empty() )
        {
            name = cmd;
        }

        remote = false;

        if ( !rmt.empty() )
        {
            transform(rmt.begin(),rmt.end(),rmt.begin(),(int(*)(int))toupper);

            if ( rmt == "YES" )
            {
                remote = true;
            }
        }

        if ( on == "CREATE" )
        {
            VirtualMachineAllocateHook * hook;

            hook = new VirtualMachineAllocateHook(name,cmd,arg);

            add_hook(hook);
        }
        else if ( on == "RUNNING" )
        {
            VirtualMachineStateHook * hook;

            hook = new VirtualMachineStateHook(name, cmd, arg, remote,
                           VirtualMachine::RUNNING, VirtualMachine::ACTIVE);
            add_hook(hook);

            state_hook = true;
        }
        else if ( on == "SHUTDOWN" )
        {
            VirtualMachineStateHook * hook;

            hook = new VirtualMachineStateHook(name, cmd, arg, remote,
                            VirtualMachine::EPILOG, VirtualMachine::ACTIVE);
            add_hook(hook);

            state_hook = true;
        }
        else if ( on == "STOP" )
        {
            VirtualMachineStateHook * hook;

            hook = new VirtualMachineStateHook(name, cmd, arg, remote,
                            VirtualMachine::LCM_INIT, VirtualMachine::STOPPED);
            add_hook(hook);

            state_hook = true;
        }
        else if ( on == "DONE" )
        {
            VirtualMachineStateHook * hook;

            hook = new VirtualMachineStateHook(name, cmd, arg, remote,
                            VirtualMachine::LCM_INIT, VirtualMachine::DONE);
            add_hook(hook);

            state_hook = true;
        }
        else
        {
            ostringstream oss;

            oss << "Unkown VM_HOOK " << on << ". Hook not registered!";
            Nebula::log("VM",Log::WARNING,oss);
        }
    }

    if ( state_hook )
    {
        VirtualMachineUpdateStateHook * hook;

        hook = new VirtualMachineUpdateStateHook();

        add_hook(hook);
    }
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualMachinePool::allocate (
    int            uid,
    const  string& stemplate,
    int *          oid,
    bool           on_hold)
{
    VirtualMachine * vm;
    string  name;

    char *  error_msg;
    int     rc, num_attr;

    vector<Attribute *> attrs;

    // ------------------------------------------------------------------------
    // Build a new Virtual Machine object
    // ------------------------------------------------------------------------
    vm = new VirtualMachine;

    if (on_hold == true)
    {
        vm->state = VirtualMachine::HOLD;
    }
    else
    {
        vm->state = VirtualMachine::PENDING;
    }

    vm->uid = uid;

    // ------------------------------------------------------------------------
    // Parse template and keep CONTEXT apart
    // ------------------------------------------------------------------------
    rc = vm->vm_template.parse(stemplate,&error_msg);

    if ( rc != 0 )
    {
        ostringstream oss;

        oss << error_msg;
        Nebula::log("ONE", Log::ERROR, oss);
        free(error_msg);

        return -2;
    }

    vm->vm_template.remove("CONTEXT",attrs);


    // ------------------------------------------------------------------------
    // Insert the Object in the pool
    // ------------------------------------------------------------------------
    
    *oid = PoolSQL::allocate(vm);

    if ( *oid == -1 )
    {
        return -1;
    }

    // ------------------------------------------------------------------------
    // Insert parsed context in the VM template and clean-up
    // ------------------------------------------------------------------------
    
    if ((num_attr = (int) attrs.size()) > 0)
    {
        generate_context(*oid,attrs[0]);

        for (int i = 0; i < num_attr ; i++)
        {
            if (attrs[i] != 0)
            {
                delete attrs[i];
            }
        }
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualMachinePool::get_running(
    vector<int>&    oids)
{
    ostringstream   os;
    string          where;

    os << "state == " << VirtualMachine::ACTIVE
       << " and ( lcm_state == " << VirtualMachine::RUNNING
       << " or lcm_state == " << VirtualMachine::UNKNOWN << " )";

    where = os.str();

    return PoolSQL::search(oids,VirtualMachine::table,where);
};

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualMachinePool::get_pending(
    vector<int>&    oids)
{
    ostringstream   os;
    string          where;

    os << "state == " << VirtualMachine::PENDING;

    where = os.str();

    return PoolSQL::search(oids,VirtualMachine::table,where);
};

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void VirtualMachinePool::generate_context(int vm_id, Attribute * attr)
{
    VirtualMachine *  vm;
    VectorAttribute * context_parsed;
    VectorAttribute * context;

    string *          str;
    string            parsed;

    int               rc;

    char *            error_msg;

    context = dynamic_cast<VectorAttribute *>(attr);

    if (context == 0)
    {
        return;
    }

    str = context->marshall(" @^_^@ ");

    if (str == 0)
    {
        return;
    }

    rc = VirtualMachine::parse_template_attribute(vm_id,*str,parsed,&error_msg);

    if ( rc != 0 )
    {
        if (error_msg != 0)
        {
            ostringstream oss;

            oss << error_msg << ": " << *str;
            free(error_msg);

            Nebula::log("ONE", Log::ERROR, oss);
        }

        delete str;

        return;
    }

    delete str;

    context_parsed = new VectorAttribute("CONTEXT");
    context_parsed->unmarshall(parsed," @^_^@ ");

    vm = get(vm_id,true);

    if ( vm == 0 )
    {
        delete context_parsed;
        return;
    }

    if ( vm->insert_template_attribute(db,context_parsed) != 0 )
    {
        delete context_parsed;
    }

    vm->unlock();
}


