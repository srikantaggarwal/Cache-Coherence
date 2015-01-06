#include "MOESIF_protocol.h"
#include "../sim/mreq.h"
#include "../sim/sim.h"
#include "../sim/hash_table.h"

extern Simulator *Sim;

/*************************
 * Constructor/Destructor.
 *************************/
MOESIF_protocol::MOESIF_protocol (Hash_table *my_table, Hash_entry *my_entry)
    : Protocol (my_table, my_entry)
{
    // Initialize lines to not have the data yet!
    this->state = MOESIF_CACHE_I;
}

MOESIF_protocol::~MOESIF_protocol ()
{    
}

void MOESIF_protocol::dump (void)
{
    const char *block_states[9] = {"X","I","S","E","O","M","F"};
    fprintf (stderr, "MOESIF_protocol - state: %s\n", block_states[state]);
}

void MOESIF_protocol::process_cache_request (Mreq *request)
{
	switch (state) {
    case MOESIF_CACHE_I:  do_cache_I (request); break;
    case MOESIF_CACHE_S:  do_cache_S (request); break;
    case MOESIF_CACHE_E:  do_cache_E (request); break;
    case MOESIF_CACHE_O:  do_cache_O (request); break;
    case MOESIF_CACHE_M:  do_cache_M (request); break;
    case MOESIF_CACHE_F:  do_cache_F (request); break;
    case MOESIF_CACHE_IS: do_cache_IS (request); break;
    case MOESIF_CACHE_IM: do_cache_IM (request); break;
    case MOESIF_CACHE_SM: do_cache_SM (request); break;
    case MOESIF_CACHE_OM: do_cache_OM (request); break;
    case MOESIF_CACHE_FM: do_cache_FM (request); break;
    default:
        fatal_error ("Invalid Cache State for MOESIF Protocol\n");
    }
}

void MOESIF_protocol::process_snoop_request (Mreq *request)
{
	switch (state) {
    case MOESIF_CACHE_I:  do_snoop_I (request); break;
    case MOESIF_CACHE_S:  do_snoop_S (request); break;
    case MOESIF_CACHE_E:  do_snoop_E (request); break;
    case MOESIF_CACHE_O:  do_snoop_O (request); break;
    case MOESIF_CACHE_M:  do_snoop_M (request); break;
    case MOESIF_CACHE_F:  do_snoop_F (request); break;
    case MOESIF_CACHE_IS: do_snoop_IS (request); break;
    case MOESIF_CACHE_IM: do_snoop_IM (request); break;
    case MOESIF_CACHE_SM: do_snoop_SM (request); break;
    case MOESIF_CACHE_OM: do_snoop_OM (request); break;
    case MOESIF_CACHE_FM: do_snoop_FM (request); break;
    default:
    	fatal_error ("Invalid Cache State for MOESIF Protocol\n");
    }
}
	
