#include "Math/Vector3D.h"
#include "Math/Vector4D.h"
#include "TDataFrame2.hpp"
#include "TFile.h"
#include "TMath.h"
#include "TTree.h"
#include "TRandom3.h"
#include "ROOT/TSeq.hxx"
#include <cassert>
#include <iostream>


using FourVector = ROOT::Math::XYZTVector;
using FourVectors = std::vector<FourVector>;
using CylFourVector = ROOT::Math::RhoEtaPhiVector;

void getTracks(FourVectors& tracks) {
   static TRandom3 R(1);
   const double M = 0.13957;  // set pi+ mass
   auto nPart = R.Poisson(5);
   tracks.clear();
   tracks.reserve(nPart);
   for (auto j : ROOT::TSeqI(nPart)) {
      double px = R.Gaus(0,10);
      double py = R.Gaus(0,10);
      double pt = sqrt(px*px +py*py);
      double eta = R.Uniform(-3,3);
      double phi = R.Uniform(0.0 , 2*TMath::Pi() );
      CylFourVector vcyl( pt, eta, phi);
      // set energy
      double E = sqrt( vcyl.R()*vcyl.R() + M*M);
      FourVector q( vcyl.X(), vcyl.Y(), vcyl.Z(), E);
      // fill track vector
      tracks.emplace_back(q);
   }
}

// A simple helper function to fill a test tree and save it to file
// This makes the example stand-alone
void FillTree(const char* filename, const char* treeName) {
   TFile f(filename,"RECREATE");
   TTree t(treeName,treeName);
   double b1;
   int b2;
   std::vector<FourVector> tracks;
   t.Branch("b1", &b1);
   t.Branch("b2", &b2);
   t.Branch("tracks", &tracks);

   for(auto i : ROOT::TSeqI(20)) {
      b1 = i;
      b2 = i*i;
      getTracks(tracks);
      t.Fill();
   }
   t.Write();
   f.Close();
   return;
}

template<class T>
void CheckRes(const T& v, const T& ref, const char* msg) {
   if (v!=ref) {
      std::cerr << "***FAILED*** " << msg << std::endl;
   }
}

int main() {
   // Prepare an input tree to run on
   auto fileName = "myfile.root";
   auto treeName = "myTree";
   FillTree(fileName,treeName);

   TFile f(fileName);
   // Define data-frame
   TDataFrame d(treeName, &f);
   // ...and two dummy filters
   auto ok = []() { return true; };
   auto ko = []() { return false; };

   // TEST 1: no-op filter and Run
   d.Filter(ok, {}).Foreach([](double x) { std::cout << x << std::endl; }, {"b1"});
   d.Run();

   // TEST 2: Forked actions
   // always apply first filter before doing three different actions
   auto dd = d.Filter(ok, {});
   dd.Foreach([](double x) { std::cout << x << " "; }, {"b1"});
   dd.Foreach([](int y) { std::cout << y << std::endl; }, {"b2"});
   auto c = dd.Count();
   // ... and another filter-and-foreach
   auto ddd = dd.Filter(ko, {});
   ddd.Foreach([]() { std::cout << "ERROR" << std::endl; }, {});
   d.Run();
   auto cv = *c.get();
   std::cout << "c " << cv << std::endl;
   CheckRes(cv,20U,"Forked Actions");

   // TEST 3: default branches
   TDataFrame d2(treeName, &f, {"b1"});
   auto d2f = d2.Filter([](double b1) { return b1 < 5; }).Filter(ok, {});
   auto c2 = d2f.Count();
   d2f.Foreach([](double b1) { std::cout << b1 << std::endl; });
   d2.Run();
   auto c2v = *c2.get();
   std::cout << "c2 " << c2v << std::endl;
   CheckRes(c2v,5U,"Default branches");

   // TEST 4: execute Run lazily and implicitly
   TDataFrame d3(treeName, &f, {"b1"});
   auto d3f = d3.Filter([](double b1) { return b1 < 4; }).Filter(ok, {});
   auto c3 = d3f.Count();
   auto c3v = *c3.get();
   std::cout << "c3 " << c3v << std::endl;
   CheckRes(c3v,4U,"Execute Run lazily and implicitly");

   // TEST 5: non trivial branch
   TDataFrame d4(treeName, &f, {"tracks"});
   auto d4f = d4.Filter([](FourVectors const & tracks) { return tracks.size() > 7; });
   auto c4 = d4f.Count();
   auto c4v = *c4.get();
   std::cout << "c4 " << c4v << std::endl;
   CheckRes(c4v,1U,"Non trivial test");

   // TEST 6: Create a histogram
   TDataFrame d5(treeName, &f);
   auto h = d5.Histo("b1");
   std::cout << "Histo: nEntries " << h->GetEntries() << std::endl;

   return 0;
}
