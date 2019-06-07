#ifndef mu2e_artdaq_ArtModules_detail_CurrentFragment_hh
#define mu2e_artdaq_ArtModules_detail_CurrentFragment_hh

#include <cassert>
#include "artdaq-core/Data/Fragment.hh"
#include "dtcInterfaceLib/DTC_Types.h"
#include "mu2e-artdaq-core/Overlays/mu2eFragment.hh"

namespace mu2e {
namespace detail {
/// <summary>
/// Helper class for processing artdaq Fragments into art Events
/// </summary>
class CurrentFragment
{
public:
	CurrentFragment() = default;                                  ///< Default Constructor
	CurrentFragment& operator=(CurrentFragment const&) = delete;  ///< Copy Assignment Operator is deleted
	CurrentFragment& operator=(CurrentFragment&&) = default;      ///< Default Move Assignment Operator
	virtual ~CurrentFragment()
	{
		reader_.reset(nullptr);
		fragment_.reset(nullptr);
	}  ///< CurrentFragment Destructor

	// This is using pass-by-value so that we can take advantage of
	// artdaq::Fragment's move c'tor.
	/// <summary>
	/// Constructor
	/// </summary>
	/// <param name="f">Fragment to process</param>
	CurrentFragment(artdaq::Fragment f);

	/// <summary>
	/// Get the count of blocks processed from the artdaq Fragment
	/// </summary>
	/// <returns>Number of blocks processed</returns>
	std::size_t processedSuperBlocks() const { return block_count_; }

	/// <summary>
	/// Advance the reader to the next block
	/// </summary>
	void advanceOneBlock()
	{
		assert(reader_);
		current_ = reinterpret_cast<const uint8_t*>(reader_->dataAt(++block_count_));
	}

	/// <summary>
	/// Determine if the artdaq Fragment contains no more blocks
	/// </summary>
	/// <returns>True if the artdaq Fragment is now empty</returns>
	bool empty() const
	{
		if (!reader_ || !fragment_) return true;
		return block_count_ >= reader_->hdr_block_count();
	}

	/// <summary>
	/// Get the number of blocks remaining in the artdaq Fragment
	/// </summary>
	/// <returns></returns>
	size_t sizeRemaining() { return reader_->hdr_block_count() - block_count_; }

	/// <summary>
	/// Retrieve the subsystem fragments from the current block
	/// </summary>
	/// <param name="sub">Subsystem to extract blocks for</param>
	/// <returns>Pointer to Fragments list</returns>
	std::unique_ptr<artdaq::Fragments> extractFragmentsFromBlock(DTCLib::DTC_Subsystem sub);
	/// <summary>
	/// Get the number of Fragments for the given subsystem in the current block
	/// </summary>
	/// <param name="sub">Subsystem to look for</param>
	/// <returns>Number of Fragments in block</returns>
	size_t getFragmentCount(DTCLib::DTC_Subsystem sub);

private:
	std::unique_ptr<artdaq::Fragment const> fragment_{nullptr};
	std::unique_ptr<mu2eFragment const> reader_{nullptr};
	uint8_t const* current_{nullptr};
	size_t block_count_;
};

}  // namespace detail
}  // namespace mu2e

#endif /* mu2e_artdaq_core_Overlays_Offline_detail_CurrentFragment_hh */
