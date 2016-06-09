#ifndef RPIPRODUCER_HH
#define RPIPRODUCER_HH

// EUDAQ includes:
#include "eudaq/Producer.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Configuration.hh"

// system includes:
#include <iostream>
#include <ostream>
#include <vector>
#include <mutex>

// Stacking sampled triggers:
int m_trigger_stack;
// Add one mutex to protect calls to the trigger stack variable
std::mutex m_mutex;

void interrupthandler();

// Raspberry Pi Producer
class RPiProducer : public eudaq::Producer {

public:
  RPiProducer(const std::string &name, const std::string &runcontrol);
  virtual void OnConfigure(const eudaq::Configuration &config);
  virtual void OnStartRun(unsigned runnumber);
  virtual void OnStopRun();
  virtual void OnTerminate();
  void ReadoutLoop();
  
private:
  void readDHT22();

  unsigned m_run, m_ev;
  bool m_terminated;

  int m_trigger_pin;
  int m_dht22_pin;
  int m_dht22_threshold;
  
  unsigned m_sampling_freq;
  std::vector<double> dhtEvent;
  std::string m_name;
};

#endif /*RPIPRODUCER_HH*/
