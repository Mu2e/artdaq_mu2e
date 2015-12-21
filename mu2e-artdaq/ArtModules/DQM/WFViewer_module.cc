
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Run.h"
#include "art/Utilities/InputTag.h"
#include "art/Framework/Core/ModuleMacros.h"

#include "artdaq-core/Data/Fragments.hh"

#include "mu2e-artdaq-core/Overlays/FragmentType.hh"

#include "cetlib/exception.h"

#include "TRootCanvas.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TAxis.h"
#include "TH1D.h"
#include "TStyle.h"

#include <numeric>
#include <vector>
#include <functional>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <initializer_list>
#include <limits>

using std::cout;
using std::cerr;
using std::endl;

namespace mu2e {

  class WFViewer : public art::EDAnalyzer {

  public:
    explicit WFViewer (fhicl::ParameterSet const & p);
    virtual ~WFViewer () = default;

    void analyze (art::Event const & e) override;
    void beginRun(art::Run const &) override;

  private:

    std::unique_ptr<TCanvas> canvas_[2];
    std::vector<Double_t> x_;
    int prescale_;
    bool digital_sum_only_;
    art::RunNumber_t current_run_;

    std::size_t num_x_plots_;
    std::size_t num_y_plots_;

    std::vector<std::string> fragment_type_labels_;
    std::vector<artdaq::Fragment::fragment_id_t> fragment_ids_;

    std::vector<std::unique_ptr<TGraph>> graphs_;
    std::vector<std::unique_ptr<TH1D>> histograms_;

    std::map<artdaq::Fragment::fragment_id_t, std::size_t> id_to_index_;
  };

}

mu2e::WFViewer::WFViewer (fhicl::ParameterSet const & ps): 
  art::EDAnalyzer(ps), 
  prescale_(ps.get<int> ("prescale")), 
  digital_sum_only_(ps.get<bool> ("digital_sum_only", false)), 
  current_run_(0), 
  num_x_plots_(ps.get<std::size_t>("num_x_plots", std::numeric_limits<std::size_t>::max() )),
  num_y_plots_(ps.get<std::size_t>("num_y_plots", std::numeric_limits<std::size_t>::max() )),
  fragment_type_labels_(ps.get<std::vector<std::string>>("fragment_type_labels")),
  fragment_ids_(ps.get<std::vector<artdaq::Fragment::fragment_id_t> >("fragment_ids")),
  graphs_( fragment_ids_.size() ), 
  histograms_( fragment_ids_.size() )
{

   if (num_x_plots_ == std::numeric_limits<std::size_t>::max() ||
       num_y_plots_ == std::numeric_limits<std::size_t>::max() ) {

     switch ( fragment_ids_.size() ) {
     case 1: num_x_plots_ = num_y_plots_ = 1; break;
     case 2: num_x_plots_ = 2; num_y_plots_ = 1; break;
     case 3:
     case 4: num_x_plots_ = 2; num_y_plots_ = 2; break;
     case 5: 
     case 6:  num_x_plots_ = 3; num_y_plots_ = 2; break;
     case 7:
     case 8: num_x_plots_ = 4; num_y_plots_ = 2; break;
     default: 
       num_x_plots_ = num_y_plots_ = static_cast<std::size_t>( ceil( sqrt( fragment_type_labels_.size() ) ) );
     }

   }

   // id_to_index_ will translate between a fragment's ID and where in
   // the vector of graphs and histograms it's located

   for (std::size_t i_f = 0; i_f < fragment_ids_.size(); ++i_f) {
     id_to_index_[ fragment_ids_[i_f] ] = i_f;
   }


  // Throw out any duplicate fragment_type_labels_ ; in this context we only
  // care about the different types that we plan to encounter, not how
  // many of each there are

  sort( fragment_type_labels_.begin(), fragment_type_labels_.end() );
  fragment_type_labels_.erase( unique( fragment_type_labels_.begin(), fragment_type_labels_.end() ), fragment_type_labels_.end() );

  gStyle->SetOptStat("irm");
  gStyle->SetMarkerStyle(22);
  gStyle->SetMarkerColor(4);
}


