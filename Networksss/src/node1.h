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

#ifndef __NETWORKSSS_NODE1_H_
#define __NETWORKSSS_NODE1_H_

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
class Node1 : public cSimpleModule
{

private:
  NodeTypes nodeType;                          // to store the type of node (sender or receiver)
  MyMessage_Base **buffer;                     // to store messages to be sent
  vector<string> alldata;                      // to store all messages from file
  vector<bitset<4>> message_error_codes;       // to store error codes for each message
  int next_frame_to_send;                      // next frame to send
  int frame_expected;                          // the frame expected to arrive
  int nbuffered;                               // number of buffered messages
  int MAX_SEQ;                                 // Maximum Sequence Number and Window Size
  int time_out;                                // time out for the message
  int ack_expected;                            // ack expected to arrive (The Lower Bound of the Window)
  int current_frame;                           // current frame taken from alldata
  double processing_time;                      // processing time for the message
  double transmission_time;                    // transmission time for the message
  double duplication_time;                     // duplication time for the message
  double error_time;                           // error time for the message
  double loss_probability;                     // loss probability for the ack/nack message
  bitset<4> message_error_code;                // to store error code for each message
  string data;                                 // to read each message from file
  bool is_processing;                          // to check if the message is being processed
  MyMessage_Base *processed_ack_or_nack;       // ack or nack that was processed
  vector<MyMessage_Base *> waited_messages;    // to store message waiting to be processed
  MyMessage_Base *last_correct_frame_received; // to store the last correct frame received that arrives when frame nr is the increment of frame_expected and arrives while and when the receiver is processing a ack/nack

public:
  bool coordinator_message_checker(cMessage *msg);
  MyMessage_Base *process_and_check_ack(int frame_expected, bool error);
  bool ErrorDetection(MyMessage_Base *msg);
  int inc(int seq_nr);
  bitset<8> trailer_byte(string data);
  string Deframing(const string &data);

protected:
  virtual void initialize() override;
  virtual void handleMessage(cMessage *msg) override;
};

#endif
