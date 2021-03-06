#define Step2Analyzer_cxx
#include "Step2Analyzer.h"
#include <TH2.h>
#include <TStyle.h>
#include <TCanvas.h>





void Step2Analyzer::Loop()
{
	//   In a ROOT session, you can do:
	//      Root > .L Step2Analyzer.C
	//      Root > Step2Analyzer t
	//      Root > t.GetEntry(12); // Fill t data members with entry number 12
	//      Root > t.Show();       // Show values of entry 12
	//      Root > t.Show(16);     // Read and show values of entry 16
	//      Root > t.Loop();       // Loop on all entries
	//

	//     This is the loop skeleton where:
	//    jentry is the global entry number in the chain
	//    ientry is the entry number in the current Tree
	//  Note that the argument to GetEntry must be:
	//    jentry for TChain::GetEntry
	//    ientry for TTree::GetEntry and TBranch::GetEntry
	//
	//       To read only selected branches, Insert statements like:
	// METHOD1:
	//    fChain->SetBranchStatus("*",0);  // disable all branches
	//    fChain->SetBranchStatus("branchname",1);  // activate branchname
	// METHOD2: replace line
	//    fChain->GetEntry(jentry);       //read all branches
	//by  b_branchname->GetEntry(ientry); //read only this branch
	if (fChain == 0) return;


	//Get the 4-vect of Hbb, Wlv
	TLorentzVector bjet0;
	TLorentzVector bjet1;
	TLorentzVector chargelep;
	TString lep_type;
	TLorentzVector met;
	TLorentzVector neutrino;
	TLorentzVector higgs;
	TLorentzVector wlep;

	TH1D hist_Higgs_mass("hist_Higgs_mass","hist_Higgs_mass",50,0,300);
	TH1D hist_V_mass("hist_V_mass","hist_V_mass",50,0,160);
	TH1D    hist_costheta1("hist_costheta1","hist_costheta1",50,-1.2,1.2); 
	TH1D    hist_costheta2("hist_costheta2","hist_costheta2",50,-1.2,1.2);
	TH1D          hist_phi("hist_phi","hist_phi",50,-4,4);
	TH1D hist_costhetastar("hist_costhetastar","hist_costhetastar",50,-1.2,1.2);
	TH1D         hist_phi1("hist_phi1","hist_phi1",50,-4,4);
	TH1D          hist_mVH("hist_mVH","hist_mVH",50,-10,1000);
	TH1D   hist_rapidityVH("hist_rapidityVH","hist_rapidityVH",50,-4,4);


	Long64_t nentries = fChain->GetEntriesFast();

	Long64_t nbytes = 0, nb = 0;
	for (Long64_t jentry=0; jentry<nentries;jentry++) {
		Long64_t ientry = LoadTree(jentry);
		if (ientry < 0) break;
		nb = fChain->GetEntry(jentry);   nbytes += nb;
		if (Cut(ientry) < 0) continue;//pre-selection

		//cout<<"jentry="<<jentry<<endl;

		//Get the 4-vect of Hbb, Wlv
		bjet0.SetPtEtaPhiE(hJet_pt[0],hJet_eta[0], hJet_phi[0],hJet_e[0]);
		bjet1.SetPtEtaPhiE(hJet_pt[1],hJet_eta[1], hJet_phi[1],hJet_e[1]);
		if(vLepton_mass[0]<-1){
			cout<<"Error! Lep mass is nega="<<vLepton_mass[0]<<endl;
			cout<<"nvlep=" <<nvlep<<endl;
			cout<<"nhJets=" <<nhJets<<endl;
		}
		chargelep.SetPtEtaPhiM(vLepton_pt[0], vLepton_eta[0], vLepton_phi[0], vLepton_mass[0]);
		if(vLepton_type[0]==11){ lep_type="electron";}
		else{
			if(vLepton_type[0]==13){ lep_type="muon";
			} else{
				cout<<"Error! Unknow Lep="<<vLepton_type[0];
			}
		}

		//use MET type1 corr 
		//cout<<"MET typ1=( "<< METtype1corr.et<<", "<<METtype1corr.phi<<endl;
		//cout<<"MET unco=( "<< metUnc_et[0]<<", "<<metUnc_phi[0]<<endl;
		met.SetPtEtaPhiE(METtype1corr.et, 0, METtype1corr.phi, METtype1corr.et);
		neutrino=getNeutrino(chargelep, met, lep_type, 0);
		if (neutrino.E()<0){
			//cout<<"neu E="<<neutrino.E()<<endl;
			continue;
		}

		higgs=bjet0+bjet1;
		hist_Higgs_mass.Fill(higgs.M());
		wlep=chargelep+neutrino;
		hist_V_mass.Fill(wlep.M());


		//I assume you have the following four four-vectors:
		//fs_f0 is the four-vector of the fermion particle
		//fs_f1 is the four-vector of the fermion ***anti***-particle
		//fs_b0 is the four-vector of the b jet with the higher pT 
		//fs_b1 is the four-vector of the b jet with the lower pT
		//(Note: we use pT since we don't know which b jet is the particle or anti particle)
		TLorentzVector fs_f0;
		TLorentzVector fs_f1;
		TLorentzVector fs_b0;
		TLorentzVector fs_b1;
		if(vLepton_charge[0]==1){
			fs_f0=neutrino;
			fs_f1=chargelep;
		}else{
			if(vLepton_charge[0]==-1){
				fs_f0=chargelep; 
				fs_f1=neutrino;
			}else{
				cout<<"Error! Unknow charge: "<<vLepton_charge[0]<<endl;
			}
		}
		fs_b0=bjet0; fs_b1=bjet1;
		if(bjet0.Pt() <bjet1.Pt()){
			cout<<"strange: bjet0 pT < bjet1 pT"<<endl;
			fs_b0=bjet1;	fs_b1=bjet0;
		}

		TLorentzVector p4_Vff = fs_f0 + fs_f1; //Four-vector of V that decays to two fermions
		TLorentzVector p4_Hbb = fs_b0 + fs_b1; //Four-vector of H
		TLorentzVector p4_VH = p4_Vff + p4_Hbb; //Four-vector of off-shell V that decays to VH

		Double_t a_costheta1, a_costheta2, a_costhetastar, a_Phi, a_Phi1;
		computeAngles( p4_VH, p4_Vff, fs_f0, fs_f1, p4_Hbb, fs_b0, fs_b1,
					a_costheta1, a_costheta2, a_Phi, a_costhetastar, a_Phi1); 
		//remap to convention of arXiv:1309.4819
		float costheta1 = (float) a_costheta1;
		float costheta2 = (float) a_costhetastar;
		float phi = (float) a_Phi1;
		float costhetastar = TMath::Abs( (float) a_costheta2);
		float phi1 = (float) a_Phi;
		float mVH = (float) p4_VH.M();
		float rapidityVH = (float) p4_VH.Rapidity();

		hist_costheta1.Fill(costheta1); 
		hist_costheta2.Fill(costheta2);
		hist_phi.Fill(phi);
		hist_costhetastar.Fill(costhetastar);
		hist_phi1.Fill(phi1);
		hist_mVH.Fill(mVH);
		hist_rapidityVH.Fill(rapidityVH);

	}

	Draw_and_Save(PlotDir, hist_Higgs_mass);
	Draw_and_Save(PlotDir, hist_V_mass);
	Draw_and_Save(PlotDir,    hist_costheta1);
	Draw_and_Save(PlotDir,    hist_costheta2);
	Draw_and_Save(PlotDir,          hist_phi);
	Draw_and_Save(PlotDir, hist_costhetastar);
	Draw_and_Save(PlotDir,         hist_phi1);
	Draw_and_Save(PlotDir,          hist_mVH);
	Draw_and_Save(PlotDir,   hist_rapidityVH);





}

