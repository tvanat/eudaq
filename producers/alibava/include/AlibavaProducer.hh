#ifndef ALIBAVAPRODUCER_HH
#define ALIBAVAPRODUCER_HH

// EUDAQ includes:
#include "eudaq/Producer.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Configuration.hh"

// system includes:
#include <iostream>
#include <ostream>
#include <vector>
#include <mutex>

// AlibavaProducer
class AlibavaProducer : public eudaq::Producer {

public:
  AlibavaProducer(const std::string & name, const std::string & runcontrol, const std::string & verbosity);
  virtual void OnConfigure(const eudaq::Configuration & config);
  virtual void OnStartRun(unsigned runnumber);
  virtual void OnStopRun();
  virtual void OnTerminate();
  void ReadoutLoop();

private:

  unsigned m_run, m_ev;
  std::string m_verbosity, m_producerName;
  bool m_terminated, m_running;
  eudaq::Configuration m_config;
};
#endif /*ALIBAVAPRODUCER_HH*/
