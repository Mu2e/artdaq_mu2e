#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "artdaq/DAQdata/Globals.hh"
#define TRACE_NAME "DTCDataVerifier"

#include "canvas/Utilities/InputTag.h"

#include "artdaq-core/Data/Fragment.hh"

#include "dtcInterfaceLib/DTC.h"
#include "artdaq-core-mu2e/Overlays/DTCFragment.hh"
#include "artdaq-core-mu2e/Overlays/FragmentType.hh"
#include "artdaq-core-mu2e/Overlays/mu2eFragment.hh"

#include "cetlib_except/exception.h"

#include <iomanip>
#include <sstream>

namespace mu2e {
class DTCDataVerifier : public art::EDAnalyzer
{
public:
	explicit DTCDataVerifier(fhicl::ParameterSet const& p);
	virtual ~DTCDataVerifier() = default;

	void analyze(art::Event const& e) override;

private:
	std::vector<std::string> fragment_type_labels_;
	uint64_t next_timestamp_;
};
}  // namespace mu2e

mu2e::DTCDataVerifier::DTCDataVerifier(fhicl::ParameterSet const& ps)
	: art::EDAnalyzer(ps), fragment_type_labels_(ps.get<std::vector<std::string>>("fragment_type_labels")), next_timestamp_(0)
{
	// Throw out any duplicate fragment_type_labels_ ; in this context we only
	// care about the different types that we plan to encounter, not how
	// many of each there are

	sort(fragment_type_labels_.begin(), fragment_type_labels_.end());
	fragment_type_labels_.erase(unique(fragment_type_labels_.begin(), fragment_type_labels_.end()),
								fragment_type_labels_.end());
}

void mu2e::DTCDataVerifier::analyze(art::Event const& e)
{
	artdaq::Fragments fragments;

	for (auto label : fragment_type_labels_)
	{
		art::Handle<artdaq::Fragments> fragments_with_label;

		e.getByLabel("daq", label, fragments_with_label);

		for (auto frag : *fragments_with_label)
		{
			fragments.emplace_back(frag);
		}
	}

	artdaq::Fragment::sequence_id_t expected_sequence_id = std::numeric_limits<artdaq::Fragment::sequence_id_t>::max();

	//  for (std::size_t i = 0; i < fragments.size(); ++i) {
	for (const auto& frag : fragments)
	{
		// Pointers to the types of fragment overlays DTCDataVerifier can handle;
		// only one will be used per fragment, of course

		//  const auto& frag( fragments[i] );  // Basically a shorthand

		//    if (i == 0)
		if (expected_sequence_id == std::numeric_limits<artdaq::Fragment::sequence_id_t>::max())
		{
			expected_sequence_id = frag.sequenceID();
		}

		if (expected_sequence_id != frag.sequenceID())
		{
			TLOG(TLVL_WARNING) << "Expected fragment with sequence ID " << expected_sequence_id
							   << ", received one with sequence ID " << frag.sequenceID();
		}

		FragmentType fragtype = static_cast<FragmentType>(frag.type());

		switch (fragtype)
		{
			case FragmentType::DTC: {
				auto ts = static_cast<DTCFragment>(frag).hdr_timestamp();
				if (ts != next_timestamp_)
				{
					TLOG(TLVL_WARNING) << "Timestamp does not match expected: ts=" << static_cast<double>(ts)
									   << ", expected=" << static_cast<double>(next_timestamp_);
				}
				next_timestamp_ = ts + 1;
			}
			break;
			case FragmentType::MU2E: {
				auto mfrag = static_cast<mu2eFragment>(frag);
				for (size_t ii = 0; ii < mfrag.hdr_block_count(); ++ii)
				{
					auto ptr = const_cast<mu2eFragment::Header::data_t*>(mfrag.dataAt(ii));

					auto dp = DTCLib::DTC_DataPacket(reinterpret_cast<void*>(ptr));
					auto dhp = DTCLib::DTC_DataHeaderPacket(dp);
					auto ts = dhp.GetEventWindowTag().GetEventWindowTag(true);

					if (ts != next_timestamp_ && ts != next_timestamp_ - BLOCK_COUNT_MAX)
					{
						TLOG(TLVL_WARNING) << "Block " << static_cast<double>(ii)
										   << ": Timestamp does not match expected: ts=" << static_cast<double>(ts)
										   << ", expected=" << static_cast<double>(next_timestamp_);
					}
					next_timestamp_ = ts + 1;
				}
				if (mfrag.hdr_block_count() != BLOCK_COUNT_MAX)
				{
					TLOG(TLVL_WARNING) << "There were " << mfrag.hdr_block_count()
									   << " DataBlocks, mu2eFragment capacity is " << BLOCK_COUNT_MAX;
				}
			}
			break;
			case FragmentType::EMPTY:
				TLOG(TLVL_INFO) << "Not processing Empty Fragment";
				break;
			default: {
				std::ostringstream str;
				str << "Error in DTCDataVerifier: unknown fragment type supplied (" << fragmentTypeToString(fragtype) << ")";
				throw cet::exception(str.str());
			}
		}
	}
}

DEFINE_ART_MODULE(mu2e::DTCDataVerifier)
