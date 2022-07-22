#include "art/Framework/Principal/Event.h"
#include "art/Framework/Services/Registry/ActivityRegistry.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Registry/ServiceMacros.h"
#include "art/Framework/Services/Registry/ServiceTable.h"
#include "art/Framework/Services/System/DatabaseConnection.h"
#include "art/Persistency/Provenance/ModuleContext.h"
#include "art/Persistency/Provenance/ModuleDescription.h"
#include "art/Persistency/Provenance/PathContext.h"
#include "art/Persistency/Provenance/ScheduleContext.h"
#include "art/Utilities/Globals.h"
#include "art/Utilities/PerScheduleContainer.h"
#include "boost/format.hpp"
#include "canvas/Persistency/Provenance/EventID.h"
#include "cetlib/HorizontalRule.h"
#include "cetlib/sqlite/Connection.h"
#include "cetlib/sqlite/Ntuple.h"
#include "cetlib/sqlite/helpers.h"
#include "cetlib/sqlite/statistics.h"
#include "fhiclcpp/types/Atom.h"
#include "fhiclcpp/types/Name.h"
#include "fhiclcpp/types/Table.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "tbb/concurrent_unordered_map.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "artdaq/DAQdata/Globals.hh"

using namespace cet;
using namespace hep::concurrency;

using std::chrono::steady_clock;

using namespace art;

namespace art {

using ConcurrentKey = std::pair<ScheduleID, std::string>;
struct ConcurrentKeyHasher
{
	size_t
	operator()(ConcurrentKey const& key) const
	{
		static std::hash<ScheduleID> schedule_hasher{};
		static std::hash<std::string> string_hasher{};
		// A better hash will be desirable if this becomes a bottleneck.
		return schedule_hasher(key.first) ^ string_hasher(key.second);
	}
};

auto key(ScheduleID const sid)
{
	return ConcurrentKey{sid, {}};
}
auto key(ModuleContext const& mc)
{
	return ConcurrentKey{mc.scheduleID(), mc.moduleLabel()};
}

auto now = bind(&steady_clock::now);

struct Statistics
{
	explicit Statistics() = default;

	explicit Statistics(std::string const& p,
						std::string const& label,
						std::string const& type,
						sqlite3* const db,
						std::string const& table,
						std::string const& column)
		: path{p}
		, mod_label{label}
		, mod_type{type}
		, min{sqlite::min(db, table, column)}
		, mean{sqlite::mean(db, table, column)}
		, max{sqlite::max(db, table, column)}
		, median{sqlite::median(db, table, column)}
		, rms{sqlite::rms(db, table, column)}
		, n{sqlite::nrows(db, table)}
	{}

	std::string path{};
	std::string mod_label{};
	std::string mod_type{};
	double min{-1.};
	double mean{-1.};
	double max{-1.};
	double median{-1.};
	double rms{-1.};
	unsigned n{0u};
};

std::ostream&
operator<<(std::ostream& os, Statistics const& info)
{
	std::string label{info.path};
	if (!info.mod_label.empty())
	{
		label += ':' + info.mod_label;
	}
	if (!info.mod_type.empty())
	{
		label += ':' + info.mod_type;
	}
	os << label << "  " << boost::format(" %=12g ") % info.min
	   << boost::format(" %=12g ") % info.mean
	   << boost::format(" %=12g ") % info.max
	   << boost::format(" %=12g ") % info.median
	   << boost::format(" %=12g ") % info.rms
	   << boost::format(" %=10d ") % info.n;
	return os;
}

}  // namespace art

/**
   * \brief Interface for ArtdaqFragmentNamingService. This interface is declared to art as part of the required registration of an art Service
   */
class ArtdaqTimeTrackerServiceInterface
{
public:
	//static constexpr bool service_handle_allowed{false};

