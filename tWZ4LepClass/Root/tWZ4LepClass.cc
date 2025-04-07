#include "tWZ4LepClass/tWZ4LepClass.h"

#include "FastFrames/DefineHelpers.h"
#include "FastFrames/UniqueSampleID.h"

#include "Math/GenVector/Boost.h"
#include "Math/Math.h"
#include <Math/VectorUtil.h>
#include <TLorentzVector.h>
#include <ROOT/RVec.hxx>

using ROOT::VecOps::RVec;
using ROOT::VecOps::Take;
using ROOT::Math::Boost;

ROOT::RDF::RNode tWZ4LepClass::defineVariables(
  ROOT::RDF::RNode mainNode,
  const std::shared_ptr<Sample>& /*sample*/,
  const UniqueSampleID& /*id*/) 
{

  // You can also use the UniqueSampleID object to apply a custom defione
  // based on the sample and the subsample
  //   sample->name(): is the name of the sample defined in the config
  //   id.dsid() returns sample DSID
  //   id.campaign() returns sample campaign
  //   id.simulation() return simulation flavour
  // You can use it in your functions to apply only per sample define

  // If running ML model
  // Function
  auto skipEvent = [](
      const unsigned long long& eventNumber
  ) {
    int result = 0;
    if (eventNumber % 2 != 0) { result = 1; }
  
    return result;
  };
  // Variable
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "skipEvent_NOSYS",
      skipEvent,
      {"eventNumber"}
  );



  /* 
    ======================================================
        Create a pdgID variable for electrons and muons 
    ======================================================
  */
  // Function
  auto electronPdgID = [] (
    const std::vector<float>& el_charge
  ) {
    const int size = el_charge.size();

    std::vector<float> result;
    for(int i = 0; i<size; i++){
      result.push_back( 11. * -1. * el_charge.at(i) );
    }

    return result;
  };
  // Variable - Electron
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "el_pdgID_NOSYS",
      electronPdgID,
      {"el_charge"}
  );
  // Function
  auto muonPdgID = [] (
    const std::vector<float>& mu_charge
  ) {
    const int size = mu_charge.size();

    std::vector<float> result;
    for(int i = 0; i<size; i++){
      result.push_back( 13. * -1. * mu_charge.at(i) );
    }

    return result;
  };
  // Variable - Muon
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "mu_pdgID_NOSYS",
      muonPdgID,
      {"mu_charge"}
  );

  /* 
    ======================================================
        Create lepton variables 
    ======================================================
  */
  // Function - combine floats
  auto combineElectronsAndMuonsFloat = [] (
      const std::vector<float>& electron_variable,
      const std::vector<float>& muon_variable
  ) {
      // Construct a new vector with elements from 'a', then append 'b'
      std::vector<float> result(electron_variable.begin(), electron_variable.end());
      result.insert(result.end(), muon_variable.begin(), muon_variable.end());

      return result;
  };
  // Variables - Lepton
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "lep_pt_NOSYS",
      combineElectronsAndMuonsFloat,
      {"el_pt_NOSYS","mu_pt_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
    mainNode,
    "lep_e_NOSYS",
    combineElectronsAndMuonsFloat,
    {"el_e_NOSYS","mu_e_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
    mainNode,
    "lep_eta_NOSYS",
    combineElectronsAndMuonsFloat,
    {"el_eta","mu_eta"}
  );
  mainNode = MainFrame::systematicDefine(
    mainNode,
    "lep_phi_NOSYS",
    combineElectronsAndMuonsFloat,
    {"el_phi","mu_phi"}
  );
  mainNode = MainFrame::systematicDefine(
    mainNode,
    "lep_charge_NOSYS",
    combineElectronsAndMuonsFloat,
    {"el_charge","mu_charge"}
  );
  mainNode = MainFrame::systematicDefine(
    mainNode,
    "lep_pdgID_NOSYS",
    combineElectronsAndMuonsFloat,
    {"el_pdgID_NOSYS","mu_pdgID_NOSYS"}
  );
  // Function - combine int
  auto combineElectronsAndMuonsInt = [] (
      const std::vector<int>& electron_variable,
      const std::vector<int>& muon_variable
  ) {
      // Construct a new vector with elements from 'a', then append 'b'
      std::vector<int> result(electron_variable.begin(), electron_variable.end());
      result.insert(result.end(), muon_variable.begin(), muon_variable.end());

      return result;
  };
  mainNode = MainFrame::systematicDefine(
    mainNode,
    "lep_IFFClass_NOSYS",
    combineElectronsAndMuonsInt,
    {"el_IFFClass","mu_IFFClass"}
  );
  // Function - combine char
  auto combineElectronsAndMuonsChar = [] (
      const std::vector<char>& electron_variable,
      const std::vector<char>& muon_variable
  ) {
      // Construct a new vector with elements from 'a', then append 'b'
      std::vector<char> result(electron_variable.begin(), electron_variable.end());
      result.insert(result.end(), muon_variable.begin(), muon_variable.end());

      return result;
  };
  // Variables - Lepton
  mainNode = MainFrame::systematicDefine(
    mainNode,
    "lep_select_passesOR_NOSYS",
    combineElectronsAndMuonsChar,
    {"el_select_passesOR_NOSYS","mu_select_passesOR_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
    mainNode,
    "lep_select_loose_NOSYS",
    combineElectronsAndMuonsChar,
    {"el_select_loose_NOSYS","mu_select_loose_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
    mainNode,
    "lep_select_tight_NOSYS",
    combineElectronsAndMuonsChar,
    {"el_select_tight_NOSYS","mu_select_tight_NOSYS"}
  );

  /* 
    ====================================
        Convert pt and e to GeV 
    ====================================
  */
  // Function
  auto convertMeVToGeV_Vectors = [](
      const std::vector<float>& data
  ) {
      const int size = data.size();

      std::vector<float> result;
      for (int i = 0; i < size; ++i) {
        result.push_back( data.at(i) / 1.e3 );
      }

      return result;
  };
  // Variables
  // Jet
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "jet_pt_GeV_NOSYS",
      convertMeVToGeV_Vectors,
      {"jet_pt_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "jet_e_GeV_NOSYS",
      convertMeVToGeV_Vectors,
      {"jet_e_NOSYS"}
  );
  // Lepton
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "lep_pt_GeV_NOSYS",
      convertMeVToGeV_Vectors,
      {"lep_pt_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "lep_e_GeV_NOSYS",
      convertMeVToGeV_Vectors,
      {"lep_e_NOSYS"}
  );

  // Function
  auto convertMeVToGeV_Scalar = [](
      float& data
  ) {
      float result = data/ 1.e3;

      return result;
  };
  // Variable - MET
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "met_met_GeV_NOSYS",
      convertMeVToGeV_Scalar,
      {"met_met_NOSYS"}
  );

  /* 
    ====================================
        Convert 4 vectors 
    ====================================
  */
  // Function
  auto createTLV = [](
      const std::vector<float>& pt,
      const std::vector<float>& eta,
      const std::vector<float>& phi,
      const std::vector<float>& e
  ) {
      const int size = pt.size();

      std::vector<TLV> result;
      for (int i = 0; i < size; ++i) {
        result.push_back( TLV{pt.at(i), eta.at(i), phi.at(i), e.at(i)} );
      }

      return result;
  };
  // Variables
  // Jets
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "jet_TLV_NOSYS",
      createTLV,
      {"jet_pt_GeV_NOSYS","jet_eta","jet_phi","jet_e_GeV_NOSYS"}
  );
  // Leptons
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "lep_TLV_NOSYS",
      createTLV,
      {"lep_pt_GeV_NOSYS","lep_eta_NOSYS","lep_phi_NOSYS","lep_e_GeV_NOSYS"}
  );

  /* 
    ======================================================
        Determine the indexes of highest to lowest pt 
    ======================================================
  */
  // Function - with selection cuts
  auto sortByPtAndApplySelections = [](
      const std::vector<float>& pt, 
      const std::vector<char>& sel1,
      const std::vector<char>& sel2
  ) {
      return DefineHelpers::sortedPassedIndices(pt, sel1, sel2);
  };
  // Variables
  // Jets
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "jet_good_idx_NOSYS",
      sortByPtAndApplySelections,
      {"jet_pt_GeV_NOSYS", "jet_select_passesOR_NOSYS", "jet_select_baselineJvt_NOSYS"}
  );
  // Leptons
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "lep_good_idx_NOSYS",
      sortByPtAndApplySelections,
      {"lep_pt_GeV_NOSYS", "lep_select_passesOR_NOSYS", "lep_select_loose_NOSYS"}
  );

  /* 
    ===========================================
        Sort variables by pt 
    ===========================================
  */
  // Function - TLV data type
  auto sortedSelectedTLVs = [](
      const std::vector<TLV> data,
      const std::vector<std::size_t> indices
  ) {
      RVec<TLV> rvec(data.begin(), data.end());
      RVec<TLV> selected_rvec = Take(rvec, indices);
      
      return std::vector<TLV>(selected_rvec.begin(), selected_rvec.end());
  };
  // Variables
  // Jets
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "sorted_jet_TLV_NOSYS",
      sortedSelectedTLVs,
      {"jet_TLV_NOSYS", "jet_good_idx_NOSYS"}
  );
  // Leptons
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "sorted_lep_TLV_NOSYS",
      sortedSelectedTLVs,
      {"lep_TLV_NOSYS", "lep_good_idx_NOSYS"}
  );

  // Function - Float data type
  auto sortedSelectedFloats = [](
      const std::vector<float>& data,
      const std::vector<std::size_t>& indices
  ) {
      RVec<float> rvec(data.begin(), data.end());
      RVec<float> selected_rvec = Take(rvec, indices);

      return std::vector<float>(selected_rvec.begin(), selected_rvec.end());
  };
  // Variables - Leptons
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "sorted_lep_charge_NOSYS",
      sortedSelectedFloats,
      {"lep_charge_NOSYS", "lep_good_idx_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "sorted_lep_pdgID_NOSYS",
      sortedSelectedFloats,
      {"lep_pdgID_NOSYS", "lep_good_idx_NOSYS"}
  );
  // Function - Int data type
  auto sortedSelectedInt = [](
      const std::vector<int>& data,
      const std::vector<std::size_t>& indices
  ) {
      RVec<int> rvec(data.begin(), data.end());
      RVec<int> selected_rvec = Take(rvec, indices);

      return std::vector<int>(selected_rvec.begin(), selected_rvec.end());
  };
  // Variables - Leptons
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "sorted_lep_IFFClass_NOSYS",
      sortedSelectedInt,
      {"lep_IFFClass_NOSYS", "lep_good_idx_NOSYS"}
  );
  // Function - Char data type
  auto sortedSelectedChar = [](
      const std::vector<char>& data,
      const std::vector<std::size_t>& indices
  ) {
      RVec<char> rvec(data.begin(), data.end());
      RVec<char> selected_rvec = Take(rvec, indices);

      return std::vector<char>(selected_rvec.begin(), selected_rvec.end());
  };
  // Variables - Leptons
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "sorted_lep_select_tight_NOSYS",
      sortedSelectedChar,
      {"lep_select_tight_NOSYS", "lep_good_idx_NOSYS"}
  );

  /* 
    ===========================================
        Bjets
    ===========================================
  */
  // Function - get bjet 4Vectors
  auto getBjetTLVs = [](
      const std::vector<TLV>& jets,
      const std::vector<std::size_t>& good_jets,
      const std::vector<char>& btag_select
  ) {
      RVec<char> btag_select_rvec(btag_select.begin(), btag_select.end());
      RVec<char> sorted_btag_select_rvec = Take(btag_select_rvec, good_jets);
      std::vector<char> sorted_btag_select(sorted_btag_select_rvec.begin(), sorted_btag_select_rvec.end());

      // Filter jets based on btag_select
      std::vector<TLV> bjet_tlvs;
      for (size_t i = 0; i < jets.size(); ++i) {
          if (sorted_btag_select[i]) {  // Keep only selected b-tagged jets
              bjet_tlvs.push_back(jets[i]);
          }
      }

      return bjet_tlvs;
  };
  // Variable - Bjets
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "sorted_bjet_TLV_NOSYS",
      getBjetTLVs,
      {"sorted_jet_TLV_NOSYS", "jet_good_idx_NOSYS", "jet_DL1dv01_FixedCutBEff_77_select"}
  );
  /* 
    ===========================================
        Non-B tagged jets
    ===========================================
  */
  // Function - get non-bjet 4Vectors
  auto getNonBjetTLVs = [](
      const std::vector<TLV>& jets,
      const std::vector<std::size_t>& good_jets,
      const std::vector<char>& btag_select
  ) {
      RVec<char> btag_select_rvec(btag_select.begin(), btag_select.end());
      RVec<char> sorted_btag_select_rvec = Take(btag_select_rvec, good_jets);
      std::vector<char> sorted_btag_select(sorted_btag_select_rvec.begin(), sorted_btag_select_rvec.end());

      // Filter jets based on btag_select
      std::vector<TLV> nonbjet_tlvs;
      for (size_t i = 0; i < jets.size(); ++i) {
          if (!sorted_btag_select[i]) {  // Keep only selected b-tagged jets
            nonbjet_tlvs.push_back(jets[i]);
          }
      }

      return nonbjet_tlvs;
  };
  // Variable - Non-Bjets
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "sorted_nonBjet_TLV_NOSYS",
      getNonBjetTLVs,
      {"sorted_jet_TLV_NOSYS", "jet_good_idx_NOSYS", "jet_DL1dv01_FixedCutBEff_77_select"}
  );

  /* 
    ===========================================
        Multiplicity
    ===========================================
  */
  // Jets
  mainNode = MainFrame::systematicStringDefine(
    mainNode,
    "nJets_NOSYS",
    "static_cast<float>(sorted_jet_TLV_NOSYS.size())"
  );
  // Bjets
  mainNode = MainFrame::systematicStringDefine(
    mainNode,
    "nBjets_NOSYS",
    "static_cast<float>(sorted_bjet_TLV_NOSYS.size())"
  );
  // Leptons
  mainNode = MainFrame::systematicStringDefine(
    mainNode,
    "nLeptons_NOSYS",
    "static_cast<float>(sorted_lep_TLV_NOSYS.size())"
  );
  // Function - nLeptons_tight
  auto calculateNTightLeptons = [](
      const std::vector<char>& tight_flag
  ) {
      const int size = tight_flag.size();

      float result = 0;
      for (int i=0; i<size; i++) {
        if (tight_flag.at(i)) {
          result += 1.;
        }
      }

      return result;
  };
  // Variable - Leptons
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "nLeptons_tight_NOSYS",
      calculateNTightLeptons,
      {"sorted_lep_select_tight_NOSYS"}
  );

  /* 
    =====================================================
        Separate out objects by leading order sequence
    =====================================================
  */
  // Add dummy variable for leadingOrderPosition
  mainNode = MainFrame::systematicStringDefine(
      mainNode,
      "LOPosition_NOSYS",
      "static_cast<int>(0)"
  );
  mainNode = MainFrame::systematicStringDefine(
      mainNode,
      "NLOPosition_NOSYS",
      "static_cast<int>(1)"
  );
  mainNode = MainFrame::systematicStringDefine(
      mainNode,
      "NNLOPosition_NOSYS",
      "static_cast<int>(2)"
  );
  mainNode = MainFrame::systematicStringDefine(
      mainNode,
      "NNNLOPosition_NOSYS",
      "static_cast<int>(3)"
  );
  // Function - extract the different leading orders for TLV data type
  auto extractLeadingOrderTLV = [](
      const std::vector<TLV>& tlv,
      const int & leadingOrderPosition
  ) {
      const std::size_t size = tlv.size();
      const std::size_t sizeConstraint = leadingOrderPosition + 1;

      TLV result;    
      if (size >= sizeConstraint) { result = tlv.at(leadingOrderPosition); } 
      else{ result = TLV{-999, -999, -999, -999}; }

      return result;
  };
  // Variables - Leptons
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "lo_lep_TLV_NOSYS",
      extractLeadingOrderTLV,
      {"sorted_lep_TLV_NOSYS","LOPosition_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "nlo_lep_TLV_NOSYS",
      extractLeadingOrderTLV,
      {"sorted_lep_TLV_NOSYS","NLOPosition_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "nnlo_lep_TLV_NOSYS",
      extractLeadingOrderTLV,
      {"sorted_lep_TLV_NOSYS","NNLOPosition_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "nnnlo_lep_TLV_NOSYS",
      extractLeadingOrderTLV,
      {"sorted_lep_TLV_NOSYS","NNNLOPosition_NOSYS"}
  );

  /* 
    ============================================================
        Calculate the sum of lepton charges
    ============================================================
  */

  // Function
  auto calculateSumOfLeptonCharges = [](
    const std::vector<float>& lepton_charges
  ) {
    const int size = lepton_charges.size();

    int result = 0;
    for(int i = 0; i< size; i++) {
      result += lepton_charges.at(i);
    }

    return result;
  };

  // Variables - Leptons
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "sumOfLeptonCharges_NOSYS",
      calculateSumOfLeptonCharges,
      {"sorted_lep_charge_NOSYS"}
  );

  /* 
    ============================================================
        Calculate mass of opposite sign same flavour leptons
    ============================================================
  */
  // Function
  auto calculateMassOSSF = [](
      const std::vector<TLV>& leptons, 
      const std::vector<float>& lep_pdgID
  )  {
      const int nLeptons = leptons.size();
      const double z_mass = 91.1876;
    
      std::vector<float> massOSSF(nLeptons);
      if (leptons.size() == 0) { throw std::runtime_error("Encountered event with no leptons!"); }
      else {
        for(int lep_i = 0; lep_i<nLeptons; lep_i++) {
          for(int lep_j = 0; lep_j<nLeptons; lep_j++) {
            if (lep_i == lep_j) { continue; }
            if (lep_pdgID.at(lep_i) + lep_pdgID.at(lep_j) != 0.) { continue; } // OSSF
            
            TLV pair = leptons.at(lep_i) + leptons.at(lep_j);
            float mass = pair.M();
            float diff = std::abs(z_mass - mass);

            if (massOSSF.at(lep_i) > diff || massOSSF.at(lep_j) > diff) { continue; }

            massOSSF.at(lep_i) = mass;
            massOSSF.at(lep_j) = mass;
          }
        }
      }
    
      return massOSSF;
  };
  // Variable  - Leptons
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "massOSSF_NOSYS",
      calculateMassOSSF,
      {"sorted_lep_TLV_NOSYS","sorted_lep_pdgID_NOSYS"}
  );

  /* 
    ============================================================
        Create massOSSF Flag for selection cut purposes
    ============================================================
  */
  // Function
  auto massOSSFFlag = [](
      const std::vector<float>& mass
  ) {
      const std::size_t size = mass.size();

      float result = 1.;    
      for (std::size_t mass_i = 0; mass_i<size; mass_i++) {
        if (mass.at(mass_i)!=0 && mass.at(mass_i)<10.)  {
          result = 0.;
          break;
        }
      }

      return result;
  };
  // Variable - OSSF Leptons
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "massOSSFFlag_NOSYS",
      massOSSFFlag,
      {"massOSSF_NOSYS"}
  );

  /* 
    ============================================================
        Create zCandidates off the back of massOSSF
    ============================================================
  */
  // Function
  auto zCandidatesFlag = [](
      const std::vector<float>& mass
  ) {
      const double z_mass = 91.1876;
      const double minMass = z_mass - 30.;
      const double maxMass = z_mass + 30.;

      const std::size_t size = mass.size();

      std::vector<float> result(size);    
      for (std::size_t mass_i = 0; mass_i<size; mass_i++) {
        if (minMass>mass.at(mass_i) || maxMass<mass.at(mass_i)) { continue; }

        for (std::size_t mass_j = 0; mass_j<size; mass_j++) {
          if (mass_i >= mass_j) { continue; }
          if (minMass>mass.at(mass_j) || maxMass<mass.at(mass_j)) { continue; }

          if (mass.at(mass_i) == mass.at(mass_j)) { 
            result.at(mass_i) = 1.;
            result.at(mass_j) = 1.;
            break;
          }
        }
      }

      return result;
  };
  // Variable - Z Candidates
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "zCandidateFlag_NOSYS",
      zCandidatesFlag,
      {"massOSSF_NOSYS"}
  );

  /*
    ===============================================================
      Create Z candidate 4 vector
    ===============================================================
  */
  // Function
  auto createZCandidate4Vector = [](
      const std::vector<TLV>& lepton,
      const std::vector<float>& mass,
      const std::vector<float>& zCandidateFlag
  ) {
      const int size = lepton.size();
      
      std::vector<TLV> result;
      for (int i=0; i<size; i++){
        if (zCandidateFlag.at(i) == 0.) { continue; } //Not a Z candidate
        
        for (int j=0; j<size; j++) {
          if (i>=j) {continue; }
          if (zCandidateFlag.at(j) == 0.) { continue; }
          if (mass.at(i) != mass.at(j)) { continue; }
           
          result.push_back(lepton.at(i) + lepton.at(j));
        }
      }

      return result;
  };
  // Variable s - Z Candidates
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "zCandidate_TLV_NOSYS",
      createZCandidate4Vector,
      {"sorted_lep_TLV_NOSYS","massOSSF_NOSYS","zCandidateFlag_NOSYS"}
  );
  // Function 
  auto sortByPt = [](
      const std::vector<TLV>& tlv
  ) {
      const int size = tlv.size();

      std::vector<TLV> result;
      for (int i=0; i<size; i++) {
        result.push_back(tlv.at(i));
      }
      // sort them based on pT
      std::sort(result.begin(), result.end(), [](const TLV& v1, const TLV& v2) { return v1.pt() > v2.pt(); });

      return result;
  };
  // Variable - Z Candidates
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "sorted_zCandidate_TLV_NOSYS",
      sortByPt,
      {"zCandidate_TLV_NOSYS"}
  );

  /* 
    ============================================================
        Calculate the Z candidate mutliplicity
    ============================================================
  */
  // Function
  auto nZCandidates = [](
      const std::vector<float>& candidates
  ) {
      const std::size_t size = candidates.size();

      float result = 0.;    
      for (std::size_t cand_i = 0; cand_i<size; cand_i++) {
        if (candidates.at(cand_i) == 1.)  {
          result += 0.5;
        }
      }

      return result;
  };
  // Variable - Z Candidates
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "nZCandidates_NOSYS",
      nZCandidates,
      {"zCandidateFlag_NOSYS"}
  );

  /* 
    ============================================================
        Specific additional identifier for tWZOFSR | tWZSFSR
    ============================================================
  */
  // Function
  auto signalRegionType = [](
      const std::vector<float>& candidatesFlag,
      const std::vector<float>& leptonPdgID
  ) {
      const std::size_t size = candidatesFlag.size();

      float result = 5.;    
      for (std::size_t cand_i = 0; cand_i<size; cand_i++) {
        if (result != 5) { break; }
        if (candidatesFlag.at(cand_i) == 1.) { continue; }

        for (std::size_t cand_j = 0; cand_j<size; cand_j++) {
          if (candidatesFlag.at(cand_j) == 1.) { continue; }
          if (cand_i >= cand_j) { continue; }

          if (leptonPdgID.at(cand_i) + leptonPdgID.at(cand_j) == 0) {
            result = 1.; // Same Flavour (SF)
            break;
          }
          else {
            result = 0.; // Opposite Flavour (OF)
            break;
          }
        }       
      }

      return result;
  };
  // Variable - OF or SF
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "signalRegionType_NOSYS",
      signalRegionType,
      {"zCandidateFlag_NOSYS","sorted_lep_pdgID_NOSYS"}
  );

  /*
    =========================================
      Calculate the Scalar sum of pt
    =========================================
  */
  // Function
  auto calculateScalarSumOfPt = [](
      const std::vector<TLV>& tlv
  ) {
      const std::size_t size = tlv.size();

      float result = 0.;    
      for (std::size_t tlv_i = 0; tlv_i<size; tlv_i++) {
        result += tlv.at(tlv_i).Pt();
      }

      return result;
  };
  // Variable
  // Jets
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "HT_NOSYS",
      calculateScalarSumOfPt,
      {"sorted_jet_TLV_NOSYS"}
  );
  // Leptons
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "LT_NOSYS",
      calculateScalarSumOfPt,
      {"sorted_lep_TLV_NOSYS"}
  );
  // Bjets
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "sumBjetPt_NOSYS",
      calculateScalarSumOfPt,
      {"sorted_bjet_TLV_NOSYS"}
  );
  // Z Candidates
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "sumZCandidatePt_NOSYS",
      calculateScalarSumOfPt,
      {"sorted_zCandidate_TLV_NOSYS"}
  );

  /*
    ===================================================
      Calculate the scalar sum of pt from two object
    ===================================================
  */
  // Function
  auto calculateScalarSumOfPtTwoObjects = [](
      const std::vector<TLV>& tlv1,
      const std::vector<TLV>& tlv2
  ) {
      const std::size_t size1 = tlv1.size();
      const std::size_t size2 = tlv2.size();

      float result = 0.;    
      for (std::size_t tlv_i = 0; tlv_i<size1; tlv_i++) {
        result += tlv1.at(tlv_i).Pt();
      }

      for (std::size_t tlv_j = 0; tlv_j<size2; tlv_j++) {
        result += tlv2.at(tlv_j).Pt();
      }

      return result;
  };
  // Variables - Jets & Leptons
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "ST_NOSYS",
      calculateScalarSumOfPtTwoObjects,
      {"sorted_jet_TLV_NOSYS","sorted_lep_TLV_NOSYS"}
  );

  /*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  */
  // Function
  auto calculateSMT = [](
      const std::vector<TLV>& tlv1,
      const std::vector<TLV>& tlv2,
      const float& met
  ) {
      const std::size_t size1 = tlv1.size();
      const std::size_t size2 = tlv2.size();

      float result = 0.;    
      for (std::size_t tlv_i = 0; tlv_i<size1; tlv_i++) {
        result += tlv1.at(tlv_i).Pt();
      }

      for (std::size_t tlv_j = 0; tlv_j<size2; tlv_j++) {
        result += tlv2.at(tlv_j).Pt();
      }

      result += met;

      return result;
  };
  // Variables - Jets & Leptons & MET
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "SMT_NOSYS",
      calculateSMT,
      {"sorted_jet_TLV_NOSYS","sorted_lep_TLV_NOSYS","met_met_GeV_NOSYS"}
  );

  /*
    ===============================================================
      Variables split into LO | NLO | NNLO | NNNLO 
    ===============================================================
  */
  // Function - extract the different leading orders for float data type
  auto extractLeadingOrderPt = [](
      const std::vector<TLV>& tlv,
      const int & leadingOrderPosition
  ) {
      const int size = tlv.size();
      const int sizeConstraint = leadingOrderPosition + 1;

      float result;    
      if (size >= sizeConstraint) { result = tlv.at(leadingOrderPosition).Pt(); } 
      else{ result = -999.; }

      return result;
  };
  // Variables - Leptons
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "lo_lep_pt_NOSYS",
      extractLeadingOrderPt,
      {"sorted_lep_TLV_NOSYS","LOPosition_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "nlo_lep_pt_NOSYS",
      extractLeadingOrderPt,
      {"sorted_lep_TLV_NOSYS","NLOPosition_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "nnlo_lep_pt_NOSYS",
      extractLeadingOrderPt,
      {"sorted_lep_TLV_NOSYS","NNLOPosition_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "nnnlo_lep_pt_NOSYS",
      extractLeadingOrderPt,
      {"sorted_lep_TLV_NOSYS","NNNLOPosition_NOSYS"}
  );
  // Bjets
  mainNode = MainFrame::systematicDefine(
    mainNode,
    "lo_bjet_pt_NOSYS",
    extractLeadingOrderPt,
    {"sorted_bjet_TLV_NOSYS","LOPosition_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "nlo_bjet_pt_NOSYS",
      extractLeadingOrderPt,
      {"sorted_bjet_TLV_NOSYS","NLOPosition_NOSYS"}
  );
  // Non-Bjets
  mainNode = MainFrame::systematicDefine(
    mainNode,
    "lo_nonBjet_pt_NOSYS",
    extractLeadingOrderPt,
    {"sorted_nonBjet_TLV_NOSYS","LOPosition_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "nlo_nonBjet_pt_NOSYS",
      extractLeadingOrderPt,
      {"sorted_nonBjet_TLV_NOSYS","NLOPosition_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "nnlo_nonBjet_pt_NOSYS",
      extractLeadingOrderPt,
      {"sorted_nonBjet_TLV_NOSYS","NLOPosition_NOSYS"}
  );
  // Z Candidates
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "lo_zCandidate_pt_NOSYS",
      extractLeadingOrderPt,
      {"sorted_zCandidate_TLV_NOSYS","LOPosition_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "nlo_zCandidate_pt_NOSYS",
      extractLeadingOrderPt,
      {"sorted_zCandidate_TLV_NOSYS","NLOPosition_NOSYS"}
  );
  // Function - extract the different leading orders for float data type
  auto extractLeadingOrderEta = [](
      const std::vector<TLV>& tlv,
      const int & leadingOrderPosition
  ) {
      const int size = tlv.size();
      const int sizeConstraint = leadingOrderPosition + 1;

      float result;    
      if (size >= sizeConstraint) { result = tlv.at(leadingOrderPosition).Eta(); } 
      else{ result = -999.; }

      return result;
  };
  // Variables
  // Leptons
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "lo_lep_eta_NOSYS",
      extractLeadingOrderEta,
      {"sorted_lep_TLV_NOSYS","LOPosition_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "nlo_lep_eta_NOSYS",
      extractLeadingOrderEta,
      {"sorted_lep_TLV_NOSYS","NLOPosition_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "nnlo_lep_eta_NOSYS",
      extractLeadingOrderEta,
      {"sorted_lep_TLV_NOSYS","NNLOPosition_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "nnnlo_lep_eta_NOSYS",
      extractLeadingOrderEta,
      {"sorted_lep_TLV_NOSYS","NNNLOPosition_NOSYS"}
  );
  // Bjets
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "lo_bjet_eta_NOSYS",
      extractLeadingOrderEta,
      {"sorted_bjet_TLV_NOSYS","LOPosition_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "nlo_bjet_eta_NOSYS",
      extractLeadingOrderEta,
      {"sorted_bjet_TLV_NOSYS","NLOPosition_NOSYS"}
  );
  // Non-Bjets
  mainNode = MainFrame::systematicDefine(
    mainNode,
    "lo_nonBjet_eta_NOSYS",
    extractLeadingOrderEta,
    {"sorted_nonBjet_TLV_NOSYS","LOPosition_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "nlo_nonBjet_eta_NOSYS",
      extractLeadingOrderEta,
      {"sorted_nonBjet_TLV_NOSYS","NLOPosition_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "nnlo_nonBjet_eta_NOSYS",
      extractLeadingOrderEta,
      {"sorted_nonBjet_TLV_NOSYS","NLOPosition_NOSYS"}
  );
  // Z Candidates
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "lo_zCandidate_eta_NOSYS",
      extractLeadingOrderEta,
      {"sorted_zCandidate_TLV_NOSYS","LOPosition_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "nlo_zCandidate_eta_NOSYS",
      extractLeadingOrderEta,
      {"sorted_zCandidate_TLV_NOSYS","NLOPosition_NOSYS"}
  );
  // Function - extract the different leading orders for float data type
  auto extractLeadingOrderPhi = [](
      const std::vector<TLV>& tlv,
      const int & leadingOrderPosition
  ) {
      const int size = tlv.size();
      const int sizeConstraint = leadingOrderPosition + 1;

      float result;    
      if (size >= sizeConstraint) { result = tlv.at(leadingOrderPosition).Phi(); } 
      else{ result = -999.; }

      return result;
  };
  // Variables
  // Leptons
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "lo_lep_phi_NOSYS",
      extractLeadingOrderPhi,
      {"sorted_lep_TLV_NOSYS","LOPosition_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "nlo_lep_phi_NOSYS",
      extractLeadingOrderPhi,
      {"sorted_lep_TLV_NOSYS","NLOPosition_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "nnlo_lep_phi_NOSYS",
      extractLeadingOrderPhi,
      {"sorted_lep_TLV_NOSYS","NNLOPosition_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "nnnlo_lep_phi_NOSYS",
      extractLeadingOrderPhi,
      {"sorted_lep_TLV_NOSYS","NNNLOPosition_NOSYS"}
  );
  // Bjets
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "lo_bjet_phi_NOSYS",
      extractLeadingOrderPhi,
      {"sorted_bjet_TLV_NOSYS","LOPosition_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "nlo_bjet_phi_NOSYS",
      extractLeadingOrderPhi,
      {"sorted_bjet_TLV_NOSYS","NLOPosition_NOSYS"}
  );
  // Non-Bjets
  mainNode = MainFrame::systematicDefine(
    mainNode,
    "lo_nonBjet_phi_NOSYS",
    extractLeadingOrderPhi,
    {"sorted_nonBjet_TLV_NOSYS","LOPosition_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "nlo_nonBjet_phi_NOSYS",
      extractLeadingOrderPhi,
      {"sorted_nonBjet_TLV_NOSYS","NLOPosition_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "nnlo_nonBjet_phi_NOSYS",
      extractLeadingOrderPhi,
      {"sorted_nonBjet_TLV_NOSYS","NLOPosition_NOSYS"}
  );
  // Z Candidates
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "lo_zCandidate_phi_NOSYS",
      extractLeadingOrderPhi,
      {"sorted_zCandidate_TLV_NOSYS","LOPosition_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "nlo_zCandidate_phi_NOSYS",
      extractLeadingOrderPhi,
      {"sorted_zCandidate_TLV_NOSYS","NLOPosition_NOSYS"}
  );
  // Function - extract the different leading orders for float data type
  auto extractLeadingOrderMass = [](
      const std::vector<TLV>& tlv,
      const int & leadingOrderPosition
  ) {
      const int size = tlv.size();
      const int sizeConstraint = leadingOrderPosition + 1;

      float result;    
      if (size >= sizeConstraint) { result = tlv.at(leadingOrderPosition).M(); } 
      else{ result = -999.; }

      return result;
  };
  // Variables - Z Candidates
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "lo_zCandidate_mass_NOSYS",
      extractLeadingOrderMass,
      {"sorted_zCandidate_TLV_NOSYS","LOPosition_NOSYS"}
  );
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "nlo_zCandidate_mass_NOSYS",
      extractLeadingOrderMass,
      {"sorted_zCandidate_TLV_NOSYS","NLOPosition_NOSYS"}
  );
  /*
    ===============================================================
      Calculate 4 vector and components of the 4 lepton system
    ===============================================================
  */
  // Function
  auto combineAllLeptons = [](
      const std::vector<TLV>& tlv
  ) {
      const int size = tlv.size();

      TLV result;
      for (int i=0; i<size; i++) {
        result += tlv.at(i);
      }

      return result;
  };
  // Variables - Leptons
  mainNode = MainFrame::systematicDefine(
      mainNode,
      "llll_TLV_NOSYS",
      combineAllLeptons,
      {"sorted_lep_TLV_NOSYS"}
  );
  mainNode = MainFrame::systematicStringDefine(
      mainNode,
      "llll_pt_NOSYS",
      "static_cast<float>(llll_TLV_NOSYS.Pt())"
  );
  mainNode = MainFrame::systematicStringDefine(
      mainNode,
      "llll_eta_NOSYS",
      "static_cast<float>(llll_TLV_NOSYS.Eta())"
  );
  mainNode = MainFrame::systematicStringDefine(
      mainNode,
      "llll_phi_NOSYS",
      "static_cast<float>(llll_TLV_NOSYS.Phi())"
  );
  mainNode = MainFrame::systematicStringDefine(
      mainNode,
      "llll_mass_NOSYS",
      "static_cast<float>(llll_TLV_NOSYS.M())"
  );

  /*
    ===============================================================
      Combine the 4 lepton system with other particles
    ===============================================================
  */
  // 4Lepton + Leading order Bjet
  mainNode = MainFrame::systematicStringDefine(
      mainNode,
      "llll_loBjet_TLV_NOSYS",
      "static_cast<ROOT::Math::PtEtaPhiEVector>((llll_TLV_NOSYS + sorted_bjet_TLV_NOSYS.at(0)))"
  );
  mainNode = MainFrame::systematicStringDefine(
      mainNode,
      "llll_loBjet_pt_NOSYS",
      "static_cast<float>((llll_TLV_NOSYS + sorted_bjet_TLV_NOSYS.at(0)).Pt())"
  );
  mainNode = MainFrame::systematicStringDefine(
      mainNode,
      "llll_loBjet_eta_NOSYS",
      "static_cast<float>((llll_TLV_NOSYS + sorted_bjet_TLV_NOSYS.at(0)).Eta())"
  );
  mainNode = MainFrame::systematicStringDefine(
      mainNode,
      "llll_loBjet_mass_NOSYS",
      "static_cast<float>((llll_TLV_NOSYS + sorted_bjet_TLV_NOSYS.at(0)).M())"
  );
  mainNode = MainFrame::systematicStringDefine(
      mainNode,
      "deltaPt_llll_loBjet_NOSYS",
      "static_cast<float>(std::abs(llll_TLV_NOSYS.Pt() - sorted_bjet_TLV_NOSYS.at(0).Pt()))"
  );
  // Leading order Z Candidate + 4Lepton & Leading Order Bjet system
  mainNode = MainFrame::systematicStringDefine(
      mainNode,
      "deltaPhi_loZCandidate_llllloBjet_NOSYS",
      "static_cast<float>(std::abs(llll_loBjet_TLV_NOSYS.Phi() - lo_zCandidate_phi_NOSYS))"
  );
  // MET + 4Lepton & Leading Order Bjet system
  mainNode = MainFrame::systematicStringDefine(
      mainNode,
      "llllloBjet_MET_pt_NOSYS",
      "static_cast<float>(llll_loBjet_TLV_NOSYS.Pt() + met_met_GeV_NOSYS)"
  );
  // MET + 4Lepton system
  mainNode = MainFrame::systematicStringDefine(
      mainNode,
      "llll_MET_pt_NOSYS",
      "static_cast<float>(llll_TLV_NOSYS.Pt() + met_met_GeV_NOSYS)"
  );
  mainNode = MainFrame::systematicStringDefine(
      mainNode,
      "llll_MET_mass_NOSYS",
      "static_cast<float>(llll_TLV_NOSYS.M() + met_met_GeV_NOSYS)"
  );
  // Leptons + MET
  mainNode = MainFrame::systematicStringDefine(
      mainNode,
      "l_l_l_l_MET_pt_NOSYS",
      "static_cast<float>(LT_NOSYS + met_met_GeV_NOSYS)"
  );
  // Leptons + MET + Leading Order Bjet
  mainNode = MainFrame::systematicStringDefine(
      mainNode,
      "l_l_l_l_MET_loBjet_pt_NOSYS",
      "static_cast<float>(LT_NOSYS + met_met_GeV_NOSYS + sorted_bjet_TLV_NOSYS.at(0).Pt())"
  );
  // MET + Leading Order Bjet
  mainNode = MainFrame::systematicStringDefine(
      mainNode,
      "MET_loBjet_pt_NOSYS",
      "static_cast<float>(met_met_GeV_NOSYS + sorted_bjet_TLV_NOSYS.at(0).Pt())"
  );
  mainNode = MainFrame::systematicStringDefine(
      mainNode,
      "MET_loBjet_mass_NOSYS",
      "static_cast<float>(met_met_GeV_NOSYS + sorted_bjet_TLV_NOSYS.at(0).M())"
  );


  /*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  */

  /*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  */

  /*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  */

  /*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  */
  /*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  *//*
    ===============================================================
      Calculate the Scalar sum of pt from Jets, Leptons and MET
    ===============================================================
  */


  /* 
  ======================================================
    Analysis Variables
  ======================================================
  */

  /* Metadata */
  // eventNumber
  // mcChannelNumber
  // runNumber

  /* Weights */
  // weight_beamspot
  // weight_pileup_NOSYS
  // weight_mc_NOSYS
  // weight_ftag_effSF_DL1dv01_FixedCutBEff_77_NOSYS
  // weight_jvt_effSF_NOSYS
  // weight_leptonSF_tight_NOSYS
  // globalTriggerEffSF_NOSYS

  /* Leptons */
  // nLeptons_loose
  // nLeptons_tight
  // sum_lep_charges
  


  // nJets - nJets
  // nBjets - nBjets
  // nZCands - nZCandidates
  // nLeptons - nLepton_tight | nLeptons_loose
  // Electron pt | eta | phi | e - already exists
  // Electron pdgID - already exists in electron flavour
  // Muon pt | eta | phi | e - already exists
  // Muon pdgID - already exists in muon flavour
  // Lepton pt - already exists


  

  // MET - met_met_GeV_NOSYS
  // MET phi - met_phi_NOSYS
  // Sum Z Pt
  // Sum Bjet Pt
  // Sum Bjet_Lepton Pt
  // 4LepPt+MET
  // lep_pt|eta|phi
  // deltaPhi(lep,MET)
  // Sum lepton charges



  /* 
  ================================
    End of defineVariables
  ================================
  */

  return mainNode;
}

ROOT::RDF::RNode tWZ4LepClass::defineVariablesNtuple(ROOT::RDF::RNode mainNode,
                                                      const std::shared_ptr<Sample>& sample,
                                                      const UniqueSampleID& id) {

  mainNode = defineVariables(mainNode,sample,id);

  return mainNode;
}

ROOT::RDF::RNode tWZ4LepClass::defineVariablesTruth(ROOT::RDF::RNode node,
                                                     const std::string& /*sample*/,
                                                     const std::shared_ptr<Sample>& /*sample*/,
                                                     const UniqueSampleID& /*sampleID*/) {
  return node;
}

ROOT::RDF::RNode tWZ4LepClass::defineVariablesNtupleTruth(ROOT::RDF::RNode node,
                                                           const std::string& /*treeName*/,
                                                           const std::shared_ptr<Sample>& /*sample*/,
                                                           const UniqueSampleID& /*sampleID*/) {
  return node;
}
  
