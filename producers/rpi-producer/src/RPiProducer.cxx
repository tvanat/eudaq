#include "eudaq/Producer.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Configuration.hh"

#include "RPiProducer.hh"
#include <wiringPi.h>

#include <iostream>
#include <ostream>
#include <vector>
#include <sched.h>
#include <unistd.h>

using namespace std;

#define MAXTIMINGS 85

void interrupthandler() {
  // Acquire lock for trigger stack:
  std::lock_guard<std::mutex> lck(m_mutex);
  m_trigger_stack++;
}


void RPiProducer::readDHT22() {

  uint8_t laststate = 1;
  uint8_t counter = 0;
  uint8_t j=0;

  int dht22_dat[5] = {0,0,0,0,0};

  pinMode(m_dht22_pin, OUTPUT);
  digitalWrite(m_dht22_pin, 1);
  eudaq::mSleep(10);
  digitalWrite(m_dht22_pin, 0);
  eudaq::mSleep(5);

  digitalWrite(m_dht22_pin, 1);
  usleep(20);

  pinMode(m_dht22_pin, INPUT);

  for(int i=0; i<MAXTIMINGS; i++){

    counter=0;

    while(digitalRead(m_dht22_pin) == laststate){
      counter++;
      usleep(1);
      if (counter == 255) break;
    }

    laststate = digitalRead(m_dht22_pin);


    if(counter == 255) break;

    if((i >= 4) && (i%2 == 0)){
      dht22_dat[j/8] <<= 1;
      if(counter > m_dht22_threshold){
        dht22_dat[j/8] |= 1;
      }
      j++;
    }
  }

  if((j >= 40) && (dht22_dat[4] == ((dht22_dat[0]+dht22_dat[1]+dht22_dat[2]+dht22_dat[3]) & 0xFF))) {

    // New sample is fine:
    dhtEvent.clear();
  
    float t, h;
    h = (float)dht22_dat[0]*256 + (float)dht22_dat[1];
    h /= 10.;
    t = (float)(dht22_dat[2] & 0x7F)*256 + (float)dht22_dat[3];
    t /= 10.;

    if((dht22_dat[2] & 0x80) != 0) t*= -1;

    cout << "Temperature: " << t << "*C" << endl;
    cout << "Feuchtigkeit: " << h << "%" << endl;
    dhtEvent.push_back(t);
    dhtEvent.push_back(h);
  }
}

RPiProducer::RPiProducer(const std::string &name,
			 const std::string &runcontrol)
  : eudaq::Producer(name, runcontrol), m_run(0), m_ev(0), m_terminated(false), m_name(name), m_trigger_pin(13), m_dht22_pin(2), m_dht22_threshold(25), m_sampling_freq(10), dhtEvent() {
  if(wiringPiSetupGpio() == -1) {
    std::cout << "WiringPi could not be set up" << std::endl;
    throw eudaq::LoggedException("WiringPi could not be set up");
  }
}

void RPiProducer::OnConfigure(const eudaq::Configuration &config) {

  std::cout << "Configuring: " << config.Name() << std::endl;

  // Read and store configured pin from eudaq config:
  m_trigger_pin = config.Get("trigger_pin", 13);
  std::cout << m_name << ": Configured pin " << std::to_string(m_trigger_pin) << " as trigger input." << std::endl;
  EUDAQ_INFO(string("Configured pin " + std::to_string(m_trigger_pin) + " as trigger input."));

  m_dht22_pin = config.Get("dht22_pin", 2);
  std::cout << m_name << ": Configured pin " << std::to_string(m_dht22_pin) << " as DHT22 input." << std::endl;
  EUDAQ_INFO(string("Configured pin " + std::to_string(m_dht22_pin) + " as DHT22 input."));

  // Store temperature sampling frequency in Hz:
  m_sampling_freq = config.Get("sampling_freq", 10);
  std::cout << m_name << ": Sampling temperature at " << std::to_string(m_sampling_freq) << " Hz." << std::endl;
  EUDAQ_INFO(string("Sampling temperature at " + std::to_string(m_sampling_freq) + " Hz."));

  m_dht22_threshold = config.Get("dht22_threshold", 25);
  std::cout << m_name << ": DHT22 threshold level " << std::to_string(m_dht22_threshold) << std::endl;
  EUDAQ_INFO(string("DHT22 threshold level " + std::to_string(m_dht22_threshold)));
  
  // Set pin mode to input:
  pinMode(m_trigger_pin, INPUT);

  // Initialize DHT22 measurement:
  while (dhtEvent.empty()) { readDHT22(); }
  EUDAQ_INFO(string("Validated DHT22 sensor reading."));

  try {
    SetStatus(eudaq::Status::LVL_OK, "Configured (" + config.Name() + ")");
  } catch (...) {
    EUDAQ_ERROR(string("Unknown exception."));
    SetStatus(eudaq::Status::LVL_ERROR, "Unknown exception.");
  }
}

