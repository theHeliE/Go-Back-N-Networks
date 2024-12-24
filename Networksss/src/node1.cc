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

#include "node1.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem> // C++17 or later
#include <algorithm>

Define_Module(Node1);
using namespace std;

bitset<4> zeroes(0);
bitset<8> dummy(0);
string emptyString = "";

void Node1::open_write_close(string filename, string data)
{
    // Open the file
    ofstream output_file1(filename, ios_base::app);
    if (!output_file1.is_open())
    {
        cerr << "Error opening file: " << filename << endl;
        return;
    }

    // Write the data to the file
    output_file1 << data << endl;

    // Close the file
    output_file1.close();
}

void Node1::logEvent(LogEvent event, float curr_time, bitset<4> &err_code, int frame_ack_num, string &frame_payload, bitset<8> &frame_trailer, bool nack_ack_lost)
{
    switch (event)
    {

    case PROCESSING:
    {
        string message = "At: " + to_string(curr_time) + ", Node: 1, Introducing channel error with code " + err_code.to_string();
        open_write_close("Textfiles/output.txt", message);
        EV << message << endl;
        break;
    }
    case SENT_FRAME:
    {
        string message = "At: " + to_string(curr_time) + ", Node: 1 [sent] frame with " + to_string(frame_ack_num) +
                         " and payload = " + Framing(frame_payload) +
                         " and trailer = " + frame_trailer.to_string() +
                         " , Modified = " + ((err_code[3] == 0) ? "-1" : "1") +
                         " , Lost = " + ((err_code[2] == 1) ? "YES" : "NO") +
                         " , Duplicate = " + to_string(err_code[1]) +
                         " , Delay = " + ((err_code[0] == 0) ? "0" : to_string(error_time));
        open_write_close("Textfiles/output.txt", message);
        EV << message << endl;

        if (err_code[1] == 1)
        {
            message = "At: " + to_string(curr_time + duplication_time) + ", Node: 1 [sent] frame with " + to_string(frame_ack_num) +
                      " and payload = " + Framing(frame_payload) +
                      " and trailer = " + frame_trailer.to_string() +
                      " , Modified = " + ((err_code[3] == 0) ? "-1" : "1") +
                      " , Lost = " + ((err_code[2] == 1) ? "YES" : "NO") +
                      " , Duplicate = " + to_string(err_code[1] + 1) +
                      " , Delay = " + ((err_code[0] == 0) ? "0" : to_string(error_time));
            open_write_close("Textfiles/output.txt", message);
            EV << message << endl;
        }
        break;
    }
    case SENT_ACK:
    {
        string message = "At: " + to_string(curr_time) + ", Node: 1 Sending ACK with number: " +
                         to_string(frame_ack_num) + ", loss " + (nack_ack_lost ? "YES" : "NO");
        open_write_close("Textfiles/output.txt", message);
        EV << message << endl;
        break;
    }
    case SENT_NACK:
    {
        string message = "At: " + to_string(curr_time) + ", Node: 1 Sending NACK with number: " +
                         to_string(frame_ack_num) + ", loss " + (nack_ack_lost ? "YES" : "NO");
        open_write_close("Textfiles/output.txt", message);
        EV << message << endl;
        break;
    }
    case RCVD_NACK:
    {
        string message = "At: " + to_string(curr_time) + ", Node: 1 [Received NACK] for seq_number: " +
                         to_string(frame_ack_num);
        open_write_close("Textfiles/output.txt", message);
        EV << message << endl;
        break;
    }
    case TIMEOUT:
    {
        string message = "Timeout event at time: " + to_string(curr_time) + ", Node: 1 for frame with seq_number: " +
                         to_string(frame_ack_num);
        open_write_close("Textfiles/output.txt", message);
        EV << message << endl;
        break;
    }
    }
}

string Node1::Framing(const string &data)
{
    string framedData;

    // START
    framedData += '$';

    for (char c : data)
    {
        if (c == '$' || c == '/')
        {
            // add ESC
            framedData += '/';
        }
        // add Data
        framedData += c;
    }

    // END
    framedData += '$';

    return framedData;
}

