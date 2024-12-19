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

Define_Module(Node1);

int Node1::inc(int seq_nr)
{

    return (seq_nr + 1) % (MAX_SEQ + 1);
}

bool Node1::coordinator_message_checker(cMessage *msg)
{
    // if the message is from the coordinator and the message is sender then this node
    // is the sender else if the message is receiver then this node is the receiver
    // else it is not from the coordinator
    if (strcmp(msg->getName(), "sender") == 0)
    {
        nodeType = SENDER;
        // ReadFile(); // Read the input file
        EV << "Node1 set as SENDER with " << alldata.size() << " messages." << endl;

        return true;
    }

    else if (strcmp(msg->getName(), "receiver") == 0)
    {
        nodeType = RECEIVER;
        EV << "receiver" << endl;
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

MyMessage_Base *Node1::process_and_check_ack(int frame_expected, bool error)
{
    MyMessage_Base *msg = new MyMessage_Base("");
    if (error)
    {
        msg->setM_Type(NACK);
    }
    else
    {
        msg->setM_Type(ACK);
    }
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

    // is_processing = true
    is_processing = true;
    scheduleAt(simTime() + processing_time, new MyMessage_Base("selfMsg"));
    return msg;
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
void Node1::initialize()
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
    is_processing = false;

    buffer = new MyMessage_Base *[MAX_SEQ + 1];
}

void Node1::handleMessage(cMessage *msg)
{
    if (!coordinator_message_checker(msg))
    {

        if (nodeType == SENDER)
        {
        }
        else if (nodeType == RECEIVER)
        {

            EV << "receiver got a msg: " << msg->getName() << endl;
            // if the ack or nack is processed
            if (strcmp(msg->getName(), "selfMsg") == 0)
            {
                if (processed_ack_or_nack != nullptr)
                {
                    EV << "Sending processed ack or nack with ack number" <<processed_ack_or_nack->getACK_Num() << endl;
                    frame_expected = inc(frame_expected);
                    sendDelayed(processed_ack_or_nack, transmission_time, "out");
                    processed_ack_or_nack = nullptr; // Clear after sending
                    is_processing = false;           // Done processing

                    // check if there are messages waiting to be processed
                    // process the First message in the waiting list and remove it from the list
                    if (waited_messages.size() > 0)
                    {
                        // send the ack
                        bool error = ErrorDetection(waited_messages[0]);
                        // process the ack
                        processed_ack_or_nack = process_and_check_ack(frame_expected, error);
                        EV << "Processing Ack for frame " << frame_expected << endl;
                        waited_messages.erase(waited_messages.begin());
                    }
                }
                else
                {
                    EV << "Error: processed_ack_or_nack is nullptr" << endl;
                }
                delete msg;
                return;
            }
            // check if the sent message is correct
            MyMessage_Base *myMsg = check_and_cast<MyMessage_Base *>(msg);
            EV << "msg name: " << myMsg->getM_Payload() << endl;
            if (!myMsg)
            {
                EV << "Received message is not of type MyMessage_Base" << endl;
                delete msg; // Clean up the message if it's not the expected type
                return;
            }
            // if the message is a data message
            if (myMsg->getM_Type() == DATA)
            {
                // First , check if I am currently processing a message
                // add the received message to the waiting list and
                if (is_processing)
                {
                    waited_messages.push_back(myMsg);
                }
                else
                {
                    // send the ack
                    bool error = ErrorDetection(myMsg);
                    // process the ack
                    processed_ack_or_nack = process_and_check_ack(frame_expected, error);
                    EV << "Processing Ack for frame " << frame_expected << endl;
                }
            }
        }
    }
}
