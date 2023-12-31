/*
 * PndMLTracking.cxx
 *
 *  Created on: Nov 8, 2018
 *      Author: Adeel Akram
 */

#include <FairRootManager.h>
#include <FairRunAna.h>
#include <FairRuntimeDb.h>
#include <PndSttMapCreator.h>
#include <PndSttTube.h>
#include <PndTrackCand.h>
#include <TClonesArray.h>

#include "FairTask.h"
#include "FairMCPoint.h"
#include "PndMLTracking.h"

ClassImp(PndMLTracking)

// PndMLTracking()
PndMLTracking::PndMLTracking() {

    fSttParameters = nullptr;
	fEventHeader = nullptr;
	fTubeArray = nullptr;
	fMCTrackArray = nullptr;
	fSttPointArray = nullptr;
	fSttHitArray = nullptr;
	mcTrackBranchID = -1;
	sttPointBranchID = -1;
	sttHitBranchID = -1;
	fHitId = 0;
	fEventId = 0;
	fCsvFilesPath = "./data/";
	
}

// ~PndMLTracking()
PndMLTracking::~PndMLTracking() {
}

// SetParContainers()
void PndMLTracking::SetParContainers() {

	FairRuntimeDb *rtdb = FairRunAna::Instance()->GetRuntimeDb();
	fSttParameters = (PndGeoSttPar*) rtdb->getContainer("PndGeoSttPar");

}

// Init()
InitStatus PndMLTracking::Init() {

	// Get instance of the FairRootManager to access tree branches
	FairRootManager *ioman = FairRootManager::Instance();
	if(!ioman) 
	{
		std::cout << "-E- PndMLTracking::Init: FairRootManager not instantiated!" << std::endl;
		return kFATAL;
	}

	// Get the EventHeader
	fEventHeader = (TClonesArray*) ioman->GetObject("EventHeader.");
	if(!fEventHeader) 
	{
		std::cout << "-E- PndMLTracking::Init: EventHeader not loaded!" << std::endl;
		return kFATAL;
	}

	// Access STTHit branch and determine its branch ID
	fSttHitArray = (TClonesArray*) ioman->GetObject("STTHit");
	sttHitBranchID = ioman->GetBranchId("STTHit");
	
	// Access STTPoint branch and determine its branch ID
	fSttPointArray = (TClonesArray*) ioman->GetObject("STTPoint");
	sttPointBranchID = ioman->GetBranchId("STTPoint");
	
	// Access MCTrack branch and determine its branch ID
	fMCTrackArray = (TClonesArray*) ioman->GetObject("MCTrack");
	mcTrackBranchID = ioman->GetBranchId("MCTrack");
	
	// Access STTMapCreater
	PndSttMapCreator *mapper = new PndSttMapCreator(fSttParameters);
	fTubeArray = mapper->FillTubeArray();
	      
	std::cout << "-I- PndMLTracking: Initialisation successful" << std::endl;
	return kSUCCESS;
}

// Exec()
void PndMLTracking::Exec(Option_t* /*opt*/) {
	
	//std::cout << "Processing Event: " << std::endl;

    // Debugging Tasks
    // TODO: Find STTHits and STTPoints Belonging to Primary Tracks (Kind of Filtering)
    
    // 1 - Get STTPoint connected to STTHit
    // 2 - Get STTPoint connected to MCTrack
    // 3 - Filter STTPoints/STTHits if IsGeneratorCreated()
    // 4 - Get TrackID/MCTrackID of STTPoint/STTHit
    
	/*
    // Start with STTHits
	int counter=0;
	std::cout << "\nEvent# " << fEventId << " SttHits (Total): " << fSttHitArray->GetEntriesFast() << std::endl;
	for (int idx = 0; idx < fSttHitArray->GetEntriesFast(); idx++) {
	
		// Get FairRootManager Instance
  		FairRootManager *ioman = FairRootManager::Instance();
  		
		// Get sttHitsLinks from STTHit
		FairMultiLinkedData_Interface *sttHitsLinks = (FairMultiLinkedData_Interface*)fSttHitArray->At(idx);
		
		// Get sttPointLinks & Data (TCloneArray) from sttHitsLinks
		FairMultiLinkedData sttPointLinks = sttHitsLinks->GetLinksWithType(sttPointBranchID);
		FairMCPoint *sttpoint = (FairMCPoint*)ioman->GetCloneOfLinkData(sttPointLinks.GetLink(0));
			        
        // Terminate if sttpoint=NULL
        if (sttpoint == 0) {continue;}
        
        // Get mcTrackLinks & Data (TCloneArray) from sttpoint (OR sttPointLinks?)
        FairMultiLinkedData mcTrackLinks = sttpoint->GetLinksWithType(mcTrackBranchID); 
        PndMCTrack *mctrack = (PndMCTrack*)ioman->GetCloneOfLinkData(mcTrackLinks.GetLink(0));
        
        // Terminate if mcTrack=NULL
        if (mctrack == 0) {continue;}

        // Terminate if not GeneratorCreated
        if (!mctrack->IsGeneratorCreated())
        {	
        	//std::cout << "Excluding STTPoint " << std::endl;
        	continue;
        }
        
        counter++;
        
        // Write STTHits and STTPoints (Primary Tracks) to a CSV
        // TODO: Moved GenerateData()
    }
	*/
	
	
	// Call GenerateData() to Write Data into CSV Files (Similar to TrackML)
	// GenerateData();
    GenerateData_V2();
    
	std::cout << "-I- PndMLTracking: Exec Successful with Event: " << (fEventId - 1) << std::endl;
	    
}//end-Exec()


