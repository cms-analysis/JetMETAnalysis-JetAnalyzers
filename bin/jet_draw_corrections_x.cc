///////////////////////////////////////////////////////////////////
//
// jet_draw_corrections_x
// ----------------------
//
//            09/01/2011 Ricardo Eusebi eusebi@tamu.edu
//
// This executable reads the relevant txt files and validates the 
// output that is obtained when using through FactorizedJetCorrector
// It will complain strongly on the presence of nan's or inf's or 
// corrections factors outside the range (0.8,3.0) and
// it will produce validation plots as well.
///////////////////////////////////////////////////////////////////

#include "JetMETAnalysis/JetAnalyzers/interface/Settings.h"
#include "JetMETAnalysis/JetAnalyzers/interface/Style.h"
#include "JetMETAnalysis/JetUtilities/interface/CommandLine.h"
#include "CondFormats/JetMETObjects/interface/JetCorrectorParameters.h"
#include "CondFormats/JetMETObjects/interface/FactorizedJetCorrector.h"

#include "TROOT.h"
#include "TSystem.h"
#include "TStyle.h"
#include "TObject.h"
#include "TFile.h"
#include "TCanvas.h"
#include "TH1.h"
#include "TH1F.h"
#include "TH1D.h"
#include "TH2.h"
#include "TH2F.h"
#include "TH3F.h"
#include "TF1.h"
#include "TString.h"
#include "TPaveText.h"
#include "TLegend.h"
#include "TLatex.h"
#include "TMath.h"

#include <fstream>
#include <string>
#include <cmath>

using namespace std;

////////////////////////////////////////////////////////////////////////////////
// main
////////////////////////////////////////////////////////////////////////////////
FactorizedJetCorrector * getFactorizedCorrector(TString algo, CommandLine & cl, TString & );

void analyzeAlgo(TString algo, CommandLine & cl);

TCanvas * getCorrectionVsEtaCanvas(TString algo, FactorizedJetCorrector * jetCorr, TString suffix);

TCanvas * getCorrectionVsEtaCanvasTDR(TString algo, FactorizedJetCorrector * jetCorr, TString suffix);

TCanvas * getCorrectionVsPtCanvas(TString algo, FactorizedJetCorrector * jetCorr, TString suffix);

string getAlias(TString s);

///CMS Preliminary label;
void cmsPrelim(double intLUMI = 0);

//______________________________________________________________________________
int main(int argc,char**argv)
{
  gROOT->SetStyle("Plain");
  gStyle->SetOptStat(0);

  gSystem->Load("libFWCoreFWLite.so");

  // evaluate command-line / configuration file options
  CommandLine cl;
  if (!cl.parse(argc,argv)) return 0;

  vector<TString> algs         = cl.getVector<TString> ("algs");
  TString         path         = cl.getValue<TString>  ("path"         , ""      );
  string          outputDir    = cl.getValue<string>   ("outputDir"    , "./");
  if (algs.size()==0){
    cout<<"ERROR you must use the flag -algs to select one or more algorithms!"<<endl;
  }

  // Loop over the algorithms 
  for(unsigned int a=0; a<algs.size(); a++){
    
    // create the output file
    if(outputDir.length() > 0 && outputDir[outputDir.length()-1] != '/') outputDir += "/";
    TString outf_str = outputDir+"Corrections_Overview_"+algs[a]+".root" ;
    TFile *outf = new TFile(outf_str,"RECREATE");
    if (!outf) {
      cout<<" Output file="<<outf_str<<" could not be created"<<endl;
      exit(0);
    }
    
    // analyze that algorithm.
    analyzeAlgo(algs[a], cl);
    
    // write and close the output file
    outf->cd();
    outf->Write();
    outf->Close();
    
  }// for algos
  
}//main