void RPiProducer::OnStartRun(unsigned runnumber) {
  m_run = runnumber;
  m_ev = 0;

  try {

    std::cout << "Start Run: " << m_run << std::endl;

    eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE("RPIDHT22", m_run));
    // Set random information:
    bore.SetTag("TYPE", "Schnitzelboretchen 4.0");
    // Send the event out:
    SendEvent(bore);

    // Enable interrupt:
    int code = wiringPiISR(m_trigger_pin,INT_EDGE_RISING,&interrupthandler);
    EUDAQ_INFO(string("Enabled trigger interrupt for pin " + std::to_string(m_trigger_pin)));  

    SetStatus(eudaq::Status::LVL_OK, "Running");
  } catch (...) {
    EUDAQ_ERROR(string("Unknown exception."));
    SetStatus(eudaq::Status::LVL_ERROR, "Unknown exception.");
  }
}

// This gets called whenever a run is stopped
void RPiProducer::OnStopRun() {

  try {
    // FIXME disable interrupt!
    pinMode(m_trigger_pin, OUTPUT);
    EUDAQ_INFO(string("Disable trigger interrupt on pin " + std::to_string(m_trigger_pin)));
    
    SetStatus(eudaq::Status::LVL_OK, "Stopped");
  } catch (const std::exception &e) {
    printf("While Stopping: Caught exception: %s\n", e.what());
    SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
  } catch (...) {
    printf("While Stopping: Unknown exception\n");
    SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
  }
}

void RPiProducer::OnTerminate() {

  std::cout << "RPiProducer terminating..." << std::endl;

  m_terminated = true;
  std::cout << "RPiProducer " << m_name << " terminated." << std::endl;
}

void RPiProducer::ReadoutLoop() {

  // Loop until Run Control tells us to terminate
  while (!m_terminated) {
    eudaq::mSleep(1000/m_sampling_freq);

    // Sample new temperature/humidity:
    readDHT22();

    // Send out events:
    while(m_trigger_stack > 0) {
      // Prepare event before locking:
      eudaq::RawDataEvent ev("RPIDHT22", m_run, ++m_ev);
      
      // Ship event with the latest sample:
      ev.AddBlock(0, reinterpret_cast<const char *>(&dhtEvent[0]),
		  sizeof(dhtEvent[0]) * dhtEvent.size());
      SendEvent(ev);

      // Lock stack and decrement:
      std::lock_guard<std::mutex> lck(m_mutex);
      m_trigger_stack--;
    }
  }
}

// The main function that will create a Producer instance and run it
int main(int /*argc*/, const char **argv) {
  // You can use the OptionParser to get command-line arguments
  // then they will automatically be described in the help (-h) option
  eudaq::OptionParser op("Raspberry Pi Producer", "0.0", "Run options");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
                                   "tcp://localhost:44000", "address",
                                   "The address of the RunControl.");
  eudaq::Option<std::string> level(
      op, "l", "log-level", "NONE", "level",
      "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name(op, "n", "name", "RPiProducer", "string",
                                  "The name of this Producer");
  try {
    // This will look through the command-line arguments and set the options
    op.Parse(argv);
    // Set the Log level for displaying messages based on command-line
    EUDAQ_LOG_LEVEL(level.Value());

    // Create the instance
    RPiProducer producer(name.Value(), rctrl.Value());
    // And set it running...
    producer.ReadoutLoop();

    // When the keep-alive loop terminates, it is time to go
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    // This does some basic error handling of common exceptions
    return op.HandleMainException();
  }
  return 0;
}
