//
// jet_combine_closure_pt_x
// ------------------------
//
//            10/18/2011 Alexx Perloff  <aperloff@physics.tamu.edu>
///////////////////////////////////////////////////////////////////

#include "JetMETAnalysis/JetAnalyzers/interface/Settings.h"
#include "JetMETAnalysis/JetAnalyzers/interface/Style.h"
#include "JetMETAnalysis/JetUtilities/interface/CommandLine.h"

#include "TROOT.h"
#include "TSystem.h"
#include "TStyle.h"
#include "TFile.h"
#include "TCanvas.h"
#include "TH1.h"
#include "TH1F.h"
#include "TH1D.h"
#include "TH2.h"
#include "TH2F.h"
#include "TF1.h"
#include "TString.h"
#include "TPaveText.h"
#include "TLegend.h"
#include "TLatex.h"

#include <fstream>
#include <string>
#include <stdio.h>
#include <stdarg.h>

using namespace std;

////////////////////////////////////////////////////////////////////////////////
// define local functions
////////////////////////////////////////////////////////////////////////////////

///CMS Preliminary label;
void cmsPrelim(double intLUMI = 0);

////////////////////////////////////////////////////////////////////////////////
// main
////////////////////////////////////////////////////////////////////////////////

//______________________________________________________________________________
int main(int argc,char**argv)
{
  gROOT->SetStyle("Plain");
  gStyle->SetOptStat(0);

  gSystem->Load("libFWCoreFWLite.so");

  //
  // evaluate command-line / configuration file options
  // 
  CommandLine cl;
  if (!cl.parse(argc,argv)) return 0;
  
  vector<TString> algs         = cl.getVector<TString> ("algs");
  TString         flavor       = cl.getValue<TString>  ("flavor",           "");
  vector<TString> paths        = cl.getVector<TString> ("paths",            "");
  TString         outputDir    = cl.getValue<TString>  ("outputDir",  "images");
  TString         outputFormat = cl.getValue<TString>  ("outputFormat", ".png");
  bool            combinePU    = cl.getValue<bool>     ("combinePU",     false);
  TString         divByNPU0    = cl.getValue<TString>  ("divByNPU0",        "");
  bool            tdr          = cl.getValue<bool>     ("tdr",           false);

  if (!cl.check()) return 0;
  cl.print();

  if (tdr) {
     setStyle();
  }

  vector<TFile*> infs;
  vector<TFile*> infsNPU0;
  TString algNames;
  vector<TString> puLabelsVec;
  TString puLabels;
  for(unsigned int a=0; a<algs.size(); a++)
    {
      //
      // Open the file containing the original, unformated closure plots
      //
      if(paths.empty()) paths.push_back(string (gSystem->pwd())+"/");
      for(unsigned int p=0; p<paths.size(); p++)
        {     
          if(!paths[p].EndsWith("/")) paths[p]+="/";
          if(!divByNPU0.IsNull() && !divByNPU0.EndsWith("/")) divByNPU0+="/";

          if (!flavor.IsNull()) infs.push_back(new TFile(paths[p]+"ClosureVsPt_"+algs[a]+"_"+flavor+".root"));
          else infs.push_back(new TFile(paths[p]+"ClosureVsPt_"+algs[a]+".root"));
          if(!infs.back()->IsOpen())
            {
              cout << "WARNING:File " << paths[p] << "ClosureVsPt_" << algs[a] << ".root is was not opened." << endl << " Check the path and filename and try again." << endl << " The program will now exit." << endl;
              return 0;
            }

          if(!divByNPU0.IsNull())
            {
              if (!flavor.IsNull()) infsNPU0.push_back(new TFile(divByNPU0+"ClosureVsPt_"+algs[a]+"_"+flavor+".root"));
              else infsNPU0.push_back(new TFile(divByNPU0+"ClosureVsPt_"+algs[a]+".root"));
              if(!infsNPU0.back()->IsOpen())
                {
                  cout << "WARNING:File " << divByNPU0 << "ClosureVsPt_" << algs[a] << ".root is was not opened." << endl << " Check the path and filename and try again." << endl << " The program will now exit." << endl;
              return 0;
                }
            }

          TString path(paths[p]);
          puLabelsVec.push_back(path((TString(path(0,path.Length()-1))).Last('/')+1, (TString(path(0,paths[p].Length()-2))).Length() - (TString(path(0,path.Length()-1))).Last('/')));
          if(a==0)
            {
              if(p==0) puLabels+=puLabelsVec.back();
              else puLabels+="_"+puLabelsVec.back();
            }
        }

      if(a==0) algNames+=algs[a];
      else algNames+="_"+algs[a];
    }

  //
  // Create guides (lines) for the output histograms
  //
  TF1 *line = new TF1("line","0*x+1",0,5000);
  line->SetLineColor(1);
  line->SetLineWidth(1);
  line->SetLineStyle(2);
  TF1 *linePlus = new TF1("linePlus","0*x+1.02",0,5000);
  linePlus->SetLineColor(1);
  linePlus->SetLineWidth(1);
  linePlus->SetLineStyle(2);
  TF1 *lineMinus = new TF1("lineMinus","0*x+0.98",0,5000);
  lineMinus->SetLineColor(1);
  lineMinus->SetLineWidth(1);
  lineMinus->SetLineStyle(2);
  
  TCanvas *can[3];
  TString Text[3] = {"|#eta| < 1.3","1.3 < |#eta| < 3","3 < |#eta| < 5"};
  TPaveText *pave[3];

  double XminCalo[3] = {15,15,15};
  double XminPF[3] = {5,5,5};
  double Xmax[3] = {3000,3000,190};
  
  char name[1024];
  TH1F *h=0;
  vector<vector<TH1F*> > hNPU0 (3,vector<TH1F*>(infs.size(),h));
  vector<vector<TH1F*> > hClosure (3,vector<TH1F*>(infs.size(),h));
  //cout << "size = " << infs.size() << endl;
  vector<TLegend*> leg;

  //
  // Open/create the output directory and file
  //
  if(!outputDir.EndsWith("/")) outputDir+="/";
  if(!gSystem->OpenDirectory(outputDir)) gSystem->mkdir(outputDir);
  TString ofname = outputDir+"ClosureVsPt_"+algNames+".root";
  if(combinePU) ofname.ReplaceAll("ClosureVsPt_","ClosureVsPt_"+puLabels+"_");
  if(!flavor.IsNull()) ofname.ReplaceAll(".root","_"+flavor+".root");
  TFile* outf = new TFile(ofname,"RECREATE");

  //
  // Format and save the output
  //
  for(int j=0;j<3;j++)
    {
      leg.push_back(new TLegend(0.7,0.8,1.0,1.0));
      pave[j] = new TPaveText(0.3,0.9,0.8,1.0,"NDC");
      pave[j]->AddText(Text[j]);      
      sprintf(name,"ClosureVsPt_%d",j);
      TString ss(name);
      if(!divByNPU0.IsNull()) ss.ReplaceAll("ClosureVsPt","ClosureVsPtRelNPU0");
      if(combinePU) ss+="_"+puLabels+"_"+algNames;
      else ss+="_"+algNames;
      if(!flavor.IsNull()) ss+="_"+flavor;
      can[j] = new TCanvas(ss,ss,800,800);
      if (j<2)
        gPad->SetLogx();
  
      for(unsigned int a=0; a<infs.size(); a++)
        {
          cout << "sfsg1" << endl;
          if(j==0)
            {
              hClosure[j][a] = (TH1F*)infs[a]->Get("ClosureVsPt_Bar");
              if(!divByNPU0.IsNull())
                {
                  hNPU0[j][a] = (TH1F*)infsNPU0[a]->Get("ClosureVsPt_Bar");
                }
            }
          else if(j==1)
            {
              hClosure[j][a] = (TH1F*)infs[a]->Get("ClosureVsPt_End");
              if(!divByNPU0.IsNull())
                { 
                  hNPU0[j][a] = (TH1F*)infsNPU0[a]->Get("ClosureVsPt_End");
                }
            }
          else if(j==2)
            {
              hClosure[j][a] = (TH1F*)infs[a]->Get("ClosureVsPt_Fwd");
              if(!divByNPU0.IsNull())
                {
                  hNPU0[j][a] = (TH1F*)infsNPU0[a]->Get("ClosureVsPt_Fwd");
                }
            }

          if(!divByNPU0.IsNull() && !puLabelsVec[a].Contains("0_0"))
            {
              hNPU0[j][a]->Sumw2();
              hClosure[j][a]->Divide(hNPU0[j][a]);
            }

          if (ss.Contains("pf"))	
            hClosure[j][a]->GetXaxis()->SetRangeUser(XminPF[j],Xmax[j]);
          else
            hClosure[j][a]->GetXaxis()->SetRangeUser(XminCalo[j],Xmax[j]);	    
          hClosure[j][a]->GetXaxis()->SetTitle("GenJet p_{T} (GeV)"); 
          hClosure[j][a]->GetYaxis()->SetTitle("Corrected Response");
          hClosure[j][a]->GetYaxis()->SetTitleOffset(1.25);
          hClosure[j][a]->GetXaxis()->SetLabelSize(0.035);
          hClosure[j][a]->GetXaxis()->SetMoreLogLabels();
          hClosure[j][a]->GetXaxis()->SetNoExponent();
          hClosure[j][a]->GetYaxis()->SetLabelSize(0.035); 
          if (tdr) {
             hClosure[j][a]->SetMarkerStyle(20);
             hClosure[j][a]->SetMarkerSize(0.5);
          }
          else {
             hClosure[j][a]->SetMarkerSize(2.0);
          }
          hClosure[j][a]->SetMarkerColor(a+1);
          hClosure[j][a]->SetLineColor(a+1);
          hClosure[j][a]->SetMaximum(1.1);
          hClosure[j][a]->SetMinimum(0.9);
          if(!divByNPU0.IsNull() && !puLabelsVec[a].Contains("0_0"))
            {
              hClosure[j][a]->GetYaxis()->SetTitle("p_{T}^{RECO}/p_{T}^{GEN} (Relative to NPU=0)");
              hClosure[j][a]->GetXaxis()->SetTitle("p_{T}^{GEN} (GeV/c)");
              hClosure[j][a]->SetTitle("N_{PU}^{GEN} = "+puLabelsVec[a]);
              //hClosure[j][a]->GetXaxis()->SetRangeUser(30,500);
              //hClosure[j][a]->SetMaximum(1.3);
              //hClosure[j][a]->SetMinimum(0.98);
              //gPad->SetLogx(0);
            }

          if(a==0)
              hClosure[j][a]->Draw();
          else
            hClosure[j][a]->Draw("same");

          //cout << "a = " << a << "    hClosure[" << j << "][" << a <<"] = " << hClosure[j][a] << endl;
          //if (a>=4) hClosure[j][4]->Draw("same");
          hClosure[j][a]->Write();

          if(combinePU) leg[j]->AddEntry(hClosure[j][a],puLabelsVec[a]+"_"+algs[a/paths.size()],"le");
          else leg[j]->AddEntry(hClosure[j][a],algs[a/paths.size()],"le");
        }
      line->Draw("same");
      linePlus->Draw("same");
      lineMinus->Draw("same");
      pave[j]->SetFillColor(0);
      pave[j]->SetBorderSize(0);
      pave[j]->SetTextFont(42);
      pave[j]->SetTextSize(0.05);
      pave[j]->Draw();
      leg[j]->SetFillColor(0);
      leg[j]->SetLineColor(0);
      leg[j]->Draw("same");
      if (tdr) cmsPrelim();
      can[j]->SaveAs(outputDir+ss+outputFormat);
      can[j]->Write();

    }
  outf->Close();
  
}

////////////////////////////////////////////////////////////////////////////////
// implement local functions
////////////////////////////////////////////////////////////////////////////////

//______________________________________________________________________________
void cmsPrelim(double intLUMI)
{
   const float LUMINOSITY = intLUMI;
  TLatex latex;
  latex.SetNDC();
  latex.SetTextSize(0.04);

  latex.SetTextAlign(31); // align right
  latex.DrawLatex(0.93,0.96,"#sqrt{s} = 7 TeV");
  if (LUMINOSITY > 0.) {
    latex.SetTextAlign(31); // align right
    //latex.DrawLatex(0.82,0.7,Form("#int #font[12]{L} dt = %d pb^{-1}", (int) LUMINOSITY)); //Original
    latex.DrawLatex(0.65,0.85,Form("#int #font[12]{L} dt = %d pb^{-1}", (int) LUMINOSITY)); //29/07/2011
  }
  latex.SetTextAlign(11); // align left
  latex.DrawLatex(0.16,0.96,"CMS preliminary 2012");
}