// FinishTask()
void PndMLTracking::FinishTask() {

	// Close all files
	fHits.close();
	fTruths.close();
	fParticles.close();
	fTubes.close();

	std::cout << "-I- Task Generating CSVs has Finished." << std::endl;
}


// GenerateData_V2()
void PndMLTracking::GenerateData_V2() {
		
	// Get Event Index
	std::stringstream ss;
	ss << std::setw(10) << std::setfill('0') << fEventId;
	std::string fidx = ss.str();
	
	// std::cout << "Processing Event: " << "event"+fidx << std::endl;

	// Open Files (CSV)
	fHits.open(fCsvFilesPath+"event"+fidx+"-hits.csv");
	fTruths.open(fCsvFilesPath+"event"+fidx+"-truth.csv");
	fParticles.open(fCsvFilesPath+"event"+fidx+"-particles.csv");
	fTubes.open(fCsvFilesPath+"event"+fidx+"-cells.csv");
	
	// ------------------------------------------------------------------------
    //                            Add CSV Headers                  
    // ------------------------------------------------------------------------
    
    /* ------------------------------------------------------------------------
	* Event Hits: (STTHits)
	*
    * The hits file contains the following values for each hit/entry:
    *
    *  - hit_id : numerical identifier of the hit inside the event.
    *  - x, y, z : measured x, y, z position (in mm) of the hit in global coordinates.
    *  - volume_id : numerical identifier of the detector group.
    *  - layer_id : numerical identifier of the detector layer inside the group.
    *  - module_id : numerical identifier of the detector module inside the layer.
    *
    * The volume/layer/module id could in principle be deduced from x, y, z. They are 
    * given here to simplify detector-specific data handling.
    *
	* -----------------------------------------------------------------------*/
	
    fHits   << "hit_id" << ","
            << "x" << "," 
            << "y" << "," 
            << "z" << ","
            << "volume_id" << ","       // e.g. STT (sub-detectors)
            << "layer_id"  << ","       // e.g. layer_id in STT
            << "module_id"              // e.g. module_id/tube_id -> layer_id -> STT
            << std::endl;
    
    /* ------------------------------------------------------------------------
    * Event Truth:
    *
    * The truth file contains the mapping between hits and generating particles 
    * and the true particle state at each measured hit. Each entry maps one hit 
    * to one particle.
    *
    * - hit_id: numerical identifier of the hit as defined in the hits file.
    * - particle_id: numerical identifier of the generating particle as defined 
    *                in the particles file. A value of 0 means that the hit did 
    *                not originate from a reconstructible 
    *                particle, but e.g. from detector noise.
    *
    * - tx, ty, tz: true intersection point in global coordinates (in mm)
    *               between the particle trajectory and the sensitive surface.
    *
    * - tpx,tpy,tpz: true particle momentum (in GeV/c) in the global coordinate 
    *                system at the intersection point. The corresponding vector 
    *               is tangent to the particle trajectory at intersection point.
    *
    * - weight: per-hit weight used for the scoring metric; total sum of weights 
    *           within one event equals to one.
    * -----------------------------------------------------------------------*/
    
    fTruths << "hit_id" << ","
        	<< "tx"  << ","
            << "ty"  << ","
            << "tz"  << ","
            << "tpx" << ","
            << "tpy" << ","
            << "tpz" << ","
            << "weight" << ","
            << "particle_id"
            << std::endl;
    
    /* ------------------------------------------------------------------------
    * Event Particles
    *
    * The particles files contains the following values for each particle/entry:
    * 
    * - particle_id: numerical identifier of the particle inside the event.
    * - particle_type: numerical identifier of the particle type (PDG CODE)
    * - vx, vy, vz: initial position or vertex (in mm) in global coordinates.
    * - px, py, pz: initial momentum (in GeV/c) along each global axis.
    * - q: particle charge (as multiple of the absolute electron charge).
    * - nhits: number of hits generated by this particle.
    * 
    * All entries contain the generated information or ground truth.
    * -----------------------------------------------------------------------*/
    
    fParticles  << "particle_id" <<","  // identifier of a track (track_id)
                << "vx" << ","
                << "vy" << ","
                << "vz" << ","
                << "px" << ","
                << "py" << ","
                << "pz" << ","
                << "q"  << ","
                << "nhits" << ","
                << "pdgcode" << ","
                << "start_time"
                << std::endl;
    
    /* ------------------------------------------------------------------------
    * Event Hit Cells/Tubes
    *
    * The cells file contains the constituent active detector cells that comprise
    * each hit. The cells can be used to refine the hit to track association. A 
    * cell is the smallest granularity inside each detector module, much like a 
    * pixel on a screen, except that depending on the volume_id a cell can be a 
    * square or a long rectangle. It is identified by two channel identifiers that
    * are unique within each detector module and encode the position, much like 
    * column/row numbers of a matrix. A cell can provide signal information that 
    * the detector module has recorded in addition to the position. Depending on
    * the detector type only one of the channel identifiers is valid, e.g. for the
    * strip detectors, and the value might have different resolution.

    * - hit_id: numerical identifier of the hit as defined in the hits file.
    * - isochrone: signal value information, e.g. how much charge a particle has deposited.
    * - depcharge:
    * - energyloss:
    *
    * TODO: Refine this COMMENT.
    * -----------------------------------------------------------------------*/

	// Future Add: GetHalfLength(), GetWireDirection(), GetRadIn(), GetRadOut(),
	//             GetNeighborings(), GetDistance(), IsParallel()

	fTubes  << "hit_id"    << ","
            << "isochrone" << "," 
            << "depcharge" << "," 
            << "energyloss"<< ","
            << "volume_id" << ","       // e.g. STT (sub-detectors)
            << "layer_id"  << ","       // e.g. layer_id in STT
            << "module_id"   << ","       // e.g. tube_id -> layer_id -> STT
            << "skewed"    << ","       // if tube_id is skewed.
            << "sector_id"              // in which sector tube exists.
            <<std::endl;
	
	// ------------------------------------------------------------------------
    //               Event Loop (fSttHitArray) to Write Data into CSVs
    // ------------------------------------------------------------------------
    int hit_counter = 0;
    
	for (int hit_idx=0; hit_idx < fSttHitArray->GetEntriesFast(); hit_idx++) {
		
		// Get FairRootManager Instance
  		FairRootManager *ioman = FairRootManager::Instance();
  		
		// Get sttHitsLinks from STTHit
		FairMultiLinkedData_Interface *sttHitsLinks = (FairMultiLinkedData_Interface*)fSttHitArray->At(hit_idx);
		
		// Get sttPointLinks & Data (TCloneArray) from sttHitsLinks
		FairMultiLinkedData sttPointLinks = sttHitsLinks->GetLinksWithType(sttPointBranchID);
		FairMCPoint *sttpoint = (FairMCPoint*)ioman->GetCloneOfLinkData(sttPointLinks.GetLink(0));
			        
        // Terminate if sttpoint=NULL
        if (sttpoint == 0) {continue;}
        
        // Get mcTrackLinks & Data (TCloneArray) from sttpoint (OR sttPointLinks?)
        FairMultiLinkedData mcTrackLinks = sttpoint->GetLinksWithType(mcTrackBranchID); 
        PndMCTrack *mctrack = (PndMCTrack*)ioman->GetCloneOfLinkData(mcTrackLinks.GetLink(0));
        
        // Terminate if mcTrack=NULL
        if (mctrack == 0) {continue;}
        
        // Terminate if not Primary
        if (!mctrack->IsGeneratorCreated())
        	continue;
        
        // Hit Counter (very important in case number of tracks vary per event)
        hit_counter++;


        // --------------------------------------------------------------------
		//                     Event Hits (e.g. SttHits)
		// --------------------------------------------------------------------
		
		// First Get access to SttHit & PndSttTube
		PndSttHit* stthit = (PndSttHit*)fSttHitArray->At(hit_idx);
		PndSttTube *tube = (PndSttTube*) fTubeArray->At(stthit->GetTubeID());
		
		// Write to xxx-hits.csv
		fHits << hit_counter 		    << ","      // hit_id
			  << stthit->GetX() 		<< ","      // x-position
			  << stthit->GetY() 		<< ","      // y-position
			  << stthit->GetZ() 		<< ","      // z-position
			  << stthit->GetDetectorID()<< ","      // volume_id
			  << tube->GetLayerID() 	<< ","      // layer_id
			  << stthit->GetTubeID()                // tube_id/module_id
              << std::endl;
        // --------------------------------------------------------------------
        //                     Event Truth (SttPoint: MCPoint in STT)
        // --------------------------------------------------------------------
        
        // Write to xxx-truth.csv	    
    	fTruths << hit_counter       << ","   // hit_id  
				<< sttpoint->GetX()	 << ","   // tx = true x
				<< sttpoint->GetY()  << ","   // ty = true y
		      	<< sttpoint->GetZ()  << ","   // tz = true z
		      	<< sttpoint->GetPx() << ","   // tpx = true px
		      	<< sttpoint->GetPy() << ","   // tpy = true py
		      	<< sttpoint->GetPz() << ","   // tpz = true pz
		      	<< (0.0)             << ",";  // weight (will be changed in future)
        
        
        // Get MCTrack ID associated with each hit_idx (fSttHitArray)
        std::vector<FairLink> mcTrackLinks_sorted = sttHitsLinks->GetSortedMCTracks();
		for (unsigned int trackIndex = 0; trackIndex < mcTrackLinks_sorted.size(); trackIndex++) {
		
			// Append "particle_id" to xxx-truth.csv
			fTruths << std::to_string(mcTrackLinks_sorted[trackIndex].GetIndex() + 1);
		}
		
		fTruths << std::endl;
		
		// --------------------------------------------------------------------
		//                            Event Hit Cells/Tubes
		// --------------------------------------------------------------------
		
		// Future Add: GetHalfLength(), GetWireDirection(), GetRadIn(), GetRadOut(),
		//             GetNeighborings(), GetDistance(), IsParallel()
		
		// Write to xxx-cells.csv
		fTubes  << hit_counter 			   << ","   // hit_id
                << stthit->GetIsochrone()  << ","   // isochrone radius
                << stthit->GetDepCharge()  << ","   // deposited charge
                << stthit->GetEnergyLoss() << ","   // energy loss
			  	<< stthit->GetDetectorID() << ","   // volume_id
			  	<< tube->GetLayerID()      << ","   // layer_id
                << stthit->GetTubeID()	   << ","   // tube_id/module_id
                << tube->IsSkew() 		   << ","   // if tube_id is skewed.
                << tube->GetSectorID()	            // in which sector tube exists.
			    << std::endl;

		// --------------------------------------------------------------------
		//                            fParticles
		// --------------------------------------------------------------------
		
		// TODO: The issue is fParticles should contain unique particle_ids
		// e.g. if there are 10 particles/event then it should contain 10 records.
		// But right now fParticles contain a particle record = # of fSttHitArray.
		// One solution might be to write the fParticles outside of fSttHitArray.
		
		/*
		//std::cout << "Number of MCTracks (fParticles): " << mcTrackLinks_sorted.size() << std::endl;
		
		//std::vector<FairLink> mcTrackLinks_sorted = sttHitsLinks->GetSortedMCTracks();
		for (unsigned int trackIndex=0; trackIndex < mcTrackLinks_sorted.size(); trackIndex++) {
		
		    std::cout << "Number of MCTracks (fParticles): " << trackIndex << std::endl;
		    
			PndMCTrack *mcTrack = (PndMCTrack*)ioman->GetCloneOfLinkData(mcTrackLinks_sorted[trackIndex]);
			
			int q =0;
			if (mcTrack->GetPdgCode()==13)           //mu+
			    q = 1;
			if (mcTrack->GetPdgCode()==-13)          //mu-
			    q = -1;
			
			
			// CSV:: Writting Info to CSV File. 		   
			fParticles 	<< std::to_string(mcTrackLinks_sorted[trackIndex].GetIndex() + 1) << ","  // track_id > 0
			
						<< (mcTrack->GetStartVertex()).X() << ","   // vx = start x
						<< (mcTrack->GetStartVertex()).Y() << ","   // vy = start y
						<< (mcTrack->GetStartVertex()).Z() << ","   // vz = start z
						<< (mcTrack->GetMomentum()).X()    << ","   // px = x-component of track momentum
						<< (mcTrack->GetMomentum()).Y()    << ","   // py = y-component of track momentum
						<< (mcTrack->GetMomentum()).Z()    << ","   // pz = z-component of track momentum
						<< q                               << ","   // q = charge of mu-/mu+
						<< (1.0)                           << ","   // nhits =  number of hits in a track
						<< mcTrack->GetPdgCode()           << ","   // pdgcode = PDG ID e.g. mu- has pdgcode=-13
						<< mcTrack->GetStartTime()                  // start_time = starting time of particle track
						<< std::endl;
					              
			delete (mcTrack);
		}
		
		*/
		
		
		// Add Line Breaks (Already Done !!!)
		// TODO: Delete Me.
		
		//fHits << std::endl;
		//fTruths << std::endl;
		//fTubes << std::endl;
		//fParticles << std::endl;
		
	}//end-for-fSttHitArray
	
	
	// --------------------------------------------------------------------
	//                            fParticles
	// --------------------------------------------------------------------
	
	for (Int_t mc=0; mc < fMCTrackArray->GetEntries(); mc++) {
	
		PndMCTrack *mcTrack = (PndMCTrack*)fMCTrackArray->At(mc);
		if (mcTrack->IsGeneratorCreated()) {
		
		    // Print MCTrack
		    // mcTrack->Print(mc);
		    // std::cout << "mcTrack->GetNPoints(kSTT): " <<  mcTrack->GetNPoints(kSTT) << std::endl;
		    
			int q =0;
			if (mcTrack->GetPdgCode()==13)           //mu+
			    q = 1;
			    
			else if (mcTrack->GetPdgCode()==-13)     //mu-
			    q = -1;
		
			// CSV:: Writting Info to CSV File. 		   
			fParticles 	<< (mc + 1) << ","                          // track_id > 0
			            << (mcTrack->GetStartVertex()).X() << ","   // vx = start x
						<< (mcTrack->GetStartVertex()).Y() << ","   // vy = start y
						<< (mcTrack->GetStartVertex()).Z() << ","   // vz = start z
						<< (mcTrack->GetMomentum()).X()    << ","   // px = x-component of track momentum
						<< (mcTrack->GetMomentum()).Y()    << ","   // py = y-component of track momentum
						<< (mcTrack->GetMomentum()).Z()    << ","   // pz = z-component of track momentum
						<< q                               << ","   // q = charge of mu-/mu+
						<< (1.0)                           << ","   // nhits =  number of hits in a track
						<< mcTrack->GetPdgCode()           << ","   // pdgcode = PDG ID e.g. mu- has pdgcode=-13
						<< mcTrack->GetStartTime()                  // start_time = starting time of particle track
						<< std::endl;
						
		}//endif-IsGeneratorCreated()
	}//end-for-fMCTrackArray

	
	// Close Files (Need to close the files every event)
    fHits.close();
	fTruths.close();
	fParticles.close();
	fTubes.close();

    fEventId++;
    
    //std::cout << "-I- GenerateData:: Successful Event: " << (fEventId - 1) << " with Prefix: " << "event"+fidx << std::endl;
    
}//end-GenerateData_V2()