//---------------------------------------------------------------------
// Do here all the analysis for the given algorithm
void analyzeAlgo(TString algo, CommandLine & cl){

  // Report
  cout<<"Doing algorithm "<<algo<<endl;

  // get the factorized corrector according to the command line options
  TString corrLabel = "";
  FactorizedJetCorrector * jetCorr = getFactorizedCorrector(algo, cl, corrLabel);
  if (!jetCorr){
    cout<<"ERROR FactorizedJetCorrector could not be defined. Skipping algo."<<endl;
    return;
  }
    
  // see if flavor separation is required
  TString         flavor       = cl.getValue<TString>  ("flavor",                  "");
  string          outputDir    = cl.getValue<string>   ("outputDir",         "images");
  vector<TString> outputFormat = cl.getVector<TString> ("outputFormat", ".png:::.eps");
  bool            tdr          = cl.getValue<bool>     ("tdr",                   true);

  if (tdr) {
     setStyle();
  }

  if(outputDir.length() > 0 && outputDir[outputDir.length()-1] != '/') outputDir += "/";

  //Create the suffix for the names
  TString suffix;
  suffix += "_" + algo;
  if( !flavor.IsNull() ) suffix += "_" + flavor;
  suffix += "_" + corrLabel;

  // get the canvas of correction vs eta, write and save to file
  TCanvas * ove = getCorrectionVsEtaCanvas(algo, jetCorr, suffix);
  for(unsigned int of=0; of<outputFormat.size(); of++) {
     ove->SaveAs(outputDir+string(ove->GetName())+outputFormat[of]);
  }
  ove->Write();

  // get the canvas of correction vs eta in tdr format, write and save to file
  if(tdr) {
     TCanvas * ovetdr = getCorrectionVsEtaCanvasTDR(algo, jetCorr, suffix);
     for(unsigned int of=0; of<outputFormat.size(); of++) {
        ovetdr->SaveAs(outputDir+string(ovetdr->GetName())+outputFormat[of]);
     }
     ovetdr->Write();     
  }

  // get the canvas of correction vs pt, write and save to file
  TCanvas * ovp = getCorrectionVsPtCanvas(algo, jetCorr, suffix);
  for(unsigned int of=0; of<outputFormat.size(); of++) {
     ovp->SaveAs(outputDir+string(ovp->GetName())+outputFormat[of]);
  }
  ovp->Write();
  

}//analyzeAlgo

//---------------------------------------------------------------------
TCanvas * getCorrectionVsEtaCanvas(TString algo, FactorizedJetCorrector * jetCorr, TString suffix) {

 //Create canvas vs eta for different pts  
  vector<double> PtVals;
  PtVals.push_back(0.00001);
  PtVals.push_back(0.1);
  PtVals.push_back(1);
  PtVals.push_back(5);
  PtVals.push_back(7);
  PtVals.push_back(10);
  PtVals.push_back(20);
  PtVals.push_back(30);
  PtVals.push_back(40);
  PtVals.push_back(50);
  PtVals.push_back(150);
  PtVals.push_back(250);
  PtVals.push_back(500);
  PtVals.push_back(1000);
  PtVals.push_back(1500);
  PtVals.push_back(2500);

  //Create the canvas with multiple pads
  TString ss("CorrectionVsEta_Overview");
  ss += suffix;
  TCanvas *ove = new TCanvas(ss,ss,1200,800);
  ove->Divide(4,4);

  // loop over all pads
  for (int c = 0; c < ove->GetListOfPrimitives()->GetSize(); c++) {

    // just make we don't write the extra pads 
    if (c <= (int) PtVals.size()){

      //Create and fill the histo
      TString hstr; hstr.Form("EtaSF_%d",c);
      TH1F * cc = new TH1F(hstr,hstr,NETA,veta);
      for (int b = 1; b <= cc->GetNbinsX(); b++){
	jetCorr->setJetPt(PtVals[c]);
	jetCorr->setJetEta(cc->GetBinCenter(b));
	double cor = jetCorr->getCorrection();
	if (std::isnan((double)cor) || std::isinf((double)cor) ){
	  cout<<" *** ERROR *** getCorrectionVsEtaCanvas(). For eta="<<cc->GetBinCenter(b)
	      <<" and pt="<<PtVals[c]<<" the correction is "<<cor<<"!!"<<endl;
	  cor = 10000;
	}
	if ( cor < 0.8  || cor > 3 ){
	  cout<<" WARNING  *** getCorrectionVsEtaCanvas(). Correction of "<<cor<<" is out of the (0.8,3) range"<<endl;
	}

	cc->SetBinContent(b,cor);
      }//for eta bins
      
      cc->GetXaxis()->SetTitle("#eta");
      cc->GetYaxis()->SetTitle("Corr. Factor");
      cc->GetYaxis()->SetTitleOffset(1.5);
      cc->GetYaxis()->SetTitleSize(0.058);
      cc->GetXaxis()->SetTitleSize(0.058);
      cc->GetXaxis()->SetTitleOffset(0.95);

      cc->GetYaxis()->SetRangeUser(0.70,3.5);

      //Create a pave indicating the pt 
      TString ptstr;
      if (PtVals[c]<0.1)
	ptstr.Form("P_{T}=%f",PtVals[c]);
      else 
	ptstr.Form("P_{T}=%.1f",PtVals[c]);

      TPaveText * pave = new TPaveText(0.3,0.75,0.8,0.9,"NDC");
      pave->AddText(algo);
      pave->AddText(ptstr);      
      pave->SetFillColor(0);
      pave->SetShadowColor(0);

      ove->cd(c+1);
      cc->SetFillColor(30);
      cc->SetFillStyle(3001);
      cc->Draw();
      pave->Draw();

    } else break;

  }//for loop

  // return the canvas
  return ove;

}//getCorrectionVsEtaCanvas()

