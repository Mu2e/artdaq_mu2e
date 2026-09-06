#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "fhiclcpp/types/Sequence.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "artdaq-core/Data/Fragment.hh"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace mu2e {

class FragmentAggregator : public art::EDProducer
{
public:
	struct Config
	{
		fhicl::Sequence<std::string> instanceNames{
			fhicl::Name("instanceNames"),
			fhicl::Comment("Fragment product instance names to aggregate and re-emit"),
			std::vector<std::string>{"CFO", "DTCEVT", "ContainerDTCEVT"}};
		fhicl::Atom<bool> throwOnDuplicate{
			fhicl::Name("throwOnDuplicate"),
			fhicl::Comment("Throw if two input handles share an instance name (false = concatenate)"),
			true};
	};

	using Parameters = art::EDProducer::Table<Config>;

	explicit FragmentAggregator(Parameters const& params);

	void produce(art::Event& event) override;

private:
	std::set<std::string> instanceNames_;
	bool throwOnDuplicate_;
};

}  // namespace mu2e

mu2e::FragmentAggregator::FragmentAggregator(Parameters const& params)
	: art::EDProducer{params},
	  throwOnDuplicate_(params().throwOnDuplicate())
{
	auto const& names = params().instanceNames();
	for (auto const& name : names)
	{
		instanceNames_.insert(name);
		produces<artdaq::Fragments>(name);
	}

	consumesMany<artdaq::Fragments>();
}

void mu2e::FragmentAggregator::produce(art::Event& event)
{
	std::map<std::string, std::unique_ptr<artdaq::Fragments>> aggregated;
	for (auto const& name : instanceNames_)
	{
		aggregated[name] = std::make_unique<artdaq::Fragments>();
	}

	std::map<std::string, std::string> seenSources;

	auto handles = event.getMany<std::vector<artdaq::Fragment>>();

	for (auto const& handle : handles)
	{
		if (!handle.isValid() || handle->empty())
			continue;

		std::string const& instance = handle.provenance()->productInstanceName();

		if (instanceNames_.find(instance) == instanceNames_.end())
			continue;

		auto it = seenSources.find(instance);
		if (it != seenSources.end())
		{
			if (throwOnDuplicate_)
			{
				throw art::Exception(art::errors::DataCorruption)
					<< "FragmentAggregator: duplicate instance name '" << instance
					<< "' -- first from process '" << it->second
					<< "', again from '" << handle.provenance()->processName()
					<< "' (set throwOnDuplicate: false to concatenate instead)";
			}
		}
		else
		{
			seenSources[instance] = handle.provenance()->processName();
		}

		auto& dest = *aggregated[instance];
		dest.reserve(dest.size() + handle->size());
		for (auto const& frag : *handle)
		{
			dest.push_back(frag);
		}
	}

	for (auto& [name, frags] : aggregated)
	{
		event.put(std::move(frags), name);
	}
}

DEFINE_ART_MODULE(mu2e::FragmentAggregator)
