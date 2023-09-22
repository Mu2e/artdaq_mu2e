#include "artdaq-mu2e/Generators/STMReceiver.hh"

#include "artdaq/Generators/GeneratorMacros.hh"
#include "STMDAQ-TestBeam/utils/dataVars.hh"
#include "STMDAQ-TestBeam/utils/xml.hh"
#include "STMDAQ-TestBeam/utils/Hex.hh"
#include "STMDAQ-TestBeam/utils/EnvVars.hh"

#include "trace.h"
#define TRACE_NAME "STMReceiver"
int64_t pNum = -1;
uint64_t eNum = 1;
uint64_t data_file_bytes  = 0;
uint16_t packet_counter = 0;
int64_t remaining_adc_bytes_from_event = 0;
bool finished_previous_tHdr_ingest = true;
uint64_t file_size = 0;
// TODO - remove this when the data structure is correct.
bool verbose = true;
bool exit_on_failed_tests = false;
uint32_t packet_adc_data_size = 0;
mu2e::STMReceiver::STMReceiver(fhicl::ParameterSet const &ps)
	: artdaq::CommandableFragmentGenerator(ps)
	, fromInputFile_(ps.get<bool>("from_input_file", false))
	, toOutputFile_(ps.get<bool>("to_output_file", false))
	, read_to_fragment_(ps.get<bool>("read_to_fragment", false))
{
	TLOG(TLVL_DEBUG) << "STMReceiver Initialized";

	if (fromInputFile_)
	{
		auto input_file = ps.get<std::string>("input_file", "");
		inputFileStream_.open(input_file, std::ios::in | std::ios::binary);
		inputFileStream_.seekg(0, ios::end);
		file_size = inputFileStream_.tellg();
		inputFileStream_.seekg(0);
	}
	else { // read from a socket
	  for (uint i = 0; i < chNum; i++){    
	    // Setup server
	    recvSock[i] = udp[i].setupServer(i,SW);
	  }
	  rcv_buffer = new UDPsocket::packet [udp[0].RECVMMSG_NUM];
	}

	if (toOutputFile_)
	{
		auto output_file = ps.get<std::string>("output_file", "");
		outputFileStream_.open(output_file, std::ios::out | std::ios::binary);
	}
}

mu2e::STMReceiver::~STMReceiver()
{
	if (fromInputFile_)
	{
		inputFileStream_.close();
	}

	if (toOutputFile_)
	{
		outputFileStream_.close();
	}
}

