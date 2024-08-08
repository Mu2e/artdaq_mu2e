#include "artdaq-mu2e/Generators/STMUDPReceiver.hh"

#include "artdaq/Generators/GeneratorMacros.hh"
//#include "STMDAQ-TestBeam/utils/dataVars.hh"
#include "STMDAQ-TestBeam/utils/xml.hh"
#include "STMDAQ-TestBeam/utils/Hex.hh"
#include "STMDAQ-TestBeam/utils/EnvVars.hh"

#include "trace.h"
#define TRACE_NAME "STMUDPReceiver"
mu2e::STMUDPReceiver::STMUDPReceiver(fhicl::ParameterSet const &ps)
	: artdaq::CommandableFragmentGenerator(ps)
  , verboseLevel_(ps.get<int>("verboseLevel", 0))
  , i_ch(ps.get<int>("channelNumber"))
  , port(ps.get<int>("port"))
  , ip_address(ps.get<std::string>("ip_address"))
{
	TLOG(TLVL_DEBUG) << "STMUDPReceiver Initialized";
  
  //  for (uint i = 0; i < chNum; i++){    
    // Setup server
  //    recvSock = udp.setupServer(i_ch,SW);
  recvSock = udp.setupServer(port, ip_address);
    rcv_buffer = new int16_t[udp.RECVMMSG_NUM * MAX_PACKET_LEN];
    recvCount = 0;
    //  }
}

mu2e::STMUDPReceiver::~STMUDPReceiver()
{
}

bool mu2e::STMUDPReceiver::getNext_(artdaq::FragmentPtrs &frags)//getNext_packet
{
	if (should_stop())
	{
    recvCount = 0;
		return false;
	}

  //  STMFragment::STM_fw_pHdr pHdr;

  // // Define a new fragment
  // frags.emplace_back(new artdaq::Fragment(ev_counter(), fragment_id(), FragmentType::STM, 0)); 
  // auto dataBegin = frags.back()->dataBegin();

  //  int event_len = 30003;
  //  int event_size = event_len*sizeof(int16_t);

  
  int header_len = 3;
  int header_size = header_len*sizeof(int16_t);
  // int pulse_len = 300000;
  // int pulse_size = pulse_len*sizeof(int16_t);

  int packet_len = 30003;
  // Loop until timouet
  //  while(udp.timeout_counter != udp.getTimeoutMax()){
    // Get packet and check if it times out (returns 0)
  //    if ((retval = udp.recv(rcv_buffer,recvSock,i_ch)) <= 0){
  if ((retval = udp.recv(rcv_buffer,recvSock)) <= 0){
    // Increment timeout counters
    udp.timeout_counter++;
    //    TLOG(TLVL_DEBUG+10) << "udp.timeout_counter = " << udp.timeout_counter;
    //      return true;
    if (udp.timeout_counter == 10*udp.getTimeoutMax()){
      TLOG(TLVL_DEBUG) << "STMUDPReceiver: UDP socket timed out!!" << std::endl;
      return false;
    }
    else { return true; }
  } // Else continue...}
    // If not timed out...
  else{
    // Reset timeout counter
    udp.timeout_counter = 0;
    // Increase receive counter
    recvCount += retval;
    
    TLOG(TLVL_DEBUG+10) << "retval = " << retval;

    // // Put all packets into a single fragment
    // frags.emplace_back(new artdaq::Fragment(ev_counter(), fragment_id(), FragmentType::STM, 0));
    // frags.back()->resizeBytes(retval*MAX_PACKET_SIZE); // very important: caused issues with writing data when this line was missing...
    // auto dataBegin = frags.back()->dataBegin();
    // TLOG(TLVL_DEBUG) << "rcv_buffer[0,1,2] = " << rcv_buffer[0] << ", " << rcv_buffer[1] << ", " << rcv_buffer[2];
    // memcpy(dataBegin, rcv_buffer, retval*MAX_PACKET_SIZE);
    // ev_counter_inc();  // increment event counter

    // // Put in one packet per fragment - TBD whether this means we drop packets
    // for (int i = 0; i < retval; ++i) {
    //   frags.emplace_back(new artdaq::Fragment(ev_counter(), fragment_id(), FragmentType::STM, 0));
    //   frags.back()->resizeBytes(event_size); // very important: caused issues with writing data when this line was missing...
    //   auto dataBegin = frags.back()->dataBegin();
    //   TLOG(TLVL_DEBUG+10) << "rcv_buffer[0,1,8] = " << rcv_buffer[i*event_len+0] << ", " << rcv_buffer[i*event_len+1] << ", " << rcv_buffer[i*event_len+8];
    //   memcpy(dataBegin, rcv_buffer+(i*event_len), event_size);
    //   TLOG(TLVL_DEBUG+10) << "dataBegin[0,1,8] = " << *(reinterpret_cast<int16_t*>(dataBegin)) << ", " << *(reinterpret_cast<int16_t*>(dataBegin) + 1) << ", " << *(reinterpret_cast<int16_t*>(dataBegin) + 8);
    //   ev_counter_inc();  // increment event counter
    // }

    // An event may be spread over multiple packets
    int n_data_copied = 0;
    TLOG(TLVL_DEBUG) << "max_data_to_copy = " << retval*packet_len;
    while (n_data_copied < retval*packet_len) {
      // sneak a look at the event length
      int event_len = *(rcv_buffer+n_data_copied+STMFragment::tHdr.EvLen);

      TLOG(TLVL_DEBUG) << "event_len = " << event_len;
      int event_size = event_len*sizeof(int16_t);
      
      frags.emplace_back(new artdaq::Fragment(ev_counter(), fragment_id(), FragmentType::STM, 0));
      frags.back()->resizeBytes(header_size+event_size); // very important: caused issues with writing data when this line was missing...

      auto dataBegin = frags.back()->dataBegin();
      memcpy(dataBegin, rcv_buffer+(n_data_copied), header_size+event_size); // copy the header
      n_data_copied += header_len+event_len;

      //      TLOG(TLVL_DEBUG) << "n_data_copied after header = " << n_data_copied;
      TLOG(TLVL_DEBUG) << "dataBegin[0,1,2] = " << *(reinterpret_cast<int16_t*>(dataBegin)) << ", " << *(reinterpret_cast<int16_t*>(dataBegin) + 1) << ", " << *(reinterpret_cast<int16_t*>(dataBegin) + 2);
      
      //      memcpy(dataBegin+n_data_copied, rcv_buffer+(n_data_copied), event_size); // copy the pulse
      //      n_data_copied += event_len;
      TLOG(TLVL_DEBUG) << "n_data_copied at end of loop = " << n_data_copied;
      ev_counter_inc();  // increment event counter
        
      //      break;
    }
    // Break loop
    //      break;
  } // End if timeout
    //  } // End loop    

    // auto stmDataBegin = reinterpret_cast<int16_t*>(frags.back()->dataBegin());
    // for (int i = 0; i < 32; ++i) {
    //   TLOG(TLVL_DEBUG) << "*dataBegin+" << i << ") = " << *(stmDataBegin+i);
    // }

  TLOG(TLVL_DEBUG) << "STMUDPReceiver: Number of packets received = " << recvCount << std::endl;

  return true;
}
// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(mu2e::STMUDPReceiver)
