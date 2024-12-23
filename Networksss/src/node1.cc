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
    // Set the message type to ACK or NACK depending on the error
    // if it no error send ACK else send NACK
    // also in ACK increment the frame expected
    if (error)
    {
        msg->setM_Type(NACK);
    }
    else
    {
        frame_expected = inc(frame_expected);
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

    // Schedule the message to be sent after the processing time
    scheduleAt(simTime() + processing_time, new MyMessage_Base("selfMsg"));

    // return msg
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
void Node1::initialize()
{
    // Intialization of Needed Parameters
    nodeType = NEITHER;
    next_frame_to_send = 0;
    frame_expected = 0;
    nbuffered = 0;
    ack_expected = 0;
    current_frame = 0;
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

            // if the message is from the sender
            // check if the sent message is correct
            MyMessage_Base *myMsg = check_and_cast<MyMessage_Base *>(msg);
            EV << "msg name: " << Deframing(myMsg->getM_Payload()) << endl;
            if (!myMsg)
            {
                EV << "Received message is not of type MyMessage_Base" << endl;
                delete msg; // Clean up the message if it's not the expected type
                return;
            }

            // if the message is an data
            if (myMsg->getM_Type() == DATA)
            {
                // check if this frame is the one expected
                if (myMsg->getSeq_Num() == frame_expected)
                {
                    // first check if there is ack or nack that is being processed

                    if (is_processing)
                    {
                        // if there is an ack or nack being processed then store the last correct frame received
                        last_correct_frame_received = myMsg;
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
                // if the last correct frame received is not null then send it
                if (processed_ack_or_nack != nullptr)
                {
                    // if it is an ack then send the ack
                    if (processed_ack_or_nack->getM_Type() == ACK)
                    {
                        // send the ack
                        EV << "Ack " << processed_ack_or_nack->getACK_Num() << " sent";
                        sendDelayed(processed_ack_or_nack, transmission_time, "out");
                    }
                    else
                    {
                        // send the nack
                        EV << "Nack " << processed_ack_or_nack->getNACK_Num() << " sent";
                        sendDelayed(processed_ack_or_nack, transmission_time, "out");
                    }
                    processed_ack_or_nack = nullptr; // Clear after sending
                    is_processing = false;           // Done processing

                    if (last_correct_frame_received)
                    {
                        // if there is a last correct frame received then process it
                        bool error = ErrorDetection(last_correct_frame_received);
                        processed_ack_or_nack = process_and_check_ack(frame_expected, error);
                        last_correct_frame_received = nullptr;
                    }
                }
            }
        }
    }
}
