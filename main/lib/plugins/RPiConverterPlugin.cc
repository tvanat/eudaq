#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Configuration.hh"

#include "iostream"
#include "bitset"

namespace eudaq {

  static const char *EVENT_TYPE = "RPIDHT22";

  class RPiConverterPlugin : public DataConverterPlugin {
  public:
    virtual void Initialize(const Event &bore, const Configuration &cnf) { }

    virtual bool GetStandardSubEvent(StandardEvent &out,
                                     const Event &in) const { }

  private:
    RPiConverterPlugin()
      : DataConverterPlugin(EVENT_TYPE) {}

    static RPiConverterPlugin m_instance;
  };
  RPiConverterPlugin RPiConverterPlugin::m_instance;
}