	struct Config
	{
		fhicl::Atom<bool> printSummary{fhicl::Name{"printSummary"}, true};
		struct DBoutput
		{
			fhicl::Atom<std::string> filename{fhicl::Name{"filename"}, ""};
			fhicl::Atom<bool> overwrite{fhicl::Name{"overwrite"}, false};
		};
		fhicl::Table<DBoutput> dbOutput{fhicl::Name{"dbOutput"}};
	};
	using Parameters = ServiceTable<Config>;
	ArtdaqTimeTrackerServiceInterface(Parameters const& config, ActivityRegistry& areg)
		: printSummary_{config().printSummary()}
		, db_{ServiceHandle<DatabaseConnection>{}->get(
			  config().dbOutput().filename() + "_" + app_name + ".csv")}
		, overwriteContents_{config().dbOutput().overwrite()}
		, timeSourceColumnNames_{{"Run", "SubRun", "Event", "Source", "Time"}}
		, timeEventColumnNames_{{"Run", "SubRun", "Event", "Time"}}
		, timeModuleColumnNames_{{"Run",
								  "SubRun",
								  "Event",
								  "Path",
								  "ModuleLabel",
								  "ModuleType",
								  "Time"}}
		, timeSourceTable_{*db_,
						   "TimeSource",
						   timeSourceColumnNames_,
						   overwriteContents_}
		, timeEventTable_{*db_,
						  "TimeEvent",
						  timeEventColumnNames_,
						  overwriteContents_}
		, timeModuleTable_{*db_,
						   "TimeModule",
						   timeModuleColumnNames_,
						   overwriteContents_}
	{
		areg.sPostSourceConstruction.watch(this,
										   &ArtdaqTimeTrackerServiceInterface::postSourceConstruction);
		areg.sPostEndJob.watch(this, &ArtdaqTimeTrackerServiceInterface::postEndJob);
		// Event reading
		areg.sPreSourceEvent.watch(this, &ArtdaqTimeTrackerServiceInterface::preEventReading);
		areg.sPostSourceEvent.watch(this, &ArtdaqTimeTrackerServiceInterface::postEventReading);
		// Event execution
		areg.sPreProcessEvent.watch(this, &ArtdaqTimeTrackerServiceInterface::preEventProcessing);
		areg.sPostProcessEvent.watch(this, &ArtdaqTimeTrackerServiceInterface::postEventProcessing);
		// Module execution
		areg.sPreModule.watch(this, &ArtdaqTimeTrackerServiceInterface::startTime);
		areg.sPostModule.watch(
			[this](auto const& mc) { this->recordTime(mc, ""s); });
		areg.sPreWriteEvent.watch(this, &ArtdaqTimeTrackerServiceInterface::startTime);
		areg.sPostWriteEvent.watch(
			[this](auto const& mc) { this->recordTime(mc, "(write)"s); });
	}

	virtual ~ArtdaqTimeTrackerServiceInterface() = default;

private:
	struct PerScheduleData
	{
		EventID eventID;
		steady_clock::time_point eventStart;
		steady_clock::time_point moduleStart;
	};
	template<unsigned SIZE>
	using name_array = cet::sqlite::name_array<SIZE>;
	using timeSource_t =
		cet::sqlite::Ntuple<uint32_t, uint32_t, uint32_t, std::string, double>;
	using timeEvent_t =
		cet::sqlite::Ntuple<uint32_t, uint32_t, uint32_t, double>;
	using timeModule_t = cet::sqlite::
		Ntuple<uint32_t, uint32_t, uint32_t, std::string, std::string, std::string, double>;

	void postSourceConstruction(art::ModuleDescription const&);
	void postEndJob();
	void preEventReading(ScheduleContext);
	void postEventReading(Event const&, ScheduleContext);
	void preEventProcessing(Event const&, ScheduleContext);
	void postEventProcessing(Event const&, ScheduleContext);
	void startTime(ModuleContext const& mc);
	void recordTime(ModuleContext const& mc, std::string const& suffix);
	void logToDestination_(Statistics const& evt,
						   std::vector<Statistics> const& modules);

	tbb::concurrent_unordered_map<ConcurrentKey, PerScheduleData, ConcurrentKeyHasher> data_;
	bool const printSummary_;
	std::unique_ptr<cet::sqlite::Connection> const db_;
	bool const overwriteContents_;
	std::string sourceType_{};
	name_array<5u> const timeSourceColumnNames_;
	name_array<4u> const timeEventColumnNames_;
	name_array<7u> const timeModuleColumnNames_;
	timeSource_t timeSourceTable_;
	timeEvent_t timeEventTable_;
	timeModule_t timeModuleTable_;
};

