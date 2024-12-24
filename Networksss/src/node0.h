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
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem> // C++17 or later
#include <algorithm>
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

enum LogEvent
{
  PROCESSING = 0,
  SENT_FRAME = 1,
  SENT_ACK = 2,
  SENT_NACK = 3,
  RCVD_NACK = 4,
  TIMEOUT = 5
};

// The message stored in the buffer
struct message_collection
{
  MyMessage_Base *msg;
  MyMessage_Base *timer;
  bitset<4> error_code;
};

/**
 * TODO - Generated class
 */
class Node0 : public cSimpleModule
{

private:
  NodeTypes nodeType = NEITHER;                // to store the type of node (sender or receiver)
  message_collection **buffer;                 // to store messages to be sent
  vector<string> alldata;                      // to store all messages from file
  vector<bitset<4>> message_error_codes;       // to store error codes for each message
  int next_frame_to_send;                      // next frame to send
  int frame_expected;                          // the frame expected to arrive
  int ack_expected;                            // ack expected to arrive (The Lower Bound of the Window)
  int nbuffered;                               // number of buffered messages
  int MAX_SEQ;                                 // Maximum Sequence Number and Window Size
  int time_out;                                // time out for the message
  int current_frame = 0;                       // current frame taken from alldata
  double processing_time;                      // processing time for the message
  double transmission_time;                    // transmission time for the message
  double duplication_time;                     // duplication time for the message
  double error_time;                           // error time for the message
  double loss_probability;                     // loss probability for the ack/nack message
  bitset<4> message_error_code;                // to store error code for each message
  string data;                                 // to read each message from file
  ofstream output_file;                        // to write the output to the file
  int old_next_frame_to_send;                  // to store the old next frame to send
  int timeout_buffer_count;                    // to store the number of messages sent when timeout occurs
  bool is_processing;                          // to check if the message is being processed
  MyMessage_Base *processed_ack_or_nack;       // ack or nack that was processed
  vector<MyMessage_Base *> waited_messages;    // to store message waiting to be processed
  MyMessage_Base *last_correct_frame_received; // to store the last correct frame received that arrives when frame nr is the increment of frame_expected and arrives while and when the receiver is processing a ack/nack

public:
  string Framing(const string &data);                                                                                                                                                                    // function to frame the data
  string Deframing(const string &data);                                                                                                                                                                  // function to deframe the data
  bool ErrorDetection(MyMessage_Base *msg);                                                                                                                                                              // function to detect errors in the message
  void ReadFile();                                                                                                                                                                                       // function to read the input file
  bool coordinator_message_checker(cMessage *msg);                                                                                                                                                       // to check if the message is from the coordinator or not
  bool inBetween(int seq_nra, int seq_nrb, int seq_nrc);                                                                                                                                                 // to check if the frame is in between the sender and receiver window
  void processing_frame(int frame_nr, int next_frame_to_send, int frame_expected, double processing_time, message_collection **&buffer, vector<string> &alldata, vector<bitset<4>> &message_error_code); // to send the frame
  MyMessage_Base *process_and_check_ack(int &frame_expected, bool error);
  int inc(int seq_nr);                                                                                                                                                               // to circularly increment the sequence number
  bitset<8> trailer_byte(string data);                                                                                                                                               // to calculate the trailer byte(parity) of a framed message
  void message_construction(int frame_nr, int next_frame_to_send, int frame_expected, message_collection **&buffer, vector<string> &alldata, vector<bitset<4>> &message_error_code); // to construct the message given the data
  void start_timer(int seq_nr, int time_out);                                                                                                                                        // to start the timer of a certain frame
  void stop_timer(int seq_nr);                                                                                                                                                       // to stop timer
  void send_message(message_collection *msg_to_be_sent);                                                                                                                             // send the message based on the error codes given
  void message_manipulation(MyMessage_Base *&msg);                                                                                                                                   // to manipulate the message based on the error codes given
  void logEvent(LogEvent event, float curr_time, bitset<4> &err_code, int frame_ack_num, string &frame_payload, bitset<8> &frame_trailer, bool nack_ack_lost);                       // to log the events in the output file
  void open_write_close(string filename, string data);                                                                                                                               // to write the data to the file

protected:
  virtual void initialize() override;
  virtual void handleMessage(cMessage *msg) override;
};

#endif