//---------------------------------------------------------------------
TCanvas * getCorrectionVsEtaCanvasTDR(TString algo, FactorizedJetCorrector * jetCorr, TString suffix) {

 //Create canvas vs eta for different pts  
  vector<double> PtVals;
  PtVals.push_back(30);
  PtVals.push_back(100);
  PtVals.push_back(300);

  //Create the canvas with multiple pads
  TString ss("CorrectionVsEta_Overview_TDR");
  ss += suffix;
  TCanvas *ove = new TCanvas(ss,ss,1200,800);
  ove->cd();

  // Create a legend for pt values
  TLegend * leg = new TLegend(0.3,0.7,0.8,0.9);
  //TLegend * leg = new TLegend(0.5,0.7,0.9,0.9);
  leg->SetTextSize(0.04);
  leg->SetBorderSize(0);
  leg->SetFillColor(0);

  //Create a pave indicating the algorithm name
  TString algNameLong;
  if(algo.Contains("ak"))        algNameLong += "Anti-kT";
  if(algo.Contains("4"))         algNameLong += " R=0.4";
  else if(algo.Contains("5"))    algNameLong += " R=0.5";
  else if(algo.Contains("6"))    algNameLong += " R=0.6";
  else if(algo.Contains("7"))    algNameLong += " R=0.7";
  if(algo.Contains("pfchs"))     algNameLong += ", PFlow+CHS";
  //else if(algo.Contains("pf"))   algNameLong += ", PFlow";
  else if(algo.Contains("pf"))   algNameLong += ", Particle-Flow Jets";
  else if(algo.Contains("calo")) algNameLong += ", Calo";
  else if(algo.Contains("jpt"))  algNameLong += ", JPT";
  
  leg->AddEntry((TObject*)0,algNameLong,"");
  leg->AddEntry((TObject*)0,"","");

  //TPaveText * pave = new TPaveText(0.3,0.84,0.8,0.94,"NDC");
  //pave->AddText(algNameLong);
  //pave->SetFillColor(0);
  //pave->SetShadowColor(0);
  //pave->SetTextFont(42);
  //pave->SetTextSize(0.05);

  // loop over all pads
  for (unsigned int c = 0; c < PtVals.size(); c++) {

     //Create and fill the histo
     TString hstr; hstr.Form("EtaSF_TDR_%d",c);
     TH1F * cc = new TH1F(hstr,hstr,NETA,veta);
     for (int b = 1; b <= cc->GetNbinsX(); b++){
        jetCorr->setJetPt(PtVals[c]);
        jetCorr->setJetEta(cc->GetBinCenter(b));
        double cor = jetCorr->getCorrection();
        if (std::isnan((double)cor) || std::isinf((double)cor) ){
           cout<<" *** ERROR *** getCorrectionVsEtaCanvas(). For eta="<<cc->GetBinCenter(b)
               <<" and pt="<<PtVals[c]<<" the correction is "<<cor<<"!!"<<endl;
           cor = 10000;
        }
        if ( cor < 0.8  || cor > 3 ){
           cout<<" WARNING  *** getCorrectionVsEtaCanvas(). Correction of "<<cor<<" is out of the (0.8,3) range"<<endl;
           }
        
        cc->SetBinContent(b,cor);
     }//for eta bins
     
     cc->GetXaxis()->SetTitle("#eta");
     cc->GetYaxis()->SetTitle("Corr. Factor");
     if(algo.Contains("calo"))
        cc->GetYaxis()->SetRangeUser(0.90,2.5);
     else
        cc->GetYaxis()->SetRangeUser(0.90,1.8);
     cc->SetFillColor(30);
     cc->SetFillStyle(3001);

     //Set marker colors and styles
     if(c == 0) {
        cc->SetMarkerStyle(24);
        cc->SetMarkerColor(1);
        cc->SetLineColor(1);
        cc->SetMarkerSize(1.6);
     }
     else if(c == 1) {
        cc->SetMarkerStyle(21);
        cc->SetMarkerColor(2);
        cc->SetLineColor(2);
        cc->SetMarkerSize(1.6);
     }
     else if(c == 2) {
        cc->SetMarkerStyle(20);
        cc->SetMarkerColor(38);
        cc->SetLineColor(38);
        cc->SetMarkerSize(1.6);
     }
     else {
        cc->SetMarkerStyle(25);
        cc->SetMarkerColor(6);
        cc->SetLineColor(6);
        cc->SetMarkerSize(1.6);
     }

     if(c == 0)
        cc->Draw("P");
     else
        cc->Draw("Psame");

     TString ptstr;
     if (PtVals[c]<0.1)
        ptstr.Form("%f",PtVals[c]);
     else 
        ptstr.Form("%.0f",PtVals[c]);

     leg->AddEntry(cc,"P_{T} = "+ptstr+" GeV","p");

  }//for loop

  //pave->Draw("same");
  leg->Draw("same");
  cmsPrelim();
  
  // return the canvas
  return ove;
  
}//getCorrectionVsEtaCanvasTDR()


