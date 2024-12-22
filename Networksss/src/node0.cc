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

#include "node0.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem> // C++17 or later
#include <algorithm>

Define_Module(Node0);
using namespace std;

string Node0::Framing(const string &data)
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

string Node0::Deframing(const string &data)
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

bool Node0::ErrorDetection(MyMessage_Base *msg)
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

void Node0::ReadFile()
{
    // Open the file
    ifstream infile("Textfiles/input0.txt");
    if (!infile.is_open())
    {
        cerr << "Error opening file: Textfiles/input0.txt" << endl;
        return;
    }

    // Read the file line by line
    string line;
    while (getline(infile, line))
    {
        if (!line.empty() && !std::all_of(line.begin(), line.end(), ::isspace)){
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

bool Node0 ::coordinator_message_checker(cMessage *msg)
{
    // if the message is from the coordinator and the message is sender then this node
    // is the sender else if the message is receiver then this node is the receiver
    // else it is not from the coordinator
    if (strcmp(msg->getName(), "sender") == 0)
    {
        nodeType = SENDER;
        ReadFile(); // Read the input file
        EV << "Node0 set as SENDER with " << alldata.size() << " messages." << endl;

        return true;
    }

    else if (strcmp(msg->getName(), "receiver") == 0)
    {
        nodeType = RECEIVER;
        return true;
    }

    else
    {
        return false;
    }
}
// circulary increment the sequence number
int Node0::inc(int seq_nr)
{
    return (seq_nr + 1) % (MAX_SEQ + 1);
}

// check if the frame is in between the window

bool Node0::inBetween(int seq_nra, int seq_nrb, int seq_nrc)
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

bitset<8> Node0::trailer_byte(string data)
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

void Node0::message_construction(int frame_nr, int next_frame_to_send, int frame_expected, message_collection **&buffer, vector<string> &alldata, vector<bitset<4>> &message_error_code)
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

    // increment the nbuffer and add the msg to the buffer
    nbuffered++;
    buffer[next_frame_to_send]->msg = nmsg;

    // set the error_code of the message
    buffer[next_frame_to_send]->error_code = message_error_code[frame_nr];

    // set the timer of the message to Null
    buffer[next_frame_to_send]->timer = nullptr;

    EV << "Message Constructed" << endl;
}

void Node0::processing_frame(int frame_nr, int next_frame_to_send, int frame_expected, double processing_time, message_collection **&buffer, vector<string> &alldata, vector<bitset<4>> &message_error_code)
{
    // construct message and add it to buffer
    message_construction(frame_nr, next_frame_to_send, frame_expected, buffer, alldata, message_error_code);

    // Schedule the message
    scheduleAt(simTime() + processing_time, new MyMessage_Base("selfMsg"));

    EV << "processing frame " << buffer[next_frame_to_send]->msg->getSeq_Num() << endl
       << "Payload: " << buffer[next_frame_to_send]->msg->getM_Payload() << endl
       << " " << "Trailer: " << buffer[next_frame_to_send]->msg->getM_Trailer().to_ulong() << endl;
}

void Node0::send_ack(int frame_nr, int frame_expected, bool error)
{
    MyMessage_Base *msg;
    string dummy = "";
    if (error)
    {
        msg = new MyMessage_Base("NACK", NACK);
        msg->setM_Type(NACK);
        dummy = "NACK";
    }
    else
    {
        msg = new MyMessage_Base("ACK", ACK);
        msg->setM_Type(ACK);
        dummy = "ACK";
    }
    // Set the frame number
    msg->setSeq_Num(frame_nr);

    // Set the ack expected
    msg->setACK_Num((frame_expected + MAX_SEQ) % (MAX_SEQ + 1));

    // Set the Nack expected
    msg->setNACK_Num((frame_expected + MAX_SEQ) % (MAX_SEQ + 1));

    // Set the payload
    msg->setM_Payload(dummy.c_str());

    // Set the trailer
    msg->setM_Trailer(trailer_byte(dummy));
}

void Node0::start_timer(int seq_nr, int time_out)
{
    // create timer
    MyMessage_Base *timer = new MyMessage_Base("timer");

    // set the timer message in the buffer
    buffer[seq_nr]->timer = timer;

    // start the timer
    scheduleAt(simTime() + time_out, timer);
}

void Node0::stop_timer(int seq_nr)
{
    // stop the timer
    cancelEvent(buffer[seq_nr]->timer);

    // set the timer to null
    buffer[seq_nr]->timer = nullptr;
}

void Node0::message_manipulation(message_collection *&msg_to_be_sent)
{
    const char *pl = msg_to_be_sent->msg->getM_Payload();
    string str = pl;
    // Access the second character and modify its fourth bit
    unsigned char firstChar = str[1];
    firstChar ^= (1 << 3);
    str[1] = firstChar;
    EV << " Adding Error by changing the Fourth bit of second character in the payload (the first character after the start flag)" << endl;
    pl = str.c_str();
    msg_to_be_sent->msg->setM_Payload(pl);
}

void Node0::send_message(message_collection *msg_to_be_sent)
{
    // first define the total sending time
    double total_time = transmission_time;
    double total_time_duplicate = 0;
    MyMessage_Base *duplicated_msg = nullptr;

    // if there is loss , ignore the messages
    if (msg_to_be_sent->error_code[2] == 1)
    {
        EV << "This message will be lost with frame number : " << msg_to_be_sent->msg->getSeq_Num() << endl;
        return;
    }

    else
    {
        // classify errors based on error_code
        // check if there is modification (bit 3)
        if (msg_to_be_sent->error_code[3] == 1)
        {
            EV << "This message will be modified at second character fourth bit" << endl;
            // modify the message
            message_manipulation(msg_to_be_sent);
        }

        // check if there is delay error
        if (msg_to_be_sent->error_code[0] == 1)
        {
            // if there is delay error, add delay to the sending time
            EV << "This message will be delayed by =" << error_time << endl;
            total_time += error_time;
        }

        // check if there is duplication delay
        if (msg_to_be_sent->error_code[1] == 1)
        {
            EV << "This message will be duplicated with duplication time =" << duplication_time << endl;
            // if there is duplication delay, create the duplicated Message
            duplicated_msg = msg_to_be_sent->msg->dup();

            // update the total sending time
            total_time_duplicate = total_time + duplication_time;
        }

        // send the message
        sendDelayed(msg_to_be_sent->msg->dup(), total_time, "out");
        EV << "Total Transmission time for this message :" << total_time << endl;

        // if there is duplicate send it
        if (duplicated_msg)
        {

            sendDelayed(duplicated_msg->dup(), total_time_duplicate, "out");
            EV << "Total Transmission time for the duplicate message :" << total_time_duplicate << endl;
        }
    }
}

void Node0::initialize()
{

    // Intialization of Needed Parameters
    next_frame_to_send = 0;
    frame_expected = 0;
    nbuffered = 0;
    ack_expected = 0;
    MAX_SEQ = getParentModule()->par("WS");
    time_out = getParentModule()->par("TO");
    processing_time = double(getParentModule()->par("PT"));
    transmission_time = double(getParentModule()->par("TD"));
    duplication_time = double(getParentModule()->par("DD"));
    error_time = double(getParentModule()->par("ED"));
    loss_probability = double(getParentModule()->par("LP"));

    buffer = new message_collection *[MAX_SEQ + 1];

    // Allocate memory for each element in the buffer
    for (int i = 0; i <= MAX_SEQ; ++i)
    {
        buffer[i] = new message_collection(); // Allocate memory
        buffer[i]->msg = nullptr;             // Initialize msg to nullptr
        buffer[i]->timer = nullptr;           // Initialize timer to nullptr
    }

    EV << "Node initialized with parameters: "
       << "MAX_SEQ=" << MAX_SEQ << ", "
       << "time_out=" << time_out << ", "
       << "procesing_time=" << processing_time << ", "
       << "transmission_time=" << transmission_time << ", "
       << "duplication_time=" << duplication_time << ", "
       << "error_time=" << error_time << ", "
       << "loss_probability=" << loss_probability << endl;
}

void Node0::handleMessage(cMessage *msg)
{
    // TODO - Generated method body
    // if it is a coordinator message only then check if it is sender or receiver message
    // else focus on the protocol
    if (coordinator_message_checker(msg))
    {
        EV << "Coordinator message received and started processing the First frame" << endl;
        if (nodeType == SENDER)
                    processing_frame(current_frame, next_frame_to_send, frame_expected, processing_time, buffer, alldata, message_error_codes);
            }
            else
            {

                if (nodeType == RECEIVER)
                {
                    MyMessage_Base *myMsg = check_and_cast<MyMessage_Base *>(msg);
                    // if the message is a data message
                    if (myMsg->getM_Type() == DATA)
                    {
                        // send the ack
                        bool error = ErrorDetection(myMsg);
                        send_ack(myMsg->getSeq_Num(), frame_expected, error);
                    }
                }
                else if (nodeType == SENDER)
                {

                    // if the message is ack or nack then check if it is ack
                    EV << "sender got a msg: " << msg->getName() << endl;
                    EV << "next_frame_to_send_b: " << next_frame_to_send << endl;
                    EV << "frame_expected_b: " << frame_expected << endl;
                    EV << "ack_expected_b: " << ack_expected << endl;
                    EV << "nbuffered_b: " << nbuffered << endl;
                    MyMessage_Base *myMsg = check_and_cast<MyMessage_Base *>(msg);
                    if (myMsg)
                    {
                        EV << "Message Type :" << myMsg->getM_Type() << endl;
                    }
                    // Ack Arrival
                    if (myMsg->getM_Type() == ACK)
                    {
                        EV << "Ack " << myMsg->getACK_Num() << "received" << endl;
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
                        EV << "next_frame_to_send_a: " << next_frame_to_send << endl;
                        EV << "frame_expected_a: " << frame_expected << endl;
                        EV << "ack_expected_a: " << ack_expected << endl;
                        EV << "nbuffered_a: " << nbuffered << endl;
                    }
                    if (myMsg->getM_Type()== NACK){
                        EV<<"NACK " << myMsg->getACK_Num() << endl;
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

                        // increment the next frame to send and current frame
                        next_frame_to_send = inc(next_frame_to_send);
                        current_frame++;

                        //  make the old next frame to send = -1
                        old_next_frame_to_send = -1;

                        EV << "next_frame_to_send_a: " << next_frame_to_send << endl;
                        EV << "frame_expected_a: " << frame_expected << endl;
                        EV << "ack_expected_a: " << ack_expected << endl;
                        EV << "nbuffered_a: " << nbuffered << endl;
                    }

                    // if timeout occurs
                    if (strcmp(msg->getName(), "timer") == 0)
                    {
                        EV << "time_out frame" << ack_expected;

                        // send all of the outstanding frames
                        // set the next_frame_to_send to the start of the window
                        next_frame_to_send = ack_expected;

                        // send the first frame without any errors and increment the next frame to send and reset the timer
                        sendDelayed(buffer[next_frame_to_send]->msg->dup(), transmission_time, "out");
                        stop_timer(next_frame_to_send);
                        start_timer(next_frame_to_send, time_out);
                        next_frame_to_send = inc(next_frame_to_send);

                        // send the rest of the frames with errors
                        for (int i = 1; i < nbuffered; i++)
                        {
                            // send the message with errors
                            send_message(buffer[next_frame_to_send]);
                            stop_timer(next_frame_to_send);
                            start_timer(next_frame_to_send, time_out);
                            next_frame_to_send = inc(next_frame_to_send);
                        }
                    }

                    // if nbuffered less than max_seq & current_frame index < alldata size
                    // and also if there is a change in next_frame_to_send to avoid sending the same frame
                    // process another frame
                    if (nbuffered < MAX_SEQ && current_frame < alldata.size() && old_next_frame_to_send != next_frame_to_send)
                        processing_frame(current_frame, next_frame_to_send, frame_expected, processing_time, buffer, alldata, message_error_codes);
                }
            }
}
