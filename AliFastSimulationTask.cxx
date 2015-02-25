//
// Fast simulation task.
//
// Author: S.Aiola

#include "AliFastSimulationTask.h"

#include <TClonesArray.h>
#include <TFolder.h>
#include <TLorentzVector.h>
#include <TParticle.h>
#include <TParticlePDG.h>
#include <TRandom3.h>
#include <TProfile.h>
#include <TH1F.h>
#include <TList.h>
#include <Riostream.h>

#include "AliAnalysisManager.h"
#include "AliGenerator.h"
#include "AliGenPythia.h"
#include "AliHeader.h"
#include "AliLog.h"
#include "AliMCParticle.h"
#include "AliRun.h"
#include "AliRunLoader.h"
#include "AliStack.h"
#include "AliGenPythiaEventHeader.h"

ClassImp(AliFastSimulationTask)

//________________________________________________________________________
AliFastSimulationTask::AliFastSimulationTask() : 
AliAnalysisTaskSE("AliFastSimulationTask"),
  fQAhistos(kFALSE),
  fGen(0),
  fMCParticlesName("GenParticles"),
  fIsInit(kFALSE),
  fStack(0),
  fMCParticles(0),
  fHistTrials(0),
  fHistXsection(0),
  fHistPtHard(0),
  fOutput(0)
{
  // Default constructor.

}

//________________________________________________________________________
AliFastSimulationTask::AliFastSimulationTask(const char *name, Bool_t drawqa) :
  AliAnalysisTaskSE(name),
  fQAhistos(drawqa),
  fGen(0),
  fMCParticlesName("GenParticles"),
  fIsInit(kFALSE),
  fStack(0),
  fMCParticles(0),
  fHistTrials(0),
  fHistXsection(0),
  fHistPtHard(0),
  fOutput(0)
{
  // Standard constructor.

  if (fQAhistos) DefineOutput(1, TList::Class()); 
}

//________________________________________________________________________
AliFastSimulationTask::~AliFastSimulationTask()
{
  // Destructor
}

//________________________________________________________________________
void AliFastSimulationTask::UserCreateOutputObjects()
{
  // Create user output.

  if (!fQAhistos) return;

  OpenFile(1);
  fOutput = new TList();
  fOutput->SetOwner();

  fHistTrials = new TH1F("fHistTrials", "fHistTrials", 1, 0, 1);
  fHistTrials->GetYaxis()->SetTitle("trials");
  fOutput->Add(fHistTrials);

  fHistXsection = new TProfile("fHistXsection", "fHistXsection", 1, 0, 1);
  fHistXsection->GetYaxis()->SetTitle("xsection");
  fOutput->Add(fHistXsection);

  fHistPtHard = new TH1F("fHistPtHard", "fHistPtHard", 500, 0., 500.);
  fHistPtHard->GetXaxis()->SetTitle("p_{T,hard} (GeV/c)");
  fHistPtHard->GetYaxis()->SetTitle("counts");
  fOutput->Add(fHistPtHard);

  PostData(1, fOutput);
}

//________________________________________________________________________
void AliFastSimulationTask::UserExec(Option_t *) 
{
  // Execute per event.

  if (!fIsInit) fIsInit = ExecOnce();

  if (!fIsInit) return;

  Run();
}

//________________________________________________________________________
Bool_t AliFastSimulationTask::ExecOnce() 
{
  // Exec only once.

  if (!gAlice) {
    new AliRun("gAlice","The ALICE Off-line Simulation Framework");
    delete gRandom;
    gRandom = new TRandom3(0);
  }
  
  fGen->SetRandom(gRandom);

  AliGenPythia *genPythia = dynamic_cast<AliGenPythia*>(fGen);
  if (genPythia) genPythia->SetEventListRange(0,1);

  TFolder *folder = new TFolder(GetName(),GetName());
  AliRunLoader *rl = new AliRunLoader(folder);
  rl->MakeHeader();
  rl->MakeStack();
  fStack = rl->Stack();
  fGen->SetStack(fStack);
  fGen->Init();

  if (!(InputEvent()->FindListObject(fMCParticlesName))) {
    fMCParticles = new TClonesArray("AliMCParticle", 1000);
    fMCParticles->SetName(fMCParticlesName);
    InputEvent()->AddObject(fMCParticles);
  }

  if (!(InputEvent()->FindListObject(fStack->GetName()))) {
    InputEvent()->AddObject(fStack);
  }
  
  return kTRUE;
}

//________________________________________________________________________
void AliFastSimulationTask::Run() 
{
  // Run simulation.

  fMCParticles->Clear("C");

  fStack->Reset();
  fGen->Generate();
  const Int_t nprim = fStack->GetNprimary();
  Int_t nParticles = 0;
  for (Int_t i=0; i < nprim; i++) {
    TParticle *part = fStack->Particle(i);
    if (!part) continue;

    AliMCParticle *mcpart = new ((*fMCParticles)[nParticles]) AliMCParticle(part, 0, i);
    mcpart->SetMother(part->GetFirstMother());

    nParticles++;
  }

  FillPythiaHistograms();
}

//________________________________________________________________________
void AliFastSimulationTask::FillPythiaHistograms() 
{
  //Get PYTHIA info: pt-hard, x-section, trials

  if (!fQAhistos)
    return;

  AliRunLoader *rl = AliRunLoader::Instance();
  AliGenPythiaEventHeader *genPH = dynamic_cast<AliGenPythiaEventHeader*>(rl->GetHeader()->GenEventHeader());
  if(genPH) {
    Float_t xsec = genPH->GetXsection();
    Int_t trials = genPH->Trials();
    Float_t pthard = genPH->GetPtHard();

    fHistXsection->Fill(0.5,xsec);
    fHistTrials->Fill(0.5,trials);
    fHistPtHard->Fill(pthard);
  }
}