//---------------------------------------------------------------------
TCanvas * getCorrectionVsPtCanvas(TString algo, FactorizedJetCorrector * jetCorr, TString suffix) {

 //Create canvas vs eta for different pts  
  vector<double> EtaVals;
  EtaVals.push_back(-4.8);
  EtaVals.push_back(-4.0);
  EtaVals.push_back(-3.2);
  EtaVals.push_back(-2.4);
  EtaVals.push_back(-1.6);
  EtaVals.push_back(-0.8);
  EtaVals.push_back(0);
  EtaVals.push_back( 0.8);
  EtaVals.push_back( 1.6);
  EtaVals.push_back( 2.4);
  EtaVals.push_back( 3.2);
  EtaVals.push_back( 4.0);
  EtaVals.push_back( 4.8);

  //Create the canvas with multiple pads
  TString ss("CorrectionVsPt_Overview");
  ss += suffix;
  TCanvas *ovp = new TCanvas(ss,ss,1200,800);
  ovp->Divide(4,4);

  // loop over all pads
  for (int c = 0; c < ovp->GetListOfPrimitives()->GetSize(); c++) {

    // just make we don't write the extra pads 
    if (c <= (int) EtaVals.size()){

      //Create and fill the histo
      TString hstr; hstr.Form("PtSF_%d",c);
      TH1F * cc = new TH1F(hstr,hstr,NPtBinsHLT,vpt_HLT);
      for (int b = 1; b <= cc->GetNbinsX(); b++){
	jetCorr->setJetPt(cc->GetBinCenter(b));
	jetCorr->setJetEta(EtaVals[c]);
	double cor = jetCorr->getCorrection();
	if (std::isnan((double)cor) ||  std::isinf((double)cor) ){
	  cout<<" *** ERROR *** getCorrectionVsPtCanvas(). For eta="<<EtaVals[c]
	      <<" and pt="<<cc->GetBinCenter(b)<<" the correction is "<<cor<<"!!"<<endl;
	  cor = 10000;
	}
	if ( cor < 0.8  || cor > 3 ){
	  cout<<" WARNING  *** getCorrectionVsPtCanvas(). Correction of "<<cor<<" is out of the (0.8,3) range"<<endl;
	}

	cc->SetBinContent(b,cor);
      }//for pt bins
      cc->GetXaxis()->SetTitle("Pt");
      cc->GetYaxis()->SetTitle("Corr. Factor");
      cc->GetYaxis()->SetRangeUser(0.70,3.0);

      //Create a pave indicating the eta 
      TString ptstr;
      ptstr.Form("#eta=%.1f",EtaVals[c]);
      TPaveText * pave = new TPaveText(0.3,0.75,0.8,0.9,"NDC");
      pave->AddText(algo);
      pave->AddText(ptstr);      
      pave->SetFillColor(0);
      pave->SetShadowColor(0);

      (ovp->cd(c+1))->SetLogx(1);
      cc->SetFillColor(30);
      cc->SetFillStyle(3001);
      cc->Draw();
      pave->Draw();

    } else break;

  }//for loop

  // return the canvas
  return ovp;

}//getCorrectionVsPtCanvas()