TLorentzVector getNeutrino(TLorentzVector chargelep, TLorentzVector met, TString lep_type, Int_t diffMode)//diffMode: different neutrino vz 
{
	TLorentzVector p4_neutrino;
	METzCalculator metzcal;
	metzcal.SetMET(met);
	metzcal.SetLepton(chargelep);
	if(lep_type=="electron" || lep_type=="muon") metzcal.SetLeptonType(lep_type.Data());
	else cout<<"Error! Unknow Lep="<<lep_type.Data();
	Double_t pzNu1=metzcal.Calculate(diffMode);
	//Double_t pzNu2=metzcal.getOther();

	Double_t energy=TMath::Sqrt(met.E()*met.E()+pzNu1*pzNu1);
	p4_neutrino.SetPxPyPzE(met.Px(), met.Py(), pzNu1, energy);

	if(metzcal.IsComplex())p4_neutrino.SetPxPyPzE(0.,0.,0.,-10);
	return p4_neutrino;

}


void Draw_and_Save(TString plotDir, TH1D h1, char* addtional_info){
	TCanvas *c1 = new TCanvas(Form("c1_%s",h1.GetTitle()),Form("c1_%s",h1.GetTitle()),200,10,600,600);
	c1->cd();
	h1.Draw();
	if( addtional_info){
		TLatex tl;
		tl.SetTextSize(0.04 ); tl.SetTextAlign(13);
		tl.DrawLatex(h1.GetXaxis()->GetXmin()*0.9+h1.GetXaxis()->GetXmax()*0.1, h1.GetMinimum()*0.1 +h1.GetMaximum()*0.9,addtional_info);
	};
	c1->Print(Form("%s/%s.png",plotDir.Data(), h1.GetTitle()));
	delete c1;
}

