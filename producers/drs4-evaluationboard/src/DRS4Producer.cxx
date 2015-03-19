#ifndef DRS4PRODUCER_HH
#define DRS4PRODUCER_HH

// EUDAQ includes:
#include "eudaq/Producer.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Configuration.hh"

// pxarCore includes:
#include "api.h"

// system includes:
#include <iostream>
#include <ostream>
#include <vector>
#include <mutex>
DRS4PProducer::DRS4PProducer(const std::string & name, const std::string & runcontrol, const std::string & verbosity)  : eudaq::Producer(name, runcontrol),
m_run(0),
m_ev(0),
m_ev_filled(0),
m_ev_runningavg_filled(0),
m_terminated(false),
m_running(false),
triggering(false),
m_producerName(name){

}

void DRS4PProducer::OnConfigure(const eudaq::Configuration& conf) {

	m_config = conf;
	/* do initial scan */
	m_drs = new DRS();

	/* show any found board(s) */
	for (i=0 ; i<m_drs->GetNumberOfBoards() ; i++) {
		m_b = m_drs->GetBoard(i);
		printf("Found DRS4 evaluation board, serial #%d, firmware revision %d\n",
				m_b->GetBoardSerialNumber(), m_b->GetFirmwareVersion());
	}

	/* exit if no board found */
	int nBoards = m_drs->GetNumberOfBoards();

	/* continue working with first board only */
	m_b = m_drs->GetBoard(0);


	/* initialize board */
	m_b->Init();

	/* set sampling frequency */
	m_b->SetFrequency(5, true);

	/* enable transparent mode needed for analog trigger */
	m_b->SetTranspMode(1);

	/* set input range to -0.5V ... +0.5V */
	m_b->SetInputRange(0);

	/* use following line to set range to 0..1V */
	//b->SetInputRange(0.5);

	/* use following line to turn on the internal 100 MHz clock connected to all channels  */
	m_b->EnableTcal(1);

	/* use following lines to enable hardware trigger on CH1 at 50 mV positive edge */
	if (m_b->GetBoardType() >= 8) {        // Evaluation Board V4&5
		m_b->EnableTrigger(1, 0);           // enable hardware trigger
		m_b->SetTriggerSource(1<<0);        // set CH1 as source
	} else if (b->GetBoardType() == 7) { // Evaluation Board V3
		m_b->EnableTrigger(0, 1);           // lemo off, analog trigger on
		m_b->SetTriggerSource(0);           // use CH1 as source
	}
	m_b->SetTriggerLevel(0.05);            // 0.05 V
	m_b->SetTriggerPolarity(false);        // positive edge
}


int main(int /*argc*/, const char ** argv) {
  // You can use the OptionParser to get command-line arguments
  // then they will automatically be described in the help (-h) option
  eudaq::OptionParser op("CMS Pixel Producer", "0.0",
			 "Run options");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
				   "tcp://localhost:44000", "address", "The address of the RunControl.");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
				   "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "CMSPixel", "string",
				   "The name of this Producer");
  eudaq::Option<std::string> verbosity(op, "v", "verbosity mode", "INFO", "string");
  try {
    // This will look through the command-line arguments and set the options
    op.Parse(argv);
    // Set the Log level for displaying messages based on command-line
    EUDAQ_LOG_LEVEL(level.Value());
    // Create a producer
    DRS4PixelProducer producer(name.Value(), rctrl.Value(), verbosity.Value());
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

#endif /*DRS4PRODUCER_HH*/