// GenerateData()
void PndMLTracking::GenerateData() {
		
	// Event Files
	std::stringstream ss;
	ss << std::setw(10) << std::setfill('0') << fEventId;
	std::string fidx = ss.str();
	//std::cout << "Processing Event: " << "event"+fidx << std::endl;

	// Open Files (CSV)
	fHits.open(fCsvFilesPath+"event"+fidx+"-hits.csv");
	fTruths.open(fCsvFilesPath+"event"+fidx+"-truth.csv");
	fParticles.open(fCsvFilesPath+"event"+fidx+"-particles.csv");
	fTubes.open(fCsvFilesPath+"event"+fidx+"-tubes.csv");
	
	// ////////////////////////////////////////////////////////////////////
    //                            Add Header                             //
    // ////////////////////////////////////////////////////////////////////
	// Hit Positions
	fHits << "hit_id" <<","
		  << "x" << "," 
		  << "y" << "," 
		  << "z" << ","
		  << "tube_id" << "," 
		  << "skewed" << "," 
		  << "layer_id" << "," 
		  << "sector_id" << "," 
		  << "volume_id" << "," 
		  << "isochrone" << "," 
		  << "depcharge" << "," 
		  << "energyloss" << "," 
		  << "particle_id" 
		  <<std::endl;
    
    // MC Truth	   
    fTruths << "hit_id" <<","
    	    << "tx" << ","
    	    << "ty" << ","
    	    << "tz" << ","
    	    << "tpx"<< ","
    	    << "tpy"<< ","
    	    << "tpz"<< ","
    	    // << "weight" << ","
    	    << "particle_id"
    	    << std::endl;
    
    // Particle initial position/vertex(mm), momentum(GeV/c) and charge (q)
    fParticles << "particle_id" <<","
    		   << "vx" << ","
    		   << "vy" << ","
    		   << "vz" << ","
    		   << "px" << ","
    		   << "py" << ","
    		   << "pz" << ","
    		   << "q"  << ","
    		   // << "nhits" << ","
    		   << "pdg_code" << ","
    		   << "start_time"
    		   << std::endl;
    		   
	// Tube Parameters
	// Future Add: GetHalfLength(), GetWireDirection(), GetRadIn(), GetRadOut(),
	//             GetNeighborings(), GetDistance(), IsParallel()
	// Remove Tube Parameters  (redundent w.r.t fHits)
	fTubes << "hit_id"    << ","
		   << "tube_id"   << "," 
		   << "skewed"    << "," 
		   << "layer_id"  << "," 
		   << "sector_id" << "," 
		   << "volume_id" << "," 
		   << "isochrone" << "," 
		   << "depcharge" << "," 
		   << "energyloss"
		   <<std::endl;
	
	// ////////////////////////////////////////////////////////////////////
    //                            Event Loop
    // ////////////////////////////////////////////////////////////////////
    int hit_id = 0;
	for (int hit_idx = 0; hit_idx < fSttHitArray->GetEntriesFast(); hit_idx++) {
		
		// Get FairRootManager Instance
  		FairRootManager *ioman = FairRootManager::Instance();
  		
		// Get sttHitsLinks from STTHit
		FairMultiLinkedData_Interface *sttHitsLinks = (FairMultiLinkedData_Interface*)fSttHitArray->At(hit_idx);
		
		// Get sttPointLinks & Data (TCloneArray) from sttHitsLinks
		FairMultiLinkedData sttPointLinks = sttHitsLinks->GetLinksWithType(sttPointBranchID);
		FairMCPoint *sttpoint = (FairMCPoint*)ioman->GetCloneOfLinkData(sttPointLinks.GetLink(0));
			        
        // Terminate if sttpoint=NULL
        if (sttpoint == 0) {continue;}
        
        // Get mcTrackLinks & Data (TCloneArray) from sttpoint (OR sttPointLinks?)
        FairMultiLinkedData mcTrackLinks = sttpoint->GetLinksWithType(mcTrackBranchID); 
        PndMCTrack *mctrack = (PndMCTrack*)ioman->GetCloneOfLinkData(mcTrackLinks.GetLink(0));
        
        // Terminate if mcTrack=NULL
        if (mctrack == 0) {continue;}
        
        // Terminate if not Primary
        if (!mctrack->IsGeneratorCreated())
        	continue;
        
        // Hit Counter (very important in case number of tracks vary per event)
        hit_id++;
        
        // --------------------------------------------------------------------
        //                            MCPoints/fTruths
        // --------------------------------------------------------------------
    	fTruths << hit_id << ","            // hit_id=0 is for interaction vertex
				<< sttpoint->GetX()	<< ","
				<< sttpoint->GetY() << ","
		      	<< sttpoint->GetZ() << ","
		      	<< sttpoint->GetPx()<< ","
		      	<< sttpoint->GetPy()<< ","
		      	<< sttpoint->GetPz()<< ",";

		// --------------------------------------------------------------------
		//                            STTHists/fHits
		// --------------------------------------------------------------------
		// First Get access to SttHit & PndSttTube
		PndSttHit* stthit = (PndSttHit*)fSttHitArray->At(hit_idx);
		PndSttTube *tube = (PndSttTube*) fTubeArray->At(stthit->GetTubeID());
		fHits << hit_id 				<<","
			  << stthit->GetX() 		<< "," 
			  << stthit->GetY() 		<< "," 
			  << stthit->GetZ() 		<< ","
			  << stthit->GetTubeID()	<< "," 
			  << tube->IsSkew() 		<< "," 
			  << tube->GetLayerID() 	<< ","
			  << tube->GetSectorID()	<< "," 
			  << stthit->GetDetectorID()<< "," 
			  << stthit->GetIsochrone() << "," 
			  << stthit->GetDepCharge() << "," 
			  << stthit->GetEnergyLoss()<< ",";
		
		// --------------------------------------------------------------------
		//                            PndTube/fTubes
		// --------------------------------------------------------------------
		// Future Add: GetHalfLength(), GetWireDirection(), GetRadIn(), GetRadOut(),
		//             GetNeighborings(), GetDistance(), IsParallel()
		// Remove Tube Parameters  (redundent w.r.t fHits)
		fTubes<< hit_id 				<< ","
			  << stthit->GetTubeID()	<< ","
			  << tube->IsSkew() 		<< ","
			  << tube->GetLayerID() 	<< ","
			  << tube->GetSectorID()	<< ","
			  << stthit->GetDetectorID()<< "," 
			  << stthit->GetIsochrone() << ","
			  << stthit->GetDepCharge() << "," 
			  << stthit->GetEnergyLoss();
		
		// Particle/Track initial position/vertex(mm), momentum(GeV/c) and charge (q)
		
		std::vector<FairLink> mcTrackLinks_sorted = sttHitsLinks->GetSortedMCTracks();
		for (unsigned int trackIndex = 0; trackIndex < mcTrackLinks_sorted.size(); trackIndex++) {
		
			// 
			PndMCTrack *myTrack = (PndMCTrack*)ioman->GetCloneOfLinkData(mcTrackLinks_sorted[trackIndex]);
			
			// Get Track Index of MCTrack >> fHits			
			fHits 	<< std::to_string(mcTrackLinks_sorted[trackIndex].GetIndex() + 1);      // particle_id into hits.csv
			fTruths << std::to_string(mcTrackLinks_sorted[trackIndex].GetIndex() + 1);    	// particle_id into truths.csv
					
			// ----------------------------------------------------------------
			//                            fParticles
			// ----------------------------------------------------------------   		   
			fParticles 	<< std::to_string(mcTrackLinks_sorted[trackIndex].GetIndex() + 1) << ","
						<< (myTrack->GetStartVertex()).X() << ","
						<< (myTrack->GetStartVertex()).Y() << ","
						<< (myTrack->GetStartVertex()).Z() << ","
						<< (myTrack->GetMomentum()).X() << ","
						<< (myTrack->GetMomentum()).Y() << ","
						<< (myTrack->GetMomentum()).Z() << ","
						<< (-1) << ","                                   // muon
						<< myTrack->GetPdgCode() << ","
						<< myTrack->GetStartTime();
					              
			delete (myTrack);
		}
		
		// Add Line Breaks
		fHits << std::endl;
		fTruths << std::endl;
		fParticles << std::endl;
		fTubes << std::endl;
		
	}//end-for
	
	// Close Files
    fHits.close();
	fTruths.close();
	fParticles.close();
	fTubes.close();

    fEventId++;
}