void mu2e::WFViewer::analyze (art::Event const & e) {

  static std::size_t evt_cntr = -1;
  evt_cntr++;

  // John F., 1/22/14 -- there's probably a more elegant way of
  // collecting fragments of various types using ART interface code;
  // will investigate. Right now, we're actually re-creating the
  // fragments locally

  artdaq::Fragments fragments;

  for (auto label: fragment_type_labels_) {

    art::Handle<artdaq::Fragments> fragments_with_label;

    e.getByLabel ("daq", label, fragments_with_label);
    
    //    for (int i_l = 0; i_l < static_cast<int>(fragments_with_label->size()); ++i_l) {
    //      fragments.emplace_back( (*fragments_with_label)[i_l] );
    //    }

    for (auto frag : *fragments_with_label) { 
      fragments.emplace_back( frag);
    }
  }

  // John F., 1/5/14 

  // Here, we loop over the fragments passed to the analyze
  // function. A warning is flashed if either (A) the fragments aren't
  // all from the same event, or (B) there's an unexpected number of
  // fragments given the number of boardreaders and the number of
  // fragments per board

  // For every Nth event, where N is the "prescale" setting, plot the
  // distribution of ADC counts from each board_id / fragment_id
  // combo. Also, if "digital_sum_only" is set to false in the FHiCL
  // string, then plot, for the Nth event, a graph of the ADC values
  // across all channels in each board_id / fragment_id combo

  artdaq::Fragment::sequence_id_t expected_sequence_id = std::numeric_limits<artdaq::Fragment::sequence_id_t>::max();

  //  for (std::size_t i = 0; i < fragments.size(); ++i) {
  for (const auto& frag : fragments ) {

    // Pointers to the types of fragment overlays WFViewer can handle;
    // only one will be used per fragment, of course

    //  const auto& frag( fragments[i] );  // Basically a shorthand

    //    if (i == 0) 
    if (expected_sequence_id == std::numeric_limits<artdaq::Fragment::sequence_id_t>::max()) { 
      expected_sequence_id = frag.sequenceID();
    }

    if (expected_sequence_id != frag.sequenceID()) {
      cerr << "Warning in WFViewer: expected fragment with sequence ID " << expected_sequence_id << ", received one with sequence ID " << frag.sequenceID() << endl;
    }
    
    FragmentType fragtype = static_cast<FragmentType>( frag.type() );
    std::size_t max_adc_count = std::numeric_limits<std::size_t>::max();
    std::size_t total_adc_values = std::numeric_limits<std::size_t>::max();

    // John F., 1/22/14 -- this should definitely be improved; I'm
    // just using the max # of bits per ADC value for a given fragment
    // type as is currently defined for the V172x fragments (as
    // opposed to the Toy fragment, which have this value in their
    // metadata). Since it's not using external variables for this
    // quantity, this would need to be edited should these values
    // change.

    switch ( fragtype ) {

    default: 
      throw cet::exception("Error in WFViewer: unknown fragment type supplied");
    }

    artdaq::Fragment::fragment_id_t fragment_id = frag.fragmentID();
    std::size_t ind = id_to_index_[ fragment_id ];


    // If a histogram doesn't exist for this board_id / fragment_id combo, create it

    if (!histograms_[ind]) {

      histograms_[ind] = std::unique_ptr<TH1D>(new TH1D( Form ("Fragment_%d_hist", fragment_id), "", max_adc_count+1, -0.5, max_adc_count + 0.5));

      histograms_[ind]->SetTitle (Form ("Frag %d, Type %s", fragment_id, 
				       fragmentTypeToString( fragtype  ).c_str() ) );
      histograms_[ind]->GetXaxis()->SetTitle("ADC value");
    }

    // For every event, fill the histogram (prescale is ignored here)

    // Is there some way to templatize an ART module? If not, we're
    // stuck with this switch code...

    switch ( fragtype ) {
    default: 
      throw cet::exception("Error in WFViewer: unknown fragment type supplied");
    }

    if (evt_cntr % prescale_ - 1) {
      continue;
    }

    // If we pass the prescale, then if we're not going with
    // digital_sum_only, plot the ADC counts for this particular event/board/fragment_id
    
    if (!digital_sum_only_) {

      // Create the graph's x-axis

      if (x_.size () != total_adc_values) {
	x_.resize (total_adc_values);

	std::iota (x_.begin (), x_.end (), 0);
      }

      // If the graph doesn't exist, create it. Not sure whether to
      // make it an error if the total_adc_values is new

      if (!graphs_[ind] || static_cast<std::size_t>( graphs_[ind]->GetN () ) != total_adc_values) {
	graphs_[ind] = std::unique_ptr<TGraph>(new TGraph (total_adc_values));
	graphs_[ind]->SetName( Form ("Fragment_%d_graph", fragment_id));
	graphs_[ind]->SetLineColor ( 4 );
	std::copy (x_.begin (), x_.end (), graphs_[ind]->GetX ());
      }

      // Get the data from the fragment

      // Is there some way to templatize an ART module? If not, we're stuck with this awkward switch code...

      switch ( fragtype ) {
      default: 
	throw cet::exception("Error in WFViewer: unknown fragment type supplied");
      }


      // And now prepare the graphics without actually drawing anything yet
      
      canvas_[1] -> cd(ind+1);
      TVirtualPad* pad = static_cast<TVirtualPad*>(canvas_[1]->GetPad(ind+1) ); 

      Double_t lo_x, hi_x, lo_y, hi_y, dummy;

      graphs_[ind]->GetPoint(0, lo_x, dummy);
      graphs_[ind]->GetPoint( graphs_[ind]->GetN()-1, hi_x, dummy);

      lo_x -= 0.5;
      hi_x += 0.5;

      lo_y = -0.5;
      hi_y = max_adc_count+0.5;


      TH1F* padframe = static_cast<TH1F*>( pad->DrawFrame( lo_x, lo_y, hi_x, hi_y ) );
      padframe->SetTitle( Form ("Frag %d, Type %s, SeqID %d", static_cast<int>(fragment_id), 
				fragmentTypeToString( fragtype  ).c_str(), 
				static_cast<int>(expected_sequence_id)) );
      padframe->GetXaxis()->SetTitle("ADC #");
      pad->SetGrid();
      padframe->Draw("SAME");

    }

    // Draw the histogram

    canvas_[0]->cd(ind+1);
    histograms_[ind]->Draw();

    canvas_[0] -> Modified();
    canvas_[0] -> Update();

    // And, if desired, the Nth event's ADC counts
  
    if (!digital_sum_only_) {
      canvas_[1]->cd(ind+1);
      graphs_[ind]->Draw("PSAME");

      canvas_[1] -> Modified();
      canvas_[1] -> Update();
    }

  }
}

void mu2e::WFViewer::beginRun(art::Run const &e) { 
  if (e.run () == current_run_) return;
  current_run_ = e.run ();

  for (int i = 0; i < 2; i++) canvas_[i] = 0;
  for (auto &x: graphs_) x = 0;
  for (auto &x: histograms_) x = 0;

  for (int i = 0; (i < 2 && !digital_sum_only_) || i < 1 ; i++) {
    canvas_[i] = std::unique_ptr<TCanvas>(new TCanvas(Form("wf%d",i)));
    canvas_[i]->Divide (num_x_plots_, num_y_plots_);
    canvas_[i]->Update ();
    ((TRootCanvas*)canvas_[i]->GetCanvasImp ())->DontCallClose ();
  }

  canvas_[0]->SetTitle("ADC Value Distribution");

  if (! digital_sum_only_) {
    canvas_[1]->SetTitle("ADC Values, Event Snapshot");
  }
}


DEFINE_ART_MODULE(mu2e::WFViewer)
