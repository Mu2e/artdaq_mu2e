
// Sam Grant 2025
// Decode fragments and print everything
// Independent of Offline

// C++ includes
#include <iomanip>
#include <map>
#include <string>
#include <thread>

// art includes
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"

// artdaq includes
//#include "artdaq-core-mu2e/Data/CRVDataDecoder.hh"
#include "artdaq-core-mu2e/Overlays/DTCEventFragment.hh"
#include "artdaq-core-mu2e/Overlays/FragmentType.hh"
#include "artdaq-core/Data/ContainerFragment.hh"
#include "artdaq-core/Data/Fragment.hh"

namespace ots {

// Utility to convert enum values to strings for better logging
std::string subsystemToString(uint8_t subsystem) {
	switch (subsystem) {
	case 0: return "Tracker";
	case 1: return "Calorimeter";
	case 2: return "CRV";
	case 3: return "Other";
	case 4: return "STM";
	case 5: return "ExtMon";
	default: return "Unknown (" + std::to_string(subsystem) + ")";
	}
}

class DTCEVTPrint : public art::EDAnalyzer {
  public:
	// Constructor
	explicit DTCEVTPrint(fhicl::ParameterSet const &ps);
	// Destructor
	~DTCEVTPrint() override;

  private:
	// Functions
	void beginJob() override;
	void analyze(art::Event const &e) override;
	void endJob() override;

	// Parameters
	int diagLevel_;

