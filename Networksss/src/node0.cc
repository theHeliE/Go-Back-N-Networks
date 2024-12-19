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

void Node0::message_construction(int frame_nr, int next_frame_to_send, int frame_expected, MyMessage_Base **&buffer, vector<string> &alldata)
{
    // Create a new message
    MyMessage_Base *msg = new MyMessage_Base("");

    // Set the frame number
    msg->setSeq_Num(next_frame_to_send);

    // Set the ack expected
    msg->setACK_Num((frame_expected + MAX_SEQ) % (MAX_SEQ + 1));

    // Set the Nack expected
    msg->setNACK_Num((frame_expected + MAX_SEQ) % (MAX_SEQ + 1));

    // Set the payload
    string framedPayload = Framing(alldata[frame_nr]);
    msg->setM_Payload(framedPayload.c_str());

    // Set the trailer
    msg->setM_Trailer(trailer_byte(alldata[frame_nr]));

    // set the Type
    msg->setM_Type(DATA);

    buffer[next_frame_to_send] = msg;
}

void Node0::processing_frame(int frame_nr, int next_frame_to_send, int frame_expected, double processing_time, MyMessage_Base **&buffer, vector<string> &alldata)
{
    // construct message and add it to buffer
    message_construction(frame_nr, next_frame_to_send, frame_expected, buffer, alldata);

    // Schedule the message
    scheduleAt(simTime() + processing_time, new MyMessage_Base("selfMsg"));
    EV << "processing frame " << buffer[next_frame_to_send]->getSeq_Num() << "Payload: " << buffer[next_frame_to_send]->getM_Payload() << " " << "Trailer: " << buffer[next_frame_to_send]->getM_Trailer().to_ulong() << endl;
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

    buffer = new MyMessage_Base *[MAX_SEQ];
    EV << "Node initialized with parameters: "
       << "MAX_SEQ=" << MAX_SEQ << ", "
       << "time_out=" << time_out << ", "
       << "procesing_time=" << processing_time << ", "
       << "transmission_time=" << transmission_time << ", "
       << "duplication_time=" << duplication_time << ", "
       << "error_time=" << error_time << ", "
       << "loss_probability=" << loss_probability << endl;

    // while I am waiting for the coordinator message , wait
    // while (nodeType == NEITHER);

    // if (nodeType == SENDER)
    //{
    // while (alldata.size() > 0)
    //{
    // }
    //}
}

void Node0::handleMessage(cMessage *msg)
{
    // TODO - Generated method body
    // if it is a coordinator message only then check if it is sender or receiver message
    // else focus on the protocol
    if (coordinator_message_checker(msg))
    {
        EV << "Coordinator message received and started processing the First frame" << endl;
        processing_frame(current_frame, next_frame_to_send, frame_expected, processing_time, buffer, alldata);
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
            EV << "next_frame_to_send: " << next_frame_to_send << endl;
            EV << "frame_expected: " << frame_expected << endl;
            EV << "ack_expected: " << ack_expected << endl;
            EV << "nbuffered: " << nbuffered << endl;
            MyMessage_Base *myMsg = check_and_cast<MyMessage_Base *>(msg);
            if (myMsg->getM_Type() == ACK)
            {
                EV << "Ack " << myMsg->getACK_Num() << "received";
                // if it is in between ack_expected and next_frame_to_send
                // we inc the Window slide to it
                while (inBetween(ack_expected, myMsg->getACK_Num(), next_frame_to_send))
                {
                    nbuffered--;
                    // advance the Lower side of the window
                    ack_expected = inc(ack_expected);
                }
            }
            // after processing time , send the message
            if (strcmp(msg->getName(), "selfMsg") == 0)
            {
                // if it is a self message then send the data message
                sendDelayed(buffer[next_frame_to_send], transmission_time, "out");
                nbuffered++;
                next_frame_to_send = inc(next_frame_to_send);
                current_frame++;
            }
            // if nbuffered less than max_seq process another frame
            if (nbuffered < MAX_SEQ)
                processing_frame(current_frame, next_frame_to_send, frame_expected, processing_time, buffer, alldata);
        }
    }
}