//////////////////////////////////
//// P A P E R 4 - V E C T O R D E F I N I T I O N O F P H I A N D P H I 1
//////////////////////////////////
void computeAngles(TLorentzVector thep4H, TLorentzVector thep4Z1, TLorentzVector thep4M11, TLorentzVector thep4M12, TLorentzVector thep4Z2, TLorentzVector thep4M21, TLorentzVector thep4M22, double& costheta1, double& costheta2, double& Phi, double& costhetastar, double& Phi1){

	///////////////////////////////////////////////
	// check for z1/z2 convention, redefine all 4 vectors with convention
	///////////////////////////////////////////////
	TLorentzVector p4H, p4Z1, p4M11, p4M12, p4Z2, p4M21, p4M22;
	p4H = thep4H;

	p4Z1 = thep4Z1; p4M11 = thep4M11; p4M12 = thep4M12;
	p4Z2 = thep4Z2; p4M21 = thep4M21; p4M22 = thep4M22;
	//// costhetastar
	TVector3 boostX = -(thep4H.BoostVector());
	TLorentzVector thep4Z1inXFrame( p4Z1 );
	TLorentzVector thep4Z2inXFrame( p4Z2 );	
	thep4Z1inXFrame.Boost( boostX );
	thep4Z2inXFrame.Boost( boostX );
	TVector3 theZ1X_p3 = TVector3( thep4Z1inXFrame.X(), thep4Z1inXFrame.Y(), thep4Z1inXFrame.Z() );
	TVector3 theZ2X_p3 = TVector3( thep4Z2inXFrame.X(), thep4Z2inXFrame.Y(), thep4Z2inXFrame.Z() );
	costhetastar = theZ1X_p3.CosTheta();

	//// --------------------------- costheta1
	TVector3 boostV1 = -(thep4Z1.BoostVector());
	TLorentzVector p4M11_BV1( p4M11 );
	TLorentzVector p4M12_BV1( p4M12 );	
	TLorentzVector p4M21_BV1( p4M21 );
	TLorentzVector p4M22_BV1( p4M22 );
	p4M11_BV1.Boost( boostV1 );
	p4M12_BV1.Boost( boostV1 );
	p4M21_BV1.Boost( boostV1 );
	p4M22_BV1.Boost( boostV1 );

	TLorentzVector p4V2_BV1 = p4M21_BV1 + p4M22_BV1;
	//// costheta1
	costheta1 = -p4V2_BV1.Vect().Dot( p4M11_BV1.Vect() )/p4V2_BV1.Vect().Mag()/p4M11_BV1.Vect().Mag();

	//// --------------------------- costheta2
	TVector3 boostV2 = -(thep4Z2.BoostVector());
	TLorentzVector p4M11_BV2( p4M11 );
	TLorentzVector p4M12_BV2( p4M12 );	
	TLorentzVector p4M21_BV2( p4M21 );
	TLorentzVector p4M22_BV2( p4M22 );
	p4M11_BV2.Boost( boostV2 );
	p4M12_BV2.Boost( boostV2 );
	p4M21_BV2.Boost( boostV2 );
	p4M22_BV2.Boost( boostV2 );

	TLorentzVector p4V1_BV2 = p4M11_BV2 + p4M12_BV2;
	//// costheta2
	costheta2 = -p4V1_BV2.Vect().Dot( p4M21_BV2.Vect() )/p4V1_BV2.Vect().Mag()/p4M21_BV2.Vect().Mag();

	//// --------------------------- Phi and Phi1
	// TVector3 boostX = -(thep4H.BoostVector());
	TLorentzVector p4M11_BX( p4M11 );
	TLorentzVector p4M12_BX( p4M12 );	
	TLorentzVector p4M21_BX( p4M21 );
	TLorentzVector p4M22_BX( p4M22 );	

	p4M11_BX.Boost( boostX );
	p4M12_BX.Boost( boostX );
	p4M21_BX.Boost( boostX );
	p4M22_BX.Boost( boostX );

	TVector3 tmp1 = p4M11_BX.Vect().Cross( p4M12_BX.Vect() );
	TVector3 tmp2 = p4M21_BX.Vect().Cross( p4M22_BX.Vect() );

	TVector3 normal1_BX( tmp1.X()/tmp1.Mag(), tmp1.Y()/tmp1.Mag(), tmp1.Z()/tmp1.Mag() );
	TVector3 normal2_BX( tmp2.X()/tmp2.Mag(), tmp2.Y()/tmp2.Mag(), tmp2.Z()/tmp2.Mag() );

	//// Phi
	TLorentzVector p4Z1_BX = p4M11_BX + p4M12_BX;
	double tmpSgnPhi = p4Z1_BX.Vect().Dot( normal1_BX.Cross( normal2_BX) );
	double sgnPhi = tmpSgnPhi/fabs(tmpSgnPhi);
	Phi = sgnPhi * acos( -1.*normal1_BX.Dot( normal2_BX) );


	//////////////

	TVector3 beamAxis(0,0,1);
	TVector3 tmp3 = (p4M11_BX + p4M12_BX).Vect();

	TVector3 p3V1_BX( tmp3.X()/tmp3.Mag(), tmp3.Y()/tmp3.Mag(), tmp3.Z()/tmp3.Mag() );
	TVector3 tmp4 = beamAxis.Cross( p3V1_BX );
	TVector3 normalSC_BX( tmp4.X()/tmp4.Mag(), tmp4.Y()/tmp4.Mag(), tmp4.Z()/tmp4.Mag() );

	//// Phi1
	double tmpSgnPhi1 = p4Z1_BX.Vect().Dot( normal1_BX.Cross( normalSC_BX) );
	double sgnPhi1 = tmpSgnPhi1/fabs(tmpSgnPhi1);
	Phi1 = sgnPhi1 * acos( normal1_BX.Dot( normalSC_BX) );

	// std::cout << "extractAngles: " << std::endl;
	// std::cout << "costhetastar = " << costhetastar << ", costheta1 = " << costheta1 << ", costheta2 = " << costheta2 << std::endl;
	// std::cout << "Phi = " << Phi << ", Phi1 = " << Phi1 << std::endl;

}
