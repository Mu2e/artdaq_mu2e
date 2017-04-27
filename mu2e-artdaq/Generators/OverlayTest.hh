#ifndef mu2e_artdaq_Generators_OverlayTest_hh
#define mu2e_artdaq_Generators_OverlayTest_hh

// OverlayTest is a fragment generator based off of ToySimulator
// that is intended to serve as a test platform for the development
// of mu2e-specific overlay classes.
//
// OverlayTest uses the DTC interface library to fill tracker,
// calorimeter, or cosmic ray veto packets depending on the
// specified "fragment_type" (either "TRK", "CAL", or "CRV").

#include "fhiclcpp/fwd.h"
#include "artdaq-core/Data/Fragment.hh" 
#include "artdaq/Application/CommandableFragmentGenerator.hh"
#include "mu2e-artdaq-core/Overlays/FragmentType.hh"
#include "mu2e-artdaq-core/Overlays/DetectorFragment.hh"

#include <random>
#include <vector>
#include <atomic>

#include <bitset>

#include <queue>

#include "dtcInterfaceLib/DTC.h"
#include "dtcInterfaceLib/DTCSoftwareCFO.h"

namespace mu2e
{
	class OverlayTest : public artdaq::CommandableFragmentGenerator
	{
	public:
		explicit OverlayTest(fhicl::ParameterSet const& ps);
		virtual ~OverlayTest();

	private:

		std::bitset<128> bitArray(mu2e::DetectorFragment::adc_t const* beginning)
		{
			// Return 128 bit bitset filled with bits starting at the indicated position in the fragment
			std::bitset<128> theArray;
			for (int bitIdx = 127, adcIdx = 0; adcIdx < 8; adcIdx++)
			{
				for (int offset = 0; offset < 16; offset++)
				{
					if (((*((mu2e::DetectorFragment::adc_t const *)(beginning + adcIdx))) & (1 << offset)) != 0)
					{
						theArray.set(bitIdx);
					}
					else
					{
						theArray.reset(bitIdx);
					}
					bitIdx--;
				}
			}
			return theArray;
		}


		mu2e::DetectorFragment::adc_t convertFromBinary(std::bitset<128> theArray, int minIdx, int maxIdx)
		{
			std::bitset<16> retVal;
			for (int i = minIdx + 1; i <= maxIdx; i++)
			{
				retVal.set(maxIdx - i, theArray[i]);
			}
			return retVal.to_ulong();
		}


		// The "generateFragmentID" function is used to parse out the
		// ring and ROC numbers from a DTC header packet and combine
		// them to create a fragment_id.

		artdaq::Fragment::fragment_id_t generateFragmentID(DTCLib::DTC_DataHeaderPacket& thePacket);

		// The "getNext_" function is used to implement user-specific
		// functionality; it's a mandatory override of the pure virtual
		// getNext_ function declared in CommandableFragmentGenerator

		bool getNext_(artdaq::FragmentPtrs& output) override;

		void start() override {}

		void stop() override {}

		void stopNoMutex() override {}

		// Like "getNext_", "fragmentIDs_" is a mandatory override; it
		// returns a vector of the fragment IDs an instance of this class
		// is responsible for. Currently, the fragment IDs in OverlayTest
		// are just 8-bit words in which the 4 most significant bits are
		// the ring number, and the 4 least significant bits are the ROC
		// number.

		std::vector<artdaq::Fragment::fragment_id_t> fragmentIDs_()
		{
			return fragment_ids_;
		}

		// FHiCL-configurable variables. Note that the C++ variable names
		// are the FHiCL variable names with a "_" appended

		std::size_t const nADCcounts_; // ADC values per fragment per event
		FragmentType const fragment_type_; // Type of fragment (see FragmentType.hh)
		std::size_t const throttle_usecs_;

		std::vector<artdaq::Fragment::fragment_id_t> fragment_ids_;

		size_t dataIdx;
		std::vector<DTCLib::DTC_DataBlock> data;

		// State
		size_t data_packets_read_;
		size_t events_read_;
		DTCLib::DTC_SimMode mode_;

		// Debug Packet Settings
		uint16_t debugPacketCount_;
		DTCLib::DTC_DebugType debugType_;
		bool stickyDebugType_;

		DTCLib::DTC* theInterface;
		DTCLib::DTCSoftwareCFO* theCFO_;
	};
}

#endif /* mu2e_artdaq_Generators_OverlayTest_hh */