bool mu2e::STMReceiver::getNext_(artdaq::FragmentPtrs &frags)//getNext_packet
{
	if (should_stop())
	{
		return false;
	}

	if (fromInputFile_)
	{
	  std::cout << "fromInputFile_ is: " << fromInputFile_ << std::endl;
	  std::cout << "Read in from file size " << MAX_PACKET_SIZE*packet_counter << ", test is " << ((MAX_PACKET_SIZE*packet_counter) == file_size) << std::endl;
	  std::cout << "File size is " << file_size << std::endl;
	  std::cout << "inputFileStream_.eof(): " << inputFileStream_.eof() << std::endl;
	  if (inputFileStream_.eof() | ((MAX_PACKET_SIZE*packet_counter) == file_size))
		{  // if we reached the end of the file, then stop
			return false;
		}
	}

	// Define the STM packet header
	STMFragment::STM_fw_pHdr pHdr;
	
	// Define the STM trigger header
	STMFragment::STM_fw_tHdr tHdr;

	// Create a vector with enough space to hold all the packet ADC data
	uint16_t max_size_words = MAX_PACKET_LEN - fw_pHdr_Len - fw_tHdr_Len;
	std::vector<int16_t> adcs(max_size_words, 0);
	std::vector<uint16_t> pNumTestBuffer(MAX_PACKET_LEN, 0);

	// Define parameters to count with
	uint16_t tHdr_adc_bytes = 0;
	uint16_t bytes_from_packet = 0;
	bool multiple_events_in_packet = false;
	uint incorrect_pNums = 0;
	if (fromInputFile_)
	{
	  // Define a new fragment
	  frags.emplace_back(new artdaq::Fragment(ev_counter(), fragment_id(), FragmentType::STM, 0)); 
	  auto dataBegin = frags.back()->dataBegin();

	  // Read in the packet header
	  inputFileStream_.read(reinterpret_cast<char*>(&pHdr), fw_pHdr_Size);
	  bytes_from_packet += fw_pHdr_Size;
	  std::cout << "Read in the pHdr. bytes_from_packet is " << bytes_from_packet << std::endl;

	  if(read_to_fragment)_{
	    memcpy(dataBegin, &pHdr, fw_pHdr_Size);
	    dataBegin += fw_pHdr_Size;
	    if(verbose){std::cout << "Read the packet header into the fragment" << std::endl;};
	  };

	  // Test the pHdr data
	  std::cout << "19. Packet test. pNum=" << pNum << ", pHdr.pNum()=" << pHdr.pNum() << ", test is " << pHdr._fw_pHdr[2] << std::endl;
	  std::cout << std::hex << "0."<< pHdr._fw_pHdr[0] << "\n1." << pHdr._fw_pHdr[1] << "\n2." << pHdr._fw_pHdr[2] << std::dec << std::endl;
	  
	  // Run the tests
	  if (pHdr._fw_pHdr[2] != 0xFFEE){
	    std::cout << "24. Wrong test value. Expecting -18, got " << pHdr._fw_pHdr[2] << std::endl;
	    if(exit_on_failed_tests){exit(0);};
	  }

	  if (((int64_t)pHdr.pNum() - pNum) != 1){
	    std::cout << "10. Error reading packets - pNum is " << std::hex << pHdr.pNum() << std::dec << ", expected " << std::hex << pNum + 1 << std::dec << std::endl;
	    incorrect_pNums++;
	    if(exit_on_failed_tests){exit(0);};
	  }
	  pNum++;
	  
	  while (bytes_from_packet < MAX_PACKET_SIZE){

	    // Read in the trigger header locally
	    inputFileStream_.read(reinterpret_cast<char*>(&tHdr), fw_tHdr_Size);
	    bytes_from_packet += fw_tHdr_Size;

	    // Read in the trigger header to the fragment
	    if (read_to_fragment_){
	      memcpy(dataBegin, &tHdr, fw_tHdr_Size);
	      dataBegin += fw_tHdr_Size;
	      if(verbose){std::cout << "Read the trigger header into the fragment" << std::endl;};
	    };

	    // Debugging
	    if (verbose){

	      std::cout << "Read in the tHdr. bytes_from_packet is " << bytes_from_packet << std::endl;
	    
	      // Read out all the trigger header parameters
	      std::cout << "DTC Clock:      " << tHdr.DTCclk() << std::endl;
	      std::cout << "ADC Clock:      " << tHdr.ADCclk() << std::endl;
	      std::cout << "EvNum:          " << tHdr.EvNum() << std::endl;
	      std::cout << "EWT:            " << tHdr.EWT() << std::endl;
	      std::cout << "EM:             " << tHdr.EM() << std::endl;
	      std::cout << "DRTDC:          " << tHdr.DRTDC() << std::endl;
	      std::cout << "EvStart:        " << tHdr.EvStart() << std::endl;
	      std::cout << "EvLen:          " << tHdr.EvLen() << std::endl;
	      std::cout << "Channel number: " << tHdr.channel() << std::endl;
	    
	      // Verify the ending of the trigger is as expected
	      for (uint i = 19; i < 32; i++){
		std::cout << "Index " << i << ": " << std::hex << tHdr._fw_tHdr[i] << std::dec << std::endl;
	      };
	    };


	    // TODO - there is a packet drop in channe0_subrun0.bin, remove the exit comment with the next datafile
	    // Check the event number increment
	    if (multiple_events_in_packet && (eNum != tHdr.EvNum())){
	      std::cout << "Expected the event number to increment. Expected " << eNum << ", got " << tHdr.EvNum() << std::endl;
	      if(exit_on_failed_tests){exit(0);};
	    };

	    // Define the size of the trigger header ADC data
	    tHdr_adc_bytes = tHdr.EvLen() * sizeof(uint16_t);
	    

	    // Read in the data from the IFStream
	    // If the remainder of the event's ADC data is larger than the packet size
	    if (tHdr_adc_bytes >= (MAX_PACKET_SIZE - bytes_from_packet)){
	      if(verbose){std::cout << "00. Rest of the packet is read in as ADC data, breaking loop" << std::endl;};
	      // Read in only the rest of the packet - the ADC data.
	      inputFileStream_.read(reinterpret_cast<char*>(&adcs[0]), (MAX_PACKET_SIZE - bytes_from_packet));

	      // Set the bytes read from the packet to the maximum size available
	      bytes_from_packet = MAX_PACKET_SIZE;

	      // Read data into the header
	      if(read_to_fragment_){
		memcpy(dataBegin, &adcs[0], (MAX_PACKET_SIZE - bytes_from_packet));
		dataBegin += (MAX_PACKET_SIZE - bytes_from_packet);
		if(verbose){std::cout << "Read the adc data into the fragment" << std::endl;};
	      };
	    }

	    // Else if the remainder of the event's ADC data is fully contained in the packet
	    else if (tHdr_adc_bytes > 0){
	      if(verbose){std::cout << "01. Reading in the ADC data. Remaining data size is " << tHdr_adc_bytes << ", max that can be taken is " << (MAX_PACKET_SIZE-bytes_from_packet) << std::endl;};
	      // Read in the rest of the event's ADC data
	      inputFileStream_.read(reinterpret_cast<char*>(&adcs[0]), tHdr_adc_bytes);

	      // Increment the bytes read from the packet by the data size read from the event
	      bytes_from_packet += tHdr_adc_bytes;

	      // Read data into fragment
	      if (read_to_fragment_){
		memcpy(dataBegin, &adcs[0], tHdr_adc_bytes);
		dataBegin += tHdr_adc_bytes;
		if(verbose){std::cout << "Read the adc data into the fragment" << std::endl;};
	      }

	      // Set the flag to true. This is used to check if the event number is incrementing correctly.
	      multiple_events_in_packet = true;

	      // Increment the event number
	      eNum++;

	      if(verbose){
		std::cout << "11. Multiple events in packet is " << multiple_events_in_packet << std::endl;
		std::cout << "16. Incremented eNum=" << eNum << std::endl;
	      };
	    }

	    // If all else fails.
	    else{
	      std::cout << "Sizing error" << std::endl;
	      exit(0);
	    };

	    if(verbose){
	      std::cout << "03. Number of bytes read from current packet is " << bytes_from_packet << std::endl;
	      std::cout << "17. Sizing test. \nMAX_PACKET_SIZE - bytes_from_packet=" << MAX_PACKET_SIZE - bytes_from_packet << ", fw_tHdr_Size=" << fw_tHdr_Size << std::endl;
	      std::cout << "18. Other size test.\nMAX_PACKET_SIZE - bytes_from_packet=" << MAX_PACKET_SIZE - bytes_from_packet << ", expecting 0" << std::endl;
	    };

	    // Check and read in 0xDEADBEEFs
	    if ((static_cast<uint16_t>(MAX_PACKET_SIZE - bytes_from_packet) <= fw_tHdr_Size) && (bytes_from_packet != MAX_PACKET_SIZE)){

	      // Read in the remainder of the data from the packet
	      uint16_t deadbeefarray[fw_tHdr_Len];
	      inputFileStream_.read(reinterpret_cast<char*>(&deadbeefarray[0]), (MAX_PACKET_SIZE - bytes_from_packet));

	      // Read into fragment
	      if(read_to_fragment_){
		memcpy(dataBegin, &adcs[0], (MAX_PACKET_SIZE - bytes_from_packet));
		dataBegin += (MAX_PACKET_SIZE - bytes_from_packet);
		if(verbose){std::cout << "Read the DEADBEEFs into the fragment" << std::endl;};
	      };

	      // Check every 0xDEADBEEF
	      for (int j = 0; j < (MAX_PACKET_SIZE - bytes_from_packet)/4; j++){
		if ((deadbeefarray[2*j] != 0xBEEF) | (deadbeefarray[2*j+1] != 0xDEAD)){
		  std::cout << "21. Expecting DEAD BEEF, got " << std::hex << deadbeefarray[2*j] << " " << deadbeefarray[2*j+1] << std::dec << "\n" << j << std::endl;
		  exit(0);
		};
	      };

	      // Finished reading the packet. Set the size read to the max size.
	      bytes_from_packet = MAX_PACKET_SIZE;
	    }
	  }

	  // Check if a full packet has been read in. If not, throw.
	  if (bytes_from_packet != MAX_PACKET_SIZE){
	    std::cout << "Haven't read a complete packet in. Exiting" << std::endl; 
	    exit(0);
	  };
	  /*


	    std::cout << "Ended reading the header, finished reading the data from this trigger into the fragment" << std::endl;
	    std::cout << "Read first packet, including the header the cumulative total is " << bytes_from_packet << std::endl;
	    
	    if (toOutputFile_)
	      {
		if (!read_pHdr_into_outputfile){
		  outputFileStream_.write(reinterpret_cast<char *>(&pHdr), fw_pHdr_Size);
		  read_pHdr_into_outputfile = true;
		};
		outputFileStream_.write(reinterpret_cast<char *>(&tHdr), fw_tHdr_Size);
		outputFileStream_.write(reinterpret_cast<char *>(&adcs), tHdr_adc_bytes);
	      }
	    ev_counter_inc();
	    if (bytes_from_packet < MAX_PACKET_SIZE){std::cout << "Running the loop again: bytes_from_packet<MAX_PACKET_SIZE as " << bytes_from_packet << "<" << MAX_PACKET_SIZE << std::endl;}
	  };
	  read_pHdr_into_frags = false;
	  read_pHdr_into_outputfile = false;
	  bytes_from_packet = 0;
	    */
	  }
	  else {

	    // Include multiple message UDP server

	    // Loop until timouet
	    while(udp[0].timeout_counter != udp[0].getTimeoutMax()){
	      // Get packet and check if it times out (returns 0)
	      if ((retval = udp[0].getPackets(rcv_buffer,recvSock[0],0)) <= 0){
		// Increment timeout counters
		udp[0].timeout_counter++;
	      } // Else continue...}
	      // If not timed out...
	      else{
		// Reset timeout counter
		udp[0].timeout_counter = 0;
		// Increase receive counter
		recvCount += retval;
		// Break loop
		break;
	      } // End if timeout
	    } // End loop    

	    if (udp[0].timeout_counter == udp[0].getTimeoutMax()){
	      std::cout << "UDP socket timed out!!" << std::endl;
	      std::cout << "Number of packets received = " << recvCount << std::endl;
	      return false;
	    }
	  }

	  /*
	if (toOutputFile_)
	{
	  outputFileStream_.write(reinterpret_cast<char *>(&pHdr), fw_pHdr_Size);
	  outputFileStream_.write(reinterpret_cast<char *>(&tHdr), fw_tHdr_Size);
	  outputFileStream_.write(reinterpret_cast<char *>(&adcs), tHdr_adc_bytes);
	  }*/
	packet_counter++;
	ev_counter_inc();  // increment event counter
	std::cout << "\n\nReading in the next packet. Read in " << packet_counter << " packets.\nRead data size is << " << packet_counter*MAX_PACKET_SIZE << "\nIncorrect pNums: " << incorrect_pNums << "\n\n" << std::endl;
	return true;
}
// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(mu2e::STMReceiver)
