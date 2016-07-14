#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#ifdef CANVAS
#include "canvas/Utilities/InputTag.h"
#else
#include "art/Utilities/InputTag.h"
#endif

#include "artdaq-core/Data/Fragments.hh"

#include "mu2e-artdaq-core/Overlays/FragmentType.hh"
#include "mu2e-artdaq-core/Overlays/mu2eFragment.hh"
#include "mu2e-artdaq-core/Overlays/DTCFragment.hh"
#include "dtcInterfaceLib/DTC.h"

#include "cetlib/exception.h"

#include <sstream>
#include <iomanip>


namespace mu2e
{
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
}

mu2e::DTCDataVerifier::DTCDataVerifier(fhicl::ParameterSet const& ps):
  art::EDAnalyzer(ps),
  fragment_type_labels_(ps.get<std::vector<std::string>>("fragment_type_labels")),
  next_timestamp_(0)
{
  // Throw out any duplicate fragment_type_labels_ ; in this context we only
  // care about the different types that we plan to encounter, not how
  // many of each there are

  sort(fragment_type_labels_.begin(), fragment_type_labels_.end());
  fragment_type_labels_.erase(unique(fragment_type_labels_.begin(), fragment_type_labels_.end()), fragment_type_labels_.end());
}


void mu2e::DTCDataVerifier::analyze(art::Event const& e)
{
  artdaq::Fragments fragments;

  for (auto label : fragment_type_labels_)
    {
      art::Handle<artdaq::Fragments> fragments_with_label;

      e.getByLabel("daq", label, fragments_with_label);

      //    for (int i_l = 0; i_l < static_cast<int>(fragments_with_label->size()); ++i_l) {
      //      fragments.emplace_back( (*fragments_with_label)[i_l] );
      //    }

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
	  mf::LogWarning("DTCDataVerifier") << "Expected fragment with sequence ID " << expected_sequence_id << ", received one with sequence ID " << frag.sequenceID();
	}

      FragmentType fragtype = static_cast<FragmentType>(frag.type());

      switch (fragtype)
	{
	case FragmentType::DTC:
	  {
	    auto ts = static_cast<DTCFragment>(frag).hdr_timestamp();
	    if(ts != next_timestamp_) {
	      std::ostringstream str;
	      str << std::hex << "Timestamp does not match expected: ts=0x" << static_cast<double>(ts) << ", expected=0x" << static_cast<double>(next_timestamp_);
	      mf::LogWarning("DTCDataVerifier") << str.str();
	    }
	    next_timestamp_ = ts + 1;
	  }
	  break;
	case FragmentType::MU2E:
	  {
	    auto mfrag = static_cast<mu2eFragment>(frag);
	    for(size_t ii = 0; ii < mfrag.hdr_block_count(); ++ii)
	      {
		auto ptr = const_cast<mu2eFragment::Header::data_t*>(mfrag.dataAt(ii));
		
		auto dp = DTCLib::DTC_DataPacket(reinterpret_cast<void*>(ptr));
		auto dhp = DTCLib::DTC_DataHeaderPacket(dp);
		auto ts = dhp.GetTimestamp().GetTimestamp(true);
		      
		if(ts != next_timestamp_) {
		  std::ostringstream str;
		  str << std::hex << "Timestamp does not match expected: ts=0x" << static_cast<double>(ts) << ", expected=0x" << static_cast<double>(next_timestamp_);
		  mf::LogWarning("DTCDataVerifier") << str.str();
		}
		next_timestamp_ = ts + 1;

	      }
	  }
	  break;
	default:
	  throw cet::exception("Error in DTCDataVerifier: unknown fragment type supplied");
	}
    }
}

DEFINE_ART_MODULE(mu2e::DTCDataVerifier)

