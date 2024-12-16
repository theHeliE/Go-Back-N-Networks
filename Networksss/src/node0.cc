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
    string s = std::string(msg->getM_Payload());
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

        EV << "Code : " << message_error_code << endl;
        EV << "Message: " << data << endl;
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

void Node0::send_frame(int frame_nr, int frame_expected, vector<string> buffer, bitset<4> error)
{
    // Create a new message
    MyMessage_Base *msg = new MyMessage_Base("DATA", DATA);

    // Set the frame number
    msg->setSeq_Num(frame_nr);

    // Set the ack expected
    msg->setACK_Num((frame_expected + MAX_SEQ) % (MAX_SEQ + 1));

    // Set the Nack expected
    msg->setNACK_Num((frame_expected + MAX_SEQ) % (MAX_SEQ + 1));

    // Set the payload
    msg->setM_Payload(buffer[frame_nr]);

    // Set the trailer
    msg->setM_Trailer(trailer_byte(buffer[frame_nr]));

    // set the Type
    msg->setM_Type(DATA);
}

void Node0::initialize()
{

    // Intialization of Needed Parameters
    next_frame_to_send = 0;
    frame_expected = 0;
    nbuffered = 0;
    MAX_SEQ = getParentModule()->par("WS");
    time_out = getParentModule()->par("TO");
    procesing_time = double(getParentModule()->par("PT"));
    transmission_time = double(getParentModule()->par("TD"));
    duplication_time = double(getParentModule()->par("DD"));
    error_time = double(getParentModule()->par("ED"));
    loss_probability = double(getParentModule()->par("LP"));

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
        EV << "Coordinator message received" << endl;
    }
}