//---------------------------------------------------------------------
FactorizedJetCorrector * getFactorizedCorrector(TString algo, CommandLine & cl, TString & label) {

  string  path         = cl.getValue<string> ("path","./");
  string  era          = cl.getValue<string> ("era");
  bool    useL1FasCor  = cl.getValue<bool>   ("useL1FasCor"  , false   );
  bool    useL1OffCor  = cl.getValue<bool>   ("useL1OffCor"  , false   );
  bool    useL2Cor     = cl.getValue<bool>   ("useL2Cor"     , false   );
  bool    useL3Cor     = cl.getValue<bool>   ("useL3Cor"     , false   );
  bool    useL2L3ResCor= cl.getValue<bool>   ("useL2L3ResCor", false   );

  if (era.length()==0) {
    cout<<"ERROR flag -era must be specified"<<endl;
    return 0;
  }
    

  string alias = getAlias(algo);

  // Create the corrections from the text files
  vector<JetCorrectorParameters> vPar;
  if(useL1FasCor)
    {
      string txtfname = path + era + "_L1FastJet_"   + alias + ".txt";
      cout << "\tUsing " << txtfname<<endl;
      JetCorrectorParameters * jcPar = new JetCorrectorParameters(txtfname);
      vPar.push_back(*jcPar);
      label += "L1Fast";
    }
  if(useL1OffCor)
    {
      string txtfname = path + era + "_L1Offset_"   + alias + ".txt";
      cout << "\tUsing " << txtfname<<endl;
      JetCorrectorParameters * jcPar = new JetCorrectorParameters(txtfname);
      vPar.push_back(*jcPar);
      label += "L1Off";
    }
  if(useL2Cor)
    {
      string txtfname = path + era + "_L2Relative_"   + alias + ".txt";
      cout << "\tUsing " << txtfname<<endl;
      JetCorrectorParameters * jcPar = new JetCorrectorParameters(txtfname);
      vPar.push_back(*jcPar);
      label += "L2";
    }
  if(useL3Cor)
    {
      string txtfname = path + era + "_L3Absolute_"   + alias + ".txt";
      cout << "\tUsing " << txtfname<<endl;
      JetCorrectorParameters * jcPar = new JetCorrectorParameters(txtfname);
      vPar.push_back(*jcPar);
      label += "L3";
    }
  if(useL2L3ResCor)
    {
      string txtfname = path + era + "_L2L3Residual_"   + alias + ".txt";
      cout << "\tUsing " << txtfname<<endl;
      JetCorrectorParameters * jcPar = new JetCorrectorParameters(txtfname);
      vPar.push_back(*jcPar);
      label += "L2L3Res";
    }
  if (vPar.size()==0){
    cout<<"ERROR you must set one of following flags to true:"<<endl
	<<"\t-useL1FasCor  "<<endl
	<<"\t-useL1OffCor  "<<endl
	<<"\t-useL2Cor	    "<<endl
	<<"\t-useL3Cor	    "<<endl
	<<"\t-useL2L3ResCor"<<endl;
    return 0;
  }
  
  return new FactorizedJetCorrector(vPar);

}//getFactorizedCorrector


