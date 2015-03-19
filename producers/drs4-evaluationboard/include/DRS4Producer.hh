#ifndef DRS4PRODUCER_HH
#define DRS4PIXELPRODUCER_HH

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "strlcpy.h"
#include "DRS.h"

// EUDAQ includes:
#include "eudaq/Producer.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Configuration.hh"

#include <math.h>
#include <signal.h>
#include <sys/types.h>
#include<sys/time.h>
#include <vector>
using namespace std;

#define NumOfCha 1280
#define NumOfSi 3
#define BuffSize NumOfCha*NumOfSi

#define EventLength 1280*3 + 20 + 256 + 10

class DRS4PProducer : public eudaq::Producer {
public:
  DRS4PProducer(const std::string & name, const std::string & runcontrol, const std::string & verbosity);
  virtual void OnConfigure(const eudaq::Configuration & config);
  virtual void OnStartRun(unsigned runnumber);
  virtual void OnStopRun();
  virtual void OnTerminate();
  void ReadoutLoop();
  virtual ~DRS4PProducer();
private:
  unsigned m_run, m_ev, m_ev_filled, m_ev_runningavg_filled;
  bool m_terminated, m_running, triggering;
  std::string m_verbosity, m_producerName;
  unsigned NumOfChan, NumOfSil, NumOfADC;
  eudaq::Configuration m_config;
  eudaq::Timer* m_t;
  DRS *m_drs;
  DRSBoard *m_b;
  float time_array[8][1024];
  float wave_array[8][1024];
};

#endif /*DRS4PRODUCER_HH*/