string Node1::Deframing(const string &data)
{
    // define a string to store the deframed data as well as a
    // when the data is deframed, first and last character is removed (flag characters)
    // then when a '/' is found, the next character is added to the deframed data
    // this is because the character after the '/' is the original character
    string deframedData;

    for (int i = 1; i < data.length() - 1; i++)
    {
        // if the character is a '/', skip it and go to the next character
        if (data[i] == '/')
        {
            i++;
        }
        deframedData += data[i];
    }

    return deframedData;
}

// circulary increment the sequence number
int Node1::inc(int seq_nr)
{
    return (seq_nr + 1) % (MAX_SEQ + 1);
}

bool Node1 ::coordinator_message_checker(cMessage *msg)
{
    // if the message is from the coordinator and the message is sender then this node
    // is the sender else if the message is receiver then this node is the receiver
    // else it is not from the coordinator
    if (strcmp(msg->getName(), "sender") == 0)
    {
        nodeType = SENDER;
        ReadFile(); // Read the input file
        //EV << "Node1 set as SENDER with " << alldata.size() << " messages." << endl;

        return true;
    }

    else if (strcmp(msg->getName(), "receiver") == 0)
    {
        nodeType = RECEIVER;
        //EV << "receiver" << endl;
        return true;
    }

    else
    {
        return false;
    }
}

bool Node1::ErrorDetection(MyMessage_Base *msg)
{

    // Get the Payload and create the Variable that stores the XOR value
    string s = string(msg->getM_Payload());
    bitset<8> xorval(0);

    // XOR the payload
    for (int i = 0; i < s.size(); i++)
    {
        bitset<8> x(s[i]);
        xorval = xorval ^ x;
    }

    // XOR the trailer
    xorval ^= bitset<8>(msg->getM_Trailer());

    // Check if the XOR value is not 0 then error detected else no error detected

    return xorval != 0;
}

void Node1::ReadFile()
{
    // Open the file
    ifstream infile("Textfiles/input1.txt");
    if (!infile.is_open())
    {
        cerr << "Error opening file: Textfiles/input1.txt" << endl;
        return;
    }

    // Read the file line by line
    string line;
    while (getline(infile, line))
    {
        if (!line.empty() && !std::all_of(line.begin(), line.end(), ::isspace))
        {
            // Parse the line to extract time and node_id
            istringstream iss(line);

            // Extract the first 4 characters as a bitset
            message_error_code = bitset<4>(line.substr(0, 4));

            // Extract the remainder of the line as a message string
            data = line.substr(5);

            // Store the error code and message in the vectors
            message_error_codes.push_back(message_error_code);
            alldata.push_back(data);

            // EV << "Code : " << message_error_code << endl;
            // EV << "Message: " << data << endl;
        }
    }

    // Close the file
    infile.close();
}

MyMessage_Base *Node1::process_and_check_ack(int &frame_expected, bool error)
{
    MyMessage_Base *msg = new MyMessage_Base("");

    // Set the frame number
    msg->setSeq_Num(frame_expected);

    // Set the ack expected
    msg->setACK_Num(frame_expected);

    // Set the Nack expected
    msg->setNACK_Num(frame_expected);

    // Set the payload
    string dummy = "dummy";
    msg->setM_Payload(dummy.c_str());

    // Set the trailer
    msg->setM_Trailer(trailer_byte(dummy));

    // Set the message type to ACK or NACK depending on the error
    // if it no error send ACK else send NACK
    // also in ACK increment the frame expected

    if (error)
    {

        msg->setM_Type(NACK);
    }
    else
    {
        //EV << "Frame Expected_old: " << frame_expected << endl;
        frame_expected = inc(frame_expected);
       // EV << "Frame Expected_new: " << frame_expected << endl;
        msg->setM_Type(ACK);
    }

    // is_processing = true
    is_processing = true;

    // Schedule the message to be sent after the processing time
    scheduleAt(simTime() + processing_time, new MyMessage_Base("selfMsg"));

    // return msg
    return msg;
}

// check if the frame is in between the window