//______________________________________________________________________________
string getAlias(TString s)
{
   if (s=="ic5calo")
      return "IC5Calo";
   else if (s=="ic5pf")
      return "IC5PF";
   else if (s=="ak5calo")
      return "AK5Calo";  
   else if (s=="ak5calol1")
      return "AK5Calol1";
   else if (s=="ak5calol1off")
      return "AK5Calol1off";
   else if (s=="ak5calol1offl2l3")
      return "AK5Calol1off";
   else if (s=="ak7calo")
      return "AK7Calo";
   else if (s=="ak7calol1")
      return "AK7Calol1";
   else if (s=="ak7calol1off")
      return "AK7Calol1off";
   else if (s=="ak5caloHLT")
      return "AK5CaloHLT";
   else if (s=="ak5caloHLTl1")
      return "AK5CaloHLTl1";
   else if (s=="ak5pf")
      return "AK5PF";
   else if (s=="ak5pfl1")
      return "AK5PFl1";
   else if (s=="ak5pfl1l2l3")
      return "AK5PFl1";
   else if (s=="ak5pfl1off")
      return "AK5PFl1off";
   else if (s=="ak7pf")
      return "AK7PF";
   else if (s=="ak7pfl1")
      return "AK7PFl1";
   else if (s=="ak7pfl1off")
      return "AK7PFl1off";
   else if (s=="ak5pfchs")
      return "AK5PFchs";
   else if (s=="ak5pfchsl1")
      return "AK5PFchsl1";
   else if (s=="ak5pfchsl1l2l3")
      return "AK5PFchsl1";
   else if (s=="ak5pfchsl1off")
      return "AK5PFchsl1off";
   else if (s=="ak7pfchs")
      return "AK7PFchs";
   else if (s=="ak7pfchsl1")
      return "AK7PFchsl1";
   else if (s=="ak7pfchsl1off")
      return "AK7PFchsl1off";
   else if (s=="ak5pfHLT")
      return "AK5PFHLT";
  else if (s=="ak5pfHLTl1")
      return "AK5PFHLTl1";
   else if (s=="ak5pfchsHLT")
      return "AK5PFchsHLT";
   else if (s=="ak5pfchsHLTl1")
      return "AK5PFchsHLTl1";
   else if (s=="ak5jpt")
      return "AK5JPT";
   else if (s=="ak5jptl1")
      return "AK5JPTl1";
   else if (s=="ak5jptl1off")
      return "AK5JPTl1off";
   else if (s=="ak5jptl1l2l3")
      return "AK5JPTl1";
   else if (s=="ak5jptl1offl2l3")
      return "AK5JPTl1off";
   else if (s=="ak7jpt")
      return "AK7JPT";
   else if (s=="ak7jptl1")
      return "AK7JPTl1";
   else if (s=="ak7jptl1off")
      return "AK7JPTl1off";
   else if (s=="sc5calo")
      return "SC5Calo";
   else if (s=="sc5pf")
      return "SC5PF";
   else if (s=="sc7calo")
      return "SC5Calo";
   else if (s=="sc7pf")
      return "SC5PF";
   else if (s=="kt4calo")
      return "KT4Calo";
   else if (s=="kt4pf")
      return "KT4PF";
   else if (s=="kt6calo")
      return "KT6Calo";
   else if (s=="kt6pf")
      return "KT6PF";
   else
      return "unknown";
}

//______________________________________________________________________________
void cmsPrelim(double intLUMI)
{
   const float LUMINOSITY = intLUMI;
   TLatex latex;
   latex.SetNDC();
   latex.SetTextSize(0.045);

   latex.SetTextAlign(31); // align right
   latex.DrawLatex(0.93,0.96,"#sqrt{s} = 8 TeV");
   if (LUMINOSITY > 0.) {
      latex.SetTextAlign(31); // align right
      //latex.DrawLatex(0.82,0.7,Form("#int #font[12]{L} dt = %d pb^{-1}", (int) LUMINOSITY)); //Original
      latex.DrawLatex(0.65,0.85,Form("#int #font[12]{L} dt = %d pb^{-1}", (int) LUMINOSITY)); //29/07/2011
   }
   latex.SetTextAlign(11); // align left
   //latex.DrawLatex(0.16,0.96,"CMS preliminary");// 2012");
   latex.DrawLatex(0.16,0.96,"CMS Simulation Preliminary");// 2012");
}
