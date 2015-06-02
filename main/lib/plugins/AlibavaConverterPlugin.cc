#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Configuration.hh"

#include "iostream"
#include "bitset"

namespace eudaq {

  static const char* EVENT_TYPE = "ALIBAVA";

  class AlibavaConverterPlugin : public DataConverterPlugin {
  public:
    // virtual unsigned GetTriggerID(const Event & ev) const{}; 

    virtual void Initialize(const Event & bore, const Configuration & cnf) {
      // Initialize if you need it.
    }

    virtual bool GetStandardSubEvent(StandardEvent & out, const Event & in) const {
      // Check if we have BORE or EORE:
      if (in.IsBORE() || in.IsEORE()) { return true; }

      // Do the event decoding here.
    }

#if USE_LCIO && USE_EUTELESCOPE
    virtual void GetLCIORunHeader(lcio::LCRunHeader & header, eudaq::Event const & bore, eudaq::Configuration const & conf) const {
      eutelescope::EUTelRunHeaderImpl runHeader(&header);
      // Type of data: real.
      runHeader.setDAQHWName(EUTELESCOPE::DAQDATA);
    }

    virtual bool GetLCIOSubEvent(lcio::LCEvent & result, const Event & source) const {
      if(source.IsBORE()) {
	std::cout << "AlibavaConverterPlugin::GetLCIOSubEvent BORE " << source << std::endl;
	return true;
      } else if(source.IsEORE()) {
	std::cout << "AlibavaConverterPlugin::GetLCIOSubEvent EORE " << source << std::endl;
	return true;
      }

      // Set event type Data Event (kDE):
      result.parameters().setValue(eutelescope::EUTELESCOPE::EVENTTYPE, eutelescope::kDE);

      // Prepare the data collection and check for its existence:
      LCCollectionVec * zsDataCollection;
      bool zsDataCollectionExists = false;
      try {
	/// FIXME choose another name for the collection!
	zsDataCollection = static_cast<LCCollectionVec*>(result.getCollection(m_event_type));
	zsDataCollectionExists = true;
      } catch(lcio::DataNotAvailableException& e) {
	zsDataCollection = new LCCollectionVec(lcio::LCIO::TRACKERDATA);
      }

      // Set the proper cell encoder
      CellIDEncoder<TrackerDataImpl> zsDataEncoder(eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING, zsDataCollection);

      // Decode the raw data and retrieve the eudaq StandardEvent:
      StandardEvent tmp_evt;
      GetStandardSubEvent(tmp_evt, source);

      // Loop over all planes available in the data stream:
      for (size_t iPlane = 0; iPlane < tmp_evt.NumPlanes(); ++iPlane) {
	StandardPlane plane = static_cast<StandardPlane>(tmp_evt.GetPlane(iPlane));

	zsDataEncoder["sensorID"] = plane.ID();
	zsDataEncoder["sparsePixelType"] = eutelescope::kEUTelGenericSparsePixel;

	// Get the total number of pixels
	size_t nPixel = plane.HitPixels();

	// Prepare a new TrackerData for the ZS data
	std::auto_ptr<lcio::TrackerDataImpl>zsFrame(new lcio::TrackerDataImpl);
	zsDataEncoder.setCellID(zsFrame.get());

	// This is the structure that will host the sparse pixels
	std::auto_ptr<eutelescope::EUTelTrackerDataInterfacerImpl<eutelescope::EUTelGenericSparsePixel> >
	  sparseFrame(new eutelescope::EUTelTrackerDataInterfacerImpl<eutelescope::EUTelGenericSparsePixel>(zsFrame.get()));

	// Prepare a sparse pixel to be added to the sparse data:
	std::auto_ptr<eutelescope::EUTelGenericSparsePixel> sparsePixel(new eutelescope::EUTelGenericSparsePixel);
	for(size_t iPixel = 0; iPixel < nPixel; ++iPixel) {

	  // Fill the sparse pixel coordinates with decoded data:
	  sparsePixel->setXCoord((size_t)plane.GetX(iPixel));
	  sparsePixel->setYCoord((size_t)plane.GetY(iPixel));
	  // Fill the pixel charge:
	  sparsePixel->setSignal((int32_t)plane.GetPixel(iPixel));
	  
	  // Add the pixel to the readout frame:
	  sparseFrame->addSparsePixel(sparsePixel.get());
	}

	// Now add the TrackerData to the collection
	zsDataCollection->push_back(zsFrame.release());
      } // loop over all planes

      // Add the collection to the event only if not empty and not yet there
      if(!zsDataCollectionExists){
	if(zsDataCollection->size() != 0) {
	  result.addCollection(zsDataCollection, m_event_type);
	} else {
	  delete zsDataCollection; // clean up if not storing the collection here
	}
      }

      return true;
    }
#endif

  private:
    AlibavaConverterPlugin() : DataConverterPlugin(EVENT_TYPE) {}
    static AlibavaConverterPlugin m_instance;
  };
  AlibavaConverterPlugin AlibavaConverterPlugin::m_instance;
}
