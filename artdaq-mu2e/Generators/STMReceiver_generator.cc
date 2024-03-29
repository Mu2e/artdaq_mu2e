#include "artdaq-mu2e/Generators/STMReceiver.hh"

#include "artdaq/Generators/GeneratorMacros.hh"

#include "trace.h"
#define TRACE_NAME "STMReceiver"

mu2e::STMReceiver::STMReceiver(fhicl::ParameterSet const &ps)
	: artdaq::CommandableFragmentGenerator(ps)
	, fromInputFile_(ps.get<bool>("from_input_file", false))
	, toOutputFile_(ps.get<bool>("to_output_file", false))
{
	TLOG(TLVL_DEBUG) << "STMReceiver Initialized";

	if (fromInputFile_)
	{
		auto input_file = ps.get<std::string>("input_file", "");
		inputFileStream_.open(input_file, std::ios::in | std::ios::binary);
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

bool mu2e::STMReceiver::getNext_(artdaq::FragmentPtrs &frags)
{
	if (should_stop())
	{
		return false;
	}

	if (fromInputFile_)
	{
		if (inputFileStream_.eof())
		{  // if we reached the end of the file, then stop
			return false;
		}
	}

	// STMFragment::STMDataPacket data[1]; //Andy's implementation
	STMFragment::STM_tHdr tHdr;
	STMFragment::STM_sHdr sHdr;
	uint16_t data[0x50000];  // read this from fhicl::ps. This acts as a buffer for the adc data.
	uint16_t data_size = 0;
	if (fromInputFile_)
	{
		inputFileStream_.read(reinterpret_cast<char *>(&tHdr), sw_tHdr_size_bytes);
		inputFileStream_.read(reinterpret_cast<char *>(&sHdr), sw_sHdr_size_bytes);
		data_size = sHdr.sliceSize();
		inputFileStream_.read(reinterpret_cast<char *>(&data[0]), data_size);

		double fragment_timestamp = 0;
		frags.emplace_back(new artdaq::Fragment(ev_counter(), fragment_id(), FragmentType::STM, fragment_timestamp));
		// Next two lines define an inefficient buffer implementation.
		// frags.back()->resizeBytes(0x50000);
		// memcpy(frags.back()->dataBegin(), &data[0], 0x50000);

		frags.back()->resizeBytes(sw_tHdr_size_bytes + sw_sHdr_size_bytes + data_size);
		uint8_t *dataBegin = reinterpret_cast<uint8_t *>(frags.back()->dataBegin());
		memcpy(dataBegin, &tHdr, sw_tHdr_size_bytes);
		dataBegin += sw_tHdr_size_bytes;
		memcpy(dataBegin, &sHdr, sw_sHdr_size_bytes);
		dataBegin += sw_sHdr_size_bytes;
		memcpy(dataBegin, &data[0], data_size);
	}

	// Andy's implementation
	// if (toOutputFile_) {
	// outputFileStream_.write(reinterpret_cast<char *>(&data), sizeof(data[0]));
	//}

	if (toOutputFile_)
	{
		outputFileStream_.write(reinterpret_cast<char *>(&tHdr), sw_tHdr_size_bytes);
		outputFileStream_.write(reinterpret_cast<char *>(&sHdr), sw_sHdr_size_bytes);
		outputFileStream_.write(reinterpret_cast<char *>(&data), data_size);
	}

	ev_counter_inc();  // increment event counter
	return true;
}

// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(mu2e::STMReceiver)
