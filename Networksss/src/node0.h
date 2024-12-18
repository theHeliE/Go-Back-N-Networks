//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#ifndef __NETWORKSSS_NODE0_H_
#define __NETWORKSSS_NODE0_H_

#include <omnetpp.h>
#include "MyMessage_m.h"
#include <bitset>
using namespace omnetpp;
using namespace std;

enum MyMessageTypes
{
  DATA = 0,
  ACK = 1,
  NACK = 2
};

enum NodeTypes
{
  SENDER = 0,
  RECEIVER = 1,
  NEITHER = 2
};

/**
 * TODO - Generated class
 */
class Node0 : public cSimpleModule
{

private:
  NodeTypes nodeType = NEITHER;          // to store the type of node (sender or receiver)
  MyMessage_Base **buffer;               // to store messages to be sent
  vector<string> alldata;                // to store all messages from file
  vector<bitset<4>> message_error_codes; // to store error codes for each message
  int next_frame_to_send;                // next frame to send
  int frame_expected;                    // the frame expected to arrive
  int ack_expected;                      // ack expected to arrive (The Lower Bound of the Window)
  int nbuffered;                         // number of buffered messages
  int MAX_SEQ;                           // Maximum Sequence Number and Window Size
  int time_out;                          // time out for the message
  int current_frame = 0;                 // current frame taken from alldata
  double processing_time;                // processing time for the message
  double transmission_time;              // transmission time for the message
  double duplication_time;               // duplication time for the message
  double error_time;                     // error time for the message
  double loss_probability;               // loss probability for the ack/nack message
  bitset<4> message_error_code;          // to store error code for each message
  string data;                           // to read each message from file

public:
  string Framing(const string &data);                                                                                                                        // function to frame the data
  string Deframing(const string &data);                                                                                                                      // function to deframe the data
  bool ErrorDetection(MyMessage_Base *msg);                                                                                                                  // function to detect errors in the message
  void ReadFile();                                                                                                                                           // function to read the input file
  bool coordinator_message_checker(cMessage *msg);                                                                                                           // to check if the message is from the coordinator or not
  bool inBetween(int seq_nra, int seq_nrb, int seq_nrc);                                                                                                     // to check if the frame is in between the sender and receiver window
  void processing_frame(int frame_nr, int next_frame_to_send, int frame_expected, double processing_time, MyMessage_Base **&buffer, vector<string> &alldata); // to send the frame
  void send_ack(int frame_nr, int frame_expected, bool error);                                                                                               // to send the ack
  int inc(int seq_nr);                                                                                                                                       // to circularly increment the sequence number
  bitset<8> trailer_byte(string data);
  void message_construction(int frame_nr, int next_frame_to_send, int frame_expected, MyMessage_Base **&buffer, vector<string> &alldata);

protected:
  virtual void initialize() override;
  virtual void handleMessage(cMessage *msg) override;
};

#endif