inline void MOESIF_protocol::do_cache_I (Mreq *request)
{
    switch (request->msg) {
    // If we get a request from the processor we need to get the data
    case LOAD:
    	/* Line up the GETS in the Bus' queue */
    	send_GETS(request->addr);
    	/* The IS state means that we have sent the GET message and we are now waiting
    	 * on DATA
    	 */
    	state = MOESIF_CACHE_IS;
    	/* This is a cache miss */
    	Sim->cache_misses++;
    	break;
    case STORE:
    	/* Line up the GETM in the Bus' queue */
    	send_GETM(request->addr);
    	/* The IM state means that we have sent the GET message and we are now waiting
    	 * on DATA
    	 */
    	state = MOESIF_CACHE_IM;
    	/* This is a cache miss */
    	Sim->cache_misses++;
    	break;
    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: I state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_cache_S (Mreq *request)
{
    switch (request->msg) {
    case LOAD:
    /* This is how you send data back to the processor to finish the request        	
     * Note: There was no need to send anything on the bus on a hit.
     */
        send_DATA_to_proc(request->addr);
    	break;
    case STORE:
    /* You are in S state. If you encounter STORE, you give a GetM command
     * And invalidate the other processors
     */
        send_GETM(request->addr);
        state = MOESIF_CACHE_SM;
        Sim->cache_misses++;
        break;
    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: M state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_cache_E (Mreq *request)
{
    switch (request->msg) {
    /* The E state means we have the data exclusively.  Therefore any request
     * from the processor (read or write) can be immediately satisfied.
     */
    case LOAD:
    /* This is how you send data back to the processor to finish the request        	
     * Note: There was no need to send anything on the bus on a hit.
     */
        send_DATA_to_proc(request->addr);
    	break;
    case STORE:
    /* You are in E state. If you encounter STORE, you silently upgrade to M
     * No need to invalidate other processors.
     */
        send_DATA_to_proc(request->addr);
        state = MOESIF_CACHE_M;
        Sim->silent_upgrades++;
        break;
    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: M state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_cache_O (Mreq *request)
{
    switch (request->msg) {
    /* The M state means we have the data and we can modify it.  Therefore any request
     * from the processor (read or write) can be immediately satisfied.
     */
    case LOAD:
    /* This is how you send data back to the processor to finish the request
     * Note: There was no need to send anything on the bus on a hit.
     */
        send_DATA_to_proc(request->addr);
    	break;    
    case STORE:
    /* We are the owner of the data. So if we modify it, we need to send GetM and
     * invalidate the other processors that use that data
     */
        send_GETM(request->addr);
        state = MOESIF_CACHE_OM;
        Sim->cache_misses++;                                                                        
        break;
    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: M state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_cache_M (Mreq *request)
{
    switch (request->msg) {
    /* The M state means we have the data and we can modify it.  Therefore any request
     * from the processor (read or write) can be immediately satisfied.
     */
    case LOAD:
    case STORE:
    /* This is how you send data back to the processor to finish the request
     * Note: There was no need to send anything on the bus on a hit.
     */
    	send_DATA_to_proc(request->addr);
    	break;
    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: M state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_cache_F (Mreq *request)
{
    switch (request->msg) {
    /* The M state means we have the data and we can modify it.  Therefore any request
     * from the processor (read or write) can be immediately satisfied.
     */
    case LOAD:
        send_DATA_to_proc(request->addr);
    	break;    
    case STORE:
    /* We are responsible for forwarding the data. So if we modify it, we need to send GetM and
     * invalidate the other processors that use that data
     */
        send_GETM(request->addr);
        state = MOESIF_CACHE_FM;
        Sim->cache_misses++;                                                                        
        break;
    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: M state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_cache_IS (Mreq *request)
{
	switch (request->msg) {
	/* If the block is in the IS state that means it sent out a GET message
	 * and is waiting on DATA.  Therefore the processor should be waiting
	 * on a pending request. Therefore we should not be getting any requests from
	 * the processor.
	 */
	case LOAD:
                request->print_msg (my_table->moduleID, "ERROR");
		fatal_error("Should only have one outstanding request per processor!");
	case STORE:
                request->print_msg (my_table->moduleID, "ERROR");
		fatal_error("Should only have one outstanding request per processor!");
	default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: I state shouldn't see this message\n");
	}
}

inline void MOESIF_protocol::do_cache_IM (Mreq *request)
{
	switch (request->msg) {
	/* If the block is in the IM state that means it sent out a GET message
	 * and is waiting on DATA.  Therefore the processor should be waiting
	 * on a pending request. Therefore we should not be getting any requests from
	 * the processor.
	 */
	case LOAD:
                request->print_msg (my_table->moduleID, "ERROR");
		fatal_error("Should only have one outstanding request per processor!");
	case STORE:
		request->print_msg (my_table->moduleID, "ERROR");
		fatal_error("Should only have one outstanding request per processor!");
	default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: I state shouldn't see this message\n");
	}
}

inline void MOESIF_protocol::do_cache_OM (Mreq *request)
{
	switch (request->msg) {
	/* If the block is in the OM state that means it sent out a GET message
	 * and is waiting on DATA.  Therefore the processor should be waiting
	 * on a pending request. Therefore we should not be getting any requests from
	 * the processor.
	 */
	case LOAD:
                request->print_msg (my_table->moduleID, "ERROR");
		fatal_error("Should only have one outstanding request per processor!");
	case STORE:
		request->print_msg (my_table->moduleID, "ERROR");
		fatal_error("Should only have one outstanding request per processor!");
	default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: I state shouldn't see this message\n");
	}
}

inline void MOESIF_protocol::do_cache_SM (Mreq *request)
{
	switch (request->msg) {
	/* If the block is in the SM state that means it sent out a GET message
	 * and is waiting on DATA.  Therefore the processor should be waiting
	 * on a pending request. Therefore we should not be getting any requests from
	 * the processor.
	 */
	case LOAD:
                request->print_msg (my_table->moduleID, "ERROR");
		fatal_error("Should only have one outstanding request per processor!");
	case STORE:
		request->print_msg (my_table->moduleID, "ERROR");
		fatal_error("Should only have one outstanding request per processor!");
	default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: I state shouldn't see this message\n");
	}
}

inline void MOESIF_protocol::do_cache_FM (Mreq *request)
{
	switch (request->msg) {
	/* If the block is in the FM state that means it sent out a GET message
	 * and is waiting on DATA.  Therefore the processor should be waiting
	 * on a pending request. Therefore we should not be getting any requests from
	 * the processor.
	 */
	case LOAD:
                request->print_msg (my_table->moduleID, "ERROR");
		fatal_error("Should only have one outstanding request per processor!");
	case STORE:
		request->print_msg (my_table->moduleID, "ERROR");
		fatal_error("Should only have one outstanding request per processor!");
	default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: I state shouldn't see this message\n");
	}
}

inline void MOESIF_protocol::do_snoop_I (Mreq *request)
{
    switch (request->msg) {
    case GETS:
    case GETM:
    case DATA:
    	/**
    	 * If we snoop a message from another cache and we are in I, then we don't
    	 * need to do anything!  We obviously cannot supply data since we don't have
    	 * it, and we don't need to downgrade our state since we are already in I.
    	 */
    	break;
    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: I state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_snoop_S (Mreq *request)
{
    switch (request->msg) {
    case GETS:
    	/**
         * Another cache wants the data without the intention to modify,
         * so we send it to them and stay in the Shared state
         */
    	set_shared_line();
    	break;        
    case GETM:
    	/**
    	 * Another cache wants the data so we send it to them and transition to
    	 * Invalid since they will be transitioning to M.  When we send the DATA
    	 * it will go on the bus the next cycle and the memory will see it and cancel
    	 * its lookup for the DATA.
    	 */
    	/**
    	 * We use set_shared_line() here to demonstrate its use.  Since we had a copy
    	 * of the line being requested, we should indicate that the data is shared on
    	 * chip.  (This will be essential to understand in order to implement MESI/
    	 * MOSI/MOESI)
    	 */
    	set_shared_line();
    	state = MOESIF_CACHE_I;
    	break;
    case DATA:
    	fatal_error ("Should not see data for this line!  I have the line!");
    	break;
    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: M state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_snoop_E (Mreq *request)
{
    switch (request->msg) {
    case GETS:
    	/**
         * Another cache wants the data without the intention to modify,
         * so we send it to them and go to the Shared state
         */
    	set_shared_line();
    	send_DATA_on_bus(request->addr,request->src_mid);
    	state = MOESIF_CACHE_F;
    	break;        
    case GETM:
    	/**
    	 * Another cache wants the data so we send it to them and transition to
    	 * Invalid since they will be transitioning to M.  When we send the DATA
    	 * it will go on the bus the next cycle and the memory will see it and cancel
    	 * its lookup for the DATA.
    	 */
    	/**
    	 * We use set_shared_line() here to demonstrate its use.  Since we had a copy
    	 * of the line being requested, we should indicate that the data is shared on
    	 * chip.  (This will be essential to understand in order to implement MESI/
    	 * MOSI/MOESI)
    	 */
    	set_shared_line();
    	send_DATA_on_bus(request->addr,request->src_mid);
    	state = MOESIF_CACHE_I;
    	break;
    case DATA:
    	fatal_error ("Should not see data for this line!  I have the line!");
    	break;
    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: M state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_snoop_O (Mreq *request)
{
    switch (request->msg) {
    case GETS:
        /**
         * Another cache wants the data and we are the owner of the data
         * So we send the data to them and stay in the same state
         */
    	set_shared_line();
    	send_DATA_on_bus(request->addr,request->src_mid);
    	break;
    case GETM:
    	/**
    	 * Another cache wants the data so we send it to them and transition to
    	 * Invalid since they will be transitioning to M.  When we send the DATA
    	 * it will go on the bus the next cycle and the memory will see it and cancel
    	 * its lookup for the DATA.
    	 */
    	/**
    	 * We use set_shared_line() here to demonstrate its use.  Since we had a copy
    	 * of the line being requested, we should indicate that the data is shared on
    	 * chip.  (This will be essential to understand in order to implement MESI/
    	 * MOSI/MOESI)
    	 */
    	set_shared_line();
    	send_DATA_on_bus(request->addr,request->src_mid);
    	state = MOESIF_CACHE_I;
    	break;
    case DATA:
    	fatal_error ("Should not see data for this line!  I have the line!");
    	break;
    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: M state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_snoop_M (Mreq *request)
{
    switch (request->msg) {
    case GETS:
    	/**
    	 * Another cache wants the data and we have the dirty copy of the data
    	 * Memory has the stale copy of the data
         */
    	/**
    	 * We use set_shared_line() here to demonstrate its use.  Since we had a copy
    	 * of the line being requested, we should indicate that the data is shared on
    	 * chip.  (This will be essential to understand in order to implement MESI/
    	 * MOSI/MOESI)
    	 */
    	set_shared_line();
    	send_DATA_on_bus(request->addr,request->src_mid);
    	state = MOESIF_CACHE_O;
    	break;
    case GETM:
    	/**
    	 * Another cache wants the data so we send it to them and transition to
    	 * Invalid since they will be transitioning to M.  When we send the DATA
    	 * it will go on the bus the next cycle and the memory will see it and cancel
    	 * its lookup for the DATA.
    	 */
    	/**
    	 * We use set_shared_line() here to demonstrate its use.  Since we had a copy
    	 * of the line being requested, we should indicate that the data is shared on
    	 * chip.  (This will be essential to understand in order to implement MESI/
    	 * MOSI/MOESI)
    	 */
    	set_shared_line();
    	send_DATA_on_bus(request->addr,request->src_mid);
    	state = MOESIF_CACHE_I;
    	break;
    case DATA:
    	fatal_error ("Should not see data for this line!  I have the line!");
    	break;
    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: M state shouldn't see this message\n");
    }		
}

inline void MOESIF_protocol::do_snoop_F (Mreq *request)
{
    switch (request->msg) {
    case GETS:
       /**
        * Another cache wants the data and we are the forwarder of the data
        * So we send them the data and remain in the same state
        */
    	set_shared_line();
    	send_DATA_on_bus(request->addr,request->src_mid);
    	state = MOESIF_CACHE_F;
    	break;
    case GETM:
    	/**
    	 * Another cache wants the data so we send it to them and transition to
    	 * Invalid since they will be transitioning to M.  When we send the DATA
    	 * it will go on the bus the next cycle and the memory will see it and cancel
    	 * its lookup for the DATA.
    	 */
    	/**
    	 * We use set_shared_line() here to demonstrate its use.  Since we had a copy
    	 * of the line being requested, we should indicate that the data is shared on
    	 * chip.  (This will be essential to understand in order to implement MESI/
    	 * MOSI/MOESI)
    	 */
    	set_shared_line();
    	send_DATA_on_bus(request->addr,request->src_mid);
    	state = MOESIF_CACHE_I;
    	break;
    case DATA:
    	fatal_error ("Should not see data for this line!  I have the line!");
    	break;
    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: M state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_snoop_IS (Mreq *request)
{
	switch (request->msg) {
	case GETS:
	case GETM:
		/** While in IS we will see our own GETS or GETM on the bus.  We should just
		 * ignore it and wait for DATA to show up.
		 */
		break;
	case DATA:
		/** IS state meant that the block had sent the GET and was waiting on DATA.
		 * Now that Data is received we can send the DATA to the processor and finish
		 * the transition to M.
		 */
		/**
		 * Note we use get_shared_line() here to demonstrate its use.
		 * (Hint) The shared line indicates when another cache has a copy and is useful
		 * for knowing when to go to the E/S state.
		 * Since we only have I and M state in the MI protocol, what the shared line
		 * means here is whether a cache sent the data or the memory controller.
		 */
		if (get_shared_line())
		{
		  send_DATA_to_proc(request->addr);
		  state = MOESIF_CACHE_S;
		}
                else
                {
		  send_DATA_to_proc(request->addr);
		  state = MOESIF_CACHE_E;
                }
		break;
	default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: I state shouldn't see this message\n");
	}
}

inline void MOESIF_protocol::do_snoop_IM (Mreq *request)
{
	switch (request->msg) {
	case GETS:
	case GETM:
		/** While in IM we will see our own GETS or GETM on the bus.  We should just
		 * ignore it and wait for DATA to show up.
		 */
		break;
	case DATA:
		/** IM state meant that the block had sent the GET and was waiting on DATA.
		 * Now that Data is received we can send the DATA to the processor and finish
		 * the transition to M.
		 */
		/**
		 * Note we use get_shared_line() here to demonstrate its use.
		 * (Hint) The shared line indicates when another cache has a copy and is useful
		 * for knowing when to go to the E/S state.
		 * Since we only have I and M state in the MI protocol, what the shared line
		 * means here is whether a cache sent the data or the memory controller.
		 */
		send_DATA_to_proc(request->addr);
		state = MOESIF_CACHE_M;
		break;
	default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: I state shouldn't see this message\n");
	}
}

inline void MOESIF_protocol::do_snoop_OM (Mreq *request)
{
	switch (request->msg) {
	case GETS:
                set_shared_line();
    	        send_DATA_on_bus(request->addr,request->src_mid); 
                break;               
	case GETM:
		/** The processor is transitioning from O state to M state
                 * So if it sees GETM, it has to go to the IM state
                 */
                set_shared_line();
    	        send_DATA_on_bus(request->addr,request->src_mid);
                state = MOESIF_CACHE_IM;
		break;
	case DATA:
		/** IM state meant that the block had sent the GET and was waiting on DATA.
		 * Now that Data is received we can send the DATA to the processor and finish
		 * the transition to M.
		 */
		/**
		 * Note we use get_shared_line() here to demonstrate its use.
		 * (Hint) The shared line indicates when another cache has a copy and is useful
		 * for knowing when to go to the E/S state.
		 * Since we only have I and M state in the MI protocol, what the shared line
		 * means here is whether a cache sent the data or the memory controller.
		 */
		send_DATA_to_proc(request->addr);
		state = MOESIF_CACHE_M;
		break;
	default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: I state shouldn't see this message\n");
	}
}

inline void MOESIF_protocol::do_snoop_SM (Mreq *request)
{
	switch (request->msg) {
	case GETS: 
                set_shared_line();  
                break;
	case GETM:
                set_shared_line();
		break;
	case DATA:
		/** SM state meant that the block had sent the GET and was waiting on DATA.
		 * Now that Data is received we can send the DATA to the processor and finish
		 * the transition to M.
		 */
		/**
		 * Note we use get_shared_line() here to demonstrate its use.
		 * (Hint) The shared line indicates when another cache has a copy and is useful
		 * for knowing when to go to the E/S state.
		 * Since we only have I and M state in the MI protocol, what the shared line
		 * means here is whether a cache sent the data or the memory controller.
		 */
		send_DATA_to_proc(request->addr);
		state = MOESIF_CACHE_M;
		break;
	default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: I state shouldn't see this message\n");
	}
}

inline void MOESIF_protocol::do_snoop_FM (Mreq *request)
{
	switch (request->msg) {
	case GETS:
                set_shared_line();
    	        send_DATA_on_bus(request->addr,request->src_mid); 
                break;               
	case GETM:
		/** The processor is transitioning from F state to M state
                 * So if it sees GETM, it has to go to the IM state
                 */
                set_shared_line();
    	        send_DATA_on_bus(request->addr,request->src_mid);
                state = MOESIF_CACHE_IM;
		break;
	case DATA:
		/** IM state meant that the block had sent the GET and was waiting on DATA.
		 * Now that Data is received we can send the DATA to the processor and finish
		 * the transition to M.
		 */
		/**
		 * Note we use get_shared_line() here to demonstrate its use.
		 * (Hint) The shared line indicates when another cache has a copy and is useful
		 * for knowing when to go to the E/S state.
		 * Since we only have I and M state in the MI protocol, what the shared line
		 * means here is whether a cache sent the data or the memory controller.
		 */
		send_DATA_to_proc(request->addr);
		state = MOESIF_CACHE_O;
		break;
	default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: I state shouldn't see this message\n");
	}
}
