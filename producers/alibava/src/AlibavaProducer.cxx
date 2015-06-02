#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Configuration.hh"
#include "config.h" // Version symbols

#include "AlibavaProducer.hh"

#include <iostream>
#include <ostream>
#include <vector>
#include <sched.h>

using namespace std; 

// event type name, needed for readout with eudaq. Links to /main/lib/src/CMSPixelConverterPlugin.cc:
static const std::string EVENT_TYPE = "ALIBAVA";

AlibavaProducer::AlibavaProducer(const std::string & name, const std::string & runcontrol, const std::string & verbosity)
  : eudaq::Producer(name, runcontrol),
    m_run(0),
    m_ev(0),
    m_terminated(false),
    m_running(false),
    m_producerName(name),
    m_config()
{}

void AlibavaProducer::OnConfigure(const eudaq::Configuration & config) {

  std::cout << "Configuring: " << config.Name() << std::endl;
  m_config = config;

  try {
    // Do some crazy Alibava initialization here

    SetStatus(eudaq::Status::LVL_OK, "Configured (" + config.Name() + ")");
  }
  catch (...) {
    EUDAQ_ERROR(string("Unknown exception."));
    SetStatus(eudaq::Status::LVL_ERROR, "Unknown exception.");
  }
}

void AlibavaProducer::OnStartRun(unsigned runnumber) {

  try {
    // Do some crazy StartRun here...

    std::cout << "Start Run: " << m_run << std::endl;

    eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run));
    // Store eudaq library version:
    bore.SetTag("EUDAQ", PACKAGE_VERSION);

    // Send the event out:
    SendEvent(bore);

    // Start the Data Acquisition

    // 
    SetStatus(eudaq::Status::LVL_OK, "Running.");
    m_running = true;
  }
  catch (...) {
    EUDAQ_ERROR(string("Unknown exception."));
    SetStatus(eudaq::Status::LVL_ERROR, "Unknown exception.");
  }
}

// This gets called whenever a run is stopped
void AlibavaProducer::OnStopRun() {
  // Break the readout loop
  m_running = false;
  std::cout << "Stopping Run" << std::endl;

  try {
    // Do some crazy Alibava StopRun action.

    // Sending the final end-of-run event:
    SendEvent(eudaq::RawDataEvent::EORE(EVENT_TYPE, m_run, m_ev));


    SetStatus(eudaq::Status::LVL_OK, "Stopped");
  } catch (const std::exception & e) {
    printf("While Stopping: Caught exception: %s\n", e.what());
    SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
  } catch (...) {
    printf("While Stopping: Unknown exception\n");
    SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
  }
}

void AlibavaProducer::OnTerminate() {

  std::cout << "AlibavaProducer terminating..." << std::endl;
  // Stop the readout loop, force routine to return:
  m_terminated = true;

  // Kill USB cocnnection to Alibava board.
  
  std::cout << "AlibavaProducer " << m_producerName << " terminated." << std::endl;
}

void AlibavaProducer::ReadoutLoop() {

  // Loop until Run Control tells us to terminate
  while (!m_terminated) {
    // No run is m_running, cycle and wait:
    if(!m_running) {
      // Move this thread to the end of the scheduler queue:
      sched_yield();
      continue;
    }
    else {
      // Do some crazy readout stuff for Alibava.


      // get event

      // new EUDAQ RAW event:
      eudaq::RawDataEvent ev(EVENT_TYPE, m_run, m_ev);

      // Add data to event:
      //ev.AddBlock(0, reinterpret_cast<const char*>(&event[0]), sizeof(event[0])*event.size());

      // Send event to data collector:
      SendEvent(ev);
      m_ev++;
    }
  }
}


// The main function that will create a Producer instance and run it
int main(int /*argc*/, const char ** argv) {

  // You can use the OptionParser to get command-line arguments
  // then they will automatically be described in the help (-h) option
  eudaq::OptionParser op("Alibava Producer", "0.0",
			 "Run options");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
				   "tcp://localhost:44000", "address", "The address of the RunControl.");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
				   "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "alibava", "string",
				   "The name of this Producer");
  eudaq::Option<std::string> verbosity(op, "v", "verbosity mode", "INFO", "string");
  try {
    // This will look through the command-line arguments and set the options
    op.Parse(argv);
    // Set the Log level for displaying messages based on command-line
    EUDAQ_LOG_LEVEL(level.Value());
    // Create a producer
    AlibavaProducer producer(name.Value(), rctrl.Value(), verbosity.Value());
    // And set it running...
    producer.ReadoutLoop();
    // When the readout loop terminates, it is time to go
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    // This does some basic error handling of common exceptions
    return op.HandleMainException();
  }
  return 0;
}