void ArtdaqTimeTrackerServiceInterface::postEndJob()
{
	timeSourceTable_.flush();
	timeEventTable_.flush();
	timeModuleTable_.flush();
	if (!printSummary_)
	{
		return;
	}
	using namespace cet::sqlite;
	query_result<size_t> rEvents;
	rEvents << select("count(*)").from(*db_, timeEventTable_.name());
	query_result<size_t> rModules;
	rModules << select("count(*)").from(*db_, timeModuleTable_.name());
	auto const nEventRows = unique_value(rEvents);
	auto const nModuleRows = unique_value(rModules);
	if ((nEventRows == 0) && (nModuleRows == 0))
	{
		logToDestination_(Statistics{}, std::vector<Statistics>{});
		return;
	}
	if (nEventRows == 0 && nModuleRows != 0)
	{
		std::string const errMsg{
			"Malformed ArtdaqTimeTrackerServiceInterface database.  The TimeEvent table is empty, but\n"
			"the TimeModule table is not.  This can happen if an exception has\n"
			"been thrown from a module while processing the first event.  Any\n"
			"saved database file is suspect and should not be used."};
		mf::LogAbsolute("ArtdaqTimeTrackerServiceInterface") << errMsg;
		return;
	}
	// Gather statistics for full Event
	// -- Unfortunately, this is not a simple query since the (e.g.)
	//    'RootOutput(write)' times and the source time are not
	//    recorded in the TimeEvent rows.  They must be added in.
	std::string const fullEventTime_ddl =
		"CREATE TABLE temp.fullEventTime AS "
		"SELECT Run,Subrun,Event,SUM(Time) AS FullEventTime FROM ("
		"       SELECT Run,Subrun,Event,Time FROM TimeEvent"
		"       UNION"
		"       SELECT Run,Subrun,Event,Time FROM TimeModule WHERE ModuleType "
		"LIKE '%(write)'"
		"       UNION"
		"       SELECT Run,Subrun,Event,Time FROM TimeSource"
		") GROUP BY Run,Subrun,Event";
	using namespace cet::sqlite;
	exec(*db_, fullEventTime_ddl);
	Statistics const evtStats{
		"Full event", "", "", *db_, "temp.fullEventTime", "FullEventTime"};
	drop_table(*db_, "temp.fullEventTime");
	query_result<std::string, std::string, std::string> r;
	r << select_distinct("Path", "ModuleLabel", "ModuleType")
			 .from(*db_, timeModuleTable_.name());
	std::vector<Statistics> modStats;
	modStats.emplace_back(
		"source", sourceType_ + "(read)", "", *db_, "TimeSource", "Time");
	for (auto const& row : r)
	{
		auto const& [path, mod_label, mod_type] = row;
		create_table_as("temp.tmpModTable",
						select("*")
							.from(*db_, "TimeModule")
							.where("Path='"s + path + "'"s + " AND ModuleLabel='"s +
								   mod_label + "'"s + " AND ModuleType='"s +
								   mod_type + "'"s));
		modStats.emplace_back(
			path, mod_label, mod_type, *db_, "temp.tmpModTable", "Time");
		drop_table(*db_, "temp.tmpModTable");
	}
	logToDestination_(evtStats, modStats);
}

void ArtdaqTimeTrackerServiceInterface::postSourceConstruction(art::ModuleDescription const& md)
{
	sourceType_ = md.moduleName();
}

void ArtdaqTimeTrackerServiceInterface::preEventReading(ScheduleContext const sc)
{
	auto& d = data_[key(sc.id())];
	d.eventID = EventID::invalidEvent();
	d.eventStart = now();
}

void ArtdaqTimeTrackerServiceInterface::postEventReading(Event const& e, ScheduleContext const sc)
{
	auto& d = data_[key(sc.id())];
	d.eventID = e.id();
	auto const t = std::chrono::duration<double>{now() - d.eventStart}.count();
	timeSourceTable_.insert(
		d.eventID.run(), d.eventID.subRun(), d.eventID.event(), sourceType_, t);
}

void ArtdaqTimeTrackerServiceInterface::preEventProcessing(Event const& e [[maybe_unused]],
														   ScheduleContext const sc)
{
	auto& d = data_[key(sc.id())];
	assert(d.eventID == e.id());
	d.eventStart = now();
}