// GetFairMCPoint()
FairMCPoint* PndMLTracking::GetFairMCPoint(TString fBranchName, FairMultiLinkedData_Interface* links, FairMultiLinkedData& array)
{
	// get the mc point(s) from each reco hit
	FairMultiLinkedData mcpoints = links->GetLinksWithType(FairRootManager::Instance()->GetBranchId(fBranchName));
	array = mcpoints;
	if (array.GetNLinks() == 0){
		return 0;
	}
	return (FairMCPoint*) FairRootManager::Instance()->GetCloneOfLinkData(array.GetLink(0));
}

// GenerateCSV()
void PndMLTracking::GenerateCSV() {
	
	// Call inside Exec (Waleed)
	InFile1.open (fCsvFilesPath+"box_detector_"+std::to_string(fEventId)+".csv");
	InFile2.open (fCsvFilesPath+"box_real_"+std::to_string(fEventId)+".csv");
	InFile3.open (fCsvFilesPath+"box_mctruth_"+std::to_string(fEventId)+".csv");

    InFile1	<< "hit_id" <<","<< "x" <<","<< "y" <<","<< "z"<<","
    		<< "tube_id" <<","<< "skewed"<<","<< "layer_id"<<","<< "sector_id"<<","
    		<< "isochrone" <<","<< "depcharge"<<","<< "particle_id"<<std::endl;
    	   
    InFile2 << "hit_id" <<","<< "tx" <<","<< "ty" <<","<< "tz" <<","<< "px" <<","<< "py"<<","<<"pz"<<std::endl;
    InFile3 << "hit_id" <<","<< "pdgcode" <<","<< "vertexX" <<","<< "vertexY" <<","<< "vertexZ" <<","<< "vertexT"<<std::endl;

	for (int i = 0; i < fSttHitArray->GetEntriesFast(); i++)
	{
		fHitId++;
		
		// Get access to MCPoint >> STTPoint using FairMultiLinkedData_Interface
		FairMultiLinkedData_Interface* myData = (FairMultiLinkedData_Interface*)fSttHitArray->At(i);
		FairMultiLinkedData array;
		FairMCPoint *sttpoint = GetFairMCPoint("STTPoint", myData, array);
        if (sttpoint == 0) {continue;}
        
		InFile2 << fHitId  <<","
				<< sttpoint->GetX() <<","
				<< sttpoint->GetY() <<","
		      	<< sttpoint->GetZ() <<","
		      	<< sttpoint->GetPx() <<","
		      	<< sttpoint->GetPy() <<","
		      	<< sttpoint->GetPz();
		
		// Get access to SttHit & PndSttTube
		PndSttHit* stthit = (PndSttHit*)fSttHitArray->At(i);
		PndSttTube *tube = (PndSttTube*) fTubeArray->At(stthit->GetTubeID());
		
		InFile1 << fHitId <<","
				<< stthit->GetX() <<","
				<< stthit->GetY() <<","
				<< stthit->GetZ() <<","
				<< stthit->GetTubeID() <<","
				<< tube->IsSkew() << ","
				<< tube->GetLayerID() << ","
				<< tube->GetSectorID() << ","
				<< stthit->GetIsochrone() <<","
				<< stthit->GetDepCharge() <<",";
		
		std::vector<FairLink> myLinks = myData->GetSortedMCTracks();
		for (unsigned int trackIndex = 0; trackIndex < myLinks.size(); trackIndex++)
		{
			PndMCTrack* myTrack = (PndMCTrack*)FairRootManager::Instance()->GetCloneOfLinkData(myLinks[trackIndex]);
			
			// Get Track Index of MCTrack >> InFile1				
			InFile1 << std::to_string(fEventId)+std::to_string(myLinks[trackIndex].GetIndex());
			
			InFile3 << fHitId <<","
					<<myTrack->GetPdgCode()<<","
					<<(myTrack->GetMomentum()).X()<<"," //(myTrack->GetStartVertex()).X()
					<<(myTrack->GetMomentum()).Y()<<"," //(myTrack->GetStartVertex()).Y()
					<<(myTrack->GetMomentum()).Z()<<"," //(myTrack->GetStartVertex()).Z()
					<<myTrack->GetStartTime();
						                 
			delete (myTrack);
		}
		
		InFile1<<std::endl;
		InFile2<<std::endl;
		InFile3<<std::endl;
		
	}//end-for
	
    InFile1.close();
    InFile2.close();
    InFile3.close();
    fEventId++;
}//end-GenerateCSV()