	std::string outputPrefix_;
	size_t fragmentCounts_{0};
	size_t subEventCounts_{0};
	size_t blockCounts_{0};
	size_t eventWindowTagCounts_{0};
	size_t packetCounts_{0};
};

// Constructor implementation
DTCEVTPrint::DTCEVTPrint(fhicl::ParameterSet const &ps)
    : art::EDAnalyzer(ps),
      diagLevel_(ps.get<int>("diagLevel", 1)) {
	outputPrefix_ = "[DTCEVTPrint] ";
}

// Destructor implementation
DTCEVTPrint::~DTCEVTPrint() {}

void DTCEVTPrint::beginJob() {
	std::cout << outputPrefix_ << "Beginning job with debug level (diagLevel_) " << diagLevel_ << std::endl;
}

void DTCEVTPrint::analyze(art::Event const &e) {
	// Get fragments
	std::vector<art::Handle<artdaq::Fragments>> fragmentHandles;
	fragmentHandles = e.getMany<std::vector<artdaq::Fragment>>();
	artdaq::FragmentPtrs containerFragments;
	artdaq::Fragments fragments;

	if (diagLevel_ > 0) {
		std::cout << outputPrefix_ << "=================== " << e.id() << " ===================" << std::endl;
		std::cout << outputPrefix_ << "Number of fragment handles: " << fragmentHandles.size() << std::endl;
	}

	// Iterate through fragment handles
	for (const auto &handle : fragmentHandles) {
		// Catch invalid or empty handles
		if (!handle.isValid() || handle->empty()) {
			if (diagLevel_ > 0) {
				std::cout << outputPrefix_ << "Found invalid or empty handle" << std::endl;
			}
			continue;
		}

		if (handle->front().type() == artdaq::Fragment::ContainerFragmentType) {
			// Iterate through containers
			if (diagLevel_ > 1) {
				std::cout << outputPrefix_ << "Container fragments (ContainerFragmentType) in handle: " << handle->size() << std::endl;
			}
			for (const auto &cont : *handle) {
				artdaq::ContainerFragment contf(cont);

				if (diagLevel_ > 1) {
					std::cout << outputPrefix_ << "Container fragment type: " << contf.fragment_type() << std::endl;
					std::cout << outputPrefix_ << "Container block count: " << contf.block_count() << std::endl;
				}

				// Break if this is single fragment rather than a container
				if (contf.fragment_type() != mu2e::FragmentType::DTCEVT) {
					if (diagLevel_ > 1) {
						std::cout << outputPrefix_ << "Container fragment type is not DTCEVT" << std::endl;
					}
					break;
				}
				// Iterate through fragments in container and fill fragments vector
				for (size_t i = 0; i < contf.block_count(); ++i) {
					containerFragments.push_back(contf[i]);
					fragments.push_back(*containerFragments.back());
				}
			}
		} else if (handle->front().type() == mu2e::FragmentType::DTCEVT) { // If the first object in the handle a single fragment
			if (diagLevel_ > 1) {
				std::cout << outputPrefix_ << "DTC Event (DTCEVT) fragments in handle: " << handle->size() << std::endl;
			}
			// Iterate through fragments and fill fragments vector
			for (auto frag : *handle) {
				fragments.emplace_back(frag);
			}
		} else {
			std::cerr << outputPrefix_ << "Handle type '" << handle->front().type() << "' not recognised" << std::endl;
		}
	}

	fragmentCounts_ += fragments.size();

	// Summary counters for current event
	size_t currentEventSubEvents = 0;
	size_t currentEventBlocks = 0;
	size_t currentEventPackets = 0;

	// Handle the fragments
	for (const auto &frag : fragments) {
		try {
			mu2e::DTCEventFragment bb(frag);
			auto data = bb.getData();
			auto event = &data;

			auto EWT = event->GetEventWindowTag().GetEventWindowTag(true);

			if (diagLevel_ > 1) {
				std::cout << outputPrefix_ << "Event Window Tag: " << EWT << std::dec << std::endl;
			}
			++eventWindowTagCounts_;

			// Event header
			DTCLib::DTC_EventHeader *eventHeader = event->GetHeader();
			size_t subEventsCount = event->GetSubEventCount();
			currentEventSubEvents += subEventsCount;

			if (diagLevel_ > 1) {
				std::cout << outputPrefix_ << "Subevents count: " << subEventsCount << std::endl;
				std::cout << outputPrefix_ << eventHeader->toJson() << std::endl;
			}

			for (unsigned int iSubEvent = 0; iSubEvent < subEventsCount; ++iSubEvent) {
				// Subevent
				DTCLib::DTC_SubEvent &subevent = *(event->GetSubEvent(iSubEvent));

				// Subevent header
				const DTCLib::DTC_SubEventHeader *subeventHeader = subevent.GetHeader();
				size_t blockCount = subevent.GetDataBlockCount();

				if (diagLevel_ > 1) {
					std::cout << outputPrefix_ << "---> Subevent [" << iSubEvent << "]:" << std::endl;
					std::cout << outputPrefix_ << "Number of Data Blocks: " << blockCount << std::endl;
					std::cout << outputPrefix_ << subeventHeader->toJson() << std::endl;
				}

				currentEventBlocks += blockCount;

				for (size_t iBlock = 0; iBlock < blockCount; ++iBlock) {
					auto block = subevent.GetDataBlock(iBlock);
					auto blockheader = block->GetHeader();
					auto subsystem = blockheader->GetSubsystem();
					size_t packetCount = blockheader->GetPacketCount();
					currentEventPackets += packetCount;

					if (diagLevel_ > 1) {
						std::cout << outputPrefix_ << "---> Block [" << iBlock << "]:" << std::endl;
						std::cout << outputPrefix_ << "Packet Count: " << blockheader->GetPacketCount() << std::endl;
						std::cout << outputPrefix_ << blockheader->toJSON() << std::endl;
						std::cout << outputPrefix_ << "Block details:" << std::endl
						          << outputPrefix_ << "  Subsystem: " << subsystemToString(subsystem) << std::endl
						          << outputPrefix_ << "  Valid: " << (blockheader->isValid() ? "Yes" : "No") << std::endl
						          << outputPrefix_ << "  Version: 0x" << std::hex << (int)blockheader->GetVersion() << std::dec << std::endl
						          << outputPrefix_ << "  DTC ID: " << (int)blockheader->GetID() << std::endl
						          << outputPrefix_ << "  Byte Count: " << block->byteSize << std::endl
						          << outputPrefix_ << "  Event Window Tag: " << blockheader->GetEventWindowTag().GetEventWindowTag(true) << std::endl; // " (0x"

						if (diagLevel_ > 2) {
							for (int iPacket = 0; iPacket < blockheader->GetPacketCount(); ++iPacket) {
								std::cout << outputPrefix_ << "---> Packet [" << iPacket << "]: " << DTCLib::DTC_DataPacket(((uint8_t *)block->blockPointer) + ((iPacket + 1) * 16)).toJSON() << std::endl;
							}
						}
					}

					// Make sure we only process CRV data
					/*
					if (blockheader->GetSubsystem() == DTCLib::DTC_Subsystem_CRV) {
					}*/
				}
			}
		} catch (const std::exception &e) {
			if (diagLevel_ > 0) {
				std::cerr << outputPrefix_ << "Error processing fragment: " << e.what() << std::endl;
			}
			continue;
		}
	}

	// Print summary for this event
	if (diagLevel_ > 0) {
		std::cout << outputPrefix_ << "Event Summary: "
		          << currentEventSubEvents << " SubEvents; "
		          << currentEventBlocks << " Blocks; "
		          << currentEventPackets << " Packets; "
		          << std::endl;
	}

	subEventCounts_ += currentEventSubEvents;
	blockCounts_ += currentEventBlocks;
	packetCounts_ += currentEventPackets;
}

void DTCEVTPrint::endJob() {
	// Print job-level statistics
	std::cout << outputPrefix_ << "================= Job Summary =================" << std::endl;
	std::cout << outputPrefix_ << "Total Fragments: " << fragmentCounts_ << std::endl;
	std::cout << outputPrefix_ << "Total SubEvents: " << subEventCounts_ << std::endl;
	std::cout << outputPrefix_ << "Total Blocks: " << blockCounts_ << std::endl;
	std::cout << outputPrefix_ << "===============================================" << std::endl;
}

DEFINE_ART_MODULE(DTCEVTPrint)
} // namespace ots