void ArtdaqTimeTrackerServiceInterface::postEventProcessing(Event const&, ScheduleContext const sc)
{
	auto const& d = data_[key(sc.id())];
	auto const t = std::chrono::duration<double>{now() - d.eventStart}.count();
	timeEventTable_.insert(
		d.eventID.run(), d.eventID.subRun(), d.eventID.event(), t);
}

void ArtdaqTimeTrackerServiceInterface::startTime(ModuleContext const& mc)
{
	data_[key(mc)].eventID = data_[key(mc.scheduleID())].eventID;
	data_[key(mc)].moduleStart = now();
}

void ArtdaqTimeTrackerServiceInterface::recordTime(ModuleContext const& mc, std::string const& suffix)
{
	auto const& d = data_[key(mc)];
	auto const t = std::chrono::duration<double>{now() - d.moduleStart}.count();
	timeModuleTable_.insert(d.eventID.run(),
							d.eventID.subRun(),
							d.eventID.event(),
							mc.pathName(),
							mc.moduleLabel(),
							mc.moduleName() + suffix,
							t);
}

void ArtdaqTimeTrackerServiceInterface::logToDestination_(Statistics const& evt,
														  std::vector<Statistics> const& modules)
{
	size_t width{30};
	auto identifier_size = [](Statistics const& s) {
		return s.path.size() + s.mod_label.size() + s.mod_type.size() +
			   2;  // Don't forget the two ':'s.
	};
	cet::for_all(modules, [&identifier_size, &width](auto const& mod) {
		width = std::max(width, identifier_size(mod));
	});
	std::ostringstream msgOss;
	HorizontalRule const rule{width + 4 + 5 * 14 + 12};
	msgOss << '\n'
		   << rule('=') << '\n'
		   << std::setw(width + 2) << std::left << "ArtdaqTimeTrackerServiceInterface printout (sec)"
		   << boost::format(" %=12s ") % "Min"
		   << boost::format(" %=12s ") % "Avg"
		   << boost::format(" %=12s ") % "Max"
		   << boost::format(" %=12s ") % "Median"
		   << boost::format(" %=12s ") % "RMS"
		   << boost::format(" %=10s ") % "nEvts"
		   << "\n";
	msgOss << rule('=') << '\n';
	if (evt.n == 0u)
	{
		msgOss << "[ No processed events ]\n";
	}
	else
	{
		// N.B. setw(width) applies to the first field in
		//      std::ostream& operator<<(std::ostream&, Statistics const&).
		msgOss << std::setw(width) << evt << '\n'
			   << rule('-') << '\n';
		for (auto const& mod : modules)
		{
			msgOss << std::setw(width) << mod << '\n';
		}
	}
	msgOss << rule('=');
	mf::LogAbsolute("ArtdaqTimeTrackerServiceInterface") << msgOss.str();
}

//   // =======================================================================
//   class ArtdaqTimeTrackerService : public ArtdaqTimeTrackerServiceInterface {
//   public:
//   /**
//   * \brief DefaultArtdaqFragmentNamingService Destructor
//   */
//   virtual ~ArtdaqTimeTrackerService() = default;

//   /**
//    * \brief ArtdaqTimeTrackerService Constructor
//    */
//   ArtdaqTimeTrackerService(ServiceTable<Config> const &, art::ActivityRegistry&);

// };  // ArtdaqTimeTrackerService

// ======================================================================
// std::ostream&
// operator<<(std::ostream& os, Statistics const& info)
// {
//   std::string label {info.path};
//   if (info.path != "Full event")
//     label += ":"s + info.mod_label + ":"s + info.mod_type;
//   os << label << "  "
//      << boost::format(" %=12g ") % info.min
//      << boost::format(" %=12g ") % info.mean
//      << boost::format(" %=12g ") % info.max
//      << boost::format(" %=12g ") % info.median
//      << boost::format(" %=12g ") % info.rms
//      << boost::format(" %=10d ") % info.n;
//   return os;
// }

DECLARE_ART_SERVICE_INTERFACE(ArtdaqTimeTrackerServiceInterface, LEGACY)

//#endif /* artdaq_mu2e_ArtModules_ArtdaqTimeTrackerService_h */