bool Node1::inBetween(int seq_nra, int seq_nrb, int seq_nrc)
{
    if ((seq_nra <= seq_nrb && seq_nrb < seq_nrc) || (seq_nra <= seq_nrb && seq_nra > seq_nrc) || (seq_nra > seq_nrc && seq_nrb < seq_nrc))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bitset<8> Node1::trailer_byte(string data)
{
    // Get the Payload and create the Variable that stores the XOR value
    bitset<8> xorval(0);

    // XOR the payload
    for (int i = 0; i < data.size(); i++)
    {
        bitset<8> x(data[i]);
        xorval = xorval ^ x;
    }

    return xorval;
}

void Node1::message_construction(int frame_nr, int next_frame_to_send, int frame_expected, message_collection **&buffer, vector<string> &alldata, vector<bitset<4>> &message_error_code)
{
    // Create a new message
    MyMessage_Base *nmsg = new MyMessage_Base("");

    // Set the frame number
    nmsg->setSeq_Num(next_frame_to_send);

    // Set the ack expected
    nmsg->setACK_Num((frame_expected + MAX_SEQ) % (MAX_SEQ + 1));

    // Set the Nack expected
    nmsg->setNACK_Num((frame_expected + MAX_SEQ) % (MAX_SEQ + 1));

    // Set the payload
    string framedPayload = Framing(alldata[frame_nr]);
    nmsg->setM_Payload(framedPayload.c_str());

    // Set the trailer
    nmsg->setM_Trailer(trailer_byte(framedPayload));

    // set the Type
    nmsg->setM_Type(DATA);
    // In order to not process 2 frames at the same time
    old_next_frame_to_send = next_frame_to_send;

    // add the msg to the buffer
    buffer[next_frame_to_send]->msg = nmsg;

    // set the error_code of the message
    buffer[next_frame_to_send]->error_code = message_error_code[frame_nr];

    // set the timer of the message to Null
    buffer[next_frame_to_send]->timer = nullptr;

    //EV << "Message Constructed" << endl;
}

void Node1::processing_frame(int frame_nr, int next_frame_to_send, int frame_expected, double processing_time, message_collection **&buffer, vector<string> &alldata, vector<bitset<4>> &message_error_code)
{
    this->logEvent(PROCESSING, (float)(simTime().dbl()), message_error_codes[frame_nr], 0, emptyString, dummy, false);
    // construct message and add it to buffer
    message_construction(frame_nr, next_frame_to_send, frame_expected, buffer, alldata, message_error_code);

    // Schedule the message
    scheduleAt(simTime() + processing_time, new MyMessage_Base("selfMsg"));

    //EV << "processing frame " << buffer[next_frame_to_send]->msg->getSeq_Num() << endl
      // << "Payload: " << buffer[next_frame_to_send]->msg->getM_Payload() << endl
       //<< " " << "Trailer: " << buffer[next_frame_to_send]->msg->getM_Trailer().to_ulong() << endl;
}

void Node1::start_timer(int seq_nr, int time_out)
{
    // create timer
    MyMessage_Base *timer = new MyMessage_Base("timer");

    // set the timer message in the buffer
    buffer[seq_nr]->timer = timer;

    // start the timer
    scheduleAt(simTime() + time_out, timer);
}

void Node1::stop_timer(int seq_nr)
{
    // stop the timer
    cancelEvent(buffer[seq_nr]->timer);

    // set the timer to null
    buffer[seq_nr]->timer = nullptr;
}

void Node1::message_manipulation(MyMessage_Base *&msg)
{
    const char *pl = msg->getM_Payload();
    string str = pl;
    // Access the second character and modify its second bit
    unsigned char firstChar = str[1];
    firstChar ^= (1 << 1);
    str[1] = firstChar;
    //EV << " Adding Error by changing the Fourth bit of second character in the payload (the first character after the start flag)" << endl;
    pl = str.c_str();
    msg->setM_Payload(pl);
}

void Node1::send_message(message_collection *msg_to_be_sent)
{
    // first define the total sending time and total sending time for the duplicate
    // also define the duplicated message if there is a duplication error
    // and the message that will be sent
    double total_time = transmission_time;
    double total_time_duplicate = 0;
    MyMessage_Base *duplicated_msg = nullptr;
    MyMessage_Base *msg = msg_to_be_sent->msg->dup();

    // if there is loss , ignore the messages
    if (msg_to_be_sent->error_code[2] == 1)
    {
        //EV << "This message will be lost with frame number : " << msg_to_be_sent->msg->getSeq_Num() << endl;
    }

    else
    {
        // classify errors based on error_code
        // check if there is modification (bit 3)
        if (msg_to_be_sent->error_code[3] == 1)
        {
           // EV << "This message will be modified at second character fourth bit" << endl;
            // modify the message

            message_manipulation(msg);
        }

        // check if there is delay error
        if (msg_to_be_sent->error_code[0] == 1)
        {
            // if there is delay error, add delay to the sending time
            //EV << "This message will be delayed by =" << error_time << endl;
            total_time += error_time;
        }

        // check if there is duplication delay
        if (msg_to_be_sent->error_code[1] == 1)
        {
            //EV << "This message will be duplicated with duplication time =" << duplication_time << endl;
            // if there is duplication delay, create the duplicated Message
            duplicated_msg = msg->dup();

            // update the total sending time
            total_time_duplicate = total_time + duplication_time;
        }

        // send the message
        sendDelayed(msg, total_time, "out");
       // EV << "Total Transmission time for this message :" << total_time << endl;

        // if there is duplicate send it
        if (duplicated_msg)
        {

            sendDelayed(duplicated_msg, total_time_duplicate, "out");
           // EV << "Total Transmission time for the duplicate message :" << total_time_duplicate << endl;
        }
    }
    string deframed_payload = Deframing(msg->getM_Payload());
    bitset<8> trailer = trailer_byte(msg_to_be_sent->msg->getM_Payload());

    this->logEvent(SENT_FRAME, (float)(simTime().dbl()), msg_to_be_sent->error_code, msg_to_be_sent->msg->getSeq_Num(), deframed_payload, trailer, false);
}

void Node1::initialize()
{

    // Intialization of Needed Parameters
    next_frame_to_send = 0;
    frame_expected = 0;
    nbuffered = 0;
    ack_expected = 0;
    timeout_buffer_count = 0;
    MAX_SEQ = getParentModule()->par("WS");
    time_out = getParentModule()->par("TO");
    processing_time = double(getParentModule()->par("PT"));
    transmission_time = double(getParentModule()->par("TD"));
    duplication_time = double(getParentModule()->par("DD"));
    error_time = double(getParentModule()->par("ED"));
    loss_probability = double(getParentModule()->par("LP"));
    is_processing = false;
    processed_ack_or_nack = nullptr;
    last_correct_frame_received = nullptr;
    // output_file1.open("Textfiles/output.txt", ios_base::app);

    buffer = new message_collection *[MAX_SEQ + 1];

    // Allocate memory for each element in the buffer
    for (int i = 0; i <= MAX_SEQ; ++i)
    {
        buffer[i] = new message_collection(); // Allocate memory
        buffer[i]->msg = nullptr;             // Initialize msg to nullptr
        buffer[i]->timer = nullptr;           // Initialize timer to nullptr
    }

    //EV << "Node initialized with parameters: "
      // << "MAX_SEQ=" << MAX_SEQ << ", "
      // << "time_out=" << time_out << ", "
       //<< "procesing_time=" << processing_time << ", "
      // << "transmission_time=" << transmission_time << ", "
       //<< "duplication_time=" << duplication_time << ", "
      // << "error_time=" << error_time << ", "
      // << "loss_probability=" << loss_probability << endl;
}

void Node1::handleMessage(cMessage *msg)
{
    // if it is a coordinator message only then check if it is sender or receiver message
    // else focus on the protocol
    if (coordinator_message_checker(msg))
    {
        if (nodeType == SENDER)
        {
           // EV << "Coordinator message received and started processing the First frame" << endl;
            processing_frame(current_frame, next_frame_to_send, frame_expected, processing_time, buffer, alldata, message_error_codes);
        }
    }
    else
    {
        if (nodeType == RECEIVER)
        {
            // if the message is from the sender
            // check if the sent message is correct
            MyMessage_Base *myMsg = check_and_cast<MyMessage_Base *>(msg);
           // EV << "msg name: " << msg->getName() << endl;
            if (!myMsg)
            {
                EV << "Received message is not of type MyMessage_Base" << endl;
                delete msg; // Clean up the message if it's not the expected type
                return;
            }

            // if the message is an data
            if (myMsg->getM_Type() == DATA && strcmp(myMsg->getM_Payload(), "") > 0)
            {
                // deframe the message
                string message = string(myMsg->getM_Payload());
                EV << "Message Received : " << Deframing(message) << endl;
                // check if this frame is the one expected
                if (myMsg->getSeq_Num() == frame_expected)
                {
                    // first check if there is ack or nack that is being processed

                    if (is_processing)
                    {
                        // if there is an ack or nack being processed then store the last correct frame received
                        last_correct_frame_received = myMsg;
                       // EV << "correct frame received is not null : " << last_correct_frame_received->getM_Payload() << endl;
                    }
                    else
                    {
                        bool error = ErrorDetection(myMsg);

                        // process the ack or nack
                        processed_ack_or_nack = process_and_check_ack(frame_expected, error);
                    }
                }
            }

            // if the message is a self msg
            if (strcmp(msg->getName(), "selfMsg") == 0)
            {
                // generate the random number
                int random_number = int(uniform(0, 100));
                // if the last correct frame received is not null then send it
                if (processed_ack_or_nack != nullptr)
                {
                    // if it is an ack then send the ack
                    if (processed_ack_or_nack->getM_Type() == ACK)
                    {
                        // to send check if random_number >= loss_probability send
                        if (random_number >= loss_probability)
                        {
                            sendDelayed(processed_ack_or_nack, transmission_time, "out");

                            // send the ack
                           // EV << "Ack " << processed_ack_or_nack->getACK_Num() << " sent" << endl;
                            this->logEvent(SENT_ACK, (float)simTime().dbl(), zeroes, processed_ack_or_nack->getACK_Num(), emptyString, dummy, false);
                        }
                        else
                        {
                           // EV << "Ack" << processed_ack_or_nack->getACK_Num() << " lost" << endl;
                            this->logEvent(SENT_ACK, (float)simTime().dbl(), zeroes, processed_ack_or_nack->getACK_Num(), emptyString, dummy, true);
                        }
                    }
                    else
                    {
                        // to send check if random_number >= loss_probability send
                        if (random_number >= loss_probability)
                        {
                            sendDelayed(processed_ack_or_nack, transmission_time, "out");

                            // send the nack
                            //EV << "Nack " << processed_ack_or_nack->getNACK_Num() << " sent" << endl;
                            this->logEvent(SENT_NACK, (float)simTime().dbl(), zeroes, processed_ack_or_nack->getACK_Num(), emptyString, dummy, false);
                        }
                        else
                        {
                           // EV << "Nack" << processed_ack_or_nack->getACK_Num() << " lost" << endl;
                            this->logEvent(SENT_NACK, (float)simTime().dbl(), zeroes, processed_ack_or_nack->getACK_Num(), emptyString, dummy, true);
                        }
                    }
                    processed_ack_or_nack = nullptr; // Clear after sending
                    is_processing = false;           // Done processing

                    if (last_correct_frame_received != nullptr)
                    {
                        //EV << "Last correct frame received is not null : " << last_correct_frame_received->getM_Payload() << endl;
                        // if there is a last correct frame received then process it
                        bool error = ErrorDetection(last_correct_frame_received);
                        processed_ack_or_nack = process_and_check_ack(frame_expected, error);
                        last_correct_frame_received = nullptr;
                    }
                }
            }
        }
        else if (nodeType == SENDER)
        {

            // if the message is ack or nack then check if it is ack
            //EV << "sender got a msg: " << msg->getName() << endl;
            //EV << "next_frame_to_send_b: " << next_frame_to_send << endl;
            ///EV << "frame_expected_b: " << frame_expected << endl;
            //EV << "ack_expected_b: " << ack_expected << endl;
           // EV << "nbuffered_b: " << nbuffered << endl;
            MyMessage_Base *myMsg = check_and_cast<MyMessage_Base *>(msg);
            if (myMsg)
            {
                //EV << "Message Type :" << myMsg->getM_Type() << endl;
            }
            // Ack Arrival
            if (myMsg->getM_Type() == ACK)
            {
               // EV << "Ack " << myMsg->getACK_Num() << "received" << endl;
                // if it is in between ack_expected and next_frame_to_send
                // we inc the Window slide to it
                while (inBetween(ack_expected, myMsg->getACK_Num(), next_frame_to_send))
                {
                    nbuffered--;
                    // stop the timer
                    stop_timer(ack_expected);
                    // advance the Lower side of the window
                    ack_expected = inc(ack_expected);
                }
                //EV << "next_frame_to_send_a: " << next_frame_to_send << endl;
               // EV << "frame_expected_a: " << frame_expected << endl;
               // EV << "ack_expected_a: " << ack_expected << endl;
                //EV << "nbuffered_a: " << nbuffered << endl;
            }
            bitset<4> zeros;
            if (myMsg->getM_Type() == NACK)
            {
                //EV << "NACK " << myMsg->getACK_Num() << endl;
               this->logEvent(RCVD_NACK, (float)(simTime().dbl()), zeros, myMsg->getACK_Num(), emptyString, dummy, false);
            }
            // after processing time , send the message
            if (strcmp(msg->getName(), "selfMsg") == 0)
            {
                // if it is a self message then send the data message (it means that the message is
                // buffered and processed and it is time to send it)

                // start the timer
                start_timer(next_frame_to_send, time_out);

                // send the data message
                send_message(buffer[next_frame_to_send]);

                // increment the next frame to send and current frame and nbuffered
                next_frame_to_send = inc(next_frame_to_send);
                current_frame++;
                nbuffered++;

                //  make the old next frame to send = -1
                old_next_frame_to_send = -1;

                //EV << "next_frame_to_send_a: " << next_frame_to_send << endl;
               // EV << "frame_expected_a: " << frame_expected << endl;
                //EV << "ack_expected_a: " << ack_expected << endl;
               // EV << "nbuffered_a: " << nbuffered << endl;
            }

            // if timeout occurs
            if (strcmp(msg->getName(), "timer") == 0)
            {
               // EV << "time_out frame" << ack_expected;

                // set the next_frame_to_send to the start of the window
                next_frame_to_send = ack_expected;

                // send the rest of the frames with errors
                for (int i = 0; i < nbuffered; i++)
                {
                    // stop timer of all frames in buffer
                    stop_timer(next_frame_to_send);
                    next_frame_to_send = inc(next_frame_to_send);
                }

                // set it again to the start of the window
                next_frame_to_send = ack_expected;

                this->logEvent(TIMEOUT, (float)(simTime().dbl()), zeros, ack_expected, emptyString, dummy, false);

                // process the frame again
                scheduleAt(simTime() + processing_time, new MyMessage_Base("selftimeout"));
            }

            if (strcmp(msg->getName(), "selftimeout") == 0)
            {
                // if it is a self message then send the data message (it means that the message is
                // buffered and processed and it is time to send it)

                // start the timer
                start_timer(next_frame_to_send, time_out);

                bitset<4> zeros;
                string deframe_payload = Deframing(buffer[next_frame_to_send]->msg->getM_Payload());
                bitset<8> trail_byte = trailer_byte(buffer[next_frame_to_send]->msg->getM_Payload());

                // if time_out_buffer_count ==0
                /// send message without errors
                if (timeout_buffer_count == 0)
                {
                    // send without errors
                    sendDelayed(buffer[next_frame_to_send]->msg->dup(), transmission_time, "out");
                    this->logEvent(SENT_FRAME, (float)(simTime().dbl()), zeros, next_frame_to_send, deframe_payload, trail_byte, false);
                }
                else
                {
                    // send the data message
                    send_message(buffer[next_frame_to_send]);
                }

                // increment the next frame to send and timeout_buffer_count
                next_frame_to_send = inc(next_frame_to_send);
                timeout_buffer_count++;

                // I sent the whole window
                if (timeout_buffer_count == nbuffered)
                {
                    timeout_buffer_count = 0;
                }
                else
                {
                    // simulate the next frame
                    scheduleAt(simTime() + processing_time, new MyMessage_Base("selftimeout"));
                }

                //EV << "next_frame_to_send_a: " << next_frame_to_send << endl;
                //EV << "frame_expected_a: " << frame_expected << endl;
               // EV << "ack_expected_a: " << ack_expected << endl;
               // EV << "nbuffered_a: " << nbuffered << endl;
            }

            // if nbuffered less than max_seq & current_frame index < alldata size
            // and also if there is a change in next_frame_to_send to avoid sending the same frame
            // process another frame
            if (nbuffered < MAX_SEQ && current_frame < alldata.size() && old_next_frame_to_send != next_frame_to_send)
                processing_frame(current_frame, next_frame_to_send, frame_expected, processing_time, buffer, alldata, message_error_codes);
        }
    }
}
